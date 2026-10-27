/**
 * port_testbot.c (linked after port_state.c: editing it keeps save states valid)
 * PC port: a playthrough bot for gameplay verification (KHCOM_BOT=1).
 *
 * It plays from power-on with ordinary pad input decided from the game's own
 * state each frame: no state is written, no fixture or warp is used. Events,
 * menus and messages are confirmed with A; on the field Sora walks to a door
 * that leads toward unexplored rooms (event doors first) and strikes it; in
 * battles the nearest enemy is approached and attacked, holding A on reload.
 */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "types.h"
#include "field_state.h"
#include "map.h"
#include "map_runtime.h"
#include "map_api.h"
#include "battle_work.h"
#include "battle_actor.h"
#include "listpool.h"
#include "game_state.h"
#include "card_msgwin.h"
#include "card_api.h"
#include "card_ids.h"

extern void PortLog(const char* format, ...);
u16 CountMapCards(void); /* card_worldselect.c */
extern const char* PortHarnessModeName(void);
u8 IsMessageWindowOpen();

enum { K_A = 1, K_B = 2, K_SELECT = 4, K_START = 8, K_RIGHT = 16, K_LEFT = 32, K_UP = 64, K_DOWN = 128, K_R = 256, K_L = 512 };

static int sEnabled = -1;
static uint32_t sLastLog;
static int sMenuFrames, sNeedCards, sNeedFrames, sNoProgress, sDetour, sBestDist = 1 << 30;
static int sTargetSide = -1;
static uint8_t sTriedSide[32][4];
static FldPos* sDoorPos[4];

/* map_cell.c: everything strikeable on the field tests the attack each
   frame; keep this frame's and last frame's positions. */
static struct { int x, y, z; } sStrike[2][24];
static int sStrikeCount[2];
static uint32_t sStrikeFrame;
static uint8_t sStruck[64];
static uint16_t sReach[64];
extern uint32_t PortFrameCount(void);

void PortBotStrikeable(const FldPos* pos) {
    uint32_t f = PortFrameCount();
    int i, n;
    if (f != sStrikeFrame) {
        sStrikeCount[1] = sStrikeCount[0];
        memcpy(sStrike[1], sStrike[0], sizeof(sStrike[0]));
        sStrikeCount[0] = 0;
        sStrikeFrame = f;
    }
    n = sStrikeCount[0];
    for (i = 0; i < n; i++)
        if (sStrike[0][i].x == pos->x >> 8 && sStrike[0][i].y == pos->y >> 8)
            return;
    if (n < 24) {
        sStrike[0][n].x = pos->x >> 8;
        sStrike[0][n].y = pos->y >> 8;
        sStrike[0][n].z = pos->z >> 8;
        sStrikeCount[0] = n + 1;
    }
}

/* map_tasks.c: the room's save point (its first talk starts the map tutorial). */
static FldPos* sSavePoint;
void PortBotSavePoint(FldPos* pos) {
    sSavePoint = pos;
}

/* map_tasks.c: a map card prize waiting to be picked up. */
static int sPrizeX, sPrizeY, sPrizeZ;
static uint32_t sPrizeFrame;
void PortBotPrize(int x, int y, int z) {
    sPrizeX = x; sPrizeY = y; sPrizeZ = z;
    sPrizeFrame = PortFrameCount();
}

/* map_stair.c: the halls' stairs. Up-right goes forward (into the world
   from the entrance hall, to the next floor from the exit hall). */
static uint16_t Toward(int dx, int dy, int dead);
static FldPos* sStairForward;
void PortBotStair(int angle, FldPos* pos) {
    if (angle == FLD_ANGLE_UP_RIGHT) sStairForward = pos;
}

