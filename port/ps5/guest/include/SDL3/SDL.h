/* The part of SDL3 the game's platform layer uses, drawn through the host
 * bridge (guest/src/sdl_shim.c). Names and values follow SDL3 where they reach
 * the configuration file (key codes, gamepad buttons). */
#ifndef SDL_h_
#define SDL_h_

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>

typedef uint8_t Uint8;
typedef uint16_t Uint16;
typedef uint32_t Uint32;
typedef uint64_t Uint64;
typedef int8_t Sint8;
typedef int16_t Sint16;
typedef int32_t Sint32;
typedef int64_t Sint64;

#define SDL_MAJOR_VERSION 3
#define SDL_MINOR_VERSION 4
#define SDL_MICRO_VERSION 0
#define SDL_PI_F 3.14159265358979323846f

#define SDL_min(a, b) (((a) < (b)) ? (a) : (b))
#define SDL_max(a, b) (((a) > (b)) ? (a) : (b))
#define SDL_clamp(x, a, b) (((x) < (a)) ? (a) : (((x) > (b)) ? (b) : (x)))
#define SDL_abs(x) ((x) < 0 ? -(x) : (x))
#define SDL_arraysize(a) (sizeof(a) / sizeof((a)[0]))
#define SDL_zero(x) memset(&(x), 0, sizeof((x)))
#define SDL_malloc malloc
#define SDL_calloc calloc
#define SDL_realloc realloc
#define SDL_free free
#define SDL_memset memset
#define SDL_memcpy memcpy
#define SDL_memmove memmove
#define SDL_memcmp memcmp
#define SDL_strlen strlen
#define SDL_strcmp strcmp
#define SDL_strncmp strncmp
#define SDL_strchr strchr
#define SDL_strrchr strrchr
#define SDL_strstr strstr
#define SDL_atoi atoi
#define SDL_snprintf snprintf
#define SDL_sscanf sscanf
#define SDL_qsort qsort
#define SDL_getenv getenv
#define SDL_isspace isspace
#define SDL_floorf __builtin_floorf
#define SDL_floor __builtin_floor
#define SDL_ceilf __builtin_ceilf
#define SDL_roundf __builtin_roundf
#define SDL_fabsf __builtin_fabsf
#define SDL_sinf sinf
#define SDL_cosf cosf
#define SDL_powf powf
#define SDL_sqrtf __builtin_sqrtf
#define SDL_expf(x) ((float)exp((double)(x)))
size_t SDL_strlcpy(char* dst, const char* src, size_t size);
size_t SDL_strlcat(char* dst, const char* src, size_t size);

#define SDL_INIT_AUDIO 0x10u
#define SDL_INIT_VIDEO 0x20u
#define SDL_INIT_GAMEPAD 0x2000u
#define SDL_INIT_EVENTS 0x4000u
bool SDL_Init(Uint32 flags);
void SDL_Quit(void);
const char* SDL_GetError(void);
#define SDL_INIT_INTERFACE(iface) do { memset((iface), 0, sizeof(*(iface))); (iface)->version = sizeof(*(iface)); } while (0)

Uint64 SDL_GetTicks(void);
Uint64 SDL_GetPerformanceCounter(void);
Uint64 SDL_GetPerformanceFrequency(void);
void SDL_Delay(Uint32 ms);
void SDL_DelayPrecise(Uint64 ns);

/* Threads, semaphores and atomics: the PPU's other cores on the Switch (port_ppu.c with PORT_PPU_THREADS,
 * port/switch/guest/stubs_a64.c); other images have none (SDL_GetNumLogicalCPUCores 1, SDL_CreateThread NULL). */
