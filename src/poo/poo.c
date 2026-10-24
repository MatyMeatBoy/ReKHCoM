/**
 * poo.c
 * 100 Acre Wood Minigame Objects
 */

#include "macros.h"
#include "poo.h"
#include "sprites_btl.h"
#include "sprites_evt.h"
#include "sprites_fld.h"
#include "sprites_map.h"
#include "sprites_pooh.h"
#include "sprites_sora.h"
#include "gba/keys.h"
#include "btl_api.h"
#include "malloc.h"
#include "mode_pooh_api.h"
#include "songs.h"
#include "player_progression.h"
#include <stdlib.h>
#include "poo_background_data.h"
#include <string.h>
#include "anim.h"
#include "battle_actor_types.h"
#include "bos4_api.h"
#include "btl_collision.h"
#include "display.h"
#include "engine_math.h"
#include "field_state.h"
#include "game_state.h"
#include "key.h"
#include "listpool.h"
#include "m4a_song.h"
#include "obj.h"
#include "obj_api.h"
#include "player_progression_types.h"
#include "poo_api.h"
#include "poo_data.h"
#include "pooh_actor_types.h"
#include "prize_types.h"
#include "registration_data.h"
#include "task_descriptors.h"
#include "taskpool.h"
#include "types.h"
#include <stddef.h>
#include "default_bg_map.h"
#include "event_backgrounds.h"
#include "sprite_palettes.h"
#include "card_message_data.h"
#include "event_ids.h"
#include "jiminy_records_index_data.h"
#include "gba/defines.h"
#include "fld_types.h"
#include "map_types.h"

u8 gPooAttackActive EWRAM_COMMON(4);
s32 gPoohRequestX EWRAM_COMMON(4);
u16 gPoohGaugeTimer EWRAM_COMMON(4);
u16 gPoohGauge EWRAM_COMMON(4);
s32 gPoohRequestY EWRAM_COMMON(4);
struct PooPos* gPoohPos EWRAM_COMMON(4);
u32 gPoohRequest EWRAM_COMMON(4);
u32 gUnk_0203C3F4 EWRAM_COMMON(4);
u16 gPooScrollY EWRAM_COMMON(4);
s32 gPooCameraX EWRAM_COMMON(4);
s32 gPooCameraFocusY EWRAM_COMMON(4);
s32 gPooCameraFocusX EWRAM_COMMON(4);
s32 gPooCameraY EWRAM_COMMON(4);
u16 gPooScrollX EWRAM_COMMON(4);
struct PooNode* gPooSoraNode EWRAM_COMMON(4);
void* gPooSoraCollider EWRAM_COMMON(4);
PooActor gPooActor EWRAM_COMMON(16);
void* gStockMesDispWork EWRAM_COMMON(4);
PooState gPooState EWRAM_COMMON(16);
void* gSharedModeWork EWRAM_COMMON(4);

#define sPoohInteractions ((PoohInteractionRegistry*)gSharedModeWork)

extern AnimDef gTrap01AnimDefs[5];

static const PooSpot sPooh04FrameOffsets[55] = {
    { 0, 0, 0 },
    { 0, -512, 0 },
    { 0, -512, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, -1024, 0 },
    { 0, -256, 0 },
    { 0, -256, 0 },
    { 0, -256, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 512, 0 },
    { 0, 512, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 1024, 0 },
    { 0, 256, 0 },
    { 0, -256, 0 },
    { 0, 256, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { -768, 0, 0 },
    { -1280, 0, 0 },
    { -1024, 1280, 0 },
    { -512, -768, 0 },
    { -768, 1280, 0 },
    { -256, -1024, 0 },
    { -256, 0, 0 },
    { -256, 256, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { -512, 0, 0 },
    { -1280, 0, 0 },
    { -1536, 0, 256 },
    { -768, 0, 512 },
    { -1280, 0, -768 },
    { -512, 0, 0 },
    { -256, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { -768, 0, 0 },
    { -1280, 0, 0 },
    { -1024, -1280, 0 },
    { -512, 768, 0 },
    { -768, -1280, 0 },
    { -256, 1024, 0 },
    { -256, 0, 0 },
    { -256, -256, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
};

static const PooSpot sPooh04aFrameOffsets[80] = {
    { 0, 0, 0 },
    { 0, 512, 0 },
    { 0, 256, 0 },
    { 0, 256, 0 },
    { 0, 512, 0 },
    { -256, 0, 0 },
    { 0, 0, 0 },
    { 0, -512, 0 },
    { 0, 256, 0 },
    { 0, -256, 0 },
    { 0, -512, 0 },
    { 0, 512, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, -512, 0 },
    { 0, -256, 0 },
    { 0, -256, 0 },
    { 0, -512, 0 },
    { -256, 0, 0 },
    { 0, 0, 0 },
    { 0, 512, 0 },
    { 0, -256, 0 },
    { 0, 256, 0 },
    { 0, 512, 0 },
    { 0, -512, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { -256, -256, 0 },
    { 0, 0, 0 },
    { 0, 512, 0 },
    { 256, -256, 0 },
    { 0, 0, 0 },
    { -256, 0, 0 },
    { 768, 0, 0 },
    { 0, 0, 0 },
    { 256, 0, 0 },
    { 512, 0, 0 },
    { 512, 0, 0 },
    { 256, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 256, 0, 0 },
    { 0, 0, 0 },
    { 256, 0, 0 },
    { -256, 0, 0 },
    { -256, 0, 0 },
    { -256, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 768, 0, 0 },
    { 256, 0, 0 },
    { 0, 0, 0 },
    { 256, 0, 0 },
    { 256, 0, 0 },
    { 768, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
    { -256, 256, 0 },
    { 0, 0, 0 },
    { 0, 512, 0 },
    { 256, 256, 0 },
    { 0, 0, 0 },
    { -256, 0, 0 },
    { 768, 0, 0 },
    { 0, 0, 0 },
    { 256, 0, 0 },
    { 512, 0, 0 },
    { 512, 0, 0 },
    { 256, 0, 0 },
    { 0, 0, 0 },
    { 0, 0, 0 },
};

static const PooPoint sPoohStumpCircle[4] = {
    { 593408, 315648 },
    { 584192, 320256 },
    { 593408, 324864 },
    { 602624, 320256 },
};

static PoohWork* sPooWork;
static u32 sPoohAction;
static PooSpawnArgs sPooSpawnArgs;
static PooPos sPooSpawnPos;
static s32 sPooMoveAdjustX;
static s32 sPooMoveAdjustY;
static u8 sPooCameraFrozen;
static u8 sPooCameraFollowPooh;
static s32 sPooCameraMaxY;
static PooSoraWork* sPooSoraWork;
static u16 sPooSoraPriority;
static s32 sPooBestNodeScore;
static s32 sPooNodeScore;
static ListPool sPooNodes;
static PooPos sPooOwlBalloonPos;
static u16 sPooEeyoreTailTimer;
static u32 sPooHoneycombState;
static PooWagonWork* sPooWagon;
static u16 sPooWagonPriority2;
static u16 sPooWagonPriority;
static u16 sPooWagonWheelPriority;
static u32 sPooBeeCount;
static PooBeeAfterEventWork* sPooBeeAfterEventWork;
static u16 sPooCabbageCount;
static u16 sPooCabbageLandedCount;

s32 GetPooManhattanDistance(PooPos* from, PooPos* to) {
    s32 dx;
    s32 dy;

    dx = from->x - to->x;

    if (dx < 0) {
        dx = to->x - from->x;
    }

    dy = from->y - to->y;

    if (dy < 0) {
        dy = to->y - from->y;
    }

    return dx + dy;
}

void SetPoohPalette(PoohWork* work, u32 action) {
    u16* pal;

    switch (action) {
    case POOH_ACTION_TRAP_FALL:
        pal = gTrap0001Palette;
        break;
    case POOH_ACTION_TRAPPED:
    case POOH_ACTION_TRAPPED_WITH_ROO:
        pal = gTrap0002Palette;
        break;
    case POOH_ACTION_BALLOON:
    case POOH_ACTION_OWL_BALLOON:
        pal = gTrap0003Palette;
        break;
    default:
        pal = gPoohPalette;
        break;
    }

    if (work->palette->src != pal) {
        ReleaseObjPalette(work->palette);
        work->palette = LoadObjPalette(pal, 32);
    }
}

enum PoohRequest {
    POOH_REQUEST_NONE,
    POOH_REQUEST_TRAP,
    POOH_REQUEST_TRAP_BALLOON,
    POOH_REQUEST_HONEY,
    POOH_REQUEST_PIGLET,
    POOH_REQUEST_BLOCKED,
    POOH_REQUEST_IDLE,
    POOH_REQUEST_OWL_BALLOON,
    POOH_REQUEST_STUMP,
    POOH_REQUEST_HONEYCOMB,
    POOH_REQUEST_HONEYCOMB_DONE,
    POOH_REQUEST_WAGON,
    POOH_REQUEST_CANCEL
};

void SetPoohAction(PoohWork* work, u32 action) {
    sPoohAction = action;

    if (action == POOH_ACTION_IDLE) {
        gPoohRequest = POOH_REQUEST_NONE;
    }

    if (action >= POOH_ACTION_BALLOON && action <= POOH_ACTION_OWL_BALLOON) {
        work->balloonTimer = 0;
        TASK_CREATE_IF_INACTIVE(work->task, &work->tasks, &gTaskDescPooBalloon, &work->pos);
    }

    if (action == POOH_ACTION_THINK || action == POOH_ACTION_SIT || action == POOH_ACTION_WALK_AWAY ||
        action == POOH_ACTION_LOOK_AT_HONEYCOMB_DONE) {
        work->actionTimer = 0;
    }

    if (action == POOH_ACTION_JUMP_SCARED) {
        work->vz = -0x130;
    }

    if (action >= POOH_ACTION_TRAPPED && action <= POOH_ACTION_TRAPPED_WITH_ROO) {
        m4aSongNumStart(SONG_SND_329);
    } else if (action == POOH_ACTION_TRAP_FALL) {
        m4aSongNumStart(SONG_SYS_PO_FALL);
    } else if (action == POOH_ACTION_OWL_BALLOON || action == POOH_ACTION_OWL_DESCENT ||
               (action >= POOH_ACTION_SIT_FOR_HONEY && action <= POOH_ACTION_EAT_HONEY_3)) {
        work->angle = FLD_ANGLE_DOWN_LEFT;
        work->lookAngle = FLD_ANGLE_DOWN_LEFT;
        work->lookColumn = work->angle;
    }

    if (action > POOH_ACTION_EAT_HONEY_3) {
        work->angle = FLD_ANGLE_DOWN_RIGHT;
        work->lookAngle = FLD_ANGLE_DOWN_RIGHT;
        work->lookColumn = work->angle;
    }

    SetPoohPalette(work, action);
}

void task_poo_pooh_0(PoohWork* work) {
    PooShadowArgs args;

    sPooWork = work;
    work->unk_CC = 0;
    GetPooStateGauge(&gPoohGauge, &gPoohGaugeTimer);
    work->targetNode = NULL;
    work->lookTimer = 0;
    gPoohRequest = POOH_REQUEST_NONE;
    work->callTimer = 0;
    work->callCount = 0;
    work->angle = FLD_ANGLE_UP_RIGHT;
    work->lookAngle = FLD_ANGLE_UP_RIGHT;
    work->lookColumn = work->angle;
    SetPoohDir5Right(work);
    work->speed = 0;
    work->flipped = 0;
    work->animAction = 0xFFFF;
    GetPooStatePooh(&work->pos, &sPoohAction);
    work->pos.ground = 0;
    work->targetX = work->pos.x;
    work->targetY = work->pos.y;
    gPoohPos = &work->pos;
    work->tiles = AllocObjTiles(gPoohHitBox.tileCount * 32, NULL);
    work->palette = LoadObjPalette(gPoohHitBox.palette, 32);
    AnimInit(&work->anim, NULL, NULL);
    SetPoohAction(work, sPoohAction);
    work->hideShadow = FALSE;
    SetPoohAnimation(work, sPoohAction);
    work->gfx = AnimGetGfx(&work->anim);
    ColliderInit(&work->collider, COLLIDER_TYPE_POOH, gPoohHitBox.radius, gPoohHitBox.height);
    ColliderSetPosition(&work->collider, work->pos.x, work->pos.y, work->pos.z);
    TaskPoolInit(&work->tasks, 10);
    args.pos = &work->pos;
    args.shadowInfo = &work->shadowInfo;
    TaskCreate(&work->tasks, &gTaskDescPooShadowdodai, &args);
    work->task = NULL;
    work->zzzTask = NULL;
    work->groundZ = GetPooGroundZ(&work->collider, &work->pos, &work->onCollider);
}

u8 HandlePoohRequest(PoohWork* work) {
    if (gPoohRequest == POOH_REQUEST_TRAP) {
        SetPoohAction(work, POOH_ACTION_TRAP_FALL);
        work->pos.x = gPoohRequestX;
        work->pos.y = gPoohRequestY;
    } else if (gPoohRequest == POOH_REQUEST_TRAP_BALLOON) {
        work->pos.x = gPoohRequestX;
        work->pos.y = gPoohRequestY;
        SetPoohAction(work, POOH_ACTION_BALLOON);
    } else if (gPoohRequest == POOH_REQUEST_OWL_BALLOON) {
        work->pos.x = gPoohRequestX;
        work->pos.y = gPoohRequestY;
        SetPoohAction(work, POOH_ACTION_OWL_BALLOON);
    } else if (gPoohRequest == POOH_REQUEST_STUMP) {
        work->angle = GetAngle(work->pos.x, work->pos.y, gPoohRequestX, gPoohRequestY);
        work->lookAngle = work->angle;
        work->lookColumn = work->angle;
        SetPoohAction(work, POOH_ACTION_STUMP_CLIMB);
    } else if (gPoohRequest == POOH_REQUEST_WAGON) {
        work->angle = GetAngle(work->pos.x, work->pos.y, gPoohRequestX, gPoohRequestY);
        work->lookAngle = work->angle;
        work->lookColumn = work->angle;
        work->leavingWagon = FALSE;
        SetPoohAction(work, POOH_ACTION_WAGON_CLIMB);
    } else if (gPoohRequest == POOH_REQUEST_HONEY) {
        SetPoohAction(work, POOH_ACTION_SIT_FOR_HONEY);
    } else if (gPoohRequest == POOH_REQUEST_PIGLET || gPoohRequest == POOH_REQUEST_IDLE) {
        SetPoohAction(work, POOH_ACTION_IDLE);
    } else if (gPoohRequest == POOH_REQUEST_BLOCKED) {
        SetPoohAction(work, POOH_ACTION_BLOCKED);
    } else if (gPoohRequest == POOH_REQUEST_HONEYCOMB) {
        SetPoohAction(work, POOH_ACTION_LOOK_AT_HONEYCOMB);
    } else if (gPoohRequest == POOH_REQUEST_HONEYCOMB_DONE) {
        SetPoohAction(work, POOH_ACTION_LOOK_AT_HONEYCOMB_DONE);
    } else if (gPoohRequest == POOH_REQUEST_CANCEL) {
        gPoohRequest = POOH_REQUEST_NONE;
        return FALSE;
    } else {
        SetPoohAction(work, POOH_ACTION_BLOCKED);
    }

    gPoohRequest = POOH_REQUEST_NONE;
    return TRUE;
}

u8 CheckPoohInterrupts(PoohWork* work, PooNode* node) {
    if (ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_PLAYER)) {
        if (!IsPooSoraOverWagon()) {
            SetPoohAction(work, POOH_ACTION_BLOCKED);
            return TRUE;
        }
    }

    if (gPoohRequest != POOH_REQUEST_NONE) {
        return HandlePoohRequest(work);
    }

    return FALSE;
}

void ChoosePoohTarget(PoohWork* work, PooNode* node) {
    if (node != NULL) {
        if (IsPooSoraCallStarting()) {
            if (work->callTimer != 0 || work->targetNode == gPooSoraNode) {
                work->callCount++;

                if (work->callCount > 10) {
                    work->callTimer = 0;
                    work->callCount = 0;
                    SetPoohAction(work, POOH_ACTION_THINK_START);
                }

                return;
            }

            work->callTimer = 90;
        }

        if (work->callTimer != 0) {
            work->callTimer--;
            node = gPooSoraNode;
        }

        if (work->targetNode != node && work->lookTimer <= 59) {
            work->lookAngle = work->angle;
            work->lookColumn = work->angle;
            work->lookTarget = GetAngle(work->pos.x, work->pos.y, ((PooPos*)node->pos)->x, ((PooPos*)node->pos)->y);
            work->targetX = work->pos.x;
            work->targetY = work->pos.y;
            SetPoohAction(work, POOH_ACTION_LOOK);
        } else {
            work->targetNode = node;
            work->lookTimer = 0;
            work->targetX = ((PooPos*)work->targetNode->pos)->x;
            work->targetY = ((PooPos*)work->targetNode->pos)->y;
            SetPoohAction(work, POOH_ACTION_WALK);
        }
    } else {
        work->targetX = work->pos.x;
        work->targetY = work->pos.y;
        SetPoohAction(work, POOH_ACTION_SIT_DOWN);
    }
}

void ApplyPoohFrameOffset(PoohWork* work, const PooSpot* offsets, u16 frameCount) {
    u16 frame;
    s32 offset;
    s32 i;

    frame = AnimGetFrame(&work->anim) + 1;

    if (!work->collider.colliding) {
        i = work->dirIndex * frameCount + frame;
        offset = offsets[i].x;

        if (work->flipped != 0) {
            offset = -offset;
        }

        work->pos.x += offset;
        offset = offsets[i].y;
        work->pos.y += offset;
    }

    work->pos.z += offsets[work->dirIndex * frameCount + frame].z;
}

void ApplyPooh04FrameOffset(PoohWork* work) {
    ApplyPoohFrameOffset(work, sPooh04FrameOffsets, 11);
}

void ApplyPooh04aFrameOffset(PoohWork* work) {
    ApplyPoohFrameOffset(work, sPooh04aFrameOffsets, 0x10);
}

s32 GetPoohStumpIndex(PoohWork* work) {
    PooPoint circle[4];
    u32 i;

    memcpy(circle, sPoohStumpCircle, sizeof(circle));

    for (i = 0; i < ARRAY_COUNT(circle); i++) {
        if (work->collider.platformX == circle[i].x && work->collider.platformY == circle[i].y) {
            break;
        }
    }

    return i;
}

void ResetPoohStumpCount(PoohWork* work) {
    work->stumpCount = 0;
    work->stumpIndex = GetPoohStumpIndex(work);
}

u32 NextPoohStumpIndex(u32 index) {
    index++;

    if (index > 3) {
        index = 0;
    }

    return index;
}

void UpdatePoohStumpCircle(PoohWork* work) {
    s32 stumpIndex;

    stumpIndex = GetPoohStumpIndex(work);

    if (stumpIndex == NextPoohStumpIndex(work->stumpIndex)) {
        work->stumpIndex = stumpIndex;
        work->stumpCount++;

        if (work->stumpCount > 3) {
            if (!IsPooEventDone(POO_EVENT_TIGGER)) {
                ExitPoohMode(EVENT_141_100ACREWOOD_LV5);
                SetPooEventDone(POO_EVENT_TIGGER);
                SetJiminyFlag(JIMINY_RECORD_CHARACTER_TIGGER);
            }
        }
    } else {
        ResetPoohStumpCount(work);
    }
}

s32 GetPooGroundZ(Collider* collider, PooPos* pos, u8* onCollider) {
    s32 groundZ;

    if ((collider->standFlags & COLLIDER_STAND_OVER_PLATFORM) != 0) {
        if (pos->ground < collider->platformZ) {
            groundZ = pos->ground;
        } else {
            groundZ = collider->platformZ;
        }

        *onCollider = 1;
    } else {
        *onCollider = 0;
        groundZ = pos->ground;
    }

    return groundZ;
}

void MovePooh(PoohWork* work, s32 maxSpeed, u8 faceTarget) {
    work->speed += 6;

    if (work->speed > maxSpeed) {
        work->speed = maxSpeed;
    }

    if (faceTarget) {
        work->lookAngle = work->angle = GetAngle(work->pos.x, work->pos.y, work->targetX, work->targetY);
        work->lookColumn = work->angle;
    }

    work->pos.x += gSineTable[work->angle] * work->speed >> 8;
    work->pos.y += -gSineTable[work->angle + 0x40] * work->speed >> 8;
}

u8 IsPoohNearScreenEdge() {
    s16 x;
    s16 y;

    x = (gPoohPos->x >> 8) - gPooScrollX;
    y = (gPoohPos->y >> 8) + (gPoohPos->z >> 8) - gPooScrollY;

    if (x < gPoohHitBox.radius * 2 || DISPLAY_WIDTH - gPoohHitBox.radius * 2 < x || y < gPoohHitBox.height * 2 || y > 152) {
        return TRUE;
    }

    return FALSE;
}

void UpdatePoohAction(PoohWork* work, PooNode* node) {
    u16 honeycombLookAngle;
    u16 targetLookAngle;
    u16 doneLookAngle;
    u16 tripOdds;
    u16 weight;
    s32 maxSpeed;

    switch (sPoohAction) {
    case POOH_ACTION_WALK:
        if (AnimGetGfxIndex(&work->anim) == 8 && gPoohPos->y > 0x1BD00 && (GetKeysPressed() & A_BUTTON) != 0) {
            tripOdds = 128;

            if (IsPooCabbageGameActive()) {
                tripOdds = 2;
            }

            if (GetRandom() % tripOdds == 0) {
                SetPoohAction(work, POOH_ACTION_TRIP);
            }

            break;
        }

        maxSpeed = 0x4C;

        if (IsPooSoraCalling() && work->targetNode == node && node == gPooSoraNode) {
            maxSpeed = 152;
            work->speed = 152;
        } else if (work->speed > 82) {
            maxSpeed = work->speed - 6;
        }

        MovePooh(work, maxSpeed, TRUE);

        if (CheckPoohInterrupts(work, node)) {
            work->speed = 0;
        } else {
            ChoosePoohTarget(work, node);
        }

        break;
    case POOH_ACTION_BLOCKED:
        if (work->collider.colliding) {
            if (ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_PLAYER)) {
                break;
            }

            if (!IsPooSoraCallStarting()) {
                break;
            }

            work->pos.x += work->collider.pushX;
            work->pos.y += work->collider.pushY;
            work->targetNode = NULL;
            ChoosePoohTarget(work, node);
            break;
        }

        if (GetPooManhattanDistance(&work->pos, &gPooActor.pos) > 0x1B00) {
            SetPoohAction(work, POOH_ACTION_IDLE);
        }

        break;
    case POOH_ACTION_LOOK_AT_HONEYCOMB:
        honeycombLookAngle = work->lookAngle;
        work->lookTarget = GetAngle(work->pos.x, work->pos.y, 0x8DE00, 0x45C00);
        ApproachAngle(&honeycombLookAngle, work->lookTarget, 4);
        work->lookAngle = honeycombLookAngle;
        work->lookTimer++;

        if (!AreAllPooBeesOut()) {
            break;
        }

        SetPoohAction(work, POOH_ACTION_JUMP_SCARED);
        work->angle = FLD_ANGLE_RIGHT;
        work->lookAngle = FLD_ANGLE_RIGHT;
        work->lookColumn = work->angle;
        break;
    case POOH_ACTION_JUMP_SCARED:
        work->pos.z += work->vz;
        work->vz += 71;

        if (work->pos.z < 0) {
            break;
        }

        work->pos.z = 0;
        SetPoohAction(work, POOH_ACTION_FLEE_BEES_1);
        work->speed = 228;
        break;
    case POOH_ACTION_FLEE_BEES_1:
        work->targetX = 0x87F00;
        work->targetY = 0x4B700;
        MovePooh(work, 456, TRUE);

        if (!IsWithinPoohRadius(work->targetX >> 8, work->targetY >> 8, work->pos.x >> 8, work->pos.y >> 8)) {
            break;
        }

        SetPoohAction(work, POOH_ACTION_FLEE_BEES_2);
        break;
    case POOH_ACTION_FLEE_BEES_2:
        work->targetX = 0x75D00;
        work->targetY = 0x49E00;
        MovePooh(work, 456, TRUE);

        if (!IsWithinPoohRadius(work->targetX >> 8, work->targetY >> 8, work->pos.x >> 8, work->pos.y >> 8)) {
            break;
        }

        SetPoohAction(work, POOH_ACTION_BEE_CHASE_OVER);
        BtlMapStartShake();
        m4aSongNumStart(SONG_SND_372);
        break;
    case POOH_ACTION_IDLE:
        if (work->pos.z < 0) {
            SetPoohAction(work, POOH_ACTION_FALL);
            break;
        }

        ChoosePoohTarget(work, node);

        if (sPoohAction != POOH_ACTION_IDLE) {
            break;
        }

        if (work->targetNode == NULL) {
            break;
        }

        weight = GetPooNodeWeight(work->targetNode);

        if (weight <= 1) {
            break;
        }

        weight >>= 1;
        SetPooNodeWeight(work->targetNode, weight);
        break;
    case POOH_ACTION_THINK_START:
        if (!AnimIsFinished(&work->anim)) {
            break;
        }

        SetPoohAction(work, POOH_ACTION_THINK);
        break;
    case POOH_ACTION_THINK:
        if (!AnimIsFinished(&work->anim)) {
            break;
        }

        if (work->actionTimer <= 1) {
            work->actionTimer++;
            AnimReset(&work->anim);
            break;
        }

        if (!AnimIsFrameEnding(&work->anim)) {
            break;
        }

        if (AnimGetFrame(&work->anim) != 0) {
            break;
        }

        SetPoohAction(work, POOH_ACTION_THINK_END);
        break;
    case POOH_ACTION_THINK_END:
        if (!AnimIsFinished(&work->anim)) {
            break;
        }

        SetPoohAction(work, POOH_ACTION_LOOK);
        break;
    case POOH_ACTION_LOOK:
        if (work->lookTimer <= 59) {
            targetLookAngle = work->lookAngle;
            // @bug node is NULL near the screen's edge (NULL read).
            // The open-bus word read through NULL is then used as a pointer: BUS_PTR decodes it like the bus.
            work->lookTarget = GetAngle(work->pos.x, work->pos.y, BUS_PTR(s32*, BIOS_PTR(node)->pos)[0],
                                        BUS_PTR(s32*, BIOS_PTR(node)->pos)[1]);
            ApproachAngle(&targetLookAngle, work->lookTarget, 4);
            work->lookAngle = targetLookAngle;
            work->lookTimer++;
        } else {
            ChoosePoohTarget(work, node);
        }

        break;
    case POOH_ACTION_TRAP_FALL:
        if (!AnimIsFinished(&work->anim)) {
            break;
        }

        if (work->pos.x == 0x4A700 && work->pos.y == 0x28E00 && !IsPooEventDone(POO_EVENT_ROO)) {
            SetPoohAction(work, POOH_ACTION_TRAPPED_WITH_ROO);
            TaskCreate(&work->tasks, &gTaskDescPooRoo, &work->pos);
            break;
        }

        SetPoohAction(work, POOH_ACTION_TRAPPED);
        break;
    case POOH_ACTION_TRAPPED_WITH_ROO:
        if (!AnimIsFinished(&work->anim)) {
            break;
        }

        SetPoohAction(work, POOH_ACTION_BALLOON);
        work->pos.z -= 0x1700;
        StartPooCameraFollowPooh();
        break;
    case POOH_ACTION_TRAPPED:
        if (!AnimIsFinished(&work->anim)) {
            break;
        }

        SetPoohAction(work, POOH_ACTION_BALLOON);
        break;
    case POOH_ACTION_BALLOON:
        if (work->balloonTimer <= 59) {
            work->pos.z -= 204;
        } else if (work->balloonTimer <= 79) {
            work->pos.z -= 204;
        } else {
            work->pos.z += 204;
        }

        if (work->balloonTimer > 60) {
            work->pos.x += 128;
            work->pos.y += 64;
        }

        work->balloonTimer++;

        if (work->balloonTimer > 80 && work->pos.z >= -0x800 && IsTaskActive(work->task)) {
            TaskKill(&work->tasks, work->task);
            TaskCreate(&work->tasks, &gTaskDescPooFreeballoon, &work->pos);
            gPoohRequest = POOH_REQUEST_NONE;
        }

        if (work->pos.z < 0) {
            break;
        }

        work->pos.z = 0;

        if (ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_PLAYER)) {
            SetPoohAction(work, POOH_ACTION_IDLE);
            break;
        }

        if (gPoohRequest == POOH_REQUEST_NONE) {
            SetPoohAction(work, POOH_ACTION_IDLE);
            break;
        }

        HandlePoohRequest(work);
        break;
    case POOH_ACTION_OWL_BALLOON:
        work->pos.z -= 204;

        if (work->pos.z > -0xD500) {
            break;
        }

        if (IsTaskActive(work->task)) {
            TaskKill(&work->tasks, work->task);
        }

        SetPoohAction(work, POOH_ACTION_OWL_DESCENT);
        break;
    case POOH_ACTION_OWL_DESCENT:
        if (work->pos.z < -0x1000) {
            work->pos.z += 204;
            break;
        }

        if (!IsPooEventDone(POO_EVENT_OWL)) {
            ExitPoohMode(EVENT_137_100ACREWOOD_LV2);
            SetPooEventDone(POO_EVENT_OWL);
            SetJiminyFlag(JIMINY_RECORD_CHARACTER_OWL);
        } else {
            ExitPoohMode(EVENT_138_100ACREWOOD_LV2_RETRY);
        }

        break;
    case POOH_ACTION_TRIP:
        if (AnimIsFinished(&work->anim)) {
            if (work->actionTimer > 40) {
                SetPoohAction(work, POOH_ACTION_GET_UP);
                ApplyPooh04aFrameOffset(work);
                work->pos.z = 0;
            }

            work->actionTimer++;
            break;
        }

        if (AnimIsFrameEnding(&work->anim)) {
            ApplyPooh04FrameOffset(work);
        }

        work->actionTimer = 0;
        break;
    case POOH_ACTION_GET_UP:
        if (AnimIsFinished(&work->anim)) {
            SetPoohAction(work, POOH_ACTION_IDLE);
            break;
        }

        if (!AnimIsFrameEnding(&work->anim)) {
            break;
        }

        ApplyPooh04aFrameOffset(work);
        break;
    case POOH_ACTION_WAGON_CLIMB:
        if (!AnimIsFinished(&work->anim)) {
            break;
        }

        if (work->dirIndex == 0) {
            if (work->flipped != 0) {
                work->angle = 19;
            } else {
                work->angle = 224;
            }
        } else {
            if (work->flipped != 0) {
                work->angle = 83;
            } else {
                work->angle = 147;
            }
        }

        work->lookAngle = work->angle;
        work->lookColumn = work->angle;
        work->hopAngle = work->angle;
        SetPoohAction(work, POOH_ACTION_WAGON_DROP);
        work->vz = 0;
        work->pos.z = -0xD00;

        if (work->flipped == 0) {
            work->pos.x -= 0x900;
        } else {
            work->pos.x += 0x900;
        }

        if (!work->leavingWagon) {
            ClampToPooWagonArea((u32*)&work->pos.x, (u32*)&work->pos.y, 1);
            break;
        }

        while ((u8)IsInPooWagonAreaForPooh(&work->pos)) {
            work->pos.x += gSineTable[work->angle] * 2;
            work->pos.y -= gSineTable[work->angle + 0x40] * 2;
        }

        break;
    case POOH_ACTION_WAGON_DROP:
        work->pos.z += work->vz;
        work->vz += 17;

        if (work->pos.z < 0) {
            break;
        }

        work->pos.z = 0;

        if (!work->leavingWagon) {
            SetPoohAction(work, POOH_ACTION_WAGON_WAIT);
            break;
        }

        SetPoohAction(work, POOH_ACTION_IDLE);
        gPoohRequest = POOH_REQUEST_NONE;
        break;
    case POOH_ACTION_WAGON_WAIT:
        if (!IsPooSoraCallStarting()) {
            break;
        }

        if ((u8)IsInPooWagonArea(&gPooActor.pos)) {
            break;
        }

        work->angle = work->hopAngle + 128;
        work->lookAngle = work->angle;
        work->lookColumn = work->angle;
        SetPoohAction(work, POOH_ACTION_WAGON_CLIMB);
        work->leavingWagon = TRUE;
        break;
    case POOH_ACTION_STUMP_CLIMB:
        if (!AnimIsFinished(&work->anim)) {
            break;
        }

        SetPoohAction(work, POOH_ACTION_STUMP_WAIT);
        ResetPoohStumpCount(work);
        work->pos.z -= 0xE00;
        work->lookAngle = work->angle;
        work->lookColumn = work->angle;

        if (work->flipped == 0) {
            work->pos.x -= 0x900;
        } else {
            work->pos.x += 0x900;
        }

        break;
    case POOH_ACTION_STUMP_WAIT:
        work->onCollider = 1;

        if (!IsPooSoraCallStarting()) {
            break;
        }

        if (IsPoohNearScreenEdge()) {
            break;
        }

        SetPoohAction(work, POOH_ACTION_STUMP_WALK);
        break;
    case POOH_ACTION_LOOK_AT_HONEYCOMB_DONE:
        if (work->actionTimer <= 179) {
            doneLookAngle = work->lookAngle;
            work->lookTarget = GetAngle(work->pos.x, work->pos.y, 0x8DE00, 0x45C00);
            ApproachAngle(&doneLookAngle, work->lookTarget, 4);
            work->lookAngle = doneLookAngle;
            work->actionTimer++;
        } else {
            work->angle += 128;
            work->lookAngle = work->angle;
            work->lookColumn = work->angle;
            SetPoohAction(work, POOH_ACTION_WALK_AWAY);
        }

        break;
    case POOH_ACTION_WALK_AWAY:
        if (work->actionTimer > 119) {
            SetPoohAction(work, POOH_ACTION_IDLE);
            break;
        }

        work->actionTimer++;
        MovePooh(work, 76, FALSE);
        break;
    case POOH_ACTION_STUMP_WALK:
        if (work->onCollider != 0) {
            work->targetX = gPooActor.pos.x;
            work->targetY = gPooActor.pos.y;
            MovePooh(work, 76, TRUE);
            break;
        }

        work->targetX = gPooActor.pos.x;
        work->targetY = gPooActor.pos.y;
        work->angle = GetAngle(work->pos.x, work->pos.y, work->targetX, work->targetY);
        work->vz = -0x130;
        work->speed = 237;
        SetPoohAction(work, POOH_ACTION_STUMP_JUMP);
        break;
    case POOH_ACTION_STUMP_JUMP:
        if (AnimIsFinished(&work->anim) && work->onCollider != 0 && work->pos.z < -0x100) {
            SetPoohAction(work, POOH_ACTION_STUMP_WAIT);
            UpdatePoohStumpCircle(work);
            break;
        }

        if (AnimGetFrame(&work->anim) == 2 && work->anim.timer == 0) {
            m4aSongNumStart(SONG_SND_960);
        }

        if ((AnimGetFrame(&work->anim) > 1 && AnimGetFrame(&work->anim) <= 4) ||
            (AnimGetFrame(&work->anim) > 4 && work->onCollider == 0 && work->pos.z < work->groundZ)) {
            work->pos.x += gSineTable[work->angle] * work->speed >> 8;
            work->pos.y += -gSineTable[work->angle + 0x40] * work->speed >> 8;
        }

        if (AnimGetFrame(&work->anim) <= 1) {
            break;
        }
    case POOH_ACTION_FALL:
        work->pos.z += work->vz;
        work->vz += 17;

        if (work->pos.z >= work->groundZ) {
            work->pos.z = work->groundZ;
        }

        if (AnimIsFinished(&work->anim)) {
            SetPoohAction(work, POOH_ACTION_IDLE);
        }

        break;
    case POOH_ACTION_SIT_DOWN:
        if (!AnimIsFinished(&work->anim)) {
            break;
        }

        SetPoohAction(work, POOH_ACTION_SIT);
        break;
    case POOH_ACTION_SIT:
        if (work->actionTimer <= 119) {
            work->actionTimer++;

            if (!IsPooSoraCallStarting()) {
                break;
            }

            if (IsPoohNearScreenEdge()) {
                break;
            }

            if (gPoohGauge != 0) {
                SetPoohAction(work, POOH_ACTION_STAND_UP);
            }
        } else {
            SetPoohAction(work, POOH_ACTION_LIE_DOWN);
        }

        break;
    case POOH_ACTION_LIE_DOWN:
        if (!AnimIsFinished(&work->anim)) {
            break;
        }

        SetPoohAction(work, POOH_ACTION_SLEEP);
        work->sleepTimer = gPoohGaugeTimer * 1800 / 1851;
        break;
    case POOH_ACTION_SLEEP:
        if (IsPooSoraCallStarting() && !IsPoohNearScreenEdge() && gPoohGauge != 0) {
            SetPoohAction(work, POOH_ACTION_WAKE_UP);

            if (!IsTaskActive(work->zzzTask)) {
                break;
            }

            TaskKill(&work->tasks, work->zzzTask);
            break;
        }

        TASK_CREATE_IF_INACTIVE(work->zzzTask, &work->tasks, &gTaskDescPooZzz, &work->flipped);

        if (gPoohGauge == 0) {
            work->sleepTimer++;

            if (work->sleepTimer > 1800) {
                gPoohGauge++;
                gPoohGaugeTimer = 1851;
                work->sleepTimer = 0;
            }

            if (gPoohGauge > 3) {
                gPoohGauge = 3;
            }
        }

        break;
    case POOH_ACTION_WAKE_UP:
        if (AnimIsFinished(&work->anim)) {
            SetPoohAction(work, POOH_ACTION_STAND_UP);
        }

        break;
    case POOH_ACTION_STAND_UP:
        if (AnimIsFinished(&work->anim)) {
            SetPoohAction(work, POOH_ACTION_IDLE);
        }

        break;
    case POOH_ACTION_SIT_FOR_HONEY:
        if (!AnimIsFinished(&work->anim)) {
            break;
        }

        if (gPoohGauge == 1) {
            SetPoohAction(work, POOH_ACTION_EAT_HONEY_1);
        } else if (gPoohGauge == 2) {
            SetPoohAction(work, POOH_ACTION_EAT_HONEY_2);
        } else {
            SetPoohAction(work, POOH_ACTION_EAT_HONEY_3);
        }

        break;
    case POOH_ACTION_EAT_HONEY_1:
    case POOH_ACTION_EAT_HONEY_2:
    case POOH_ACTION_EAT_HONEY_3:
        if (!AnimIsFinished(&work->anim)) {
            break;
        }

        SetPoohAction(work, POOH_ACTION_STAND_UP);
        work->pos.x -= 0x400;
        work->pos.y += 0x300;
        break;
    }
}

void UpdatePoohGauge(PoohWork* work) {
    if (sPoohAction == POOH_ACTION_WALK) {
        if (gPoohGauge != 0) {
            gPoohGaugeTimer--;
        }

        if (gPoohGaugeTimer == 0) {
            if (gPoohGauge == 0) {
                SetPoohAction(work, POOH_ACTION_SIT_DOWN);
            } else {
                gPoohGauge--;

                if (gPoohGauge == 0) {
                    SetPoohAction(work, POOH_ACTION_SIT_DOWN);
                } else {
                    gPoohGaugeTimer = 0x73B;
                }
            }
        }
    }
}

u8 IsPoohOffScreen() {
    s32 x;
    s32 y;

    x = (gPoohPos->x >> 8) - gPooScrollX;
    y = (gPoohPos->y >> 8) + (gPoohPos->z >> 8) - gPooScrollY;

    if (IsRectOutsideScreen(x, y, gPoohHitBox.height, 0, gPoohHitBox.radius, gPoohHitBox.radius)) {
        return TRUE;
    }

    return FALSE;
}

u8 task_poo_pooh_1(PoohWork* work) {
    PooNode* node;

    work->groundZ = GetPooGroundZ(&work->collider, &work->pos, &work->onCollider);

    if (IsPoohNearScreenEdge()) {
        node = NULL;
    } else {
        node = FindPoohTargetNode();
    }

    work->hideShadow = FALSE;
    UpdatePoohAction(work, node);
    UpdatePoohGauge(work);
    SetPoohAnimation(work, sPoohAction);
    work->gfx = AnimUpdate(&work->anim);

    if (sPoohAction == POOH_ACTION_WALK || sPoohAction == POOH_ACTION_STUMP_WALK) {
        if (work->anim.timer == 0) {
            switch (AnimGetFrame(&work->anim)) {
            case 9:
                m4aSongNumStart(SONG_SYS_POO_FOOTL);
                break;
            case 3:
                m4aSongNumStart(SONG_SYS_POO_FOOTR);
                break;
            }
        }
    }

    ColliderSetPosition(&work->collider, work->pos.x, work->pos.y, work->pos.z);
    TaskPoolUpdate(&work->tasks);
    return 1;
}

void task_poo_pooh_2(PoohWork* work) {
    s16 x;
    s16 y;
    s32 flags;
    u16 priority;

    x = (work->pos.x >> 8) - gPooScrollX;
    y = (work->pos.y >> 8) + (work->pos.z >> 8) - gPooScrollY;

    if (work->flipped != 0) {
        flags = SPRITE_PRIORITY(2) | SPRITE_FLAG_HFLIP;
    } else {
        flags = SPRITE_PRIORITY(2);
    }

    if (sPoohAction == POOH_ACTION_STUMP_CLIMB && work->dirIndex == 1) {
        if (AnimGetFrame(&work->anim) <= 4) {
            priority = -0x1003 - ((gPoohRequestY - 0x500) >> 8) * 4;
            work->shadowInfo.z = 0;
        } else {
            priority = -0x1005 - ((gPoohRequestY - 0x500) >> 8) * 4;
            work->shadowInfo.priority = 0;
        }
    } else if (IsPoohOnWagon()) {
        priority = GetPooWagonPriority() - 4;
        work->shadowInfo.priority = priority + 1;
        work->shadowInfo.z = 0;
    } else if ((u8)GetPooWagonSide(gPoohPos->x, gPoohPos->y) != 0) {
        if (sPoohAction == POOH_ACTION_WAGON_CLIMB && work->dirIndex == 1 && work->leavingWagon) {
            priority = GetPooWagonPriority2() - 3;
        } else if ((u8)GetPooWagonSide(gPoohPos->x, gPoohPos->y) == FLD_ANGLE_DOWN_RIGHT ||
                   (u8)GetPooWagonSide(gPoohPos->x, gPoohPos->y) == FLD_ANGLE_DOWN_LEFT) {
            if (gPoohPos->y < gPooActor.pos.y) {
                priority = GetPooWagonPriority() + 5;
            } else {
                priority = GetPooWagonPriority() + 1;
            }
        } else {
            if (gPoohPos->y < gPooActor.pos.y) {
                priority = GetPooWagonPriority2() - 2;
            } else {
                priority = GetPooWagonPriority2() - 6;
            }
        }

        work->shadowInfo.priority = priority + 1;
        work->shadowInfo.z = 0;
    } else if (work->onCollider != 0) {
        priority = -0x1008 - (work->collider.platformY >> 8) * 4;

        if (work->pos.y >= gPooActor.pos.y) {
            priority -= 2;
        } else {
            priority += 2;
        }

        if (work->collider.penetration <= work->collider.radius || work->collider.other->radius == 0x400) {
            if (work->collider.platformZ != 0) {
                work->shadowInfo.priority = 0;
            } else {
                work->shadowInfo.priority = priority + 1;
            }

            work->shadowInfo.z = 0;
        } else {
            work->shadowInfo.z = work->collider.platformZ;
            work->shadowInfo.priority = priority + 1;
        }
    } else {
        priority = -0x1004 - (work->pos.y >> 8) * 4;
        work->shadowInfo.z = 0;

        if (work->shadowInfo.z != work->pos.ground) {
            work->shadowInfo.priority = 0;
        } else {
            work->shadowInfo.priority = 0xFFF0;
        }
    }

    if (work->hideShadow) {
        work->shadowInfo.priority = 0;
    }

    DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, flags, priority);
    TaskPoolDraw(&work->tasks);
}

void task_poo_pooh_3(PoohWork* work) {
    if (sPoohAction == POOH_ACTION_OWL_DESCENT) {
        sPoohAction = POOH_ACTION_IDLE;
        work->pos.z = 0;
        work->pos.y += 0x2000;
    } else if (sPoohAction == POOH_ACTION_BEE_CHASE_OVER) {
        sPoohAction = POOH_ACTION_IDLE;
        work->pos.x = 0x7F700;
        work->pos.y = 0x47E00;
        work->pos.z = 0;
    }

    SetPooStatePooh(&work->pos, sPoohAction);
    SetPooStateGauge(gPoohGauge, gPoohGaugeTimer);
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ColliderUnregister(&work->collider);
    TaskPoolDestroy(&work->tasks);
}

u8 IsPoohDescendingWithOwl() {
    if (sPoohAction == POOH_ACTION_OWL_DESCENT) {
        return TRUE;
    }

    return FALSE;
}

u8 IsPoohOnOwlBalloon() {
    if (sPoohAction == POOH_ACTION_OWL_BALLOON) {
        return TRUE;
    }

    return FALSE;
}

u8 IsPoohWalkingToTarget() {
    if (sPoohAction == POOH_ACTION_WALK) {
        return TRUE;
    }

    return FALSE;
}

u8 IsPoohBeeChaseOver() {
    if (sPoohAction == POOH_ACTION_BEE_CHASE_OVER) {
        return TRUE;
    }

    return FALSE;
}

u8 IsPoohWaitingOnWagon() {
    if (sPoohAction == POOH_ACTION_WAGON_WAIT) {
        return TRUE;
    }

    return FALSE;
}

u8 IsPoohOnWagon() {
    PoohWork* work;

    if (sPoohAction == POOH_ACTION_WAGON_WAIT) {
        return TRUE;
    }

    if (sPoohAction == POOH_ACTION_WAGON_DROP) {
        if (sPooWork->leavingWagon) {
            return FALSE;
        }

        return TRUE;
    }

    if (sPoohAction != POOH_ACTION_WAGON_CLIMB) {
        return FALSE;
    }

    work = sPooWork;

    if (work->leavingWagon) {
        if (work->dirIndex != 1) {
            return TRUE;
        }

        if (AnimGetFrame(&work->anim) > 4) {
            return FALSE;
        }

        return TRUE;
    }

    if (work->dirIndex == 0) {
        return FALSE;
    }

    if (AnimGetFrame(&work->anim) <= 4) {
        return FALSE;
    }

    return TRUE;
}

u8 IsPoohAtLowerExit() {
    if (GetPooExitAt(gPoohPos) == 2) {
        return TRUE;
    }

    return FALSE;
}

u16 GetPoohHoneyAnim() {
    if (sPoohAction == POOH_ACTION_EAT_HONEY_1) {
        return 0;
    }

    if (sPoohAction == POOH_ACTION_EAT_HONEY_2) {
        return 1;
    }

    if (sPoohAction == POOH_ACTION_EAT_HONEY_3) {
        return 2;
    }

    return 3;
}

u8 IsPoohLookingAtHoneycomb() {
    if (sPoohAction == POOH_ACTION_LOOK_AT_HONEYCOMB) {
        return TRUE;
    }

    return FALSE;
}

void CreatePooPileTasks(void* pool) {
    u32 i;

    for (i = 0; i < 12; i++) {
        CreatePooPileTask(pool, gPooPileDescs[i].kind, gPooPileDescs[i].x, gPooPileDescs[i].y);
    }
}

u16 CreatePooMapobjhitTasks(void* pool, u16 prizeId) {
    u32 i;

    for (i = 0; i < 80; i++) {
        CreatePooMapobjhitTask(pool, gPooTileDescs[i].kind, gPooTileDescs[i].x, gPooTileDescs[i].y, prizeId);
        prizeId++;
    }

    return prizeId;
}

u16 CreatePooSpawnTasks(void* pool, u16 prizeId) {
    u32 i;

    for (i = 0; i < ARRAY_COUNT(gPooSpawns); i++) {
        sPooSpawnPos.x = gPooSpawns[i].x;
        sPooSpawnPos.y = gPooSpawns[i].y;
        sPooSpawnPos.z = 0;

        if (gPooSpawns[i].desc == &gTaskDescPooTanpopo || gPooSpawns[i].desc == &gTaskDescPooLeaf) {
            sPooSpawnArgs.pos = sPooSpawnPos;
            sPooSpawnArgs.prizeId = prizeId;
            TaskCreate(pool, gPooSpawns[i].desc, &sPooSpawnArgs);
            prizeId++;
        } else {
            TaskCreate(pool, gPooSpawns[i].desc, &sPooSpawnPos);
        }
    }

    return prizeId;
}

void SetPooCameraFocus(s32 x, s32 y) {
    gPooCameraFocusX = x;
    gPooCameraFocusY = y;
}

void UpdatePooCameraCenter() {
    s32 y;

    if (sPooCameraFrozen) {
        return;
    }

    if (sPooCameraFollowPooh) {
        SetPooCameraFocus(gPoohPos->x, gPoohPos->y + gPoohPos->z);
    }

    y = gPoohPos->y + gPoohPos->z;

    if ((gPooCameraFocusX - gPoohPos->x >= 0 ? gPooCameraFocusX - gPoohPos->x < 0xF000
                                                    : gPoohPos->x - gPooCameraFocusX < 0xF000) &&
        (gPooCameraFocusY - y >= 0 ? gPooCameraFocusY - y < 0xA000 : y - gPooCameraFocusY < 0xA000)) {
        gPooCameraX = (gPooCameraFocusX + gPoohPos->x) / 2;
        gPooCameraY = (gPooCameraFocusY + y) / 2;
    } else {
        gPooCameraX = gPooCameraFocusX;
        gPooCameraY = gPooCameraFocusY;
    }
}

void FreezePooCamera() {
    sPooCameraFrozen = TRUE;
}

void UnfreezePooCamera() {
    sPooCameraFrozen = FALSE;
}

void StartPooCameraFollowPooh() {
    sPooCameraFollowPooh = TRUE;
}

void StopPooCameraFollowPooh() {
    sPooCameraFollowPooh = FALSE;
}

void task_poo_map_0(PooMapWork* work) {
    PooPos poohPos;
    s32 poohAction;

    UnfreezePooCamera();
    StopPooCameraFollowPooh();
    GetPooStatePooh(&poohPos, &poohAction);
    gPoohPos = &poohPos;
    UpdatePooCameraCenter();
    sPooCameraMaxY = gPooCameraY;
    gPooScrollX = (gPooCameraX >> 8) - DISPLAY_WIDTH / 2;
    gPooScrollY = (gPooCameraY >> 8) - DISPLAY_HEIGHT / 2;
    gFieldState->x = gPooScrollX << 8;
    gFieldState->y = gPooScrollY << 8;
    work->mapWidth = gPooMapBgDesc.mapWidth;
    work->mapHeight = gPooMapBgDesc.mapHeight;
    TaskPoolInit(&work->tasks, 178);
    CreatePooMapobjhitTasks(&work->tasks, CreatePooSpawnTasks(&work->tasks, 0));
    CreatePooPileTasks(&work->tasks);
    TaskCreate(&work->tasks, &gTaskDescPooMapanime, NULL);
    LoadBgTiles(3, gPooMapBgDesc.tiles, gPooMapBgDesc.tilesSize);
    LoadBgTiles(2, gPooMapBgDesc.tiles2, gPooMapBgDesc.tilesSize2);
    LoadBgPalette(3, gPooMapBgDesc.palette, gPooMapBgDesc.paletteSize);
    SetBgMapBlocks(3, gPooBg3MapBlocks, work->mapWidth, work->mapHeight);
    RedrawBgMapAt(3, gPooScrollX, gPooScrollY);
    func_080CA35C();
    SetBgMapBlocks(1, gPooBg1MapBlocks, work->mapWidth, work->mapHeight);
    RedrawBgMapAt(1, gPooScrollX, gPooScrollY);
    SetBgMapBlocks(2, gPooBg2MapBlocks, work->mapWidth, work->mapHeight);
    RedrawBgMapAt(2, gPooScrollX, gPooScrollY);
    BtlMapResetShake();
}

u8 task_poo_map_1(PooMapWork* work) {
    UpdatePooCameraCenter();
    ScrollPooCamera(work);
    ScrollBgMapTo(3, gPooScrollX, gPooScrollY);
    ScrollBgMapTo(1, gPooScrollX, gPooScrollY);
    ScrollBgMapTo(2, gPooScrollX, gPooScrollY);
    TaskPoolUpdate(&work->tasks);
    return 1;
}

void task_poo_map_2(PooMapWork* work) {
    TaskPoolDraw(&work->tasks);
}

void task_poo_map_3(PooMapWork* work) {
    TaskPoolDestroy(&work->tasks);
}

void ScrollPooCamera(PooMapWork* work) {
    s32 tx;
    s32 ty;

    BtlMapUpdateShake();
    tx = gPooCameraX - 0x7800;
    ty = gPooCameraY - 0x5000;

    if (ty < 0) {
        ty = 0;
    }

    if (gPooCameraX + 0x7800 > 0xEDF00) {
        tx = 0xDEF00;
    }

    if (ty + 0xA000 > 0x83000) {
        ty = 0x79000;
    }

    tx = (tx - (gPooScrollX << 8)) >> 3;
    ty = (ty - (gPooScrollY << 8)) >> 3;

    if (tx > 0x300) {
        tx = 0x300;
    } else if (tx < -0x300) {
        tx = -0x300;
    }

    if (ty > 0x300) {
        ty = 0x300;
    } else if (ty < -0x300) {
        ty = -0x300;
    }

    if (abs(tx) <= 50) {
        tx = 0;
    }

    if (abs(ty) <= 50) {
        ty = 0;
    }

    gPooScrollX += tx >> 8;
    gPooScrollY += ty >> 8;
    gPooScrollY += BtlMapGetShake() >> 8;
    gFieldState->x = gPooScrollX << 8;
    gFieldState->y = gPooScrollY << 8;
}

void func_080CA35C() {
    gUnk_0203C3F4 = 0;
}

void func_080CA368(s32 bg, u16 x, u16 y) {
}

u16 GetPooMapTile(u16 x, u16 y) {
    const u16** blocks;
    u32 bx;
    u32 by;
    u32 tx;
    u32 ty;

    blocks = gPooBg3MapBlocks;
    bx = x >> 8;
    by = y >> 8;
    tx = (x >> 3) & 0x1F;
    ty = (y >> 3) & 0x1F;
    // @bug Reads past the end of gPooBg3MapBlocks off the map.
#ifdef PLATFORM_PC
    /* the word past the table is no pointer: read where the bus sends it (BUS_PTR) */
    return *BUS_PTR(const u16*, (__UINTPTR_TYPE__)blocks[by * 16 + bx] + (ty * 32 + tx) * 2);
#else
    return blocks[by * 16 + bx][ty * 32 + tx];
#endif
}

u8 IsPooPosBlocked(PooPos* pos) {
    u32 tile;

    tile = GetPooMapTile(pos->x >> 8, pos->y >> 8) & 0x3FF;

    if ((u16)(tile - 1) <= 8) {
        return FALSE;
    }

    if ((u16)(tile - 0x20) <= 9) {
        return FALSE;
    }

    if ((u16)(tile - 0x40) <= 9) {
        return FALSE;
    }

    if ((u16)(tile - 0x1E0) > 0x5F) {
        return TRUE;
    }

    return FALSE;
}

u8 GetPooWallSlide(PooActor* actor, s32 x, s32 y, s32* ox, s32* oy) {
    PooPos probe;
    s32 dirX;
    s32 dirY;
    s32 speed;
    s32 zero;
    u8 slideAngle;

    if (IsPooPosBlocked(&actor->pos)) {
        slideAngle = actor->angle + 0x40;
        dirX = gSineTable[slideAngle];
        dirY = -gSineTable[slideAngle + 0x40];
        probe.x = dirX * 4 + x;
        probe.y = dirY * 4 + y;
        probe.z = actor->pos.z;
        probe.ground = actor->pos.ground;
        speed = actor->speed;

        if (speed > 0x200) {
            speed = 0x200;
        }

        if (!IsPooPosBlocked(&probe)) {
            *ox = dirX * speed >> 8;
            *oy = dirY * speed >> 8;
            return TRUE;
        } else {
            slideAngle = actor->angle - 0x40;
            dirX = gSineTable[slideAngle];
            dirY = -gSineTable[slideAngle + 0x40];
            probe.x = dirX * 4 + x;
            probe.y = dirY * 4 + y;

            if (!IsPooPosBlocked(&probe)) {
                *ox = dirX * speed >> 8;
                *oy = dirY * speed >> 8;
                return TRUE;
            } else {
                zero = 0;
                *ox = zero;
                *oy = zero;
                return TRUE;
            }
        }
    }

    return FALSE;
}

u8 GetPooScreenOverflow(s16 x, s16 y, s16 h, s16 vy, s16 w, s16 vx, s32* ox, s32* oy) {
    s32 edge;
    s32 overflowed;

    overflowed = FALSE;
    *oy = 0;
    *ox = 0;
    edge = x + vx;

    if (edge >= 0) {
        edge = x - w;

        if (edge > DISPLAY_WIDTH) {
            edge -= DISPLAY_WIDTH;
            *ox = edge << 8;
            overflowed = TRUE;
        }
    } else {
        *ox = edge << 8;
        overflowed = TRUE;
    }

    edge = y + vy;

    if (edge >= 0) {
        edge = y - h;

        if (edge > DISPLAY_HEIGHT) {
            edge -= DISPLAY_HEIGHT;
            *oy = edge << 8;
            overflowed = TRUE;
        }
    } else {
        *oy = edge << 8;
        overflowed = TRUE;
    }

    return overflowed;
}

u8 ConstrainPooActorMove(PooActor* actor, s32 x, s32 y) {
    s16 sx;
    s16 sy;

    if (GetPooWallSlide(actor, x, y, &sPooMoveAdjustX, &sPooMoveAdjustY)) {
        actor->pos.x = x + sPooMoveAdjustX;
        actor->pos.y = y + sPooMoveAdjustY;
    }

    sx = (x >> 8) - gPooScrollX;
    sy = (y >> 8) - gPooScrollY;

    if (!GetPooScreenOverflow(sx, sy, 48, 0, 18, 18, &sPooMoveAdjustX, &sPooMoveAdjustY)) {
        sx = (actor->pos.x >> 8) - gPooScrollX;
        sy = (actor->pos.y >> 8) - gPooScrollY;

        if (GetPooScreenOverflow(sx, sy, 48, 0, 18, 18, &sPooMoveAdjustX, &sPooMoveAdjustY)) {
            actor->pos.x -= sPooMoveAdjustX;
            actor->pos.y -= sPooMoveAdjustY;
        }
    } else {
        actor->pos.x = x;
        actor->pos.y = y;
    }

    return 1;
}

u8 IsPooExitTile(PooPos* pos) {
    if ((GetPooMapTile(pos->x >> 8, pos->y >> 8) & 0x3FF) > 0x3BA) {
        return TRUE;
    }

    return FALSE;
}

u16 GetPooExitAt(PooPos* pos) {
    if (IsPooExitTile(pos)) {
        if (pos->y <= 0x1FFFF) {
            return 1;
        }

        return 2;
    }

    return 0;
}

void MovePooCamera(s32 dx, s32 dy) {
    gPooCameraX += dx;
    gPooCameraY += dy;

    if (sPooCameraMaxY < gPooCameraY) {
        gPooCameraY = sPooCameraMaxY;
    }

    gPooScrollX = (gPooCameraX >> 8) - DISPLAY_WIDTH / 2;
    gPooScrollY = (gPooCameraY >> 8) - DISPLAY_HEIGHT / 2;
    gFieldState->x = gPooScrollX << 8;
    gFieldState->y = gPooScrollY << 8;
    ScrollBgMapTo(3, gPooScrollX, gPooScrollY);
    ScrollBgMapTo(1, gPooScrollX, gPooScrollY);
    ScrollBgMapTo(2, gPooScrollX, gPooScrollY);
}

void SetPooActorAngleFromDpad(PooActor* actor) {
    if ((GetKeysHeld() & DPAD_LEFT) != 0 && (GetKeysHeld() & DPAD_DOWN) != 0) {
        actor->angle = FLD_ANGLE_DOWN_LEFT;
    } else if ((GetKeysHeld() & DPAD_UP) != 0 && (GetKeysHeld() & DPAD_LEFT) != 0) {
        actor->angle = FLD_ANGLE_UP_LEFT;
    } else if ((GetKeysHeld() & DPAD_UP) != 0 && (GetKeysHeld() & DPAD_RIGHT) != 0) {
        actor->angle = FLD_ANGLE_UP_RIGHT;
    } else if ((GetKeysHeld() & DPAD_RIGHT) != 0 && (GetKeysHeld() & DPAD_DOWN) != 0) {
        actor->angle = FLD_ANGLE_DOWN_RIGHT;
    } else if ((GetKeysHeld() & DPAD_DOWN) != 0 && GetKeyReleaseTime(DPAD_LEFT) <= 4) {
        actor->angle = FLD_ANGLE_DOWN_LEFT;
    } else if ((GetKeysHeld() & DPAD_DOWN) != 0 && GetKeyReleaseTime(DPAD_RIGHT) <= 4) {
        actor->angle = FLD_ANGLE_DOWN_RIGHT;
    } else if ((GetKeysHeld() & DPAD_UP) != 0 && GetKeyReleaseTime(DPAD_LEFT) <= 4) {
        actor->angle = FLD_ANGLE_UP_LEFT;
    } else if ((GetKeysHeld() & DPAD_UP) != 0 && GetKeyReleaseTime(DPAD_RIGHT) <= 4) {
        actor->angle = FLD_ANGLE_UP_RIGHT;
    } else if ((GetKeysHeld() & DPAD_LEFT) != 0 && GetKeyReleaseTime(DPAD_UP) <= 4) {
        actor->angle = FLD_ANGLE_UP_LEFT;
    } else if ((GetKeysHeld() & DPAD_LEFT) != 0 && GetKeyReleaseTime(DPAD_DOWN) <= 4) {
        actor->angle = FLD_ANGLE_DOWN_LEFT;
    } else if ((GetKeysHeld() & DPAD_RIGHT) != 0 && GetKeyReleaseTime(DPAD_UP) <= 4) {
        actor->angle = FLD_ANGLE_UP_RIGHT;
    } else if ((GetKeysHeld() & DPAD_RIGHT) != 0 && GetKeyReleaseTime(DPAD_DOWN) <= 4) {
        actor->angle = FLD_ANGLE_DOWN_RIGHT;
    } else if ((GetKeysHeld() & DPAD_DOWN) != 0) {
        actor->angle = FLD_ANGLE_DOWN;
    } else if ((GetKeysHeld() & DPAD_UP) != 0) {
        actor->angle = FLD_ANGLE_UP;
    } else if ((GetKeysHeld() & DPAD_LEFT) != 0) {
        actor->angle = FLD_ANGLE_LEFT;
    } else if ((GetKeysHeld() & DPAD_RIGHT) != 0) {
        actor->angle = FLD_ANGLE_RIGHT;
    }
}

u8 ApplyPooSoraPushOut(PooSoraWork* work, PooPos* pos) {
    if (work->collider.colliding && !ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_PICKUP) && !ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_ENEMY) && !ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_PICKUP) && !ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_ENEMY_NO_ENEMY_COLLISION)) {
        if (IsPooSoraOverWagon()) {
            pos->x += work->collider.pushX;
            pos->y += work->collider.pushY;
            SnapToPooWagonLine((u32*)&pos->x, (u32*)&pos->y, 1);
        } else {
            pos->x += work->collider.pushX;
            pos->y += work->collider.pushY;
        }

        return TRUE;
    }

    return FALSE;
}

