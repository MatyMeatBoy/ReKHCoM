/**
 * bos_ga.c
 * Guard Armor Boss
 */

#include "bos_ga.h"
#include "anim.h"
#include "sprites_worldinspect.h"
#include "sprites_room.h"
#include "chara_types.h"
#include "chara_api.h"
#include "btl_api.h"
#include "songs.h"
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "boss_background_types.h"
#include "btl_collision.h"
#include "btl_effect.h"
#include "card_api.h"
#include "engine_math.h"
#include "ga_types.h"
#include "gba/defines.h"
#include "m4a_song.h"
#include "obj.h"
#include "obj_api.h"
#include "room_data.h"
#include "task_descriptors.h"
#include "taskpool.h"
#include "types.h"
#include <stddef.h>
#include "sprite_palettes.h"
#include "enemy_ids.h"
#include "macros.h"

static const EmyKind sBosGaEmyKind =
{ENEMY_GUARD_ARMOR, 100, 16, 16, 0, 100, EMY_KIND_FLAG_NO_COLLIDER}
;

enum BosGaPart {
    BOS_GA_PART_TORSO,
    BOS_GA_PART_HEAD,
    BOS_GA_PART_NEAR_HAND,
    BOS_GA_PART_FAR_HAND,
    BOS_GA_PART_NEAR_FOOT,
    BOS_GA_PART_FAR_FOOT
};

static const GaEntryDef sGaEntryDefs[6] = {
    {Q_8_8(1), 0, 0, -15872, 0, 0, gBosGaTorsoTiles, gBosGaTorsoAnims, gBosGaTorsoFrames, ARRAY_COUNT(gBosGaTorsoFrames)},
    {Q_8_8(1), -2560, 2048, -25088, 0, 0, gBosGaHeadTiles, gBosGaHeadAnims, gBosGaHeadFrames, ARRAY_COUNT(gBosGaHeadFrames)},
    {Q_8_8(1), 2560, 5632, -14848, 0, 0, gBosGaNearHandTiles, gBosGaNearHandAnims, gBosGaNearHandFrames, ARRAY_COUNT(gBosGaNearHandFrames)},
    {Q_8_8(1), -11264, -3328, -12288, 0, 0, gBosGaFarHandTiles, gBosGaFarHandAnims, gBosGaFarHandFrames, ARRAY_COUNT(gBosGaFarHandFrames)},
    {Q_8_8(1), 3840, 512, -2048, 0, 0, gBosGaNearFootTiles, gBosGaNearFootAnims, gBosGaNearFootFrames, ARRAY_COUNT(gBosGaNearFootFrames)},
    {Q_8_8(1), -3328, -1792, -2048, 0, 0, gBosGaFarFootTiles, gBosGaFarFootAnims, gBosGaFarFootFrames, ARRAY_COUNT(gBosGaFarFootFrames)},
};

static const BosMapConfig sBosMapConfig =
{gBosGaBgTiles, 16352, gBosGaBgPalette, sizeof(gBosGaBgPalette), {gBosGaBgTopLeftMap, gBosGaBgTopRightMap, gBosGaBgBottomLeftMap, gBosGaBgBottomRightMap}}
;

static const s32 sBosGaTanTable[32] = {
    6,
    12,
    18,
    25,
    31,
    37,
    44,
    50,
    57,
    64,
    70,
    77,
    84,
    91,
    98,
    106,
    113,
    121,
    128,
    136,
    145,
    153,
    162,
    171,
    180,
    189,
    199,
    210,
    220,
    232,
    243,
    256,
};

TaskDesc gTaskDescBosGa = {
    "task_bos_ga",
    (TaskInitFunc)task_bos_ga_0,
    (TaskUpdateFunc)task_bos_ga_1,
    (TaskDrawFunc)task_bos_ga_2,
    (TaskDestroyFunc)task_bos_ga_3,
    sizeof(GaWork),
};

static GaWork* sGaWork;

u16 BosGaAtan(s32 ratio) {
    u16 i;

    if (ratio == 0x100) {
        return 0x20;
    }

    i = 0;

    if (ratio >= sBosGaTanTable[0]) {
        do {
            i++;

            if (i > 0x3F) {
                break;
            }
        } while (ratio >= sBosGaTanTable[i]);
    }

    return i;
}

s32 BosGaGetAngle(s32 x0, s32 y0, s32 x1, s32 y1) {
    s32 dx;
    s32 dy;
    s32 oldDx;
    s32 quadrant;
    u16 angle;

    dx = x1 - x0;
    dy = y1 - y0;

    if (dx >= 0) {
        quadrant = 0;

        if (dy < 0) {
            quadrant = 3;
            oldDx = dx;
            dx = -dy;
            dy = oldDx;
        }
    } else if (dy >= 0) {
        quadrant = 1;
        oldDx = dx;
        dx = dy;
        dy = -oldDx;
    } else {
        quadrant = 2;
        dx = -dx;
        dy = -dy;
    }

    if (dy > dx) {
        if (dy == 0) {
            return 0;
        }

        dx <<= 8;
        angle = 0x40 - BosGaAtan(dx / dy);
    } else {
        if (dx == 0) {
            return 0;
        }

        dy <<= 8;
        angle = BosGaAtan(dy / dx);
    }

    return (angle + (quadrant << 6) + 0x40) & 0xFF;
}

void BosGaEntryUpdateFall(GaEntryWork* work) {
    work->vz += 0x4C;
    work->actor.z += work->vz;

    if (work->actor.z > 0) {
        if (work->vz > 0x500) {
            m4aSongNumStart(SONG_BTL_IRON_GIMICBREAK);
        }

        work->actor.z = 0;
        work->vz = -work->vz / 2;
    }

    if (work->vx > 0) {
        work->actor.x += work->vx;
        work->vx -= 0x11;

        if (work->vx < 0) {
            work->vx = 0;
        }
    } else if (work->vx < 0) {
        work->actor.x += work->vx;
        work->vx += 0x11;

        if (work->vx > 0) {
            work->vx = 0;
        }
    }

    if (work->vy > 0) {
        work->actor.y += work->vy / 2;
        work->vy -= 0x11;

        if (work->vy < 0) {
            work->vy = 0;
        }
    } else if (work->vy < 0) {
        work->actor.y += work->vy / 2;
        work->vy += 0x11;

        if (work->vy > 0) {
            work->vy = 0;
        }
    }

    ClampBattlePosition(&work->actor.x, &work->actor.y, -0x18, -0x0C);
}

enum BosGaState {
    BOS_GA_STATE_ASSEMBLE,
    BOS_GA_STATE_IDLE,
    BOS_GA_STATE_WALK,
    BOS_GA_STATE_STOMP,
    BOS_GA_STATE_THRUST,
    BOS_GA_STATE_ORBIT,
    BOS_GA_STATE_JUMP,
    BOS_GA_STATE_BODY_CHASE,
    BOS_GA_STATE_BODY_DASH,
    BOS_GA_STATE_BODY_JUMP,
    BOS_GA_STATE_GIMMICK,
    BOS_GA_STATE_DEFEATED
};

void BosGaRequestState(GaWork* work, s32 state) {
    if (work->nextState != BOS_GA_STATE_DEFEATED && work->state != BOS_GA_STATE_DEFEATED) {
        work->nextState = state;
        work->flags |= GA_FLAG_STATE_REQUESTED;
    }
}

s32 BosGaEntryOffsetX(GaWork* work, s16 index) {
    s32 offsetX;

    offsetX = sGaEntryDefs[index].offsetX;

    if (work->flipped) {
        offsetX = -offsetX;
    }

    return offsetX;
}

s32 BosGaEntryOffsetY(GaWork* work, s16 index) {
    return sGaEntryDefs[index].offsetY;
}

s32 BosGaEntryHomeX(GaWork* work, s16 index) {
    return BosGaEntryOffsetX(work, index) + gBtlWork->bossX;
}

s32 BosGaEntryHomeY(GaWork* work, s16 index) {
    return BosGaEntryOffsetY(work, index) + gBtlWork->bossY;
}

s32 BosGaEntryHomeZ(GaWork* work, s16 index) {
    return sGaEntryDefs[index].offsetZ + gBtlWork->bossZ;
}

void BosGaEntryResetHome(GaWork* work, s32 index) {
    GaEntryWork* entry;
    s32 x2;

    entry = &work->entries[index];

    if (!work->flipped) {
        entry->actor.flags |= BTLOBJ_FLAG_FACING_LEFT;
    } else {
        entry->actor.flags &= ~BTLOBJ_FLAG_FACING_LEFT;
    }

    entry->offsetX = BosGaEntryOffsetX(work, index);
    entry->offsetY = BosGaEntryOffsetY(work, index);
    entry->baseX = BosGaEntryHomeX(work, index);
    entry->baseY = BosGaEntryHomeY(work, index);
    entry->baseZ = BosGaEntryHomeZ(work, index);
    x2 = sGaEntryDefs[index].x2;

    if (work->flipped) {
        x2 = -x2;
    }

    entry->x2 = x2;
    entry->y2 = sGaEntryDefs[index].y2;
}

void BosGaUpdateFacing(GaWork* work) {
    s32 flip;
    u32 i;

    flip = FALSE;

    if (gBtlWork->bossX <= gBtlWork->actor->x) {
        flip = TRUE;
    }

    if (work->flipped != flip) {
        work->flipped = flip;

        for (i = 0; i <= 5; i++) {
            BosGaEntryResetHome(work, i);
        }
    }
}

enum BosGaEntryMode {
    BOS_GA_ENTRY_MODE_FOLLOW,
    BOS_GA_ENTRY_MODE_FALL,
    BOS_GA_ENTRY_MODE_DESTROYED = 3
};

void BosGaEntryInit(GaWork* work, u32 index, s32 assemble) {
    GaEntryWork* entry;
    void* gfxTable;

    entry = &work->entries[index];
    entry->index = index;
    entry->bobZ = 0;
    entry->bobAngle = GetRandom();
    entry->mode = BOS_GA_ENTRY_MODE_FOLLOW;
    entry->vz = 0;
    entry->rotation = 0;
    entry->flags = 0;
    entry->counter = 0;
    entry->vx = entry->vy = 0;
    BosGaEntryResetHome(work, index);
    entry->unk_130 = 0;
    entry->unk_134 = 0;
    entry->unk_138 = 0;
    entry->orbitAngle = 0;

    if (assemble != 0) {
        if (index != BOS_GA_PART_HEAD) {
            entry->baseX -= (GetRandom() & 0x1F) << 8;
            entry->baseY -= (GetRandom() & 0x1F) << 8;
        }

        entry->baseZ -= 0xA000;
    }

    InitEnemyBtlObj(&entry->actor, &sBosGaEmyKind, entry->baseX, entry->baseY, entry->baseZ);
    SetEnemyHpFromStats(&entry->actor, sBosGaEmyKind.id, sGaEntryDefs[index].hpScale);
    entry->actor.radiusY = 0x10;

    if (index == BOS_GA_PART_TORSO) {
        entry->actor.flags |= 0x400;
    } else {
        entry->actor.flags |= BTLOBJ_FLAG_NEVER_USES_CARDS;
    }

    entry->actor.flags |= BTLOBJ_FLAG_FACING_LEFT;

    switch (index) {
    case BOS_GA_PART_NEAR_FOOT:
    case BOS_GA_PART_FAR_FOOT:
        entry->flags |= GA_ENTRY_FLAG_NO_BOB;
        break;
    }

    TaskPoolInit(&entry->tasks, 1);
    TaskCreate(&entry->tasks, &gTaskDescBtlShadow, &entry->actor);
    gfxTable = sGaEntryDefs[index].gfxTable;
    entry->tiles = AllocObjTiles(GetMaxSpriteTileBytes(gfxTable, sGaEntryDefs[index].spriteCount), sGaEntryDefs[index].owner);
    AnimInit(&entry->anim, sGaEntryDefs[index].anims, gfxTable);
    AnimStart(&entry->anim, 0, ANIM_FLAG_LOOP);
    entry->gfx = AnimGetGfx(&entry->anim);

    if (index == BOS_GA_PART_TORSO) {
        work->tiles = AllocObjTiles(GetMaxSpriteTileBytes(gBosGaCollarFrames, ARRAY_COUNT(gBosGaCollarFrames)), gBosGaCollarTiles);
        AnimInit(&work->anim, gBosGaCollarAnims, gBosGaCollarFrames);
        AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
        work->gfx = AnimGetGfx(&work->anim);
    }

    ColliderInit(&entry->actor.collider, COLLIDER_TYPE_BOSS, 8, 0x10);
}

