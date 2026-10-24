/* The 64-bit Android host (port/progress/ANDROID_64.md): loads the game image (port/android64/build_android64.py: AArch64
 * code with 32-bit pointers, a position-independent Mach-O linked at 0x01000000, moved elsewhere below 4 GiB where that
 * range is taken) from the APK, serves the bridge
 * (port/ps5/shared/bridge_list.h) with SDL3 and starts the game.
 *
 * The 32-bit app needs the GBA's hardware addresses, which some phones keep busy (issues #22, #34) and 64-bit-only
 * phones cannot run at all. Here the GBA space is the image's own memory (PORT_GBA_RELOC) and the image sits below
 * ART's heap (0x12c00000), claimed as soon as this library loads.
 *
 * Video: the game's quads go to the GPU with SDL_RenderGeometry. Sound: an SDL audio stream (the bridge's rate
 * ratio becomes the stream's frequency ratio). Input: SDL gamepads, the touch screen (touch_poll; the game draws its
 * own touch controls), the background state (app_state). Files: the data folder the activity passes (--data-dir);
 * "apk:/<name>" paths are the APK's assets. */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <dirent.h>
#include <errno.h>
#include <math.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/ucontext.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/system_properties.h>
#include <jni.h>
#include <unistd.h>

#include "bridge.h"

#define IMAGE_BASE 0x01000000u
#define IMAGE_LIMIT 0x0C000000u /* the reservation: the image (about 150 MB) ends well before ART's heap */

static int sReserved;

