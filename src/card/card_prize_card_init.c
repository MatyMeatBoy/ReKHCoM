/**
 * card_prize_card_init.c
 * Map Card Prizes and Key Display
 */

#include "registration_data.h"
#include "system_state.h"
#include "map_api.h"
#include "msg_api.h"
#include "mode_debug.h"
#include "m4a_song.h"
#include "game_state.h"
#include "text.h"
#include "fade.h"
#include "btl_collision.h"
#include "obj_api.h"
#include "battle_actor.h"
#include "display.h"
#include "engine_math.h"
#include "anim.h"
#include "obj.h"
#include "taskpool.h"
#include "card.h"
#include "sprites_btl.h"
#include "prize_card.h"
#include "sprites_worldselect.h"
#include "sprites_card_pictures.h"
#include "sprites_card.h"
#include "gba/io_reg.h"
#include "card_ids.h"
#ifdef PLATFORM_PC
#include "engine.h"
#endif
#include "songs.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "card_api.h"
#include "card_def_data.h"
#include "card_types.h"
#include "card_ui_types.h"
#include "gba/defines.h"
#include "map_types.h"
#include "player_progression_types.h"
#include "types.h"
#include "gba/macro.h"
#include <stddef.h>
#include "lockon.h"
#include "card_prize_card_init.h"
#ifdef PLATFORM_PC
#include "synthesis_requirement_hd.inc"
#endif
#include "battle.h"
#include "sprite_palettes.h"
#include "battle_ids.h"
#include "card_label_data.h"
#include "macros.h"

static const u16 sPrizeMapCardValueChances[10] = { 10, 5, 5, 15, 15, 15, 15, 10, 5, 5 };

static const PrizeMapCardEntry sSoraPrizeTraverseTownTier0[1] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_MEETING_GROUND, 0), 0 },
};

static const PrizeMapCardEntry sSoraPrizeTraverseTownTier1[4] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_TEEMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_TRANQUIL_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_SLEEPING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_FEEBLE_DARKNESS, 0), 0 },
};

static const PrizeMapCardGroup sSoraPrizeTraverseTownTiers[2] = {
    { sSoraPrizeTraverseTownTier0, ARRAY_COUNT(sSoraPrizeTraverseTownTier0), 30 },
    { sSoraPrizeTraverseTownTier1, ARRAY_COUNT(sSoraPrizeTraverseTownTier1), 100 },
};

static const PrizeMapCardEntry sSoraPrizeAgrabahTier0[2] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_SORCEROUS_WAKING, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_CALM_BOUNTY, 0), 0 },
};

static const PrizeMapCardEntry sSoraPrizeAgrabahTier1[7] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_TEEMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_TRANQUIL_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_SLEEPING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOMENTS_REPRIEVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_FEEBLE_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOOGLE_ROOM, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STAGNANT_SPACE, 0), 0 },
};

static const PrizeMapCardEntry sSoraPrizeAgrabahTier2[8] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_MARTIAL_WAKING, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALCHEMIC_WAKING, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MEETING_GROUND, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STRONG_INITIATIVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LASTING_DAZE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_PREMIUM_ROOM, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_WHITE_ROOM, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_BLACK_ROOM, 0), 0 },
};

static const PrizeMapCardGroup sSoraPrizeAgrabahTiers[3] = {
    { sSoraPrizeAgrabahTier0, ARRAY_COUNT(sSoraPrizeAgrabahTier0), 40 },
    { sSoraPrizeAgrabahTier1, ARRAY_COUNT(sSoraPrizeAgrabahTier1), 85 },
    { sSoraPrizeAgrabahTier2, ARRAY_COUNT(sSoraPrizeAgrabahTier2), 100 },
};

static const PrizeMapCardEntry sSoraPrizeHalloweenTownTier0[2] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_LASTING_DAZE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_CALM_BOUNTY, 0), 0 },
};

static const PrizeMapCardEntry sSoraPrizeHalloweenTownTier1[7] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_TEEMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_TRANQUIL_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_SLEEPING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOMENTS_REPRIEVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_FEEBLE_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOOGLE_ROOM, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STAGNANT_SPACE, 0), 0 },
};

static const PrizeMapCardEntry sSoraPrizeHalloweenTownTier2[8] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_SORCEROUS_WAKING, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MARTIAL_WAKING, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALCHEMIC_WAKING, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MEETING_GROUND, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STRONG_INITIATIVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_PREMIUM_ROOM, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_WHITE_ROOM, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_BLACK_ROOM, 0), 0 },
};

static const PrizeMapCardGroup sSoraPrizeHalloweenTownTiers[3] = {
    { sSoraPrizeHalloweenTownTier0, ARRAY_COUNT(sSoraPrizeHalloweenTownTier0), 40 },
    { sSoraPrizeHalloweenTownTier1, ARRAY_COUNT(sSoraPrizeHalloweenTownTier1), 85 },
    { sSoraPrizeHalloweenTownTier2, ARRAY_COUNT(sSoraPrizeHalloweenTownTier2), 100 },
};

static const PrizeMapCardEntry sSoraPrizeMonstroTier0[2] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_CALM_BOUNTY, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALCHEMIC_WAKING, 0), 0 },
};

static const PrizeMapCardEntry sSoraPrizeMonstroTier1[7] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_TEEMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_TRANQUIL_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_SLEEPING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOMENTS_REPRIEVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_FEEBLE_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOOGLE_ROOM, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STAGNANT_SPACE, 0), 0 },
};

static const PrizeMapCardEntry sSoraPrizeMonstroTier2[7] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_SORCEROUS_WAKING, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MARTIAL_WAKING, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MEETING_GROUND, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STRONG_INITIATIVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_PREMIUM_ROOM, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_WHITE_ROOM, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_BLACK_ROOM, 0), 0 },
};

static const PrizeMapCardGroup sSoraPrizeMonstroTiers[3] = {
    { sSoraPrizeMonstroTier0, ARRAY_COUNT(sSoraPrizeMonstroTier0), 40 },
    { sSoraPrizeMonstroTier1, ARRAY_COUNT(sSoraPrizeMonstroTier1), 85 },
    { sSoraPrizeMonstroTier2, ARRAY_COUNT(sSoraPrizeMonstroTier2), 100 },
};

static const PrizeMapCardEntry sSoraPrizeOlympusColiseumTier0[2] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_CALM_BOUNTY, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MARTIAL_WAKING, 0), 0 },
};

static const PrizeMapCardEntry sSoraPrizeOlympusColiseumTier1[7] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_TEEMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_TRANQUIL_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_SLEEPING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOMENTS_REPRIEVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_FEEBLE_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOOGLE_ROOM, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STAGNANT_SPACE, 0), 0 },
};

static const PrizeMapCardEntry sSoraPrizeOlympusColiseumTier2[8] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_SORCEROUS_WAKING, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALCHEMIC_WAKING, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MEETING_GROUND, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STRONG_INITIATIVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LASTING_DAZE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_PREMIUM_ROOM, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_WHITE_ROOM, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_BLACK_ROOM, 0), 0 },
};

static const PrizeMapCardGroup sSoraPrizeOlympusColiseumTiers[3] = {
    { sSoraPrizeOlympusColiseumTier0, ARRAY_COUNT(sSoraPrizeOlympusColiseumTier0), 40 },
    { sSoraPrizeOlympusColiseumTier1, ARRAY_COUNT(sSoraPrizeOlympusColiseumTier1), 85 },
    { sSoraPrizeOlympusColiseumTier2, ARRAY_COUNT(sSoraPrizeOlympusColiseumTier2), 100 },
};

static const PrizeMapCardEntry sSoraPrizeWonderlandTier0[2] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_CALM_BOUNTY, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STRONG_INITIATIVE, 0), 0 },
};

static const PrizeMapCardEntry sSoraPrizeWonderlandTier1[7] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_TEEMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_TRANQUIL_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_SLEEPING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOMENTS_REPRIEVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_FEEBLE_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOOGLE_ROOM, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STAGNANT_SPACE, 0), 0 },
};

static const PrizeMapCardEntry sSoraPrizeWonderlandTier2[8] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_SORCEROUS_WAKING, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MARTIAL_WAKING, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALCHEMIC_WAKING, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MEETING_GROUND, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LASTING_DAZE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_PREMIUM_ROOM, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_WHITE_ROOM, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_BLACK_ROOM, 0), 0 },
};

static const PrizeMapCardGroup sSoraPrizeWonderlandTiers[3] = {
    { sSoraPrizeWonderlandTier0, ARRAY_COUNT(sSoraPrizeWonderlandTier0), 40 },
    { sSoraPrizeWonderlandTier1, ARRAY_COUNT(sSoraPrizeWonderlandTier1), 85 },
    { sSoraPrizeWonderlandTier2, ARRAY_COUNT(sSoraPrizeWonderlandTier2), 100 },
};

static const PrizeMapCardEntry sSoraPrizeAtlanticaTier0[1] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_WHITE_ROOM, 0), 0 },
};

static const PrizeMapCardEntry sSoraPrizeAtlanticaTier1[5] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_GUARDED_TROVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LOOMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALMIGHTY_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_FALSE_BOUNTY, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOOGLE_ROOM, 0), 0 },
};

static const PrizeMapCardEntry sSoraPrizeAtlanticaTier2[9] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_MOMENTS_REPRIEVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_SORCEROUS_WAKING, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MARTIAL_WAKING, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALCHEMIC_WAKING, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MEETING_GROUND, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STRONG_INITIATIVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LASTING_DAZE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_PREMIUM_ROOM, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_BLACK_ROOM, 0), 0 },
};

static const PrizeMapCardGroup sSoraPrizeAtlanticaTiers[3] = {
    { sSoraPrizeAtlanticaTier0, ARRAY_COUNT(sSoraPrizeAtlanticaTier0), 20 },
    { sSoraPrizeAtlanticaTier1, ARRAY_COUNT(sSoraPrizeAtlanticaTier1), 80 },
    { sSoraPrizeAtlanticaTier2, ARRAY_COUNT(sSoraPrizeAtlanticaTier2), 100 },
};

static const PrizeMapCardEntry sSoraPrizeNeverLandTier0[1] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_PREMIUM_ROOM, 0), 0 },
};

static const PrizeMapCardEntry sSoraPrizeNeverLandTier1[5] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_GUARDED_TROVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LOOMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALMIGHTY_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_FALSE_BOUNTY, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOOGLE_ROOM, 0), 0 },
};

static const PrizeMapCardEntry sSoraPrizeNeverLandTier2[9] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_MOMENTS_REPRIEVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_SORCEROUS_WAKING, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MARTIAL_WAKING, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALCHEMIC_WAKING, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MEETING_GROUND, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STRONG_INITIATIVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LASTING_DAZE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_WHITE_ROOM, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_BLACK_ROOM, 0), 0 },
};