void BosGaEntryRelease(GaEntryWork* work) {
    if (!(work->flags & GA_ENTRY_FLAG_RELEASED)) {
        ColliderUnregister(&work->actor.collider);
        ReleaseObjTiles(work->tiles);
        ReleaseEnemyBtlObj(&work->actor);
        TaskPoolDestroy(&work->tasks);
        work->flags |= GA_ENTRY_FLAG_RELEASED;
    }
}

void BosGaReleaseBody() {
    BosGaEntryRelease(&sGaWork->entries[BOS_GA_PART_HEAD]);
    BosGaEntryRelease(&sGaWork->entries[BOS_GA_PART_TORSO]);
}

void BosGaEntryDraw(GaWork* work, GaEntryWork* entry) {
    ObjAffine* affine;
    u16 flags;
    void* pal;
    u16 sx;
    u16 sy;
    GaEntryWork* q;

    if (entry->flags & GA_ENTRY_FLAG_RELEASED) {
        return;
    }

    affine = AllocObjAffineAngle(entry->rotation, TRUE);
    q = entry;
    flags = GetBattleSpritePriorityFlags(entry->actor.y);

    if (work->flipped == TRUE) {
        flags |= SPRITE_FLAG_HFLIP;
    }

    if (StepHitFlash(&entry->actor)) {
        pal = work->palette2;
    } else {
        pal = work->palette;
    }

    WorldToScreen(&sx, &sy, q->actor.x, q->actor.y, q->actor.z);
    DrawSprite(sx + entry->x2, sy + entry->y2, entry->gfx, entry->tiles, pal, affine, flags,
               0xEFFC - ((q->actor.y >> 8) << 2));

    if (entry->index == BOS_GA_PART_TORSO && work->state != BOS_GA_STATE_BODY_CHASE && work->state != BOS_GA_STATE_BODY_DASH && work->state != BOS_GA_STATE_BODY_JUMP) {
        DrawSprite(sx + entry->x2, sy + entry->y2, work->gfx, work->tiles, pal, affine, flags,
                   0xEFFC - ((q->actor.y >> 8) << 2));
    }

    TaskPoolDraw(&entry->tasks);
}

enum BosGaPhase {
    BOS_GA_PHASE_ENTER,
    BOS_GA_PHASE_UPDATE,
    BOS_GA_PHASE_EXIT
};

enum BosGaAssembleStep {
    BOS_GA_ASSEMBLE_STEP_WAIT,
    BOS_GA_ASSEMBLE_STEP_START_FALL,
    BOS_GA_ASSEMBLE_STEP_FALL,
    BOS_GA_ASSEMBLE_STEP_SETTLE,
    BOS_GA_ASSEMBLE_STEP_BOUNCE,
    BOS_GA_ASSEMBLE_STEP_LOWER_HEAD,
    BOS_GA_ASSEMBLE_STEP_DONE
};

u8 BosGaUpdateAssemble(GaWork* work) {
    u32 i = 0;
    GaEntryWork* entry;

    if (work->flags & GA_FLAG_STATE_REQUESTED) {
        work->statePhase = BOS_GA_PHASE_EXIT;
    }

    switch (work->statePhase) {
    case BOS_GA_PHASE_ENTER:
        work->timer = 60;
        work->step = i;
        work->statePhase = BOS_GA_PHASE_UPDATE;
        break;
    case BOS_GA_PHASE_UPDATE:
        switch (work->step) {
        case BOS_GA_ASSEMBLE_STEP_WAIT:
            work->timer--;

            if (work->timer <= 0) {
                work->step = BOS_GA_ASSEMBLE_STEP_START_FALL;
            }

            break;
        case BOS_GA_ASSEMBLE_STEP_START_FALL:
            for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
                entry = &work->entries[i];
                entry->baseVx = 0;
                entry->baseVy = 0;
                entry->baseVz = 1;
                entry->counter = i * 8;

                if (i == 1) {
                    entry->rotation = 0;
                } else {
                    entry->rotation = GetRandom() % 100;
                }
            }

            gBtlWork->bossZ = -0x2000;
            work->timer = 5;
            work->step = BOS_GA_ASSEMBLE_STEP_FALL;
            break;
        case BOS_GA_ASSEMBLE_STEP_FALL:
            for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
                entry = &work->entries[i];

                if (i != 1) {
                    if (entry->counter > 0) {
                        entry->counter--;
                    } else if (entry->counter == 0) {
                        entry->baseX += entry->baseVx;
                        entry->baseY += entry->baseVy;
                        entry->baseZ += entry->baseVz;

                        if (entry->baseVz > 0) {
                            entry->baseVz += 76;

                            if (entry->baseZ > -0x800) {
                                s32 homeZ = BosGaEntryHomeZ(work, i);
                                entry->baseVz = (homeZ - entry->baseZ) / 15;
                                entry->baseAccelZ = -((homeZ - entry->baseZ) * 2) / 900;
                                entry->baseVx = (BosGaEntryHomeX(work, i) - entry->baseX) / 30;
                                entry->baseVy = (BosGaEntryHomeY(work, i) - entry->baseY) / 30;
                                entry->rotationFixed = entry->rotation << 8;
                                entry->landSteps = 30;
                                m4aSongNumStart(SONG_SND_388);
                            }
                        } else {
                            ApproachValue(&entry->rotationFixed, 0x10000, entry->landSteps);
                            entry->rotation = entry->rotationFixed >> 8;
                            entry->baseVz += entry->baseAccelZ;
                            entry->landSteps--;

                            if ((s16)entry->landSteps <= 0) {
                                BosGaEntryResetHome(work, i);
                                entry->rotation = 0;
                                entry->baseVz = 0;
                                entry->counter = -1;
                                work->timer--;

                                if (work->timer <= 0) {
                                    work->timer = 60;
                                    work->step = BOS_GA_ASSEMBLE_STEP_SETTLE;
                                }
                            }
                        }
                    }
                }
            }

            break;
        case BOS_GA_ASSEMBLE_STEP_SETTLE:
            ApproachValue(&gBtlWork->bossZ, 0, work->timer);

            for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
                entry = &work->entries[i];

                if (i != 1) {
                    ApproachValue(&entry->baseZ, BosGaEntryHomeZ(work, i), work->timer);
                }
            }

            work->timer--;

            if (work->timer <= 0) {
                work->timer = 0;
                work->vz = 256;
                work->step = BOS_GA_ASSEMBLE_STEP_BOUNCE;
            }

            break;
        case BOS_GA_ASSEMBLE_STEP_BOUNCE:
            for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
                switch (i) {
                case BOS_GA_PART_TORSO:
                case BOS_GA_PART_NEAR_HAND:
                case BOS_GA_PART_FAR_HAND:
                    work->entries[i].baseZ += work->vz;
                    break;
                }
            }

            work->vz -= 10;

            if (work->vz > 0) {
                work->timer++;
            } else {
                work->timer--;

                if (work->timer <= 0) {
                    work->timer = 60;
                    work->step = BOS_GA_ASSEMBLE_STEP_LOWER_HEAD;
                }
            }

            break;
        case BOS_GA_ASSEMBLE_STEP_LOWER_HEAD:
            for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
                entry = &work->entries[i];

                if (i == 1) {
                    ApproachValue(&entry->baseZ, BosGaEntryHomeZ(work, i), work->timer);
                }
            }

            work->timer--;

            if (work->timer <= 0) {
                work->step = BOS_GA_ASSEMBLE_STEP_DONE;
            }

            break;
        }

        break;
    case BOS_GA_PHASE_EXIT:
        break;
    }

    if (work->statePhase == BOS_GA_PHASE_ENTER) {
        work->statePhase = BOS_GA_PHASE_UPDATE;
    }

    if (work->statePhase == BOS_GA_PHASE_EXIT) {
        work->state = work->nextState;
        work->statePhase = BOS_GA_PHASE_ENTER;
        work->flags &= ~GA_FLAG_STATE_REQUESTED;
    }

    return 1;
}

u8 BosGaUpdateIdle(GaWork* work) {
    s32 distSq;
    s32 dx;
    s32 dy;

    if (work->flags & GA_FLAG_STATE_REQUESTED) {
        work->statePhase = BOS_GA_PHASE_EXIT;
    }

    switch (work->statePhase) {
    case BOS_GA_PHASE_ENTER:
        work->timer = 15;
        break;
    case BOS_GA_PHASE_UPDATE:
        if (gBtlWork->phase == BTL_PHASE_START) {
            break;
        }

        work->timer--;

        if (work->timer > 0) {
            break;
        }

        dx = gBtlWork->actor->x - gBtlWork->bossX;
        dx = (dx * dx) >> 8;
        dy = gBtlWork->actor->y - gBtlWork->bossY;
        dy = (dy * dy) >> 8;
        distSq = (dx + dy) >> 8;

        if (distSq <= 0xE0F) {
            if (!work->attackToggle) {
                if (GetRandom() % 3 != 0) {
                    BosGaRequestState(work, BOS_GA_STATE_IDLE);
                    RequestEnemyCardUse(&work->entries[BOS_GA_PART_TORSO].actor);
                } else {
                    BosGaRequestState(work, BOS_GA_STATE_WALK);
                }
            } else {
                if (GetRandom() % 3 != 0) {
                    BosGaRequestState(work, BOS_GA_STATE_WALK);
                } else {
                    BosGaRequestState(work, BOS_GA_STATE_IDLE);
                    RequestEnemyCardUse(&work->entries[BOS_GA_PART_TORSO].actor);
                }
            }
        } else if (distSq <= 0x270F) {
            if (!work->attackToggle) {
                if (GetRandom() & 1) {
                    BosGaRequestState(work, BOS_GA_STATE_IDLE);
                    RequestEnemyCardUse(&work->entries[BOS_GA_PART_TORSO].actor);
                } else {
                    BosGaRequestState(work, BOS_GA_STATE_WALK);
                }
            } else {
                if (GetRandom() & 1) {
                    BosGaRequestState(work, BOS_GA_STATE_WALK);
                } else {
                    BosGaRequestState(work, BOS_GA_STATE_IDLE);
                    RequestEnemyCardUse(&work->entries[BOS_GA_PART_TORSO].actor);
                }
            }
        } else {
            if (!work->attackToggle) {
                if (GetRandom() % 3 != 0) {
                    BosGaRequestState(work, BOS_GA_STATE_WALK);
                } else {
                    BosGaRequestState(work, BOS_GA_STATE_IDLE);
                    RequestEnemyCardUse(&work->entries[BOS_GA_PART_TORSO].actor);
                }
            } else {
                if (GetRandom() % 3 != 0) {
                    BosGaRequestState(work, BOS_GA_STATE_IDLE);
                    RequestEnemyCardUse(&work->entries[BOS_GA_PART_TORSO].actor);
                } else {
                    BosGaRequestState(work, BOS_GA_STATE_WALK);
                }
            }
        }

        break;
    case BOS_GA_PHASE_EXIT:
        break;
    }

    if (work->statePhase == BOS_GA_PHASE_ENTER) {
        work->statePhase = BOS_GA_PHASE_UPDATE;
    }

    if (work->statePhase == BOS_GA_PHASE_EXIT) {
        work->state = work->nextState;
        work->statePhase = BOS_GA_PHASE_ENTER;
        work->flags &= ~GA_FLAG_STATE_REQUESTED;
    }

    return 1;
}