/* Before the graphics driver, the audio stack and the Java heap's growth can land in the range. */
__attribute__((constructor)) static void ReserveImageSpace(void) {
    void* p = mmap((void*)(uintptr_t)IMAGE_BASE, IMAGE_LIMIT - IMAGE_BASE, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (p == (void*)(uintptr_t)IMAGE_BASE)
        sReserved = 1;
    else if (p != MAP_FAILED)
        munmap(p, IMAGE_LIMIT - IMAGE_BASE);
}

/* ---- log ------------------------------------------------------------------------------------------------------ */

static FILE* sLog;
static char sData[768] = ".";

static void Log(const char* fmt, ...) {
    char line[1024];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(line, sizeof(line), fmt, ap);
    va_end(ap);
    size_t n = strlen(line);
    while (n && (line[n - 1] == '\n' || line[n - 1] == '\r'))
        line[--n] = 0;
    SDL_Log("[ReKHCoM] %s", line);
    if (sLog) {
        fprintf(sLog, "%s\n", line);
        fflush(sLog);
    }
}

/* GameActivity.shareLog: the logs and the device in Android's share sheet */
static void ShareLog(void) {
    JNIEnv* env = (JNIEnv*)SDL_GetAndroidJNIEnv();
    jobject activity = (jobject)SDL_GetAndroidActivity();
    if (env && activity) {
        jclass cls = (*env)->GetObjectClass(env, activity);
        jmethodID share = (*env)->GetMethodID(env, cls, "shareLog", "()V");
        if (share)
            (*env)->CallVoidMethod(env, activity, share);
        if ((*env)->ExceptionCheck(env))
            (*env)->ExceptionClear(env);
        (*env)->DeleteLocalRef(env, cls);
        (*env)->DeleteLocalRef(env, activity);
        SDL_Delay(1500); /* the share sheet is up before this process ends */
    }
}

static void Fatal(const char* text) {
    static const SDL_MessageBoxButtonData buttons[] = {
        { SDL_MESSAGEBOX_BUTTON_ESCAPEKEY_DEFAULT, 0, "OK" },
        { SDL_MESSAGEBOX_BUTTON_RETURNKEY_DEFAULT, 1, "Share log" },
    };
    SDL_MessageBoxData box = { SDL_MESSAGEBOX_ERROR, NULL, "Kingdom Hearts: Chain of Memories", text, 2, buttons, NULL };
    int pressed = 0;
    Log("%s", text);
    if (sLog)
        fflush(sLog);
    if (SDL_ShowMessageBox(&box, &pressed) && pressed == 1)
        ShareLog();
    exit(1);
}

/* The device, at the top of the log: what a report needs to tell phones apart */
static void LogDevice(void) {
    char model[PROP_VALUE_MAX] = "", maker[PROP_VALUE_MAX] = "", release[PROP_VALUE_MAX] = "", sdk[PROP_VALUE_MAX] = "";
    __system_property_get("ro.product.manufacturer", maker);
    __system_property_get("ro.product.model", model);
    __system_property_get("ro.build.version.release", release);
    __system_property_get("ro.build.version.sdk", sdk);
    Log("device: %s %s, Android %s (API %s), page %ld bytes, linked range %s", maker, model, release, sdk,
        sysconf(_SC_PAGESIZE), sReserved ? "reserved at load" : "TAKEN at load");
}

/* ---- bridge: process, files ----------------------------------------------------------------------------------- */

static int sQuit, sBackground;
static jmp_buf sExit; /* the game's exit returns to main: SDL's activity ends the process cleanly from there */
static int sExitCode;

static void H_log(const char* s) { Log("%s", s); }
static void H_exit(int code) {
    Log("the game ended (%d)", code);
    if (sLog)
        fflush(sLog);
    sExitCode = code;
    longjmp(sExit, 1);
}
static uint64_t H_ticks_us(void) { return SDL_GetTicksNS() / 1000; }
static void H_sleep_us(uint32_t us) { SDL_DelayNS((Uint64)us * 1000); }
static int H_getenv(const char* name, char* out, int cap) {
    const char* v = SDL_getenv(name);
    if (!v)
        return -1;
    SDL_strlcpy(out, v, (size_t)cap);
    return (int)strlen(out);
}

/* Files are SDL streams: a normal path is a file, "apk:/name" an asset of the APK (with or without "assets/"). */
static SDL_IOStream* sFiles[64];
static SDL_IOStream* OpenPath(const char* path, const char* mode) {
    if (strncmp(path, "apk:/", 5) == 0) {
        const char* name = path + 5;
        SDL_IOStream* io = SDL_IOFromFile(name, "rb");
        if (!io && strncmp(name, "assets/", 7) == 0)
            io = SDL_IOFromFile(name + 7, "rb");
        return io;
    }
    return SDL_IOFromFile(path, mode);
}
static int H_fopen(const char* path, int mode) {
    static const char* const modes[] = { "rb", "wb", "ab", "r+b" };
    for (int i = 0; i < 64; ++i)
        if (!sFiles[i]) {
            sFiles[i] = OpenPath(path, modes[mode & 3]);
            return sFiles[i] ? i : -1;
        }
    return -1;
}
static int H_fread(int h, void* buf, uint32_t n) { return (int)SDL_ReadIO(sFiles[h], buf, n); }
static int H_fwrite(int h, const void* buf, uint32_t n) { return (int)SDL_WriteIO(sFiles[h], buf, n); }
static int H_fseek(int h, int32_t off, int whence) {
    return SDL_SeekIO(sFiles[h], off, whence == 0 ? SDL_IO_SEEK_SET : whence == 1 ? SDL_IO_SEEK_CUR : SDL_IO_SEEK_END) < 0 ? -1 : 0;
}
static int H_ftell(int h) { return (int)SDL_TellIO(sFiles[h]); }
static int H_fclose(int h) {
    bool ok = SDL_CloseIO(sFiles[h]);
    sFiles[h] = NULL;
    return ok ? 0 : -1;
}
static int H_fflush(int h) { return SDL_FlushIO(sFiles[h]) ? 0 : -1; }
static int H_stat(const char* p) {
    struct stat s;
    if (strncmp(p, "apk:/", 5) == 0) {
        SDL_IOStream* io = OpenPath(p, "rb");
        if (!io)
            return 0;
        SDL_CloseIO(io);
        return 1;
    }
    return stat(p, &s) ? 0 : S_ISDIR(s.st_mode) ? 2 : 1;
}
static int H_remove(const char* p) { return remove(p); }
static int H_rename(const char* a, const char* b) { return rename(a, b); }
static int H_mkdir(const char* p) { return mkdir(p, 0777); }
static DIR* sDirs[16];
static int H_dir_open(const char* p) {
    for (int i = 0; i < 16; ++i)
        if (!sDirs[i]) {
            sDirs[i] = opendir(p);
            return sDirs[i] ? i : -1;
        }
    return -1;
}
static int H_dir_next(int d, char* name, int cap) {
    struct dirent* e;
    while ((e = readdir(sDirs[d]))) {
        if (e->d_name[0] == '.')
            continue;
        SDL_strlcpy(name, e->d_name, (size_t)cap);
        return 1;
    }
    return 0;
}
static void H_dir_close(int d) {
    closedir(sDirs[d]);
    sDirs[d] = NULL;
}

/* ---- video: the GPU ------------------------------------------------------------------------------------------- */

static SDL_Window* sWindow;
static SDL_Renderer* sRenderer;
static SDL_Texture* sTex[4096];
static int sTexW[4096], sTexH[4096], sTexSmooth[4096];
static int sBlend = 1;

static void H_gfx_size(int* w, int* h) { SDL_GetCurrentRenderOutputSize(sRenderer, w, h); }
static void H_gfx_begin(uint32_t clear) {
    SDL_SetRenderClipRect(sRenderer, NULL);
    SDL_SetRenderDrawColor(sRenderer, (clear >> 16) & 255, (clear >> 8) & 255, clear & 255, 255);
    if (clear >> 24)
        SDL_RenderClear(sRenderer);
}
static int H_tex_create(const void* rgba, int w, int h, int smooth) {
    for (int i = 0; i < 4096; i++)
        if (!sTex[i]) {
            sTex[i] = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, w, h);
            if (!sTex[i])
                return -1;
            SDL_UpdateTexture(sTex[i], NULL, rgba, w * 4);
            SDL_SetTextureBlendMode(sTex[i], SDL_BLENDMODE_BLEND);
            sTexW[i] = w;
            sTexH[i] = h;
            sTexSmooth[i] = smooth;
            return i;
        }
    return -1;
}
static void H_tex_update(int id, const void* rgba) { SDL_UpdateTexture(sTex[id], NULL, rgba, sTexW[id] * 4); }
static void H_tex_free(int id) {
    SDL_DestroyTexture(sTex[id]);
    sTex[id] = NULL;
}
static void H_gfx_clip(int x, int y, int w, int h) {
    SDL_Rect r = { x, y, w, h };
    SDL_SetRenderClipRect(sRenderer, w > 0 ? &r : NULL);
}
static void H_gfx_blend(int on) { sBlend = on; }