#define SDLCALL
typedef struct SDL_Thread SDL_Thread;
typedef struct SDL_Semaphore SDL_Semaphore;
typedef int (*SDL_ThreadFunction)(void* data);
typedef struct SDL_AtomicInt { int value; } SDL_AtomicInt;
int SDL_GetNumLogicalCPUCores(void);
SDL_Thread* SDL_CreateThread(SDL_ThreadFunction fn, const char* name, void* data);
void SDL_DetachThread(SDL_Thread* thread);
SDL_Semaphore* SDL_CreateSemaphore(Uint32 initial);
void SDL_WaitSemaphore(SDL_Semaphore* sem);
void SDL_SignalSemaphore(SDL_Semaphore* sem);
static inline int SDL_AddAtomicInt(SDL_AtomicInt* a, int v) { return __atomic_fetch_add(&a->value, v, __ATOMIC_SEQ_CST); }
static inline int SDL_SetAtomicInt(SDL_AtomicInt* a, int v) { return __atomic_exchange_n(&a->value, v, __ATOMIC_SEQ_CST); }
static inline int SDL_GetAtomicInt(SDL_AtomicInt* a) { return __atomic_load_n(&a->value, __ATOMIC_SEQ_CST); }
const char* SDL_GetBasePath(void);
bool SDL_CreateDirectory(const char* path);
void* SDL_LoadFile(const char* file, size_t* datasize);

/* ---- geometry and pixels ---- */
typedef struct SDL_Rect { int x, y, w, h; } SDL_Rect;
typedef struct SDL_FRect { float x, y, w, h; } SDL_FRect;
typedef struct SDL_FPoint { float x, y; } SDL_FPoint;
typedef struct SDL_FColor { float r, g, b, a; } SDL_FColor;
typedef struct SDL_Vertex { SDL_FPoint position; SDL_FColor color; SDL_FPoint tex_coord; } SDL_Vertex;

typedef enum SDL_PixelFormat {
    SDL_PIXELFORMAT_UNKNOWN = 0,
    SDL_PIXELFORMAT_ARGB8888 = 1, /* bytes B, G, R, A */
    SDL_PIXELFORMAT_XRGB8888 = 2, /* bytes B, G, R, x */
    SDL_PIXELFORMAT_RGBA32 = 3,   /* bytes R, G, B, A */
    SDL_PIXELFORMAT_BGRA32 = 4    /* bytes B, G, R, A */
} SDL_PixelFormat;

typedef enum SDL_ScaleMode {
    SDL_SCALEMODE_NEAREST = 0,
    SDL_SCALEMODE_LINEAR = 1,
    SDL_SCALEMODE_PIXELART = 2
} SDL_ScaleMode;
typedef enum SDL_FlipMode {
    SDL_FLIP_NONE = 0,
    SDL_FLIP_HORIZONTAL = 1,
    SDL_FLIP_VERTICAL = 2
} SDL_FlipMode;
typedef Uint32 SDL_BlendMode;
#define SDL_BLENDMODE_NONE 0u
#define SDL_BLENDMODE_BLEND 1u
#define SDL_BLENDMODE_ADD 2u /* the host blends on or off: drawn as BLEND (Re:CoM's layouts, port_hdlayout.c) */
typedef enum SDL_TextureAccess {
    SDL_TEXTUREACCESS_STATIC = 0,
    SDL_TEXTUREACCESS_STREAMING = 1
} SDL_TextureAccess;

typedef struct SDL_Surface {
    Uint32 flags;
    SDL_PixelFormat format;
    int w, h, pitch;
    void* pixels;
    int owned;
} SDL_Surface;
SDL_Surface* SDL_CreateSurface(int w, int h, SDL_PixelFormat format);
SDL_Surface* SDL_CreateSurfaceFrom(int w, int h, SDL_PixelFormat format, void* pixels, int pitch);
SDL_Surface* SDL_ConvertSurface(SDL_Surface* surface, SDL_PixelFormat format);
void SDL_DestroySurface(SDL_Surface* surface);
bool SDL_SetSurfaceBlendMode(SDL_Surface* surface, SDL_BlendMode mode);
bool SDL_BlitSurfaceScaled(SDL_Surface* src, const SDL_Rect* srcrect, SDL_Surface* dst, const SDL_Rect* dstrect,
                           SDL_ScaleMode scaleMode);
bool SDL_SaveBMP(SDL_Surface* surface, const char* file);