u8 GetPooAngleToPooh(PooPos* pos) {
    u8 angle;

    angle = GetAngle(pos->x, pos->y, gPoohPos->x, gPoohPos->y);

    switch (ANGLE_DIR8(angle)) {
    case DIR8_UP_RIGHT:
        return FLD_ANGLE_UP_RIGHT;
    case DIR8_RIGHT:
        return FLD_ANGLE_RIGHT;
    case DIR8_DOWN_RIGHT:
        return FLD_ANGLE_DOWN_RIGHT;
    case DIR8_DOWN:
        return FLD_ANGLE_DOWN;
    case DIR8_DOWN_LEFT:
        return FLD_ANGLE_DOWN_LEFT;
    case DIR8_LEFT:
        return FLD_ANGLE_LEFT;
    case DIR8_UP_LEFT:
        return FLD_ANGLE_UP_LEFT;
    case DIR8_UP:
    default:
        return FLD_ANGLE_UP;
    }
}

void UpdatePooActorAngle(PooActor* actor) {
    u8 old;

    old = actor->angle;
    SetPooActorAngleFromDpad(actor);

    if (old != actor->angle) {
        if (abs((s8)GetAngleDiff(old, actor->angle)) > 100) {
            actor->speed = 0;
        } else {
            actor->speed >>= 1;
        }
    }
}

s32 GetPooSoraGroundZ(PooSoraWork* work) {
    PooPos* pos;
    s32 groundZ;

    pos = &gPooActor.pos;

    if ((work->collider.standFlags & COLLIDER_STAND_OVER_PLATFORM) != 0) {
        if (pos->ground < work->collider.platformZ) {
            groundZ = pos->ground;
        } else {
            groundZ = work->collider.platformZ;
        }

        work->onCollider = 1;
    } else {
        work->onCollider = 0;
        groundZ = pos->ground;
    }

    return groundZ;
}

void SetPooSoraAnimation(PooSoraWork* work, s32 animAction, u16 flags) {
    const PooAnimDesc* desc;
    s32 dir;

    switch (gPooActor.angle) {
    case FLD_ANGLE_UP_RIGHT:
        dir = 4;
        work->flags |= POO_SORA_FLAG_FLIP_X;
        break;
    case FLD_ANGLE_RIGHT:
        dir = 3;
        work->flags |= POO_SORA_FLAG_FLIP_X;
        break;
    case FLD_ANGLE_DOWN_RIGHT:
        dir = 2;
        work->flags |= POO_SORA_FLAG_FLIP_X;
        break;
    case FLD_ANGLE_DOWN:
        dir = 1;
        work->flags &= ~POO_SORA_FLAG_FLIP_X;
        break;
    case FLD_ANGLE_DOWN_LEFT:
        dir = 2;
        work->flags &= ~POO_SORA_FLAG_FLIP_X;
        break;
    case FLD_ANGLE_LEFT:
        dir = 3;
        work->flags &= ~POO_SORA_FLAG_FLIP_X;
        break;
    case FLD_ANGLE_UP_LEFT:
        dir = 4;
        work->flags &= ~POO_SORA_FLAG_FLIP_X;
        break;
    case FLD_ANGLE_UP:
    default:
        dir = 0;
        work->flags &= ~POO_SORA_FLAG_FLIP_X;
        break;
    }

    if (work->animAction == animAction) {
        flags |= ANIM_FLAG_KEEP_FRAME;
    }

    work->animAction = animAction;
    desc = &gPooSoraAnimDescs[animAction][dir];
    AnimChangeWithTables(&work->anim, (u8)desc->animId, flags, desc->unk_04, desc->unk_00);
    SetObjTileSource(work->tiles, desc->tiles);
}

void SetPooSoraAttackPoint(PooActor* actor) {
    s32 x;
    s32 y;

    switch (actor->angle) {
    case FLD_ANGLE_UP_RIGHT:
    case FLD_ANGLE_UP_LEFT:
        x = actor->pos.x + gSineTable[actor->angle] * 12;
        y = actor->pos.y + -gSineTable[actor->angle + 0x40] * 12;
        break;
    case FLD_ANGLE_RIGHT:
    case FLD_ANGLE_LEFT:
        x = actor->pos.x + gSineTable[actor->angle] * 27;
        y = actor->pos.y + -gSineTable[actor->angle + 0x40] * 27;
        break;
    case FLD_ANGLE_UP:
    case FLD_ANGLE_DOWN_RIGHT:
    case FLD_ANGLE_DOWN:
    case FLD_ANGLE_DOWN_LEFT:
    default:
        x = actor->pos.x + gSineTable[actor->angle] * 20;
        y = actor->pos.y + -gSineTable[actor->angle + 0x40] * 20;
        break;
    }

    SetPooAttackPoint(x, y, actor->pos.z - 0x800);
}

enum PooSoraState {
    POO_SORA_STATE_STAND,
    POO_SORA_STATE_WALK,
    POO_SORA_STATE_JUMP_START,
    POO_SORA_STATE_JUMP_RISE,
    POO_SORA_STATE_JUMP_FALL,
    POO_SORA_STATE_JUMP_LAND,
    POO_SORA_STATE_ATTACK,
    POO_SORA_STATE_AIR_ATTACK,
    POO_SORA_STATE_CALL
};

void task_poo_sora_0(PooSoraWork* work) {
    PooActor* actor = &gPooActor;

    gPooSoraCollider = &work->collider;
    gPooSoraNode = &work->node;
    sPooSoraWork = work;
    work->tiles = AllocObjTiles(0xA00, NULL);
    work->palette = LoadObjPalette(gSoraPalette, sizeof(gSoraPalette));
    actor->height = 16;
    work->onCollider = 0;
    work->timer = 0;
    work->flags = 0;
    work->animAction = 12;
    actor->unk_32 = 0;
    actor->kind = FLD_OBJ_KIND_PLAYER;
    GetPooStatePos2(&actor->pos);
    actor->angle = FLD_ANGLE_DOWN_LEFT;
    actor->pos.ground = 0;
    actor->speed = 0;
    SetPooCameraFocus(actor->pos.x, actor->pos.y + actor->pos.z);
    AnimInit(&work->anim, NULL, NULL);
    SetPooSoraAnimation(work, 0, 1);
    work->gfx = AnimGetGfx(&work->anim);
    work->sounds = gPooSoraSounds;
    TaskPoolInit(&work->tasks, 2);
    gFieldState = EwramAlloc(sizeof(FieldState));
    TaskCreate(&work->tasks, &gTaskDescFldShadow, actor);
    AddPooNode(&work->node, 1, actor);
    ColliderInit(&work->collider, COLLIDER_TYPE_PLAYER, 18, 48);
    ColliderSetPosition(&work->collider, actor->pos.x, actor->pos.y, actor->pos.z);
}