/* HostVertex triangles; the shim adds 2, 4 or 6 to u for pixel art and the MMPX filters (port/ps5/guest/src/sdl_shim.c):
   those are drawn crisp (nearest), as the software hosts read them */
static void H_gfx_draw(int id, const void* verts, int count) {
    const HostVertex* v = verts;
    SDL_Texture* t = id >= 0 && id < 4096 ? sTex[id] : NULL;
    static SDL_Vertex out[8192];
    int crisp = 0;
    float shift = 0;
    if (id >= 0 && !t)
        return;
    if (count > 8192)
        count = 8192;
    if (count > 0 && t) {
        shift = floorf((v[0].u + 0.25f) / 2.0f) * 2.0f;
        crisp = shift > 0;
        SDL_SetTextureScaleMode(t, crisp || !sTexSmooth[id] ? SDL_SCALEMODE_NEAREST : SDL_SCALEMODE_LINEAR);
        SDL_SetTextureBlendMode(t, sBlend ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
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
    SDL_SetRenderDrawBlendMode(sRenderer, sBlend ? SDL_BLENDMODE_BLEND : SDL_BLENDMODE_NONE);
    SDL_RenderGeometry(sRenderer, t, out, count, NULL, 0);
}

/* notices: assets/<name>.rgba (w, h header) drawn over the game for five seconds */
static SDL_Texture* sToast;
static int sToastW, sToastH;
static Uint64 sToastUntil;
static SDL_Texture* LoadNotice(const char* name, int* w, int* h) {
    char path[160];
    size_t size = 0;
    SDL_snprintf(path, sizeof(path), "%s.rgba", name);
    uint8_t* d = SDL_LoadFile(path, &size);
    SDL_Texture* t = NULL;
    if (d && size > 8) {
        uint32_t ww, hh;
        memcpy(&ww, d, 4);
        memcpy(&hh, d + 4, 4);
        if ((size_t)ww * hh * 4 + 8 <= size) {
            t = SDL_CreateTexture(sRenderer, SDL_PIXELFORMAT_ABGR8888, SDL_TEXTUREACCESS_STATIC, (int)ww, (int)hh);
            SDL_UpdateTexture(t, NULL, d + 8, (int)ww * 4);
            SDL_SetTextureBlendMode(t, SDL_BLENDMODE_BLEND);
            *w = (int)ww;
            *h = (int)hh;
        }
    }
    SDL_free(d);
    return t;
}
static void H_notify(const char* name) {
    if (sToast)
        SDL_DestroyTexture(sToast);
    sToast = LoadNotice(name, &sToastW, &sToastH);
    sToastUntil = SDL_GetTicks() + 5000;
}
static void DrawBox(SDL_Texture* t, int w, int h, float y0, float alpha) {
    int ow, oh;
    SDL_GetCurrentRenderOutputSize(sRenderer, &ow, &oh);
    float k = ow / 1920.0f;
    SDL_FRect r = { (ow - w * k) / 2, y0 < 0 ? (oh - h * k) / 2 : y0, w * k, h * k };
    SDL_SetRenderClipRect(sRenderer, NULL);
    SDL_SetTextureAlphaModFloat(t, alpha);
    SDL_RenderTexture(sRenderer, t, NULL, &r);
}
static void H_gfx_end(void) {
    if (sToast) {
        Uint64 now = SDL_GetTicks();
        if (now >= sToastUntil) {
            SDL_DestroyTexture(sToast);
            sToast = NULL;
        } else {
            int ow, oh;
            SDL_GetCurrentRenderOutputSize(sRenderer, &ow, &oh);
            float k = ow / 1920.0f, left = (float)(sToastUntil - now);
            DrawBox(sToast, sToastW, sToastH, oh - sToastH * k - 90 * k, left < 400 ? left / 400 : 1);
        }
    }
    {
        /* test harness: KHCOM_HOST_SHOT=frame[,frame...] saves those frames as host_shot_<frame>.bmp in the data folder */
        static unsigned frame;
        const char* shots = SDL_getenv("KHCOM_HOST_SHOT");
        frame++;
        for (const char* p = shots; p && *p;) {
            if ((unsigned)SDL_atoi(p) == frame) {
                SDL_Surface* s = SDL_RenderReadPixels(sRenderer, NULL);
                if (s) {
                    char path[900];
                    SDL_snprintf(path, sizeof(path), "%s/host_shot_%u.bmp", sData, frame);
                    SDL_SaveBMP(s, path);
                    SDL_DestroySurface(s);
                    Log("frame %u saved", frame);
                }
            }
            p = SDL_strchr(p, ',');
            if (p)
                p++;
        }
    }
    SDL_RenderPresent(sRenderer);
}

/* ---- audio ---------------------------------------------------------------------------------------------------- */

static SDL_AudioStream* sAudio;
static int sGameRate = 48000; /* the rate the game opened the audio at */
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
    Log("audio: %d Hz", rate);
    return 0;
}
static void GamePush(const int16_t* frames, uint32_t count) {
    if (sAudio && count)
        SDL_PutAudioStreamData(sAudio, frames, (int)count * 4);
}

/* HD music: the host decodes the track the game names and mixes it into the game's blocks (shared/hd_music_host.h) */
#define HD_MUSIC_LOG Log
#include "hd_music_host.h"
/* the network link's socket (shared/udp_host.h): BSD sockets */
#define UDP_HOST_POSIX
#include "udp_host.h"
static void H_audio_push(const int16_t* frames, uint32_t count) { HdMusicPush(frames, count, sGameRate, GamePush); }

/* HD movies: played here to the end with the NDK's decoders (port/portable/hdmovie_android.c, as on 32-bit Android) */
int HdMovieAndroidPlay(SDL_Renderer* renderer, const char* path, const char* subs1, const char* subs2, const char* font);
void HdMovieLog(const char* fmt, ...) {
    char text[1024];
    va_list ap;
    va_start(ap, fmt);
    SDL_vsnprintf(text, sizeof(text), fmt, ap);
    va_end(ap);
    Log("%s", text);
}
static int H_hd_movie(const char* movie, const char* subs1, const char* subs2, const char* font) {
    int played = HdMovieAndroidPlay(sRenderer, movie, subs1, subs2, font);
    if (played == 2)
        sQuit = 1;
    return played != 0;
}
static uint32_t H_audio_queued(void) { return sAudio ? (uint32_t)SDL_GetAudioStreamQueued(sAudio) / 4 : 0; }
static void H_audio_clear(void) {
    if (sAudio)
        SDL_ClearAudioStream(sAudio);
}
static void H_audio_ratio(double ratio) {
    if (sAudio && ratio > 0.5 && ratio < 2.0)
        SDL_SetAudioStreamFrequencyRatio(sAudio, (float)ratio);
}

/* ---- input ---------------------------------------------------------------------------------------------------- */

static SDL_Gamepad* sPad;
static HostTouch sFingers[HOSTTOUCH_MAX];
static int sFingerCount;

static void TouchEvent(const SDL_TouchFingerEvent* e, int down) {
    int i;
    for (i = 0; i < sFingerCount && sFingers[i].id != (uint32_t)e->fingerID; i++) {
    }
    if (!down) {
        if (i < sFingerCount)
            sFingers[i] = sFingers[--sFingerCount];
        return;
    }
    if (i == sFingerCount) {
        if (sFingerCount == HOSTTOUCH_MAX)
            return;
        sFingerCount++;
    }
    sFingers[i].id = (uint32_t)e->fingerID;
    sFingers[i].x = e->x;
    sFingers[i].y = e->y;
}

static void Pump(void) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_EVENT_QUIT: sQuit = 1; break;
            case SDL_EVENT_WILL_ENTER_BACKGROUND:
            case SDL_EVENT_DID_ENTER_BACKGROUND: sBackground = 1; break;
            case SDL_EVENT_WILL_ENTER_FOREGROUND:
            case SDL_EVENT_DID_ENTER_FOREGROUND: sBackground = 0; break;
            case SDL_EVENT_GAMEPAD_ADDED:
                if (!sPad)
                    sPad = SDL_OpenGamepad(e.gdevice.which);
                break;
            case SDL_EVENT_GAMEPAD_REMOVED:
                if (sPad && SDL_GetGamepadID(sPad) == e.gdevice.which) {
                    SDL_CloseGamepad(sPad);
                    sPad = NULL;
                }
                break;
            case SDL_EVENT_FINGER_DOWN:
            case SDL_EVENT_FINGER_MOTION: TouchEvent(&e.tfinger, 1); break;
            case SDL_EVENT_FINGER_UP:
            case SDL_EVENT_FINGER_CANCELED: TouchEvent(&e.tfinger, 0); break;
            default: break;
        }
    }
}

