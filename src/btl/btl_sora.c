/**
 * btl_sora.c
 * Sora Battle Character
 */

#include "task_descriptors.h"
#include "engine.h"
#include "card_battle.h"
#include "engine_math.h"
#include "display.h"
#include "obj_api.h"
#include "btl.h"
#include "btl_effect.h"
#include "btl_api.h"
#include "sprites_btl.h"
#include "sprites_fld.h"
#include "sprites_sora.h"
#include "gba/keys.h"
#include "system_state.h"
#include "player_progression.h"
#include "fade.h"
#include "songs.h"
#include <string.h>
#include "btl_tasks.h"
#include "anim.h"
#include "battle.h"
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "btl_collision.h"
#include "card_api.h"
#include "fld_types.h"
#include "game_state.h"
#include "key.h"
#include "listpool.h"
#include "m4a_song.h"
#include "obj.h"
#include "player_progression_types.h"
#include "taskpool.h"
#include "types.h"
#include "key_state.h"
#include <stddef.h>
#include "sprite_palettes.h"
#include "battle_ids.h"
#include "card_label_data.h"
#include "card_ids.h"
#include "btl3.h"
#include "mode_riku_tutorial.h"
#include "sio_api.h"

static const AnimDef sBtlSoraAnimDefs[77] = {
    { gSor1ll50Frames, gSor1ll50Anims, gSor1ll50Tiles, 0 },
    { gSor1ll51Frames, gSor1ll51Anims, gSor1ll51Tiles, 0 },
    { gSor1ll55Frames, gSor1ll55Anims, gSor1ll55Tiles, 4 },
    { gSor1ll55Frames, gSor1ll55Anims, gSor1ll55Tiles, 3 },
    { gSor1ll55Frames, gSor1ll55Anims, gSor1ll55Tiles, 0 },
    { gSor1ll55Frames, gSor1ll55Anims, gSor1ll55Tiles, 1 },
    { gSor1ll55Frames, gSor1ll55Anims, gSor1ll55Tiles, 2 },
    { gSor1ll57Frames, gSor1ll57Anims, gSor1ll57Tiles, 4 },
    { gSor1ll57Frames, gSor1ll57Anims, gSor1ll57Tiles, 3 },
    { gSor1ll57Frames, gSor1ll57Anims, gSor1ll57Tiles, 0 },
    { gSor1ll57Frames, gSor1ll57Anims, gSor1ll57Tiles, 1 },
    { gSor1ll57Frames, gSor1ll57Anims, gSor1ll57Tiles, 2 },
    { gSor1ll56Frames, gSor1ll56Anims, gSor1ll56Tiles, 4 },
    { gSor1ll56Frames, gSor1ll56Anims, gSor1ll56Tiles, 3 },
    { gSor1ll56Frames, gSor1ll56Anims, gSor1ll56Tiles, 0 },
    { gSor1ll56Frames, gSor1ll56Anims, gSor1ll56Tiles, 1 },
    { gSor1ll56Frames, gSor1ll56Anims, gSor1ll56Tiles, 2 },
    { gSor1ll58Frames, gSor1ll58Anims, gSor1ll58Tiles, 4 },
    { gSor1ll58Frames, gSor1ll58Anims, gSor1ll58Tiles, 3 },
    { gSor1ll58Frames, gSor1ll58Anims, gSor1ll58Tiles, 0 },
    { gSor1ll58Frames, gSor1ll58Anims, gSor1ll58Tiles, 1 },
    { gSor1ll58Frames, gSor1ll58Anims, gSor1ll58Tiles, 2 },
    { gSor1ll59Frames, gSor1ll59Anims, gSor1ll59Tiles, 4 },
    { gSor1ll59Frames, gSor1ll59Anims, gSor1ll59Tiles, 3 },
    { gSor1ll59Frames, gSor1ll59Anims, gSor1ll59Tiles, 0 },
    { gSor1ll59Frames, gSor1ll59Anims, gSor1ll59Tiles, 1 },
    { gSor1ll59Frames, gSor1ll59Anims, gSor1ll59Tiles, 2 },
    { gSor1ll61Frames, gSor1ll61Anims, gSor1ll61Tiles, 5 },
    { gSor1ll61Frames, gSor1ll61Anims, gSor1ll61Tiles, 4 },
    { gSor1ll61Frames, gSor1ll61Anims, gSor1ll61Tiles, 1 },
    { gSor1ll61Frames, gSor1ll61Anims, gSor1ll61Tiles, 2 },
    { gSor1ll61Frames, gSor1ll61Anims, gSor1ll61Tiles, 3 },
    { gSor1ll60Frames, gSor1ll60Anims, gSor1ll60Tiles, 4 },
    { gSor1ll60Frames, gSor1ll60Anims, gSor1ll60Tiles, 3 },
    { gSor1ll60Frames, gSor1ll60Anims, gSor1ll60Tiles, 0 },
    { gSor1ll60Frames, gSor1ll60Anims, gSor1ll60Tiles, 1 },
    { gSor1ll60Frames, gSor1ll60Anims, gSor1ll60Tiles, 2 },
    { gSor1ll65Frames, gSor1ll65Anims, gSor1ll65Tiles, 0 },
    { gSor1ll50Frames, gSor1ll50Anims, gSor1ll50Tiles, 2 },
    { gSor1ll50Frames, gSor1ll50Anims, gSor1ll50Tiles, 3 },
    { gSor1ll50Frames, gSor1ll50Anims, gSor1ll50Tiles, 4 },
    { gSor1ll50Frames, gSor1ll50Anims, gSor1ll50Tiles, 6 },
    { gSor1ll08Frames, gSor1ll08Anims, gSor1ll08Tiles, 0 },
    { gSor1ll09Frames, gSor1ll09Anims, gSor1ll09Tiles, 0 },
    { gSor1ll09Frames, gSor1ll09Anims, gSor1ll09Tiles, 1 },
    { gSor1ll09Frames, gSor1ll09Anims, gSor1ll09Tiles, 2 },
    { gSor1ll09Frames, gSor1ll09Anims, gSor1ll09Tiles, 3 },
    { gSor1ll54Frames, gSor1ll54Anims, gSor1ll54Tiles, 3 },
    { gSor1ll54Frames, gSor1ll54Anims, gSor1ll54Tiles, 2 },
    { gSor1ll54Frames, gSor1ll54Anims, gSor1ll54Tiles, 0 },
    { gSor1ll54Frames, gSor1ll54Anims, gSor1ll54Tiles, 1 },
    { gSor1ll66Frames, gSor1ll66Anims, gSor1ll66Tiles, 0 },
    { gSor1ll07Frames, gSor1ll07Anims, gSor1ll07Tiles, 0 },
    { gSor1ll67Frames, gSor1ll67Anims, gSor1ll67Tiles, 0 },
    { gSor1ll68Frames, gSor1ll68Anims, gSor1ll68Tiles, 0 },
    { gSor1ll68Frames, gSor1ll68Anims, gSor1ll68Tiles, 1 },
    { gSor1ll68Frames, gSor1ll68Anims, gSor1ll68Tiles, 2 },
    { gSor1ll68Frames, gSor1ll68Anims, gSor1ll68Tiles, 3 },
    { gSor1ll67Frames, gSor1ll67Anims, gSor1ll67Tiles, 1 },
    { gSor1ll67Frames, gSor1ll67Anims, gSor1ll67Tiles, 3 },
    { gSor1ll69Frames, gSor1ll69Anims, gSor1ll69Tiles, 0 },
    { gSor1ll69Frames, gSor1ll69Anims, gSor1ll69Tiles, 1 },
    { gSor1ll69Frames, gSor1ll69Anims, gSor1ll69Tiles, 2 },
    { gSor1ll69Frames, gSor1ll69Anims, gSor1ll69Tiles, 3 },
    { gSor1ll70Frames, gSor1ll70Anims, gSor1ll70Tiles, 0 },
    { gSor1ll70Frames, gSor1ll70Anims, gSor1ll70Tiles, 1 },
    { gSor1ll70Frames, gSor1ll70Anims, gSor1ll70Tiles, 2 },
    { gSor1ll70Frames, gSor1ll70Anims, gSor1ll70Tiles, 3 },
    { gSor1ll71Frames, gSor1ll71Anims, gSor1ll71Tiles, 0 },
    { gSor1ll72Frames, gSor1ll72Anims, gSor1ll72Tiles, 0 },
    { gSor1ll73Frames, gSor1ll73Anims, gSor1ll73Tiles, 0 },
    { gSor1ll74Frames, gSor1ll74Anims, gSor1ll74Tiles, 0 },
    { gSor1ll74Frames, gSor1ll74Anims, gSor1ll74Tiles, 1 },
    { gSor1ll74Frames, gSor1ll74Anims, gSor1ll74Tiles, 2 },
    { gSor1ll75Frames, gSor1ll75Anims, gSor1ll75Tiles, 0 },
    { gSor1ll75Frames, gSor1ll75Anims, gSor1ll75Tiles, 1 },
    { gSor1ll75Frames, gSor1ll75Anims, gSor1ll75Tiles, 2 },
};

static const AnimDef sBtlSoraDirAnimDefs[6][5] = {
    { { gSor1ff02Frames, gSor1ff02Anims, gSor1ff02Tiles, 0 }, { gSor1bb02Frames, gSor1bb02Anims, gSor1bb02Tiles, 0 }, { gSor1fl02Frames, gSor1fl02Anims, gSor1fl02Tiles, 0 }, { gSor1ll02Frames, gSor1ll02Anims, gSor1ll02Tiles, 0 }, { gSor1bl02Frames, gSor1bl02Anims, gSor1bl02Tiles, 0 } },
    { { gSor1ff03Frames, gSor1ff03Anims, gSor1ff03Tiles, 0 }, { gSor1bb03Frames, gSor1bb03Anims, gSor1bb03Tiles, 0 }, { gSor1fl03Frames, gSor1fl03Anims, gSor1fl03Tiles, 0 }, { gSor1ll03Frames, gSor1ll03Anims, gSor1ll03Tiles, 0 }, { gSor1bl03Frames, gSor1bl03Anims, gSor1bl03Tiles, 0 } },
    { { gSor1ff03Frames, gSor1ff03Anims, gSor1ff03Tiles, 1 }, { gSor1bb03Frames, gSor1bb03Anims, gSor1bb03Tiles, 1 }, { gSor1fl03Frames, gSor1fl03Anims, gSor1fl03Tiles, 1 }, { gSor1ll03Frames, gSor1ll03Anims, gSor1ll03Tiles, 1 }, { gSor1bl03Frames, gSor1bl03Anims, gSor1bl03Tiles, 1 } },
    { { gSor1ff03Frames, gSor1ff03Anims, gSor1ff03Tiles, 2 }, { gSor1bb03Frames, gSor1bb03Anims, gSor1bb03Tiles, 2 }, { gSor1fl03Frames, gSor1fl03Anims, gSor1fl03Tiles, 2 }, { gSor1ll03Frames, gSor1ll03Anims, gSor1ll03Tiles, 2 }, { gSor1bl03Frames, gSor1bl03Anims, gSor1bl03Tiles, 2 } },
    { { gSor1ff03Frames, gSor1ff03Anims, gSor1ff03Tiles, 3 }, { gSor1bb03Frames, gSor1bb03Anims, gSor1bb03Tiles, 3 }, { gSor1fl03Frames, gSor1fl03Anims, gSor1fl03Tiles, 3 }, { gSor1ll03Frames, gSor1ll03Anims, gSor1ll03Tiles, 3 }, { gSor1bl03Frames, gSor1bl03Anims, gSor1bl03Tiles, 3 } },
    { { gSor1ff03Frames, gSor1ff03Anims, gSor1ff03Tiles, 4 }, { gSor1bb03Frames, gSor1bb03Anims, gSor1bb03Tiles, 4 }, { gSor1fl03Frames, gSor1fl03Anims, gSor1fl03Tiles, 4 }, { gSor1ll03Frames, gSor1ll03Anims, gSor1ll03Tiles, 4 }, { gSor1bl03Frames, gSor1bl03Anims, gSor1bl03Tiles, 4 } },
};

static const u16 sBtlSoraGroundSongs[4][4] = {
    { SONG_BTL_SR_FOOTL, SONG_BTL_SR_FOOTR, SONG_BTL_SR_JUMP, SONG_BTL_SR_LAND },
    { SONG_BTL_SR_STONEL, SONG_BTL_SR_STONER, SONG_BTL_SR_STONEJP, SONG_BTL_SR_STONELD },
    { SONG_BTL_SR_MUDL, SONG_BTL_SR_MUDR, SONG_BTL_SR_MUDJP, SONG_BTL_SR_MUDLD },
    { SONG_SYS_SR_STONEL, SONG_SYS_SR_STONER, SONG_SYS_SR_STONEJP, SONG_SYS_SR_STONELD },
};

static const s32 sBtlSoraSwing1AttackIds[18] = {
    12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45, 48, 51, 54, 57, 60, 63,
};

static const s32 sBtlSoraSwing2AttackIds[18] = {
    13, 16, 19, 22, 25, 28, 31, 34, 37, 40, 43, 46, 49, 52, 55, 58, 61, 64,
};

static const s32 sBtlSoraSwing3AttackIds[18] = {
    14, 17, 20, 23, 26, 29, 32, 35, 38, 41, 44, 47, 50, 53, 56, 59, 62, 65,
};

static const SoraAttackDef sBtlSoraSwing1 = { 2, sBtlSoraSwing1AttackIds, SONG_VO_SR_ATTACK01, SONG_BTL_SR_ATT00, 0, 0, NULL };

static const SoraAttackDef sBtlSoraSwing2 = { 7, sBtlSoraSwing2AttackIds, SONG_VO_SR_ATTACK03, SONG_BTL_SR_ATT01, 0, 0, NULL };

static const SoraAttackDef sBtlSoraSwing1Wide = { 12, sBtlSoraSwing1AttackIds, SONG_VO_SR_ATTACK01, SONG_BTL_SR_ATT00, 0, 0, NULL };

static const SoraAttackDef sBtlSoraSwing3 = { 17, sBtlSoraSwing3AttackIds, SONG_VO_SR_ATTACK05, SONG_BTL_SR_ATT02, 0, COMBO_FLAG_ZOOM_ON_HIT, NULL };

static const SoraAttackDef sBtlSoraAirSwing1Hop = { 27, sBtlSoraSwing1AttackIds, SONG_VO_SR_ATTACK02, SONG_BTL_SR_ATT01, -640, COMBO_FLAG_AERIAL_SWING, &sBtlSoraSwing1 };

static const SoraAttackDef sBtlSoraAirSwing1 = { 22, sBtlSoraSwing2AttackIds, SONG_VO_SR_ATTACK00, SONG_BTL_SR_ATT00, 0, COMBO_FLAG_AERIAL_SWING, &sBtlSoraSwing1 };

static const SoraAttackDef sBtlSoraAirSwing2 = { 27, sBtlSoraSwing1AttackIds, SONG_VO_SR_ATTACK02, SONG_BTL_SR_ATT01, 0, COMBO_FLAG_AERIAL_SWING, &sBtlSoraSwing2 };

static const SoraAttackDef sBtlSoraAirSwing3 = { 32, sBtlSoraSwing3AttackIds, SONG_VO_SR_ATTACK05, SONG_BTL_SR_ATT02, 0, COMBO_FLAG_AERIAL_SWING | COMBO_FLAG_ZOOM_ON_HIT, &sBtlSoraSwing3 };

static const u8 sBtlSoraSwingHitFrames[5] = {
    10, 12, 15, 18, 20,
};

TaskDesc gTaskDescBtlSora = {
    "task_btl_sora",
    (TaskInitFunc)task_btl_sora_0,
    (TaskUpdateFunc)task_btl_sora_1,
    (TaskDrawFunc)task_btl_sora_2,
    (TaskDestroyFunc)task_btl_sora_3,
    sizeof(BtlSoraWork),
};

void EnableBtlSoraPassThrough(BtlSoraWork* work) {
    u16 flags = work->flags | BTL_SORA_FLAG_PASS_THROUGH;
    u16 colliderFlags;

    work->flags = flags;
    colliderFlags = work->actor.collider.flags | COLLIDER_FLAG_PASS_THROUGH;
    work->actor.collider.flags = colliderFlags;
}

void DisableBtlSoraPassThrough(BtlSoraWork* work) {
    u16 flags = work->flags & ~BTL_SORA_FLAG_PASS_THROUGH;
    u16 colliderFlags;

    work->flags = flags;
    colliderFlags = work->actor.collider.flags & ~COLLIDER_FLAG_PASS_THROUGH;
    work->actor.collider.flags = colliderFlags;
}

u16 GetBtlSoraComboType(BtlSoraWork* work) {
    BtlObj* act;
    BtlObj* target;
    s32 delta;

    act = work->actor.btl->actor;
    target = work->actor.btl->actor2;

    if (work->actor.btl->flags & BTL_FLAG_PLAYER_AIRBORNE) {
        return COMBO_TYPE_AIRBORNE;
    }

    if (target == NULL) {
        return COMBO_TYPE_NORMAL;
    }

    if (act->z - target->z > 0x2000) {
        return COMBO_TYPE_TARGET_ABOVE;
    }

    delta = target->x - act->x;

    if (delta >= 0 ? delta > 0x2800 : act->x - target->x > 0x2800) {
        return COMBO_TYPE_TARGET_FAR_X;
    }

    delta = target->y - act->y;

    if (delta >= 0 ? delta > 0xC00 : act->y - target->y > 0xC00) {
        return COMBO_TYPE_TARGET_FAR_Y;
    }

    return COMBO_TYPE_NORMAL;
}

void FocusBtlSoraCameraOnTarget(BtlSoraWork* work) {
    BtlObj* target;
    s32 x;
    s32 y;
    s32 z;

    if (!work->mainSide) {
        return;
    }

    target = work->actor.btl->actor2;

    if (target != NULL) {
        if (gBtlWork->flags & BTL_FLAG_VS_BATTLE) {
            x = (work->actor.x + target->x) >> 1;
            x = (work->actor.x + x) >> 1;
            y = work->actor.y;
            z = work->actor.z;

            if (z < -0x3200) {
                z = -0x3200;
            }
        } else {
            x = (work->actor.x + target->x) >> 1;
            y = (work->actor.y + target->y) >> 1;
            z = (work->actor.z + target->z) >> 1;
        }

        BtlMapFollowPosition(x, y, z);
    } else {
        BtlMapFollowPosition(work->actor.x, work->actor.y, work->actor.z);
    }
}

void FocusBtlSoraCameraOnBgFx(BtlSoraWork* work) {
    s32 x;
    s32 y;
    s32 z;

    if (work->mainSide) {
        BgFxGetPosition(&x, &y, &z);
        BtlMapFollowPosition(x, gBtlWork->actor->y, gBtlWork->actor->z);
    }
}

void FocusBtlSoraCamera(BtlSoraWork* work) {
    if (work->mainSide) {
        BtlMapFollowPosition(work->actor.x, work->actor.y, work->actor.z);
    }
}

void SetBtlSoraAnimation(BtlSoraWork* work, u16 index, u16 flags) {
    const FldAnimDef* def;

    def = &sBtlSoraAnimDefs[index];
    AnimChangeWithTables(&work->anim, def->animId, flags, def->anims, def->gfxTable);
    SetObjTileSource(work->tiles, def->tiles);
}

void SetBtlSoraDirAnimation(BtlSoraWork* work, u16 index, u16 flags) {
    const FldAnimDef* def;
    s32 dir;

    dir = 0;

    switch (ANGLE_DIR8(work->angle)) {
    case DIR8_UP:
        dir = 1;
        break;
    case DIR8_DOWN:
        dir = 0;
        break;
    case DIR8_DOWN_RIGHT:
    case DIR8_DOWN_LEFT:
        dir = 2;
        break;
    case DIR8_RIGHT:
    case DIR8_LEFT:
        dir = 3;
        break;
    case DIR8_UP_RIGHT:
    case DIR8_UP_LEFT:
        dir = 4;
        break;
    }

    def = &sBtlSoraDirAnimDefs[index][dir];
    AnimChangeWithTables(&work->anim, def->animId, flags, def->anims, def->gfxTable);
    SetObjTileSource(work->tiles, def->tiles);
}

void LoadBtlSoraPalette(BtlSoraWork* work) {
    work->tiles = work->actor.btl->tiles;

    if (work->mainSide) {
        work->palette = LoadObjPalette(gSoraPalette, sizeof(gSoraPalette));
    } else {
        work->palette = LoadObjPalette(gBtlOtherSidePalette, sizeof(gBtlOtherSidePalette));
    }
}

void ReleaseBtlSoraPalette(BtlSoraWork* work) {
    if (work->palette != NULL) {
        ReleaseObjPalette(work->palette);
    }

    work->tiles = NULL;
    work->palette = NULL;
}

void UpdateBtlSoraWalk(BtlSoraWork* work, u16 held) {
    BtlObj* act;

    act = &work->actor;

    if ((held & 0x10) && (held & 0x40)) {
        work->angle = 0x20;
        act->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
    } else if ((held & 0x10) && (held & 0x80)) {
        work->angle = 0x60;
        act->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
    } else if ((held & 0x20) && (held & 0x80)) {
        work->angle = 0xA0;
        act->flags |= BTLOBJ_FLAG_FACING_LEFT;
    } else if ((held & 0x20) && (held & 0x40)) {
        work->angle = 0xE0;
        act->flags |= BTLOBJ_FLAG_FACING_LEFT;
    } else if (held & 0x40) {
        work->angle = 0;
    } else if (held & 0x10) {
        work->angle = 0x40;
        act->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
    } else if (held & 0x80) {
        work->angle = 0x80;
    } else if (held & 0x20) {
        work->angle = 0xC0;
        act->flags |= BTLOBJ_FLAG_FACING_LEFT;
    }

    if (held & 0xF0) {
        SetBtlSoraDirAnimation(work, 0, 1);

        if (work->anim.timer == 0) {
            switch (work->anim.frame) {
            case 3:
                m4aSongNumStart(work->groundSongs[0]);
                break;
            case 7:
                m4aSongNumStart(work->groundSongs[1]);
                break;
            }
        }
    } else {
        SetBtlSoraAnimation(work, 1, 1);
    }

    if (held & 0xF0) {
        if (act->btl->hcEffect == HC_EFFECT_DASH) {
            work->speed += 256;

            if (work->speed > 1024) {
                work->speed = 1024;
            }
        } else {
            work->speed += 128;

            if (work->speed > 614) {
                work->speed = 614;
            }
        }
    } else {
        work->speed -= 128;

        if (work->speed < 0) {
            work->speed = 0;
        }
    }
}