static uint16_t HallKeys(uint32_t frame) {
    FldPos* p = &gFieldState->actor.fieldPosition;
    int dx, dy;
    if (IsMessageWindowOpen() || (gFieldState->flags & (FIELD_FLAG_MENU_OPEN | FIELD_FLAG_ROOM_CREATE)))
        return (frame % 40) < 2 ? K_A : ((frame % 40) >= 20 && (frame % 40) < 22 && ++sMenuFrames > 200 ? K_RIGHT : 0);
    sMenuFrames = 0;
    if (sStairForward == NULL || (sStairForward->x >> 8) <= 0) {
        /* a fixed room without stairs (Traverse Town's first room): strike
           its door and step onto it */
        int i, bi = -1, bd = 1 << 30;
        for (i = 0; i < sStrikeCount[1]; i++) {
            int d = abs(sStrike[1][i].x - (p->x >> 8)) + abs(sStrike[1][i].y - (p->y >> 8));
            if (d < bd) { bd = d; bi = i; }
        }
        if (bi < 0)
            return (uint16_t)(((frame / 120) % 2 ? K_UP : K_RIGHT) | ((frame % 30) < 2 ? K_A : 0));
        dx = sStrike[1][bi].x - (p->x >> 8);
        dy = sStrike[1][bi].y - (p->y >> 8);
        if ((frame % 300) == 0)
            PortLog("bot: fixed room target %d,%d player %d,%d", sStrike[1][bi].x, sStrike[1][bi].y,
                    (int)(p->x >> 8), (int)(p->y >> 8));
        return Toward(dx, dy, 1) | ((abs(dx) < 20 && abs(dy) < 14 && (frame % 16) < 2) ? K_A : 0);
    }
    dx = (sStairForward->x - p->x) >> 8;
    dy = (sStairForward->y - p->y) >> 8;
    if ((frame % 300) == 0)
        PortLog("bot: hall stair %d,%d player %d,%d", (int)(sStairForward->x >> 8), (int)(sStairForward->y >> 8),
                (int)(p->x >> 8), (int)(p->y >> 8));
    return Toward(dx, dy, 1) | (((frame / 200) % 4 == 3 && (frame % 40) < 3) ? K_B : 0);
}

/* map.c: a door task records where it can be struck (NULL when it goes). */
void PortBotDoor(int side, FldPos* pos) {
    if ((unsigned)side < 4)
        sDoorPos[side] = pos;
}

/* Explicit test fixture only; the playthrough bot never calls this. The
   following A input still enters the door through normal collision/tasks. */
int PortHarnessDoorPose(int eventDoor) {
    static const int dx[4] = {-14, 14, -14, 14};
    static const int dy[4] = {8, -8, -8, 8};
    static const int angle[4] = {FLD_ANGLE_UP_RIGHT, FLD_ANGLE_DOWN_LEFT,
                               FLD_ANGLE_DOWN_RIGHT, FLD_ANGLE_UP_LEFT};
    int side, card;
    for (side = 0; side < 4; side++) {
        MapDoor* door = GetMapDoor(side);
        if (!sDoorPos[side] || !(door->flags & DOOR_FLAG_PRESENT) ||
            (door->flags & DOOR_FLAG_OPEN) || !!(door->flags & DOOR_FLAG_EVENT) != !!eventDoor)
            continue;
        gFieldState->actor.fieldPosition = *sDoorPos[side];
        gFieldState->actor.fieldPosition.x += dx[side] * 256;
        gFieldState->actor.fieldPosition.y += dy[side] * 256;
        gFieldState->actor.angle = angle[side];
        MapSetCameraTarget(gFieldState->actor.fieldPosition.x, gFieldState->actor.fieldPosition.y);
        if (eventDoor == 3) {
            extern void ClearMapCardInventory(void);
            ClearMapCardInventory(); /* minimal red card + keys, separate from palette pressure */
            AddMapCard(MAP_CARD_ID(MAP_CARD_GROUP_TRANQUIL_DARKNESS, 0));
        } else {
            for (card = 0; card < 270; card += 10) AddMapCard(card);
        }
        /* doorpose:2 adds story keys to the pressure case; :3 uses minimal cards.
           Both can exercise payment/entry instead of stopping at the selector. */
        if (eventDoor >= 2)
            for (card = MAP_CARD_GROUP_KEY_OF_BEGINNINGS; card <= MAP_CARD_GROUP_KEY_TO_REWARDS; card++)
                AddMapCard(MAP_CARD_ID(card, 1));
        PortLog("harness: door pose side=%d event=%d room=%d", side, eventDoor, door->room);
        return 1;
    }
    PortLog("harness: door pose unavailable event=%d", eventDoor);
    return 0;
}

static uint16_t Toward(int dx, int dy, int dead) {
    uint16_t k = 0;
    if (dx > dead) k |= K_RIGHT;
    else if (dx < -dead) k |= K_LEFT;
    if (dy > dead) k |= K_DOWN;
    else if (dy < -dead) k |= K_UP;
    return k;
}

