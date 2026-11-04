/**
 * fld_riku.c
 * Riku Field Character
 */

#include "task_descriptors.h"
#include "map_api.h"
#include "fld.h"
#include "gba/keys.h"
#include "sprites_evt.h"
#include "sprites_fld.h"
#include "sprites_riku.h"
#include "world_types.h"
#include "fade.h"
#include "songs.h"
#include <stdlib.h>
#include "fld_tasks.h"
#include "anim.h"
#include "battle_actor_types.h"
#include "btl_collision.h"
#include "engine_math.h"
#include "field_state.h"
#include "fld_types.h"
#include "game_state.h"
#include "key.h"
#include "m4a_song.h"
#include "map_runtime.h"
#include "map_types.h"
#include "obj_api.h"
#include <stddef.h>
#include "taskpool.h"
#include "types.h"
#include "sprite_palettes.h"
#include "obj.h"

static const AnimDef sFldRikuAnimDefs[15][5] = {
    { { gRikuBb00Frames, gRikuBb00Anims, gRikuBb00Tiles, 0 }, { gRikuFf00Frames, gRikuFf00Anims, gRikuFf00Tiles, 0 }, { gRikuFl00Frames, gRikuFl00Anims, gRikuFl00Tiles, 0 }, { gRikuLl00Frames, gRikuLl00Anims, gRikuLl00Tiles, 0 }, { gRikuBl00Frames, gRikuBl00Anims, gRikuBl00Tiles, 0 } },
    { { gRik1bb01Frames, gRik1bb01Anims, gRik1bb01Tiles, 0 }, { gRik1ff01Frames, gRik1ff01Anims, gRik1ff01Tiles, 0 }, { gRik1fl01Frames, gRik1fl01Anims, gRik1fl01Tiles, 0 }, { gRik1ll01Frames, gRik1ll01Anims, gRik1ll01Tiles, 0 }, { gRik1bl01Frames, gRik1bl01Anims, gRik1bl01Tiles, 0 } },
    { { gRik1bb02Frames, gRik1bb02Anims, gRik1bb02Tiles, 0 }, { gRik1ff02Frames, gRik1ff02Anims, gRik1ff02Tiles, 0 }, { gRik1fl02Frames, gRik1fl02Anims, gRik1fl02Tiles, 0 }, { gRik1ll02Frames, gRik1ll02Anims, gRik1ll02Tiles, 0 }, { gRik1bl02Frames, gRik1bl02Anims, gRik1bl02Tiles, 0 } },
    { { gRik1bb03Frames, gRik1bb03Anims, gRik1bb03Tiles, 0 }, { gRik1ff03Frames, gRik1ff03Anims, gRik1ff03Tiles, 0 }, { gRik1fl03Frames, gRik1fl03Anims, gRik1fl03Tiles, 0 }, { gRik1ll03Frames, gRik1ll03Anims, gRik1ll03Tiles, 0 }, { gRik1bl03Frames, gRik1bl03Anims, gRik1bl03Tiles, 0 } },
    { { gRik1bb03Frames, gRik1bb03Anims, gRik1bb03Tiles, 1 }, { gRik1ff03Frames, gRik1ff03Anims, gRik1ff03Tiles, 1 }, { gRik1fl03Frames, gRik1fl03Anims, gRik1fl03Tiles, 1 }, { gRik1ll03Frames, gRik1ll03Anims, gRik1ll03Tiles, 1 }, { gRik1bl03Frames, gRik1bl03Anims, gRik1bl03Tiles, 1 } },
    { { gRik1bb03Frames, gRik1bb03Anims, gRik1bb03Tiles, 2 }, { gRik1ff03Frames, gRik1ff03Anims, gRik1ff03Tiles, 2 }, { gRik1fl03Frames, gRik1fl03Anims, gRik1fl03Tiles, 2 }, { gRik1ll03Frames, gRik1ll03Anims, gRik1ll03Tiles, 2 }, { gRik1bl03Frames, gRik1bl03Anims, gRik1bl03Tiles, 2 } },
    { { gRik1bb03Frames, gRik1bb03Anims, gRik1bb03Tiles, 3 }, { gRik1ff03Frames, gRik1ff03Anims, gRik1ff03Tiles, 3 }, { gRik1fl03Frames, gRik1fl03Anims, gRik1fl03Tiles, 3 }, { gRik1ll03Frames, gRik1ll03Anims, gRik1ll03Tiles, 3 }, { gRik1bl03Frames, gRik1bl03Anims, gRik1bl03Tiles, 3 } },
    { { gRik1bb03Frames, gRik1bb03Anims, gRik1bb03Tiles, 4 }, { gRik1ff03Frames, gRik1ff03Anims, gRik1ff03Tiles, 4 }, { gRik1fl03Frames, gRik1fl03Anims, gRik1fl03Tiles, 4 }, { gRik1ll03Frames, gRik1ll03Anims, gRik1ll03Tiles, 4 }, { gRik1bl03Frames, gRik1bl03Anims, gRik1bl03Tiles, 4 } },
    { { gRikuClimbFrames, gRikuClimbAnims, gRikuClimbTiles, 0 }, { gRikuClimbFrames, gRikuClimbAnims, gRikuClimbTiles, 0 }, { gRikuClimbFrames, gRikuClimbAnims, gRikuClimbTiles, 0 }, { gRikuClimbFrames, gRikuClimbAnims, gRikuClimbTiles, 0 }, { gRikuClimbFrames, gRikuClimbAnims, gRikuClimbTiles, 0 } },
    { { gRikuLedgeBackFrames, gRikuLedgeBackAnims, gRikuLedgeBackTiles, 0 }, { gRikuLedgeFrames, gRikuLedgeAnims, gRikuLedgeTiles, 0 }, { gRikuLedgeFrames, gRikuLedgeAnims, gRikuLedgeTiles, 0 }, { gRikuLedgeFrames, gRikuLedgeAnims, gRikuLedgeTiles, 0 }, { gRikuLedgeFrames, gRikuLedgeAnims, gRikuLedgeTiles, 0 } },
    { { gRikuLedgeBackFrames, gRikuLedgeBackAnims, gRikuLedgeBackTiles, 1 }, { gRikuLedgeFrames, gRikuLedgeAnims, gRikuLedgeTiles, 1 }, { gRikuLedgeFrames, gRikuLedgeAnims, gRikuLedgeTiles, 1 }, { gRikuLedgeFrames, gRikuLedgeAnims, gRikuLedgeTiles, 1 }, { gRikuLedgeFrames, gRikuLedgeAnims, gRikuLedgeTiles, 1 } },
    { { gRikuLedgeBackFrames, gRikuLedgeBackAnims, gRikuLedgeBackTiles, 2 }, { gRikuLedgeFrames, gRikuLedgeAnims, gRikuLedgeTiles, 2 }, { gRikuLedgeFrames, gRikuLedgeAnims, gRikuLedgeTiles, 2 }, { gRikuLedgeFrames, gRikuLedgeAnims, gRikuLedgeTiles, 2 }, { gRikuLedgeFrames, gRikuLedgeAnims, gRikuLedgeTiles, 2 } },
    { { gRikuCardPoseBackFrames, gRikuCardPoseBackAnims, gRikuCardPoseBackTiles, 0 }, { gRikuCardPoseFrames, gRikuCardPoseAnims, gRikuCardPoseTiles, 0 }, { gRikuCardPoseFrames, gRikuCardPoseAnims, gRikuCardPoseTiles, 0 }, { gRikuCardPoseFrames, gRikuCardPoseAnims, gRikuCardPoseTiles, 0 }, { gRikuCardPoseBackFrames, gRikuCardPoseBackAnims, gRikuCardPoseBackTiles, 0 } },
    { { gRik1bb10Frames, gRik1bb10Anims, gRik1bb10Tiles, 0 }, { gRik1ff10Frames, gRik1ff10Anims, gRik1ff10Tiles, 0 }, { gRik1fl10Frames, gRik1fl10Anims, gRik1fl10Tiles, 0 }, { gRikuBt11Frames, gRikuBt11Anims, gRikuBt11Tiles, 0 }, { gRik1bl10Frames, gRik1bl10Anims, gRik1bl10Tiles, 0 } },
    { { gRikuBb17Frames, gRikuBb17Anims, gRikuBb17Tiles, 0 }, { gRikuFf17Frames, gRikuFf17Anims, gRikuFf17Tiles, 0 }, { gRikuFl17Frames, gRikuFl17Anims, gRikuFl17Tiles, 0 }, { gRikuLl17Frames, gRikuLl17Anims, gRikuLl17Tiles, 0 }, { gRikuBl17Frames, gRikuBl17Anims, gRikuBl17Tiles, 0 } },
};