u8 BosGaUpdateWalk(GaWork* work) {
    u32 i;
    GaEntryWork* entry;
    s32 x, y;

    if (work->flags & GA_FLAG_STATE_REQUESTED) {
        work->statePhase = BOS_GA_PHASE_EXIT;
    }

    for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
        entry = &work->entries[i];

        switch (work->statePhase) {
        case BOS_GA_PHASE_ENTER: {
            s32 distSq;

            if (entry->index != BOS_GA_PART_TORSO) {
                break;
            }

            x = gBtlWork->actor->x;
            y = gBtlWork->actor->y;

            if (!work->attackToggle) {
                if (x > entry->baseX) {
                    x -= 0x2800;

                    if (x < 0) {
                        x += 0x5000;
                    }
                } else {
                    x += 0x2800;

                    if (x > 0x10000) {
                        x -= 0x5000;
                    }
                }
            } else {
                if (x > entry->baseX) {
                    x -= 0x6400;

                    if (x < 0) {
                        x += 0xC800;
                    }
                } else {
                    x += 0x6400;

                    if (x > 0x10000) {
                        x -= 0xC800;
                    }
                }

                y = ((GetRandom() & 80) + 296) << 8;
            }

            work->vx = ((x - entry->baseX) * (x - entry->baseX)) >> 8;
            work->vy = ((y - entry->baseY) * (y - entry->baseY)) >> 8;
            distSq = (work->vx + work->vy) >> 8;
            work->timer = 15;
            work->stepsLeft = distSq / 4900 + 2;
            work->vz = 768;
            work->vzDelta = work->vz * 2 / work->timer;
            work->angle = BosGaGetAngle(entry->baseX, entry->baseY, x, y);
            work->vx = (x - entry->baseX) / ((work->stepsLeft - 1) * work->timer * 2);

            if (work->vx > 640) {
                work->vx = 640;
            } else if (work->vx < -640) {
                work->vx = -640;
            }

            work->vy = (y - entry->baseY) / ((work->stepsLeft - 1) * work->timer * 2);

            if (work->vy > 640) {
                work->vy = 640;
            } else if (work->vy < -640) {
                work->vy = -640;
            }

            if (work->vy >= 0) {
                work->entries[BOS_GA_PART_NEAR_FOOT].flags |= GA_ENTRY_FLAG_STEPPING;
                work->entries[BOS_GA_PART_FAR_FOOT].flags &= ~GA_ENTRY_FLAG_STEPPING;
            } else {
                work->entries[BOS_GA_PART_NEAR_FOOT].flags &= ~GA_ENTRY_FLAG_STEPPING;
                work->entries[BOS_GA_PART_FAR_FOOT].flags |= GA_ENTRY_FLAG_STEPPING;
            }

            BosGaUpdateFacing(work);
            break;
        }
        case BOS_GA_PHASE_UPDATE:
            switch (entry->index) {
            case BOS_GA_PART_NEAR_FOOT:
            case BOS_GA_PART_FAR_FOOT: {
                s32 velocity;

                if (!(entry->flags & GA_ENTRY_FLAG_STEPPING)) {
                    break;
                }

                velocity = work->vx;
                entry->baseX += velocity;

                if (velocity < 0) {
                    if (entry->baseX < 0) {
                        entry->baseX = 0;
                    }
                } else if (velocity > 0) {
                    if (entry->baseX > 0x10000) {
                        entry->baseX = 0x10000;
                    }
                }

                velocity = work->vy;
                entry->baseY += velocity;

                if (velocity < 0) {
                    if (entry->baseY < 0x12800) {
                        entry->baseY = 0x12800;
                    }
                } else if (velocity > 0) {
                    if (entry->baseY > 0x17800) {
                        entry->baseY = 0x17800;
                    }
                }

                entry->baseZ -= work->vz;
                work->vz -= work->vzDelta;
                work->timer--;

                if (work->timer <= 0) {
                    if (!(entry->flags & GA_ENTRY_FLAG_DESTROYED)) {
                        if (entry->index == BOS_GA_PART_NEAR_FOOT) {
                            m4aSongNumStart(SONG_BTL_IRON_FOOTL);
                        } else if (entry->index == BOS_GA_PART_FAR_FOOT) {
                            m4aSongNumStart(SONG_BTL_IRON_FOOTR);
                        }
                    }

                    work->stepsLeft--;
                    entry->baseZ = BosGaEntryHomeZ(work, i);

                    if (work->stepsLeft > 0) {
                        if (work->stepsLeft == 1) {
                            work->timer = 15;
                        } else {
                            work->timer = 30;
                        }

                        work->vz = 768;
                        work->vzDelta = work->vz * 2 / work->timer;
                        work->entries[BOS_GA_PART_FAR_FOOT].flags ^= GA_ENTRY_FLAG_STEPPING;
                        work->entries[BOS_GA_PART_NEAR_FOOT].flags ^= GA_ENTRY_FLAG_STEPPING;
                    } else {
                        BosGaRequestState(work, BOS_GA_STATE_IDLE);
                    }
                }

                break;
            }
            case BOS_GA_PART_TORSO: {
                s32 nearPos, farPos, gap;
                nearPos = work->entries[BOS_GA_PART_NEAR_FOOT].baseX - work->entries[BOS_GA_PART_NEAR_FOOT].offsetX;
                farPos = work->entries[BOS_GA_PART_FAR_FOOT].baseX - work->entries[BOS_GA_PART_FAR_FOOT].offsetX;
                gap = nearPos - farPos >= 0 ? nearPos - farPos : farPos - nearPos;
                entry->baseX = (nearPos < farPos ? gap + 1 : gap) ? nearPos : farPos;
                gBtlWork->bossX = entry->baseX;
                nearPos = work->entries[BOS_GA_PART_NEAR_FOOT].baseY - work->entries[BOS_GA_PART_NEAR_FOOT].offsetY;
                farPos = work->entries[BOS_GA_PART_FAR_FOOT].baseY - work->entries[BOS_GA_PART_FAR_FOOT].offsetY;
                gap = nearPos - farPos >= 0 ? nearPos - farPos : farPos - nearPos;
                entry->baseY = (nearPos < farPos ? gap + 1 : gap) ? nearPos : farPos;
                gBtlWork->bossY = entry->baseY;
                break;
            }
            default:
                BosGaEntryResetHome(work, i);
                break;
            }

            break;
        case BOS_GA_PHASE_EXIT:
            switch (entry->index) {
            case BOS_GA_PART_NEAR_FOOT:
            case BOS_GA_PART_FAR_FOOT:
                entry->flags &= ~GA_ENTRY_FLAG_STEPPING;
                break;
            }

            BosGaEntryResetHome(work, i);
            break;
        }
    }

    if (work->statePhase == BOS_GA_PHASE_ENTER) {
        work->statePhase = BOS_GA_PHASE_UPDATE;
    }

    if (work->statePhase == BOS_GA_PHASE_EXIT) {
        work->state = work->nextState;
        work->statePhase = BOS_GA_PHASE_ENTER;
        work->flags &= ~GA_FLAG_STATE_REQUESTED;
    }

    return 1;
}

enum BosGaStompStep {
    BOS_GA_STOMP_STEP_STAMP,
    BOS_GA_STOMP_STEP_STAMP_END,
    BOS_GA_STOMP_STEP_START_MARCH,
    BOS_GA_STOMP_STEP_MARCH,
    BOS_GA_STOMP_STEP_START_RETURN,
    BOS_GA_STOMP_STEP_RETURN
};

u8 BosGaUpdateStomp(GaWork* work) {
    u32 i = 0;
    GaEntryWork* entry;
    s32 velocity;

    if (work->flags & GA_FLAG_STATE_REQUESTED) {
        work->statePhase = BOS_GA_PHASE_EXIT;
    }

    switch (work->statePhase) {
    case BOS_GA_PHASE_ENTER:
            for (; i < ARRAY_COUNT(work->entries); i++) {
                entry = &work->entries[i];

                switch (entry->index) {
                case BOS_GA_PART_NEAR_FOOT:
                    entry->flags |= GA_ENTRY_FLAG_STEPPING;
                    break;
                case BOS_GA_PART_FAR_FOOT:
                    entry->flags &= ~GA_ENTRY_FLAG_STEPPING;
                    break;
                case BOS_GA_PART_TORSO:
                    work->timer = 10;
                    work->stepsLeft = 5;
                    work->vz = 1024;
                    work->vzDelta = work->vz * 2 / work->timer;
                    work->step = BOS_GA_STOMP_STEP_STAMP;
                    BosGaUpdateFacing(work);
                    break;
                }
            }

            break;
    case BOS_GA_PHASE_UPDATE:
        switch (work->step) {
        case BOS_GA_STOMP_STEP_STAMP:
            for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
                entry = &work->entries[i];

                switch (entry->index) {
                case BOS_GA_PART_NEAR_FOOT:
                case BOS_GA_PART_FAR_FOOT:
                    if (!(entry->flags & GA_ENTRY_FLAG_STEPPING)) {
                        break;
                    }

                    entry->baseZ -= work->vz;
                    work->vz -= work->vzDelta;
                    work->timer--;

                    if (work->timer <= 0) {
                        if (!(entry->flags & GA_ENTRY_FLAG_DESTROYED)) {
                            if (entry->index == BOS_GA_PART_NEAR_FOOT) {
                                m4aSongNumStart(SONG_BTL_IRON_FOOTL);
                            } else if (entry->index == BOS_GA_PART_FAR_FOOT) {
                                m4aSongNumStart(SONG_BTL_IRON_FOOTR);
                            }
                        }

                        work->stepsLeft--;
                        entry->baseZ = BosGaEntryHomeZ(work, i);

                        if (work->stepsLeft > 0) {
                            if (work->stepsLeft == 1) {
                                work->timer = 10;
                            } else {
                                work->timer = 20;
                            }

                            work->vz = 896;
                            work->vzDelta = work->vz * 2 / work->timer;
                            work->entries[BOS_GA_PART_FAR_FOOT].flags ^= GA_ENTRY_FLAG_STEPPING;
                            work->entries[BOS_GA_PART_NEAR_FOOT].flags ^= GA_ENTRY_FLAG_STEPPING;
                        } else {
                            work->step = BOS_GA_STOMP_STEP_STAMP_END;
                        }
                    }

                    break;
                default:
                    BosGaEntryResetHome(work, i);
                    break;
                }
            }

            break;
        case BOS_GA_STOMP_STEP_STAMP_END:
            for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
                entry = &work->entries[i];

                switch (entry->index) {
                case BOS_GA_PART_TORSO:
                    work->step = BOS_GA_STOMP_STEP_START_MARCH;
                    break;
                case BOS_GA_PART_NEAR_FOOT:
                case BOS_GA_PART_FAR_FOOT:
                    entry->flags &= ~GA_ENTRY_FLAG_STEPPING;
                    break;
                }

                BosGaEntryResetHome(work, i);
            }

            break;
        case BOS_GA_STOMP_STEP_START_MARCH:
            for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
                entry = &work->entries[i];

                switch (entry->index) {
                case BOS_GA_PART_NEAR_FOOT:
                    entry->flags |= GA_ENTRY_FLAG_STEPPING;
                    break;
                case BOS_GA_PART_FAR_FOOT:
                    entry->flags &= ~GA_ENTRY_FLAG_STEPPING;
                    break;
                case BOS_GA_PART_TORSO:
                    work->timer = 10;
                    work->stepsLeft = 5;
                    work->vz = 1024;
                    work->vzDelta = work->vz * 2 / work->timer;
                    work->angle = BosGaGetAngle(entry->baseX, entry->baseY, gBtlWork->actor->x, gBtlWork->actor->y);
                    work->step = BOS_GA_STOMP_STEP_MARCH;
                    break;
                }
            }

            break;
        case BOS_GA_STOMP_STEP_MARCH:
            for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
                entry = &work->entries[i];

                switch (entry->index) {
                case BOS_GA_PART_NEAR_FOOT:
                case BOS_GA_PART_FAR_FOOT:
                    if (!(entry->flags & GA_ENTRY_FLAG_STEPPING)) {
                        break;
                    }

                    velocity = gSineTable[work->angle] * 972 >> 8;
                    entry->baseX += velocity;

                    if (velocity < 0) {
                        if (entry->baseX < 0) {
                            entry->baseX = 0;
                        }
                    } else if (velocity > 0) {
                        if (entry->baseX > 0x10000) {
                            entry->baseX = 0x10000;
                        }
                    }

                    velocity = -gSineTable[work->angle + 64] * 972 >> 8;
                    entry->baseY += velocity;

                    if (velocity < 0) {
                        if (entry->baseY < 0x12800) {
                            entry->baseY = 0x12800;
                        }
                    } else if (velocity > 0) {
                        if (entry->baseY > 0x17800) {
                            entry->baseY = 0x17800;
                        }
                    }

                    entry->baseZ -= work->vz;
                    work->vz -= work->vzDelta;
                    work->timer--;

                    if (work->timer <= 0) {
                        if (!(entry->flags & GA_ENTRY_FLAG_DESTROYED)) {
                            if (ApplyAttackBox(226, entry->actor.x, entry->actor.y, entry->actor.z, 16, 16, 16)) {
                                m4aSongNumStart(SONG_BTL_IRON_HIT00);
                            } else if (entry->index == BOS_GA_PART_NEAR_FOOT) {
                                m4aSongNumStart(SONG_BTL_IRON_FOOTL);
                            } else if (entry->index == BOS_GA_PART_FAR_FOOT) {
                                m4aSongNumStart(SONG_BTL_IRON_FOOTR);
                            }
                        }

                        work->stepsLeft--;
                        entry->baseZ = BosGaEntryHomeZ(work, i);

                        if (work->stepsLeft > 0) {
                            if (work->stepsLeft == 1) {
                                work->timer = 10;
                            } else {
                                work->timer = 20;
                            }

                            work->vz = 896;
                            work->vzDelta = work->vz * 2 / work->timer;
                            work->entries[BOS_GA_PART_FAR_FOOT].flags ^= GA_ENTRY_FLAG_STEPPING;
                            work->entries[BOS_GA_PART_NEAR_FOOT].flags ^= GA_ENTRY_FLAG_STEPPING;
                        } else {
                            work->step = BOS_GA_STOMP_STEP_START_RETURN;
                        }
                    }

                    break;
                }
            }

            break;
        case BOS_GA_STOMP_STEP_START_RETURN:
            work->timer = 20;
            work->stepsLeft = 6;
            work->vz = 1024;
            work->vzDelta = work->vz * 2 / work->timer;
            work->step = BOS_GA_STOMP_STEP_RETURN;

            for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
                entry = &work->entries[i];

                switch (entry->index) {
                case BOS_GA_PART_NEAR_FOOT:
                    entry->flags |= GA_ENTRY_FLAG_STEPPING;
                    entry->baseVx = (BosGaEntryHomeX(work, i) - entry->baseX) / (work->stepsLeft * work->timer / 2);
                    entry->baseVy = (BosGaEntryHomeY(work, i) - entry->baseY) / (work->stepsLeft * work->timer / 2);
                    break;
                case BOS_GA_PART_FAR_FOOT:
                    entry->flags &= ~GA_ENTRY_FLAG_STEPPING;
                    entry->baseVx = (BosGaEntryHomeX(work, i) - entry->baseX) / (work->stepsLeft * work->timer / 2);
                    entry->baseVy = (BosGaEntryHomeY(work, i) - entry->baseY) / (work->stepsLeft * work->timer / 2);
                    break;
                }
            }

            break;
        case BOS_GA_STOMP_STEP_RETURN:
            for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
                entry = &work->entries[i];

                switch (entry->index) {
                case BOS_GA_PART_NEAR_FOOT:
                case BOS_GA_PART_FAR_FOOT:
                    if (!(entry->flags & GA_ENTRY_FLAG_STEPPING)) {
                        break;
                    }

                    entry->baseX += entry->baseVx;

                    if (entry->baseVx < 0) {
                        if (entry->baseX < 0) {
                            entry->baseX = 0;
                        }
                    } else if (entry->baseVx > 0) {
                        if (entry->baseX > 0x10000) {
                            entry->baseX = 0x10000;
                        }
                    }

                    entry->baseY += entry->baseVy;

                    if (entry->baseVy < 0) {
                        if (entry->baseY < 0x12800) {
                            entry->baseY = 0x12800;
                        }
                    } else if (entry->baseVy > 0) {
                        if (entry->baseY > 0x17800) {
                            entry->baseY = 0x17800;
                        }
                    }

                    entry->baseZ -= work->vz;
                    work->vz -= work->vzDelta;
                    work->timer--;

                    if (work->timer <= 0) {
                        if (!(entry->flags & GA_ENTRY_FLAG_DESTROYED)) {
                            if (entry->index == BOS_GA_PART_NEAR_FOOT) {
                                m4aSongNumStart(SONG_BTL_IRON_FOOTL);
                            } else if (entry->index == BOS_GA_PART_FAR_FOOT) {
                                m4aSongNumStart(SONG_BTL_IRON_FOOTR);
                            }
                        }

                        work->stepsLeft--;
                        entry->baseZ = BosGaEntryHomeZ(work, i);

                        if (work->stepsLeft > 0) {
                            work->timer = 20;
                            work->vz = 896;
                            work->vzDelta = work->vz * 2 / work->timer;
                            work->entries[BOS_GA_PART_FAR_FOOT].flags ^= GA_ENTRY_FLAG_STEPPING;
                            work->entries[BOS_GA_PART_NEAR_FOOT].flags ^= GA_ENTRY_FLAG_STEPPING;
                        } else {
                            BosGaRequestState(work, BOS_GA_STATE_IDLE);
                        }
                    }

                    break;
                }
            }

            break;
        }

        break;
    case BOS_GA_PHASE_EXIT:
        for (; i < ARRAY_COUNT(work->entries); i++) {
            entry = &work->entries[i];

            switch (entry->index) {
            case BOS_GA_PART_TORSO:
                ClearBtlObjActionFlags(&entry->actor);
                break;
            case BOS_GA_PART_NEAR_FOOT:
            case BOS_GA_PART_FAR_FOOT:
                entry->flags &= ~GA_ENTRY_FLAG_STEPPING;
                break;
            }

            BosGaEntryResetHome(work, i);
        }

        break;
    }

    if (work->statePhase == BOS_GA_PHASE_ENTER) {
        work->statePhase = BOS_GA_PHASE_UPDATE;
    }

    if (work->statePhase == BOS_GA_PHASE_EXIT) {
        work->state = work->nextState;
        work->statePhase = BOS_GA_PHASE_ENTER;
        work->flags &= ~GA_FLAG_STATE_REQUESTED;
    }

    return 1;
}