static int RoomVisited(int room) {
    return room < 32 && (gMapFloorState.rooms[room].flags & FLOOR_ROOM_FLAG_VISITED);
}

/* First step (door side from `from`) of the shortest walk over rooms already
   created to a room satisfying `goal`; -1 when none is reachable. */
static int PathStep(int from, int (*goal)(int room, int* side)) {
    uint8_t seen[32] = {0}, queue[32], first[32];
    int head = 0, count = 0, side, goalSide;

    if (from >= 32) return -1;
    seen[from] = 1;
    queue[count++] = (uint8_t)from;
    first[from] = 0xFF;
    while (head < count) {
        int room = queue[head++];
        if (goal(room, &goalSide))
            return room == from ? goalSide : first[room];
        for (side = 0; side < 4; side++) {
            int next = GetMapRoomLink((u8)room, (u8)side);
            if (next >= 32 || seen[next] || !RoomVisited(next))
                continue;
            seen[next] = 1;
            first[next] = (uint8_t)(room == from ? side : first[room]);
            queue[count++] = (uint8_t)next;
        }
    }
    return -1;
}

static uint8_t sEventTried[32];

/* A story door (event room, boss room) not yet taken, tried fewer than three times. */
static int GoalEventDoor(int room, int* side) {
    int i;
    for (i = 0; i < 32; i++) {
        MapEventDoor* ed = GetMapEventDoor((u8)i);
        int next;
        if (ed->kind == EVENT_DOOR_END)
            break;
        if (ed->room != room || ed->keyList == EVENT_KEY_LIST_NONE || sEventTried[i] >= 3)
            continue;
        next = GetMapRoomLink((u8)room, ed->side);
        if (next < 32 && (gMapFloorState.rooms[next].flags & FLOOR_ROOM_FLAG_EVENT_DONE))
            continue;
        *side = ed->side;
        return 1;
    }
    return 0;
}

/* A room with a door leading to a room not yet created. */
static int GoalUnexplored(int room, int* side) {
    int s;
    for (s = 0; s < 4; s++) {
        int next = GetMapRoomLink((u8)room, (u8)s);
        if (next < 32 && !RoomVisited(next) && sTriedSide[room][s] < 3) {
            *side = s;
            return 1;
        }
    }
    return 0;
}

/* The exit hall, once nothing else is left. */
static int GoalExit(int room, int* side) {
    int s, exitRoom = GetMapFloorDef(gGameState.floor)->exitRoom;
    for (s = 0; s < 4; s++)
        if (GetMapRoomLink((u8)room, (u8)s) == exitRoom) {
            *side = s;
            return 1;
        }
    return 0;
}

static int sGoalKind;

/* Field: which door of this room to take. */
static int ChooseDoor(int room) {
    int side;
    {
        int i, sd;
        for (i = 0; i < 32 && GetMapEventDoor((u8)i)->kind != EVENT_DOOR_END; i++) {
            MapEventDoor* ed = GetMapEventDoor((u8)i);
            PortLog("bot: eventdoor %d kind %d keys %d room %d side %d return %d/%d", i, ed->kind, ed->keyList,
                    ed->room, ed->side, ed->returnRoom, ed->returnSide);
        }
        for (sd = 0; sd < 4; sd++)
            PortLog("bot: room %d side %d link %d door flags %x to %d", room, sd, GetMapRoomLink((u8)room, (u8)sd),
                    GetMapDoor((u8)sd)->flags, GetMapDoor((u8)sd)->room);
    }
    if ((side = PathStep(room, GoalEventDoor)) >= 0) { sGoalKind = 1; return side; }
    if ((side = PathStep(room, GoalUnexplored)) >= 0) { sGoalKind = 2; return side; }
    if ((side = PathStep(room, GoalExit)) >= 0) { sGoalKind = 3; return side; }
    sGoalKind = 0;
    for (side = 0; side < 4; side++) {
        MapDoor* door = GetMapDoor(side);
        if ((door->flags & DOOR_FLAG_PRESENT) && !(door->flags & DOOR_FLAG_SEALED) && sTriedSide[room][side] < 3
            && door->room < 32)
            return side;
    }
    return -1;
}