static const u16 sFldRikuSounds[8][8] = {
    { SONG_SYS_SR_STONEL, SONG_SYS_SR_STONER, SONG_SYS_SR_STONEJP, SONG_SYS_SR_STONELD, SONG_SYS_SR_STONEL, SONG_SYS_SR_STONER, SONG_SYS_SR_STONELJP, 0 },
    { SONG_SYS_SR_STONEL, SONG_SYS_SR_STONER, SONG_SYS_SR_STONEJP, SONG_SYS_SR_STONELD, SONG_SYS_SR_STONEL, SONG_SYS_SR_STONER, SONG_SYS_SR_STONELJP, 0 },
    { SONG_SYS_SR_MUDL, SONG_SYS_SR_MUDR, SONG_SYS_SR_MUDJP, SONG_SYS_SR_MUDLD, SONG_SYS_SR_MUDL, SONG_SYS_SR_MUDR, SONG_SYS_SR_MUDLJP, 0 },
    { SONG_SYS_SR_MUDL, SONG_SYS_SR_MUDR, SONG_SYS_SR_MUDJP, SONG_SYS_SR_MUDLD, SONG_SYS_SR_MUDL, SONG_SYS_SR_MUDR, SONG_SYS_SR_MUDLJP, 0 },
    { SONG_SYS_SR_FOOTL, SONG_SYS_SR_FOOTR, SONG_SYS_SR_JUMP, SONG_SYS_SR_LAND, SONG_SYS_SR_GRASSUP, SONG_SYS_SR_GRASSUP, SONG_SYS_SR_GRASSJP, 0 },
    { SONG_SYS_SR_STONEL, SONG_SYS_SR_STONER, SONG_SYS_SR_STONEJP, SONG_SYS_SR_STONELD, SONG_SYS_SR_STONEL, SONG_SYS_SR_STONER, SONG_SYS_SR_STONELJP, 0 },
    { SONG_SYS_SR_STONEL, SONG_SYS_SR_STONER, SONG_SYS_SR_STONEJP, SONG_SYS_SR_STONELD, SONG_SYS_SR_STONEL, SONG_SYS_SR_STONER, SONG_SYS_SR_STONELJP, 0 },
    { SONG_SYS_SR_FOOTL, SONG_SYS_SR_FOOTR, SONG_SYS_SR_JUMP, SONG_SYS_SR_LAND, SONG_SYS_SR_STONEL, SONG_SYS_SR_STONER, SONG_SYS_SR_STONELJP, 0 },
};

void FldRikuSetAngleFromDpad(FldActor* act) {
    if ((GetKeysHeld() & DPAD_LEFT) && (GetKeysHeld() & DPAD_DOWN)) {
        act->angle = FLD_ANGLE_DOWN_LEFT;
    } else if ((GetKeysHeld() & DPAD_UP) && (GetKeysHeld() & DPAD_LEFT)) {
        act->angle = FLD_ANGLE_UP_LEFT;
    } else if ((GetKeysHeld() & DPAD_UP) && (GetKeysHeld() & DPAD_RIGHT)) {
        act->angle = FLD_ANGLE_UP_RIGHT;
    } else if ((GetKeysHeld() & DPAD_RIGHT) && (GetKeysHeld() & DPAD_DOWN)) {
        act->angle = FLD_ANGLE_DOWN_RIGHT;
    } else if ((GetKeysHeld() & DPAD_DOWN) && GetKeyReleaseTime(DPAD_LEFT) <= 4) {
        act->angle = FLD_ANGLE_DOWN_LEFT;
    } else if ((GetKeysHeld() & DPAD_DOWN) && GetKeyReleaseTime(DPAD_RIGHT) <= 4) {
        act->angle = FLD_ANGLE_DOWN_RIGHT;
    } else if ((GetKeysHeld() & DPAD_UP) && GetKeyReleaseTime(DPAD_LEFT) <= 4) {
        act->angle = FLD_ANGLE_UP_LEFT;
    } else if ((GetKeysHeld() & DPAD_UP) && GetKeyReleaseTime(DPAD_RIGHT) <= 4) {
        act->angle = FLD_ANGLE_UP_RIGHT;
    } else if ((GetKeysHeld() & DPAD_LEFT) && GetKeyReleaseTime(DPAD_UP) <= 4) {
        act->angle = FLD_ANGLE_UP_LEFT;
    } else if ((GetKeysHeld() & DPAD_LEFT) && GetKeyReleaseTime(DPAD_DOWN) <= 4) {
        act->angle = FLD_ANGLE_DOWN_LEFT;
    } else if ((GetKeysHeld() & DPAD_RIGHT) && GetKeyReleaseTime(DPAD_UP) <= 4) {
        act->angle = FLD_ANGLE_UP_RIGHT;
    } else if ((GetKeysHeld() & DPAD_RIGHT) && GetKeyReleaseTime(DPAD_DOWN) <= 4) {
        act->angle = FLD_ANGLE_DOWN_RIGHT;
    } else if (GetKeysHeld() & DPAD_DOWN) {
        act->angle = FLD_ANGLE_DOWN;
    } else if (GetKeysHeld() & DPAD_UP) {
        act->angle = FLD_ANGLE_UP;
    } else if (GetKeysHeld() & DPAD_LEFT) {
        act->angle = FLD_ANGLE_LEFT;
    } else if (GetKeysHeld() & DPAD_RIGHT) {
        act->angle = FLD_ANGLE_RIGHT;
    }
}

u8 FldRikuCheckBlocked(FldPos* pos) {
    FldPos up;
    FldPos down;
    s32 lo;
    s32 hi;
    s32 ground;

    up = *pos;
    down = *pos;
    up.y -= 0x600;
    down.y += 0x600;

    lo = GetFldPosGround(&up);

    if (lo > up.ground) {
        up.ground = lo;
    }

    hi = GetFldPosGround(&down);

    if (hi > down.ground) {
        down.ground = hi;
    }

    if (IsFldPosBlocked(&up) != 0 || IsFldPosBlocked(&down) != 0) {
        return TRUE;
    }

    ground = hi;

    if (ground > lo) {
        ground = lo;
    }

    pos->ground = ground;
    return FALSE;
}

s32 FldRikuProbeGround(FldPos* pos) {
    FldPos up;
    FldPos down;
    s32 lo;
    s32 hi;

    up = *pos;
    down = *pos;
    up.y -= 0x600;
    down.y += 0x600;

    lo = GetFldPosGround(&up);
    hi = GetFldPosGround(&down);

    if (hi > lo) {
        hi = lo;
    }

    return hi;
}

u8 FldRikuCheckClimb(FldPos* pos, FldWork* work) {
    FldPos up;
    FldPos down;
    u8 dir;

    up = *pos;
    down = *pos;
    up.y -= 0x600;
    down.y += 0x600;

    dir = GetFldPosClimbDir(&up);

    if (dir != FLD_CLIMB_DIR_NONE) {
        work->targetX = up.x;
        work->targetY = up.y;
        return dir;
    }

    dir = GetFldPosClimbDir(&down);

    if (dir != FLD_CLIMB_DIR_NONE) {
        work->targetX = down.x;
        work->targetY = down.y;
        return dir;
    }

    return FLD_CLIMB_DIR_NONE;
}

u8 FldRikuCheckDoorAhead(FldActor* act) {
    FldPos ahead;

    ahead = act->fieldPosition;
    ahead.x += gSineTable[act->angle] * 8;
    ahead.y -= gSineTable[act->angle + 64] * 8;

    if (MapFindOpenDoor(&ahead)) {
        return TRUE;
    }

    return FALSE;
}

s32 FldRikuGetGround(FldWork* work) {
    FldActor* act;
    s32 ground;

    act = &gFieldState->actor;

    if (work->collider.standFlags & COLLIDER_STAND_OVER_PLATFORM) {
        if (act->fieldPosition.ground < work->collider.platformZ) {
            ground = act->fieldPosition.ground;
        } else {
            ground = work->collider.platformZ;
        }

        work->onCollider = TRUE;
    } else {
        work->onCollider = FALSE;
        ground = act->fieldPosition.ground;
    }

    return ground;
}

void FldRikuTurn(FldActor* act) {
    u8 dir;
    s32 diff;

    dir = act->angle;
    FldRikuSetAngleFromDpad(act);

    if (dir != act->angle) {
        diff = (s8)GetAngleDiff(dir, act->angle);

        if (diff < 0) {
            diff = -diff;
        }

        if (diff > 100) {
            act->speed = 0;
        } else {
            act->speed = act->speed >> 1;
        }
    }
}