enum BosGaThrustStep {
    BOS_GA_THRUST_STEP_FAR_WINDUP,
    BOS_GA_THRUST_STEP_FAR_THRUST,
    BOS_GA_THRUST_STEP_NEAR_WINDUP,
    BOS_GA_THRUST_STEP_NEAR_THRUST
};

u8 BosGaUpdateThrust(GaWork* work) {
    GaEntryWork* entry;
    u32 i;

    if (work->flags & GA_FLAG_STATE_REQUESTED) {
        work->statePhase = BOS_GA_PHASE_EXIT;
    }

    for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
        entry = &work->entries[i];

        switch (work->statePhase) {
        case BOS_GA_PHASE_ENTER:
            switch (entry->index) {
            case BOS_GA_PART_TORSO:
                work->timer = 0;
                work->step = BOS_GA_THRUST_STEP_FAR_WINDUP;
                break;
            case BOS_GA_PART_NEAR_HAND:
            case BOS_GA_PART_FAR_HAND:
                entry->flags |= GA_ENTRY_FLAG_NO_BOB;
                break;
            }

            break;
        case BOS_GA_PHASE_UPDATE:
            switch (work->step) {
            case BOS_GA_THRUST_STEP_FAR_WINDUP:
                if (entry->index == BOS_GA_PART_FAR_HAND) {
                    entry->baseX = entry->baseX + (!work->flipped ? 0x80 : -0x80);
                    work->timer++;

                    if (work->timer > 30) {
                        work->timer = 0;
                        work->step = BOS_GA_THRUST_STEP_FAR_THRUST;
                    }
                } else {
                    BosGaEntryResetHome(work, i);
                }

                break;
            case BOS_GA_THRUST_STEP_FAR_THRUST:
                if (entry->index == BOS_GA_PART_FAR_HAND) {
                    entry->baseX = entry->baseX + (!work->flipped ? -0x300 : 0x300);
                    entry->baseY += 0x133;

                    if (!(entry->flags & GA_ENTRY_FLAG_DESTROYED)) {
                        if (ApplyAttackBox(0xE3, entry->actor.x, entry->actor.y, entry->actor.z, 0x10, 0x10, 0x20)) {
                            m4aSongNumStart(SONG_BTL_IRON_HIT00);
                        }
                    }

                    work->timer++;

                    if (work->timer > 15) {
                        work->timer = 0;
                        work->step = BOS_GA_THRUST_STEP_NEAR_WINDUP;
                    }
                } else {
                    BosGaEntryResetHome(work, i);
                }

                break;
            case BOS_GA_THRUST_STEP_NEAR_WINDUP:
                if (entry->index == BOS_GA_PART_NEAR_HAND) {
                    entry->baseX = entry->baseX + (!work->flipped ? 0x80 : -0x80);
                    work->timer++;

                    if (work->timer > 30) {
                        work->timer = 0;
                        work->step = BOS_GA_THRUST_STEP_NEAR_THRUST;
                    }
                } else {
                    BosGaEntryResetHome(work, i);
                }

                break;
            case BOS_GA_THRUST_STEP_NEAR_THRUST:
                if (entry->index == BOS_GA_PART_NEAR_HAND) {
                    entry->baseX = entry->baseX + (!work->flipped ? -0x300 : 0x300);
                    entry->baseY -= 0x133;

                    if (!(entry->flags & GA_ENTRY_FLAG_DESTROYED)) {
                        if (ApplyAttackBox(0xE3, entry->actor.x, entry->actor.y, entry->actor.z, 0x10, 0x10, 0x20)) {
                            m4aSongNumStart(SONG_BTL_IRON_HIT00);
                        }
                    }

                    work->timer++;

                    if (work->timer > 15) {
                        BosGaRequestState(work, BOS_GA_STATE_IDLE);
                    }
                } else {
                    BosGaEntryResetHome(work, i);
                }

                break;
            }

            break;
        case BOS_GA_PHASE_EXIT:
            switch (entry->index) {
            case BOS_GA_PART_TORSO:
                ClearBtlObjActionFlags(&entry->actor);
                break;
            case BOS_GA_PART_NEAR_HAND:
            case BOS_GA_PART_FAR_HAND:
                entry->flags &= ~GA_ENTRY_FLAG_NO_BOB;
                break;
            }

            BosGaEntryResetHome(work, i);
            break;
        }
    }

    if (work->statePhase == BOS_GA_PHASE_ENTER) {
        work->statePhase = BOS_GA_PHASE_UPDATE;
    }

    if (work->statePhase == BOS_GA_PHASE_EXIT) {
        work->state = work->nextState;
        work->statePhase = BOS_GA_PHASE_ENTER;
        work->flags &= ~GA_FLAG_STATE_REQUESTED;
    }

    return 1;
}

enum BosGaOrbitStep {
    BOS_GA_ORBIT_STEP_WINDUP,
    BOS_GA_ORBIT_STEP_SPIN
};