/* Room cells: walkable floor, its height (smaller z is higher). */
static int CellFloor(int cx, int cy, int* z, int* flags) {
    MapCell* c;
    if (cx < 0 || cy < 0 || cx >= gMapRoomState->cols || cy >= gMapRoomState->rows)
        return 0;
    c = MapCellAt((s16)cx, (s16)cy);
    if (c == NULL || c->type != MAP_CELL_TYPE_FLOOR || c->upperZ == MAP_Z_TOP || c->upperZ == MAP_Z_BOTTOM)
        return 0;
    *z = c->upperZ >> 8;
    *flags = c->flags;
    return 1;
}

/* Next cell toward (gx, gy) over the room's floor: same height walks, lower
   drops, higher only at stairs or a jump pad. Returns 0 when unreachable;
   *jump set when the step needs B. */
static int NextCell(int sx, int sy, int gx, int gy, int* nx, int* ny, int* jump) {
    static int16_t prev[64 * 96];
    static uint16_t queue[64 * 96];
    int cols = gMapRoomState->cols, rows = gMapRoomState->rows, head = 0, count = 0, i;
    int z0, f0;
    if (cols <= 0 || rows <= 0 || cols * rows > 64 * 96)
        return 0;
    for (i = 0; i < cols * rows; i++) prev[i] = -1;
    if (!CellFloor(sx, sy, &z0, &f0)) {
        /* standing on a stair or between cells: search the neighbours */
        int dx2, dy2, found = 0;
        for (dy2 = -1; dy2 <= 1 && !found; dy2++)
            for (dx2 = -1; dx2 <= 1 && !found; dx2++)
                if (CellFloor(sx + dx2, sy + dy2, &z0, &f0)) { sx += dx2; sy += dy2; found = 1; }
        if (!found) return 0;
    }
    prev[sy * cols + sx] = (int16_t)(sy * cols + sx);
    queue[count++] = (uint16_t)(sy * cols + sx);
    while (head < count) {
        int cur = queue[head++], cx = cur % cols, cy = cur / cols, z, fl, d;
        static const int8_t step[8][2] = { {1,0},{-1,0},{0,1},{0,-1},{1,1},{-1,-1},{1,-1},{-1,1} };
        if (abs(cx - gx) <= 1 && abs(cy - gy) <= 1) {
            /* walk back to the first step */
            int at = cur, first = cur;
            while (prev[at] != at) { first = at; at = prev[at]; }
            *nx = first % cols;
            *ny = first / cols;
            {
                int za, fa, zb, fb;
                *jump = CellFloor(sx, sy, &za, &fa) && CellFloor(*nx, *ny, &zb, &fb) && zb < za - 4
                        && !((fa | fb) & MAP_CELL_FLAG_STAIRS);
            }
            return 1;
        }
        CellFloor(cx, cy, &z, &fl);
        for (d = 0; d < 8; d++) {
            int ex = cx + step[d][0], ey = cy + step[d][1], ez, ef, idx;
            if (!CellFloor(ex, ey, &ez, &ef))
                continue;
            idx = ey * cols + ex;
            if (prev[idx] >= 0)
                continue;
            if (d >= 4) { /* diagonal: both sides must be floor at a reachable height */
                int az, af, bz, bf;
                if (!CellFloor(cx + step[d][0], cy, &az, &af) || !CellFloor(cx, cy + step[d][1], &bz, &bf))
                    continue;
            }
            if (ez < z - 4 && !((fl | ef) & (MAP_CELL_FLAG_STAIRS | MAP_CELL_FLAG_JUMP_PAD)))
                continue; /* higher ground without a way up */
            prev[idx] = (int16_t)cur;
            queue[count++] = (uint16_t)idx;
        }
    }
    return 0;
}

/* Walk toward field point (tx, ty) at height tz: along the cell path when
   one exists, straight otherwise; B on climbs. */
