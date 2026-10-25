/* Explicit test build only. Exercises real InitEventKeyCard/UpdateEventKeyTotal
 * and DrawLayeredCardSprite in a controlled room; no shipping behavior. */
#include <stdlib.h>
#include <string.h>
#include "port/port.h"
static EventKeyCard sSynthesisFixtureCard;
static EventKey sSynthesisFixtureKey;
static int sSynthesisFixtureActive;

static void SynthesisFixtureBegin(void) {
    const char* name = getenv("KHCOM_SYNTHESIS_FIXTURE");
    EventKey* key = &sSynthesisFixtureKey;
    sSynthesisFixtureActive = 0;
    if (!name) return;
    key->kind = MAP_CARD_NONE;
    if (!strcmp(name, "red-at-most-7")) {
        key->color = MAP_CARD_COLOR_RED; key->rule = EVENT_KEY_RULE_AT_MOST; key->value = 7;
    } else if (!strcmp(name, "green-exact-12")) {
        key->color = MAP_CARD_COLOR_GREEN; key->rule = EVENT_KEY_RULE_EXACT; key->value = 12;
    } else if (!strcmp(name, "blue-total-10to9")) {
        key->color = MAP_CARD_COLOR_BLUE; key->rule = EVENT_KEY_RULE_TOTAL; key->value = 10;
    } else return;
    InitEventKeyCard(&sSynthesisFixtureCard, key);
    if (sSynthesisFixtureCard.sprite.palette2)
        FadeSetPaletteExcluded(sSynthesisFixtureCard.sprite.palette2->index + 16, TRUE);
    if (sSynthesisFixtureCard.sprite.palette3)
        FadeSetPaletteExcluded(sSynthesisFixtureCard.sprite.palette3->index + 16, TRUE);
    sSynthesisFixtureActive = 1;
    PortLog("synthesis fixture: %s real InitEventKeyCard", name);
}

static int SynthesisFixtureDraw(void) {
    int first;
    if (!sSynthesisFixtureActive) return 0;
    if (sSynthesisFixtureKey.rule == EVENT_KEY_RULE_TOTAL &&
        PortFrameCount() >= 490 && sSynthesisFixtureCard.drawnTotal == 10) {
        /* A controlled display update; real PayEventKey is tested separately
         * by the host harness with production function bodies. */
        sSynthesisFixtureCard.total = 9;
        UpdateEventKeyTotal(&sSynthesisFixtureCard);
        PortLog("synthesis fixture: real UpdateEventKeyTotal 10->9 frame=%u", PortFrameCount());
    }
    SetLayeredCardSpritePos(120 << 8, 56 << 8, &sSynthesisFixtureCard.sprite);
    first = gSpriteWork->entryCount;
    DrawLayeredCardSprite(&sSynthesisFixtureCard.sprite, SPRITE_FLAG_MOSAIC);
    PortSynthesisHdTagCard(&sSynthesisFixtureCard, &sSynthesisFixtureKey, first, 0);
    return 1;
}

static void SynthesisFixtureEnd(void) {
    if (sSynthesisFixtureActive)
        ReleaseLayeredCardSprite(&sSynthesisFixtureCard.sprite);
    sSynthesisFixtureActive = 0;
}
