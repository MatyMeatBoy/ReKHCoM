/* The 64-bit Linux host (26/10): the game as the x32 image the PS5 build makes (port/ps5/build_ps5.py guest: x86-64
 * code, 32-bit pointers, flat at PS5G_BASE), served by a native 64-bit program with SDL3. Every 64-bit Linux (Steam
 * Deck, desktops without 32-bit libraries) draws with its own GPU driver; the i386 build stays the fallback.
 *
 *   rekhcom64 guest-eu.bin [--data DIR]          (ReKHCoM.sh picks it on x86-64)
 *
 * The bridge is the PS5 host's (shared/bridge.h); files as port/ps5/test/linux_host.c, the GPU, audio and pads as
 * port/android64/host/android64_host.c; HD movies with the system's ffmpeg (hdmovie_ffmpeg.inc).
 */
#define _GNU_SOURCE
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bridge.h"
#include "../include/port/port_device_profile_host.h"

static char sData[1024] = ".";
static int sQuit;

static void Log(const char* fmt, ...) {
    char text[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(text, sizeof(text), fmt, ap);
    va_end(ap);
    fprintf(stderr, "[rekhcom64] %s\n", text);
}

/* ---- the basics and files ---- */
static FILE* sFiles[64];
static DIR* sDirs[16];
static void H_log(const char* s) { fprintf(stderr, "%s\n", s); }
static void H_exit(int code) { fflush(NULL); SDL_Quit(); _exit(code); }
static uint64_t H_ticks_us(void) { return SDL_GetTicksNS() / 1000; }
static void H_sleep_us(uint32_t us) { SDL_DelayNS((Uint64)us * 1000); }
static uint32_t sRendererGeneration; /* reserved H_getenv query; app_state remains 0/1 */
static int HostGpuQuery(const char*, char*, int);
static int H_getenv(const char* name, char* out, int cap) {
    int gpuQuery = HostGpuQuery(name, out, cap);
    if (gpuQuery != -2) return gpuQuery;
    if (!strcmp(name, "KHCOM_RENDERER_GENERATION")) {
        if (cap <= 0) return -1;
        snprintf(out, (size_t)cap, "bridge-textures-v1:%u", sRendererGeneration);
        return (int)strlen(out);
    }

    const char* v = PortDeviceHostQuery(name);
    if (!v) v = getenv(name);
    if (!v) return -1;
    snprintf(out, (size_t)cap, "%s", v);
    return (int)strlen(out);
}
static int H_fopen(const char* path, int mode) {
    static const char* const modes[] = { "rb", "wb", "ab", "r+b" };
    for (int i = 0; i < 64; i++)
        if (!sFiles[i]) {
            sFiles[i] = fopen(path, modes[mode & 3]);
            return sFiles[i] ? i : -1;
        }
    return -1;
}
static int H_fread(int h, void* buf, uint32_t n) { return (int)fread(buf, 1, n, sFiles[h]); }
static int H_fwrite(int h, const void* buf, uint32_t n) { return (int)fwrite(buf, 1, n, sFiles[h]); }
static int H_fseek(int h, int32_t off, int whence) { return fseek(sFiles[h], off, whence); }
static int H_ftell(int h) { return (int)ftell(sFiles[h]); }
static int H_fclose(int h) { int r = fclose(sFiles[h]); sFiles[h] = NULL; return r; }
static int H_fflush(int h) { return fflush(sFiles[h]); }
static int H_stat(const char* p) { struct stat s; return stat(p, &s) ? 0 : S_ISDIR(s.st_mode) ? 2 : 1; }
static int H_remove(const char* p) { return remove(p); }
static int H_rename(const char* a, const char* b) { return rename(a, b); }
static int H_mkdir(const char* p) { return mkdir(p, 0755); }
static int H_dir_open(const char* p) {
    for (int i = 0; i < 16; i++)
        if (!sDirs[i]) {
            sDirs[i] = opendir(p);
            return sDirs[i] ? i : -1;
        }
    return -1;
}
static int H_dir_next(int d, char* name, int cap) {
    struct dirent* e;
    while ((e = readdir(sDirs[d]))) {
        if (e->d_name[0] == '.') continue;
        snprintf(name, (size_t)cap, "%s", e->d_name);
        return 1;
    }
    return 0;
}
static void H_dir_close(int d) { closedir(sDirs[d]); sDirs[d] = NULL; }

/* ---- video: the GPU ---- */
static SDL_Window* sWindow;
static SDL_Renderer* sRenderer;
static int sBlend = 1;

static void H_gfx_size(int* w, int* h) { SDL_GetCurrentRenderOutputSize(sRenderer, w, h); }
static void H_gfx_begin(uint32_t clear) {
    SDL_SetRenderClipRect(sRenderer, NULL);
    SDL_SetRenderDrawColor(sRenderer, (clear >> 16) & 255, (clear >> 8) & 255, clear & 255, 255);
    if (clear >> 24)
        SDL_RenderClear(sRenderer);
}
#define HOST_GPU_TARGETS 1
#include "host_texture_slots.h"
#include "host_gpu_targets.h"
static void RendererDeviceReset(void);
static void H_gfx_clip(int x, int y, int w, int h) {
    SDL_Rect r = { x, y, w, h };
    SDL_SetRenderClipRect(sRenderer, w > 0 ? &r : NULL);
}
static void H_gfx_blend(int on) { sBlend = on; }
/* the shim adds 2, 4 or 6 to u for pixel art and the MMPX filters: those are drawn crisp (android64_host.c) */
static void H_gfx_draw(int id, const void* verts, int count) {
    const HostVertex* v = verts;
    int slot = HostTextureSlot(id);
    SDL_Texture* t = slot >= 0 ? sTex[slot] : NULL;
    static SDL_Vertex out[8192];
    float shift = 0;
    if (id >= 0 && !t)
        return;
    if (count > 8192)
        count = 8192;
    if (count > 0 && t) {
        shift = floorf((v[0].u + 0.25f) / 2.0f) * 2.0f;
        SDL_SetTextureScaleMode(t, shift > 0 || !sTexSmooth[slot] ? SDL_SCALEMODE_NEAREST : SDL_SCALEMODE_LINEAR);
        SDL_SetTextureBlendMode(t, (SDL_BlendMode)sBlend);
    }
    for (int i = 0; i < count; i++) {
        out[i].position.x = v[i].x;
        out[i].position.y = v[i].y;
        out[i].tex_coord.x = v[i].u - shift;
        out[i].tex_coord.y = v[i].v;
        out[i].color.r = ((v[i].argb >> 16) & 255) / 255.0f;
        out[i].color.g = ((v[i].argb >> 8) & 255) / 255.0f;
        out[i].color.b = (v[i].argb & 255) / 255.0f;
        out[i].color.a = (v[i].argb >> 24) / 255.0f;
    }
    SDL_SetRenderDrawBlendMode(sRenderer, (SDL_BlendMode)sBlend);
    SDL_RenderGeometry(sRenderer, t, out, count, NULL, 0);
}
static void H_notify(const char* name) { (void)name; }
static void H_gfx_end(void) {
    /* test harness: KHCOM_HOST_SHOT=frame[,frame...] saves those frames as host_shot_<frame>.bmp in the data folder */
    static unsigned frame;
    const char* shots = getenv("KHCOM_HOST_SHOT");
    frame++;
    for (const char* p = shots; p && *p;) {
        if ((unsigned)atoi(p) == frame) {
            SDL_Surface* s = SDL_RenderReadPixels(sRenderer, NULL);
            if (s) {
                char path[1200];
                snprintf(path, sizeof(path), "%s/host_shot_%u.bmp", sData, frame);
                SDL_SaveBMP(s, path);
                SDL_DestroySurface(s);
            }
        }
        p = strchr(p, ',');
        if (p) p++;
    }
    SDL_RenderPresent(sRenderer);
}

/* ---- audio ---- */
static SDL_AudioStream* sAudio;
static int sGameRate = 48000;
static int H_audio_open(int rate) {
    sGameRate = rate;
    if (sAudio)
        return 0;
    SDL_AudioSpec spec = { SDL_AUDIO_S16, 2, rate };
    sAudio = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
    if (!sAudio) {
        Log("audio: %s", SDL_GetError());
        return -1;
    }
    SDL_ResumeAudioStreamDevice(sAudio);
    return 0;
}
static void GamePush(const int16_t* frames, uint32_t count) {
    if (sAudio && count)
        SDL_PutAudioStreamData(sAudio, frames, (int)count * 4);
}
#define HD_MUSIC_LOG Log
#include "hd_music_host.h"
#define UDP_HOST_POSIX
#include "udp_host.h"
#define H_thread H_thread_legacy
#include "thread_host.h"
#undef H_thread
static int H_thread(int op, uint32_t a, uint32_t b, uint32_t c, uint32_t d) {
    int result = HostGpuCommand(op, a, b, c, d);
    return result == -2 ? H_thread_legacy(op, a, b, c, d) : result;
}

static void H_audio_push(const int16_t* frames, uint32_t count) { HdMusicPush(frames, count, sGameRate, GamePush); }
static uint32_t H_audio_queued(void) { return sAudio ? (uint32_t)SDL_GetAudioStreamQueued(sAudio) / 4 : 0; }
static void H_audio_clear(void) { if (sAudio) SDL_ClearAudioStream(sAudio); }
static void H_audio_ratio(double ratio) {
    if (sAudio && ratio > 0.5 && ratio < 2.0)
        SDL_SetAudioStreamFrequencyRatio(sAudio, (float)ratio);
}
static int H_hd_movie(const char* movie, const char* subs1, const char* subs2, const char* font); /* hdmovie_ffmpeg.inc */

/* ---- input: a gamepad, or the keyboard ---- */
static SDL_Gamepad* sPad;
static void RendererDeviceReset(void) {
    HostTextureReset();
    Log("renderer reset: bridge texture generation %u", sRendererGeneration);
}
static void Pump(void) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_EVENT_RENDER_DEVICE_RESET: RendererDeviceReset(); break;
            case SDL_EVENT_QUIT: sQuit = 1; break;
            case SDL_EVENT_GAMEPAD_ADDED:
                if (!sPad) sPad = SDL_OpenGamepad(e.gdevice.which);
                break;
            case SDL_EVENT_GAMEPAD_REMOVED:
                if (sPad && SDL_GetGamepadID(sPad) == e.gdevice.which) {
                    SDL_CloseGamepad(sPad);
                    sPad = NULL;
                }
                break;
            case SDL_EVENT_KEY_DOWN:
                if (e.key.key == SDLK_F11 || (e.key.key == SDLK_RETURN && (e.key.mod & SDL_KMOD_ALT)))
                    SDL_SetWindowFullscreen(sWindow, !(SDL_GetWindowFlags(sWindow) & SDL_WINDOW_FULLSCREEN));
                break;
            default: break;
        }
    }
}
static int H_pad_poll(HostPad* out, int count) {
    /* RetroPad numbering by position (bridge.h): B Y SELECT START UP DOWN LEFT RIGHT A X L R */
    static const struct { SDL_GamepadButton b; int bit; } map[] = {
        { SDL_GAMEPAD_BUTTON_SOUTH, 0 }, { SDL_GAMEPAD_BUTTON_WEST, 1 }, { SDL_GAMEPAD_BUTTON_BACK, 2 },
        { SDL_GAMEPAD_BUTTON_START, 3 }, { SDL_GAMEPAD_BUTTON_DPAD_UP, 4 }, { SDL_GAMEPAD_BUTTON_DPAD_DOWN, 5 },
        { SDL_GAMEPAD_BUTTON_DPAD_LEFT, 6 }, { SDL_GAMEPAD_BUTTON_DPAD_RIGHT, 7 }, { SDL_GAMEPAD_BUTTON_EAST, 8 },
        { SDL_GAMEPAD_BUTTON_NORTH, 9 }, { SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, 10 },
        { SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, 11 }, { SDL_GAMEPAD_BUTTON_LEFT_STICK, 14 },
        { SDL_GAMEPAD_BUTTON_RIGHT_STICK, 15 },
    };
    /* the keyboard, as the PC build's defaults: Z B, X A, A Y, S X, Q L, W R, Enter START, Backspace SELECT */
    static const struct { SDL_Scancode k; int bit; } keys[] = {
        { SDL_SCANCODE_Z, 0 }, { SDL_SCANCODE_A, 1 }, { SDL_SCANCODE_BACKSPACE, 2 }, { SDL_SCANCODE_RETURN, 3 },
        { SDL_SCANCODE_UP, 4 }, { SDL_SCANCODE_DOWN, 5 }, { SDL_SCANCODE_LEFT, 6 }, { SDL_SCANCODE_RIGHT, 7 },
        { SDL_SCANCODE_X, 8 }, { SDL_SCANCODE_S, 9 }, { SDL_SCANCODE_Q, 10 }, { SDL_SCANCODE_W, 11 },
    };
    const bool* kb;
    memset(out, 0, sizeof(HostPad) * (size_t)count);
    Pump();
    if (count < 1)
        return 0;
    out[0].connected = 1;
    kb = SDL_GetKeyboardState(NULL);
    for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++)
        if (kb[keys[i].k]) out[0].buttons |= 1u << keys[i].bit;
    if (sPad) {
        for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); i++)
            if (SDL_GetGamepadButton(sPad, map[i].b)) out[0].buttons |= 1u << map[i].bit;
        out[0].lx = SDL_GetGamepadAxis(sPad, SDL_GAMEPAD_AXIS_LEFTX);
        out[0].ly = SDL_GetGamepadAxis(sPad, SDL_GAMEPAD_AXIS_LEFTY);
        out[0].rx = SDL_GetGamepadAxis(sPad, SDL_GAMEPAD_AXIS_RIGHTX);
        out[0].ry = SDL_GetGamepadAxis(sPad, SDL_GAMEPAD_AXIS_RIGHTY);
        out[0].l2 = (uint8_t)(SDL_GetGamepadAxis(sPad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) >> 7);
        out[0].r2 = (uint8_t)(SDL_GetGamepadAxis(sPad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) >> 7);
        if (out[0].l2 > 64) out[0].buttons |= 1u << 12;
        if (out[0].r2 > 64) out[0].buttons |= 1u << 13;
    }
    return 1;
}
#include "hdmovie_ffmpeg.inc"