static uint16_t WalkTo(uint32_t frame, int tx, int ty, int tz) {
    FldPos* p = &gFieldState->actor.fieldPosition;
    int px = p->x >> 8, py = p->y >> 8, nx, ny, jump;
    int pcx = p->x >> 13, pcy = (p->y + p->z) >> 12, gcx = tx >> 5, gcy = (ty + tz) >> 4;
    if ((frame % 300) == 77) {
        int ok = NextCell(pcx, pcy, gcx, gcy, &nx, &ny, &jump), z0 = 0, f0 = 0, z1 = 0, f1 = 0;
        int c0 = CellFloor(pcx, pcy, &z0, &f0), c1 = CellFloor(gcx, gcy, &z1, &f1);
        PortLog("bot: walk from cell %d,%d (floor %d z %d) to %d,%d (floor %d z %d flags %x) path %d next %d,%d jump %d",
                pcx, pcy, c0, z0, gcx, gcy, c1, z1, f1, ok, nx, ny, jump);
    }
    if (NextCell(pcx, pcy, gcx, gcy, &nx, &ny, &jump) && !(nx == pcx && ny == pcy)) {
        int wz, wf;
        CellFloor(nx, ny, &wz, &wf);
        return Toward(nx * 32 + 16 - px, ny * 16 + 8 - wz - py, 2) | ((jump && (frame % 30) < 3) ? K_B : 0);
    }
    return Toward(tx - px, ty - py, 2) | (((frame / 240) % 3 == 2 && (frame % 60) < 3) ? K_B : 0);
}

/* Through a door: its outward direction (sides: up-right, down-left,
   down-right, up-left). */
static uint16_t DoorOut(int side) {
    static const uint16_t out[4] = { K_UP | K_RIGHT, K_DOWN | K_LEFT, K_DOWN | K_RIGHT, K_UP | K_LEFT };
    return out[side & 3];
}