u8 PooSoraUpdateJump(PooSoraWork* work, Task* task) {
    PooActor* actor = &gPooActor;
    PooPos prevPos;
    s32 z;
    s32 sx;
    s32 sy;
    u16 jumpPressed;
    u16 steps;

    z = GetPooSoraGroundZ(work);
    sx = actor->pos.x;
    sy = actor->pos.y;
    UpdatePooActorAngle(actor);

    switch (work->state) {
    case POO_SORA_STATE_AIR_ATTACK:
        if (work->timer == 0) {
            SetPooSoraAnimation(work, 10, 0);
        }

        actor->pos.x += gSineTable[actor->angle] * actor->speed >> 8;
        actor->pos.y += -gSineTable[actor->angle + 0x40] * actor->speed >> 8;

        if (AnimGetFrame(&work->anim) > 3) {
            actor->pos.z += work->vz;
            work->vz += 66;

            if (actor->pos.z > z) {
                actor->pos.z = z;
                work->vz = 0;
            }
        } else {
            work->vz = 0;
        }

        actor->speed -= 38;

        if (actor->speed < 0) {
            actor->speed = 0;
        }

        switch (AnimGetFrame(&work->anim)) {
        case 3:
        case 4:
            SetPooSoraAttackPoint(actor);
            break;
        }

        if (AnimIsFinished(&work->anim)) {
            if (work->vz < 0) {
                work->state = POO_SORA_STATE_JUMP_RISE;
            } else {
                work->state = POO_SORA_STATE_JUMP_FALL;
            }
        } else {
            work->timer++;
        }

        break;
    case POO_SORA_STATE_JUMP_START:
        if (work->timer == 0) {
            SetPooSoraAnimation(work, 3, 0);
            actor->speed >>= 1;
        }

        actor->pos.x += gSineTable[actor->angle] * actor->speed >> 8;
        actor->pos.y += -gSineTable[actor->angle + 0x40] * actor->speed >> 8;

        if (work->timer > 3) {
            if (GetRandom() % 2 != 0) {
                m4aSongNumStart(SONG_SYS_SR_I_VO00);
            } else {
                m4aSongNumStart(SONG_SYS_SR_I_VO01);
            }

            work->state = POO_SORA_STATE_JUMP_RISE;
            work->vz = -0x533;
            actor->speed <<= 1;
            work->timer = 0;
        } else {
            work->timer++;
        }

        break;
    case POO_SORA_STATE_JUMP_RISE:
        if ((GetKeysHeld() & DPAD_ANY) != 0) {
            actor->speed += 17;

            if (actor->speed > 0x200) {
                actor->speed = 0x200;
            }
        } else {
            actor->speed -= 38;

            if (actor->speed < 0) {
                actor->speed = 0;
            }
        }

        if (work->vz > -0x200) {
            SetPooSoraAnimation(work, 5, 0);
        } else {
            SetPooSoraAnimation(work, 4, 0);
        }

        actor->pos.x += gSineTable[actor->angle] * actor->speed >> 8;
        actor->pos.y += -gSineTable[actor->angle + 0x40] * actor->speed >> 8;
        actor->pos.z += work->vz;
        work->vz += 66;

        if (work->vz < 0) {
            if ((GetKeysHeld() & B_BUTTON) == 0) {
                work->vz += 64;
            }
        }

        if ((GetKeysPressed() & A_BUTTON) != 0) {
            work->timer = 0;
            work->state = POO_SORA_STATE_AIR_ATTACK;
        } else if (work->vz > 0) {
            work->timer = 0;
            work->state = POO_SORA_STATE_JUMP_FALL;
        } else {
            work->timer++;
        }

        break;
    case POO_SORA_STATE_JUMP_FALL:
        if ((GetKeysHeld() & DPAD_ANY) != 0) {
            actor->speed += 17;

            if (actor->speed > 0x200) {
                actor->speed = 0x200;
            }
        } else {
            actor->speed -= 38;

            if (actor->speed < 0) {
                actor->speed = 0;
            }
        }

        if (work->vz < 0x200) {
            SetPooSoraAnimation(work, 5, 0);
        } else {
            SetPooSoraAnimation(work, 6, 0);
        }

        actor->pos.x += gSineTable[actor->angle] * actor->speed >> 8;
        actor->pos.y += -gSineTable[actor->angle + 0x40] * actor->speed >> 8;
        actor->pos.z += work->vz;
        work->vz += 66;

        if ((GetKeysPressed() & A_BUTTON) != 0) {
            work->timer = 0;
            work->state = POO_SORA_STATE_AIR_ATTACK;
        } else if (actor->pos.z > z) {
            actor->pos.z = z;
            work->vz = 0;

            if (work->state != POO_SORA_STATE_JUMP_LAND) {
                work->state = POO_SORA_STATE_JUMP_LAND;
                work->timer = 0;
            }
        }

        break;
    case POO_SORA_STATE_JUMP_LAND:
        if (work->timer == 0) {
            SetPooSoraAnimation(work, 7, 0);
            m4aSongNumStart(work->sounds[3]);
        }

        actor->speed = 0;
        jumpPressed = GetKeysPressed() & B_BUTTON;

        if (jumpPressed != 0) {
            work->timer = 0;
            work->state = POO_SORA_STATE_JUMP_START;
        } else if (work->timer > 6) {
            work->state = POO_SORA_STATE_STAND;
            work->timer = 0;
            SetTaskUpdate(task, (TaskUpdateFunc)task_poo_sora_1);
        } else {
            work->timer++;
        }

        break;
    }

    if (ApplyPooSoraPushOut(work, &actor->pos)) {
        actor->speed = actor->speed * 230 >> 8;
    }

    ConstrainPooActorMove(actor, sx, sy);

    if ((u8)IsInPooWagonArea(&actor->pos)) {
        if (actor->pos.z > -0xA00) {
            prevPos.x = sx;
            prevPos.y = sy;

            if (!(u8)IsInPooWagonArea(&prevPos)) {
                actor->pos.x = sx;
                actor->pos.y = sy;
                actor->speed = 0;
            } else if (work->vz >= 0) {
                actor->speed = 0;
                steps = (-actor->pos.z >> 8) + 1;
                SnapToPooWagonLine((u32*)&actor->pos, (u32*)&actor->pos.y, steps);
            }
        }
    }

    ColliderSetPosition(&work->collider, actor->pos.x, actor->pos.y, actor->pos.z);
    SetPooCameraFocus(actor->pos.x, actor->pos.y + actor->pos.z);
    work->gfx = AnimUpdate(&work->anim);
    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 PooSoraUpdateAttack(PooSoraWork* work, Task* task) {
    PooActor* actor = &gPooActor;
    s32 sx;
    s32 sy;

    sx = actor->pos.x;
    sy = actor->pos.y;

    if (work->state == POO_SORA_STATE_ATTACK) {
        if (work->timer == 0) {
            SetPooSoraAnimation(work, 9, 0);
            actor->speed = 0;
            m4aSongNumStart(SONG_SYS_SR_AT_VO00);
        }

        if (work->anim.timer == 0) {
            switch (actor->angle) {
            case FLD_ANGLE_DOWN_LEFT:
                switch (AnimGetFrame(&work->anim)) {
                case 0:
                    actor->pos.x -= 0x500;
                    actor->pos.y += 0x400;
                    break;
                case 1:
                    actor->pos.x -= 0x200;
                    break;
                case 2:
                    actor->pos.x -= 0x300;
                    break;
                }

                break;
            case FLD_ANGLE_DOWN_RIGHT:
                switch (AnimGetFrame(&work->anim)) {
                case 0:
                    actor->pos.x += 0x500;
                    actor->pos.y += 0x400;
                    break;
                case 1:
                    actor->pos.x += 0x200;
                    break;
                case 2:
                    actor->pos.x += 0x300;
                    break;
                }

                break;
            case FLD_ANGLE_UP_LEFT:
                switch (AnimGetFrame(&work->anim)) {
                case 0:
                    actor->pos.x -= 0x500;
                    actor->pos.y -= 0x200;
                    break;
                case 1:
                    actor->pos.x -= 0x500;
                    break;
                case 2:
                    actor->pos.x -= 0x200;
                    break;
                }

                break;
            case FLD_ANGLE_UP_RIGHT:
                switch (AnimGetFrame(&work->anim)) {
                case 0:
                    actor->pos.x += 0x500;
                    actor->pos.y -= 0x200;
                    break;
                case 1:
                    actor->pos.x += 0x500;
                    break;
                case 2:
                    actor->pos.x += 0x200;
                    break;
                }

                break;
            case FLD_ANGLE_DOWN:
                switch (AnimGetFrame(&work->anim)) {
                case 0:
                    actor->pos.x -= 0x300;
                    actor->pos.y += 0x400;
                    break;
                case 1:
                    actor->pos.x += 0x100;
                    actor->pos.y += 0x100;
                    break;
                case 2:
                    actor->pos.y += 0x200;
                    break;
                case 3:
                    actor->pos.y += 0x100;
                    break;
                }

                break;
            case FLD_ANGLE_RIGHT:
                switch (AnimGetFrame(&work->anim)) {
                case 0:
                    actor->pos.x += 0x700;
                    actor->pos.y += 0x100;
                    break;
                case 1:
                    actor->pos.x += 0x300;
                    break;
                case 2:
                    actor->pos.x += 0x200;
                    break;
                }

                break;
            case FLD_ANGLE_LEFT:
                switch (AnimGetFrame(&work->anim)) {
                case 0:
                    actor->pos.x -= 0x700;
                    actor->pos.y += 0x100;
                    break;
                case 1:
                    actor->pos.x -= 0x300;
                    break;
                case 2:
                    actor->pos.x -= 0x200;
                    break;
                }

                break;
            case FLD_ANGLE_UP:
                switch (AnimGetFrame(&work->anim)) {
                case 0:
                    actor->pos.y -= 0x400;
                    break;
                case 1:
                    actor->pos.y -= 0x400;
                    break;
                case 2:
                    actor->pos.x -= 0x100;
                    actor->pos.y += 0x100;
                    break;
                case 3:
                    actor->pos.y -= 0x100;
                    break;
                }

                break;
            }
        }

        if (work->timer > 14) {
            SetPooSoraAttackPoint(actor);
        }

        if (AnimIsFinished(&work->anim)) {
            switch (actor->angle) {
            case FLD_ANGLE_DOWN_LEFT:
                actor->pos.x -= 0x200;
                actor->pos.y += 0x200;
                break;
            case FLD_ANGLE_DOWN_RIGHT:
                actor->pos.x += 0x200;
                actor->pos.y += 0x200;
                break;
            case FLD_ANGLE_UP_LEFT:
            case FLD_ANGLE_UP_RIGHT:
                actor->pos.y -= 0x400;
                break;
            case FLD_ANGLE_DOWN:
                actor->pos.y += 0x200;
                break;
            case FLD_ANGLE_UP:
                actor->pos.y -= 0x200;
                break;
            }

            SetPooSoraAnimation(work, 0, 0);
            work->state = POO_SORA_STATE_STAND;
            SetTaskUpdate(task, (TaskUpdateFunc)task_poo_sora_1);
        } else {
            work->timer++;
        }
    }

    ApplyPooSoraPushOut(work, &actor->pos);
    ConstrainPooActorMove(actor, sx, sy);

    if ((u8)IsInPooWagonArea(&actor->pos)) {
        actor->pos.x = sx;
        actor->pos.y = sy;
        actor->speed = 0;
    }

    ColliderSetPosition(&work->collider, actor->pos.x, actor->pos.y, actor->pos.z);
    SetPooCameraFocus(actor->pos.x, actor->pos.y + actor->pos.z);
    work->gfx = AnimUpdate(&work->anim);
    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 PooSoraUpdateCall(PooSoraWork* work, Task* task) {
    PooActor* actor = &gPooActor;
    s32 x;
    s32 y;
    u16 keys;

    x = actor->pos.x;
    y = actor->pos.y;

    if (work->state == POO_SORA_STATE_CALL) {
        if (work->timer == 0) {
            SetPooSoraAnimation(work, 8, 1);
            actor->speed = 0;
        }

        if (work->timer > 29) {
            keys = GetKeysHeld() & R_BUTTON;

            if (keys != 0) {
                work->timer = 0;
            } else {
                work->timer = 0;
                work->state = POO_SORA_STATE_STAND;
                SetTaskUpdate(task, (TaskUpdateFunc)task_poo_sora_1);
            }
        } else {
            work->timer++;
        }
    }

    ApplyPooSoraPushOut(work, &actor->pos);
    ConstrainPooActorMove(actor, x, y);

    if ((u8)IsInPooWagonArea(&actor->pos)) {
        actor->pos.x = x;
        actor->pos.y = y;
        actor->speed = 0;
    }

    ColliderSetPosition(&work->collider, actor->pos.x, actor->pos.y, actor->pos.z);
    SetPooCameraFocus(actor->pos.x, actor->pos.y + actor->pos.z);
    work->gfx = AnimUpdate(&work->anim);
    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 task_poo_sora_1(PooSoraWork* work, Task* task) {
    PooActor* actor = &gPooActor;
    s32 z;
    s32 sx;
    s32 sy;
    u16 message;

    z = GetPooSoraGroundZ(work);
    sx = actor->pos.x;
    sy = actor->pos.y;

    if (work->state <= POO_SORA_STATE_WALK) {
        UpdatePooActorAngle(actor);

        if ((GetKeysHeld() & DPAD_ANY) != 0) {
            actor->speed += 128;
            SetPooSoraAnimation(work, 2, 1);

            if (actor->speed > 0x266) {
                actor->speed = 0x266;
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
            SetPooSoraAnimation(work, 0, 1);
            actor->speed -= 128;

            if (actor->speed < 0) {
                actor->speed = 0;
            }
        }

        actor->pos.x += gSineTable[actor->angle] * actor->speed >> 8;
        actor->pos.y += -gSineTable[actor->angle + 0x40] * actor->speed >> 8;

        if ((GetKeysPressed() & B_BUTTON) != 0) {
            work->timer = 0;
            work->state = POO_SORA_STATE_JUMP_START;
            SetTaskUpdate(task, (TaskUpdateFunc)PooSoraUpdateJump);
            m4aSongNumStart(work->sounds[2]);
        } else if ((GetKeysPressed() & A_BUTTON) != 0) {
            SetPooSoraAttackPoint(actor);
            gPooAttackActive = FALSE;
            message = FindPoohInteractionMessage();

            if (message != CARD_MSG_COUNT) {
                OpenPoohModeMessage(message);
            } else {
                work->timer = 0;
                work->state = POO_SORA_STATE_ATTACK;
                SetTaskUpdate(task, (TaskUpdateFunc)PooSoraUpdateAttack);
            }
        } else if ((GetKeysPressed() & R_BUTTON) != 0) {
            work->timer = 0;
            work->state = POO_SORA_STATE_CALL;
            SetTaskUpdate(task, (TaskUpdateFunc)PooSoraUpdateCall);
            actor->angle = GetPooAngleToPooh(&actor->pos);
        }
    } else if (AnimIsFinished(&work->anim)) {
        work->state = POO_SORA_STATE_STAND;
    }

    if (CheckPooSoraExit(&actor->pos) != 0) {
        actor->speed = 0;
    }

    if (ApplyPooSoraPushOut(work, &actor->pos)) {
        actor->speed = actor->speed * 230 >> 8;
    }

    ConstrainPooActorMove(actor, sx, sy);

    if ((u8)IsInPooWagonArea(&actor->pos)) {
        actor->pos.x = sx;
        actor->pos.y = sy;
        actor->speed = 0;
    }

    if (z != actor->pos.z) {
        actor->speed >>= 2;
        work->vz = 0;
        work->timer = 0;
        work->state = POO_SORA_STATE_JUMP_FALL;
        SetTaskUpdate(task, (TaskUpdateFunc)PooSoraUpdateJump);
    }

    ColliderSetPosition(&work->collider, actor->pos.x, actor->pos.y, actor->pos.z);
    SetPooCameraFocus(actor->pos.x, actor->pos.y + actor->pos.z);
    work->gfx = AnimUpdate(&work->anim);
    TaskPoolUpdate(&work->tasks);
    return 1;
}

void task_poo_sora_2(PooSoraWork* work) {
    PooActor* actor = &gPooActor;
    s32 prio;
    s32 flipX;
    s32 onCollider;
    s16 x;
    s16 y;

    flipX = work->flags & POO_SORA_FLAG_FLIP_X;
    prio = SPRITE_PRIORITY(2);

    if (flipX != 0) {
        prio = SPRITE_PRIORITY(2) | SPRITE_FLAG_HFLIP;
    }

    onCollider = work->onCollider;

    if (onCollider != 0) {
        sPooSoraPriority = -0x1008 - (work->collider.platformY >> 8) * 4;

        if (work->collider.penetration <= work->collider.radius || work->collider.other->radius == 0x400) {
            if (work->collider.platformZ != 0) {
                actor->shadowPriority = 0;
            } else {
                actor->shadowPriority = sPooSoraPriority + 1;
            }

            actor->shadowZ = 0;
        } else {
            actor->shadowZ = work->collider.platformZ;
            actor->shadowPriority = sPooSoraPriority + 1;
        }
    } else {
        sPooSoraPriority = -0x1008 - (actor->pos.y >> 8) * 4;
        actor->shadowZ = 0;

        if (onCollider != actor->pos.ground) {
            actor->shadowPriority = 0;
        } else {
            actor->shadowPriority = sPooSoraPriority + 1;
        }
    }

    x = (actor->pos.x >> 8) - gPooScrollX;
    y = (actor->pos.y >> 8) + (actor->pos.z >> 8) - gPooScrollY;
    DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, prio, sPooSoraPriority - 1);
    TaskPoolDraw(&work->tasks);
}

void task_poo_sora_3(PooSoraWork* work) {
    SetPooStatePos2(&gPooActor.pos);
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ColliderUnregister(&work->collider);
    TaskPoolDestroy(&work->tasks);
    EwramFree(gFieldState);
    RemovePooNode(&work->node);
}

u8 IsPooSoraCallStarting() {
    if (sPooSoraWork->state == POO_SORA_STATE_CALL && sPooSoraWork->timer == 0) {
        return TRUE;
    }

    return FALSE;
}

u8 IsPooSoraCalling() {
    if (sPooSoraWork->state == POO_SORA_STATE_CALL) {
        return TRUE;
    }

    return FALSE;
}

u8 AreAllPooEventsDone() {
    s32 i;

    for (i = 0; i < 6; i++) {
        if (!IsPooEventDone(i)) {
            return FALSE;
        }
    }

    return TRUE;
}

u16 CheckPooSoraExit(PooPos* pos) {
    u16 exitId;

    exitId = GetPooExitAt(pos);
    ClearPooFlag(3);

    if (exitId == 1) {
        OpenPoohModeMessage(0xFFFE);
    } else if (exitId == 2) {
        SetJiminyFlag(JIMINY_RECORD_STORY_100_ACRE_WOOD);

        if (!IsPoohOffScreen()) {
            SetPooFlag(3);

            if (AreAllPooEventsDone()) {
                if (IsPooFlagSet(1)) {
                    ExitPoohMode(EVENT_147_100ACREWOOD_END_COMPCOMP);
                } else if (!IsPooFlagSet(0)) {
                    SetPooFlag(0);
                    SetPooFlag(1);
                    SetJiminyFlag(JIMINY_RECORD_CHARACTER_WINNIE_THE_POOH);
                    ExitPoohMode(EVENT_143_100ACREWOOD_END_1ST_COMP);
                } else {
                    SetPooFlag(1);
                    SetJiminyFlag(JIMINY_RECORD_CHARACTER_WINNIE_THE_POOH);
                    ExitPoohMode(EVENT_145_100ACREWOOD_END_COMP);
                }
            } else if (!IsPooFlagSet(0)) {
                SetPooFlag(0);
                SetJiminyFlag(JIMINY_RECORD_CHARACTER_WINNIE_THE_POOH);
                ExitPoohMode(EVENT_144_100ACREWOOD_END_1ST_NO);
            } else {
                ExitPoohMode(EVENT_146_100ACREWOOD_END_NO);
            }
        } else {
            OpenPoohModeMessage(0xFFFD);
        }
    }

    return exitId;
}

u16 GetPooSoraPriority() {
    return sPooSoraPriority - 1;
}

void task_poo_trap_0(PooTrapWork* work, PooPos* pos) {
    work->x = pos->x;
    work->y = pos->y;
    work->z = 0;
    work->tiles = LoadObjTiles(gPooTrapTiles, sizeof(gPooTrapTiles));
    work->palette = LoadObjPalette(gPooTrapPalette, sizeof(gPooTrapPalette));
    work->gfx = gPooTrapFrame0;
    ColliderSetPosition(&work->collider, work->x, work->y, work->z);
    work->colliderActive = FALSE;
}

u8 task_poo_trap_1(PooTrapWork* work) {
    if (work->colliderActive) {
        if (ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_POOH)) {
            gPoohRequestX = work->x;
            gPoohRequestY = work->y;
            gPoohRequest = POOH_REQUEST_TRAP;
        }
    }

    return 1;
}

void task_poo_trap_2(PooTrapWork* work) {
    s16 x;
    s16 y;

    x = (work->x >> 8) - gPooScrollX;
    y = (work->y >> 8) - gPooScrollY;

    if (x < -16 || x > 256 || y < -16 || y > 176) {
        if (work->colliderActive) {
            ColliderUnregister(&work->collider);
            work->colliderActive = FALSE;
        }
    } else {
        if (!work->colliderActive) {
            ColliderInit(&work->collider, COLLIDER_TYPE_POOH_OBJECT, 8, 16);
            work->colliderActive = TRUE;
        }

        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), 0xFFEF);
    }
}

void task_poo_trap_3(PooTrapWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);

    if (work->colliderActive) {
        ColliderUnregister(&work->collider);
    }
}

void task_poo_pitAndButterfly_0(PooTrapWork* work, PooPos* pos) {
    task_poo_trap_0(work, pos);
    TaskPoolInit(&work->tasks, 1);
    TaskCreate(&work->tasks, &gTaskDescPooButterfly, &work->x);
    AddPooNode(&work->node, 0xE10, &work->x);
}

u8 task_poo_pitAndButterfly_1(PooTrapWork* work) {
    task_poo_trap_1(work);

    if (work->colliderActive) {
        if (ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_POOH)) {
            SetPooNodeWeight(&work->node, 0);
        }
    }

    return 1;
}

void task_poo_pitAndButterfly_2(PooTrapWork* work) {
    task_poo_trap_2(work);

    if (work->colliderActive) {
        TaskPoolUpdate(&work->tasks);
        TaskPoolDraw(&work->tasks);
    }
}

void task_poo_pitAndButterfly_3(PooTrapWork* work) {
    task_poo_trap_3(work);
    TaskPoolDestroy(&work->tasks);
    RemovePooNode(&work->node);
}

void task_poo_balloon_0(PooBalloonObjWork* work, PooPos* pos) {
    work->pos = pos;

    if (pos->x == 0x3FD00 && pos->y == 0x21B00) {
        work->tiles = AllocObjTiles(GetMaxSpriteTileBytes(gPooBalloonFrames, ARRAY_COUNT(gPooBalloonFrames)), gPooBalloonTiles);
        work->palette = LoadObjPalette(gPooBalloonPalette, sizeof(gPooBalloonPalette));
        AnimInit(&work->anim, gPooBalloonAnims, gPooBalloonFrames);
        AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    } else {
        work->tiles = AllocObjTiles(GetMaxSpriteTileBytes(gTrap0006Frames, ARRAY_COUNT(gTrap0006Frames)), gTrap0006Tiles);
        work->palette = LoadObjPalette(gPooTrapBalloonPalette, sizeof(gPooTrapBalloonPalette));
        AnimInit(&work->anim, gTrap0006Anims, gTrap0006Frames);
        AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    }

    work->gfx = AnimGetGfx(&work->anim);
}

u8 task_poo_balloon_1(void* work) {
    return 1;
}

void task_poo_balloon_2(PooBalloonObjWork* work) {
    s16 x;
    s16 y;

    x = (work->pos->x >> 8) - gPooScrollX;
    y = (work->pos->y >> 8) + (work->pos->z >> 8) - gPooScrollY;

    if (x >= -16 && x <= 256 && y >= -16 && y <= 176) {
        work->gfx = AnimUpdate(&work->anim);
        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), -0x1004 - (work->pos->y >> 8) * 4);
    }
}

void task_poo_balloon_3(PooBalloonObjWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

void task_poo_shadow_0(TaskPool* pool, void* arg) {
    PooShadowArgs args;

    args.pos = arg;
    args.scale = Q_8_8(0.65);
    TaskPoolInit(pool, 1);
    TaskCreate(pool, &gTaskDescPooShadowscale, &args);
}

u8 task_poo_shadow_1(TaskPool* pool) {
    TaskPoolUpdate(pool);
    return 1;
}

void task_poo_shadow_2(TaskPool* pool) {
    TaskPoolDraw(pool);
}

void task_poo_shadow_3(TaskPool* pool) {
    TaskPoolDestroy(pool);
}

void task_poo_shadowdodai_0(PooShadowWork* work, PooShadowArgs* args) {
    work->pos = args->pos;
    work->shadowInfo = args->shadowInfo;
    work->x = work->pos->x;
    work->y = work->pos->y;
    work->tiles = LoadObjTiles(gBtlShadowTiles, sizeof(gBtlShadowTiles));
    work->palette = LoadObjPalette(gCommonObjPalette, sizeof(gCommonObjPalette));
    AnimInit(&work->anim, gBtlShadowAnims, gBtlShadowFrames);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimUpdate(&work->anim);
}

u8 task_poo_shadowdodai_1(PooShadowWork* work) {
    work->x = work->pos->x;
    work->y = work->pos->y;
    return 1;
}

void task_poo_shadowdodai_2(PooShadowWork* work) {
    s32 scale;
    ObjAffine* affine;
    s32 groundZ;
    s16 x;
    s16 y;

    if (work->shadowInfo->priority != 0) {
        groundZ = work->shadowInfo->z;

        if (work->pos->z >= groundZ) {
            scale = Q_8_8(0.65);
        } else {
            scale = Q_8_8(0.65) - (groundZ - work->pos->z) / 128;

            if (scale <= 0x18) {
                scale = Q_8_8(0.1);
            }
        }

        affine = AllocObjAffine(0, scale, scale, FALSE);
        x = (work->x >> 8) - gPooScrollX;
        y = (work->y >> 8) + (groundZ >> 8) - gPooScrollY;
        DrawSprite(x, y, work->gfx, work->tiles, work->palette, affine, SPRITE_PRIORITY(2), work->shadowInfo->priority);
    }
}

void task_poo_shadowdodai_3(PooShadowWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

void task_poo_shadowscale_0(PooScaleWork* work, PooShadowArgs* args) {
    work->pos = args->pos;
    work->x = work->pos->x;
    work->y = work->pos->y;
    work->scale = args->scale;
    work->tiles = LoadObjTiles(gBtlShadowTiles, sizeof(gBtlShadowTiles));
    work->palette = LoadObjPalette(gCommonObjPalette, sizeof(gCommonObjPalette));
    AnimInit(&work->anim, gBtlShadowAnims, gBtlShadowFrames);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimUpdate(&work->anim);
}

u8 task_poo_shadowscale_1(PooScaleWork* work) {
    work->x = work->pos->x;
    work->y = work->pos->y;
    return 1;
}

void task_poo_shadowscale_2(PooScaleWork* work) {
    s32 scale;
    ObjAffine* affine;
    u16 x;
    u16 y;

    if (work->pos->z >= 0) {
        scale = work->scale;
    } else {
        scale = work->scale + work->pos->z / 128;

        if (scale <= 0x18) {
            scale = Q_8_8(0.1);
        }
    }

    affine = AllocObjAffine(0, scale, scale, FALSE);
    x = (work->x >> 8) - gPooScrollX;
    y = (work->y >> 8) - gPooScrollY;
    DrawSprite(x, y, work->gfx, work->tiles, work->palette, affine, SPRITE_PRIORITY(2), 0xFFF0);
}

void task_poo_shadowscale_3(PooScaleWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

void CreatePooShadowscaleTask(void* pool, void* pos, s32 scale) {
    PooShadowArgs args;

    args.pos = pos;
    args.scale = scale;
    TaskCreate(pool, &gTaskDescPooShadowscale, &args);
}

void task_poo_freeballoon_0(PooFreeBalloonWork* work, PooPos* pos) {
    work->pos2 = *pos;
    work->pos4 = *pos;
    work->pos3 = *pos;
    work->pos5 = *pos;
    work->pos = pos;
    work->tiles = AllocObjTiles(GetMaxSpriteTileBytes(gTrap0004Frames, 4), gTrap0004Tiles);
    work->palette = LoadObjPalette(gTrap0004Palette, sizeof(gTrap0004Palette));
    AnimInit(&work->anim, gTrap0004Anims, gTrap0004Frames);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    work->tiles2 = AllocObjTiles(GetMaxSpriteTileBytes(gTrap0005Frames, 4), gTrap0005Tiles);
    work->palette2 = LoadObjPalette(gPooFreeBalloonPalette, sizeof(gPooFreeBalloonPalette));
    AnimInit(&work->anim2, gTrap0005Anims, gTrap0005Frames);
    AnimStart(&work->anim2, 0, ANIM_FLAG_LOOP);
    work->gfx2 = AnimGetGfx(&work->anim2);
    work->timer = 0;
}

u8 IsPooNearScreen(s16 x, s16 y) {
    if (x < -64 || x > 304) {
        return FALSE;
    }

    if (y < -64) {
        return FALSE;
    }

    if (y <= 224) {
        return TRUE;
    }

    return FALSE;
}

u8 task_poo_freeballoon_1(PooFreeBalloonWork* work) {
    u16 elapsed;

    work->timer++;

    if (work->timer > 5) {
        elapsed = work->timer - 5;
        work->pos4.x = work->pos5.x - elapsed * 256;
        work->pos4.y = work->pos5.y - ((elapsed * elapsed) << 8) / 32;
    } else {
        work->pos4 = *work->pos;
        work->pos5 = *work->pos;
    }

    work->pos2.x = work->pos3.x + work->timer * 256;
    work->pos2.y = work->pos3.y - ((work->timer * work->timer) << 8) / 32;
    work->x2 = (work->pos4.x >> 8) - gPooScrollX;
    work->y2 = (work->pos4.y >> 8) + (work->pos4.z >> 8) - gPooScrollY;
    work->x = (work->pos2.x >> 8) - gPooScrollX;
    work->y = (work->pos2.y >> 8) + (work->pos2.z >> 8) - gPooScrollY;

    if (IsPooNearScreen(work->x2, work->y2)) {
        work->gfx2 = AnimUpdate(&work->anim2);
    } else {
        work->gfx2 = NULL;
    }

    if (IsPooNearScreen(work->x, work->y)) {
        work->gfx = AnimUpdate(&work->anim);
    } else {
        work->gfx = NULL;
    }

    if (work->gfx == NULL && work->gfx2 == NULL) {
        return 0;
    }

    return 1;
}

void task_poo_freeballoon_2(PooFreeBalloonWork* work) {
    if (work->gfx2 != NULL) {
        DrawSprite(work->x2, work->y2, work->gfx2, work->tiles2, work->palette2, NULL, SPRITE_PRIORITY(2), -0x1004 - (work->pos4.y >> 8) * 4);
    }

    if (work->gfx != NULL) {
        DrawSprite(work->x, work->y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), -0x1004 - (work->pos2.y >> 8) * 4);
    }
}

void task_poo_freeballoon_3(PooFreeBalloonWork* work) {
    ReleaseObjTiles(work->tiles2);
    ReleaseObjPalette(work->palette2);
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

s32 GetPooGaugeFrame(u16 timer) {
    s32 frame;
    u32 blinkTime;
    u8 blinkOn;

    blinkTime = timer;
    frame = 3 - gPoohGauge;

    if (gPoohGaugeTimer <= 0x1CD) {
        blinkOn = (blinkTime / 20) & 1;

        if (blinkOn != 0) {
            if (frame <= 2) {
                frame++;
            }
        }
    }

    return frame;
}

void task_poo_gauge_0(PooGaugeWork* work) {
    work->blinkTimer = 0;
    work->tiles = AllocObjTiles(GetMaxSpriteTileBytes(gPooGaugeFrames, 4), gPooGaugeTiles);
    work->palette = LoadObjPalette(gPoohGaugePalettes[0], sizeof(gPoohGaugePalettes[0]));
    work->paletteSrc = gPoohGaugePalettes[0];
    work->gfx = gPooGaugeFrames[GetPooGaugeFrame(work->blinkTimer)];
    work->warning = FALSE;
}

u8 task_poo_gauge_1(PooGaugeWork* work) {
    work->blinkTimer++;
    work->gfx = gPooGaugeFrames[GetPooGaugeFrame(work->blinkTimer)];

    if (gPoohGauge <= 1 && gPoohGaugeTimer <= 0x1CD) {
        work->warning = TRUE;
    } else {
        work->warning = FALSE;
    }

    if (work->warning) {
        if (work->paletteSrc != gPoohGaugePalettes[1]) {
            LoadObjPaletteBank(work->palette->index, gPoohGaugePalettes[1]);
            work->paletteSrc = gPoohGaugePalettes[1];
        }
    }

    if (!work->warning) {
        if (work->paletteSrc != gPoohGaugePalettes[0]) {
            LoadObjPaletteBank(work->palette->index, gPoohGaugePalettes[0]);
            work->paletteSrc = gPoohGaugePalettes[0];
        }
    }

    return 1;
}

void task_poo_gauge_2(PooGaugeWork* work) {
    DrawSprite(0xDC, 0x18, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1), 0);
}

void task_poo_gauge_3(PooGaugeWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

void task_poo_trapballoon_0(PooBalloonWork* work, PooPos* pos) {
    work->pos = *pos;
    work->pos.z = 0;
    work->pos.ground = 0;
    work->tileBytes = GetMaxSpriteTileBytes(gTrap0006Frames, ARRAY_COUNT(gTrap0006Frames));
    work->palette = NULL;
    AnimInit(&work->anim, gTrap0006Anims, gTrap0006Frames);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    TaskPoolInit(&work->tasks, 3);
    work->task = TaskCreate(&work->tasks, &gTaskDescPooShadow, &work->pos);
    work->freeBalloonTask = NULL;
    work->angle = GetRandom();
    ColliderSetPosition(&work->collider, work->pos.x, work->pos.y, work->pos.z);
}

u8 task_poo_trapballoon_1(PooBalloonWork* work) {
    PooPos sparkPos;

    if (work->palette == NULL) {
        return 1;
    }

    if (IsTaskActive(work->freeBalloonTask)) {
        return 1;
    }

    if (!IsTaskActive(work->task) && !IsTaskActive(work->freeBalloonTask)) {
        return 0;
    }

    if (ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_POOH) && IsPoohWalkingToTarget()) {
        gPoohRequestX = work->pos.x;
        gPoohRequestY = work->pos.y;
        gPoohRequest = POOH_REQUEST_TRAP_BALLOON;
        SetPooNodeWeight(&work->node, 0);
        return 0;
    }

    if (!gPooAttackActive) {
        if (!ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_POOH)) {
            return 1;
        }

        if (IsPoohWalkingToTarget()) {
            return 1;
        }
    }

    if (!PooAttackHitsCollider(&work->collider)) {
        return 1;
    }

    sparkPos = work->pos;
    sparkPos.z -= 0x1000;
    TaskCreate(&work->tasks, &gTaskDescPooSpark, &sparkPos);
    SetPooNodeWeight(&work->node, 0);
    ColliderSetDisabled(&work->collider, TRUE);
    TaskKill(&work->tasks, work->task);
    work->pos.x -= 0x800;
    work->pos.y += 0x1000;
    work->freeBalloonTask = TaskCreate(&work->tasks, &gTaskDescPooFreeballoon, &work->pos);
    m4aSongNumStart(SONG_SYS_PO_BLOON);
    return 1;
}

void task_poo_trapballoon_2(PooBalloonWork* work) {
    s32 bobY;
    s16 x;
    s16 y;

    if (IsTaskActive(work->freeBalloonTask)) {
        TaskPoolUpdate(&work->tasks);
        TaskPoolDraw(&work->tasks);
    } else {
        work->gfx = AnimUpdate(&work->anim);
        work->angle += 2;
        bobY = SIN(work->angle) * 2;
        x = ((work->pos.x - 0x800) >> 8) - gPooScrollX;
        bobY += 0x1200;
        y = ((work->pos.y + bobY) >> 8) + (work->pos.z >> 8) - gPooScrollY;

        if (IsRectOutsideScreen(x, y, 64, 8, 24, 24)) {
            if (work->palette != NULL) {
                ReleaseObjTiles(work->tiles);
                ReleaseObjPalette(work->palette);
                work->palette = NULL;
                ColliderUnregister(&work->collider);
                RemovePooNode(&work->node);
            }
        } else {
            if (work->palette == NULL) {
                work->tiles = AllocObjTiles(work->tileBytes, gTrap0006Tiles);
                work->palette = LoadObjPalette(gPooTrapBalloonPalette, sizeof(gPooTrapBalloonPalette));
                ColliderInit(&work->collider, COLLIDER_TYPE_POOH_OBJECT, 8, 16);
                AddPooNode(&work->node, 0x400, &work->pos);
            }

            DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), -0x1004 - (work->pos.y >> 8) * 4);
            TaskPoolUpdate(&work->tasks);
            TaskPoolDraw(&work->tasks);
        }
    }
}

void task_poo_trapballoon_3(PooBalloonWork* work) {
    if (work->palette != NULL) {
        ReleaseObjTiles(work->tiles);
        ReleaseObjPalette(work->palette);
        ColliderUnregister(&work->collider);
        RemovePooNode(&work->node);
    }

    TaskPoolDestroy(&work->tasks);
}

void task_poo_owlballoon_0(PooOwlBalloonWork* work, PooPos* pos) {
    work->pos = *pos;
    work->pos.z = 0;
    work->pos.ground = 0;
    work->tileBytes = GetMaxSpriteTileBytes(gPooBalloonFrames, ARRAY_COUNT(gPooBalloonFrames));
    work->palette = NULL;
    AnimInit(&work->anim, gPooBalloonAnims, gPooBalloonFrames);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    TaskPoolInit(&work->tasks, 2);
    work->task = TaskCreate(&work->tasks, &gTaskDescPooShadow, &work->pos);
    ColliderSetPosition(&work->collider, work->pos.x, work->pos.y, work->pos.z);
    AddPooNode(&work->node, 0x240, &work->pos);
}

u8 task_poo_owlballoon_1(PooOwlBalloonWork* work) {
    if (work->palette != NULL && ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_POOH) && IsPoohWalkingToTarget()) {
        gPoohRequestX = work->pos.x;
        gPoohRequestY = work->pos.y;
        gPoohRequest = POOH_REQUEST_OWL_BALLOON;
        SetPooNodeWeight(&work->node, 0);
        m4aSongNumStart(SONG_SND_385);
        return 0;
    }

    return 1;
}

void task_poo_owlballoon_2(PooOwlBalloonWork* work) {
    s16 x;
    s16 y;

    work->gfx = AnimUpdate(&work->anim);
    x = ((work->pos.x - 0x800) >> 8) - gPooScrollX;
    y = ((work->pos.y + 0x1000) >> 8) - gPooScrollY;

    if (IsRectOutsideScreen(x, y, 64, 8, 24, 24)) {
        if (work->palette != NULL) {
            ReleaseObjTiles(work->tiles);
            ReleaseObjPalette(work->palette);
            work->palette = NULL;
            ColliderUnregister(&work->collider);
        }
    } else {
        if (work->palette == NULL) {
            work->tiles = AllocObjTiles(work->tileBytes, gPooBalloonTiles);
            work->palette = LoadObjPalette(gPooBalloonPalette, sizeof(gPooBalloonPalette));
            ColliderInit(&work->collider, COLLIDER_TYPE_POOH_OBJECT, 8, 16);
        }

        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), -0x1003 - (work->pos.y >> 8) * 4);
        TaskPoolUpdate(&work->tasks);
        TaskPoolDraw(&work->tasks);
    }
}

void task_poo_owlballoon_3(PooOwlBalloonWork* work) {
    if (work->palette != NULL) {
        ReleaseObjTiles(work->tiles);
        ReleaseObjPalette(work->palette);
        ColliderUnregister(&work->collider);
    }

    TaskPoolDestroy(&work->tasks);
    RemovePooNode(&work->node);
}

u16 GetPooNodeWeight(PooNode* node) {
    return node->weight;
}

void SetPooNodeWeight(PooNode* node, u16 weight) {
    node->weight = weight;
}

u16 GetPooNodeBaseWeight(PooNode* node) {
    return node->baseWeight;
}

void SetPooNodeBaseWeight(PooNode* node, u16 weight) {
    node->baseWeight = weight;
}

void AddPooNode(PooNode* node, u16 weight, void* pos) {
    SetPooNodeWeight(node, weight);
    SetPooNodeBaseWeight(node, weight);
    node->pos = pos;
    node->unk_04 = 0;
    ListNodeInit(&node->node, &sPooNodes, node);
    ListPoolAppend(&node->node, &sPooNodes);
}