u8 BosGaUpdateOrbit(GaWork* work) {
    GaEntryWork* entry;
    u32 i;
    s32 angle;

    if (work->flags & GA_FLAG_STATE_REQUESTED) {
        work->statePhase = BOS_GA_PHASE_EXIT;
    }

    for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
        entry = &work->entries[i];

        switch (work->statePhase) {
        case BOS_GA_PHASE_ENTER:
            switch (entry->index) {
            case BOS_GA_PART_TORSO:
                work->step = BOS_GA_ORBIT_STEP_WINDUP;
                work->timer = 0;
                work->orbitRadius = 0x1E00;
                break;
            case BOS_GA_PART_NEAR_HAND:
                entry->orbitAngle = 0x80;
                AnimStart(&entry->anim, 1, ANIM_FLAG_LOOP);
                break;
            case BOS_GA_PART_FAR_HAND:
                entry->orbitAngle = !work->flipped ? 0xC0 : 0x40;
                AnimStart(&entry->anim, 1, ANIM_FLAG_LOOP);
                break;
            }

            break;
        case BOS_GA_PHASE_UPDATE:
            switch (work->step) {
            case BOS_GA_ORBIT_STEP_WINDUP:
                switch (entry->index) {
                case BOS_GA_PART_TORSO:
                    work->timer++;

                    if (work->timer > 31) {
                        work->timer = 0;
                        work->step = BOS_GA_ORBIT_STEP_SPIN;
                    }

                    break;
                case BOS_GA_PART_NEAR_HAND:
                    angle = entry->orbitAngle;
                    entry->orbitAngle = !work->flipped ? angle - 1 : angle + 1;
                    entry->baseX = (gSineTable[entry->orbitAngle] * work->orbitRadius >> 8) + work->entries[BOS_GA_PART_TORSO].baseX;
                    entry->baseY = (-gSineTable[entry->orbitAngle + 0x40] * work->orbitRadius >> 8) + work->entries[BOS_GA_PART_TORSO].baseY;
                    break;
                case BOS_GA_PART_FAR_HAND:
                    angle = entry->orbitAngle;
                    entry->orbitAngle = !work->flipped ? angle + 1 : angle - 1;
                    entry->baseX = (gSineTable[entry->orbitAngle] * work->orbitRadius >> 8) + work->entries[BOS_GA_PART_TORSO].baseX;
                    entry->baseY = (-gSineTable[entry->orbitAngle + 0x40] * work->orbitRadius >> 8) + work->entries[BOS_GA_PART_TORSO].baseY;
                    break;
                default:
                    BosGaEntryResetHome(work, i);
                    break;
                }

                break;
            case BOS_GA_ORBIT_STEP_SPIN:
                switch (entry->index) {
                case BOS_GA_PART_TORSO:
                    work->orbitRadius += 0x59;
                    work->timer++;

                    if (work->timer > 0x7F) {
                        BosGaRequestState(work, BOS_GA_STATE_IDLE);
                    }

                    break;
                case BOS_GA_PART_NEAR_HAND:
                case BOS_GA_PART_FAR_HAND:
                    entry->orbitAngle = entry->orbitAngle + 4;

                    if (!(entry->flags & GA_ENTRY_FLAG_DESTROYED)) {
                        if (ApplyAttackBox(0xE4, entry->actor.x, entry->actor.y, entry->actor.z, 0x10, 0x10, 0x20)) {
                            m4aSongNumStart(SONG_BTL_IRON_HIT00);
                        }
                    }

                    entry->baseX = (gSineTable[entry->orbitAngle] * work->orbitRadius >> 8) + work->entries[BOS_GA_PART_TORSO].baseX;
                    entry->baseY = (-gSineTable[entry->orbitAngle + 0x40] * work->orbitRadius >> 8) + work->entries[BOS_GA_PART_TORSO].baseY;
                    break;
                default:
                    BosGaEntryResetHome(work, i);
                    break;
                }

                break;
            }

            break;
        case BOS_GA_PHASE_EXIT:
            switch (entry->index) {
            case BOS_GA_PART_TORSO:
                ClearBtlObjActionFlags(&entry->actor);
                break;
            case BOS_GA_PART_NEAR_HAND:
            case BOS_GA_PART_FAR_HAND:
                AnimStart(&entry->anim, 0, ANIM_FLAG_LOOP);
                break;
            }

            BosGaEntryResetHome(work, i);
            break;
        }
    }

    if (work->statePhase == BOS_GA_PHASE_ENTER) {
        work->statePhase = BOS_GA_PHASE_UPDATE;
    }

    if (work->statePhase == BOS_GA_PHASE_EXIT) {
        work->state = work->nextState;
        work->statePhase = BOS_GA_PHASE_ENTER;
        work->flags &= ~GA_FLAG_STATE_REQUESTED;
    }

    return 1;
}

enum BosGaJumpStep {
    BOS_GA_JUMP_STEP_CROUCH,
    BOS_GA_JUMP_STEP_LEAP,
    BOS_GA_JUMP_STEP_AIRBORNE,
    BOS_GA_JUMP_STEP_LAND,
    BOS_GA_JUMP_STEP_RECOVER
};

u8 BosGaUpdateJump(GaWork* work) {
    GaEntryWork* entry;
    u32 i;
    s32 vz;

    entry = NULL;

    if (work->flags & GA_FLAG_STATE_REQUESTED) {
        work->statePhase = BOS_GA_PHASE_EXIT;
    }

    switch (work->statePhase) {
    case BOS_GA_PHASE_ENTER:
        for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
            entry = &work->entries[i];
            entry->flags |= GA_ENTRY_FLAG_NO_BOB;

            if (i == 0) {
                AnimStart(&work->anim, 1, ANIM_FLAG_LOOP);
                AnimStart(&entry->anim, 2, ANIM_FLAG_LOOP);
            }
        }

        work->step = BOS_GA_JUMP_STEP_CROUCH;
        break;
    case BOS_GA_PHASE_UPDATE:
        switch (work->step) {
        case BOS_GA_JUMP_STEP_CROUCH:
            gBtlWork->bossZ += 0x33;

            if (gBtlWork->bossZ > 0x1800) {
                work->step = BOS_GA_JUMP_STEP_LEAP;
            }

            for (i = 0; i <= 5; i++) {
                if (i <= 3) {
                    BosGaEntryResetHome(work, i);
                }
            }

            break;
        case BOS_GA_JUMP_STEP_LEAP:
            gBtlWork->bossZ = 0;
            work->vx = (gBtlWork->actor->x - gBtlWork->bossX) / 60;
            work->vy = (gBtlWork->actor->y - gBtlWork->bossY) / 60;
            work->vz = 0x600;
            work->step = BOS_GA_JUMP_STEP_AIRBORNE;
            break;
        case BOS_GA_JUMP_STEP_AIRBORNE:
            gBtlWork->bossX += work->vx;
            gBtlWork->bossY += work->vy;
            gBtlWork->bossZ -= work->vz;
            work->vz -= 0x33;

            if (gBtlWork->bossZ > 0) {
                BtlMapStartShake();
                m4aSongNumStart(SONG_BTL_IRON_RUMB);
                ApplyAttackBox(0xE5, gBtlWork->viewX, gBtlWork->viewY, 0, 0x140, 0xF0, 1);
                work->step = BOS_GA_JUMP_STEP_LAND;
            }

            for (i = 0; i <= 5; i++) {
                BosGaEntryResetHome(work, i);
            }

            break;
        case BOS_GA_JUMP_STEP_LAND:
            gBtlWork->bossZ = 0;
            work->vz = 0x100;
            work->step = BOS_GA_JUMP_STEP_RECOVER;
            break;
        case BOS_GA_JUMP_STEP_RECOVER:
            gBtlWork->bossZ += work->vz;
            vz = work->vz - 7;
            work->vz = vz;

            if (gBtlWork->bossZ <= 0 && vz < 0) {
                BosGaRequestState(work, BOS_GA_STATE_IDLE);
            }

            for (i = 0; i <= 5; i++) {
                if (i <= 3) {
                    BosGaEntryResetHome(work, i);
                }
            }

            break;
        }

        break;
    case BOS_GA_PHASE_EXIT:
        gBtlWork->bossZ = 0;

        for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
            entry = &work->entries[i];

            switch (i) {
            case BOS_GA_PART_TORSO:
                AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
                AnimStart(&entry->anim, 0, ANIM_FLAG_LOOP);
                ClearBtlObjActionFlags(&entry->actor);
            default:
                entry->flags &= ~GA_ENTRY_FLAG_NO_BOB;
                break;
            case BOS_GA_PART_NEAR_FOOT:
            case BOS_GA_PART_FAR_FOOT:
                break;
            }

            BosGaEntryResetHome(work, i);
        }

        break;
    }

    if (work->statePhase == BOS_GA_PHASE_ENTER) {
        work->statePhase = BOS_GA_PHASE_UPDATE;
    }

    if (work->statePhase == BOS_GA_PHASE_EXIT) {
        work->state = work->nextState;
        work->statePhase = BOS_GA_PHASE_ENTER;
        work->flags &= ~GA_FLAG_STATE_REQUESTED;
    }

    return 1;
}

enum BosGaBodyStep {
    BOS_GA_BODY_STEP_LOWER,
    BOS_GA_BODY_STEP_START_MOVE,
    BOS_GA_BODY_STEP_MOVE,
    BOS_GA_BODY_STEP_STOP,
    BOS_GA_BODY_STEP_RAISE
};

u8 BosGaUpdateBodyChase(GaWork* work) {
    GaEntryWork* entry;
    u32 i;

    entry = NULL;

    if (work->flags & GA_FLAG_STATE_REQUESTED) {
        work->statePhase = BOS_GA_PHASE_EXIT;
    }

    switch (work->statePhase) {
    case BOS_GA_PHASE_ENTER:
        for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
            entry = &work->entries[i];
            entry->flags |= GA_ENTRY_FLAG_NO_BOB;

            switch (i) {
            case BOS_GA_PART_TORSO:
                AnimStart(&entry->anim, 1, ANIM_FLAG_LOOP);
                break;
            case BOS_GA_PART_HEAD:
                entry->baseX = gBtlWork->bossX;
                entry->baseY = gBtlWork->bossY + 0x100;
                break;
            }
        }

        work->step = BOS_GA_BODY_STEP_LOWER;
        break;
    case BOS_GA_PHASE_UPDATE:
        switch (work->step) {
        case BOS_GA_BODY_STEP_LOWER:
            gBtlWork->bossZ += 0x66;

            if (gBtlWork->bossZ > 0x25FF) {
                work->step = BOS_GA_BODY_STEP_START_MOVE;
            }

            for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
                entry = &work->entries[i];

                if (i <= 1) {
                    entry->baseZ = BosGaEntryHomeZ(work, i);
                }
            }

            break;
        case BOS_GA_BODY_STEP_START_MOVE:
            work->vx = 0;
            work->vy = 0;
            work->timer = 0x12C;
            work->step = BOS_GA_BODY_STEP_MOVE;
            BosGaUpdateFacing(work);
            break;
        case BOS_GA_BODY_STEP_MOVE:
            work->angle = BosGaGetAngle(gBtlWork->bossX, gBtlWork->bossY, gBtlWork->actor->x, gBtlWork->actor->y);
            work->vx += gSineTable[work->angle] * 5 >> 8;

            if (work->vx > 0x200) {
                work->vx = 0x200;
            } else if (work->vx < -0x200) {
                work->vx = -0x200;
            }

            gBtlWork->bossX += work->vx;

            if (work->vx < 0) {
                if (gBtlWork->bossX < 0) {
                    work->vx = -work->vx;
                    gBtlWork->bossX = 0;
                    BosGaUpdateFacing(work);
                }
            } else if (work->vx > 0) {
                if (gBtlWork->bossX > 0x10000) {
                    work->vx = -work->vx;
                    gBtlWork->bossX = 0x10000;
                    BosGaUpdateFacing(work);
                }
            }

            work->vy += -gSineTable[work->angle + 0x40] * 5 >> 8;

            if (work->vy > 0x200) {
                work->vy = 0x200;
            } else if (work->vy < -0x200) {
                work->vy = -0x200;
            }

            gBtlWork->bossY += work->vy;

            if (work->vy < 0) {
                if (gBtlWork->bossY <= 0x127FF) {
                    work->vy = -work->vy;
                    gBtlWork->bossY = 0x12800;
                    BosGaUpdateFacing(work);
                }
            } else if (work->vy > 0) {
                if (gBtlWork->bossY > 0x17800) {
                    work->vy = -work->vy;
                    gBtlWork->bossY = 0x17800;
                    BosGaUpdateFacing(work);
                }
            }

            if (ApplyAttackBox(0xE6, work->entries[BOS_GA_PART_TORSO].actor.x, work->entries[BOS_GA_PART_TORSO].actor.y, work->entries[BOS_GA_PART_TORSO].actor.z, 0x10, 0x10, 0x18)) {
                m4aSongNumStart(SONG_BTL_MON_HIT03);
                work->step = BOS_GA_BODY_STEP_STOP;
            }

            work->timer--;

            if (work->timer < 0) {
                work->step = BOS_GA_BODY_STEP_STOP;
            }

            for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
                entry = &work->entries[i];

                switch (i) {
                case BOS_GA_PART_TORSO:
                    entry->baseX = gBtlWork->bossX;
                    entry->baseY = gBtlWork->bossY;
                    break;
                case BOS_GA_PART_HEAD:
                    entry->baseX = gBtlWork->bossX;
                    entry->baseY = gBtlWork->bossY + 0x100;
                    break;
                }
            }

            break;
        case BOS_GA_BODY_STEP_STOP:
            work->step = BOS_GA_BODY_STEP_RAISE;
            break;
        case BOS_GA_BODY_STEP_RAISE:
            gBtlWork->bossZ -= 0x33;

            if (gBtlWork->bossZ <= 0) {
                BosGaRequestState(work, BOS_GA_STATE_IDLE);
            }

            for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
                entry = &work->entries[i];

                if (i <= 1) {
                    entry->baseZ = BosGaEntryHomeZ(work, i);
                }
            }

            break;
        }

        break;
    case BOS_GA_PHASE_EXIT:
        gBtlWork->bossZ = 0;

        for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
            entry = &work->entries[i];

            switch (i) {
            case BOS_GA_PART_TORSO:
                AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
                AnimStart(&entry->anim, 0, ANIM_FLAG_LOOP);
                ClearBtlObjActionFlags(&entry->actor);
            default:
                entry->flags &= ~GA_ENTRY_FLAG_NO_BOB;
                break;
            case BOS_GA_PART_NEAR_FOOT:
            case BOS_GA_PART_FAR_FOOT:
                break;
            }

            BosGaEntryResetHome(work, i);
        }

        break;
    }

    if (work->statePhase == BOS_GA_PHASE_ENTER) {
        work->statePhase = BOS_GA_PHASE_UPDATE;
    }

    if (work->statePhase == BOS_GA_PHASE_EXIT) {
        work->state = work->nextState;
        work->statePhase = BOS_GA_PHASE_ENTER;
        work->flags &= ~GA_FLAG_STATE_REQUESTED;
    }

    return 1;
}