static uint16_t FieldKeys(uint32_t frame) {
    FldPos* p = &gFieldState->actor.fieldPosition;
    int room = gMapFloorState.room;
    int px = p->x >> 8, py = p->y >> 8;
    MapDoor* door;
    int dx, dy, tx, ty;

    if (gFieldState->flags & (FIELD_FLAG_MENU_OPEN | FIELD_FLAG_ROOM_CREATE)) {
        /* the door's card menu: try the cards in turn; when none is accepted
           (the door wants a value the hand lacks), back out and go win map
           cards from the room's Heartless */
        if (++sMenuFrames > 700) {
            sNeedCards = 1;
            return (frame % 30) < 2 ? K_B : 0;
        }
        return (frame % 40) < 2 ? K_A : ((frame % 40) >= 20 && (frame % 40) < 22 ? K_RIGHT : 0);
    }
    sMenuFrames = 0;
    if (IsMessageWindowOpen())
        return (frame % 16) < 2 ? K_A : 0;

    if (gMapRoomState->flags & ROOM_FLAG_SAVE_MENU_OPEN)
        return (frame % 30) < 2 ? K_B : 0; /* leave the save menu without saving */
    if (sSavePoint != NULL && !(gGameState.progression.tutorialFlags & TUTORIAL_FLAG_SAVE_POINT)) {
        /* the save point's first talk opens the field tutorial (barrel, battle) */
        if ((frame % 300) == 0)
            PortLog("bot: to the save point %d,%d player %d,%d", (int)(sSavePoint->x >> 8), (int)(sSavePoint->y >> 8), px, py);
        return WalkTo(frame, sSavePoint->x >> 8, sSavePoint->y >> 8, sSavePoint->z >> 8);
    }
    if (PortFrameCount() - sPrizeFrame < 3) {
        /* a card on the floor: pick it up first */
        if ((frame % 300) == 0)
            PortLog("bot: prize at %d,%d z %d player %d,%d", sPrizeX, sPrizeY, sPrizeZ, px, py);
        return WalkTo(frame, sPrizeX, sPrizeY - sPrizeZ, sPrizeZ);
    }
    if (sNeedCards && ++sNeedFrames > 2400) {
        /* nothing more to win here: back to the hall and in again, the room
           is created anew with its Heartless */
        int sd;
        for (sd = 0; sd < 4; sd++) {
            MapDoor* hd = GetMapDoor(sd);
            if ((hd->flags & DOOR_FLAG_PRESENT) && hd->room >= 32 && sDoorPos[sd] != NULL) {
                int hx = (sDoorPos[sd]->x >> 8) - px, hy = (sDoorPos[sd]->y >> 8) - py;
                if ((frame % 300) == 0)
                    PortLog("bot: back to the hall by side %d", sd);
                if (abs(hx) < 18 && abs(hy) < 14)
                    return DoorOut(sd);
                return WalkTo(frame, sDoorPos[sd]->x >> 8, sDoorPos[sd]->y >> 8, sDoorPos[sd]->z >> 8);
            }
        }
    }
    if (sNeedCards) {
        /* strike the nearest Heartless on this floor level to start a battle */
        Task* task;
        FldObj* best = NULL;
        int bestD = 1 << 30;
        /* field Heartless are MapEnmWork tasks in tasks4 */
        for (task = ListPoolFirst(&gFieldState->tasks4.head); task != NULL; task = ListPoolNext(&task->node)) {
            MapEnmWork* enm = task->work;
            FldObj* obj;
            int d;
            if (enm == NULL || task->desc == NULL || task->desc->workSize < (s32)sizeof(MapEnmWork) || enm->def == NULL)
                continue;
            obj = &enm->obj;
            if (obj->kind != FLD_OBJ_KIND_ENEMY)
                continue;
            d = abs((obj->fieldPosition.x >> 8) - px) + abs((obj->fieldPosition.y >> 8) - py);
            if (d < bestD) { bestD = d; best = obj; }
        }
        if ((frame % 300) == 0) {
            int n = 0;
            for (task = ListPoolFirst(&gFieldState->tasks4.head); task != NULL; task = ListPoolNext(&task->node))
                n++;
            PortLog("bot: hunt room enemiesLeft %d przLeft %d flags %x", gMapFloorState.rooms[room & 31].enemiesLeft,
                    gMapFloorState.rooms[room & 31].przCardsLeft, gMapFloorState.rooms[room & 31].flags);
            PortLog("bot: hunt tasks4 %d best %d,%d z %d player %d,%d z %d", n,
                    best ? (int)(best->fieldPosition.x >> 8) : -1, best ? (int)(best->fieldPosition.y >> 8) : -1,
                    best ? (int)(best->fieldPosition.z >> 8) : -1, px, py, (int)(p->z >> 8));
        }
        if (best != NULL) {
            dx = (best->fieldPosition.x >> 8) - px;
            dy = (best->fieldPosition.y >> 8) - py;
            if (abs(dx) < 20 && abs(dy) < 14)
                return (frame % 10) < 2 ? K_A : Toward(dx, dy, 2);
            return WalkTo(frame, best->fieldPosition.x >> 8, best->fieldPosition.y >> 8, best->fieldPosition.z >> 8);
        }
        /* no Heartless: strike the room's objects (lamps, barrels, crates
           drop map cards), each a few times, sweeping over what falls */
        {
            int i, bi = -1, bd = 1 << 30, h;
            for (i = 0; i < sStrikeCount[1]; i++) {
                int ox = sStrike[1][i].x, oy = sStrike[1][i].y, d, isDoor = 0, s2;
                for (s2 = 0; s2 < 4; s2++)
                    if (sDoorPos[s2] && (sDoorPos[s2]->x >> 8) == ox && (sDoorPos[s2]->y >> 8) == oy)
                        isDoor = 1;
                h = ((ox * 31 + oy) & 63);
                if (isDoor || sStruck[h] >= 6)
                    continue;
                d = abs(ox - px) + abs(oy - py);
                if (d < bd) { bd = d; bi = i; }
            }
            if (bi >= 0) {
                dx = sStrike[1][bi].x - px;
                dy = sStrike[1][bi].y - py;
                h = ((sStrike[1][bi].x * 31 + sStrike[1][bi].y) & 63);
                if ((frame % 300) == 0)
                    PortLog("bot: strike target %d,%d (%d objects) player %d,%d", sStrike[1][bi].x, sStrike[1][bi].y,
                            sStrikeCount[1], px, py);
                if (abs(dx) < 16 && abs(dy) < 12) {
                    if (gMapRoomState->attackActive && (frame % 20) == 1)
                        PortLog("bot: swing px %d py %d angle %x at %d,%d z %d target %d,%d z %d player z %d", px, py,
                                gFieldState->actor.angle, (int)(gMapRoomState->attackX >> 8),
                                (int)(gMapRoomState->attackY >> 8), (int)(gMapRoomState->attackZ >> 8), sStrike[1][bi].x,
                                sStrike[1][bi].y, sStrike[1][bi].z, (int)(p->z >> 8));
                    if ((frame % 20) == 0) sStruck[h]++;
                    /* stand beside it facing it, then strike */
                    if ((frame % 20) < 6)
                        return Toward(dx, dy, 0); /* face it */
                    return (frame % 20) < 9 ? K_A : 0;
                }
                if (++sReach[h] > 600) { sReach[h] = 0; sStruck[h] = 6; } /* out of reach: skip it */
                return WalkTo(frame, sStrike[1][bi].x, sStrike[1][bi].y, sStrike[1][bi].z);
            }
            if ((frame % 3000) == 0)
                memset(sStruck, 0, sizeof(sStruck)); /* objects may drop again later */
        }
        /* none here: wander and let them come, or move on */
        return (uint16_t)(((frame / 90) % 4 == 0 ? K_RIGHT : (frame / 90) % 4 == 1 ? K_DOWN :
                           (frame / 90) % 4 == 2 ? K_LEFT : K_UP));
    }
    if (sTargetSide < 0 || !(GetMapDoor(sTargetSide)->flags & DOOR_FLAG_PRESENT)) {
        sTargetSide = ChooseDoor(room);
        sBestDist = 1 << 30;
        sNoProgress = 0;
        sDetour = 0;
    }
    if (sTargetSide < 0)
        return (frame % 30) < 2 ? K_A : 0;
    door = GetMapDoor(sTargetSide);
    if (sDoorPos[sTargetSide] != NULL) {
        tx = sDoorPos[sTargetSide]->x >> 8;
        ty = sDoorPos[sTargetSide]->y >> 8;
    } else {
        tx = (door->cellX << 5) + 16;
        ty = (door->cellY << 4) + 8;
    }
    if ((frame % 300) == 0)
        PortLog("bot: field room %d goal %d door side %d target %d,%d player %d,%d z %d", room, sGoalKind, sTargetSide, tx, ty,
                px, py, (int)(p->z >> 8));
    dx = tx - px;
    dy = ty - py;

    if (abs(dx) < 14 && abs(dy) < 12) {
        sNoProgress = 0;
        if (door->flags & DOOR_FLAG_OPEN)
            return DoorOut(sTargetSide); /* open: walk through */
        return (frame % 12) < 2 ? K_A : Toward(dx, dy, 2); /* strike the door */
    }
    /* follow the cell path when there is one */
    {
        int pcx = p->x >> 13, pcy = (p->y + p->z) >> 12, gcx = door->cellX, gcy = door->cellY, nx, ny, jump;
        if (NextCell(pcx, pcy, gcx, gcy, &nx, &ny, &jump)) {
            int wz, wf, wx, wy;
            CellFloor(nx, ny, &wz, &wf);
            wx = nx * 32 + 16;
            wy = ny * 16 + 8 - wz;
            if ((frame % 300) == 150)
                PortLog("bot: path cell %d,%d -> %d,%d door %d,%d jump %d", pcx, pcy, nx, ny, gcx, gcy, jump);
            if (!(nx == pcx && ny == pcy) || abs(dx) > 40) {
                uint16_t k = Toward(wx - px, wy - py, 2);
                if (jump && (frame % 30) < 3) k |= K_B;
                if (sNoProgress < 600) {
                    if (abs(dx) + abs(dy) < sBestDist - 2) { sBestDist = abs(dx) + abs(dy); sNoProgress = 0; }
                    else sNoProgress++;
                    return k;
                }
            }
        }
    }
    /* steer: the eight directions ordered by how well they point at the door;
       when the distance stops shrinking (a wall, a ledge), take the next one
       for a while, so Sora slides along edges; jump now and then; give the
       door up after a long time without getting closer */
    {
        static const int8_t dirs[8][2] = { {1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1},{0,-1},{1,-1} };
        int order[8], score[8], i, j, dist = abs(dx) + abs(dy);
        for (i = 0; i < 8; i++) {
            order[i] = i;
            score[i] = dirs[i][0] * dx + dirs[i][1] * dy * 2;
            if (dirs[i][0] && dirs[i][1]) score[i] = score[i] * 7 / 10;
        }
        for (i = 0; i < 8; i++)
            for (j = i + 1; j < 8; j++)
                if (score[order[j]] > score[order[i]]) { int t = order[i]; order[i] = order[j]; order[j] = t; }
        if (dist < sBestDist - 2) {
            sBestDist = dist;
            sNoProgress = 0;
            if (sDetour > 0) sDetour--;
        } else if (++sNoProgress % 24 == 0) {
            sDetour = (sDetour + 1) % 8;
            sBestDist = dist; /* a detour may move away first */
        }
        if (sNoProgress > 900) {
            sNoProgress = 0;
            sDetour = 0;
            if (++sTriedSide[room & 31][sTargetSide] >= 3) {
                for (i = 0; i < 32 && GetMapEventDoor((u8)i)->kind != EVENT_DOOR_END; i++)
                    if (GetMapEventDoor((u8)i)->room == room && GetMapEventDoor((u8)i)->side == sTargetSide)
                        sEventTried[i]++;
            }
            sTargetSide = -1;
            return 0;
        }
        {
            int d = order[sDetour], kx = dirs[d][0], ky = dirs[d][1];
            uint16_t k = (uint16_t)((kx > 0 ? K_RIGHT : kx < 0 ? K_LEFT : 0) | (ky > 0 ? K_DOWN : ky < 0 ? K_UP : 0));
            if (sNoProgress > 100 && (frame % 60) < 3)
                k |= K_B; /* a step or a ledge: jump */
            return k;
        }
    }
}