static int H_pad_poll(HostPad* out, int count) {
    memset(out, 0, sizeof(HostPad) * (size_t)count);
    Pump();
    if (count < 1 || !sPad)
        return 0;
    /* RetroPad numbering by position (bridge.h) */
    static const struct { SDL_GamepadButton b; int bit; } map[] = {
        { SDL_GAMEPAD_BUTTON_SOUTH, 0 }, { SDL_GAMEPAD_BUTTON_WEST, 1 }, { SDL_GAMEPAD_BUTTON_BACK, 2 },
        { SDL_GAMEPAD_BUTTON_START, 3 }, { SDL_GAMEPAD_BUTTON_DPAD_UP, 4 }, { SDL_GAMEPAD_BUTTON_DPAD_DOWN, 5 },
        { SDL_GAMEPAD_BUTTON_DPAD_LEFT, 6 }, { SDL_GAMEPAD_BUTTON_DPAD_RIGHT, 7 }, { SDL_GAMEPAD_BUTTON_EAST, 8 },
        { SDL_GAMEPAD_BUTTON_NORTH, 9 }, { SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, 10 },
        { SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER, 11 }, { SDL_GAMEPAD_BUTTON_LEFT_STICK, 14 },
        { SDL_GAMEPAD_BUTTON_RIGHT_STICK, 15 }, { SDL_GAMEPAD_BUTTON_TOUCHPAD, 16 },
    };
    out[0].connected = 1;
    for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); i++)
        if (SDL_GetGamepadButton(sPad, map[i].b))
            out[0].buttons |= 1u << map[i].bit;
    out[0].lx = SDL_GetGamepadAxis(sPad, SDL_GAMEPAD_AXIS_LEFTX);
    out[0].ly = SDL_GetGamepadAxis(sPad, SDL_GAMEPAD_AXIS_LEFTY);
    out[0].rx = SDL_GetGamepadAxis(sPad, SDL_GAMEPAD_AXIS_RIGHTX);
    out[0].ry = SDL_GetGamepadAxis(sPad, SDL_GAMEPAD_AXIS_RIGHTY);
    out[0].l2 = (uint8_t)(SDL_GetGamepadAxis(sPad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER) >> 7);
    out[0].r2 = (uint8_t)(SDL_GetGamepadAxis(sPad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) >> 7);
    if (out[0].l2 > 64)
        out[0].buttons |= 1u << 12;
    if (out[0].r2 > 64)
        out[0].buttons |= 1u << 13;
    return 1;
}
static int H_touch_poll(HostTouch* fingers, int max) {
    int n = sFingerCount < max ? sFingerCount : max;
    memcpy(fingers, sFingers, sizeof(HostTouch) * (size_t)n);
    return n;
}
static int H_app_state(void) { return sBackground; }
static int H_quit_requested(void) { return sQuit; }
static void H_set_title(const char* s) { (void)s; }