enum BtlSoraState {
    BTL_SORA_STATE_ENTER,
    BTL_SORA_STATE_IDLE,
    BTL_SORA_STATE_JUMP,
    BTL_SORA_STATE_AIRBORNE,
    BTL_SORA_STATE_LAND,
    BTL_SORA_STATE_SONIC_BLADE,
    BTL_SORA_STATE_FIRE,
    BTL_SORA_STATE_BLIZZARD,
    BTL_SORA_STATE_THUNDER,
    BTL_SORA_STATE_GRAVITY,
    BTL_SORA_STATE_CURE,
    BTL_SORA_STATE_STOP,
    BTL_SORA_STATE_ITEM,
    BTL_SORA_STATE_HURT,
    BTL_SORA_STATE_DEFEATED,
    BTL_SORA_STATE_ESCAPE,
    BTL_SORA_STATE_COMBO,
    BTL_SORA_STATE_SUMMON,
    BTL_SORA_STATE_SUMMON_IDLE,
    BTL_SORA_STATE_SUMMON_JUMP,
    BTL_SORA_STATE_SUMMON_AIRBORNE,
    BTL_SORA_STATE_SUMMON_LAND,
    BTL_SORA_STATE_SUMMON_DODGE,
    BTL_SORA_STATE_SUMMON_TAKEOFF,
    BTL_SORA_STATE_SUMMON_EXIT,
    BTL_SORA_STATE_SUMMON_OFFSCREEN,
    BTL_SORA_STATE_SUMMON_RETURN,
    BTL_SORA_STATE_SUMMON_RETURN_LAND,
    BTL_SORA_STATE_CAST_POSE,
    BTL_SORA_STATE_CARD_BROKEN,
    BTL_SORA_STATE_DODGE,
    BTL_SORA_STATE_RAID,
    BTL_SORA_STATE_RAID_CATCH,
    BTL_SORA_STATE_GUARDED,
    BTL_SORA_STATE_STUNNED,
    BTL_SORA_STATE_FIELD_HIDDEN,
    BTL_SORA_STATE_SLIP,
    BTL_SORA_STATE_SLIP_RECOVER,
    BTL_SORA_STATE_HAZARD,
    BTL_SORA_STATE_TRINITY_LIMIT,
    BTL_SORA_STATE_TRINITY_LIMIT_JUMP,
    BTL_SORA_STATE_TRINITY_LIMIT_CHARGE,
    BTL_SORA_STATE_TRINITY_LIMIT_BLAST,
    BTL_SORA_STATE_STOPPED,
    BTL_SORA_STATE_WARP,
    BTL_SORA_STATE_RAGNAROK,
    BTL_SORA_STATE_RAGNAROK_JUMP,
    BTL_SORA_STATE_RAGNAROK_HOVER,
    BTL_SORA_STATE_RAGNAROK_CHARGE,
    BTL_SORA_STATE_RAGNAROK_SHOT,
    BTL_SORA_STATE_ARS_ARCANUM,
    BTL_SORA_STATE_ARS_ARCANUM_FINISH,
    BTL_SORA_STATE_BLITZ,
    BTL_SORA_STATE_BLITZ_STRIKE,
    BTL_SORA_STATE_STUN_IMPACT = 55,
    BTL_SORA_STATE_SLIDING_DASH,
    BTL_SORA_STATE_SLIDING_DASH_SLIDE,
    BTL_SORA_STATE_SLIDING_DASH_END,
    BTL_SORA_STATE_ZANTETSUKEN,
    BTL_SORA_STATE_ZANTETSUKEN_SLASH,
    BTL_SORA_STATE_ZANTETSUKEN_END,
    BTL_SORA_STATE_MEGA_FLARE,
    BTL_SORA_STATE_FIRAGA_BREAK,
    BTL_SORA_STATE_AQUA_SPLASH,
    BTL_SORA_STATE_AQUA_SPLASH_JUMP,
    BTL_SORA_STATE_AQUA_SPLASH_HOVER,
    BTL_SORA_STATE_AQUA_SPLASH_SPRAY,
    BTL_SORA_STATE_GIFTED_MIRACLE,
    BTL_SORA_STATE_HOMING_FIRA,
    BTL_SORA_STATE_HOMING_BLIZZARA,
    BTL_SORA_STATE_QUAKE,
    BTL_SORA_STATE_SYNCHRO,
    BTL_SORA_STATE_SHOCK_IMPACT,
    BTL_SORA_STATE_TELEPORT,
    BTL_SORA_STATE_TELEPORT_ARRIVE,
    BTL_SORA_STATE_WARPINATOR,
    BTL_SORA_STATE_TERROR,
    BTL_SORA_STATE_HOLY,
    BTL_SORA_STATE_TORNADO,
    BTL_SORA_STATE_CONFUSE,
    BTL_SORA_STATE_BIND,
    BTL_SORA_STATE_RECOVER,
    BTL_SORA_STATE_FROZEN,
    BTL_SORA_STATE_REVIVE,
    BTL_SORA_STATE_END_BATTLE,
    BTL_SORA_STATE_AERO,
    BTL_SORA_STATE_GRAVITY_SQUASH,
    BTL_SORA_STATE_GRAVITY_HOLD,
    BTL_SORA_STATE_GRAVITY_RECOVER
};

void task_btl_sora_0(BtlSoraWork* work, BtlTaskArg* arg) {
    BtlObj* act;

    act = &work->actor;
    work->flags = 0;

    if (arg != NULL) {
        if (arg->side == SIO_PLAYER_PARENT) {
            act->x = 0xC000;
            act->flags = 0;
            work->sioKeysA = TRUE;
        } else {
            act->x = 0x14000;
            act->flags = BTLOBJ_FLAG_FACING_LEFT;
            work->sioKeysA = FALSE;
        }

        if (arg->mainSide) {
            work->mainSide = TRUE;
            act->btl = gBtlWork;
            act->maxHp = gGameState.linkMaxHp;
            act->hp = gGameState.linkMaxHp;
            act->attack = gGameState.linkAp;
        } else {
            work->mainSide = FALSE;
            act->btl = gRikuBtlWork;
            act->maxHp = gGameState.linkPartnerMaxHp;
            act->hp = gGameState.linkPartnerMaxHp;
            act->attack = gGameState.linkPartnerAp;
        }
    } else {
        work->mainSide = TRUE;
        work->sioKeysA = TRUE;
        act->btl = gBtlWork;

        if ((act->btl->flags & BTL_FLAG_HUM_BATTLE) || (act->btl->flags & BTL_FLAG_TUTORIAL)) {
            act->x = 0xC000;
        } else {
            act->x = 0x10000;
        }

        act->flags = 0;
        act->attack = gGameState.progression.ap;
        act->maxHp = gGameState.progression.maxHp;
        act->hp = gGameState.hp;

        if (act->hp > act->maxHp) {
            act->hp = act->maxHp;
        }
    }

    act->flags |= BTLOBJ_FLAG_IMMUNE_TERROR;
    act->flags |= BTLOBJ_FLAG_PLAYER;
    act->flags |= (BTLOBJ_FLAG_INTANGIBLE | BTLOBJ_FLAG_CARD_USE_BLOCKED);

    if (gBtlWork->flags & BTL_FLAG_BOSS_BATTLE) {
        act->y = 0x16000;
    } else {
        act->y = 0x18100;
    }

    act->invincibleTimer = 0;
    act->delayedDamage = 0;
    act->z = 0;
    act->groundZ = 0;
    act->damage = 0;
    act->height = 32;
    act->radiusX = 12;
    act->radiusY = 6;
    act->centerHeight = 12;
    act->kind = BTLOBJ_KIND_PLAYER;
    act->badStatusTimer = 0;
    act->floorZ = 0;
    act->parent = NULL;
    act->badStatus = BAD_STATUS_NONE;
    act->popCooldown = 0;
    act->vx = act->vy = 0;

    // @bug arg is NULL in normal battles (NULL read).
    if (BIOS_PTR(arg)->mainSide) {
        ColliderInit(&act->collider, COLLIDER_TYPE_PLAYER, act->radiusX, act->height);
    } else {
        ColliderInit(&act->collider, COLLIDER_TYPE_OPPONENT, act->radiusX, act->height);
    }

    gBtlWork->targetX = act->x;
    gBtlWork->targetY = act->y;
    gBtlWork->targetZ = act->z;
    LoadBtlSoraPalette(work);
    act->btl->actor = act;
    AnimInit(&work->anim, NULL, NULL);
    SetBtlSoraAnimation(work, 1, 1);
    work->gfx = AnimGetGfx(&work->anim);
    work->state = BTL_SORA_STATE_ENTER;
    work->nextState = BTL_SORA_STATE_ENTER;
    work->vz = 0;
    act->vx = 0;
    act->vy = 0;
    work->stateTimer = 0;
    work->steps = 0;
    work->speed = 0;
    work->angle = 0;
    work->comboCount = 0;
    work->tapTimers[0] = 0;
    work->tapTimers[1] = 0;
    work->task = NULL;
    work->breakAnim = 2;
    work->scaleX = work->scaleY = Q_8_8(1);
    work->frameCount = 0;

    if (gBtlWork->flags & (BTL_FLAG_BOSS_BATTLE | BTL_FLAG_HUM_BATTLE)) {
        switch (gBtlWork->battleId) {
        case BATTLE_GUARD_ARMOR:
        case BATTLE_TRICKMASTER:
        case BATTLE_OOGIE_BOOGIE:
        case BATTLE_RIKU_1:
        case BATTLE_AXEL_1:
        case BATTLE_LARXENE_1:
        case BATTLE_VEXEN_1:
        case BATTLE_MARLUXIA:
        case BATTLE_LEXAEUS:
        case BATTLE_RIKU_2:
        case BATTLE_RIKU_3:
        case BATTLE_RIKU_4:
        case BATTLE_RIKU_5:
        case BATTLE_RIKU_6:
        case BATTLE_AXEL_2:
        case BATTLE_LARXENE_2:
            work->groundSongs = sBtlSoraGroundSongs[1];
            break;
        case BATTLE_PARASITE_CAGE:
            work->groundSongs = sBtlSoraGroundSongs[2];
            break;
        case BATTLE_HOOK:
            work->groundSongs = sBtlSoraGroundSongs[3];
            break;
        default:
            work->groundSongs = sBtlSoraGroundSongs[0];
            break;
        }
    } else {
        switch (gGameState.battleStage) {
        case BATTLE_STAGE_WONDERLAND:
        case BATTLE_STAGE_GARDEN:
            work->groundSongs = sBtlSoraGroundSongs[0];
            break;
        case BATTLE_STAGE_AGRABAH:
        case BATTLE_STAGE_OLYMPUS_COLISEUM:
        case BATTLE_STAGE_HALLOWEEN_TOWN:
            work->groundSongs = sBtlSoraGroundSongs[1];
            break;
        case BATTLE_STAGE_ATLANTICA:
        case BATTLE_STAGE_MONSTRO:
            work->groundSongs = sBtlSoraGroundSongs[2];
            break;
        default:
            work->groundSongs = sBtlSoraGroundSongs[0];
            break;
        }
    }

    TaskPoolInit(&work->tasks, 7);
    TaskCreate(&work->tasks, &gTaskDescBtlShadow, act);
    TaskCreate(&work->tasks, &gTaskDescBtlBadstatus, act);
}

void SetBtlSoraState(BtlSoraWork* work, u32 state) {
    work->state = state;
    work->steps = 0;
    work->stateTimer = 0;
    ClearBtlObjActionFlags(&work->actor);
}

void SetBtlSoraStateNoReset(BtlSoraWork* work, u32 state) {
    work->state = state;
    ClearBtlObjActionFlags(&work->actor);
}

void StartBtlSoraCombo(BtlSoraWork* work) {
    u16 flags;

    if (work->state == BTL_SORA_STATE_COMBO && work->comboCount <= 1) {
        work->comboCount++;
        work->stateTimer = 0;
        work->steps = 0;
    } else {
        switch (GetBtlSoraComboType(work)) {
        case COMBO_TYPE_NORMAL:
            work->attacks[0] = &sBtlSoraSwing1;
            work->attacks[1] = &sBtlSoraSwing2;
            work->attacks[2] = &sBtlSoraSwing3;
            break;
        case COMBO_TYPE_TARGET_FAR_X:
            work->attacks[0] = &sBtlSoraSwing2;
            work->attacks[1] = &sBtlSoraSwing1;
            work->attacks[2] = &sBtlSoraSwing3;
            break;
        case COMBO_TYPE_TARGET_ABOVE:
            work->attacks[0] = &sBtlSoraAirSwing1;
            work->attacks[1] = &sBtlSoraAirSwing2;
            work->attacks[2] = &sBtlSoraAirSwing3;
            break;
        case COMBO_TYPE_AIRBORNE:
            work->attacks[0] = &sBtlSoraAirSwing1Hop;
            work->attacks[1] = &sBtlSoraAirSwing1;
            work->attacks[2] = &sBtlSoraAirSwing3;
            break;
        case COMBO_TYPE_TARGET_FAR_Y:
        default:
            work->attacks[0] = &sBtlSoraSwing1Wide;
            work->attacks[1] = &sBtlSoraSwing2;
            work->attacks[2] = &sBtlSoraSwing3;
            break;
        }

        work->state = BTL_SORA_STATE_COMBO;
        work->steps = 0;
        work->stateTimer = 0;
        work->comboCount = 0;
        flags = work->flags & ~BTL_SORA_FLAG_COMBO_EXTENDED;
        work->flags = flags;
    }

    if (work->actor.btl->hcEffect == HC_EFFECT_ATTACK_HASTE) {
        work->swingSpeed = 0;
    }
}

void StartBtlSoraKnockback(BtlSoraWork* work) {
    work->vz = -work->actor.knockbackLift * 3;
    work->actor.vx = ((gSineTable[work->actor.angle] << 1) * work->actor.knockbackSpeed) >> 8;
    work->actor.vy = ((-gSineTable[work->actor.angle + 0x40] << 1) * work->actor.knockbackSpeed) >> 8;
}

BtlObj* PickBtlSoraTarget(BtlSoraWork* work) {
    BtlObj* list[10];
    BtlObj* enemy;
    s16 n;

    if (work->actor.btl->actor2 != NULL) {
        return work->actor.btl->actor2;
    }

    if (gBtlWork->flags & BTL_FLAG_VS_BATTLE) {
        if (work->mainSide) {
            enemy = gRikuBtlWork->actor;
        } else {
            enemy = gBtlWork->actor;
        }

        if (enemy->hp <= 0) {
            return NULL;
        }

        return enemy;
    }

    n = 0;
    enemy = ListPoolFirst(&gBtlWork->pool);

    if (enemy != NULL) {
        list[0] = enemy;
        n = 1;

        do {
            enemy = ListPoolNext(&enemy->node);

            if (enemy == NULL) {
                break;
            }

            list[n] = enemy;
            n++;
        } while (n <= 9);
    }

    if (n == 0) {
        return NULL;
    }

    enemy = list[GetRandom() % n];
    return enemy;
}

u16 SwapBtlSoraKeyBits(u16 keys, u16 bitA, u16 bitB) {
    u16 d;

    d = bitA;

    if (keys & bitA) {
        if ((keys & bitB) == 0) {
            keys &= ~bitA;
        }

        keys |= bitB;
    } else if (keys & bitB) {
        keys &= ~bitB;
        keys |= d;
    }

    return keys;
}

BtlObj* GetBtlSoraActiveOpponent(BtlSoraWork* work) {
    if (gBtlWork->flags & BTL_FLAG_VS_BATTLE) {
        if (work->mainSide) {
            if (gBtlWork->flags & BTL_FLAG_OPPONENT_CARD_ACTION) {
                return gRikuBtlWork->actor;
            }
        } else {
            if (gBtlWork->flags & BTL_FLAG_PLAYER_CARD_ACTION) {
                return gBtlWork->actor;
            }
        }
    } else {
        if (gBtlWork->flags & BTL_FLAG_OPPONENT_CARD_ACTION) {
            return gBtlWork->actor3;
        }
    }

    return NULL;
}

