/**
 * bos_tm_foot.c
 * Trickmaster Boss Limbs, Clubs and Table
 */

#include "macros.h"
#include "boss_tm.h"
#include "sprites_boss_tm.h"
#include "engine_math.h"
#include "m4a_song.h"
#include "btl_effect.h"
#include "system_state.h"
#include <stddef.h>
#include "btl_api.h"
#include "songs.h"
#include "anim.h"
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "btl_collision.h"
#include "display.h"
#include "obj.h"
#include "obj_api.h"
#include "taskpool.h"
#include "types.h"
#include "sprite_palettes.h"
#include "sprites_bos2.h"

s16 gBosTmArmImpactViewX EWRAM_COMMON(4);
s32 gBosTmArmImpactViewFixedX EWRAM_COMMON(4);
s16 gUnk_0203AC68 EWRAM_COMMON(4);
s16 gBosTmArmImpactViewY EWRAM_COMMON(4);
s32 gUnk_0203AC70 EWRAM_COMMON(4);
u16 gBosTmArmSpinTimer EWRAM_COMMON(4);
s32 gBosTmArmImpactViewFixedY EWRAM_COMMON(4);

static u8 sBosTmFootIdleFrames[8] = { 2, 1, 0, 1, 2, 3, 4, 3 };

static s16 sBosTmFootIdleZ[5] = { -15, -6, 0, 8, 20 };

s16 gUnk_09EF21C2 = 0;