/* ---- window and renderer ---- */
typedef Uint32 SDL_DisplayID;
typedef struct SDL_DisplayMode { SDL_DisplayID displayID; SDL_PixelFormat format; int w, h; float pixel_density; float refresh_rate; } SDL_DisplayMode;
typedef struct SDL_Window SDL_Window;
typedef struct SDL_Renderer SDL_Renderer;
typedef struct SDL_Texture { int w, h; int refcount; } SDL_Texture;
#define SDL_WINDOW_FULLSCREEN 0x1u
#define SDL_WINDOW_RESIZABLE 0x20u
#define SDL_WINDOW_HIGH_PIXEL_DENSITY 0x2000u
SDL_Window* SDL_CreateWindow(const char* title, int w, int h, Uint64 flags);
SDL_Renderer* SDL_CreateRenderer(SDL_Window* window, const char* name);
SDL_Renderer* SDL_GetRenderer(SDL_Window* window);
const char* SDL_GetRendererName(SDL_Renderer* renderer);
SDL_Window** SDL_GetWindows(int* count);
bool SDL_SyncWindow(SDL_Window* window);
bool SDL_SetWindowFullscreen(SDL_Window* window, bool fullscreen);
bool SDL_SetWindowFullscreenMode(SDL_Window* window, const SDL_DisplayMode* mode);
const SDL_DisplayMode* SDL_GetWindowFullscreenMode(SDL_Window* window);
bool SDL_SetWindowSize(SDL_Window* window, int w, int h);
bool SDL_SetWindowMinimumSize(SDL_Window* window, int w, int h);
bool SDL_GetWindowSizeInPixels(SDL_Window* window, int* w, int* h);
Uint64 SDL_GetWindowFlags(SDL_Window* window);
SDL_DisplayID SDL_GetDisplayForWindow(SDL_Window* window);
SDL_DisplayMode** SDL_GetFullscreenDisplayModes(SDL_DisplayID display, int* count);
bool SDL_GetClosestFullscreenDisplayMode(SDL_DisplayID display, int w, int h, float refresh, bool includeHighDensity,
                                         SDL_DisplayMode* closest);
const char* SDL_GetCurrentVideoDriver(void);
#define SDL_MESSAGEBOX_ERROR 0x10u
bool SDL_ShowSimpleMessageBox(Uint32 flags, const char* title, const char* message, SDL_Window* window);

bool SDL_SetRenderVSync(SDL_Renderer* renderer, int vsync);
bool SDL_GetRenderVSync(SDL_Renderer* renderer, int* vsync);
bool SDL_GetCurrentRenderOutputSize(SDL_Renderer* renderer, int* w, int* h);
bool SDL_SetRenderDrawColor(SDL_Renderer* renderer, Uint8 r, Uint8 g, Uint8 b, Uint8 a);
bool SDL_SetRenderDrawBlendMode(SDL_Renderer* renderer, SDL_BlendMode mode);
bool SDL_SetRenderViewport(SDL_Renderer* renderer, const SDL_Rect* rect);
bool SDL_SetRenderClipRect(SDL_Renderer* renderer, const SDL_Rect* rect);
bool SDL_SetRenderScale(SDL_Renderer* renderer, float x, float y);
bool SDL_GetRenderScale(SDL_Renderer* renderer, float* x, float* y);
bool SDL_RenderClear(SDL_Renderer* renderer);
bool SDL_RenderFillRect(SDL_Renderer* renderer, const SDL_FRect* rect);
bool SDL_RenderFillRects(SDL_Renderer* renderer, const SDL_FRect* rects, int count);
bool SDL_RenderTexture(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_FRect* src, const SDL_FRect* dst);
bool SDL_RenderTextureRotated(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_FRect* src, const SDL_FRect* dst,
                              double angle, const SDL_FPoint* center, SDL_FlipMode flip);
bool SDL_RenderGeometry(SDL_Renderer* renderer, SDL_Texture* texture, const SDL_Vertex* vertices, int numVertices,
                        const int* indices, int numIndices);
bool SDL_RenderPresent(SDL_Renderer* renderer);
SDL_Surface* SDL_RenderReadPixels(SDL_Renderer* renderer, const SDL_Rect* rect);
bool SDL_RenderDebugText(SDL_Renderer* renderer, float x, float y, const char* str);