void RemovePooNode(PooNode* node) {
    ListPoolRemove(&node->node, &sPooNodes);
}

void InitPooNodes() {
    ListPoolInit(&sPooNodes);
}

s32 GetPooNodeScore(PooNode* node) {
    PooPos* poohPos;
    PooPos* nodePos;
    s16 dx;
    s16 dy;
    u16 weight;
    s32 distSq;

    poohPos = gPoohPos;
    nodePos = node->pos;
    dx = (poohPos->x - nodePos->x) >> 8;
    dy = (poohPos->y - nodePos->y) >> 8;
    weight = GetPooNodeWeight(node);

    if (dx * dx > 0x3840 && dy * dy > 0x1900) {
        return 0;
    }

    distSq = dx * dx + dy * dy;

    if (distSq == 0) {
        return weight << 8;
    }

    if (weight != 0 && (weight << 8) / (distSq << 8) == 0) {
        return 1;
    }

    return (weight << 8) / ((dx * dx + dy * dy) << 8);
}

PooNode* FindPoohTargetNode() {
    PooNode* best;
    PooNode* node;

    best = ListPoolFirst(&sPooNodes);
    node = best;
    sPooBestNodeScore = 0;

    while (node != NULL) {
        sPooNodeScore = GetPooNodeScore(node);

        if (sPooNodeScore > sPooBestNodeScore) {
            sPooBestNodeScore = sPooNodeScore;
            best = node;
        }

        node = ListPoolNext(&node->node);
    }

    if (sPooBestNodeScore == 0 && best == (PooNode*)ListPoolFirst(&sPooNodes)) {
        return NULL;
    }

    return best;
}

enum PooHoneyState {
    POO_HONEY_STATE_WAIT_POOH,
    POO_HONEY_STATE_WAIT_EAT,
    POO_HONEY_STATE_EATEN
};

void task_poo_honey_0(PooHoneyWork* work, PooPos* pos) {
    work->pos.x = pos->x;
    work->pos.y = pos->y;
    work->pos.z = 0;
    work->palette = NULL;
    work->tileBytes = GetMaxSpriteTileBytes(gPoohHoneyFrames, ARRAY_COUNT(gPoohHoneyFrames));
    AnimInit(&work->anim, gPoohHoneyAnims, gPoohHoneyFrames);
    AnimStart(&work->anim, 3, ANIM_FLAG_LOOP);
    ColliderSetPosition(&work->collider, work->pos.x, work->pos.y, work->pos.z);
    work->pos2 = work->pos;
    work->pos2.x += 0xB00;
    work->pos2.y -= 0xA00;
    work->minPos = work->pos2;
    work->minPos.x -= 0x100;
    work->minPos.y -= 0x100;
    work->maxPos = work->pos2;
    work->maxPos.x += 0x100;
    work->maxPos.y += 0x100;
    TaskPoolInit(&work->tasks, 1);
    CreatePooShadowscaleTask(&work->tasks, &work->pos, Q_8_8(0.8));
    work->state = POO_HONEY_STATE_WAIT_POOH;
    work->timer = 0;
}

u8 task_poo_honey_1(PooHoneyWork* work) {
    switch (work->state) {
    case POO_HONEY_STATE_WAIT_POOH:
        if (work->minPos.x <= gPoohPos->x && gPoohPos->x <= work->maxPos.x && work->minPos.y <= gPoohPos->y && gPoohPos->y <= work->maxPos.y) {
            gPoohRequest = POOH_REQUEST_HONEY;
            SetPooNodeWeight(&work->node, 0);
            work->state++;
        }

        break;
    case POO_HONEY_STATE_WAIT_EAT:
        if (GetPoohHoneyAnim() <= 2) {
            AnimStart(&work->anim, GetPoohHoneyAnim(), 0);
            AnimUpdate(&work->anim);
            work->state++;
        }

        break;
    case POO_HONEY_STATE_EATEN:
        AnimUpdate(&work->anim);

        if (AnimIsFinished(&work->anim)) {
            return 0;
        }

        switch (AnimGetGfxIndex(&work->anim)) {
        case 1:
            work->pos = *gPoohPos;
            work->pos.x -= 0xB00;
            work->pos.y += 0xA00;
            work->pos.z = 0;
            work->pos3 = work->pos;
            break;
        case 2:
        case 3:
        case 4:
        case 5:
        case 6:
            work->pos.x = work->pos3.x + 0x200;
            work->pos.y = work->pos3.y;
            work->pos.z = work->pos3.z - 0x200;
            break;
        case 7:
            work->pos.x = work->pos3.x - 0x200;
            work->pos.y = work->pos3.y - 0x500;
            work->pos.z = work->pos3.z - 0xB00;
            break;
        case 8:
            gPoohGauge = 3;
            gPoohGaugeTimer = 0x73B;
            work->pos.x = work->pos3.x + 0xA00;
            work->pos.y = work->pos3.y - 0x900;
            work->pos.z = work->pos3.z - 0x1000;
            break;
        case 9:
            work->pos.x = work->pos3.x + 0x200;
            work->pos.y = work->pos3.y;
            work->pos.z = work->pos3.z - 0x300;
            break;
        case 10:
            work->pos.x = work->pos3.x + 0x200;
            work->pos.y = work->pos3.y;
            work->pos.z = work->pos3.z - 0x100;
            break;
        case 11:
        case 12:
        case 13:
            work->pos.x = work->pos3.x;
            work->pos.y = work->pos3.y;
            work->pos.z = work->pos3.z;
            work->timer++;
            break;
        case 0:
        default:
            break;
        }

        break;
    default:
        break;
    }

    return 1;
}

void task_poo_honey_2(PooHoneyWork* work) {
    s16 x;
    s16 y;

    if (work->timer > 29 && (work->timer & 1) != 0) {
        return;
    }

    x = (work->pos.x >> 8) - gPooScrollX;
    y = (work->pos.y >> 8) + (work->pos.z >> 8) - gPooScrollY;

    if (IsRectOutsideScreen(x, y, 24, 8, 16, 16)) {
        if (work->palette != NULL) {
            ReleaseObjTiles(work->tiles);
            ReleaseObjPalette(work->palette);
            work->palette = NULL;
            ColliderUnregister(&work->collider);
            RemovePooNode(&work->node);
        }
    } else {
        if (work->palette == NULL) {
            work->tiles = AllocObjTiles(work->tileBytes, gPoohHoneyTiles);
            work->palette = LoadObjPalette(gPoohGaugePalettes[0], sizeof(gPoohGaugePalettes[0]));
            ColliderInit(&work->collider, COLLIDER_TYPE_POOH_OBJECT, 8, 16);
            AddPooNode(&work->node, 0x1FA4, &work->pos2);
        }

        DrawSprite(x, y, AnimGetGfx(&work->anim), work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), -0x1004 - (work->pos.y >> 8) * 4);
        TaskPoolUpdate(&work->tasks);
        TaskPoolDraw(&work->tasks);
    }
}

void task_poo_honey_3(PooHoneyWork* work) {
    if (work->palette != NULL) {
        ReleaseObjTiles(work->tiles);
        ReleaseObjPalette(work->palette);
        ColliderUnregister(&work->collider);
        RemovePooNode(&work->node);
    }

    TaskPoolDestroy(&work->tasks);
}

void task_poo_mapanime_0(PooMapAnimeWork* work) {
    BosMapanimeInit(&work->anims[0], &gPooMapanimeDef0);
    BosMapanimeInit(&work->anims[1], &gPooMapanimeDef1);
}

u8 task_poo_mapanime_1(PooMapAnimeWork* work) {
    u8 defer;
    u32 i;

    defer = FALSE;

    for (i = 0; i < ARRAY_COUNT(work->anims); i++) {
        defer = BosMapanimeUpdate(&work->anims[i], work->anims[i].def, defer);
    }

    return 1;
}

void task_poo_mapanime_2(void* work) {
}

void task_poo_mapanime_3(void* work) {
}

enum PooPileStage {
    POO_PILE_STAGE_HEIGHT_4,
    POO_PILE_STAGE_HIT_TO_3,
    POO_PILE_STAGE_HEIGHT_3,
    POO_PILE_STAGE_HIT_TO_2,
    POO_PILE_STAGE_HEIGHT_2,
    POO_PILE_STAGE_HIT_TO_1,
    POO_PILE_STAGE_HEIGHT_1,
    POO_PILE_STAGE_FLAT,
    POO_PILE_STAGE_RANDOM
};

s32 GetRandomPooPileStage() {
    switch (GetRandom() % 40 / 10) {
    case 0:
        return POO_PILE_STAGE_HEIGHT_4;
    case 1:
        return POO_PILE_STAGE_HEIGHT_3;
    case 2:
        return POO_PILE_STAGE_HEIGHT_2;
    }

    return POO_PILE_STAGE_HEIGHT_1;
}

s32 NextPooPileStage(u32 stage) {
    if (stage == POO_PILE_STAGE_HEIGHT_4) {
        return POO_PILE_STAGE_HIT_TO_3;
    }

    if (stage <= POO_PILE_STAGE_HEIGHT_3) {
        return POO_PILE_STAGE_HIT_TO_2;
    }

    if (stage <= POO_PILE_STAGE_HEIGHT_2) {
        return POO_PILE_STAGE_HIT_TO_1;
    }

    return POO_PILE_STAGE_FLAT;
}

s32 GetPooPileHeight(u32 stage) {
    if (stage == POO_PILE_STAGE_HEIGHT_4) {
        return 32;
    }

    if (stage <= POO_PILE_STAGE_HEIGHT_3) {
        return 24;
    }

    if (stage <= POO_PILE_STAGE_HEIGHT_2) {
        return 16;
    }

    return 9;
}

void task_poo_pile_0(PooPileWork* work, PooPileArgs* args) {
    work->pos.x = args->x;
    work->pos.y = args->y;
    work->pos.z = 0;
    work->palette = NULL;
    AnimInit(&work->anim, gPooPileAnims, gPooPileFrames);

    if (args->stage == POO_PILE_STAGE_RANDOM) {
        work->stage = GetRandomPooPileStage();
    } else {
        work->stage = args->stage;
    }

    AnimStart(&work->anim, work->stage, 0);
    work->gfx = AnimGetGfx(&work->anim);
    ColliderSetPosition(&work->collider, work->pos.x, work->pos.y, work->pos.z);
    work->colliderActive = FALSE;
    TaskPoolInit(&work->tasks, 1);
    work->task = NULL;
}

u8 task_poo_pile_1(PooPileWork* work) {
    PooPos sparkPos;

    if (work->stage == POO_PILE_STAGE_FLAT) {
        return 1;
    }

    if (!work->colliderActive) {
        return 1;
    }

    if (ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_POOH)) {
        gPoohRequest = POOH_REQUEST_BLOCKED;
    }

    if (!gPooAttackActive) {
        return 1;
    }

    if (!PooAttackHitsCollider(&work->collider)) {
        return 1;
    }

    if (!AnimIsFinished(&work->anim)) {
        return 1;
    }

    sparkPos = work->pos;
    sparkPos.z -= (u16)GetPooPileHeight(work->stage) * 256;

    if (IsTaskActive(work->task)) {
        TaskKill(&work->tasks, work->task);
    }

    work->task = TaskCreate(&work->tasks, &gTaskDescPooSpark, &sparkPos);
    work->stage = NextPooPileStage(work->stage);
    AnimStart(&work->anim, work->stage, 0);
    m4aSongNumStart(SONG_SYS_PO_WOOD);

    if (work->stage == POO_PILE_STAGE_FLAT) {
        ColliderUnregister(&work->collider);
        work->colliderActive = FALSE;
        RemovePooNode(&work->node);
    } else {
        ColliderSetHeight(&work->collider, GetPooPileHeight(work->stage));
    }

    return 1;
}

void task_poo_pile_2(PooPileWork* work) {
    u16 z;
    s16 x;
    s16 y;

    x = (work->pos.x >> 8) - gPooScrollX;
    y = (work->pos.y >> 8) - gPooScrollY;

    if (x < -16 || x > 256 || y < -36 || y > 196) {
        if (work->palette != NULL) {
            ReleaseObjTiles(work->tiles);
            ReleaseObjPalette(work->palette);
            work->palette = NULL;
        }

        if (work->colliderActive) {
            ColliderUnregister(&work->collider);
            RemovePooNode(&work->node);
            work->colliderActive = FALSE;
        }

        TaskPoolUpdate(&work->tasks);
        TaskPoolDraw(&work->tasks);
    } else {
        work->gfx = AnimUpdate(&work->anim);

        if (work->palette == NULL) {
            work->tiles = LoadObjTiles(gPooPileTiles, sizeof(gPooPileTiles));
            work->palette = LoadObjPalette(gPooPilePalette, sizeof(gPooPilePalette));
        }

        if (work->stage != POO_PILE_STAGE_FLAT) {
            z = -0x1004 - (work->pos.y >> 8) * 4;

            if (!work->colliderActive) {
                ColliderInit(&work->collider, COLLIDER_TYPE_PLATFORM, 4, GetPooPileHeight(work->stage));
                AddPooNode(&work->node, 0x240, &work->pos);
                work->colliderActive = TRUE;
            }
        } else {
            z = 0xFFF1;
        }

        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), z);
        TaskPoolUpdate(&work->tasks);
        TaskPoolDraw(&work->tasks);
    }
}

void task_poo_pile_3(PooPileWork* work) {
    if (work->palette != NULL) {
        ReleaseObjTiles(work->tiles);
        ReleaseObjPalette(work->palette);
    }

    if (work->colliderActive) {
        ColliderUnregister(&work->collider);
        RemovePooNode(&work->node);
    }

    TaskPoolDestroy(&work->tasks);
}

void CreatePooPileTask(void* pool, u16 kind, s32 x, s32 y) {
    s32 kindStages[6];
    PooPileArgs args;

    memcpy(kindStages, gPooPileKindStages, sizeof(kindStages));
    args.x = x;
    args.y = y;
    args.stage = kindStages[kind];
    TaskCreate(pool, &gTaskDescPooPile, &args);
}

void task_poo_tigerstump_0(PooStumpWork* work, PooPos* pos) {
    work->x = pos->x;
    work->y = pos->y + 0x800;
    work->z = 0;
    work->palette = NULL;
    work->gfx = gPooTigerStumpFrame0;
    ColliderSetPosition(&work->collider, work->x, work->y, 0);
}

u8 task_poo_tigerstump_1(PooStumpWork* work) {
    if (work->palette != NULL) {
        if (ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_POOH)) {
            gPoohRequest = POOH_REQUEST_BLOCKED;
        }
    }

    return 1;
}

void task_poo_tigerstump_2(PooStumpWork* work) {
    s16 x;
    s16 y;

    x = (work->x >> 8) - gPooScrollX;
    y = ((work->y - 0x800) >> 8) - gPooScrollY;

    if (x < -96 || x > 336 || y < -64 || y > 224) {
        if (work->palette != NULL) {
            ReleaseObjTiles(work->tiles);
            ReleaseObjPalette(work->palette);
            work->palette = NULL;
            ColliderUnregister(&work->collider);
        }
    } else {
        if (work->palette == NULL) {
            work->tiles = LoadObjTiles(gPooTigerStumpTiles, sizeof(gPooTigerStumpTiles));
            work->palette = LoadObjPalette(gPooStumpPalette, sizeof(gPooStumpPalette));
            ColliderInit(&work->collider, COLLIDER_TYPE_PLATFORM, 15, 24);
        }

        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), -0x1004 - ((work->y - 0x700) >> 8) * 4);
    }
}

void task_poo_tigerstump_3(PooStumpWork* work) {
    if (work->palette != NULL) {
        ReleaseObjTiles(work->tiles);
        ReleaseObjPalette(work->palette);
        ColliderUnregister(&work->collider);
    }
}

void task_poo_poohstump_0(PooStumpWork* work, PooPos* pos) {
    work->x = pos->x;
    work->y = pos->y;
    work->z = 0;
    work->palette = NULL;
    work->gfx = gPooPoohStumpFrame0;
    ColliderSetPosition(&work->collider, work->x, work->y, 0);
}

u8 task_poo_poohstump_1(PooStumpWork* work) {
    if (work->palette != NULL) {
        if (ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_POOH)) {
            gPoohRequestX = work->x;
            gPoohRequestY = work->y;
            gPoohRequest = POOH_REQUEST_STUMP;
        }
    }

    return 1;
}

void task_poo_poohstump_2(PooStumpWork* work) {
    s16 x;
    s16 y;

    x = (work->x >> 8) - gPooScrollX;
    y = (work->y >> 8) - gPooScrollY;

    if (x < -80 || x > 320 || y < -24 || y > 184) {
        if (work->palette != NULL) {
            ReleaseObjTiles(work->tiles);
            ReleaseObjPalette(work->palette);
            work->palette = NULL;
            ColliderUnregister(&work->collider);
        }
    } else {
        if (work->palette == NULL) {
            work->tiles = LoadObjTiles(gPooPoohStumpTiles, sizeof(gPooPoohStumpTiles));
            work->palette = LoadObjPalette(gPooStumpPalette, sizeof(gPooStumpPalette));
            ColliderInit(&work->collider, COLLIDER_TYPE_PLATFORM, 7, 14);
        }

        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), -0x1004 - ((work->y - 0x500) >> 8) * 4);
    }
}

void task_poo_poohstump_3(PooStumpWork* work) {
    if (work->palette != NULL) {
        ReleaseObjTiles(work->tiles);
        ReleaseObjPalette(work->palette);
        ColliderUnregister(&work->collider);
    }
}

void SetPooPigletAnimation(PooPigletWork* work, s32 animIndex, u16 flags) {
    if (work->animIndex != animIndex) {
        work->animIndex = animIndex;
        AnimChangeWithTables(&work->anim, gPooPigletAnimDescs[animIndex].animId, flags, gPooPigletAnimDescs[animIndex].unk_00, gPooPigletAnimDescs[animIndex].unk_04);
        SetObjTileSource(work->tiles, gPooPigletAnimDescs[animIndex].tiles);
    }
}

enum PooPigletState {
    POO_PIGLET_STATE_WAIT,
    POO_PIGLET_STATE_WALK_OUT,
    POO_PIGLET_STATE_FACE_AWAY,
    POO_PIGLET_STATE_FACE_FRONT,
    POO_PIGLET_STATE_FACE_AWAY_AGAIN,
    POO_PIGLET_STATE_FACE_FRONT_AGAIN,
    POO_PIGLET_STATE_WALK_BACK
};

void task_poo_piglet_0(PooPigletWork* work) {
    u16 maxBytes;
    u16 bytes;
    u8 i;

    work->x = 0x2A500;
    work->y = 0x21100;
    work->z = 0;
    work->state = POO_PIGLET_STATE_WAIT;
    work->timer = 0;
    work->palette = NULL;
    maxBytes = 0;

    for (i = 0; i < 4; i++) {
        bytes = GetMaxSpriteTileBytes(gPooPigletGfxDescs[i].gfxTable, gPooPigletGfxDescs[i].gfxCount);

        if (maxBytes < bytes) {
            maxBytes = bytes;
        }
    }

    work->tiles = AllocObjTiles(maxBytes, NULL);
    AnimInit(&work->anim, NULL, NULL);
    work->animIndex = 4;
    SetPooPigletAnimation(work, 0, 1);
    work->flipped = FALSE;
    work->gfx = AnimGetGfx(&work->anim);
    TaskPoolInit(&work->tasks, 1);
    TaskCreate(&work->tasks, &gTaskDescPooShadow, &work->x);

    if (IsPooEventDone(POO_EVENT_PIGLET)) {
        work->interactionId = AddPoohInteraction(&work->collider, CARD_MSG_PIGLET_TALK);
        SetPoohInteractionEnabled(work->interactionId, FALSE);
    }
}

u8 task_poo_piglet_1(PooPigletWork* work) {
    if (work->palette != NULL && work->collider.colliding) {
        if (!ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_POOH)) {
            return 1;
        }

        if (IsPooEventDone(POO_EVENT_PIGLET)) {
            return 1;
        }

        gPoohRequestX = work->x;
        gPoohRequestY = work->y;
        gPoohRequest = POOH_REQUEST_PIGLET;
        ExitPoohMode(EVENT_136_100ACREWOOD_LV1);
        SetPooEventDone(POO_EVENT_PIGLET);
        SetJiminyFlag(JIMINY_RECORD_CHARACTER_PIGLET);
    }

    switch (work->state) {
    case POO_PIGLET_STATE_WAIT:
        SetPooPigletAnimation(work, 0, 1);
        work->flipped = FALSE;

        if (work->timer > 209) {
            work->state = POO_PIGLET_STATE_WALK_OUT;
            work->speed = 0;
        } else {
            work->timer++;
        }

        break;
    case POO_PIGLET_STATE_WALK_OUT:
        SetPooPigletAnimation(work, 3, 1);
        work->flipped = TRUE;
        work->speed += 0x600;

        if (work->speed > 128) {
            work->speed = 128;
        }

        work->x += gSineTable[0x20] * work->speed >> 8;
        work->y += -gSineTable[0x60] * work->speed >> 8;

        if (work->x > 0x2C8FF) {
            work->state = POO_PIGLET_STATE_FACE_AWAY;
            work->timer = 0;
        }

        break;
    case POO_PIGLET_STATE_FACE_AWAY:
        SetPooPigletAnimation(work, 1, 1);
        work->flipped = TRUE;

        if (work->timer > 39) {
            work->state = POO_PIGLET_STATE_FACE_FRONT;
            work->timer = 0;
        } else {
            work->timer++;
        }

        break;
    case POO_PIGLET_STATE_FACE_FRONT:
        SetPooPigletAnimation(work, 0, 1);
        work->flipped = TRUE;

        if (work->timer > 29) {
            work->state = POO_PIGLET_STATE_FACE_AWAY_AGAIN;
            work->timer = 0;
        } else {
            work->timer++;
        }

        break;
    case POO_PIGLET_STATE_FACE_AWAY_AGAIN:
        SetPooPigletAnimation(work, 1, 1);
        work->flipped = TRUE;

        if (work->timer > 29) {
            work->state = POO_PIGLET_STATE_FACE_FRONT_AGAIN;
            work->timer = 0;
        } else {
            work->timer++;
        }

        break;
    case POO_PIGLET_STATE_FACE_FRONT_AGAIN:
        SetPooPigletAnimation(work, 0, 1);
        work->flipped = TRUE;

        if (work->timer <= 59) {
            work->timer++;
        } else {
            work->state = POO_PIGLET_STATE_WALK_BACK;
            work->speed = 0;
        }

        break;
    case POO_PIGLET_STATE_WALK_BACK:
        SetPooPigletAnimation(work, 2, 1);
        work->flipped = FALSE;
        work->speed += 0x600;

        if (work->speed > 128) {
            work->speed = 128;
        }

        work->x += gSineTable[0xA0] * work->speed >> 8;
        work->y += -gSineTable[0xE0] * work->speed >> 8;

        if (work->x <= 0x2A500) {
            work->x = 0x2A500;
            work->y = 0x21100;
            work->state = POO_PIGLET_STATE_WAIT;
            work->timer = 0;
        }

        break;
    default:
        break;
    }

    work->gfx = AnimUpdate(&work->anim);
    return 1;
}

void task_poo_piglet_2(PooPigletWork* work) {
    u16 flags;
    s16 x;
    s16 y;

    x = (work->x >> 8) - gPooScrollX;
    y = (work->y >> 8) - gPooScrollY;

    if (IsRectOutsideScreen(x, y, 24, 8, 8, 8)) {
        if (work->palette != NULL) {
            ReleaseObjPalette(work->palette);
            ColliderUnregister(&work->collider);
            SetPoohInteractionEnabled(work->interactionId, FALSE);
            work->palette = NULL;
        }
    } else {
        TaskPoolUpdate(&work->tasks);

        if (work->palette == NULL) {
            work->palette = LoadObjPalette(gPigletPalette, sizeof(gPigletPalette));

            if (IsPooEventDone(POO_EVENT_PIGLET)) {
                ColliderInit(&work->collider, COLLIDER_TYPE_POOH_OBJECT, 4, 16);
            } else {
                ColliderInit(&work->collider, COLLIDER_TYPE_POOH_OBJECT, 16, 16);
            }

            SetPoohInteractionEnabled(work->interactionId, TRUE);
        }

        ColliderSetPosition(&work->collider, work->x, work->y, work->z);
        flags = work->flipped ? (SPRITE_PRIORITY(2) | SPRITE_FLAG_HFLIP) : SPRITE_PRIORITY(2);
        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, flags, -0x1004 - (work->y >> 8) * 4);
        TaskPoolDraw(&work->tasks);
    }
}

void task_poo_piglet_3(PooPigletWork* work) {
    ReleaseObjTiles(work->tiles);

    if (work->palette != NULL) {
        ReleaseObjPalette(work->palette);
        ColliderUnregister(&work->collider);
    }

    TaskPoolDestroy(&work->tasks);
}

void task_poo_eeyore_0(PooEeyoreWork* work) {
    work->x = 0x82700;
    work->y = 0x47E00;
    work->z = 0;
    work->ground = 0;
    work->tileBytes = GetMaxSpriteTileBytes(gEeyoreFl00Frames, ARRAY_COUNT(gEeyoreFl00Frames));
    work->tiles = NULL;
    work->palette = NULL;

    if (IsPooEventDone(POO_EVENT_EEYORE)) {
        work->animId = 0;
    } else {
        work->animId = 4;
    }

    AnimInit(&work->anim, gEeyoreFl00Anims, gEeyoreFl00Frames);
    AnimStart(&work->anim, work->animId, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    TaskPoolInit(&work->tasks, 1);
    TaskCreate(&work->tasks, &gTaskDescPooShadow, &work->x);
    ColliderInit(&work->collider, COLLIDER_TYPE_POOH_OBJECT, 16, 16);
    ColliderSetPosition(&work->collider, work->x, work->y, work->z);
    work->colliderActive = 1;

    if (!IsPooEventDone(POO_EVENT_EEYORE)) {
        work->interactionId = AddPoohInteraction(&work->collider, CARD_MSG_EEYORE_TALK_0);
    } else {
        work->interactionId = AddPoohInteraction(&work->collider, CARD_MSG_EEYORE_TALK_1);
    }

    SetPoohInteractionEnabled(work->interactionId, TRUE);
    work->moveTimer = 0;
}

u8 task_poo_eeyore_1(PooEeyoreWork* work) {
    if (work->colliderActive != 0) {
        if (ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_POOH)) {
            gPoohRequest = POOH_REQUEST_BLOCKED;
        }
    }

    if (IsPooEeyoreTailLanded() && work->animId == 4) {
        work->animId = 5;
        AnimStart(&work->anim, 5, ANIM_FLAG_LOOP);
        work->moveTimer = 180;
    }

    if (work->moveTimer != 0) {
        ApproachValue((u32*)&work->x, 0x80B00, work->moveTimer);
        ApproachValue((u32*)&work->y, 0x48C00, work->moveTimer);
        SetPooCameraFocus(work->x, work->y + work->z);
        work->moveTimer--;

        if (work->moveTimer == 0) {
            ExitPoohMode(EVENT_140_100ACREWOOD_LV4);
            SetPooEventDone(POO_EVENT_EEYORE);
            SetJiminyFlag(JIMINY_RECORD_CHARACTER_EEYORE);
            work->animId = 1;
            AnimStart(&work->anim, 1, ANIM_FLAG_LOOP);
        }
    }

    return 1;
}

void task_poo_eeyore_2(PooEeyoreWork* work) {
    u8* colliderActive;
    s16 x;
    s16 y;

    x = (work->x >> 8) - gPooScrollX;
    y = (work->y >> 8) - gPooScrollY;

    if (IsRectOutsideScreen(x, y, 24, 10, 24, 24)) {
        if (work->palette != NULL) {
            ReleaseObjPalette(work->palette);
            work->palette = NULL;
            ReleaseObjTiles(work->tiles);
        }

        colliderActive = &work->colliderActive;

        if (*colliderActive != 0) {
            ColliderUnregister(&work->collider);
            SetPoohInteractionEnabled(work->interactionId, FALSE);
            *colliderActive = 0;
        }
    } else {
        colliderActive = &work->colliderActive;

        if (*colliderActive == 0) {
            ColliderInit(&work->collider, COLLIDER_TYPE_POOH_OBJECT, 16, 16);
            SetPoohInteractionEnabled(work->interactionId, TRUE);
            *colliderActive = 1;
        }

        ColliderSetPosition(&work->collider, work->x, work->y, work->z);
        work->gfx = AnimUpdate(&work->anim);

        if (work->palette == NULL) {
            work->palette = LoadObjPalette(gEeyorePalette, sizeof(gEeyorePalette));
            work->tiles = AllocObjTiles(work->tileBytes, gEeyoreFl00Tiles);
        }

        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), -0x1004 - (work->y >> 8) * 4);
        TaskPoolUpdate(&work->tasks);
        TaskPoolDraw(&work->tasks);
    }
}

void task_poo_eeyore_3(PooEeyoreWork* work) {
    if (work->palette != NULL) {
        ReleaseObjTiles(work->tiles);
        ReleaseObjPalette(work->palette);
    }

    if (work->colliderActive != 0) {
        ColliderUnregister(&work->collider);
    }

    TaskPoolDestroy(&work->tasks);
}

void task_poo_owl_0(PooOwlWork* work) {
    work->pos.x = 0x41500;
    work->pos.y = 0x20700;
    work->pos.z = -0x3000;
    work->pos.ground = 0;
    work->tileBytes = GetMaxSpriteTileBytes(gOwlFl00Frames, 18);
    work->palette = NULL;
    work->gfx = gOwlFl00Frame0;
    AnimInit(&work->anim, gOwlFl00Anims, gOwlFl00Frames);
    work->flying = FALSE;
    work->descending = FALSE;
    sPooOwlBalloonPos.x = 0x3FD00;
    sPooOwlBalloonPos.y = 0x21B00;
    TaskPoolInit(&work->tasks, 1);
    TaskCreate(&work->tasks, &gTaskDescPooOwlballoon, &sPooOwlBalloonPos);
}

u8 task_poo_owl_1(PooOwlWork* work) {
    if (IsPoohOnOwlBalloon()) {
        SetPooCameraFocus(gPoohPos->x, gPoohPos->y + gPoohPos->z);

        if (gPoohPos->z <= -0x3800) {
            FreezePooCamera();
        }

        if (IsPoohOffScreen()) {
            if (!work->flying) {
                work->flying = TRUE;
                AnimStart(&work->anim, 1, 0);
                work->flyTimer = 60;
                m4aSongNumStart(SONG_SND_351);
            }

            if (AnimGetFrame(&work->anim) > 3) {
                if (work->flyTimer != 0) {
                    ApproachValueHalfSteps(&work->pos.z, -0x9000, work->flyTimer);
                    work->flyTimer--;
                    work->pos.x -= 204;
                } else {
                    work->pos.z -= 0x100;
                }
            }

            work->gfx = AnimUpdate(&work->anim);
        }
    }

    if (IsPoohDescendingWithOwl()) {
        if (!work->descending) {
            work->descending = TRUE;
            AnimStart(&work->anim, 4, ANIM_FLAG_LOOP);
        }

        if (AnimGetFrame(&work->anim) == 0 && work->anim.timer == 0) {
            m4aSongNumStart(SONG_EV_HUKUROUJUMP);
        }

        work->pos = *gPoohPos;
        work->gfx = AnimUpdate(&work->anim);
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

void task_poo_owl_2(PooOwlWork* work) {
    s16 x;
    s16 y;

    TaskPoolDraw(&work->tasks);
    x = (work->pos.x >> 8) - gPooScrollX;
    y = (work->pos.y >> 8) + (work->pos.z >> 8) - gPooScrollY;

    if (IsRectOutsideScreen(x, y, 24, 8, 8, 8)) {
        if (work->palette != NULL) {
            ReleaseObjTiles(work->tiles);
            ReleaseObjPalette(work->palette);
            work->palette = NULL;
        }
    } else {
        if (work->palette == NULL) {
            work->tiles = AllocObjTiles(work->tileBytes, gOwlFl00Tiles);
            work->palette = LoadObjPalette(gOwlPalette, sizeof(gOwlPalette));
        }

        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), -0x1004 - ((work->pos.y + work->pos.z) >> 8) * 4);
    }
}

void task_poo_owl_3(PooOwlWork* work) {
    if (work->palette != NULL) {
        ReleaseObjTiles(work->tiles);
        ReleaseObjPalette(work->palette);
    }

    TaskPoolDestroy(&work->tasks);
}

enum PooRabbitAnim {
    POO_RABBIT_ANIM_STAND,
    POO_RABBIT_ANIM_HARVEST,
    POO_RABBIT_ANIM_WALK_BACK,
    POO_RABBIT_ANIM_STAND_AWAY,
    POO_RABBIT_ANIM_CARRY_SACK,
    POO_RABBIT_ANIM_DROP_SACK,
    POO_RABBIT_ANIM_HOLD_SACK,
    POO_RABBIT_ANIM_NONE
};

void SetPooRabbitAnimation(PooRabbitWork* work, s32 animIndex, u16 flags) {
    if (work->animIndex != animIndex) {
        work->animIndex = animIndex;
        AnimChangeWithTables(&work->anim, gPooRabbitAnimDescs[animIndex].animId, flags, gPooRabbitAnimDescs[animIndex].unk_00, gPooRabbitAnimDescs[animIndex].unk_04);
        SetObjTileSource(work->tiles, gPooRabbitAnimDescs[animIndex].tiles);
    }
}

void task_poo_rabbit_0(PooRabbitWork* work) {
    u16 maxBytes;
    u16 bytes;
    u8 i;

    work->x = 0x1B700;
    work->y = 0x16E00;
    work->z = 0;
    work->ground = 0;
    work->palette = NULL;
    maxBytes = 0;

    for (i = 0; i < 2; i++) {
        bytes = GetMaxSpriteTileBytes(gPooRabbitGfxDescs[i].gfxTable, gPooRabbitGfxDescs[i].gfxCount);

        if (maxBytes < bytes) {
            maxBytes = bytes;
        }
    }

    work->tiles = AllocObjTiles(maxBytes, NULL);
    AnimInit(&work->anim, NULL, NULL);
    work->animIndex = POO_RABBIT_ANIM_NONE;
    SetPooRabbitAnimation(work, POO_RABBIT_ANIM_HARVEST, 0);
    work->flipped = FALSE;
    work->timer = 0;
    work->gfx = AnimGetGfx(&work->anim);
    TaskPoolInit(&work->tasks, 1);
    CreatePooShadowscaleTask(&work->tasks, &work->x, Q_8_8(1));
    ColliderSetPosition(&work->collider, work->x, work->y, work->z);
    work->interactionId = AddPoohInteraction(&work->collider, CARD_MSG_RABBIT_TALK_0);
    SetPoohInteractionEnabled(work->interactionId, FALSE);
}

u8 task_poo_rabbit_1(PooRabbitWork* work) {
    switch (work->animIndex) {
    case POO_RABBIT_ANIM_HARVEST:
        if (AnimIsFinished(&work->anim)) {
            work->timer++;

            if (work->timer <= 3) {
                AnimReset(&work->anim);
            } else {
                SetPooRabbitAnimation(work, POO_RABBIT_ANIM_CARRY_SACK, 1);
                work->flipped = TRUE;
                work->timer = 312;
            }
        }

        break;
    case POO_RABBIT_ANIM_CARRY_SACK:
        if (work->palette != NULL && work->collider.colliding && ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_POOH)) {
            SetPooRabbitAnimation(work, POO_RABBIT_ANIM_HOLD_SACK, 0);
            work->waitTimer = 20;
        } else {
            ApproachValue(&work->x, 0x23000, work->timer);
            ApproachValue(&work->y, 0x12400, work->timer);
            work->timer--;

            if (work->timer == 0) {
                SetPooRabbitAnimation(work, POO_RABBIT_ANIM_DROP_SACK, 0);
                work->flipped = TRUE;
            }
        }

        break;
    case POO_RABBIT_ANIM_DROP_SACK:
        if (AnimIsFinished(&work->anim)) {
            SetPooRabbitAnimation(work, POO_RABBIT_ANIM_WALK_BACK, 1);
            work->flipped = FALSE;
            work->timer = 260;
        }

        break;
    case POO_RABBIT_ANIM_WALK_BACK:
        if (work->palette != NULL && work->collider.colliding && ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_POOH)) {
            work->waitTimer = 20;
            SetPooRabbitAnimation(work, POO_RABBIT_ANIM_STAND, 0);
        } else {
            ApproachValue(&work->x, 0x1B700, work->timer);
            ApproachValue(&work->y, 0x16E00, work->timer);
            work->timer--;

            if (work->timer == 0) {
                SetPooRabbitAnimation(work, POO_RABBIT_ANIM_HARVEST, 0);
                work->flipped = FALSE;
                work->timer = 0;
            }
        }

        break;
    case POO_RABBIT_ANIM_HOLD_SACK:
        if (work->waitTimer == 0) {
            if (work->palette != NULL && !ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_POOH)) {
                SetPooRabbitAnimation(work, POO_RABBIT_ANIM_CARRY_SACK, 1);
            }
        } else {
            work->waitTimer--;
        }

        break;
    case POO_RABBIT_ANIM_STAND:
        if (work->waitTimer != 0) {
            work->waitTimer--;
        } else if (work->palette != NULL && !ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_POOH)) {
            SetPooRabbitAnimation(work, POO_RABBIT_ANIM_WALK_BACK, 1);
        }

        break;
    default:
        break;
    }

    work->gfx = AnimUpdate(&work->anim);
    return 1;
}