static TmFootStep sBosTmFootSteps[3] = {
    { 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
    { 0, 6, 0, 1, 0, 6, 0, 1, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
    { 0, 9, 0, 0, 0, 9, 0, 0, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
};

TmFootStep gUnk_09EF2224 = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } };

static TmFootStep sBosTmFootThrowSteps[16] = {
    { 0, -8, 0, 3, 0, -8, 0, 3, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
    { 0, -12, 0, 4, 0, -12, 0, 4, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
    { 0, -8, 0, 5, 0, -8, 0, 5, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
    { 0, 0, 0, 5, 0, 0, 0, 5, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
    { 0, 0, 0, 5, 0, 0, 0, 5, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
    { 0, 0, 0, 5, 0, 0, 0, 5, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
    { 0, 0, 0, 5, 0, 0, 0, 5, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
    { 0, 0, 0, 5, 0, 0, 0, 5, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
    { 0, 8, 0, 4, 0, 8, 0, 4, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
    { 0, 12, 0, 3, 0, 12, 0, 3, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
    { 0, 8, 0, 2, 0, 8, 0, 2, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
    { 0, 6, 0, 1, 0, 6, 0, 1, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
    { 0, 9, 0, 0, 0, 9, 0, 0, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
    { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
    { 0, -9, 0, 1, 0, -9, 0, 1, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
    { 0, -6, 0, 2, 0, -6, 0, 2, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
};

TmFootStep gUnk_09EF2444 = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } };

static TmFootStep sBosTmFootWalkSteps[10] = {
    { 0, 0, 0, 7, 0, 0, 0, 2, -13, -10, { 0, 0, 0, 0 }, 0, 21, { 0, 0, 0, 0 } },
    { 0, 0, 0, 8, 0, 0, 0, 3, -20, 29, { 0, 0, 0, 0 }, 11, 25, { 0, 0, 0, 0 } },
    { 0, 0, 0, 9, 0, 0, 0, 4, -23, 39, { 0, 0, 0, 0 }, 24, 12, { 0, 0, 0, 0 } },
    { 0, 0, 0, 0, 0, 0, 0, 5, -4, 27, { 0, 0, 0, 0 }, 16, -15, { 0, 0, 0, 0 } },
    { 0, 0, 0, 1, 0, 0, 0, 6, 0, 10, { 0, 0, 0, 0 }, -4, -7, { 0, 0, 0, 0 } },
    { 0, 0, 0, 2, 0, 0, 0, 7, 0, 21, { 0, 0, 0, 0 }, -13, -10, { 0, 0, 0, 0 } },
    { 0, 0, 0, 3, 0, 0, 0, 8, 11, 25, { 0, 0, 0, 0 }, -20, 29, { 0, 0, 0, 0 } },
    { 0, 0, 0, 4, 0, 0, 0, 9, 24, 12, { 0, 0, 0, 0 }, -23, 39, { 0, 0, 0, 0 } },
    { 0, 0, 0, 5, 0, 0, 0, 0, 16, -15, { 0, 0, 0, 0 }, -4, 27, { 0, 0, 0, 0 } },
    { 0, 0, 0, 6, 0, 0, 0, 1, -4, -7, { 0, 0, 0, 0 }, 0, 10, { 0, 0, 0, 0 } },
};

static TmFootStep sBosTmFootSpinSteps[9] = {
    { 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
    { 0, 6, 0, 1, 0, 6, 0, 1, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
    { 0, 9, 0, 0, 0, 9, 0, 0, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
    { 0, -9, 0, 0, 0, -9, 0, 0, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
    { 0, -6, 0, 1, 0, -6, 0, 1, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
    { 0, 0, 0, 2, 0, 0, 0, 2, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
    { 0, -8, 0, 3, 0, -8, 0, 3, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
    { 0, -12, 0, 4, 0, -12, 0, 4, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
    { 0, -8, 0, 5, 0, -8, 0, 5, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } },
};

TmFootStep gUnk_09EF26C4 = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, { 0, 0, 0, 0 }, 0, 0, { 0, 0, 0, 0 } };

TaskDesc gTaskDescBosTmFoot = {
    "task_bos_tm_foot",
    (TaskInitFunc)task_bos_tm_foot_0,
    (TaskUpdateFunc)task_bos_tm_foot_1,
    (TaskDrawFunc)task_bos_tm_foot_2,
    (TaskDestroyFunc)task_bos_tm_foot_3,
    sizeof(TmFootWork),
};

static TaskDesc sTaskDescBosTmClb = {
    "task_bos_tm_clb",
    (TaskInitFunc)task_bos_tm_clb_0,
    (TaskUpdateFunc)task_bos_tm_clb_1,
    (TaskDrawFunc)task_bos_tm_clb_2,
    (TaskDestroyFunc)task_bos_tm_clb_3,
    sizeof(TmClbWork),
};

static const TmAnimFrame sBosTmArm0IdleLeftFrames[3] = {
    { 10, { 0, 0 }, { 175, 0, 0, 0, 160, 0, 0, 0, 140, 0, 0, 0, 0, 0, 0, 0 } },
    { 10, { 0, 0 }, { 175, 0, 0, 0, 155, 0, 0, 0, 145, 0, 0, 0, 180, 0, 0, 0 } },
    { 10, { 0, 0 }, { 175, 0, 0, 0, 165, 0, 0, 0, 152, 0, 0, 0, 220, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm1IdleLeftFrames[3] = {
    { 10, { 0, 0 }, { 80, 0, 0, 0, 102, 0, 0, 0, 95, 0, 0, 0, 160, 0, 0, 0 } },
    { 10, { 0, 0 }, { 90, 0, 0, 0, 128, 0, 0, 0, 220, 0, 0, 0, 240, 0, 0, 0 } },
    { 10, { 0, 0 }, { 76, 0, 0, 0, 128, 0, 0, 0, 160, 0, 0, 0, 224, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm0IdleRightFrames[3] = {
    { 10, { 0, 0 }, { 176, 0, 0, 0, 154, 0, 0, 0, 161, 0, 0, 0, 160, 0, 0, 0 } },
    { 10, { 0, 0 }, { 166, 0, 0, 0, 128, 0, 0, 0, 36, 0, 0, 0, 240, 0, 0, 0 } },
    { 10, { 0, 0 }, { 180, 0, 0, 0, 128, 0, 0, 0, 96, 0, 0, 0, 224, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm1IdleRightFrames[3] = {
    { 10, { 0, 0 }, { 81, 0, 0, 0, 96, 0, 0, 0, 116, 0, 0, 0, 0, 0, 0, 0 } },
    { 10, { 0, 0 }, { 81, 0, 0, 0, 101, 0, 0, 0, 111, 0, 0, 0, 180, 0, 0, 0 } },
    { 10, { 0, 0 }, { 81, 0, 0, 0, 91, 0, 0, 0, 104, 0, 0, 0, 220, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm0HurtLeftFrames[1] = {
    { 5, { 0, 0 }, { 128, 0, 0, 0, 128, 0, 0, 0, 108, 0, 0, 0, 214, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm1HurtLeftFrames[1] = {
    { 5, { 0, 0 }, { 128, 0, 0, 0, 128, 0, 0, 0, 108, 0, 0, 0, 64, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm0HurtRightFrames[1] = {
    { 5, { 0, 0 }, { 128, 0, 0, 0, 128, 0, 0, 0, 148, 0, 0, 0, 64, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm1HurtRightFrames[1] = {
    { 5, { 0, 0 }, { 128, 0, 0, 0, 128, 0, 0, 0, 148, 0, 0, 0, 214, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm1FireLeftFrames[3] = {
    { 2, { 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
    { 2, { 0, 0 }, { 192, 0, 0, 0, 192, 0, 0, 0, 192, 0, 0, 0, 192, 0, 0, 0 } },
    { 250, { 0, 0 }, { 120, 0, 0, 0, 160, 0, 0, 0, 220, 0, 0, 0, 230, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm0FireLeftFrames[3] = {
    { 2, { 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
    { 2, { 0, 0 }, { 192, 0, 0, 0, 192, 0, 0, 0, 192, 0, 0, 0, 192, 0, 0, 0 } },
    { 250, { 0, 0 }, { 120, 0, 0, 0, 160, 0, 0, 0, 220, 0, 0, 0, 5, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm1FireRightFrames[3] = {
    { 2, { 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
    { 2, { 0, 0 }, { 64, 0, 0, 0, 64, 0, 0, 0, 64, 0, 0, 0, 192, 0, 0, 0 } },
    { 250, { 0, 0 }, { 136, 0, 0, 0, 96, 0, 0, 0, 36, 0, 0, 0, 5, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm0FireRightFrames[3] = {
    { 2, { 0, 0 }, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } },
    { 2, { 0, 0 }, { 64, 0, 0, 0, 64, 0, 0, 0, 64, 0, 0, 0, 192, 0, 0, 0 } },
    { 250, { 0, 0 }, { 136, 0, 0, 0, 112, 0, 0, 0, 36, 0, 0, 0, 245, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm1ThrowLeftFrames[6] = {
    { 3, { 0, 0 }, { 192, 0, 0, 0, 208, 0, 0, 0, 224, 0, 0, 0, 240, 0, 0, 0 } },
    { 3, { 0, 0 }, { 0, 0, 0, 0, 24, 0, 0, 0, 33, 0, 0, 0, 40, 0, 0, 0 } },
    { 24, { 0, 0 }, { 38, 0, 0, 0, 53, 0, 0, 0, 68, 0, 0, 0, 83, 0, 0, 0 } },
    { 1, { 0, 0 }, { 248, 0, 0, 0, 121, 0, 0, 0, 106, 0, 0, 0, 91, 0, 0, 0 } },
    { 1, { 0, 0 }, { 188, 0, 0, 0, 203, 0, 0, 0, 218, 0, 0, 0, 233, 0, 0, 0 } },
    { 250, { 0, 0 }, { 148, 0, 0, 0, 163, 0, 0, 0, 178, 0, 0, 0, 193, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm0ThrowLeftFrames[6] = {
    { 3, { 0, 0 }, { 192, 0, 0, 0, 208, 0, 0, 0, 224, 0, 0, 0, 240, 0, 0, 0 } },
    { 3, { 0, 0 }, { 0, 0, 0, 0, 24, 0, 0, 0, 33, 0, 0, 0, 40, 0, 0, 0 } },
    { 24, { 0, 0 }, { 38, 0, 0, 0, 53, 0, 0, 0, 68, 0, 0, 0, 83, 0, 0, 0 } },
    { 1, { 0, 0 }, { 248, 0, 0, 0, 121, 0, 0, 0, 106, 0, 0, 0, 91, 0, 0, 0 } },
    { 1, { 0, 0 }, { 188, 0, 0, 0, 203, 0, 0, 0, 218, 0, 0, 0, 233, 0, 0, 0 } },
    { 250, { 0, 0 }, { 148, 0, 0, 0, 163, 0, 0, 0, 178, 0, 0, 0, 193, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm1ThrowRightFrames[6] = {
    { 3, { 0, 0 }, { 64, 0, 0, 0, 48, 0, 0, 0, 32, 0, 0, 0, 240, 0, 0, 0 } },
    { 3, { 0, 0 }, { 0, 0, 0, 0, 232, 0, 0, 0, 223, 0, 0, 0, 40, 0, 0, 0 } },
    { 24, { 0, 0 }, { 218, 0, 0, 0, 203, 0, 0, 0, 188, 0, 0, 0, 83, 0, 0, 0 } },
    { 1, { 0, 0 }, { 8, 0, 0, 0, 135, 0, 0, 0, 150, 0, 0, 0, 91, 0, 0, 0 } },
    { 1, { 0, 0 }, { 68, 0, 0, 0, 53, 0, 0, 0, 38, 0, 0, 0, 233, 0, 0, 0 } },
    { 250, { 0, 0 }, { 108, 0, 0, 0, 93, 0, 0, 0, 78, 0, 0, 0, 193, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm0ThrowRightFrames[6] = {
    { 3, { 0, 0 }, { 64, 0, 0, 0, 48, 0, 0, 0, 32, 0, 0, 0, 240, 0, 0, 0 } },
    { 3, { 0, 0 }, { 0, 0, 0, 0, 232, 0, 0, 0, 223, 0, 0, 0, 40, 0, 0, 0 } },
    { 24, { 0, 0 }, { 218, 0, 0, 0, 203, 0, 0, 0, 188, 0, 0, 0, 83, 0, 0, 0 } },
    { 1, { 0, 0 }, { 8, 0, 0, 0, 135, 0, 0, 0, 150, 0, 0, 0, 91, 0, 0, 0 } },
    { 1, { 0, 0 }, { 68, 0, 0, 0, 53, 0, 0, 0, 38, 0, 0, 0, 233, 0, 0, 0 } },
    { 250, { 0, 0 }, { 108, 0, 0, 0, 93, 0, 0, 0, 78, 0, 0, 0, 193, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm1ThrowSlowLeftFrames[6] = {
    { 3, { 0, 0 }, { 192, 0, 0, 0, 208, 0, 0, 0, 224, 0, 0, 0, 240, 0, 0, 0 } },
    { 3, { 0, 0 }, { 0, 0, 0, 0, 24, 0, 0, 0, 33, 0, 0, 0, 40, 0, 0, 0 } },
    { 39, { 0, 0 }, { 38, 0, 0, 0, 53, 0, 0, 0, 68, 0, 0, 0, 83, 0, 0, 0 } },
    { 1, { 0, 0 }, { 248, 0, 0, 0, 121, 0, 0, 0, 106, 0, 0, 0, 91, 0, 0, 0 } },
    { 1, { 0, 0 }, { 188, 0, 0, 0, 203, 0, 0, 0, 218, 0, 0, 0, 233, 0, 0, 0 } },
    { 250, { 0, 0 }, { 148, 0, 0, 0, 163, 0, 0, 0, 178, 0, 0, 0, 193, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm0ThrowSlowLeftFrames[6] = {
    { 3, { 0, 0 }, { 192, 0, 0, 0, 208, 0, 0, 0, 224, 0, 0, 0, 240, 0, 0, 0 } },
    { 3, { 0, 0 }, { 0, 0, 0, 0, 24, 0, 0, 0, 33, 0, 0, 0, 40, 0, 0, 0 } },
    { 39, { 0, 0 }, { 38, 0, 0, 0, 53, 0, 0, 0, 68, 0, 0, 0, 83, 0, 0, 0 } },
    { 1, { 0, 0 }, { 248, 0, 0, 0, 121, 0, 0, 0, 106, 0, 0, 0, 91, 0, 0, 0 } },
    { 1, { 0, 0 }, { 188, 0, 0, 0, 203, 0, 0, 0, 218, 0, 0, 0, 233, 0, 0, 0 } },
    { 250, { 0, 0 }, { 148, 0, 0, 0, 163, 0, 0, 0, 178, 0, 0, 0, 193, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm1ThrowSlowRightFrames[6] = {
    { 3, { 0, 0 }, { 64, 0, 0, 0, 48, 0, 0, 0, 32, 0, 0, 0, 240, 0, 0, 0 } },
    { 3, { 0, 0 }, { 0, 0, 0, 0, 232, 0, 0, 0, 223, 0, 0, 0, 40, 0, 0, 0 } },
    { 39, { 0, 0 }, { 218, 0, 0, 0, 203, 0, 0, 0, 188, 0, 0, 0, 83, 0, 0, 0 } },
    { 1, { 0, 0 }, { 8, 0, 0, 0, 135, 0, 0, 0, 150, 0, 0, 0, 91, 0, 0, 0 } },
    { 1, { 0, 0 }, { 68, 0, 0, 0, 53, 0, 0, 0, 38, 0, 0, 0, 233, 0, 0, 0 } },
    { 250, { 0, 0 }, { 108, 0, 0, 0, 93, 0, 0, 0, 78, 0, 0, 0, 193, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm0ThrowSlowRightFrames[6] = {
    { 3, { 0, 0 }, { 64, 0, 0, 0, 48, 0, 0, 0, 32, 0, 0, 0, 240, 0, 0, 0 } },
    { 3, { 0, 0 }, { 0, 0, 0, 0, 232, 0, 0, 0, 223, 0, 0, 0, 40, 0, 0, 0 } },
    { 39, { 0, 0 }, { 218, 0, 0, 0, 203, 0, 0, 0, 188, 0, 0, 0, 83, 0, 0, 0 } },
    { 1, { 0, 0 }, { 8, 0, 0, 0, 135, 0, 0, 0, 150, 0, 0, 0, 91, 0, 0, 0 } },
    { 1, { 0, 0 }, { 68, 0, 0, 0, 53, 0, 0, 0, 38, 0, 0, 0, 233, 0, 0, 0 } },
    { 250, { 0, 0 }, { 108, 0, 0, 0, 93, 0, 0, 0, 78, 0, 0, 0, 193, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm1WalkLeftFrames[6] = {
    { 10, { 0, 0 }, { 148, 0, 0, 0, 128, 0, 0, 0, 108, 0, 0, 0, 88, 0, 0, 0 } },
    { 15, { 0, 0 }, { 138, 0, 0, 0, 118, 0, 0, 0, 98, 0, 0, 0, 68, 0, 0, 0 } },
    { 10, { 0, 0 }, { 128, 0, 0, 0, 148, 0, 0, 0, 168, 0, 0, 0, 208, 0, 0, 0 } },
    { 10, { 0, 0 }, { 118, 0, 0, 0, 148, 0, 0, 0, 178, 0, 0, 0, 218, 0, 0, 0 } },
    { 15, { 0, 0 }, { 138, 0, 0, 0, 168, 0, 0, 0, 228, 0, 0, 0, 248, 0, 0, 0 } },
    { 10, { 0, 0 }, { 158, 0, 0, 0, 138, 0, 0, 0, 108, 0, 0, 0, 58, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm0WalkLeftFrames[6] = {
    { 10, { 0, 0 }, { 108, 0, 0, 0, 138, 0, 0, 0, 168, 0, 0, 0, 208, 0, 0, 0 } },
    { 15, { 0, 0 }, { 138, 0, 0, 0, 158, 0, 0, 0, 198, 0, 0, 0, 238, 0, 0, 0 } },
    { 10, { 0, 0 }, { 168, 0, 0, 0, 148, 0, 0, 0, 98, 0, 0, 0, 48, 0, 0, 0 } },
    { 10, { 0, 0 }, { 148, 0, 0, 0, 128, 0, 0, 0, 78, 0, 0, 0, 28, 0, 0, 0 } },
    { 15, { 0, 0 }, { 138, 0, 0, 0, 108, 0, 0, 0, 48, 0, 0, 0, 3, 0, 0, 0 } },
    { 10, { 0, 0 }, { 128, 0, 0, 0, 148, 0, 0, 0, 168, 0, 0, 0, 208, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm1WalkRightFrames[6] = {
    { 10, { 0, 0 }, { 108, 0, 0, 0, 128, 0, 0, 0, 148, 0, 0, 0, 88, 0, 0, 0 } },
    { 15, { 0, 0 }, { 118, 0, 0, 0, 138, 0, 0, 0, 158, 0, 0, 0, 68, 0, 0, 0 } },
    { 10, { 0, 0 }, { 128, 0, 0, 0, 108, 0, 0, 0, 88, 0, 0, 0, 208, 0, 0, 0 } },
    { 10, { 0, 0 }, { 138, 0, 0, 0, 108, 0, 0, 0, 78, 0, 0, 0, 218, 0, 0, 0 } },
    { 15, { 0, 0 }, { 118, 0, 0, 0, 88, 0, 0, 0, 28, 0, 0, 0, 248, 0, 0, 0 } },
    { 10, { 0, 0 }, { 98, 0, 0, 0, 118, 0, 0, 0, 148, 0, 0, 0, 58, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm0WalkRightFrames[6] = {
    { 10, { 0, 0 }, { 148, 0, 0, 0, 118, 0, 0, 0, 88, 0, 0, 0, 208, 0, 0, 0 } },
    { 15, { 0, 0 }, { 118, 0, 0, 0, 98, 0, 0, 0, 58, 0, 0, 0, 238, 0, 0, 0 } },
    { 10, { 0, 0 }, { 88, 0, 0, 0, 108, 0, 0, 0, 158, 0, 0, 0, 48, 0, 0, 0 } },
    { 10, { 0, 0 }, { 108, 0, 0, 0, 128, 0, 0, 0, 178, 0, 0, 0, 28, 0, 0, 0 } },
    { 15, { 0, 0 }, { 118, 0, 0, 0, 148, 0, 0, 0, 208, 0, 0, 0, 3, 0, 0, 0 } },
    { 10, { 0, 0 }, { 128, 0, 0, 0, 108, 0, 0, 0, 88, 0, 0, 0, 208, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm1SpinLeftFrames[5] = {
    { 20, { 0, 0 }, { 128, 0, 0, 0, 112, 0, 0, 0, 72, 0, 0, 0, 58, 0, 0, 0 } },
    { 1, { 0, 0 }, { 160, 0, 0, 0, 144, 0, 0, 0, 112, 0, 0, 0, 96, 0, 0, 0 } },
    { 1, { 0, 0 }, { 192, 0, 0, 0, 184, 0, 0, 0, 96, 0, 0, 0, 118, 0, 0, 0 } },
    { 1, { 0, 0 }, { 192, 0, 0, 0, 188, 0, 0, 0, 178, 0, 0, 0, 160, 0, 0, 0 } },
    { 20, { 0, 0 }, { 192, 0, 0, 0, 198, 0, 0, 0, 224, 0, 0, 0, 248, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm0SpinLeftFrames[5] = {
    { 20, { 0, 0 }, { 128, 0, 0, 0, 112, 0, 0, 0, 72, 0, 0, 0, 58, 0, 0, 0 } },
    { 1, { 0, 0 }, { 160, 0, 0, 0, 144, 0, 0, 0, 112, 0, 0, 0, 96, 0, 0, 0 } },
    { 1, { 0, 0 }, { 192, 0, 0, 0, 168, 0, 0, 0, 96, 0, 0, 0, 118, 0, 0, 0 } },
    { 1, { 0, 0 }, { 192, 0, 0, 0, 188, 0, 0, 0, 178, 0, 0, 0, 160, 0, 0, 0 } },
    { 20, { 0, 0 }, { 192, 0, 0, 0, 198, 0, 0, 0, 224, 0, 0, 0, 248, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm1SpinRightFrames[5] = {
    { 20, { 0, 0 }, { 128, 0, 0, 0, 144, 0, 0, 0, 184, 0, 0, 0, 58, 0, 0, 0 } },
    { 1, { 0, 0 }, { 96, 0, 0, 0, 112, 0, 0, 0, 144, 0, 0, 0, 96, 0, 0, 0 } },
    { 1, { 0, 0 }, { 64, 0, 0, 0, 72, 0, 0, 0, 160, 0, 0, 0, 118, 0, 0, 0 } },
    { 1, { 0, 0 }, { 64, 0, 0, 0, 68, 0, 0, 0, 78, 0, 0, 0, 160, 0, 0, 0 } },
    { 20, { 0, 0 }, { 64, 0, 0, 0, 58, 0, 0, 0, 32, 0, 0, 0, 248, 0, 0, 0 } },
};

static const TmAnimFrame sBosTmArm0SpinRightFrames[5] = {
    { 20, { 0, 0 }, { 128, 0, 0, 0, 144, 0, 0, 0, 184, 0, 0, 0, 58, 0, 0, 0 } },
    { 1, { 0, 0 }, { 96, 0, 0, 0, 112, 0, 0, 0, 144, 0, 0, 0, 96, 0, 0, 0 } },
    { 1, { 0, 0 }, { 64, 0, 0, 0, 88, 0, 0, 0, 160, 0, 0, 0, 118, 0, 0, 0 } },
    { 1, { 0, 0 }, { 64, 0, 0, 0, 68, 0, 0, 0, 78, 0, 0, 0, 160, 0, 0, 0 } },
    { 20, { 0, 0 }, { 64, 0, 0, 0, 58, 0, 0, 0, 32, 0, 0, 0, 248, 0, 0, 0 } },
};

static const u16 sBosTmArmSegmentLengths[6] = { 24, 26, 28, 30, 28, 26 };

void BosTmFootInitPart(BtlObj* work, s16 x, s16 y, s16 z, s16 radius, s16 height, s32 inEvent, s16 part) {
    work->x = x << 8;
    work->y = y << 8;
    work->z = z << 8;

    if (part >= 6 && part <= 7) {
        ColliderInit(&work->collider, COLLIDER_TYPE_BOSS, radius, height);
        ColliderSetPosition(&work->collider, work->x, work->y, work->z);
    }
}

void BosTmFootSetPartPos(BtlObj* obj, s32 x, s32 y, s32 z) {
    obj->x = (s16)x << 8;
    obj->y = (s16)y << 8;
    obj->z = (s16)z << 8;
}

void BosTmFootReleasePart(BtlObj* work) {
    ColliderUnregister(&work->collider);
}

void BosTmFootSyncCollider(BtlObj* sub, TmFootWork* work) {
    ColliderSetPosition(&sub->collider, sub->x, sub->y, sub->z);
}

void BosTmFootResetPose(TmFootWork* work) {
    work->unk_002 = 0;
    work->unk_000 = 0;
    SetObjTileSource(work->tiles2, gBosTmFootTiles);
    SetObjTileSource(work->tiles3, gBosTmFootTiles);
    work->gfx = gBosTmFootFrames[2];
    work->gfx2 = gBosTmFootFrames[2];

    if (work->tm->flags & TM_FLAG_FACING_LEFT) {
        work->body.x = work->tm->baseX + 0x100;
        work->body2.x = work->tm->baseX - 0x600;
        work->body3.x = work->tm->baseX + 0x600;
        work->body4.x = work->tm->baseX - 0x200;
        work->body.y = work->tm->baseY + 0x200;
        work->body2.y = work->tm->baseY - 0x200;
        work->body3.y = work->tm->baseY + 0x500;
        work->body4.y = work->tm->baseY - 0x200;
        work->body.z = work->tm->baseZ - 0x400;
        work->body2.z = work->tm->baseZ - 0x400;
        work->body3.z = work->tm->baseZ + 0x2800;
        work->body4.z = work->tm->baseZ + 0x2B00;
    } else {
        work->body.x = work->tm->baseX + 0x600;
        work->body2.x = work->tm->baseX - 0x100;
        work->body3.x = work->tm->baseX + 0x200;
        work->body4.x = work->tm->baseX - 0x600;
        work->body.y = work->tm->baseY - 0x200;
        work->body2.y = work->tm->baseY + 0x200;
        work->body3.y = work->tm->baseY - 0x500;
        work->body4.y = work->tm->baseY + 0x200;
        work->body.z = work->tm->baseZ - 0x400;
        work->body2.z = work->tm->baseZ - 0x400;
        work->body3.z = work->tm->baseZ + 0x2800;
        work->body4.z = work->tm->baseZ + 0x2B00;
    }
}

void BosTmFootSetBreakPose(TmFootWork* work) {
    SetObjTileSource(work->tiles2, gBosTmFootTiles);
    SetObjTileSource(work->tiles3, gBosTmFootTiles);
    work->gfx = gBosTmFootFrames[0];
    work->gfx2 = gBosTmFootFrames[0];

    if (work->tm->flags & TM_FLAG_FACING_LEFT) {
        work->body.x = work->tm->baseX + 0x100;
        work->body2.x = work->tm->baseX - 0x600;
        work->body3.x = work->tm->baseX + 0x600;
        work->body4.x = work->tm->baseX - 0x200;
        work->body.y = work->tm->y2 + 0x200;
        work->body2.y = work->tm->y2 - 0x200;
        work->body3.y = work->tm->y2 + 0x500;
        work->body4.y = work->tm->y2 - 0x200;
        work->body.z = work->tm->z2 - 0x400;
        work->body2.z = work->tm->z2 - 0x400;
        work->body3.z = work->tm->z2 + 0x1900;
        work->body4.z = work->tm->z2 + 0x1C00;
    } else {
        work->body.x = work->tm->baseX + 0x600;
        work->body2.x = work->tm->baseX - 0x100;
        work->body3.x = work->tm->baseX + 0x200;
        work->body4.x = work->tm->baseX - 0x600;
        work->body.y = work->tm->y2 - 0x200;
        work->body2.y = work->tm->y2 + 0x200;
        work->body3.y = work->tm->y2 - 0x500;
        work->body4.y = work->tm->y2 + 0x200;
        work->body.z = work->tm->z2 - 0x400;
        work->body2.z = work->tm->z2 - 0x400;
        work->body3.z = work->tm->z2 + 0x1900;
        work->body4.z = work->tm->z2 + 0x1C00;
    }
}

void BosTmFootApplyThrowStep(TmFootWork* work, s16 step) {
    work->gfx = gBosTmFootFrames[sBosTmFootThrowSteps[step].gfxIndex];
    work->gfx2 = gBosTmFootFrames[sBosTmFootThrowSteps[step].gfx2Index];
    work->body.z += sBosTmFootThrowSteps[step].dz << 8;
    work->body2.z += sBosTmFootThrowSteps[step].dz2 << 8;
}

void BosTmFootSetWalkPose(TmFootWork* work) {
    work->unk_002 = 0;
    work->unk_000 = 0;
    SetObjTileSource(work->tiles2, gBosTmFootWalkTiles);
    SetObjTileSource(work->tiles3, gBosTmFootWalkTiles);
    work->gfx = gBosTmFootWalkFrames[6];
    work->gfx2 = gBosTmFootWalkFrames[1];

    if (work->tm->flags & TM_FLAG_FACING_LEFT) {
        work->body.x = work->tm->x2 + 0x500;
        work->body2.x = work->tm->x2 - 0x600;
        work->body3.x = work->tm->x2 + 0x600;
        work->body4.x = work->tm->x2 - 0x200;
        work->body.y = work->tm->y2 + 0x200;
        work->body2.y = work->tm->y2 - 0x200;
        work->body3.y = work->tm->y2 + 0x500;
        work->body4.y = work->tm->y2 - 0x200;
        work->body.z = work->tm->z2 - 0x400;
        work->body2.z = work->tm->z2 - 0x400;
        work->body3.z = work->tm->z2 + 0x1E00;
        work->body4.z = work->tm->z2 + 0x3200;
    } else {
        work->body.x = work->tm->x2 + 0x200;
        work->body2.x = work->tm->x2 - 0x100;
        work->body3.x = work->tm->x2 + 0x200;
        work->body4.x = work->tm->x2 - 0x600;
        work->body.y = work->tm->y2 - 0x200;
        work->body2.y = work->tm->y2 + 0x200;
        work->body3.y = work->tm->y2 - 0x500;
        work->body4.y = work->tm->y2 + 0x200;
        work->body.z = work->tm->z2 - 0x400;
        work->body2.z = work->tm->z2 - 0x400;
        work->body3.z = work->tm->z2 + 0x1E00;
        work->body4.z = work->tm->z2 + 0x3200;
    }
}

void BosTmFootWalk(TmFootWork* work) {
    if (work->tm->stepTimer != 0) {
        return;
    }

    work->gfx = gBosTmFootWalkFrames[sBosTmFootWalkSteps[work->tm->step].gfxIndex];
    work->gfx2 = gBosTmFootWalkFrames[sBosTmFootWalkSteps[work->tm->step].gfx2Index];

    if (work->tm->flags & TM_FLAG_FACING_LEFT) {
        work->body3.x = work->tm->x2 + ((sBosTmFootWalkSteps[work->tm->step].x3 + 6) << 8);
        work->body4.x = work->tm->x2 + ((sBosTmFootWalkSteps[work->tm->step].x4 - 2) << 8);
        work->body.x = work->tm->x2 + 0x100;
        work->body2.x = work->tm->x2 - 0x600;
        work->body3.y = work->tm->y2 + 0x500;
        work->body4.y = work->tm->y2 - 0x200;
        work->body.y = work->tm->y2 + 0x200;
        work->body2.y = work->tm->y2 - 0x200;
        work->body3.z = work->tm->z2 + ((sBosTmFootWalkSteps[work->tm->step].z3 + 40) << 8);
        work->body4.z = work->tm->z2 + ((sBosTmFootWalkSteps[work->tm->step].z4 + 43) << 8);
        work->body.z = work->tm->z2 - 0x400;
        work->body2.z = work->tm->z2 - 0x400;
    } else {
        work->body3.x = work->tm->x2 + ((2 - sBosTmFootWalkSteps[work->tm->step].x3) << 8);
        work->body4.x = work->tm->x2 + ((-6 - sBosTmFootWalkSteps[work->tm->step].x4) << 8);
        work->body.x = work->tm->x2 + 0x600;
        work->body2.x = work->tm->x2 - 0x100;
        work->body3.y = work->tm->y2 - 0x500;
        work->body4.y = work->tm->y2 + 0x200;
        work->body.y = work->tm->y2 - 0x200;
        work->body2.y = work->tm->y2 + 0x200;
        work->body3.z = work->tm->z2 + ((sBosTmFootWalkSteps[work->tm->step].z3 + 40) << 8);
        work->body4.z = work->tm->z2 + ((sBosTmFootWalkSteps[work->tm->step].z4 + 43) << 8);
        work->body.z = work->tm->z2 - 0x400;
        work->body2.z = work->tm->z2 - 0x400;
    }

    if (work->tm->step == 2) {
        if (ApplyAttackBox(239, work->body3.x, work->body3.y - 0x500, 0, 20, 16, 20) == ATTACK_RESULT_HIT) {
            m4aSongNumStart(SONG_BTL_MON_HIT03);
        }
    }

    if (work->tm->step == 7) {
        if (ApplyAttackBox(239, work->body4.x, work->body4.y - 0x500, 0, 20, 16, 20) == ATTACK_RESULT_HIT) {
            m4aSongNumStart(SONG_BTL_MON_HIT03);
        }
    }
}

void BosTmFootApplySpinStep(TmFootWork* work, s16 step) {
    work->gfx = gBosTmFootFrames[sBosTmFootSpinSteps[step].gfxIndex];
    work->gfx2 = gBosTmFootFrames[sBosTmFootSpinSteps[step].gfx2Index];
    work->body.z += sBosTmFootSpinSteps[step].dz << 8;
    work->body2.z += sBosTmFootSpinSteps[step].dz2 << 8;
}

void task_bos_tm_foot_0(TmFootWork* work, TmWork* arg) {
    u16 inEvent;

    work->tiles = LoadObjTiles(gBosTmObjTiles, sizeof(gBosTmObjTiles));
    work->tiles2 = AllocObjTiles(0x440, gBosTmFootTiles);
    work->tiles3 = AllocObjTiles(0x440, gBosTmFootTiles);
    work->palette = LoadObjPalette(gBoss03objPalettes, sizeof(gBoss03objPalettes));
    work->palette2 = LoadObjPalette(gHitFlashPalette, sizeof(gHitFlashPalette));
    work->gfx = gBosTmFootFrames[2];
    work->gfx2 = gBosTmFootFrames[2];
    work->gfx3 = gBosTmShoe0Frames[0];
    work->gfx4 = gBosTmShoe1Frames[0];
    work->tm = arg;
#ifdef PLATFORM_PC
    /* tiles a full pool did not give add none (OptiPort v2.8) */
    work->tm->tileCount += (PORT_TILES_OK(work->tiles2) ? work->tiles2->count : 0) + (PORT_TILES_OK(work->tiles3) ? work->tiles3->count : 0);
#else
    work->tm->tileCount += work->tiles2->count + work->tiles3->count;
#endif
    work->footFrame = 0;
    work->footFrame2 = 0;
    work->unk_000 = 0;
    work->unk_480 = -0x100;
    work->unk_002 = 0;
    work->angle = 0;
    work->angle2 = 0;
    work->angle3 = 0;
    work->angle4 = 0;
    inEvent = work->tm->flags & TM_FLAG_IN_EVENT;

    if (inEvent != 0) {
        BosTmFootSetPartPos(&work->body, (s16)(work->tm->x + 1),
                      (s16)(work->tm->y + 2), (s16)(work->tm->z - 4));
        BosTmFootSetPartPos(&work->body2, (s16)(work->tm->x - 6),
                      (s16)(work->tm->y - 2), (s16)(work->tm->z - 4));
        BosTmFootSetPartPos(&work->body3, (s16)(work->tm->x + 6),
                      (s16)(work->tm->y + 5), (s16)(work->tm->z + 40));
        BosTmFootSetPartPos(&work->body4, (s16)(work->tm->x - 2),
                      (s16)(work->tm->y - 2), (s16)(work->tm->z + 43));
    } else {
        BosTmFootInitPart(&work->body, work->tm->x + 1,
                      work->tm->y + 2, work->tm->z - 4, 4, 32, inEvent,
                      4);
        BosTmFootInitPart(&work->body2, work->tm->x - 6,
                      work->tm->y - 2, work->tm->z - 4, 4, 32, inEvent,
                      5);
        BosTmFootInitPart(&work->body3, work->tm->x + 6,
                      work->tm->y + 5, work->tm->z + 40, 20, 140,
                      inEvent, 6);
        BosTmFootInitPart(&work->body4, work->tm->x - 2,
                      work->tm->y - 2, work->tm->z + 43, 20, 140,
                      inEvent, 7);
    }
}

u8 task_bos_tm_foot_1(TmFootWork* work) {
    u16 step;
    TmFootStep* recoilStep;
    TmFootStep* recoilSteps;

    BosTmFootSyncCollider(&work->body3, work);
    BosTmFootSyncCollider(&work->body4, work);

    switch (work->tm->state) {
    case BOS_TM_STATE_IDLE:
    case BOS_TM_STATE_EVENT_IDLE:
        if (work->tm->stateTimer != 0) {
            if (work->tm->stepTimer != 0) {
                break;
            }

            work->footFrame = sBosTmFootIdleFrames[work->tm->step];
            work->footFrame2 = sBosTmFootIdleFrames[(work->tm->step + 4) & 7];
            work->gfx = gBosTmFootFrames[(s8)work->footFrame];
            work->gfx2 = gBosTmFootFrames[(s8)work->footFrame2];
            work->body3.z =
                work->tm->z2 + ((sBosTmFootIdleZ[(s8)work->footFrame] + 40) << 8);
            work->body4.z =
                work->tm->z2 + ((sBosTmFootIdleZ[(s8)work->footFrame2] + 43) << 8);
            work->body.z = work->tm->z2 - 0x400;
            work->body2.z = work->tm->z2 - 0x400;
        } else {
            BosTmFootResetPose(work);
        }

        break;
    case BOS_TM_STATE_WALK_LEFT:
    case BOS_TM_STATE_WALK_LEFT_SETTLE:
    case BOS_TM_STATE_WALK_RIGHT:
    case BOS_TM_STATE_WALK_RIGHT_SETTLE:
        if (work->tm->stateTimer != 0) {
            BosTmFootWalk(work);
        } else {
            BosTmFootSetWalkPose(work);
        }

        break;
    case BOS_TM_STATE_FIRE:
    case BOS_TM_STATE_FIRE_TWICE:
        if (work->tm->stateTimer == 0) {
            BosTmFootResetPose(work);
        }

        break;
    case BOS_TM_STATE_SLAM_TABLE:
    case BOS_TM_STATE_SLAM_GROUND:
        if (work->tm->stateTimer != 0) {
            step = work->tm->step;

            if (work->tm->step <= 3) {
                BosTmFootApplyThrowStep(work, work->tm->step);
            } else if (step >= 66 && step <= 74) {
                step -= 62;
                BosTmFootApplyThrowStep(work, step);
            } else if (step >= 98 && step <= 100) {
                step -= 85;
                BosTmFootApplyThrowStep(work, step);
            } else {
                break;
            }
        } else {
            BosTmFootResetPose(work);
        }

        break;
    case BOS_TM_STATE_SLAM_GROUND_SLOW:
        if (work->tm->stateTimer != 0) {
            step = work->tm->step;

            if (work->tm->step <= 3) {
                BosTmFootApplyThrowStep(work, work->tm->step);
            } else if (step >= 96 && step <= 104) {
                step -= 92;
                BosTmFootApplyThrowStep(work, step);
            } else if (step >= 128 && step <= 130) {
                step -= 115;
                BosTmFootApplyThrowStep(work, step);
            } else {
                break;
            }
        } else {
            BosTmFootResetPose(work);
        }

        break;
    case BOS_TM_STATE_SPIN:
        if (work->tm->stateTimer != 0) {
            step = work->tm->step;

            if (work->tm->step <= 2) {
                BosTmFootApplySpinStep(work, work->tm->step);
            } else if (step >= 41 && step <= 46) {
                step -= 38;
                BosTmFootApplySpinStep(work, step);
            } else {
                break;
            }
        } else {
            BosTmFootResetPose(work);
        }

        break;
    case BOS_TM_STATE_RECOIL:
        if (work->tm->hitCount == 1) {
            work->unk_002 = 0;
            work->unk_000 = 0;
            work->gfx = gBosTmFootFrames[1];
            work->gfx2 = gBosTmFootFrames[1];
            work->body.z = work->tm->baseZ +
                            ((sBosTmFootSteps[work->tm->step].dz - 4) << 8);
            work->body2.z = work->tm->baseZ +
                            ((sBosTmFootSteps[work->tm->step].dz2 - 4) << 8);
            work->body3.z = work->tm->baseZ + 0x2800;
            work->body4.z = work->tm->baseZ + 0x2800;
            break;
        }

        if (work->tm->step <= 2) {
            work->gfx = gBosTmFootFrames[sBosTmFootSteps[work->tm->step].gfxIndex];
            work->gfx2 = gBosTmFootFrames[sBosTmFootSteps[work->tm->step].gfx2Index];
            work->body.z += sBosTmFootSteps[work->tm->step].dz << 8;
            work->body2.z += sBosTmFootSteps[work->tm->step].dz2 << 8;
        }

        if (work->tm->hurtTimer <= 2) {
            work->gfx =
                gBosTmFootFrames[(recoilSteps = sBosTmFootSteps, recoilStep = &recoilSteps[work->tm->hurtTimer])->gfxIndex + 1];
            work->gfx2 = gBosTmFootFrames[recoilStep->gfx2Index + 1];
            work->body.z -= recoilStep->dz << 8;
            work->body2.z -= recoilStep->dz2 << 8;
        }

        break;
    case BOS_TM_STATE_CARD_BROKEN:
        if (work->tm->stateTimer == 0) {
            BosTmFootSetBreakPose(work);
        } else if (work->tm->stateTimer > 59) {
            if (work->tm->flags & TM_FLAG_SWITCHING_SIDES) {
                BosTmFootSetWalkPose(work);
            } else {
                BosTmFootResetPose(work);
            }
        }

        break;
    case BOS_TM_STATE_DEFEATED:
        if (work->tm->step == 0) {
            BosTmFootSetBreakPose(work);
        }

        break;
    case BOS_TM_STATE_RESUME_WALK:
    case BOS_TM_STATE_NONE:
    case BOS_TM_STATE_FROZEN:
    default:
        break;
    }

    return 1;
}

void task_bos_tm_foot_2(TmFootWork* work) {
    void* pal;
    s32 facingLeft;
    s16 x;
    s16 y;
    BtlObj* body;
    BtlObj* body2;
    BtlObj* body3;
    BtlObj* body4;
    u16 flags;

    facingLeft = work->tm->flags & TM_FLAG_FACING_LEFT;
    flags = SPRITE_PRIORITY(2) | SPRITE_FLAG_HFLIP;

    if (facingLeft != 0) {
        flags = SPRITE_PRIORITY(2);
    }

    if (gBtlWork->paused) {
        pal = work->palette;
    } else if ((work->tm->flags & TM_FLAG_HURT) && (gFrameCounter & 1)) {
        pal = work->palette2;
    } else {
        pal = work->palette;
    }

    body = &work->body;
    body2 = &work->body2;
    body3 = &work->body3;
    body4 = &work->body4;
    WorldToScreen(&x, &y, body->x, body->y, body->z);
    DrawSprite(x, y, work->gfx, work->tiles2, pal, NULL, flags, -4100 - (body->y >> 8) * 4);
    WorldToScreen(&x, &y, body2->x, body2->y, body2->z);
    DrawSprite(x, y, work->gfx2, work->tiles3, pal, NULL, flags, -4100 - (body2->y >> 8) * 4);
    WorldToScreen(&x, &y, body3->x, body3->y, body3->z);
    DrawSprite(x, y, work->gfx3, work->tiles, pal, NULL, flags, -4100 - (body3->y >> 8) * 4);
    WorldToScreen(&x, &y, body4->x, body4->y, body4->z);
    DrawSprite(x, y, work->gfx4, work->tiles, pal, NULL, flags, -4100 - (body4->y >> 8) * 4);
}

void task_bos_tm_foot_3(TmFootWork* work) {
    if ((work->tm->flags & TM_FLAG_IN_EVENT) == 0) {
        BosTmFootReleasePart(&work->body3);
        BosTmFootReleasePart(&work->body4);
    }

    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ReleaseObjPalette(work->palette2);
}

enum BosTmClbMoveMode {
    BOS_TM_CLB_MOVE_MODE_FOLLOW,
    BOS_TM_CLB_MOVE_MODE_HOLD,
    BOS_TM_CLB_MOVE_MODE_HOLD_LEFT,
    BOS_TM_CLB_MOVE_MODE_HOLD_RIGHT_HIGH,
    BOS_TM_CLB_MOVE_MODE_THROWN
};

enum BosTmClbSpinMode {
    BOS_TM_CLB_SPIN_MODE_TWIRL,
    BOS_TM_CLB_SPIN_MODE_ALIGN,
    BOS_TM_CLB_SPIN_MODE_LOCKED
};

void CreateBosTmClbTask(TaskPool* pool, TmClbArg* clb, TmArmPos* tip) {
    clb->src = tip;
    clb->moveMode = BOS_TM_CLB_MOVE_MODE_FOLLOW;
    clb->spinMode = BOS_TM_CLB_SPIN_MODE_ALIGN;
    clb->vz = 0;
    TaskCreate(pool, &sTaskDescBosTmClb, clb);
}

void BosTmClbThrow(TmClbArg* clb, TmArmPos* tip, s32 vz) {
    clb->src = tip;
    clb->vz = vz;
    clb->moveMode = BOS_TM_CLB_MOVE_MODE_THROWN;
    clb->spinMode = BOS_TM_CLB_SPIN_MODE_TWIRL;
}

void BosTmClbHoldSpinning(TmClbArg* clb, TmArmPos* tip) {
    clb->src = tip;
    clb->vz = 0;
    clb->moveMode = BOS_TM_CLB_MOVE_MODE_FOLLOW;
    clb->spinMode = BOS_TM_CLB_SPIN_MODE_TWIRL;
}

enum BosTmClbHoldMode {
    BOS_TM_CLB_HOLD_ARM1_FACING_LEFT,
    BOS_TM_CLB_HOLD_ARM0_FACING_LEFT,
    BOS_TM_CLB_HOLD_ARM1_FACING_RIGHT,
    BOS_TM_CLB_HOLD_ARM0_FACING_RIGHT
};

void BosTmClbHold(TmClbArg* clb, TmArmPos* tip, u8 mode) {
    clb->src = tip;
    clb->vz = 0;
    clb->spinMode = BOS_TM_CLB_SPIN_MODE_LOCKED;

    switch (mode) {
    case BOS_TM_CLB_HOLD_ARM1_FACING_LEFT:
        clb->moveMode = BOS_TM_CLB_MOVE_MODE_HOLD;
        break;
    case BOS_TM_CLB_HOLD_ARM0_FACING_LEFT:
        clb->moveMode = BOS_TM_CLB_MOVE_MODE_HOLD;
        break;
    case BOS_TM_CLB_HOLD_ARM1_FACING_RIGHT:
        clb->moveMode = BOS_TM_CLB_MOVE_MODE_HOLD_LEFT;
        break;
    case BOS_TM_CLB_HOLD_ARM0_FACING_RIGHT:
        clb->moveMode = BOS_TM_CLB_MOVE_MODE_HOLD_RIGHT_HIGH;
        break;
    }
}

void task_bos_tm_clb_0(TmClbWork* work, TmClbArg* arg) {
    TmArmPos* tip;

    work->tiles = LoadObjTiles(gBosTmObjTiles, sizeof(gBosTmObjTiles));
    work->palette = LoadObjPalette(gBoss03objPalettes, sizeof(gBoss03objPalettes));
    work->arg = arg;
    tip = arg->src;
    work->angle = tip->angle;
    work->x = tip->x;
    work->y = tip->y;
    work->z = tip->z;
}

u8 task_bos_tm_clb_1(TmClbWork* work) {
    TmClbArg* clb = work->arg;

    switch (clb->moveMode) {
    case BOS_TM_CLB_MOVE_MODE_THROWN:
        work->x += (clb->src->x - work->x) >> 4;
        work->y = clb->src->y;
        work->z += clb->vz;
        clb->vz += 51;

        if (clb->vz > 0 && work->z >= clb->src->z) {
            work->z = clb->src->z;
            clb->moveMode = BOS_TM_CLB_MOVE_MODE_FOLLOW;
            clb->spinMode = BOS_TM_CLB_SPIN_MODE_ALIGN;
        }

        break;
    case BOS_TM_CLB_MOVE_MODE_FOLLOW:
        work->x = clb->src->x;
        work->y = clb->src->y;
        work->z = clb->src->z;
        break;
    case BOS_TM_CLB_MOVE_MODE_HOLD:
        work->x = clb->src->x;
        work->y = clb->src->y;
        work->z = clb->src->z;
        break;
    case BOS_TM_CLB_MOVE_MODE_HOLD_LEFT:
        work->x = clb->src->x - 0x600;
        work->y = clb->src->y;
        work->z = clb->src->z;
        break;
    case BOS_TM_CLB_MOVE_MODE_HOLD_RIGHT_HIGH:
        work->x = clb->src->x + 0x600;
        work->y = clb->src->y;
        work->z = clb->src->z - 0x500;
        break;
    }

    switch (clb->spinMode) {
    case BOS_TM_CLB_SPIN_MODE_TWIRL:
        work->angle += 0x10;
        break;
    case BOS_TM_CLB_SPIN_MODE_ALIGN:
        ApproachAngle(&work->angle, clb->src->angle, 2);
        break;
    case BOS_TM_CLB_SPIN_MODE_LOCKED:
        work->angle = clb->src->angle;
        break;
    }

    return 1;
}

void task_bos_tm_clb_2(TmClbWork* work) {
    ObjAffine* affine;
    s16 x;
    s16 y;

    affine = AllocObjAffineAngle(work->angle, FALSE);
    WorldToScreen(&x, &y, work->x, work->y, work->z);
    DrawSprite(x, y, gBosTmClubHandleFrame0, work->tiles, work->palette, affine, SPRITE_PRIORITY(2),
               -0x1002 - (work->y >> 8) * 4);
    // @bug AllocObjAffineAngle returns NULL at angle 0 (NULL write).
    BIOS_PTR(affine)->doubleSize = TRUE;
    DrawSprite(x, y, work->arg->gfx, work->arg->tiles, work->palette, affine, SPRITE_PRIORITY(2),
               -0x1003 - (work->y >> 8) * 4);
}

void task_bos_tm_clb_3(TmClbWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

void BosTmArmSetTargetAngles(TmArmJoint* joints, const u8* src) {
    s32 i;

    i = 3;

    do {
        joints->targetAngle = *src;
        src += 4;
        joints++;
    } while (--i >= 0);
}

void BosTmArmStartJointAnim(TmAnim* anim, const TmAnimFrame* src, u16 frameCount, TmArmJoint* joints) {
    if (anim->frames != src) {
        anim->frames = src;
        anim->timer = 0;
        anim->frame = 0;
        anim->frameCount = frameCount;
        BosTmArmSetTargetAngles(joints, src->angles);
    }
}

void BosTmArmUpdateArm1Tip(TmArmWork* work) {
    TmArmJoint* hand = &work->joints.all[7];
    TmArmPos* tip = &work->tips[0];

    tip->x = hand->curX + gSineTable[hand->angle] * 12 + work->src->x;
    tip->z = hand->curY + -gSineTable[hand->angle + 0x40] * 12 + work->src->z;
    tip->y = work->src->y;
}

void BosTmArmUpdateArm0Tip(TmArmWork* work) {
    TmArmJoint* hand = &work->joints.all[3];
    TmArmPos* tip = &work->tips[1];

    tip->x = hand->curX + gSineTable[hand->angle] * 12 + work->src->x2;
    tip->z = hand->curY + -gSineTable[hand->angle + 0x40] * 12 + work->src->z2;
    tip->y = work->src->y2;
}

void BosTmArmComputeJointPositions(TmArmJoint* joints) {
    s32 x;
    s32 y;
    s32 i;
    s32 index;
    TmArmJoint* joint;

    x = 0;
    y = 0;

    for (i = 0; i < 3; i++) {
        joint = &joints[i];
        joint->x = x;
        joint->y = y;
        x += gSineTable[joint->angle] * sBosTmArmSegmentLengths[index = joint->anim.frame];
        y += -gSineTable[joint->angle + 0x40] * sBosTmArmSegmentLengths[index = joint->anim.frame];
    }

    joint = &joints[index = 3];
    joint->x = x;
    joint->y = y;
}

void BosTmArmUpdateJoints(TmArmJoint* joints, u16 shift) {
    s32 i;
    u8* angle;
    TmArmJoint* joint;

    for (i = 0; i < 4; i++) {
        joint = &joints[i];

        angle = &joint->angle;
        ApproachAngle((u16*)angle, joint->targetAngle, shift);
    }

    BosTmArmComputeJointPositions(joints);

    for (i = 0; i < 4; i++) {
        joint = &joints[i];

        joint->curX += (joint->x - joint->curX) >> 1;
        joint->curY += (joint->y - joint->curY) >> 1;
    }
}

void BosTmArmStepJointAnim(TmArmJoint* joints, TmAnim* anim) {
    if (anim->timer >= anim->frames[anim->frame].duration) {
        anim->timer = 0;
        anim->frame++;

        if (anim->frame >= anim->frameCount) {
            anim->frame = 0;
        }

        BosTmArmSetTargetAngles(joints, anim->frames[anim->frame].angles);
    }

    anim->timer++;
    BosTmArmUpdateJoints(joints, 1);
}

void task_bos_tm_arm_0(TmArmWork* work, TmArmSrc* arg) {
    s32 i;
    void* gfx;
    TmArmJoint* arm0;
    TmArmJoint* arm1;
    TmArmJoint* arm0Joint;
    TmArmJoint* arm1Joint;

    work->src = arg;
    work->tiles = LoadObjTiles(gBosTmObjTiles, sizeof(gBosTmObjTiles));
    work->palette = LoadObjPalette(gBoss03objPalettes, sizeof(gBoss03objPalettes));
    work->palette2 = LoadObjPalette(gHitFlashPalette, sizeof(gHitFlashPalette));
    work->timer = 0;
    work->timer2 = 0;
    work->clbSwapped = TRUE;
    work->prevState = BOS_TM_STATE_IDLE;
    work->jointAnim2.frames = NULL;
    work->jointAnim.frames = NULL;
    BosTmArmStartJointAnim(&work->jointAnim2, sBosTmArm0IdleLeftFrames, ARRAY_COUNT(sBosTmArm0IdleLeftFrames), work->joints.arms[0]);
    BosTmArmStartJointAnim(&work->jointAnim, sBosTmArm1IdleLeftFrames, ARRAY_COUNT(sBosTmArm1IdleLeftFrames), &work->joints.arms[1][0]);

    for (i = 0; i < 4; i++) {
        arm0Joint = &work->joints.arms[0][i];
        arm1Joint = &work->joints.arms[1][i];
        *(u16*)&arm0Joint->angle = arm0Joint->targetAngle;
        *(u16*)&arm1Joint->angle = arm1Joint->targetAngle;
    }

    arm0 = work->joints.arms[0];
    BosTmArmComputeJointPositions(arm0);
    arm1 = &work->joints.arms[1][0];
    BosTmArmComputeJointPositions(arm1);

    for (i = 0; i < 4; i++) {
        work->joints.arms[0][i].curX = work->joints.arms[0][i].x;
        work->joints.arms[0][i].curY = work->joints.arms[0][i].y;
        work->joints.arms[1][i].curX = work->joints.arms[1][i].x;
        work->joints.arms[1][i].curY = work->joints.arms[1][i].y;
    }

    BosTmArmUpdateArm1Tip(work);
    BosTmArmUpdateArm0Tip(work);
    work->tips[0].angle = 272;
    work->tips[1].angle = 240;

    for (i = 0; i < 3; i++) {
        AnimInit(&work->joints.arms[0][i].anim, gBosTmArmAnims, gBosTmArmFrames);
        AnimStart(&work->joints.arms[0][i].anim, 0, ANIM_FLAG_LOOP);
        work->joints.arms[0][i].anim.frame = i * 2;
        work->joints.arms[0][i].gfx = AnimGetGfx(&work->joints.arms[0][i].anim);
        AnimInit(&work->joints.arms[1][i].anim, gBosTmArmAnims, gBosTmArmFrames);
        AnimStart(&work->joints.arms[1][i].anim, 0, ANIM_FLAG_LOOP);
        work->joints.arms[1][i].anim.frame = i * 2;
        work->joints.arms[1][i].gfx = AnimGetGfx(&work->joints.arms[1][i].anim);
    }

    work->joints.arms[0][3].gfx = gBosTmHandFrame0;
    work->joints.arms[1][3].gfx = gBosTmHandFrame0;
    work->tiles2 = AllocObjTiles(0x140, gBosTmClubTiles);
    if (PORT_TILES_OK(work->tiles2))
        work->src->tm->tileCount += work->tiles2->count;
    AnimInit(&work->anim, gBosTmClubAnims, gBosTmClubFrames);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->clb.tiles = work->tiles2;
    work->clb2.tiles = work->tiles2;
    gfx = AnimGetGfx(&work->anim);
    work->clb.gfx = gfx;
    work->clb2.gfx = gfx;
    work->paletteStep = 0;
    TaskPoolInit(&work->tasks, 2);
    CreateBosTmClbTask(&work->tasks, &work->clb, &work->tips[0]);
    CreateBosTmClbTask(&work->tasks, &work->clb2, &work->tips[1]);
    gBosTmArmSpinTimer = 0;
    gBosTmArmImpactViewFixedX = 0;
    gBosTmArmImpactViewFixedY = 0;
    gUnk_0203AC70 = 0;
    gBosTmArmImpactViewX = 0;
    gBosTmArmImpactViewY = 0;
    gUnk_0203AC68 = 0;
}

void BosTmArmUpdateArm1(TmArmWork* work) {
    TmArmJoint* hand;
    TmArmJoint* hand2;
    s32 i;
    s32 hit;
    s32 x;
    s32 y;
    s32 z;
    s32 x2;
    s32 y2;
    s32 z2;
    u8 v;

    switch (work->src->tm->state) {
    case BOS_TM_STATE_IDLE:
    case BOS_TM_STATE_EVENT_IDLE:
        if (work->timer == 0) {
            work->tips[0].angle = 0x110;

            if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
                BosTmArmStartJointAnim(&work->jointAnim, sBosTmArm1IdleLeftFrames, ARRAY_COUNT(sBosTmArm1IdleLeftFrames), &work->joints.all[4]);
            } else {
                BosTmArmStartJointAnim(&work->jointAnim, sBosTmArm1IdleRightFrames, ARRAY_COUNT(sBosTmArm1IdleRightFrames), &work->joints.all[4]);
            }
        }

        switch (work->timer % 30) {
        case 0:
            v = FALSE;

            if (!work->clbSwapped) {
                v = TRUE;
            }

            work->clbSwapped = v;
            break;
        case 22:
            if (work->clbSwapped) {
                BosTmClbThrow(&work->clb, &work->tips[0], -0x380);
            } else {
                BosTmClbThrow(&work->clb2, &work->tips[0], -0x380);
            }

            break;
        }

        work->timer++;
        break;
    case BOS_TM_STATE_WALK_LEFT:
    case BOS_TM_STATE_WALK_LEFT_SETTLE:
    case BOS_TM_STATE_WALK_RIGHT:
    case BOS_TM_STATE_WALK_RIGHT_SETTLE:
        if (work->timer == 0) {
            work->tips[0].angle = 0x110;
            BosTmClbHoldSpinning(&work->clb, &work->tips[0]);

            if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
                BosTmArmStartJointAnim(&work->jointAnim, sBosTmArm1WalkLeftFrames, ARRAY_COUNT(sBosTmArm1WalkLeftFrames), &work->joints.all[4]);
            } else {
                BosTmArmStartJointAnim(&work->jointAnim, sBosTmArm1WalkRightFrames, ARRAY_COUNT(sBosTmArm1WalkRightFrames), &work->joints.all[4]);
            }
        }

        work->timer++;
        break;
    case BOS_TM_STATE_RECOIL:
    case BOS_TM_STATE_CARD_BROKEN:
        if (work->timer == 0) {
            work->tips[0].angle = 90;
            BosTmClbThrow(&work->clb, &work->tips[0], -128);

            if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
                BosTmArmStartJointAnim(&work->jointAnim, sBosTmArm1HurtLeftFrames, ARRAY_COUNT(sBosTmArm1HurtLeftFrames), &work->joints.all[4]);
            } else {
                BosTmArmStartJointAnim(&work->jointAnim, sBosTmArm1HurtRightFrames, ARRAY_COUNT(sBosTmArm1HurtRightFrames), &work->joints.all[4]);
            }
        }

        work->timer++;
        break;
    case BOS_TM_STATE_DEFEATED:
        if (work->timer == 0) {
            work->tips[0].angle = 90;
            BosTmClbThrow(&work->clb, &work->tips[0], -128);

            if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
                BosTmArmStartJointAnim(&work->jointAnim, sBosTmArm1HurtLeftFrames, ARRAY_COUNT(sBosTmArm1HurtLeftFrames), &work->joints.all[4]);
            } else {
                BosTmArmStartJointAnim(&work->jointAnim, sBosTmArm1HurtRightFrames, ARRAY_COUNT(sBosTmArm1HurtRightFrames), &work->joints.all[4]);
            }
        }

        work->timer++;
        break;
    case BOS_TM_STATE_FIRE:
        if (work->timer == 0) {
            if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
                BosTmArmStartJointAnim(&work->jointAnim, sBosTmArm1FireLeftFrames, ARRAY_COUNT(sBosTmArm1FireLeftFrames), &work->joints.all[4]);
                work->tips[0].angle = 0xE8;
                BosTmClbHold(&work->clb, &work->tips[0], BOS_TM_CLB_HOLD_ARM1_FACING_LEFT);
            } else {
                BosTmArmStartJointAnim(&work->jointAnim, sBosTmArm1FireRightFrames, ARRAY_COUNT(sBosTmArm1FireRightFrames), &work->joints.all[4]);
                work->tips[0].angle = 0xF4;
                BosTmClbHold(&work->clb, &work->tips[0], BOS_TM_CLB_HOLD_ARM1_FACING_RIGHT);
            }
        }

        if (work->timer == 45) {
            hand = &work->joints.all[3];
            y = work->src->y2;
            z = work->src->z2 + hand->curY - 0x2300;

            if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
                x = work->src->x2 + hand->curX - 0x3E00;
                BgFxStartFireAtPlayer(x, y, z, TRUE, 0, 168, 20);
            } else {
                x = work->src->x2 + hand->curX + 0x4800;
                BgFxStartFireAtPlayer(x, y, z, FALSE, 0, 168, 20);
            }
        } else if (work->timer > 55) {
            if (!BgFxIsActive()) {
                work->tips[0].angle = 0x110;
                work->src->tm->flags |= TM_FLAG_ATTACK_DONE;
            }
        }

        work->timer++;
        break;
    case BOS_TM_STATE_FIRE_TWICE:
        if (work->timer == 0) {
            if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
                BosTmArmStartJointAnim(&work->jointAnim, sBosTmArm1FireLeftFrames, ARRAY_COUNT(sBosTmArm1FireLeftFrames), &work->joints.all[4]);
                work->tips[0].angle = 0xE8;
                BosTmClbHold(&work->clb, &work->tips[0], BOS_TM_CLB_HOLD_ARM1_FACING_LEFT);
            } else {
                BosTmArmStartJointAnim(&work->jointAnim, sBosTmArm1FireRightFrames, ARRAY_COUNT(sBosTmArm1FireRightFrames), &work->joints.all[4]);
                work->tips[0].angle = 0xF4;
                BosTmClbHold(&work->clb, &work->tips[0], BOS_TM_CLB_HOLD_ARM1_FACING_RIGHT);
            }
        }

        if (work->timer == 30) {
            hand = &work->joints.all[3];
            y = work->src->y2;
            z = work->src->z2 + hand->curY - 0x2300;

            if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
                x = work->src->x2 + hand->curX - 0x3E00;
                BgFxStartFireAtPlayer(x, y, z, TRUE, 0, 168, 18);
            } else {
                x = work->src->x2 + hand->curX + 0x4800;
                BgFxStartFireAtPlayer(x, y, z, FALSE, 0, 168, 18);
            }
        } else if (work->timer > 70) {
            if (!BgFxIsActive()) {
                work->tips[0].angle = 0x110;
                work->src->tm->flags |= TM_FLAG_ATTACK_DONE;
            }
        } else if (work->timer > 50) {
            v = BgFxIsActive();

            if (!v) {
                hand2 = &work->joints.all[3];
                y2 = work->src->y2;
                z2 = work->src->z2 + hand2->curY - 0x2300;

                if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
                    x2 = work->src->x2 + hand2->curX - 0x3E00;
                    BgFxStartFireAtPlayer(x2, y2, z2, TRUE, 0, 168, 18);
                } else {
                    x2 = work->src->x2 + hand2->curX + 0x4800;
                    BgFxStartFireAtPlayer(x2, y2, z2, FALSE, 0, 168, 18);
                }
            }
        }

        work->timer++;
        break;
    case BOS_TM_STATE_SLAM_TABLE:
        if (work->timer == 0) {
            if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
                BosTmArmStartJointAnim(&work->jointAnim, sBosTmArm1ThrowLeftFrames, ARRAY_COUNT(sBosTmArm1ThrowLeftFrames), &work->joints.all[4]);
            } else {
                BosTmArmStartJointAnim(&work->jointAnim, sBosTmArm1ThrowRightFrames, ARRAY_COUNT(sBosTmArm1ThrowRightFrames), &work->joints.all[4]);
            }

            BosTmClbThrow(&work->clb, &work->tips[0], -0xB00);
            work->tips[0].angle = 0xE8;
        }

        if (work->timer == 35) {
            work->tips[0].angle = 0x110;
            work->src->tm->tableState = BOS_TM_TABLE_STATE_MOVING;

            if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
                hit = ApplyAttackBox(237, work->tips[1].x - 0x1000, work->tips[1].y,
                                  work->tips[1].z + 0x1400, 16, 16, 16);
            } else {
                hit = ApplyAttackBox(237, work->tips[1].x + 0x2800, work->tips[1].y,
                                  work->tips[1].z + 0x1400, 16, 16, 16);
            }

            if (hit == 1) {
                m4aSongNumStart(SONG_BTL_MON_HIT03);
            }

            m4aSongNumStart(SONG_BTL_LB_RUMB);
        }

        if (work->timer > 50) {
            work->src->tm->flags |= TM_FLAG_ATTACK_DONE;
        } else {
            work->timer++;
        }

        break;
    case BOS_TM_STATE_SLAM_GROUND:
        if (work->timer == 0) {
            if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
                BosTmArmStartJointAnim(&work->jointAnim, sBosTmArm1ThrowLeftFrames, ARRAY_COUNT(sBosTmArm1ThrowLeftFrames), &work->joints.all[4]);
            } else {
                BosTmArmStartJointAnim(&work->jointAnim, sBosTmArm1ThrowRightFrames, ARRAY_COUNT(sBosTmArm1ThrowRightFrames), &work->joints.all[4]);
            }

            BosTmClbThrow(&work->clb, &work->tips[0], -0xB00);
            work->tips[0].angle = 0xE8;
        }

        if (work->timer == 35) {
            work->tips[0].angle = 0x110;
        }

        if (work->timer == 37) {
            gBosTmArmImpactViewFixedX = gBtlWork->viewX;
            gBosTmArmImpactViewFixedY = gBtlWork->viewY;
            gBosTmArmImpactViewX = gBosTmArmImpactViewFixedX >> 8;
            gBosTmArmImpactViewY = gBosTmArmImpactViewFixedY >> 8;

            if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
                BgFxStartGroundImpact(0x10D00, 0x15800);
            } else {
                BgFxStartGroundImpact(0xF000, 0x15800);
            }

            BtlMapStartShake();
            m4aSongNumStart(SONG_BTL_LB_RUMB);
            ApplyAttackBox(238, gBtlWork->viewX, gBtlWork->viewY, 0, 320, 240, 1);
            gBosTmArmImpactViewFixedX = gBtlWork->viewX;
            gBosTmArmImpactViewFixedY = gBtlWork->viewY;
            gBosTmArmImpactViewX = gBosTmArmImpactViewFixedX >> 8;
            gBosTmArmImpactViewY = gBosTmArmImpactViewFixedY >> 8;
        }

        if (work->timer > 50) {
            work->src->tm->flags |= TM_FLAG_ATTACK_DONE;
        } else {
            work->timer++;
        }

        break;
    case BOS_TM_STATE_SLAM_GROUND_SLOW:
        if (work->timer == 0) {
            if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
                BosTmArmStartJointAnim(&work->jointAnim, sBosTmArm1ThrowSlowLeftFrames, ARRAY_COUNT(sBosTmArm1ThrowSlowLeftFrames), &work->joints.all[4]);
            } else {
                BosTmArmStartJointAnim(&work->jointAnim, sBosTmArm1ThrowSlowRightFrames, ARRAY_COUNT(sBosTmArm1ThrowSlowRightFrames), &work->joints.all[4]);
            }

            BosTmClbThrow(&work->clb, &work->tips[0], -0xB00);
            work->tips[0].angle = 0xE8;
        }

        if (work->timer == 50) {
            work->tips[0].angle = 0x110;
        }

        if (work->timer == 52) {
            gBosTmArmImpactViewFixedX = gBtlWork->viewX;
            gBosTmArmImpactViewFixedY = gBtlWork->viewY;
            gBosTmArmImpactViewX = gBosTmArmImpactViewFixedX >> 8;
            gBosTmArmImpactViewY = gBosTmArmImpactViewFixedY >> 8;

            if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
                BgFxStartGroundImpact(0x10D00, 0x15800);
            } else {
                BgFxStartGroundImpact(0xF000, 0x15800);
            }

            BtlMapStartShake();
            m4aSongNumStart(SONG_BTL_LB_RUMB);
            ApplyAttackBox(238, gBtlWork->viewX, gBtlWork->viewY, 0, 320, 240, 1);
            gBosTmArmImpactViewFixedX = gBtlWork->viewX;
            gBosTmArmImpactViewFixedY = gBtlWork->viewY;
            gBosTmArmImpactViewX = gBosTmArmImpactViewFixedX >> 8;
            gBosTmArmImpactViewY = gBosTmArmImpactViewFixedY >> 8;
        }

        if (work->timer > 65) {
            work->src->tm->flags |= TM_FLAG_ATTACK_DONE;
        } else {
            work->timer++;
        }

        break;
    case BOS_TM_STATE_SPIN:
        if (work->timer == 0) {
            work->tips[0].angle = 0x10C;
            BosTmClbHoldSpinning(&work->clb, &work->tips[0]);

            if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
                BosTmArmStartJointAnim(&work->jointAnim, sBosTmArm1SpinLeftFrames, ARRAY_COUNT(sBosTmArm1SpinLeftFrames), &work->joints.all[4]);
            } else {
                BosTmArmStartJointAnim(&work->jointAnim, sBosTmArm1SpinRightFrames, ARRAY_COUNT(sBosTmArm1SpinRightFrames), &work->joints.all[4]);
            }
        }

        if (work->timer == 21) {
            if (ApplyAttackBox(240, work->src->tm->baseX, work->src->tm->baseY,
                              work->tips[1].z, 36, 32, 32) == ATTACK_RESULT_HIT) {
                m4aSongNumStart(SONG_BTL_MON_HIT03);
            }
        }

        if (work->timer > 33) {
            work->src->tm->flags |= TM_FLAG_ATTACK_DONE;
        } else {
            work->timer++;
        }

        gBosTmArmSpinTimer = work->timer;
        break;
    case BOS_TM_STATE_FROZEN:
        return;
    }

    BosTmArmStepJointAnim(&work->joints.all[4], &work->jointAnim);
    BosTmArmUpdateArm1Tip(work);

    for (i = 0; i < 3; i++) {
        work->joints.arms[1][i].gfx = AnimUpdate(&work->joints.arms[1][i].anim);
    }
}

void BosTmArmUpdateArm0(TmArmWork* work) {
    s32 i;

    switch (work->src->tm->state) {
    case BOS_TM_STATE_IDLE:
    case BOS_TM_STATE_EVENT_IDLE:
        if (work->timer2 == 0) {
            work->tips[1].angle = 0x110;

            if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
                BosTmArmStartJointAnim(&work->jointAnim2, sBosTmArm0IdleLeftFrames, ARRAY_COUNT(sBosTmArm0IdleLeftFrames), work->joints.all);
            } else {
                BosTmArmStartJointAnim(&work->jointAnim2, sBosTmArm0IdleRightFrames, ARRAY_COUNT(sBosTmArm0IdleRightFrames), work->joints.all);
            }
        }

        if (work->timer2 % 30 == 12) {
            if (work->clbSwapped) {
                BosTmClbThrow(&work->clb2, &work->tips[1], -0x600);
            } else {
                BosTmClbThrow(&work->clb, &work->tips[1], -0x600);
            }
        }

        work->timer2++;
        break;
    case BOS_TM_STATE_WALK_LEFT:
    case BOS_TM_STATE_WALK_LEFT_SETTLE:
    case BOS_TM_STATE_WALK_RIGHT:
    case BOS_TM_STATE_WALK_RIGHT_SETTLE:
        if (work->timer2 == 0) {
            work->tips[1].angle = 0x110;
            BosTmClbHoldSpinning(&work->clb2, &work->tips[1]);

            if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
                BosTmArmStartJointAnim(&work->jointAnim2, sBosTmArm0WalkLeftFrames, ARRAY_COUNT(sBosTmArm0WalkLeftFrames), work->joints.all);
            } else {
                BosTmArmStartJointAnim(&work->jointAnim2, sBosTmArm0WalkRightFrames, ARRAY_COUNT(sBosTmArm0WalkRightFrames), work->joints.all);
            }
        }

        work->timer2++;
        break;
    case BOS_TM_STATE_RECOIL:
    case BOS_TM_STATE_CARD_BROKEN:
        if (work->timer2 == 0) {
            work->tips[1].angle = 185;
            BosTmClbThrow(&work->clb2, &work->tips[1], -128);

            if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
                BosTmArmStartJointAnim(&work->jointAnim2, sBosTmArm0HurtLeftFrames, ARRAY_COUNT(sBosTmArm0HurtLeftFrames), work->joints.all);
            } else {
                BosTmArmStartJointAnim(&work->jointAnim2, sBosTmArm0HurtRightFrames, ARRAY_COUNT(sBosTmArm0HurtRightFrames), work->joints.all);
            }
        }

        work->timer2++;
        break;
    case BOS_TM_STATE_DEFEATED:
        if (work->timer2 == 0) {
            work->tips[1].angle = 185;
            BosTmClbThrow(&work->clb2, &work->tips[1], -128);

            if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
                BosTmArmStartJointAnim(&work->jointAnim2, sBosTmArm0HurtLeftFrames, ARRAY_COUNT(sBosTmArm0HurtLeftFrames), work->joints.all);
            } else {
                BosTmArmStartJointAnim(&work->jointAnim2, sBosTmArm0HurtRightFrames, ARRAY_COUNT(sBosTmArm0HurtRightFrames), work->joints.all);
            }
        }

        work->timer2++;
        break;
    case BOS_TM_STATE_FIRE:
        if (work->timer2 == 0) {
            if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
                BosTmArmStartJointAnim(&work->jointAnim2, sBosTmArm0FireLeftFrames, ARRAY_COUNT(sBosTmArm0FireLeftFrames), work->joints.all);
                work->tips[1].angle = 0x10C;
                BosTmClbHold(&work->clb2, &work->tips[1], BOS_TM_CLB_HOLD_ARM0_FACING_LEFT);
            } else {
                BosTmArmStartJointAnim(&work->jointAnim2, sBosTmArm0FireRightFrames, ARRAY_COUNT(sBosTmArm0FireRightFrames), work->joints.all);
                work->tips[1].angle = 0x118;
                BosTmClbHold(&work->clb2, &work->tips[1], BOS_TM_CLB_HOLD_ARM0_FACING_RIGHT);
            }
        }

        if (work->timer2 > 55) {
            if (!BgFxIsActive()) {
                work->tips[1].angle = 240;
            }
        }

        work->timer2++;
        break;
    case BOS_TM_STATE_FIRE_TWICE:
        if (work->timer2 == 0) {
            if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
                BosTmArmStartJointAnim(&work->jointAnim2, sBosTmArm0FireLeftFrames, ARRAY_COUNT(sBosTmArm0FireLeftFrames), work->joints.all);
                work->tips[1].angle = 0x10C;
                BosTmClbHold(&work->clb2, &work->tips[1], BOS_TM_CLB_HOLD_ARM0_FACING_LEFT);
            } else {
                BosTmArmStartJointAnim(&work->jointAnim2, sBosTmArm0FireRightFrames, ARRAY_COUNT(sBosTmArm0FireRightFrames), work->joints.all);
                work->tips[1].angle = 0x118;
                BosTmClbHold(&work->clb2, &work->tips[1], BOS_TM_CLB_HOLD_ARM0_FACING_RIGHT);
            }
        }

        if (work->timer2 > 70) {
            if (!BgFxIsActive()) {
                work->tips[1].angle = 240;
            }
        }

        work->timer2++;
        break;
    case BOS_TM_STATE_SLAM_TABLE:
        if (work->timer2 == 0) {
            if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
                BosTmArmStartJointAnim(&work->jointAnim2, sBosTmArm0ThrowLeftFrames, ARRAY_COUNT(sBosTmArm0ThrowLeftFrames), work->joints.all);
            } else {
                BosTmArmStartJointAnim(&work->jointAnim2, sBosTmArm0ThrowRightFrames, ARRAY_COUNT(sBosTmArm0ThrowRightFrames), work->joints.all);
            }

            BosTmClbThrow(&work->clb2, &work->tips[1], -0xB00);
            work->tips[1].angle = 0x10C;
        }

        if (work->timer2 == 35) {
            work->tips[1].angle = 240;
        }

        work->timer2++;
        break;
    case BOS_TM_STATE_SLAM_GROUND:
        if (work->timer2 == 0) {
            if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
                BosTmArmStartJointAnim(&work->jointAnim2, sBosTmArm0ThrowLeftFrames, ARRAY_COUNT(sBosTmArm0ThrowLeftFrames), work->joints.all);
            } else {
                BosTmArmStartJointAnim(&work->jointAnim2, sBosTmArm0ThrowRightFrames, ARRAY_COUNT(sBosTmArm0ThrowRightFrames), work->joints.all);
            }

            BosTmClbThrow(&work->clb2, &work->tips[1], -0xB00);
            work->tips[1].angle = 0x10C;
        }

        if (work->timer2 == 35) {
            work->tips[1].angle = 240;
        }

        work->timer2++;
        break;
    case BOS_TM_STATE_SLAM_GROUND_SLOW:
        if (work->timer2 == 0) {
            if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
                BosTmArmStartJointAnim(&work->jointAnim2, sBosTmArm0ThrowSlowLeftFrames, ARRAY_COUNT(sBosTmArm0ThrowSlowLeftFrames), work->joints.all);
            } else {
                BosTmArmStartJointAnim(&work->jointAnim2, sBosTmArm0ThrowSlowRightFrames, ARRAY_COUNT(sBosTmArm0ThrowSlowRightFrames), work->joints.all);
            }

            BosTmClbThrow(&work->clb2, &work->tips[1], -0xB00);
            work->tips[1].angle = 0x10C;
        }

        if (work->timer2 == 50) {
            work->tips[1].angle = 240;
        }

        work->timer2++;
        break;
    case BOS_TM_STATE_SPIN:
        if (work->timer2 == 0) {
            work->tips[1].angle = 0x110;
            BosTmClbHoldSpinning(&work->clb2, &work->tips[1]);

            if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
                BosTmArmStartJointAnim(&work->jointAnim2, sBosTmArm0SpinLeftFrames, ARRAY_COUNT(sBosTmArm0SpinLeftFrames), work->joints.all);
            } else {
                BosTmArmStartJointAnim(&work->jointAnim2, sBosTmArm0SpinRightFrames, ARRAY_COUNT(sBosTmArm0SpinRightFrames), work->joints.all);
            }
        }

        work->timer2++;
        break;
    case BOS_TM_STATE_FROZEN:
        return;
    }

    BosTmArmStepJointAnim(work->joints.all, &work->jointAnim2);
    BosTmArmUpdateArm0Tip(work);

    for (i = 0; i < 3; i++) {
        work->joints.all[i].gfx = AnimUpdate(&work->joints.all[i].anim);
    }
}

u8 task_bos_tm_arm_1(TmArmWork* work) {
    void* gfx;

    if (work->prevState != work->src->tm->state) {
        work->prevState = work->src->tm->state;
        work->timer = 0;
        work->timer2 = 0;
    }

    if (gFrameCounter % 2 != 0) {
        BosTmArmUpdateArm1(work);
    } else {
        BosTmArmUpdateArm0(work);
    }

    if (work->src->tm->state != BOS_TM_STATE_DEFEATED) {
        gfx = AnimUpdate(&work->anim);
        work->clb.gfx = gfx;
        work->clb2.gfx = gfx;

        if (gFrameCounter % 5 == 0) {
            LoadObjPaletteBank(BIOS_PTR(work->palette)->index + 1, gBosTmClubPalettes + work->paletteStep * 16);
            work->paletteStep = (work->paletteStep + 1) & 7;
        }
    }

    TaskPoolUpdate(&work->tasks);

    return 1;
}

void task_bos_tm_arm_2(TmArmWork* work) {
    void* pal;
    s32 scaleX;
    ObjAffine* affine;
    s16 depth;
    s16 endDepth;
    s16 x;
    s16 y;
    s32 i;
    TmArmJoint* joint;

    if (gBtlWork->paused) {
        pal = work->palette;
    } else if ((work->src->tm->flags & TM_FLAG_HURT) && (gFrameCounter & 1)) {
        pal = work->palette2;
    } else {
        pal = work->palette;
    }

    for (i = 0; i < 3; i++) {
        joint = &work->joints.all[i + 4];
        affine = AllocObjAffine(joint->angle, Q_8_8(1), Q_8_8(1), FALSE);
        WorldToScreen(&x, &y, work->src->x + joint->curX, work->src->y,
                      work->src->z + joint->curY);
        depth = -4100;
        DrawSprite(x, y, joint->gfx, work->tiles, pal, affine, SPRITE_PRIORITY(2),
                   (depth -= (work->src->y >> 8) * 4, (u16)depth));
        joint = &work->joints.all[i];
        affine = AllocObjAffine(joint->angle, Q_8_8(1), Q_8_8(1), FALSE);
        WorldToScreen(&x, &y, work->src->x2 + joint->curX, work->src->y2,
                      work->src->z2 + joint->curY);
        depth = -4100;
        DrawSprite(x, y, joint->gfx, work->tiles, pal, affine, SPRITE_PRIORITY(2),
                   (depth -= (work->src->y2 >> 8) * 4, (u16)depth));
    }

    if (work->src->tm->flags & TM_FLAG_FACING_LEFT) {
        scaleX = Q_8_8(1);
    } else {
        scaleX = Q_8_8(-1);
    }

    joint = &work->joints.all[7];
    affine = AllocObjAffine(joint->angle, scaleX, Q_8_8(1), FALSE);
    WorldToScreen(&x, &y, work->src->x + joint->curX, work->src->y,
                  work->src->z + joint->curY);
    DrawSprite(x, y, joint->gfx, work->tiles, pal, affine, SPRITE_PRIORITY(2),
               (endDepth = -4100 - (work->src->y >> 8) * 4, (u16)endDepth));
    joint = &work->joints.all[3];
    affine = AllocObjAffine(joint->angle, scaleX, Q_8_8(1), FALSE);
    WorldToScreen(&x, &y, work->src->x2 + joint->curX, work->src->y2,
                  work->src->z2 + joint->curY);
    DrawSprite(x, y, joint->gfx, work->tiles, pal, affine, SPRITE_PRIORITY(2),
               (endDepth = -4100 - (work->src->y2 >> 8) * 4, (u16)endDepth));
    TaskPoolDraw(&work->tasks);
}

void task_bos_tm_arm_3(TmArmWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjTiles(work->tiles2);
    ReleaseObjPalette(work->palette);
    ReleaseObjPalette(work->palette2);
    TaskPoolDestroy(&work->tasks);
}

enum BosTmTblState {
    BOS_TM_TBL_STATE_DOWN,
    BOS_TM_TBL_STATE_UP,
    BOS_TM_TBL_STATE_RISING,
    BOS_TM_TBL_STATE_SINKING
};

void task_bos_tm_tbl_0(TmTblWork* work, TmWork* arg) {
    ColliderInit(&work->collider, COLLIDER_TYPE_PLATFORM, 0x1C, 0);
    ColliderSetPosition(&work->collider, 0x10000, 0x16000, 0);
    ColliderSetDisabled(&work->collider, FALSE);
    DisableBg(1);
    work->tm = arg;
    work->state = BOS_TM_TBL_STATE_DOWN;
    work->unk_062 = 1;
    work->unk_064 = 0;
    work->frame = 0;
    work->gimmickPlayed = FALSE;
    work->height = 0;
}

u8 task_bos_tm_tbl_1(TmTblWork* work) {
    u16 flags;

    switch (work->state) {
    case BOS_TM_TBL_STATE_UP:
        if (work->tm->tableState == BOS_TM_TABLE_STATE_MOVING) {
            work->frame = 0;
            work->state = BOS_TM_TBL_STATE_SINKING;
        }

        break;
    case BOS_TM_TBL_STATE_DOWN:
        if (!work->gimmickPlayed) {
            if (ConsumeGimmickFlag(0)) {
                work->gimmickPlayed = TRUE;
            }
        } else {
            work->state = BOS_TM_TBL_STATE_RISING;
            work->tm->tableState = BOS_TM_TABLE_STATE_MOVING;
            work->gimmickPlayed = FALSE;
        }

        break;
    case BOS_TM_TBL_STATE_RISING:
        switch (work->frame) {
        case 0:
            m4aSongNumStart(SONG_BTL_TABLE_U);
            EnableBg(1);
            LoadBgMap(1, gBosTmTableMaps[8], sizeof(gBosTmTableMaps[0]));
            ColliderSetDisabled(&work->collider, FALSE);
            break;
        case 2:
            LoadBgMap(1, gBosTmTableMaps[7], sizeof(gBosTmTableMaps[0]));
            break;
        case 4:
            LoadBgMap(1, gBosTmTableMaps[6], sizeof(gBosTmTableMaps[0]));
            break;
        case 6:
            LoadBgMap(1, gBosTmTableMaps[5], sizeof(gBosTmTableMaps[0]));
            break;
        case 8:
            LoadBgMap(1, gBosTmTableMaps[4], sizeof(gBosTmTableMaps[0]));
            break;
        case 10:
            LoadBgMap(1, gBosTmTableMaps[3], sizeof(gBosTmTableMaps[0]));
            break;
        case 12:
            LoadBgMap(1, gBosTmTableMaps[2], sizeof(gBosTmTableMaps[0]));
            break;
        case 14:
            LoadBgMap(1, gBosTmTableMaps[1], sizeof(gBosTmTableMaps[0]));
            break;
        case 16:
            LoadBgMap(1, gBosTmTableMaps[0], sizeof(gBosTmTableMaps[0]));
            break;
        }

        if (work->frame > 15) {
            work->frame = 0;
            work->state = BOS_TM_TBL_STATE_UP;
            work->tm->tableState = BOS_TM_TABLE_STATE_UP;
            flags = work->tm->flags | TM_FLAG_TABLE_JUST_RAISED;
            work->tm->flags = flags;
        } else {
            ColliderSetHeight(&work->collider, work->height);
            work->height += 3;
            work->frame++;
        }

        break;
    case BOS_TM_TBL_STATE_SINKING:
        switch (work->frame) {
        case 0:
            LoadBgMap(1, gBosTmTableMaps[1], sizeof(gBosTmTableMaps[0]));
            ColliderSetDisabled(&work->collider, TRUE);
            work->height = 0;
            break;
        case 1:
            LoadBgMap(1, gBosTmTableMaps[2], sizeof(gBosTmTableMaps[0]));
            break;
        case 2:
            LoadBgMap(1, gBosTmTableMaps[3], sizeof(gBosTmTableMaps[0]));
            break;
        case 3:
            LoadBgMap(1, gBosTmTableMaps[4], sizeof(gBosTmTableMaps[0]));
            break;
        case 4:
            LoadBgMap(1, gBosTmTableMaps[5], sizeof(gBosTmTableMaps[0]));
            break;
        case 5:
            LoadBgMap(1, gBosTmTableMaps[6], sizeof(gBosTmTableMaps[0]));
            break;
        case 6:
            LoadBgMap(1, gBosTmTableMaps[7], sizeof(gBosTmTableMaps[0]));
            break;
        case 7:
            LoadBgMap(1, gBosTmTableMaps[8], sizeof(gBosTmTableMaps[0]));
            break;
        case 8:
            DisableBg(1);
            break;
        }

        if (work->frame > 7) {
            work->frame = 0;
            work->state = BOS_TM_TBL_STATE_DOWN;
            work->tm->tableState = BOS_TM_TABLE_STATE_DOWN;
        } else {
            work->frame++;
        }

        break;
    }

    SetBgScroll(1, (u16)((gBtlWork->viewX >> 8) + 8), (u16)((gBtlWork->viewY >> 8) - 70));
    return 1;
}

void task_bos_tm_tbl_3(TmTblWork* work) {
    ColliderUnregister(&work->collider);
    DisableBg(1);
}

TaskDesc gTaskDescBosTmArm = {
    "task_bos_tm_arm",
    (TaskInitFunc)task_bos_tm_arm_0,
    (TaskUpdateFunc)task_bos_tm_arm_1,
    (TaskDrawFunc)task_bos_tm_arm_2,
    (TaskDestroyFunc)task_bos_tm_arm_3,
    sizeof(TmArmWork),
};

TaskDesc gTaskDescBosTmTbl = {
    "task_bos_tm_tbl",
    (TaskInitFunc)task_bos_tm_tbl_0,
    (TaskUpdateFunc)task_bos_tm_tbl_1,
    NULL,
    (TaskDestroyFunc)task_bos_tm_tbl_3,
    sizeof(TmTblWork),
};
