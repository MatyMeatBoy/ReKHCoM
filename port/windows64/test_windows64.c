/* Exercises the actual host bridge definitions with a native MS64 caller. */
#define SDL_MAIN_HANDLED
#define main host_main
#include "windows64_host.c"
#undef main
#include <assert.h>

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