static const PrizeMapCardGroup sSoraPrizeNeverLandTiers[3] = {
    { sSoraPrizeNeverLandTier0, ARRAY_COUNT(sSoraPrizeNeverLandTier0), 20 },
    { sSoraPrizeNeverLandTier1, ARRAY_COUNT(sSoraPrizeNeverLandTier1), 80 },
    { sSoraPrizeNeverLandTier2, ARRAY_COUNT(sSoraPrizeNeverLandTier2), 100 },
};

static const PrizeMapCardEntry sSoraPrizeHollowBastionTier0[1] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_BLACK_ROOM, 0), 0 },
};

static const PrizeMapCardEntry sSoraPrizeHollowBastionTier1[5] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_GUARDED_TROVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LOOMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALMIGHTY_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_FALSE_BOUNTY, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOOGLE_ROOM, 0), 0 },
};

static const PrizeMapCardEntry sSoraPrizeHollowBastionTier2[9] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_MOMENTS_REPRIEVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_SORCEROUS_WAKING, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MARTIAL_WAKING, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALCHEMIC_WAKING, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MEETING_GROUND, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STRONG_INITIATIVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LASTING_DAZE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_PREMIUM_ROOM, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_WHITE_ROOM, 0), 0 },
};

static const PrizeMapCardGroup sSoraPrizeHollowBastionTiers[3] = {
    { sSoraPrizeHollowBastionTier0, ARRAY_COUNT(sSoraPrizeHollowBastionTier0), 20 },
    { sSoraPrizeHollowBastionTier1, ARRAY_COUNT(sSoraPrizeHollowBastionTier1), 80 },
    { sSoraPrizeHollowBastionTier2, ARRAY_COUNT(sSoraPrizeHollowBastionTier2), 100 },
};

static const PrizeMapCardEntry sSoraPrizeTwilightTownTier0[1] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_MINGLING_WORLDS, 0), 0 },
};

static const PrizeMapCardEntry sSoraPrizeTwilightTownTier1[6] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_TEEMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_TRANQUIL_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_SLEEPING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_FEEBLE_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_CALM_BOUNTY, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STAGNANT_SPACE, 0), 0 },
};

static const PrizeMapCardEntry sSoraPrizeTwilightTownTier2[8] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_MOMENTS_REPRIEVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOOGLE_ROOM, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_SORCEROUS_WAKING, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MARTIAL_WAKING, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALCHEMIC_WAKING, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MEETING_GROUND, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STRONG_INITIATIVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LASTING_DAZE, 0), 0 },
};

static const PrizeMapCardGroup sSoraPrizeTwilightTownTiers[3] = {
    { sSoraPrizeTwilightTownTier0, ARRAY_COUNT(sSoraPrizeTwilightTownTier0), 20 },
    { sSoraPrizeTwilightTownTier1, ARRAY_COUNT(sSoraPrizeTwilightTownTier1), 80 },
    { sSoraPrizeTwilightTownTier2, ARRAY_COUNT(sSoraPrizeTwilightTownTier2), 100 },
};

static const PrizeMapCardEntry sSoraPrizeDestinyIslandsTier0[1] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_MINGLING_WORLDS, 0), 0 },
};

static const PrizeMapCardEntry sSoraPrizeDestinyIslandsTier1[4] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_GUARDED_TROVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LOOMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALMIGHTY_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_FALSE_BOUNTY, 0), 0 },
};

static const PrizeMapCardEntry sSoraPrizeDestinyIslandsTier2[5] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_MOMENTS_REPRIEVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOOGLE_ROOM, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_PREMIUM_ROOM, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_WHITE_ROOM, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_BLACK_ROOM, 0), 0 },
};

static const PrizeMapCardGroup sSoraPrizeDestinyIslandsTiers[3] = {
    { sSoraPrizeDestinyIslandsTier0, ARRAY_COUNT(sSoraPrizeDestinyIslandsTier0), 30 },
    { sSoraPrizeDestinyIslandsTier1, ARRAY_COUNT(sSoraPrizeDestinyIslandsTier1), 80 },
    { sSoraPrizeDestinyIslandsTier2, ARRAY_COUNT(sSoraPrizeDestinyIslandsTier2), 100 },
};

static const PrizeMapCardEntry sSoraPrizeCastleOblivionTier0[9] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_TEEMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_TRANQUIL_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_GUARDED_TROVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LOOMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_SLEEPING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_FEEBLE_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALMIGHTY_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_CALM_BOUNTY, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_FALSE_BOUNTY, 0), 0 },
};

static const PrizeMapCardEntry sSoraPrizeCastleOblivionTier1[3] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_MOMENTS_REPRIEVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOOGLE_ROOM, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MINGLING_WORLDS, 0), 0 },
};

static const PrizeMapCardGroup sSoraPrizeCastleOblivionTiers[2] = {
    { sSoraPrizeCastleOblivionTier0, ARRAY_COUNT(sSoraPrizeCastleOblivionTier0), 85 },
    { sSoraPrizeCastleOblivionTier1, ARRAY_COUNT(sSoraPrizeCastleOblivionTier1), 100 },
};

static const PrizeMapCardGroupList sSoraPrizeMapCardGroups[14] = {
    { sSoraPrizeTraverseTownTiers, ARRAY_COUNT(sSoraPrizeTraverseTownTiers) },
    { sSoraPrizeAgrabahTiers, ARRAY_COUNT(sSoraPrizeAgrabahTiers) },
    { sSoraPrizeAtlanticaTiers, ARRAY_COUNT(sSoraPrizeAtlanticaTiers) },
    { sSoraPrizeOlympusColiseumTiers, ARRAY_COUNT(sSoraPrizeOlympusColiseumTiers) },
    { sSoraPrizeWonderlandTiers, ARRAY_COUNT(sSoraPrizeWonderlandTiers) },
    { sSoraPrizeMonstroTiers, ARRAY_COUNT(sSoraPrizeMonstroTiers) },
    { sSoraPrizeHalloweenTownTiers, ARRAY_COUNT(sSoraPrizeHalloweenTownTiers) },
    { sSoraPrizeNeverLandTiers, ARRAY_COUNT(sSoraPrizeNeverLandTiers) },
    { sSoraPrizeHollowBastionTiers, ARRAY_COUNT(sSoraPrizeHollowBastionTiers) },
    { sSoraPrizeDestinyIslandsTiers, ARRAY_COUNT(sSoraPrizeDestinyIslandsTiers) },
    { sSoraPrizeTraverseTownTiers, ARRAY_COUNT(sSoraPrizeTraverseTownTiers) },
    { sSoraPrizeTwilightTownTiers, ARRAY_COUNT(sSoraPrizeTwilightTownTiers) },
    { sSoraPrizeCastleOblivionTiers, ARRAY_COUNT(sSoraPrizeCastleOblivionTiers) },
    { sSoraPrizeCastleOblivionTiers, ARRAY_COUNT(sSoraPrizeCastleOblivionTiers) },
};

const u16 gUnk_09035E3C[10] = { 10, 5, 5, 15, 15, 15, 15, 10, 5, 5 };

static const PrizeMapCardEntry sRikuPrizeTraverseTownCards[11] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_TEEMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_TRANQUIL_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LOOMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_SLEEPING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOMENTS_REPRIEVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_FEEBLE_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALMIGHTY_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MINGLING_WORLDS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STRONG_INITIATIVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LASTING_DAZE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STAGNANT_SPACE, 0), 0 },
};

static const PrizeMapCardGroup sRikuPrizeTraverseTownTiers[1] = {
    { sRikuPrizeTraverseTownCards, ARRAY_COUNT(sRikuPrizeTraverseTownCards), 100 },
};

static const PrizeMapCardEntry sRikuPrizeAgrabahCards[11] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_TEEMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_TRANQUIL_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LOOMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_SLEEPING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOMENTS_REPRIEVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_FEEBLE_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALMIGHTY_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MINGLING_WORLDS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STRONG_INITIATIVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LASTING_DAZE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STAGNANT_SPACE, 0), 0 },
};

static const PrizeMapCardGroup sRikuPrizeAgrabahTiers[1] = {
    { sRikuPrizeAgrabahCards, ARRAY_COUNT(sRikuPrizeAgrabahCards), 100 },
};

static const PrizeMapCardEntry sRikuPrizeHalloweenTownCards[11] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_TEEMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_TRANQUIL_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LOOMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_SLEEPING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOMENTS_REPRIEVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_FEEBLE_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALMIGHTY_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MINGLING_WORLDS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STRONG_INITIATIVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LASTING_DAZE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STAGNANT_SPACE, 0), 0 },
};

static const PrizeMapCardGroup sRikuPrizeHalloweenTownTiers[1] = {
    { sRikuPrizeHalloweenTownCards, ARRAY_COUNT(sRikuPrizeHalloweenTownCards), 100 },
};

static const PrizeMapCardEntry sRikuPrizeMonstroCards[11] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_TEEMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_TRANQUIL_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LOOMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_SLEEPING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOMENTS_REPRIEVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_FEEBLE_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALMIGHTY_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MINGLING_WORLDS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STRONG_INITIATIVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LASTING_DAZE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STAGNANT_SPACE, 0), 0 },
};

static const PrizeMapCardGroup sRikuPrizeMonstroTiers[1] = {
    { sRikuPrizeMonstroCards, ARRAY_COUNT(sRikuPrizeMonstroCards), 100 },
};

static const PrizeMapCardEntry sRikuPrizeOlympusColiseumCards[11] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_TEEMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_TRANQUIL_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LOOMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_SLEEPING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOMENTS_REPRIEVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_FEEBLE_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALMIGHTY_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MINGLING_WORLDS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STRONG_INITIATIVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LASTING_DAZE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STAGNANT_SPACE, 0), 0 },
};

static const PrizeMapCardGroup sRikuPrizeOlympusColiseumTiers[1] = {
    { sRikuPrizeOlympusColiseumCards, ARRAY_COUNT(sRikuPrizeOlympusColiseumCards), 100 },
};

static const PrizeMapCardEntry sRikuPrizeWonderlandCards[11] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_TEEMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_TRANQUIL_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LOOMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_SLEEPING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOMENTS_REPRIEVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_FEEBLE_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALMIGHTY_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MINGLING_WORLDS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STRONG_INITIATIVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LASTING_DAZE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STAGNANT_SPACE, 0), 0 },
};

static const PrizeMapCardGroup sRikuPrizeWonderlandTiers[1] = {
    { sRikuPrizeWonderlandCards, ARRAY_COUNT(sRikuPrizeWonderlandCards), 100 },
};