SDL_Texture* SDL_CreateTexture(SDL_Renderer* renderer, SDL_PixelFormat format, SDL_TextureAccess access, int w, int h);
SDL_Texture* SDL_CreateTextureFromSurface(SDL_Renderer* renderer, SDL_Surface* surface);
void SDL_DestroyTexture(SDL_Texture* texture);
bool SDL_UpdateTexture(SDL_Texture* texture, const SDL_Rect* rect, const void* pixels, int pitch);
bool SDL_SetTextureScaleMode(SDL_Texture* texture, SDL_ScaleMode mode);
bool SDL_SetTextureBlendMode(SDL_Texture* texture, SDL_BlendMode mode);
bool SDL_SetTextureColorMod(SDL_Texture* texture, Uint8 r, Uint8 g, Uint8 b);
bool SDL_SetTextureAlphaMod(SDL_Texture* texture, Uint8 a);

/* ---- audio ---- */
typedef Uint32 SDL_AudioFormat;
#define SDL_AUDIO_F32 0x8120u
typedef Uint32 SDL_AudioDeviceID;
#define SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK 0xFFFFFFFFu
typedef struct SDL_AudioSpec { SDL_AudioFormat format; int channels; int freq; } SDL_AudioSpec;
typedef struct SDL_AudioStream SDL_AudioStream;
typedef void (*SDL_AudioStreamCallback)(void* userdata, SDL_AudioStream* stream, int additional, int total);
SDL_AudioStream* SDL_OpenAudioDeviceStream(SDL_AudioDeviceID devid, const SDL_AudioSpec* spec,
                                           SDL_AudioStreamCallback callback, void* userdata);
void SDL_DestroyAudioStream(SDL_AudioStream* stream);
bool SDL_PauseAudioStreamDevice(SDL_AudioStream* stream);
bool SDL_ResumeAudioStreamDevice(SDL_AudioStream* stream);
bool SDL_ClearAudioStream(SDL_AudioStream* stream);
bool SDL_SetAudioStreamFrequencyRatio(SDL_AudioStream* stream, float ratio);
bool SDL_SetAudioStreamGain(SDL_AudioStream* stream, float gain);
bool SDL_SetAudioStreamFormat(SDL_AudioStream* stream, const SDL_AudioSpec* srcSpec, const SDL_AudioSpec* dstSpec);
bool SDL_GetAudioStreamFormat(SDL_AudioStream* stream, SDL_AudioSpec* srcSpec, SDL_AudioSpec* dstSpec);
bool SDL_PutAudioStreamData(SDL_AudioStream* stream, const void* buf, int len);
int SDL_GetAudioStreamQueued(SDL_AudioStream* stream);

/* ---- events and gamepads ---- */
typedef Uint32 SDL_JoystickID;
typedef Uint32 SDL_Keycode;
typedef Uint32 SDL_Keymod;
typedef Uint64 SDL_FingerID;
typedef int SDL_Scancode;
#define SDL_KMOD_ALT 0x0300u
#define SDLK_ESCAPE 0x1Bu
#define SDLK_BACKSPACE 0x08u
#define SDLK_SPACE 0x20u
#define SDLK_A 0x61u
#define SDLK_C 0x63u
#define SDLK_D 0x64u
#define SDLK_J 0x6Au
#define SDLK_K 0x6Bu
#define SDLK_S 0x73u
#define SDLK_V 0x76u
#define SDLK_W 0x77u
#define SDLK_E 0x65u
#define SDLK_Q 0x71u
#define SDLK_PAGEUP 0x4000004Bu
#define SDLK_PAGEDOWN 0x4000004Eu
#define SDLK_X 0x78u
#define SDLK_TAB 0x09u
#define SDLK_Z 0x7Au
#define SDLK_RIGHT 0x4000004Fu
#define SDLK_LEFT 0x40000050u
#define SDLK_DOWN 0x40000051u
#define SDLK_UP 0x40000052u
#define SDLK_RSHIFT 0x400000E5u
#define SDLK_RETURN 0x0Du
#define SDLK_F1 0x4000003Au
#define SDLK_F5 0x4000003Eu
#define SDLK_F6 0x4000003Fu
#define SDLK_F7 0x40000040u
#define SDLK_F8 0x40000041u
#define SDLK_F9 0x40000042u
#define SDLK_F11 0x40000044u
#define SDLK_PLUS 0x0000002Bu
#define SDLK_MINUS 0x0000002Du
#define SDLK_EQUALS 0x0000003Du
#define SDLK_KP_MINUS 0x40000056u
#define SDLK_KP_PLUS 0x40000057u
#define SDLK_DELETE 0x0000007Fu
#define SDLK_AC_BACK 0x4000011Au

