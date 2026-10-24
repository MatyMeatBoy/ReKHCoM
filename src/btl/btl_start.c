/**
 * btl_start.c
 * Battle Start Effect
 */

#ifdef PLATFORM_PC
#include "engine.h"
#endif
#include "display.h"
#include "m4a_song.h"
#include "btl2.h"
#include "fade.h"
#include "songs.h"
#include "battle_actor.h"
#include "battle_work.h"
#include "bg_animation_data.h"
#include "taskpool.h"
#include "types.h"
#include <stddef.h>
#include "engine_math.h"

void task_btl_start_0(BtlStartWork* work) {
    BgAnimStart(&gBgAnimDefBtlStart, 120, 72);
    BgAnimSetTransform(0, Q_8_8(2), Q_8_8(2));
    SetBgBlend(gBtlWork->bg, 16, 16);
    SetBattleZoom(1, Q_8_8(2), 0x10000, 0x14000);
    FadeStartIn(FADE_MODE_BLACK, 60);
    work->timer = 0;
    m4aSongNumStart(SONG_SYS_ENCOUNT);
    SetBgPriority(gBtlWork->bg, 0);
}

s32 task_btl_start_1(BtlStartWork* work) {
#ifdef PLATFORM_PC
    /* The heart: GBA-size until the white flash hides the switch back. The flash set at timer 43 reaches the screen
       one frame later, so the framing holds through 43 too (a frame of the small heart over the stage showed). */
    if (work->timer <= 43)
        PortFitScreen();
#endif
    if (work->timer <= 20) {
        FadeStartIn(FADE_MODE_BLACK, 40);
    }

    switch (work->timer) {
    case 34:
        SetBattleZoom(35, Q_8_8(1), gBtlWork->x2, gBtlWork->y2);
        break;
    case 43:
        FadeStartIn(FADE_MODE_ADD_WHITE, 30);
        break;
    case 74:
        return 0;
    }

    work->timer++;
    return 1;
}

TaskDesc gTaskDescBtlStart = {
    "task_btl_start",
    (TaskInitFunc)task_btl_start_0,
    (TaskUpdateFunc)task_btl_start_1,
    NULL,
    NULL,
    sizeof(BtlStartWork),
};
