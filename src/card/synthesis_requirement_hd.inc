/* PC-only synthesis metadata. The real OAM frame, VRAM and palette stay intact.
 * Only these task-local entry copies receive an HD lookup identity. */
#include "sprite.h"
#include "port/port_hdsprites.h"
#include "port/synthesis_requirement_hd.h"

const unsigned char gSynthesisHdIdentities[SYNTHESIS_HD_COUNT] = {0};
static EventKey sSynthesisHdKeys[4];
static ObjTiles sSynthesisHdEntryTiles[4][2];
static ObjTiles sSynthesisHdRoomTiles[2];

static void SynthesisHdRemember(int index, const EventKey* key) {
    if ((unsigned)index < 4)
        sSynthesisHdKeys[index] = *key;
}

static void SynthesisHdTagEntries(ObjTiles* bodyTiles, ObjTiles* requirementTiles,
                                 unsigned color, unsigned rule, unsigned value,
                                 int first, ObjTiles copies[2]) {
    const unsigned char *body, *requirement = NULL;
    int j;

    if (color > MAP_CARD_COLOR_GOLD || rule > EVENT_KEY_RULE_TOTAL || bodyTiles == NULL)
        return;
    body = &gSynthesisHdIdentities[color];
    if (!PortHdSpriteFind(body, NULL))
        return; /* includes HD off and assets not staged */
    if (rule != EVENT_KEY_RULE_NONE) {
        if (value >= SYNTHESIS_HD_VALUES || requirementTiles == NULL)
            return;
        requirement = &gSynthesisHdIdentities[SYNTHESIS_HD_BODY_COUNT +
            (rule - EVENT_KEY_RULE_AT_LEAST) * SYNTHESIS_HD_VALUES + value];
        if (!PortHdSpriteFind(requirement, NULL))
            return;
    }
    for (j = first; j < gSpriteWork->entryCount; j++) {
        SpriteEntry* entry = &gSpriteWork->entries[j];
        int part = entry->tiles == bodyTiles ? 0 :
            (entry->tiles == requirementTiles && requirement != NULL ? 1 : -1);
        ObjTiles* copy;
        if (part < 0)
            continue;
        copy = &copies[part];
        /* DrawSprite has already submitted the original DMA. EmitOam reads
         * index/allocated from this copy and src only for PortOamSetHdPart.
         * These copies live through sorting/emission; never free or load them. */
        *copy = *(ObjTiles*)entry->tiles;
        copy->src = part == 0 ? body : requirement;
        entry->tiles = copy;
    }
}

static void SynthesisHdTag(SelmapEventKeyWork* work, int index, int first) {
    const EventKey* key;
    EventKeyCard* card;
    if ((unsigned)index >= 4)
        return;
    key = &sSynthesisHdKeys[index];
    card = &work->cards[index];
    PortSynthesisHdTagCard(card, key, first, index);
}

void PortSynthesisHdTagCard(EventKeyCard* card, const EventKey* key, int first, int slot) {
    if ((unsigned)slot >= 4 || key->kind != MAP_CARD_NONE)
        return;
    SynthesisHdTagEntries(card->sprite.tiles2, card->sprite.tiles3, key->color, key->rule,
        key->rule == EVENT_KEY_RULE_TOTAL ? card->drawnTotal : key->value,
        first, sSynthesisHdEntryTiles[slot]);
}

void PortSynthesisHdTagRoom(ObjTiles* body, ObjTiles* requirement, unsigned value, int first) {
    /* The normal door's zero is drawn without an operator by the cartridge. */
    SynthesisHdTagEntries(body, requirement, MAP_CARD_COLOR_NONE,
        value == 0 ? EVENT_KEY_RULE_TOTAL : EVENT_KEY_RULE_AT_LEAST,
        value, first, sSynthesisHdRoomTiles);
}