typedef enum SDL_EventType {
    SDL_EVENT_QUIT = 0x100,
    SDL_EVENT_WILL_ENTER_BACKGROUND = 0x103,
    SDL_EVENT_DID_ENTER_BACKGROUND = 0x104,
    SDL_EVENT_DID_ENTER_FOREGROUND = 0x106,
    SDL_EVENT_WINDOW_FOCUS_LOST = 0x20C,
    SDL_EVENT_KEY_DOWN = 0x300,
    SDL_EVENT_KEY_UP = 0x301,
    SDL_EVENT_MOUSE_MOTION = 0x400,
    SDL_EVENT_MOUSE_BUTTON_DOWN = 0x401,
    SDL_EVENT_MOUSE_BUTTON_UP = 0x402,
    SDL_EVENT_GAMEPAD_BUTTON_DOWN = 0x650,
    SDL_EVENT_GAMEPAD_BUTTON_UP = 0x651,
    SDL_EVENT_GAMEPAD_AXIS_MOTION = 0x652,
    SDL_EVENT_GAMEPAD_ADDED = 0x653,
    SDL_EVENT_FINGER_DOWN = 0x700,
    SDL_EVENT_FINGER_UP = 0x701,
    SDL_EVENT_FINGER_MOTION = 0x702,
    SDL_EVENT_FINGER_CANCELED = 0x703,
    SDL_EVENT_RENDER_DEVICE_RESET = 0x2001
} SDL_EventType;

typedef struct SDL_KeyboardEvent { Uint32 type; Uint32 reserved; Uint64 timestamp; Uint32 windowID; Uint32 which; SDL_Scancode scancode; SDL_Keycode key; SDL_Keymod mod; Uint16 raw; bool down; bool repeat; } SDL_KeyboardEvent;
typedef struct SDL_GamepadButtonEvent { Uint32 type; Uint32 reserved; Uint64 timestamp; SDL_JoystickID which; Uint8 button; bool down; } SDL_GamepadButtonEvent;
typedef struct SDL_GamepadAxisEvent { Uint32 type; Uint32 reserved; Uint64 timestamp; SDL_JoystickID which; Uint8 axis; Uint8 padding1, padding2, padding3; Sint16 value; Uint16 padding4; } SDL_GamepadAxisEvent;
typedef struct SDL_GamepadDeviceEvent { Uint32 type; Uint32 reserved; Uint64 timestamp; SDL_JoystickID which; } SDL_GamepadDeviceEvent;
typedef struct SDL_TouchFingerEvent { Uint32 type; Uint32 reserved; Uint64 timestamp; Uint64 touchID; SDL_FingerID fingerID; float x, y, dx, dy, pressure; Uint32 windowID; } SDL_TouchFingerEvent;
typedef union SDL_Event {
    Uint32 type;
    SDL_KeyboardEvent key;
    SDL_GamepadButtonEvent gbutton;
    SDL_GamepadAxisEvent gaxis;
    SDL_GamepadDeviceEvent gdevice;
    SDL_TouchFingerEvent tfinger;
    Uint8 padding[128];
} SDL_Event;
bool SDL_PollEvent(SDL_Event* event);
const bool* SDL_GetKeyboardState(int* numkeys);
SDL_Scancode SDL_GetScancodeFromKey(SDL_Keycode key, SDL_Keymod* modstate);

