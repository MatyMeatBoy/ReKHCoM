/**
 * btl_riku.c
 * Riku Battle Character
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
#include "sprites_evt.h"
#include "sprites_fld.h"
#include "sprites_hum.h"
#include "sprites_riku.h"
#include "gba/keys.h"
#include "gba/io_reg.h"
#include "system_state.h"
#include "player_progression.h"
#include "fade.h"
#include "songs.h"
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
#include "sio_api.h"

static const AnimDef sBtlRikuAnimDefs[35] = {
    { gRikuBt00Frames, gRikuBt00Anims, gRikuBt00Tiles, 0 },
    { gRikuBt11Frames, gRikuBt11Anims, gRikuBt11Tiles, 1 },
    { gRikuBt10Frames, gRikuBt10Anims, gRikuBt10Tiles, 3 },
    { gRikuBt11Frames, gRikuBt11Anims, gRikuBt11Tiles, 2 },
    { gRikuBt13Frames, gRikuBt13Anims, gRikuBt13Tiles, 0 },
    { gRikuBt11Frames, gRikuBt11Anims, gRikuBt11Tiles, 2 },
    { gRikuLl17Frames, gRikuLl17Anims, gRikuLl17Tiles, 1 },
    { gRikuBt02Frames, gRikuBt02Anims, gRikuBt02Tiles, 0 },
    { gRikuBt02Frames, gRikuBt02Anims, gRikuBt02Tiles, 1 },
    { gRikuBt04Frames, gRikuBt04Anims, gRikuBt04Tiles, 0 },
    { gRikuBt05Frames, gRikuBt05Anims, gRikuBt05Tiles, 0 },
    { gBtlRikuStartFrames, gBtlRikuStartAnims, gBtlRikuStartTiles, 0 },
    { gNiserikuIdolFrames, gNiserikuIdolAnims, gNiserikuIdolTiles, 0 },
    { gNiserikuRunFrames, gNiserikuRunAnims, gNiserikuRunTiles, 0 },
    { gNiserikuDamageFrames, gNiserikuDamageAnims, gNiserikuDamageTiles, 0 },
    { gNiserikuJumpFrames, gNiserikuJumpAnims, gNiserikuJumpTiles, 0 },
    { gNiserikuJumpFrames, gNiserikuJumpAnims, gNiserikuJumpTiles, 1 },
    { gNiserikuJumpFrames, gNiserikuJumpAnims, gNiserikuJumpTiles, 2 },
    { gNiserikuJumpFrames, gNiserikuJumpAnims, gNiserikuJumpTiles, 3 },
    { gNiserikuJumpFrames, gNiserikuJumpAnims, gNiserikuJumpTiles, 4 },
    { gNiserikuDashFrames, gNiserikuDashAnims, gNiserikuDashTiles, 0 },
    { gNiserikuFuriharaiFrames, gNiserikuFuriharaiAnims, gNiserikuFuriharaiTiles, 0 },
    { gNiserikuTategiriFrames, gNiserikuTategiriAnims, gNiserikuTategiriTiles, 0 },
    { gNiserikuKabutoFrames, gNiserikuKabutoAnims, gNiserikuKabutoTiles, 0 },
    { gNiserikuKabutoFrames, gNiserikuKabutoAnims, gNiserikuKabutoTiles, 2 },
    { gNiserikuDarkfigaFrames, gNiserikuDarkfigaAnims, gNiserikuDarkfigaTiles, 0 },
    { gNiserikuYamiStartFrames, gNiserikuYamiStartAnims, gNiserikuYamiStartTiles, 0 },
    { gNiserikuYamiStartFrames, gNiserikuYamiStartAnims, gNiserikuYamiStartTiles, 1 },
    { gNiserikuYamiStartFrames, gNiserikuYamiStartAnims, gNiserikuYamiStartTiles, 2 },
    { gNiserikuYamiIngFrames, gNiserikuYamiIngAnims, gNiserikuYamiIngTiles, 0 },
    { gNiserikuYamiIngFrames, gNiserikuYamiIngAnims, gNiserikuYamiIngTiles, 1 },
    { gNiserikuYamiIngFrames, gNiserikuYamiIngAnims, gNiserikuYamiIngTiles, 2 },
    { gNiserikuYamiEndFrames, gNiserikuYamiEndAnims, gNiserikuYamiEndTiles, 0 },
    { gNiserikuDashFrames, gNiserikuDashAnims, gNiserikuDashTiles, 1 },
    { gNiserikuDashFrames, gNiserikuDashAnims, gNiserikuDashTiles, 3 },
};

static const AnimDef sBtlRikuDirAnimDefs[6][5] = {
    { { gRik1ff02Frames, gRik1ff02Anims, gRik1ff02Tiles, 0 }, { gRik1bb02Frames, gRik1bb02Anims, gRik1bb02Tiles, 0 }, { gRik1fl02Frames, gRik1fl02Anims, gRik1fl02Tiles, 0 }, { gRik1ll02Frames, gRik1ll02Anims, gRik1ll02Tiles, 0 }, { gRik1bl02Frames, gRik1bl02Anims, gRik1bl02Tiles, 0 } },
    { { gRik1ff03Frames, gRik1ff03Anims, gRik1ff03Tiles, 0 }, { gRik1bb03Frames, gRik1bb03Anims, gRik1bb03Tiles, 0 }, { gRik1fl03Frames, gRik1fl03Anims, gRik1fl03Tiles, 0 }, { gRik1ll03Frames, gRik1ll03Anims, gRik1ll03Tiles, 0 }, { gRik1bl03Frames, gRik1bl03Anims, gRik1bl03Tiles, 0 } },
    { { gRik1ff03Frames, gRik1ff03Anims, gRik1ff03Tiles, 1 }, { gRik1bb03Frames, gRik1bb03Anims, gRik1bb03Tiles, 1 }, { gRik1fl03Frames, gRik1fl03Anims, gRik1fl03Tiles, 1 }, { gRik1ll03Frames, gRik1ll03Anims, gRik1ll03Tiles, 1 }, { gRik1bl03Frames, gRik1bl03Anims, gRik1bl03Tiles, 1 } },
    { { gRik1ff03Frames, gRik1ff03Anims, gRik1ff03Tiles, 2 }, { gRik1bb03Frames, gRik1bb03Anims, gRik1bb03Tiles, 2 }, { gRik1fl03Frames, gRik1fl03Anims, gRik1fl03Tiles, 2 }, { gRik1ll03Frames, gRik1ll03Anims, gRik1ll03Tiles, 2 }, { gRik1bl03Frames, gRik1bl03Anims, gRik1bl03Tiles, 2 } },
    { { gRik1ff03Frames, gRik1ff03Anims, gRik1ff03Tiles, 3 }, { gRik1bb03Frames, gRik1bb03Anims, gRik1bb03Tiles, 3 }, { gRik1fl03Frames, gRik1fl03Anims, gRik1fl03Tiles, 3 }, { gRik1ll03Frames, gRik1ll03Anims, gRik1ll03Tiles, 3 }, { gRik1bl03Frames, gRik1bl03Anims, gRik1bl03Tiles, 3 } },
    { { gRik1ff03Frames, gRik1ff03Anims, gRik1ff03Tiles, 4 }, { gRik1bb03Frames, gRik1bb03Anims, gRik1bb03Tiles, 4 }, { gRik1fl03Frames, gRik1fl03Anims, gRik1fl03Tiles, 4 }, { gRik1ll03Frames, gRik1ll03Anims, gRik1ll03Tiles, 4 }, { gRik1bl03Frames, gRik1bl03Anims, gRik1bl03Tiles, 4 } },
};

static const u16 sBtlRikuGroundSongs[4][4] = {
    { SONG_BTL_SR_FOOTL, SONG_BTL_SR_FOOTR, SONG_BTL_SR_JUMP, SONG_BTL_SR_LAND },
    { SONG_BTL_SR_STONEL, SONG_BTL_SR_STONER, SONG_BTL_SR_STONEJP, SONG_BTL_SR_STONELD },
    { SONG_BTL_SR_MUDL, SONG_BTL_SR_MUDR, SONG_BTL_SR_MUDJP, SONG_BTL_SR_MUDLD },
    { SONG_SYS_SR_STONEL, SONG_SYS_SR_STONER, SONG_SYS_SR_STONEJP, SONG_SYS_SR_STONELD },
};

static const s32 sBtlRikuAttackIds[3] = {
    0, 1, 2,
};

static const RikuAttackDef sBtlRikuSwing1 = { 1, 15, sBtlRikuAttackIds, SONG_VO_RK_ATTACK01, SONG_BTL_RK_HIT00, 0, 0, NULL };

static const RikuAttackDef sBtlRikuSwing2 = { 2, 17, &sBtlRikuAttackIds[1], SONG_VO_RK_ATTACK00, SONG_BTL_RK_HIT01, 0, 0, NULL };

static const RikuAttackDef sBtlRikuSwing1Wide = { 3, 15, sBtlRikuAttackIds, SONG_VO_RK_ATTACK03, SONG_BTL_RK_HIT00, 0, 0, NULL };

static const RikuAttackDef sBtlRikuSwing3 = { 4, 21, &sBtlRikuAttackIds[2], SONG_VO_RK_ATTACK06, SONG_BTL_RK_HIT02, 0, 0, NULL };

static const RikuAttackDef sBtlRikuAirSwing1Hop = { 6, 15, sBtlRikuAttackIds, SONG_VO_RK_ATTACK03, SONG_BTL_RK_HIT01, -640, COMBO_FLAG_AERIAL_SWING, &sBtlRikuSwing1 };

static const RikuAttackDef sBtlRikuAirSwing1 = { 5, 15, &sBtlRikuAttackIds[1], SONG_VO_RK_ATTACK00, SONG_BTL_RK_HIT00, 0, COMBO_FLAG_AERIAL_SWING, &sBtlRikuSwing1 };

static const RikuAttackDef sBtlRikuAirSwing2 = { 5, 15, sBtlRikuAttackIds, SONG_VO_RK_ATTACK01, SONG_BTL_RK_HIT01, 0, COMBO_FLAG_AERIAL_SWING, &sBtlRikuSwing2 };

static const RikuAttackDef sBtlRikuAirSwing3 = { 6, 15, &sBtlRikuAttackIds[2], SONG_VO_RK_ATTACK05, SONG_BTL_RK_HIT02, 0, COMBO_FLAG_AERIAL_SWING, &sBtlRikuSwing3 };

void EnableBtlRikuPassThrough(BtlRikuWork* work) {
    u16 flags = work->flags | BTL_RIKU_FLAG_PASS_THROUGH;
    u16 colliderFlags;

    work->flags = flags;
    colliderFlags = work->actor.collider.flags | COLLIDER_FLAG_PASS_THROUGH;
    work->actor.collider.flags = colliderFlags;
}

void DisableBtlRikuPassThrough(BtlRikuWork* work) {
    u16 flags = work->flags & ~BTL_RIKU_FLAG_PASS_THROUGH;
    u16 colliderFlags;

    work->flags = flags;
    colliderFlags = work->actor.collider.flags & ~COLLIDER_FLAG_PASS_THROUGH;
    work->actor.collider.flags = colliderFlags;
}

u16 GetBtlRikuComboType(BtlRikuWork* work) {
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

void FocusBtlRikuCameraOnTarget(BtlRikuWork* work) {
    BtlObj* target;

    if (!work->mainSide) {
        return;
    }

    target = work->actor.btl->actor2;

    if (target != NULL) {
        BtlMapFollowPosition((work->actor.x + target->x) >> 1, (work->actor.y + target->y) >> 1,
                      (work->actor.z + target->z) >> 1);
    } else {
        BtlMapFollowPosition(work->actor.x, work->actor.y, work->actor.z);
    }
}

void FocusBtlRikuCameraOnBgFx(BtlRikuWork* work) {
    s32 x;
    s32 y;
    s32 z;

    if (work->mainSide) {
        BgFxGetPosition(&x, &y, &z);
        BtlMapFollowPosition(x, gBtlWork->actor->y, gBtlWork->actor->z);
    }
}

void SaveBtlRikuAfterimage(BtlRikuWork* work, BtlDrawInfo* out) {
    BtlObj* act;

    act = &work->actor;
    out->x = act->x;
    out->y = act->y;
    out->z = act->z;

    if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
        out->flags |= BTL_DRAW_INFO_FLAG_FACING_LEFT;
    } else {
        out->flags &= ~BTL_DRAW_INFO_FLAG_FACING_LEFT;
    }

    out->anim = work->anim;
    out->tileSrc = work->tiles2->src;
    out->scale = gBtlWork->scale;
}

void DrawBtlRikuAfterimage(BtlRikuWork* work, BtlDrawInfo* out) {
    BtlObj* act;
    void* gfx;
    u16 flags;
    ObjAffine* affine;
    s32 sy;
    s32 sx;
    s16 x;
    s16 y;
    s32 priority;
    s32 scale;

    gfx = AnimGetGfx(&out->anim);
    act = &work->actor;

    if (!BgFxIsActive()) {
        gBldCnt = (BLDCNT_TGT1_OBJ | BLDCNT_TGT2_BG0 | BLDCNT_TGT2_BG1 | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_BG3);
        SetBlendAlpha(6, 12);
        flags = SPRITE_PRIORITY(2) | SPRITE_FLAG_BLEND;
    } else {
        flags = GetBattleSpritePriorityFlags(act->y);
    }

    if (out->flags & BTL_DRAW_INFO_FLAG_FACING_LEFT) {
        sy = out->scale;
        sx = sy;
    } else {
        sy = out->scale;

        if (sy == Q_8_8(1)) {
            sx = sy;
            flags |= SPRITE_FLAG_HFLIP;
        } else {
            scale = gBtlWork->scale;
            sx = -scale;
            sy = scale;
        }
    }

    if (sy == Q_8_8(1) && sx == sy) {
        affine = NULL;
    } else if (sy <= 255) {
        affine = AllocObjAffine(0, sx, sy, FALSE);
    } else {
        affine = AllocObjAffine(0, sx, sy, TRUE);
    }

    priority = 0xFFF0;
    WorldToScreen(&x, &y, out->x, out->y, out->z);
    SetObjTileSource(work->tiles, out->tileSrc);
    DrawSprite(x, y, gfx, work->tiles, work->palette, affine, flags, priority);
}

void SetBtlRikuAnimation(BtlRikuWork* work, u16 index, u16 flags) {
    const FldAnimDef* def;

    def = &sBtlRikuAnimDefs[index];
    AnimChangeWithTables(&work->anim, def->animId, flags, def->anims, def->gfxTable);
    SetObjTileSource(work->tiles2, def->tiles);
}

void SetBtlRikuDirAnimation(BtlRikuWork* work, u16 index, u16 flags) {
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

    def = &sBtlRikuDirAnimDefs[index][dir];
    AnimChangeWithTables(&work->anim, def->animId, flags, def->anims, def->gfxTable);
    SetObjTileSource(work->tiles2, def->tiles);
}

void LoadBtlRikuPalette(BtlRikuWork* work) {
    work->tiles2 = work->actor.btl->tiles;

    if (work->mainSide) {
        work->palette = LoadObjPalette(work->paletteData, 0x20);
    } else {
        work->palette = LoadObjPalette(gBtlOtherSidePalette, sizeof(gBtlOtherSidePalette));
    }
}

void ReleaseBtlRikuPalette(BtlRikuWork* work) {
    ReleaseObjPalette(work->palette);
    work->tiles2 = NULL;
    work->palette = NULL;
}

void UpdateBtlRikuWalk(BtlRikuWork* work, u16 held) {
    BtlObj* act;

    act = &work->actor;

    if ((held & 0x10) && (held & 0x40)) {
        work->angle = 0x20;
        act->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
        act->vx += 25;
        act->vy -= 12;
    } else if ((held & 0x10) && (held & 0x80)) {
        work->angle = 0x60;
        act->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
        act->vx += 25;
        act->vy += 12;
    } else if ((held & 0x20) && (held & 0x80)) {
        work->angle = 0xA0;
        act->flags |= BTLOBJ_FLAG_FACING_LEFT;
        act->vx -= 25;
        act->vy += 12;
    } else if ((held & 0x20) && (held & 0x40)) {
        work->angle = 0xE0;
        act->flags |= BTLOBJ_FLAG_FACING_LEFT;
        act->vx -= 25;
        act->vy -= 12;
    } else if (held & 0x40) {
        work->angle = 0;
        act->vy -= 12;
    } else if (held & 0x10) {
        work->angle = 0x40;
        act->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
        act->vx += 25;
    } else if (held & 0x80) {
        work->angle = 0x80;
        act->vy += 12;
    } else if (held & 0x20) {
        work->angle = 0xC0;
        act->flags |= BTLOBJ_FLAG_FACING_LEFT;
        act->vx -= 25;
    }

    if (act->vx > 512) {
        act->vx = 512;
    } else if (act->vx < -512) {
        act->vx = -512;
    }

    if (act->vy > 256) {
        act->vy = 256;
    } else if (act->vy < -256) {
        act->vy = -256;
    }

    if (held & 0xF0) {
        SetBtlRikuDirAnimation(work, 0, 1);

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
        SetBtlRikuAnimation(work, 0, 1);
    }

    if (held & 0xF0) {
        if (act->btl->hcEffect == HC_EFFECT_DASH) {
            work->speed += 256;

            if (work->speed > 768) {
                work->speed = 768;
            }
        } else {
            work->speed += 128;

            if (work->speed > 512) {
                work->speed = 512;
            }
        }
    } else {
        work->speed -= 128;

        if (work->speed < 0) {
            work->speed = 0;
        }
    }
}

void UpdateBtlRikuDarkWalk(BtlRikuWork* work, u16 held) {
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
        SetBtlRikuAnimation(work, 13, 1);

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
        SetBtlRikuAnimation(work, 12, 1);
    }

    if (held & 0xF0) {
        if (act->btl->hcEffect == HC_EFFECT_DASH) {
            work->speed += 256;

            if (work->speed > 768) {
                work->speed = 768;
            }
        } else {
            work->speed += 128;

            if (work->speed > 640) {
                work->speed = 640;
            }
        }
    } else {
        work->speed -= 128;

        if (work->speed < 0) {
            work->speed = 0;
        }
    }
}

enum BtlRikuState {
    BTL_RIKU_STATE_ENTER,
    BTL_RIKU_STATE_IDLE,
    BTL_RIKU_STATE_JUMP,
    BTL_RIKU_STATE_AIRBORNE,
    BTL_RIKU_STATE_LAND,
    BTL_RIKU_STATE_ITEM,
    BTL_RIKU_STATE_HURT,
    BTL_RIKU_STATE_DEFEATED,
    BTL_RIKU_STATE_ESCAPE,
    BTL_RIKU_STATE_COMBO,
    BTL_RIKU_STATE_ITEM_POSE,
    BTL_RIKU_STATE_CARD_BROKEN,
    BTL_RIKU_STATE_GUARDED,
    BTL_RIKU_STATE_STUNNED,
    BTL_RIKU_STATE_FIELD_HIDDEN,
    BTL_RIKU_STATE_HAZARD,
    BTL_RIKU_STATE_DARK_HAZARD,
    BTL_RIKU_STATE_STOPPED,
    BTL_RIKU_STATE_RECOVER,
    BTL_RIKU_STATE_DARK_RECOVER,
    BTL_RIKU_STATE_REVIVE,
    BTL_RIKU_STATE_END_BATTLE,
    BTL_RIKU_STATE_DODGE,
    BTL_RIKU_STATE_DODGE_LAND,
    BTL_RIKU_STATE_SUMMON_TAKEOFF,
    BTL_RIKU_STATE_SUMMON_EXIT,
    BTL_RIKU_STATE_SUMMON_OFFSCREEN,
    BTL_RIKU_STATE_SUMMON_RETURN,
    BTL_RIKU_STATE_SUMMON_RETURN_LAND,
    BTL_RIKU_STATE_FROZEN = 30,
    BTL_RIKU_STATE_DARK_MODE_START,
    BTL_RIKU_STATE_DARK_MODE_TRANSFORM,
    BTL_RIKU_STATE_DARK_MODE_END,
    BTL_RIKU_STATE_DARK_MODE_REVERT,
    BTL_RIKU_STATE_DARK_IDLE,
    BTL_RIKU_STATE_DARK_HURT,
    BTL_RIKU_STATE_DARK_JUMP,
    BTL_RIKU_STATE_DARK_AIRBORNE,
    BTL_RIKU_STATE_DARK_LAND,
    BTL_RIKU_STATE_DARK_DOUBLE_SLASH,
    BTL_RIKU_STATE_DARK_AIR_SLASH,
    BTL_RIKU_STATE_DARK_VERTICAL_SLASH,
    BTL_RIKU_STATE_DARK_BREAK,
    BTL_RIKU_STATE_DARK_BREAK_RISE,
    BTL_RIKU_STATE_DARK_BREAK_DIVE,
    BTL_RIKU_STATE_DARK_BREAK_REBOUND,
    BTL_RIKU_STATE_DARK_BREAK_END,
    BTL_RIKU_STATE_DARK_FIRAGA,
    BTL_RIKU_STATE_DARK_AURA,
    BTL_RIKU_STATE_DARK_AURA_JUMP,
    BTL_RIKU_STATE_DARK_AURA_EXIT,
    BTL_RIKU_STATE_DARK_AURA_ENTRY,
    BTL_RIKU_STATE_DARK_AURA_DASH,
    BTL_RIKU_STATE_DARK_AURA_FINISH,
    BTL_RIKU_STATE_DARK_CARD_BROKEN,
    BTL_RIKU_STATE_DARK_LEAP,
    BTL_RIKU_STATE_DARK_DASH,
    BTL_RIKU_STATE_DARK_DASH_VERTICAL,
    BTL_RIKU_STATE_DARK_DASH_LAND,
    BTL_RIKU_STATE_DARK_SLASH,
    BTL_RIKU_STATE_DARK_LUNGE_SLASH,
    BTL_RIKU_STATE_DARK_STUNNED
};

void task_btl_riku_0(BtlRikuWork* work, BtlTaskArg* arg) {
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
        } else {
            work->mainSide = FALSE;
            act->btl = gRikuBtlWork;
        }

        act->maxHp = 1000;
        act->hp = 1000;
        act->attack = 10;
    } else {
        work->mainSide = TRUE;
        work->sioKeysA = TRUE;
        act->btl = gBtlWork;

        if (act->btl->flags & BTL_FLAG_HUM_BATTLE) {
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

    act->flags |= (BTLOBJ_FLAG_IMMUNE_GRAVITY | BTLOBJ_FLAG_IMMUNE_WARP);
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
    act->height = 40;
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
    work->paletteData = gRikuPalette;
    work->tiles = AllocObjTiles(0x640, NULL);
    LoadBtlRikuPalette(work);
    act->btl->actor = act;
    AnimInit(&work->anim, NULL, NULL);
    SetBtlRikuAnimation(work, 0, 1);
    work->gfx = AnimGetGfx(&work->anim);
    work->state = BTL_RIKU_STATE_ENTER;
    work->nextState = BTL_RIKU_STATE_ENTER;
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
    work->task = 0;
    work->scaleX = work->scaleY = Q_8_8(1);
    work->frameCount = 0;

    if (gBtlWork->flags & (BTL_FLAG_BOSS_BATTLE | BTL_FLAG_HUM_BATTLE)) {
        switch (gBtlWork->battleId) {
        case BATTLE_JAFAR:
        case BATTLE_URSULA:
        case BATTLE_DRAGON_MALEFICENT:
        case BATTLE_DARKSIDE:
        case BATTLE_MARLUXIA_2:
        case BATTLE_LEON:
            work->groundSongs = sBtlRikuGroundSongs[0];
            break;
        case BATTLE_GUARD_ARMOR:
        case BATTLE_TRICKMASTER:
        case BATTLE_OOGIE_BOOGIE:
            work->groundSongs = sBtlRikuGroundSongs[1];
            break;
        case BATTLE_PARASITE_CAGE:
            work->groundSongs = sBtlRikuGroundSongs[2];
            break;
        case BATTLE_HOOK:
            work->groundSongs = sBtlRikuGroundSongs[3];
            break;
        default:
            work->groundSongs = sBtlRikuGroundSongs[0];
            break;
        }
    } else {
        switch (gGameState.battleStage) {
        case BATTLE_STAGE_WONDERLAND:
        case BATTLE_STAGE_GARDEN:
            work->groundSongs = sBtlRikuGroundSongs[0];
            break;
        case BATTLE_STAGE_AGRABAH:
        case BATTLE_STAGE_OLYMPUS_COLISEUM:
        case BATTLE_STAGE_HALLOWEEN_TOWN:
            work->groundSongs = sBtlRikuGroundSongs[1];
            break;
        case BATTLE_STAGE_ATLANTICA:
        case BATTLE_STAGE_MONSTRO:
            work->groundSongs = sBtlRikuGroundSongs[2];
            break;
        default:
            work->groundSongs = sBtlRikuGroundSongs[0];
            break;
        }
    }

    TaskPoolInit(&work->tasks, 7);
    TaskCreate(&work->tasks, &gTaskDescBtlShadow, act);
    TaskCreate(&work->tasks, &gTaskDescBtlBadstatus, act);
    work->drawCount = 0;
    SaveBtlRikuAfterimage(work, &work->drawInfo[0]);
    work->drawInfo[1] = work->drawInfo[0];
    work->drawInfo[2] = work->drawInfo[0];
    work->drawInfo[3] = work->drawInfo[0];
    work->drawInfo[4] = work->drawInfo[0];
    work->drawInfo[5] = work->drawInfo[0];
    work->drawInfo[6] = work->drawInfo[0];
    work->drawInfo[7] = work->drawInfo[0];
    work->drawInfo[8] = work->drawInfo[0];
}

void SetBtlRikuState(BtlRikuWork* work, u32 state) {
    work->state = state;
    work->steps = 0;
    work->stateTimer = 0;
    ClearBtlObjActionFlags(&work->actor);
}

void StartBtlRikuCombo(BtlRikuWork* work) {
    u16 flags;

    if (work->state == BTL_RIKU_STATE_COMBO && work->comboCount <= 1) {
        work->comboCount++;
        work->stateTimer = 0;
        work->steps = 0;
    } else {
        switch (GetBtlRikuComboType(work)) {
        case COMBO_TYPE_NORMAL:
            work->attacks[0] = &sBtlRikuSwing1;
            work->attacks[1] = &sBtlRikuSwing2;
            work->attacks[2] = &sBtlRikuSwing3;
            break;
        case COMBO_TYPE_TARGET_FAR_X:
            work->attacks[0] = &sBtlRikuSwing2;
            work->attacks[1] = &sBtlRikuSwing1;
            work->attacks[2] = &sBtlRikuSwing3;
            break;
        case COMBO_TYPE_TARGET_ABOVE:
            work->attacks[0] = &sBtlRikuAirSwing1;
            work->attacks[1] = &sBtlRikuAirSwing2;
            work->attacks[2] = &sBtlRikuAirSwing3;
            break;
        case COMBO_TYPE_AIRBORNE:
            work->attacks[0] = &sBtlRikuAirSwing1Hop;
            work->attacks[1] = &sBtlRikuAirSwing1;
            work->attacks[2] = &sBtlRikuAirSwing3;
            break;
        case COMBO_TYPE_TARGET_FAR_Y:
        default:
            work->attacks[0] = &sBtlRikuSwing1Wide;
            work->attacks[1] = &sBtlRikuSwing2;
            work->attacks[2] = &sBtlRikuSwing3;
            break;
        }

        work->state = BTL_RIKU_STATE_COMBO;
        work->steps = 0;
        work->stateTimer = 0;
        work->comboCount = 0;
        flags = work->flags & ~BTL_RIKU_FLAG_COMBO_EXTENDED;
        work->flags = flags;
    }
}

void StartBtlRikuKnockback(BtlRikuWork* work) {
    work->vz = -work->actor.knockbackLift * 3;
    work->actor.vx = ((gSineTable[work->actor.angle] << 1) * work->actor.knockbackSpeed) >> 8;
    work->actor.vy = ((-gSineTable[work->actor.angle + 0x40] << 1) * work->actor.knockbackSpeed) >> 8;
}

BtlObj* GetBtlRikuActiveOpponent(BtlRikuWork* work) {
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

BtlObj* FindHighestEnemy(BtlRikuWork* work) {
    BtlObj* enemy;
    BtlObj* best;
    s32 centerZ;
    s32 min;

    min = 0x10000;
    best = NULL;
    enemy = ListPoolFirst(&gBtlWork->pool);

    while (enemy != NULL) {
        if (!(enemy->flags & BTLOBJ_FLAG_UNHITTABLE)) {
            centerZ = enemy->z - (enemy->centerHeight << 8);

            if (min > centerZ) {
                best = enemy;
                min = centerZ;
            }
        }

        enemy = ListPoolNext(&enemy->node);
    }

    return best;
}

BtlObj* PickBtlRikuTarget(BtlRikuWork* work) {
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

u16 SwapBtlRikuKeyBits(u16 keys, u16 bitA, u16 bitB) {
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

void EndRikuDarkMode(BtlRikuWork* work) {
    if (gBtlWork->flags & BTL_FLAG_DARK_MODE) {
        work->paletteData = gRikuPalette;
        LoadObjPaletteBank(work->palette->index, gRikuPalette);
        gBtlWork->flags &= ~BTL_FLAG_DARK_MODE;
        gBtlWork->flags |= BTL_FLAG_DARK_MODE_CHANGED;
    }
}

void AddDarkPoints(s16 points) {
    if (gGameState.flags & GAME_FLAG_DARK_POINTS_LOCKED) {
        return;
    }

    gBtlWork->darkPoints += points;

    if (gBtlWork->darkPoints < 0) {
        gBtlWork->darkPoints = 0;
    } else if (gBtlWork->darkPoints > 999) {
        gBtlWork->darkPoints = 999;
    }
}

s32 task_btl_riku_1(BtlRikuWork* work) {
    BtlObj* act;
    BtlObj* enemy;
    u16 held;
    u16 pressed;
    u16 elapsed;
    s16 heal;
    s32 vx;
    s32 vy;
    s32 tx;
    s32 ty;
    s32 tz;
    s32 ahead;
    s16 dx;
    s16 step;
    s16 dz;
    s32 mode;
    s32 mode2;
    u16 frame;
    s32 ex;
    s32 ey;
    s32 oa;
    s32 ob;
    s32 oc;
    s16 od;
    s32 climb;
    s32 hit;
    s32 targetX;
    s32 t5;
    const RikuAttackDef* swing;
    BtlSpawnArgs spawn;
    u32 move;
    s32 stockMoves[6];

    act = &work->actor;

    if (gBtlWork->phase == BTL_PHASE_END && (act->flags & BTLOBJ_FLAG_IN_CARD_ACTION)) {
        switch (work->state) {
        case BTL_RIKU_STATE_SUMMON_TAKEOFF:
        case BTL_RIKU_STATE_SUMMON_EXIT:
        case BTL_RIKU_STATE_SUMMON_OFFSCREEN:
        case BTL_RIKU_STATE_SUMMON_RETURN:
            act->x = act->originX;
            act->y = act->originY;
            act->z = act->originZ;

#ifndef VERSION_EU
            act->flags &= ~BTLOBJ_FLAG_NO_BREAK_POP;
            CreateBtlPopTask(act, BTL_POP_CARD_BREAK);
#endif

            if (work->flags & BTL_RIKU_FLAG_HIDDEN) {
                work->flags &= ~BTL_RIKU_FLAG_HIDDEN;
                LoadBtlRikuPalette(work);
            }

            if (gBtlWork->flags & BTL_FLAG_DARK_MODE) {
                SetBtlRikuState(work, BTL_RIKU_STATE_DARK_AIRBORNE);
            } else {
                SetBtlRikuState(work, BTL_RIKU_STATE_AIRBORNE);
            }

            break;
        default:
            if (gBtlWork->flags & BTL_FLAG_DARK_MODE) {
                SetBtlRikuState(work, BTL_RIKU_STATE_DARK_RECOVER);
            } else {
                SetBtlRikuState(work, BTL_RIKU_STATE_RECOVER);
            }

            break;
        }

        work->scaleX = work->scaleY = Q_8_8(1);
        ColliderSetDisabled(&act->collider, FALSE);
        DisableBtlRikuPassThrough(work);
        act->flags &= 0xFFFFDFFBFF7FFFFFLL;
        work->speed = 0;
        gBtlWork->flags |= BTL_FLAG_STOP_BGFX;
        act->btl->flags |= BTL_FLAG_DISMISS_SUMMONS;
    }

    if (CanLevelUp() && LevelUp()) {
        CreateLevelUpEffectTask(act, &work->tasks);
    }

    if (work->flags & BTL_RIKU_FLAG_HC_STATUS) {
        work->flags &= ~BTL_RIKU_FLAG_HC_STATUS;
        act->flags &= ~BTLOBJ_FLAGS_ELEMENT_AFFINITY;
    }

    switch (act->btl->hcEffect) {
    case HC_EFFECT_SECOND_CHANCE:
        work->flags |= BTL_RIKU_FLAG_HC_STATUS;
        act->flags |= (BTLOBJ_FLAG_WEAK_FIRE | BTLOBJ_FLAG_RESIST_THUNDER);
        break;
    case HC_EFFECT_BERSERK:
        work->flags |= BTL_RIKU_FLAG_HC_STATUS;
        act->flags |= (BTLOBJ_FLAG_WEAK_BLIZZARD | BTLOBJ_FLAG_RESIST_FIRE);
        break;
    case HC_EFFECT_SLEIGHT_LOCK:
        work->flags |= BTL_RIKU_FLAG_HC_STATUS;
        act->flags |= (BTLOBJ_FLAG_RESIST_FIRE | BTLOBJ_FLAG_RESIST_BLIZZARD | BTLOBJ_FLAG_RESIST_THUNDER);
        break;
    case HC_EFFECT_QUICK_RECOVERY:
        work->flags |= BTL_RIKU_FLAG_HC_STATUS;
        act->flags |= (BTLOBJ_FLAG_IMMUNE_FIRE | BTLOBJ_FLAG_WEAK_BLIZZARD);
        break;
    case HC_EFFECT_DASH:
        work->flags |= BTL_RIKU_FLAG_HC_STATUS;
        act->flags |= (BTLOBJ_FLAG_IMMUNE_THUNDER | BTLOBJ_FLAG_WEAK_NEUTRAL);
        break;
    case HC_EFFECT_AUTO_LIFE:
        work->flags |= BTL_RIKU_FLAG_HC_STATUS;
        act->flags |= (BTLOBJ_FLAG_IMMUNE_BLIZZARD | BTLOBJ_FLAG_WEAK_FIRE);
        break;
    case HC_EFFECT_DOUBLE_SLEIGHT:
        work->flags |= BTL_RIKU_FLAG_HC_STATUS;
        act->flags |= (BTLOBJ_FLAG_WEAK_PHYSICAL | BTLOBJ_FLAG_RESIST_FIRE | BTLOBJ_FLAG_RESIST_BLIZZARD | BTLOBJ_FLAG_RESIST_THUNDER | BTLOBJ_FLAG_RESIST_NEUTRAL);
        break;
    case HC_EFFECT_WARP_BREAK:
        work->flags |= BTL_RIKU_FLAG_HC_STATUS;
        act->flags |= (BTLOBJ_FLAG_IMMUNE_BLIZZARD | BTLOBJ_FLAG_RESIST_PHYSICAL | BTLOBJ_FLAG_WEAK_NEUTRAL);
        break;
    case HC_EFFECT_SLEIGHTBLIND:
        work->flags |= BTL_RIKU_FLAG_HC_STATUS;
        act->flags |= (BTLOBJ_FLAG_RESIST_FIRE | BTLOBJ_FLAG_RESIST_BLIZZARD | BTLOBJ_FLAG_RESIST_THUNDER);
        break;
    }

    act->flags &= ~BTLOBJ_FLAG_GUARD_PHYSICAL;

    switch ((u32)act->btl->hcEffect) {
    case HC_EFFECT_GUARD:
        enemy = GetBtlRikuActiveOpponent(work);

        if (enemy != NULL) {
            if ((enemy->x < act->x && (act->flags & BTLOBJ_FLAG_FACING_LEFT)) ||
                (enemy->x > act->x && !(act->flags & BTLOBJ_FLAG_FACING_LEFT))) {
                act->flags |= BTLOBJ_FLAG_GUARD_PHYSICAL;
            }
        }

        break;
    case HC_EFFECT_REGEN: {
        u16 hp;
        u16 maxHp;
        s32 newHp;

        hp = act->hp;

        if ((s16)hp > 0 && (s16)hp < (s16)(maxHp = act->maxHp) && work->frameCount % 120 == 0) {
            heal = (act->maxHp - act->hp) << 13 >> 16;

            if (heal <= 0) {
                heal = 1;
            }

            newHp = heal + hp;
            act->hp = newHp;

            if ((s16)newHp > (s16)maxHp) {
                act->hp = maxHp;
            }

            act->btl->hcEffectCount--;
        }

        break;
    }
    case HC_EFFECT_BIO:
        if (work->frameCount % 180 == 0) {
            if (gBtlWork->flags & BTL_FLAG_VS_BATTLE) {
                BtlObj* enemy;
                u16 hp;

                if (work->mainSide) {
                    enemy = gRikuBtlWork->actor;
                } else {
                    enemy = gBtlWork->actor;
                }

                hp = enemy->hp;

                if (enemy->hp > 1) {
                    enemy->hp = hp - 1;
                }
            } else {
                BtlObj* enemy;
                u16 hp;

                enemy = ListPoolFirst(&gBtlWork->pool);

                while (enemy != NULL) {
                    hp = enemy->hp;

                    if (enemy->hp > 1) {
                        enemy->hp = hp - 1;
                    }

                    enemy = ListPoolNext(&enemy->node);
                }
            }
        }

        break;
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

    if (act->badStatus == BAD_STATUS_CONFUSE) {
        held = SwapBtlRikuKeyBits(held, 0x20, 0x10);
        held = SwapBtlRikuKeyBits(held, 0x40, 0x80);
        pressed = SwapBtlRikuKeyBits(pressed, 0x20, 0x10);
        pressed = SwapBtlRikuKeyBits(pressed, 0x40, 0x80);
    }

    if (work->state != BTL_RIKU_STATE_ESCAPE && (act->btl->flags & BTL_FLAG_ESCAPED)) {
        act->flags |= BTLOBJ_FLAG_IGNORE_BOUNDS;
        work->state = BTL_RIKU_STATE_ESCAPE;
        work->steps = 0;
        work->stateTimer = 0;
        act->flags |= BTLOBJ_FLAG_INTANGIBLE;
    } else {
        switch (UpdateBtlObjReaction(act)) {
        case BTL_REACTION_HURT:
        case BTL_REACTION_GRAVITY:
            work->speed = 0;

            if (gBtlWork->flags & BTL_FLAG_DARK_MODE) {
                AddDarkPoints(-5);
                work->state = BTL_RIKU_STATE_DARK_HURT;
                work->steps = 0;
                work->stateTimer = 0;
            } else {
                AddDarkPoints(1);
                work->state = BTL_RIKU_STATE_HURT;
                work->steps = 0;
                work->stateTimer = 0;
            }

            break;
        case BTL_REACTION_DEFEATED:
        case BTL_REACTION_GRAVITY_DEFEATED:
            EndRikuDarkMode(work);
            work->speed = 0;

            if (act->btl->hcEffect == HC_EFFECT_AUTO_LIFE) {
                work->state = BTL_RIKU_STATE_REVIVE;
                work->steps = 0;
                work->stateTimer = 0;
            } else {
                work->state = BTL_RIKU_STATE_DEFEATED;
                work->steps = 0;
                work->stateTimer = 0;
            }

            break;
        case BTL_REACTION_HEALED:
            act->flags &= ~BTLOBJ_FLAG_HURT;
            work->state = BTL_RIKU_STATE_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
            act->flags &= ~BTLOBJ_FLAG_CARD_ACTION_PENDING;
            break;
        case BTL_REACTION_CARD_ACTION: {
            BtlObj* lockon;

            DisableBtlRikuPassThrough(work);
            work->stateTimer = 0;
            act->btl->flags &= ~BTL_FLAG_DISMISS_SUMMONS;
            act->btl->flags &= ~BTL_FLAG_PUSHING_EDGE;
            work->flags |= BTL_RIKU_FLAG_PASS_THROUGH;
            move = ResolveActiveCardsMove(stockMoves);

            if (move == MOVE_STOCK_SEQUENCE) {
                if (!(act->btl->flags & BTL_FLAG_STOCK_SEQUENCE)) {
                    act->btl->flags |= BTL_FLAG_STOCK_SEQUENCE;
                    act->btl->stockMove = 0;
                }

                move = stockMoves[act->btl->stockMove];
                act->btl->stockMove++;
            }

            switch (move) {
            case MOVE_HI_POTION:
                work->state = BTL_RIKU_STATE_ITEM;
                work->steps = 0;
                work->stateTimer = 0;
                work->variant[0] = BTL_ITEM_HI_POTION;
                break;
            case MOVE_POTION:
                work->state = BTL_RIKU_STATE_ITEM;
                work->steps = 0;
                work->stateTimer = 0;
                work->variant[0] = BTL_ITEM_POTION;
                break;
            case MOVE_MEGA_POTION:
                work->state = BTL_RIKU_STATE_ITEM;
                work->steps = 0;
                work->stateTimer = 0;
                work->variant[0] = BTL_ITEM_MEGA_POTION;
                break;
            case MOVE_ETHER:
                work->state = BTL_RIKU_STATE_ITEM;
                work->steps = 0;
                work->stateTimer = 0;
                work->variant[0] = BTL_ITEM_ETHER;
                break;
            case MOVE_MEGA_ETHER:
                work->state = BTL_RIKU_STATE_ITEM;
                work->steps = 0;
                work->stateTimer = 0;
                work->variant[0] = BTL_ITEM_MEGA_ETHER;
                break;
            case MOVE_ELIXIR:
                work->state = BTL_RIKU_STATE_ITEM;
                work->steps = 0;
                work->stateTimer = 0;
                work->variant[0] = BTL_ITEM_ELIXIR;
                break;
            case MOVE_MEGALIXIR:
                work->state = BTL_RIKU_STATE_ITEM;
                work->steps = 0;
                work->stateTimer = 0;
                work->variant[0] = BTL_ITEM_MEGALIXIR;
                break;
            case MOVE_THE_KING:
                work->state = BTL_RIKU_STATE_SUMMON_TAKEOFF;
                work->steps = 0;
                work->stateTimer = 0;
                work->summonDesc = &gTaskDescSmnKing;
                work->variant[0] = SUMMON_LEVEL_SINGLE;
                break;
            case MOVE_MM_MIRACLE_PAIR:
                work->state = BTL_RIKU_STATE_SUMMON_TAKEOFF;
                work->steps = 0;
                work->stateTimer = 0;
                work->summonDesc = &gTaskDescSmnKing;
                work->variant[0] = SUMMON_LEVEL_PAIR;
                break;
            case MOVE_MM_MIRACLE:
                work->state = BTL_RIKU_STATE_SUMMON_TAKEOFF;
                work->steps = 0;
                work->stateTimer = 0;
                work->summonDesc = &gTaskDescSmnKing;
                work->variant[0] = SUMMON_LEVEL_TRIPLE;
                break;
            case MOVE_DARK_BREAK:
                work->state = BTL_RIKU_STATE_DARK_BREAK;
                break;
            case MOVE_DARK_FIRAGA:
                work->state = BTL_RIKU_STATE_DARK_FIRAGA;
                break;
            case MOVE_DARK_AURA:
                work->state = BTL_RIKU_STATE_DARK_AURA;
                break;
            case MOVE_DARK_MODE:
                work->state = BTL_RIKU_STATE_DARK_MODE_START;
                break;
            case MOVE_SOUL_EATER:
                if (gBtlWork->flags & BTL_FLAG_DARK_MODE) {
                    switch (work->state) {
                    case BTL_RIKU_STATE_DARK_LEAP:
                        work->state = BTL_RIKU_STATE_DARK_VERTICAL_SLASH;
                        work->steps = 0;
                        work->stateTimer = 0;
                        break;
                    case BTL_RIKU_STATE_DARK_JUMP:
                    case BTL_RIKU_STATE_DARK_AIRBORNE:
                        work->comboCount = 0;
                        work->state = BTL_RIKU_STATE_DARK_AIR_SLASH;
                        work->steps = 0;
                        work->stateTimer = 0;
                        break;
                    case BTL_RIKU_STATE_DARK_AIR_SLASH:
                        work->comboCount++;
                        work->state = BTL_RIKU_STATE_DARK_AIR_SLASH;
                        work->steps = 0;
                        work->stateTimer = 0;
                        break;
                    case BTL_RIKU_STATE_DARK_SLASH:
                        work->state = BTL_RIKU_STATE_DARK_LUNGE_SLASH;
                        work->steps = 0;
                        work->stateTimer = 0;
                        break;
                    case BTL_RIKU_STATE_DARK_LUNGE_SLASH:
                        work->state = BTL_RIKU_STATE_DARK_VERTICAL_SLASH;
                        work->steps = 0;
                        work->stateTimer = 0;
                        break;
                    default:
                        work->state = BTL_RIKU_STATE_DARK_SLASH;
                        work->steps = 0;
                        work->stateTimer = 0;
                        break;
                    }
                } else {
                    StartBtlRikuCombo(work);
                }

                break;
            default:
                if (gBtlWork->flags & BTL_FLAG_DARK_MODE) {
                    SetBtlRikuState(work, BTL_RIKU_STATE_DARK_IDLE);
                } else {
                    SetBtlRikuState(work, BTL_RIKU_STATE_IDLE);
                }

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
        }
        case BTL_REACTION_HAZARD:
            if ((gBtlWork->flags & BTL_FLAG_OPPONENT_CARD_ACTION) == 0) {
                gBtlWork->flags |= BTL_FLAG_STOP_BGFX;
            }

            SetBattleZoom(12, Q_8_8(1), gBtlWork->x2, gBtlWork->y2);
            ColliderSetDisabled(&act->collider, FALSE);
            act->flags &= 0xFFFFDFFBFF7FFFFFLL;
            DisableBtlRikuPassThrough(work);
            act->btl->flags |= BTL_FLAG_DISMISS_SUMMONS;
            work->speed = 0;
            work->scaleX = work->scaleY = Q_8_8(1);

            if (gBtlWork->flags & BTL_FLAG_DARK_MODE) {
                work->state = BTL_RIKU_STATE_DARK_HAZARD;
                work->steps = 0;
                work->stateTimer = 0;
            } else {
                work->state = BTL_RIKU_STATE_HAZARD;
                work->steps = 0;
                work->stateTimer = 0;
            }

            break;
        case BTL_REACTION_CARD_BROKEN:
            switch (work->state) {
            case BTL_RIKU_STATE_SUMMON_TAKEOFF:
            case BTL_RIKU_STATE_SUMMON_EXIT:
            case BTL_RIKU_STATE_SUMMON_OFFSCREEN:
            case BTL_RIKU_STATE_SUMMON_RETURN:
                act->x = act->originX;
                act->y = act->originY;
                act->z = act->originZ;
#ifdef VERSION_EU
                act->flags &= ~BTLOBJ_FLAG_NO_BREAK_POP;
                CreateBtlPopTask(act, BTL_POP_CARD_BREAK);
#endif
                break;
            }

            work->scaleX = work->scaleY = Q_8_8(1);
            ColliderSetDisabled(&act->collider, FALSE);
            DisableBtlRikuPassThrough(work);
            act->flags &= 0xFFFFDFFBFF7FFFFFLL;
            act->btl->flags |= BTL_FLAG_DISMISS_SUMMONS;
            work->speed = 0;

            if (gBtlWork->flags & BTL_FLAG_DARK_MODE) {
                if (gBtlWork->darkPoints <= 0) {
                    EndRikuDarkMode(work);
                    work->state = BTL_RIKU_STATE_CARD_BROKEN;
                    work->steps = 0;
                    work->stateTimer = 0;
                } else {
                    work->state = BTL_RIKU_STATE_DARK_CARD_BROKEN;
                    work->steps = 0;
                    work->stateTimer = 0;
                }
            } else {
                work->state = BTL_RIKU_STATE_CARD_BROKEN;
                work->steps = 0;
                work->stateTimer = 0;
            }

            break;
        case BTL_REACTION_STUNNED:
        case BTL_REACTION_WARPED:
            work->speed = 0;

            if (gBtlWork->flags & BTL_FLAG_DARK_MODE) {
                AddDarkPoints(-5);
                work->state = BTL_RIKU_STATE_DARK_STUNNED;
                work->steps = 0;
                work->stateTimer = 0;
            } else {
                AddDarkPoints(1);
                work->state = BTL_RIKU_STATE_STUNNED;
                work->steps = 0;
                work->stateTimer = 0;
            }

            break;
        case BTL_REACTION_STOPPED:
            if (work->state != BTL_RIKU_STATE_STOPPED) {
                work->flags |= BTL_RIKU_FLAG_PASS_THROUGH;
                work->speed = 0;
                act->vx = act->vy = 0;
                work->state = BTL_RIKU_STATE_STOPPED;
                work->steps = 0;
                work->stateTimer = 0;
            }

            break;
        }
    }

    if (act->btl->flags & BTL_FLAG_FIELD_HIDDEN) {
        if (work->state != BTL_RIKU_STATE_FIELD_HIDDEN) {
            EndRikuDarkMode(work);
            work->state = BTL_RIKU_STATE_FIELD_HIDDEN;
            work->steps = 0;
            work->stateTimer = 0;
        }
    } else if (act->flags & BTLOBJ_FLAG_FREEZE_PENDING) {
        act->flags &= ~BTLOBJ_FLAG_FREEZE_PENDING;
        work->state = BTL_RIKU_STATE_FROZEN;
        work->steps = 0;
        work->stateTimer = 0;
    }

    work->flags &= ~BTL_RIKU_FLAG_AFTERIMAGE;

    switch (work->state) {
    case BTL_RIKU_STATE_DARK_AURA:
        BtlMapFollowPosition(act->x, act->y, act->z);

        if (work->stateTimer == 0) {
            SetBtlRikuAnimation(work, 26, 0);
            m4aSongNumStart(SONG_VO_RK_ATTACK08);
            m4aSongNumStart(SONG_BTL_AN_STANDENTRY);
            FadeStartOut(FADE_MODE_DARK_MAGENTA, 80);
        }

        work->vz = 0;
        act->z += (-10240 - act->z) >> 5;

        if (!AnimIsFinished(&work->anim)) {
            work->stateTimer++;
            break;
        }

        work->state = BTL_RIKU_STATE_DARK_AURA_JUMP;
        work->stateTimer = 0;
        break;
    case BTL_RIKU_STATE_DARK_AURA_JUMP:
        BtlMapFollowPosition(act->x, act->y, act->z);

        if (work->stateTimer == 0) {
            SetBtlRikuAnimation(work, 27, 0);
        }

        switch (AnimGetFrame(&work->anim)) {
        case 1:
        case 2:
            work->vz -= 179;
            break;
        }

        work->flags |= BTL_RIKU_FLAG_AFTERIMAGE;

        if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
            s32 offsetX;

            offsetX = act->x - 12288;
            act->x += (act->originX - offsetX) >> 3;
        } else {
            s32 offsetX;

            offsetX = act->x + 12288;
            act->x += (act->originX - offsetX) >> 3;
        }

        if (AnimIsFinished(&work->anim)) {
            work->state = BTL_RIKU_STATE_DARK_AURA_EXIT;
            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_RIKU_STATE_DARK_AURA_EXIT:
        BtlMapFollowPosition(act->x, act->y, act->z);

        if (work->stateTimer == 0) {
            SetBtlRikuAnimation(work, 28, 0);
            act->flags |= BTLOBJ_FLAG_IGNORE_BOUNDS;
        }

        work->vz = 0;
        work->flags |= BTL_RIKU_FLAG_AFTERIMAGE;

        if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
            act->x -= 3072;
        } else {
            act->x += 3072;
        }

        if (act->x < ((gBtlWork->xMin - 48) << 8) || act->x > ((gBtlWork->xMax + 48) << 8)) {
            work->state = BTL_RIKU_STATE_DARK_AURA_ENTRY;
            work->stateTimer = 0;
            work->limitDashCount = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_DARK_AURA_ENTRY: {
        s32 tx;
        s32 ty;
        s32 tz;

        BtlMapFollowPosition(act->x, act->y, act->z);

        if (work->stateTimer == 0) {
            enemy = PickBtlRikuTarget(work);

            if (enemy != NULL) {
                tx = enemy->x;
                ty = enemy->y;
                tz = enemy->z - 4096;
            } else {
                tx = act->originX;
                ty = act->originY;
                tz = -4096;
            }

            act->flags ^= BTLOBJ_FLAG_FACING_LEFT;

            switch (GetRandom() % 3) {
            case 0:
                SetBtlRikuAnimation(work, 29, 1);

                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    work->unk_194 = GetRandom() % 17 + 184;
                } else {
                    work->unk_194 = GetRandom() % 17 + 56;
                }

                act->y = ty - 4096 + (GetRandom() % 33 << 8);
                break;
            case 1:
                SetBtlRikuAnimation(work, 30, 1);

                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    work->unk_194 = GetRandom() % 17 + 203;
                } else {
                    work->unk_194 = GetRandom() % 17 + 37;
                }

                act->y = ty + 4096 + (GetRandom() % 17 << 8);
                break;
            case 2:
                SetBtlRikuAnimation(work, 31, 1);

                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    work->unk_194 = GetRandom() % 17 + 165;
                } else {
                    work->unk_194 = GetRandom() % 17 + 75;
                }

                act->y = ty - 4096 - (GetRandom() % 17 << 8);
                break;
            }

            act->z = tz;

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                act->x = tx + 25344;
                BgFxStartRikuLimit(act->x, act->y, tz, 192);
            } else {
                act->x = tx - 25344;
                BgFxStartRikuLimit(act->x, act->y, tz, 64);
            }

            m4aSongNumStart(SONG_BTL_RK_LIMITENTRY);
            work->steps = 10;
            work->scaleX = 10;
        }

        work->vz = 0;
        ApproachValue(&work->scaleX, Q_8_8(1), work->steps);

        if (--work->steps <= 0) {
            work->state = BTL_RIKU_STATE_DARK_AURA_DASH;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    }
    case BTL_RIKU_STATE_DARK_AURA_DASH:
        BtlMapFollowPosition(act->x, act->y, act->z);

        if (work->stateTimer == 0) {
            m4aSongNumStart(SONG_EF_RK_LIMITMOV);
        }

        work->flags |= BTL_RIKU_FLAG_AFTERIMAGE;
        act->x += gSineTable[(u8)work->unk_194] * 12;
        act->y += -gSineTable[(u8)work->unk_194 + 64] * 12;

        if (ApplyAttackBox(9, act->x, act->y, act->z, 24, 16, 24) != ATTACK_RESULT_NONE) {
            m4aSongNumStart(SONG_BTL_RK_HIT03);
        }

        work->vz = 0;

        if (work->stateTimer == 15 && (s16)work->limitDashCount > 4) {
            work->state = BTL_RIKU_STATE_DARK_AURA_FINISH;
            work->stateTimer = 0;
            break;
        }

        elapsed = work->stateTimer;

        if (work->stateTimer <= 30) {
            work->stateTimer = elapsed + 1;
            break;
        }

        work->state = BTL_RIKU_STATE_DARK_AURA_ENTRY;
        work->stateTimer = 0;
        work->limitDashCount++;
        break;
    case BTL_RIKU_STATE_DARK_AURA_FINISH:
        BtlMapFollowPosition(act->x, act->y, act->z);

        if (work->stateTimer == 0) {
            SetBtlRikuAnimation(work, 32, 0);
            work->steps = 40;
        }

        if (work->steps > 0) {
            ApproachValue(&act->x, act->originX, work->steps);
            ApproachValue(&act->y, act->originY, work->steps);
            ApproachValue(&act->z, act->originZ, work->steps);

            if (--work->steps <= 0) {
                BgFxStartRikuLimitFinish(act->x, act->y - 8192, 0);
            }
        }

        if (!BgFxIsActive() && work->steps <= 0 && AnimIsFinished(&work->anim)) {
            act->flags &= ~BTLOBJ_FLAG_IGNORE_BOUNDS;
            ClearBtlObjActionFlags(act);
            work->state = BTL_RIKU_STATE_DARK_IDLE;
            FadeStartIn(FADE_MODE_DARK_MAGENTA, 30);
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_DARK_VERTICAL_SLASH:
        FocusBtlRikuCameraOnTarget(work);

        if (work->stateTimer == 0) {
            SetBtlRikuAnimation(work, 22, 0);
            m4aSongNumStart(SONG_VO_RK_ATTACK02);
        }

        switch (AnimGetFrame(&work->anim)) {
        case 1:
            if (work->anim.timer == 0) {
                work->vz = -972;
            }

            break;
        case 2:
        case 3:
            if (ApplyAttackBox(7, act->x, act->y, act->z - 12288, 24, 20, 16) != ATTACK_RESULT_NONE) {
                m4aSongNumStart(SONG_BTL_RK_HIT00);
            }

            break;
        case 4:
            if (work->anim.timer == 0) {
                work->vz = 4096;
            }

            if (work->anim.timer % 6 == 0) {
                MakeOpponentsHittable();
            }

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                if (ApplyAttackBox(7, act->x - 8192, act->y, act->z - 8192, 20, 25, 32) != ATTACK_RESULT_NONE) {
                    m4aSongNumStart(SONG_BTL_RK_HIT00);
                }
            } else {
                if (ApplyAttackBox(7, act->x + 8192, act->y, act->z - 8192, 20, 25, 32) != ATTACK_RESULT_NONE) {
                    m4aSongNumStart(SONG_BTL_RK_HIT00);
                }
            }

            break;
        case 5:
            if (act->z < act->groundZ) {
                work->anim.timer = 0;
                work->anim.frame--;
            }

            break;
        }

        work->flags |= BTL_RIKU_FLAG_AFTERIMAGE;

        switch (AnimGetFrame(&work->anim)) {
        case 1:
        case 2:
        case 3: {
            s32 tx;
            s32 ty;

            enemy = act->btl->actor2;

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                if (enemy != NULL) {
                    tx = enemy->x + 4096;
                    ty = enemy->y;
                } else {
                    tx = act->originX - 10240;
                    ty = act->y;
                }
            } else {
                if (enemy != NULL) {
                    tx = enemy->x - 4096;
                    ty = enemy->y;
                } else {
                    tx = act->originX + 10240;
                    ty = act->y;
                }
            }

            act->x += (tx - act->x) >> 3;
            act->y += (ty - act->y) >> 3;
            break;
        }
        }

        if (AnimIsFinished(&work->anim)) {
            ClearBtlObjActionFlags(act);
            work->state = BTL_RIKU_STATE_DARK_IDLE;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_DARK_FIRAGA:
        if (work->stateTimer == 0) {
            FocusBtlRikuCameraOnTarget(work);
            SetBtlRikuAnimation(work, 25, 0);
            m4aSongNumStart(SONG_VO_RK_ATTACK08);
            work->flags &= ~BTL_RIKU_FLAG_FIRE_LAUNCHED;
            work->target = PickBtlRikuTarget(work);
        }

        if (work->flags & BTL_RIKU_FLAG_FIRE_LAUNCHED) {
            FocusBtlRikuCameraOnBgFx(work);
            enemy = work->target;

            if (enemy != NULL) {
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
        }

        if ((work->flags & BTL_RIKU_FLAG_FIRE_LAUNCHED) == 0 && work->anim.timer == 0) {
            s16 step;
            s32 launch;

            step = 0;
            launch = FALSE;

            switch (AnimGetFrame(&work->anim)) {
            case 0:
                step = -10;
                break;
            case 1:
                step = -24;
                break;
            case 4:
                step = 12;
                break;
            case 5:
                step = 15;
                break;
            case 6:
                step = 7;
                launch = TRUE;
                break;
            }

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                act->x -= step << 8;
            } else {
                act->x += step << 8;
            }

            if (launch) {
                work->flags |= BTL_RIKU_FLAG_FIRE_LAUNCHED;

                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    BgFxStartFire(SPELL_TIER_DARK, act->x - 18944, act->y, act->z - 6144, act->originX - 51200, act->originY, act->z - 6144, TRUE, 10);
                } else {
                    BgFxStartFire(SPELL_TIER_DARK, act->x + 18944, act->y, act->z - 6144, act->originX + 51200, act->originY, act->z - 6144, FALSE, 10);
                }
            }
        }

        if (AnimIsFinished(&work->anim)) {
            SetBtlRikuAnimation(work, 12, 1);
        }

        if (AnimIsFinished(&work->anim) && !BgFxIsActive()) {
            ClearBtlObjActionFlags(act);
            work->state = BTL_RIKU_STATE_DARK_IDLE;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_DARK_AIR_SLASH:
        FocusBtlRikuCameraOnTarget(work);

        if (work->stateTimer == 0) {
            SetBtlRikuAnimation(work, 21, 0);
            m4aSongNumStart(GetRandom() % 2 + SONG_VO_RK_ATTACK04);
            work->vz = -384;
            work->flags &= ~BTL_RIKU_FLAG_SWING_HIT;

            switch (act->btl->hcEffect) {
            case HC_EFFECT_BERSERK:
            case HC_EFFECT_WIDE_ATTACK_2:
            case HC_EFFECT_OVERDRIVE:
            case HC_EFFECT_ATTACK_HASTE:
            case HC_EFFECT_WARP_BREAK:
                act->btl->hcEffectCount--;
                break;
            }
        }

        work->flags |= BTL_RIKU_FLAG_AFTERIMAGE;

        if (work->anim.timer == 0) {
            step = 0;

            switch (AnimGetGfxIndex(&work->anim)) {
            case 3:
                step = 16;
                break;
            case 4:
                step = 5;
                break;
            case 6:
                step = 1;
                break;
            case 2:
            case 7:
                step = 4;
                break;
            case 8:
                step = 6;
                break;
            case 9:
            case 10:
                step = 2;
                break;
            }

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                act->x -= step << 8;
            } else {
                act->x += step << 8;
            }

            switch (AnimGetFrame(&work->anim)) {
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    act->x -= 256;
                } else {
                    act->x += 256;
                }

                break;
            }
        }

        frame = AnimGetFrame(&work->anim);

        if (frame >= 1 && frame <= 9) {
            if (act->btl->hcEffect == HC_EFFECT_WIDE_ATTACK_2) {
                if (ApplyAttackBox(6, act->x, act->y, act->z - 5120, 65, 30, 12) != ATTACK_RESULT_NONE) {
                    m4aSongNumStart(SONG_BTL_RK_HIT01);
                    work->flags |= BTL_RIKU_FLAG_SWING_HIT;
                }
            } else {
                if (ApplyAttackBox(6, act->x, act->y, act->z - 5120, 45, 20, 12) != ATTACK_RESULT_NONE) {
                    m4aSongNumStart(SONG_BTL_RK_HIT01);
                    work->flags |= BTL_RIKU_FLAG_SWING_HIT;
                }
            }

            gBtlWork->damageScale = 0;
        } else if (frame == 10) {
            if (work->comboCount <= 1) {
                if (work->flags & BTL_RIKU_FLAG_SWING_HIT) {
                    act->flags &= ~BTLOBJ_FLAG_IN_CARD_ACTION;
                }
            }
        }

        if (AnimIsFinished(&work->anim)) {
            ClearBtlObjActionFlags(act);
            work->state = BTL_RIKU_STATE_DARK_IDLE;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_DARK_DOUBLE_SLASH:
        FocusBtlRikuCameraOnTarget(work);

        if (work->stateTimer == 0) {
            SetBtlRikuAnimation(work, 20, 0);
            m4aSongNumStart(GetRandom() % 2 + SONG_VO_RK_ATTACK04);
        }

        if (AnimGetGfxIndex(&work->anim) == 6) {
            work->flags |= BTL_RIKU_FLAG_AFTERIMAGE;

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                s32 offsetX;

                offsetX = act->x + 22528;
                act->x += (act->originX - offsetX) >> 2;
            } else {
                s32 offsetX;

                offsetX = act->x - 22528;
                act->x += (act->originX - offsetX) >> 2;
            }
        }

        if (work->anim.timer == 0) {
            s16 step;
            s32 strike;
            s32 attack;

            step = 0;
            strike = FALSE;
            attack = 4;

            switch (AnimGetGfxIndex(&work->anim)) {
            case 1:
                step = 15;
                break;
            case 2:
                step = 5;
                strike = TRUE;
                attack = 4;
                break;
            case 4:
                step = -5;
                break;
            case 5:
                step = 10;
                break;
            case 6:
                step = 20;
                strike = TRUE;
                attack = 5;
                break;
            case 7:
                step = -9;
                break;
            case 8:
                step = 6;
                break;
            case 9:
                step = 1;
                break;
            }

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                act->x -= step << 8;
            } else {
                act->x += step << 8;
            }

            if (strike) {
                MakeOpponentsHittable();

                if ((act->flags & BTLOBJ_FLAG_FACING_LEFT) ? ApplyAttackBox(attack, act->x - 5120, act->y, act->z, 20, 8, 16)
                                   : ApplyAttackBox(attack, act->x + 5120, act->y, act->z, 20, 8, 16)) {
                    m4aSongNumStart(SONG_BTL_RK_HIT01);

                    if (attack == 5) {
                        FadeStartIn(FADE_MODE_ADD_WHITE, 45);

                        if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                            SetBattleZoom(6, Q_8_8(1.3), act->x - 8192, act->y - 6144 + act->z);
                        } else {
                            SetBattleZoom(6, Q_8_8(1.3), act->x + 8192, act->y - 6144 + act->z);
                        }
                    }
                }
            }
        }

        if (work->anim.timer == 2 && AnimGetGfxIndex(&work->anim) == 6) {
            SetBattleZoom(8, Q_8_8(1), gBtlWork->x2, gBtlWork->y2);
        }

        if (AnimIsFinished(&work->anim)) {
            ClearBtlObjActionFlags(act);
            work->state = BTL_RIKU_STATE_DARK_IDLE;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_DARK_SLASH:
        FocusBtlRikuCameraOnTarget(work);

        if (work->stateTimer == 0) {
            SetBtlRikuAnimation(work, 33, 0);
            m4aSongNumStart(GetRandom() % 2 + SONG_VO_RK_ATTACK04);
            MakeOpponentsHittable();
            work->flags &= ~BTL_RIKU_FLAG_SWING_HIT;

            switch (act->btl->hcEffect) {
            case HC_EFFECT_BERSERK:
            case HC_EFFECT_WIDE_ATTACK_2:
            case HC_EFFECT_OVERDRIVE:
            case HC_EFFECT_ATTACK_HASTE:
            case HC_EFFECT_WARP_BREAK:
                act->btl->hcEffectCount--;
                break;
            }
        }

        if (work->flags & BTL_RIKU_FLAG_SWING_HIT) {
            act->flags &= ~BTLOBJ_FLAG_IN_CARD_ACTION;
        }

        if (work->anim.timer == 0) {
            s16 step;
            s32 strike;
            s32 attack;

            step = 0;
            strike = FALSE;
            attack = 4;

            switch (AnimGetGfxIndex(&work->anim)) {
            case 1:
                step = 15;
                break;
            case 2:
                step = 5;
                strike = TRUE;
                attack = 4;
                break;
            case 4:
                step = -5;
                break;
            case 5:
                step = 10;
                break;
            case 6:
                step = 20;
                break;
            case 7:
                step = -9;
                break;
            case 8:
                step = 6;
                break;
            case 9:
                step = 1;
                break;
            }

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                act->x -= step << 8;
            } else {
                act->x += step << 8;
            }

            if (strike) {
                if (act->btl->hcEffect == HC_EFFECT_WIDE_ATTACK_2) {
                    if ((act->flags & BTLOBJ_FLAG_FACING_LEFT) ? ApplyAttackBox(attack, act->x - 8960, act->y, act->z, 35, 25, 40)
                                       : ApplyAttackBox(attack, act->x + 8960, act->y, act->z, 35, 25, 40)) {
                        work->flags |= BTL_RIKU_FLAG_SWING_HIT;
                        m4aSongNumStart(SONG_BTL_RK_HIT01);
                    }
                } else {
                    if ((act->flags & BTLOBJ_FLAG_FACING_LEFT) ? ApplyAttackBox(attack, act->x - 5120, act->y, act->z, 20, 18, 40)
                                       : ApplyAttackBox(attack, act->x + 5120, act->y, act->z, 20, 18, 40)) {
                        work->flags |= BTL_RIKU_FLAG_SWING_HIT;
                        m4aSongNumStart(SONG_BTL_RK_HIT01);
                    }
                }
            }

            gBtlWork->damageScale = 0;
        }

        switch (AnimGetFrame(&work->anim)) {
        case 1:
        case 2:
            enemy = act->btl->actor2;

            if (enemy != NULL) {
                ex = enemy->x - act->x;
                ey = enemy->y - act->y;

                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    s32 target;
                    s32 origin;

                    if (ex > 0) {
                        ex = 0;
                    }

                    target = act->originX + ex;
                    origin = act->x - 4096;
                    act->x += (target - origin) >> 3;
                } else {
                    s32 target;
                    s32 origin;

                    if (ex < 0) {
                        ex = 0;
                    }

                    target = act->originX + ex;
                    origin = act->x + 4096;
                    act->x += (target - origin) >> 3;
                }

                act->y += ((act->originY + ey) - act->y) >> 4;
            }

            break;
        }

        if (AnimIsFinished(&work->anim)) {
            ClearBtlObjActionFlags(act);
            work->state = BTL_RIKU_STATE_DARK_IDLE;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_DARK_LUNGE_SLASH: {
        s16 step;
        s32 strike;
        s32 attack;
        s32 reach;
        s32 halfX;
        s32 halfY;
        s16 halfZ;

        FocusBtlRikuCameraOnTarget(work);

        if (work->stateTimer == 0) {
            SetBtlRikuAnimation(work, 34, 0);
            m4aSongNumStart(GetRandom() % 2 + SONG_VO_RK_ATTACK04);
            work->flags &= ~BTL_RIKU_FLAG_SWING_HIT;

            switch (act->btl->hcEffect) {
            case HC_EFFECT_BERSERK:
            case HC_EFFECT_WIDE_ATTACK_2:
            case HC_EFFECT_OVERDRIVE:
            case HC_EFFECT_ATTACK_HASTE:
            case HC_EFFECT_WARP_BREAK:
                act->btl->hcEffectCount--;
                break;
            }
        }

        if ((work->flags & BTL_RIKU_FLAG_SWING_HIT) && work->anim.timer == 2 && AnimGetGfxIndex(&work->anim) == 6) {
            act->flags &= ~BTLOBJ_FLAG_IN_CARD_ACTION;
            SetBattleZoom(8, Q_8_8(1), gBtlWork->x2, gBtlWork->y2);
        }

        if (AnimGetGfxIndex(&work->anim) == 6) {
            work->flags |= BTL_RIKU_FLAG_AFTERIMAGE;

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                s32 offsetX;

                offsetX = act->x + 22528;
                act->x += (act->originX - offsetX) >> 2;
            } else {
                s32 offsetX;

                offsetX = act->x - 22528;
                act->x += (act->originX - offsetX) >> 2;
            }
        }

        if (work->anim.timer == 0) {
            step = 0;
            strike = FALSE;
            attack = 4;

            switch (AnimGetGfxIndex(&work->anim)) {
            case 1:
                step = 15;
                break;
            case 2:
                step = 5;
                break;
            case 4:
                step = -5;
                break;
            case 5:
                step = 10;
                break;
            case 6:
                step = 20;
                strike = TRUE;
                attack = 5;
                break;
            case 7:
                step = -9;
                break;
            case 8:
                step = 6;
                break;
            case 9:
                step = 1;
                break;
            }

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                act->x -= step << 8;
            } else {
                act->x += step << 8;
            }

            if (strike) {
                MakeOpponentsHittable();

                if (act->btl->hcEffect == HC_EFFECT_WIDE_ATTACK_2) {
                    reach = 35;
                    halfX = 35;
                    halfY = 25;
                    halfZ = 32;
                } else {
                    reach = 20;
                    halfX = 20;
                    halfY = 16;
                    halfZ = 32;
                }

                if ((act->flags & BTLOBJ_FLAG_FACING_LEFT)
                    ? ApplyAttackBox(attack, act->x - (reach << 8), act->y, act->z, halfX, halfY, halfZ)
                    : ApplyAttackBox(attack, act->x + (reach << 8), act->y, act->z, halfX, halfY, halfZ)) {
                    work->flags |= BTL_RIKU_FLAG_SWING_HIT;
                    m4aSongNumStart(SONG_BTL_RK_HIT01);

                    if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                        SetBattleZoom(6, Q_8_8(1.2), act->x - 8192, act->y - 6144 + act->z);
                    } else {
                        SetBattleZoom(6, Q_8_8(1.2), act->x + 8192, act->y - 6144 + act->z);
                    }
                }

                gBtlWork->damageScale = 0;
            }
        }

        if (AnimIsFinished(&work->anim)) {
            ClearBtlObjActionFlags(act);
            work->state = BTL_RIKU_STATE_DARK_IDLE;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    }
    case BTL_RIKU_STATE_DARK_BREAK:
        BtlMapFollowPosition(act->x, act->y, act->z);

        if (act->z < act->groundZ) {
            break;
        }

        if (work->stateTimer == 0) {
            FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
            SetBtlRikuAnimation(work, 15, 0);
            m4aSongNumStart(SONG_VO_RK_ATTACK02);
        }

        if (!AnimIsFinished(&work->anim)) {
            work->stateTimer++;
            break;
        }

        work->stateTimer = 0;
        work->state = BTL_RIKU_STATE_DARK_BREAK_RISE;
        work->vz = -3072;
        break;
    case BTL_RIKU_STATE_DARK_BREAK_RISE: {
        BtlObj* enemy;

        BtlMapFollowPosition(act->x, act->y, act->z);

        if (work->stateTimer == 0) {
            work->target = FindHighestEnemy(work);
        }

        enemy = work->target;

        if (enemy != NULL) {
            act->x += (enemy->x - act->x) >> 3;
            act->y += (enemy->y - act->y) >> 3;
        }

        work->flags |= BTL_RIKU_FLAG_AFTERIMAGE;

        if (work->vz < 0) {
            if (work->vz > -512) {
                SetBtlRikuAnimation(work, 17, 0);
            } else {
                SetBtlRikuAnimation(work, 16, 0);
            }
        } else {
            work->stateTimer = 0;
            work->steps = 0;
            work->state = BTL_RIKU_STATE_DARK_BREAK_DIVE;
            break;
        }

        work->stateTimer++;
        break;
    }
    case BTL_RIKU_STATE_DARK_BREAK_DIVE:
        if (work->stateTimer == 0) {
            SetBtlRikuAnimation(work, 23, 0);
            work->vz = 1792;
            work->flags &= ~BTL_RIKU_FLAG_SWING_HIT;
            MakeOpponentsHittable();
        }

        work->flags |= BTL_RIKU_FLAG_AFTERIMAGE;

        if (work->target != NULL) {
            BtlMapFollowPosition(act->x, act->y,
                          work->target->z - (work->target->centerHeight << 8));
            act->x += (work->target->x - act->x) >> 3;
            act->y += (work->target->y - act->y) >> 3;
        } else {
            BtlMapFollowPosition(act->x, act->y, act->groundZ);
        }

        if (AnimGetFrame(&work->anim) > 1) {
            if (ApplyAttackBox(8, act->x, act->y, act->z, 32, 16, 24) != ATTACK_RESULT_NONE) {
                m4aSongNumStart(SONG_BTL_RK_HIT02);
                work->flags |= BTL_RIKU_FLAG_SWING_HIT;
                BgFxStartRikuDiveHit(act->x, act->y, act->z);
            }
        }

        if ((work->flags & BTL_RIKU_FLAG_SWING_HIT) != 0 || act->z >= act->groundZ) {
            work->stateTimer = 0;

            if (work->steps > 4) {
                work->state = BTL_RIKU_STATE_DARK_BREAK_END;
                break;
            }

            work->state = BTL_RIKU_STATE_DARK_BREAK_REBOUND;
            work->steps++;
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_RIKU_STATE_DARK_BREAK_REBOUND: {
        BtlObj* enemy;

        if (work->stateTimer == 0) {
            work->target = FindHighestEnemy(work);
            SetBtlRikuAnimation(work, 24, 0);
            work->vz = -2176;
        }

        work->flags |= BTL_RIKU_FLAG_AFTERIMAGE;
        enemy = work->target;

        if (enemy != NULL) {
            act->x += (enemy->x - act->x) >> 3;
            act->y += (enemy->y - act->y) >> 3;
        }

        if (AnimGetFrame(&work->anim) != 4) {
            work->stateTimer++;
            break;
        }

        work->state = BTL_RIKU_STATE_DARK_BREAK_DIVE;
        work->stateTimer = 0;
        break;
    }
    case BTL_RIKU_STATE_DARK_BREAK_END:
        BtlMapFollowPosition(act->x, act->y, act->z);

        if (work->stateTimer == 0) {
            SetBtlRikuAnimation(work, 24, 0);
            work->vz = -1536;

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                work->targetX = act->x + 12288;
            } else {
                work->targetX = act->x - 12288;
            }
        }

        work->flags |= BTL_RIKU_FLAG_AFTERIMAGE;
        act->x += (work->targetX - act->x) >> 3;

        if (AnimIsFinished(&work->anim) && act->z >= act->groundZ) {
            FadeToOriginal(FADE_MODE_BLACK, 8);
            ClearBtlObjActionFlags(act);
            work->state = BTL_RIKU_STATE_DARK_IDLE;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_DARK_JUMP:
        FocusBtlRikuCameraOnTarget(work);

        if (work->stateTimer == 0) {
            SetBtlRikuAnimation(work, 15, 0);

            if ((held & DPAD_ANY) == 0) {
                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    work->angle = 192;
                } else {
                    work->angle = 64;
                }
            }

            work->speed >>= 1;
        }

        if (pressed & B_BUTTON) {
            work->state = BTL_RIKU_STATE_DARK_LEAP;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        }

        if (AnimIsFinished(&work->anim)) {
            work->state = BTL_RIKU_STATE_DARK_AIRBORNE;
            work->steps = 0;
            work->stateTimer = 0;
            work->vz = -1600;
            act->btl->flags |= BTL_FLAG_PLAYER_AIRBORNE;
            work->speed <<= 1;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_DARK_AIRBORNE:
        FocusBtlRikuCameraOnTarget(work);
        act->btl->flags |= BTL_FLAG_PLAYER_AIRBORNE;

        if (work->vz < 0) {
            if (work->vz <= -512) {
                SetBtlRikuAnimation(work, 16, 0);
            } else {
                SetBtlRikuAnimation(work, 17, 0);
            }
        } else if (work->vz <= 511) {
            SetBtlRikuAnimation(work, 17, 0);
        } else {
            SetBtlRikuAnimation(work, 18, 0);
        }

        if (work->vz < 0 && (held & B_BUTTON) == 0) {
            work->vz += 64;
        }

        if (pressed & B_BUTTON) {
            work->state = BTL_RIKU_STATE_DARK_LEAP;
            work->steps = 0;
            work->stateTimer = 0;
            break;
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

            if (work->speed > 512) {
                work->speed = 512;
            }
        } else {
            work->speed -= 38;

            if (work->speed < 0) {
                work->speed = 0;
            }
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_DARK_LAND:
        FocusBtlRikuCameraOnTarget(work);

        if (work->stateTimer == 0) {
            m4aSongNumStart(work->groundSongs[3]);
            work->speed = 0;
            SetBtlRikuAnimation(work, 19, 0);
        } else if (pressed & B_BUTTON) {
            m4aSongNumStart(work->groundSongs[2]);
            act->btl->flags |= BTL_FLAG_PLAYER_AIRBORNE;
            work->state = BTL_RIKU_STATE_DARK_JUMP;
            work->steps = 0;
            work->stateTimer = 0;
        } else if (pressed & DPAD_LEFT) {
            if (work->tapTimers[0] != 0) {
                act->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
                work->state = BTL_RIKU_STATE_DARK_DASH;
                work->steps = 0;
                work->stateTimer = 0;
                break;
            }
        } else if (pressed & DPAD_RIGHT) {
            if (work->tapTimers[1] != 0) {
                act->flags |= BTLOBJ_FLAG_FACING_LEFT;
                work->state = BTL_RIKU_STATE_DARK_DASH;
                work->steps = 0;
                work->stateTimer = 0;
                break;
            }
        } else if (pressed & DPAD_UP) {
            if (work->tapTimers[2] != 0) {
                work->flags |= BTL_RIKU_FLAG_DASH_UP;
                work->state = BTL_RIKU_STATE_DARK_DASH_VERTICAL;
                work->steps = 0;
                work->stateTimer = 0;
                break;
            }
        } else if (pressed & DPAD_DOWN) {
            if (work->tapTimers[3] != 0) {
                work->flags &= ~BTL_RIKU_FLAG_DASH_UP;
                work->state = BTL_RIKU_STATE_DARK_DASH_VERTICAL;
                work->steps = 0;
                work->stateTimer = 0;
                break;
            }
        }

        if (AnimIsFinished(&work->anim)) {
            work->state = BTL_RIKU_STATE_DARK_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_DARK_HURT:
        FocusBtlRikuCameraOnTarget(work);
        act->originX = act->x;
        act->originY = act->y;

        if (work->stateTimer == 0) {
            AnimReset(&work->anim);
            SetBtlRikuAnimation(work, 14, 0);
            gBtlWork->hitStop = gBtlWork->pendingHitStop;
            StartBtlRikuKnockback(work);
            func_0807B3C4(30);

            if (gBtlWork->hcEffect == HC_EFFECT_QUICK_RECOVERY) {
                act->btl->hcEffectCount--;
                work->steps = 0;
            } else {
                work->steps = 15;
            }
        } else if (work->stateTimer == 6) {
            switch (GetRandom() % 3) {
            case 0:
                m4aSongNumStart(SONG_VO_RK_DAMAGE00);
                break;
            case 1:
                m4aSongNumStart(SONG_VO_RK_DAMAGE01);
                break;
            case 2:
            default:
                m4aSongNumStart(SONG_VO_RK_DAMAGE02);
                break;
            }
        }

        if (work->stateTimer >= work->steps && gBtlWork->darkPoints > 0) {
            act->flags &= ~BTLOBJ_FLAG_CARD_USE_BLOCKED;
        }

        if (AnimIsFinished(&work->anim)) {
            ClearBtlObjActionFlags(act);
            act->originX = act->x;
            act->originY = act->y;

            if (gBtlWork->darkPoints > 0) {
                work->state = BTL_RIKU_STATE_DARK_IDLE;
                work->steps = 0;
                work->stateTimer = 0;
                break;
            }

            act->flags |= (BTLOBJ_FLAG_INTANGIBLE | BTLOBJ_FLAG_CARD_USE_BLOCKED);
            work->state = BTL_RIKU_STATE_DARK_MODE_END;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_DARK_IDLE:
        FocusBtlRikuCameraOnTarget(work);
        DisableBtlRikuPassThrough(work);

        if (gBtlWork->darkPoints <= 0) {
            act->flags |= (BTLOBJ_FLAG_INTANGIBLE | BTLOBJ_FLAG_CARD_USE_BLOCKED);
            work->state = BTL_RIKU_STATE_DARK_MODE_END;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        }

        if (pressed & DPAD_LEFT) {
            if (work->tapTimers[0] != 0) {
                act->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
                work->state = BTL_RIKU_STATE_DARK_DASH;
                work->steps = 0;
                work->stateTimer = 0;
                break;
            }
        } else if (pressed & DPAD_RIGHT) {
            if (work->tapTimers[1] != 0) {
                act->flags |= BTLOBJ_FLAG_FACING_LEFT;
                work->state = BTL_RIKU_STATE_DARK_DASH;
                work->steps = 0;
                work->stateTimer = 0;
                break;
            }
        } else if (pressed & DPAD_UP) {
            if (work->tapTimers[2] != 0) {
                work->flags |= BTL_RIKU_FLAG_DASH_UP;
                work->state = BTL_RIKU_STATE_DARK_DASH_VERTICAL;
                work->steps = 0;
                work->stateTimer = 0;
                break;
            }
        } else if (pressed & DPAD_DOWN) {
            if (work->tapTimers[3] != 0) {
                work->flags &= ~BTL_RIKU_FLAG_DASH_UP;
                work->state = BTL_RIKU_STATE_DARK_DASH_VERTICAL;
                work->steps = 0;
                work->stateTimer = 0;
                break;
            }
        }

        UpdateBtlRikuDarkWalk(work, held);

        if (pressed & B_BUTTON) {
            m4aSongNumStart(work->groundSongs[2]);
            work->state = BTL_RIKU_STATE_DARK_JUMP;
            work->steps = 0;
            work->stateTimer = 0;
        }

        break;
    case BTL_RIKU_STATE_DARK_MODE_START:
        if (act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) {
            FocusBtlRikuCameraOnTarget(work);
            break;
        }

        if (act->z < act->groundZ) {
            FocusBtlRikuCameraOnTarget(work);
            break;
        }

        BtlMapFollowPosition(act->x, act->y, act->z);

        if (work->stateTimer == 0) {
            SetBtlRikuAnimation(work, 0, 1);
            FadeToAmount(FADE_MODE_DARK_MAGENTA, 17, 16);
            m4aSongNumStart(SONG_SND_702);
            BgFxStartRikuDarkModeFlash(act->x, act->y, act->z);
            work->steps = 6;
            act->vx = act->vy = work->speed = 0;
        }

        work->vz = 0;

        if (work->stateTimer > 35 && work->steps != 0) {
            ApproachValue(&work->scaleY, 5, work->steps);
            work->steps--;
        }

        if (work->steps > 0) {
            work->stateTimer++;
            break;
        }

        work->state = BTL_RIKU_STATE_DARK_MODE_TRANSFORM;
        work->steps = 0;
        work->stateTimer = 0;
        break;
    case BTL_RIKU_STATE_DARK_MODE_TRANSFORM:
        BtlMapFollowPosition(act->x, act->y, act->z);

        if (work->stateTimer == 0) {
            SetBtlRikuAnimation(work, 24, 0);
            work->paletteData = gBtlRikuDarkModePalette;
            LoadObjPaletteBank(work->palette->index, gBtlRikuDarkModePalette);
            gBtlWork->flags |= BTL_FLAG_DARK_MODE;
            gBtlWork->flags |= BTL_FLAG_DARK_MODE_CHANGED;
            gBtlWork->darkPoints = gGameState.progression.dp;
            RequestSoraRemoveItemCards();
            work->scaleY = 5;
            work->steps = 6;
            work->vz = -1536;
            m4aSongNumStart(SONG_SND_715);
            m4aSongNumStart(SONG_SND_292);
            BgFxStartRikuDarkMode(act->x, act->y, act->z - 10240);
        }

        if (work->vz < 0) {
            if (work->vz > -256) {
                gBtlWork->hitStop = 4;
            } else {
                gBtlWork->hitStop = 1;
            }
        }

        BgFxSetPosition(act->x, act->y, act->z - 10240);

        if (work->scaleY == Q_8_8(1)) {
            work->flags |= BTL_RIKU_FLAG_AFTERIMAGE;
        }

        if (work->steps != 0) {
            ApproachValue(&work->scaleY, Q_8_8(1), work->steps);
            work->steps--;
        }

        if (work->steps <= 0 && act->z >= act->groundZ) {
            FadeToOriginal(FADE_MODE_DARK_MAGENTA, 10);
            ClearBtlObjActionFlags(act);
            work->state = BTL_RIKU_STATE_DARK_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_DARK_MODE_END:
        if (act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) {
            FocusBtlRikuCameraOnTarget(work);
            break;
        }

        if (act->z < act->groundZ) {
            FocusBtlRikuCameraOnTarget(work);
            break;
        }

        BtlMapFollowPosition(act->x, act->y, act->z);

        if (work->stateTimer == 0) {
            SetBtlRikuAnimation(work, 14, 0);
            m4aSongNumStart(SONG_SND_716);
            BgFxStartRikuDarkModeFlash(act->x, act->y, act->z);
            work->steps = 6;
            act->vx = act->vy = work->speed = 0;
        }

        work->vz = 0;

        if (work->stateTimer > 20 && work->steps != 0) {
            ApproachValue(&work->scaleY, 5, work->steps);
            work->steps--;
        }

        if (work->steps > 0) {
            work->stateTimer++;
            break;
        }

        work->state = BTL_RIKU_STATE_DARK_MODE_REVERT;
        work->steps = 0;
        work->stateTimer = 0;
        break;
    case BTL_RIKU_STATE_DARK_MODE_REVERT:
        BtlMapFollowPosition(act->x, act->y, act->z);

        if (work->stateTimer == 0) {
            EndRikuDarkMode(work);
            SetBtlRikuAnimation(work, 0, 1);
            work->scaleY = 5;
            work->steps = 6;
        }

        if (work->scaleY == Q_8_8(1)) {
            work->flags |= BTL_RIKU_FLAG_AFTERIMAGE;
        }

        if (work->steps != 0) {
            ApproachValue(&work->scaleY, Q_8_8(1), work->steps);
            work->steps--;
        }

        if (work->steps <= 0) {
            act->flags &= ~(BTLOBJ_FLAG_INTANGIBLE | BTLOBJ_FLAG_CARD_USE_BLOCKED);
            work->state = BTL_RIKU_STATE_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_ENTER:
        if (work->stateTimer == 0) {
            if (work->steps == 0) {
                SetBtlRikuAnimation(work, 11, 0);
            }

            if (work->steps <= 69) {
                AnimReset(&work->anim);
            }

            if (!AnimIsFinished(&work->anim)) {
                work->steps++;
                break;
            }

            work->stateTimer = 1;
            break;
        }

        SetBtlRikuAnimation(work, 0, 1);

        if (gBtlWork->phase == BTL_PHASE_START) {
            break;
        }

        act->flags &= ~BTLOBJ_FLAG_CARD_USE_BLOCKED;
        act->flags &= ~BTLOBJ_FLAG_INTANGIBLE;
        work->state = BTL_RIKU_STATE_IDLE;
        work->steps = 0;
        work->stateTimer = 0;
        break;
    case BTL_RIKU_STATE_IDLE:
        FocusBtlRikuCameraOnTarget(work);
        DisableBtlRikuPassThrough(work);

        if (act->z < act->groundZ) {
            work->state = BTL_RIKU_STATE_AIRBORNE;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        }

        if (pressed & DPAD_LEFT) {
            if (work->tapTimers[0] != 0) {
                act->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
                act->originX = act->x;
                work->state = BTL_RIKU_STATE_DODGE;
                work->steps = 0;
                work->stateTimer = 0;
                break;
            }
        } else if (pressed & DPAD_RIGHT) {
            if (work->tapTimers[1] != 0) {
                act->flags |= BTLOBJ_FLAG_FACING_LEFT;
                act->originX = act->x;
                work->state = BTL_RIKU_STATE_DODGE;
                work->steps = 0;
                work->stateTimer = 0;
                break;
            }
        }

        UpdateBtlRikuWalk(work, held);

        if ((pressed & B_BUTTON) == 0) {
            break;
        }

        m4aSongNumStart(work->groundSongs[2]);
        work->state = BTL_RIKU_STATE_JUMP;
        work->steps = 0;
        work->stateTimer = 0;
        break;
    case BTL_RIKU_STATE_JUMP:
        FocusBtlRikuCameraOnTarget(work);

        if (work->stateTimer == 0) {
            SetBtlRikuDirAnimation(work, 1, 1);

            if ((held & DPAD_ANY) == 0) {
                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    work->angle = 192;
                } else {
                    work->angle = 64;
                }
            }

            work->speed >>= 1;
        }

        elapsed = work->stateTimer;

        if (work->stateTimer <= 3) {
            work->stateTimer = elapsed + 1;
            break;
        }

        work->state = BTL_RIKU_STATE_AIRBORNE;
        work->steps = 0;
        work->stateTimer = 0;
        work->vz = -1664;
        act->btl->flags |= BTL_FLAG_PLAYER_AIRBORNE;
        work->speed <<= 1;
        break;
    case BTL_RIKU_STATE_FROZEN:
        if (work->stateTimer == 0) {
            act->flags |= BTLOBJ_FLAG_INTANGIBLE;
            act->flags |= BTLOBJ_FLAG_CARD_USE_BLOCKED;
            work->speed = 0;
            work->flags |= BTL_RIKU_FLAG_HIDDEN;
        }

        if (work->stateTimer > 285) {
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

                if (act->hp > 1) {
                    act->hp = hp - 1;
                }
            }
        }

        elapsed = work->stateTimer;

        if (work->stateTimer > 300) {
            act->flags &= ~BTLOBJ_FLAG_INTANGIBLE;
            act->flags &= ~BTLOBJ_FLAG_CARD_USE_BLOCKED;
            work->flags &= ~BTL_RIKU_FLAG_HIDDEN;

            if (gBtlWork->flags & BTL_FLAG_DARK_MODE) {
                work->state = BTL_RIKU_STATE_DARK_IDLE;
                work->steps = 0;
                work->stateTimer = 0;
                break;
            }

            work->state = BTL_RIKU_STATE_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
        } else {
            work->stateTimer = elapsed + 1;
        }

        break;
    case BTL_RIKU_STATE_AIRBORNE:
        FocusBtlRikuCameraOnTarget(work);
        act->btl->flags |= BTL_FLAG_PLAYER_AIRBORNE;

        if (work->vz < 0) {
            if (work->vz <= -512) {
                SetBtlRikuDirAnimation(work, 2, 1);
            } else {
                SetBtlRikuDirAnimation(work, 3, 1);
            }
        } else if (work->vz <= 511) {
            SetBtlRikuDirAnimation(work, 3, 1);
        } else {
            SetBtlRikuDirAnimation(work, 4, 1);
        }

        if (work->vz < 0 && (held & B_BUTTON) == 0) {
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

            if (work->speed > 512) {
                work->speed = 512;
            }
        } else {
            work->speed -= 38;

            if (work->speed < 0) {
                work->speed = 0;
            }
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_LAND:
        FocusBtlRikuCameraOnTarget(work);

        if (work->stateTimer == 0) {
            m4aSongNumStart(work->groundSongs[3]);
            work->speed = 0;
            SetBtlRikuDirAnimation(work, 5, 0);
        } else if (pressed & B_BUTTON) {
            m4aSongNumStart(work->groundSongs[2]);
            act->btl->flags |= BTL_FLAG_PLAYER_AIRBORNE;
            work->state = BTL_RIKU_STATE_JUMP;
            work->steps = 0;
            work->stateTimer = 0;
        } else if (pressed & DPAD_LEFT) {
            if (work->tapTimers[0] != 0) {
                act->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
                act->originX = act->x;
                work->state = BTL_RIKU_STATE_DODGE;
                work->steps = 0;
                work->stateTimer = 0;
                break;
            }
        } else if (pressed & DPAD_RIGHT) {
            if (work->tapTimers[1] != 0) {
                act->flags |= BTLOBJ_FLAG_FACING_LEFT;
                act->originX = act->x;
                work->state = BTL_RIKU_STATE_DODGE;
                work->steps = 0;
                work->stateTimer = 0;
                break;
            }
        }

        elapsed = work->stateTimer;

        if (work->stateTimer <= 6) {
            work->stateTimer = elapsed + 1;
            break;
        }

        work->state = BTL_RIKU_STATE_IDLE;
        work->steps = 0;
        work->stateTimer = 0;
        break;
    case BTL_RIKU_STATE_ESCAPE:
        SetBtlRikuDirAnimation(work, 0, 1);
        work->speed = 0;

        if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
            work->angle = 192;
            act->x -= 512;
        } else {
            work->angle = 64;
            act->x += 512;
        }

        break;
    case BTL_RIKU_STATE_ITEM_POSE:
        if (work->stateTimer == 0) {
            SetBtlRikuAnimation(work, 10, 0);
        }

        if (AnimIsFinished(&work->anim)) {
            SetBtlRikuState(work, BTL_RIKU_STATE_IDLE);
            break;
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_CARD_BROKEN:
        if (work->flags & BTL_RIKU_FLAG_HIDDEN) {
            break;
        }

        FocusBtlRikuCameraOnTarget(work);

        if (work->stateTimer == 0) {
            SetBtlRikuAnimation(work, 9, 0);
        }

        FocusBtlRikuCameraOnTarget(work);

        if (AnimIsFinished(&work->anim)) {
            ClearBtlObjActionFlags(act);
            work->state = BTL_RIKU_STATE_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_DARK_CARD_BROKEN:
        if (work->flags & BTL_RIKU_FLAG_HIDDEN) {
            break;
        }

        FocusBtlRikuCameraOnTarget(work);

        if (work->stateTimer == 0) {
            SetBtlRikuAnimation(work, 14, 0);
        }

        FocusBtlRikuCameraOnTarget(work);

        if (AnimIsFinished(&work->anim)) {
            ClearBtlObjActionFlags(act);
            work->state = BTL_RIKU_STATE_DARK_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_DARK_LEAP:
        FocusBtlRikuCameraOnTarget(work);

        if (work->stateTimer == 0) {
            work->actor2 = PickBtlRikuTarget(work);
            SetBtlRikuAnimation(work, 24, 0);
            EnableBtlRikuPassThrough(work);
            enemy = work->actor2;

            if (enemy != NULL) {
                if (act->x < enemy->x) {
                    act->flags |= BTLOBJ_FLAG_FACING_LEFT;
                } else {
                    act->flags &= ~BTLOBJ_FLAG_FACING_LEFT;
                }

                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    work->targetX = enemy->x + 0x2D00;
                } else {
                    work->targetX = enemy->x - 0x2D00;
                }

                work->targetY = enemy->y;
            } else {
                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    work->targetX = act->x;
                } else {
                    work->targetX = act->x;
                }

                act->flags ^= BTLOBJ_FLAG_FACING_LEFT;
                work->targetY = act->y;
            }

            work->vz = -1152;
        }

        if (AnimGetFrame(&work->anim) != 0) {
            act->x += (work->targetX - act->x) >> 3;
            act->y += (work->targetY - act->y) >> 3;
            work->flags |= BTL_RIKU_FLAG_AFTERIMAGE;
        }

        if (AnimIsFinished(&work->anim) && act->z >= act->groundZ) {
            DisableBtlRikuPassThrough(work);
            work->state = BTL_RIKU_STATE_DARK_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_DODGE:
        FocusBtlRikuCameraOnTarget(work);

        if (work->stateTimer == 0) {
            work->speed = 0;
            work->unk_194 = 0x880;
            work->steps = 20;
            act->flags |= BTLOBJ_FLAG_CARD_USE_BLOCKED;
            work->vz = -819;

            if (GetRandom() % 2) {
                m4aSongNumStart(SONG_VO_RK_ATTACK01);
            } else {
                m4aSongNumStart(SONG_VO_RK_ATTACK03);
            }

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                work->angle = 192;
            } else {
                work->angle = 64;
            }
        }

        if (work->vz < 0) {
            if (work->vz <= -512) {
                SetBtlRikuDirAnimation(work, 2, 1);
            } else {
                SetBtlRikuDirAnimation(work, 3, 1);
            }
        } else if (work->vz <= 511) {
            SetBtlRikuDirAnimation(work, 3, 1);
        } else {
            SetBtlRikuDirAnimation(work, 4, 1);
        }

        if (work->stateTimer == 4) {
            EnableBtlRikuPassThrough(work);
            act->flags |= BTLOBJ_FLAG_HIT_LOCKED;
        }

        if (work->steps != 0) {
            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                act->x += work->unk_194;
            } else {
                act->x -= work->unk_194;
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
        }

        if (work->steps == 0 && act->z >= act->groundZ) {
            act->flags &= ~BTLOBJ_FLAG_HIT_LOCKED;
            DisableBtlRikuPassThrough(work);
            work->state = BTL_RIKU_STATE_DODGE_LAND;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_DODGE_LAND:
        if (work->stateTimer == 0) {
            m4aSongNumStart(work->groundSongs[3]);
            SetBtlRikuDirAnimation(work, 5, 0);
        }

        if (!AnimIsFinished(&work->anim)) {
            work->stateTimer++;
            break;
        }

        work->state = BTL_RIKU_STATE_IDLE;
        work->steps = 0;
        work->stateTimer = 0;
        act->flags &= ~BTLOBJ_FLAG_CARD_USE_BLOCKED;
        break;
    case BTL_RIKU_STATE_DARK_DASH:
        FocusBtlRikuCameraOnTarget(work);

        if (work->stateTimer == 0) {
            work->speed = 0;
            work->unk_194 = 0x1080;
            work->steps = 20;
            work->vz = -819;
            m4aSongNumStart(GetRandom() % 2 + SONG_VO_RK_ATTACK04);
        }

        if (work->vz < 0) {
            if (work->vz <= -512) {
                SetBtlRikuAnimation(work, 16, 0);
            } else {
                SetBtlRikuAnimation(work, 17, 0);
            }
        } else if (work->vz <= 511) {
            SetBtlRikuAnimation(work, 17, 0);
        } else {
            SetBtlRikuAnimation(work, 18, 0);
        }

        if (work->stateTimer == 4) {
            EnableBtlRikuPassThrough(work);
        }

        work->flags |= BTL_RIKU_FLAG_AFTERIMAGE;

        if (work->steps != 0) {
            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                act->x += work->unk_194;
            } else {
                act->x -= work->unk_194;
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
        }

        if (work->steps == 0 && act->z >= act->groundZ) {
            DisableBtlRikuPassThrough(work);
            work->state = BTL_RIKU_STATE_DARK_DASH_LAND;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_DARK_DASH_LAND:
        if (work->stateTimer == 0) {
            m4aSongNumStart(work->groundSongs[3]);
            SetBtlRikuAnimation(work, 19, 0);
        }

        if (!AnimIsFinished(&work->anim)) {
            work->stateTimer++;
            break;
        }

        work->state = BTL_RIKU_STATE_DARK_IDLE;
        work->steps = 0;
        work->stateTimer = 0;
        break;
    case BTL_RIKU_STATE_DARK_DASH_VERTICAL:
        FocusBtlRikuCameraOnTarget(work);

        if (work->stateTimer == 0) {
            work->speed = 0;
            work->unk_194 = 0x580;
            work->steps = 20;
            work->vz = -819;
            m4aSongNumStart(GetRandom() % 2 + SONG_VO_RK_ATTACK04);
        }

        if (work->vz < 0) {
            if (work->vz <= -512) {
                SetBtlRikuAnimation(work, 16, 0);
            } else {
                SetBtlRikuAnimation(work, 17, 0);
            }
        } else if (work->vz <= 511) {
            SetBtlRikuAnimation(work, 17, 0);
        } else {
            SetBtlRikuAnimation(work, 18, 0);
        }

        if (work->stateTimer == 4) {
            EnableBtlRikuPassThrough(work);
        }

        work->flags |= BTL_RIKU_FLAG_AFTERIMAGE;

        if (work->steps != 0) {
            if (work->flags & BTL_RIKU_FLAG_DASH_UP) {
                act->y -= work->unk_194;
            } else {
                act->y += work->unk_194;
            }

            ApproachValue(&work->unk_194, 0, work->steps);
            work->steps--;

            if (act->z < act->groundZ) {
                if (held & DPAD_UP) {
                    act->x -= 640;
                } else if (held & DPAD_DOWN) {
                    act->x += 640;
                }
            }
        }

        if (work->steps == 0 && act->z >= act->groundZ) {
            DisableBtlRikuPassThrough(work);
            work->state = BTL_RIKU_STATE_DARK_DASH_LAND;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_GUARDED:
        FocusBtlRikuCameraOnTarget(work);

        if (work->stateTimer == 0) {
            SetBtlRikuAnimation(work, 9, 0);
        }

        if (AnimIsFinished(&work->anim)) {
            ClearBtlObjActionFlags(act);
            work->state = BTL_RIKU_STATE_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_ITEM:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlRikuCameraOnTarget(work);
            break;
        }

        FocusBtlRikuCameraOnTarget(work);

#ifdef VERSION_EU
        if (gBtlWork->flags & BTL_FLAG_DARK_MODE) {
            SetBtlRikuState(work, BTL_RIKU_STATE_DARK_IDLE);
            break;
        }
#endif

        if (work->stateTimer == 0) {
            SetBtlRikuAnimation(work, 10, 0);
        }

        if (work->stateTimer == 23) {
            BgFxStartPotion(act->x, act->y, act->z);
        }

        if (work->stateTimer > 23 && !BgFxIsActive()) {
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

            SetBtlRikuState(work, BTL_RIKU_STATE_IDLE);
            break;
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_COMBO: {
        s32 attack;
        s32 dy;
        hit = ATTACK_RESULT_NONE;
        FocusBtlRikuCameraOnTarget(work);

        if (act->btl->hcEffect == HC_EFFECT_COMBO_PLUS) {
            if ((work->flags & BTL_RIKU_FLAG_COMBO_EXTENDED) == 0) {
                if (work->comboCount == 2) {
                    work->comboCount = 1;
                    work->flags |= BTL_RIKU_FLAG_COMBO_EXTENDED;
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

        if (work->stateTimer == 0) {
            MakeOpponentsHittable();
            SetBtlRikuAnimation(work, swing->animId, 0);

            if (work->comboCount == 2) {
                m4aSongNumStart(GetRandom() % 2 + SONG_VO_RK_ATTACK05);
            } else {
                m4aSongNumStart(GetRandom() % 3 + SONG_VO_RK_ATTACK00);
            }

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
        } else if (work->stateTimer == swing->hitFrame) {
            if (act->btl->hcEffect == HC_EFFECT_WIDE_ATTACK_2) {
                switch (swing->animId) {
                case 5:
                    if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                        hit = ApplyAttackBox(swing->attackIds[0], act->x - 5120, act->y, act->z - 7168, 40, 24, 44);
                    } else {
                        hit = ApplyAttackBox(swing->attackIds[0], act->x + 5120, act->y, act->z - 7168, 40, 24, 44);
                    }

                    break;
                case 3:
                    if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                        hit = ApplyAttackBox(swing->attackIds[0], act->x - 8192, act->y, act->z, 32, 35, 32);
                    } else {
                        hit = ApplyAttackBox(swing->attackIds[0], act->x + 8192, act->y, act->z, 32, 35, 32);
                    }

                    break;
                default:
                    if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                        hit = ApplyAttackBox(swing->attackIds[0], act->x - 9216, act->y, act->z, 32, 20, 32);
                    } else {
                        hit = ApplyAttackBox(swing->attackIds[0], act->x + 9216, act->y, act->z, 32, 20, 32);
                    }

                    break;
                }
            } else {
                if (act->btl->hcEffect == HC_EFFECT_WARP_BREAK && work->comboCount == 2) {
                    if (GetRandom() % 2 != 0) {
                        attack = 164;
                    } else {
                        CreateBtlPopTask(act, BTL_POP_MISS);
                        attack = swing->attackIds[0];
                    }
                } else {
                    attack = swing->attackIds[0];
                }

                switch (swing->animId) {
                case 5:
                    if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                        hit = ApplyAttackBox(attack, act->x - 5120, act->y, act->z - 7168, 28, 20, 44);
                    } else {
                        hit = ApplyAttackBox(attack, act->x + 5120, act->y, act->z - 7168, 28, 20, 44);
                    }

                    break;
                case 3:
                    if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                        hit = ApplyAttackBox(attack, act->x - 8192, act->y, act->z, 20, 26, 32);
                    } else {
                        hit = ApplyAttackBox(attack, act->x + 8192, act->y, act->z, 20, 26, 32);
                    }

                    break;
                default:
                    if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                        hit = ApplyAttackBox(attack, act->x - 9216, act->y, act->z, 20, 16, 32);
                    } else {
                        hit = ApplyAttackBox(attack, act->x + 9216, act->y, act->z, 20, 16, 32);
                    }

                    break;
                }

                gBtlWork->damageScale = 0;
            }

            if (hit == ATTACK_RESULT_HIT) {
                m4aSongNumStart(swing->song);

                if (swing->flags & COMBO_FLAG_ZOOM_ON_HIT) {
                    if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                        SetBattleZoom(8, Q_8_8(1.5), act->x - 5120, (act->y - 5120) + act->z);
                    } else {
                        SetBattleZoom(8, Q_8_8(1.5), act->x + 5120, (act->y - 5120) + act->z);
                    }
                }

                work->flags |= BTL_RIKU_FLAG_SWING_HIT;
            } else {
                work->flags &= ~BTL_RIKU_FLAG_SWING_HIT;
            }
        } else if (work->stateTimer == swing->hitFrame + 2) {
            if (swing->flags & COMBO_FLAG_ZOOM_ON_HIT) {
                SetBattleZoom(15, Q_8_8(1), gBtlWork->x2, gBtlWork->y2);
            }

            if (work->comboCount <= 1) {
                if (work->flags & BTL_RIKU_FLAG_SWING_HIT) {
                    act->flags &= ~BTLOBJ_FLAG_IN_CARD_ACTION;
                    MakeOpponentsHittable();
                }
            }
        }

        if (hit == ATTACK_RESULT_BLOCKED) {
            SetBattleZoom(8, Q_8_8(1), gBtlWork->x2, gBtlWork->y2);
            SetBtlRikuState(work, BTL_RIKU_STATE_GUARDED);
            act->flags |= BTLOBJ_FLAG_CARD_USE_BLOCKED;
            break;
        }

        if (AnimIsFinished(&work->anim) && (act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) == 0) {
            SetBtlRikuState(work, BTL_RIKU_STATE_IDLE);
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
                    climb = (enemy->z - (enemy->centerHeight << 8)) - act->z;

                    if (climb < 0) {
                        act->btl->flags |= BTL_FLAG_PLAYER_AIRBORNE;
                        act->z += ((act->originZ + climb) - act->z) >> 3;
                        work->vz = 0;
                    }
                }
            } else {
                if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                    targetX = act->originX - 4096;
                    act->x += (targetX - act->x) >> 3;
                } else {
                    targetX = act->originX + 4096;
                    act->x += (targetX - act->x) >> 3;
                }
            }
        }

        work->stateTimer++;
        break;
    }
    case BTL_RIKU_STATE_SUMMON_TAKEOFF:
        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) || act->z < act->groundZ) {
            FocusBtlRikuCameraOnTarget(work);
            break;
        }

        FocusBtlRikuCameraOnTarget(work);

        if (work->stateTimer == 0) {
            act->btl->actor2 = NULL;

            if (gBtlWork->flags & BTL_FLAG_DARK_MODE) {
                SetBtlRikuAnimation(work, 15, 0);
            } else {
                SetBtlRikuDirAnimation(work, 1, 0);
            }

            if (act->flags & BTLOBJ_FLAG_FACING_LEFT) {
                work->angle = 192;
            } else {
                work->angle = 64;
            }
        }

        elapsed = work->stateTimer;

        if (work->stateTimer > 3) {
            work->state = BTL_RIKU_STATE_SUMMON_EXIT;
            work->steps = 0;
            work->stateTimer = 0;
            work->vz = -1024;
        } else {
            work->stateTimer = elapsed + 1;
        }

        break;
    case BTL_RIKU_STATE_SUMMON_EXIT:
        if (work->stateTimer == 0) {
            work->steps = 20;
            ColliderSetDisabled(&act->collider, TRUE);
            act->flags |= BTLOBJ_FLAG_IGNORE_BOUNDS;
            act->flags |= BTLOBJ_FLAG_NO_BREAK_POP;
            act->originZ = act->z;
        }

        switch (work->stateTimer) {
        case 0:
            if (gBtlWork->flags & BTL_FLAG_DARK_MODE) {
                SetBtlRikuAnimation(work, 16, 0);
            } else {
                SetBtlRikuDirAnimation(work, 2, 0);
            }

            break;
        case 10:
            if (gBtlWork->flags & BTL_FLAG_DARK_MODE) {
                SetBtlRikuAnimation(work, 17, 0);
            } else {
                SetBtlRikuDirAnimation(work, 3, 0);
            }

            break;
        }

        if (act->originX < 0x10000) {
            ApproachValue(&act->x, -8192, work->steps);
        } else {
            ApproachValue(&act->x, 0x22000, work->steps);
        }

        work->steps--;

        if (work->steps == 0) {
            work->state = BTL_RIKU_STATE_SUMMON_OFFSCREEN;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_SUMMON_OFFSCREEN:
        if (work->stateTimer == 0) {
            work->flags |= BTL_RIKU_FLAG_HIDDEN;
            ReleaseBtlRikuPalette(work);
            spawn.mainSide = work->mainSide;
            spawn.variant = work->variant[0];
            TaskCreate(&gBtlWork->taskPools[0], work->summonDesc, &spawn);
            act->x = act->originX;
            act->y = act->originY;
            act->z = -65536;
        }

        work->vz = 0;

        if ((act->btl->flags & BTL_FLAG_SUMMON_ACTIVE) == 0) {
            work->flags &= ~BTL_RIKU_FLAG_HIDDEN;
            LoadBtlRikuPalette(work);

            if (act->originX < 0x10000) {
                act->x = -8192;
            } else {
                act->x = 0x22000;
            }

            act->z = act->originZ - 12800;
            act->btl->flags |= BTL_FLAG_PLAYER_AIRBORNE;
            work->vz = 0;

            if (gBtlWork->flags & BTL_FLAG_DARK_MODE) {
                SetBtlRikuAnimation(work, 12, 1);
            } else {
                SetBtlRikuAnimation(work, 0, 1);
            }

            work->state = BTL_RIKU_STATE_SUMMON_RETURN;
            work->steps = 0;
            work->stateTimer = 0;
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_RIKU_STATE_SUMMON_RETURN:
        if (work->stateTimer == 0) {
            work->steps = 20;
        }

        switch (work->stateTimer) {
        case 0:
            if (gBtlWork->flags & BTL_FLAG_DARK_MODE) {
                SetBtlRikuAnimation(work, 17, 0);
            } else {
                SetBtlRikuDirAnimation(work, 3, 0);
            }

            break;
        case 10:
            if (gBtlWork->flags & BTL_FLAG_DARK_MODE) {
                SetBtlRikuAnimation(work, 18, 0);
            } else {
                SetBtlRikuDirAnimation(work, 4, 0);
            }

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

        if ((act->btl->flags & BTL_FLAG_PLAYER_AIRBORNE) == 0) {
            work->state = BTL_RIKU_STATE_SUMMON_RETURN_LAND;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_SUMMON_RETURN_LAND:
        if (work->stateTimer == 0) {
            ColliderSetDisabled(&act->collider, FALSE);
            act->flags &= ~BTLOBJ_FLAG_IGNORE_BOUNDS;
            act->flags &= ~BTLOBJ_FLAG_NO_BREAK_POP;

            if (gBtlWork->flags & BTL_FLAG_DARK_MODE) {
                SetBtlRikuAnimation(work, 19, 0);
            } else {
                SetBtlRikuDirAnimation(work, 5, 0);
            }
        }

        elapsed = work->stateTimer;

        if (work->stateTimer > 6) {
            if (gBtlWork->flags & BTL_FLAG_DARK_MODE) {
                SetBtlRikuState(work, BTL_RIKU_STATE_DARK_IDLE);
            } else {
                SetBtlRikuState(work, BTL_RIKU_STATE_IDLE);
            }

            break;
        }

        work->stateTimer = elapsed + 1;
        break;
    case BTL_RIKU_STATE_HAZARD: {
        u16 prev;

        FocusBtlRikuCameraOnTarget(work);

        if (work->stateTimer == 0) {
            FadeFromAmount(FADE_MODE_RED, 4, 10);
            SetBtlRikuAnimation(work, 7, 0);
            act->flags |= BTLOBJ_FLAG_HURT;
            work->speed = 0;
            prev = act->hp;

            if (act->hp > 1) {
                act->hp = prev - 1;
            }

            switch (GetRandom() % 3) {
            case 0:
                m4aSongNumStart(SONG_VO_RK_DAMAGE00);
                break;
            case 1:
                m4aSongNumStart(SONG_VO_RK_DAMAGE01);
                break;
            case 2:
            default:
                m4aSongNumStart(SONG_VO_RK_DAMAGE02);
                break;
            }
        }

        prev = work->stateTimer;

        if (work->stateTimer > 24) {
            act->flags &= ~BTLOBJ_FLAG_HURT;
            work->state = BTL_RIKU_STATE_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
        } else {
            work->stateTimer = prev + 1;
        }

        break;
    }
    case BTL_RIKU_STATE_DARK_HAZARD: {
        u16 prev;

        FocusBtlRikuCameraOnTarget(work);

        if (work->stateTimer == 0) {
            FadeFromAmount(FADE_MODE_RED, 4, 10);
            SetBtlRikuAnimation(work, 14, 0);
            act->flags |= BTLOBJ_FLAG_HURT;
            work->speed = 0;
            prev = act->hp;

            if (act->hp > 1) {
                act->hp = prev - 1;
            }

            switch (GetRandom() % 3) {
            case 0:
                m4aSongNumStart(SONG_VO_RK_DAMAGE00);
                break;
            case 1:
                m4aSongNumStart(SONG_VO_RK_DAMAGE01);
                break;
            case 2:
            default:
                m4aSongNumStart(SONG_VO_RK_DAMAGE02);
                break;
            }
        }

        prev = work->stateTimer;

        if (work->stateTimer > 24) {
            act->flags &= ~BTLOBJ_FLAG_HURT;
            work->state = BTL_RIKU_STATE_DARK_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
        } else {
            work->stateTimer = prev + 1;
        }

        break;
    }
    case BTL_RIKU_STATE_HURT:
        FocusBtlRikuCameraOnTarget(work);
        act->originX = act->x;
        act->originY = act->y;

        if (work->stateTimer == 0) {
            AnimReset(&work->anim);
            SetBtlRikuAnimation(work, 7, 0);
            gBtlWork->hitStop = gBtlWork->pendingHitStop;
            StartBtlRikuKnockback(work);

            if (gBtlWork->hcEffect == HC_EFFECT_QUICK_RECOVERY) {
                act->btl->hcEffectCount--;
                work->steps = 0;
            } else {
                work->steps = 15;
            }
        } else if (work->stateTimer == 6) {
            switch (GetRandom() % 3) {
            case 0:
                m4aSongNumStart(SONG_VO_RK_DAMAGE00);
                break;
            case 1:
                m4aSongNumStart(SONG_VO_RK_DAMAGE01);
                break;
            case 2:
            default:
                m4aSongNumStart(SONG_VO_RK_DAMAGE02);
                break;
            }
        }

        if (work->stateTimer >= work->steps) {
            act->flags &= ~BTLOBJ_FLAG_CARD_USE_BLOCKED;
        }

        if (AnimIsFinished(&work->anim)) {
            ClearBtlObjActionFlags(act);
            act->originX = act->x;
            act->originY = act->y;
            work->state = BTL_RIKU_STATE_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
        break;
    case BTL_RIKU_STATE_DARK_RECOVER:
        if (AnimIsFinished(&work->anim)) {
            work->state = BTL_RIKU_STATE_DARK_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
        }

        break;
    case BTL_RIKU_STATE_RECOVER:
        if (AnimIsFinished(&work->anim)) {
            work->state = BTL_RIKU_STATE_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
        }

        break;
    case BTL_RIKU_STATE_REVIVE:
        if (work->stateTimer == 0) {
            StartBtlRikuKnockback(work);
            work->flags |= BTL_RIKU_FLAG_PASS_THROUGH;
            SetBtlRikuAnimation(work, 8, 0);
            m4aSongNumStart(SONG_VO_RK_DEATH00);
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
            gBtlWork->hitStop = 3;

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

            if (gBtlWork->enemyCount == 0 && gBtlWork->pendingEnemies <= 0) {
                work->state = BTL_RIKU_STATE_END_BATTLE;
                work->steps = 0;
                work->stateTimer = 0;
                break;
            }

            work->state = BTL_RIKU_STATE_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
            break;
        }

        work->stateTimer++;
        break;
    case BTL_RIKU_STATE_END_BATTLE: {
        u16 elapsed;

        SetBtlRikuAnimation(work, 0, 0);
        elapsed = work->stateTimer;

        if (work->stateTimer > 60) {
            gBtlWork->flags |= BTL_FLAG_BATTLE_OVER;
            work->state = BTL_RIKU_STATE_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
        } else {
            work->stateTimer = elapsed + 1;
        }

        break;
    }
    case BTL_RIKU_STATE_DEFEATED:
        FocusBtlRikuCameraOnTarget(work);

        if (work->stateTimer == 0) {
            gBtlWork->flags |= BTL_FLAG_PLAYER_DEFEATED;
            StartBtlRikuKnockback(work);
            work->flags |= BTL_RIKU_FLAG_PASS_THROUGH;
            SetBtlRikuAnimation(work, 8, 0);
            m4aSongNumStart(SONG_VO_RK_DEATH00);
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
            gBtlWork->hitStop = 3;

            if (work->vz > 0) {
                work->vz = 0;
            }
        }

        break;
    case BTL_RIKU_STATE_STOPPED:
        BtlMapFollowPosition(act->x, act->y, act->z);
        work->vz = 0;

        if (act->badStatus != BAD_STATUS_STOP) {
            work->state = BTL_RIKU_STATE_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
            ClearBtlObjActionFlags(act);
            work->flags &= ~BTL_RIKU_FLAG_PASS_THROUGH;
        } else {
            work->stateTimer++;
        }

        break;
    case BTL_RIKU_STATE_STUNNED:
        BtlMapFollowPosition(act->x, act->y, act->z);

        if (work->stateTimer == 0) {
            AnimReset(&work->anim);
            SetBtlRikuAnimation(work, 7, 0);
            work->stateTimer++;
            act->flags |= BTLOBJ_FLAG_CARD_USE_BLOCKED;
            StartBtlRikuKnockback(work);
            work->speed = 0;
        }

        if (AnimIsFinished(&work->anim)) {
            act->flags &= ~BTLOBJ_FLAG_HIT_LOCKED;
        }

        if ((A_BUTTON | B_BUTTON | DPAD_ANY) & pressed) {
            act->badStatusTimer -= 1;
        }

        if (act->badStatus != BAD_STATUS_STUN) {
            ClearBtlObjActionFlags(act);
            work->state = BTL_RIKU_STATE_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
        }

        break;
    case BTL_RIKU_STATE_DARK_STUNNED:
        BtlMapFollowPosition(act->x, act->y, act->z);

        if (work->stateTimer == 0) {
            AnimReset(&work->anim);
            SetBtlRikuAnimation(work, 14, 0);
            work->stateTimer++;
            act->flags |= BTLOBJ_FLAG_CARD_USE_BLOCKED;
            StartBtlRikuKnockback(work);
            work->speed = 0;
        }

        if (AnimIsFinished(&work->anim)) {
            act->flags &= ~BTLOBJ_FLAG_HIT_LOCKED;
        }

        if ((A_BUTTON | B_BUTTON | DPAD_ANY) & pressed) {
            act->badStatusTimer -= 1;
        }

        if (act->badStatus != BAD_STATUS_STUN) {
            ClearBtlObjActionFlags(act);
            work->state = BTL_RIKU_STATE_DARK_IDLE;
            work->steps = 0;
            work->stateTimer = 0;
        }

        break;
    case BTL_RIKU_STATE_FIELD_HIDDEN:
        act->flags &= ~BTLOBJ_FLAG_HURT;
#ifdef VERSION_EU
        act->vx = act->vy = 0;
        work->speed = 0;
#endif
        break;
    }

    if (pressed & DPAD_LEFT) {
        work->tapTimers[0] = 13;
        work->tapTimers[1] = 0;
    } else if (pressed & DPAD_RIGHT) {
        work->tapTimers[1] = 13;
        work->tapTimers[0] = 0;
    } else if (pressed & DPAD_UP) {
        work->tapTimers[2] = 13;
        work->tapTimers[3] = 0;
    } else if (pressed & DPAD_DOWN) {
        work->tapTimers[3] = 13;
        work->tapTimers[2] = 0;
    }

    if (work->tapTimers[1] != 0) {
        work->tapTimers[1]--;
    }

    if (work->tapTimers[0] != 0) {
        work->tapTimers[0]--;
    }

    if (work->tapTimers[2] != 0) {
        work->tapTimers[2]--;
    }

    if (work->tapTimers[3] != 0) {
        work->tapTimers[3]--;
    }

    if (act->badStatus != BAD_STATUS_BIND) {
        act->x += gSineTable[work->angle] * work->speed >> 8;
        act->y += -gSineTable[work->angle + 64] * (work->speed >> 1) >> 8;
    }

    if (act->collider.colliding && act->collider.otherType != COLLIDER_TYPE_SLIPPERY) {
        if ((work->flags & BTL_RIKU_FLAG_ON_PLATFORM) && act->collider.otherType == COLLIDER_TYPE_PLATFORM) {
            act->collider.standFlags |= COLLIDER_STAND_OVER_PLATFORM;
        } else if (!(work->flags & BTL_RIKU_FLAG_PASS_THROUGH)) {
            act->x += act->collider.pushX >> 1;
            act->y += act->collider.pushY >> 1;
        }
    }

    if (!(act->flags & BTLOBJ_FLAG_IGNORE_BOUNDS)) {
        ApplyBattleBounds(&act->x, &act->y, &act->z, &act->floorZ);
    }

    act->z += work->vz;
    work->vz += gBtlWork->gravity;

    if (act->collider.standFlags & COLLIDER_STAND_OVER_PLATFORM) {
        act->groundZ = act->collider.platformZ;
        work->flags |= BTL_RIKU_FLAG_OVER_PLATFORM;
        work->platformPriority = -4100 - ((act->collider.platformY + 0x400) >> 8) * 4;
    } else {
        work->flags &= ~BTL_RIKU_FLAG_OVER_PLATFORM;
        act->groundZ = act->floorZ;
    }

    if ((work->flags & BTL_RIKU_FLAG_ON_PLATFORM) && gBtlWork->platform == act->collider.other) {
        act->x += act->collider.platformX - work->platformX;
        act->y += act->collider.platformY - work->platformY;
        act->z += act->collider.platformZ - work->platformZ;
    }

    if (act->z >= act->groundZ) {
        if (act->collider.standFlags & COLLIDER_STAND_OVER_PLATFORM) {
            work->flags |= BTL_RIKU_FLAG_ON_PLATFORM;
            work->platformX = act->collider.platformX;
            work->platformY = act->collider.platformY;
            work->platformZ = act->collider.platformZ;
            gBtlWork->platform = act->collider.other;
        } else {
            work->flags &= ~BTL_RIKU_FLAG_ON_PLATFORM;
            gBtlWork->platform = NULL;
        }

        work->vz = 0;
        act->z = act->groundZ;
        act->btl->flags &= ~BTL_FLAG_PLAYER_AIRBORNE;

        if (work->state == BTL_RIKU_STATE_AIRBORNE) {
            work->state = BTL_RIKU_STATE_LAND;
            work->steps = 0;
            work->stateTimer = 0;
        } else if (work->state == BTL_RIKU_STATE_DARK_AIRBORNE) {
            work->state = BTL_RIKU_STATE_DARK_LAND;
            work->steps = 0;
            work->stateTimer = 0;
        }
    } else {
        if (work->flags & BTL_RIKU_FLAG_ON_PLATFORM) {
            work->flags &= ~BTL_RIKU_FLAG_ON_PLATFORM;

            if (work->state == BTL_RIKU_STATE_IDLE) {
                if (gBtlWork->flags & BTL_FLAG_DARK_MODE) {
                    work->state = BTL_RIKU_STATE_DARK_AIRBORNE;
                    work->steps = 0;
                    work->stateTimer = 0;
                } else {
                    work->state = BTL_RIKU_STATE_AIRBORNE;
                    work->steps = 0;
                    work->stateTimer = 0;
                }
            }
        }

        gBtlWork->platform = NULL;
    }

    vx = act->vx;

    if (vx > 0) {
        act->x += vx;
        act->vx -= 17;

        if (act->vx < 0) {
            act->vx = 0;
        }
    } else if (vx < 0) {
        act->x += vx;
        act->vx += 17;

        if (act->vx > 0) {
            act->vx = 0;
        }
    }

    vy = act->vy;

    if (vy > 0) {
        act->y += vy;
        act->vy -= 17;

        if (act->vy < 0) {
            act->vy = 0;
        }
    } else if (vy < 0) {
        act->y += vy;
        act->vy += 17;

        if (act->vy > 0) {
            act->vy = 0;
        }
    }

    if (!(act->flags & BTLOBJ_FLAG_IGNORE_BOUNDS)) {
        switch (ClampBattlePosition(&act->x, &act->y, -16, 0)) {
        case BATTLE_EDGE_LEFT:
            act->vx = 0;

            if (act->z == 0 && (held & DPAD_LEFT)) {
                act->btl->flags |= BTL_FLAG_PUSHING_EDGE;
            } else {
                act->btl->flags &= ~BTL_FLAG_PUSHING_EDGE;
            }

            work->flags |= BTL_RIKU_FLAG_AT_SIDE_EDGE;
            break;
        case BATTLE_EDGE_RIGHT:
            act->vx = 0;

            if (act->z == 0 && (held & DPAD_RIGHT)) {
                act->btl->flags |= BTL_FLAG_PUSHING_EDGE;
            } else {
                act->btl->flags &= ~BTL_FLAG_PUSHING_EDGE;
            }

            work->flags |= BTL_RIKU_FLAG_AT_SIDE_EDGE;
            break;
        case BATTLE_EDGE_TOP:
        case BATTLE_EDGE_BOTTOM:
            act->vy = 0;
            act->btl->flags &= ~BTL_FLAG_PUSHING_EDGE;
            break;
        default:
            act->btl->flags &= ~BTL_FLAG_PUSHING_EDGE;
            work->flags &= ~BTL_RIKU_FLAG_AT_SIDE_EDGE;
            break;
        }

        if (act->btl->flags & BTL_FLAG_SUMMON_ACTIVE) {
            act->btl->flags &= ~BTL_FLAG_PUSHING_EDGE;
        }

        if (act->btl->flags & BTL_FLAG_PUSHING_EDGE) {
            if (work->mainSide) {
                BtlMapFollowPosition(act->x, act->y, act->z);
            }
        }
    }

    TaskPoolUpdate(&work->tasks);

    if (work->state == BTL_RIKU_STATE_CARD_BROKEN || work->state == BTL_RIKU_STATE_DARK_CARD_BROKEN) {
        if (work->flags & BTL_RIKU_FLAG_HIDDEN) {
            work->flags &= ~BTL_RIKU_FLAG_HIDDEN;
            LoadBtlRikuPalette(work);
            SetBtlRikuAnimation(work, 9, 0);
            act->x = act->originX;
            act->y = act->originY;
            act->z = act->originZ;
        }
    }

    if (act->badStatus != BAD_STATUS_STOP) {
        work->gfx = AnimUpdate(&work->anim);
    }

    ColliderSetPosition(&act->collider, act->x, act->y, act->z);
    work->frameCount++;

    return 1;
}

void task_btl_riku_2(BtlRikuWork* work) {
    BtlObj* act;
    ObjAffine* affine;
    s32 sx;
    s32 sy;
    u16 attr;
    u16 priority;
    s16 x;
    s16 y;

    act = &work->actor;

    if (work->flags & BTL_RIKU_FLAG_HIDDEN) {
        return;
    }

    if (work->actor.btl->hcEffect == HC_EFFECT_VANISH) {
        if (work->mainSide) {
            if (gFrameCounter & 1) {
                return;
            }
        } else if (gFrameCounter % 120 <= 59) {
            return;
        }
    }

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

    if (work->flags & BTL_RIKU_FLAG_OVER_PLATFORM) {
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
        u16 flags = work->flags | BTL_RIKU_FLAG_HIT_FLASH;

        work->flags = flags;
        LoadObjPaletteBank(work->palette->index, gHitFlashPalette);
    } else if (work->flags & BTL_RIKU_FLAG_HIT_FLASH) {
        u16 flags = work->flags & ~BTL_RIKU_FLAG_HIT_FLASH;

        work->flags = flags;

        if (work->mainSide) {
            LoadObjPaletteBank(work->palette->index, work->paletteData);
        } else {
            LoadObjPaletteBank(work->palette->index, gBtlOtherSidePalette);
        }
    }

    DrawSprite(x, y, work->gfx, work->tiles2, work->palette, affine, attr, priority);

    if (work->flags & BTL_RIKU_FLAG_AFTERIMAGE) {
        switch (work->drawCount % 2) {
        case 0:
            DrawBtlRikuAfterimage(work, &work->drawInfo[3]);
            break;
        case 1:
            DrawBtlRikuAfterimage(work, &work->drawInfo[6]);
            break;
        }

        work->drawCount++;
    }

    work->drawInfo[6] = work->drawInfo[5];
    work->drawInfo[5] = work->drawInfo[4];
    work->drawInfo[4] = work->drawInfo[3];
    work->drawInfo[3] = work->drawInfo[2];
    work->drawInfo[2] = work->drawInfo[1];
    work->drawInfo[1] = work->drawInfo[0];
    SaveBtlRikuAfterimage(work, &work->drawInfo[0]);
    TaskPoolDraw(&work->tasks);
}

void task_btl_riku_3(BtlRikuWork* work) {
    BtlObj* act;

    act = &work->actor;

    if (gBtlWork->phase == BTL_PHASE_GAME_OVER) {
        gGameState.hp = gGameState.progression.maxHp;
    } else {
        gGameState.hp = act->hp;
    }

    ColliderUnregister(&act->collider);
    ReleaseBtlRikuPalette(work);
    ReleaseObjTiles(work->tiles);
    TaskPoolDestroy(&work->tasks);
}

TaskDesc gTaskDescBtlRiku = {
    "task_btl_riku",
    (TaskInitFunc)task_btl_riku_0,
    (TaskUpdateFunc)task_btl_riku_1,
    (TaskDrawFunc)task_btl_riku_2,
    (TaskDestroyFunc)task_btl_riku_3,
    sizeof(BtlRikuWork),
};