/* ---- the game image ------------------------------------------------------------------------------------------- */

typedef struct {
    uint32_t magic, cputype, cpusubtype, filetype, ncmds, sizeofcmds, flags;
} MachHeader;
typedef struct {
    uint32_t cmd, cmdsize;
    char segname[16];
    uint32_t vmaddr, vmsize, fileoff, filesize, maxprot, initprot, nsects, flags;
} MachSegment;
typedef struct {
    uint32_t cmd, cmdsize, rebase_off, rebase_size; /* the rest of LC_DYLD_INFO_ONLY is not used */
} MachDyldInfo;

static char sImageError[160]; /* why the image did not load: shown in the error box too (issues #22, #34) */
static uint32_t sImageLow, sImageHigh, sImageSlide;

/* A crash in the game: the log gets the fault and the game's call chain as linked addresses (pc minus the slide, to
   look up with llvm-symbolizer on guest-<region>.macho), so a tester's "Share log" says where. Then the default
   action runs: Android's own crash report. */
static void CrashSignal(int sig, siginfo_t* info, void* context) {
    const ucontext_t* uc = (const ucontext_t*)context;
    uint64_t pc = uc->uc_mcontext.pc, lr = uc->uc_mcontext.regs[30], fp = uc->uc_mcontext.regs[29];
#define LINKED(a) ((a) >= sImageLow && (a) < sImageHigh ? (unsigned long long)((a) - sImageSlide) : 0ull)
    Log("crash: signal %d, fault address %p, pc 0x%llx (game 0x%llx), lr 0x%llx (game 0x%llx)", sig, info->si_addr,
        (unsigned long long)pc, LINKED(pc), (unsigned long long)lr, LINKED(lr));
    /* the game is built with frame pointers: [fp] = caller's fp, [fp + 8] = return address */
    for (int i = 0; i < 24 && fp >= sImageLow && fp + 16 <= 0x100000000ull && !(fp & 7); i++) {
        uint64_t ret = ((const uint64_t*)(uintptr_t)fp)[1], next = ((const uint64_t*)(uintptr_t)fp)[0];
        Log("  #%02d game 0x%llx", i, LINKED(ret));
        if (next <= fp)
            break;
        fp = next;
    }
#undef LINKED
    signal(sig, SIG_DFL);
}

static void CatchCrashes(void) {
    static const int sigs[] = { SIGSEGV, SIGBUS, SIGILL, SIGFPE, SIGABRT };
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = CrashSignal;
    sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
    for (size_t i = 0; i < sizeof(sigs) / sizeof(sigs[0]); i++)
        sigaction(sigs[i], &sa, NULL);
}

static int ImageFail(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(sImageError, sizeof(sImageError), fmt, ap);
    va_end(ap);
    Log("%s", sImageError);
    return 0;
}

static int MappedSegment(const MachSegment* seg) {
    return seg->cmd == 1 && seg->vmsize && strncmp(seg->segname, "__PAGEZERO", 16) && strncmp(seg->segname, "__LINKEDIT", 16);
}

/* The busy ranges below 4 GiB, from /proc/self/maps: logged when the linked address is taken, so a report shows by
   what, and used to find another place for the image. */
static int BusyRanges(uint64_t (*out)[2], int max) {
    FILE* f = fopen("/proc/self/maps", "r");
    char line[512];
    int n = 0;
    if (!f)
        return 0;
    while (n < max && fgets(line, sizeof(line), f)) {
        unsigned long long a, b;
        if (sscanf(line, "%llx-%llx", &a, &b) == 2 && a < 0x100000000ull) {
            out[n][0] = a;
            out[n][1] = b;
            n++;
        }
    }
    fclose(f);
    return n;
}