static uint16_t BattleKeys(uint32_t frame) {
    BtlObj* me = gBtlWork->actor;
    BtlObj* obj;
    BtlObj* best = NULL;
    int bestD = 1 << 30;

    if (me == NULL)
        return (frame % 10) < 2 ? K_A : 0;
    {
        /* every 8 seconds, the moves the battle tutorials ask for: reload
           (hold A), jump, dodge roll (double tap), cycle cards, switch list */
        uint32_t t = frame % 480;
        if (t >= 200 && t < 260) return K_A;
        if (t >= 262 && t < 266) return K_B;
        if ((t >= 300 && t < 303) || (t >= 307 && t < 310)) return K_RIGHT;
        if (t >= 340 && t < 343) return K_R;
        if (t >= 360 && t < 363) return K_L;
        if (t >= 400 && t < 403 && (frame / 480) % 3 == 0) return K_SELECT;
    }
    for (obj = ListPoolFirst(&gBtlWork->pool); obj != NULL; obj = ListPoolNext(&obj->node)) {
        int d = abs((obj->x - me->x) >> 8) + abs((obj->y - me->y) >> 8);
        if (obj->hp > 0 && d < bestD) {
            bestD = d;
            best = obj;
        }
    }
    if (best == NULL)
        return (frame % 10) < 2 ? K_A : 0;
    {
        int dx = (best->x - me->x) >> 8, dy = (best->y - me->y) >> 8;
        uint16_t k = Toward(dx, dy, 18);
        if (abs(dx) < 48 && abs(dy) < 24)
            k = (uint16_t)((k & (K_UP | K_DOWN)) | ((frame % 8) < 3 ? K_A : 0)
                           | (dx > 0 ? K_RIGHT : K_LEFT) * ((frame % 8) == 7));
        else if ((frame / 120) % 4 == 3)
            k |= K_A; /* hold A: charges a reload when the hand is empty */
        return k;
    }
}