static int H_touch_poll(HostTouch* fingers, int max) { (void)fingers; (void)max; return 0; }
static int H_app_state(void) { return 0; }
static int H_quit_requested(void) { return sQuit; }
/* the image's full screen row arrives here as "\001fullscreen=0/1" (port/ps5/guest/src/sdl_shim.c) */
static void H_set_title(const char* s) {
    if (!sWindow || !s)
        return;
    if (s[0] == '\x01') {
        if (!strncmp(s + 1, "fullscreen=", 11))
            fprintf(stderr, "[rekhcom64] fullscreen %d\n",
                    SDL_SetWindowFullscreen(sWindow, s[12] == '1') ? s[12] - '0' : -1);
        return;
    }
    SDL_SetWindowTitle(sWindow, s);
}

/* ---- the image ---- */
static int MapFixed(uint32_t base, size_t size) {
    void* p = mmap((void*)(uintptr_t)base, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (p != (void*)(uintptr_t)base) {
        Log("cannot map %08x (+%zx): %s", base, size, strerror(errno));
        return 0;
    }
    return 1;
}

#define HF(name) [HF_##name] = (uint64_t)(uintptr_t)H_##name
#define HM(name, fn) [HF_##name] = (uint64_t)(uintptr_t)fn

int main(int argc, char** argv) {
    const char* image = argc > 1 ? argv[1] : "guest-eu.bin";
    const char* bootStatus = NULL;
    for (int i = 2; i < argc; i++)
        if (!strcmp(argv[i], "--data") && i + 1 < argc)
            snprintf(sData, sizeof(sData), "%s", argv[++i]);
        else if (!strcmp(argv[i], "--boot-status") && i + 1 < argc)
            bootStatus = argv[++i];
    setenv("KHCOM_DATA", sData, 1);
    {
        /* the game's packaged files, its HD text fonts among them, as the other hosts say (#277: without it the
           Steam Deck's HD text found no font and the dialogue showed none) */
        char assets[1100];
        snprintf(assets, sizeof(assets), "%s/assets", sData);
        setenv("KHCOM_ASSETS", assets, 0);
    }
    FILE* f = fopen(image, "rb");
    if (!f) {
        Log("%s: %s", image, strerror(errno));
        return 2;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t* tmp = malloc((size_t)size);
    if (!tmp || fread(tmp, 1, (size_t)size, f) != (size_t)size)
        return 2;
    fclose(f);
    Ps5GuestHeader h;
    memcpy(&h, tmp, sizeof(h));
    if (h.magic != PS5G_MAGIC) {
        Log("%s is not a game image", image);
        return 2;
    }
    /* the GBA's spaces at their hardware addresses, and the image */
    if (!MapFixed(0x02000000, 0x40000) || !MapFixed(0x03000000, 0x10000) || !MapFixed(0x04000000, 0x10000) ||
        !MapFixed(0x05000000, 0x10000) || !MapFixed(0x06000000, 0x20000) || !MapFixed(0x07000000, 0x10000) ||
        !MapFixed(0x08000000, 0x02000000) || !MapFixed(0x0E000000, 0x10000) ||
        !MapFixed(PS5G_BASE, h.regionEnd - PS5G_BASE))
        return 3;
    memcpy((void*)(uintptr_t)PS5G_BASE, tmp, (size_t)size);
    free(tmp);

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD)) {
        Log("SDL_Init: %s", SDL_GetError());
        return 4;
    }
    sWindow = SDL_CreateWindow("Kingdom Hearts: Chain of Memories", 1280, 720,
                               SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    sRenderer = sWindow ? SDL_CreateRenderer(sWindow, NULL) : NULL;
    if (!sRenderer) {
        Log("no renderer: %s", SDL_GetError());
        return 4;
    }
    SDL_SetRenderVSync(sRenderer, 1);
    Log("renderer: %s (64-bit host)", SDL_GetRendererName(sRenderer));
    {
        /* full screen as kh_com.ini says (the Steam Deck package's preset), or REKHCOM_FULLSCREEN=1 */
        char ini[1200], line[256];
        int full = getenv("REKHCOM_FULLSCREEN") != NULL;
        FILE* cfg;
        snprintf(ini, sizeof(ini), "%s/kh_com.ini", sData);
        if (!full && (cfg = fopen(ini, "r")) != NULL) {
            while (fgets(line, sizeof(line), cfg))
                if (!strncmp(line, "fullscreen=", 11))
                    full = atoi(line + 11) != 0;
            fclose(cfg);
        }
        if (!full) {
            snprintf(ini, sizeof(ini), "%s/kh_com.default.ini", sData);
            if ((cfg = fopen(ini, "r")) != NULL) {
                while (fgets(line, sizeof(line), cfg))
                    if (!strncmp(line, "fullscreen=", 11))
                        full = atoi(line + 11) != 0;
                fclose(cfg);
            }
        }
        if (full)
            SDL_SetWindowFullscreen(sWindow, true);
    }

    uint64_t* table = (uint64_t*)(uintptr_t)h.hostTable;
    uint64_t t[HF_COUNT] = {
        HF(log), HF(exit), HF(ticks_us), HF(sleep_us), HF(getenv), HF(fopen), HF(fread), HF(fwrite), HF(fseek), HF(ftell),
        HF(fclose), HF(fflush), HF(stat), HF(remove), HF(rename), HF(mkdir), HF(dir_open), HF(dir_next), HF(dir_close),
        HF(gfx_size), HF(gfx_begin), HF(gfx_end), HF(tex_create), HF(tex_update), HF(tex_free), HF(gfx_draw), HF(gfx_clip),
        HF(gfx_blend), HF(audio_open), HF(audio_push), HF(audio_queued), HF(audio_clear), HF(audio_ratio), HF(pad_poll),
        HF(quit_requested), HM(sin, sin), HM(cos, cos), HM(tan, tan), HM(atan2, atan2), HM(pow, pow), HM(exp, exp),
        HM(log_e, log), HM(fmod, fmod), HM(sinf, sinf), HM(cosf, cosf), HM(powf, powf), HF(set_title), HF(notify),
        HF(touch_poll), HF(app_state), HF(hd_music), HF(hd_movie),
        HM(udp_open, H_udp_open), HM(udp_resolve, H_udp_resolve), HM(udp_send, H_udp_send), HM(udp_recv, H_udp_recv),
        HM(thread, H_thread),
    };
    _Static_assert(sizeof(t) / sizeof(t[0]) == HF_COUNT, "bridge table out of step with bridge_list.h");
    memcpy(table, t, sizeof(t));
    int (*boot)(uint32_t) = (int (*)(uint32_t))(uintptr_t)h.boot;
    if (bootStatus) {
        FILE* ready = fopen(bootStatus, "w");
        if (!ready) { SDL_Quit(); return 4; }
        fputs("ready\n", ready);
        fclose(ready);
    }
    int rc = boot(h.stackTop);
    SDL_Quit();
    return rc;
}