/* Claims [base, base + size) for the image: the range reserved when this library loaded (the linked address), or a
   free one below 4 GiB, the image's pointers being 32-bit. Another place is searched downwards from 0xF0000000: ART
   hands out its own low-memory blocks upwards from the bottom, so the top of the range is the last to fill. */
static uint32_t ClaimImageSpace(uint32_t linked, uint32_t size) {
    static uint64_t busy[1024][2];
    size_t page = (size_t)sysconf(_SC_PAGESIZE);
    const uint32_t align = 0x00100000u, top = 0xF0000000u;
    uint32_t base;
    int n, i;
    /* KHCOM_IMAGE_MOVE (test harness): load elsewhere even when the linked address is free */
    if (sReserved && linked >= IMAGE_BASE && linked + (uint64_t)size <= IMAGE_LIMIT && !getenv("KHCOM_IMAGE_MOVE"))
        return linked;
    if (sReserved) /* reserved but the image does not fit there: give it back */
        munmap((void*)(uintptr_t)IMAGE_BASE, IMAGE_LIMIT - IMAGE_BASE);
    sReserved = 0;
    n = BusyRanges(busy, 1024);
    Log("%d mappings below 4 GiB", n);
    for (i = 0; i < n; i++)
        if (busy[i][1] > linked && busy[i][0] < (uint64_t)linked + size)
            Log("0x%08x is taken: %08llx-%08llx", linked, (unsigned long long)busy[i][0], (unsigned long long)busy[i][1]);
    for (base = (top - size) & ~(align - 1); base >= align; base -= align) {
        void* p;
        for (i = 0; i < n; i++)
            if (busy[i][1] > base && busy[i][0] < (uint64_t)base + size)
                break;
        if (i < n) { /* skip below the range in the way */
            uint64_t below = busy[i][0] > size ? (busy[i][0] - size) & ~(uint64_t)(align - 1) : 0;
            if (below < align)
                break;
            base = (uint32_t)below + align;
            continue;
        }
        p = mmap((void*)(uintptr_t)base, (size + page - 1) & ~(page - 1), PROT_NONE,
                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
        if (p == (void*)(uintptr_t)base) {
            sReserved = 1;
            return base;
        }
        if (p != MAP_FAILED)
            munmap(p, size);
    }
    return 0;
}

/* Where the system refuses to turn written memory executable (an SELinux policy without execmem), the code goes into
   an anonymous memory file mapped executable over the same addresses: what ART's JIT does. */
static int ExecFromMemfd(void* p, size_t len) {
    int fd = (int)syscall(__NR_memfd_create, "rekhcom-code", 1u /* MFD_CLOEXEC */), ok = 0;
    if (fd < 0)
        return 0;
    if (write(fd, p, len) == (ssize_t)len &&
        mmap(p, len, PROT_READ | PROT_EXEC, MAP_SHARED | MAP_FIXED, fd, 0) == p) {
        __builtin___clear_cache((char*)p, (char*)p + len);
        Log("game code mapped from a memory file (no execmem)");
        ok = 1;
    }
    close(fd);
    return ok;
}

/* Mach-O rebase opcodes (LC_DYLD_INFO_ONLY): every 32-bit pointer of the image moves by the slide */
static int Rebase(const uint8_t* op, const uint8_t* end, const uint32_t* segAt, int segs, uint32_t slide) {
    int seg = -1, type = 0;
    uint64_t addr = 0, count, skip;
#define ULEB(v) \
    do { \
        int shift_ = 0; \
        (v) = 0; \
        while (op < end) { \
            uint8_t b_ = *op++; \
            (v) |= (uint64_t)(b_ & 0x7F) << shift_; \
            shift_ += 7; \
            if (!(b_ & 0x80)) \
                break; \
        } \
    } while (0)
#define POINTER() \
    do { \
        if (seg < 0 || seg >= segs || type != 1) \
            return 0; \
        *(uint32_t*)(uintptr_t)(segAt[seg] + slide + addr) += slide; \
        addr += 4; \
    } while (0)
    while (op < end) {
        uint8_t imm = *op & 0x0F, code = *op & 0xF0;
        op++;
        switch (code) {
            case 0x00: /* DONE */
                return 1;
            case 0x10: /* SET_TYPE_IMM */
                type = imm;
                break;
            case 0x20: /* SET_SEGMENT_AND_OFFSET_ULEB */
                seg = imm;
                ULEB(addr);
                break;
            case 0x30: /* ADD_ADDR_ULEB */
                ULEB(skip);
                addr += skip;
                break;
            case 0x40: /* ADD_ADDR_IMM_SCALED */
                addr += imm * 4u;
                break;
            case 0x50: /* DO_REBASE_IMM_TIMES */
                for (count = imm; count; count--)
                    POINTER();
                break;
            case 0x60: /* DO_REBASE_ULEB_TIMES */
                ULEB(count);
                for (; count; count--)
                    POINTER();
                break;
            case 0x70: /* DO_REBASE_ADD_ADDR_ULEB */
                POINTER();
                ULEB(skip);
                addr += skip;
                break;
            case 0x80: /* DO_REBASE_ULEB_TIMES_SKIPPING_ULEB */
                ULEB(count);
                ULEB(skip);
                for (; count; count--) {
                    POINTER();
                    addr += skip;
                }
                break;
            default:
                return 0;
        }
    }
#undef POINTER
#undef ULEB
    return 1;
}

/* The image is a position-independent Mach-O linked at 0x01000000: it loads there when that range was free when this
   library loaded, otherwise at another free range below 4 GiB (issues #22, #34: phones where something sits at the
   linked address), its pointers moved by the slide. Quick states hold raw pointers: their build id includes where
   the game's data is (state_ps5.c), so a state from another address is refused, not loaded. */
static int LoadImage(const char* region, Ps5GuestHeader* header) {
    char name[64];
    size_t size = 0, page = (size_t)sysconf(_SC_PAGESIZE); /* 4 or 16 KiB; the image's segments are 16 KiB aligned */
    SDL_snprintf(name, sizeof(name), "guest-%s.macho", region);
    uint8_t* file = SDL_LoadFile(name, &size);
    const MachHeader* mh = (const MachHeader*)file;
    const MachDyldInfo* dyld = NULL;
    uint32_t segAt[16], low = 0xFFFFFFFFu, high = 0, headerAt = 0, base, slide;
    const uint8_t* lc;
    int segs = 0;
    if (!file || size < sizeof(MachHeader) || mh->magic != 0xFEEDFACEu) {
        SDL_free(file);
        return ImageFail("the game image %s is missing from the APK", name);
    }
    lc = file + sizeof(MachHeader);
    for (uint32_t i = 0; i < mh->ncmds; i++, lc += ((const MachSegment*)lc)->cmdsize) {
        const MachSegment* seg = (const MachSegment*)lc;
        if (seg->cmd == 1 && segs < 16)
            segAt[segs++] = seg->vmaddr;
        if (seg->cmd == 0x80000022u) /* LC_DYLD_INFO_ONLY */
            dyld = (const MachDyldInfo*)lc;
        if (MappedSegment(seg)) {
            if (seg->vmaddr < low)
                low = seg->vmaddr;
            if (seg->vmaddr + seg->vmsize > high)
                high = seg->vmaddr + seg->vmsize;
        }
    }
    high = (uint32_t)((high + page - 1) & ~(page - 1));
    if (low >= high || !(mh->flags & 0x200000u) || !dyld || dyld->rebase_off + (uint64_t)dyld->rebase_size > size) {
        SDL_free(file);
        return ImageFail("the game image %s is not relocatable", name);
    }
    base = ClaimImageSpace(low, high - low);
    if (!base) {
        static uint64_t busy[1024][2];
        int n = BusyRanges(busy, 1024);
        for (int i = 0; i < n; i++)
            Log("  %08llx-%08llx", (unsigned long long)busy[i][0], (unsigned long long)busy[i][1]);
        SDL_free(file);
        return ImageFail("no free %u MB below 4 GB for the game image", (unsigned)((high - low) >> 20));
    }
    slide = base - low;
    lc = file + sizeof(MachHeader);
    for (uint32_t i = 0; i < mh->ncmds; i++, lc += ((const MachSegment*)lc)->cmdsize) {
        const MachSegment* seg = (const MachSegment*)lc;
        size_t len = (seg->vmsize + page - 1) & ~(page - 1);
        void* p;
        if (!MappedSegment(seg))
            continue;
        /* over the range claimed above (MAP_FIXED is safe here: the range is ours) */
        p = mmap((void*)(uintptr_t)(seg->vmaddr + slide), len, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED,
                 -1, 0);
        if (p != (void*)(uintptr_t)(seg->vmaddr + slide)) {
            SDL_free(file);
            return ImageFail("segment %.16s at 0x%08x: %s", seg->segname, seg->vmaddr + slide, strerror(errno));
        }
        if (seg->filesize)
            memcpy(p, file + seg->fileoff, seg->filesize);
        if (!strncmp(seg->segname, "__RKHDR", 16))
            headerAt = seg->vmaddr + slide;
    }
    if (slide && !Rebase(file + dyld->rebase_off, file + dyld->rebase_off + dyld->rebase_size, segAt, segs, slide)) {
        SDL_free(file);
        return ImageFail("the game image's relocations could not be applied");
    }
    /* the code becomes executable once its pointers are in place */
    lc = file + sizeof(MachHeader);
    for (uint32_t i = 0; i < mh->ncmds; i++, lc += ((const MachSegment*)lc)->cmdsize) {
        const MachSegment* seg = (const MachSegment*)lc;
        size_t len = (seg->vmsize + page - 1) & ~(page - 1);
        char* p = (char*)(uintptr_t)(seg->vmaddr + slide);
        if (MappedSegment(seg) && (seg->initprot & 4)) {
            __builtin___clear_cache(p, p + len);
            if (mprotect(p, len, PROT_READ | PROT_EXEC) && !ExecFromMemfd(p, len)) {
                SDL_free(file);
                return ImageFail("the game's code cannot be made executable: %s", strerror(errno));
            }
        }
    }
    SDL_free(file);
    if (!headerAt)
        return ImageFail("the game image %s has no header", name);
    memcpy(header, (const void*)(uintptr_t)headerAt, sizeof(*header));
    header->imageEnd = header->regionEnd = high + slide;
    Log("game image: %s at 0x%08x (%s), ends at 0x%08x", name, base, slide ? "moved: the linked address was taken" : "its linked address",
        high + slide);
    sImageLow = base;
    sImageHigh = high + slide;
    sImageSlide = slide;
    if (header->magic != PS5G_MAGIC)
        return ImageFail("the game image %s has another bridge version", name);
    return 1;
}

static const char* RegionOf(const char* rom) {
    static const struct { const char* code; const char* region; } known[] = { { "B8CP", "eu" }, { "B8CE", "us" }, { "B8CJ", "jp" } };
    char hdr[0xB0];
    SDL_IOStream* io = SDL_IOFromFile(rom, "rb");
    size_t got = io ? SDL_ReadIO(io, hdr, sizeof(hdr)) : 0;
    if (io)
        SDL_CloseIO(io);
    if (got != sizeof(hdr))
        return NULL;
    for (size_t i = 0; i < 3; i++)
        if (memcmp(hdr + 0xAC, known[i].code, 4) == 0)
            return known[i].region;
    return NULL;
}

int main(int argc, char** argv) {
    const char* rom = NULL;
    char logPath[900];
    for (int i = 1; i + 1 < argc; i++) {
        if (!strcmp(argv[i], "--data-dir"))
            SDL_strlcpy(sData, argv[++i], sizeof(sData));
        else if (!strcmp(argv[i], "--rom"))
            rom = argv[++i];
    }
    SDL_snprintf(logPath, sizeof(logPath), "%s/kh_com_host.log", sData);
    sLog = fopen(logPath, "w");
    Log("ReKHCoM (Android 64-bit) starting");
    LogDevice();
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD))
        Fatal(SDL_GetError());
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    sWindow = SDL_CreateWindow("Kingdom Hearts: Chain of Memories", 1280, 720, SDL_WINDOW_FULLSCREEN | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    sRenderer = sWindow ? SDL_CreateRenderer(sWindow, NULL) : NULL;
    if (!sRenderer)
        Fatal(SDL_GetError());
    SDL_SetRenderVSync(sRenderer, 1);
    Log("renderer: %s", SDL_GetRendererName(sRenderer));

    const char* region = rom ? RegionOf(rom) : NULL;
    if (!region)
        Fatal("Kingdom Hearts: Chain of Memories: the ROM could not be read.");
    SDL_setenv_unsafe("KHCOM_DATA", sData, 1);
    SDL_setenv_unsafe("KHCOM_ASSETS", "apk:", 1);
    SDL_setenv_unsafe("KHCOM_ROM", rom, 1);
    Ps5GuestHeader header;
    if (!LoadImage(region, &header)) {
        char text[400];
        SDL_snprintf(text, sizeof(text), "The game image could not be loaded (%s). Please send kh_com_host.log "
                     "(Share log) or a picture of this message.", sImageError);
        Fatal(text);
    }

    uint64_t* table = (uint64_t*)(uintptr_t)header.hostTable;
#define HF(name) [HF_##name] = (uint64_t)(uintptr_t)H_##name
#define HM(name, fn) [HF_##name] = (uint64_t)(uintptr_t)(fn)
    uint64_t t[HF_COUNT] = {
        HF(log), HF(exit), HF(ticks_us), HF(sleep_us), HF(getenv), HF(fopen), HF(fread), HF(fwrite), HF(fseek), HF(ftell),
        HF(fclose), HF(fflush), HF(stat), HF(remove), HF(rename), HF(mkdir), HF(dir_open), HF(dir_next), HF(dir_close),
        HF(gfx_size), HF(gfx_begin), HF(gfx_end), HF(tex_create), HF(tex_update), HF(tex_free), HF(gfx_draw), HF(gfx_clip),
        HF(gfx_blend), HF(audio_open), HF(audio_push), HF(audio_queued), HF(audio_clear), HF(audio_ratio), HF(pad_poll),
        HF(quit_requested),
        HM(sin, (double (*)(double))sin), HM(cos, (double (*)(double))cos), HM(tan, (double (*)(double))tan),
        HM(atan2, (double (*)(double, double))atan2), HM(pow, (double (*)(double, double))pow), HM(exp, (double (*)(double))exp),
        HM(log_e, (double (*)(double))log), HM(fmod, (double (*)(double, double))fmod), HM(sinf, (float (*)(float))sinf),
        HM(cosf, (float (*)(float))cosf), HM(powf, (float (*)(float, float))powf), HF(set_title), HF(notify),
        HF(touch_poll), HF(app_state), HF(hd_music), HF(hd_movie),
        HF(udp_open), HF(udp_resolve), HF(udp_send), HF(udp_recv),
    };
    for (int i = 0; i < HF_COUNT; ++i)
        if (!t[i]) {
            Log("bridge entry %d has no host function", i);
            return 1;
        }
    memcpy(table, t, sizeof(t));
    Log("starting the game (%d bridge entries)", HF_COUNT);
    CatchCrashes();
    int (*boot)(uint32_t) = (int (*)(uint32_t))(uintptr_t)header.boot;
    if (setjmp(sExit) == 0)
        sExitCode = boot(header.stackTop);
    if (sAudio)
        SDL_DestroyAudioStream(sAudio);
    SDL_DestroyRenderer(sRenderer);
    SDL_DestroyWindow(sWindow);
    SDL_Quit();
    return sExitCode;
}
