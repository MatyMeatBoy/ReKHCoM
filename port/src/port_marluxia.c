/* Marluxia's last fight in the wide view (Preliminar V2 of the boss arenas, waits for the user's approval).
 * BATTLE_SCENERY_AUDIT.md #7: the field and the body cut at the picture; the wide view switched on and off within the
 * fight (only while the effect layer BG2 was on), so the picture jumped between two sizes.
 *
 * The field (BG0, bos_lst_fld.c) is a 256x256 screen background loaded into VRAM, scrolled line by line (horizon,
 * tunnel, star stream, star drift): on the GBA it wraps, so beyond the picture it goes on the same way. The body (BG1,
 * bos_lst.c) is one 256x256 block of a 512x256 map that follows the boss; the game clears its rows off the screen
 * before loading it, so the wide view draws it from the map as built (PortMarluxiaBodyMap), one copy, whole.
 * Presentation only: the game's flow and VRAM are untouched. */
#include <string.h>

#include "port/port.h"
#include "port/port_battle_backdrop.h"

static int sField;
static uint16_t sBody[0x400];
static int sBodyValid;

void PortMarluxiaField(int on) {
    sField = on;
    sBodyValid = 0;
}

void PortMarluxiaBodyMap(const void* map) {
    memcpy(sBody, map, sizeof(sBody));
    sBodyValid = 1;
}

int PortMarluxiaFieldOn(void) {
    return sField && PortArenasV2();
}

/* the field and body beyond the picture (KHCOM_NO_MARLUXIA_FIELD: the isolated before/after harness, wide view kept) */
int PortMarluxiaExtendOn(void) {
    const char* off;
    PORT_ENV_ONCE(off, getenv, "KHCOM_NO_MARLUXIA_FIELD");
    return PortMarluxiaFieldOn() && !off;
}

/* the body's map entry at picture position (gx, gy), from its scroll; one copy (the nearest the picture), -1 outside
   it, 0 for an empty cell */
int PortMarluxiaBodyEntry(int gx, int gy, int hofs, int vofs) {
    int left = (-hofs) & 511, top = (-vofs) & 255, u, v;
    if (!sBodyValid)
        return -1;
    if (left + 128 - GBA_SCREEN_W / 2 > 256) /* the copy nearest the picture */
        left -= 512;
    if (top + 128 - GBA_SCREEN_H / 2 > 128)
        top -= 256;
    u = gx - left;
    v = gy - top;
    if (u < 0 || u >= 256 || v < 0 || v >= 256)
        return -1;
    return sBody[(v >> 3) * 32 + (u >> 3)];
}
