/*
 * Cheats page (Options -> Cheats): requested to help players stuck on card
 * battles. Toggles are off by default; the actions change the game in
 * progress only while walking on the map, inside the cartridge's own limits.
 */
#include <string.h>
#include "types.h"
#include "game_state.h"
#include "player_progression.h"
#include "card_api.h"
#include "card_map_anim.h"
#include "card_def_data.h"
#include "card_types.h"
#include "card_ids.h"
#include <SDL3/SDL.h>
#include "port/port.h"

extern const char* GetModeName(void);
extern u8 gMapCardCounts[270];
extern u16 gCardCollection[999];
extern u16 gCardCount;
u16 CountCardsById(u16 cardId);
void ExpandCardCollectionForNewCard(u16 cardId);

/* Enemies fall to one hit; the player loses no HP. Applied after the
   difficulty scaling, to damage only (healing passes untouched). */
int PortCheatDamage(int damage, int player) {
    int out = damage;
    if (damage > 0 && player && gPortConfig.cheatInfiniteHp)
        out = 0;
    else if (damage > 0 && !player && gPortConfig.cheatOneHit)
        out = 9999;
    const char* damageLog;
    PORT_ENV_ONCE(damageLog, SDL_getenv, "KHCOM_DAMAGELOG");
    if (out != damage && damageLog)
        PortLog("cheat: %s damage %d -> %d", player ? "player" : "enemy", damage, out);
    return out;
}

/* the actions: only walking a room (world or castle hall), never in a
   battle, menu or event */
int PortCheatActionsAvailable(void) {
    const char* mode = GetModeName();
    return strcmp(mode, "Mode_MapFld") == 0 || strcmp(mode, "Mode_MapFix") == 0;
}

/* Level 99 with every level-up bonus at once (normally one per level):
   HP +15 and CP +25 (Sora) or AP/DP (Riku) for each of the 98 levels, within
   the game's caps, and every level-up sleight. */
int PortCheatMaxLevel(void) {
    PlayerProgression* p = &gGameState.progression;
    u32 exp = 0;
    int level;

    if (!PortCheatActionsAvailable())
        return 0;
    PortAchievementsInvalidate("Max level cheat");
    PortAchievementsTaintSession();
    p->level = 1;
    p->nextExp = 0x19;
    for (level = 1; level < 99; level++) {
        exp = p->nextExp;
        AdvanceLevelExpThreshold(p);
        p->level++;
    }
    if (p->exp < exp)
        p->exp = exp;
    p->maxHp = 560; /* 80 + 98 x 15 is past the cap */
    gGameState.hp = p->maxHp;
    if (gGameState.flags & GAME_FLAG_RIKU) {
        p->ap = 10 + 20; /* one AP at each of the 20 AP levels */
        p->dp = 8 + 98 * 2;
        p->levelMilestone = 20;
    } else {
        static const u8 kSleights[] = {
            LEARNED_STOCK_SLIDING_DASH, LEARNED_STOCK_STUN_IMPACT, LEARNED_STOCK_STRIKE_RAID,
            LEARNED_STOCK_BLITZ, LEARNED_STOCK_SONIC_BLADE, LEARNED_STOCK_ZANTETSUKEN,
            LEARNED_STOCK_TORNADO, LEARNED_STOCK_ARS_ARCANUM, LEARNED_STOCK_HOLY,
            LEARNED_STOCK_RAGNAROK, LEARNED_STOCK_MEGA_FLARE,
        };
        size_t i;
        p->cp = 275 + 98 * 25;
        for (i = 0; i < sizeof(kSleights); i++)
            LearnStock(kSleights[i]);
        p->levelMilestone = 11;
    }
    return 1;
}

/* Map cards up to the cartridge's own limit: 99 regular cards in all, 9 of
   one card at most (AddMapCard). The cards already held stay; the free room
   is shared out one card per kind in turn, each round a different value (0
   opens any door, 9 nearly any; the event doors ask for at least, at most or
   exactly a value), through AddMapCard itself, so the journal records them as
   when the game awards them. Key cards are untouched. A count past 99 (left
   by this cheat before 2.0: 9 of all 220) goes back to the shared-out 99. */