u8 BosGaUpdateBodyDash(GaWork* work) {
    GaEntryWork* entry;
    u32 i;

    entry = NULL;

    if (work->flags & GA_FLAG_STATE_REQUESTED) {
        work->statePhase = BOS_GA_PHASE_EXIT;
    }

    switch (work->statePhase) {
    case BOS_GA_PHASE_ENTER:
        for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
            entry = &work->entries[i];
            entry->flags |= GA_ENTRY_FLAG_NO_BOB;

            switch (i) {
            case BOS_GA_PART_TORSO:
                AnimStart(&entry->anim, 1, ANIM_FLAG_LOOP);
                break;
            case BOS_GA_PART_HEAD:
                entry->baseX = gBtlWork->bossX;
                entry->baseY = gBtlWork->bossY + 0x100;
                break;
            }
        }

        work->step = BOS_GA_BODY_STEP_LOWER;
        break;
    case BOS_GA_PHASE_UPDATE:
        switch (work->step) {
        case BOS_GA_BODY_STEP_LOWER:
            gBtlWork->bossZ += 0x66;

            if (gBtlWork->bossZ > 0x25FF) {
                work->step = BOS_GA_BODY_STEP_START_MOVE;
            }

            for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
                entry = &work->entries[i];

                if (i <= 1) {
                    entry->baseZ = BosGaEntryHomeZ(work, i);
                }
            }

            break;
        case BOS_GA_BODY_STEP_START_MOVE:
            work->angle = BosGaGetAngle(gBtlWork->bossX, gBtlWork->bossY, gBtlWork->actor->x, gBtlWork->actor->y);
            work->vx = gSineTable[work->angle] * 4;
            work->vy = -gSineTable[work->angle + 0x40] * 4;
            work->timer = 0x12C;
            work->step = BOS_GA_BODY_STEP_MOVE;
            BosGaUpdateFacing(work);
            break;
        case BOS_GA_BODY_STEP_MOVE:
            gBtlWork->bossX += work->vx;

            if (work->vx < 0) {
                if (gBtlWork->bossX < 0) {
                    work->vx = -work->vx;
                    gBtlWork->bossX = 0;
                    BosGaUpdateFacing(work);
                }
            } else if (work->vx > 0) {
                if (gBtlWork->bossX > 0x10000) {
                    work->vx = -work->vx;
                    gBtlWork->bossX = 0x10000;
                    BosGaUpdateFacing(work);
                }
            }

            gBtlWork->bossY += work->vy;

            if (work->vy < 0) {
                if (gBtlWork->bossY <= 0x127FF) {
                    work->vy = -work->vy;
                    gBtlWork->bossY = 0x12800;
                    BosGaUpdateFacing(work);
                }
            } else if (work->vy > 0) {
                if (gBtlWork->bossY > 0x17800) {
                    work->vy = -work->vy;
                    gBtlWork->bossY = 0x17800;
                    BosGaUpdateFacing(work);
                }
            }

            if (ApplyAttackBox(0xE6, work->entries[BOS_GA_PART_TORSO].actor.x, work->entries[BOS_GA_PART_TORSO].actor.y, work->entries[BOS_GA_PART_TORSO].actor.z, 0x10, 0x10, 0x18)) {
                m4aSongNumStart(SONG_BTL_MON_HIT03);
                work->step = BOS_GA_BODY_STEP_STOP;
            }

            work->timer--;

            if (work->timer < 0) {
                work->step = BOS_GA_BODY_STEP_STOP;
            }

            for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
                entry = &work->entries[i];

                switch (i) {
                case BOS_GA_PART_TORSO:
                    entry->baseX = gBtlWork->bossX;
                    entry->baseY = gBtlWork->bossY;
                    break;
                case BOS_GA_PART_HEAD:
                    entry->baseX = gBtlWork->bossX;
                    entry->baseY = gBtlWork->bossY + 0x100;
                    break;
                }
            }

            break;
        case BOS_GA_BODY_STEP_STOP:
            work->step = BOS_GA_BODY_STEP_RAISE;
            break;
        case BOS_GA_BODY_STEP_RAISE:
            gBtlWork->bossZ -= 0x33;

            if (gBtlWork->bossZ <= 0) {
                BosGaRequestState(work, BOS_GA_STATE_IDLE);
            }

            for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
                entry = &work->entries[i];

                if (i <= 1) {
                    entry->baseZ = BosGaEntryHomeZ(work, i);
                }
            }

            break;
        }

        break;
    case BOS_GA_PHASE_EXIT:
        gBtlWork->bossZ = 0;

        for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
            entry = &work->entries[i];

            switch (i) {
            case BOS_GA_PART_TORSO:
                AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
                AnimStart(&entry->anim, 0, ANIM_FLAG_LOOP);
                ClearBtlObjActionFlags(&entry->actor);
            default:
                entry->flags &= ~GA_ENTRY_FLAG_NO_BOB;
                break;
            case BOS_GA_PART_NEAR_FOOT:
            case BOS_GA_PART_FAR_FOOT:
                break;
            }

            BosGaEntryResetHome(work, i);
        }

        break;
    }

    if (work->statePhase == BOS_GA_PHASE_ENTER) {
        work->statePhase = BOS_GA_PHASE_UPDATE;
    }

    if (work->statePhase == BOS_GA_PHASE_EXIT) {
        work->state = work->nextState;
        work->statePhase = BOS_GA_PHASE_ENTER;
        work->flags &= ~GA_FLAG_STATE_REQUESTED;
    }

    return 1;
}

u8 BosGaUpdateBodyJump(GaWork* work) {
    GaEntryWork* entry;
    u32 i;

    entry = NULL;

    if (work->flags & GA_FLAG_STATE_REQUESTED) {
        work->statePhase = BOS_GA_PHASE_EXIT;
    }

    switch (work->statePhase) {
    case BOS_GA_PHASE_ENTER:
        for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
            entry = &work->entries[i];
            entry->flags |= GA_ENTRY_FLAG_NO_BOB;

            switch (i) {
            case BOS_GA_PART_TORSO:
                AnimStart(&entry->anim, 1, ANIM_FLAG_LOOP);
                break;
            case BOS_GA_PART_HEAD:
                entry->baseX = gBtlWork->bossX;
                entry->baseY = gBtlWork->bossY + 0x100;
                break;
            }
        }

        work->step = BOS_GA_BODY_STEP_LOWER;
        break;
    case BOS_GA_PHASE_UPDATE:
        switch (work->step) {
        case BOS_GA_BODY_STEP_LOWER:
            gBtlWork->bossZ += 0x66;

            if (gBtlWork->bossZ > 0x25FF) {
                gBtlWork->bossZ = 0x2600;
                work->timer = 3;
                work->step = BOS_GA_BODY_STEP_START_MOVE;
            }

            for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
                entry = &work->entries[i];

                if (i <= 1) {
                    entry->baseZ = BosGaEntryHomeZ(work, i);
                }
            }

            break;
        case BOS_GA_BODY_STEP_START_MOVE:
            work->vx = (gBtlWork->actor->x - gBtlWork->bossX) / 60;
            work->vy = (gBtlWork->actor->y - gBtlWork->bossY) / 60;
            work->vz = -0x400;
            work->vzDelta = 0x22;
            work->step = BOS_GA_BODY_STEP_MOVE;
            BosGaUpdateFacing(work);
            break;
        case BOS_GA_BODY_STEP_MOVE:
            gBtlWork->bossX += work->vx;

            if (work->vx < 0) {
                if (gBtlWork->bossX < 0) {
                    work->vx = -work->vx;
                    gBtlWork->bossX = 0;
                    BosGaUpdateFacing(work);
                }
            } else if (work->vx > 0) {
                if (gBtlWork->bossX > 0x10000) {
                    work->vx = -work->vx;
                    gBtlWork->bossX = 0x10000;
                    BosGaUpdateFacing(work);
                }
            }

            gBtlWork->bossY += work->vy;

            if (work->vy < 0) {
                if (gBtlWork->bossY <= 0x127FF) {
                    work->vy = -work->vy;
                    gBtlWork->bossY = 0x12800;
                    BosGaUpdateFacing(work);
                }
            } else if (work->vy > 0) {
                if (gBtlWork->bossY > 0x17800) {
                    work->vy = -work->vy;
                    gBtlWork->bossY = 0x17800;
                    BosGaUpdateFacing(work);
                }
            }

            gBtlWork->bossZ += work->vz;
            work->vz += work->vzDelta;

            if (gBtlWork->bossZ > 0x2600) {
                gBtlWork->bossZ = 0x2600;
                BtlMapStartShake();
                m4aSongNumStart(SONG_BTL_IRON_RUMB);
                ApplyAttackBox(0xE5, gBtlWork->viewX, gBtlWork->viewY, 0, 0x140, 0xF0, 1);
                work->timer--;

                if (work->timer > 0) {
                    work->step = BOS_GA_BODY_STEP_START_MOVE;
                } else {
                    work->step = BOS_GA_BODY_STEP_STOP;
                }
            }

            for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
                entry = &work->entries[i];

                switch (i) {
                case BOS_GA_PART_TORSO:
                    BosGaEntryResetHome(work, BOS_GA_PART_TORSO);
                    break;
                case BOS_GA_PART_HEAD:
                    entry->baseX = gBtlWork->bossX;
                    entry->baseY = gBtlWork->bossY + 0x100;
                    entry->baseZ = BosGaEntryHomeZ(work, BOS_GA_PART_HEAD);
                    break;
                }
            }

            break;
        case BOS_GA_BODY_STEP_STOP:
            work->step = BOS_GA_BODY_STEP_RAISE;
            break;
        case BOS_GA_BODY_STEP_RAISE:
            gBtlWork->bossZ -= 0x33;

            if (gBtlWork->bossZ <= 0) {
                BosGaRequestState(work, BOS_GA_STATE_IDLE);
            }

            for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
                entry = &work->entries[i];

                if (i <= 1) {
                    entry->baseZ = BosGaEntryHomeZ(work, i);
                }
            }

            break;
        }

        break;
    case BOS_GA_PHASE_EXIT:
        gBtlWork->bossZ = 0;

        for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
            entry = &work->entries[i];

            switch (i) {
            case BOS_GA_PART_TORSO:
                AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
                AnimStart(&entry->anim, 0, ANIM_FLAG_LOOP);
                ClearBtlObjActionFlags(&entry->actor);
            default:
                entry->flags &= ~GA_ENTRY_FLAG_NO_BOB;
                break;
            case BOS_GA_PART_NEAR_FOOT:
            case BOS_GA_PART_FAR_FOOT:
                break;
            }

            BosGaEntryResetHome(work, i);
        }

        break;
    }

    if (work->statePhase == BOS_GA_PHASE_ENTER) {
        work->statePhase = BOS_GA_PHASE_UPDATE;
    }

    if (work->statePhase == BOS_GA_PHASE_EXIT) {
        work->state = work->nextState;
        work->statePhase = BOS_GA_PHASE_ENTER;
        work->flags &= ~GA_FLAG_STATE_REQUESTED;
    }

    return 1;
}