static const PrizeMapCardEntry sRikuPrizeAtlanticaCards[11] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_TEEMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_TRANQUIL_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LOOMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_SLEEPING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOMENTS_REPRIEVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_FEEBLE_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALMIGHTY_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MINGLING_WORLDS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STRONG_INITIATIVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LASTING_DAZE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STAGNANT_SPACE, 0), 0 },
};

static const PrizeMapCardGroup sRikuPrizeAtlanticaTiers[1] = {
    { sRikuPrizeAtlanticaCards, ARRAY_COUNT(sRikuPrizeAtlanticaCards), 100 },
};

static const PrizeMapCardEntry sRikuPrizeNeverLandCards[11] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_TEEMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_TRANQUIL_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LOOMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_SLEEPING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOMENTS_REPRIEVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_FEEBLE_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALMIGHTY_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MINGLING_WORLDS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STRONG_INITIATIVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LASTING_DAZE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STAGNANT_SPACE, 0), 0 },
};

static const PrizeMapCardGroup sRikuPrizeNeverLandTiers[1] = {
    { sRikuPrizeNeverLandCards, ARRAY_COUNT(sRikuPrizeNeverLandCards), 100 },
};

#ifdef VERSION_EU
static const PrizeMapCardEntry sRikuPrizeHollowBastionCards[9] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_TEEMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_TRANQUIL_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LOOMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_SLEEPING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_FEEBLE_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALMIGHTY_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STRONG_INITIATIVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LASTING_DAZE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STAGNANT_SPACE, 0), 0 },
};
#else
static const PrizeMapCardEntry sRikuPrizeHollowBastionCards[10] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_TEEMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_TRANQUIL_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LOOMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_SLEEPING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_FEEBLE_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALMIGHTY_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MINGLING_WORLDS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STRONG_INITIATIVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LASTING_DAZE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STAGNANT_SPACE, 0), 0 },
};
#endif

static const PrizeMapCardGroup sRikuPrizeHollowBastionTiers[1] = {
    { sRikuPrizeHollowBastionCards, ARRAY_COUNT(sRikuPrizeHollowBastionCards), 100 },
};

static const PrizeMapCardEntry sRikuPrizeTwilightTownCards[11] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_TEEMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_TRANQUIL_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LOOMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_SLEEPING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOMENTS_REPRIEVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_FEEBLE_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALMIGHTY_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MINGLING_WORLDS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STRONG_INITIATIVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LASTING_DAZE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STAGNANT_SPACE, 0), 0 },
};

static const PrizeMapCardGroup sRikuPrizeTwilightTownTiers[1] = {
    { sRikuPrizeTwilightTownCards, ARRAY_COUNT(sRikuPrizeTwilightTownCards), 100 },
};

static const PrizeMapCardEntry sRikuPrizeDestinyIslandsCards[11] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_TEEMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_TRANQUIL_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LOOMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_SLEEPING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOMENTS_REPRIEVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_FEEBLE_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALMIGHTY_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MINGLING_WORLDS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STRONG_INITIATIVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LASTING_DAZE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STAGNANT_SPACE, 0), 0 },
};

static const PrizeMapCardGroup sRikuPrizeDestinyIslandsTiers[1] = {
    { sRikuPrizeDestinyIslandsCards, ARRAY_COUNT(sRikuPrizeDestinyIslandsCards), 100 },
};

static const PrizeMapCardEntry sRikuPrizeCastleOblivionCards[11] = {
    { MAP_CARD_ID(MAP_CARD_GROUP_TEEMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_TRANQUIL_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LOOMING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_SLEEPING_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MOMENTS_REPRIEVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_FEEBLE_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_ALMIGHTY_DARKNESS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_MINGLING_WORLDS, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STRONG_INITIATIVE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_LASTING_DAZE, 0), 0 },
    { MAP_CARD_ID(MAP_CARD_GROUP_STAGNANT_SPACE, 0), 0 },
};

static const PrizeMapCardGroup sRikuPrizeCastleOblivionTiers[1] = {
    { sRikuPrizeCastleOblivionCards, ARRAY_COUNT(sRikuPrizeCastleOblivionCards), 100 },
};

static const PrizeMapCardGroupList sRikuPrizeMapCardGroups[14] = {
    { sRikuPrizeTraverseTownTiers, ARRAY_COUNT(sRikuPrizeTraverseTownTiers) },
    { sRikuPrizeAgrabahTiers, ARRAY_COUNT(sRikuPrizeAgrabahTiers) },
    { sRikuPrizeAtlanticaTiers, ARRAY_COUNT(sRikuPrizeAtlanticaTiers) },
    { sRikuPrizeOlympusColiseumTiers, ARRAY_COUNT(sRikuPrizeOlympusColiseumTiers) },
    { sRikuPrizeWonderlandTiers, ARRAY_COUNT(sRikuPrizeWonderlandTiers) },
    { sRikuPrizeMonstroTiers, ARRAY_COUNT(sRikuPrizeMonstroTiers) },
    { sRikuPrizeHalloweenTownTiers, ARRAY_COUNT(sRikuPrizeHalloweenTownTiers) },
    { sRikuPrizeNeverLandTiers, ARRAY_COUNT(sRikuPrizeNeverLandTiers) },
    { sRikuPrizeHollowBastionTiers, ARRAY_COUNT(sRikuPrizeHollowBastionTiers) },
    { sRikuPrizeDestinyIslandsTiers, ARRAY_COUNT(sRikuPrizeDestinyIslandsTiers) },
    { sRikuPrizeTraverseTownTiers, ARRAY_COUNT(sRikuPrizeTraverseTownTiers) },
    { sRikuPrizeTwilightTownTiers, ARRAY_COUNT(sRikuPrizeTwilightTownTiers) },
    { sRikuPrizeCastleOblivionTiers, ARRAY_COUNT(sRikuPrizeCastleOblivionTiers) },
    { sRikuPrizeCastleOblivionTiers, ARRAY_COUNT(sRikuPrizeCastleOblivionTiers) },
};

static const u16 sKeyToRewardsChances[16] = { 0, 0, 8, 0, 0, 0, 0, 8, 8, 12, 0, 12, 16, 16, 16, 0 };

void PrizeCardInitInit(PrizeCardInitWork* work, PrizeCardArgs* args) {
    work->spawned = FALSE;
    work->args = *args;
    TaskPoolInit(&work->tasks, 1);
}