void task_poo_rabbit_2(PooRabbitWork* work) {
    TaskPool* pool;
    s32 flags;
    s16 x;
    s16 y;

    x = (work->x >> 8) - gPooScrollX;
    y = (work->y >> 8) - gPooScrollY;

    if (IsRectOutsideScreen(x, y, 48, 8, 16, 16)) {
        if (work->palette != NULL) {
            ReleaseObjPalette(work->palette);
            ColliderUnregister(&work->collider);
            SetPoohInteractionEnabled(work->interactionId, FALSE);
            work->palette = NULL;
        }
    } else {
        pool = &work->tasks;
        TaskPoolUpdate(pool);

        if (work->palette == NULL) {
            work->palette = LoadObjPalette(gRabbitPalettes, sizeof(gRabbitPalettes));
            ColliderInit(&work->collider, COLLIDER_TYPE_POOH_OBJECT, 4, 48);
            SetPoohInteractionEnabled(work->interactionId, TRUE);
        }

        ColliderSetPosition(&work->collider, work->x, work->y, work->z);
        flags = work->flipped ? (SPRITE_PRIORITY(2) | SPRITE_FLAG_HFLIP) : SPRITE_PRIORITY(2);
        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, flags, -0x1004 - (work->y >> 8) * 4);
        TaskPoolDraw(pool);
    }
}

void task_poo_rabbit_3(PooRabbitWork* work) {
    if (work->palette != NULL) {
        ReleaseObjTiles(work->tiles);
        ReleaseObjPalette(work->palette);
        ColliderUnregister(&work->collider);
    }

    TaskPoolDestroy(&work->tasks);
}

enum PooTiggerMode {
    POO_TIGGER_MODE_STAND,
    POO_TIGGER_MODE_BOUNCE,
    POO_TIGGER_MODE_HOP
};

void SetPooTiggerrooAnimation(PooTiggerWork* work, u16 flags) {
    s32 animIndex;

    AnimReset(&work->anim);

    if (work->mode == POO_TIGGER_MODE_HOP) {
        switch (work->heading) {
        case FLD_ANGLE_DOWN_LEFT:
            animIndex = 2;
            work->flipped = FALSE;
            break;
        case FLD_ANGLE_DOWN_RIGHT:
            animIndex = 2;
            work->flipped = TRUE;
            break;
        case FLD_ANGLE_UP_LEFT:
            animIndex = 3;
            work->flipped = FALSE;
            break;
        case FLD_ANGLE_UP:
        default:
            animIndex = 3;
            work->flipped = TRUE;
            break;
        }
    } else {
        animIndex = 0;
    }

    if (work->animIndex != animIndex) {
        work->animIndex = animIndex;
        AnimStart(&work->anim, animIndex, flags);
    }
}

void SetPooTiggerAnimation(PooTiggerWork* work, u16 flags) {
    u16 animIndex;

    AnimReset(&work->anim);

    if (work->mode == POO_TIGGER_MODE_STAND) {
        animIndex = 0;
    } else if (work->mode == POO_TIGGER_MODE_BOUNCE) {
        animIndex = 1;
    } else if (work->mode == POO_TIGGER_MODE_HOP) {
        switch (work->heading) {
        case FLD_ANGLE_DOWN_LEFT:
            animIndex = 2;
            work->flipped = FALSE;
            break;
        case FLD_ANGLE_DOWN_RIGHT:
            animIndex = 2;
            work->flipped = TRUE;
            break;
        case FLD_ANGLE_UP_LEFT:
            animIndex = 3;
            work->flipped = FALSE;
            break;
        case FLD_ANGLE_UP_RIGHT:
        default:
            animIndex = 3;
            work->flipped = TRUE;
            break;
        }
    } else {
        animIndex = 0;
    }

    if (work->animIndex != animIndex) {
        work->animIndex = animIndex;
        AnimChangeWithTables(&work->anim, gPooTiggerAnimDescs[animIndex].animId, flags, gPooTiggerAnimDescs[animIndex].unk_00, gPooTiggerAnimDescs[animIndex].unk_04);
        SetObjTileSource(work->tiles, gPooTiggerAnimDescs[animIndex].tiles);
    }
}

void StartPooTiggerHopStep(PooTiggerWork* work) {
    PooAnimData* anim;
    s32 hopHeights[8];

    memcpy(hopHeights, gPooTiggerHopHeights, sizeof(hopHeights));
    anim = ((PooAnimData**)gPooTiggerAnimDescs[work->animIndex].unk_00)[gPooTiggerAnimDescs[work->animIndex].animId];
    work->stepTimer = (&anim->frames[work->step])->duration;
    work->targetZ = hopHeights[work->step] - 0x1800;
    work->step++;
}

u16 GetPooTiggerAnimDuration(PooTiggerWork* work) {
    PooAnimData* anim;
    u16 duration;
    s32 i;

    anim = ((PooAnimData**)gPooTiggerAnimDescs[work->animIndex].unk_00)[gPooTiggerAnimDescs[work->animIndex].animId];
    duration = 0;

    for (i = 0; i < anim->frameCount; i++) {
        duration += anim->frames[i].duration;
    }

    return duration;
}

void StartPooTiggerHop(PooTiggerWork* work) {
    work->hopTimer = GetPooTiggerAnimDuration(work);

    if (work->heading == FLD_ANGLE_DOWN_LEFT) {
        work->x = gPooTiggerHopCorners[0];
        work->y = gPooTiggerHopCorners[1];
        work->targetX = gPooTiggerHopCorners[2];
        work->targetY = gPooTiggerHopCorners[3];
    } else if (work->heading == FLD_ANGLE_DOWN_RIGHT) {
        work->x = gPooTiggerHopCorners[2];
        work->y = gPooTiggerHopCorners[3];
        work->targetX = gPooTiggerHopCorners[4];
        work->targetY = gPooTiggerHopCorners[5];
    } else if (work->heading == FLD_ANGLE_UP_RIGHT) {
        work->x = gPooTiggerHopCorners[4];
        work->y = gPooTiggerHopCorners[5];
        work->targetX = gPooTiggerHopCorners[6];
        work->targetY = gPooTiggerHopCorners[7];
    } else {
        work->x = gPooTiggerHopCorners[6];
        work->y = gPooTiggerHopCorners[7];
        work->targetX = gPooTiggerHopCorners[0];
        work->targetY = gPooTiggerHopCorners[1];
    }

    work->z = -0x1800;
    work->step = 0;
    StartPooTiggerHopStep(work);
}

void PlayPooTiggerHopSound(s32 x, s32 y, s32 z, u8 isTigger) {
    s16 sx;
    s16 sy;

    sx = (x >> 8) - gPooScrollX;
    sy = (y >> 8) + (z >> 8) - gPooScrollY;

    if (!IsRectOutsideScreen(sx, sy, 120, 8, 24, 24)) {
        if (isTigger) {
            m4aSongNumStart(SONG_SND_961);
        } else {
            m4aSongNumStart(SONG_SYS_LU_JP);
        }
    }
}

void task_poo_tigger_0(PooTiggerWork* work) {
    PooShadowArgs args;
    u16 maxBytes;
    u16 bytes;
    u8 i;

    work->mode = POO_TIGGER_MODE_HOP;
    work->heading = FLD_ANGLE_DOWN_LEFT;
    work->isTigger = TRUE;
    work->palette = NULL;
    maxBytes = 0;

    for (i = 0; i < 4; i++) {
        bytes = GetMaxSpriteTileBytes(gPooTiggerGfxDescs[i].gfxTable, gPooTiggerGfxDescs[i].gfxCount);

        if (maxBytes < bytes) {
            maxBytes = bytes;
        }
    }

    work->tiles = AllocObjTiles(maxBytes, NULL);
    AnimInit(&work->anim, NULL, NULL);
    work->animIndex = 4;
    SetPooTiggerAnimation(work, 0);
    StartPooTiggerHop(work);
    work->gfx = AnimGetGfx(&work->anim);
    TaskPoolInit(&work->tasks, 1);
    args.pos = (PooPos*)&work->x;
    args.shadowInfo = &work->shadowInfo;
    TaskCreate(&work->tasks, &gTaskDescPooShadowdodai, &args);
    ColliderSetPosition(&work->collider, work->x, work->y, work->z);
}

u8 task_poo_tiggerroo_1(PooTiggerWork* work) {
    GetPooGroundZ(&work->collider, (PooPos*)&work->x, &work->onCollider);

    if (work->mode == POO_TIGGER_MODE_HOP) {
        if (work->hopTimer > 0) {
            ApproachValueHalfSteps(&work->x, work->targetX, work->hopTimer);
            ApproachValueHalfSteps(&work->y, work->targetY, work->hopTimer);
            work->hopTimer--;
            ApproachValue((u32*)&work->z, work->targetZ, work->stepTimer);
            work->stepTimer--;

            if (work->stepTimer == 0) {
                StartPooTiggerHopStep(work);
            }
        } else {
            switch (work->heading) {
            case FLD_ANGLE_DOWN_LEFT:
                work->heading = FLD_ANGLE_DOWN_RIGHT;
                break;
            case FLD_ANGLE_DOWN_RIGHT:
                work->heading = FLD_ANGLE_UP_RIGHT;
                break;
            case FLD_ANGLE_UP_RIGHT:
                work->heading = FLD_ANGLE_UP_LEFT;
                break;
            case FLD_ANGLE_UP_LEFT:
                work->heading = FLD_ANGLE_DOWN_LEFT;
                break;
            }

            if (work->isTigger) {
                SetPooTiggerAnimation(work, 0);
            } else {
                SetPooTiggerrooAnimation(work, 0);
            }

            StartPooTiggerHop(work);
            PlayPooTiggerHopSound(work->x, work->y, work->z, work->isTigger);
        }
    }

    work->gfx = AnimUpdate(&work->anim);
    TaskPoolUpdate(&work->tasks);
    return 1;
}

void task_poo_tiggerroo_2(PooTiggerWork* work) {
    u16 z;
    s32 flags;
    s32 onCollider;
    s16 x;
    s16 y;

    x = (work->x >> 8) - gPooScrollX;
    y = (work->y >> 8) + (work->z >> 8) - gPooScrollY;

    if (IsRectOutsideScreen(x, y, 56, 8, 24, 24)) {
        if (work->palette != NULL) {
            ReleaseObjPalette(work->palette);
            work->palette = NULL;
            ColliderUnregister(&work->collider);

            if (!work->isTigger) {
                if (work->tiles != NULL) {
                    ReleaseObjTiles(work->tiles);
                    work->tiles = NULL;
                }
            }
        }
    } else {
        if (work->palette == NULL) {
            if (work->isTigger) {
                work->palette = LoadObjPalette(gTiggerPalette, sizeof(gTiggerPalette));
                ColliderInit(&work->collider, COLLIDER_TYPE_NPC, 8, 8);
            } else {
                work->palette = LoadObjPalette(gRooPalette, sizeof(gRooPalette));
                work->tiles = AllocObjTiles(work->tileBytes, gRooFl00Tiles);
                ColliderInit(&work->collider, COLLIDER_TYPE_NPC, 8, 8);
            }
        }

        ColliderSetPosition(&work->collider, work->x, work->y, work->z);
        flags = work->flipped ? (SPRITE_PRIORITY(2) | SPRITE_FLAG_HFLIP) : SPRITE_PRIORITY(2);
        onCollider = work->onCollider;

        if (onCollider != 0) {
            z = -0x1008 - (work->collider.platformY >> 8) * 4;

            if (work->y >= gPooActor.pos.y) {
                z -= 2;
            } else {
                z += 2;
            }

            if (work->collider.penetration <= work->collider.radius) {
                work->shadowInfo.z = 0;
                work->shadowInfo.priority = 0;
            } else {
                work->shadowInfo.z = work->collider.platformZ;
                work->shadowInfo.priority = z + 1;
            }
        } else {
            z = -0x1004 - (work->y >> 8) * 4;
            work->shadowInfo.z = onCollider;

            if (onCollider != work->ground) {
                work->shadowInfo.priority = 0;
            } else {
                work->shadowInfo.priority = z + 1;
            }
        }

        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, flags, z);
        TaskPoolDraw(&work->tasks);
    }
}

void task_poo_tiggerroo_3(PooTiggerWork* work) {
    if (work->tiles != NULL) {
        ReleaseObjTiles(work->tiles);
    }

    if (work->palette != NULL) {
        ReleaseObjPalette(work->palette);
        ColliderUnregister(&work->collider);
    }

    TaskPoolDestroy(&work->tasks);
}

void task_poo_tiggerroo_0(PooTiggerWork* work) {
    PooShadowArgs args;

    work->mode = POO_TIGGER_MODE_HOP;
    work->heading = FLD_ANGLE_UP_RIGHT;
    work->isTigger = FALSE;
    work->palette = NULL;
    work->tiles = NULL;
    work->tileBytes = GetMaxSpriteTileBytes(gRooFl00Frames, 18);
    AnimInit(&work->anim, gRooFl00Anims, gRooFl00Frames);
    work->animIndex = 4;
    SetPooTiggerrooAnimation(work, 0);
    StartPooTiggerHop(work);
    work->gfx = AnimGetGfx(&work->anim);
    TaskPoolInit(&work->tasks, 1);
    args.pos = (PooPos*)&work->x;
    args.shadowInfo = &work->shadowInfo;
    TaskCreate(&work->tasks, &gTaskDescPooShadowdodai, &args);
    ColliderSetPosition(&work->collider, work->x, work->y, work->z);
}

enum PooRooState {
    POO_ROO_STATE_TRAPPED,
    POO_ROO_STATE_HANG,
    POO_ROO_STATE_JUMP_OFF,
    POO_ROO_STATE_HOME
};

void task_poo_roo_0(PooRooWork* work, PooPos* pos) {
    gStockMesDispWork = work;
    work->srcPos = pos;
    work->tiles = AllocObjTiles(GetMaxSpriteTileBytes(gRooFl00Frames, 8), gRooFl00Tiles);
    work->palette = LoadObjPalette(gRooPalette, sizeof(gRooPalette));
    AnimInit(&work->anim, gRooFl00Anims, gRooFl00Frames);

    if (IsPooEventDone(POO_EVENT_ROO)) {
        work->pos.x = 0x95F00;
        work->pos.y = 0x4EE00;
        work->pos.z = 0;
        AnimStart(&work->anim, 0, 0);
        work->flipped = FALSE;
        work->state = POO_ROO_STATE_HOME;
    } else {
        work->pos = *work->srcPos;
        AnimStart(&work->anim, 4, 0);
        work->flipped = FALSE;
        work->state = POO_ROO_STATE_TRAPPED;
    }

    work->gfx = AnimGetGfx(&work->anim);
    TaskPoolInit(&work->tasks, 1);
    TaskCreate(&work->tasks, &gTaskDescPooShadow, &work->pos);
    ColliderInit(&work->collider, COLLIDER_TYPE_POOH_OBJECT, 4, 32);
    ColliderSetPosition(&work->collider, work->pos.x, work->pos.y, work->pos.z);

    if (IsPooEventDone(POO_EVENT_ROO)) {
        work->interactionId = AddPoohInteraction(&work->collider, CARD_MSG_ROO_TALK);
    }
}

u8 task_poo_roo_1(PooRooWork* work) {
    s32 hangZ;

    switch (work->state) {
    case POO_ROO_STATE_TRAPPED:
        if (AnimIsFinished(&work->anim)) {
            AnimStart(&work->anim, 5, 0);
            work->state = POO_ROO_STATE_HANG;
            work->pos = *work->srcPos;
            work->pos.x -= 0x600;
            work->pos.z += 0x1F00;
        }

        break;
    case POO_ROO_STATE_HANG:
        work->lastZ = work->pos.z;
        work->pos = *work->srcPos;
        work->pos.x -= 0x600;
        hangZ = work->pos.z + 0x1F00;
        work->pos.z = hangZ;

        if (work->lastZ - hangZ < 0 && hangZ >= -0x2100) {
            AnimStart(&work->anim, 6, 0);
            work->state = POO_ROO_STATE_JUMP_OFF;
            work->vz = 0;
        }

        break;
    case POO_ROO_STATE_JUMP_OFF:
        work->pos.z += work->vz;
        work->vz += 7;

        if (work->pos.z >= 0) {
            work->pos.z = 0;
        } else {
            work->pos.x -= 0x40;
            work->pos.y += 0x40;
        }

        if (work->srcPos->z >= 0) {
            ExitPoohMode(EVENT_139_100ACREWOOD_LV3);
            SetPooEventDone(POO_EVENT_ROO);
            SetJiminyFlag(JIMINY_RECORD_CHARACTER_ROO);
        }

        break;
    case POO_ROO_STATE_HOME:
        if (ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_POOH)) {
            gPoohRequest = POOH_REQUEST_BLOCKED;
        }

        break;
    }

    work->gfx = AnimUpdate(&work->anim);
    ColliderSetPosition(&work->collider, work->pos.x, work->pos.y, work->pos.z);
    TaskPoolUpdate(&work->tasks);
    return 1;
}

void task_poo_roo_2(PooRooWork* work) {
    s16 x;
    s16 y;
    s32 flags;
    s32 baseY;

    x = (work->pos.x >> 8) - gPooScrollX;
    baseY = work->pos.y >> 8;
    y = baseY + (work->pos.z >> 8) - gPooScrollY;
    flags = work->flipped ? (SPRITE_PRIORITY(2) | SPRITE_FLAG_HFLIP) : SPRITE_PRIORITY(2);
    DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, flags, -0x1006 - baseY * 4);

    if (work->state != POO_ROO_STATE_TRAPPED) {
        TaskPoolDraw(&work->tasks);
    }
}

void task_poo_roo_3(PooRooWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ColliderUnregister(&work->collider);
    TaskPoolDestroy(&work->tasks);
}

u8 IsPooRooAnimFrameEnding() {
    return AnimIsFrameEnding(&((PooRooWork*)gStockMesDispWork)->anim);
}

u8 IsPooRooAnimFinished() {
    return AnimIsFinished(&((PooRooWork*)gStockMesDispWork)->anim);
}

void task_poo_roo_footmark_0(PooFootmarkWork* work) {
    work->x = 0x4A700;
    work->y = 0x28E00;
    work->z = 0;
    work->tiles = LoadObjTiles(gRoFootmarkTiles, sizeof(gRoFootmarkTiles));
    work->palette = NULL;

    if (!IsPooEventDone(POO_EVENT_ROO)) {
        work->gfx = gRoFootmarkFrame0;
    } else {
        work->gfx = gRoFootmarkFrame1;
    }
}

u8 task_poo_roo_footmark_1(void* work) {
    return 1;
}

void task_poo_roo_footmark_2(PooFootmarkWork* work) {
    PooNode* node;
    s16 x;
    s16 y;

    x = (work->x >> 8) - gPooScrollX;
    y = (work->y >> 8) - gPooScrollY;

    if (IsRectOutsideScreen(x, y, 0, 48, 0, 48)) {
        if (work->palette != NULL) {
            ReleaseObjPalette(work->palette);
            RemovePooNode(&work->node);
            work->palette = NULL;
        }
    } else {
        if (work->palette == NULL) {
            work->palette = LoadObjPalette(gRoFootmarkPalette, sizeof(gRoFootmarkPalette));
            node = &work->node;
            AddPooNode(node, 0x240, &work->x);

            if (IsPooEventDone(POO_EVENT_ROO)) {
                SetPooNodeWeight(node, 0);
            }
        }

        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), 0xFFF1);
    }
}

void task_poo_roo_footmark_3(PooFootmarkWork* work) {
    ReleaseObjTiles(work->tiles);

    if (work->palette != NULL) {
        ReleaseObjPalette(work->palette);
        RemovePooNode(&work->node);
    }
}

void task_poo_leaf_0(PooLeafWork* work, PooSpawnArgs* args) {
    work->x = args->pos.x;
    work->y = args->pos.y;
    work->z = 0;
    work->prizeId = args->prizeId;
    work->tileBytes = GetMaxSpriteTileBytes(gPooLeafFrames, 5);
    work->palette = NULL;
    AnimInit(&work->anim, gPooLeafAnims, gPooLeafFrames);
    AnimStart(&work->anim, 0, 0);
    work->gfx = AnimGetGfx(&work->anim);
    ColliderSetPosition(&work->collider, work->x + 0x1C00, work->y + 0x1000, work->z);
    work->playing = 0;
}

u8 task_poo_leaf_1(PooLeafWork* work) {
    if (work->palette != NULL && (work->collider.standFlags & COLLIDER_STAND_STOOD_ON) != 0 && work->playing == 0) {
        work->playing = 1;
        AnimReset(&work->anim);
        m4aSongNumStart(SONG_SND_224);

        if (!IsPooPrizeDropped(work->prizeId)) {
            if (SpawnPooPrizes(MAP_PRIZE_MOOGLE_POINTS_SMALL, 3, work->x + 0x1C00, work->y + 0x2000, work->z) != 0) {
                SetPooPrizeDropped(work->prizeId);
            }
        }
    }

    return 1;
}

void task_poo_leaf_2(PooLeafWork* work) {
    u8* playing;
    s16 x;
    s16 y;

    x = (work->x >> 8) - gPooScrollX;
    y = (work->y >> 8) - gPooScrollY;

    if (IsRectOutsideScreen(x, y, 0, 32, 0, 56)) {
        if (work->palette != NULL) {
            ReleaseObjTiles(work->tiles);
            ReleaseObjPalette(work->palette);
            work->palette = NULL;
            ColliderUnregister(&work->collider);
            work->playing = 0;
        }
    } else {
        if (work->palette == NULL) {
            work->tiles = AllocObjTiles(work->tileBytes, gPooLeafTiles);
            work->palette = LoadObjPalette(gPooLeafPalette, sizeof(gPooLeafPalette));
            ColliderInit(&work->collider, COLLIDER_TYPE_SOLID, 28, 0);
        }

        playing = &work->playing;

        if (*playing != 0) {
            work->gfx = AnimUpdate(&work->anim);

            if (!AnimIsFinished(&work->anim)) {
                DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), 0xFFF1);
            } else if ((work->collider.standFlags & COLLIDER_STAND_STOOD_ON) == 0) {
                *playing = 0;
            }
        }
    }
}

void task_poo_leaf_3(PooLeafWork* work) {
    if (work->palette != NULL) {
        ReleaseObjTiles(work->tiles);
        ReleaseObjPalette(work->palette);
        ColliderUnregister(&work->collider);
    }
}

void task_poo_tanpopo_0(PooTanpopoWork* work, PooSpawnArgs* args) {
    work->x = args->pos.x;
    work->y = args->pos.y;
    work->z = 0;
    work->prizeId = args->prizeId;
    work->tileBytes = GetMaxSpriteTileBytes(gPooTanpopoFrames, ARRAY_COUNT(gPooTanpopoFrames));
    work->tileBytes2 = GetMaxSpriteTileBytes(gPooTanpopoSeedFrames, ARRAY_COUNT(gPooTanpopoSeedFrames));
    work->palette = NULL;
    AnimInit(&work->anim, gPooTanpopoAnims, gPooTanpopoFrames);
    AnimStart(&work->anim, 0, 0);
    work->gfx = AnimGetGfx(&work->anim);
    AnimInit(&work->anim2, gPooTanpopoSeedAnims, gPooTanpopoSeedFrames);
    AnimStart(&work->anim2, 0, 0);
    work->gfx2 = AnimGetGfx(&work->anim2);
    ColliderSetPosition(&work->collider, work->x + 0x1800, work->y + 0x1000, work->z);
    work->playing = 0;
}

u8 task_poo_tanpopo_1(PooTanpopoWork* work) {
    if (work->palette != NULL && (work->collider.standFlags & COLLIDER_STAND_STOOD_ON) != 0 && work->playing == 0) {
        work->playing = 1;
        AnimReset(&work->anim);
        AnimReset(&work->anim2);

        if (!IsPooPrizeDropped(work->prizeId)) {
            if (SpawnPooPrizes(MAP_PRIZE_MOOGLE_POINTS_SMALL, 1, work->x + 0x1800, work->y + 0x2000, work->z) != 0) {
                SetPooPrizeDropped(work->prizeId);
            }
        }
    }

    return 1;
}

void task_poo_tanpopo_2(PooTanpopoWork* work) {
    u8* playing;
    s16 x;
    s16 y;

    x = (work->x >> 8) - gPooScrollX;
    y = (work->y >> 8) - gPooScrollY;

    if (IsRectOutsideScreen(x, y, 0, 32, 0, 48)) {
        if (work->palette != NULL) {
            ReleaseObjTiles(work->tiles);
            ReleaseObjTiles(work->tiles2);
            ReleaseObjPalette(work->palette);
            work->palette = NULL;
            ColliderUnregister(&work->collider);
            work->playing = 0;
        }
    } else {
        if (work->palette == NULL) {
            work->tiles = LoadObjTiles(gPooTanpopoTiles, sizeof(gPooTanpopoTiles));
            work->tiles2 = LoadObjTiles(gPooTanpopoSeedTiles, sizeof(gPooTanpopoSeedTiles));
            work->palette = LoadObjPalette(gPooTanpopoPalette, sizeof(gPooTanpopoPalette));
            ColliderInit(&work->collider, COLLIDER_TYPE_SOLID, 24, 0);
        }

        playing = &work->playing;

        if (*playing != 0) {
            work->gfx = AnimUpdate(&work->anim);
            work->gfx2 = AnimUpdate(&work->anim2);
            DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), 0xFFF1);

            if (!AnimIsFinished(&work->anim2)) {
                DrawSprite(x, y, work->gfx2, work->tiles2, work->palette, NULL, SPRITE_PRIORITY(2), 100);
            } else if ((work->collider.standFlags & COLLIDER_STAND_STOOD_ON) == 0) {
                *playing = 0;
            }
        }
    }
}

void task_poo_tanpopo_3(PooTanpopoWork* work) {
    if (work->palette != NULL) {
        ReleaseObjPalette(work->palette);
        ReleaseObjTiles(work->tiles);
        ReleaseObjTiles(work->tiles2);
        ColliderUnregister(&work->collider);
    }
}

void task_poo_ti_board_0(PooBoardWork* work, PooPos* pos) {
    work->x = pos->x;
    work->y = pos->y;
    work->z = 0;
    work->tiles = LoadObjTiles(gPooTiBoardTiles, sizeof(gPooTiBoardTiles));
    work->palette = NULL;
    work->gfx = gPooTiBoardFrame0;
    ColliderSetPosition(&work->collider, work->x, work->y, work->z);
}

u8 task_poo_ti_board_1(PooBoardWork* work) {
    if (work->palette != NULL) {
        if (ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_POOH)) {
            gPoohRequest = POOH_REQUEST_BLOCKED;
        }
    }

    return 1;
}

void task_poo_ti_board_2(PooBoardWork* work) {
    s16 x;
    s16 y;

    x = (work->x >> 8) - gPooScrollX;
    y = (work->y >> 8) - gPooScrollY;

    if (IsRectOutsideScreen(x, y, 16, 1, 8, 8)) {
        if (work->palette != NULL) {
            ReleaseObjPalette(work->palette);
            work->palette = NULL;
            ColliderUnregister(&work->collider);
        }
    } else {
        if (work->palette == NULL) {
            work->palette = LoadObjPalette(gPooTiBoardPalette, sizeof(gPooTiBoardPalette));
            ColliderInit(&work->collider, COLLIDER_TYPE_PLATFORM, 8, 16);
        }

        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), -0x1004 - (work->y >> 8) * 4);
    }
}

void task_poo_ti_board_3(PooBoardWork* work) {
    ReleaseObjTiles(work->tiles);

    if (work->palette != NULL) {
        ReleaseObjPalette(work->palette);
        ColliderUnregister(&work->collider);
    }
}

void task_poo_eeyoretail_0(PooEeyoreTailWork* work) {
    work->x = 0x7CD00;
    work->y = 0x49E00;
    work->z = -0x2000;
    work->ground = 0;
    work->tileBytes = GetMaxSpriteTileBytes(gEeyoreFl00Frames, ARRAY_COUNT(gEeyoreFl00Frames));
    work->palette = NULL;
    work->gfx = gEeyoreFl00Frame15;
    TaskPoolInit(&work->tasks, 1);
    CreatePooShadowscaleTask(&work->tasks, &work->x, Q_8_8(0.4));
    sPooEeyoreTailTimer = 0x1E;
    work->height = -work->z;
}

u8 task_poo_eeyoretail_1(PooEeyoreTailWork* work) {
    if (IsPoohBeeChaseOver()) {
        if (sPooEeyoreTailTimer != 0) {
            ApproachValue(&work->x, 0x7FD00, sPooEeyoreTailTimer);
            ApproachValue(&work->y, 0x49300, sPooEeyoreTailTimer);
            ApproachValue(&work->height, 0, sPooEeyoreTailTimer);
            work->z = -work->height;
            sPooEeyoreTailTimer--;
            SetPooCameraFocus(work->x, work->y + work->z);
        }

        TaskPoolUpdate(&work->tasks);
    }

    return 1;
}

void task_poo_eeyoretail_2(PooEeyoreTailWork* work) {
    s16 x;
    s16 y;
    u16 flags;
    s32 z;

    x = ((s32)work->x >> 8) - gPooScrollX;
    y = ((s32)work->y >> 8) + (work->z >> 8) - gPooScrollY;

    if (IsRectOutsideScreen(x, y, 8, 8, 8, 8)) {
        if (work->palette != NULL) {
            ReleaseObjTiles(work->tiles);
            ReleaseObjPalette(work->palette);
            work->palette = NULL;
        }
    } else {
        if (work->palette == NULL) {
            work->tiles = AllocObjTiles(work->tileBytes, gEeyoreFl00Tiles);
            work->palette = LoadObjPalette(gEeyorePalette, sizeof(gEeyorePalette));
        }

        if (IsPooEeyoreTailLanded()) {
            flags = SPRITE_PRIORITY(2);
            z = 0xFFEF;
        } else {
            flags = SPRITE_PRIORITY(1);
            z = 10;
        }

        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, flags, z);

        if (IsPoohBeeChaseOver()) {
            TaskPoolDraw(&work->tasks);
        }
    }
}

void task_poo_eeyoretail_3(PooEeyoreTailWork* work) {
    if (work->palette != NULL) {
        ReleaseObjTiles(work->tiles);
        ReleaseObjPalette(work->palette);
    }

    TaskPoolDestroy(&work->tasks);
}

u8 IsPooEeyoreTailLanded() {
    if (sPooEeyoreTailTimer == 0) {
        return TRUE;
    }

    return FALSE;
}

enum PooHoneycombState {
    POO_HONEYCOMB_STATE_IDLE,
    POO_HONEYCOMB_STATE_SHAKING,
    POO_HONEYCOMB_STATE_SHAKEN
};

void task_poo_honeycomb_0(PooHoneycombWork* work) {
    work->x = 0x8DE00;
    work->y = 0x46600;
    work->z = -0xA00;
    work->ground = 0;
    work->tileBytes = GetMaxSpriteTileBytes(gEeHoneycombFrames, ARRAY_COUNT(gEeHoneycombFrames));
    work->palette = NULL;
    work->gfx = gEeHoneycombFrame0;
    ColliderSetPosition(&work->collider, work->x, work->y, 0);
    work->colliderActive = 0;
    sPooHoneycombState = POO_HONEYCOMB_STATE_IDLE;
    work->shakeX = 0;
    work->angle = 0;
}

u8 task_poo_honeycomb_1(PooHoneycombWork* work) {
    u8 eventDone;

    if (work->colliderActive != 0) {
        switch (sPooHoneycombState) {
        case POO_HONEYCOMB_STATE_SHAKEN:
            break;
        case POO_HONEYCOMB_STATE_IDLE:
            if (ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_POOH)) {
                eventDone = IsPooEventDone(POO_EVENT_EEYORE);

                if (!eventDone) {
                    gPoohRequest = POOH_REQUEST_HONEYCOMB;

                    if (IsPoohLookingAtHoneycomb()) {
                        sPooHoneycombState = POO_HONEYCOMB_STATE_SHAKING;
                        work->shakeTimer = eventDone;
                    }
                } else {
                    gPoohRequest = POOH_REQUEST_HONEYCOMB_DONE;
                }
            }

            break;
        case POO_HONEYCOMB_STATE_SHAKING:
            work->shakeX = gSineTable[(u8)work->angle];
            work->angle += 16;
            work->shakeTimer++;

            if (work->shakeTimer > 60) {
                work->shakeX = 0;
                sPooHoneycombState = POO_HONEYCOMB_STATE_SHAKEN;
                m4aSongNumStart(SONG_SND_371);
            }

            break;
        }
    }

    return 1;
}

void task_poo_honeycomb_2(PooHoneycombWork* work) {
    u8* colliderActive;
    s16 x;
    s16 y;

    x = ((work->x + work->shakeX) >> 8) - gPooScrollX;
    y = (work->y >> 8) + (work->z >> 8) - gPooScrollY;

    if (IsRectOutsideScreen(x, y, 16, 16, 16, 16)) {
        if (work->palette != NULL) {
            ReleaseObjPalette(work->palette);
            ReleaseObjTiles(work->tiles);
            work->palette = NULL;
        }

        colliderActive = &work->colliderActive;

        if (*colliderActive != 0) {
            ColliderUnregister(&work->collider);
            *colliderActive = 0;
        }
    } else {
        if (work->palette == NULL) {
            work->palette = LoadObjPalette(gEeHoneycombPalette, sizeof(gEeHoneycombPalette));
            work->tiles = AllocObjTiles(work->tileBytes, gEeHoneycombTiles);
        }

        colliderActive = &work->colliderActive;

        if (*colliderActive == 0) {
            ColliderInit(&work->collider, COLLIDER_TYPE_SOLID, 64, 0);
            *colliderActive = 1;
        }

        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(3), 0xFFF0);
    }
}

void task_poo_honeycomb_3(PooHoneycombWork* work) {
    if (work->palette != NULL) {
        ReleaseObjTiles(work->tiles);
        ReleaseObjPalette(work->palette);
    }

    if (work->colliderActive != 0) {
        ColliderUnregister(&work->collider);
    }
}

u8 IsPooHoneycombShaken() {
    if (sPooHoneycombState == POO_HONEYCOMB_STATE_SHAKEN) {
        return TRUE;
    }

    return FALSE;
}

void task_poo_vegetable_0(PooVegetableWork* work) {
    work->x = 0x1AC00;
    work->y = 0x18000;
    work->z = 0;
    work->ground = 0;
    work->tileBytes = GetMaxSpriteTileBytes(gRaVegetablesFrames, 1);
    work->palette = NULL;
    work->gfx = gRaVegetablesFrame0;
    ColliderSetPosition(&work->collider, work->x, work->y, work->z);
}

u8 task_poo_vegetable_1(PooVegetableWork* work) {
    if (work->palette != NULL) {
        if (ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_POOH)) {
            gPoohRequest = POOH_REQUEST_BLOCKED;
        }
    }

    return 1;
}

void task_poo_vegetable_2(PooVegetableWork* work) {
    s16 x;
    s16 y;

    x = (work->x >> 8) - gPooScrollX;
    y = (work->y >> 8) - gPooScrollY;

    if (IsRectOutsideScreen(x, y, 32, 40, 48, 48)) {
        if (work->palette != NULL) {
            ReleaseObjTiles(work->tiles);
            ReleaseObjPalette(work->palette);
            work->palette = NULL;
            ColliderUnregister(&work->collider);
        }
    } else {
        if (work->palette == NULL) {
            work->tiles = AllocObjTiles(work->tileBytes, gRaVegetablesTiles);
            work->palette = LoadObjPalette(gRaVegetablesPalette, sizeof(gRaVegetablesPalette));
            ColliderInit(&work->collider, COLLIDER_TYPE_PLATFORM, 0x26, 12);
        }

        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), -0x1004 - (work->y >> 8) * 4);
    }
}

void task_poo_vegetable_3(PooVegetableWork* work) {
    if (work->palette != NULL) {
        ReleaseObjTiles(work->tiles);
        ReleaseObjPalette(work->palette);
        ColliderUnregister(&work->collider);
    }
}

s32 IsInPooWagonArea(PooPos* pos) {
    s32 depth;
    s32 x;
    s32 y;

    depth = 0x2500;

    if (IsPooEventDone(POO_EVENT_WAGON)) {
        depth = 0x2100;
    }

    x = pos->x - sPooWagon->pos.x;
    y = pos->y - sPooWagon->pos.y;

    if (y + x < -0x1A00) {
        return FALSE;
    }

    if (y - x / 2 > depth - 0xB80) {
        return FALSE;
    }

    if (y + x > depth + 0x1700) {
        return FALSE;
    }

    if (y - x / 2 < -0x1180) {
        return FALSE;
    }

    return TRUE;
}