void FldRikuSetAnim(FldWork* work, s32 index, u16 flags) {
    const FldAnimDef* def;
    s32 dir;

    switch (gFieldState->actor.angle) {
    case FLD_ANGLE_UP_RIGHT:
        dir = 4;
        work->flags |= FLD_FLAG_HFLIP;
        break;
    case FLD_ANGLE_RIGHT:
        dir = 3;
        work->flags |= FLD_FLAG_HFLIP;
        break;
    case FLD_ANGLE_DOWN_RIGHT:
        dir = 2;
        work->flags |= FLD_FLAG_HFLIP;
        break;
    case FLD_ANGLE_DOWN:
        dir = 1;
        work->flags &= ~FLD_FLAG_HFLIP;
        break;
    case FLD_ANGLE_DOWN_LEFT:
        dir = 2;
        work->flags &= ~FLD_FLAG_HFLIP;
        break;
    case FLD_ANGLE_LEFT:
        dir = 3;
        work->flags &= ~FLD_FLAG_HFLIP;
        break;
    case FLD_ANGLE_UP_LEFT:
        dir = 4;
        work->flags &= ~FLD_FLAG_HFLIP;
        break;
    case FLD_ANGLE_UP:
    default:
        dir = 0;
        work->flags &= ~FLD_FLAG_HFLIP;
        break;
    }

    if (work->animAction == index) {
        flags |= ANIM_FLAG_KEEP_FRAME;
    }

    work->animAction = index;
    def = &sFldRikuAnimDefs[index][dir];
    AnimChangeWithTables(&work->anim, def->animId, flags, def->anims, def->gfxTable);
    SetObjTileSource(work->tiles, def->tiles);
}

void task_fld_riku_0(FldWork* work) {
    FldActor* act;

    act = &gFieldState->actor;
    work->tiles = AllocObjTiles(0xA00, NULL);
    work->palette = LoadObjPalette(gRikuPalette, sizeof(gRikuPalette));
    act->height = 16;
    work->onCollider = FALSE;
    work->unk_9C = 0;
    work->unk_9D = 0;
    work->unk_9E = 0;
    work->timer = 0;
    work->flags = FLD_FLAG_RESTORE_STATE;
    work->animAction = 16;
    act->unk_32 = 0;
    act->kind = FLD_OBJ_KIND_PLAYER;

    if (gGameState.fieldResume) {
        act->fieldPosition = gGameState.fieldPosition;
        act->angle = gGameState.fieldAngle;
        act->speed = gGameState.fieldSpeed;
        work->state = gGameState.fieldState;
        work->vz = gGameState.fieldVz;
        work->targetX = gGameState.fieldTargetX;
        work->targetY = gGameState.fieldTargetY;
        work->targetZ = gGameState.fieldTargetZ;
    } else {
        act->fieldPosition.x = gFieldState->spawnX;
        act->fieldPosition.y = gFieldState->spawnY;
        act->fieldPosition.z = 0;
        act->angle = gFieldState->spawnAngle;
        FldPosInitGround(&act->fieldPosition);
        act->fieldPosition.z = act->fieldPosition.ground;
        act->fieldPosition.y -= act->fieldPosition.ground;
        act->speed = 0;
        work->state = FLD_STATE_GROUND;
        work->vz = 0;
    }

    AnimInit(&work->anim, NULL, NULL);
    FldRikuSetAnim(work, 0, 1);
    work->gfx = AnimGetGfx(&work->anim);

    switch (gGameState.world) {
    case WORLD_NEVER_LAND:
        work->sounds = sFldRikuSounds[1];
        break;
    case WORLD_ATLANTICA:
        work->sounds = sFldRikuSounds[2];
        break;
    case WORLD_MONSTRO:
        work->sounds = sFldRikuSounds[3];
        break;
    case WORLD_WONDERLAND:
        work->sounds = sFldRikuSounds[4];
        break;
    case WORLD_HALLOWEEN_TOWN:
        work->sounds = sFldRikuSounds[5];
        break;
    case 0:
    case WORLD_OLYMPUS_COLISEUM:
    case WORLD_CASTLE_OBLIVION:
        work->sounds = sFldRikuSounds[6];
        break;
    case WORLD_DESTINY_ISLANDS:
        work->sounds = sFldRikuSounds[7];
        break;
    default:
        work->sounds = sFldRikuSounds[0];
        break;
    }

    TaskPoolInit(&work->tasks, 2);
    TaskCreate(&work->tasks, &gTaskDescFldShadow, &gFieldState->actor);
    ColliderInit(&work->collider, COLLIDER_TYPE_PLAYER, 4, 32);
    ColliderSetPosition(&work->collider, act->fieldPosition.x, act->fieldPosition.y, act->fieldPosition.z);
}

u8 FldRikuWaitRoomCreate(FldWork* work, void* task) {
    FldActor* act;
    s16* timer;
    s32 flags;

    act = &gFieldState->actor;
    flags = gFieldState->flags;

    if (flags & FIELD_FLAG_CARD_POSE) {
        FldRikuSetAnim(work, 12, 0);
    } else if (flags & FIELD_FLAG_AUTO_WALK) {
        FldRikuSetAnim(work, 1, 1);
    } else {
        FldRikuSetAnim(work, 0, 1);
    }

    if ((gFieldState->flags & FIELD_FLAG_ROOM_CREATE) == 0) {
        FadeSetPaletteExcluded(BIOS_PTR(work->palette)->index + 16, FALSE);
        work->state = FLD_STATE_GROUND;
        work->timer = 0;
        SetTaskUpdate(task, (TaskUpdateFunc)task_fld_riku_1);
        TaskPoolUpdate(&work->tasks);
    } else {
        timer = &work->timer;

        if (*timer == 0) {
            FadeSetPaletteExcluded(BIOS_PTR(work->palette)->index + 16, TRUE);
            act->speed = 0;
            work->onCollider = FALSE;
        }

        TaskPoolUpdate(&work->tasks);
        work->gfx = AnimUpdate(&work->anim);
        ColliderSetPosition(&work->collider, act->fieldPosition.x, act->fieldPosition.y, act->fieldPosition.z);
        (*timer)++;
    }

    return 1;
}