u8 BosGaUpdateGimmick(GaWork* work) {
    GaEntryWork* entry;
    u32 i;
    s32 destroyed;

    entry = NULL;

    if (work->flags & GA_FLAG_STATE_REQUESTED) {
        work->statePhase = BOS_GA_PHASE_EXIT;
    }

    switch (work->statePhase) {
    case BOS_GA_PHASE_ENTER:
        work->timer = 0x12C;

        for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
            entry = &work->entries[i];

            if (!(entry->flags & GA_ENTRY_FLAG_DESTROYED)) {
                entry->vz = -COS(GetRandom() % 0x20) * -3;
                entry->vx = SIN(GetRandom() % 0x100) * 0x233 >> 8;
                entry->vy = -COS(GetRandom() % 0x100) * 0x233 >> 8;
                entry->mode = BOS_GA_ENTRY_MODE_FALL;

                if (i == 0) {
                    AnimStart(&work->anim, 1, ANIM_FLAG_LOOP);
                    AnimStart(&entry->anim, 2, ANIM_FLAG_LOOP);
                }
            }
        }

        m4aSongNumStart(SONG_BTL_IRON_GIMICBREAK);
        BtlMapStartShake();
        break;
    case BOS_GA_PHASE_UPDATE:
        work->timer--;

        if (work->timer > 0) {
            break;
        }

        BosGaRequestState(work, BOS_GA_STATE_IDLE);
        break;
    case BOS_GA_PHASE_EXIT:
        gBtlWork->bossZ = 0;

        for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
            entry = &work->entries[i];
            destroyed = entry->flags & GA_ENTRY_FLAG_DESTROYED;

            if (destroyed == 0) {
                if (i == 0) {
                    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
                    AnimStart(&entry->anim, 0, ANIM_FLAG_LOOP);
                }

                entry->mode = destroyed;
                BosGaEntryResetHome(work, i);
            }
        }

        break;
    }

    if (work->statePhase == BOS_GA_PHASE_ENTER) {
        work->statePhase = BOS_GA_PHASE_UPDATE;
    }

    if (work->statePhase == BOS_GA_PHASE_EXIT) {
        work->state = work->nextState;
        work->statePhase = BOS_GA_PHASE_ENTER;
        work->flags &= ~GA_FLAG_STATE_REQUESTED;
    }

    return 1;
}

enum BosGaDefeatStep {
    BOS_GA_DEFEAT_STEP_COLLAPSE,
    BOS_GA_DEFEAT_STEP_START_HEAD_SLIDE,
    BOS_GA_DEFEAT_STEP_HEAD_SLIDE,
    BOS_GA_DEFEAT_STEP_START_HEAD_FALL,
    BOS_GA_DEFEAT_STEP_HEAD_FALL,
    BOS_GA_DEFEAT_STEP_START_DEATH,
    BOS_GA_DEFEAT_STEP_DEATH
};

u8 BosGaUpdateDefeat(GaWork* work) {
    // @bug param.flags is never set (uninitialized read).
    CharaObjParam param;
    u32 i;
    GaEntryWork* entry;
    u8 result = 1;

#ifdef PLATFORM_PC
    /* Only Darkside asks for the fade from black (flags = 1); the drift ending
       is Guard Armor's, so do not leave the choice to stack contents. */
    param.flags = 0;
#endif

    if (work->flags & GA_FLAG_STATE_REQUESTED) {
        work->statePhase = BOS_GA_PHASE_EXIT;
    }

    switch (work->statePhase) {
    case BOS_GA_PHASE_ENTER:
        for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
            entry = &work->entries[i];
            entry->flags |= GA_ENTRY_FLAG_NO_BOB;

            switch (i) {
            case BOS_GA_PART_TORSO:
                AnimStart(&work->anim, 1, 0);
                AnimStart(&entry->anim, 2, 0);
                entry->baseVz = 0;
                break;
            case BOS_GA_PART_HEAD:
                entry->baseVz = 0;
                break;
            }
        }

        work->timer = 3;
        work->step = BOS_GA_DEFEAT_STEP_COLLAPSE;
        break;
    case BOS_GA_PHASE_UPDATE:
        switch (work->step) {
        case BOS_GA_DEFEAT_STEP_COLLAPSE:
            for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
                entry = &work->entries[i];

                switch (i) {
                case BOS_GA_PART_TORSO:
                    entry->baseZ += entry->baseVz;
                    entry->baseVz += 128;

                    if (entry->baseZ > -0x1800) {
                        m4aSongNumStart(SONG_SND_389);
                        entry->baseZ = -0x1800;
                        entry->baseVz = -(entry->baseVz / 2);
                        work->entries[BOS_GA_PART_HEAD].baseVz = -(work->entries[BOS_GA_PART_HEAD].baseVz / 2);
                        work->timer--;

                        if (work->timer <= 0) {
                            work->step = BOS_GA_DEFEAT_STEP_START_HEAD_SLIDE;
                        }
                    }

                    break;
                case BOS_GA_PART_HEAD:
                    entry->baseZ += entry->baseVz;
                    entry->baseVz += 128;
                    break;
                }
            }

            break;
        case BOS_GA_DEFEAT_STEP_START_HEAD_SLIDE:
            for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
                entry = &work->entries[i];

                if (i == 1) {
                    work->timer = 20;
                    entry->baseVx = (!work->flipped ? -0xA00 : 0xA00) / work->timer;
                    entry->baseVy = 0x600 / work->timer;
                    work->step = BOS_GA_DEFEAT_STEP_HEAD_SLIDE;
                }
            }

            break;
        case BOS_GA_DEFEAT_STEP_HEAD_SLIDE:
            for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
                entry = &work->entries[i];

                if (i == 1) {
                    entry->baseX += entry->baseVx;
                    entry->baseY += entry->baseVy;
                    work->timer--;

                    if (work->timer <= 0) {
                        work->step = BOS_GA_DEFEAT_STEP_START_HEAD_FALL;
                    }
                }
            }

            break;
        case BOS_GA_DEFEAT_STEP_START_HEAD_FALL:
            for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
                entry = &work->entries[i];

                if (i == 1) {
                    entry->baseVz = 0;
                    work->timer = 3;
                    work->step = BOS_GA_DEFEAT_STEP_HEAD_FALL;
                }
            }

            break;
        case BOS_GA_DEFEAT_STEP_HEAD_FALL:
            for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
                entry = &work->entries[i];

                if (i == 1) {
                    entry->baseZ += entry->baseVz;
                    entry->baseVz += 128;

                    if (entry->baseZ > -0x800) {
                        m4aSongNumStart(SONG_BTL_IRON_FOOTR);
                        entry->baseZ = -0x800;
                        entry->baseVz = -(entry->baseVz / 2);
                        work->timer--;

                        if (work->timer <= 0) {
                            work->step = BOS_GA_DEFEAT_STEP_START_DEATH;
                        }
                    }
                }
            }

            break;
        case BOS_GA_DEFEAT_STEP_START_DEATH:
            for (i = 0; i < ARRAY_COUNT(work->entries); i++) {
                entry = &work->entries[i];

                switch (i) {
                case BOS_GA_PART_TORSO:
                    param.tilesAddr = OBJ_VRAM0 + (entry->tiles->index << 5);
                    param.tileCount = entry->tiles->count;
                    param.tilesAddr2 = OBJ_VRAM0 + (work->tiles->index << 5);
                    param.tileCount2 = work->tiles->count;
                    param.x = entry->baseX + (!work->flipped ? -0x700 : 0x700);
                    param.y = entry->baseY;
                    param.z = entry->baseZ + 0x1000;
                    param.prizeObj = &entry->actor;
                    break;
                case BOS_GA_PART_HEAD:
                    param.tilesAddr3 = OBJ_VRAM0 + (entry->tiles->index << 5);
                    param.tileCount3 = entry->tiles->count;
                    break;
                }
            }

            param.paletteAddr = PLTT_MIRROR(OBJ_PLTT + (BIOS_PTR(work->palette)->index << 5));
#ifdef PLATFORM_PC
            /* no palette from a full pool: nothing to fade (the BIOS's count would size a copy past palette RAM) */
            param.paletteSize = work->palette != NULL ? work->palette->count << 5 : 0;
#else
            param.paletteSize = work->palette->count << 5;
#endif
            param.tilesAddr4 = 0;
            param.tileCount4 = 0;
            param.paletteAddr2 = 0;
            param.paletteSize2 = 0;
            param.callback = BosGaReleaseBody;
            CharaObjInitDefeat(&param);
            work->step = BOS_GA_DEFEAT_STEP_DEATH;
            break;
        case BOS_GA_DEFEAT_STEP_DEATH:
            if (!CharaObjUpdateDefeat()) {
                EndBossDefeat();
                result = 0;
            }

            break;
        }

        break;
    case BOS_GA_PHASE_EXIT:
        break;
    }

    if (work->statePhase == BOS_GA_PHASE_ENTER) {
        work->statePhase = BOS_GA_PHASE_UPDATE;
    }

    if (work->statePhase == BOS_GA_PHASE_EXIT) {
        work->state = work->nextState;
        work->statePhase = BOS_GA_PHASE_ENTER;
        work->flags &= ~GA_FLAG_STATE_REQUESTED;
    }

    return result;
}