s32 IsInPooWagonAreaForPooh(PooPos* pos) {
    s32 depth;
    s32 x;
    s32 y;

    depth = 0x2500;

    if (IsPooEventDone(POO_EVENT_WAGON)) {
        depth = 0x2100;
    }

    x = pos->x - sPooWagon->pos.x;
    y = pos->y - sPooWagon->pos.y;

    if (y + x < -0x1A00) {
        return FALSE;
    }

    if (y - x / 2 > depth - 0xB80) {
        return FALSE;
    }

    if (y + x > 0x3100) {
        return FALSE;
    }

    if (y - x / 2 < -0x1180) {
        return FALSE;
    }

    return TRUE;
}

u8 IsPooSoraOnWagon() {
    if (gPooActor.pos.z < 0) {
        return FALSE;
    }

    return IsInPooWagonArea(&gPooActor.pos);
}

u8 IsPooSoraOverWagon() {
    if (gPooActor.pos.z < -0x2000) {
        return FALSE;
    }

    return IsInPooWagonArea(&gPooActor.pos);
}

void SnapToPooWagonLine(u32* px, u32* py, u16 steps) {
    s32 offsetX;
    s32 offsetY;

    offsetX = *px - sPooWagon->pos.x;

    if (offsetX < -0x600) {
        offsetX = -0x600;
    } else if (offsetX > 0xA00) {
        offsetX = 0xA00;
    }

    offsetY = offsetX / 2 - 0x300;
    ApproachValue(px, offsetX + sPooWagon->pos.x, steps);
    ApproachValue(py, offsetY + sPooWagon->pos.y, steps);
}

void ProjectToPooWagonEdgeUL(s32* px, s32* py) {
    s32 x;
    s32 y;

    x = *px;
    y = *py;
    x -= 0x4600;
    *px = (x - y * 2) / 5;
    *py = -0x2300 - *px * 2;
}

void ProjectToPooWagonEdgeLR(s32* px, s32* py) {
    s32 x;
    s32 y;

    x = *px;
    y = *py;
    x += 0x5000;
    *px = (x - y * 2) / 5;
    *py = 0x2800 - *px * 2;
}

void ProjectToPooWagonEdgeUR(s32* px, s32* py) {
    s32 x;
    s32 y;

    x = *px;
    y = *py;
    *px = (y * 2 + x * 4 + 0xF00) / 5;
    *py = *px / 2 - 0x800;
}

void ProjectToPooWagonEdgeLL(s32* px, s32* py) {
    s32 x;
    s32 y;

    x = *px;
    y = *py;
    *px = (y * 2 + x * 4 - 1280) / 5;
    *py = *px / 2 + 768;
}

void ClampToPooWagonArea(u32* px, u32* py, u16 steps) {
    s32 x;
    s32 y;

    x = *px - sPooWagon->pos.x;
    y = *py - sPooWagon->pos.y;

    if (x < -0xB00 && y < -0xD00) {
        ProjectToPooWagonEdgeUL(&x, &y);

        if (y < -0xD00) {
            ProjectToPooWagonEdgeUR(&x, &y);
        } else if (y > -0xD00) {
            ProjectToPooWagonEdgeLL(&x, &y);
        }
    } else if (x > 0xF00 && y > 0x600) {
        ProjectToPooWagonEdgeLR(&x, &y);

        if (y < 0x600) {
            ProjectToPooWagonEdgeUR(&x, &y);
        } else if (y > 0xA00) {
            ProjectToPooWagonEdgeLL(&x, &y);
        }
    } else if (-x / 2 + y > 0) {
        ProjectToPooWagonEdgeLL(&x, &y);

        if (x < -0x1700) {
            ProjectToPooWagonEdgeUL(&x, &y);
        } else if (x > 0xF00) {
            ProjectToPooWagonEdgeLR(&x, &y);
        }
    } else {
        ProjectToPooWagonEdgeUR(&x, &y);

        if (x < -0xB00) {
            ProjectToPooWagonEdgeUL(&x, &y);
        } else if (x > 0x1800) {
            ProjectToPooWagonEdgeLR(&x, &y);
        }
    }

    ApproachValue(px, x + sPooWagon->pos.x, steps);
    ApproachValue(py, y + sPooWagon->pos.y, steps);
}

s32 GetPooWagonNearestSide(s32 x, s32 y) {
    s32 dx;
    s32 dy;

    dx = x - sPooWagon->pos.x;
    dy = y - sPooWagon->pos.y;

    if (dx < -0xB00 && dy < -0xD00) {
        return FLD_ANGLE_DOWN_RIGHT;
    }

    if (dx > 0xF00 && dy > 0x600) {
        return FLD_ANGLE_UP_LEFT;
    }

    if (-dx / 2 + dy > 0) {
        return FLD_ANGLE_UP_RIGHT;
    }

    return FLD_ANGLE_DOWN_LEFT;
}

s32 GetPooWagonSide(s32 posX, s32 posY) {
    s32 x;
    s32 y;

    x = posX - sPooWagon->pos.x;
    y = posY - sPooWagon->pos.y;

    if (x < -0x3D00 || x > 0x4300 || y < -0x1F00 || y > 0x3800) {
        return 0;
    }

    if (x < -0x1100 && y <= 0x2FF) {
        return FLD_ANGLE_DOWN_RIGHT;
    }

    if (x > 0x1700 && y > 0x500) {
        return FLD_ANGLE_UP_LEFT;
    }

    if (-x / 2 + y > 0) {
        return FLD_ANGLE_UP_RIGHT;
    }

    return FLD_ANGLE_DOWN_LEFT;
}

void task_poo_wagon_0(PooWagonWork* work) {
    sPooWagon = work;
    work->pos.x = 0x2AE00;
    work->pos.y = 0x17700;
    work->pos.z = 0;
    work->pos.ground = 0;
    work->pos2 = work->pos;

    if (IsPooEventDone(POO_EVENT_WAGON)) {
        work->pos.y += 0xC00;
    }

    work->palette = NULL;
    work->gfx = gRaWagonFrame11;
    work->gfx2 = gRaWagonFrame1;
    work->gfx3 = gRaWagonFrame12;
    work->poohAboard = FALSE;
    work->timer = 0;
    work->angle = 0;
}

u8 task_poo_wagon_1(PooWagonWork* work) {
    u8 poohInArea;
    s32 tippedY;
    s32 delta;

    if (IsPooSoraOnWagon()) {
        if (work->pos.y == work->pos2.y) {
            work->pos.y += 0x100;
            gPooActor.pos.y += 0x100;

            if (work->poohAboard) {
                gPoohPos->y += 0x100;
            }
        }
    } else if (!IsPooEventDone(POO_EVENT_WAGON)) {
        if (work->pos.y != work->pos2.y) {
            work->pos.y -= 0x100;
            gPooActor.pos.y -= 0x100;

            if (work->poohAboard) {
                gPoohPos->y -= 0x100;
            }
        }
    }

    poohInArea = IsInPooWagonAreaForPooh(gPoohPos);

    if (poohInArea) {
        if (!work->poohAboard) {
            gPoohRequestX = work->pos.x;
            gPoohRequestY = work->pos.y;
            gPoohRequest = POOH_REQUEST_WAGON;
            work->poohAboard = TRUE;
        }
    } else {
        work->poohAboard = FALSE;
    }

    if (IsPooSoraOnWagon() && IsPoohWaitingOnWagon() && !IsPooEventDone(POO_EVENT_WAGON)) {
        work->timer++;

        if (work->timer > 100) {
            if (work->pos.y != work->pos2.y + 0xC00) {
                tippedY = work->pos.y - 0xC00;
                delta = work->pos2.y - tippedY;
                work->pos.y += delta;
                gPooActor.pos.y += delta;
                gPoohPos->y += delta;
                SetPooEventDone(POO_EVENT_WAGON);
                m4aSongNumStart(SONG_SYS_OBJ_BREAK);
                work->timer = 0;
            }
        }
    } else {
        work->timer = 0;
    }

    return 1;
}

void task_poo_wagon_2(PooWagonWork* work) {
    s32 shakeX;
    s16 x;
    s16 y;
    u16 soraPriority;
    u16 priority;
    s32 side;
    s32 baseY;

    side = 0;

    if (work->timer != 0) {
        shakeX = gSineTable[(u8)work->angle];
        work->angle += 16;
    } else {
        shakeX = 0;
    }

    x = ((work->pos.x + shakeX) >> 8) - gPooScrollX;
    y = (work->pos.y >> 8) + (work->pos.z >> 8) - gPooScrollY;

    if (IsRectOutsideScreen(x, y, 32, 40, 48, 48)) {
        if (work->palette != NULL) {
            ReleaseObjTiles(work->tiles);
            ReleaseObjTiles(work->tiles2);
            ReleaseObjTiles(work->tiles3);
            ReleaseObjPalette(work->palette);
            work->palette = NULL;
        }

        return;
    }

    if (work->palette == NULL) {
        work->tiles = AllocObjTiles(0x560, gRaWagonTiles);
        work->tiles2 = AllocObjTiles(0x4C0, gRaWagonTiles);
        work->tiles3 = AllocObjTiles(160, gRaWagonTiles);
        work->palette = LoadObjPalette(gRaWagonPalette, sizeof(gRaWagonPalette));
    }

    soraPriority = GetPooSoraPriority();

    if (IsPooSoraOverWagon()) {
        sPooWagonPriority = soraPriority + 3;
        sPooWagonPriority2 = soraPriority - 1;

        if (IsPoohOnWagon()) {
            if (gPooActor.pos.y >= gPoohPos->y) {
                sPooWagonPriority += 6;
            } else {
                sPooWagonPriority2 += 0xFFFC;
            }
        }
    } else {
        side = (u8)GetPooWagonSide(gPooActor.pos.x, gPooActor.pos.y);

        if (side == 0) {
            baseY = work->pos.y + 0x300;
            sPooWagonPriority = -0x1004 - (baseY >> 8) * 4;
            sPooWagonPriority2 = -0x1009 - (baseY >> 8) * 4;
        } else if (side == FLD_ANGLE_DOWN_RIGHT || side == FLD_ANGLE_DOWN_LEFT) {
            sPooWagonPriority = soraPriority - 3;
            sPooWagonPriority2 = soraPriority - 8;
        } else {
            sPooWagonPriority2 = soraPriority + 4;
            sPooWagonPriority = soraPriority + 9;
        }
    }

    DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), sPooWagonPriority);
    DrawSprite(x, y, work->gfx2, work->tiles2, work->palette, NULL, SPRITE_PRIORITY(2), sPooWagonPriority2);
    priority = -0x1002 - ((work->pos.y - 0xE00) >> 8) * 4;

    if (!IsPooSoraOverWagon() && soraPriority > priority && (side == FLD_ANGLE_DOWN_RIGHT || side == FLD_ANGLE_DOWN_LEFT)) {
        priority = soraPriority - 1;
    }

    DrawSprite(x, y, work->gfx3, work->tiles3, work->palette, NULL, SPRITE_PRIORITY(2), priority);
}

void task_poo_wagon_3(PooWagonWork* work) {
    if (work->palette != NULL) {
        ReleaseObjTiles(work->tiles);
        ReleaseObjTiles(work->tiles2);
        ReleaseObjTiles(work->tiles3);
        ReleaseObjPalette(work->palette);
    }
}

u16 GetPooWagonPriority2() {
    return sPooWagonPriority2;
}

u16 GetPooWagonPriority() {
    return sPooWagonPriority;
}

void task_poo_wagonwheel_0(PooWheelWork* work) {
    s16 x;
    s16 y;

    if (!IsPooEventDone(POO_EVENT_WAGON)) {
        work->x = 0x2A800;
        work->y = 0x18D00;
        work->animId = 2;
        work->gfx = gRaWagonFrame3;
    } else {
        GetPooStateWheelPos(&x, &y);
        work->x = x << 8;
        work->y = y << 8;
        work->animId = 4;
        work->gfx = gRaWagonFrame10;
    }

    work->startX = work->x;
    work->z = 0;
    work->ground = 0;
    work->tileBytes = 0x180;
    work->palette = NULL;
    AnimInit(&work->anim, gRaWagonAnims, gRaWagonFrames);
    AnimStart(&work->anim, work->animId, ANIM_FLAG_LOOP);
    work->speed = 0;
    work->removeWhenOffscreen = FALSE;
}

u8 task_poo_wagonwheel_1(PooWheelWork* work) {
    if (work->animId == 2 && IsPooEventDone(POO_EVENT_WAGON)) {
        work->animId = 3;
        AnimStart(&work->anim, 3, ANIM_FLAG_LOOP);
    }

    if (work->animId == 3) {
        work->gfx = AnimUpdate(&work->anim);

        if (work->speed <= 0x4FF) {
            work->speed += 6;
        }

        work->y += work->speed;
        work->x += work->speed;

        if (work->x > work->startX + 0x4800) {
            work->x = work->startX + 0x4800;
        }
    }

    if (work->removeWhenOffscreen && work->palette == NULL) {
        return 0;
    }

    return 1;
}

void task_poo_wagonwheel_2(PooWheelWork* work) {
    u16* priority;
    s16 x;
    s16 y;

    x = (work->x >> 8) - gPooScrollX;
    y = (work->y >> 8) + (work->z >> 8) - gPooScrollY;

    if (IsRectOutsideScreen(x, y, 32, 0, 16, 16)) {
        if (work->palette != NULL) {
            ReleaseObjTiles(work->tiles);
            ReleaseObjPalette(work->palette);
            work->palette = NULL;

            if (work->animId == 3) {
                work->animId = 4;
                work->gfx = gRaWagonFrame10;
            }
        }
    } else {
        if (work->palette == NULL) {
            work->tiles = AllocObjTiles(work->tileBytes, gRaWagonTiles);
            work->palette = LoadObjPalette(gRaWagonPalette, sizeof(gRaWagonPalette));
        }

        if (!IsPooEventDone(POO_EVENT_WAGON)) {
            priority = &sPooWagonWheelPriority;
            *priority = GetPooWagonPriority2() - 1;
        } else {
            sPooWagonWheelPriority = -0x1004 - (work->y >> 8) * 4;
        }

        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), sPooWagonWheelPriority);
    }
}

void task_poo_wagonwheel_3(PooWheelWork* work) {
    if (IsPooEventDone(POO_EVENT_WAGON)) {
        SetPooStateWheelPos(work->x >> 8, work->y >> 8);
    }

    if (work->palette != NULL) {
        ReleaseObjTiles(work->tiles);
        ReleaseObjPalette(work->palette);
    }
}

void task_poo_spark_0(PooSparkWork* work, PooPos* pos) {
    work->pos = *pos;
    work->tiles = AllocObjTiles(0x200, gMapSparkTiles);
    work->palette = LoadObjPalette(gCommonObjPalette, sizeof(gCommonObjPalette));
    AnimInit(&work->anim, gMapSparkAnims, gMapSparkFrames);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
}

u8 task_poo_spark_1(PooSparkWork* work) {
    AnimUpdate(&work->anim);

    if (AnimIsFinished(&work->anim)) {
        return 0;
    }

    return 1;
}

void task_poo_spark_2(PooSparkWork* work) {
    u16 x;
    u16 y;

    x = (work->pos.x >> 8) - gPooScrollX;
    y = (work->pos.y >> 8) + (work->pos.z >> 8) - gPooScrollY;
    DrawSprite(x, y, AnimGetGfx(&work->anim), work->tiles, work->palette, NULL, SPRITE_PRIORITY(1), 0x50);
}

void task_poo_spark_3(PooSparkWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

void task_poo_bee_0(PooBeeWork* work) {
    void* anims;
    void* gfxTable;
    s32 i;

    work->x = 0x8DE00;
    work->y = 0x46600;
    work->z = -0xA00;
    work->ground = 0;
    i = 0;
    anims = gEeBeeAnims;
    gfxTable = gEeBeeFrames;

    for (; i < 4; i++) {
        work->sub[i].x = -0x500;
        work->sub[i].y = 0x500;
        work->sub[i].targetX = gPooBeePoints[i].x;
        work->sub[i].targetY = gPooBeePoints[i].y;
    }

    AnimInit(&work->anim, anims, gfxTable);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    work->palette = NULL;
    sPooBeeCount = 0;
    work->releaseTimer = 8;
    work->setupPending = TRUE;
}

u8 task_poo_bee_1(PooBeeWork* work) {
    s32 i;

    if (IsPooHoneycombShaken()) {
        if (work->setupPending) {
            work->setupPending = FALSE;

            for (i = 0; i < 4; i++) {
                work->sub[i].x = work->x - 0x500;
                work->sub[i].y = work->y + 0x500;
                work->sub[i].z = work->z;
                work->sub[i].targetX = 0x2000 + gPoohPos->x + gPooBeePoints[i].x;
                work->sub[i].targetY = -0x2000 + gPoohPos->y + gPooBeePoints[i].y;
            }
        }

        if (sPooBeeCount < ARRAY_COUNT(work->sub)) {
            ApproachValue(&work->sub[sPooBeeCount].x, work->sub[sPooBeeCount].targetX, work->releaseTimer);
            ApproachValue(&work->sub[sPooBeeCount].y, work->sub[sPooBeeCount].targetY, work->releaseTimer);
            work->releaseTimer--;

            if (work->releaseTimer == 0) {
                work->releaseTimer = 8;
                sPooBeeCount++;

                if (sPooBeeCount > 3) {
                    work->x = gPoohPos->x + 0x2000;
                    work->y = gPoohPos->y - 0x2000;
                    work->dx = work->dy = 0;
                }
            }

            SetPooCameraFocus(work->x, work->y + work->z);
        } else {
            work->dx = work->x - (gPoohPos->x + 0x2000);
            work->dy = work->y - (gPoohPos->y - 0x2000);
            work->x -= work->dx;
            work->y -= work->dy;

            for (i = 0; i < 4; i++) {
                work->sub[i].x -= work->dx;
                work->sub[i].y -= work->dy;
            }

            if (!IsPoohBeeChaseOver()) {
                SetPooCameraFocus(work->x, work->y + work->z);
            }
        }

        work->gfx = AnimUpdate(&work->anim);
    }

    return 1;
}

void task_poo_bee_2(PooBeeWork* work) {
    s32 x;
    s32 y;
    s32 subX;
    s32 subY;
    s32 i;

    if (!IsPooHoneycombShaken()) {
        return;
    }

    x = (work->x >> 8) - gPooScrollX;
    y = (work->y >> 8) + (work->z >> 8) - gPooScrollY;

    if (IsRectOutsideScreen(x, y, 19, 17, 46, 16)) {
        if (work->palette != NULL) {
            ReleaseObjPalette(work->palette);
            ReleaseObjTiles(work->tiles);
            work->palette = NULL;
        }

        return;
    }

    if (work->palette == NULL) {
        work->palette = LoadObjPalette(gEeBeePalette, sizeof(gEeBeePalette));
        work->tiles = LoadObjTiles(gEeBeeTiles, sizeof(gEeBeeTiles));
    }

    for (i = 0; i < sPooBeeCount + 1 && i <= 3; i++) {
        subX = (work->sub[i].x >> 8) - gPooScrollX;
        subY = (work->sub[i].y >> 8) + (work->sub[i].z >> 8) - gPooScrollY;
        DrawSprite(subX, subY, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), i - ((work->y >> 8) * 4 + 0x1003));
    }
}

void task_poo_bee_3(PooBeeWork* work) {
    if (work->palette != NULL) {
        ReleaseObjTiles(work->tiles);
        ReleaseObjPalette(work->palette);
    }
}

u8 AreAllPooBeesOut() {
    if (sPooBeeCount <= 3) {
        return FALSE;
    }

    return TRUE;
}

void task_poo_beeAfterEvent_0(PooBeeAfterEventWork* work) {
    sPooBeeAfterEventWork = work;
    work->x = 0x8DE00;
    work->y = 0x46600;
    work->z = -0xA00;
    work->ground = 0;
    AnimInit(&work->anim, gEeBeeAnims, gEeBeeFrames);
    AnimStart(&work->anim, 1, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    AnimInit(&work->anim2, gEeBeeAnims, gEeBeeFrames);
    AnimStart(&work->anim2, 0, ANIM_FLAG_LOOP);
    work->gfx2 = AnimGetGfx(&work->anim2);
    work->palette = NULL;
}

u8 task_poo_beeAfterEvent_1(PooBeeAfterEventWork* work) {
    work->gfx = AnimUpdate(&work->anim);
    work->gfx2 = AnimUpdate(&work->anim2);
    return 1;
}

void task_poo_beeAfterEvent_2(PooBeeAfterEventWork* work) {
    s16 x;
    s16 y;

    x = (work->x >> 8) - gPooScrollX;
    y = (work->y >> 8) + (work->z >> 8) - gPooScrollY;

    if (IsRectOutsideScreen(x, y, 19, 17, 46, 16)) {
        if (work->palette != NULL) {
            ReleaseObjPalette(work->palette);
            ReleaseObjTiles(work->tiles);
            ReleaseObjTiles(work->tiles2);
            work->palette = NULL;

            if (!IsPooMapBeeVisible()) {
                m4aSongNumStop(SONG_SND_386);
            }
        }
    } else {
        if (work->palette == NULL) {
            work->palette = LoadObjPalette(gEeBeePalette, sizeof(gEeBeePalette));
            work->tiles = LoadObjTiles(gEeBeeTiles, sizeof(gEeBeeTiles));
            work->tiles2 = LoadObjTiles(gEeBeeTiles, sizeof(gEeBeeTiles));
            m4aSongNumStart(SONG_SND_386);
        }

        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), -0x1002 - (work->y >> 8) * 4);
        DrawSprite(x - 5, y + 5, work->gfx2, work->tiles2, work->palette, NULL, SPRITE_PRIORITY(2), -0x1003 - (work->y >> 8) * 4);
    }
}

void task_poo_beeAfterEvent_3(PooBeeAfterEventWork* work) {
    if (work->palette != NULL) {
        ReleaseObjTiles(work->tiles);
        ReleaseObjTiles(work->tiles2);
        ReleaseObjPalette(work->palette);
    }
}

u8 IsPooBeeAfterEventVisible() {
    if (IsPooEventDone(POO_EVENT_EEYORE)) {
        if (sPooBeeAfterEventWork->palette != NULL) {
            return TRUE;
        }
    }

    return FALSE;
}

void GetPooCabbageStackSpot(PooSpot* spot) {
    PooSpot spots[18];
    u16 i;

    memcpy(spots, gPooCabbageStackOffsets, sizeof(spots));
    i = GetPooCabbageCount();

    if (i > 13) {
        i += GetRandom() % 4;
    }

    spot->x = spots[i].x + 0xAB300;
    spot->y = spots[i].y + 0x57100;
    spot->z = spots[i].z;
}

enum PooCabbageState {
    POO_CABBAGE_STATE_STACKED = 1,
    POO_CABBAGE_STATE_BOUNCE,
    POO_CABBAGE_STATE_FLY_TO_STACK,
    POO_CABBAGE_STATE_LAND
};

void task_poo_cabbage_0(PooCabbageWork* work) {
    u16 randomValue;

    work->x = 0x98300;
    work->y = 0x4D100;
    work->z = 0;
    work->vz = 0x4CC;
    randomValue = GetRandom();
    work->angle = (randomValue & 15) + 88;
    work->speed = 0x1CC;
    work->palette = NULL;
    AnimInit(&work->anim, gRaVegetablesAnims, gRaVegetablesFrames);
    work->state = POO_CABBAGE_STATE_BOUNCE;
    AnimStart(&work->anim, 2, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    ColliderInit(&work->collider, COLLIDER_TYPE_POOH_OBJECT, 8, 16);
    ColliderSetPosition(&work->collider, work->x, work->y, work->z);
    work->colliderActive = TRUE;
    TaskPoolInit(&work->tasks, 2);
    CreatePooShadowscaleTask(&work->tasks, &work->x, Q_8_8(0.5));
    work->task = NULL;
    work->age = 0;
    work->wasOnScreen = FALSE;
    work->animating = TRUE;
}

u8 task_poo_cabbage_1(PooCabbageWork* work) {
    u16 removeCounts[15];
    u16 sx;
    s16 sy;
    s32 screenX;

    memcpy(removeCounts, gPooCabbageRemoveCounts, sizeof(removeCounts));
    work->age++;

    switch (work->state) {
    case POO_CABBAGE_STATE_BOUNCE:
        if (gPooAttackActive && PooAttackHitsCollider(&work->collider)) {
            if (IsTaskActive(work->task)) {
                TaskKill(&work->tasks, work->task);
            }

            work->task = TaskCreate(&work->tasks, &gTaskDescPooSpark, &work->x);
            work->state = POO_CABBAGE_STATE_FLY_TO_STACK;
            AnimStart(&work->anim, 3, 0);
            m4aSongNumStart(SONG_SND_222);
            work->moveTimer = 30;
            work->zTimer = 20;
            work->hopHeight = -0x2000;
            GetPooCabbageStackSpot((PooSpot*)&work->targetX);
            work->stackIndex = GetPooCabbageCount();
            IncPooCabbageCount();

            if (work->colliderActive) {
                ColliderUnregister(&work->collider);
                work->colliderActive = FALSE;
            }
        } else {
            work->x += gSineTable[work->angle] * work->speed >> 8;
            work->y += -gSineTable[work->angle + 0x40] * work->speed >> 8;
            work->vz += 51;
            work->z += work->vz;

            if (work->z > 0) {
                work->z = 0;
                work->vz = -(work->vz * 179 >> 8);
            }

            if (work->wasOnScreen) {
                screenX = (work->x >> 8) - gPooScrollX;
                sy = (work->y >> 8) + (work->z >> 8) - gPooScrollY;
                sx = screenX;

                if ((u16)(sx + 16) > 272 || sy < -36 || sy > 196) {
                    return 0;
                }
            }
        }

        break;
    case POO_CABBAGE_STATE_FLY_TO_STACK:
        if (work->zTimer != 0) {
            ApproachValue(&work->z, work->targetZ + work->hopHeight, work->zTimer);
            work->zTimer--;

            if (work->zTimer == 0 && work->hopHeight < 0) {
                work->zTimer = 10;
                work->hopHeight = 0;
            }
        }

        ApproachValue(&work->x, work->targetX, work->moveTimer);
        ApproachValue(&work->y, work->targetY, work->moveTimer);
        work->moveTimer--;

        if (work->moveTimer == 0) {
            work->state = POO_CABBAGE_STATE_LAND;
            AnimStart(&work->anim, 4, 0);
        }

        break;
    case POO_CABBAGE_STATE_LAND:
        if (!IsPooEventDone(POO_EVENT_RABBIT) && work->stackIndex == 13) {
            ExitPoohMode(EVENT_142_100ACREWOOD_LV6);
            SetPooEventDone(POO_EVENT_RABBIT);
            SetJiminyFlag(JIMINY_RECORD_CHARACTER_RABBIT);
        }

        if (AnimIsFinished(&work->anim)) {
            work->state = POO_CABBAGE_STATE_STACKED;
            AnimStart(&work->anim, 1, 0);
            IncPooCabbageLandedCount();
            work->animating = FALSE;

            if (work->stackIndex == 5) {
                work->x = 0xAB300;
                work->y = 0x57100;
                work->z = 0;
                work->gfx = gRaVegetablesFrame10;
            } else if (work->stackIndex == 8) {
                work->x = 0xAB300;
                work->y = 0x57100;
                work->z = 0;
                work->gfx = gRaVegetablesFrame11;
            } else {
                work->gfx = gRaVegetablesFrame1;
            }
        }

        break;
    case POO_CABBAGE_STATE_STACKED:
        if (removeCounts[work->stackIndex] < GetPooCabbageLandedCount()) {
            return 0;
        }

        break;
    }

    return 1;
}

void task_poo_cabbage_2(PooCabbageWork* work) {
    u16 stackPriorities[5];
    u16 z;
    s16 x;
    s16 y;

    memcpy(stackPriorities, gPooCabbageStackPriorities, sizeof(stackPriorities));
    x = (work->x >> 8) - gPooScrollX;
    y = (work->y >> 8) + (work->z >> 8) - gPooScrollY;

    if (x < -16 || x > 256 || y < -36 || y > 196) {
        if (work->palette != NULL) {
            ReleaseObjTiles(work->tiles);
            ReleaseObjPalette(work->palette);
            work->palette = NULL;
        }
    } else {
        if (work->palette == NULL) {
            work->tiles = LoadObjTiles(gRaVegetablesTiles, sizeof(gRaVegetablesTiles));
            work->palette = LoadObjPalette(gRaVegetablesPalette, sizeof(gRaVegetablesPalette));
        }

        if (work->animating) {
            work->gfx = AnimUpdate(&work->anim);
        }

        work->wasOnScreen = TRUE;

        if (work->colliderActive) {
            ColliderSetPosition(&work->collider, work->x, work->y, work->z);
        }

        if (work->stackIndex < 9 || work->stackIndex > 13) {
            z = -0x1004 - (work->y >> 8) * 4;
        } else {
            z = 0xDA38 - stackPriorities[work->stackIndex - 9];
        }

        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), z);

        if (work->state != POO_CABBAGE_STATE_LAND && work->state != POO_CABBAGE_STATE_STACKED) {
            TaskPoolUpdate(&work->tasks);
            TaskPoolDraw(&work->tasks);
        }
    }
}

void task_poo_cabbage_3(PooCabbageWork* work) {
    if (work->palette != NULL) {
        ReleaseObjTiles(work->tiles);
        ReleaseObjPalette(work->palette);
    }

    if (work->colliderActive) {
        ColliderUnregister(&work->collider);
    }

    TaskPoolDestroy(&work->tasks);
}

void task_poo_cabbageborn_0(PooCabbageBornWork* work) {
    TaskPoolInit(&work->tasks, 0x20);
    work->unk_14 = 0;
    work->timer = 0;
    sPooCabbageCount = 0;
    sPooCabbageLandedCount = 0;
}

u8 CanSpawnPooCabbage() {
    if (IsPooEventDone(POO_EVENT_WAGON) && !IsPoohOffScreen() && gPooScrollX > 0x9EB && gPooScrollX <= 0xA8A && gPooScrollY <= 0x548 && gPooScrollY > 0x4F9) {
        return TRUE;
    }

    return FALSE;
}

u8 task_poo_cabbageborn_1(PooCabbageBornWork* work) {
    if (CanSpawnPooCabbage() && work->timer == 0) {
        TaskCreate(&work->tasks, &gTaskDescPooCabbage, NULL);
        work->timer = 40;
    }

    if (work->timer != 0) {
        work->timer--;
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

void task_poo_cabbageborn_2(PooCabbageBornWork* work) {
    TaskPoolDraw(&work->tasks);
}

void task_poo_cabbageborn_3(PooCabbageBornWork* work) {
    TaskPoolDestroy(&work->tasks);
}

void IncPooCabbageCount() {
    if (sPooCabbageCount <= 13) {
        sPooCabbageCount++;
    }
}

u16 GetPooCabbageCount() {
    return sPooCabbageCount;
}

void IncPooCabbageLandedCount() {
    sPooCabbageLandedCount++;
}

u16 GetPooCabbageLandedCount() {
    return sPooCabbageLandedCount;
}

u8 IsPooCabbageGameActive() {
    if (!IsPooEventDone(POO_EVENT_RABBIT)) {
        if (IsPooEventDone(POO_EVENT_WAGON)) {
            if (gPooScrollY > 0x4F9) {
                return TRUE;
            }
        }
    }

    return FALSE;
}

void task_poo_mapobjhit_0(PooMapObjHitWork* work, PooMapObjHitArgs* args) {
    work->x = args->x;
    work->y = args->y;
    work->z = 0;
    work->kind = args->kind;
    work->prizeId = args->prizeId;
    work->desc = args->desc;
    work->tileBytes = GetMaxSpriteTileBytes(work->desc->gfxTable, work->desc->gfxCount);
    work->palette = NULL;
    AnimInit(&work->anim, work->desc->anims, work->desc->gfxTable);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    work->collider.radius = 0x1000;
    work->collider.height = 0x1800;
    ColliderSetPosition(&work->collider, work->x + 0x1000, work->y + 0xC00, work->z);
    work->playing = FALSE;
}

u8 task_poo_mapobjhit_1(PooMapObjHitWork* work) {
    SetPooRabbitTalkBlocked(FALSE);

    if (gPooAttackActive) {
        if (PooAttackHitsCollider(&work->collider)) {
            if (!work->playing) {
                work->playing = TRUE;
                AnimReset(&work->anim);

                if (!IsPooPrizeDropped(work->prizeId)) {
                    if (SpawnPooPrizes(MAP_PRIZE_MOOGLE_POINTS_SMALL, 1, work->x + 0x1000, work->y + 0x1800, work->z) != 0) {
                        SetPooPrizeDropped(work->prizeId);
                    }
                }

                if (work->kind == 4) {
                    m4aSongNumStart(SONG_SYS_PO_WOOD);
                } else {
                    m4aSongNumStart(SONG_SND_224);
                }
            }
        }
    }

    if (work->playing) {
        work->gfx = AnimUpdate(&work->anim);

        if (AnimIsFinished(&work->anim)) {
            work->playing = FALSE;
        }
    }

    return 1;
}

void task_poo_mapobjhit_2(PooMapObjHitWork* work) {
    s16 x;
    s16 y;
    s32 flags;

    x = (work->x >> 8) - gPooScrollX;
    y = (work->y >> 8) - gPooScrollY;

    if (!work->playing || IsRectOutsideScreen(x, y, 0, 24, 0, 32)) {
        if (work->palette != NULL) {
            ReleaseObjTiles(work->tiles);
            ReleaseObjPalette(work->palette);
            work->palette = NULL;
        }
    } else {
        if (work->palette == NULL) {
            work->tiles = AllocObjTiles(work->tileBytes, work->desc->tiles);
            work->palette = LoadObjPalette(work->desc->palette, 0x20);
        }

        flags = SPRITE_PRIORITY(2);
        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, flags, 0xFFF1);
        SetPooRabbitTalkBlocked(TRUE);
    }
}

void task_poo_mapobjhit_3(PooMapObjHitWork* work) {
    if (work->palette != NULL) {
        ReleaseObjPalette(work->palette);
        ReleaseObjTiles(work->tiles);
    }
}

void CreatePooMapobjhitTask(void* pool, u32 kind, s32 x, s32 y, u16 prizeId) {
    PooMapObjHitArgs args;

    args.x = x;
    args.y = y;
    args.desc = &gPooMapObjHitDescs[kind];
    args.kind = kind;
    args.prizeId = prizeId;
    TaskCreate(pool, &gTaskDescPooMapobjhit, &args);
}

void PooPrizeUpdateBounce(PooPrizeWork* work) {
    u8 visible;

    work->vz += 56;
    work->z += work->vz;
    work->x += gSineTable[work->angle] * work->speed >> 8;
    work->y += -gSineTable[work->angle + 0x40] * work->speed >> 8;

    if (IsPooPosBlocked((PooPos*)work)) {
        work->angle = (u8)(work->angle + 100) + GetRandom() % 57;
    } else {
        work->ground = 0;
    }

    if (work->z > work->ground) {
        work->z = work->ground;
        work->vz = -(work->vz * 179 >> 8);
        work->speed = work->speed * 212 >> 8;
    }

    if (work->collider.colliding && ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_PLAYER)) {
        switch (work->kind) {
        case MAP_PRIZE_MOOGLE_POINTS_SMALL:
        case MAP_PRIZE_MOOGLE_POINTS_LARGE:
            m4aSongNumStart(SONG_SYS_POWER_GET);
            gGameState.progression.mooglePoints += work->amount;

            if (gGameState.progression.mooglePoints > 99999) {
                gGameState.progression.mooglePoints = 99999;
            }
#ifdef PLATFORM_PC
            {
                extern void PortMoogleNotice(unsigned amount, unsigned total);
                PortMoogleNotice(work->amount, gGameState.progression.mooglePoints);
            }
#endif

            break;
        case MAP_PRIZE_HP_SMALL:
        case MAP_PRIZE_HP_LARGE:
        default:
            m4aSongNumStart(SONG_SYS_POWER_GET);
            gGameState.hp += work->amount;

            if (gGameState.hp > gGameState.progression.maxHp) {
                gGameState.hp = gGameState.progression.maxHp;
            }

            break;
        }

        work->update = PooPrizeUpdateCollect;
        work->timer = 0;
        work->angle = GetAngle(gPooActor.pos.x, gPooActor.pos.y, work->x, work->y);
        work->collected = TRUE;
        work->visible = TRUE;
        work->spin = GetRandom() % 6 + 5;
        ColliderSetDisabled(&work->collider, TRUE);
    } else {
        ColliderSetPosition(&work->collider, work->x, work->y, work->z);

        if (work->timer == 20) {
            ColliderSetDisabled(&work->collider, FALSE);
        }

        if (work->timer > 420) {
            visible = FALSE;

            if (!work->visible) {
                visible = TRUE;
            }

            work->visible = visible;
        }

        if (work->timer++ > 480) {
            work->update = NULL;
        }
    }
}