s32 PrizeCardInit_1(PrizeCardInitWork* work) {
    s32 args[9];
    s32 battleId;

    if (!work->spawned) {
        if ((gGameState.progression.tutorialFlags & TUTORIAL_FLAG_MAP) == 0) {
            *(PrizeCardArgs*)args = work->args;
            args[8] = MAP_CARD_ID(MAP_CARD_GROUP_TRANQUIL_DARKNESS, 2);
            CreatePrizeMapCardTask(&work->tasks, args);
            gGameState.progression.tutorialFlags |= TUTORIAL_FLAG_MAP;
        } else if (gGameState.floor == 0) {
            if (CountZeroValueMapCards() == 0) {
                *(PrizeCardArgs*)args = work->args;
                args[8] = PickPrizeMapCardKindForWorld(gGameState.world, 1);

                if (args[8] != MAP_CARD_ID_NONE) {
                    CreatePrizeMapCardTask(&work->tasks, args);
                } else {
                    return 0;
                }
            } else {
                *(PrizeCardArgs*)args = work->args;
                args[8] = PickPrizeMapCardForWorld(gGameState.world, 1);

                if (args[8] != MAP_CARD_ID_NONE) {
                    CreatePrizeMapCardTask(&work->tasks, args);
                } else {
                    return 0;
                }
            }
        } else {
            battleId = gBtlWork->battleId;

            if (battleId >= BATTLE_BARREL_0 && battleId <= BATTLE_BARREL_2) {
                if ((gGameState.flags & GAME_FLAG_RIKU) == 0) {
                    if (GetRandom() % 100 < 20) {
                        *(PrizeCardArgs*)args = work->args;

#ifdef VERSION_EU
                        if (CountRegularMapCards() <= 98) {
                            args[8] = MAP_CARD_ID(MAP_CARD_GROUP_MOOGLE_ROOM, GetRandom() % 10);
                        } else {
                            args[8] = MAP_CARD_ID_NONE;
                        }
#else
                        args[8] = MAP_CARD_ID(MAP_CARD_GROUP_MOOGLE_ROOM, GetRandom() % 10);
#endif
                    } else {
                        *(PrizeCardArgs*)args = work->args;
                        args[8] = PickPrizeMapCardForWorld(gGameState.world, 1);
                    }

                    if (args[8] != MAP_CARD_ID_NONE) {
                        CreatePrizeMapCardTask(&work->tasks, args);
                    } else {
                        return 0;
                    }
                } else {
                    *(PrizeCardArgs*)args = work->args;
                    args[8] = PickPrizeMapCardForWorld(gGameState.world, 1);

                    if (args[8] != MAP_CARD_ID_NONE) {
                        CreatePrizeMapCardTask(&work->tasks, args);
                    } else {
                        return 0;
                    }
                }
            } else if (battleId >= BATTLE_BLACK_FUNGUS_0 && battleId <= BATTLE_BLACK_FUNGUS_2) {
#ifdef VERSION_EU
                if (CountRegularMapCards() <= 98) {
                    *(PrizeCardArgs*)args = work->args;
                    args[8] = MAP_CARD_ID(MAP_CARD_GROUP_CALM_BOUNTY, GetRandom() % 10);
                    CreatePrizeMapCardTask(&work->tasks, args);
                }
#else
                *(PrizeCardArgs*)args = work->args;
                args[8] = MAP_CARD_ID(MAP_CARD_GROUP_CALM_BOUNTY, GetRandom() % 10);
                CreatePrizeMapCardTask(&work->tasks, args);
#endif
            } else {
                *(PrizeCardArgs*)args = work->args;

                if ((gGameState.flags & GAME_FLAG_RIKU) == 0) {
                    if (!HasMapCard(MAP_CARD_ID(MAP_CARD_GROUP_KEY_TO_REWARDS, 1))) {
                        if (!AreWorldPrizesCollected()) {
                            if (sKeyToRewardsChances[gGameState.world] != 0) {
                                if (GetRandom() % 100 <= sKeyToRewardsChances[gGameState.world]) {
                                    args[8] = MAP_CARD_ID(MAP_CARD_GROUP_KEY_TO_REWARDS, 1);
                                } else {
                                    args[8] = PickPrizeMapCardForWorld(gGameState.world, 1);
                                }
                            } else {
                                args[8] = PickPrizeMapCardForWorld(gGameState.world, 1);
                            }
                        } else {
                            args[8] = PickPrizeMapCardForWorld(gGameState.world, 1);
                        }
                    } else {
                        args[8] = PickPrizeMapCardForWorld(gGameState.world, 1);
                    }
                } else {
                    args[8] = PickPrizeMapCardForWorld(gGameState.world, 1);
                }

                if (args[8] != MAP_CARD_ID_NONE) {
                    CreatePrizeMapCardTask(&work->tasks, args);
                } else {
                    return 0;
                }
            }
        }

        work->spawned = TRUE;
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

s32 PrizeCardInit_Boss_1(PrizeCardInitWork* work, void* task) {
    PrizeCardTaskArgs args;

    if (!work->spawned) {
        *(PrizeCardArgs*)&args = work->args;

        switch (gBtlWork->battleId) {
        case BATTLE_GUARD_ARMOR:
            args.cardId = CARD_GUARD_ARMOR_1;
            break;
        case BATTLE_JAFAR:
            args.cardId = CARD_JAFAR_1;
            break;
        case BATTLE_TRICKMASTER:
            args.cardId = CARD_TRICKMASTER_1;
            break;
        case BATTLE_URSULA:
            args.cardId = CARD_URSULA_1;
            break;
        case BATTLE_PARASITE_CAGE:
            args.cardId = CARD_PARASITE_CAGE_1;
            break;
        case BATTLE_DRAGON_MALEFICENT:
            args.cardId = CARD_DRAGON_MALEFICENT_1;
            break;
        case BATTLE_DARKSIDE:
            args.cardId = CARD_DARKSIDE_1;
            break;
        case BATTLE_OOGIE_BOOGIE:
            args.cardId = CARD_OOGIE_BOOGIE_1;
            break;
        case BATTLE_MARLUXIA_2:
            args.cardId = CARD_MARLUXIA_1;
            break;
        case BATTLE_CARD_SOLDIERS:
            args.cardId = CARD_CARD_SOLDIER_2;
            break;
        case BATTLE_AXEL_1:
            args.cardId = CARD_ID(CARD_FIRE, 5);
            break;
        case BATTLE_LARXENE_1:
            args.cardId = CARD_ID(CARD_THUNDER, 7);
            break;
        case BATTLE_RIKU_1:
            args.cardId = CARD_ID(CARD_AERO, 6);
            break;
        case BATTLE_LEON:
        case BATTLE_HOOK:
            args.cardId = CARD_HOOK_9;
            break;
        case BATTLE_CLOUD:
            args.cardId = CARD_ID(CARD_HI_POTION, 3);
            break;
        case BATTLE_HADES:
            args.cardId = CARD_HADES_9;
            break;
        case BATTLE_MARLUXIA:
            args.cardId = CARD_MARLUXIA_9;
            break;
        case BATTLE_VEXEN_1:
            args.cardId = CARD_ID(CARD_MEGA_ETHER, 4);
            break;
        case BATTLE_RIKU_3:
            args.cardId = CARD_ID(CARD_MEGA_POTION, 2);
            break;
        case BATTLE_RIKU_4:
            args.cardId = CARD_RIKU_9;
            break;
        case BATTLE_AXEL_2:
            args.cardId = CARD_AXEL_9;
            break;
        case BATTLE_LARXENE_2:
            args.cardId = CARD_LARXENE_9;
            break;
        case BATTLE_VEXEN_2:
            args.cardId = CARD_VEXEN_9;
            break;
        case BATTLE_SHADOW_100:
            args.cardId = CARD_ID(CARD_DUMBO, 3);
            break;
        case BATTLE_EVENT_AGRABAH_2:
            args.cardId = CARD_ID(CARD_ETHER, 3);
            break;
        case BATTLE_LEXAEUS:
            args.cardId = CARD_LEXAEUS_9;
            break;
        default:
            work->spawned = TRUE;
            return 1;
        }

        if (gBtlWork->battleId != BATTLE_SHADOW_100) {
            if (!CollectionHasCard(args.cardId)) {
                TaskCreate(&work->tasks, &gTaskDescPrizeBoss, &args);
            }
        } else {
            TaskCreate(&work->tasks, &gTaskDescPrizeBoss, &args);
        }

        work->spawned = TRUE;
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

void PrizeCardInitDraw(PrizeCardInitWork* work) {
    TaskPoolDraw(&work->tasks);
}

void PrizeCardInitDestroy(PrizeCardInitWork* work) {
    TaskPoolDestroy(&work->tasks);
}

u16 PickPrizeMapCardKind(const PrizeMapCardGroup* groups, u16 groupCount) {
    s32 i;
    const PrizeMapCardEntry* entries;
    u16 cnt;
    u16 v;
    u16 card;

    i = 0;

    if (CountRegularMapCards() <= 98) {
        while (i < groupCount) {
            entries = groups[i].entries;
            cnt = groups[i].count;
            v = GetRandom() % 100;

            if (v <= groups[i].chance) {
                card = entries[GetRandom() % cnt].cardId;
                v = CountMapCardsOfKind(card);

                if (v <= 89) {
                    return card;
                }
            }

            i++;

            if (i >= groupCount) {
                i = 0;
            }
        }
    } else {
        return MAP_CARD_ID_NONE;
    }
}

u16 PickPrizeMapCardValue() {
    u16 i;

    do {
        i = GetRandom() % 10;
    } while (sPrizeMapCardValueChances[i] <= GetRandom() % 100);

    return i;
}

u16 PickPrizeMapCardKindForWorld(u16 world, s32 b) {
    const PrizeMapCardGroup* tiles;
    u16 n;

    if (gGameState.flags & GAME_FLAG_RIKU) {
        tiles = sRikuPrizeMapCardGroups[world].data;
        n = sRikuPrizeMapCardGroups[world].size;
    } else {
        tiles = sSoraPrizeMapCardGroups[world].data;
        n = sSoraPrizeMapCardGroups[world].size;
    }

    return PickPrizeMapCardKind(tiles, n);
}

u16 PickPrizeMapCardForWorld(u16 world, s32 b) {
    const PrizeMapCardGroup* tiles;
    u16 base;
    u16 off;

    off = 0;

    if (gGameState.flags & GAME_FLAG_RIKU) {
        tiles = sRikuPrizeMapCardGroups[world].data;
        base = PickPrizeMapCardKind(tiles, sRikuPrizeMapCardGroups[world].size);
    } else {
        tiles = sSoraPrizeMapCardGroups[world].data;
        base = PickPrizeMapCardKind(tiles, sSoraPrizeMapCardGroups[world].size);
    }

    if (base != MAP_CARD_ID_NONE) {
        do {
            off = PickPrizeMapCardValue();
        } while (gMapCardCounts[base + off] == 9);
    }

    return base + off;
}

void CreatePrizeCardTask(TaskPool* pool, struct BtlPrizeSrc* src) {
    TaskCreate(pool, &gTaskDescPrizeCardInit, src);
}

void CreateBossPrizeCardTask(void* pool, void* src) {
    TaskCreate(pool, &gTaskDescPrizeCardInitBoss, src);
}

void DispCardname_0(DispCardnameWork* work, u16* text) {
    ObjPalette* textPalette;
    s32 x;

    InitTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
    textPalette = LoadTextPalette(1);
    work->textPalette = textPalette;
    FadeSetPaletteExcluded(textPalette->index + 16, TRUE);
    work->textSlotCount = LoadTextSlots(text, work->textSlots);
    work->tiles = LoadObjTiles(gMsgBoxTiles, sizeof(gMsgBoxTiles));
    work->palette = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
#ifdef VERSION_JP
    x = (DISPLAY_WIDTH - work->textSlotCount * 10) / 2;
#else
    x = (DISPLAY_WIDTH - GetTextSlotsWidth(work->textSlots, work->textSlotCount)) / 2;
#endif
    work->x = x;
}

s32 DispCardname_1() {
    return 1;
}

void DispCardname_2(DispCardnameWork* work) {
    DrawTextSlots(work->x, 120, work->textSlots, work->textPalette, 50,
                  work->textSlotCount);
    DrawSprite(120, 125, gMsgBoxFrames[0], work->tiles,
               work->palette, NULL, 0, 55);
}

void DispCardname_3(DispCardnameWork* work) {
    FreeTextSlots(work->textSlots, ARRAY_COUNT(work->textSlots));
    ReleaseObjTiles(work->tiles);
    FadeSetPaletteExcluded(work->textPalette->index + 16, FALSE);
    ReleaseObjPalette(work->textPalette);
    ReleaseObjPalette(work->palette);
}

void CreateCardNameDisplay(void* pool, const void* name) {
    TaskCreate(pool, &gTaskDescDispCardname, name);
}

void Version_0(VersionWork* work) {
    work->tiles = LoadSmallFontTiles();
    work->palette = LoadSmallFontPalette();
    work->textLength = EncodeSmallFontString(gVersionString, work->text);
}

s32 Version_1() {
    return 1;
}

void Version_2(VersionWork* work) {
    DrawSmallFontString(0, 152, work->text, work->tiles, work->palette, 0,
                  work->textLength);
}

void Version_3(VersionWork* work) {
    FreeSmallFontResources(work->tiles, work->palette);
}

Task* CreateVersionDisplay(TaskPool* pool) {
    return TaskCreate(pool, &gTaskDescVersion, NULL);
}

static void PrizeCard_0(PrizeMapCardWork* work, s32* args) {
    Collider* collider;

    work->cardId = args[8];
    work->cardDef = &gMapCardDefs[args[8]];
    work->cardBack = &gMapCardBackDefs[work->cardDef->backIndex];
    work->tiles = LoadObjTiles(work->cardDef->tiles, 0x300);
    work->palette = LoadObjPalette(work->cardDef->palette, 32);
    *(u64*)&work->kind = *(u64*)&work->cardDef->kind;
    work->tiles2 = LoadObjTiles(work->cardBack->tiles, work->cardBack->tilesSize);
    work->tiles3 = LoadObjTiles(work->cardBack->tiles, work->cardBack->tilesSize);
    work->palette2 = LoadObjPalette(work->cardBack->palette, work->cardBack->paletteSize);
    work->tiles4 = LoadObjTiles(gCardValueDigitTiles, sizeof(gCardValueDigitTiles));
    work->tiles5 = LoadObjTiles(gBtlShadowTiles, sizeof(gBtlShadowTiles));
    work->palette3 = LoadObjPalette(gCommonObjPalette, sizeof(gCommonObjPalette));
    work->posX = args[0];
    work->posY = args[1];
    work->posZ = 0;
    work->groundZ = 0;
    work->rotation = 24;
    work->vz = -(GetRandom() % 129 + 0x300);
    work->speed = GetRandom() % 129 + 0x80;
    work->moveAngle = GetRandom() % 256;
    work->scaleX = Q_8_8(0.5);
    work->scaleY = Q_8_8(0.5);
    work->scale = Q_8_8(0.5);
    work->flipAngleY = 0;
    work->flipAngleX = 0;
    collider = &work->collider;
    ColliderInit(collider, COLLIDER_TYPE_PICKUP, 8, 10);
    ColliderSetDisabled(collider, TRUE);
    ColliderSetPosition(collider, work->posX, work->posY, work->posZ);
    work->backAnimTimer = 0;
    work->backAnimStep = 0;
    work->backFrame = 0;
    work->timer = 0;
    work->collected = FALSE;
    work->steps = 0;
    work->holdTimer = 0;
    TaskPoolInit(&work->tasks, 1);
    gBtlWork->prizeCount++;
}

static u8 PrizeCard_1(PrizeMapCardWork* work, void* task) {
    s16 x;
    s16 y;

    work->vz += 56;
    work->posZ += work->vz;
    work->posX += (gSineTable[(u8)work->moveAngle] * work->speed) >> 8;
    work->posY += (-gSineTable[(u8)work->moveAngle + 64] * work->speed) >> 8;

    if (ClampBattlePosition(&work->posX, &work->posY, -10, -10)) {
        work->moveAngle += GetRandom() % 57 + 100;
    }

    if (gBtlWork->hcEffect == HC_EFFECT_DRAW) {
        ColliderSetRadius(&work->collider, 50);
    } else {
        ColliderSetRadius(&work->collider, 10);
    }

    if (work->posZ - 8 > work->groundZ) {
        work->posZ = work->groundZ - 8;
        work->vz = -((work->vz * 217) >> 8);
        work->moveAngle = GetAngle(work->posX, work->posY, gBtlWork->actor->x, gBtlWork->actor->y);
        work->moveAngle += GetRandom() % 65 - 32;

        if (work->vz > -0x200) {
            work->vz = -0x200;
        }
    }

    if (work->collider.colliding) {
        work->collected = TRUE;
        m4aSongNumStart(SONG_SYS_ITEMGET);
        AddMapCard(work->cardId);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdatePrizeMapCardFlight);
        WorldToScreen(&x, &y, work->posX, work->posY, work->posZ);
        work->posX = x << 8;
        work->posY = y << 8;
        ColliderSetDisabled(&work->collider, TRUE);
        work->priority = 50;
        AimPrizeMapCardAtCenter(work);
        return 1;
    } else {
        ColliderSetPosition(&work->collider, work->posX, work->posY, work->posZ);
        WorldToScreen(&work->x, &work->y, work->posX, work->posY, work->posZ);
        WorldToScreen(&work->shadowX, &work->shadowY, work->posX, work->posY, work->groundZ);
        work->priority = -0x1004 - (work->posY >> 8) * 4;
        UpdatePrizeMapCardScale(work);
        work->flipAngleX += 2;

        if (work->timer == 20) {
            ColliderSetDisabled(&work->collider, FALSE);
        }

        if (work->timer <= 59) {
            work->timer++;
        }
    }

    return 1;
}

void AimPrizeMapCardAtCenter(PrizeMapCardWork* work) {
    s16 x;
    s16 y;
    s32 dx;
    s32 dy;
    s32 tx;
    s32 ty;

    WorldToScreen(&x, &y, gBtlWork->actor->x, gBtlWork->actor->y, gBtlWork->actor->z);
    tx = 0x7800;
    ty = 0x5000;
    dx = tx - work->posX;
    dy = ty - work->posY;
    work->distance = NormalizeVector2D8(&dx, &dy);
    work->dirX = -dx;
    work->dirY = -dy;
    work->speed = 0x300;
    work->vz = 2;
}

u8 UpdatePrizeMapCardFlight(PrizeMapCardWork* work, void* task) {
    s32 dx;
    s32 dy;
    u8 zero;
    u8 rotation;
    s32 x;
    s32 y;
    s16* px;
    s16* py;

    if (work->speed < 0) {
        dx = 0x7800 - work->posX;
        dy = 0x5000 - work->posY;
        NormalizeVector2D8(&dx, &dy);
        work->dirX = -dx;
        work->dirY = -dy;

        if (work->distance <= 0x7FF) {
            work->steps = 0;
            work->rotation = 0;
            SetTaskUpdate(task, (TaskUpdateFunc)UpdatePrizeMapCardShow);
            CreateCardNameDisplay(&work->tasks, GetRoomName(work->cardDef->kind));
        }
    }

    work->posX += (work->dirX * work->speed) >> 8;
    work->posY += (work->dirY * work->speed) >> 8;
    rotation = work->rotation + 32;
    zero = 0;
    work->rotation = rotation;
    work->flipAngleY += (64 - work->flipAngleY) >> 4;
    work->flipAngleX = zero;
    work->distance = VectorLength2D(0x7800 - work->posX, 0x5000 - work->posY);
    work->speed -= work->vz;
    work->vz += 2;

    if (work->scale <= 0xFF) {
        work->scale += 3;
    }

    x = work->posX >> 8;
    px = &work->x;
    *px = x;
    y = work->posY >> 8;
    py = &work->y;
    *py = y;
    UpdatePrizeMapCardScale(work);
    return 1;
}

u8 UpdatePrizeMapCardShow(PrizeMapCardWork* work, void* task) {
    s32 rotation;
    s16 lim;
    s32 x;
    s16* screenCoord;

    rotation = work->rotation << 8;
    ApproachValue((s32*)&work->flipAngleY, 0, work->steps);
    ApproachValue(&rotation, 0, work->steps);
    ApproachValue(&work->posX, 0x7800, work->steps);
    ApproachValue(&work->posY, 0x5800, work->steps);
    work->rotation = rotation >> 8;

    if (work->steps != 0) {
        work->steps--;
    }

    lim = Q_8_8(1);

    if (work->scale < Q_8_8(1)) {
        work->scale += 2;
    } else {
        work->scale = lim;
    }

    x = work->posX >> 8;
    screenCoord = &work->x;
    *screenCoord = x;
    x = work->posY >> 8;
    screenCoord = &work->y;
    *screenCoord = x;
    UpdatePrizeMapCardScale(work);
    work->holdTimer++;

    if (work->holdTimer == 30) {
        work->holdTimer = 0;
        SetTaskUpdate(task, (TaskUpdateFunc)UpdatePrizeMapCardShrink);
    }

    TaskPoolUpdate(&work->tasks);
    return 1;
}

u8 UpdatePrizeMapCardShrink(PrizeMapCardWork* work) {
    work->rotation += 32;
    WorldToScreen(&work->targetX, &work->targetY, gBtlWork->actor->x, gBtlWork->actor->y, gBtlWork->actor->z);
    work->x += (work->targetX - work->x) >> 3;
    work->y += (work->targetY - work->y) >> 3;
    work->scaleX -= 10;
    work->scaleY -= 10;

    if (work->scaleX <= 10) {
        return 0;
    }

    return 1;
}

static void PrizeCard_2(PrizeMapCardWork* work) {
    u16 pal;
    ObjAffine* affine;
    void* gfx;
    s16 shadowScale;

    pal = !work->collected ? GetBattleSpritePriorityFlags(work->posY) : 0;

    if (work->scaleX == Q_8_8(1) && work->rotation == 0) {
        affine = NULL;
    } else {
        affine = AllocObjAffine(work->rotation, work->scaleX, work->scaleY, TRUE);
    }

    DrawSprite(work->x, (u16)work->y - 8,
               *work->cardDef->sprites,
               work->tiles, work->palette, affine, pal,
               work->priority + 1);

    if (work->cardDef->backIndex == MAP_CARD_COLOR_GOLD) {
        gfx = work->cardBack->sprites[work->backFrame];
    } else {
        gfx = work->cardBack->sprites[0];
    }

    DrawSprite(work->x, (u16)work->y - 8, gfx,
               work->tiles2, work->palette2, affine, pal,
               work->priority);

    if (work->cardDef->backIndex != MAP_CARD_COLOR_GOLD) {
        gfx = gCardValueDigitFrames[work->value];
        DrawSprite(work->x, (u16)work->y - 8, gfx,
                   work->tiles4, work->palette2, affine, pal,
                   work->priority - 1);
    }

    if (!work->collected) {
        shadowScale = Q_8_8(0.8) - ((work->groundZ - work->posZ) >> 7);

        if (shadowScale <= 2) {
            shadowScale = 2;
        }

        DrawSprite(work->shadowX, work->shadowY, gBtlShadowFrames[0],
                   work->tiles5, work->palette3,
                   AllocObjAffine(0, shadowScale, shadowScale, FALSE), pal,
                   work->priority + 2);
    }

    TaskPoolDraw(&work->tasks);
}

static void PrizeCard_3(PrizeMapCardWork* work) {
    FadeSetPaletteExcluded(work->palette2->index + 16, FALSE);
    FadeSetPaletteExcluded(work->palette->index + 16, FALSE);
    ColliderUnregister(&work->collider);
    ReleaseObjTiles(work->tiles);
    ReleaseObjTiles(work->tiles2);
    ReleaseObjTiles(work->tiles4);
    ReleaseObjTiles(work->tiles3);
    ReleaseObjTiles(work->tiles5);
    ReleaseObjPalette(work->palette);
    ReleaseObjPalette(work->palette2);
    ReleaseObjPalette(work->palette3);
    TaskPoolDestroy(&work->tasks);
    gBtlWork->prizeCount--;
}

void UpdatePrizeMapCardScale(PrizeMapCardWork* work) {
    work->scaleX = (-COS(work->flipAngleX + 0x80) * work->scale) >> 8;
    work->scaleY = (-COS(work->flipAngleY + 0x80) * work->scale) >> 8;

    if ((u16)(work->scaleX + 2) <= 4) {
        work->scaleX = 2;
    }

    if ((u16)(work->scaleY + 2) <= 4) {
        work->scaleY = 2;
    }
}

#ifndef VERSION_EU
void UpdatePrizeMapCardBackAnim(PrizeMapCardWork* work) {
    u8* backAnimTimer;
    u8* backFrame;
    u8 k;
    u8 sprite;
    u8 z;
    sprite = gPrizeMapCardBackAnim[work->backAnimStep].sprite;
    backFrame = &work->backFrame;
    z = 0;
    *backFrame = sprite;
    backAnimTimer = &work->backAnimTimer;
    k = work->backAnimStep;

    if (*backAnimTimer == gPrizeMapCardBackAnim[k].duration) {
        work->backAnimStep = k + 1;

        if (work->backAnimStep == 7) {
            work->backAnimStep = z;
        }

        *backAnimTimer = z;
    }

    work->backAnimTimer++;
}
#endif

void CreatePrizeMapCardTask(TaskPool* pool, s32* args) {
    TaskCreate(pool, &gTaskDescPrizeMapCard, args);
}

void SpotLight_0(SpotlightWork* work, u8* src) {
#ifdef PLATFORM_PC
    PortSetSpotlight(1);
#endif
    if (src != NULL) {
        work->endFlag = src;
    } else {
        work->endFlag = &work->ownEndFlag;
        work->ownEndFlag = 0;
    }

    LoadBgTiles(0, gSpotLightTiles, 0xCA0);
    LoadPalette(gSpotLightPalette, (void*)(BG_PLTT + 13 * PLTT_SIZE_4BPP), sizeof(gSpotLightPalette));
    FadeSetPaletteExcluded(13, TRUE);
    LoadBgMap(0, gSpotLightMap, 0x800);
    SetBgScroll(0, 0, 0);
    work->steps = 30;
    work->blendB = 0x1000;
    work->blendA = 0;
    FadeStartOut(FADE_MODE_BLACK, 30);
    gBldCnt = (BLDCNT_TGT1_BG0 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG1 | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_BG3);
}

u8 SpotLight_1(SpotlightWork* work, void* task) {
    ApproachValue(&work->blendA, 0x1000, work->steps);

    if (work->steps != 0) {
        work->steps--;
        work->bldAlpha = ((work->blendB >> 8) << 8) | (work->blendA >> 8);
        gBldAlpha = work->bldAlpha;
    }

    if (*work->endFlag == 1) {
        FadeStartIn(FADE_MODE_BLACK, 30);
        work->steps = 30;
        gBldCnt = (BLDCNT_TGT1_BG0 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG1 | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_BG3);
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateSpotLightFadeOut);
    }

    return 1;
}

s32 UpdateSpotLightFadeOut(SpotlightWork* work) {
    s32 bldAlpha;

    ApproachValue(&work->blendA, 0, work->steps);

    if (work->steps != 0) {
        work->steps--;
    }

    bldAlpha = ((work->blendB >> 8) << 8) | (work->blendA >> 8);
    work->bldAlpha = bldAlpha;
    gBldAlpha = bldAlpha;
    return 1;
}

void SpotLight_2() {
}

void SpotLight_3() {
#ifdef PLATFORM_PC
    PortSetSpotlight(0);
#endif
    FadeSetPaletteExcluded(13, FALSE);
}

void SELMAP_EVKEY_0(SelmapEventKeyWork* work, SelmapEventKeyArgs* args) {
    s32 i;

    CpuFill32(0, work, sizeof(SelmapEventKeyWork));
    work->args = args;
    work->unk_F8 = args->unk_04;
    work->keyCount = CountRemainingEventKeys();

    for (i = 0; i < work->keyCount; i++) {
        InitEventKeyCard(&work->cards[i], GetEventKey(i));
#ifdef PLATFORM_PC
        SynthesisHdRemember(i, GetEventKey(i));
#endif

        if (work->cards[i].sprite.palette != NULL) {
            FadeSetPaletteExcluded(work->cards[i].sprite.palette->index + 16, TRUE);
        }

        if (work->cards[i].sprite.palette2 != NULL) {
            FadeSetPaletteExcluded(work->cards[i].sprite.palette2->index + 16, TRUE);
        }

        if (work->cards[i].sprite.palette3 != NULL) {
            FadeSetPaletteExcluded(work->cards[i].sprite.palette3->index + 16, TRUE);
        }
    }

#ifdef VERSION_EU
    work->tiles = AllocObjTiles(0x800, NULL);
    SetObjTileSource(work->tiles, gSelmapEventKeyTitleTilesByLanguage[gLanguage]);
    AnimInit(&work->anim, gSelmapEventKeyTitleAnimsByLanguage[gLanguage], gSelmapEventKeyTitleFramesByLanguage[gLanguage]);
#else
#ifdef VERSION_JP
    work->tiles = AllocObjTiles(0x480, NULL);
#else
    work->tiles = AllocObjTiles(0x6C0, NULL);
#endif
    SetObjTileSource(work->tiles, gSelmapEventKeyTitleTiles);
    AnimInit(&work->anim, gSelmapEventKeyTitleAnims, gSelmapEventKeyTitleFrames);
#endif
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    work->palette = work->args->palette;
    work->unk_11C = 0;
    work->frame = 0;
    work->mosaicX = 8;
    work->mosaicY = 8;
    work->mosaicTimer = 0;
    work->unk_123 = 0;
    SetObjMosaicSize(work->mosaicX, work->mosaicY);
    work->rowX = 0x7800;
    work->rowY = 0x4000;
    work->unk_114 = 0x100;
    work->unk_116 = 0x100;
    work->unk_11B = 0;
    work->paidCount = 0;
    work->slideSteps = 8;
    work->pulseAngle = 0;
    SetLayeredCardSpritePos(0x10000, work->rowY, &work->cards[0].sprite);
    SetLayeredCardSpritePos(0x10000, work->rowY, &work->cards[1].sprite);
    SetLayeredCardSpritePos(0x10000, work->rowY, &work->cards[2].sprite);
    SetLayeredCardSpritePos(0x10000, work->rowY, &work->cards[3].sprite);
}

s32 SELMAP_EVKEY_1(SelmapEventKeyWork* work, void* task) {
    s32 i;
    EventKey* key;
    u8 n;

    work->gfx = AnimUpdate(&work->anim);
    work->rowTargetX = ((DISPLAY_WIDTH - (work->keyCount - work->paidCount) * 32) << 7) + 0x1000;
    ApproachValue(&work->rowX, work->rowTargetX, work->slideSteps);

    if (work->slideSteps != 0) {
        work->slideSteps--;
    }

    for (i = work->paidCount; i < work->keyCount; i++) {
        SetLayeredCardSpritePos(work->rowX + ((i - work->paidCount) << 13), work->rowY, &work->cards[i].sprite);
    }

    SetObjMosaicSize(work->mosaicX, work->mosaicY);

    if (work->mosaicTimer == 2) {
        if (work->mosaicX != 0) {
            work->mosaicX--;
        }

        if (work->mosaicY != 0) {
            work->mosaicY--;
        }

        work->mosaicTimer = 0;
    }

    work->mosaicTimer++;
    work->frame++;

    if (work->args->closeMode != SELMAP_EVENT_KEY_CLOSE_NONE) {
        SetTaskUpdate(task, (TaskUpdateFunc)UpdateSelmapEventKeyClose);
    }

    // @bug Reads past the list once the last card is paid.
    if (work->cards[work->paidCount].total != 0) {
        key = GetEventKey(0);
        work->cards[work->paidCount].total = key->value;
        n = work->paidCount;

        if (work->cards[n].drawnTotal != work->cards[n].total) {
            UpdateEventKeyTotal(&work->cards[n]);
        }
    }

    work->pulseAngle += 8;
    return 1;
}

s32 UpdateSelmapEventKeyClose(SelmapEventKeyWork* work) {
    u8* mosaicX;
    u8* mosaicY;

    mosaicX = &work->mosaicX;

    if (*mosaicX <= 14) {
        (*mosaicX)++;
    }

    mosaicY = &work->mosaicY;

    if (*mosaicY <= 14) {
        (*mosaicY)++;
    }

    SetObjMosaicSize(*mosaicX, *mosaicY);
    return 1;
}

void SELMAP_EVKEY_2(SelmapEventKeyWork* work) {
    s32 i;
#ifdef PLATFORM_PC
    int firstEntry;
#endif

    switch (work->args->closeMode) {
    case SELMAP_EVENT_KEY_CLOSE_NONE:
    case SELMAP_EVENT_KEY_CLOSE_CANCELLED:
        if (work->mosaicX == 15) {
            return;
        }

        if (work->mosaicY == 15) {
            break;
        }

        for (i = work->paidCount; i < work->keyCount; i++) {
#ifdef PLATFORM_PC
            firstEntry = gSpriteWork->entryCount;
#endif
            if (i == work->paidCount) {
                DrawLayeredCardSpriteScaled(&work->cards[i].sprite, SPRITE_PRIORITY(2) | SPRITE_FLAG_MOSAIC, 0,
                              (gSineTable[(u8)work->pulseAngle] >> 8) * 8 + Q_8_8(1));

                switch (work->cards[i].color) {
                case MAP_CARD_COLOR_RED:
                    DrawSprite(work->cards[i].sprite.x >> 8, (work->cards[i].sprite.y >> 8) + 8,
                               gMapCardUiResources.sprites[4], gMapCardUiResources.extraTiles, gMapCardUiResources.palette, NULL, SPRITE_FLAG_MOSAIC, 20);
                    break;
                case MAP_CARD_COLOR_BLUE:
                    DrawSprite(work->cards[i].sprite.x >> 8, (work->cards[i].sprite.y >> 8) + 8,
                               gMapCardUiResources.sprites[8], gMapCardUiResources.extraTiles, gMapCardUiResources.palette, NULL, SPRITE_FLAG_MOSAIC, 20);
                    break;
                case MAP_CARD_COLOR_GREEN:
                    DrawSprite(work->cards[i].sprite.x >> 8, (work->cards[i].sprite.y >> 8) + 8,
                               gMapCardUiResources.sprites[6], gMapCardUiResources.extraTiles, gMapCardUiResources.palette, NULL, SPRITE_FLAG_MOSAIC, 20);
                    break;
                case MAP_CARD_COLOR_NONE:
                case MAP_CARD_COLOR_GOLD:
                default:
                    break;
                }
            } else {
                DrawLayeredCardSprite(&work->cards[i].sprite, SPRITE_PRIORITY(2) | SPRITE_FLAG_MOSAIC);
            }
#ifdef PLATFORM_PC
            SynthesisHdTag(work, i, firstEntry);
#endif
        }

        break;
    }

    if (work->mosaicX == 15) {
        return;
    }

    if (work->mosaicY == 15) {
        return;
    }

    DrawSprite(120, 42, work->gfx, work->tiles, work->palette, NULL, SPRITE_FLAG_MOSAIC, 10);
}

void SELMAP_EVKEY_3(SelmapEventKeyWork* work) {
    s32 i;

    for (i = 0; i < work->keyCount; i++) {
        ReleaseLayeredCardSprite(&work->cards[i].sprite);
    }

    ReleaseObjTiles(work->tiles);
}

void InitEventKeyCard(EventKeyCard* card, EventKey* key) {
    MapCardDef* def;
    MapCardBackDef* backDef;
    MapCardBackDef* colorBackDef;
    const CardBack* cardBack;
    u8 n;
    void* empty;
    u8 total;
    u8* color;

    CpuFill32(0, card, sizeof(EventKeyCard));

    if (key->kind != MAP_CARD_NONE) {
        // @bug kind is a MapCardKind, not a group (latent: only key card kinds are used).
        def = &gMapCardDefs[key->kind * 10];
        backDef = &gMapCardBackDefs[def->backIndex];
        card->sprite.tiles = LoadObjTiles(def->tiles, def->tilesSize);
        card->sprite.palette = LoadObjPalette(def->palette, def->paletteSize);
        card->sprite.gfx = *def->sprites;
        card->sprite.tiles2 = LoadObjTiles(backDef->tiles, backDef->tilesSize);
        card->sprite.palette2 = LoadObjPalette(backDef->palette, backDef->paletteSize);
        card->sprite.gfx2 = *backDef->sprites;
        card->sprite.tiles3 = NULL;
        card->sprite.palette3 = NULL;
        return;
    }

    card->sprite.tiles = NULL;
    card->sprite.palette = NULL;
    card->sprite.gfx = NULL;
    card->sprite.tiles3 = NULL;
    card->sprite.palette3 = NULL;

    if (key->color == MAP_CARD_COLOR_NONE) {
        n = key->color;
        card->sprite.tiles2 = NULL;
        card->sprite.palette2 = NULL;
        card->sprite.gfx2 = NULL;
    } else {
        colorBackDef = &gMapCardBackDefs[key->color];
        card->sprite.tiles2 = LoadObjTiles(colorBackDef->tiles2, colorBackDef->tilesSize2);
        card->sprite.palette2 = LoadObjPalette(colorBackDef->palette, colorBackDef->paletteSize);
        card->sprite.gfx2 = *colorBackDef->sprites2;
        card->sprite.tiles3 = NULL;
        card->sprite.tiles = NULL;
        card->sprite.palette = NULL;
        card->sprite.gfx = NULL;
        card->sprite.palette3 = NULL;

        switch (key->color) {
        case MAP_CARD_COLOR_GREEN:
            n = MAP_CARD_COLOR_GREEN;
            break;
        case MAP_CARD_COLOR_RED:
            n = MAP_CARD_COLOR_RED;
            break;
        case MAP_CARD_COLOR_BLUE:
            n = MAP_CARD_COLOR_BLUE;
            break;
        default:
            n = key->color;
            break;
        }
    }

    color = &card->color;
    *color = (empty = NULL, n);

    if (key->rule == EVENT_KEY_RULE_NONE) {
        return;
    }

    switch (key->rule) {
    case EVENT_KEY_RULE_AT_LEAST:
        if (key->value <= 9) {
            card->sprite.tiles3 = AllocSpriteFrameTiles(0x100);
            UpdateSpriteFrameTiles(card->sprite.tiles3, gKeyValueFrames[1], gMapSelectRequirementTiles);
            card->sprite.gfx3 = empty;
            RequestDma3Copy(card->sprite.tiles3->src + key->value * 128, (void*)(OBJ_VRAM0 + (card->sprite.tiles3->index + 4) * 32), 128);
            RequestDma3Copy(card->sprite.tiles3->src + 0x500, (void*)(OBJ_VRAM0 + card->sprite.tiles3->index * 32), 128);
        } else {
            card->sprite.tiles3 = AllocSpriteFrameTiles(0x180);
            UpdateSpriteFrameTiles(card->sprite.tiles3, gKeyValueFrames[3], gMapSelectRequirementTiles);
            card->sprite.gfx3 = empty;
            RequestDma3Copy(card->sprite.tiles3->src + (u8)(key->value / 10) * 128, (void*)(OBJ_VRAM0 + (card->sprite.tiles3->index + 4) * 32), 128);
            RequestDma3Copy(card->sprite.tiles3->src + (key->value - (u8)(key->value / 10) * 10) * 128, (void*)(OBJ_VRAM0 + (card->sprite.tiles3->index + 8) * 32), 128);
            RequestDma3Copy(card->sprite.tiles3->src + 0x500, (void*)(OBJ_VRAM0 + card->sprite.tiles3->index * 32), 128);
        }

        break;
    case EVENT_KEY_RULE_AT_MOST:
        if (key->value <= 9) {
            card->sprite.tiles3 = AllocSpriteFrameTiles(0x100);
            UpdateSpriteFrameTiles(card->sprite.tiles3, gKeyValueFrames[1], gMapSelectRequirementTiles);
            card->sprite.gfx3 = empty;
            RequestDma3Copy(card->sprite.tiles3->src + key->value * 128, (void*)(OBJ_VRAM0 + (card->sprite.tiles3->index + 4) * 32), 128);
            RequestDma3Copy(card->sprite.tiles3->src + 0x580, (void*)(OBJ_VRAM0 + card->sprite.tiles3->index * 32), 128);
        } else {
            card->sprite.tiles3 = AllocSpriteFrameTiles(0x180);
            UpdateSpriteFrameTiles(card->sprite.tiles3, gKeyValueFrames[3], gMapSelectRequirementTiles);
            card->sprite.gfx3 = empty;
            RequestDma3Copy(card->sprite.tiles3->src + (u8)(key->value / 10) * 128, (void*)(OBJ_VRAM0 + (card->sprite.tiles3->index + 4) * 32), 128);
            RequestDma3Copy(card->sprite.tiles3->src + (key->value - (u8)(key->value / 10) * 10) * 128, (void*)(OBJ_VRAM0 + (card->sprite.tiles3->index + 8) * 32), 128);
            RequestDma3Copy(card->sprite.tiles3->src + 0x580, (void*)(OBJ_VRAM0 + card->sprite.tiles3->index * 32), 128);
        }

        break;
    case EVENT_KEY_RULE_EXACT:
        if (key->value <= 9) {
            card->sprite.tiles3 = AllocSpriteFrameTiles(0x100);
            UpdateSpriteFrameTiles(card->sprite.tiles3, gKeyValueFrames[1], gMapSelectRequirementTiles);
            card->sprite.gfx3 = empty;
            RequestDma3Copy(card->sprite.tiles3->src + key->value * 128, (void*)(OBJ_VRAM0 + (card->sprite.tiles3->index + 4) * 32), 128);
            RequestDma3Copy(card->sprite.tiles3->src + 0x600, (void*)(OBJ_VRAM0 + card->sprite.tiles3->index * 32), 128);
        } else {
            card->sprite.tiles3 = AllocSpriteFrameTiles(0x180);
            UpdateSpriteFrameTiles(card->sprite.tiles3, gKeyValueFrames[3], gMapSelectRequirementTiles);
            card->sprite.gfx3 = empty;
            RequestDma3Copy(card->sprite.tiles3->src + (u8)(key->value / 10) * 128, (void*)(OBJ_VRAM0 + (card->sprite.tiles3->index + 4) * 32), 128);
            RequestDma3Copy(card->sprite.tiles3->src + (key->value - (u8)(key->value / 10) * 10) * 128, (void*)(OBJ_VRAM0 + (card->sprite.tiles3->index + 8) * 32), 128);
            RequestDma3Copy(card->sprite.tiles3->src + 0x600, (void*)(OBJ_VRAM0 + card->sprite.tiles3->index * 32), 128);
        }

        break;
    case EVENT_KEY_RULE_TOTAL:
        if (key->value <= 9) {
            card->sprite.tiles3 = AllocSpriteFrameTiles(0x80);
            UpdateSpriteFrameTiles(card->sprite.tiles3, gKeyValueFrames[0], gMapSelectRequirementTiles);
            card->sprite.gfx3 = empty;
            RequestDma3Copy(card->sprite.tiles3->src + key->value * 128, (void*)(OBJ_VRAM0 + card->sprite.tiles3->index * 32), 128);
        } else {
            card->sprite.tiles3 = AllocSpriteFrameTiles(0x100);
            UpdateSpriteFrameTiles(card->sprite.tiles3, gKeyValueFrames[2], gMapSelectRequirementTiles);
            card->sprite.gfx3 = empty;
            RequestDma3Copy(card->sprite.tiles3->src + (u8)(key->value / 10) * 128, (void*)(OBJ_VRAM0 + card->sprite.tiles3->index * 32), 128);
            RequestDma3Copy(card->sprite.tiles3->src + (key->value - (u8)(key->value / 10) * 10) * 128, (void*)(OBJ_VRAM0 + (card->sprite.tiles3->index + 4) * 32), 128);
        }

        total = key->value;
        card->total = total;
        card->drawnTotal = total;
        break;
    }

    card->sprite.tiles = NULL;
    card->sprite.palette = NULL;
    card->sprite.gfx = NULL;
    card->sprite.palette3 = LoadObjPalette(gDoorCardPalette, sizeof(gDoorCardPalette));

    if (card->sprite.tiles2 == NULL) {
        cardBack = &gCardBacks[CARD_BACK_WHITE];
        card->sprite.tiles2 = LoadObjTiles(cardBack->tiles2, 0x300);
        card->sprite.palette2 = LoadObjPalette(gDoorCardPalette, sizeof(gDoorCardPalette));
        card->sprite.gfx2 = cardBack->gfx2;
    }
}

void UpdateEventKeyTotal(EventKeyCard* card) {
    void* empty;

    if (card->total <= 9) {
        empty = NULL;
        UpdateSpriteFrameTiles(card->sprite.tiles3, gKeyValueFrames[0], gMapSelectRequirementTiles);
        card->sprite.gfx3 = empty;
        RequestDma3Copy(card->sprite.tiles3->src + card->total * 128, (void*)(OBJ_VRAM0 + card->sprite.tiles3->index * 32), 128);
    } else {
        empty = NULL;
        UpdateSpriteFrameTiles(card->sprite.tiles3, gKeyValueFrames[2], gMapSelectRequirementTiles);
        card->sprite.gfx3 = empty;
        RequestDma3Copy(card->sprite.tiles3->src + (u16)(card->total / 10) * 128, (void*)(OBJ_VRAM0 + card->sprite.tiles3->index * 32), 128);
        RequestDma3Copy(card->sprite.tiles3->src + (card->total - (u16)(card->total / 10) * 10) * 128, (void*)(OBJ_VRAM0 + (card->sprite.tiles3->index + 4) * 32), 128);
    }

    card->drawnTotal = card->total;
}

void SetLayeredCardSpritePos(s32 x, s32 y, LayeredCardSprite* sprite) {
    sprite->x = x;
    sprite->y = y;
}

void DrawLayeredCardSpriteScaled(LayeredCardSprite* sprite, u16 flags, s16 dy, s16 scale) {
    ObjAffine* affine;

    affine = AllocObjAffine(0, scale, scale, TRUE);

    if (sprite->tiles != NULL) {
        DrawSprite(sprite->x >> 8, dy + (sprite->y >> 8), sprite->gfx, sprite->tiles, sprite->palette, affine, flags, 10);
    }

    if (sprite->tiles2 != NULL) {
        DrawSprite(sprite->x >> 8, dy + (sprite->y >> 8), sprite->gfx2, sprite->tiles2, sprite->palette2, affine, flags, 9);
    }

    if (sprite->tiles3 != NULL) {
        DrawSprite(sprite->x >> 8, dy + (sprite->y >> 8), sprite->gfx3, sprite->tiles3, sprite->palette3, affine, flags, 8);
    }
}

void DrawLayeredCardSprite(LayeredCardSprite* sprite, u16 flags) {
    if (sprite->tiles != NULL) {
        DrawSprite(sprite->x >> 8, sprite->y >> 8, sprite->gfx, sprite->tiles, sprite->palette, NULL, flags, 10);
    }

    if (sprite->tiles2 != NULL) {
        DrawSprite(sprite->x >> 8, sprite->y >> 8, sprite->gfx2, sprite->tiles2, sprite->palette2, NULL, flags, 9);
    }

    if (sprite->tiles3 != NULL) {
        DrawSprite(sprite->x >> 8, sprite->y >> 8, sprite->gfx3, sprite->tiles3, sprite->palette3, NULL, flags, 8);
    }
}

ObjTiles* AllocKeyValueTiles(u8 value) {
    ObjTiles* obj;

    if (value != 0) {
        obj = AllocSpriteFrameTiles(256);
        UpdateSpriteFrameTiles(obj, gKeyValueFrames[1], gMapSelectRequirementTiles);
        RequestDma3Copy(&(obj->src)[value * 128], (void*)(OBJ_VRAM0 + (obj->index + 4) * 32), 128);
        RequestDma3Copy(&(obj->src)[0x500], (void*)(OBJ_VRAM0 + obj->index * 32), 128);
    } else {
        obj = AllocSpriteFrameTiles(128);
        UpdateSpriteFrameTiles(obj, gKeyValueFrames[0], gMapSelectRequirementTiles);
    }

    return obj;
}

void ReleaseLayeredCardSprite(LayeredCardSprite* sprite) {
    if (sprite->tiles != NULL) {
        ReleaseObjTiles(sprite->tiles);
    }

    if (sprite->tiles2 != NULL) {
        ReleaseObjTiles(sprite->tiles2);
    }

    if (sprite->tiles3 != NULL) {
        ReleaseObjTiles(sprite->tiles3);
    }

    if (sprite->palette != NULL) {
        ReleaseObjPalette(sprite->palette);
    }

    if (sprite->palette2 != NULL) {
        ReleaseObjPalette(sprite->palette2);
    }

    if (sprite->palette3 != NULL) {
        ReleaseObjPalette(sprite->palette3);
    }
}

u8 GetRoomCardBackIndex(u16 kind) {
    u8 idx;

    idx = 0;

    switch (kind) {
    case MAP_CARD_TRANQUIL_DARKNESS:
        idx = MAP_CARD_ID(MAP_CARD_GROUP_TRANQUIL_DARKNESS, 0);
        break;
    case MAP_CARD_GUARDED_TROVE:
        idx = MAP_CARD_ID(MAP_CARD_GROUP_GUARDED_TROVE, 0);
        break;
    case MAP_CARD_LOOMING_DARKNESS:
        idx = MAP_CARD_ID(MAP_CARD_GROUP_LOOMING_DARKNESS, 0);
        break;
    case MAP_CARD_SLEEPING_DARKNESS:
        idx = MAP_CARD_ID(MAP_CARD_GROUP_SLEEPING_DARKNESS, 0);
        break;
    case MAP_CARD_MOMENTS_REPRIEVE:
        idx = MAP_CARD_ID(MAP_CARD_GROUP_MOMENTS_REPRIEVE, 0);
        break;
    case MAP_CARD_FEEBLE_DARKNESS:
        idx = MAP_CARD_ID(MAP_CARD_GROUP_FEEBLE_DARKNESS, 0);
        break;
    case MAP_CARD_ALMIGHTY_DARKNESS:
        idx = MAP_CARD_ID(MAP_CARD_GROUP_ALMIGHTY_DARKNESS, 0);
        break;
    case MAP_CARD_CALM_BOUNTY:
        idx = MAP_CARD_ID(MAP_CARD_GROUP_CALM_BOUNTY, 0);
        break;
    case MAP_CARD_FALSE_BOUNTY:
        idx = MAP_CARD_ID(MAP_CARD_GROUP_FALSE_BOUNTY, 0);
        break;
    case MAP_CARD_MOOGLE_ROOM:
        idx = MAP_CARD_ID(MAP_CARD_GROUP_MOOGLE_ROOM, 0);
        break;
    case MAP_CARD_SORCEROUS_WAKING:
        idx = MAP_CARD_ID(MAP_CARD_GROUP_SORCEROUS_WAKING, 0);
        break;
    case MAP_CARD_MARTIAL_WAKING:
        idx = MAP_CARD_ID(MAP_CARD_GROUP_MARTIAL_WAKING, 0);
        break;
    case MAP_CARD_ALCHEMIC_WAKING:
        idx = MAP_CARD_ID(MAP_CARD_GROUP_ALCHEMIC_WAKING, 0);
        break;
    case MAP_CARD_MEETING_GROUND:
        idx = MAP_CARD_ID(MAP_CARD_GROUP_MEETING_GROUND, 0);
        break;
    case MAP_CARD_MINGLING_WORLDS:
        idx = MAP_CARD_ID(MAP_CARD_GROUP_MINGLING_WORLDS, 0);
        break;
    case MAP_CARD_STRONG_INITIATIVE:
        idx = MAP_CARD_ID(MAP_CARD_GROUP_STRONG_INITIATIVE, 0);
        break;
    case MAP_CARD_LASTING_DAZE:
        idx = MAP_CARD_ID(MAP_CARD_GROUP_LASTING_DAZE, 0);
        break;
    case MAP_CARD_STAGNANT_SPACE:
        idx = MAP_CARD_ID(MAP_CARD_GROUP_STAGNANT_SPACE, 0);
        break;
    case MAP_CARD_PREMIUM_ROOM:
        idx = MAP_CARD_ID(MAP_CARD_GROUP_PREMIUM_ROOM, 0);
        break;
    case MAP_CARD_WHITE_ROOM:
        idx = MAP_CARD_ID(MAP_CARD_GROUP_WHITE_ROOM, 0);
        break;
    case MAP_CARD_BLACK_ROOM:
        idx = MAP_CARD_ID(MAP_CARD_GROUP_BLACK_ROOM, 0);
        break;
    case MAP_CARD_KEY_OF_BEGINNINGS:
        idx = MAP_CARD_ID(MAP_CARD_GROUP_KEY_OF_BEGINNINGS, 0);
        break;
    case MAP_CARD_KEY_OF_GUIDANCE:
        idx = MAP_CARD_ID(MAP_CARD_GROUP_KEY_OF_GUIDANCE, 0);
        break;
    case MAP_CARD_KEY_TO_TRUTH:
        idx = MAP_CARD_ID(MAP_CARD_GROUP_KEY_TO_TRUTH, 0);
        break;
    case MAP_CARD_TEEMING_DARKNESS:
    case ROOM_NAME_UNKNOWN_PLACE:
        idx = MAP_CARD_ID(MAP_CARD_GROUP_TEEMING_DARKNESS, 0);
        break;
    case MAP_CARD_KEY_TO_REWARDS:
    case ROOM_NAME_HIDDEN_CHAMBER:
        idx = MAP_CARD_ID(MAP_CARD_GROUP_KEY_TO_REWARDS, 0);
        break;
    }

    return gMapCardDefs[idx].backIndex;
}

TaskDesc gTaskDescPrizeCardInit = {
    "PrizeCardInit",
    (TaskInitFunc)PrizeCardInitInit,
    (TaskUpdateFunc)PrizeCardInit_1,
    (TaskDrawFunc)PrizeCardInitDraw,
    (TaskDestroyFunc)PrizeCardInitDestroy,
    sizeof(PrizeCardInitWork),
};

TaskDesc gTaskDescPrizeCardInitBoss = {
    "PrizeCardInit_Boss",
    (TaskInitFunc)PrizeCardInitInit,
    (TaskUpdateFunc)PrizeCardInit_Boss_1,
    (TaskDrawFunc)PrizeCardInitDraw,
    (TaskDestroyFunc)PrizeCardInitDestroy,
    sizeof(PrizeCardInitWork),
};

TaskDesc gTaskDescDispCardname = {
    "DispCardname",
    (TaskInitFunc)DispCardname_0,
    (TaskUpdateFunc)DispCardname_1,
    (TaskDrawFunc)DispCardname_2,
    (TaskDestroyFunc)DispCardname_3,
    sizeof(DispCardnameWork),
};

TaskDesc gTaskDescVersion = {
    "Version",
    (TaskInitFunc)Version_0,
    (TaskUpdateFunc)Version_1,
    (TaskDrawFunc)Version_2,
    (TaskDestroyFunc)Version_3,
    sizeof(VersionWork),
};

TaskDesc gTaskDescPrizeMapCard = {
    "PrizeCard",
    (TaskInitFunc)PrizeCard_0,
    (TaskUpdateFunc)PrizeCard_1,
    (TaskDrawFunc)PrizeCard_2,
    (TaskDestroyFunc)PrizeCard_3,
    sizeof(PrizeMapCardWork),
};

TaskDesc gTaskDescSpotLight = {
    "SpotLight",
    (TaskInitFunc)SpotLight_0,
    (TaskUpdateFunc)SpotLight_1,
    (TaskDrawFunc)SpotLight_2,
    (TaskDestroyFunc)SpotLight_3,
    sizeof(SpotlightWork),
};

#ifdef VERSION_EU
void* gSelmapEventKeyTitleAnimsByLanguage[5] = { gSelmapEventKeyTitleAnims, gSelmapEventKeyTitleFrenchAnims, gSelmapEventKeyTitleGermanAnims, gSelmapEventKeyTitleItalianAnims, gSelmapEventKeyTitleSpanishAnims };
void* gSelmapEventKeyTitleFramesByLanguage[5] = { gSelmapEventKeyTitleFrames, gSelmapEventKeyTitleFrenchFrames, gSelmapEventKeyTitleGermanFrames, gSelmapEventKeyTitleItalianFrames, gSelmapEventKeyTitleSpanishFrames };
void* gSelmapEventKeyTitleTilesByLanguage[5] = { gSelmapEventKeyTitleTiles, gSelmapEventKeyTitleFrenchTiles, gSelmapEventKeyTitleGermanTiles, gSelmapEventKeyTitleItalianTiles, gSelmapEventKeyTitleSpanishTiles };
#endif

TaskDesc gTaskDescSELMAPEVKEY = {
    "SELMAP_EVKEY",
    (TaskInitFunc)SELMAP_EVKEY_0,
    (TaskUpdateFunc)SELMAP_EVKEY_1,
    (TaskDrawFunc)SELMAP_EVKEY_2,
    (TaskDestroyFunc)SELMAP_EVKEY_3,
    sizeof(SelmapEventKeyWork),
};
