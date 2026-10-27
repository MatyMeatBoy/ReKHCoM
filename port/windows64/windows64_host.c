/* Native Windows x64 SDL3 host for the flat x32 guest (64-bit instructions,
 * 32-bit game pointers). Every bridge entry has a typed SysV wrapper: SDL,
 * Windows and libc keep the native Microsoft x64 ABI. Game layouts stay x32.
 * Usage: rekhcom64.exe guest-eu.bin --data DIR
 * Initial milestone: HD music is shared; HD movies and UDP use guest fallbacks.
 */
#define _GNU_SOURCE
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <direct.h>
#include <io.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bridge.h"

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

/* Windows filesystem APIs consume UTF-16; the guest and SDL pass UTF-8. */
static int WidePath(const char* path, wchar_t* out, int cap) {
    return path && MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, out, cap) > 0;
}
static FILE* Utf8Open(const char* path, const char* mode) {
    wchar_t wp[4096], wm[16];
    if (!WidePath(path, wp, 4096) || !WidePath(mode, wm, 16)) return NULL;
    return _wfopen(wp, wm);
}
#define fopen Utf8Open

/* ---- the basics and files ---- */
static FILE* sFiles[64];
typedef struct { HANDLE handle; WIN32_FIND_DATAW entry; int first; } HostDir;
static HostDir sDirs[16];
static void H_log(const char* s) { fprintf(stderr, "%s\n", s); }
static void H_exit(int code) { fflush(NULL); SDL_Quit(); _exit(code); }
static uint64_t H_ticks_us(void) { return SDL_GetTicksNS() / 1000; }
static void H_sleep_us(uint32_t us) { SDL_DelayNS((Uint64)us * 1000); }
static uint32_t sRendererGeneration;
static int H_getenv(const char* name, char* out, int cap) {
    if (!strcmp(name, "KHCOM_RENDERER_GENERATION")) {
        if (cap <= 0) return -1;
        snprintf(out, (size_t)cap, "bridge-textures-v1:%u", sRendererGeneration);
        return (int)strlen(out);
    }
    const char* v = getenv(name);
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
static int H_stat(const char* p) {
    wchar_t wp[4096];
    if (!WidePath(p, wp, 4096)) return 0;
    DWORD attrs = GetFileAttributesW(wp);
    return attrs == INVALID_FILE_ATTRIBUTES ? 0 : (attrs & FILE_ATTRIBUTE_DIRECTORY) ? 2 : 1;
}
static int H_remove(const char* p) { wchar_t wp[4096]; return WidePath(p, wp, 4096) ? _wremove(wp) : -1; }
static int H_rename(const char* a, const char* b) {
    wchar_t wa[4096], wb[4096];
    if (!WidePath(a, wa, 4096) || !WidePath(b, wb, 4096)) return -1;
    return MoveFileExW(wa, wb, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) ? 0 : -1;
}
static int H_mkdir(const char* p) { wchar_t wp[4096]; return WidePath(p, wp, 4096) ? _wmkdir(wp) : -1; }
static int H_dir_open(const char* p) {
    char pattern[1200];
    if (snprintf(pattern, sizeof(pattern), "%s/*", p) >= (int)sizeof(pattern)) return -1;
    for (int i = 0; i < 16; i++) {
        if (!sDirs[i].handle) {
            wchar_t wp[4096];
            if (!WidePath(pattern, wp, 4096)) return -1;
            HANDLE h = FindFirstFileW(wp, &sDirs[i].entry);
            if (h == INVALID_HANDLE_VALUE) return -1;
            sDirs[i].handle = h;
            sDirs[i].first = 1;
            return i;
        }
    }
    return -1;
}
static int H_dir_next(int d, char* name, int cap) {
    HostDir* dir;
    if (d < 0 || d >= 16 || !sDirs[d].handle || cap <= 0) return 0;
    dir = &sDirs[d];
    do {
        if (dir->first) dir->first = 0;
        else if (!FindNextFileW(dir->handle, &dir->entry)) return 0;
        if (dir->entry.cFileName[0] == '.') continue;
        if (!WideCharToMultiByte(CP_UTF8, 0, dir->entry.cFileName, -1, name, cap, NULL, NULL)) return 0;
        return 1;
    } while (1);
}
static void H_dir_close(int d) {
    if (d >= 0 && d < 16 && sDirs[d].handle) {
        FindClose(sDirs[d].handle);
        sDirs[d].handle = NULL;
    }
}

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
#include "host_texture_slots.h"
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
/* UDP currently uses the cable fallback. */
#include "udp_host.h"
#include "thread_host.h"
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
static int H_hd_movie(const char* a, const char* b, const char* c, const char* d) { (void)a; (void)b; (void)c; (void)d; return 0; }

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
    void* wanted = (void*)(uintptr_t)base;
    void* p = VirtualAlloc(wanted, size, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (p != wanted) {
        if (p) VirtualFree(p, 0, MEM_RELEASE);
        Log("cannot map %08x (+%zx): Windows error %lu", base, size, GetLastError());
        return 0;
    }
    return 1;
}

static void __attribute__((sysv_abi)) B_log(const char* s) { H_log(s); }
static void __attribute__((sysv_abi)) B_exit(int code) { H_exit(code); }
static uint64_t __attribute__((sysv_abi)) B_ticks_us(void) { return H_ticks_us(); }
static void __attribute__((sysv_abi)) B_sleep_us(uint32_t us) { H_sleep_us(us); }
static int __attribute__((sysv_abi)) B_getenv(const char* name, char* out, int cap) { return H_getenv(name, out, cap); }
static int __attribute__((sysv_abi)) B_fopen(const char* path, int mode) { return H_fopen(path, mode); }
static int __attribute__((sysv_abi)) B_fread(int h, void* buf, uint32_t n) { return H_fread(h, buf, n); }
static int __attribute__((sysv_abi)) B_fwrite(int h, const void* buf, uint32_t n) { return H_fwrite(h, buf, n); }
static int __attribute__((sysv_abi)) B_fseek(int h, int32_t off, int whence) { return H_fseek(h, off, whence); }
static int __attribute__((sysv_abi)) B_ftell(int h) { return H_ftell(h); }
static int __attribute__((sysv_abi)) B_fclose(int h) { return H_fclose(h); }
static int __attribute__((sysv_abi)) B_fflush(int h) { return H_fflush(h); }
static int __attribute__((sysv_abi)) B_stat(const char* p) { return H_stat(p); }
static int __attribute__((sysv_abi)) B_remove(const char* p) { return H_remove(p); }
static int __attribute__((sysv_abi)) B_rename(const char* a, const char* b) { return H_rename(a, b); }
static int __attribute__((sysv_abi)) B_mkdir(const char* p) { return H_mkdir(p); }
static int __attribute__((sysv_abi)) B_dir_open(const char* p) { return H_dir_open(p); }
static int __attribute__((sysv_abi)) B_dir_next(int d, char* name, int cap) { return H_dir_next(d, name, cap); }
static void __attribute__((sysv_abi)) B_dir_close(int d) { H_dir_close(d); }
static void __attribute__((sysv_abi)) B_gfx_size(int* w, int* h) { H_gfx_size(w, h); }
static void __attribute__((sysv_abi)) B_gfx_begin(uint32_t clear) { H_gfx_begin(clear); }
static void __attribute__((sysv_abi)) B_gfx_end(void) { H_gfx_end(); }
static int __attribute__((sysv_abi)) B_tex_create(const void* rgba, int w, int h, int smooth) { return H_tex_create(rgba, w, h, smooth); }
static void __attribute__((sysv_abi)) B_tex_update(int id, const void* rgba) { H_tex_update(id, rgba); }
static void __attribute__((sysv_abi)) B_tex_free(int id) { H_tex_free(id); }
static void __attribute__((sysv_abi)) B_gfx_draw(int id, const void* verts, int count) { H_gfx_draw(id, verts, count); }
static void __attribute__((sysv_abi)) B_gfx_clip(int x, int y, int w, int h) { H_gfx_clip(x, y, w, h); }
static void __attribute__((sysv_abi)) B_gfx_blend(int on) { H_gfx_blend(on); }
static int __attribute__((sysv_abi)) B_audio_open(int rate) { return H_audio_open(rate); }
static void __attribute__((sysv_abi)) B_audio_push(const int16_t* frames, uint32_t count) { H_audio_push(frames, count); }
static uint32_t __attribute__((sysv_abi)) B_audio_queued(void) { return H_audio_queued(); }
static void __attribute__((sysv_abi)) B_audio_clear(void) { H_audio_clear(); }
static void __attribute__((sysv_abi)) B_audio_ratio(double ratio) { H_audio_ratio(ratio); }
static int __attribute__((sysv_abi)) B_pad_poll(HostPad* out, int count) { return H_pad_poll(out, count); }
static int __attribute__((sysv_abi)) B_quit_requested(void) { return H_quit_requested(); }
static double __attribute__((sysv_abi)) B_sin(double a) { return sin(a); }
static double __attribute__((sysv_abi)) B_cos(double a) { return cos(a); }
static double __attribute__((sysv_abi)) B_tan(double a) { return tan(a); }
static double __attribute__((sysv_abi)) B_atan2(double a, double b) { return atan2(a, b); }
static double __attribute__((sysv_abi)) B_pow(double a, double b) { return pow(a, b); }
static double __attribute__((sysv_abi)) B_exp(double a) { return exp(a); }
static double __attribute__((sysv_abi)) B_log_e(double a) { return log(a); }
static double __attribute__((sysv_abi)) B_fmod(double a, double b) { return fmod(a, b); }
static float __attribute__((sysv_abi)) B_sinf(float a) { return sinf(a); }
static float __attribute__((sysv_abi)) B_cosf(float a) { return cosf(a); }
static float __attribute__((sysv_abi)) B_powf(float a, float b) { return powf(a, b); }
static void __attribute__((sysv_abi)) B_set_title(const char* s) { H_set_title(s); }
static void __attribute__((sysv_abi)) B_notify(const char* name) { H_notify(name); }
static int __attribute__((sysv_abi)) B_touch_poll(HostTouch* fingers, int max) { return H_touch_poll(fingers, max); }
static int __attribute__((sysv_abi)) B_app_state(void) { return H_app_state(); }
static int __attribute__((sysv_abi)) B_hd_music(int op, const char* path, int gain) { return H_hd_music(op, path, gain); }
static int __attribute__((sysv_abi)) B_hd_movie(const char* a, const char* b, const char* c, const char* d) { return H_hd_movie(a, b, c, d); }
static int __attribute__((sysv_abi)) B_udp_open(int port) { return H_udp_open(port); }
static int __attribute__((sysv_abi)) B_udp_resolve(const char* host, int port, uint8_t* addr) { return H_udp_resolve(host, port, addr); }
static void __attribute__((sysv_abi)) B_udp_send(const uint8_t* addr, const void* data, int size) { H_udp_send(addr, data, size); }
static int __attribute__((sysv_abi)) B_udp_recv(void* data, int cap, uint8_t* addr) { return H_udp_recv(data, cap, addr); }
static int __attribute__((sysv_abi)) B_thread(int op, uint32_t a, uint32_t b, uint32_t c, uint32_t d) { return H_thread(op, a, b, c, d); }

#define HF(name) [HF_##name] = (uint64_t)(uintptr_t)B_##name
#define HM(name, fn) [HF_##name] = (uint64_t)(uintptr_t)B_##name

int main(int argc, char** argv) {
    const char* image = "guest-eu.bin";
    const char* bootStatus = NULL;
    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        if ((!strcmp(arg, "--data") || !strcmp(arg, "--data-dir")) && i + 1 < argc)
            snprintf(sData, sizeof(sData), "%s", argv[++i]);
        else if (!strcmp(arg, "--rom") && i + 1 < argc) _putenv_s("KHCOM_ROM", argv[++i]);
        else if (!strcmp(arg, "--language") && i + 1 < argc) _putenv_s("KHCOM_LANGUAGE", argv[++i]);
        else if (!strcmp(arg, "--boot-status") && i + 1 < argc) bootStatus = argv[++i];
        else if (arg[0] != '-') image = arg;
        else { Log("unknown or incomplete argument: %s", arg); return 2; }
    }
    /* The launcher supplies a unique marker path. Remove a stale marker before any boot checks. */
    if (bootStatus) H_remove(bootStatus);
    _putenv_s("KHCOM_DATA", sData);
    {
        /* the game's packaged files, its HD text fonts among them, as the other hosts say (#277: without it the
           Steam Deck's HD text found no font and the dialogue showed none) */
        char assets[1100];
        snprintf(assets, sizeof(assets), "%s/assets", sData);
        if (!getenv("KHCOM_ASSETS")) _putenv_s("KHCOM_ASSETS", assets);
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
    if (size < (long)sizeof(Ps5GuestHeader)) { fclose(f); return 2; }
    if (!tmp || fread(tmp, 1, (size_t)size, f) != (size_t)size)
        return 2;
    fclose(f);
    Ps5GuestHeader h;
    memcpy(&h, tmp, sizeof(h));
    if (h.magic != PS5G_MAGIC || h.regionEnd <= PS5G_BASE || h.regionEnd > 0xF0000000u ||
        (uint64_t)size > (uint64_t)h.regionEnd - PS5G_BASE ||
        h.boot < PS5G_BASE || h.boot >= h.regionEnd ||
        h.hostTable < PS5G_BASE || (uint64_t)h.hostTable + HF_COUNT * 8 > h.regionEnd ||
        h.stackTop <= PS5G_BASE || h.stackTop > h.regionEnd) {
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
                               SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY |
                               (getenv("KHCOM_HOST_HIDDEN") ? SDL_WINDOW_HIDDEN : 0));
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
    typedef int (__attribute__((sysv_abi)) *GuestBoot)(uint32_t);
    GuestBoot boot = (GuestBoot)(uintptr_t)h.boot;
    if (bootStatus) {
        FILE* marker = fopen(bootStatus, "wb");
        if (!marker) { Log("cannot create boot status marker"); SDL_Quit(); return 4; }
        int wrote = fwrite("guest-entered\n", 1, 14, marker) == 14;
        int flushed = fflush(marker) == 0 && _commit(_fileno(marker)) == 0;
        int closed = fclose(marker) == 0;
        if (!wrote || !flushed || !closed) {
            H_remove(bootStatus); SDL_Quit(); return 4;
        }
    }
    int rc = boot(h.stackTop);
    SDL_Quit();
    return rc;
}