s32 task_btl_sora_1(BtlSoraWork* work) {
    BtlObj* act;
    BtlObj* enemy;
    s16 heal;
    u16 held;
    u16 pressed;
    u64 btlFlags;
    s16 timer;
    u16 elapsed;
    s32 attack;
    s32 hit;
    s32 targetZ;
    u32 move;
    const SoraAttackDef* swing;
    s32 stockMoves[6];
    u8 hitFrames[5];
    BtlObj* lockon;
    BtlTaskArgs args;
    BtlSpawnArgs spawn;
    BtlSpawnArgs spawn2;

    act = &work->actor;

    if (gBtlWork->phase == BTL_PHASE_END && (act->flags & BTLOBJ_FLAG_IN_CARD_ACTION)) {
        switch (work->state) {
        case BTL_SORA_STATE_SUMMON_TAKEOFF:
        case BTL_SORA_STATE_SUMMON_EXIT:
        case BTL_SORA_STATE_SUMMON_OFFSCREEN:
        case BTL_SORA_STATE_SUMMON_RETURN:
            act->x = act->originX;
            act->y = act->originY;
            act->z = act->originZ;

            if (work->flags & BTL_SORA_FLAG_HIDDEN) {
                work->flags &= ~BTL_SORA_FLAG_HIDDEN;
                LoadBtlSoraPalette(work);
            }

            SetBtlSoraState(work, BTL_SORA_STATE_AIRBORNE);
            break;
        case BTL_SORA_STATE_AQUA_SPLASH_SPRAY:
            m4aSongNumStop(SONG_EF_DAMBO_SPLOOP);
            SetBtlSoraState(work, BTL_SORA_STATE_RECOVER);
            break;
        case BTL_SORA_STATE_TORNADO:
            m4aSongNumStop(SONG_EF_TRUNEDO);
            SetBtlSoraState(work, BTL_SORA_STATE_RECOVER);
            break;
        default:
            SetBtlSoraState(work, BTL_SORA_STATE_RECOVER);
            break;
        }

        ColliderSetDisabled(&act->collider, FALSE);
        DisableBtlSoraPassThrough(work);
        act->flags &= ~0x0000200400800000LL;
        gBtlWork->flags |= BTL_FLAG_STOP_BGFX;
        act->btl->flags |= BTL_FLAG_DISMISS_SUMMONS;
    }

    if (CanLevelUp()) {
        if (LevelUp()) {
            CreateLevelUpEffectTask(act, &work->tasks);
        }
    }

    if (work->flags & BTL_SORA_FLAG_HC_STATUS) {
        work->flags &= ~BTL_SORA_FLAG_HC_STATUS;
        act->flags &= ~BTLOBJ_FLAGS_ELEMENT_AFFINITY;
    }

    switch (act->btl->hcEffect) {
    case HC_EFFECT_SECOND_CHANCE:
        work->flags |= BTL_SORA_FLAG_HC_STATUS;
        act->flags |= (BTLOBJ_FLAG_WEAK_FIRE | BTLOBJ_FLAG_RESIST_THUNDER);
        break;
    case HC_EFFECT_BERSERK:
        work->flags |= BTL_SORA_FLAG_HC_STATUS;
        act->flags |= (BTLOBJ_FLAG_WEAK_BLIZZARD | BTLOBJ_FLAG_RESIST_FIRE);
        break;
    case HC_EFFECT_QUICK_RECOVERY:
        work->flags |= BTL_SORA_FLAG_HC_STATUS;
        act->flags |= (BTLOBJ_FLAG_IMMUNE_FIRE | BTLOBJ_FLAG_WEAK_BLIZZARD);
        break;
    case HC_EFFECT_DASH:
        work->flags |= BTL_SORA_FLAG_HC_STATUS;
        act->flags |= (BTLOBJ_FLAG_IMMUNE_THUNDER | BTLOBJ_FLAG_WEAK_NEUTRAL);
        break;
    case HC_EFFECT_AUTO_LIFE:
        work->flags |= BTL_SORA_FLAG_HC_STATUS;
        act->flags |= (BTLOBJ_FLAG_IMMUNE_BLIZZARD | BTLOBJ_FLAG_WEAK_FIRE);
        break;
    case HC_EFFECT_DOUBLE_SLEIGHT:
        work->flags |= BTL_SORA_FLAG_HC_STATUS;
        act->flags |= (BTLOBJ_FLAG_WEAK_PHYSICAL | BTLOBJ_FLAG_RESIST_FIRE | BTLOBJ_FLAG_RESIST_BLIZZARD | BTLOBJ_FLAG_RESIST_THUNDER | BTLOBJ_FLAG_RESIST_NEUTRAL);
        break;
    case HC_EFFECT_WARP_BREAK:
        work->flags |= BTL_SORA_FLAG_HC_STATUS;
        act->flags |= (BTLOBJ_FLAG_IMMUNE_BLIZZARD | BTLOBJ_FLAG_RESIST_PHYSICAL | BTLOBJ_FLAG_WEAK_NEUTRAL);
        break;
    case HC_EFFECT_SLEIGHT_LOCK:
    case HC_EFFECT_SLEIGHTBLIND:
        work->flags |= BTL_SORA_FLAG_HC_STATUS;
        act->flags |= (BTLOBJ_FLAG_RESIST_FIRE | BTLOBJ_FLAG_RESIST_BLIZZARD | BTLOBJ_FLAG_RESIST_THUNDER);
        break;
    }

    act->flags &= ~BTLOBJ_FLAG_GUARD_PHYSICAL;

    switch ((u32)act->btl->hcEffect) {
    case HC_EFFECT_GUARD:
        enemy = GetBtlSoraActiveOpponent(work);

        if (enemy != NULL) {
            if ((enemy->x < act->x && (act->flags & BTLOBJ_FLAG_FACING_LEFT)) ||
                (enemy->x > act->x && !(act->flags & BTLOBJ_FLAG_FACING_LEFT))) {
                act->flags |= BTLOBJ_FLAG_GUARD_PHYSICAL;
            }
        }

        break;
    case HC_EFFECT_REGEN: {
        u16 hp;
        s16 maxHp;
        s32 newHp;
        hp = act->hp;

        if ((s16)hp > 0) {
            maxHp = act->maxHp;

            if ((s16)hp < maxHp && work->frameCount % 120 == 0) {
                heal = (act->maxHp - act->hp) << 13 >> 16;

                if (heal <= 0) {
                    heal = 1;
                }

                if (act->badStatus != BAD_STATUS_STOP) {
                    newHp = heal + hp;
                    act->hp = newHp;
                }

                if (act->hp > maxHp) {
                    act->hp = maxHp;
                }

                act->btl->hcEffectCount--;
            }
        }

        break;
    }
    case HC_EFFECT_BIO: {
        BtlObj* enemy;
        u16 hp;

        if (work->frameCount % 20 == 0) {
            if (gBtlWork->flags & BTL_FLAG_VS_BATTLE) {
                if (work->mainSide) {
                    enemy = gRikuBtlWork->actor;
                } else {
                    enemy = gBtlWork->actor;
                }

                hp = enemy->hp;

                if ((s16)hp > 1 && enemy->badStatus != BAD_STATUS_STOP) {
                    enemy->hp = hp - 1;
                }
            } else {
                enemy = ListPoolFirst(&gBtlWork->pool);

                while (enemy != NULL) {
                    hp = enemy->hp;

                    if ((s16)hp > 1 && enemy->badStatus != BAD_STATUS_STOP) {
                        enemy->hp = hp - 1;
                    }

                    enemy = ListPoolNext(&enemy->node);
                }
            }
        }

        break;
    }
    }

    if (gBtlWork->flags & BTL_FLAG_VS_BATTLE) {
        if (work->sioKeysA) {
            held = SioKeyGetHeldA();
            pressed = SioKeyGetPressedA();
        } else {
            held = SioKeyGetHeldB();
            pressed = SioKeyGetPressedB();
        }
    } else {
        held = GetKeysHeld();
        pressed = GetKeysPressed();
#ifdef PLATFORM_PC
        {
            /* Dodge button: the second tap of a d-pad double tap. */
            u16 dodge = PortTakeDodge(held, (act->flags & BTLOBJ_FLAG_FACING_LEFT) != 0);

            if (dodge != 0) {
                work->tapTimers[dodge == DPAD_LEFT ? 0 : 1] = 13;
                pressed |= dodge;
            }
        }
#endif
    }

    btlFlags = gBtlWork->flags;

    if (btlFlags & BTL_FLAG_TUTORIAL) {
        if (btlFlags & BTL_FLAG_TUTORIAL_NO_CONTROL) {
            held &= ~A_BUTTON;
            held &= ~B_BUTTON;
            held &= ~DPAD_UP;
            held &= ~DPAD_DOWN;
            held &= ~DPAD_LEFT;
            held &= ~DPAD_RIGHT;
            pressed &= ~A_BUTTON;
            pressed &= ~B_BUTTON;
            pressed &= ~DPAD_UP;
            pressed &= ~DPAD_DOWN;
            pressed &= ~DPAD_LEFT;
            pressed &= ~DPAD_RIGHT;
        }

        if (btlFlags & BTL_FLAG_TUTORIAL_NO_JUMP) {
            pressed &= ~B_BUTTON;
        }

        if (btlFlags & BTL_FLAG_TUTORIAL_NO_DODGE) {
            pressed &= ~DPAD_LEFT;
            pressed &= ~DPAD_RIGHT;
        }

        if (btlFlags & BTL_FLAG_TUTORIAL_NO_CARD_USE) {
            pressed &= ~A_BUTTON;
            held &= ~A_BUTTON;
        }
    }

    if (act->badStatus == BAD_STATUS_CONFUSE) {
        held = SwapBtlSoraKeyBits(held, 32, 16);
        held = SwapBtlSoraKeyBits(held, 64, 128);
        pressed = SwapBtlSoraKeyBits(pressed, 32, 16);
        pressed = SwapBtlSoraKeyBits(pressed, 64, 128);
    }

    if (work->state != BTL_SORA_STATE_ESCAPE && (act->btl->flags & BTL_FLAG_ESCAPED)) {
        act->flags |= BTLOBJ_FLAG_IGNORE_BOUNDS;
        work->state = BTL_SORA_STATE_ESCAPE;
        work->steps = 0;
        work->stateTimer = 0;
        act->flags |= BTLOBJ_FLAG_INTANGIBLE;
    } else {
        act->btl->flags &= ~BTL_FLAG_CAN_CHARGE_RELOAD;
    }

    switch (UpdateBtlObjReaction(act)) {
    case BTL_REACTION_GRAVITY:
        work->speed = 0;
        work->state = BTL_SORA_STATE_GRAVITY_SQUASH;
        work->steps = 0;
        work->stateTimer = 0;
        break;
    case BTL_REACTION_HURT:
        work->speed = 0;
        work->state = BTL_SORA_STATE_HURT;
        work->steps = 0;
        work->stateTimer = 0;
        break;
    case BTL_REACTION_DEFEATED:
    case BTL_REACTION_GRAVITY_DEFEATED:
        work->speed = 0;

        if (act->btl->hcEffect == HC_EFFECT_AUTO_LIFE) {
            work->state = BTL_SORA_STATE_REVIVE;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        }

        work->state = BTL_SORA_STATE_DEFEATED;
        work->steps = 0;
        work->stateTimer = 0;
        break;
    case BTL_REACTION_HEALED:
        act->flags &= ~BTLOBJ_FLAG_HURT;
        work->state = BTL_SORA_STATE_IDLE;
        work->steps = 0;
        work->stateTimer = 0;
        act->flags &= ~BTLOBJ_FLAG_CARD_ACTION_PENDING;
        break;
    case BTL_REACTION_CARD_ACTION:
        act->btl->flags &= ~BTL_FLAG_DISMISS_SUMMONS;
        act->btl->flags &= ~BTL_FLAG_PUSHING_EDGE;
        work->flags |= BTL_SORA_FLAG_PASS_THROUGH;

        if (gBtlWork->flags & BTL_FLAG_VS_BATTLE) {
            if (work->mainSide) {
                move = ResolveLinkActiveCardsMove(stockMoves, LINK_SIDE_SELF);
            } else {
                move = ResolveLinkActiveCardsMove(stockMoves, LINK_SIDE_PARTNER);
            }
        } else {
            move = ResolveActiveCardsMove(stockMoves);
        }

        if (move == MOVE_STOCK_SEQUENCE) {
            if (!(act->btl->flags & BTL_FLAG_STOCK_SEQUENCE)) {
                act->btl->flags |= BTL_FLAG_STOCK_SEQUENCE;
                act->btl->stockMove = 0;
            }

            move = stockMoves[act->btl->stockMove];
            act->btl->stockMove++;
        }

        work->breakAnim = 2;

        switch (move) {
        case MOVE_KINGDOM_KEY:
            work->keyblade = 0;
            work->swingSpeed = 2;
            work->breakAnim = 2;
            StartBtlSoraCombo(work);
            break;
        case MOVE_KEYBLADE_1:
            work->keyblade = 1;
            work->swingSpeed = 0;
            work->breakAnim = 3;
            StartBtlSoraCombo(work);
            break;
        case MOVE_OLYMPIA:
            work->keyblade = 2;
            work->swingSpeed = 3;
            work->breakAnim = 1;
            StartBtlSoraCombo(work);
            break;
        case MOVE_THREE_WISHES:
            work->keyblade = 3;
            work->swingSpeed = 1;
            work->breakAnim = 2;
            StartBtlSoraCombo(work);
            break;
        case MOVE_CRABCLAW:
            work->keyblade = 4;
            work->swingSpeed = 2;
            work->breakAnim = 0;
            StartBtlSoraCombo(work);
            break;
        case MOVE_PUMPKINHEAD:
            work->keyblade = 5;
            work->swingSpeed = 2;
            work->breakAnim = 1;
            StartBtlSoraCombo(work);
            break;
        case MOVE_FAIRY_HARP:
            work->keyblade = 6;
            work->swingSpeed = 0;
            work->breakAnim = 2;
            StartBtlSoraCombo(work);
            break;
        case MOVE_WISHING_STAR:
            work->keyblade = 7;
            work->swingSpeed = 1;
            work->breakAnim = 1;
            StartBtlSoraCombo(work);
            break;
        case MOVE_SPELLBINDER:
            work->keyblade = 8;
            work->swingSpeed = 3;
            work->breakAnim = 1;
            StartBtlSoraCombo(work);
            break;
        case MOVE_METAL_CHOCOBO:
            work->keyblade = 9;
            work->swingSpeed = 3;
            work->breakAnim = 2;
            StartBtlSoraCombo(work);
            break;
        case MOVE_LIONHEART:
            work->keyblade = 10;
            work->swingSpeed = 4;
            work->breakAnim = 1;
            StartBtlSoraCombo(work);
            break;
        case MOVE_LADY_LUCK:
            work->keyblade = 11;
            work->swingSpeed = 1;
            work->breakAnim = 2;
            StartBtlSoraCombo(work);
            break;
        case MOVE_DIVINE_ROSE:
            work->keyblade = 12;
            work->swingSpeed = 1;
            work->breakAnim = 3;
            StartBtlSoraCombo(work);
            break;
        case MOVE_OATHKEEPER:
            work->keyblade = 13;
            work->swingSpeed = 2;
            work->breakAnim = 2;
            StartBtlSoraCombo(work);
            break;
        case MOVE_OBLIVION:
            work->keyblade = 14;
            work->swingSpeed = 3;
            work->breakAnim = 1;
            StartBtlSoraCombo(work);
            break;
        case MOVE_ULTIMA_WEAPON:
            work->keyblade = 15;
            work->swingSpeed = 2;
            work->breakAnim = 2;
            StartBtlSoraCombo(work);
            break;
        case MOVE_DIAMOND_DUST:
            work->keyblade = 16;
            work->swingSpeed = 0;
            work->breakAnim = 0;
            StartBtlSoraCombo(work);
            break;
        case MOVE_ONE_WINGED_ANGEL:
            work->keyblade = 17;
            work->swingSpeed = 1;
            work->breakAnim = 3;
            StartBtlSoraCombo(work);
            break;
        case MOVE_FIRE:
            work->state = BTL_SORA_STATE_FIRE;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = SPELL_TIER_BASE;
            work->breakAnim = 1;
            break;
        case MOVE_FIRA:
            work->state = BTL_SORA_STATE_FIRE;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = SPELL_TIER_RA;
            work->breakAnim = 2;
            break;
        case MOVE_FIRAGA:
            work->state = BTL_SORA_STATE_FIRE;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = SPELL_TIER_GA;
            work->breakAnim = 3;
            break;
        case MOVE_BLIZZARD:
            work->state = BTL_SORA_STATE_BLIZZARD;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = SPELL_TIER_BASE;
            work->breakAnim = 1;
            break;
        case MOVE_BLIZZARA:
            work->state = BTL_SORA_STATE_BLIZZARD;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = SPELL_TIER_RA;
            work->breakAnim = 2;
            break;
        case MOVE_BLIZZAGA:
            work->state = BTL_SORA_STATE_BLIZZARD;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = SPELL_TIER_GA;
            work->breakAnim = 3;
            break;
        case MOVE_THUNDER:
            work->state = BTL_SORA_STATE_THUNDER;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = SPELL_TIER_BASE;
            work->breakAnim = 1;
            break;
        case MOVE_THUNDARA:
            work->state = BTL_SORA_STATE_THUNDER;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = SPELL_TIER_RA;
            work->breakAnim = 2;
            break;
        case MOVE_THUNDAGA:
            work->state = BTL_SORA_STATE_THUNDER;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = SPELL_TIER_GA;
            work->breakAnim = 3;
            break;
        case MOVE_CURE:
            work->state = BTL_SORA_STATE_CURE;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = SPELL_TIER_BASE;
            work->breakAnim = 0;
            break;
        case MOVE_CURA:
            work->state = BTL_SORA_STATE_CURE;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = SPELL_TIER_RA;
            work->breakAnim = 0;
            break;
        case MOVE_CURAGA:
            work->state = BTL_SORA_STATE_CURE;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = SPELL_TIER_GA;
            work->breakAnim = 0;
            break;
        case MOVE_STOP:
            work->state = BTL_SORA_STATE_STOP;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = SPELL_TIER_BASE;
            work->breakAnim = 1;
            break;
        case MOVE_STOPRA:
            work->state = BTL_SORA_STATE_STOP;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = SPELL_TIER_RA;
            work->breakAnim = 2;
            break;
        case MOVE_STOPGA:
            work->state = BTL_SORA_STATE_STOP;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = SPELL_TIER_GA;
            work->breakAnim = 3;
            break;
        case MOVE_GRAVITY:
            work->state = BTL_SORA_STATE_GRAVITY;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = SPELL_TIER_BASE;
            work->breakAnim = 1;
            break;
        case MOVE_GRAVIRA:
            work->state = BTL_SORA_STATE_GRAVITY;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = SPELL_TIER_RA;
            work->breakAnim = 2;
            break;
        case MOVE_GRAVIGA:
            work->state = BTL_SORA_STATE_GRAVITY;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = SPELL_TIER_GA;
            work->breakAnim = 3;
            break;
        case MOVE_GOOFY:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescFrdGoofy;
            work->variant[0] = SUMMON_LEVEL_SINGLE;
            break;
        case MOVE_GOOFY_CHARGE:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescFrdGoofy;
            work->variant[0] = SUMMON_LEVEL_PAIR;
            break;
        case MOVE_GOOFY_TORNADO:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescFrdGoofy;
            work->variant[0] = SUMMON_LEVEL_TRIPLE;
            break;
        case MOVE_DONALD_DUCK:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescFrdDonald;
            work->variant[0] = SUMMON_LEVEL_SINGLE;
            break;
        case MOVE_MAGIC_PAIR:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescFrdDonald;
            work->variant[0] = SUMMON_LEVEL_PAIR;
            break;
        case MOVE_MAGIC:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescFrdDonald;
            work->variant[0] = SUMMON_LEVEL_TRIPLE;
            break;
        case MOVE_TINKER_BELL:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescSmnTink;
            work->variant[0] = SUMMON_LEVEL_SINGLE;
            work->breakAnim = 1;
            break;
        case MOVE_TWINKLE_PAIR:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescSmnTink;
            work->variant[0] = SUMMON_LEVEL_PAIR;
            work->breakAnim = 1;
            break;
        case MOVE_TWINKLE:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescSmnTink;
            work->variant[0] = SUMMON_LEVEL_TRIPLE;
            work->breakAnim = 1;
            break;
        case MOVE_ARIEL:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescFrdAriel;
            work->variant[0] = SUMMON_LEVEL_SINGLE;
            break;
        case MOVE_SPIRAL_WAVE_PAIR:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescFrdAriel;
            work->variant[0] = SUMMON_LEVEL_PAIR;
            break;
        case MOVE_SPIRAL_WAVE:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescFrdAriel;
            work->variant[0] = SUMMON_LEVEL_TRIPLE;
            break;
        case MOVE_MUSHU:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescSmnMushu;
            work->variant[0] = SUMMON_LEVEL_SINGLE;
            break;
        case MOVE_FLARE_BREATH_PAIR:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescSmnMushu;
            work->variant[0] = SUMMON_LEVEL_PAIR;
            break;
        case MOVE_FLARE_BREATH:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescSmnMushu;
            work->variant[0] = SUMMON_LEVEL_TRIPLE;
            break;
        case MOVE_SIMBA:
            work->state = BTL_SORA_STATE_SUMMON_TAKEOFF;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescSmnSimba;
            work->variant[0] = SUMMON_LEVEL_SINGLE;
            break;
        case MOVE_PROUD_ROAR_PAIR:
            work->state = BTL_SORA_STATE_SUMMON_TAKEOFF;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescSmnSimba;
            work->variant[0] = SUMMON_LEVEL_PAIR;
            break;
        case MOVE_PROUD_ROAR:
            work->state = BTL_SORA_STATE_SUMMON_TAKEOFF;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescSmnSimba;
            work->variant[0] = SUMMON_LEVEL_TRIPLE;
            break;
        case MOVE_CLOUD:
            work->state = BTL_SORA_STATE_SUMMON_TAKEOFF;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescSmnCloud;
            work->variant[0] = SUMMON_LEVEL_SINGLE;
            break;
        case MOVE_CROSS_SLASH:
            work->state = BTL_SORA_STATE_SUMMON_TAKEOFF;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescSmnCloud;
            work->variant[0] = SUMMON_LEVEL_PAIR;
            break;
        case MOVE_OMNISLASH:
            work->state = BTL_SORA_STATE_SUMMON_TAKEOFF;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescSmnCloud;
            work->variant[0] = SUMMON_LEVEL_TRIPLE;
            break;
        case MOVE_BAMBI:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescSmnBambi;
            work->variant[0] = SUMMON_LEVEL_SINGLE;
            break;
        case MOVE_PARADISE_PAIR:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescSmnBambi;
            work->variant[0] = SUMMON_LEVEL_PAIR;
            break;
        case MOVE_PARADISE:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescSmnBambi;
            work->variant[0] = SUMMON_LEVEL_TRIPLE;
            break;
        case MOVE_JACK:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescFrdJack;
            work->variant[0] = SUMMON_LEVEL_SINGLE;
            break;
        case MOVE_SURPRISE_PAIR:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescFrdJack;
            work->variant[0] = SUMMON_LEVEL_PAIR;
            break;
        case MOVE_SURPRISE:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescFrdJack;
            work->variant[0] = SUMMON_LEVEL_TRIPLE;
            break;
        case MOVE_ALADDIN:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescFrdAladdin;
            work->variant[0] = SUMMON_LEVEL_SINGLE;
            break;
        case MOVE_SANDSTORM_PAIR:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescFrdAladdin;
            work->variant[0] = SUMMON_LEVEL_PAIR;
            break;
        case MOVE_SANDSTORM:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescFrdAladdin;
            work->variant[0] = SUMMON_LEVEL_TRIPLE;
            break;
        case MOVE_PETER_PAN:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescFrdPan;
            work->variant[0] = SUMMON_LEVEL_SINGLE;
            break;
        case MOVE_HUMMINGBIRD_PAIR:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescFrdPan;
            work->variant[0] = SUMMON_LEVEL_PAIR;
            break;
        case MOVE_HUMMINGBIRD:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescFrdPan;
            work->variant[0] = SUMMON_LEVEL_TRIPLE;
            break;
        case MOVE_DUMBO:
            work->state = BTL_SORA_STATE_SUMMON_TAKEOFF;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescSmnDumbo;
            work->variant[0] = SUMMON_LEVEL_SINGLE;
            break;
        case MOVE_SPLASH_PAIR:
            work->state = BTL_SORA_STATE_SUMMON_TAKEOFF;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescSmnDumbo;
            work->variant[0] = SUMMON_LEVEL_PAIR;
            break;
        case MOVE_SPLASH:
            work->state = BTL_SORA_STATE_SUMMON_TAKEOFF;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescSmnDumbo;
            work->variant[0] = SUMMON_LEVEL_TRIPLE;
            break;
        case MOVE_GENIE:
            work->state = BTL_SORA_STATE_SUMMON_TAKEOFF;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescSmnGenie;
            work->variant[0] = SUMMON_LEVEL_SINGLE;
            break;
        case MOVE_SHOWTIME_PAIR:
            work->state = BTL_SORA_STATE_SUMMON_TAKEOFF;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescSmnGenie;
            work->variant[0] = SUMMON_LEVEL_PAIR;
            break;
        case MOVE_SHOWTIME:
            work->state = BTL_SORA_STATE_SUMMON_TAKEOFF;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescSmnGenie;
            work->variant[0] = SUMMON_LEVEL_TRIPLE;
            break;
        case MOVE_THE_BEAST:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescFrdBeast;
            work->variant[0] = SUMMON_LEVEL_SINGLE;
            break;
        case MOVE_FEROCIOUS_LUNGE_PAIR:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescFrdBeast;
            work->variant[0] = SUMMON_LEVEL_PAIR;
            break;
        case MOVE_FEROCIOUS_LUNGE:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescFrdBeast;
            work->variant[0] = SUMMON_LEVEL_TRIPLE;
            break;
        case MOVE_POTION:
            work->state = BTL_SORA_STATE_ITEM;
            work->steps = 0;
            work->stateTimer = 0;
            work->breakAnim = 0;
            work->variant[0] = BTL_ITEM_POTION;
            break;
        case MOVE_HI_POTION:
            work->state = BTL_SORA_STATE_ITEM;
            work->steps = 0;
            work->stateTimer = 0;
            work->breakAnim = 0;
            work->variant[0] = BTL_ITEM_HI_POTION;
            break;
        case MOVE_MEGA_POTION:
            work->state = BTL_SORA_STATE_ITEM;
            work->steps = 0;
            work->stateTimer = 0;
            work->breakAnim = 0;
            work->variant[0] = BTL_ITEM_MEGA_POTION;
            break;
        case MOVE_ETHER:
            work->state = BTL_SORA_STATE_ITEM;
            work->steps = 0;
            work->stateTimer = 0;
            work->breakAnim = 0;
            work->variant[0] = BTL_ITEM_ETHER;
            break;
        case MOVE_MEGA_ETHER:
            work->state = BTL_SORA_STATE_ITEM;
            work->steps = 0;
            work->stateTimer = 0;
            work->breakAnim = 0;
            work->variant[0] = BTL_ITEM_MEGA_ETHER;
            break;
        case MOVE_ELIXIR:
            work->state = BTL_SORA_STATE_ITEM;
            work->steps = 0;
            work->stateTimer = 0;
            work->breakAnim = 0;
            work->variant[0] = BTL_ITEM_ELIXIR;
            break;
        case MOVE_MEGALIXIR:
            work->state = BTL_SORA_STATE_ITEM;
            work->steps = 0;
            work->stateTimer = 0;
            work->breakAnim = 0;
            work->variant[0] = BTL_ITEM_MEGALIXIR;
            break;
        case MOVE_AERO:
            work->state = BTL_SORA_STATE_AERO;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = SPELL_TIER_BASE;
            break;
        case MOVE_AERORA:
            work->state = BTL_SORA_STATE_AERO;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = SPELL_TIER_RA;
            break;
        case MOVE_AEROGA:
            work->state = BTL_SORA_STATE_AERO;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = SPELL_TIER_GA;
            break;
        case MOVE_SONIC_BLADE:
            work->state = BTL_SORA_STATE_SONIC_BLADE;
            work->steps = 0;
            work->stateTimer = 0;
            work->comboCount = 0;
            break;
        case MOVE_STRIKE_RAID:
            work->state = BTL_SORA_STATE_RAID;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = BTL_RAID_STRIKE;
            break;
        case MOVE_BLITZ:
            work->state = BTL_SORA_STATE_BLITZ;
            work->steps = 0;
            work->stateTimer = 0;
            work->comboCount = 2;
            break;
        case MOVE_TRINITY_LIMIT:
            work->state = BTL_SORA_STATE_TRINITY_LIMIT;
            work->steps = 0;
            work->stateTimer = 0;
            work->breakAnim = 3;
            break;
        case MOVE_ARS_ARCANUM:
            work->state = BTL_SORA_STATE_ARS_ARCANUM;
            work->steps = 0;
            work->stateTimer = 0;
            work->nextState = BTL_SORA_STATE_ARS_ARCANUM_FINISH;
            work->comboCount = 8;
            break;
        case MOVE_RAGNAROK:
            work->state = BTL_SORA_STATE_RAGNAROK;
            work->steps = 0;
            work->stateTimer = 0;
            work->breakAnim = 3;
            break;
        case MOVE_SLIDING_DASH:
            work->state = BTL_SORA_STATE_SLIDING_DASH;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        case MOVE_STUN_IMPACT:
            work->state = BTL_SORA_STATE_STUN_IMPACT;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        case MOVE_ZANTETSUKEN:
            work->state = BTL_SORA_STATE_ZANTETSUKEN;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        case MOVE_FIRAGA_BREAK:
            work->state = BTL_SORA_STATE_FIRAGA_BREAK;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        case MOVE_GIFTED_MIRACLE:
            work->state = BTL_SORA_STATE_GIFTED_MIRACLE;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        case MOVE_HOMING_FIRA:
            work->state = BTL_SORA_STATE_HOMING_FIRA;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        case MOVE_HOMING_BLIZZARA:
            work->state = BTL_SORA_STATE_HOMING_BLIZZARA;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        case MOVE_WARP:
            work->state = BTL_SORA_STATE_WARP;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        case MOVE_WARPINATOR:
            work->state = BTL_SORA_STATE_WARPINATOR;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        case MOVE_FIRE_RAID:
            work->state = BTL_SORA_STATE_RAID;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = BTL_RAID_FIRE;
            break;
        case MOVE_STOP_RAID:
            work->state = BTL_SORA_STATE_RAID;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = BTL_RAID_STOP;
            break;
        case MOVE_BLIZZARD_RAID:
            work->state = BTL_SORA_STATE_RAID;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = BTL_RAID_BLIZZARD;
            break;
        case MOVE_THUNDER_RAID:
            work->state = BTL_SORA_STATE_RAID;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = BTL_RAID_THUNDER;
            break;
        case MOVE_GRAVITY_RAID:
            work->state = BTL_SORA_STATE_RAID;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = BTL_RAID_GRAVITY;
            break;
        case MOVE_REFLECT_RAID:
            work->state = BTL_SORA_STATE_RAID;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = BTL_RAID_REFLECT;
            break;
        case MOVE_BLAZING_DONALD:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescFrdDonald;
            work->variant[0] = SUMMON_LEVEL_SLEIGHT;
            break;
        case MOVE_QUAKE:
            work->state = BTL_SORA_STATE_QUAKE;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        case MOVE_IDYLL_ROMP:
            work->state = BTL_SORA_STATE_SUMMON;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescSmnBambi;
            work->variant[0] = SUMMON_LEVEL_SLEIGHT;
            break;
        case MOVE_CROSS_SLASH_PLUS:
            work->state = BTL_SORA_STATE_SUMMON_TAKEOFF;
            work->steps = 0;
            work->stateTimer = 0;
            work->summonDesc = &gTaskDescSmnCloud;
            work->variant[0] = SUMMON_LEVEL_SLEIGHT;
            break;
        case MOVE_AQUA_SPLASH:
            work->state = BTL_SORA_STATE_AQUA_SPLASH;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        case MOVE_HOLY:
            work->state = BTL_SORA_STATE_HOLY;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        case MOVE_SYNCHRO:
            work->state = BTL_SORA_STATE_SYNCHRO;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        case MOVE_MEGA_FLARE:
            work->state = BTL_SORA_STATE_MEGA_FLARE;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        case MOVE_SHOCK_IMPACT:
            work->state = BTL_SORA_STATE_SHOCK_IMPACT;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        case MOVE_TELEPORT:
            work->state = BTL_SORA_STATE_TELEPORT;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        case MOVE_TERROR:
            work->state = BTL_SORA_STATE_TERROR;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        case MOVE_JUDGMENT:
            work->state = BTL_SORA_STATE_RAID;
            work->steps = 0;
            work->stateTimer = 0;
            work->variant[0] = BTL_RAID_JUDGMENT;
            break;
        case MOVE_TORNADO:
            work->state = BTL_SORA_STATE_TORNADO;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        case MOVE_BIND:
            work->state = BTL_SORA_STATE_BIND;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        case MOVE_CONFUSE:
            work->state = BTL_SORA_STATE_CONFUSE;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        case MOVE_SLEIGHT_57:
        case MOVE_SLEIGHT_68:
        default:
            SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
            break;
        }

        lockon = act->btl->actor2;

        if (lockon != NULL) {
            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                if (act->x < lockon->x) {
                    act->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
                }
            } else {
                if (act->x > lockon->x) {
                    act->flags |= BTLOBJ_FLAG_FACING_LEFT;
                }
            }
        }

        AnimReset(&work->anim);
        work->speed = 0;
        act->vx = act->vy = 0;
        break;
    case BTL_REACTION_HAZARD:
        switch (work->state) {
        case BTL_SORA_STATE_AQUA_SPLASH_SPRAY:
            m4aSongNumStop(SONG_EF_DAMBO_SPLOOP);
            break;
        case BTL_SORA_STATE_TORNADO:
            m4aSongNumStop(SONG_EF_TRUNEDO);
            break;
#ifdef VERSION_EU
        case BTL_SORA_STATE_HOLY:
            m4aSongNumStop(SONG_EF_HOLLY);
            break;
        case BTL_SORA_STATE_GIFTED_MIRACLE:
            m4aSongNumStop(SONG_EF_XMAS);
            break;
#endif
        }

        if (!(gBtlWork->flags & BTL_FLAG_OPPONENT_CARD_ACTION)) {
            gBtlWork->flags |= BTL_FLAG_STOP_BGFX;
        }

        SetBattleZoom(12, Q_8_8(1), gBtlWork->x2, gBtlWork->y2);
        ColliderSetDisabled(&act->collider, FALSE);
        DisableBtlSoraPassThrough(work);
        act->flags &= 0xFFFFDFFBFF7FFFFFLL;
        act->btl->flags |= BTL_FLAG_DISMISS_SUMMONS;
        work->speed = 0;
        work->scaleX = Q_8_8(1);
        work->scaleY = Q_8_8(1);
        work->state = BTL_SORA_STATE_HAZARD;
        work->steps = 0;
        work->stateTimer = 0;
        break;
    case BTL_REACTION_CARD_BROKEN:
        switch (work->state) {
        case BTL_SORA_STATE_SUMMON_TAKEOFF:
        case BTL_SORA_STATE_SUMMON_EXIT:
        case BTL_SORA_STATE_SUMMON_OFFSCREEN:
        case BTL_SORA_STATE_SUMMON_RETURN:
            act->x = act->originX;
            act->y = act->originY;
            act->z = act->originZ;
#ifndef VERSION_EU
            act->btl->flags &= ~BTL_FLAG_PLAYER_OFFSCREEN;
#endif
            act->flags &= ~BTLOBJ_FLAG_NO_BREAK_POP;
            CreateBtlPopTask(act, BTL_POP_CARD_BREAK);
            break;
        case BTL_SORA_STATE_AQUA_SPLASH_SPRAY:
            m4aSongNumStop(SONG_EF_DAMBO_SPLOOP);
            break;
        case BTL_SORA_STATE_TORNADO:
            m4aSongNumStop(SONG_EF_TRUNEDO);
            break;
#ifdef VERSION_EU
        case BTL_SORA_STATE_HOLY:
            m4aSongNumStop(SONG_EF_HOLLY);
            break;
        case BTL_SORA_STATE_GIFTED_MIRACLE:
            m4aSongNumStop(SONG_EF_XMAS);
            break;
#endif
        }

#ifdef VERSION_EU
        act->btl->flags &= ~BTL_FLAG_PLAYER_OFFSCREEN;
#endif
        work->scaleX = work->scaleY = Q_8_8(1);
        ColliderSetDisabled(&act->collider, FALSE);
        DisableBtlSoraPassThrough(work);
        act->flags &= 0xFFFFDFFBFF7FFFFFLL;
        act->btl->flags |= BTL_FLAG_DISMISS_SUMMONS;
        work->speed = 0;
        work->state = BTL_SORA_STATE_CARD_BROKEN;
        work->steps = 0;
        work->stateTimer = 0;
        break;
    case BTL_REACTION_WARPED:
        FadeStartIn(FADE_MODE_ADD_WHITE, 20);
        gBtlWork->hitStop = 15;

        if (act->badStatus != BAD_STATUS_STUN) {
            act->badStatus = BAD_STATUS_STUN;
            act->badStatusTimer = 360;
        }

        work->speed = 0;
        work->state = BTL_SORA_STATE_STUNNED;
        work->steps = 0;
        work->stateTimer = 0;
        break;
    case BTL_REACTION_STUNNED:
        StartBtlSoraKnockback(work);
        work->speed = 0;
        work->state = BTL_SORA_STATE_STUNNED;
        work->steps = 0;
        work->stateTimer = 0;
        break;
    case BTL_REACTION_STOPPED:
        if (work->state != BTL_SORA_STATE_STOPPED) {
            work->flags |= BTL_SORA_FLAG_PASS_THROUGH;
            work->speed = 0;
            act->vx = act->vy = 0;
            work->state = BTL_SORA_STATE_STOPPED;
            work->steps = 0;
            work->stateTimer = 0;
        }

        break;
    }

    if (act->btl->flags & BTL_FLAG_FIELD_HIDDEN) {
        work->state = BTL_SORA_STATE_FIELD_HIDDEN;
        work->steps = 0;
        work->stateTimer = 0;
    } else if (act->flags & BTLOBJ_FLAG_FREEZE_PENDING) {
        act->flags &= ~BTLOBJ_FLAG_FREEZE_PENDING;
        work->state = BTL_SORA_STATE_FROZEN;
        work->steps = 0;
        work->stateTimer = 0;
    }

    switch (work->state) {
    case BTL_SORA_STATE_ENTER:
        if ((s16)work->stateTimer == 0) {
            if (work->steps == 0) {
                SetBtlSoraAnimation(work, 0, 0);
            }

            if (work->steps <= 29) {
                AnimReset(&work->anim);
            }

            if (AnimIsFinished(&work->anim)) {
                work->stateTimer = 1;
            } else {
                work->steps++;
            }

            break;
        }

        SetBtlSoraAnimation(work, 1, 1);

        if (gBtlWork->phase == BTL_PHASE_START) {
            break;
        }

        act->flags &= ~BTLOBJ_FLAG_INTANGIBLE;
        act->flags &= ~BTLOBJ_FLAG_CARD_USE_BLOCKED;
        work->state = BTL_SORA_STATE_IDLE;
        work->steps = 0;
        work->stateTimer = 0;
        break;
    case BTL_SORA_STATE_SUMMON_IDLE:
        if (!(act->btl->flags & BTL_FLAG_SUMMON_ACTIVE)) {
            SetBtlSoraStateNoReset(work, BTL_SORA_STATE_IDLE);
        }
    case BTL_SORA_STATE_IDLE: {
        s32 state;

        FocusBtlSoraCameraOnTarget(work);
        DisableBtlSoraPassThrough(work);

        if (act->z < act->groundZ) {
            if (work->state == BTL_SORA_STATE_SUMMON_IDLE) {
                work->state = BTL_SORA_STATE_SUMMON_AIRBORNE;
                work->steps = 0;
                work->stateTimer = 0;
            } else {
                work->state = BTL_SORA_STATE_AIRBORNE;
                work->steps = 0;
                work->stateTimer = 0;
            }

            break;
        }

        state = work->state;

        if (state != BTL_SORA_STATE_SUMMON_IDLE) {
            act->btl->flags |= BTL_FLAG_CAN_CHARGE_RELOAD;

            if ((act->btl->flags & BTL_FLAG_RELOAD_CHARGING) && act->btl->hcEffect != HC_EFFECT_RELOAD_KINESIS) {
                SetBtlSoraAnimation(work, 51, 0);

                if (AnimIsFinished(&work->anim)) {
                    AnimSetFrame(&work->anim, 3);
                }

                work->speed = 0;
                break;
            }
        }

        if ((u16)(pressed & DPAD_LEFT) != 0) {
            if (work->tapTimers[0] != 0) {
                act->flags |= BTLOBJ_FLAG_FACING_LEFT;
                act->originX = act->x;

                if (state == BTL_SORA_STATE_SUMMON_IDLE) {
                    work->state = BTL_SORA_STATE_SUMMON_DODGE;
                    work->steps = 0;
                    work->stateTimer = 0;
                } else {
                    work->state = BTL_SORA_STATE_DODGE;
                    work->steps = 0;
                    work->stateTimer = 0;
                }

                break;
            }
        } else if (pressed & DPAD_RIGHT) {
            if (work->tapTimers[1] != 0) {
                act->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
                act->originX = act->x;

                if (state == BTL_SORA_STATE_SUMMON_IDLE) {
                    work->state = BTL_SORA_STATE_SUMMON_DODGE;
                    work->steps = 0;
                    work->stateTimer = 0;
                } else {
                    work->state = BTL_SORA_STATE_DODGE;
                    work->steps = 0;
                    work->stateTimer = 0;
                }

                break;
            }
        }

        UpdateBtlSoraWalk(work, held);

        if (!(pressed & B_BUTTON)) {
            break;
        }

        m4aSongNumStart(work->groundSongs[2]);

        if (work->state == BTL_SORA_STATE_SUMMON_IDLE) {
            work->state = BTL_SORA_STATE_SUMMON_JUMP;
            work->steps = 0;
            work->stateTimer = 0;
        } else {
            work->state = BTL_SORA_STATE_JUMP;
            work->steps = 0;
            work->stateTimer = 0;
        }
    }

        break;
    case BTL_SORA_STATE_SUMMON_JUMP:
        if (!(act->btl->flags & BTL_FLAG_SUMMON_ACTIVE)) {
            SetBtlSoraStateNoReset(work, BTL_SORA_STATE_JUMP);
        }
    case BTL_SORA_STATE_JUMP: {
        s32 elapsed;

        FocusBtlSoraCameraOnTarget(work);

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraDirAnimation(work, 1, 1);

            if (!(held & DPAD_ANY)) {
                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    work->angle = 192;
                } else {
                    work->angle = 64;
                }
            }

            work->speed >>= 1;
        }

        elapsed = work->stateTimer;

        if ((s16)elapsed <= 3) {
            work->stateTimer = elapsed + 1;
            break;
        }

        if (work->state == BTL_SORA_STATE_SUMMON_JUMP) {
            work->state = BTL_SORA_STATE_SUMMON_AIRBORNE;
            work->steps = 0;
            work->stateTimer = 0;
        } else {
            work->state = BTL_SORA_STATE_AIRBORNE;
            work->steps = 0;
            work->stateTimer = 0;
        }

        work->vz = -1344;
        act->btl->flags |= BTL_FLAG_PLAYER_AIRBORNE;
        work->speed <<= 1;
    }

        break;
    case BTL_SORA_STATE_SUMMON_AIRBORNE:
        if (!(act->btl->flags & BTL_FLAG_SUMMON_ACTIVE)) {
            SetBtlSoraStateNoReset(work, BTL_SORA_STATE_AIRBORNE);
        }
    case BTL_SORA_STATE_AIRBORNE:
        FocusBtlSoraCameraOnTarget(work);
        act->btl->flags |= BTL_FLAG_PLAYER_AIRBORNE;

        if (work->vz < 0) {
            if (work->vz <= -512) {
                SetBtlSoraDirAnimation(work, 2, 1);
            } else {
                SetBtlSoraDirAnimation(work, 3, 1);
            }
        } else if (work->vz <= 511) {
            SetBtlSoraDirAnimation(work, 3, 1);
        } else {
            SetBtlSoraDirAnimation(work, 4, 1);
        }

        if (work->vz < 0 && !(held & B_BUTTON)) {
            work->vz += 64;
        }

        if (((s16)held & (DPAD_RIGHT | DPAD_UP)) == (DPAD_RIGHT | DPAD_UP)) {
            if (work->angle != 32) {
                work->angle = 32;
                work->speed = 0;
            }

            act->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
        } else if (((s16)held & (DPAD_RIGHT | DPAD_DOWN)) == (DPAD_RIGHT | DPAD_DOWN)) {
            if (work->angle != 96) {
                work->angle = 96;
                work->speed = 0;
            }

            act->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
        } else if (((s16)held & (DPAD_LEFT | DPAD_DOWN)) == (DPAD_LEFT | DPAD_DOWN)) {
            if (work->angle != 160) {
                work->angle = 160;
                work->speed = 0;
            }

            act->flags |= BTLOBJ_FLAG_FACING_LEFT;
        } else if (((s16)held & (DPAD_LEFT | DPAD_UP)) == (DPAD_LEFT | DPAD_UP)) {
            if (work->angle != 224) {
                work->angle = 224;
                work->speed = 0;
            }

            act->flags |= BTLOBJ_FLAG_FACING_LEFT;
        } else if (held & DPAD_UP) {
            if (work->angle != 0) {
                work->angle = 0;
                work->speed = 0;
            }
        } else if (held & DPAD_RIGHT) {
            if (work->angle != 64) {
                work->angle = 64;
                work->speed = 0;
            }

            act->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
        } else if (held & DPAD_DOWN) {
            if (work->angle != 128) {
                work->angle = 128;
                work->speed = 0;
            }
        } else if (held & DPAD_LEFT) {
            if (work->angle != 192) {
                work->angle = 192;
                work->speed = 0;
            }

            act->flags |= BTLOBJ_FLAG_FACING_LEFT;
        }

        if (held & DPAD_ANY) {
            work->speed += 17;

            if (work->speed > 614) {
                work->speed = 614;
            }
        } else {
            work->speed -= 38;

            if (work->speed < 0) {
                work->speed = 0;
            }
        }

        work->stateTimer++;
        break;
    case BTL_SORA_STATE_SUMMON_LAND:
        if (!(act->btl->flags & BTL_FLAG_SUMMON_ACTIVE)) {
            SetBtlSoraStateNoReset(work, BTL_SORA_STATE_LAND);
        }
    case BTL_SORA_STATE_LAND: {
        s32 elapsed;

        FocusBtlSoraCameraOnTarget(work);
        timer = work->stateTimer;

        if (timer == 0) {
            m4aSongNumStart(work->groundSongs[3]);
            work->speed = 0;
            SetBtlSoraDirAnimation(work, 5, 0);
            gBtlWork->flags |= BTL_FLAG_JUMP_LANDED;
        } else if (pressed & B_BUTTON) {
            m4aSongNumStart(work->groundSongs[2]);
            act->btl->flags |= BTL_FLAG_PLAYER_AIRBORNE;

            if (work->state == BTL_SORA_STATE_SUMMON_LAND) {
                work->state = BTL_SORA_STATE_SUMMON_JUMP;
                work->steps = 0;
                work->stateTimer = 0;
            } else {
                work->state = BTL_SORA_STATE_JUMP;
                work->steps = 0;
                work->stateTimer = 0;
            }
        } else if ((u16)(pressed & DPAD_LEFT) != 0) {
            if (work->tapTimers[0] != 0) {
                act->flags |= BTLOBJ_FLAG_FACING_LEFT;
                act->originX = act->x;

                if (work->state == BTL_SORA_STATE_SUMMON_LAND) {
                    work->state = BTL_SORA_STATE_SUMMON_DODGE;
                    work->steps = 0;
                    work->stateTimer = 0;
                } else {
                    work->state = BTL_SORA_STATE_DODGE;
                    work->steps = 0;
                    work->stateTimer = 0;
                }

                break;
            }
        } else if (pressed & DPAD_RIGHT) {
            if (work->tapTimers[1] != 0) {
                act->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
                act->originX = act->x;

                if (work->state == BTL_SORA_STATE_SUMMON_LAND) {
                    work->state = BTL_SORA_STATE_SUMMON_DODGE;
                    work->steps = 0;
                    work->stateTimer = 0;
                } else {
                    work->state = BTL_SORA_STATE_DODGE;
                    work->steps = 0;
                    work->stateTimer = 0;
                }

                break;
            }
        }

        elapsed = work->stateTimer;

        if ((s16)elapsed <= 6) {
            work->stateTimer = elapsed + 1;
            break;
        }

        if (work->state == BTL_SORA_STATE_SUMMON_LAND) {
            work->state = BTL_SORA_STATE_SUMMON_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
        } else {
            work->state = BTL_SORA_STATE_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
        }

        break;
    }
    case BTL_SORA_STATE_ESCAPE:
        SetBtlSoraDirAnimation(work, 0, 1);
        work->speed = 0;

        if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
            work->angle = 192;
            act->x -= 614;
        } else {
            work->angle = 64;
            act->x += 614;
        }

        break;
    case BTL_SORA_STATE_CAST_POSE:
        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 44, 0);
        }

        if (AnimIsFinished(&work->anim)) {
            SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_SORA_STATE_CARD_BROKEN:
        if (work->flags & BTL_SORA_FLAG_HIDDEN) {
            break;
        }

        FocusBtlSoraCameraOnTarget(work);

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, work->breakAnim + 47, 0);
        }

        FocusBtlSoraCameraOnTarget(work);

        if (AnimIsFinished(&work->anim)) {
            ClearBtlObjActionFlags(act);
            work->state = BTL_SORA_STATE_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_SORA_STATE_SUMMON_DODGE:
        if (!(act->btl->flags & BTL_FLAG_SUMMON_ACTIVE)) {
            SetBtlSoraStateNoReset(work, BTL_SORA_STATE_DODGE);
        }
    case BTL_SORA_STATE_DODGE:
        FocusBtlSoraCameraOnTarget(work);

        if ((s16)work->stateTimer == 0) {
            work->speed = 0;
            work->unk_194 = 1664;
            SetBtlSoraAnimation(work, 53, 0);
            work->steps = 32;

            if (work->state != BTL_SORA_STATE_SUMMON_DODGE) {
                act->flags |= BTLOBJ_FLAG_CARD_USE_BLOCKED;
            }
        }

        if ((s16)work->stateTimer == 4) {
            EnableBtlSoraPassThrough(work);

            if (work->state != BTL_SORA_STATE_SUMMON_DODGE) {
                act->flags |= BTLOBJ_FLAG_HIT_LOCKED;
            }

            work->vz = -460;
            m4aSongNumStart(SONG_VO_SR_ATTACK09);
        } else if ((s16)work->stateTimer > 4 && work->steps != 0) {
            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                act->x -= work->unk_194;
            } else {
                act->x += work->unk_194;
            }

            ApproachValue(&work->unk_194, 0, work->steps);
            work->steps--;

            if (act->z < act->groundZ) {
                if (held & DPAD_UP) {
                    act->y -= 384;
                } else if (held & DPAD_DOWN) {
                    act->y += 384;
                }
            }

            if (work->anim.timer == 0 && AnimGetFrame(&work->anim) == 5) {
                m4aSongNumStart(work->groundSongs[3]);
            }

#ifndef VERSION_EU
            if (work->steps == 8) {
            } else
#endif
            if (work->steps == 0) {
                DisableBtlSoraPassThrough(work);

                if (work->state != BTL_SORA_STATE_SUMMON_DODGE) {
                    act->flags &= ~BTLOBJ_FLAG_HIT_LOCKED;
                }
            }
        }

        if (AnimIsFinished(&work->anim)) {
            if (work->state == BTL_SORA_STATE_SUMMON_DODGE) {
                work->state = BTL_SORA_STATE_SUMMON_IDLE;
                work->steps = 0;
                work->stateTimer = 0;
            } else {
                work->state = BTL_SORA_STATE_IDLE;
                work->steps = 0;
                work->stateTimer = 0;
                act->flags &= ~BTLOBJ_FLAG_CARD_USE_BLOCKED;
            }

            gBtlWork->flags |= BTL_FLAG_DODGE_ROLL_DONE;
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_SORA_STATE_GUARDED:
        FocusBtlSoraCameraOnTarget(work);

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, work->breakAnim + 47, 0);
        }

        if (AnimIsFinished(&work->anim)) {
            ClearBtlObjActionFlags(act);
            work->state = BTL_SORA_STATE_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_SORA_STATE_CURE:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        FocusBtlSoraCameraOnTarget(work);

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 44, 0);
            m4aSongNumStart(SONG_VO_SR_CAREL00);
        } else if ((s16)work->stateTimer == 27) {
            switch (work->variant[0]) {
            case SPELL_TIER_BASE:
                BgFxStartCure(SPELL_TIER_BASE, act->x, act->y, act->z - 11264);
                break;
            case SPELL_TIER_RA:
                BgFxStartCure(SPELL_TIER_RA, act->x, act->y, act->z - 11264);
                break;
            case SPELL_TIER_GA:
                BgFxStartCure(SPELL_TIER_GA, act->x, act->y, act->z - 11264);
                break;
            }
        }

        if (AnimIsFinished(&work->anim)) {
            SetBtlSoraAnimation(work, 1, 1);
        }

        if ((s16)work->stateTimer > 27 && !BgFxIsActive()) {
            switch (act->btl->hcEffect) {
            case HC_EFFECT_CURE_BOOST:
                switch (work->variant[0]) {
                case SPELL_TIER_BASE:
                    act->hp += 75;
                    break;
                case SPELL_TIER_RA:
                    act->hp += 225;
                    break;
                case SPELL_TIER_GA:
                    act->hp += 450;
                    break;
                }

                break;
            case HC_EFFECT_MAGIC_BOOST:
                switch (work->variant[0]) {
                case SPELL_TIER_BASE:
                    act->hp += 65;
                    break;
                case SPELL_TIER_RA:
                    act->hp += 195;
                    break;
                case SPELL_TIER_GA:
                    act->hp += 390;
                    break;
                }

                break;
            default:
                switch (work->variant[0]) {
                case SPELL_TIER_BASE:
                    act->hp += 50;
                    break;
                case SPELL_TIER_RA:
                    act->hp += 150;
                    break;
                case SPELL_TIER_GA:
                    act->hp += 300;
                    break;
                }

                break;
            }

            if (act->hp > act->maxHp) {
                act->hp = act->maxHp;
            }

            CreateBtlPopTask(act, BTL_POP_RECOVER);
            SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
            break;
        }

        work->stateTimer++;
        break;
    case BTL_SORA_STATE_STOP: {
        s32 x;
        s32 y;
        s32 z;

        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        FocusBtlSoraCameraOnTarget(work);

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 44, 0);
            m4aSongNumStart(SONG_VO_SR_STOP00);
            FadeStartOut(FADE_MODE_GRAY, 8);
        } else if ((s16)work->stateTimer == 25) {
            if (act->btl->actor2 != NULL) {
                enemy = act->btl->actor2;
                x = enemy->x;
                y = enemy->y;
                z = enemy->z - (enemy->centerHeight << 8);
            } else {
                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    x = act->x - 12800;
                } else {
                    x = act->x + 12800;
                }

                y = act->y;
                z = act->z - 4096;
            }

            switch (work->variant[0]) {
            case SPELL_TIER_BASE:
                BgFxStartStop(SPELL_TIER_BASE, x, y, z, 78);
                break;
            case SPELL_TIER_RA:
                BgFxStartStop(SPELL_TIER_RA, x, y, z, 79);
                break;
            case SPELL_TIER_GA:
            default:
                BgFxStartStop(work->variant[0], x, y, z, 80);
                break;
            }
        }

        if (AnimIsFinished(&work->anim)) {
            SetBtlSoraAnimation(work, 1, 1);
        }

        if ((s16)work->stateTimer > 25 && !BgFxIsActive()) {
            FadeStartIn(FADE_MODE_GRAY, 8);
            SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
        } else {
            work->stateTimer++;
        }
    }

        break;
    case BTL_SORA_STATE_ITEM:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        FocusBtlSoraCameraOnTarget(work);

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 52, 0);
        }

        if ((s16)work->stateTimer == 23) {
            BgFxStartPotion(act->x, act->y, act->z);
        }

        if ((s16)work->stateTimer > 23 && !BgFxIsActive()) {
            switch (work->variant[0]) {
            case BTL_ITEM_POTION:
                if (work->mainSide) {
                    RequestSoraPotion();
                } else {
                    RequestRikuPotion();
                }

                break;
            case BTL_ITEM_HI_POTION:
                if (work->mainSide) {
                    RequestSoraHiPotion();
                } else {
                    RequestRikuHiPotion();
                }

                break;
            case BTL_ITEM_MEGA_POTION:
                if (work->mainSide) {
                    RequestSoraMegaPotion();
                } else {
                    RequestRikuMegaPotion();
                }

                break;
            case BTL_ITEM_ETHER:
                if (work->mainSide) {
                    RequestSoraEther();
                } else {
                    RequestRikuEther();
                }

                break;
            case BTL_ITEM_MEGA_ETHER:
                if (work->mainSide) {
                    RequestSoraMegaEther();
                } else {
                    RequestRikuMegaEther();
                }

                break;
            case BTL_ITEM_ELIXIR:
                if (work->mainSide) {
                    RequestSoraElixir();
                } else {
                    RequestRikuElixir();
                }

                break;
            default:
                if (work->mainSide) {
                    RequestSoraMegalixir();
                } else {
                    RequestRikuMegalixir();
                }

                break;
            }

            SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
            break;
        }

        work->stateTimer++;
        break;
    case BTL_SORA_STATE_COMBO: {
        s32 attack;
        s32 dy;

        hit = ATTACK_RESULT_NONE;
        FocusBtlSoraCameraOnTarget(work);
        memcpy(hitFrames, sBtlSoraSwingHitFrames, sizeof(hitFrames));

        if (act->btl->hcEffect == HC_EFFECT_COMBO_PLUS) {
            if ((work->flags & BTL_SORA_FLAG_COMBO_EXTENDED) == 0) {
                if (work->comboCount == 2) {
                    work->comboCount = 1;
                    work->flags |= BTL_SORA_FLAG_COMBO_EXTENDED;
                    swing = work->attacks[0];
                } else {
                    swing = work->attacks[work->comboCount];
                }
            } else {
                swing = work->attacks[work->comboCount];
            }
        } else if (act->btl->hcEffect == HC_EFFECT_COMBO_FINISH) {
            work->comboCount = 2;
            swing = work->attacks[2];
        } else {
            swing = work->attacks[work->comboCount];
        }

        if (work->comboCount != 0) {
            if (swing->flags & COMBO_FLAG_AERIAL_SWING) {
                if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) == 0) {
                    swing = swing->next;
                }
            }
        }

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, swing->animId + work->swingSpeed, 0);
            m4aSongNumStart(swing->swingSound);
            work->vz = swing->vz;

            switch (act->btl->hcEffect) {
            case HC_EFFECT_BERSERK:
            case HC_EFFECT_WIDE_ATTACK_2:
            case HC_EFFECT_OVERDRIVE:
            case HC_EFFECT_ATTACK_HASTE:
            case HC_EFFECT_WARP_BREAK:
                act->btl->hcEffectCount--;
                break;
            }
        } else if ((s16)work->stateTimer == hitFrames[work->swingSpeed]) {
            MakeOpponentsHittable();

            if (act->btl->hcEffect == HC_EFFECT_WIDE_ATTACK_2) {
                switch (swing->animId) {
                case 22:
                    if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                        hit = ApplyAttackBox(swing->attackIds[work->keyblade], act->x - 5120, act->y,
                                          act->z - 7168, 40, 16, 44);
                    } else {
                        hit = ApplyAttackBox(swing->attackIds[work->keyblade], act->x + 5120, act->y,
                                          act->z - 7168, 40, 16, 44);
                    }

                    break;
                case 12:
                    if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                        hit = ApplyAttackBox(swing->attackIds[work->keyblade], act->x - 8192, act->y,
                                          act->z, 28, 20, 32);
                    } else {
                        hit = ApplyAttackBox(swing->attackIds[work->keyblade], act->x + 8192, act->y,
                                          act->z, 28, 20, 32);
                    }

                    break;
                default:
                    if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                        hit = ApplyAttackBox(swing->attackIds[work->keyblade], act->x - 9216, act->y,
                                          act->z, 32, 12, 32);
                    } else {
                        hit = ApplyAttackBox(swing->attackIds[work->keyblade], act->x + 9216, act->y,
                                          act->z, 32, 12, 32);
                    }

                    break;
                }
            } else {
                if (act->btl->hcEffect == HC_EFFECT_WARP_BREAK && work->comboCount == 2) {
                    if (GetRandom() % 3 != 0) {
                        attack = 164;
                    } else {
                        CreateBtlPopTask(act, BTL_POP_MISS);
                        attack = swing->attackIds[work->keyblade];
                    }
                } else {
                    attack = swing->attackIds[work->keyblade];
                }

                switch (swing->animId) {
                case 22:
                    if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                        hit = ApplyAttackBox(attack, act->x - 5120, act->y, act->z - 7168, 28, 16, 44);
                    } else {
                        hit = ApplyAttackBox(attack, act->x + 5120, act->y, act->z - 7168, 28, 16, 44);
                    }

                    break;
                case 12:
                    if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                        hit = ApplyAttackBox(attack, act->x - 8192, act->y, act->z, 16, 20, 32);
                    } else {
                        hit = ApplyAttackBox(attack, act->x + 8192, act->y, act->z, 16, 20, 32);
                    }

                    break;
                default:
                    if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                        hit = ApplyAttackBox(attack, act->x - 9216, act->y, act->z, 20, 12, 32);
                    } else {
                        hit = ApplyAttackBox(attack, act->x + 9216, act->y, act->z, 20, 12, 32);
                    }

                    break;
                }
            }

            if (hit == ATTACK_RESULT_HIT) {
                m4aSongNumStart(swing->hitSound);

                if (swing->flags & COMBO_FLAG_ZOOM_ON_HIT) {
                    if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                        SetBattleZoom(8, Q_8_8(1.5), act->x - 5120, (act->y - 5120) + act->z);
                    } else {
                        SetBattleZoom(8, Q_8_8(1.5), act->x + 5120, (act->y - 5120) + act->z);
                    }
                }

                work->flags |= BTL_SORA_FLAG_SWING_HIT;
            } else {
                work->flags &= ~BTL_SORA_FLAG_SWING_HIT;
            }
        } else if ((s16)work->stateTimer == hitFrames[work->swingSpeed] + 2) {
            if (swing->flags & COMBO_FLAG_ZOOM_ON_HIT) {
                SetBattleZoom(15, Q_8_8(1), gBtlWork->x2, gBtlWork->y2);
            }

            if (work->comboCount <= 1) {
                if (work->flags & BTL_SORA_FLAG_SWING_HIT) {
                    act->flags &= ~BTLOBJ_FLAG_IN_CARD_ACTION;
                }
            }
        }

        if (hit == ATTACK_RESULT_BLOCKED) {
            SetBattleZoom(8, Q_8_8(1), gBtlWork->x2, gBtlWork->y2);
            SetBtlSoraState(work, BTL_SORA_STATE_GUARDED);
            act->flags |= BTLOBJ_FLAG_CARD_USE_BLOCKED;
            break;
        }

        if (AnimIsFinished(&work->anim) && (act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) == 0) {
            SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
            break;
        }

        if (AnimGetFrame(&work->anim) <= 2) {
            enemy = act->btl->actor2;

            if (enemy != NULL) {
                if (AnimGetFrame(&work->anim) > 1) {
                    s32 dx;

                    dx = enemy->x - act->x;
                    dy = enemy->y - act->y;

                    if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                        s32 target;
                        s32 origin;

                        if (dx > 0) {
                            dx = 0;
                        }

                        target = act->originX + dx;
                        origin = act->x - 4096;
                        act->x += (target - origin) >> 3;
                    } else {
                        s32 target;
                        s32 origin;

                        if (dx < 0) {
                            dx = 0;
                        }

                        target = act->originX + dx;
                        origin = act->x + 4096;
                        act->x += (target - origin) >> 3;
                    }

                    act->y += ((act->originY + dy) - act->y) >> 4;
                }

                if (swing->flags & COMBO_FLAG_AERIAL_SWING) {
                    s32 climb = (enemy->z - (enemy->centerHeight << 8)) - act->z;

                    if (climb < 0) {
                        act->btl->flags |= BTL_FLAG_PLAYER_AIRBORNE;
                        act->z += ((act->originZ + climb) - act->z) >> 3;
                        work->vz = 0;
                    }
                }
            } else {
                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    s32 targetX = act->originX - 10240;
                    act->x += (targetX - act->x) >> 3;
                } else {
                    s32 targetX = act->originX + 10240;
                    act->x += (targetX - act->x) >> 3;
                }
            }
        }

        work->stateTimer++;
        break;
    }
    case BTL_SORA_STATE_GIFTED_MIRACLE: {
        BtlObj* obj;

        FocusBtlSoraCamera(work);

        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 51, 0);
            FadeStartOut(FADE_MODE_BLACK, 30);
        }

        if ((s16)work->stateTimer == 30) {
            BgFxStartXmas(60);
            m4aSongNumStart(SONG_EF_XMAS);
        } else if ((s16)work->stateTimer > 30 && !BgFxIsActive()) {
            obj = gBtlWork->actor;
            CreateBtlPopTask(obj, BTL_POP_RECOVER);
            obj->hp += 50;

            if (obj->hp > obj->maxHp) {
                obj->hp = obj->maxHp;
            }

            if (gBtlWork->flags & BTL_FLAG_VS_BATTLE) {
                obj = gRikuBtlWork->actor;

                if (obj->badStatus != BAD_STATUS_STOP) {
                    CreateBtlPopTask(obj, BTL_POP_RECOVER);
                    obj->hp += 50;

                    if (obj->hp > obj->maxHp) {
                        obj->hp = obj->maxHp;
                    }
                }
            } else {
                obj = ListPoolFirst(&gBtlWork->pool);

                while (obj != NULL) {
                    if (obj->badStatus != BAD_STATUS_STOP && obj->parent == NULL) {
                        CreateBtlPopTask(obj, BTL_POP_RECOVER);
                        obj->hp += 50;

                        if (obj->hp > obj->maxHp) {
                            obj->hp = obj->maxHp;
                        }
                    }

                    obj = ListPoolNext(&obj->node);
                }
            }

            if (work->mainSide) {
                RequestSoraMegalixir();
            } else {
                RequestRikuMegalixir();
            }

            SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
            FadeStartIn(FADE_MODE_BLACK, 16);
            break;
        }

        work->stateTimer++;
        break;
    }
    case BTL_SORA_STATE_WARPINATOR: {
        u16 elapsed;

        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        FocusBtlSoraCameraOnTarget(work);

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 43, 0);
            m4aSongNumStart(SONG_VO_SR_SUMMON06);
        }

        if ((s16)work->stateTimer == 40) {
            if ((GetRandom() & 1) || (gBtlWork->flags & (BTL_FLAG_BOSS_BATTLE | BTL_FLAG_HUM_BATTLE | BTL_FLAG_VS_BATTLE))) {
                CreateBtlPopTask(act, BTL_POP_MISS);
            } else {
                s32 minDist;
                BtlObj* nearest;

                minDist = 0x40000;
                nearest = NULL;
                enemy = ListPoolFirst(&gBtlWork->pool);

                while (enemy != NULL) {
                    s32 dx;
                    s32 dy;
                    s32 dist;

                    dx = (act->x - enemy->x) >> 8;
                    dy = (act->y - enemy->y) >> 8;
                    dist = Sqrt8(dx * dx + dy * dy);

                    if (dist < minDist) {
                        minDist = dist;
                        nearest = enemy;
                    }

                    enemy = ListPoolNext(&enemy->node);
                }

                if (nearest != NULL) {
                    FadeFromAmount(FADE_MODE_BLUE, 15, 32);

                    if (nearest->flags & BTLOBJ_FLAG_IMMUNE_WARP) {
                        CreateBtlPopTask(nearest, BTL_POP_GUARD);
                    } else {
                        nearest->flags |= BTLOBJ_FLAG_WARP_PENDING;
                        nearest->hitFlags = 0;
                        m4aSongNumStart(SONG_EF_TELEP);
                    }
                } else {
                    CreateBtlPopTask(act, BTL_POP_MISS);
                }
            }
        }

        elapsed = work->stateTimer;

        if ((s16)elapsed > 41) {
            SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
        } else {
            work->stateTimer = elapsed + 1;
        }

        break;
    }
    case BTL_SORA_STATE_TERROR:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        FocusBtlSoraCamera(work);

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 43, 0);
            m4aSongNumStart(SONG_VO_SR_ATTACK08);
        }

        if ((s16)work->stateTimer == 40) {
            FadeStartIn(FADE_MODE_WHITE_BLEND, 8);
            FadeLock();
            m4aSongNumStart(SONG_EF_TELER);
            gBtlWork->hitStop = 8;
        } else if ((s16)work->stateTimer == 41) {
            ApplyAttackBox(110, act->x, act->y, act->z, 256, 256, 256);
        }

        elapsed = work->stateTimer;

        if ((s16)elapsed > 70) {
            SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
        } else {
            work->stateTimer = elapsed + 1;
        }

        break;
    case BTL_SORA_STATE_AERO:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        FocusBtlSoraCamera(work);

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 45, 0);
            m4aSongNumStart(SONG_VO_SR_AIRO00);
        }

        if ((s16)work->stateTimer == 27) {
            switch (work->variant[0]) {
            case SPELL_TIER_BASE:
                BgFxStartAero(SPELL_TIER_BASE, act->x, act->y, act->z, 81);
                break;
            case SPELL_TIER_RA:
                BgFxStartAero(SPELL_TIER_RA, act->x, act->y, act->z, 82);
                break;
            case SPELL_TIER_GA:
                BgFxStartAero(SPELL_TIER_GA, act->x, act->y, act->z, 83);
                break;
            }

            m4aSongNumStart(SONG_EF_AIRO_HIT);
        } else if ((s16)work->stateTimer > 27 && !BgFxIsActive()) {
            SetBtlSoraAnimation(work, 46, 0);
            SetBtlSoraState(work, BTL_SORA_STATE_RECOVER);
            break;
        }

        work->stateTimer++;
        break;
    case BTL_SORA_STATE_HOLY:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        FocusBtlSoraCamera(work);

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 45, 0);
            m4aSongNumStart(SONG_VO_SR_SUMMON00);
        }

        if ((s16)work->stateTimer == 27) {
            BtlObj* enemy;
            s32 x;
            s32 y;
            s32 z;

            enemy = act->btl->actor2;

            if (enemy != NULL) {
                x = enemy->x;
                y = enemy->y;
                z = enemy->groundZ;
            } else {
                x = act->flags & BTLOBJ_FLAG_FACING_LEFT ? act->x - 6144 : act->x + 6144;
                y = act->y;
                z = act->groundZ;
            }

            BgFxStartHoly(x, y, z, 112);

            m4aSongNumStart(SONG_EF_HOLLY);
        } else if ((s16)work->stateTimer > 27 && !BgFxIsActive()) {
            SetBtlSoraAnimation(work, 46, 0);
            SetBtlSoraState(work, BTL_SORA_STATE_RECOVER);
            break;
        }

        work->stateTimer++;
        break;
    case BTL_SORA_STATE_TORNADO:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 45, 0);
            FocusBtlSoraCamera(work);
            m4aSongNumStart(SONG_VO_SR_SUMMON06);
        }

        if ((s16)work->stateTimer == 27) {
            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                BgFxStartTornado(act->x, act->y, 0, 113, TRUE);
            } else {
                BgFxStartTornado(act->x, act->y, 0, 113, FALSE);
            }

            m4aSongNumStart(SONG_EF_TRUNEDO);
        } else if ((s16)work->stateTimer > 27) {
            FocusBtlSoraCameraOnBgFx(work);

            if (held & DPAD_LEFT) {
                BgFxAddPosition(-256, 0, 0);
            } else if (held & DPAD_RIGHT) {
                BgFxAddPosition(256, 0, 0);
            }

            if (held & DPAD_UP) {
                BgFxAddPosition(0, -128, 0);
            } else if (held & DPAD_DOWN) {
                BgFxAddPosition(0, 128, 0);
            }

            if (!BgFxIsActive()) {
                SetBtlSoraAnimation(work, 46, 0);
                SetBtlSoraState(work, BTL_SORA_STATE_RECOVER);
                m4aSongNumStop(SONG_EF_TRUNEDO);
                break;
            }
        }

        work->stateTimer++;
        break;
    case BTL_SORA_STATE_CONFUSE:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        FocusBtlSoraCamera(work);

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 45, 0);
            m4aSongNumStart(SONG_VO_SR_ATTACK08);
        }

        if ((s16)work->stateTimer == 27) {
            work->timer = 20;
            BtlMapStartShake();
            ApplyAttackBox(114, act->x, act->y, act->z, 256, 256, 256);
            FadeFromAmount(FADE_MODE_BLUE, 16, 20);
            m4aSongNumStart(SONG_BTL_JF_BALLTHR);
        } else if ((s16)work->stateTimer > 27 && (s16)--work->timer <= 0) {
            SetBtlSoraAnimation(work, 46, 0);
            SetBtlSoraState(work, BTL_SORA_STATE_RECOVER);
            break;
        }

        work->stateTimer++;
        break;
    case BTL_SORA_STATE_BIND:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        FocusBtlSoraCamera(work);

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 45, 0);
            m4aSongNumStart(SONG_VO_SR_STOP00);
        }

        if ((s16)work->stateTimer == 27) {
            work->timer = 20;
            BgFxStartBind(act->x, 115);
            m4aSongNumStart(SONG_EF_BIND);
        } else if ((s16)work->stateTimer > 27 && !BgFxIsActive()) {
            if ((s16)--work->timer <= 0) {
                SetBtlSoraAnimation(work, 46, 0);
                SetBtlSoraState(work, BTL_SORA_STATE_RECOVER);
                break;
            }
        }

        work->stateTimer++;
        break;
    case BTL_SORA_STATE_WARP:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        FocusBtlSoraCamera(work);

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 43, 0);
        }

        if ((s16)work->stateTimer == 40) {
            m4aSongNumStart(SONG_BTL_GMIC_OK);
            FadeStartIn(FADE_MODE_CONTRAST, 8);
            FadeLock();
            gBtlWork->hitStop = 8;
        } else if ((s16)work->stateTimer == 41) {
            if (gBtlWork->flags & (BTL_FLAG_BOSS_BATTLE | BTL_FLAG_HUM_BATTLE | BTL_FLAG_VS_BATTLE)) {
                CreateBtlPopTask(act, BTL_POP_MISS);
            } else {
                ApplyAttackBox(98, act->x, act->y, act->z, 256, 256, 256);
            }
        }

        if ((s16)work->stateTimer > 41 && !FadeIsActive()) {
            SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_SORA_STATE_TELEPORT:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        FocusBtlSoraCameraOnTarget(work);

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 1, 0);
            work->steps = 16;
            FadeStartOut(FADE_MODE_GRAY, 1);
        }

        ApproachValue(&work->scaleX, 10, work->steps);
        ApproachValue(&work->scaleY, Q_8_8(2), work->steps);

        if (--work->steps > 0) {
            work->stateTimer++;
        } else {
            work->state = BTL_SORA_STATE_TELEPORT_ARRIVE;
            work->steps = 0;
            work->stateTimer = 0;
            m4aSongNumStart(SONG_EF_TELEP);
        }

        break;
    case BTL_SORA_STATE_TELEPORT_ARRIVE:
        if ((s16)work->stateTimer == 0) {
            enemy = PickBtlSoraTarget(work);
            work->steps = 16;

            if (enemy != NULL) {
                act->y = enemy->y;
                act->z = enemy->groundZ;
                act->groundZ = enemy->groundZ;

                if (enemy->x > act->x) {
                    act->x = enemy->x + 8192;
                    act->flags |= BTLOBJ_FLAG_FACING_LEFT;
                } else {
                    act->x = enemy->x - 8192;
                    act->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
                }

                ApplyAttackBox(109, enemy->x, enemy->y, enemy->z, 4, 4, 4);
            }
        }

        ApproachValue(&work->scaleX, Q_8_8(1), work->steps);
        ApproachValue(&work->scaleY, Q_8_8(1), work->steps);

        if (--work->steps <= 0) {
            FadeStartIn(FADE_MODE_GRAY, 1);
            MakeOpponentsHittable();
            SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_SORA_STATE_SYNCHRO:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        FocusBtlSoraCameraOnTarget(work);

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 45, 0);
        }

        if ((s16)work->stateTimer == 27) {
            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                BgFxStartSync(act->x + 768, act->y, act->z - 16384);
            } else {
                BgFxStartSync(act->x - 768, act->y, act->z - 16384);
            }
        } else if ((s16)work->stateTimer > 27 && !BgFxIsActive()) {
            SetBtlSoraAnimation(work, 46, 0);
            SetBtlSoraState(work, BTL_SORA_STATE_RECOVER);
            break;
        }

        work->stateTimer++;
        break;
    case BTL_SORA_STATE_THUNDER: {
        s32 targetX;
        s32 targetY;

        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        if ((s16)work->stateTimer == 0) {
            FocusBtlSoraCameraOnTarget(work);
            SetBtlSoraAnimation(work, 43, 0);
            m4aSongNumStart(SONG_VO_SR_THNDER00);
        }

        if ((s16)work->stateTimer == 27) {
            switch (work->variant[0]) {
            case SPELL_TIER_BASE:
                enemy = act->btl->actor2;

                if (enemy != NULL) {
                    targetX = enemy->x;
                    targetY = enemy->y;
                } else {
                    if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                        targetX = act->x - 20480;
                    } else {
                        targetX = act->x + 20480;
                    }

                    targetY = act->y;
                }

                hit = 0;
                BgFxStartThunder(SPELL_TIER_BASE, act->x, act->y, act->z - 16384, targetX, targetY, hit, 72);
                break;
            case SPELL_TIER_RA:
                BgFxStartWideThunder(SPELL_TIER_RA, act->x, act->y, act->z - 16384, act->groundZ, 73);
                break;
            case SPELL_TIER_GA:
            default:
                BgFxStartWideThunder(SPELL_TIER_GA, act->x, act->y, act->z - 16384, act->groundZ, 74);
                break;
            }
        } else if (work->variant[0] != SPELL_TIER_BASE && (s16)work->stateTimer == 47) {
            SetBattleZoom(15, 148, 65536, 76800);
        } else if ((s16)work->stateTimer > 27 && !BgFxIsActive()) {
            SetBattleZoom(15, Q_8_8(1), gBtlWork->x2, gBtlWork->y2);
            SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
            break;
        }

        if (AnimIsFinished(&work->anim)) {
            SetBtlSoraAnimation(work, 1, 1);
        }

        work->stateTimer++;
    }

        break;
    case BTL_SORA_STATE_GRAVITY:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        if ((s16)work->stateTimer == 0) {
            FocusBtlSoraCameraOnTarget(work);
            SetBtlSoraAnimation(work, 42, 0);
            m4aSongNumStart(SONG_VO_SR_GURABI00);
            FadeToAmount(FADE_MODE_ADD_WHITE, 13, 60);
        }

        if ((s16)work->stateTimer == 27) {
            enemy = act->btl->actor2;

            if (enemy != NULL) {
                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    BgFxStartGravity(work->variant[0], act->x - 8192, act->y, act->z - 3584,
                                  enemy->x, enemy->y, 0, TRUE, work->variant[0] + 75);
                } else {
                    BgFxStartGravity(work->variant[0], act->x + 8192, act->y, act->z - 3584,
                                  enemy->x, enemy->y, 0, FALSE, work->variant[0] + 75);
                }
            } else {
                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    BgFxStartGravity(work->variant[0], act->x - 8192, act->y, act->z - 3584,
                                  act->originX - 15360, act->originY, act->z, TRUE,
                                  work->variant[0] + 75);
                } else {
                    BgFxStartGravity(work->variant[0], act->x + 8192, act->y, act->z - 3584,
                                  act->originX + 15360, act->originY, act->z, FALSE,
                                  work->variant[0] + 75);
                }
            }
        } else if ((s16)work->stateTimer > 27 && !BgFxIsActive()) {
            SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
            FadeToOriginal(FADE_MODE_ADD_WHITE, 20);
            break;
        }

        if (AnimIsFinished(&work->anim)) {
            SetBtlSoraAnimation(work, 1, 1);
        }

        work->stateTimer++;
        break;
    case BTL_SORA_STATE_HOMING_BLIZZARA: {
        s32 targetX;
        s32 y;
        s32 z;
        s32 attack;

        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        if ((s16)work->stateTimer == 0) {
            FocusBtlSoraCameraOnTarget(work);
            SetBtlSoraAnimation(work, 42, 0);
            m4aSongNumStart(SONG_VO_SR_BURIZAD00);
            work->target = PickBtlSoraTarget(work);
        }

        if ((s16)work->stateTimer == 27) {
            attack = 70;

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                targetX = act->x - 23040;
            } else {
                targetX = act->x + 23040;
            }

            y = act->y;
            z = act->z - 4096;

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                BgFxStartBlizzard(SPELL_TIER_RA, act->x - 18432, y, z, targetX, y, z, TRUE, attack);
            } else {
                BgFxStartBlizzard(SPELL_TIER_RA, act->x + 18432, y, z, targetX, y, z, FALSE, attack);
            }
        } else if ((s16)work->stateTimer > 27) {
            enemy = work->target;

            if (enemy != NULL) {
                s32 ahead;

                ahead = FALSE;

                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    if (enemy->x < act->x - 8192) {
                        ahead = TRUE;
                    }
                } else {
                    if (enemy->x > act->x + 8192) {
                        ahead = TRUE;
                    }
                }

                if (ahead) {
                    BgFxSetTarget(enemy->x, enemy->y, enemy->z - (enemy->centerHeight << 8));
                }
            }

            FocusBtlSoraCameraOnBgFx(work);

            if (!BgFxIsActive()) {
                SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
                break;
            }
        }

        if (AnimIsFinished(&work->anim)) {
            SetBtlSoraAnimation(work, 1, 1);
        }

        work->stateTimer++;
        break;
    }
    case BTL_SORA_STATE_HOMING_FIRA: {
        s32 targetX;
        s32 y;
        s32 z;
        s32 attack;

        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        if ((s16)work->stateTimer == 0) {
            FocusBtlSoraCameraOnTarget(work);
            SetBtlSoraAnimation(work, 42, 0);
            m4aSongNumStart(SONG_VO_SR_FIRE00);
            work->target = PickBtlSoraTarget(work);
        }

        if ((s16)work->stateTimer == 27) {
            attack = 67;

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                targetX = act->x - 51200;
            } else {
                targetX = act->x + 51200;
            }

            y = act->y;
            z = act->z - 3584;

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                BgFxStartFire(SPELL_TIER_RA, act->x - 18432, y, z, targetX, y, z, TRUE, attack);
            } else {
                BgFxStartFire(SPELL_TIER_RA, act->x + 18432, y, z, targetX, y, z, FALSE, attack);
            }
        } else if ((s16)work->stateTimer > 27) {
            enemy = work->target;

            if (enemy != NULL) {
                s32 ahead;

                ahead = FALSE;

                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    if (enemy->x < act->x - 8192) {
                        ahead = TRUE;
                    }
                } else {
                    if (enemy->x > act->x + 8192) {
                        ahead = TRUE;
                    }
                }

                if (ahead) {
                    BgFxSetTarget(enemy->x, enemy->y, enemy->z - (enemy->centerHeight << 8));
                }
            }

            FocusBtlSoraCameraOnBgFx(work);

            if (!BgFxIsActive()) {
                SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
                break;
            }
        }

        if (AnimIsFinished(&work->anim)) {
            SetBtlSoraAnimation(work, 1, 1);
        }

        work->stateTimer++;
        break;
    }
    case BTL_SORA_STATE_FIRE: {
        BtlObj* enemy;

        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        if ((s16)work->stateTimer == 0) {
            FocusBtlSoraCameraOnTarget(work);
            SetBtlSoraAnimation(work, 42, 0);
            m4aSongNumStart(SONG_VO_SR_FIRE00);
        }

        if ((s16)work->stateTimer == 27) {
            switch (work->variant[0]) {
            default:
                attack = 68;
                break;
            case SPELL_TIER_BASE:
                attack = 66;
                break;
            case SPELL_TIER_RA:
                attack = 67;
                break;
            }

            enemy = act->btl->actor2;

            if (enemy != NULL) {
                targetZ = enemy->z - (enemy->centerHeight << 8);
            } else {
                targetZ = act->z - 3584;
            }

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                BgFxStartFire(work->variant[0], act->x - 18432, act->y, act->z - 3584,
                              act->originX - 51200, act->originY, targetZ, TRUE, attack);
            } else {
                BgFxStartFire(work->variant[0], act->x + 18432, act->y, act->z - 3584,
                              act->originX + 51200, act->originY, targetZ, FALSE, attack);
            }
        } else if ((s16)work->stateTimer > 27) {
            FocusBtlSoraCameraOnBgFx(work);

            if (!BgFxIsActive()) {
                SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
                break;
            }
        }

        if (AnimIsFinished(&work->anim)) {
            SetBtlSoraAnimation(work, 1, 1);
        }

        work->stateTimer++;
        break;
    }
    case BTL_SORA_STATE_MEGA_FLARE:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        if ((s16)work->stateTimer == 0) {
            FocusBtlSoraCameraOnTarget(work);
            SetBtlSoraAnimation(work, 42, 0);
            m4aSongNumStart(SONG_VO_SR_FIRE00);
        }

        if ((s16)work->stateTimer == 27) {
            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                BgFxStartFireBurst(act->x - 18432, act->y, act->z - 3584,
                              act->originX - 51200, act->originY,
                              act->z - 3584, TRUE, 96);
            } else {
                BgFxStartFireBurst(act->x + 18432, act->y, act->z - 3584,
                              act->originX + 51200, act->originY,
                              act->z - 3584, FALSE, 96);
            }
        } else if ((s16)work->stateTimer > 27) {
            FocusBtlSoraCameraOnBgFx(work);

            if (!BgFxIsActive()) {
                SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
                break;
            }
        }

        if (AnimIsFinished(&work->anim)) {
            SetBtlSoraAnimation(work, 1, 1);
        }

        work->stateTimer++;
        break;
    case BTL_SORA_STATE_BLIZZARD: {
        s32 z;
        s32 attack;

        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        if ((s16)work->stateTimer == 0) {
            FocusBtlSoraCameraOnTarget(work);
            SetBtlSoraAnimation(work, 42, 0);
            m4aSongNumStart(SONG_VO_SR_BURIZAD00);
        }

        if ((s16)work->stateTimer == 27) {
            switch (work->variant[0]) {
            case SPELL_TIER_BASE:
                attack = 69;
                break;
            case SPELL_TIER_RA:
                attack = 70;
                break;
            default:
                attack = 71;
                break;
            }

            z = act->z - 4096;

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                BgFxStartBlizzard(work->variant[0], act->x - 18432, act->y, z,
                              act->originX - 23040, act->originY,
                              z, TRUE, attack);
            } else {
                BgFxStartBlizzard(work->variant[0], act->x + 18432, act->y, z,
                              act->originX + 23040, act->originY,
                              z, FALSE, attack);
            }
        } else if ((s16)work->stateTimer > 27) {
            FocusBtlSoraCameraOnBgFx(work);

            if (!BgFxIsActive()) {
                SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
                break;
            }
        }

        if (AnimIsFinished(&work->anim)) {
            SetBtlSoraAnimation(work, 1, 1);
        }

        work->stateTimer++;
    }

        break;
    case BTL_SORA_STATE_SONIC_BLADE: {
        s32 elapsed;

        hit = ATTACK_RESULT_NONE;
        FocusBtlSoraCameraOnTarget(work);
        elapsed = work->stateTimer;

        if ((s16)elapsed == 0) {
            EnableBtlSoraPassThrough(work);
            AnimReset(&work->anim);
            SetBtlSoraAnimation(work, 37, 0);

            if (work->comboCount == 0) {
                m4aSongNumStart(SONG_VO_SR_ATTACK08);
            }

            if (work->comboCount > 5) {
                m4aSongNumStart(SONG_VO_SR_ATTACK07);

                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    SetBattleZoom(8, Q_8_8(1.5), act->x - 5120, (act->y - 5120) + act->z);
                } else {
                    SetBattleZoom(8, Q_8_8(1.5), act->x + 5120, (act->y - 5120) + act->z);
                }
            }

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                BgFxStartLimit(act->x + 3072, act->y, act->z - 4096, TRUE);
            } else {
                BgFxStartLimit(act->x - 3072, act->y, act->z - 4096, FALSE);
            }

            m4aSongNumStart(SONG_EF_LIMIMOV);
            FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
        } else if ((u16)(elapsed - 22) <= 42) {
            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                act->x -= 1280;
            } else {
                act->x += 1280;
            }

            if (work->comboCount > 5) {
                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    hit = ApplyAttackBox(85, act->x - 8192, act->y, act->z, 16, 16, 32);
                } else {
                    hit = ApplyAttackBox(85, act->x + 8192, act->y, act->z, 16, 16, 32);
                }
            } else {
                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    hit = ApplyAttackBox(84, act->x - 8192, act->y, act->z, 16, 16, 32);
                } else {
                    hit = ApplyAttackBox(84, act->x + 8192, act->y, act->z, 16, 16, 32);
                }
            }

            if (hit == ATTACK_RESULT_HIT) {
                m4aSongNumStart(SONG_BTL_LT_HIT00);
            }

            if (held & DPAD_UP) {
                act->y -= 384;
            } else if (held & DPAD_DOWN) {
                act->y += 384;
            }
        }

        if ((s16)work->stateTimer == 22) {
            if (work->comboCount > 5) {
                SetBattleZoom(15, Q_8_8(1), gBtlWork->x2, gBtlWork->y2);
            }

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                BgFxStartDashRing(act->x - 15360, act->y, act->z, FALSE);
            } else {
                BgFxStartDashRing(act->x + 15360, act->y, act->z, TRUE);
            }
        }

        if (hit == ATTACK_RESULT_BLOCKED) {
            FadeToOriginal(FADE_MODE_BLACK, 8);
            SetBtlSoraState(work, BTL_SORA_STATE_GUARDED);
            act->flags |= BTLOBJ_FLAG_CARD_USE_BLOCKED;
            DisableBtlSoraPassThrough(work);
            break;
        }

        if ((s16)work->stateTimer > 38 && (pressed & A_BUTTON) && work->comboCount <= 5) {
            act->flags ^= BTLOBJ_FLAG_FACING_LEFT;
            work->comboCount++;
            MakeOpponentsHittable();
            SelectLockonTarget();
            work->state = BTL_SORA_STATE_SONIC_BLADE;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        }

        if (AnimIsFinished(&work->anim)) {
            FadeToOriginal(FADE_MODE_BLACK, 8);
            SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
            DisableBtlSoraPassThrough(work);
            break;
        }

        work->stateTimer++;
        break;
    }
    case BTL_SORA_STATE_SUMMON: {
        s32 maxHp;

        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        FocusBtlSoraCameraOnTarget(work);

        switch ((s16)work->stateTimer) {
        case 0:
            work->flags &= ~BTL_SORA_FLAG_PASS_THROUGH;

            if (act->btl->hcEffect == HC_EFFECT_HYPER_HEALING
                && work->summonDesc != &gTaskDescSmnBambi
                && work->summonDesc != &gTaskDescSmnTink
                && work->summonDesc != &gTaskDescSmnMushu) {
                CreateBtlPopTask(act, BTL_POP_RECOVER);
                maxHp = (u16)act->maxHp;
                act->hp = ((s16)maxHp >> 2) + act->hp;

                if (act->hp > (s16)maxHp) {
                    act->hp = maxHp;
                }
            }

            work->stateTimer++;
            break;
        case 1:
            work->stateTimer++;
            break;
        case 2:
            spawn2.mainSide = work->mainSide;
            spawn2.variant = work->variant[0];
            act->originX = act->x;
            act->originY = act->y;
            act->originZ = act->z;
            TaskCreate(&gBtlWork->taskPools[0], work->summonDesc, &spawn2);
            work->state = BTL_SORA_STATE_SUMMON_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        }
    }

        break;
    case BTL_SORA_STATE_SUMMON_TAKEOFF:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        FocusBtlSoraCameraOnTarget(work);
        timer = work->stateTimer;

        if (timer == 0) {
            act->btl->flags |= BTL_FLAG_PLAYER_OFFSCREEN;
            act->btl->actor2 = NULL;
            SetBtlSoraDirAnimation(work, 1, 0);

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                work->angle = 192;
            } else {
                work->angle = 64;
            }
        }

        elapsed = work->stateTimer;

        if ((s16)elapsed <= 3) {
            work->stateTimer = elapsed + 1;
            break;
        }

        work->state = BTL_SORA_STATE_SUMMON_EXIT;
        work->steps = 0;
        work->stateTimer = 0;
        work->vz = -1024;
        break;
    case BTL_SORA_STATE_SUMMON_EXIT:
        if ((s16)work->stateTimer == 0) {
            work->steps = 20;
            ColliderSetDisabled(&act->collider, TRUE);
            act->flags |= BTLOBJ_FLAG_IGNORE_BOUNDS;
            act->flags |= BTLOBJ_FLAG_NO_BREAK_POP;
            act->originZ = act->z;
            m4aSongNumStart(SONG_VO_SR_SUMMON00);
        }

        switch ((s16)work->stateTimer) {
        case 0:
            SetBtlSoraDirAnimation(work, 2, 0);
            break;
        case 10:
            SetBtlSoraDirAnimation(work, 3, 0);
            break;
        }

        if (act->originX <= 65535) {
            ApproachValue(&act->x, -8192, work->steps);
        } else {
            ApproachValue(&act->x, 139264, work->steps);
        }

        timer = --work->steps;

        if (timer == 0) {
            work->state = BTL_SORA_STATE_SUMMON_OFFSCREEN;
            work->steps = 0;
            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_SORA_STATE_SUMMON_OFFSCREEN:
        if ((s16)work->stateTimer == 0) {
            work->flags |= BTL_SORA_FLAG_HIDDEN;
            ReleaseBtlSoraPalette(work);
            spawn.mainSide = work->mainSide;
            spawn.variant = work->variant[0];
            TaskCreate(&gBtlWork->taskPools[0], work->summonDesc, &spawn);
            act->x = act->originX;
            act->y = act->originY;
            act->z = -65536;
        }

        work->vz = 0;

        if (act->btl->flags & BTL_FLAG_SUMMON_ACTIVE) {
            work->stateTimer++;
            break;
        }

        work->flags &= ~BTL_SORA_FLAG_HIDDEN;
        LoadBtlSoraPalette(work);

        if (act->originX <= 65535) {
            act->x = -8192;
        } else {
            act->x = 139264;
        }

        act->z = act->originZ - 12800;
        act->btl->flags |= BTL_FLAG_PLAYER_AIRBORNE;
        work->vz = 0;
        SetBtlSoraAnimation(work, 1, 1);
        work->state = BTL_SORA_STATE_SUMMON_RETURN;
        work->steps = 0;
        work->stateTimer = 0;
        break;
    case BTL_SORA_STATE_SUMMON_RETURN:
        if ((s16)work->stateTimer == 0) {
            work->steps = 20;
        }

        switch ((s16)work->stateTimer) {
        case 0:
            SetBtlSoraDirAnimation(work, 3, 0);
            break;
        case 10:
            SetBtlSoraDirAnimation(work, 4, 0);
            break;
        case 18:
            ColliderSetDisabled(&act->collider, FALSE);
            act->flags &= ~BTLOBJ_FLAG_IGNORE_BOUNDS;
            act->flags &= ~BTLOBJ_FLAG_NO_BREAK_POP;
            break;
        }

        if (work->steps > 0) {
            ApproachValueHalfSteps(&act->x, act->originX, work->steps);
            work->steps--;
        }

        if (!(act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE)) {
            work->state = BTL_SORA_STATE_SUMMON_RETURN_LAND;
            work->steps = 0;
            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_SORA_STATE_SUMMON_RETURN_LAND: {
        u16 elapsed;
        s16 timer;

        timer = work->stateTimer;

        if (timer == 0) {
            ColliderSetDisabled(&act->collider, FALSE);
            act->flags &= ~BTLOBJ_FLAG_IGNORE_BOUNDS;
            act->flags &= ~BTLOBJ_FLAG_NO_BREAK_POP;
            SetBtlSoraDirAnimation(work, 5, 0);
        }

        elapsed = work->stateTimer;

        if ((s16)elapsed > 6) {
            act->btl->flags &= ~BTL_FLAG_PLAYER_OFFSCREEN;
            SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
        } else {
            work->stateTimer = elapsed + 1;
        }
    }

        break;
    case BTL_SORA_STATE_HAZARD: {
        u16 prev;

        FocusBtlSoraCameraOnTarget(work);
        timer = work->stateTimer;

        if (timer == 0) {
            FadeFromAmount(FADE_MODE_RED, 4, 10);
            AnimReset(&work->anim);
            SetBtlSoraAnimation(work, 38, 0);
            act->flags |= BTLOBJ_FLAG_HURT;
            work->speed = 0;
            prev = act->hp;

            if ((s16)prev > 1) {
                act->hp = prev - 1;
            }

            switch (GetRandom() % 3) {
            case 0:
                m4aSongNumStart(SONG_VO_SR_DAMAGE00);
                break;
            case 1:
                m4aSongNumStart(SONG_VO_SR_DAMAGE02);
                break;
            case 2:
            default:
                m4aSongNumStart(SONG_VO_SR_DAMAGE03);
                break;
            }
        }

        prev = work->stateTimer;

        if ((s16)prev > 24) {
            act->flags &= ~BTLOBJ_FLAG_HURT;
            work->state = BTL_SORA_STATE_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
        } else {
            work->stateTimer = prev + 1;
        }

        break;
    }
    case BTL_SORA_STATE_HURT:
        FocusBtlSoraCameraOnTarget(work);
        act->originX = act->x;
        act->originY = act->y;
        timer = work->stateTimer;

        if (timer == 0) {
            AnimReset(&work->anim);
            gBtlWork->hitStop = gBtlWork->pendingHitStop;
            StartBtlSoraKnockback(work);

            if (act->btl->hcEffect == HC_EFFECT_QUICK_RECOVERY) {
                SetBtlSoraAnimation(work, 39, 0);
                act->btl->hcEffectCount--;
                work->steps = 0;
            } else {
                SetBtlSoraAnimation(work, 38, 0);
                work->steps = 15;
            }
        } else if (timer == 6) {
            switch (GetRandom() % 3) {
            case 0:
                m4aSongNumStart(SONG_VO_SR_DAMAGE00);
                break;
            case 1:
                m4aSongNumStart(SONG_VO_SR_DAMAGE02);
                break;
            case 2:
            default:
                m4aSongNumStart(SONG_VO_SR_DAMAGE03);
                break;
            }
        }

        if ((s16)work->stateTimer >= work->steps) {
            act->flags &= ~BTLOBJ_FLAG_CARD_USE_BLOCKED;
        }

        if (AnimIsFinished(&work->anim)) {
            ClearBtlObjActionFlags(act);
            act->originX = act->x;
            act->originY = act->y;
            work->state = BTL_SORA_STATE_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_SORA_STATE_RECOVER:
        if (AnimIsFinished(&work->anim)) {
            work->state = BTL_SORA_STATE_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
        }

        break;
    case BTL_SORA_STATE_REVIVE:
        timer = work->stateTimer;

        if (timer == 0) {
            StartBtlSoraKnockback(work);
            work->flags |= BTL_SORA_FLAG_PASS_THROUGH;
            SetBtlSoraAnimation(work, 41, 0);
            m4aSongNumStart(SONG_VO_SR_DEATH00);
            work->speed = 0;
            FadeFromAmount(FADE_MODE_RED, 16, 60);
            work->stateTimer++;
            gBtlWork->hitStop = 30;
            act->flags &= ~BTLOBJ_FLAG_HURT;

            if (act->vx > 0) {
                act->flags |= BTLOBJ_FLAG_FACING_LEFT;
            } else if (act->vx < 0) {
                act->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
            }
        } else {
            if (!(gBtlWork->flags & BTL_FLAG_VS_BATTLE)) {
                gBtlWork->hitStop = 3;
            }

            if (work->vz > 0) {
                work->vz = 0;
            }
        }

        if (AnimIsFinished(&work->anim)) {
            FadeStartIn(FADE_MODE_ADD_WHITE, 30);
            act->btl->hcEffectCount--;
            act->invincibleTimer = 60;
            act->hp = act->maxHp / 4;
            act->flags &= ~BTLOBJ_FLAG_INTANGIBLE;
            ClearBtlObjActionFlags(act);
            CreateBtlPopTask(act, BTL_POP_RECOVER);

            if (gBtlWork->flags & BTL_FLAG_VS_BATTLE) {
                work->state = BTL_SORA_STATE_IDLE;
                work->steps = 0;
                work->stateTimer = 0;
                break;
            }

            if (gBtlWork->enemyCount == 0 && gBtlWork->pendingEnemies <= 0) {
                work->state = BTL_SORA_STATE_END_BATTLE;
                work->steps = 0;
                work->stateTimer = 0;
                break;
            }

            work->state = BTL_SORA_STATE_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_SORA_STATE_END_BATTLE: {
        u16 elapsed;

        SetBtlSoraAnimation(work, 1, 0);
        elapsed = work->stateTimer;

        if ((s16)elapsed > 60) {
            gBtlWork->flags |= BTL_FLAG_BATTLE_OVER;
            work->state = BTL_SORA_STATE_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
        } else {
            work->stateTimer = elapsed + 1;
        }

        break;
    }
    case BTL_SORA_STATE_GRAVITY_SQUASH:
        timer = work->stateTimer;

        if (timer == 0) {
            AnimReset(&work->anim);
            ColliderSetDisabled(&act->collider, TRUE);
            act->flags |= BTLOBJ_FLAG_INTANGIBLE;
            work->anim.frame = 0;
            work->anim.timer = 0;
            work->vz = 1024;
            act->vx = 0;
            act->vy = 0;
            work->steps = 10;
        }

        ApproachValue(&work->scaleY, Q_8_8(0.25), (* &work->steps)--);

        if (work->steps > 0) {
            work->stateTimer++;
        } else {
            work->stateTimer = 0;
            work->state = BTL_SORA_STATE_GRAVITY_HOLD;
        }

        break;
    case BTL_SORA_STATE_GRAVITY_HOLD:
        elapsed = work->stateTimer;

        if ((s16)elapsed > 44) {
            if (act->hp <= 0) {
                work->state = BTL_SORA_STATE_DEFEATED;
            } else {
                work->state = BTL_SORA_STATE_GRAVITY_RECOVER;
            }

            work->stateTimer = 0;
        } else {
            work->stateTimer = elapsed + 1;
        }

        break;
    case BTL_SORA_STATE_GRAVITY_RECOVER:
        if ((s16)work->stateTimer == 0) {
            ColliderSetDisabled(&act->collider, FALSE);
            work->steps = 10;
        }

        ApproachValueHalfSteps(&work->scaleY, Q_8_8(1), (* &work->steps)--);

        if (work->steps > 0) {
            work->stateTimer++;
        } else {
            act->flags &= ~BTLOBJ_FLAG_INTANGIBLE;
            ClearBtlObjActionFlags(act);
            work->state = BTL_SORA_STATE_IDLE;
            work->stateTimer = 0;
        }

        break;
    case BTL_SORA_STATE_DEFEATED:
        FocusBtlSoraCameraOnTarget(work);
        timer = work->stateTimer;

        if (timer == 0) {
            gBtlWork->flags |= BTL_FLAG_PLAYER_DEFEATED;
            StartBtlSoraKnockback(work);
            work->flags |= BTL_SORA_FLAG_PASS_THROUGH;
            SetBtlSoraAnimation(work, 41, 0);
            m4aSongNumStart(SONG_VO_SR_DEATH00);
            work->speed = 0;

            if (gBtlWork->flags & BTL_FLAG_VS_BATTLE) {
                FadeStartIn(FADE_MODE_ADD_WHITE, 20);
                FadeLock();
            } else {
                FadeFromAmount(FADE_MODE_RED, 16, 60);
            }

            work->stateTimer++;
            gBtlWork->hitStop = 30;
            act->flags &= ~BTLOBJ_FLAG_HURT;

            if (act->vx > 0) {
                act->flags |= BTLOBJ_FLAG_FACING_LEFT;
            } else if (act->vx < 0) {
                act->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
            }

            break;
        }

        gBtlWork->hitStop = 3;

        if (work->vz > 0) {
            work->vz = 0;
        }

        break;
    case BTL_SORA_STATE_ARS_ARCANUM: {
        u16 elapsed;

        FocusBtlSoraCameraOnTarget(work);
        timer = work->stateTimer;

        if (timer == 0) {
            BtlObj* enemy;

            if (act->z < act->groundZ) {
                switch (work->comboCount % 3) {
                case 0:
                    SetBtlSoraAnimation(work, 23, 0);
                    break;
                case 1:
                    SetBtlSoraAnimation(work, 28, 0);
                    break;
                case 2:
                    SetBtlSoraAnimation(work, 33, 0);
                    break;
                }
            } else {
                switch (work->comboCount % 3) {
                case 0:
                    SetBtlSoraAnimation(work, 3, 0);
                    break;
                case 1:
                    SetBtlSoraAnimation(work, 8, 0);
                    break;
                case 2:
                    SetBtlSoraAnimation(work, 13, 0);
                    break;
                }
            }

            enemy = act->btl->actor2;

            if (enemy != NULL) {
                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    if (act->x < enemy->x) {
                        act->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
                    }
                } else {
                    if (act->x > enemy->x) {
                        act->flags |= BTLOBJ_FLAG_FACING_LEFT;
                    }
                }
            }
        } else if (timer == 5) {
            m4aSongNumStart(GetRandom() % 4 + SONG_VO_SR_ATTACK00);
        }

        elapsed = work->stateTimer;

        if (elapsed >= 9 && elapsed <= 11) {
            enemy = act->btl->actor2;

            if (elapsed == 15) {
                work->vz = -1152;
            }

            if (enemy != NULL) {
                act->x += (enemy->x - act->x) >> 3;
                act->y += (enemy->y - act->y) >> 3;
            } else if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                s32 offsetX = act->x + 8192;
                act->x += (act->originX - offsetX) >> 3;
            } else {
                s32 offsetX = act->x - 8192;
                act->x += (act->originX - offsetX) >> 3;
            }
        }

        if ((s16)work->stateTimer == 12) {
            MakeOpponentsHittable();

            if (work->comboCount == 1) {
                if ((act->flags & BTLOBJ_FLAG_FACING_LEFT)
                    ? ApplyAttackBox(89, act->x - 5120, act->y, act->z, 16, 16, 48) != ATTACK_RESULT_NONE
                    : ApplyAttackBox(89, act->x + 5120, act->y, act->z, 16, 16, 48) != ATTACK_RESULT_NONE) {
                    m4aSongNumStart(SONG_BTL_SR_ATT01);
                }
            } else {
                if ((act->flags & BTLOBJ_FLAG_FACING_LEFT)
                    ? ApplyAttackBox(88, act->x - 5120, act->y, act->z, 16, 16, 48) != ATTACK_RESULT_NONE
                    : ApplyAttackBox(88, act->x + 5120, act->y, act->z, 16, 16, 48) != ATTACK_RESULT_NONE) {
                    m4aSongNumStart(SONG_BTL_SR_ATT00);
                }
            }
        }

        {
            u16 elapsed = work->stateTimer;

            if ((s16)elapsed > 14) {
                work->comboCount--;
                act->originX = act->x;
                act->originY = act->y;
                act->originZ = act->z;

                if (work->comboCount == 0) {
                    work->state = work->nextState;
                    work->steps = 0;
                    work->stateTimer = 0;
                } else {
                    work->state = BTL_SORA_STATE_ARS_ARCANUM;
                    work->steps = 0;
                    work->stateTimer = 0;
                    work->steps = 1;
                }
            } else {
                work->stateTimer = elapsed + 1;
            }

            break;
        }
    }
    case BTL_SORA_STATE_SHOCK_IMPACT:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        FocusBtlSoraCameraOnTarget(work);

        switch ((s16)work->stateTimer) {
        case 0:
            SetBtlSoraAnimation(work, 14, 0);
            FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
            m4aSongNumStart(SONG_VO_SR_ATTACK08);
            break;
        case 15:
            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                BgFxStartShockwave(act->x - 5120, act->y + act->z - 4096, TRUE);
            } else {
                BgFxStartShockwave(act->x + 5120, act->y + act->z - 4096, FALSE);
            }

            m4aSongNumStart(SONG_EF_SHOCFLOR);
            break;
        case 20:
            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                ApplyAttackBox(107, act->x - 32768, act->y, act->z, 160, 256, 256);
            } else {
                ApplyAttackBox(107, act->x + 32768, act->y, act->z, 160, 256, 256);
            }

            break;
        case 40:
            MakeOpponentsHittable();

            if (gBtlWork->flags & BTL_FLAG_VS_BATTLE) {
                if (work->mainSide) {
                    enemy = gRikuBtlWork->actor;
                } else {
                    enemy = gBtlWork->actor;
                }

                if (enemy->vx != 0 || enemy->vy != 0) {
                    ApplyAttackToBtlObj(108, enemy);
                }
            } else {
                enemy = ListPoolFirst(&gBtlWork->pool);

                while (enemy != NULL) {
                    if (enemy->vx != 0 || enemy->vy != 0) {
                        ApplyAttackToBtlObj(108, enemy);
                    }

                    enemy = ListPoolNext(&enemy->node);
                }

                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    ApplyAttackBox(108, act->x - 32768, act->y, act->z, 160, 256, 256);
                } else {
                    ApplyAttackBox(108, act->x + 32768, act->y, act->z, 160, 256, 256);
                }
            }

            break;
        }

        elapsed = work->stateTimer;

        if ((s16)elapsed == 41) {
            FadeToOriginal(FADE_MODE_BLACK, 8);
            SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
        } else {
            work->stateTimer = elapsed + 1;
        }

        break;
    case BTL_SORA_STATE_SLIDING_DASH:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        FocusBtlSoraCameraOnTarget(work);

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 71, 0);
            m4aSongNumStart(SONG_VO_SR_SUMMON05);
            work->unk_194 = 0;
        }

        if (work->anim.timer == 0) {
            switch (AnimGetFrame(&work->anim)) {
            case 4:
                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    act->x -= 2560;
                } else {
                    act->x += 2560;
                }

                break;
            case 5:
                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    act->x += 1024;
                } else {
                    act->x -= 1024;
                }

                break;
            case 6:
                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    act->x += 1024;
                } else {
                    act->x -= 1024;
                }

                break;
            }
        }

        if (AnimGetFrame(&work->anim) > 4) {
            work->unk_194 += 128;

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                act->x -= work->unk_194;
            } else {
                act->x += work->unk_194;
            }

            if ((act->flags & BTLOBJ_FLAG_FACING_LEFT)
                ? ApplyAttackBox(94, act->x - 6400, act->y, act->z, 10, 12, 32) != ATTACK_RESULT_NONE
                : ApplyAttackBox(94, act->x + 6400, act->y, act->z, 10, 12, 32) != ATTACK_RESULT_NONE) {
                m4aSongNumStart(SONG_BTL_LT_HIT00);
            }
        }

        if (AnimIsFinished(&work->anim)) {
            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                act->x -= 2048;
            } else {
                act->x += 2048;
            }

            work->state = BTL_SORA_STATE_SLIDING_DASH_SLIDE;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_SORA_STATE_SLIDING_DASH_SLIDE: {
        u16 elapsed;

        FocusBtlSoraCameraOnTarget(work);

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 72, 0);
            EnableBtlSoraPassThrough(work);
        }

        if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
            act->x -= work->unk_194;
        } else {
            act->x += work->unk_194;
        }

        if (held & DPAD_UP) {
            act->y -= 384;
        } else if (held & DPAD_DOWN) {
            act->y += 384;
        }

        if ((act->flags & BTLOBJ_FLAG_FACING_LEFT)
            ? ApplyAttackBox(94, act->x - 6400, act->y, act->z, 10, 12, 12) != ATTACK_RESULT_NONE
            : ApplyAttackBox(94, act->x + 6400, act->y, act->z, 10, 12, 12) != ATTACK_RESULT_NONE) {
            m4aSongNumStart(SONG_BTL_LT_HIT00);
        }

        elapsed = work->stateTimer;

        if ((s16)elapsed > 18) {
            work->state = BTL_SORA_STATE_SLIDING_DASH_END;
            DisableBtlSoraPassThrough(work);
            work->stateTimer = 0;
        } else {
            work->stateTimer = elapsed + 1;
        }

        break;
    }
    case BTL_SORA_STATE_SLIDING_DASH_END:
        FocusBtlSoraCameraOnTarget(work);

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 73, 0);
            act->flags &= ~BTLOBJ_FLAG_IN_CARD_ACTION;
        }

        work->unk_194 -= 128;

        if (work->unk_194 < 0) {
            work->unk_194 = 0;
        }

        if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
            act->x -= work->unk_194;
        } else {
            act->x += work->unk_194;
        }

        if (AnimGetFrame(&work->anim) == 1) {
            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                act->x += 128;
            } else {
                act->x -= 128;
            }
        }

        if (AnimIsFinished(&work->anim)) {
            work->stateTimer = 0;
            SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_SORA_STATE_ZANTETSUKEN:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        FocusBtlSoraCameraOnTarget(work);

        if ((s16)work->stateTimer == 0) {
            m4aSongNumStart(SONG_VO_SR_SUMMON06);
            SetBtlSoraAnimation(work, 74, 0);
        }

        switch (AnimGetFrame(&work->anim)) {
        case 0:
            if (work->anim.timer == 0) {
                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    act->x += 768;
                } else {
                    act->x -= 768;
                }
            }

            break;
        case 2:
        case 3:
            if (work->anim.timer == 0) {
                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    act->x += 512;
                } else {
                    act->x -= 512;
                }
            }

            break;
        case 4:
            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                s32 offsetX = act->x + 16384;
                act->x += (act->originX - offsetX) >> 4;
            } else {
                s32 offsetX = act->x - 16384;
                act->x += (act->originX - offsetX) >> 4;
            }

            break;
        }

        if (AnimIsFinished(&work->anim)) {
            work->state = BTL_SORA_STATE_ZANTETSUKEN_SLASH;
            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_SORA_STATE_ZANTETSUKEN_SLASH:
        FocusBtlSoraCameraOnTarget(work);

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 75, 0);
            work->steps = 5;
        }

        if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
            s32 offsetX = act->x + 16384;
            act->x += (act->originX - offsetX) >> 4;
        } else {
            s32 offsetX = act->x - 16384;
            act->x += (act->originX - offsetX) >> 4;
        }

        timer = --work->steps;

        if (timer <= 1) {
            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                BgFxStartZantetsuken(act->x - 5120, act->y, act->z - 7680, FALSE);
            } else {
                BgFxStartZantetsuken(act->x + 5120, act->y, act->z - 7680, TRUE);
            }

            FadeStartIn(FADE_MODE_RED, 10);

            if ((act->flags & BTLOBJ_FLAG_FACING_LEFT)
                ? ApplyAttackBox(95, act->x - 6400, act->y, act->z, 10, 12, 12) != ATTACK_RESULT_NONE
                : ApplyAttackBox(95, act->x + 6400, act->y, act->z, 10, 12, 12) != ATTACK_RESULT_NONE) {
                m4aSongNumStart(SONG_EF_ZANTETSU);
                SetBtlSoraAnimation(work, 76, 0);
                work->state = BTL_SORA_STATE_ZANTETSUKEN_END;
                work->stateTimer = 0;
                break;
            }
        }

        if (work->steps <= 0) {
            work->state = BTL_SORA_STATE_ZANTETSUKEN_END;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_SORA_STATE_ZANTETSUKEN_END:
        FocusBtlSoraCameraOnTarget(work);

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 76, 0);
        }

        if (AnimIsFinished(&work->anim)) {
            work->stateTimer = 0;
            SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_SORA_STATE_BLITZ:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        FocusBtlSoraCameraOnTarget(work);
        FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
        work->state = BTL_SORA_STATE_BLITZ_STRIKE;
        work->steps = 0;
        work->stateTimer = 0;
        break;
    case BTL_SORA_STATE_BLITZ_STRIKE:
        FocusBtlSoraCameraOnTarget(work);

        if ((s16)work->stateTimer == 0) {
            AnimReset(&work->anim);
            SetBtlSoraAnimation(work, 70, 0);
        }

        if (AnimGetFrame(&work->anim) == 5 && work->anim.timer == 0) {
            work->vz = 4096;
        }

        switch (AnimGetFrame(&work->anim)) {
        case 1:
            if (work->anim.timer != 0) {
                break;
            }

            switch (work->comboCount) {
            case 0:
                work->vz = -1536;
                m4aSongNumStart(SONG_VO_SR_ATTACK07);
                break;
            case 1:
                work->vz = -640;
                m4aSongNumStart(SONG_VO_SR_ATTACK00);
                break;
            case 2:
            default:
                work->vz = -640;
                m4aSongNumStart(SONG_VO_SR_ATTACK01);
                break;
            }

            enemy = PickBtlSoraTarget(work);

            if (enemy != NULL) {
                work->unk_194 = enemy->x;
                work->targetY = enemy->y;
            } else {
                work->unk_194 = act->x;
                work->targetY = act->y;
            }

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                work->angle = 192;
            } else {
                work->angle = 64;
            }

            if (act->x > work->unk_194) {
                act->flags |= BTLOBJ_FLAG_FACING_LEFT;
                work->unk_194 += 5120;
            } else {
                act->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
                work->unk_194 -= 5120;
            }

            break;
        case 2:
        case 3:
        case 4:
        case 5:
            act->x += (work->unk_194 - act->x) >> 3;
            act->y += (work->targetY - act->y) >> 3;
            break;
        case 6:
            if (work->anim.timer % 6 == 0) {
                MakeOpponentsHittable();
            }

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                if (ApplyAttackBox(93, act->x - 8192, act->y, act->z - 8192, 20, 25, 32) != ATTACK_RESULT_NONE) {
                    if (work->comboCount == 0) {
                        m4aSongNumStart(SONG_BTL_LT_HIT00);
                    } else {
                        m4aSongNumStart(SONG_BTL_SR_ATT02);
                    }
                }
            } else {
                if (ApplyAttackBox(93, act->x + 8192, act->y, act->z - 8192, 20, 25, 32) != ATTACK_RESULT_NONE) {
                    if (work->comboCount == 0) {
                        m4aSongNumStart(SONG_BTL_LT_HIT00);
                    } else {
                        m4aSongNumStart(SONG_BTL_SR_ATT02);
                    }
                }
            }

            break;
        case 7:
            if (act->z < act->groundZ) {
                work->anim.timer = 0;
                work->anim.frame--;
            }

            break;
        }

        if (AnimIsFinished(&work->anim)) {
            if (work->comboCount == 0) {
                SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
                FadeToOriginal(FADE_MODE_BLACK, 8);
                break;
            }

            work->comboCount--;
            work->state = BTL_SORA_STATE_BLITZ_STRIKE;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_SORA_STATE_ARS_ARCANUM_FINISH: {
        s32 frame;
        u16 elapsed;

        FocusBtlSoraCameraOnTarget(work);

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 68, 0);
            EnableBtlSoraPassThrough(work);
            SetBattleZoom(4, Q_8_8(1), gBtlWork->x2, gBtlWork->y2);
        }

        elapsed = work->stateTimer;

        if (elapsed >= 15 && elapsed <= 39) {
            enemy = act->btl->actor2;

            if (elapsed == 15) {
                work->vz = -1152;
            }

            if ((s16)work->stateTimer == 22) {
                m4aSongNumStart(SONG_VO_SR_ATTACK07);
            }

            if (enemy != NULL) {
                act->x += (enemy->x - act->x) >> 3;
                act->y += (enemy->y - act->y) >> 3;
            } else if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                s32 offsetX = act->x + 12288;
                act->x += (act->originX - offsetX) >> 3;
            } else {
                s32 offsetX = act->x - 12288;
                act->x += (act->originX - offsetX) >> 3;
            }

            if ((s16)work->stateTimer == 38) {
                MakeOpponentsHittable();

                if ((act->flags & BTLOBJ_FLAG_FACING_LEFT)
                    ? ApplyAttackBox(90, act->x - 1024, act->y, act->z, 32, 24, 32) != ATTACK_RESULT_NONE
                    : ApplyAttackBox(90, act->x + 1024, act->y, act->z, 32, 24, 32) != ATTACK_RESULT_NONE) {
                    m4aSongNumStart(SONG_BTL_SR_ATT02);

                    if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                        SetBattleZoom(8, Q_8_8(1.5), act->x - 2048, act->y + act->z);
                    } else {
                        SetBattleZoom(8, Q_8_8(1.5), act->x + 2048, act->y + act->z);
                    }

                    FadeStartIn(FADE_MODE_ADD_WHITE, 20);
                }
            } else if ((s16)work->stateTimer == 39) {
                SetBattleZoom(5, Q_8_8(1), gBtlWork->x2, gBtlWork->y2);
            }
        }

        frame = AnimGetFrame(&work->anim);

        switch (frame) {
        case 3:
        case 4:
        case 5:
        case 6:
        case 7:
        case 8:
            work->vz = 0;
            break;
        }

        if (AnimIsFinished(&work->anim)) {
            DisableBtlSoraPassThrough(work);
            SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
            break;
        }

        work->stateTimer++;
    }

        break;
    case BTL_SORA_STATE_RAGNAROK:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        FocusBtlSoraCamera(work);
        SetBtlSoraAnimation(work, 64, 0);

        if (AnimIsFinished(&work->anim)) {
            work->state = BTL_SORA_STATE_RAGNAROK_JUMP;
            work->steps = 0;
            work->stateTimer = 0;
        }

        break;
    case BTL_SORA_STATE_RAGNAROK_JUMP:
        FocusBtlSoraCamera(work);

        if ((s16)work->stateTimer == 0) {
            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                work->angle = 192;
            } else {
                work->angle = 64;
            }

            SetBtlSoraDirAnimation(work, 2, 0);
            work->vz = -896;
            m4aSongNumStart(SONG_VO_SR_SUMMON06);
        }

        if (work->vz >= 0) {
            work->state = BTL_SORA_STATE_RAGNAROK_HOVER;
            work->steps = 0;
            work->stateTimer = 0;
            act->originZ = act->z;
            work->timer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_SORA_STATE_RAGNAROK_HOVER:
        FocusBtlSoraCamera(work);
        work->vz = 0;

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 65, 0);
        }

        {
            const s16* sine = gSineTable;
            u16 phase = work->timer;
            work->timer = phase + 1;
            act->z += (act->originZ + (sine[(phase * 2) & 0xFF] << 3) - act->z) >> 3;
        }

        if (AnimIsFinished(&work->anim)) {
            work->state = BTL_SORA_STATE_RAGNAROK_CHARGE;
            work->steps = 0;
            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_SORA_STATE_RAGNAROK_CHARGE: {
        u16 elapsed;

        FocusBtlSoraCamera(work);
        work->vz = 0;

        if ((s16)work->stateTimer == 0) {
            FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
            SetBtlSoraAnimation(work, 66, 0);

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                BgFxStartRagnarokCharge(act->x - 10240, act->y, act->z - 6144);
            } else {
                BgFxStartRagnarokCharge(act->x + 10240, act->y, act->z - 6144);
            }

            work->steps = 120;
        }

        if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
            BgFxSetPosition(act->x - 10240, act->y, act->z - 6144);
        } else {
            BgFxSetPosition(act->x + 10240, act->y, act->z - 6144);
        }

        if (held & DPAD_LEFT) {
            act->x -= 640;
        } else if (held & DPAD_RIGHT) {
            act->x += 640;
        }

        {
            const s16* sine = gSineTable;
            u16 phase = work->timer;
            work->timer = phase + 1;
            act->z += (act->originZ + (sine[(phase * 2) & 0xFF] << 3) - act->z) >> 3;
        }

        if (held & DPAD_UP) {
            act->y -= 320;
        } else if (held & DPAD_DOWN) {
            act->y += 320;
        }

        elapsed = work->stateTimer;
        timer = work->stateTimer;

        if ((timer > 9 && (pressed & A_BUTTON)) || timer > 120) {
            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                BgFxStartRagnarokShot(act->x - 10240, act->y, act->z - 6144, TRUE);
            } else {
                BgFxStartRagnarokShot(act->x + 10240, act->y, act->z - 6144, FALSE);
            }

            work->state = BTL_SORA_STATE_RAGNAROK_SHOT;
            work->steps = 0;
            work->stateTimer = 0;
            act->originX = act->x;
            act->originY = act->y;
        } else {
            work->stateTimer = elapsed + 1;
        }

        break;
    }
    case BTL_SORA_STATE_RAGNAROK_SHOT: {
        FocusBtlSoraCamera(work);
        SetBtlSoraAnimation(work, 67, 0);
        work->vz = 0;

        {
            const s16* sine = gSineTable;
            u16 phase = work->timer;
            work->timer = phase + 1;
            act->z += (act->originZ + (sine[(phase * 2) & 0xFF] << 3) - act->z) >> 3;
        }

        if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
            s32 offsetX = act->x - 8192;
            act->x += (act->originX - offsetX) >> 3;
        } else {
            s32 offsetX = act->x + 8192;
            act->x += (act->originX - offsetX) >> 3;
        }

        if (AnimIsFinished(&work->anim) && !BgFxIsActive()) {
            FadeToOriginal(FADE_MODE_BLACK, 8);
            SetBtlSoraState(work, BTL_SORA_STATE_AIRBORNE);
        } else {
            work->stateTimer++;
        }

        break;
    }
    case BTL_SORA_STATE_AQUA_SPLASH:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        FocusBtlSoraCamera(work);
        SetBtlSoraAnimation(work, 64, 0);

        if (AnimIsFinished(&work->anim)) {
            work->state = BTL_SORA_STATE_AQUA_SPLASH_JUMP;
            work->steps = 0;
            work->stateTimer = 0;
        }

        break;
    case BTL_SORA_STATE_AQUA_SPLASH_JUMP:
        FocusBtlSoraCamera(work);

        if ((s16)work->stateTimer == 0) {
            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                work->angle = 192;
            } else {
                work->angle = 64;
            }

            SetBtlSoraDirAnimation(work, 2, 0);
            work->vz = -896;
            m4aSongNumStart(SONG_VO_SR_SUMMON06);
        }

        if (work->vz >= 0) {
            work->state = BTL_SORA_STATE_AQUA_SPLASH_HOVER;
            work->steps = 0;
            work->stateTimer = 0;
            act->originZ = act->z;
            work->timer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_SORA_STATE_AQUA_SPLASH_HOVER:
        FocusBtlSoraCamera(work);
        work->vz = 0;

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 65, 0);
        }

        {
            const s16* sine = gSineTable;
            u16 phase = work->timer;
            work->timer = phase + 1;
            act->z += (act->originZ + (sine[(phase * 2) & 0xFF] << 3) - act->z) >> 3;
        }

        if (AnimIsFinished(&work->anim)) {
            work->state = BTL_SORA_STATE_AQUA_SPLASH_SPRAY;
            work->steps = 0;
            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_SORA_STATE_AQUA_SPLASH_SPRAY: {
        s32 prevX;
        s32 prevY;
        s32 prevZ;

        FocusBtlSoraCamera(work);
        work->vz = 0;
        timer = work->stateTimer;

        if (timer == 0) {
            SetBtlSoraAnimation(work, 66, 0);
            m4aSongNumStart(SONG_EF_DAMBO_SPLOOP);

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                BgFxStartDumboSplash(SUMMON_LEVEL_PAIR, act->x - 9728, act->y, act->z - 6656, FALSE, 99);
            } else {
                BgFxStartDumboSplash(SUMMON_LEVEL_PAIR, act->x + 9728, act->y, act->z - 6656, TRUE, 99);
            }
        }

#ifdef VERSION_EU
        if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
            BgFxSetPosition(act->x - 9728, act->y, act->z - 6656);
        } else {
            BgFxSetPosition(act->x + 9728, act->y, act->z - 6656);
        }