uint16_t PortBotKeys(uint32_t frame) {
    const char* mode;

    if (sEnabled < 0) {
        const char* e = getenv("KHCOM_BOT");
        sEnabled = e != NULL && atoi(e) != 0;
    }
    if (!sEnabled)
        return 0;
    mode = PortHarnessModeName();
    if (frame - sLastLog >= 300) {
        sLastLog = frame;
        PortLog("bot: frame %u mode %s cards %d floor %u room %u story %u level %u hp %d", (unsigned)frame, mode,
                (int)CountMapCards(),
                (unsigned)gGameState.floor, (unsigned)gMapFloorState.room, (unsigned)gMapFloorState.progress,
                (unsigned)gGameState.progression.level,
                gFieldState ? (int)(gFieldState->actor.fieldPosition.x >> 8) : -1);
    }
    if (strcmp(mode, "mode_battle") == 0 && gBtlWork != NULL) {
        sNeedCards = 0; /* the battle's prizes may hold the map card the door wants */
        return BattleKeys(frame);
    }
    if (strcmp(mode, "Mode_MapFix") == 0 && gFieldState != NULL && gMapFloorState.room >= 32) {
        sNeedCards = 0;
        sNeedFrames = 0;
        return HallKeys(frame);
    }
    if (strcmp(mode, "Mode_MapFld") == 0 && gFieldState != NULL) {
        static int lastRoom = -1;
        if (gMapFloorState.room != lastRoom) {
            lastRoom = gMapFloorState.room;
            sTargetSide = -1;
        }
        return FieldKeys(frame);
    }
    /* title, events, movies, menus, level up, game over: confirm */
    return (frame % 20) < 2 ? K_A : 0;
}
