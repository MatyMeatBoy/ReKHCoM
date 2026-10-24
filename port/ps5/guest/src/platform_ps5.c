/* What the PS5 build adds to the desktop port: the settings it starts with, the MMPX switch, the process entry. */
#include <SDL3/SDL.h>
#include <stdint.h>
#include "port/port.h"
#include "port/port_state.h"
#include "port/port_ui.h"
#include "bridge_calls.h"

int gPortMmpx; /* the MMPX 3.2 filter on the game picture (Options, saved as mmpx) */

FILE* popen(const char* cmd, const char* mode) { (void)cmd; (void)mode; return NULL; }
int pclose(FILE* f) { (void)f; return -1; }

int main(int argc, char** argv);

/* Without a settings file the game starts with HD text and the audio filter on (HD text needs the Latin fonts: the
 * Japanese version keeps the cartridge's text). */
static void WriteDefaultSettings(void) {
    const char* path = PortConfigPath("kh_com.ini");
    FILE* f;

    if (h_stat(path) != 0)
        return;
    f = fopen(path, "w");
    if (f == NULL)
        return;
#ifdef VERSION_JP
    fputs("hd_text=0\n", f);
#else
    fputs("hd_text=1\n", f);
#endif
    fputs("smooth_audio=1\n", f);
    fclose(f);
}

/* The Japanese version has no HD text (no CJK font); called every frame in case the settings file says otherwise. */
void PortPs5LockSettings(void) {
#ifdef VERSION_JP
    gPortConfig.hdText = 0;
#endif
}

/* Called on the guest stack by ps5_boot. */
int guest_main(void) {
    static char name[] = "ReKHCoM";
    static char* argv[] = { name, NULL };
    static char rom[1024];
    extern const char* gPortRomPath;
    /* the cartridge the host was given (Android: the copy the launcher made), else the data folder's */
    if (h_getenv("KHCOM_ROM", rom, sizeof(rom)) > 0)
        gPortRomPath = rom;
    WriteDefaultSettings();
    return main(1, argv);
}