#else
        prevX = act->x;
        prevY = act->y;
        prevZ = act->z;

        {
            const s16* sine = gSineTable;
            u16 phase = work->timer;
            work->timer = phase + 1;
            act->z += (act->originZ + (sine[(phase * 2) & 0xFF] << 3) - act->z) >> 3;
        }

        if (held & DPAD_UP) {
            act->y -= 128;
        } else if (held & DPAD_DOWN) {
            act->y += 128;
        }

        if (held & DPAD_LEFT) {
            act->x -= 256;
        } else if (held & DPAD_RIGHT) {
            act->x += 256;
        }

        ClampBattlePosition(&act->x, &act->y, -16, 0);
        BgFxAddPosition(act->x - prevX, act->y - prevY, act->z - prevZ);
#endif

        if (!BgFxIsActive()) {
            m4aSongNumStop(SONG_EF_DAMBO_SPLOOP);
            SetBtlSoraState(work, BTL_SORA_STATE_AIRBORNE);
        } else {
            work->stateTimer++;
        }
    }

        break;
    case BTL_SORA_STATE_FIRAGA_BREAK:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        FocusBtlSoraCameraOnTarget(work);

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 21, 0);
            m4aSongNumStart(SONG_VO_SR_FIRE00);
        }

        if (work->anim.timer == 0) {
            switch (AnimGetFrame(&work->anim)) {
            case 3:
                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    BgFxStartFireExplosion(act->x - 12288, act->y, act->z - 4096);
                } else {
                    BgFxStartFireExplosion(act->x + 12288, act->y, act->z - 4096);
                }

                m4aSongNumStart(SONG_EF_FIRE03);
                break;
            case 5:
                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    ApplyAttackBox(97, act->x - 12288, act->y, act->z, 40, 40, 50);
                } else {
                    ApplyAttackBox(97, act->x + 12288, act->y, act->z, 40, 40, 50);
                }

                break;
            }
        }

        if (AnimIsFinished(&work->anim) && !BgFxIsActive()) {
            SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_SORA_STATE_STUN_IMPACT:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        FocusBtlSoraCamera(work);

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 69, 0);
            m4aSongNumStart(SONG_VO_SR_ATTACK08);
            FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
        }

        if (work->anim.timer == 0) {
            switch (AnimGetFrame(&work->anim)) {
            case 2:
                BgFxStartLimit(act->x, act->y, act->z - 17920, FALSE);
                m4aSongNumStart(SONG_EF_LIMITST);
                break;
            case 5:
                BgFxStartStunImpact(act->x, act->y, act->z);
                m4aSongNumStart(SONG_EF_STANIMP);
                break;
            case 6:
                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    ApplyAttackBox(92, act->x, act->y, act->z, 64, 64, 100);
                } else {
                    ApplyAttackBox(92, act->x, act->y, act->z, 64, 64, 100);
                }

                break;
            }
        }

        if (AnimIsFinished(&work->anim) && !BgFxIsActive()) {
            FadeToOriginal(FADE_MODE_BLACK, 8);
            SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_SORA_STATE_QUAKE:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        FocusBtlSoraCamera(work);

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 69, 0);
            m4aSongNumStart(SONG_VO_SR_ATTACK08);
            FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
        }

        if (work->anim.timer == 0) {
            switch (AnimGetFrame(&work->anim)) {
            case 2:
                BgFxStartLimit(act->x, act->y, act->z - 17920, FALSE);
                m4aSongNumStart(SONG_EF_LIMITST);
                break;
            case 6:
                BtlMapStartShake();
                m4aSongNumStart(SONG_EF_KUEIK);
                FadeFromAmount(FADE_MODE_DARK_MAGENTA, 15, 30);
                MakeOpponentsHittable();
                ApplyAttackBox(106, act->x, act->y, act->z, 512, 512, 1);
                break;
            }
        }

        if (AnimIsFinished(&work->anim) && !FadeIsActive()) {
            FadeToOriginal(FADE_MODE_BLACK, 8);
            SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_SORA_STATE_FROZEN:
        timer = work->stateTimer;

        if (timer == 0) {
            act->flags |= BTLOBJ_FLAG_INTANGIBLE;
            act->flags |= BTLOBJ_FLAG_CARD_USE_BLOCKED;
            work->speed = 0;
            work->flags |= BTL_SORA_FLAG_HIDDEN;
        }

        if ((s16)work->stateTimer > 285) {
            gBtlWork->flags |= 0x100000;
        } else {
            if (pressed & DPAD_ANY) {
                act->vx = GetRandom() % 257 - 128;
                act->vy = GetRandom() % 257 - 128;
                work->stateTimer += 2;
            }

            if (act->z >= act->groundZ && (pressed & B_BUTTON)) {
                work->vz = -256;
                work->stateTimer += 2;
            }

            if (work->stateTimer % 8 == 0) {
                u16 hp;

                hp = act->hp;

                if ((s16)hp > 1) {
                    act->hp = hp - 1;
                }
            }
        }

        elapsed = work->stateTimer;

        if ((s16)elapsed > 300) {
            act->flags &= ~BTLOBJ_FLAG_INTANGIBLE;
            act->flags &= ~BTLOBJ_FLAG_CARD_USE_BLOCKED;
            work->flags &= ~BTL_SORA_FLAG_HIDDEN;
            work->state = BTL_SORA_STATE_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
        } else {
            work->stateTimer = elapsed + 1;
        }

        break;
    case BTL_SORA_STATE_TRINITY_LIMIT:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        FocusBtlSoraCamera(work);

        switch ((s16)work->stateTimer) {
        case 0:
            FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
            m4aSongNumStart(SONG_VO_SR_SUMMON01);
            SetBtlSoraAnimation(work, 60, 0);
            break;
        case 15:
            BgFxStartLimit(act->x, act->y, act->z - 17920, FALSE);
            m4aSongNumStart(SONG_EF_LIMIMOV);
            break;
        }

        if (AnimIsFinished(&work->anim)) {
            work->state = BTL_SORA_STATE_TRINITY_LIMIT_JUMP;
            work->steps = 0;
            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_SORA_STATE_TRINITY_LIMIT_JUMP: {
        s32 targetY;
        s32 centerY;

        FocusBtlSoraCamera(work);

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 61, 0);
            work->steps = 20;
        }

        if (work->anim.timer == 0) {
            switch (AnimGetFrame(&work->anim)) {
            case 2:
                work->vz = -768;
                break;
            case 14:
                m4aSongNumStart(SONG_EF_TLIMIT02);
                break;
            }
        }

        if (AnimGetFrame(&work->anim) > 1 && work->steps > 0) {
            s32 targetX = 0x10000;

            targetY = (gBtlWork->yMin + gBtlWork->yMax) << 7;
            ApproachValueHalfSteps(&act->x, targetX, work->steps);
            ApproachValueHalfSteps(&act->y, targetY, work->steps);

            if (--work->steps == 0) {
                m4aSongNumStart(SONG_EF_TLIMIT01);
            }
        }

        if (act->z >= act->groundZ && work->steps == 0) {
            centerY = (gBtlWork->yMin + gBtlWork->yMax) << 7;

            if (act->groundZ != 0 || act->x != 0x10000 || act->y != centerY) {
                gBtlWork->flags |= BTL_FLAG_STOP_BGFX;
                CreateBtlPopTask(act, BTL_POP_MISS);
                ClearBtlObjActionFlags(act);
                work->state = BTL_SORA_STATE_IDLE;
                work->steps = 0;
                work->stateTimer = 0;
                break;
            }
        }

        if ((s16)work->stateTimer == 18) {
            BgFxStartTrinityLimit(0x10000, (gBtlWork->yMin + gBtlWork->yMax) << 7, 0);
            act->flags |= 0x0000000400000000LL;
        }

        if (AnimIsFinished(&work->anim)) {
            work->state = BTL_SORA_STATE_TRINITY_LIMIT_CHARGE;
            work->steps = 0;
            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }
    }

        break;
    case BTL_SORA_STATE_TRINITY_LIMIT_CHARGE:
        FocusBtlSoraCamera(work);

        switch ((s16)work->stateTimer) {
        case 0:
            SetBtlSoraAnimation(work, 62, 0);
            m4aSongNumStart(SONG_EF_TLIMIT03);
            break;
        case 30:
            BgFxStartTrinityLimitCharge(0x10000, ((gBtlWork->yMin + gBtlWork->yMax) << 7) + 512, 0);
            m4aSongNumStart(SONG_EF_TLIMIT04);
            act->flags &= ~0x0000000400000000LL;
            break;
        case 44:
            FadeToOriginal(FADE_MODE_BLACK, 8);
            break;
        }

        elapsed = work->stateTimer;

        if ((s16)elapsed > 59) {
            work->state = BTL_SORA_STATE_TRINITY_LIMIT_BLAST;
            work->steps = 0;
            work->stateTimer = 0;
        } else {
            work->stateTimer = elapsed + 1;
        }

        break;
    case BTL_SORA_STATE_TRINITY_LIMIT_BLAST:
        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 63, 0);
            BgFxStartTrinityLimitBlast(0x10000, (gBtlWork->yMin + gBtlWork->yMax) << 7, -15872);
        }

        FocusBtlSoraCamera(work);

        if (AnimIsFinished(&work->anim)) {
            SetBtlSoraAnimation(work, 1, 1);
        }

        if (!BgFxIsActive()) {
            ClearBtlObjActionFlags(act);
            work->state = BTL_SORA_STATE_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_SORA_STATE_RAID:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlSoraCameraOnTarget(work);
            break;
        }

        FocusBtlSoraCameraOnTarget(work);

        switch ((s16)work->stateTimer) {
        case 0:
            SetBtlSoraAnimation(work, 54, 0);

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                BgFxStartLimit(act->x + 3072, act->y, act->z - 4096, TRUE);
            } else {
                BgFxStartLimit(act->x - 3072, act->y, act->z - 4096, FALSE);
            }

            m4aSongNumStart(SONG_EF_LIMIMOV);
            FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
            break;
        case 25:
            SetBtlSoraAnimation(work, 55, 0);
            break;
        case 37:
            args.mainSide = work->mainSide;
            args.variant = work->variant[0];
            args.y = act->y;
            args.z = act->z - 6144;

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                args.facingLeft = TRUE;
                args.x = act->x - 6144;
            } else {
                args.facingLeft = FALSE;
                args.x = act->x + 6144;
            }

            work->task = TaskCreate(&work->tasks, &gTaskDescBtlRaid, &args);
            m4aSongNumStart(SONG_VO_SR_SUMMON06);
            break;
        case 67:
            SetBtlSoraAnimation(work, 56, 0);
            break;
        }

        if ((s16)work->stateTimer > 37 && !IsTaskActiveNamed(work->task, gTaskDescBtlRaid.name)) {
            work->state = BTL_SORA_STATE_RAID_CATCH;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_SORA_STATE_RAID_CATCH:
        FocusBtlSoraCameraOnTarget(work);
        SetBtlSoraAnimation(work, 57, 0);

        if (AnimIsFinished(&work->anim)) {
            FadeToOriginal(FADE_MODE_BLACK, 8);
            SetBtlSoraState(work, BTL_SORA_STATE_IDLE);
        }

        break;
    case BTL_SORA_STATE_STOPPED:
        FocusBtlSoraCamera(work);
        work->vz = 0;

        if (act->badStatus != BAD_STATUS_STOP) {
            work->state = BTL_SORA_STATE_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
            ClearBtlObjActionFlags(act);
            work->flags &= ~BTL_SORA_FLAG_PASS_THROUGH;
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_SORA_STATE_STUNNED:
        FocusBtlSoraCamera(work);
        timer = work->stateTimer;

        if (timer == 0) {
            AnimReset(&work->anim);
            SetBtlSoraAnimation(work, 40, 0);
            work->stateTimer++;
            act->flags |= BTLOBJ_FLAG_CARD_USE_BLOCKED;
            work->speed = 0;
        }

        if (AnimIsFinished(&work->anim)) {
            act->flags &= ~BTLOBJ_FLAG_HIT_LOCKED;
        }

        if (pressed & (A_BUTTON | B_BUTTON | DPAD_ANY)) {
            act->badStatusTimer -= 1;
        }

        if (act->badStatus != BAD_STATUS_STUN) {
            ClearBtlObjActionFlags(act);
            work->state = BTL_SORA_STATE_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
        }

        break;
    case BTL_SORA_STATE_SLIP:
        FocusBtlSoraCameraOnTarget(work);
        timer = work->stateTimer;

        if (timer == 0) {
            m4aSongNumStart(SONG_BTL_SR_SLIP);
            act->vx = 0;
            act->vy = 0;
            act->flags |= BTLOBJ_FLAG_CARD_USE_BLOCKED;
            SetBtlSoraAnimation(work, 58, 0);
            work->speed = 256;
        } else {
            act->x += (gSineTable[work->angle] * work->speed) >> 8;
        }

        act->y += (-gSineTable[work->angle + 64] * (work->speed >> 1)) >> 8;
        work->speed -= 10;

        if (work->speed < 0) {
            work->speed = 0;
        }

        elapsed = work->stateTimer;

        if ((s16)elapsed > 40) {
            work->state = BTL_SORA_STATE_SLIP_RECOVER;
            work->stateTimer = 0;
        } else {
            work->stateTimer = elapsed + 1;
        }

        break;
    case BTL_SORA_STATE_SLIP_RECOVER:
        FocusBtlSoraCameraOnTarget(work);

        if ((s16)work->stateTimer == 0) {
            SetBtlSoraAnimation(work, 59, 0);
            work->speed = 256;
        }

        if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
            act->x -= work->speed;
        } else {
            act->x += work->speed;
        }

        work->speed -= 12;

        if (work->speed < 0) {
            work->speed = 0;
        }

        if (AnimIsFinished(&work->anim)) {
            act->flags &= ~BTLOBJ_FLAG_CARD_USE_BLOCKED;
            work->speed = 0;
            work->state = BTL_SORA_STATE_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_SORA_STATE_FIELD_HIDDEN:
        act->flags &= ~BTLOBJ_FLAG_HURT;
        act->vx = act->vy = 0;
        work->speed = 0;
        break;
    }

    if ((u16)(pressed & DPAD_LEFT) != 0) {
        work->tapTimers[0] = 13;
        work->tapTimers[1] = 0;
    } else if (pressed & DPAD_RIGHT) {
        work->tapTimers[1] = 13;
        work->tapTimers[0] = 0;
    }

    if (work->tapTimers[1] != 0) {
        work->tapTimers[1]--;
    }

    if (work->tapTimers[0] != 0) {
        work->tapTimers[0]--;
    }

    if (act->badStatus == BAD_STATUS_BIND) {
        work->tapTimers[0] = 0;
        work->tapTimers[1] = 0;
        work->speed = 0;
        act->vx = act->vy = 0;

        if (work->vz < 0) {
            work->vz = 768;
        }
    } else {
        act->x += (gSineTable[work->angle] * work->speed) >> 8;
        act->y += (-gSineTable[work->angle + 64] * (work->speed >> 1)) >> 8;
    }

    if (act->collider.colliding) {
        if (act->collider.otherType == COLLIDER_TYPE_SLIPPERY) {
            if (work->speed > 0 && work->state == BTL_SORA_STATE_IDLE) {
                work->state = BTL_SORA_STATE_SLIP;
                work->steps = 0;
                work->stateTimer = 0;
            }
        } else if ((work->flags & BTL_SORA_FLAG_ON_PLATFORM) && act->collider.otherType == COLLIDER_TYPE_PLATFORM) {
            act->collider.standFlags |= COLLIDER_STAND_OVER_PLATFORM;
        } else if ((work->flags & BTL_SORA_FLAG_PASS_THROUGH) == 0 && act->collider.otherType != COLLIDER_TYPE_PICKUP &&
                   (act->collider.other->flags & COLLIDER_FLAG_PASS_THROUGH) == 0) {
            act->x += act->collider.pushX >> 1;
            act->y += act->collider.pushY >> 1;
        }
    }

    act->z += work->vz;
    work->vz += gBtlWork->gravity;

    if (act->collider.standFlags & COLLIDER_STAND_OVER_PLATFORM) {
        act->groundZ = act->collider.platformZ;
        work->flags |= BTL_SORA_FLAG_OVER_PLATFORM;
        work->platformPriority = -4100 - (((act->collider.platformY + 1024) >> 8) * 4);
    } else {
        work->flags &= ~BTL_SORA_FLAG_OVER_PLATFORM;
        act->groundZ = act->floorZ;
    }

    if (work->flags & BTL_SORA_FLAG_ON_PLATFORM) {
        if (gBtlWork->platform == act->collider.other) {
            act->x += act->collider.platformX - work->platformX;
            act->y += act->collider.platformY - work->platformY;
            act->z += act->collider.platformZ - work->platformZ;
        }
    }

    if (act->z >= act->groundZ) {
        if (act->collider.standFlags & COLLIDER_STAND_OVER_PLATFORM) {
            work->flags |= BTL_SORA_FLAG_ON_PLATFORM;
            work->platformX = act->collider.platformX;
            work->platformY = act->collider.platformY;
            work->platformZ = act->collider.platformZ;
            gBtlWork->platform = act->collider.other;
        } else {
            work->flags &= ~BTL_SORA_FLAG_ON_PLATFORM;
            gBtlWork->platform = NULL;
        }

        work->vz = 0;
        act->z = act->groundZ;
        act->btl->flags &= ~BTL_FLAG_PLAYER_AIRBORNE;

        if (work->state == BTL_SORA_STATE_AIRBORNE) {
            work->state = BTL_SORA_STATE_LAND;
            work->steps = 0;
            work->stateTimer = 0;
        } else if (work->state == BTL_SORA_STATE_SUMMON_AIRBORNE) {
            work->state = BTL_SORA_STATE_SUMMON_LAND;
            work->steps = 0;
            work->stateTimer = 0;
        }
    } else {
        if (work->flags & BTL_SORA_FLAG_ON_PLATFORM) {
            work->flags &= ~BTL_SORA_FLAG_ON_PLATFORM;

            if (work->state == BTL_SORA_STATE_IDLE) {
                work->state = BTL_SORA_STATE_AIRBORNE;
                work->steps = 0;
                work->stateTimer = 0;
            } else if (work->state == BTL_SORA_STATE_SUMMON_IDLE) {
                work->state = BTL_SORA_STATE_SUMMON_AIRBORNE;
                work->steps = 0;
                work->stateTimer = 0;
            }
        }

        gBtlWork->platform = NULL;
    }

    if (act->vx > 0) {
        act->x += act->vx;
        act->vx -= 17;

        if (act->vx < 0) {
            act->vx = 0;
        }
    } else if (act->vx < 0) {
        act->x += act->vx;
        act->vx += 17;

        if (act->vx > 0) {
            act->vx = 0;
        }
    }

    if (act->vy > 0) {
        act->y += act->vy;
        act->vy -= 17;

        if (act->vy < 0) {
            act->vy = 0;
        }
    } else if (act->vy < 0) {
        act->y += act->vy;
        act->vy += 17;

        if (act->vy > 0) {
            act->vy = 0;
        }
    }

    if ((act->flags & BTLOBJ_FLAG_IGNORE_BOUNDS) == 0) {
        switch (ClampBattlePosition(&act->x, &act->y, -16, 0)) {
        case BATTLE_EDGE_TOP:
        case BATTLE_EDGE_BOTTOM:
            act->vy = -(act->vy >> 1);
            act->btl->flags &= ~BTL_FLAG_PUSHING_EDGE;
            break;
        case BATTLE_EDGE_LEFT:
            act->vx = -(act->vx >> 1);

            if (act->z == 0 && (held & DPAD_LEFT)) {
                act->btl->flags |= BTL_FLAG_PUSHING_EDGE;
            } else {
                act->btl->flags &= ~BTL_FLAG_PUSHING_EDGE;
            }

            work->flags |= BTL_SORA_FLAG_AT_SIDE_EDGE;
            break;
        case BATTLE_EDGE_RIGHT:
            act->vx = -(act->vx >> 1);

            if (act->z == 0 && (held & DPAD_RIGHT)) {
                act->btl->flags |= BTL_FLAG_PUSHING_EDGE;
            } else {
                act->btl->flags &= ~BTL_FLAG_PUSHING_EDGE;
            }

            work->flags |= BTL_SORA_FLAG_AT_SIDE_EDGE;
            break;
        default:
            act->btl->flags &= ~BTL_FLAG_PUSHING_EDGE;
            work->flags &= ~BTL_SORA_FLAG_AT_SIDE_EDGE;
            break;
        }

        if (act->btl->flags & BTL_FLAG_SUMMON_ACTIVE) {
            act->btl->flags &= ~BTL_FLAG_PUSHING_EDGE;
        }

        if (act->btl->flags & BTL_FLAG_PUSHING_EDGE) {
            if (work->mainSide) {
                FocusBtlSoraCamera(work);
            }
        }
    }

    ApplyBattleBounds(&act->x, &act->y, &act->z, &act->floorZ);
    TaskPoolUpdate(&work->tasks);

    if (work->state == BTL_SORA_STATE_CARD_BROKEN && (work->flags & BTL_SORA_FLAG_HIDDEN)) {
        work->flags &= ~BTL_SORA_FLAG_HIDDEN;
        LoadBtlSoraPalette(work);
        SetBtlSoraAnimation(work, work->breakAnim + 47, 0);
        act->x = act->originX;
        act->y = act->originY;
        act->z = act->originZ;
    }

    if (act->badStatus != BAD_STATUS_STOP) {
        work->gfx = AnimUpdate(&work->anim);
    }

    ColliderSetPosition(&act->collider, act->x, act->y, act->z);
    work->frameCount++;
    return 1;
}