void BosGaEntryUpdate(GaWork* work, GaEntryWork* entry) {
    s32 dx;
    s32 dy;
    s32 flip;
    s32 delta;
    u16 rotation;

    if (entry->flags & GA_ENTRY_FLAG_RELEASED) {
        return;
    }

    switch (UpdateBtlObjReaction(&entry->actor)) {
    case BTL_REACTION_CARD_ACTION:
        work->cardActionSeen = TRUE;

        if (work->state == BOS_GA_STATE_GIMMICK || work->nextState == BOS_GA_STATE_GIMMICK) {
            ClearBtlObjActionFlags(&entry->actor);
        } else {
            dx = gBtlWork->actor->x - entry->baseX;
            dx = (dx * dx) >> 8;
            dy = gBtlWork->actor->y - entry->baseY;
            dy = (dy * dy) >> 8;

            if (work->entries[BOS_GA_PART_NEAR_HAND].flags & work->entries[BOS_GA_PART_FAR_HAND].flags & work->entries[BOS_GA_PART_NEAR_FOOT].flags & work->entries[BOS_GA_PART_FAR_FOOT].flags & GA_ENTRY_FLAG_DESTROYED) {
                switch (GetRandom() % 3) {
                case 0:
                    BosGaRequestState(work, BOS_GA_STATE_BODY_CHASE);
                    break;
                case 1:
                    BosGaRequestState(work, BOS_GA_STATE_BODY_DASH);
                    break;
                case 2:
                    BosGaRequestState(work, BOS_GA_STATE_BODY_JUMP);
                    break;
                }
            } else if (dx + dy <= 0xE0FFF) {
                if (GetRandom() % 100 < 70) {
                    if ((work->entries[BOS_GA_PART_NEAR_HAND].flags & work->entries[BOS_GA_PART_FAR_HAND].flags & GA_ENTRY_FLAG_DESTROYED) == 0) {
                        flip = FALSE;

                        if (gBtlWork->bossX <= gBtlWork->actor->x) {
                            flip = TRUE;
                        }

                        if (work->flipped == flip) {
                            BosGaRequestState(work, BOS_GA_STATE_THRUST);
                        } else {
                            BosGaRequestState(work, BOS_GA_STATE_ORBIT);
                        }
                    } else {
                        if (!work->attackToggle) {
                            BosGaRequestState(work, BOS_GA_STATE_JUMP);
                        } else {
                            BosGaRequestState(work, BOS_GA_STATE_STOMP);
                        }
                    }
                } else {
                    if ((work->entries[BOS_GA_PART_NEAR_FOOT].flags & work->entries[BOS_GA_PART_FAR_FOOT].flags & GA_ENTRY_FLAG_DESTROYED) == 0) {
                        if (!work->attackToggle) {
                            BosGaRequestState(work, BOS_GA_STATE_JUMP);
                        } else {
                            BosGaRequestState(work, BOS_GA_STATE_STOMP);
                        }
                    } else {
                        flip = FALSE;

                        if (gBtlWork->bossX <= gBtlWork->actor->x) {
                            flip = TRUE;
                        }

                        if (work->flipped == flip) {
                            BosGaRequestState(work, BOS_GA_STATE_THRUST);
                        } else {
                            BosGaRequestState(work, BOS_GA_STATE_ORBIT);
                        }
                    }
                }
            } else if (dx + dy <= 0x270FFF) {
                if (GetRandom() % 100 < 50) {
                    if ((work->entries[BOS_GA_PART_NEAR_HAND].flags & work->entries[BOS_GA_PART_FAR_HAND].flags & GA_ENTRY_FLAG_DESTROYED) == 0) {
                        flip = FALSE;

                        if (gBtlWork->bossX <= gBtlWork->actor->x) {
                            flip = TRUE;
                        }

                        if (work->flipped == flip) {
                            BosGaRequestState(work, BOS_GA_STATE_THRUST);
                        } else {
                            BosGaRequestState(work, BOS_GA_STATE_ORBIT);
                        }
                    } else {
                        if (!work->attackToggle) {
                            BosGaRequestState(work, BOS_GA_STATE_JUMP);
                        } else {
                            BosGaRequestState(work, BOS_GA_STATE_STOMP);
                        }
                    }
                } else {
                    if ((work->entries[BOS_GA_PART_NEAR_FOOT].flags & work->entries[BOS_GA_PART_FAR_FOOT].flags & GA_ENTRY_FLAG_DESTROYED) == 0) {
                        if (!work->attackToggle) {
                            BosGaRequestState(work, BOS_GA_STATE_JUMP);
                        } else {
                            BosGaRequestState(work, BOS_GA_STATE_STOMP);
                        }
                    } else {
                        flip = FALSE;

                        if (gBtlWork->bossX <= gBtlWork->actor->x) {
                            flip = TRUE;
                        }

                        if (work->flipped == flip) {
                            BosGaRequestState(work, BOS_GA_STATE_THRUST);
                        } else {
                            BosGaRequestState(work, BOS_GA_STATE_ORBIT);
                        }
                    }
                }
            } else {
                if (GetRandom() % 100 < 30) {
                    if ((work->entries[BOS_GA_PART_NEAR_HAND].flags & work->entries[BOS_GA_PART_FAR_HAND].flags & GA_ENTRY_FLAG_DESTROYED) == 0) {
                        flip = FALSE;

                        if (gBtlWork->bossX <= gBtlWork->actor->x) {
                            flip = TRUE;
                        }

                        if (work->flipped == flip) {
                            BosGaRequestState(work, BOS_GA_STATE_THRUST);
                        } else {
                            BosGaRequestState(work, BOS_GA_STATE_ORBIT);
                        }
                    } else {
                        if (!work->attackToggle) {
                            BosGaRequestState(work, BOS_GA_STATE_JUMP);
                        } else {
                            BosGaRequestState(work, BOS_GA_STATE_STOMP);
                        }
                    }
                } else {
                    if ((work->entries[BOS_GA_PART_NEAR_FOOT].flags & work->entries[BOS_GA_PART_FAR_FOOT].flags & GA_ENTRY_FLAG_DESTROYED) == 0) {
                        if (!work->attackToggle) {
                            BosGaRequestState(work, BOS_GA_STATE_JUMP);
                        } else {
                            BosGaRequestState(work, BOS_GA_STATE_STOMP);
                        }
                    } else {
                        flip = FALSE;

                        if (gBtlWork->bossX <= gBtlWork->actor->x) {
                            flip = TRUE;
                        }

                        if (work->flipped == flip) {
                            BosGaRequestState(work, BOS_GA_STATE_THRUST);
                        } else {
                            BosGaRequestState(work, BOS_GA_STATE_ORBIT);
                        }
                    }
                }
            }
        }

        if (GetRandom() % 3 != 0) {
            if (!work->attackToggle) {
                work->attackToggle = TRUE;
            } else {
                work->attackToggle = FALSE;
            }
        }

        break;
    case BTL_REACTION_HURT:
    case BTL_REACTION_STUNNED:
    case BTL_REACTION_GRAVITY:
        entry->flags |= GA_ENTRY_FLAG_HURT;
        entry->flashTimer = 0;

        if (entry->index == BOS_GA_PART_TORSO) {
            work->entries[BOS_GA_PART_HEAD].flags |= GA_ENTRY_FLAG_HURT;
            work->entries[BOS_GA_PART_HEAD].flashTimer = 0;
        }

        break;
    case BTL_REACTION_DEFEATED:
    case BTL_REACTION_GRAVITY_DEFEATED:
        SetBtlObjUnhittable(&entry->actor, TRUE);
        entry->flags |= GA_ENTRY_FLAG_DESTROYED;

        if (entry->index == BOS_GA_PART_TORSO) {
            BeginBossDefeat(&entry->actor);
            entry->mode = BOS_GA_ENTRY_MODE_FOLLOW;
            entry->counter = 0;
            work->entries[BOS_GA_PART_HEAD].mode = BOS_GA_ENTRY_MODE_FOLLOW;
            work->entries[BOS_GA_PART_HEAD].counter = 0;
            BosGaRequestState(work, BOS_GA_STATE_DEFEATED);
        } else {
            entry->mode = BOS_GA_ENTRY_MODE_DESTROYED;
            entry->counter = 0;

            if (work->state != BOS_GA_STATE_GIMMICK && work->nextState != BOS_GA_STATE_GIMMICK) {
                if (work->cardActionSeen) {
                    ClearBtlObjActionFlags(&work->entries[BOS_GA_PART_TORSO].actor);
                }

                BosGaRequestState(work, BOS_GA_STATE_IDLE);
            }
        }

        break;
    case BTL_REACTION_CARD_BROKEN:
        if (work->state != BOS_GA_STATE_GIMMICK && work->nextState != BOS_GA_STATE_GIMMICK) {
            if (GetRandom() % 100 < 30) {
                DropGimmickCard(0, entry->baseX, entry->baseY, entry->baseZ);
            }

            BosGaRequestState(work, BOS_GA_STATE_IDLE);
        }

        ClearBtlObjActionFlags(&entry->actor);
        break;
    }

    switch ((u32)entry->mode) {
    case BOS_GA_ENTRY_MODE_FOLLOW:
        delta = (entry->baseX - entry->actor.x) >> 1;

        if (delta > 0x600) {
            delta = 0x600;
        } else if (delta < -0x600) {
            delta = -0x600;
        }

        entry->actor.x += delta;
        delta = (entry->baseY - entry->actor.y) >> 1;

        if (delta > 0x600) {
            delta = 0x600;
        } else if (delta < -0x600) {
            delta = -0x600;
        }

        entry->actor.y += delta;
        delta = ((entry->baseZ + entry->bobZ) - entry->actor.z) >> 1;

        if (delta > 0x600) {
            delta = 0x600;
        } else if (delta < -0x600) {
            delta = -0x600;
        }

        entry->actor.z += delta;
        rotation = entry->rotation;
        ApproachAngle(&rotation, 0, 3);
        entry->rotation = rotation;

        if (entry->flags & GA_ENTRY_FLAG_NO_BOB) {
            break;
        }

        entry->bobZ = gSineTable[entry->bobAngle] << 2;
        entry->bobAngle += 4;
        break;
    case BOS_GA_ENTRY_MODE_FALL:
        BosGaEntryUpdateFall(entry);
        break;
    case BOS_GA_ENTRY_MODE_DESTROYED:
        if (entry->counter == 0) {
            entry->flags |= GA_ENTRY_FLAG_HURT;
            entry->flashTimer = 0;

            if (!BgFxIsActive()) {
                BgFxStartEnemyDeath(entry->actor.x, entry->actor.y + entry->actor.z, 0, Q_8_8(1));
                entry->counter++;
            }
        } else if (entry->counter > 0) {
            if (work->entries[BOS_GA_PART_NEAR_HAND].flags & work->entries[BOS_GA_PART_FAR_HAND].flags & work->entries[BOS_GA_PART_NEAR_FOOT].flags & work->entries[BOS_GA_PART_FAR_FOOT].flags & GA_ENTRY_FLAG_DESTROYED) {
                SetBtlObjUnhittable(&work->entries[BOS_GA_PART_TORSO].actor, FALSE);
            }

            ClearBtlObjActionFlags(&entry->actor);
            BosGaEntryRelease(entry);
            return;
        }

        BosGaEntryUpdateFall(entry);
        break;
    }

    if (entry->flags & GA_ENTRY_FLAG_HURT) {
        entry->flashTimer++;

        if (entry->flashTimer > 30) {
            ClearBtlObjActionFlags(&entry->actor);
            entry->flags &= ~GA_ENTRY_FLAG_HURT;
            entry->flashTimer = 0;
        }
    }

    entry->gfx = AnimUpdate(&entry->anim);

    if (entry->index == BOS_GA_PART_TORSO) {
        work->gfx = AnimUpdate(&work->anim);
    }

    if (entry->actor.collider.colliding) {
        entry->actor.x += entry->actor.collider.pushX;
        entry->actor.y += entry->actor.collider.pushY;
    }

    ColliderSetPosition(&entry->actor.collider, entry->actor.x, entry->actor.y, entry->actor.z + entry->bobZ);
    TaskPoolUpdate(&entry->tasks);
}

void task_bos_ga_0(GaWork* work, s32 arg) {
    u32 i;
    GaEntryWork* torso;

    sGaWork = work;

    if (arg == 0) {
        work->state = BOS_GA_STATE_IDLE;
    } else {
        work->state = BOS_GA_STATE_ASSEMBLE;
    }

    work->nextState = work->state;
    work->flags = 0;
    work->statePhase = BOS_GA_PHASE_ENTER;
    work->timer = 0;
    work->stepsLeft = 0;
    work->hurtTimer = 0;
    work->flipped = FALSE;
    work->angle = 0;
    work->attackToggle = FALSE;
    work->cardTimer = 60;
    TaskCreate(&gBtlWork->taskPools[1], &gTaskDescBosMap, &sBosMapConfig);
    gBtlWork->bossX = 0xE200;
    gBtlWork->bossY = 0x15E00;
    gBtlWork->bossZ = 0;
    SetBattleActorPosition(0x8200, 0x15E00, 0);
    torso = work->entries;

    for (i = 0; i <= 5; i++) {
        BosGaEntryInit(work, i, arg);
    }

    SetBtlObjUnhittable(&torso->actor, TRUE);
    SetBtlObjUnhittable(&work->entries[BOS_GA_PART_HEAD].actor, TRUE);
    work->palette = LoadObjPalette(gBoss01objPalette, sizeof(gBoss01objPalette));
    work->palette2 = LoadObjPalette(gHitFlashPalette, sizeof(gHitFlashPalette));
    SetBtlPaletteFadeExcluded(BIOS_PTR(work->palette)->index + 16, TRUE);
    SetBtlPaletteFadeExcluded(BIOS_PTR(work->palette2)->index + 16, TRUE);
    RequestBossCardValue(GetRandom() % 4 + 1);
}

u8 task_bos_ga_1(GaWork* work) {
    u8 result;
    GaEntryWork* entry;
    AnimState* anim;
    u32 i;

    result = 1;
    work->cardTimer--;

    if (work->cardTimer <= 0) {
        RequestBossCardValue(GetRandom() % 7 + 1);
        work->cardTimer = 60;
    }

    work->cardActionSeen = FALSE;
    i = 0;
    entry = work->entries;

    do {
        BosGaEntryUpdate(work, entry);
        entry++;
        i++;
    } while (i <= 5);

    switch (work->state) {
    case BOS_GA_STATE_ASSEMBLE:
        result = BosGaUpdateAssemble(work);
        break;
    case BOS_GA_STATE_IDLE:
        result = BosGaUpdateIdle(work);
        break;
    case BOS_GA_STATE_WALK:
        result = BosGaUpdateWalk(work);
        break;
    case BOS_GA_STATE_STOMP:
        result = BosGaUpdateStomp(work);
        break;
    case BOS_GA_STATE_THRUST:
        result = BosGaUpdateThrust(work);
        break;
    case BOS_GA_STATE_ORBIT:
        result = BosGaUpdateOrbit(work);
        break;
    case BOS_GA_STATE_JUMP:
        result = BosGaUpdateJump(work);
        break;
    case BOS_GA_STATE_BODY_CHASE:
        result = BosGaUpdateBodyChase(work);
        break;
    case BOS_GA_STATE_BODY_DASH:
        result = BosGaUpdateBodyDash(work);
        break;
    case BOS_GA_STATE_BODY_JUMP:
        result = BosGaUpdateBodyJump(work);
        break;
    case BOS_GA_STATE_GIMMICK:
        result = BosGaUpdateGimmick(work);
        break;
    case BOS_GA_STATE_DEFEATED:
        result = BosGaUpdateDefeat(work);
        break;
    }

    if (ConsumeGimmickFlag(0)) {
        BosGaRequestState(work, BOS_GA_STATE_GIMMICK);
    }

    anim = &work->entries[BOS_GA_PART_HEAD].anim;

    if (AnimIsFinished(anim) && GetRandom() % 100 == 0 && work->state != BOS_GA_STATE_DEFEATED) {
        AnimStart(anim, 1, 0);
    }

    return result;
}

void task_bos_ga_2(GaWork* work) {
    GaEntryWork* entry;
    u32 i;

    i = 0;
    entry = work->entries;

    do {
        BosGaEntryDraw(work, entry);
        entry++;
        i++;
    } while (i <= 5);
}

void task_bos_ga_3(GaWork* work) {
    GaEntryWork* entry;
    u32 i;

    i = 0;
    entry = work->entries;

    do {
        BosGaEntryRelease(entry);
        entry++;
        i++;
    } while (i <= 5);

    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ReleaseObjPalette(work->palette2);
}