u8 FldRikuGmkJump(FldWork* work, void* task) {
    FldActor* act;
    s32 x;
    s32 y;
    s32 z;

    act = &gFieldState->actor;
    x = act->fieldPosition.x;
    y = act->fieldPosition.y;
    gFieldState->lockonTarget = NULL;

    switch (work->state) {
    case FLD_STATE_GMK_JUMP_START:
        m4aSongNumStart(SONG_SYS_GIMICJP);
        work->state = FLD_STATE_GMK_JUMP;
        work->vz = -0x800;
        work->timer = 0;
        act->speed = 0;
        work->targetX = work->collider.platformX;
        work->targetY = work->collider.platformY;
        break;
    case FLD_STATE_GMK_JUMP:
        if (work->vz > -0x300) {
            FldRikuSetAnim(work, 11, 0);
            act->speed = 0x180;
            act->fieldPosition.x += gSineTable[act->angle] * act->speed >> 8;
            act->fieldPosition.y += -gSineTable[act->angle + 64] * act->speed >> 8;
        } else {
            FldRikuSetAnim(work, 4, 0);
            act->fieldPosition.x += (work->targetX - act->fieldPosition.x) >> 3;
            act->fieldPosition.y += (work->targetY - act->fieldPosition.y) >> 3;
        }

        work->vz = (work->targetZ - (z = act->fieldPosition.z + 0xF00)) >> 3;
        act->fieldPosition.z += work->vz;
        work->vz += 0x42;

        if (work->vz >= 0) {
            work->timer = 0;
            work->state = FLD_STATE_FALL;
            work->flags |= FLD_FLAG_NO_AIR_TURN;
            SetTaskUpdate(task, (TaskUpdateFunc)FldRikuJump);
        } else {
            work->timer++;
        }

        break;
    }

    if (FldRikuCheckBlocked(&act->fieldPosition)) {
        act->fieldPosition.x = x;
        act->fieldPosition.y = y;
    }

    ColliderSetPosition(&work->collider, act->fieldPosition.x, act->fieldPosition.y, act->fieldPosition.z);
    MapSetCameraTarget(act->fieldPosition.x, act->fieldPosition.y + act->fieldPosition.z);
    work->gfx = AnimUpdate(&work->anim);
    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 FldRikuJump(FldWork* work, void* task) {
    FldPos overLedge;
    FldPos underLedge;
    s32 sx;
    s32 sy;
    s32 nx;
    s32 ny;
    s32 ground;
    FldActor* act;

    act = &gFieldState->actor;
    ground = FldRikuGetGround(work);
    sx = act->fieldPosition.x;
    sy = act->fieldPosition.y;
    gFieldState->lockonTarget = NULL;

    if ((work->flags & FLD_FLAG_NO_AIR_TURN) == 0) {
        FldRikuTurn(act);
    }

    switch (work->state) {
    case FLD_STATE_AIR_ATTACK:
        if (work->timer == 0) {
            gFieldState->lockonTarget = NULL;
            FldRikuSetAnim(work, 14, 0);
        }

        act->fieldPosition.x += gSineTable[act->angle] * act->speed >> 8;
        act->fieldPosition.y += -gSineTable[act->angle + 64] * act->speed >> 8;

        if (AnimGetFrame(&work->anim) > 3) {
            act->fieldPosition.z += work->vz;
            work->vz += 66;

            if (act->fieldPosition.z > ground) {
                act->fieldPosition.z = ground;
                work->vz = 0;
            }
        } else {
            work->vz = 0;
        }

        act->speed -= 38;

        if (act->speed < 0) {
            act->speed = 0;
        }

        switch (AnimGetFrame(&work->anim)) {
        case 1:
            switch (act->angle) {
            case FLD_ANGLE_UP_RIGHT:
            case FLD_ANGLE_UP_LEFT:
                nx = act->fieldPosition.x + gSineTable[act->angle] * 12;
                ny = act->fieldPosition.y + -gSineTable[act->angle + 64] * 12;
                break;
            case FLD_ANGLE_RIGHT:
            case FLD_ANGLE_LEFT:
                nx = act->fieldPosition.x + gSineTable[act->angle] * 27;
                ny = act->fieldPosition.y + -gSineTable[act->angle + 64] * 27;
                break;
            case FLD_ANGLE_UP:
            case FLD_ANGLE_DOWN_RIGHT:
            case FLD_ANGLE_DOWN:
            case FLD_ANGLE_DOWN_LEFT:
            default:
                nx = act->fieldPosition.x + gSineTable[act->angle] * 20;
                ny = act->fieldPosition.y + -gSineTable[act->angle + 64] * 20;
                break;
            }

            SetMapAttackBox(nx, ny, act->fieldPosition.z - 0x800);
            break;
        }

        if (AnimIsFinished(&work->anim)) {
            if (work->vz < 0) {
                work->state = FLD_STATE_JUMP_RISE;
            } else {
                work->state = FLD_STATE_FALL;
            }
        } else {
            work->timer++;
        }

        break;
    case FLD_STATE_JUMP_START:
        if (work->timer == 0) {
            FldRikuSetAnim(work, 3, 0);
            act->speed >>= 1;
        }

        act->fieldPosition.x += gSineTable[act->angle] * act->speed >> 8;
        act->fieldPosition.y += -gSineTable[act->angle + 64] * act->speed >> 8;

        if (work->timer > 3) {
            work->targetZ = gMapRoomState->jumpGmkHeight;

            if (work->targetZ == 0) {
                if (GetRandom() % 2 != 0) {
                    m4aSongNumStart(SONG_SND_225);
                } else {
                    m4aSongNumStart(SONG_SND_226);
                }

                work->state = FLD_STATE_JUMP_RISE;
                work->vz = -1536;
                act->speed <<= 1;
                work->timer = 0;
                act->fieldPosition.z += work->vz;
                work->vz += 66;
            } else {
                act->angle = gMapRoomState->jumpGmkAngle;
                work->targetZ = act->fieldPosition.z - work->targetZ;
                work->state = FLD_STATE_GMK_JUMP_START;
                SetTaskUpdate(task, (TaskUpdateFunc)FldRikuGmkJump);
                work->timer = 0;
            }
        } else {
            work->timer++;
        }

        break;
    case FLD_STATE_JUMP_RISE:
        if ((GetKeysHeld() & DPAD_ANY) != 0) {
            act->speed += 17;

            if (act->speed > 512) {
                act->speed = 512;
            }
        } else {
            act->speed -= 38;

            if (act->speed < 0) {
                act->speed = 0;
            }
        }

        if (work->vz > -512) {
            FldRikuSetAnim(work, 5, 0);
        } else {
            FldRikuSetAnim(work, 4, 0);
        }

        act->fieldPosition.x += gSineTable[act->angle] * act->speed >> 8;
        act->fieldPosition.y += -gSineTable[act->angle + 64] * act->speed >> 8;
        act->fieldPosition.z += work->vz;
        work->vz += 66;

        if (work->vz < 0) {
            if ((GetKeysHeld() & B_BUTTON) == 0) {
                work->vz += 64;
            }
        }

        if ((GetKeysPressed() & A_BUTTON) != 0) {
            work->timer = 0;
            work->state = FLD_STATE_AIR_ATTACK;
        } else if (work->vz > 0) {
            work->timer = 0;
            work->state = FLD_STATE_FALL;
        } else {
            work->timer++;
        }

        break;
    case FLD_STATE_FALL:
        if ((GetKeysHeld() & DPAD_ANY) != 0) {
            act->speed += 17;

            if (act->speed > 512) {
                act->speed = 512;
            }
        } else {
            act->speed -= 38;

            if (act->speed < 0) {
                act->speed = 0;
            }
        }

        if (work->vz < 0x200) {
            FldRikuSetAnim(work, 5, 0);
        } else {
            FldRikuSetAnim(work, 6, 0);
        }

        act->fieldPosition.x += gSineTable[act->angle] * act->speed >> 8;
        act->fieldPosition.y += -gSineTable[act->angle + 64] * act->speed >> 8;
        act->fieldPosition.z += work->vz;
        work->vz += 66;

        if ((GetKeysPressed() & A_BUTTON) != 0) {
            work->timer = 0;
            work->state = FLD_STATE_AIR_ATTACK;
        } else if (act->fieldPosition.z > ground) {
            act->fieldPosition.z = ground;
            work->vz = 0;

            if (work->state != FLD_STATE_LAND) {
                work->state = FLD_STATE_LAND;
                work->timer = 0;
            }
        }

        break;
    case FLD_STATE_LAND:
        if (work->timer == 0) {
            FldRikuSetAnim(work, 7, 0);
            m4aSongNumStart(work->sounds[3]);
        }

        act->speed = 0;

        if ((GetKeysPressed() & B_BUTTON) != 0) {
            work->flags &= ~FLD_FLAG_NO_AIR_TURN;
            work->timer = 0;
            work->state = FLD_STATE_JUMP_START;
        } else if (work->timer > 6) {
            gFieldState->flags &= ~FIELD_FLAG_PLAYER_JUMPING;
            work->flags &= ~FLD_FLAG_NO_AIR_TURN;
            work->state = FLD_STATE_GROUND;
            work->timer = 0;
            SetTaskUpdate(task, (TaskUpdateFunc)task_fld_riku_1);
        } else {
            work->timer++;
        }

        break;
    }

    if (work->collider.colliding) {
        switch (work->collider.otherType) {
        case COLLIDER_TYPE_ENEMY:
        case COLLIDER_TYPE_PICKUP:
        case COLLIDER_TYPE_ENEMY_NO_ENEMY_COLLISION:
            break;
        default:
            if ((work->collider.standFlags & COLLIDER_STAND_OVER_PLATFORM) == 0) {
                act->speed = 230 * act->speed >> 8;
                act->fieldPosition.x += work->collider.pushX;
                act->fieldPosition.y += work->collider.pushY;
            }

            break;
        }
    }

    if (FldRikuCheckBlocked(&act->fieldPosition)) {
        act->fieldPosition.x = sx;
        act->fieldPosition.y = sy;

        switch (FldRikuCheckClimb(&act->fieldPosition, work)) {
        case FLD_CLIMB_DIR_UP_LEFT:
            work->timer = 0;
            work->state = FLD_STATE_CLIMB;
            act->angle = FLD_ANGLE_UP_LEFT;
            SetTaskUpdate(task, (TaskUpdateFunc)FldRikuClimb);
            break;
        case FLD_CLIMB_DIR_UP_RIGHT:
            work->timer = 0;
            work->state = FLD_STATE_CLIMB;
            act->angle = FLD_ANGLE_UP_RIGHT;
            SetTaskUpdate(task, (TaskUpdateFunc)FldRikuClimb);
            break;
        default:
            if (work->state == FLD_STATE_FALL && act->fieldPosition.ground - act->fieldPosition.z > 0xFFF) {
                overLedge = act->fieldPosition;
                overLedge.y -= 0x400;
                overLedge.z = act->fieldPosition.z - 0x3000;
                underLedge = overLedge;
                underLedge.z += 768;

                if (!FldRikuCheckBlocked(&overLedge) && FldRikuCheckBlocked(&underLedge)) {
                    work->timer = 0;
                    work->state = FLD_STATE_LEDGE_CATCH;
                    gFieldState->lockonTarget = NULL;
                    SetTaskUpdate(task, (TaskUpdateFunc)FldRikuHangLedge);
                }
            } else {
                act->speed = 230 * act->speed >> 8;
            }

            break;
        }
    }

    ColliderSetPosition(&work->collider, act->fieldPosition.x, act->fieldPosition.y, act->fieldPosition.z);
    MapSetCameraTarget(act->fieldPosition.x, act->fieldPosition.y + act->fieldPosition.z);
    work->gfx = AnimUpdate(&work->anim);
    TaskPoolUpdate(&work->tasks);

    if ((gFieldState->flags & FIELD_FLAG_ROOM_CREATE) != 0) {
        work->timer = 0;
        SetTaskUpdate(task, (TaskUpdateFunc)FldRikuWaitRoomCreate);
    }

    return 1;
}

u8 FldRikuClimb(FldWork* work, void* task) {
    FldActor* act;
    FldPos probe;
    s32 x;
    s32 y;
    s32 limit;
    s32 dz;
    s32 ny;
    s32 nx;
    s32 tx;
    s32 ty;

    act = &gFieldState->actor;
    limit = FldRikuGetGround(work);
    x = act->fieldPosition.x;
    y = act->fieldPosition.y;
    gFieldState->lockonTarget = NULL;

    switch (work->state) {
    case FLD_STATE_CLIMB:
        if (work->timer == 0) {
            work->targetZ = (act->fieldPosition.z >> 12) << 12;
            work->timer++;
            tx = (work->targetX >> 11) / 4;
            ty = (work->targetY >> 11) / 2;
            nx = (tx << 13) | 0x1000;
            ny = (ty << 12) | 0x800;
            x = nx;
            act->fieldPosition.x = nx;
            y = ny;
            act->fieldPosition.y = ny;
            m4aSongNumStart(work->sounds[5]);
            act->fieldPosition.ground = GetFldPosGround(&act->fieldPosition);
        }

        FldRikuSetAnim(work, 8, 1);
        work->anim.frame = ((act->fieldPosition.z >> 8) + 4) & 31;
        work->gfx = AnimGetGfx(&work->anim);

        if (work->anim.timer == 0) {
            switch (work->anim.frame) {
            case 8:
                m4aSongNumStart(work->sounds[4]);
                break;
            case 24:
                m4aSongNumStart(work->sounds[5]);
                break;
            }
        }

        dz = (((work->targetZ >> 12) << 12) - act->fieldPosition.z) >> 1;

        if (abs(dz) <= 24) {
            dz = 0;
        } else if (dz > 384) {
            dz = 384;
        } else if (dz < -384) {
            dz = -384;
        }

        act->fieldPosition.z += dz;

        if (dz < 0) {
            act->fieldPosition.x += gSineTable[act->angle];
            act->fieldPosition.y -= gSineTable[act->angle + 64];
            probe = act->fieldPosition;
            probe.z = work->targetZ - 0x2800;

            if (!FldRikuCheckBlocked(&probe)) {
                act->speed = 204;
                work->vz = -0x580;
                work->flags |= FLD_FLAG_NO_AIR_TURN;
                work->state = FLD_STATE_CLIMB_OVER;
                work->timer = 0;
                m4aSongNumStart(work->sounds[6]);
            }
        } else if (dz > 0) {
            if (act->fieldPosition.z >= limit) {
                act->fieldPosition.z = limit;
                act->angle += 0x80;
                act->fieldPosition.x += gSineTable[act->angle] * 10;
                act->fieldPosition.y += -gSineTable[act->angle + 64] * 10;
                FldRikuSetAnim(work, 0, 1);
                work->gfx = AnimGetGfx(&work->anim);
                work->state = FLD_STATE_GROUND;
                work->timer = 0;
                SetTaskUpdate(task, (TaskUpdateFunc)task_fld_riku_1);
            }
        } else if (dz == 0) {
            if ((GetKeysHeld() & DPAD_UP) || ((GetKeysHeld() & DPAD_LEFT) && act->angle == FLD_ANGLE_UP_LEFT) ||
                ((GetKeysHeld() & DPAD_RIGHT) && act->angle == FLD_ANGLE_UP_RIGHT)) {
                work->targetZ = ((work->targetZ >> 12) - 1) << 12;
            } else if ((GetKeysHeld() & DPAD_DOWN) || ((GetKeysHeld() & DPAD_RIGHT) && act->angle == FLD_ANGLE_UP_LEFT) ||
                       ((GetKeysHeld() & DPAD_LEFT) && act->angle == FLD_ANGLE_UP_RIGHT)) {
                work->targetZ = ((work->targetZ >> 12) + 1) << 12;
            }
        }

        if (GetKeysPressed() & B_BUTTON) {
            work->vz = 0;
            work->timer = 0;
            work->state = FLD_STATE_FALL;
            act->angle += 0x80;
            act->speed = 0x80;
            work->flags |= FLD_FLAG_NO_AIR_TURN;
            act->fieldPosition.x += gSineTable[act->angle] * 10;
            act->fieldPosition.y += -gSineTable[act->angle + 64] * 10;
            FldRikuSetAnim(work, 6, 0);
            work->gfx = AnimGetGfx(&work->anim);
            SetTaskUpdate(task, (TaskUpdateFunc)FldRikuJump);
        }

        break;
    case FLD_STATE_CLIMB_OVER:
        FldRikuSetAnim(work, 11, 0);
        act->fieldPosition.x += gSineTable[act->angle] * act->speed >> 8;
        act->fieldPosition.y += -gSineTable[act->angle + 64] * act->speed >> 8;
        work->vz += 0x42;
        act->fieldPosition.z += work->vz;

        if (work->vz > 0) {
            work->timer = 0;
            work->state = FLD_STATE_FALL;
            SetTaskUpdate(task, (TaskUpdateFunc)FldRikuJump);
        } else {
            work->timer++;
        }

        work->gfx = AnimUpdate(&work->anim);
        break;
    }

    if (FldRikuCheckBlocked(&act->fieldPosition)) {
        act->fieldPosition.x = x;
        act->fieldPosition.y = y;
    }

    ColliderSetPosition(&work->collider, act->fieldPosition.x, act->fieldPosition.y, act->fieldPosition.z);
    MapSetCameraTarget(act->fieldPosition.x, act->fieldPosition.y + act->fieldPosition.z);
    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 FldRikuLedgeInput(FldWork* work, void* task) {
    FldActor* act;

    act = &gFieldState->actor;

    if ((GetKeysPressed() & B_BUTTON) || (GetKeysPressed() & DPAD_DOWN) ||
        (act->angle == FLD_ANGLE_UP_LEFT && (GetKeysPressed() & DPAD_RIGHT)) ||
        (act->angle == FLD_ANGLE_UP_RIGHT && (GetKeysPressed() & DPAD_LEFT))) {
        work->timer = 0;
        work->state = FLD_STATE_FALL;
        work->vz = 0;
        act->angle += 0x80;
        gFieldState->lockonTarget = NULL;
        SetTaskUpdate(task, (TaskUpdateFunc)FldRikuJump);
        return TRUE;
    }

    if ((GetKeysHeld() & DPAD_UP) ||
        (act->angle == FLD_ANGLE_UP_LEFT && (GetKeysHeld() & DPAD_LEFT)) ||
        (act->angle == FLD_ANGLE_UP_RIGHT && (GetKeysHeld() & DPAD_RIGHT))) {
        work->timer = 0;
        work->state = FLD_STATE_LEDGE_CLIMB;
        act->speed = 0x133;
        work->vz = -0x5C0;
        work->flags |= FLD_FLAG_NO_AIR_TURN;
        m4aSongNumStart(SONG_SYS_SR_CATJP);
        gFieldState->lockonTarget = NULL;
        return TRUE;
    }

    return FALSE;
}

u8 FldRikuHangLedge(FldWork* work, void* task) {
    FldActor* act;
    FldPos probe;
    u8 handled;
    s32 x;
    s32 y;

    act = &gFieldState->actor;
    handled = FALSE;
    x = act->fieldPosition.x;
    y = act->fieldPosition.y;
    gFieldState->lockonTarget = NULL;

    switch (work->state) {
    case FLD_STATE_LEDGE_CATCH:
        if (work->timer == 0) {
            probe = act->fieldPosition;
            probe.y -= 0xA00;
            act->fieldPosition.z = GetFldPosGround(&probe) + 0x2B00;
            m4aSongNumStart(SONG_SYS_SR_CATCH);
            act->angle = GetLedgeAngleAt(act->fieldPosition.x, act->fieldPosition.y, act->fieldPosition.z);
            FldRikuSetAnim(work, 9, 0);
        }

        if (work->timer > 15) {
            handled = FldRikuLedgeInput(work, task);
        }

        act->fieldPosition.x += gSineTable[act->angle];
        act->fieldPosition.y -= gSineTable[act->angle + 64];

        if (AnimIsFinished(&work->anim) && !handled) {
            work->state = FLD_STATE_LEDGE_HANG;
        } else {
            work->timer++;
        }

        break;
    case FLD_STATE_LEDGE_HANG:
        FldRikuSetAnim(work, 10, 0);
        FldRikuLedgeInput(work, task);
        break;
    case FLD_STATE_LEDGE_CLIMB:
        FldRikuSetAnim(work, 11, 0);
        act->fieldPosition.x += gSineTable[act->angle] * act->speed >> 8;
        act->fieldPosition.y += -gSineTable[act->angle + 64] * act->speed >> 8;
        act->fieldPosition.z += work->vz;
        work->vz += 0x42;

        if (work->vz > 0) {
            work->timer = 0;
            work->state = FLD_STATE_FALL;
            SetTaskUpdate(task, (TaskUpdateFunc)FldRikuJump);
        } else {
            work->timer++;
        }

        break;
    }

    work->gfx = AnimUpdate(&work->anim);

    if (FldRikuCheckBlocked(&act->fieldPosition)) {
        act->fieldPosition.x = x;
        act->fieldPosition.y = y;
    }

    ColliderSetPosition(&work->collider, act->fieldPosition.x, act->fieldPosition.y, act->fieldPosition.z);
    MapSetCameraTarget(act->fieldPosition.x, act->fieldPosition.y + act->fieldPosition.z);
    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 FldRikuWalkOut(FldWork* work, void* task) {
    FldActor* act;

    act = &gFieldState->actor;

    switch (work->state) {
    case FLD_STATE_WALK_OUT:
        if (work->timer == 0) {
            work->flags |= FLD_FLAG_WALK_OUT;
            act->angle = FLD_ANGLE_UP_RIGHT;
            FldRikuSetAnim(work, 2, 1);

            act->fieldPosition.x = 0x22000;
            act->fieldPosition.y = 0xF000;
            work->steps = 30;
            act->fieldPosition.z = 0;
            act->fieldPosition.ground = 0;
            work->targetX = act->fieldPosition.x + 0x2000;
            work->targetY = act->fieldPosition.y - 0x1000;
            work->targetZ = act->fieldPosition.ground - 0x2800;
        }

        if (work->steps > 0) {
            ApproachValue(&act->fieldPosition.x, work->targetX, work->steps);
            ApproachValue(&act->fieldPosition.y, work->targetY, work->steps);
            ApproachValue(&act->fieldPosition.z, work->targetZ, work->steps);
            act->fieldPosition.ground = act->fieldPosition.z;
            work->steps--;
        }

        if (work->anim.timer == 0) {
            switch (work->anim.frame) {
            case 3:
                m4aSongNumStart(work->sounds[0]);
                break;
            case 7:
                m4aSongNumStart(work->sounds[1]);
                break;
            }
        }

        if (work->steps <= 0) {
            work->timer = 0;

            if (work->flags & FLD_FLAG_TO_WORLD_SELECT) {
                work->state = FLD_STATE_WALK_OUT_TO_WORLD_SELECT;
            } else {
                work->state = FLD_STATE_WALK_OUT_TO_EXIT;
            }
        } else {
            work->timer++;
        }

        break;
    case FLD_STATE_WALK_OUT_TO_WORLD_SELECT:
        if (work->timer == 0) {
            work->steps = 25;
            work->targetX = act->fieldPosition.x + 0x2000;
            work->targetY = act->fieldPosition.y - 0x1000;
        }

        if (work->steps > 0) {
            ApproachValue(&act->fieldPosition.x, work->targetX, work->steps);
            ApproachValue(&act->fieldPosition.y, work->targetY, work->steps);
            work->steps--;
        }

        if (work->anim.timer == 0) {
            switch (work->anim.frame) {
            case 3:
                m4aSongNumStart(work->sounds[0]);
                break;
            case 7:
                m4aSongNumStart(work->sounds[1]);
                break;
            }
        }

        if (work->steps <= 0) {
            work->timer = 0;
            work->state = FLD_STATE_WORLD_SELECT_POSE;
        } else {
            work->timer++;
        }

        break;
    case FLD_STATE_WALK_OUT_TO_EXIT:
        if (work->timer == 0) {
            work->steps = 25;
            work->targetX = act->fieldPosition.x + 0x2000;
            work->targetY = act->fieldPosition.y - 0x1000;
        }

        if (work->steps > 0) {
            ApproachValue(&act->fieldPosition.x, work->targetX, work->steps);
            ApproachValue(&act->fieldPosition.y, work->targetY, work->steps);
            work->steps--;
        }

        if (work->anim.timer == 0) {
            switch (work->anim.frame) {
            case 3:
                m4aSongNumStart(work->sounds[0]);
                break;
            case 7:
                m4aSongNumStart(work->sounds[1]);
                break;
            }
        }

        if (work->steps <= 0) {
            work->timer = 0;
            work->state = FLD_STATE_WALK_OUT_END;
        } else {
            work->timer++;
        }

        break;
    case FLD_STATE_WORLD_SELECT_POSE:
        if (work->timer == 0) {
            FldRikuSetAnim(work, 12, 0);
            FadeSetPaletteExcluded(BIOS_PTR(work->palette)->index + 16, TRUE);
        }

        if (work->timer == 40) {
            CreateWorldSelBeforeTask(&work->tasks, act->fieldPosition.x, act->fieldPosition.y, act->fieldPosition.z);
        }

        if (work->timer > 140) {
            work->timer = 0;
            work->state = FLD_STATE_WALK_OUT_END;
        } else {
            work->timer++;
        }

        break;
    case FLD_STATE_WALK_OUT_END:
        EndMapWalkOut();
        break;
    }

    ColliderSetPosition(&work->collider, act->fieldPosition.x, act->fieldPosition.y, act->fieldPosition.z);
    MapSetCameraTarget(act->fieldPosition.x, act->fieldPosition.y + act->fieldPosition.z);
    work->gfx = AnimUpdate(&work->anim);
    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 FldRikuAttack(FldWork* work, void* task) {
    FldActor* act;
    s32 x;
    s32 y;
    s32 nx;
    s32 ny;

    act = &gFieldState->actor;
    x = act->fieldPosition.x;
    y = act->fieldPosition.y;

    if (work->state == FLD_STATE_ATTACK) {
        if (work->timer == 0) {
            FldRikuSetAnim(work, 13, 0);
            act->speed = 0;
            gFieldState->lockonTarget = NULL;
            work->steps = 0;
            m4aSongNumStart(SONG_SND_227);
        }

        if (work->anim.timer == 0) {
            switch (act->angle) {
            case FLD_ANGLE_DOWN_LEFT:
                switch (AnimGetFrame(&work->anim)) {
                case 0:
                    act->fieldPosition.x -= 0x500;
                    act->fieldPosition.y += 0x400;
                    break;
                case 1:
                    act->fieldPosition.x -= 0x200;
                    break;
                case 2:
                    act->fieldPosition.x -= 0x300;
                    break;
                }

                break;
            case FLD_ANGLE_DOWN_RIGHT:
                switch (AnimGetFrame(&work->anim)) {
                case 0:
                    act->fieldPosition.x += 0x500;
                    act->fieldPosition.y += 0x400;
                    break;
                case 1:
                    act->fieldPosition.x += 0x200;
                    break;
                case 2:
                    act->fieldPosition.x += 0x300;
                    break;
                }

                break;
            case FLD_ANGLE_UP_LEFT:
                switch (AnimGetFrame(&work->anim)) {
                case 0:
                    act->fieldPosition.x -= 0x500;
                    act->fieldPosition.y -= 0x200;
                    break;
                case 1:
                    act->fieldPosition.x -= 0x500;
                    break;
                case 2:
                    act->fieldPosition.x -= 0x200;
                    break;
                }

                break;
            case FLD_ANGLE_UP_RIGHT:
                switch (AnimGetFrame(&work->anim)) {
                case 0:
                    act->fieldPosition.x += 0x500;
                    act->fieldPosition.y -= 0x200;
                    break;
                case 1:
                    act->fieldPosition.x += 0x500;
                    break;
                case 2:
                    act->fieldPosition.x += 0x200;
                    break;
                }

                break;
            case FLD_ANGLE_DOWN:
                switch (AnimGetFrame(&work->anim)) {
                case 0:
                    act->fieldPosition.x -= 0x300;
                    act->fieldPosition.y += 0x400;
                    break;
                case 1:
                    act->fieldPosition.x += 0x100;
                    act->fieldPosition.y += 0x100;
                    break;
                case 2:
                    act->fieldPosition.y += 0x200;
                    break;
                case 3:
                    act->fieldPosition.y += 0x100;
                    break;
                }

                break;
            case FLD_ANGLE_RIGHT:
                switch (AnimGetFrame(&work->anim)) {
                case 0:
                    act->fieldPosition.x += 0x700;
                    act->fieldPosition.y += 0x100;
                    break;
                case 1:
                    act->fieldPosition.x += 0x300;
                    break;
                case 2:
                    act->fieldPosition.x += 0x200;
                    break;
                }

                break;
            case FLD_ANGLE_LEFT:
                switch (AnimGetFrame(&work->anim)) {
                case 0:
                    act->fieldPosition.x -= 0x700;
                    act->fieldPosition.y += 0x100;
                    break;
                case 1:
                    act->fieldPosition.x -= 0x300;
                    break;
                case 2:
                    act->fieldPosition.x -= 0x200;
                    break;
                }

                break;
            case FLD_ANGLE_UP:
                switch (AnimGetFrame(&work->anim)) {
                case 0:
                    act->fieldPosition.y -= 0x400;
                    break;
                case 1:
                    act->fieldPosition.y -= 0x400;
                    break;
                case 2:
                    act->fieldPosition.x -= 0x100;
                    act->fieldPosition.y += 0x100;
                    break;
                case 3:
                    act->fieldPosition.y -= 0x100;
                    break;
                }

                break;
            }
        }

        if (AnimGetFrame(&work->anim) == 3) {
            switch (act->angle) {
            case FLD_ANGLE_UP_RIGHT:
            case FLD_ANGLE_UP_LEFT:
                nx = act->fieldPosition.x + gSineTable[act->angle] * 12;
                ny = act->fieldPosition.y + -gSineTable[act->angle + 64] * 12;
                break;
            case FLD_ANGLE_RIGHT:
            case FLD_ANGLE_LEFT:
                nx = act->fieldPosition.x + gSineTable[act->angle] * 27;
                ny = act->fieldPosition.y + -gSineTable[act->angle + 64] * 27;
                break;
            case FLD_ANGLE_UP:
            case FLD_ANGLE_DOWN_RIGHT:
            case FLD_ANGLE_DOWN:
            case FLD_ANGLE_DOWN_LEFT:
            default:
                nx = act->fieldPosition.x + gSineTable[act->angle] * 20;
                ny = act->fieldPosition.y + -gSineTable[act->angle + 64] * 20;
                break;
            }

            SetMapAttackBox(nx, ny, act->fieldPosition.z - 0x800);
        }

        if (AnimIsFinished(&work->anim)) {
            switch (act->angle) {
            case FLD_ANGLE_DOWN_LEFT:
                act->fieldPosition.x -= 0x200;
                act->fieldPosition.y += 0x200;
                break;
            case FLD_ANGLE_DOWN_RIGHT:
                act->fieldPosition.x += 0x200;
                act->fieldPosition.y += 0x200;
                break;
            case FLD_ANGLE_UP_RIGHT:
            case FLD_ANGLE_UP_LEFT:
                act->fieldPosition.y -= 0x400;
                break;
            case FLD_ANGLE_DOWN:
                act->fieldPosition.y += 0x200;
                break;
            case FLD_ANGLE_UP:
                act->fieldPosition.y -= 0x200;
                break;
            }

            FldRikuSetAnim(work, 0, 0);
            work->state = FLD_STATE_GROUND;
            SetTaskUpdate(task, (TaskUpdateFunc)task_fld_riku_1);
        } else {
            work->timer++;
        }
    }

    if (work->collider.colliding) {
        switch (work->collider.otherType) {
        case COLLIDER_TYPE_PICKUP:
        case COLLIDER_TYPE_ENEMY:
        case COLLIDER_TYPE_ENEMY_NO_ENEMY_COLLISION:
            break;
        default:
            if ((work->collider.standFlags & COLLIDER_STAND_OVER_PLATFORM) == 0) {
                act->fieldPosition.x += work->collider.pushX;
                act->fieldPosition.y += work->collider.pushY;
            }

            break;
        }
    }

    if (FldRikuCheckBlocked(&act->fieldPosition)) {
        act->fieldPosition.x = x;
        act->fieldPosition.y = y;
    }

    ColliderSetPosition(&work->collider, act->fieldPosition.x, act->fieldPosition.y, act->fieldPosition.z);
    MapSetCameraTarget(act->fieldPosition.x, act->fieldPosition.y + act->fieldPosition.z);
    work->gfx = AnimUpdate(&work->anim);
    TaskPoolUpdate(&work->tasks);

    if (gFieldState->flags & FIELD_FLAG_ROOM_CREATE) {
        work->timer = 0;
        SetTaskUpdate(task, (TaskUpdateFunc)FldRikuWaitRoomCreate);
        TaskPoolUpdate(&work->tasks);
    }

    return 1;
}

u8 task_fld_riku_1(FldWork* work, void* task) {
    FldPos probeA;
    FldPos probeB;
    FldPos slideB;
    FldPos slideA;
    s32 sx;
    s32 sy;
    s32 dx;
    s32 dy;
    s32 dz;
    s32 dw;
    s32 ground;
    s32 climbDir;
    u8 blockedA;
    u8 blockedB;
    FldActor* act;

    act = &gFieldState->actor;

    if ((work->flags & FLD_FLAG_RESTORE_STATE) != 0) {
        work->flags &= ~FLD_FLAG_RESTORE_STATE;

        switch (work->state) {
        case FLD_STATE_AIR_ATTACK:
            work->state = FLD_STATE_JUMP_RISE;
        case FLD_STATE_JUMP_START:
        case FLD_STATE_JUMP_RISE:
        case FLD_STATE_FALL:
        case FLD_STATE_LAND:
            SetTaskUpdate(task, (TaskUpdateFunc)FldRikuJump);
            gFieldState->lockonTarget = NULL;
            break;
        case FLD_STATE_CLIMB:
        case FLD_STATE_CLIMB_OVER:
            SetTaskUpdate(task, (TaskUpdateFunc)FldRikuClimb);
            gFieldState->lockonTarget = NULL;
            work->timer = 1;
            break;
        case FLD_STATE_LEDGE_CATCH:
        case FLD_STATE_LEDGE_HANG:
        case FLD_STATE_LEDGE_CLIMB:
            SetTaskUpdate(task, (TaskUpdateFunc)FldRikuHangLedge);
            gFieldState->lockonTarget = NULL;
            break;
        default:
            work->state = FLD_STATE_GROUND;
            break;
        }

        ColliderSetPosition(&work->collider, act->fieldPosition.x, act->fieldPosition.y, act->fieldPosition.z);
        MapSetCameraTarget(act->fieldPosition.x, act->fieldPosition.y + act->fieldPosition.z);
        work->gfx = AnimUpdate(&work->anim);
        TaskPoolUpdate(&work->tasks);
        return 1;
    } else if (GetMapWalkOutMode() != 0) {
        work->timer = 0;
        SetTaskUpdate(task, (TaskUpdateFunc)FldRikuWalkOut);
        TaskPoolUpdate(&work->tasks);
        work->state = FLD_STATE_WALK_OUT;

        if (GetMapWalkOutMode() == 1) {
            work->flags |= FLD_FLAG_TO_WORLD_SELECT;
        }

        return 1;
    } else if ((gFieldState->flags & FIELD_FLAG_ROOM_CREATE) != 0) {
        work->timer = 0;
        SetTaskUpdate(task, (TaskUpdateFunc)FldRikuWaitRoomCreate);
        TaskPoolUpdate(&work->tasks);
        return 1;
    } else {
        sx = act->fieldPosition.x;
        sy = act->fieldPosition.y;

        if (work->state <= FLD_STATE_GROUND_UNUSED) {
            if ((gFieldState->flags & 0x4000) == 0) {
                FldRikuTurn(act);
            }

            if ((gFieldState->flags & 0x4000) == 0 && (GetKeysHeld() & DPAD_ANY) != 0) {
                act->speed += 128;
                FldRikuSetAnim(work, 2, 1);

                if (act->speed > 0x300) {
                    act->speed = 0x300;
                }

                if (work->anim.timer == 0) {
                    switch (work->anim.frame) {
                    case 3:
                        m4aSongNumStart(work->sounds[0]);
                        break;
                    case 7:
                        m4aSongNumStart(work->sounds[1]);
                        break;
                    }
                }
            } else {
                FldRikuSetAnim(work, 0, 1);
                act->speed -= 128;

                if (act->speed < 0) {
                    act->speed = 0;
                }
            }

            act->fieldPosition.x += gSineTable[act->angle] * act->speed >> 8;
            act->fieldPosition.y += -gSineTable[act->angle + 64] * act->speed >> 8;

            if ((GetKeysPressed() & B_BUTTON) != 0) {
                gFieldState->flags |= FIELD_FLAG_PLAYER_JUMPING;
                gFieldState->lockonTarget = NULL;
                work->timer = 0;
                work->state = FLD_STATE_JUMP_START;
                SetTaskUpdate(task, (TaskUpdateFunc)FldRikuJump);
                m4aSongNumStart(work->sounds[2]);
            } else if ((GetKeysPressed() & A_BUTTON) != 0) {
                work->timer = 0;
                gFieldState->lockonTarget = NULL;
                work->state = FLD_STATE_ATTACK;
                SetTaskUpdate(task, (TaskUpdateFunc)FldRikuAttack);
            }
        } else if (AnimIsFinished(&work->anim)) {
            work->state = FLD_STATE_GROUND;
        }

        if (work->collider.colliding) {
            switch (work->collider.otherType) {
            case COLLIDER_TYPE_ENEMY:
            case COLLIDER_TYPE_PICKUP:
            case COLLIDER_TYPE_ENEMY_NO_ENEMY_COLLISION:
                break;
            default:
                if ((work->collider.standFlags & COLLIDER_STAND_OVER_PLATFORM) == 0) {
                    act->speed = 230 * act->speed >> 8;
                    act->fieldPosition.x += work->collider.pushX;
                    act->fieldPosition.y += work->collider.pushY;
                }

                break;
            }
        }

        if (FldRikuCheckBlocked(&act->fieldPosition)) {
            act->fieldPosition.x = sx;
            act->fieldPosition.y = sy;
            climbDir = FldRikuCheckClimb(&act->fieldPosition, work);

            if (climbDir != FLD_CLIMB_DIR_NONE) {
                switch (climbDir) {
                case FLD_CLIMB_DIR_UP_LEFT:
                    work->timer = 0;
                    work->state = FLD_STATE_CLIMB;
                    act->angle = FLD_ANGLE_UP_LEFT;
                    gFieldState->lockonTarget = NULL;
                    SetTaskUpdate(task, (TaskUpdateFunc)FldRikuClimb);
                    break;
                case FLD_CLIMB_DIR_UP_RIGHT:
                    work->timer = 0;
                    work->state = FLD_STATE_CLIMB;
                    act->angle = FLD_ANGLE_UP_RIGHT;
                    gFieldState->lockonTarget = NULL;
                    SetTaskUpdate(task, (TaskUpdateFunc)FldRikuClimb);
                    break;
                }
            } else {
                if (FldRikuCheckDoorAhead(act)) {
                    FadeSetPaletteExcluded(BIOS_PTR(work->palette)->index + 16, TRUE);
                    gFieldState->flags |= FIELD_FLAG_EXIT_ROOM;
                    return 1;
                }

                switch (act->angle) {
                case FLD_ANGLE_DOWN_LEFT:
                    dx = -256;
                    dy = 0;
                    dz = 0;
                    dw = 384;
                    break;
                case FLD_ANGLE_DOWN_RIGHT:
                    dx = 256;
                    dy = 0;
                    dz = 0;
                    dw = 384;
                    break;
                case FLD_ANGLE_UP_LEFT:
                    dx = -256;
                    dy = 0;
                    dz = 0;
                    dw = -384;
                    break;
                case FLD_ANGLE_UP_RIGHT:
                    dx = 256;
                    dy = 0;
                    dz = 0;
                    dw = -384;
                    break;
                case FLD_ANGLE_DOWN:
                    dx = -512;
                    dy = 192;
                    dz = 512;
                    dw = 192;
                    break;
                case FLD_ANGLE_UP:
                    dx = -512;
                    dy = -192;
                    dz = 512;
                    dw = -192;
                    break;
                case FLD_ANGLE_RIGHT:
                    dx = 384;
                    dy = -307;
                    dz = 384;
                    dw = 307;
                    break;
                case FLD_ANGLE_LEFT:
                    dx = -384;
                    dy = -307;
                    dz = -384;
                    dw = 307;
                    break;
                default:
                    dw = 0;
                    dz = 0;
                    dy = 0;
                    dx = 0;
                    break;
                }

                probeB = act->fieldPosition;
                probeA = probeB;
                probeA.x += dx;
                probeA.y += dy;
                probeB.x += dz;
                probeB.y += dw;
                blockedA = FldRikuCheckBlocked(&probeA);
                blockedB = FldRikuCheckBlocked(&probeB);

                if (blockedA) {
                    if (!blockedB) {
                        slideB = act->fieldPosition;
                        slideB.x += dz;
                        slideB.y += dw;
                        slideB.ground = FldRikuProbeGround(&slideB);

                        if (slideB.ground >= slideB.z) {
                            act->fieldPosition = slideB;
                        }
                    }
                } else if (blockedB) {
                    slideA = act->fieldPosition;
                    slideA.x += dx;
                    slideA.y += dy;
                    slideA.ground = FldRikuProbeGround(&slideA);

                    if (slideA.ground >= slideA.z) {
                        act->fieldPosition = slideA;
                    }
                }

                act->speed = 0;
            }
        }

        ground = FldRikuGetGround(work);

        if (act->fieldPosition.ground == MAP_Z_BOTTOM) {
            act->fieldPosition.ground = act->fieldPosition.z;
        } else if (ground != act->fieldPosition.z) {
            act->speed >>= 2;
            work->vz = 0;
            work->timer = 0;
            gFieldState->lockonTarget = NULL;
            gFieldState->flags |= FIELD_FLAG_PLAYER_JUMPING;
            work->state = FLD_STATE_FALL;
            SetTaskUpdate(task, (TaskUpdateFunc)FldRikuJump);
        } else if (ground != act->fieldPosition.ground) {
            gFieldState->lockonTarget = NULL;
        }
    }

    ColliderSetPosition(&work->collider, act->fieldPosition.x, act->fieldPosition.y, act->fieldPosition.z);
    MapSetCameraTarget(act->fieldPosition.x, act->fieldPosition.y + act->fieldPosition.z);
    work->gfx = AnimUpdate(&work->anim);
    TaskPoolUpdate(&work->tasks);
    return 1;
}

void task_fld_riku_2(FldWork* work) {
    FldActor* act;
    u16 depth;
    s32 pri;
    s32 x;
    s32 y;
    s32 z;

    act = &gFieldState->actor;
    pri = (work->flags & FLD_FLAG_HFLIP) ? (SPRITE_PRIORITY(2) | SPRITE_FLAG_HFLIP) : SPRITE_PRIORITY(2);

    if (work->onCollider) {
        depth = -0x1006 - (work->collider.platformY >> 8) * 4;

        if (work->collider.penetration <= work->collider.radius) {
            act->shadowPriority = 0;
            act->shadowZ = GetFldPosGround(&act->fieldPosition);
        } else {
            act->shadowZ = work->collider.platformZ;
            act->shadowPriority = depth + 1;
        }
    } else {
        depth = -0x1004 - (act->fieldPosition.y >> 8) * 4;

        if (work->flags & FLD_FLAG_WALK_OUT) {
            act->shadowZ = act->fieldPosition.ground;
        } else {
            act->shadowZ = GetFldPosGround(&act->fieldPosition);
        }

        if (act->shadowZ != act->fieldPosition.ground) {
            act->shadowPriority = 0;
        } else {
            act->shadowPriority = depth + 1;
        }
    }

    x = (act->fieldPosition.x >> 8) - (gFieldState->x >> 8);
    y = (act->fieldPosition.y >> 8) + (act->fieldPosition.z >> 8) - (gFieldState->y >> 8);
    DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, pri, depth);
    TaskPoolDraw(&work->tasks);
}

void task_fld_riku_3(FldWork* work) {
    FldActor* act;

    act = &gFieldState->actor;
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ColliderUnregister(&work->collider);

    if (gGameState.fieldResume) {
        gGameState.fieldSpeed = act->speed;
        gGameState.fieldPosition = act->fieldPosition;
        gGameState.fieldAngle = act->angle;
        gGameState.fieldState = work->state;
        gGameState.fieldVz = work->vz;
        gGameState.fieldTargetX = work->targetX;
        gGameState.fieldTargetY = work->targetY;
        gGameState.fieldTargetZ = work->targetZ;
    } else {
        gGameState.fieldAngle = act->angle;
    }

    TaskPoolDestroy(&work->tasks);
}

TaskDesc gTaskDescFldRiku = {
    "task_fld_riku",
    (TaskInitFunc)task_fld_riku_0,
    (TaskUpdateFunc)task_fld_riku_1,
    (TaskDrawFunc)task_fld_riku_2,
    (TaskDestroyFunc)task_fld_riku_3,
    sizeof(FldWork),
};