void task_btl_sora_2(BtlSoraWork* work) {
    BtlObj* act;
    ObjAffine* affine;
    s32 sx;
    s32 sy;
    u16 attr;
    u16 priority;
    s16 x;
    s16 y;

    act = &work->actor;

    if (work->flags & BTL_SORA_FLAG_HIDDEN) {
        return;
    }

#ifndef VERSION_EU
    if (work->actor.btl->hcEffect == HC_EFFECT_VANISH) {
        if (work->mainSide) {
            if (gFrameCounter & 1) {
                return;
            }
        } else if (gFrameCounter % 120 <= 59) {
            return;
        }
    }
#endif

    attr = GetBattleSpritePriorityFlags(act->y);

    if (work->scaleX == Q_8_8(1) && work->scaleY == Q_8_8(1)) {
        if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
            sy = gBtlWork->scale;
            sx = sy;
        } else {
            sy = gBtlWork->scale;

            if (sy == Q_8_8(1)) {
                sx = sy;
                attr |= SPRITE_FLAG_HFLIP;
            } else {
                sx = -sy;
            }
        }
    } else {
        if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
            sx = gBtlWork->scale * work->scaleX >> 8;
            sy = gBtlWork->scale * work->scaleY >> 8;
        } else {
            sx = -(gBtlWork->scale * work->scaleX >> 8);
            sy = gBtlWork->scale * work->scaleY >> 8;
        }
    }

    if (sy == Q_8_8(1) && sx == Q_8_8(1)) {
        affine = NULL;
    } else if (sy <= 255) {
        affine = AllocObjAffine(0, sx, sy, FALSE);
    } else {
        affine = AllocObjAffine(0, sx, sy, TRUE);
    }

    if (work->flags & BTL_SORA_FLAG_OVER_PLATFORM) {
        priority = work->platformPriority | 1;

        if (act->collider.penetration <= act->collider.radius) {
            if (act->groundZ != 0) {
                act->shadowPriority = 0;
            } else {
                act->shadowPriority = 0xEFFF;
            }
        } else {
            act->shadowPriority = work->platformPriority | 2;
        }
    } else {
        priority = (-0x1004 - ((act->y >> 8) << 2)) | 1;
        act->shadowPriority = 0xEFFF;
    }

    WorldToScreen(&x, &y, act->x, act->y, act->z);

    if (StepHitFlash(act)) {
        u16 flags = work->flags | BTL_SORA_FLAG_HIT_FLASH;

        work->flags = flags;
        LoadObjPaletteBank(work->palette->index, gHitFlashPalette);
    } else if (work->flags & BTL_SORA_FLAG_HIT_FLASH) {
        u16 flags = work->flags & ~BTL_SORA_FLAG_HIT_FLASH;

        work->flags = flags;

        if (work->mainSide) {
            LoadObjPaletteBank(work->palette->index, gSoraPalette);
        } else {
            LoadObjPaletteBank(work->palette->index, gBtlOtherSidePalette);
        }
    }

#ifdef VERSION_EU
    if (act->btl->hcEffect == HC_EFFECT_VANISH) {
        if (work->mainSide) {
            if (gFrameCounter & 1) {
                return;
            }
        } else if (gFrameCounter % 120 <= 59) {
            return;
        }
    }
#endif

    DrawSprite(x, y, work->gfx, work->tiles, work->palette, affine, attr, priority);
    TaskPoolDraw(&work->tasks);
}

void task_btl_sora_3(BtlSoraWork* work) {
    BtlObj* act;

    act = &work->actor;
    m4aSongNumStop(SONG_EF_DAMBO_SPLOOP);

    if (!(gBtlWork->flags & BTL_FLAG_VS_BATTLE)) {
        if (gBtlWork->phase == BTL_PHASE_GAME_OVER) {
            gGameState.hp = gGameState.progression.maxHp;
        } else {
            gGameState.hp = act->hp;
        }
    }

    ColliderUnregister(&act->collider);
    ReleaseBtlSoraPalette(work);
    TaskPoolDestroy(&work->tasks);
}
