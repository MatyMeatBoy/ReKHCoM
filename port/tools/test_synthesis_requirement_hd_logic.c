/* Targeted host test: actual synthesis adapter and actual game structures.
 * No SDL init, ROM, game loop, controller or renderer. Compile with the
 * configure_pc game flags (32-bit); see SINTESIS_REQUISITO_HD.md. */
#include <stddef.h>
#include "card.h"
#include "card_ids.h"
#include "map_types.h"
#include "map.h"
#include "sprite.h"
#include "obj_api.h"
#include "card_def_data.h"
#include "sprites_worldselect.h"
#include "sprites_card.h"
#include "sprite_palettes.h"
#include "gba/defines.h"
#include "port/synthesis_requirement_hd.h"

static SpriteWork sWork;
SpriteWork* gSpriteWork = &sWork;
static int sOn = 1, sMissing = -1;

int PortHdSpriteFind(const void* tiles, const void* frame) {
    int id = (const unsigned char*)tiles - gSynthesisHdIdentities;
    return sOn && id != sMissing ? id + 1 : 0;
}

#include "../../src/card/synthesis_requirement_hd.inc"

static SelmapEventKeyWork sTask;
static ObjTiles sBody, sRequirement;
static const unsigned char sOriginalBody[1], sOriginalRequirement[1];
#define CHECK(x) do { if (!(x)) return __LINE__; } while (0)

/* Controlled allocation and DMA. The four production function bodies below
 * are extracted verbatim, with their SHA-256 saved next to the test binary. */
static ObjTiles sTilePool[16];
static ObjPalette sPalettePool[16];
static int sTileCount, sPaletteCount;
static const void* sLastDma;
static u16 sDummyFrame[1] = {0};
static void* sDummyFrames[1] = {sDummyFrame};
u8 gMapSelectRequirementTiles[7808];
u16 gDoorCardPalette[16];
void* gKeyValueFrames[4] = {sDummyFrame, sDummyFrame, sDummyFrame, sDummyFrame};
MapCardDef gMapCardDefs[260];
MapCardBackDef gMapCardBackDefs[5];
const CardBack gCardBacks[5] = {{0}, {0}, {0}, {0}, {0, sDummyFrame, 0, 0, (void*)sOriginalBody, 0}};
static const EventKeyList* sEventKeyList;
static EventKeyProgress* sEventKeyProgress;
static EventKey sEventKey;

static void TestZero(void* data, unsigned size) {
    unsigned i;
    for (i = 0; i < size; i++) ((unsigned char*)data)[i] = 0;
}
#define CpuFill32(value, dest, size) TestZero(dest, size)
ObjTiles* LoadObjTiles(const void* src, u16 size) {
    ObjTiles* result = &sTilePool[sTileCount++];
    TestZero(result, sizeof(*result));
    result->src = src; result->index = 4 * sTileCount; result->allocated = 1;
    return result;
}
ObjTiles* AllocSpriteFrameTiles(u16 size) { return LoadObjTiles(NULL, size); }
ObjPalette* LoadObjPalette(const void* src, u16 size) {
    ObjPalette* result = &sPalettePool[sPaletteCount++];
    result->src = src; result->index = sPaletteCount;
    return result;
}
u8 UpdateSpriteFrameTiles(ObjTiles* tiles, u16* sprite, void* src) {
    tiles->src = src; tiles->sprite = sprite; return 1;
}
u8 RequestDma3Copy(const void* src, void* dst, u16 size) { sLastDma = src; return 1; }
#include "synthesis_real_functions.inc"

