/* PS5 host: loads the game image below 4 GiB, maps the GBA regions and serves
 * the guest's bridge calls with the platform layer of the PS5 app base
 * (Vulkan presentation, audio and controllers). */
#include <fcntl.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <ucontext.h>
#include <unistd.h>
#include <dirent.h>

#include "platform/platform.h"
#include "platform/ps5_crash.h"
#include "platform/update.h"
#include "platform/vk/fg_bridge.h"
#include "platform/vk/vk_present.h"
#include <ps5platform/exec.h>

#include "bridge.h"

#define DATA_ROOT "/data/ReKHCom"
#define TITLE_ID "PPSA98511"

int sceSystemServiceLoadExec(const char *path, const char **arguments);
int ps5_klog_capture_stderr(const char *prefix); /* the platform layer: stderr lines go to klog (port 3232) */

/* ---- log ------------------------------------------------------------------ */

static FILE *sLog;
static FILE *sCrashFile;      /* only the report of a crash, a file of its own: crash-07_17pm-16.10.2026.log */

static void Log(const char *fmt, ...)
{
    char line[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    size_t n = strlen(line);
    while (n && (line[n - 1] == '\n' || line[n - 1] == '\r'))
        line[--n] = 0;
    fprintf(stderr, "[ReKHCoM] %s\n", line);
    if (sLog)
    {
        fprintf(sLog, "%s\n", line);
        fflush(sLog);
    }
    if (sCrashFile)
    {
        fprintf(sCrashFile, "%s\n", line);
        fflush(sCrashFile);
    }
}

/* ---- frame interpolation is not used: the presenter links against these ---- */

FgInterpolator *ssfg_create(PFN_vkVoidFunction (*get)(VkInstance, const char *), VkInstance i, VkPhysicalDevice g, VkDevice d,
                            uint32_t w, uint32_t h, VkFormat f, char *error, size_t error_size)
{
    (void)get, (void)i, (void)g, (void)d, (void)w, (void)h, (void)f;
    snprintf(error, error_size, "not in this build");
    return NULL;
}
void ssfg_destroy(FgInterpolator *fg) { (void)fg; }
void ssfg_prepare(FgInterpolator *fg, VkCommandBuffer cmd) { (void)fg, (void)cmd; }
VkImage ssfg_frame_image(FgInterpolator *fg) { (void)fg; return VK_NULL_HANDLE; }
VkImageView ssfg_frame_view(FgInterpolator *fg) { (void)fg; return VK_NULL_HANDLE; }
bool ssfg_record(FgInterpolator *fg, VkCommandBuffer cmd, bool reset) { (void)fg, (void)cmd, (void)reset; return false; }
VkImageView ssfg_output_view(FgInterpolator *fg) { (void)fg; return VK_NULL_HANDLE; }
bool ssfg_record_n(FgInterpolator *fg, VkCommandBuffer cmd, bool reset, uint32_t n) { (void)fg, (void)cmd, (void)reset, (void)n; return false; }
VkImageView ssfg_generated_view(FgInterpolator *fg, uint32_t i) { (void)fg, (void)i; return VK_NULL_HANDLE; }
void ssfg_advance(FgInterpolator *fg) { (void)fg; }
FgPacing *ssfg_pacing_create(void) { return NULL; }
void ssfg_pacing_destroy(FgPacing *p) { (void)p; }
FgDecision ssfg_pacing_frame(FgPacing *p, uint32_t a, uint32_t b, double c, double d, double e, double f, bool g, uint32_t h)
{
    (void)p, (void)a, (void)b, (void)c, (void)d, (void)e, (void)f, (void)g, (void)h;
    FgDecision none;
    memset(&none, 0, sizeof(none));
    return none;
}
const char *ssfg_pacing_state_name(int s) { (void)s; return ""; }
const char *ssfg_pacing_event(FgPacing *p) { (void)p; return NULL; }

/* The link wraps fclose and fflush (the platform layer's memory streams); the originals are kept under these names. */
int ps5_memstream_fclose(FILE *f);
int ps5_memstream_fflush(FILE *f);
int __wrap_fclose(FILE *f) { return ps5_memstream_fclose(f); }
int __wrap_fflush(FILE *f) { return ps5_memstream_fflush(f); }

/* ---- bridge: process, files ------------------------------------------------ */

static void H_log(const char *s) { Log("%s", s); }

static void H_exit(int code)
{
    Log("the game ended (%d)", code);
    plat_audio_close();
    if (sLog)
        fflush(sLog);
    const char *none[1] = {NULL};
    sceSystemServiceLoadExec("exit", none);
    _exit(code);
}

static uint64_t H_ticks_us(void) { return plat_ticks_us(); }
static void H_sleep_us(uint32_t us) { plat_sleep_us(us); }

static int H_getenv(const char *name, char *out, int cap)
{
    const char *v = getenv(name);
    if (!v)
        return -1;
    snprintf(out, (size_t)cap, "%s", v);
    return (int)strlen(out);
}

static FILE *sFiles[64];
static DIR *sDirs[16];

static int H_fopen(const char *path, int mode)
{
    static const char *const modes[] = {"rb", "wb", "ab", "r+b"};
    for (int i = 0; i < 64; ++i)
        if (!sFiles[i])
        {
            sFiles[i] = fopen(path, modes[mode & 3]);
            return sFiles[i] ? i : -1;
        }
    return -1;
}
static int H_fread(int h, void *buf, uint32_t n) { return (int)fread(buf, 1, n, sFiles[h]); }
static int H_fwrite(int h, const void *buf, uint32_t n) { return (int)fwrite(buf, 1, n, sFiles[h]); }
static int H_fseek(int h, int32_t off, int whence) { return fseek(sFiles[h], off, whence); }
static int H_ftell(int h) { return (int)ftell(sFiles[h]); }
static int H_fclose(int h)
{
    int r = fclose(sFiles[h]);
    sFiles[h] = NULL;
    return r;
}
static int H_fflush(int h) { return fflush(sFiles[h]); }
static int H_stat(const char *p)
{
    struct stat s;
    return stat(p, &s) ? 0 : S_ISDIR(s.st_mode) ? 2 : 1;
}
static int H_remove(const char *p) { return remove(p); }
static int H_rename(const char *a, const char *b) { return rename(a, b); }
static int H_mkdir(const char *p) { return mkdir(p, 0777); }
static int H_dir_open(const char *p)
{
    for (int i = 0; i < 16; ++i)
        if (!sDirs[i])
        {
            sDirs[i] = opendir(p);
            return sDirs[i] ? i : -1;
        }
    return -1;
}
static int H_dir_next(int d, char *name, int cap)
{
    struct dirent *e;
    while ((e = readdir(sDirs[d])))
    {
        if (e->d_name[0] == '.')
            continue;
        snprintf(name, (size_t)cap, "%s", e->d_name);
        return 1;
    }
    return 0;
}
static void H_dir_close(int d)
{
    closedir(sDirs[d]);
    sDirs[d] = NULL;
}

/* ---- bridge: video ---------------------------------------------------------- */

static void H_gfx_size(int *w, int *h)
{
    *w = plat_width();
    *h = plat_height();
}
static void H_gfx_begin(uint32_t clear) { plat_begin_frame(clear); }
/* A box the app draws over the game for five seconds (assets/<name>.rgba), fading out at the end. The game asks by name. */
typedef struct
{
    int tex, w, h;
} Notice;
static Notice LoadNotice(const char *name);
static Notice sToast = {-1, 0, 0};
static uint64_t sToastUntil;

static void H_notify(const char *name)
{
    if (sToast.tex >= 0)
        vkp_ui_texture_free(sToast.tex);
    sToast = LoadNotice(name);
    sToastUntil = plat_ticks_us() + 5000000u;
}

static void DrawToast(void)
{
    uint64_t now = plat_ticks_us();
    if (sToast.tex < 0)
        return;
    if (now >= sToastUntil)
    {
        vkp_ui_texture_free(sToast.tex);
        sToast.tex = -1;
        return;
    }
    {
        uint64_t left = sToastUntil - now;
        uint32_t alpha = left < 400000u ? (uint32_t)(left * 255u / 400000u) : 255u;
        uint32_t tint = (alpha << 24) | 0xFFFFFFu;
        float k = plat_width() / 1920.0f, dw = (float)sToast.w * k, dh = (float)sToast.h * k;
        float x0 = (plat_width() - dw) / 2, y0 = plat_height() - dh - 90.0f * k;
        VkpUiVertex q[6] = {{x0, y0, 0, 0, tint},      {x0 + dw, y0, 1, 0, tint},      {x0, y0 + dh, 0, 1, tint},
                            {x0 + dw, y0, 1, 0, tint}, {x0, y0 + dh, 0, 1, tint},      {x0 + dw, y0 + dh, 1, 1, tint}};
        vkp_ui_clip(0, 0, 0, 0);
        vkp_ui_blend(true);
        vkp_ui_draw(sToast.tex, q, 6);
    }
}

/* Updates from GitHub: found at start, installed by itself a second after the app opened; the new version runs from
   the next start. /data/ReKHCom/no-update turns it off. */
static void UpdateTick(void)
{
    static uint64_t opened;
    static bool tried, told, started;
    int state = update_state();
    uint64_t now = plat_ticks_us();

    if (!opened)
        opened = now;
    /* Not before the game runs: the GBA address space and the image are mapped at fixed addresses at start, and the
       network libraries' threads and heap must not take them first. */
    if (!started)
    {
        started = true;
        if (access(DATA_ROOT "/no-update", F_OK) != 0)
            update_check();
        else
            Log("updates are off (" DATA_ROOT "/no-update)");
    }
    if (!tried && state == UPDATE_AVAILABLE && now - opened >= 1000000u)
    {
        tried = true;
        H_notify("toast-update-found");
        update_install();
    }
    else if (!told && state == UPDATE_INSTALLED)
    {
        told = true;
        H_notify("toast-update-done");
    }
}

static void H_gfx_end(void)
{
    UpdateTick();
    DrawToast();
    plat_end_frame();
}
static int H_tex_create(const void *rgba, int w, int h, int smooth)
{
    return vkp_ui_texture_create((const uint8_t *)rgba, w, h, smooth != 0, false, true);
}
static void H_tex_update(int id, const void *rgba) { vkp_ui_texture_update(id, (const uint8_t *)rgba); }
static void H_tex_free(int id) { vkp_ui_texture_free(id); }
static void H_gfx_draw(int id, const void *v, int count) { vkp_ui_draw(id, (const VkpUiVertex *)v, count); }
static void H_gfx_clip(int x, int y, int w, int h) { vkp_ui_clip(x, y, w, h); }
static void H_gfx_blend(int on) { vkp_ui_blend(on != 0); }

/* ---- bridge: audio and controllers ----------------------------------------- */

static int sGameRate = 48000; /* the rate the game opened the audio at */
static int H_audio_open(int rate) { sGameRate = rate; return plat_audio_open(rate) ? 0 : -1; }
static void GamePush(const int16_t *frames, uint32_t count) { plat_audio_push(frames, count); }

/* HD music: the host decodes the track the game names and mixes it into the game's blocks (shared/hd_music_host.h) */
#define HD_MUSIC_LOG Log
#include "hd_music_host.h"
/* the network link's socket (shared/udp_host.h): none on this host yet, the link stays the cable */
#include "udp_host.h"
static void H_audio_push(const int16_t *frames, uint32_t count) { HdMusicPush(frames, count, sGameRate, GamePush); }
/* HD movies: only the Android host plays them (bridge entry hd_movie); the game plays its own movie */
static int H_hd_movie(const char* movie, const char* subs1, const char* subs2, const char* font) {
    (void)movie, (void)subs1, (void)subs2, (void)font;
    return 0;
}
static uint32_t H_audio_queued(void) { return (uint32_t)plat_audio_queued_frames(); }
static void H_audio_clear(void) { plat_audio_clear(); }
static void H_audio_ratio(double ratio) { plat_audio_set_ratio(ratio); }

static bool sQuit;

static int H_pad_poll(HostPad *out, int count)
{
    PadState pads[HOSTPAD_MAX];
    bool quit = false;
    plat_poll(pads, &quit);
    if (quit)
        sQuit = true;
    int n = 0;
    for (int i = 0; i < HOSTPAD_MAX && i < count; ++i)
    {
        memset(&out[i], 0, sizeof(out[i]));
        if (!pads[i].connected)
            continue;
        out[i].connected = 1;
        out[i].buttons = pads[i].buttons;
        out[i].lx = pads[i].lx;
        out[i].ly = pads[i].ly;
        out[i].rx = pads[i].rx;
        out[i].ry = pads[i].ry;
        /* The triggers read 128 at rest on this driver (axis 0, not -32768): only what lies above the rest position counts,
         * and the digital L2 / R2 buttons count as fully pressed. */
        out[i].l2 = (pads[i].buttons & BIT(BTN_L2)) ? 255 : (pads[i].l2 > 128 ? (uint8_t)((pads[i].l2 - 128) * 2) : 0);
        out[i].r2 = (pads[i].buttons & BIT(BTN_R2)) ? 255 : (pads[i].r2 > 128 ? (uint8_t)((pads[i].r2 - 128) * 2) : 0);
        ++n;
    }
    return n;
}
static int H_quit_requested(void) { return sQuit; }
static void H_set_title(const char *s) { (void)s; }
/* no touch screen and no background state here (bridge entries of the Android 64-bit host) */
static int H_touch_poll(HostTouch* fingers, int max) { (void)fingers; (void)max; return 0; }
static int H_app_state(void) { return 0; }

/* ---- guest memory ------------------------------------------------------------- */

static bool MapAt(uintptr_t address, size_t bytes, const char *what)
{
    struct ps5_exec_request request = {.bytes = bytes, .address = address, .anchor = 0, .flags = PS5_EXEC_AT};
    struct ps5_exec_region region;
    memset(&region, 0, sizeof(region));
    ps5_crash_step(what);
    int rc = ps5_exec_alloc(&request, &region);
    Log("memory: %s at 0x%08lx, %zu bytes: rc 0x%08x, base %p, %zu bytes mapped", what, (unsigned long)address, bytes, (unsigned)rc,
        region.base, region.bytes);
    if (rc != 0 || (uintptr_t)region.base != address)
    {
        Log("memory: %s at 0x%08lx (%zu bytes) failed (0x%08x)", what, (unsigned long)address, bytes, (unsigned)rc);
        return false;
    }
    size_t clear = bytes < ((size_t)8 << 20) ? bytes : ((size_t)8 << 20);
    memset(region.base, 0, clear);
    Log("memory: %s cleared (%zu bytes)", what, clear);
    return true;
}

/* ---- what the GBA does with stray pointers and divisions by zero ----------------------------------------------
 * On the cartridge a read through a null pointer lands in the BIOS and returns the open-bus value (the game does it in
 * places, marked "@bug ... NULL read"), sometimes the value is used as a pointer in turn, and the bus ignores the top
 * address bits. The desktop port handles it in a fault handler: the faulting instruction's address register is moved so
 * the access lands where the bus would send it, the instruction runs one step, and the register is put back. Integer
 * divisions by zero give 0 on the cartridge and trap on x86: quotient and remainder are zeroed and the instruction is
 * skipped. Only the ModRM / SIB bytes are decoded, to find the register. Where the registers sit in the signal context
 * is found at start with a probe, since the SDK's structure does not match the console's. */
#define GBA_LOW 0x02000000u
#define GBA_HIGH 0x0E010000u

static int sGpr[16]; /* slot of each register in the context, by x86 number: rax rcx rdx rbx rsp rbp rsi rdi r8-r15 */
static int sRip = -1, sFlags = -1;
static bool sContext;
static uint32_t sBiosArea, sGuestEnd;
static struct sigaction sOldSegv, sOldFpe, sOldTrap;
static struct
{
    bool active;
    int reg;
    uint64_t old, now;
} sPending;
static uint32_t sSeen[128];
static int sSeenCount;

#define SLOT(ctx, i) (((uint64_t *)(ctx))[i])
#define SENTINEL(n) (0xC0DE000000000000ull + 0x1010ull * (unsigned)((n) + 1))

static volatile uint64_t sProbeRip, sProbeRsp, sProbeRbp;

static void OnProbe(int sig, siginfo_t *info, void *context)
{
    const uint64_t *w = (const uint64_t *)context;
    (void)sig, (void)info;
    for (int r = 0; r < 16; ++r)
    {
        if (r == 4 || r == 5)
            continue;
        for (int i = 0; i < 160; ++i)
            if (w[i] == SENTINEL(r))
            {
                sGpr[r] = i;
                break;
            }
    }
    for (int i = 0; i < 160; ++i)
    {
        if (w[i] == sProbeRip && sRip < 0)
            sRip = i;
        if (w[i] == sProbeRsp && sGpr[4] < 0)
            sGpr[4] = i;
        if (w[i] == sProbeRbp && sGpr[5] < 0)
            sGpr[5] = i;
    }
    if (sRip >= 0)
        SLOT(context, sRip) += 2; /* past the ud2 */
}

static bool ProbeContext(void)
{
    struct sigaction sa, old;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = OnProbe;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    for (int i = 0; i < 16; ++i)
        sGpr[i] = -1;
    sRip = -1;
    sigaction(SIGILL, &sa, &old);
    __asm__ volatile(
        "movq %%rsp, %[rsp]\n\t"
        "movq %%rbp, %[rbp]\n\t"
        "leaq 1f(%%rip), %%rax\n\t"
        "movq %%rax, %[rip]\n\t"
        "movabsq $0xC0DE000000001010, %%rax\n\t"
        "movabsq $0xC0DE000000002020, %%rcx\n\t"
        "movabsq $0xC0DE000000003030, %%rdx\n\t"
        "movabsq $0xC0DE000000004040, %%rbx\n\t"
        "movabsq $0xC0DE000000007070, %%rsi\n\t"
        "movabsq $0xC0DE000000008080, %%rdi\n\t"
        "movabsq $0xC0DE000000009090, %%r8\n\t"
        "movabsq $0xC0DE00000000A0A0, %%r9\n\t"
        "movabsq $0xC0DE00000000B0B0, %%r10\n\t"
        "movabsq $0xC0DE00000000C0C0, %%r11\n\t"
        "movabsq $0xC0DE00000000D0D0, %%r12\n\t"
        "movabsq $0xC0DE00000000E0E0, %%r13\n\t"
        "movabsq $0xC0DE00000000F0F0, %%r14\n\t"
        "movabsq $0xC0DE000000010100, %%r15\n\t"
        "1: ud2\n\t"
        : [rsp] "=m"(sProbeRsp), [rbp] "=m"(sProbeRbp), [rip] "=m"(sProbeRip)
        :
        : "rax", "rcx", "rdx", "rbx", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "memory", "cc");
    sigaction(SIGILL, &old, NULL);
    if (sRip < 0 || sGpr[4] < 1)
        return false;
    sFlags = sGpr[4] - 1; /* rip, cs, rflags, rsp, ss */
    for (int r = 0; r < 16; ++r)
        if (sGpr[r] < 0)
            return false;
    return true;
}

/* Where the GBA bus sends address `a`: its region's mirror, or the BIOS stand-in for the BIOS and open-bus regions (and
   for writes to the ROM, which the cartridge ignores). */
static uint32_t BusTarget(uint32_t a, int write)
{
    uint32_t low = a & 0xFFFFu;
    uint32_t bios = sBiosArea + (low > 0xFFE0u ? low - 0x100u : low);
    uint32_t o;

    switch ((a >> 24) & 0xF)
    {
    case 0x2: return 0x02000000u | (a & 0x3FFFFu);
    case 0x3: return 0x03000000u | (a & 0x7FFFu);
    case 0x4: return (a & 0xFFFFFFu) < 0x400u ? 0x04000000u | (a & 0x3FFu) : bios;
    case 0x5: return 0x05000000u | (a & 0x3FFu);
    case 0x6:
        o = a & 0x1FFFFu;
        return 0x06000000u | (o >= 0x18000u ? o - 0x8000u : o);
    case 0x7: return 0x07000000u | (a & 0x3FFu);
    case 0x8: case 0x9: case 0xA: case 0xB: case 0xC: case 0xD: return write ? bios : 0x08000000u | (a & 0x1FFFFFFu);
    case 0xE: case 0xF: return 0x0E000000u | (a & 0xFFFFu);
    default: return bios;
    }
}

static int HasModRm1(uint8_t op)
{
    if (op < 0x40)
        return (op & 7) < 4;
    switch (op)
    {
    case 0x62: case 0x63: case 0x69: case 0x6B: case 0xC0: case 0xC1: case 0xC4: case 0xC5:
    case 0xC6: case 0xC7: case 0xD0: case 0xD1: case 0xD2: case 0xD3: case 0xF6: case 0xF7:
    case 0xFE: case 0xFF:
        return 1;
    }
    return (op >= 0x80 && op <= 0x8F) || (op >= 0xD8 && op <= 0xDF);
}

static int NoModRm2(uint8_t op)
{
    return (op >= 0x05 && op <= 0x0B) || op == 0x0E || (op >= 0x30 && op <= 0x37) || op == 0x77 || (op >= 0x80 && op <= 0x8F) ||
           (op >= 0xA0 && op <= 0xA2) || (op >= 0xA8 && op <= 0xAA) || op >= 0xC8;
}

/* The register to move (x86 number) and by how much, so the instruction's address shifts by `shift`. */
static int StrayRegister(const uint8_t *p, int64_t shift, int *reg, int64_t *delta, int *write)
{
    int rex = 0;
    uint8_t op;

    *write = 0;
    for (int i = 0; i < 15; ++i, ++p)
        if (*p != 0x66 && *p != 0x67 && *p != 0xF2 && *p != 0xF3 && *p != 0xF0 && *p != 0x2E && *p != 0x3E && *p != 0x26 &&
            *p != 0x36 && *p != 0x64 && *p != 0x65)
            break;
    if ((*p & 0xF0) == 0x40)
        rex = *p++;
    op = *p++;
    if ((op >= 0xA4 && op <= 0xA7) || (op >= 0xAA && op <= 0xAF))
    {
        *reg = (op == 0xAA || op == 0xAB || op == 0xAE || op == 0xAF) ? 7 : 6;
        *delta = shift;
        *write = op == 0xA4 || op == 0xA5 || op == 0xAA || op == 0xAB;
        return 1;
    }
    if (op == 0x0F)
    {
        op = *p++;
        if (op == 0x38 || op == 0x3A)
            ++p;
        else if (NoModRm2(op))
            return 0;
    }
    else
    {
        if (!HasModRm1(op))
            return 0;
        *write = op == 0x88 || op == 0x89 || op == 0xC6 || op == 0xC7 || (op < 0x40 && !(op & 2));
    }
    {
        uint8_t modrm = *p++;
        int mod = modrm >> 6, rm = modrm & 7, base = -1, index = -1, scale = 1;

        if (mod == 3)
            return 0;
        if (rm == 4)
        {
            uint8_t sib = *p++;
            scale = 1 << (sib >> 6);
            index = ((sib >> 3) & 7) | ((rex & 2) ? 8 : 0);
            base = (sib & 7) | ((rex & 1) ? 8 : 0);
            if (index == 4)
                index = -1;
            if ((sib & 7) == 5 && mod == 0)
                base = -1;
        }
        else if (rm == 5 && mod == 0)
        {
            return 0; /* relative to rip: never a stray pointer */
        }
        else
        {
            base = rm | ((rex & 1) ? 8 : 0);
        }
        if (base >= 0 && base != index)
        {
            *reg = base;
            *delta = shift;
            return 1;
        }
        if (index >= 0 && base < 0 && shift % scale == 0)
        {
            *reg = index;
            *delta = shift / scale;
            return 1;
        }
    }
    return 0;
}

/* The crash report: where the game was, with the registers read from the slots found by the probe and the game's own call
 * chain (its frames use rbp; each return address is a function the map file of that build names). */
static void ReportCrash(int sig, siginfo_t *info, void *context)
{
    static const char *const names[16] = {"rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi",
                                           "r8",  "r9",  "r10", "r11", "r12", "r13", "r14", "r15"};
    char regs[400];
    int n = 0;
    uint64_t rip = SLOT(context, sRip), frame = SLOT(context, sGpr[5]);

    {
        time_t now = time(NULL);
        struct tm tm;
        if (local_time((long long)now, &tm))
        {
            char path[256];
            int hour = tm.tm_hour % 12;
            snprintf(path, sizeof(path), DATA_ROOT "/logs/crash-%02d_%02d%s-%02d.%02d.%04d.log", hour ? hour : 12, tm.tm_min,
                     tm.tm_hour >= 12 ? "pm" : "am", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900);
            sCrashFile = fopen(path, "a");
            if (sCrashFile)
                fprintf(sCrashFile, "Crash on %02d.%02d.%04d at %02d:%02d:%02d %s\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900,
                        hour ? hour : 12, tm.tm_min, tm.tm_sec, tm.tm_hour >= 12 ? "pm" : "am");
        }
    }
    Log("CRASH: signal %d at address 0x%llx, rip 0x%llx, eflags 0x%llx", sig, (unsigned long long)(uintptr_t)info->si_addr,
        (unsigned long long)rip, (unsigned long long)SLOT(context, sFlags));
    for (int r = 0; r < 16; ++r)
        n += snprintf(regs + n, sizeof(regs) - (size_t)n, "%s %llx ", names[r], (unsigned long long)SLOT(context, sGpr[r]));
    Log("  %s", regs);
    for (int depth = 0; depth < 24 && frame >= PS5G_BASE && frame + 16 < sGuestEnd && (frame & 7) == 0; ++depth)
    {
        Log("  frame %d: return 0x%llx", depth, (unsigned long long)((uint64_t *)(uintptr_t)frame)[1]);
        uint64_t next = ((uint64_t *)(uintptr_t)frame)[0];
        if (next <= frame)
            break;
        frame = next;
    }
    if (sLog)
        fflush(sLog);
    if (sCrashFile)
    {
        fclose(sCrashFile);
        sCrashFile = NULL;
    }
}

static void Pass(const struct sigaction *old, int sig, siginfo_t *info, void *context)
{
    ReportCrash(sig, info, context);
    if ((old->sa_flags & SA_SIGINFO) && old->sa_sigaction)
        old->sa_sigaction(sig, info, context);
    else
    {
        signal(sig, SIG_DFL);
        raise(sig);
    }
}

static void OnSegv(int sig, siginfo_t *info, void *context)
{
    uint64_t fault = (uint64_t)(uintptr_t)info->si_addr, rip = SLOT(context, sRip);
    int reg, write;
    int64_t delta;

    /* Only code of the game, and only addresses the bus decodes: anything else is a real fault. */
    if (sPending.active || rip < PS5G_BASE || rip >= sGuestEnd || fault >= 0x100000000ull ||
        (fault >= GBA_LOW && fault < GBA_HIGH) || (fault >= PS5G_BASE && fault < sGuestEnd) ||
        (fault >= 0x10000 && fault < GBA_LOW))
    {
        Pass(&sOldSegv, sig, info, context);
        return;
    }
    {
        uint32_t target = BusTarget((uint32_t)fault, 0);
        const uint8_t *code = (const uint8_t *)(uintptr_t)rip;
        bool seen = false;

        if (!StrayRegister(code, (int64_t)target - (int64_t)fault, &reg, &delta, &write))
        {
            Pass(&sOldSegv, sig, info, context);
            return;
        }
        if (write)
            target = BusTarget((uint32_t)fault, 1);
        delta = (int64_t)target - (int64_t)fault;
        for (int i = 0; i < sSeenCount; ++i)
            seen |= sSeen[i] == (uint32_t)rip;
        if (!seen && sSeenCount < 128)
        {
            sSeen[sSeenCount++] = (uint32_t)rip;
            Log("bios: %s %08x at %08x", write ? "write" : "read", (unsigned)fault, (unsigned)rip);
        }
        sPending.active = true;
        sPending.reg = reg;
        sPending.old = SLOT(context, sGpr[reg]);
        sPending.now = sPending.old + (uint64_t)delta;
        SLOT(context, sGpr[reg]) = sPending.now;
        SLOT(context, sFlags) |= 0x100; /* trap after this one instruction */
    }
}

static void OnTrap(int sig, siginfo_t *info, void *context)
{
    if (!sPending.active)
    {
        Pass(&sOldTrap, sig, info, context);
        return;
    }
    /* Put the register back unless the instruction loaded something new into it. */
    if (SLOT(context, sGpr[sPending.reg]) == sPending.now)
        SLOT(context, sGpr[sPending.reg]) = sPending.old;
    SLOT(context, sFlags) &= ~(uint64_t)0x100;
    sPending.active = false;
}

/* Length of a DIV / IDIV (F6 / F7 /6, /7) instruction, 0 if it is not one. */
static int DivLength(const uint8_t *p, int *bytes)
{
    const uint8_t *start = p;
    int size16 = 0;
    while (*p == 0x66 || *p == 0x67 || *p == 0x2E || *p == 0x3E || *p == 0x26 || *p == 0x36 || *p == 0x64 || *p == 0x65 ||
           (*p & 0xF0) == 0x40)
    {
        size16 |= *p == 0x66;
        ++p;
    }
    if (*p != 0xF6 && *p != 0xF7)
        return 0;
    *bytes = *p == 0xF6 ? 1 : (size16 ? 2 : 4);
    uint8_t modrm = p[1];
    if (((modrm >> 3) & 7) < 6)
        return 0;
    int mod = modrm >> 6, rm = modrm & 7;
    p += 2;
    if (mod != 3)
    {
        if (rm == 4)
        {
            uint8_t sib = *p++;
            if (mod == 0 && (sib & 7) == 5)
                p += 4;
        }
        else if (mod == 0 && rm == 5)
            p += 4;
        p += mod == 1 ? 1 : (mod == 2 ? 4 : 0);
    }
    return (int)(p - start);
}

static void OnFpe(int sig, siginfo_t *info, void *context)
{
    uint64_t rip = SLOT(context, sRip);
    int bytes = 0, len = 0;

    if (rip >= PS5G_BASE && rip < sGuestEnd)
        len = DivLength((const uint8_t *)(uintptr_t)rip, &bytes);
    if (len == 0)
    {
        Pass(&sOldFpe, sig, info, context);
        return;
    }
    if (bytes == 1)
        SLOT(context, sGpr[0]) &= ~0xFFFFull; /* AL quotient, AH remainder */
    else if (bytes == 2)
    {
        SLOT(context, sGpr[0]) &= ~0xFFFFull;
        SLOT(context, sGpr[2]) &= ~0xFFFFull;
    }
    else
    {
        SLOT(context, sGpr[0]) = 0;
        SLOT(context, sGpr[2]) = 0;
    }
    SLOT(context, sRip) = rip + (uint64_t)len;
}

static void InstallFaultHandlers(const Ps5GuestHeader *h)
{
    struct sigaction sa;

    sBiosArea = h->biosArea;
    sGuestEnd = h->regionEnd;
    sContext = ProbeContext();
    if (!sContext)
    {
        Log("faults: the signal context could not be probed; stray pointers and divisions by zero are not covered");
        return;
    }
    Log("faults: context slots rip %d rsp %d rbp %d rax %d rbx %d rcx %d rdx %d rsi %d rdi %d", sRip, sGpr[4], sGpr[5], sGpr[0], sGpr[3],
        sGpr[1], sGpr[2], sGpr[6], sGpr[7]);
    memset(&sa, 0, sizeof(sa));
    sa.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = OnSegv;
    sigaction(SIGSEGV, &sa, &sOldSegv);
    sigaction(SIGBUS, &sa, NULL);
    sa.sa_sigaction = OnFpe;
    sigaction(SIGFPE, &sa, &sOldFpe);
    sa.sa_sigaction = OnTrap;
    sigaction(SIGTRAP, &sa, &sOldTrap);
}

/* Notices drawn when there is nothing to play (assets/<name>.rgba: w, h, then RGBA). */

static Notice LoadNotice(const char *name)
{
    static const char *const roots[] = {"/app0/assets", "/system_ex/app/" TITLE_ID "/assets", "/data/homebrew/" TITLE_ID "/assets"};
    Notice n = {-1, 0, 0};
    for (size_t i = 0; i < sizeof(roots) / sizeof(roots[0]) && n.tex < 0; ++i)
    {
        char path[512];
        snprintf(path, sizeof(path), "%s/%s.rgba", roots[i], name);
        FILE *f = fopen(path, "rb");
        if (!f)
            continue;
        uint32_t size[2];
        if (fread(size, sizeof(size), 1, f) == 1 && size[0] && size[1] && size[0] <= 4096 && size[1] <= 4096)
        {
            size_t bytes = (size_t)size[0] * size[1] * 4;
            uint8_t *pixels = malloc(bytes);
            if (pixels && fread(pixels, 1, bytes, f) == bytes)
            {
                n.tex = vkp_ui_texture_create(pixels, (int)size[0], (int)size[1], true, false, true);
                n.w = (int)size[0];
                n.h = (int)size[1];
            }
            free(pixels);
        }
        fclose(f);
    }
    return n;
}

/* One frame of a notice, centred and scaled to the screen (1500 px wide at 1920 px). */
static void DrawNotice(const Notice *n)
{
    plat_begin_frame(0xFF0A0A18u);
    if (n->tex >= 0)
    {
        const uint32_t white = 0xFFFFFFFFu;
        float k = plat_width() / 1920.0f, dw = (float)n->w * k, dh = (float)n->h * k;
        float x0 = (plat_width() - dw) / 2, y0 = (plat_height() - dh) / 2;
        VkpUiVertex q[6] = {{x0, y0, 0, 0, white},      {x0 + dw, y0, 1, 0, white},      {x0, y0 + dh, 0, 1, white},
                            {x0 + dw, y0, 1, 0, white}, {x0, y0 + dh, 0, 1, white},      {x0 + dw, y0 + dh, 1, 1, white}};
        vkp_ui_blend(true);
        vkp_ui_draw(n->tex, q, 6);
    }
    plat_end_frame();
}

/* Shows a notice for `frames` screen refreshes, keeping the controllers and system events alive. */
static void ShowNotice(const Notice *n, int frames)
{
    for (int i = 0; i < frames; ++i)
    {
        PadState pads[HOSTPAD_MAX];
        bool quit = false;
        plat_poll(pads, &quit);
        DrawNotice(n);
    }
}

/* The version of the cartridge in the data folder: "eu", "us" or "jp". */
static const char *FindRegion(void)
{
    static const struct
    {
        const char *code, *region;
    } known[] = {{"B8CP", "eu"}, {"B8CE", "us"}, {"B8CJ", "jp"}};
    const char *wanted = getenv("KHCOM_REGION");
    DIR *d = opendir(DATA_ROOT);
    const char *found = NULL;
    while (d)
    {
        struct dirent *e = readdir(d);
        if (!e)
            break;
        size_t n = strlen(e->d_name);
        if (n < 5 || strcasecmp(e->d_name + n - 4, ".gba") != 0)
            continue;
        char path[512], header[0xB0];
        snprintf(path, sizeof(path), DATA_ROOT "/%s", e->d_name);
        FILE *f = fopen(path, "rb");
        if (!f)
            continue;
        size_t got = fread(header, 1, sizeof(header), f);
        fclose(f);
        if (got != sizeof(header))
            continue;
        for (size_t i = 0; i < 3; ++i)
            if (memcmp(header + 0xAC, known[i].code, 4) == 0 && (!wanted || strcmp(wanted, known[i].region) == 0))
            {
                found = known[i].region;
                Log("cartridge: %s (%s)", e->d_name, known[i].region);
                break;
            }
        if (found)
            break;
    }
    if (d)
        closedir(d);
    if (!found && !d)
    {
        /* a sandboxed process cannot list /data: try the usual names */
        for (size_t i = 0; i < 3 && !found; ++i)
        {
            char path[512];
            snprintf(path, sizeof(path), DATA_ROOT "/%s.gba", known[i].code);
            FILE *f = fopen(path, "rb");
            if (f && (!wanted || strcmp(wanted, known[i].region) == 0))
            {
                found = known[i].region;
                Log("cartridge: %s.gba (%s)", known[i].code, known[i].region);
            }
            if (f)
                fclose(f);
        }
    }
    return found;
}

static bool LoadImage(const char *region, Ps5GuestHeader *header)
{
    static const char *const roots[] = {"/app0/assets", "/system_ex/app/" TITLE_ID "/assets", "/data/homebrew/" TITLE_ID "/assets"};
    FILE *f = NULL;
    char path[512];
    for (size_t i = 0; i < sizeof(roots) / sizeof(roots[0]) && !f; ++i)
    {
        snprintf(path, sizeof(path), "%s/guest-%s.bin", roots[i], region);
        f = fopen(path, "rb");
        if (f)
            setenv("KHCOM_ASSETS", roots[i], 1); /* the game's fonts are there too */
    }
    if (!f)
    {
        Log("the game image for %s was not found", region);
        return false;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    Ps5GuestHeader h;
    if (fread(&h, 1, sizeof(h), f) != sizeof(h) || h.magic != PS5G_MAGIC || h.imageEnd < (uint32_t)size)
    {
        fclose(f);
        Log("%s is not a game image", path);
        return false;
    }
    fseek(f, 0, SEEK_SET);
    /* As on the desktop build: the whole GBA address range is committed and zeroed as one block, so an access to a mirror
     * or to a gap between the regions reads zeros instead of faulting. */
    if (!MapAt(GBA_LOW, GBA_HIGH - GBA_LOW, "the GBA address space"))
    {
        fclose(f);
        return false;
    }
    memset((void *)(uintptr_t)GBA_LOW, 0, GBA_HIGH - GBA_LOW);
    if (!MapAt(PS5G_BASE, h.regionEnd - PS5G_BASE, "the game"))
    {
        fclose(f);
        return false;
    }
    ps5_crash_step("read the image");
    bool ok = fread((void *)(uintptr_t)PS5G_BASE, 1, (size_t)size, f) == (size_t)size;
    fclose(f);
    Log("image read: %s", ok ? "ok" : "short");
    if (ok)
    {
        ps5_crash_step("clear the image tail");
        /* the image's bss: everything between the file and imageEnd must read as zero (the first 8 MB were cleared on mapping) */
        for (uint32_t at = PS5G_BASE + (uint32_t)size; at < h.imageEnd;)
        {
            uint32_t n = h.imageEnd - at < (1u << 20) ? h.imageEnd - at : (1u << 20);
            Log("clearing 0x%08x + 0x%x", at, n);
            memset((void *)(uintptr_t)at, 0, n);
            at += n;
        }
        *header = h;
        Log("game image: %s (%ld bytes, ends at 0x%08x)", path, size, h.imageEnd);
        ps5_crash_step("image ready");
    }
    return ok;
}

int main(void)
{
    mkdir(DATA_ROOT, 0777);
    mkdir(DATA_ROOT "/logs", 0777);

    char error[200] = "";
    bool unlocked = plat_prepare_storage(error, sizeof(error));
    plat_want_output(2);
    plat_want_high_refresh(false);
    if (!plat_init())
    {
        char msg[300];
        snprintf(msg, sizeof(msg), "ReKHCoM: %s", plat_init_error());
        plat_notify(msg);
        for (;;)
            plat_sleep_us(1000000);
    }
    mkdir(DATA_ROOT, 0777);
    mkdir(DATA_ROOT "/logs", 0777);
    char log_path[256] = DATA_ROOT "/logs/ReKHCoM.log";
    rename(log_path, DATA_ROOT "/logs/ReKHCoM.old.log"); /* the log of the run before: a crash is still there after a restart */
    sLog = fopen(log_path, "w");
    ps5_crash_install(log_path);
    ps5_klog_capture_stderr("");
    Log("ReKHCoM starting; /data %s (%s)", unlocked ? "unlocked" : "sandboxed", error);
    Log("screen: %s", plat_screen_info());
    update_init(Log);

    /* Without a cartridge the app stays open and says what is needed; it starts once the file is there. */
    const char *region = FindRegion();
    if (!region)
    {
        Notice need = LoadNotice("need-rom");
        Log("no cartridge found in " DATA_ROOT ": waiting for one");
        while (!(region = FindRegion()))
            ShowNotice(&need, 60);
    }
    Ps5GuestHeader header;
    if (!LoadImage(region, &header))
    {
        Notice failed = LoadNotice("load-error");
        for (;;)
            ShowNotice(&failed, 600);
    }

    InstallFaultHandlers(&header);
    Log("image loaded; filling the bridge table at 0x%08x", header.hostTable);
    ps5_crash_step("bridge table");
    uint64_t *table = (uint64_t *)(uintptr_t)header.hostTable;
    uint64_t t[HF_COUNT] = {
        [HF_log] = (uintptr_t)H_log, [HF_exit] = (uintptr_t)H_exit, [HF_ticks_us] = (uintptr_t)H_ticks_us,
        [HF_sleep_us] = (uintptr_t)H_sleep_us, [HF_getenv] = (uintptr_t)H_getenv, [HF_fopen] = (uintptr_t)H_fopen,
        [HF_fread] = (uintptr_t)H_fread, [HF_fwrite] = (uintptr_t)H_fwrite, [HF_fseek] = (uintptr_t)H_fseek,
        [HF_ftell] = (uintptr_t)H_ftell, [HF_fclose] = (uintptr_t)H_fclose, [HF_fflush] = (uintptr_t)H_fflush,
        [HF_stat] = (uintptr_t)H_stat, [HF_remove] = (uintptr_t)H_remove, [HF_rename] = (uintptr_t)H_rename,
        [HF_mkdir] = (uintptr_t)H_mkdir, [HF_dir_open] = (uintptr_t)H_dir_open, [HF_dir_next] = (uintptr_t)H_dir_next,
        [HF_dir_close] = (uintptr_t)H_dir_close, [HF_gfx_size] = (uintptr_t)H_gfx_size,
        [HF_gfx_begin] = (uintptr_t)H_gfx_begin, [HF_gfx_end] = (uintptr_t)H_gfx_end,
        [HF_tex_create] = (uintptr_t)H_tex_create, [HF_tex_update] = (uintptr_t)H_tex_update,
        [HF_tex_free] = (uintptr_t)H_tex_free, [HF_gfx_draw] = (uintptr_t)H_gfx_draw, [HF_gfx_clip] = (uintptr_t)H_gfx_clip,
        [HF_gfx_blend] = (uintptr_t)H_gfx_blend, [HF_audio_open] = (uintptr_t)H_audio_open,
        [HF_audio_push] = (uintptr_t)H_audio_push, [HF_audio_queued] = (uintptr_t)H_audio_queued,
        [HF_audio_clear] = (uintptr_t)H_audio_clear, [HF_audio_ratio] = (uintptr_t)H_audio_ratio,
        [HF_pad_poll] = (uintptr_t)H_pad_poll, [HF_quit_requested] = (uintptr_t)H_quit_requested,
        [HF_sin] = (uintptr_t)(double (*)(double))sin, [HF_cos] = (uintptr_t)(double (*)(double))cos,
        [HF_tan] = (uintptr_t)(double (*)(double))tan, [HF_atan2] = (uintptr_t)(double (*)(double, double))atan2,
        [HF_pow] = (uintptr_t)(double (*)(double, double))pow, [HF_exp] = (uintptr_t)(double (*)(double))exp,
        [HF_log_e] = (uintptr_t)(double (*)(double))log, [HF_fmod] = (uintptr_t)(double (*)(double, double))fmod,
        [HF_sinf] = (uintptr_t)(float (*)(float))sinf, [HF_cosf] = (uintptr_t)(float (*)(float))cosf,
        [HF_powf] = (uintptr_t)(float (*)(float, float))powf, [HF_set_title] = (uintptr_t)H_set_title, [HF_notify] = (uintptr_t)H_notify,
        [HF_touch_poll] = (uintptr_t)H_touch_poll, [HF_app_state] = (uintptr_t)H_app_state,
        [HF_hd_music] = (uintptr_t)H_hd_music,
        [HF_hd_movie] = (uintptr_t)H_hd_movie,
        [HF_udp_open] = (uintptr_t)H_udp_open, [HF_udp_resolve] = (uintptr_t)H_udp_resolve,
        [HF_udp_send] = (uintptr_t)H_udp_send, [HF_udp_recv] = (uintptr_t)H_udp_recv,
    };
    for (int i = 0; i < HF_COUNT; ++i)
        if (!t[i])
        {
            Log("bridge entry %d has no host function", i);
            return 1;
        }
    memcpy(table, t, sizeof(t));
    Log("bridge table filled (%d entries)", HF_COUNT);

    int (*boot)(uint32_t) = (int (*)(uint32_t))(uintptr_t)header.boot;
    Log("starting the game: boot %p, stack 0x%08x, table %p", (void *)boot, header.stackTop, (void *)table);
    ps5_crash_step("the game");
    int rc = boot(header.stackTop);
    Log("the game returned %d", rc);
    H_exit(rc);
    return rc;
}