int PortCheatMapCards(void) {
    static const u8 kValues[10] = { 0, 9, 5, 1, 7, 3, 8, 2, 6, 4 };
    int round, kind, added = 1;

    if (!PortCheatActionsAvailable())
        return 0;
    PortAchievementsInvalidate("Map card cheat");
    PortAchievementsTaintSession();
    if (CountRegularMapCards() > 99)
        for (kind = 0; kind < 220; kind++)
            gMapCardCounts[kind] = 0;
    for (round = 0; added && CountRegularMapCards() < 99; round++) {
        added = 0;
        for (kind = 0; kind < 22 && CountRegularMapCards() < 99; kind++) {
            int i, value = 0;
            /* this round's value, or the next one this kind still has room for */
            for (i = 0; i < 10; i++) {
                value = kValues[(round + i) % 10];
                if (gMapCardCounts[kind * 10 + value] < 9)
                    break;
            }
            if (i < 10 && AddMapCard((u16)(kind * 10 + value)))
                added = 1;
        }
    }
    return 1;
}

/* Every journal entry (Sora's Journal and Riku's report, #31), the way the
   cartridge's own debug menu fills it (SetJiminyFlag(250)). The collection
   grows from 911 to 999 slots as each card kind is first recorded, so first
   grow it for every kind not yet recorded, as ObtainCard would; the two Card
   Soldier kinds share one entry. */
int PortCheatJournal(void) {
    u8 seen[CARD_KIND_ANSEM + 1];
    int id;

    if (!PortCheatActionsAvailable())
        return 0;
    PortAchievementsInvalidate("Journal cheat");
    PortAchievementsTaintSession();
    memset(seen, 0, sizeof(seen));
    for (id = 0; id < 950; id++) {
        int kind = gCardDefs[id].kind;
        if (kind > CARD_KIND_ANSEM || seen[kind] || kind == CARD_KIND_CARD_SOLDIER_SPADE)
            continue;
        seen[kind] = 1;
        ExpandCardCollectionForNewCard(id);
    }
    SetJiminyFlag(250);
    return 1;
}

/* a card Sora can own: attack, magic, summon, item or enemy */
static int CollectableCard(int id) {
    const CardDef* def = &gCardDefs[id];
    /* Marluxia 2 is a boss-only card: kind 95 has no menu description. */
    return def->kind <= CARD_KIND_ANSEM && def->kind != CARD_KIND_MARLUXIA_2 && !(def->flags & (CARD_DEF_FLAG_FRIEND | CARD_DEF_FLAG_GIMMICK)) &&
           def->category <= CARD_CATEGORY_ENEMY;
}

/* Battle cards: the collection holds 999, up to 99 copies of one card. One
   of every card first, then copies (Keyblades first) in the free slots.
   Cards already in the collection and in decks stay where they are. */
int PortCheatBattleCards(void) {
    int pass, id, added = 1;

    if (!PortCheatActionsAvailable() || (gGameState.flags & GAME_FLAG_RIKU))
        return 0;
    PortAchievementsInvalidate("Battle card cheat");
    PortAchievementsTaintSession();
    gCardCount = 999;
    for (pass = 0; added && !IsCardCollectionFull(); pass++) {
        added = 0;
        for (id = 0; id < 950 && !IsCardCollectionFull(); id++) {
            if (!CollectableCard(id) || (pass > 0 && pass < 50 && gCardDefs[id].category != CARD_CATEGORY_ATTACK))
                continue;
            if (pass == 0 && CountCardsById(id) > 0)
                continue;
            if (ObtainCard(id) != CARD_NOT_ADDED)
                added = 1;
        }
        if (!added && pass > 0 && pass < 50) {
            pass = 49; /* Keyblades full: copies of the rest */
            added = 1;
        }
    }
    return 1;
}

unsigned PortGameFlags(void) {
    return gGameState.flags;
}

/* Debug mode (Cheats page): field enemies leave at the cartridge's 240x160
   screen instead of the wide view's edges, where they pop out of sight. */
int PortGbaMapLogic(void) {
    return gPortConfig.gbaMapLogic != 0;
}