static void reset(unsigned kind, unsigned color, unsigned rule, unsigned value) {
    EventKey key;
    key.kind = kind; key.color = color; key.rule = rule; key.value = value;
    SynthesisHdRemember(0, &key);
    sOn = 1; sMissing = -1;
    sBody.src = sOriginalBody; sBody.index = 17; sBody.allocated = 1;
    sRequirement.src = sOriginalRequirement; sRequirement.index = 29; sRequirement.allocated = 1;
    sTask.cards[0].sprite.tiles2 = &sBody;
    sTask.cards[0].sprite.tiles3 = &sRequirement;
    sTask.cards[0].drawnTotal = value;
    gSpriteWork->entryCount = 2;
    sWork.entries[0].tiles = &sBody;
    sWork.entries[1].tiles = &sRequirement;
    sWork.entries[0].priority = 9;
    sWork.entries[1].priority = 8;
    sWork.entries[0].flags = SPRITE_PRIORITY(2) | SPRITE_FLAG_MOSAIC;
}

int main(void) {
    int color, rule, value;
    ObjTiles* copy;
    EventKey realKey;
    EventKeyList list;
    EventKeyProgress progress = {0};
    MapCardAttributes payment = {0};
    for (color = 0; color < 5; color++) for (rule = 1; rule <= 4; rule++)
        for (value = 0; value < 100; value++) {
            reset(MAP_CARD_NONE, color, rule, value);
            SynthesisHdTag(&sTask, 0, 0);
            CHECK(((ObjTiles*)sWork.entries[0].tiles)->src == gSynthesisHdIdentities + color);
            copy = sWork.entries[1].tiles;
            CHECK(copy->src == gSynthesisHdIdentities + 5 + (rule - 1) * 100 + value);
            CHECK(copy->index == 29 && copy->allocated == 1);
            CHECK(sBody.src == sOriginalBody && sRequirement.src == sOriginalRequirement);
            CHECK(sWork.entries[0].priority == 9 && sWork.entries[1].priority == 8);
            CHECK(sWork.entries[0].flags == (SPRITE_PRIORITY(2) | SPRITE_FLAG_MOSAIC));
        }
    reset(MAP_CARD_NONE, 3, EVENT_KEY_RULE_TOTAL, 25);
    sTask.cards[0].drawnTotal = 10;
    SynthesisHdTag(&sTask, 0, 0);
    CHECK(((ObjTiles*)sWork.entries[1].tiles)->src == gSynthesisHdIdentities + 315);
    sWork.entries[0].tiles = &sBody; sWork.entries[1].tiles = &sRequirement;
    sTask.cards[0].drawnTotal = 9;
    SynthesisHdTag(&sTask, 0, 0);
    CHECK(((ObjTiles*)sWork.entries[1].tiles)->src == gSynthesisHdIdentities + 314);
    reset(MAP_CARD_NONE, 0, EVENT_KEY_RULE_EXACT, 12);
    sOn = 0; SynthesisHdTag(&sTask, 0, 0);
    CHECK(sWork.entries[0].tiles == &sBody && sWork.entries[1].tiles == &sRequirement);
    reset(MAP_CARD_NONE, 0, EVENT_KEY_RULE_EXACT, 12);
    sMissing = 217; SynthesisHdTag(&sTask, 0, 0);
    CHECK(sWork.entries[0].tiles == &sBody && sWork.entries[1].tiles == &sRequirement);
    reset(MAP_CARD_NONE, 0, EVENT_KEY_RULE_EXACT, 12);
    sMissing = 0; SynthesisHdTag(&sTask, 0, 0);
    CHECK(sWork.entries[0].tiles == &sBody && sWork.entries[1].tiles == &sRequirement);
    reset(0, 0, EVENT_KEY_RULE_EXACT, 12);
    SynthesisHdTag(&sTask, 0, 0);
    CHECK(sWork.entries[0].tiles == &sBody && sWork.entries[1].tiles == &sRequirement);
    reset(MAP_CARD_NONE, 1, EVENT_KEY_RULE_NONE, 0);
    SynthesisHdTag(&sTask, 0, 0);
    CHECK(sWork.entries[0].tiles != &sBody && sWork.entries[1].tiles == &sRequirement);
    reset(MAP_CARD_NONE, 0, EVENT_KEY_RULE_TOTAL, 100);
    SynthesisHdTag(&sTask, 0, 0);
    CHECK(sWork.entries[0].tiles == &sBody && sWork.entries[1].tiles == &sRequirement);
    /* Real initialization for each rule and color, then the actual adapter. */
    for (color = 0; color < 5; color++) for (rule = 1; rule <= 4; rule++) {
        sTileCount = 0; sPaletteCount = 0; sOn = 1; sMissing = -1;
        gMapCardBackDefs[color].tiles2 = (void*)sOriginalBody;
        gMapCardBackDefs[color].sprites2 = sDummyFrames;
        gMapCardBackDefs[color].palette = gDoorCardPalette;
        realKey.kind = MAP_CARD_NONE; realKey.color = color;
        realKey.rule = rule; realKey.value = 12;
        InitEventKeyCard(&sTask.cards[0], &realKey);
        sWork.entries[0].tiles = sTask.cards[0].sprite.tiles2;
        sWork.entries[1].tiles = sTask.cards[0].sprite.tiles3;
        PortSynthesisHdTagCard(&sTask.cards[0], &realKey, 0, 0);
        CHECK(((ObjTiles*)sWork.entries[0].tiles)->src == gSynthesisHdIdentities + color);
        CHECK(((ObjTiles*)sWork.entries[1].tiles)->src == gSynthesisHdIdentities + 5 + (rule - 1) * 100 + 12);
    }
    /* Real GetEventKey -> Init -> PayEventKey(1) -> GetEventKey -> update -> HD. */
    sTileCount = 0; sPaletteCount = 0;
    realKey.kind = MAP_CARD_NONE; realKey.color = 3;
    realKey.rule = EVENT_KEY_RULE_TOTAL; realKey.value = 10;
    list.count = 1; list.keys = &realKey;
    sEventKeyList = &list; sEventKeyProgress = &progress;
    InitEventKeyCard(&sTask.cards[0], GetEventKey(0));
    CHECK(sTask.cards[0].drawnTotal == 10);
    payment.value = 1;
    CHECK(PayEventKey(&payment) == 0);
    CHECK(progress.remaining == 9 && progress.paid == 0 && GetEventKey(0)->value == 9);
    sTask.cards[0].total = GetEventKey(0)->value;
    UpdateEventKeyTotal(&sTask.cards[0]);
    CHECK(sTask.cards[0].drawnTotal == 9);
    CHECK(sLastDma == gMapSelectRequirementTiles + 9 * 128);
    sWork.entries[0].tiles = sTask.cards[0].sprite.tiles2;
    sWork.entries[1].tiles = sTask.cards[0].sprite.tiles3;
    PortSynthesisHdTagCard(&sTask.cards[0], &realKey, 0, 0);
    CHECK(((ObjTiles*)sWork.entries[1].tiles)->src == gSynthesisHdIdentities + 314);
    payment.value = 9;
    CHECK(PayEventKey(&payment) == 1 && progress.paid == 1 && progress.remaining == 0);
    /* A specific kind uses the actual illustration branch, without a generic overlay. */
    sTileCount = 0; sPaletteCount = 0;
    gMapCardDefs[0].sprites = sDummyFrames;
    gMapCardBackDefs[0].sprites = sDummyFrames;
    realKey.kind = 0;
    InitEventKeyCard(&sTask.cards[0], &realKey);
    CHECK(sTask.cards[0].sprite.gfx == sDummyFrame && sTask.cards[0].sprite.tiles3 == NULL);
    sWork.entries[0].tiles = sTask.cards[0].sprite.tiles2;
    copy = sWork.entries[0].tiles;
    PortSynthesisHdTagCard(&sTask.cards[0], &realKey, 0, 0);
    CHECK(sWork.entries[0].tiles == copy);
    return 0;
}