void PooPrizeUpdateCollect(PooPrizeWork* work) {
    PooPos* actorPos;
    const s16* sine;
    s32 tx;
    s32 ty;
    s32 tz;
    s32 orbitY;
    u8 angle;

    actorPos = &gPooActor.pos;
    sine = gSineTable;
    angle = work->angle;
    tx = actorPos->x + ((sine[angle] << 5) * work->scale >> 8);
    orbitY = -gSineTable[angle + 0x40] * 22;
    ty = actorPos->y + (orbitY * work->scale >> 8);
    tz = actorPos->z - ((work->timer >> 1) << 8);
    work->angle = angle + work->spin;
    work->x += (tx - work->x) >> 2;
    work->y += (ty - work->y) >> 2;
    work->z += (tz - work->z) >> 2;
    work->ground = 0;
    work->scale -= 2;

    if (work->timer > 60) {
        work->update = NULL;
    } else {
        work->timer++;
    }
}

void task_poo_prize_0(PooPrizeWork* work, PoohPrizeArgs* args) {
    work->x = args->x;
    work->y = args->y;
    work->z = args->z;
    work->ground = 0;
    work->vz = -(GetRandom() % 0x301 + 0x200);
    work->speed = GetRandom() % 155 + 153;
    work->angle = GetRandom();
    work->tiles = LoadObjTiles(gMapPrizeTiles, sizeof(gMapPrizeTiles));
    work->palette = LoadObjPalette(gCommonObjPalette, sizeof(gCommonObjPalette));
    work->kind = args->kind;

    switch (work->kind) {
    case MAP_PRIZE_MOOGLE_POINTS_LARGE:
        work->gfx = gMapPrizeFrame3;
        work->amount = 10;
        break;
    case MAP_PRIZE_MOOGLE_POINTS_SMALL:
        work->gfx = gMapPrizeFrame2;
        work->amount = 4;
        break;
    case MAP_PRIZE_HP_LARGE:
        work->gfx = gMapPrizeFrame1;
        work->amount = 10;
        break;
    case MAP_PRIZE_HP_SMALL:
    default:
        work->gfx = gMapPrizeFrame0;
        work->amount = 3;
        break;
    }

    work->gfx2 = gMapPrizeFrame4;
    work->collected = FALSE;
    work->visible = TRUE;
    work->timer = 0;
    work->update = PooPrizeUpdateBounce;
    work->scale = Q_8_8(1);
    ColliderInit(&work->collider, COLLIDER_TYPE_PICKUP, 16, 50);
    ColliderSetPosition(&work->collider, work->x, work->y, work->z);
    ColliderSetDisabled(&work->collider, TRUE);
}

u8 task_poo_prize_1(PooPrizeWork* work) {
    if (work->update != NULL) {
        work->update(work);

        if (work->update != NULL) {
            return 1;
        }
    }

    return 0;
}

void task_poo_prize_2(PooPrizeWork* work) {
    ObjAffine* affine;
    s32 scale;
    s16 x;
    s16 y;

    if (!work->visible) {
        return;
    }

    x = (work->x >> 8) - gPooScrollX;
    y = (work->y >> 8) + (work->z >> 8) - gPooScrollY;

    if (IsRectOutsideScreen(x, y, 8, 8, 8, 8)) {
        return;
    }

    scale = work->scale;

    if (scale != Q_8_8(1)) {
        affine = AllocObjAffine(0, scale, scale, FALSE);
    } else {
        affine = NULL;
    }

    DrawSprite(x, y, work->gfx, work->tiles, work->palette, affine, SPRITE_PRIORITY(2), -0x1004 - (work->y >> 8) * 4);

    if (!work->collected) {
        y = (work->y >> 8) + (work->ground >> 8) - gPooScrollY;
        DrawSprite(x, y, work->gfx2, work->tiles, work->palette, affine, SPRITE_PRIORITY(2), 0xFFF0);
    }
}

void task_poo_prize_3(PooPrizeWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    ColliderUnregister(&work->collider);
}

void task_poo_zzz_0(PooZzzWork* work, u8* arg) {
    work->pos = gPoohPos;
    work->tiles = AllocObjTiles(0x100, gPoohFl05Tiles);
    work->palette = LoadObjPalette(gPoohPalette, sizeof(gPoohPalette));
    AnimInit(&work->anim, gPoohFl05Anims, gPoohFl05Frames);

    if (*arg != 0) {
        AnimStart(&work->anim, 8, ANIM_FLAG_LOOP);
    } else {
        AnimStart(&work->anim, 7, ANIM_FLAG_LOOP);
    }

    work->gfx = AnimGetGfx(&work->anim);
}

u8 task_poo_zzz_1(void* work) {
    return 1;
}

void task_poo_zzz_2(PooZzzWork* work) {
    PooPos* pos;
    s16 x;
    s16 y;
    void* gfx;

    pos = work->pos;
    x = (pos->x >> 8) - gPooScrollX;
    y = (pos->y >> 8) + (pos->z >> 8) - gPooScrollY;

    if (x >= -0x20 && x <= 0x110 && y >= -0x20 && y <= 0xC0) {
        gfx = AnimUpdate(&work->anim);
        work->gfx = gfx;
        DrawSprite(x, y, gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(1), 0x0B);
    }
}

void task_poo_zzz_3(PooZzzWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

void PooBflyPartSetDir(PooBflyPart* part) {
    switch (ANGLE_DIR8(part->angle)) {
    case DIR8_UP_RIGHT:
        part->dirIndex = 4;
        part->flipped = TRUE;
        break;
    case DIR8_RIGHT:
        part->dirIndex = 3;
        part->flipped = TRUE;
        break;
    case DIR8_DOWN_RIGHT:
        part->dirIndex = 2;
        part->flipped = TRUE;
        break;
    case DIR8_DOWN:
        part->dirIndex = 1;
        part->flipped = FALSE;
        break;
    case DIR8_DOWN_LEFT:
        part->dirIndex = 2;
        part->flipped = FALSE;
        break;
    case DIR8_LEFT:
        part->dirIndex = 3;
        part->flipped = FALSE;
        break;
    case DIR8_UP_LEFT:
        part->dirIndex = 4;
        part->flipped = FALSE;
        break;
    case DIR8_UP:
    default:
        part->dirIndex = 0;
        part->flipped = FALSE;
        break;
    }
}

void PooBflyPartSetAnimation(PooBflyPart* part) {
    const AnimDef* def;

    PooBflyPartSetDir(part);
    def = &gTrap01AnimDefs[part->dirIndex];
    AnimChangeWithTables(&part->anim, def->animId, ANIM_FLAG_LOOP, def->anims, def->gfxTable);
    SetObjTileSource(part->tiles, def->tiles);
}

void PooBflyPartInit(PooBflyPart* part) {
    if ((s8)part->angle >= 0) {
        part->targetX = part->pointBX;
        part->targetY = part->pointBY;
    } else {
        part->targetX = part->pointAX;
        part->targetY = part->pointAY;
    }

    part->tiles = AllocObjTiles(0x40, NULL);
    AnimInit(&part->anim, NULL, NULL);
    PooBflyPartSetAnimation(part);
    part->gfx = AnimGetGfx(&part->anim);
}

void task_poo_butterfly_0(PooButterflyWork* work, PooPos* pos) {
    work->x = pos->x;
    work->y = pos->y;
    work->z = pos->z - 0xE00;
    work->palette = LoadObjPalette(gTrap0100Palette, sizeof(gTrap0100Palette));
    work->parts[0].x = work->x - 0x1000;
    work->parts[0].y = work->y;
    work->parts[0].z = work->z;
    work->parts[0].pointAX = work->x - 0x1000;
    work->parts[0].pointAY = work->y;
    work->parts[0].pointBX = work->x + 0x600;
    work->parts[0].pointBY = work->y + 0x700;
    work->parts[0].angle = 0x60;
    work->parts[0].timer = 0x60;
    work->parts[1].x = work->x + 0x1200;
    work->parts[1].y = work->y;
    work->parts[1].z = work->z;
    work->parts[1].pointBX = work->x + 0x1200;
    work->parts[1].pointBY = work->y;
    work->parts[1].pointAX = work->x - 0x100;
    work->parts[1].pointAY = work->y - 0x700;
    work->parts[1].angle = 0xE0;
    work->parts[1].timer = 0x60;
    PooBflyPartInit(&work->parts[0]);
    PooBflyPartInit(&work->parts[1]);
}

void PooBflyPartUpdate(PooBflyPart* part) {
    if (part->timer != 0) {
        ApproachValue(&part->x, part->targetX, part->timer);
        ApproachValue(&part->y, part->targetY, part->timer);
        part->timer--;
    } else {
        part->timer = 0x60;
        part->angle += 0x80;

        if ((s8)part->angle >= 0) {
            part->targetX = part->pointBX;
            part->targetY = part->pointBY;
        } else {
            part->targetX = part->pointAX;
            part->targetY = part->pointAY;
        }
    }

    part->gfx = AnimUpdate(&part->anim);
}

u8 task_poo_butterfly_1(PooButterflyWork* work) {
    PooBflyPartUpdate(&work->parts[0]);
    PooBflyPartUpdate(&work->parts[1]);
    return 1;
}

u8 PooBflyPartDraw(PooBflyPart* part, void* pal) {
    s16 x;
    s16 y;
    s32 flags;

    x = (part->x >> 8) - gPooScrollX;
    y = (part->y >> 8) + (part->z >> 8) - gPooScrollY;

    if (x < -8 || x > 248 || y < -8 || y > 168) {
        return FALSE;
    }

    flags = part->flipped ? (SPRITE_PRIORITY(2) | SPRITE_FLAG_HFLIP) : SPRITE_PRIORITY(2);
    PooBflyPartSetAnimation(part);
    DrawSprite(x, y, part->gfx, part->tiles, pal, NULL, flags, -0x1004 - (part->y >> 8) * 4);
    return TRUE;
}

void task_poo_butterfly_2(PooButterflyWork* work) {
    PooBflyPartDraw(&work->parts[0], work->palette);
    PooBflyPartDraw(&work->parts[1], work->palette);
}

void task_poo_butterfly_3(PooButterflyWork* work) {
    ReleaseObjTiles(work->parts[0].tiles);
    ReleaseObjTiles(work->parts[1].tiles);
    ReleaseObjPalette(work->palette);
}

u8 task_poo_butterflyRight_1(PooButterflyWork* work) {
    PooBflyPartUpdate(&work->parts[1]);
    return 1;
}

void task_poo_butterflyRight_2(PooButterflyWork* work) {
    PooBflyPartDraw(&work->parts[1], work->palette);
}

u8 task_poo_butterflyLeft_1(PooButterflyWork* work) {
    PooBflyPartUpdate(&work->parts[0]);
    return 1;
}

void task_poo_butterflyLeft_2(PooButterflyWork* work) {
    PooBflyPartDraw(&work->parts[0], work->palette);
}

enum PooMapBeeState {
    POO_MAP_BEE_STATE_APPEAR,
    POO_MAP_BEE_STATE_FLY_AWAY
};

void task_poo_mapbee_0(PooMapBeeWork* work, PooPos* pos) {
    work->x = pos->x;
    work->y = pos->y;
    work->z = 0;
    work->tiles = AllocObjTiles(GetMaxSpriteTileBytes(gPooMapbeeFrames, 1), gPooMapbeeTiles);
    work->palette = LoadObjPalette(gPooMapbeePalette, sizeof(gPooMapbeePalette));
    AnimInit(&work->anim, gPooMapbeeAnims, gPooMapbeeFrames);
    AnimStart(&work->anim, 0, 0);
    work->gfx = AnimGetGfx(&work->anim);
    work->onScreen = TRUE;
    work->state = POO_MAP_BEE_STATE_APPEAR;
    m4aSongNumStart(SONG_SND_386);
}

u8 task_poo_mapbee_1(PooMapBeeWork* work) {
    if (!work->onScreen) {
        return 0;
    }

    switch (work->state) {
    case POO_MAP_BEE_STATE_APPEAR:
        if (AnimIsFinished(&work->anim)) {
            AnimStart(&work->anim, 1, ANIM_FLAG_LOOP);
            work->state = POO_MAP_BEE_STATE_FLY_AWAY;
        }

        break;
    case POO_MAP_BEE_STATE_FLY_AWAY:
        work->z -= 0xCC;
        break;
    }

    work->gfx = AnimUpdate(&work->anim);
    return 1;
}

void task_poo_mapbee_2(PooMapBeeWork* work) {
    s16 x;
    s16 y;

    x = (work->x >> 8) - gPooScrollX;
    y = (work->y >> 8) + (work->z >> 8) - gPooScrollY;

    if (IsRectOutsideScreen(x, y, 8, 8, 8, 8)) {
        work->onScreen = FALSE;
    } else {
        SetPooMapBeeVisible(TRUE);
        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), -0x1004 - (work->y >> 8) * 4);
    }
}

void task_poo_mapbee_3(PooMapBeeWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);

    if (!IsPooBeeAfterEventVisible()) {
        m4aSongNumStop(SONG_SND_386);
    }
}

void task_poo_mapbeeborn_0(PooMapBornWork* work, PooPos* pos) {
    work->pos = *pos;
    work->pos.z = 0;
    work->x = pos->x + 0x400;
    work->y = pos->y + 0x1800;
    work->z = 0;
    ColliderSetPosition(&work->collider, work->x, work->y, 0);
    work->colliderActive = FALSE;
    work->unk_7C = 0;
    work->armed = FALSE;
    TaskPoolInit(&work->tasks, 1);
    work->task = NULL;
}

u8 task_poo_mapbeeborn_1(PooMapBornWork* work) {
    if (work->colliderActive) {
        if ((work->collider.standFlags & COLLIDER_STAND_STOOD_ON) != 0) {
            if (!IsTaskActive(work->task) && work->armed) {
                work->armed = FALSE;
                work->task = TaskCreate(&work->tasks, &gTaskDescPooMapbee, &work->pos);
            }
        } else {
            work->armed = TRUE;
        }
    }

    return 1;
}

void task_poo_mapbeeborn_2(PooMapBornWork* work) {
    s16 x;
    s16 y;

    x = (work->x >> 8) - gPooScrollX;
    y = (work->y >> 8) - gPooScrollY;

    if (IsRectOutsideScreen(x, y, 0, 24, 0, 32)) {
        if (work->colliderActive) {
            work->colliderActive = FALSE;
            ColliderUnregister(&work->collider);
            work->armed = FALSE;

#ifdef VERSION_EU
            if (IsTaskActive(work->task)) {
                TaskKill(&work->tasks, work->task);
            }
#endif
        }
    } else {
        if (!work->colliderActive) {
            ColliderInit(&work->collider, COLLIDER_TYPE_SOLID, 28, 0);
            work->colliderActive = TRUE;
            work->armed = TRUE;
        }

        TaskPoolUpdate(&work->tasks);
        TaskPoolDraw(&work->tasks);
    }
}

void task_poo_mapbeeborn_3(PooMapBornWork* work) {
    if (work->colliderActive) {
        ColliderUnregister(&work->collider);
    }

    TaskPoolDestroy(&work->tasks);
}

void task_poo_mapbutterfly_0(PooMapButterflyWork* work, PooPos* pos) {
    work->x = pos->x;
    work->y = pos->y;
    work->z = 0;
    work->tiles = AllocObjTiles(0x40, gPooMapbutterflyTiles);
    work->palette = LoadObjPalette(gPooMapbutterflyPalette, sizeof(gPooMapbutterflyPalette));
    AnimInit(&work->anim, gPooMapbutterflyAnims, gPooMapbutterflyFrames);
    AnimStart(&work->anim, 0, 0);
    work->gfx = AnimGetGfx(&work->anim);
    work->onScreen = TRUE;
}

u8 task_poo_mapbutterfly_1(PooMapButterflyWork* work) {
    if (!work->onScreen) {
        return 0;
    }

    work->z -= 0x80;
    work->gfx = AnimUpdate(&work->anim);
    return 1;
}

void task_poo_mapbutterfly_2(PooMapButterflyWork* work) {
    s16 x;
    s16 y;

    x = (work->x >> 8) - gPooScrollX;
    y = (work->y >> 8) + (work->z >> 8) - gPooScrollY;

    if (IsRectOutsideScreen(x, y, 8, 8, 8, 8)) {
        work->onScreen = FALSE;
    } else {
        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), -0x1004 - (work->y >> 8) * 4);
    }
}

void task_poo_mapbutterfly_3(PooMapButterflyWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

void task_poo_mapbutterflyborn_0(PooMapBornWork* work, PooPos* pos) {
    work->pos = *pos;
    work->pos.z = 0;
    work->x = pos->x + 0x1000;
    work->y = pos->y + 0x1800;
    work->z = 0;
    ColliderSetPosition(&work->collider, work->x, work->y, 0);
    work->colliderActive = FALSE;
    work->unk_7C = 0;
    work->armed = FALSE;
    TaskPoolInit(&work->tasks, 1);
    work->task = NULL;
}

u8 task_poo_mapbutterflyborn_1(PooMapBornWork* work) {
    if (work->colliderActive) {
        if ((work->collider.standFlags & COLLIDER_STAND_STOOD_ON) != 0) {
            if (!IsTaskActive(work->task) && work->armed) {
                work->armed = FALSE;
                work->task = TaskCreate(&work->tasks, &gTaskDescPooMapbutterfly, &work->pos);
            }
        } else {
            work->armed = TRUE;
        }
    }

    return 1;
}

void task_poo_mapbutterflyborn_2(PooMapBornWork* work) {
    s16 x;
    s16 y;

    x = (work->x >> 8) - gPooScrollX;
    y = (work->y >> 8) - gPooScrollY;

    if (IsRectOutsideScreen(x, y, 0, 24, 0, 32)) {
        if (work->colliderActive) {
            work->colliderActive = FALSE;
            ColliderUnregister(&work->collider);
            work->armed = FALSE;
        }
    } else {
        if (!work->colliderActive) {
            ColliderInit(&work->collider, COLLIDER_TYPE_SOLID, 40, 0);
            work->colliderActive = TRUE;
            work->armed = TRUE;
        }

        TaskPoolUpdate(&work->tasks);
        TaskPoolDraw(&work->tasks);
    }
}

void task_poo_mapbutterflyborn_3(PooMapBornWork* work) {
    if (work->colliderActive) {
        ColliderUnregister(&work->collider);
    }

    TaskPoolDestroy(&work->tasks);
}

void task_poo_rabbitAfterEvent_0(PooRabbitAfterEventWork* work) {
    work->x = 0xA9B00;
    work->y = 0x57200;
    work->z = 0;
    work->ground = 0;
    work->palette = NULL;
    work->tileBytes = GetMaxSpriteTileBytes(gRabbitBl00Frames, ARRAY_COUNT(gRabbitBl00Frames));
    AnimInit(&work->anim, gRabbitBl00Anims, gRabbitBl00Frames);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    TaskPoolInit(&work->tasks, 1);
    CreatePooShadowscaleTask(&work->tasks, &work->x, Q_8_8(1));
    ColliderSetPosition(&work->collider, work->x, work->y, work->z);
    work->interactionId = AddPoohInteraction(&work->collider, CARD_MSG_RABBIT_TALK_1);
    SetPoohInteractionEnabled(work->interactionId, FALSE);
}

u8 task_poo_rabbitAfterEvent_1(PooRabbitAfterEventWork* work) {
    if (work->palette != NULL) {
        if (ColliderIsTouchingType(&work->collider, COLLIDER_TYPE_POOH)) {
            gPoohRequest = POOH_REQUEST_BLOCKED;
        }
    }

    return 1;
}

void task_poo_rabbitAfterEvent_2(PooRabbitAfterEventWork* work) {
    s16 x;
    s16 y;

    x = (work->x >> 8) - gPooScrollX;
    y = (work->y >> 8) - gPooScrollY;

    if (IsRectOutsideScreen(x, y, 48, 8, 16, 16)) {
        if (work->palette != NULL) {
            ReleaseObjTiles(work->tiles);
            ReleaseObjPalette(work->palette);
            ColliderUnregister(&work->collider);
            SetPoohInteractionEnabled(work->interactionId, FALSE);
            work->palette = NULL;
        }
    } else {
        work->gfx = AnimUpdate(&work->anim);
        TaskPoolUpdate(&work->tasks);

        if (work->palette == NULL) {
            work->palette = LoadObjPalette(gRabbitPalettes, sizeof(gRabbitPalettes));
            work->tiles = AllocObjTiles(work->tileBytes, gRabbitBl00Tiles);
            ColliderInit(&work->collider, COLLIDER_TYPE_POOH_OBJECT, 4, 48);
            SetPoohInteractionEnabled(work->interactionId, TRUE);
        }

        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2) | SPRITE_FLAG_HFLIP, -0x1004 - (work->y >> 8) * 4);
        TaskPoolDraw(&work->tasks);
    }
}

void task_poo_rabbitAfterEvent_3(PooRabbitAfterEventWork* work) {
    if (work->palette != NULL) {
        ReleaseObjTiles(work->tiles);
        ReleaseObjPalette(work->palette);
        ColliderUnregister(&work->collider);
    }

    TaskPoolDestroy(&work->tasks);
}

void task_poo_cabbageAfterEvent_0(PooCabbageAfterEventWork* work) {
    work->x = 0xAB300;
    work->y = 0x57100;
    work->z = 0;
    work->ground = 0;
    work->palette = NULL;
    work->tileBytes = GetMaxSpriteTileBytes(gRaVegetablesFrames, ARRAY_COUNT(gRaVegetablesFrames));
    work->gfx = gRaVegetablesFrame9;
}

u8 task_poo_cabbageAfterEvent_1(PooCabbageAfterEventWork* work) {
    *(void* volatile*)&work->palette;
    return 1;
}

void task_poo_cabbageAfterEvent_2(PooCabbageAfterEventWork* work) {
    s16 x;
    s16 y;

    x = (work->x >> 8) - gPooScrollX;
    y = (work->y >> 8) - gPooScrollY;

    if (IsRectOutsideScreen(x, y, 48, 8, 16, 16)) {
        if (work->palette != NULL) {
            ReleaseObjTiles(work->tiles);
            ReleaseObjPalette(work->palette);
            work->palette = NULL;
        }
    } else {
        if (work->palette == NULL) {
            work->palette = LoadObjPalette(gRaVegetablesPalette, sizeof(gRaVegetablesPalette));
            work->tiles = AllocObjTiles(work->tileBytes, gRaVegetablesTiles);
        }

        DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), -0x1004 - (work->y >> 8) * 4);
    }
}

void task_poo_cabbageAfterEvent_3(PooCabbageAfterEventWork* work) {
    if (work->palette != NULL) {
        ReleaseObjTiles(work->tiles);
        ReleaseObjPalette(work->palette);
    }
}

void ClearPooPrizesDropped() {
    s32 i;

    for (i = 0; i < 4; i++) {
        gPooState.droppedPrizes[i] = 0;
    }
}

void SetPooPrizeDropped(u16 prizeId) {
    u32 word;
    u32 bit;

    word = prizeId / 32;
    bit = prizeId % 32;
    gPooState.droppedPrizes[word] |= 1 << bit;
}

u8 IsPooPrizeDropped(u16 prizeId) {
    u32 word;
    u32 bit;

    word = prizeId / 32;
    bit = prizeId % 32;

    if ((gPooState.droppedPrizes[word] & (1 << bit)) != 0) {
        return TRUE;
    }

    return FALSE;
}

void ResetPooProgress() {
    ClearPooPrizesDropped();
    gPooState.eventsDone = 0;
    gPooState.gauge = 3;
    gPooState.gaugeTimer = 0x73B;
    gPoohGauge = 3;
    gPoohGaugeTimer = 0x73B;
}

void InitPooState() {
    gPooState.flags = 0;
    ResetPooProgress();
}

void SetPooStatePooh(PooPos* pos, s32 action) {
    gPooState.pos = *pos;
    gPooState.poohAction = action;
}

void GetPooStatePooh(PooPos* pos, s32* action) {
    *pos = gPooState.pos;
    *action = gPooState.poohAction;
}

void SetPooStateGauge(u16 gauge, u16 gaugeTimer) {
    gPooState.gauge = gauge;
    gPooState.gaugeTimer = gaugeTimer;
}

void GetPooStateGauge(u16* gauge, u16* gaugeTimer) {
    *gauge = gPooState.gauge;
    *gaugeTimer = gPooState.gaugeTimer;
}

void SetPooStateWheelPos(s16 x, s16 y) {
    gPooState.wheelX = x;
    gPooState.wheelY = y;
}

void GetPooStateWheelPos(u16* x, u16* y) {
    *x = gPooState.wheelX;
    *y = gPooState.wheelY;
}

void SetPooStatePos2(PooPos* pos) {
    gPooState.pos2 = *pos;
}

void GetPooStatePos2(PooPos* pos) {
    *pos = gPooState.pos2;
}

void SetPooEventDone(s32 event) {
    gPooState.eventsDone |= 1 << event;
}

u8 IsPooEventDone(s32 event) {
    if (((gPooState.eventsDone >> event) & 1) != 0) {
        return TRUE;
    }

    return FALSE;
}

void SetPooFlag(s32 flag) {
    gPooState.flags |= 1 << flag;
}

void ClearPooFlag(s32 flag) {
    gPooState.flags &= ~(1 << flag);
}

u8 IsPooFlagSet(s32 flag) {
    if ((gPooState.flags & (1 << flag)) != 0) {
        return TRUE;
    }

    return FALSE;
}

void GetPooState(void* dst) {
    memcpy(dst, &gPooState, sizeof(gPooState));
}

void SetPooState(const void* src) {
    memcpy(&gPooState, src, sizeof(gPooState));
}

u8 IsPooAltImageActive() {
    s32 eventIds[6];
    u32 i;
    s32 doneCount;

    memcpy(eventIds, gPooMainEventIds, sizeof(eventIds));

    if (IsPooFlagSet(1)) {
        return TRUE;
    }

    doneCount = 0;

    for (i = 0; i < ARRAY_COUNT(eventIds); i++) {
        if (IsPooEventDone(eventIds[i])) {
            doneCount++;
        }
    }

    if (doneCount <= 4) {
        return FALSE;
    }

    return TRUE;
}

u16 AddPoohInteraction(Collider* collider, u16 message) {
    if (sPoohInteractions->count > 5) {
        return 0xFFFF;
    }

    sPoohInteractions->entries[sPoohInteractions->count].collider = collider;
    sPoohInteractions->entries[sPoohInteractions->count].message = message;
    sPoohInteractions->entries[sPoohInteractions->count].enabled = TRUE;
    return sPoohInteractions->count++;
}

void SetPoohInteractionEnabled(u16 id, u8 enabled) {
    sPoohInteractions->entries[id].enabled = enabled;
}

void FreePoohInteractions() {
    EwramFree(gSharedModeWork);
}

void InitPoohInteractions() {
    void** state = &gSharedModeWork;

    *state = EwramAlloc(sizeof(PoohInteractionRegistry));
    ((PoohInteractionRegistry*)*state)->count = 0;
    SetPooRabbitTalkBlocked(FALSE);
}

u16 FindPoohInteractionMessage() {
    s32 i;

    for (i = 0; i < sPoohInteractions->count; i++) {
        if (sPoohInteractions->entries[i].message == CARD_MSG_RABBIT_TALK_0 && sPoohInteractions->rabbitTalkBlocked) {
            continue;
        }

        if (!sPoohInteractions->entries[i].enabled) {
            continue;
        }

        if (!PooAttackHitsCollider(sPoohInteractions->entries[i].collider)) {
            continue;
        }

        return sPoohInteractions->entries[i].message;
    }

    return CARD_MSG_COUNT;
}

void SetPooRabbitTalkBlocked(u8 blocked) {
    sPoohInteractions->rabbitTalkBlocked = blocked;
}

TaskDesc gTaskDescPooPooh = {
    "task_poo_pooh",
    (TaskInitFunc)task_poo_pooh_0,
    (TaskUpdateFunc)task_poo_pooh_1,
    (TaskDrawFunc)task_poo_pooh_2,
    (TaskDestroyFunc)task_poo_pooh_3,
    sizeof(PoohWork),
};

const PooMapBgDesc gPooMapBgDesc = {
    gPooMapBgTiles, sizeof(gPooMapBgTiles), gPooMapBgPalette, sizeof(gPooMapBgPalette), gPooMapBgTiles2, sizeof(gPooMapBgTiles2), 16, 9
};

const PooSpawn gPooSpawns[85] = {
    { 130816, 99328, &gTaskDescPooHoney },
    { 223232, 130304, &gTaskDescPooTrapballoon },
    { 229888, 153088, &gTaskDescPooTrapballoon },
    { 251392, 143104, &gTaskDescPooPitAndButterfly },
    { 242688, 159488, &gTaskDescPooTrap },
    { 263936, 162816, &gTaskDescPooHoney },
    { 279040, 175104, &gTaskDescPooTrap },
    { 304896, 167424, &gTaskDescPooTrap },
    { 301824, 182784, &gTaskDescPooTrapballoon },
    { 444672, 238848, &gTaskDescPooTigerstump },
    { 411904, 238848, &gTaskDescPooTigerstump },
    { 428288, 247040, &gTaskDescPooTigerstump },
    { 428288, 230656, &gTaskDescPooTigerstump },
    { 440576, 231680, &gTaskDescPooTiBoard },
    { 407040, 218624, &gTaskDescPooHoney },
    { 463104, 266496, &gTaskDescPooTrap },
    { 476672, 251136, &gTaskDescPooPitAndButterfly },
    { 502272, 262912, &gTaskDescPooTrap },
    { 489216, 277504, &gTaskDescPooPitAndButterfly },
    { 484096, 265472, &gTaskDescPooTrapballoon },
    { 518400, 271616, &gTaskDescPooHoney },
    { 648960, 335616, &gTaskDescPooHoney },
    { 602624, 320256, &gTaskDescPooPoohstump },
    { 584192, 320256, &gTaskDescPooPoohstump },
    { 593408, 315648, &gTaskDescPooPoohstump },
    { 593408, 324864, &gTaskDescPooPoohstump },
    { 602624, 315136, &gTaskDescPooTiBoard },
    { 585984, 305920, &gTaskDescPooPoohstump },
    { 573440, 329472, &gTaskDescPooPoohstump },
    { 566016, 325376, &gTaskDescPooPoohstump },
    { 617728, 319488, &gTaskDescPooPoohstump },
    { 282624, 150272, &gTaskDescPooLeaf },
    { 512000, 285440, &gTaskDescPooLeaf },
    { 159744, 102400, &gTaskDescPooTanpopo },
    { 194560, 114688, &gTaskDescPooTanpopo },
    { 200704, 118784, &gTaskDescPooButterfly },
    { 167936, 120832, &gTaskDescPooTanpopo },
    { 174080, 124928, &gTaskDescPooButterflyRight },
    { 198656, 133120, &gTaskDescPooTanpopo },
    { 323584, 167936, &gTaskDescPooTanpopo },
    { 313344, 180224, &gTaskDescPooTanpopo },
    { 323584, 192512, &gTaskDescPooTanpopo },
    { 333824, 182272, &gTaskDescPooTanpopo },
    { 339968, 186368, &gTaskDescPooButterflyRight },
    { 350208, 182272, &gTaskDescPooTanpopo },
    { 337920, 190464, &gTaskDescPooTanpopo },
    { 358400, 190464, &gTaskDescPooTanpopo },
    { 301056, 196608, &gTaskDescPooTanpopo },
    { 325632, 212992, &gTaskDescPooTanpopo },
    { 350208, 198656, &gTaskDescPooTanpopo },
    { 374784, 200704, &gTaskDescPooTanpopo },
    { 362496, 204800, &gTaskDescPooTanpopo },
    { 382976, 210944, &gTaskDescPooTanpopo },
    { 370688, 212992, &gTaskDescPooTanpopo },
    { 391168, 219136, &gTaskDescPooTanpopo },
    { 374784, 221184, &gTaskDescPooTanpopo },
    { 358400, 225280, &gTaskDescPooTanpopo },
    { 387072, 229376, &gTaskDescPooTanpopo },
    { 374784, 233472, &gTaskDescPooTanpopo },
    { 391168, 241664, &gTaskDescPooTanpopo },
    { 395264, 204800, &gTaskDescPooTanpopo },
    { 405504, 221184, &gTaskDescPooTanpopo },
    { 403456, 247808, &gTaskDescPooTanpopo },
    { 313344, 200704, &gTaskDescPooTanpopo },
    { 325632, 202752, &gTaskDescPooTanpopo },
    { 337920, 198656, &gTaskDescPooTanpopo },
    { 344064, 206848, &gTaskDescPooTanpopo },
    { 350208, 210944, &gTaskDescPooButterflyLeft },
    { 356352, 212992, &gTaskDescPooTanpopo },
    { 342016, 215040, &gTaskDescPooTanpopo },
    { 79872, 38912, &gTaskDescPooMapbeeborn },
    { 153600, 75776, &gTaskDescPooMapbeeborn },
    { 559104, 278528, &gTaskDescPooMapbeeborn },
    { 632832, 315392, &gTaskDescPooMapbeeborn },
    { 706560, 352256, &gTaskDescPooMapbeeborn },
    { 780288, 389120, &gTaskDescPooMapbeeborn },
    { 131072, 63488, &gTaskDescPooMapbutterflyborn },
    { 278528, 137216, &gTaskDescPooMapbutterflyborn },
    { 352256, 174080, &gTaskDescPooMapbutterflyborn },
    { 536576, 266240, &gTaskDescPooMapbutterflyborn },
    { 610304, 303104, &gTaskDescPooMapbutterflyborn },
    { 684032, 339968, &gTaskDescPooMapbutterflyborn },
    { 757760, 376832, &gTaskDescPooMapbutterflyborn },
    { 794624, 395264, &gTaskDescPooMapbutterflyborn },
    { 831488, 413696, &gTaskDescPooMapbutterflyborn },
};

const PooPileDesc gPooPileDescs[12] = {
    { 611584, 339200, 4 },
    { 619264, 343296, 3 },
    { 626944, 347392, 2 },
    { 634624, 351488, 1 },
    { 642304, 355584, 2 },
    { 649984, 359680, 3 },
    { 657664, 363776, 4 },
    { 763904, 389632, 0 },
    { 759552, 398336, 0 },
    { 754688, 405760, 0 },
    { 747264, 411904, 0 },
    { 739072, 416768, 0 },
};

const PooTileDesc gPooTileDescs[80] = {
    { 106496, 49152, 0x3 },
    { 180224, 86016, 0x3 },
    { 327680, 159744, 0x3 },
    { 512000, 251904, 0x3 },
    { 585728, 288768, 0x3 },
    { 659456, 325632, 0x3 },
    { 770048, 380928, 0x3 },
    { 217088, 104448, 0x5 },
    { 253952, 122880, 0x5 },
    { 290816, 141312, 0x5 },
    { 364544, 178176, 0x5 },
    { 401408, 196608, 0x5 },
    { 438272, 215040, 0x5 },
    { 475136, 233472, 0x5 },
    { 548864, 270336, 0x5 },
    { 622592, 307200, 0x5 },
    { 696320, 344064, 0x5 },
    { 806912, 399360, 0x5 },
    { 131072, 63488, 0x8 },
    { 204800, 100352, 0x8 },
    { 278528, 137216, 0x8 },
    { 389120, 192512, 0x8 },
    { 352256, 174080, 0x8 },
    { 462848, 229376, 0x8 },
    { 536576, 266240, 0x8 },
    { 610304, 303104, 0x8 },
    { 684032, 339968, 0x8 },
    { 757760, 376832, 0x8 },
    { 794624, 395264, 0x8 },
    { 831488, 413696, 0x9 },
    { 94208, 45056, 0x4 },
    { 167936, 81920, 0x4 },
    { 241664, 118784, 0x4 },
    { 315392, 155648, 0x4 },
    { 425984, 210944, 0x4 },
    { 499712, 247808, 0x4 },
    { 647168, 321536, 0x4 },
    { 720896, 358400, 0x4 },
    { 116736, 57344, 0x1 },
    { 227328, 112640, 0x1 },
    { 301056, 149504, 0x1 },
    { 337920, 167936, 0x1 },
    { 374784, 186368, 0x1 },
    { 485376, 241664, 0x1 },
    { 522240, 260096, 0x1 },
    { 595968, 296960, 0x1 },
    { 669696, 333824, 0x1 },
    { 743424, 370688, 0x1 },
    { 817152, 407552, 0x1 },
    { 120832, 63488, 0x0 },
    { 194560, 100352, 0x0 },
    { 305152, 155648, 0x0 },
    { 342016, 174080, 0x0 },
    { 378880, 192512, 0x0 },
    { 526336, 266240, 0x0 },
    { 600064, 303104, 0x0 },
    { 673792, 339968, 0x0 },
    { 747520, 376832, 0x0 },
    { 153600, 75776, 0x6 },
    { 190464, 94208, 0x6 },
    { 264192, 131072, 0x6 },
    { 411648, 204800, 0x6 },
    { 448512, 223232, 0x6 },
    { 559104, 278528, 0x6 },
    { 632832, 315392, 0x6 },
    { 706560, 352256, 0x6 },
    { 780288, 389120, 0x6 },
    { 79872, 38912, 0x7 },
    { 83968, 45056, 0x2 },
    { 157696, 81920, 0x2 },
    { 231424, 118784, 0x2 },
    { 268288, 137216, 0x2 },
    { 415744, 210944, 0x2 },
    { 452608, 229376, 0x2 },
    { 489472, 247808, 0x2 },
    { 563200, 284672, 0x2 },
    { 636928, 321536, 0x2 },
    { 710656, 358400, 0x2 },
    { 784384, 395264, 0x2 },
    { 821248, 413696, 0x2 },
};