typedef struct SDL_Gamepad SDL_Gamepad;
typedef enum SDL_GamepadType {
    SDL_GAMEPAD_TYPE_UNKNOWN, SDL_GAMEPAD_TYPE_STANDARD, SDL_GAMEPAD_TYPE_XBOX360, SDL_GAMEPAD_TYPE_XBOXONE,
    SDL_GAMEPAD_TYPE_PS3, SDL_GAMEPAD_TYPE_PS4, SDL_GAMEPAD_TYPE_PS5, SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_PRO,
    SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_LEFT, SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_RIGHT,
    SDL_GAMEPAD_TYPE_NINTENDO_SWITCH_JOYCON_PAIR, SDL_GAMEPAD_TYPE_COUNT
} SDL_GamepadType;
SDL_GamepadType SDL_GetGamepadType(SDL_Gamepad* gamepad);
typedef struct SDL_Joystick SDL_Joystick;
typedef enum SDL_GamepadButton {
    SDL_GAMEPAD_BUTTON_INVALID = -1,
    SDL_GAMEPAD_BUTTON_SOUTH, SDL_GAMEPAD_BUTTON_EAST, SDL_GAMEPAD_BUTTON_WEST, SDL_GAMEPAD_BUTTON_NORTH,
    SDL_GAMEPAD_BUTTON_BACK, SDL_GAMEPAD_BUTTON_GUIDE, SDL_GAMEPAD_BUTTON_START,
    SDL_GAMEPAD_BUTTON_LEFT_STICK, SDL_GAMEPAD_BUTTON_RIGHT_STICK,
    SDL_GAMEPAD_BUTTON_LEFT_SHOULDER, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER,
    SDL_GAMEPAD_BUTTON_DPAD_UP, SDL_GAMEPAD_BUTTON_DPAD_DOWN, SDL_GAMEPAD_BUTTON_DPAD_LEFT, SDL_GAMEPAD_BUTTON_DPAD_RIGHT,
    SDL_GAMEPAD_BUTTON_MISC1,
    SDL_GAMEPAD_BUTTON_COUNT = 26
} SDL_GamepadButton;
typedef enum SDL_GamepadAxis {
    SDL_GAMEPAD_AXIS_LEFTX, SDL_GAMEPAD_AXIS_LEFTY, SDL_GAMEPAD_AXIS_RIGHTX, SDL_GAMEPAD_AXIS_RIGHTY,
    SDL_GAMEPAD_AXIS_LEFT_TRIGGER, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER, SDL_GAMEPAD_AXIS_COUNT
} SDL_GamepadAxis;
SDL_JoystickID* SDL_GetGamepads(int* count);
SDL_Gamepad* SDL_GetGamepadFromID(SDL_JoystickID id);
SDL_Gamepad* SDL_OpenGamepad(SDL_JoystickID id);
const char* SDL_GetGamepadStringForButton(SDL_GamepadButton button);
const char* SDL_GetKeyName(SDL_Keycode key);
bool SDL_GetGamepadButton(SDL_Gamepad* gamepad, SDL_GamepadButton button);
Sint16 SDL_GetGamepadAxis(SDL_Gamepad* gamepad, SDL_GamepadAxis axis);
void SDL_SetGamepadEventsEnabled(bool enabled);

/* The virtual pad of the automated tests does not exist here. */
#define SDL_JOYSTICK_TYPE_GAMEPAD 1
typedef struct SDL_VirtualJoystickDesc { Uint32 version; Uint16 type; Uint16 naxes; Uint16 nbuttons; Uint16 nhats; const char* name; } SDL_VirtualJoystickDesc;
SDL_JoystickID SDL_AttachVirtualJoystick(const SDL_VirtualJoystickDesc* desc);
SDL_Joystick* SDL_OpenJoystick(SDL_JoystickID id);
bool SDL_SetJoystickVirtualButton(SDL_Joystick* joystick, int button, bool down);
bool SDL_SetJoystickVirtualAxis(SDL_Joystick* joystick, int axis, Sint16 value);
void SDL_UpdateJoysticks(void);

#endif
