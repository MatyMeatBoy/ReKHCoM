/* Exercises the actual host bridge definitions with a native MS64 caller. */
#define SDL_MAIN_HANDLED
#define main host_main
#include "windows64_host.c"
#undef main
#include <assert.h>

/* The exact production consumer, with its small state/config surface isolated. */
#define PORT_ACT_COUNT 2
static struct { int keys[PORT_ACT_COUNT][2]; } testInputConfig;
static const uint16_t sActionBits[PORT_ACT_COUNT] = { 1, 8 };
static uint16_t sKeyboard;
static int sBattleMenuKeys, sBattleMenuChord, sZoomSelect, sZoomEat;
static int sDodgeHeld, sDodgePending, sUnstockHeld, sUnstockPending;
#define gPortConfig testInputConfig
/* Guest SDL subset defines this canonical event value used on the wire. */
#define SDL_EVENT_WINDOW_FOCUS_LOST 0x20c
#include "windows_input_event_test.inc"
#undef SDL_EVENT_WINDOW_FOCUS_LOST
#undef gPortConfig

static double __attribute__((sysv_abi, noinline)) GuestProbe(int a, double b, int c, float d, int e) {
    /* SysV permits these registers to change; MS64 callers require their preservation. */
    __asm__ volatile("pxor %%xmm6, %%xmm6; pxor %%xmm15, %%xmm15" ::: "xmm6", "xmm15", "rdi", "rsi");
    return a + b + c + d + e;
}
/* Kept out of line so the assembly probe observes the compiler's real MS->SysV prologue. */
__attribute__((noinline)) double Windows64InvokeProbe(void) {
    return GuestProbe(3, 4.25, 5, 6.5f, 7);
}
static int __attribute__((noinline)) NativeFive(int a, int b, int c, int d, int e) {
    return a + b * 10 + c * 100 + d * 1000 + e * 10000;
}
static int __attribute__((sysv_abi, noinline)) BridgeFive(int a, int b, int c, int d, int e) {
    return NativeFive(a, b, c, d, e);
}
extern int Windows64CheckPreserved(void);
int main(void) {
    assert(Windows64CheckPreserved());
    assert(BridgeFive(1, 2, 3, 4, 5) == 54321);
    typedef double (__attribute__((sysv_abi)) *Probe)(int, double, int, float, int);
    volatile Probe probe = GuestProbe;
    double result = probe(3, 4.25, 5, 6.5f, 7);
    assert(result == 25.75);
    assert(fabs(B_pow(2.0, 5.0) - 32.0) < 0.000001);
    assert(fabsf(B_powf(3.0f, 2.0f) - 9.0f) < 0.000001f);
    assert(B_thread(0, 1, 2, 3, 4) == 1);
    {
        char path[MAX_PATH], value[32];
        assert(GetTempPathA(sizeof(path), path));
        assert(B_stat(path) == 2);
        _putenv_s("REKHCOM_ABI_TEST", "mixed-arguments");
        assert(B_getenv("REKHCOM_ABI_TEST", value, sizeof(value)) == 15);
        assert(!strcmp(value, "mixed-arguments"));
        int d = B_dir_open(path);
        assert(d >= 0);
        B_dir_next(d, value, sizeof(value));
        B_dir_close(d);
    }
    {
        uint32_t pixels[4] = { 0xffffffff, 0xff000000, 0xff000000, 0xffffffff };
        char generation[64];
        _putenv_s("SDL_VIDEO_DRIVER", "dummy");
        assert(SDL_Init(SDL_INIT_VIDEO));
        sWindow = SDL_CreateWindow("ABI texture regression", 16, 16, SDL_WINDOW_HIDDEN);
        assert(sWindow);
        sRenderer = SDL_CreateRenderer(sWindow, NULL);
        assert(sRenderer);
        SDL_Event key = {0}; char eventText[96];
        key.type = SDL_EVENT_KEY_DOWN; key.key.key = SDLK_F5; key.key.down = true;
        assert(SDL_PushEvent(&key)); Pump();
        assert(B_getenv("KHCOM_HOST_KEY_EVENT", eventText, sizeof(eventText)) > 0);
        assert(!strcmp(eventText, "768,1073741886,0,0"));
        assert(B_getenv("KHCOM_HOST_KEY_EVENT", eventText, sizeof(eventText)) == 0);
        key.type = SDL_EVENT_KEY_UP; key.key.down = false;
        assert(SDL_PushEvent(&key)); Pump();
        assert(B_getenv("KHCOM_HOST_KEY_EVENT", eventText, sizeof(eventText)) > 0);
        assert(!strcmp(eventText, "769,1073741886,0,0"));
        for (int n = 0; n < 70; n++) QueueKey(&key);
        assert(B_getenv("KHCOM_HOST_KEY_EVENT", eventText, sizeof(eventText)) > 0);
        assert(!strncmp(eventText, "524,", 4)); /* focus lost clears held keys after queue overflow */
        while (B_getenv("KHCOM_HOST_KEY_EVENT", eventText, sizeof(eventText)) > 0) {}
        /* Guest saw held X + Enter before the movie; movie consumes their releases. */
        testInputConfig.keys[0][0] = SDLK_X;
        testInputConfig.keys[1][0] = SDLK_RETURN;
        SDL_Event held = {0}; held.type = SDL_EVENT_KEY_DOWN;
        held.key.key = SDLK_X; PortInputHandleEvent(&held);
        held.key.key = SDLK_RETURN; PortInputHandleEvent(&held);
        assert(sKeyboard == 9);
        sDodgeHeld = sDodgePending = sUnstockHeld = sUnstockPending = 1;
        DropPendingInput(); /* movie entry: guest is blocked until movie return */
        held.type = SDL_EVENT_KEY_UP;
        held.key.key = SDLK_X; assert(SDL_PushEvent(&held));
        held.key.key = SDLK_RETURN; assert(SDL_PushEvent(&held));
        int quit = 0; assert(!PauseWanted(&quit));
        assert(!quit && sKeyboard == 9); /* native releases never reached the guest */
        DropPendingInput(); /* movie exit must deliver exactly one reset */
        assert(B_getenv("KHCOM_HOST_KEY_EVENT", eventText, sizeof(eventText)) > 0);
        unsigned eventType, eventKey, eventMod, eventRepeat;
        assert(sscanf(eventText, "%u,%u,%u,%u", &eventType, &eventKey, &eventMod, &eventRepeat) == 4);
        assert(eventType == 0x20c);
        SDL_Event guestReset = {0}; guestReset.type = eventType;
        PortInputHandleEvent(&guestReset);
        assert(!sKeyboard && !sDodgeHeld && !sDodgePending && !sUnstockHeld && !sUnstockPending);
        assert(B_getenv("KHCOM_HOST_KEY_EVENT", eventText, sizeof(eventText)) == 0);
        int old = B_tex_create(pixels, 2, 2, 0);
        assert(old >= 0 && HostTextureSlot(old) >= 0);
        SDL_Event reset = {0};
        reset.type = SDL_EVENT_RENDER_DEVICE_RESET;
        assert(SDL_PushEvent(&reset));
        Pump();
        assert(HostTextureSlot(old) == -1);
        int fresh = B_tex_create(pixels, 2, 2, 0);
        assert(fresh >= 0 && fresh != old);
        B_tex_update(old, pixels); /* stale updates and frees cannot touch the new allocation */
        B_tex_free(old);
        assert(HostTextureSlot(fresh) >= 0);
        B_tex_free(fresh);
        assert(B_getenv("KHCOM_RENDERER_GENERATION", generation, sizeof(generation)) > 0);
        assert(!strcmp(generation, "bridge-textures-v1:1"));
        SDL_DestroyRenderer(sRenderer); sRenderer = NULL;
        SDL_DestroyWindow(sWindow); sWindow = NULL;
        SDL_Quit();
    }
    {
        char tmp[MAX_PATH], dest[1024], source[1024];
        assert(GetTempPathA(sizeof(tmp), tmp));
        snprintf(dest, sizeof(dest), "%srekhcom_abi_\xc3\xb1.sav", tmp);
        snprintf(source, sizeof(source), "%s.tmp", dest);
        int h = B_fopen(dest, 1); assert(h >= 0);
        assert(B_fwrite(h, "old", 3) == 3); assert(B_fclose(h) == 0);
        h = B_fopen(source, 1); assert(h >= 0);
        assert(B_fwrite(h, "new-save", 8) == 8); assert(B_fclose(h) == 0);
        assert(B_rename(source, dest) == 0);
        h = B_fopen(dest, 0); assert(h >= 0);
        char bytes[9] = {0};
        assert(B_fread(h, bytes, 8) == 8); assert(B_fclose(h) == 0);
        assert(!strcmp(bytes, "new-save"));
        assert(B_stat(source) == 0 && B_stat(dest) == 1);
        assert(B_remove(dest) == 0);
    }
    assert(MapFixed(0x02000000, 0x10000));
    *(volatile uint32_t*)(uintptr_t)0x02000000 = 0x12345678;
    assert(!MapFixed(0x02000000, 0x10000));
    assert(*(volatile uint32_t*)(uintptr_t)0x02000000 == 0x12345678);
    assert(VirtualFree((void*)(uintptr_t)0x02000000, 0, MEM_RELEASE));
    puts("Windows64 ABI, mixed argument/return and fixed mapping tests passed");
    return 0;
}