TaskDesc gTaskDescPooMap = {
    "task_poo_map",
    (TaskInitFunc)task_poo_map_0,
    (TaskUpdateFunc)task_poo_map_1,
    (TaskDrawFunc)task_poo_map_2,
    (TaskDestroyFunc)task_poo_map_3,
    sizeof(PooMapWork),
};

const PooAnimDesc gPooSoraAnimDescs[11][5] = {
    {
        { gSor1bb00Frames, gSor1bb00Anims, gSor1bb00Tiles, 0 },
        { gSor1ff00Frames, gSor1ff00Anims, gSor1ff00Tiles, 0 },
        { gSor1fl00Frames, gSor1fl00Anims, gSor1fl00Tiles, 0 },
        { gSor1ll00Frames, gSor1ll00Anims, gSor1ll00Tiles, 0 },
        { gSor1bl00Frames, gSor1bl00Anims, gSor1bl00Tiles, 0 },
    },
    {
        { gSor1bb01Frames, gSor1bb01Anims, gSor1bb01Tiles, 0 },
        { gSor1ff01Frames, gSor1ff01Anims, gSor1ff01Tiles, 0 },
        { gSor1fl01Frames, gSor1fl01Anims, gSor1fl01Tiles, 0 },
        { gSor1ll01Frames, gSor1ll01Anims, gSor1ll01Tiles, 0 },
        { gSor1bl01Frames, gSor1bl01Anims, gSor1bl01Tiles, 0 },
    },
    {
        { gSor1bb02Frames, gSor1bb02Anims, gSor1bb02Tiles, 0 },
        { gSor1ff02Frames, gSor1ff02Anims, gSor1ff02Tiles, 0 },
        { gSor1fl02Frames, gSor1fl02Anims, gSor1fl02Tiles, 0 },
        { gSor1ll02Frames, gSor1ll02Anims, gSor1ll02Tiles, 0 },
        { gSor1bl02Frames, gSor1bl02Anims, gSor1bl02Tiles, 0 },
    },
    {
        { gSor1bb03Frames, gSor1bb03Anims, gSor1bb03Tiles, 0 },
        { gSor1ff03Frames, gSor1ff03Anims, gSor1ff03Tiles, 0 },
        { gSor1fl03Frames, gSor1fl03Anims, gSor1fl03Tiles, 0 },
        { gSor1ll03Frames, gSor1ll03Anims, gSor1ll03Tiles, 0 },
        { gSor1bl03Frames, gSor1bl03Anims, gSor1bl03Tiles, 0 },
    },
    {
        { gSor1bb03Frames, gSor1bb03Anims, gSor1bb03Tiles, 1 },
        { gSor1ff03Frames, gSor1ff03Anims, gSor1ff03Tiles, 1 },
        { gSor1fl03Frames, gSor1fl03Anims, gSor1fl03Tiles, 1 },
        { gSor1ll03Frames, gSor1ll03Anims, gSor1ll03Tiles, 1 },
        { gSor1bl03Frames, gSor1bl03Anims, gSor1bl03Tiles, 1 },
    },
    {
        { gSor1bb03Frames, gSor1bb03Anims, gSor1bb03Tiles, 2 },
        { gSor1ff03Frames, gSor1ff03Anims, gSor1ff03Tiles, 2 },
        { gSor1fl03Frames, gSor1fl03Anims, gSor1fl03Tiles, 2 },
        { gSor1ll03Frames, gSor1ll03Anims, gSor1ll03Tiles, 2 },
        { gSor1bl03Frames, gSor1bl03Anims, gSor1bl03Tiles, 2 },
    },
    {
        { gSor1bb03Frames, gSor1bb03Anims, gSor1bb03Tiles, 3 },
        { gSor1ff03Frames, gSor1ff03Anims, gSor1ff03Tiles, 3 },
        { gSor1fl03Frames, gSor1fl03Anims, gSor1fl03Tiles, 3 },
        { gSor1ll03Frames, gSor1ll03Anims, gSor1ll03Tiles, 3 },
        { gSor1bl03Frames, gSor1bl03Anims, gSor1bl03Tiles, 3 },
    },
    {
        { gSor1bb03Frames, gSor1bb03Anims, gSor1bb03Tiles, 4 },
        { gSor1ff03Frames, gSor1ff03Anims, gSor1ff03Tiles, 4 },
        { gSor1fl03Frames, gSor1fl03Anims, gSor1fl03Tiles, 4 },
        { gSor1ll03Frames, gSor1ll03Anims, gSor1ll03Tiles, 4 },
        { gSor1bl03Frames, gSor1bl03Anims, gSor1bl03Tiles, 4 },
    },
    {
        { gSor1bb15Frames, gSor1bb15Anims, gSor1bb15Tiles, 2 },
        { gSor1ff15Frames, gSor1ff15Anims, gSor1ff15Tiles, 2 },
        { gSor1fl15Frames, gSor1fl15Anims, gSor1fl15Tiles, 2 },
        { gSor1ll15Frames, gSor1ll15Anims, gSor1ll15Tiles, 2 },
        { gSor1bl15Frames, gSor1bl15Anims, gSor1bl15Tiles, 2 },
    },
    {
        { gSor1bb10Frames, gSor1bb10Anims, gSor1bb10Tiles, 0 },
        { gSor1ff10Frames, gSor1ff10Anims, gSor1ff10Tiles, 0 },
        { gSor1fl10Frames, gSor1fl10Anims, gSor1fl10Tiles, 0 },
        { gSor1ll10Frames, gSor1ll10Anims, gSor1ll10Tiles, 0 },
        { gSor1bl10Frames, gSor1bl10Anims, gSor1bl10Tiles, 0 },
    },
    {
        { gSor1bb61Frames, gSor1bb61Anims, gSor1bb61Tiles, 0 },
        { gSor1ff61Frames, gSor1ff61Anims, gSor1ff61Tiles, 0 },
        { gSor1fl61Frames, gSor1fl61Anims, gSor1fl61Tiles, 0 },
        { gSor1ll61Frames, gSor1ll61Anims, gSor1ll61Tiles, 0 },
        { gSor1bl61Frames, gSor1bl61Anims, gSor1bl61Tiles, 0 },
    },
};

const u16 gPooSoraSounds[8] = { SONG_SYS_SR_FOOTL, SONG_SYS_SR_FOOTR, SONG_SYS_SR_JUMP, SONG_SYS_SR_LAND, SONG_SYS_SR_GRASSUP, SONG_SYS_SR_GRASSUP, SONG_SYS_SR_GRASSJP, 0 };

const u16* gPooBg3MapBlocks[144] = {
    gPooBg3Map0,
    gPooBg3Map1,
    gPooBg3Map2,
    gPooBg3Map3,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gPooBg3Map16,
    gPooBg3Map17,
    gPooBg3Map18,
    gPooBg3Map19,
    gPooBg3Map20,
    gPooBg3Map21,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gPooBg3Map34,
    gPooBg3Map35,
    gPooBg3Map36,
    gPooBg3Map37,
    gPooBg3Map38,
    gPooBg3Map39,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gPooBg3Map52,
    gPooBg3Map53,
    gPooBg3Map54,
    gPooBg3Map55,
    gPooBg3Map56,
    gPooBg3Map57,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gPooBg3Map70,
    gPooBg3Map71,
    gPooBg3Map72,
    gPooBg3Map73,
    gPooBg3Map74,
    gPooBg3Map75,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gPooBg3Map88,
    gPooBg3Map89,
    gPooBg3Map90,
    gPooBg3Map91,
    gPooBg3Map92,
    gPooBg3Map93,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gPooBg3Map106,
    gPooBg3Map107,
    gPooBg3Map108,
    gPooBg3Map109,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gPooBg3Map124,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
};

const u16* gPooBg1MapBlocks[144] = {
    gPooBg1Map0,
    gPooBg1Map1,
    gPooBg1Map2,
    gPooBg1Map3,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gPooBg1Map16,
    gPooBg1Map17,
    gPooBg1Map18,
    gPooBg1Map19,
    gPooBg1Map20,
    gPooBg1Map21,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gPooBg1Map32,
    gPooBg1Map32,
    gPooBg1Map34,
    gPooBg1Map35,
    gPooBg1Map36,
    gPooBg1Map37,
    gPooBg1Map38,
    gPooBg1Map39,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gPooBg1Map32,
    gPooBg1Map32,
    gPooBg1Map52,
    gPooBg1Map53,
    gPooBg1Map54,
    gPooBg1Map55,
    gPooBg1Map56,
    gPooBg1Map57,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gPooBg1Map32,
    gPooBg1Map32,
    gPooBg1Map70,
    gPooBg1Map71,
    gPooBg1Map72,
    gPooBg1Map73,
    gPooBg1Map74,
    gPooBg1Map75,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gPooBg1Map32,
    gPooBg1Map32,
    gPooBg1Map88,
    gPooBg1Map89,
    gPooBg1Map90,
    gPooBg1Map91,
    gPooBg1Map92,
    gPooBg1Map93,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gPooBg1Map32,
    gPooBg1Map32,
    gPooBg1Map106,
    gPooBg1Map107,
    gPooBg1Map108,
    gPooBg1Map109,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gPooBg1Map32,
    gPooBg1Map32,
    gPooBg1Map124,
    gPooBg1Map125,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gPooBg1Map32,
    gPooBg1Map32,
    gDefaultBgMap,
    gDefaultBgMap,
};

const u16* gPooBg2MapBlocks[144] = {
    gDefaultBgMap,
    gPooBg2Map1,
    gPooBg2Map2,
    gPooBg2Map3,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gPooBg2Map18,
    gPooBg2Map19,
    gPooBg2Map20,
    gPooBg2Map21,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gPooBg2Map36,
    gPooBg2Map37,
    gPooBg2Map38,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gPooBg2Map54,
    gPooBg2Map55,
    gPooBg2Map56,
    gPooBg2Map57,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gPooBg2Map72,
    gPooBg2Map73,
    gPooBg2Map74,
    gPooBg2Map75,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gPooBg2Map90,
    gPooBg2Map91,
    gPooBg2Map92,
    gPooBg2Map93,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gPooBg2Map108,
    gPooBg2Map109,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
    gDefaultBgMap,
};

TaskDesc gTaskDescPooSora = {
    "task_poo_sora",
    (TaskInitFunc)task_poo_sora_0,
    (TaskUpdateFunc)task_poo_sora_1,
    (TaskDrawFunc)task_poo_sora_2,
    (TaskDestroyFunc)task_poo_sora_3,
    sizeof(PooSoraWork),
};

TaskDesc gTaskDescPooTrap = {
    "task_poo_trap",
    (TaskInitFunc)task_poo_trap_0,
    (TaskUpdateFunc)task_poo_trap_1,
    (TaskDrawFunc)task_poo_trap_2,
    (TaskDestroyFunc)task_poo_trap_3,
    sizeof(PooTrapWork),
};

TaskDesc gTaskDescPooPitAndButterfly = {
    "task_poo_pitAndButterfly",
    (TaskInitFunc)task_poo_pitAndButterfly_0,
    (TaskUpdateFunc)task_poo_pitAndButterfly_1,
    (TaskDrawFunc)task_poo_pitAndButterfly_2,
    (TaskDestroyFunc)task_poo_pitAndButterfly_3,
    sizeof(PooTrapWork),
};

TaskDesc gTaskDescPooBalloon = {
    "task_poo_balloon",
    (TaskInitFunc)task_poo_balloon_0,
    (TaskUpdateFunc)task_poo_balloon_1,
    (TaskDrawFunc)task_poo_balloon_2,
    (TaskDestroyFunc)task_poo_balloon_3,
    sizeof(PooBalloonObjWork),
};

TaskDesc gTaskDescPooShadow = {
    "task_poo_shadow",
    (TaskInitFunc)task_poo_shadow_0,
    (TaskUpdateFunc)task_poo_shadow_1,
    (TaskDrawFunc)task_poo_shadow_2,
    (TaskDestroyFunc)task_poo_shadow_3,
    sizeof(TaskPool),
};

TaskDesc gTaskDescPooShadowdodai = {
    "task_poo_shadowdodai",
    (TaskInitFunc)task_poo_shadowdodai_0,
    (TaskUpdateFunc)task_poo_shadowdodai_1,
    (TaskDrawFunc)task_poo_shadowdodai_2,
    (TaskDestroyFunc)task_poo_shadowdodai_3,
    sizeof(PooShadowWork),
};

TaskDesc gTaskDescPooShadowscale = {
    "task_poo_shadowscale",
    (TaskInitFunc)task_poo_shadowscale_0,
    (TaskUpdateFunc)task_poo_shadowscale_1,
    (TaskDrawFunc)task_poo_shadowscale_2,
    (TaskDestroyFunc)task_poo_shadowscale_3,
    sizeof(PooScaleWork),
};

TaskDesc gTaskDescPooFreeballoon = {
    "task_poo_freeballoon",
    (TaskInitFunc)task_poo_freeballoon_0,
    (TaskUpdateFunc)task_poo_freeballoon_1,
    (TaskDrawFunc)task_poo_freeballoon_2,
    (TaskDestroyFunc)task_poo_freeballoon_3,
    sizeof(PooFreeBalloonWork),
};

TaskDesc gTaskDescPooGauge = {
    "task_poo_gauge",
    (TaskInitFunc)task_poo_gauge_0,
    (TaskUpdateFunc)task_poo_gauge_1,
    (TaskDrawFunc)task_poo_gauge_2,
    (TaskDestroyFunc)task_poo_gauge_3,
    sizeof(PooGaugeWork),
};

TaskDesc gTaskDescPooTrapballoon = {
    "task_poo_trapballoon",
    (TaskInitFunc)task_poo_trapballoon_0,
    (TaskUpdateFunc)task_poo_trapballoon_1,
    (TaskDrawFunc)task_poo_trapballoon_2,
    (TaskDestroyFunc)task_poo_trapballoon_3,
    sizeof(PooBalloonWork),
};

TaskDesc gTaskDescPooOwlballoon = {
    "task_poo_owlballoon",
    (TaskInitFunc)task_poo_owlballoon_0,
    (TaskUpdateFunc)task_poo_owlballoon_1,
    (TaskDrawFunc)task_poo_owlballoon_2,
    (TaskDestroyFunc)task_poo_owlballoon_3,
    sizeof(PooOwlBalloonWork),
};

TaskDesc gTaskDescPooHoney = {
    "task_poo_honey",
    (TaskInitFunc)task_poo_honey_0,
    (TaskUpdateFunc)task_poo_honey_1,
    (TaskDrawFunc)task_poo_honey_2,
    (TaskDestroyFunc)task_poo_honey_3,
    sizeof(PooHoneyWork),
};

static const BosMapanimeFrame sPooMapanimeFrames0[4] = { { 18, 0 }, { 18, 1 }, { 18, 2 }, { 18, 3 } };

static const BosMapanimeFrame sPooMapanimeFrames1[4] = { { 18, 0 }, { 18, 1 }, { 18, 2 }, { 18, 3 } };

const BosMapanimeDef gPooMapanimeDef0 = { sPooMapanimeFrames0, ARRAY_COUNT(sPooMapanimeFrames0), gPooMapanime0Tiles, 0x2080, 0x0240, 0x0400, 3 };

const BosMapanimeDef gPooMapanimeDef1 = { sPooMapanimeFrames1, ARRAY_COUNT(sPooMapanimeFrames1), gPooMapanime1Tiles, 0x3880, 0x0240, 0x0400, 3 };

TaskDesc gTaskDescPooMapanime = {
    "task_poo_mapanime",
    (TaskInitFunc)task_poo_mapanime_0,
    (TaskUpdateFunc)task_poo_mapanime_1,
    task_poo_mapanime_2,
    task_poo_mapanime_3,
    sizeof(PooMapAnimeWork),
};

TaskDesc gTaskDescPooPile = {
    "task_poo_pile",
    (TaskInitFunc)task_poo_pile_0,
    (TaskUpdateFunc)task_poo_pile_1,
    (TaskDrawFunc)task_poo_pile_2,
    (TaskDestroyFunc)task_poo_pile_3,
    sizeof(PooPileWork),
};

const s32 gPooPileKindStages[6] = { 8, 0, 2, 4, 6, 7 };

TaskDesc gTaskDescPooTigerstump = {
    "task_poo_tigerstump",
    (TaskInitFunc)task_poo_tigerstump_0,
    (TaskUpdateFunc)task_poo_tigerstump_1,
    (TaskDrawFunc)task_poo_tigerstump_2,
    (TaskDestroyFunc)task_poo_tigerstump_3,
    sizeof(PooStumpWork),
};

TaskDesc gTaskDescPooPoohstump = {
    "task_poo_poohstump",
    (TaskInitFunc)task_poo_poohstump_0,
    (TaskUpdateFunc)task_poo_poohstump_1,
    (TaskDrawFunc)task_poo_poohstump_2,
    (TaskDestroyFunc)task_poo_poohstump_3,
    sizeof(PooStumpWork),
};

const PooAnimDesc gPooPigletAnimDescs[4] = {
    { gPigletStandFrontAnims, gPigletStandFrontFrames, gPigletStandFrontTiles, 0 },
    { gPigletStandBackAnims, gPigletStandBackFrames, gPigletStandBackTiles, 0 },
    { gPigletWalkFrontAnims, gPigletWalkFrontFrames, gPigletWalkFrontTiles, 0 },
    { gPigletWalkBackAnims, gPigletWalkBackFrames, gPigletWalkBackTiles, 0 },
};

const PooGfxDesc gPooPigletGfxDescs[4] = {
    { gPigletStandFrontFrames, ARRAY_COUNT(gPigletStandFrontFrames) },
    { gPigletStandBackFrames, ARRAY_COUNT(gPigletStandBackFrames) },
    { gPigletWalkFrontFrames, ARRAY_COUNT(gPigletWalkFrontFrames) },
    { gPigletWalkBackFrames, ARRAY_COUNT(gPigletWalkBackFrames) },
};

TaskDesc gTaskDescPooPiglet = {
    "task_poo_piglet",
    (TaskInitFunc)task_poo_piglet_0,
    (TaskUpdateFunc)task_poo_piglet_1,
    (TaskDrawFunc)task_poo_piglet_2,
    (TaskDestroyFunc)task_poo_piglet_3,
    sizeof(PooPigletWork),
};

TaskDesc gTaskDescPooEeyore = {
    "task_poo_eeyore",
    (TaskInitFunc)task_poo_eeyore_0,
    (TaskUpdateFunc)task_poo_eeyore_1,
    (TaskDrawFunc)task_poo_eeyore_2,
    (TaskDestroyFunc)task_poo_eeyore_3,
    sizeof(PooEeyoreWork),
};

TaskDesc gTaskDescPooOwl = {
    "task_poo_owl",
    (TaskInitFunc)task_poo_owl_0,
    (TaskUpdateFunc)task_poo_owl_1,
    (TaskDrawFunc)task_poo_owl_2,
    (TaskDestroyFunc)task_poo_owl_3,
    sizeof(PooOwlWork),
};

const PooAnimDesc gPooRabbitAnimDescs[7] = {
    { gRabbitFl00Anims, gRabbitFl00Frames, gRabbitFl00Tiles, 0 },
    { gRabbitFl00Anims, gRabbitFl00Frames, gRabbitFl00Tiles, 1 },
    { gRabbitFl00Anims, gRabbitFl00Frames, gRabbitFl00Tiles, 2 },
    { gRabbitBl00Anims, gRabbitBl00Frames, gRabbitBl00Tiles, 0 },
    { gRabbitBl00Anims, gRabbitBl00Frames, gRabbitBl00Tiles, 2 },
    { gRabbitBl00Anims, gRabbitBl00Frames, gRabbitBl00Tiles, 3 },
    { gRabbitBl00Anims, gRabbitBl00Frames, gRabbitBl00Tiles, 4 },
};

const PooGfxDesc gPooRabbitGfxDescs[2] = {
    { gRabbitFl00Frames, 14 },
    { gRabbitBl00Frames, ARRAY_COUNT(gRabbitBl00Frames) },
};

TaskDesc gTaskDescPooRabbit = {
    "task_poo_rabbit",
    (TaskInitFunc)task_poo_rabbit_0,
    (TaskUpdateFunc)task_poo_rabbit_1,
    (TaskDrawFunc)task_poo_rabbit_2,
    (TaskDestroyFunc)task_poo_rabbit_3,
    sizeof(PooRabbitWork),
};

const PooAnimDesc gPooTiggerAnimDescs[4] = {
    { gTiggerFl00Anims, gTiggerFl00Frames, gTiggerFl00Tiles, 0 },
    { gTiggerFl01Anims, gTiggerFl01Frames, gTiggerFl01Tiles, 0 },
    { gTiggerFl02Anims, gTiggerFl02Frames, gTiggerFl02Tiles, 0 },
    { gTiggerBl02Anims, gTiggerBl02Frames, gTiggerBl02Tiles, 0 },
};

const PooGfxDesc gPooTiggerGfxDescs[4] = {
    { gTiggerFl00Frames, ARRAY_COUNT(gTiggerFl00Frames) },
    { gTiggerFl01Frames, ARRAY_COUNT(gTiggerFl01Frames) },
    { gTiggerFl02Frames, ARRAY_COUNT(gTiggerFl02Frames) },
    { gTiggerBl02Frames, ARRAY_COUNT(gTiggerBl02Frames) },
};

const s32 gPooTiggerHopCorners[8] = { 428288, 232704, 411904, 240896, 428288, 249088, 444672, 240896 };

const s32 gPooTiggerHopHeights[8] = { -512, -2560, -4096, -5120, -5120, -4096, -2560, -512 };

TaskDesc gTaskDescPooTigger = {
    "task_poo_tigger",
    (TaskInitFunc)task_poo_tigger_0,
    (TaskUpdateFunc)task_poo_tiggerroo_1,
    (TaskDrawFunc)task_poo_tiggerroo_2,
    (TaskDestroyFunc)task_poo_tiggerroo_3,
    sizeof(PooTiggerWork),
};

TaskDesc gTaskDescPooTiggerroo = {
    "task_poo_tiggerroo",
    (TaskInitFunc)task_poo_tiggerroo_0,
    (TaskUpdateFunc)task_poo_tiggerroo_1,
    (TaskDrawFunc)task_poo_tiggerroo_2,
    (TaskDestroyFunc)task_poo_tiggerroo_3,
    sizeof(PooTiggerWork),
};

TaskDesc gTaskDescPooRoo = {
    "task_poo_roo",
    (TaskInitFunc)task_poo_roo_0,
    (TaskUpdateFunc)task_poo_roo_1,
    (TaskDrawFunc)task_poo_roo_2,
    (TaskDestroyFunc)task_poo_roo_3,
    sizeof(PooRooWork),
};

TaskDesc gTaskDescPooRooFootmark = {
    "task_poo_roo_footmark",
    (TaskInitFunc)task_poo_roo_footmark_0,
    (TaskUpdateFunc)task_poo_roo_footmark_1,
    (TaskDrawFunc)task_poo_roo_footmark_2,
    (TaskDestroyFunc)task_poo_roo_footmark_3,
    sizeof(PooFootmarkWork),
};

TaskDesc gTaskDescPooLeaf = {
    "task_poo_leaf",
    (TaskInitFunc)task_poo_leaf_0,
    (TaskUpdateFunc)task_poo_leaf_1,
    (TaskDrawFunc)task_poo_leaf_2,
    (TaskDestroyFunc)task_poo_leaf_3,
    sizeof(PooLeafWork),
};

TaskDesc gTaskDescPooTanpopo = {
    "task_poo_tanpopo",
    (TaskInitFunc)task_poo_tanpopo_0,
    (TaskUpdateFunc)task_poo_tanpopo_1,
    (TaskDrawFunc)task_poo_tanpopo_2,
    (TaskDestroyFunc)task_poo_tanpopo_3,
    sizeof(PooTanpopoWork),
};

TaskDesc gTaskDescPooTiBoard = {
    "task_poo_ti_board",
    (TaskInitFunc)task_poo_ti_board_0,
    (TaskUpdateFunc)task_poo_ti_board_1,
    (TaskDrawFunc)task_poo_ti_board_2,
    (TaskDestroyFunc)task_poo_ti_board_3,
    sizeof(PooBoardWork),
};

TaskDesc gTaskDescPooEeyoretail = {
    "task_poo_eeyoretail",
    (TaskInitFunc)task_poo_eeyoretail_0,
    (TaskUpdateFunc)task_poo_eeyoretail_1,
    (TaskDrawFunc)task_poo_eeyoretail_2,
    (TaskDestroyFunc)task_poo_eeyoretail_3,
    sizeof(PooEeyoreTailWork),
};

TaskDesc gTaskDescPooHoneycomb = {
    "task_poo_honeycomb",
    (TaskInitFunc)task_poo_honeycomb_0,
    (TaskUpdateFunc)task_poo_honeycomb_1,
    (TaskDrawFunc)task_poo_honeycomb_2,
    (TaskDestroyFunc)task_poo_honeycomb_3,
    sizeof(PooHoneycombWork),
};

TaskDesc gTaskDescPooVegetable = {
    "task_poo_vegetable",
    (TaskInitFunc)task_poo_vegetable_0,
    (TaskUpdateFunc)task_poo_vegetable_1,
    (TaskDrawFunc)task_poo_vegetable_2,
    (TaskDestroyFunc)task_poo_vegetable_3,
    sizeof(PooVegetableWork),
};

TaskDesc gTaskDescPooWagon = {
    "task_poo_wagon",
    (TaskInitFunc)task_poo_wagon_0,
    (TaskUpdateFunc)task_poo_wagon_1,
    (TaskDrawFunc)task_poo_wagon_2,
    (TaskDestroyFunc)task_poo_wagon_3,
    sizeof(PooWagonWork),
};

TaskDesc gTaskDescPooWagonwheel = {
    "task_poo_wagonwheel",
    (TaskInitFunc)task_poo_wagonwheel_0,
    (TaskUpdateFunc)task_poo_wagonwheel_1,
    (TaskDrawFunc)task_poo_wagonwheel_2,
    (TaskDestroyFunc)task_poo_wagonwheel_3,
    sizeof(PooWheelWork),
};

TaskDesc gTaskDescPooSpark = {
    "task_poo_spark",
    (TaskInitFunc)task_poo_spark_0,
    (TaskUpdateFunc)task_poo_spark_1,
    (TaskDrawFunc)task_poo_spark_2,
    (TaskDestroyFunc)task_poo_spark_3,
    sizeof(PooSparkWork),
};

const PooPoint gPooBeePoints[4] = { { -512, -1792 }, { -2304, 512 }, { 2304, -768 }, { 768, 1792 } };

TaskDesc gTaskDescPooBee = {
    "task_poo_bee",
    (TaskInitFunc)task_poo_bee_0,
    (TaskUpdateFunc)task_poo_bee_1,
    (TaskDrawFunc)task_poo_bee_2,
    (TaskDestroyFunc)task_poo_bee_3,
    sizeof(PooBeeWork),
};

TaskDesc gTaskDescPooBeeAfterEvent = {
    "task_poo_beeAfterEvent",
    (TaskInitFunc)task_poo_beeAfterEvent_0,
    (TaskUpdateFunc)task_poo_beeAfterEvent_1,
    (TaskDrawFunc)task_poo_beeAfterEvent_2,
    (TaskDestroyFunc)task_poo_beeAfterEvent_3,
    sizeof(PooBeeAfterEventWork),
};

const PooSpot gPooCabbageStackOffsets[18] = {
    { 0, 0, 0 },
    { -2560, -1280, 0 },
    { -5120, -2560, 0 },
    { -2560, -3840, 0 },
    { 0, -2560, 0 },
    { 2560, -1280, 0 },
    { 5120, -2560, 0 },
    { 2560, -3840, 0 },
    { 0, -5120, 0 },
    { 0, -640, -3200 },
    { -2560, -1920, -3200 },
    { 0, -3200, -3200 },
    { 2560, -1920, -3200 },
    { 0, -1280, -5888 },
    { 61440, 40960, 0 },
    { 0, 40960, 0 },
    { 61440, 0, 0 },
    { 0, 0, 0 },
};

const u16 gPooCabbageRemoveCounts[15] = { 6, 6, 6, 6, 6, 9, 9, 9, 65535, 65535, 65535, 65535, 65535, 65535, 65535 };

const u16 gPooCabbageStackPriorities[5] = { 3, 2, 1, 2, 4 };

TaskDesc gTaskDescPooCabbage = {
    "task_poo_cabbage",
    (TaskInitFunc)task_poo_cabbage_0,
    (TaskUpdateFunc)task_poo_cabbage_1,
    (TaskDrawFunc)task_poo_cabbage_2,
    (TaskDestroyFunc)task_poo_cabbage_3,
    sizeof(PooCabbageWork),
};

TaskDesc gTaskDescPooCabbageborn = {
    "task_poo_cabbageborn",
    (TaskInitFunc)task_poo_cabbageborn_0,
    (TaskUpdateFunc)task_poo_cabbageborn_1,
    (TaskDrawFunc)task_poo_cabbageborn_2,
    (TaskDestroyFunc)task_poo_cabbageborn_3,
    sizeof(PooCabbageBornWork),
};

const PooMapObjHitDesc gPooMapObjHitDescs[10] = {
    { gPooMapObjHit0Tiles, 5, gPooMapObjHit0Anims, gPooMapObjHit0Frames, gPooMapObjHit0Palette },
    { gPooMapObjHit1Tiles, ARRAY_COUNT(gPooMapObjHit1Frames), gPooMapObjHit1Anims, gPooMapObjHit1Frames, gPooMapObjHit1Palette },
    { gPooMapObjHit2Tiles, ARRAY_COUNT(gPooMapObjHit2Frames), gPooMapObjHit2Anims, gPooMapObjHit2Frames, gPooMapObjHit2Palette },
    { gPooMapObjHit3Tiles, ARRAY_COUNT(gPooMapObjHit3Frames), gPooMapObjHit3Anims, gPooMapObjHit3Frames, gPooMapObjHit3Palette },
    { gPooMapObjHit4Tiles, ARRAY_COUNT(gPooMapObjHit4Frames), gPooMapObjHit4Anims, gPooMapObjHit4Frames, gPooMapObjHit4Palette },
    { gPooMapObjHit5Tiles, ARRAY_COUNT(gPooMapObjHit5Frames), gPooMapObjHit5Anims, gPooMapObjHit5Frames, gPooMapObjHit5Palette },
    { gPooMapObjHit6Tiles, ARRAY_COUNT(gPooMapObjHit6Frames), gPooMapObjHit6Anims, gPooMapObjHit6Frames, gPooMapObjHit6Palette },
    { gPooMapObjHit7Tiles, ARRAY_COUNT(gPooMapObjHit7Frames), gPooMapObjHit7Anims, gPooMapObjHit7Frames, gPooMapObjHit6Palette },
    { gPooMapObjHit8Tiles, ARRAY_COUNT(gPooMapObjHit8Frames), gPooMapObjHit8Anims, gPooMapObjHit8Frames, gPooMapObjHit8Palette },
    { gPooMapObjHit9Tiles, ARRAY_COUNT(gPooMapObjHit9Frames), gPooMapObjHit9Anims, gPooMapObjHit9Frames, gPooMapObjHit8Palette },
};

TaskDesc gTaskDescPooMapobjhit = {
    "task_poo_mapobjhit",
    (TaskInitFunc)task_poo_mapobjhit_0,
    (TaskUpdateFunc)task_poo_mapobjhit_1,
    (TaskDrawFunc)task_poo_mapobjhit_2,
    (TaskDestroyFunc)task_poo_mapobjhit_3,
    sizeof(PooMapObjHitWork),
};

TaskDesc gTaskDescPooPrize = {
    "task_poo_prize",
    (TaskInitFunc)task_poo_prize_0,
    (TaskUpdateFunc)task_poo_prize_1,
    (TaskDrawFunc)task_poo_prize_2,
    (TaskDestroyFunc)task_poo_prize_3,
    sizeof(PooPrizeWork),
};

TaskDesc gTaskDescPooZzz = {
    "task_poo_zzz",
    (TaskInitFunc)task_poo_zzz_0,
    (TaskUpdateFunc)task_poo_zzz_1,
    (TaskDrawFunc)task_poo_zzz_2,
    (TaskDestroyFunc)task_poo_zzz_3,
    sizeof(PooZzzWork),
};

AnimDef gTrap01AnimDefs[5] = {
    { gTrap01Bb00Frames, gTrap01Bb00Anims, gTrap01Bb00Tiles, 0 },
    { gTrap01Ff00Frames, gTrap01Ff00Anims, gTrap01Ff00Tiles, 0 },
    { gTrap01Fl00Frames, gTrap01Fl00Anims, gTrap01Fl00Tiles, 0 },
    { gTrap01Ll00Frames, gTrap01Ll00Anims, gTrap01Ll00Tiles, 0 },
    { gTrap01Bl00Frames, gTrap01Bl00Anims, gTrap01Bl00Tiles, 0 },
};

TaskDesc gTaskDescPooButterfly = {
    "task_poo_butterfly",
    (TaskInitFunc)task_poo_butterfly_0,
    (TaskUpdateFunc)task_poo_butterfly_1,
    (TaskDrawFunc)task_poo_butterfly_2,
    (TaskDestroyFunc)task_poo_butterfly_3,
    sizeof(PooButterflyWork),
};

TaskDesc gTaskDescPooButterflyRight = {
    "task_poo_butterflyRight",
    (TaskInitFunc)task_poo_butterfly_0,
    (TaskUpdateFunc)task_poo_butterflyRight_1,
    (TaskDrawFunc)task_poo_butterflyRight_2,
    (TaskDestroyFunc)task_poo_butterfly_3,
    sizeof(PooButterflyWork),
};

TaskDesc gTaskDescPooButterflyLeft = {
    "task_poo_butterflyLeft",
    (TaskInitFunc)task_poo_butterfly_0,
    (TaskUpdateFunc)task_poo_butterflyLeft_1,
    (TaskDrawFunc)task_poo_butterflyLeft_2,
    (TaskDestroyFunc)task_poo_butterfly_3,
    sizeof(PooButterflyWork),
};

TaskDesc gTaskDescPooMapbee = {
    "task_poo_mapbee",
    (TaskInitFunc)task_poo_mapbee_0,
    (TaskUpdateFunc)task_poo_mapbee_1,
    (TaskDrawFunc)task_poo_mapbee_2,
    (TaskDestroyFunc)task_poo_mapbee_3,
    sizeof(PooMapBeeWork),
};

TaskDesc gTaskDescPooMapbeeborn = {
    "task_poo_mapbeeborn",
    (TaskInitFunc)task_poo_mapbeeborn_0,
    (TaskUpdateFunc)task_poo_mapbeeborn_1,
    (TaskDrawFunc)task_poo_mapbeeborn_2,
    (TaskDestroyFunc)task_poo_mapbeeborn_3,
    sizeof(PooMapBornWork),
};

TaskDesc gTaskDescPooMapbutterfly = {
    "task_poo_mapbutterfly",
    (TaskInitFunc)task_poo_mapbutterfly_0,
    (TaskUpdateFunc)task_poo_mapbutterfly_1,
    (TaskDrawFunc)task_poo_mapbutterfly_2,
    (TaskDestroyFunc)task_poo_mapbutterfly_3,
    sizeof(PooMapButterflyWork),
};

TaskDesc gTaskDescPooMapbutterflyborn = {
    "task_poo_mapbutterflyborn",
    (TaskInitFunc)task_poo_mapbutterflyborn_0,
    (TaskUpdateFunc)task_poo_mapbutterflyborn_1,
    (TaskDrawFunc)task_poo_mapbutterflyborn_2,
    (TaskDestroyFunc)task_poo_mapbutterflyborn_3,
    sizeof(PooMapBornWork),
};

TaskDesc gTaskDescPooRabbitAfterEvent = {
    "task_poo_rabbitAfterEvent",
    (TaskInitFunc)task_poo_rabbitAfterEvent_0,
    (TaskUpdateFunc)task_poo_rabbitAfterEvent_1,
    (TaskDrawFunc)task_poo_rabbitAfterEvent_2,
    (TaskDestroyFunc)task_poo_rabbitAfterEvent_3,
    sizeof(PooRabbitAfterEventWork),
};

TaskDesc gTaskDescPooCabbageAfterEvent = {
    "task_poo_cabbageAfterEvent",
    (TaskInitFunc)task_poo_cabbageAfterEvent_0,
    (TaskUpdateFunc)task_poo_cabbageAfterEvent_1,
    (TaskDrawFunc)task_poo_cabbageAfterEvent_2,
    (TaskDestroyFunc)task_poo_cabbageAfterEvent_3,
    sizeof(PooCabbageAfterEventWork),
};

const s32 gPooMainEventIds[6] = {
    POO_EVENT_PIGLET,
    POO_EVENT_TIGGER,
    POO_EVENT_EEYORE,
    POO_EVENT_OWL,
    POO_EVENT_RABBIT,
    POO_EVENT_ROO,
};

