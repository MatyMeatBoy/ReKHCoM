/**
 * mode_jiminy.c
 * Jiminy's Journal Screen
 */

#include "sprites_msg.h"
#include "engine.h"
#include "jiminy_journal.h"
#include "sprites_bos5.h"
#include "sprites_emy.h"
#include "sprites_evt.h"
#include "sprites_smn.h"
#include "sprites_worldinspect.h"
#include "sprites_card_pictures.h"
#include "sprites_card.h"
#include "hum.h"
#include "gba/keys.h"
#include "gba/io_reg.h"
#include "player_progression.h"
#include "malloc.h"
#include "fade.h"
#include "songs.h"
#include <stdlib.h>
#include "jiminy_records_index_data.h"
#include "common_text.h"
#include "jiminy_data.h"
#include "anim.h"
#include "display.h"
#include "engine_math.h"
#include "game_state.h"
#include "jiminy_inline_text_data.h"
#include "jiminy_types.h"
#include "key.h"
#include "m4a_song.h"
#include "map_api.h"
#include "mode.h"
#include "mode_jiminy.h"
#include "monsgage.h"
#include "msg_api.h"
#include "obj.h"
#include "obj_api.h"
#include "poo_api.h"
#include "sprites_map.h"
#include "sprites_map_tasks.h"
#include "sprites_pooh.h"
#include "sprites_sora.h"
#include "system_state.h"
#include "text.h"
#include "text_types.h"
#include "types.h"
#include "gba/defines.h"
#include <stddef.h>
#include "registration_data.h"
#include "sprite_palettes.h"
#include "macros.h"

static const JiminyEntry sJiminyEntries[21] = {
    { gJiminyRootMap, gJiminyRootNames, ARRAY_COUNT(gJiminyRootNames), JIMINY_ENTRY_NONE, gJiminyEntry00Children, NULL, 0 },
    { gJiminyStoryListMap, gJiminyEntry01Names, ARRAY_COUNT(gJiminyEntry01Names), 0, NULL, gJiminyEntry01Flags, 1 },
    { gJiminyCardListMap, gJiminyEntry02Names, ARRAY_COUNT(gJiminyEntry02Names), 0, gJiminyEntry02Children, NULL, 0 },
    { gJiminyCharacterListMap, gJiminyEntry03Names, ARRAY_COUNT(gJiminyEntry03Names), 0, gJiminyEntry03Children, NULL, 0 },
    { gJiminyCardListMap, gJiminyEntry04Names, ARRAY_COUNT(gJiminyEntry04Names), 2, NULL, gJiminyEntry04Flags, 2 },
    { gJiminyCardListMap, gJiminyEntry05Names, ARRAY_COUNT(gJiminyEntry05Names), 2, NULL, gJiminyEntry05Flags, 3 },
    { gJiminyCardListMap, gJiminyEntry06Names, ARRAY_COUNT(gJiminyEntry06Names), 2, NULL, gJiminyEntry06Flags, 4 },
    { gJiminyCardListMap, gJiminyEntry07Names, ARRAY_COUNT(gJiminyEntry07Names), 2, NULL, gJiminyEntry07Flags, 5 },
    { gJiminyCardListMap, gJiminyEntry08Names, ARRAY_COUNT(gJiminyEntry08Names), 2, NULL, gJiminyEntry08Flags, 6 },
    { gJiminyCardListMap, gJiminyEntry09Names, ARRAY_COUNT(gJiminyEntry09Names), 2, NULL, gJiminyEntry09Flags, 7 },
    { gJiminyCardListMap, gJiminyEntry10Names, ARRAY_COUNT(gJiminyEntry10Names), 2, NULL, gJiminyEntry10Flags, 8 },
    { gJiminyCharacterListMap, gJiminyEntry11Names, ARRAY_COUNT(gJiminyEntry11Names), 3, NULL, gJiminyEntry11Flags, 9 },
    { gJiminyCharacterListMap, gJiminyEntry12Names, ARRAY_COUNT(gJiminyEntry12Names), 3, NULL, gJiminyEntry12Flags, 10 },
    { gJiminyCharacterListMap, gJiminyEntry13Names, ARRAY_COUNT(gJiminyEntry13Names), 3, NULL, gJiminyEntry13Flags, 11 },
    { gJiminyRootMap, gJiminyRootNames, ARRAY_COUNT(gJiminyRootNames), JIMINY_ENTRY_NONE, gJiminyEntry14Children, NULL, 0 },
    { gJiminyStoryListMap, gJiminyEntry15Names, ARRAY_COUNT(gJiminyEntry15Names), 14, NULL, gJiminyEntry15Flags, 12 },
    { gJiminyCardListMap, gJiminyEntry16Names, ARRAY_COUNT(gJiminyEntry16Names), 14, NULL, gJiminyEntry16Flags, 13 },
    { gJiminyCharacterListMap, gJiminyEntry03Names, ARRAY_COUNT(gJiminyEntry03Names), 14, gJiminyEntry17Children, NULL, 0 },
    { gJiminyCharacterListMap, gJiminyEntry18Names, ARRAY_COUNT(gJiminyEntry18Names), 17, NULL, gJiminyEntry18Flags, 14 },
    { gJiminyCharacterListMap, gJiminyEntry19Names, ARRAY_COUNT(gJiminyEntry19Names), 17, NULL, gJiminyEntry19Flags, 15 },
    { gJiminyCharacterListMap, gJiminyEntry20Names, ARRAY_COUNT(gJiminyEntry20Names), 17, NULL, gJiminyEntry20Flags, 16 },
};

#if defined(VERSION_US)
static const JiminyDetail sJiminyEntry01Details[17] = {
    { gJiminyStoryTale1Name, gJiminyStoryTale1Lines, ARRAY_COUNT(gJiminyStoryTale1Lines), gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gJiminyStoryTale2Name, gJiminyStoryTale2Lines, ARRAY_COUNT(gJiminyStoryTale2Lines), gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gJiminyStoryTale3Name, gJiminyStoryTale3Lines, ARRAY_COUNT(gJiminyStoryTale3Lines), gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gJiminyStoryTale4Name, gJiminyStoryTale4Lines, ARRAY_COUNT(gJiminyStoryTale4Lines), gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gWorldNameTraverseTown, gJiminyStoryTraverseTownLines, ARRAY_COUNT(gJiminyStoryTraverseTownLines), gWorldImageTraverseTownFrame0, gWorldImageTraverseTownPalette, gWorldImageTraverseTownTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gWorldNameWonderland, gJiminyStoryWonderlandLines, ARRAY_COUNT(gJiminyStoryWonderlandLines), gWorldImageWonderlandFrame0, gWorldImageWonderlandPalette, gWorldImageWonderlandTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gWorldNameOlympusColiseum, gJiminyStoryOlympusColiseumLines, ARRAY_COUNT(gJiminyStoryOlympusColiseumLines), gWorldImageOlympusColiseumFrame0, gWorldImageOlympusColiseumPalette, gWorldImageOlympusColiseumTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gWorldNameAgrabah, gJiminyStoryAgrabahLines, ARRAY_COUNT(gJiminyStoryAgrabahLines), gWorldImageAgrabahFrame0, gWorldImageAgrabahPalette, gWorldImageAgrabahTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gWorldNameHalloweenTown, gJiminyStoryHalloweenTownLines, ARRAY_COUNT(gJiminyStoryHalloweenTownLines), gWorldImageHalloweenTownFrame0, gWorldImageHalloweenTownPalette, gWorldImageHalloweenTownTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gWorldNameMonstro, gJiminyStoryMonstroLines, ARRAY_COUNT(gJiminyStoryMonstroLines), gWorldImageMonstroFrame0, gWorldImageMonstroPalette, gWorldImageMonstroTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, -8, 0 },
    { gWorldNameAtlantica, gJiminyStoryAtlanticaLines, ARRAY_COUNT(gJiminyStoryAtlanticaLines), gWorldImageAtlanticaFrame0, gWorldImageAtlanticaPalette, gWorldImageAtlanticaTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gWorldNameNeverLand, gJiminyStoryNeverLandLines, ARRAY_COUNT(gJiminyStoryNeverLandLines), gWorldImageNeverLandFrame0, gWorldImageNeverLandPalette, gWorldImageNeverLandTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gWorldNameHollowBastion, gJiminyStoryHollowBastionLines, ARRAY_COUNT(gJiminyStoryHollowBastionLines), gWorldImageHollowBastionFrame0, gWorldImageHollowBastionPalette, gWorldImageHollowBastionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gWorldName100AcreWood, gJiminyStory100AcreWoodLines, ARRAY_COUNT(gJiminyStory100AcreWoodLines), gWorldImage100AcreWoodFrame0, gWorldImage100AcreWoodPalette, gWorldImage100AcreWoodTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gWorldNameTwilightTown, gJiminyStoryTwilightTownLines, ARRAY_COUNT(gJiminyStoryTwilightTownLines), gWorldImageTwilightTownFrame0, gWorldImageTwilightTownPalette, gWorldImageTwilightTownTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gWorldNameDestinyIslands, gJiminyStoryDestinyIslandsLines, ARRAY_COUNT(gJiminyStoryDestinyIslandsLines), gWorldImageDestinyIslandsFrame0, gWorldImageDestinyIslandsPalette, gWorldImageDestinyIslandsTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gWorldNameCastleOblivion, gJiminyStoryCastleOblivionLines, ARRAY_COUNT(gJiminyStoryCastleOblivionLines), gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry15Details[6] = {
    { gJiminyRikuStoryTale1Name, gJiminyRikuStoryTale1Lines, ARRAY_COUNT(gJiminyRikuStoryTale1Lines), gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gJiminyRikuStoryTale2Name, gJiminyRikuStoryTale2Lines, ARRAY_COUNT(gJiminyRikuStoryTale2Lines), gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gJiminyRikuStoryTale3Name, gJiminyRikuStoryTale3Lines, ARRAY_COUNT(gJiminyRikuStoryTale3Lines), gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gJiminyRikuStoryTale4Name, gJiminyRikuStoryTale4Lines, ARRAY_COUNT(gJiminyRikuStoryTale4Lines), gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gJiminyRikuStoryTale5Name, gJiminyRikuStoryTale5Lines, ARRAY_COUNT(gJiminyRikuStoryTale5Lines), gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gJiminyRikuStoryTale6Name, gJiminyRikuStoryTale6Lines, ARRAY_COUNT(gJiminyRikuStoryTale6Lines), gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry04Details[17] = {
    { gCardNameKingdomKey, gJiminyAttackCardKingdomKeyLines, ARRAY_COUNT(gJiminyAttackCardKingdomKeyLines), gCardWep01Frame0, gCardWep01Palette, gCardWep01Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameThreeWishes, gJiminyAttackCardThreeWishesLines, ARRAY_COUNT(gJiminyAttackCardThreeWishesLines), gCardWep04Frame0, gCardWep04Palette, gCardWep04Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameCrabclaw, gJiminyAttackCardCrabclawLines, ARRAY_COUNT(gJiminyAttackCardCrabclawLines), gCardWep05Frame0, gCardWep05Palette, gCardWep05Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNamePumpkinhead, gJiminyAttackCardPumpkinheadLines, ARRAY_COUNT(gJiminyAttackCardPumpkinheadLines), gCardWep06Frame0, gCardWep06Palette, gCardWep06Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameFairyHarp, gJiminyAttackCardFairyHarpLines, ARRAY_COUNT(gJiminyAttackCardFairyHarpLines), gCardWep07Frame0, gCardWep07Palette, gCardWep07Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameWishingStar, gJiminyAttackCardWishingStarLines, ARRAY_COUNT(gJiminyAttackCardWishingStarLines), gCardWep08Frame0, gCardWep08Palette, gCardWep08Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameSpellbinder, gJiminyAttackCardSpellbinderLines, ARRAY_COUNT(gJiminyAttackCardSpellbinderLines), gCardWep09Frame0, gCardWep09Palette, gCardWep09Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameMetalChocobo, gJiminyAttackCardMetalChocoboLines, ARRAY_COUNT(gJiminyAttackCardMetalChocoboLines), gCardWep10Frame0, gCardWep10Palette, gCardWep10Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameOlympia, gJiminyAttackCardOlympiaLines, ARRAY_COUNT(gJiminyAttackCardOlympiaLines), gCardWep03Frame0, gCardWep03Palette, gCardWep03Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameLionheart, gJiminyAttackCardLionheartLines, ARRAY_COUNT(gJiminyAttackCardLionheartLines), gCardWep11Frame0, gCardWep11Palette, gCardWep11Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameLadyLuck, gJiminyAttackCardLadyLuckLines, ARRAY_COUNT(gJiminyAttackCardLadyLuckLines), gCardWep12Frame0, gCardWep12Palette, gCardWep12Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameDivineRose, gJiminyAttackCardDivineRoseLines, ARRAY_COUNT(gJiminyAttackCardDivineRoseLines), gCardWep13Frame0, gCardWep13Palette, gCardWep13Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameOathkeeper, gJiminyAttackCardOathkeeperLines, ARRAY_COUNT(gJiminyAttackCardOathkeeperLines), gCardWep14Frame0, gCardWep14Palette, gCardWep14Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameOblivion, gJiminyAttackCardOblivionLines, ARRAY_COUNT(gJiminyAttackCardOblivionLines), gCardWep15Frame0, gCardWep15Palette, gCardWep15Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameDiamondDust, gJiminyAttackCardDiamondDustLines, ARRAY_COUNT(gJiminyAttackCardDiamondDustLines), gCardWep18Frame0, gCardWep18Palette, gCardWep18Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameOneWingedAngel, gJiminyAttackCardOneWingedAngelLines, ARRAY_COUNT(gJiminyAttackCardOneWingedAngelLines), gCardWep19Frame0, gCardWep19Palette, gCardWep19Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameUltimaWeapon, gJiminyAttackCardUltimaWeaponLines, ARRAY_COUNT(gJiminyAttackCardUltimaWeaponLines), gCardWep16Frame0, gCardWep16Palette, gCardWep16Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry05Details[14] = {
    { gCardNameFire, gJiminyMagicCardFireLines, ARRAY_COUNT(gJiminyMagicCardFireLines), gCardMgc01Frame0, gCardMgc01Palette, gCardMgc01Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameBlizzard, gJiminyMagicCardBlizzardLines, ARRAY_COUNT(gJiminyMagicCardBlizzardLines), gCardMgc02Frame0, gCardMgc02Palette, gCardMgc02Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameThunder, gJiminyMagicCardThunderLines, ARRAY_COUNT(gJiminyMagicCardThunderLines), gCardMgc03Frame0, gCardMgc03Palette, gCardMgc03Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameCure, gJiminyMagicCardCureLines, ARRAY_COUNT(gJiminyMagicCardCureLines), gCardMgc04Frame0, gCardMgc04Palette, gCardMgc04Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameGravity, gJiminyMagicCardGravityLines, ARRAY_COUNT(gJiminyMagicCardGravityLines), gCardMgc05Frame0, gCardMgc05Palette, gCardMgc05Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameStop, gJiminyMagicCardStopLines, ARRAY_COUNT(gJiminyMagicCardStopLines), gCardMgc06Frame0, gCardMgc06Palette, gCardMgc06Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameAero, gJiminyMagicCardAeroLines, ARRAY_COUNT(gJiminyMagicCardAeroLines), gCardMgc07Frame0, gCardMgc07Palette, gCardMgc07Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameSimba, gJiminyMagicCardSimbaLines, ARRAY_COUNT(gJiminyMagicCardSimbaLines), gCardSmn03Frame0, gCardSmn03Palette, gCardSmn03Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameDumbo, gJiminyMagicCardDumboLines, ARRAY_COUNT(gJiminyMagicCardDumboLines), gCardSmn06Frame0, gCardSmn06Palette, gCardSmn06Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameBambi, gJiminyMagicCardBambiLines, ARRAY_COUNT(gJiminyMagicCardBambiLines), gCardSmn05Frame0, gBanbPalette, gCardSmn05Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameMushu, gJiminyMagicCardMushuLines, ARRAY_COUNT(gJiminyMagicCardMushuLines), gCardSmn08Frame0, gMushuPalette, gCardSmn08Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameGenie, gJiminyMagicCardGenieLines, ARRAY_COUNT(gJiminyMagicCardGenieLines), gCardSmn04Frame0, gCardSmn04Palette, gCardSmn04Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameTinkerBell, gJiminyMagicCardTinkerBellLines, ARRAY_COUNT(gJiminyMagicCardTinkerBellLines), gCardSmn07Frame0, gTinkPalette, gCardSmn07Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameCloud, gJiminyMagicCardCloudLines, ARRAY_COUNT(gJiminyMagicCardCloudLines), gCardSmn09Frame0, gCardSmn09Palette, gCardSmn09Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry06Details[7] = {
    { gCardNamePotion, gJiminyItemCardPotionLines, ARRAY_COUNT(gJiminyItemCardPotionLines), gCardItm01Frame0, gCardItm01Palette, gCardItm01Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameHiPotion, gJiminyItemCardHiPotionLines, ARRAY_COUNT(gJiminyItemCardHiPotionLines), gCardItm02Frame0, gCardItm02Palette, gCardItm02Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameMegaPotion, gJiminyItemCardMegaPotionLines, ARRAY_COUNT(gJiminyItemCardMegaPotionLines), gCardItm03Frame0, gCardItm03Palette, gCardItm03Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameEther, gJiminyItemCardEtherLines, ARRAY_COUNT(gJiminyItemCardEtherLines), gCardItm04Frame0, gCardItm04Palette, gCardItm04Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameMegaEther, gJiminyItemCardMegaEtherLines, ARRAY_COUNT(gJiminyItemCardMegaEtherLines), gCardItm05Frame0, gCardItm05Palette, gCardItm05Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameElixir, gJiminyItemCardElixirLines, ARRAY_COUNT(gJiminyItemCardElixirLines), gCardItm06Frame0, gCardItm06Palette, gCardItm06Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameMegalixir, gJiminyItemCardMegalixirLines, ARRAY_COUNT(gJiminyItemCardMegalixirLines), gCardItm07Frame0, gCardItm07Palette, gCardItm07Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry07Details[7] = {
    { gCardNameDonaldDuck, gJiminyFriendCardDonaldDuckLines, ARRAY_COUNT(gJiminyFriendCardDonaldDuckLines), gCardSmn02Frame0, gDonaldPalette, gCardSmn02Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameGoofy, gJiminyFriendCardGoofyLines, ARRAY_COUNT(gJiminyFriendCardGoofyLines), gCardSmn01Frame0, gGoofyPalette, gCardSmn01Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameAladdin, gJiminyFriendCardAladdinLines, ARRAY_COUNT(gJiminyFriendCardAladdinLines), gCardSmn10Frame0, gAladdinPalette, gCardSmn10Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameJack, gJiminyFriendCardJackLines, ARRAY_COUNT(gJiminyFriendCardJackLines), gCardSmn12Frame0, gJackPalette, gCardSmn12Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameAriel, gJiminyFriendCardArielLines, ARRAY_COUNT(gJiminyFriendCardArielLines), gCardSmn11Frame0, gArielPalette, gCardSmn11Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNamePeterPan, gJiminyFriendCardPeterPanLines, ARRAY_COUNT(gJiminyFriendCardPeterPanLines), gCardSmn13Frame0, gPeterPalette, gCardSmn13Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameBeast, gJiminyFriendCardBeastLines, ARRAY_COUNT(gJiminyFriendCardBeastLines), gCardSmn14Frame0, gBeastPalette, gCardSmn14Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry08Details[49] = {
    { gEnemyNameShadow, gJiminyEnemyCardShadowLines, ARRAY_COUNT(gJiminyEnemyCardShadowLines), gCardEmy00Frame0, gEmy00Palette, gCardEmy00Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameSoldier, gJiminyEnemyCardSoldierLines, ARRAY_COUNT(gJiminyEnemyCardSoldierLines), gCardEmy14Frame0, gEmy14Palette, gCardEmy14Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameLargeBody, gJiminyEnemyCardLargeBodyLines, ARRAY_COUNT(gJiminyEnemyCardLargeBodyLines), gCardEmy38Frame0, gEmy38Palette, gCardEmy38Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameRedNocturne, gJiminyEnemyCardRedNocturneLines, ARRAY_COUNT(gJiminyEnemyCardRedNocturneLines), gCardEmy01Frame0, gEmy01Palette, gCardEmy01Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameBlueRhapsody, gJiminyEnemyCardBlueRhapsodyLines, ARRAY_COUNT(gJiminyEnemyCardBlueRhapsodyLines), gBlueRhapsodyCardFrame0, gEmy02Palette, gBlueRhapsodyCardTiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameYellowOpera, gJiminyEnemyCardYellowOperaLines, ARRAY_COUNT(gJiminyEnemyCardYellowOperaLines), gYellowOperaCardFrame0, gEmy03Palette, gYellowOperaCardTiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameGreenRequiem, gJiminyEnemyCardGreenRequiemLines, ARRAY_COUNT(gJiminyEnemyCardGreenRequiemLines), gGreenRequiemCardFrame0, gEmy04Palette, gGreenRequiemCardTiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNamePowerwild, gJiminyEnemyCardPowerwildLines, ARRAY_COUNT(gJiminyEnemyCardPowerwildLines), gCardEmy15Frame0, gEmy15Palette, gCardEmy15Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameBouncywild, gJiminyEnemyCardBouncywildLines, ARRAY_COUNT(gJiminyEnemyCardBouncywildLines), gCardEmy16Frame0, gEmy16Palette, gCardEmy16Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameAirSoldier, gJiminyEnemyCardAirSoldierLines, ARRAY_COUNT(gJiminyEnemyCardAirSoldierLines), gCardEmy18Frame0, gEmy18Palette, gCardEmy18Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameBandit, gJiminyEnemyCardBanditLines, ARRAY_COUNT(gJiminyEnemyCardBanditLines), gCardEmy19Frame0, gEmy19Palette, gCardEmy19Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameFatBandit, gJiminyEnemyCardFatBanditLines, ARRAY_COUNT(gJiminyEnemyCardFatBanditLines), gCardEmy39Frame0, gEmy39Palette, gCardEmy39Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameBarrelSpider, gJiminyEnemyCardBarrelSpiderLines, ARRAY_COUNT(gJiminyEnemyCardBarrelSpiderLines), gCardEmy21Frame0, gEmy21Palette, gCardEmy21Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameSearchGhost, gJiminyEnemyCardSearchGhostLines, ARRAY_COUNT(gJiminyEnemyCardSearchGhostLines), gCardEmy22Frame0, gEmy22Palette, gCardEmy22Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameSeaNeon, gJiminyEnemyCardSeaNeonLines, ARRAY_COUNT(gJiminyEnemyCardSeaNeonLines), gCardEmy06Frame0, gEmy06Palette, gCardEmy06Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameScrewdiver, gJiminyEnemyCardScrewdiverLines, ARRAY_COUNT(gJiminyEnemyCardScrewdiverLines), gCardEmy23Frame0, gEmy23Palette, gCardEmy23Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameAquatank, gJiminyEnemyCardAquatankLines, ARRAY_COUNT(gJiminyEnemyCardAquatankLines), gCardEmy41Frame0, gEmy41Palette, gCardEmy41Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameWightKnight, gJiminyEnemyCardWightKnightLines, ARRAY_COUNT(gJiminyEnemyCardWightKnightLines), gCardEmy25Frame0, gEmy25Palette, gCardEmy25Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameGargoyle, gJiminyEnemyCardGargoyleLines, ARRAY_COUNT(gJiminyEnemyCardGargoyleLines), gCardEmy26Frame0, gEmy26Palette, gCardEmy26Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNamePirate, gJiminyEnemyCardPirateLines, ARRAY_COUNT(gJiminyEnemyCardPirateLines), gCardEmy27Frame0, gEmy27Palette, gCardEmy27Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameAirPirate, gJiminyEnemyCardAirPirateLines, ARRAY_COUNT(gJiminyEnemyCardAirPirateLines), gCardEmy28Frame0, gEmy28Palette, gCardEmy28Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameDarkball, gJiminyEnemyCardDarkballLines, ARRAY_COUNT(gJiminyEnemyCardDarkballLines), gCardEmy29Frame0, gEmy29Palette, gCardEmy29Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameDefender, gJiminyEnemyCardDefenderLines, ARRAY_COUNT(gJiminyEnemyCardDefenderLines), gCardEmy44Frame0, gEmy44Palette, gCardEmy44Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameWyvern, gJiminyEnemyCardWyvernLines, ARRAY_COUNT(gJiminyEnemyCardWyvernLines), gCardEmy30Frame0, gEmy30Palette, gCardEmy30Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameWizard, gJiminyEnemyCardWizardLines, ARRAY_COUNT(gJiminyEnemyCardWizardLines), gCardEmy31Frame0, gEmy31Palette, gCardEmy31Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameNeoshadow, gJiminyEnemyCardNeoshadowLines, ARRAY_COUNT(gJiminyEnemyCardNeoshadowLines), gCardEmy37Frame0, gEmy37Palette, gCardEmy37Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameWhiteMushroom, gJiminyEnemyCardWhiteMushroomLines, ARRAY_COUNT(gJiminyEnemyCardWhiteMushroomLines), gCardEmy07Frame0, gEmy07Palette, gCardEmy07Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameBlackFungus, gJiminyEnemyCardBlackFungusLines, ARRAY_COUNT(gJiminyEnemyCardBlackFungusLines), gCardEmy08Frame0, gEmy07bPalette, gCardEmy08Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameCreeperPlant, gJiminyEnemyCardCreeperPlantLines, ARRAY_COUNT(gJiminyEnemyCardCreeperPlantLines), gCardEmy83Frame0, gEmy83Palette, gCardEmy83Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameTornadoStep, gJiminyEnemyCardTornadoStepLines, ARRAY_COUNT(gJiminyEnemyCardTornadoStepLines), gCardEmy81Frame0, gEmy81Palette, gCardEmy81Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameCrescendo, gJiminyEnemyCardCrescendoLines, ARRAY_COUNT(gJiminyEnemyCardCrescendoLines), gCardEmy82Frame0, gEmy82Palette, gCardEmy82Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameGuardArmor, gJiminyEnemyCardGuardArmorLines, ARRAY_COUNT(gJiminyEnemyCardGuardArmorLines), gCardBos01Frame0, gBoss01objPalette, gCardBos01Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameParasiteCage, gJiminyEnemyCardParasiteCageLines, ARRAY_COUNT(gJiminyEnemyCardParasiteCageLines), gCardBos04Frame0, gBosPcBgPalettes, gCardBos04Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameTrickmaster, gJiminyEnemyCardTrickmasterLines, ARRAY_COUNT(gJiminyEnemyCardTrickmasterLines), gCardBos03Frame0, gBoss03objPalettes, gCardBos03Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameDarkside, gJiminyEnemyCardDarksideLines, ARRAY_COUNT(gJiminyEnemyCardDarksideLines), gCardBos00Frame0, gBoss00ObjPalette, gCardBos00Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameCardSoldier, gJiminyEnemyCardCardSoldierLines, ARRAY_COUNT(gJiminyEnemyCardCardSoldierLines), gCardNpcAw04Frame0, gTrumpHPalette, gCardNpcAw04Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameHades, gJiminyEnemyCardHadesLines, ARRAY_COUNT(gJiminyEnemyCardHadesLines), gCardBos11Frame0, gCardBos11Palette, gCardBos11Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameJafar, gJiminyEnemyCardJafarLines, ARRAY_COUNT(gJiminyEnemyCardJafarLines), gCardBos05Frame0, gCardBos05Palette, gCardBos05Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameOogieBoogie, gJiminyEnemyCardOogieBoogieLines, ARRAY_COUNT(gJiminyEnemyCardOogieBoogieLines), gCardBos02Frame0, gBoss02objPalette, gCardBos02Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameUrsula, gJiminyEnemyCardUrsulaLines, ARRAY_COUNT(gJiminyEnemyCardUrsulaLines), gCardBos06Frame0, gCardBos06Palette, gCardBos06Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameHook, gJiminyEnemyCardHookLines, ARRAY_COUNT(gJiminyEnemyCardHookLines), gCardBos10Frame0, gCardBos10Palette, gCardBos10Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameDragonMaleficent, gJiminyEnemyCardDragonMaleficentLines, ARRAY_COUNT(gJiminyEnemyCardDragonMaleficentLines), gCardBos07Frame0, gCardBos07Palette, gCardBos07Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameRiku, gJiminyEnemyCardRikuLines, ARRAY_COUNT(gJiminyEnemyCardRikuLines), gCardBos12Frame0, gCardBos12Palette, gCardBos12Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameAxel, gJiminyEnemyCardAxelLines, ARRAY_COUNT(gJiminyEnemyCardAxelLines), gCardBos13Frame0, gCardBos13Palette, gCardBos13Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameLarxene, gJiminyEnemyCardLarxeneLines, ARRAY_COUNT(gJiminyEnemyCardLarxeneLines), gCardBos14Frame0, gCardBos14Palette, gCardBos14Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameVexen, gJiminyEnemyCardVexenLines, ARRAY_COUNT(gJiminyEnemyCardVexenLines), gCardBos15Frame0, gCardBos15Palette, gCardBos15Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameMarluxia, gJiminyEnemyCardMarluxiaLines, ARRAY_COUNT(gJiminyEnemyCardMarluxiaLines), gCardBos16Frame0, gCardBos16Palette, gCardBos16Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameLexaeus, gJiminyEnemyCardLexaeusLines, ARRAY_COUNT(gJiminyEnemyCardLexaeusLines), gCardBos17Frame0, gCardBos17Palette, gCardBos17Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameAnsem, gJiminyEnemyCardAnsemLines, ARRAY_COUNT(gJiminyEnemyCardAnsemLines), gCardBos18Frame0, gCardBos18Palette, gCardBos18Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry09Details[26] = {
    { gRoomNameTranquilDarkness, gJiminyMapCardTranquilDarknessLines, ARRAY_COUNT(gJiminyMapCardTranquilDarknessLines), gCardRoom02Frame0, gCardRoom02Palette, gCardRoom02Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameTeemingDarkness, gJiminyMapCardTeemingDarknessLines, ARRAY_COUNT(gJiminyMapCardTeemingDarknessLines), gCardRoom01Frame0, gCardRoom01Palette, gCardRoom01Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameFeebleDarkness, gJiminyMapCardFeebleDarknessLines, ARRAY_COUNT(gJiminyMapCardFeebleDarknessLines), gCardRoom07Frame0, gCardRoom07Palette, gCardRoom07Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameAlmightyDarkness, gJiminyMapCardAlmightyDarknessLines, ARRAY_COUNT(gJiminyMapCardAlmightyDarknessLines), gCardRoom08Frame0, gCardRoom08Palette, gCardRoom08Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameSleepingDarkness, gJiminyMapCardSleepingDarknessLines, ARRAY_COUNT(gJiminyMapCardSleepingDarknessLines), gCardRoom05Frame0, gCardRoom05Palette, gCardRoom05Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameLoomingDarkness, gJiminyMapCardLoomingDarknessLines, ARRAY_COUNT(gJiminyMapCardLoomingDarknessLines), gCardRoom04Frame0, gCardRoom04Palette, gCardRoom04Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNamePremiumRoom, gJiminyMapCardPremiumRoomLines, ARRAY_COUNT(gJiminyMapCardPremiumRoomLines), gCardRoom20Frame0, gCardRoom20Palette, gCardRoom20Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameWhiteRoom, gJiminyMapCardWhiteRoomLines, ARRAY_COUNT(gJiminyMapCardWhiteRoomLines), gCardRoom21Frame0, gCardRoom21Palette, gCardRoom21Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameBlackRoom, gJiminyMapCardBlackRoomLines, ARRAY_COUNT(gJiminyMapCardBlackRoomLines), gCardRoom22Frame0, gCardRoom22Palette, gCardRoom22Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameMartialWaking, gJiminyMapCardMartialWakingLines, ARRAY_COUNT(gJiminyMapCardMartialWakingLines), gCardRoom13Frame0, gCardRoom13Palette, gCardRoom13Tiles, gMapCardBorderGreenFrame0, gMapCardBorderPalettes, gMapCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameSorcerousWaking, gJiminyMapCardSorcerousWakingLines, ARRAY_COUNT(gJiminyMapCardSorcerousWakingLines), gCardRoom12Frame0, gCardRoom12Palette, gCardRoom12Tiles, gMapCardBorderGreenFrame0, gMapCardBorderPalettes, gMapCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameAlchemicWaking, gJiminyMapCardAlchemicWakingLines, ARRAY_COUNT(gJiminyMapCardAlchemicWakingLines), gCardRoom14Frame0, gCardRoom14Palette, gCardRoom14Tiles, gMapCardBorderGreenFrame0, gMapCardBorderPalettes, gMapCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameMeetingGround, gJiminyMapCardMeetingGroundLines, ARRAY_COUNT(gJiminyMapCardMeetingGroundLines), gCardRoom15Frame0, gCardRoom15Palette, gCardRoom15Tiles, gMapCardBorderGreenFrame0, gMapCardBorderPalettes, gMapCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameStagnantSpace, gJiminyMapCardStagnantSpaceLines, ARRAY_COUNT(gJiminyMapCardStagnantSpaceLines), gCardRoom19Frame0, gCardRoom19Palette, gCardRoom19Tiles, gMapCardBorderGreenFrame0, gMapCardBorderPalettes, gMapCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameStrongInitiative, gJiminyMapCardStrongInitiativeLines, ARRAY_COUNT(gJiminyMapCardStrongInitiativeLines), gCardRoom17Frame0, gCardRoom17Palette, gCardRoom17Tiles, gMapCardBorderGreenFrame0, gMapCardBorderPalettes, gMapCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameLastingDaze, gJiminyMapCardLastingDazeLines, ARRAY_COUNT(gJiminyMapCardLastingDazeLines), gCardRoom18Frame0, gCardRoom18Palette, gCardRoom18Tiles, gMapCardBorderGreenFrame0, gMapCardBorderPalettes, gMapCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameCalmBounty, gJiminyMapCardCalmBountyLines, ARRAY_COUNT(gJiminyMapCardCalmBountyLines), gCardRoom09Frame0, gCardRoom09Palette, gCardRoom09Tiles, gMapCardBorderBlueFrame0, gMapCardBorderPalettes, gMapCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameGuardedTrove, gJiminyMapCardGuardedTroveLines, ARRAY_COUNT(gJiminyMapCardGuardedTroveLines), gCardRoom03Frame0, gCardRoom03Palette, gCardRoom03Tiles, gMapCardBorderBlueFrame0, gMapCardBorderPalettes, gMapCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameFalseBounty, gJiminyMapCardFalseBountyLines, ARRAY_COUNT(gJiminyMapCardFalseBountyLines), gCardRoom10Frame0, gCardRoom10Palette, gCardRoom10Tiles, gMapCardBorderBlueFrame0, gMapCardBorderPalettes, gMapCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameMomentsReprieve, gJiminyMapCardMomentsReprieveLines, ARRAY_COUNT(gJiminyMapCardMomentsReprieveLines), gCardRoom06Frame0, gCardRoom06Palette, gCardRoom06Tiles, gMapCardBorderBlueFrame0, gMapCardBorderPalettes, gMapCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameMinglingWorlds, gJiminyMapCardMinglingWorldsLines, ARRAY_COUNT(gJiminyMapCardMinglingWorldsLines), gCardRoom16Frame0, gCardRoom16Palette, gCardRoom16Tiles, gMapCardBorderBlueFrame0, gMapCardBorderPalettes, gMapCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameMoogleRoom, gJiminyMapCardMoogleRoomLines, ARRAY_COUNT(gJiminyMapCardMoogleRoomLines), gCardRoom11Frame0, gCardRoom11Palette, gCardRoom11Tiles, gMapCardBorderBlueFrame0, gMapCardBorderPalettes, gMapCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameKeyOfBeginnings, gJiminyMapCardKeyOfBeginningsLines, ARRAY_COUNT(gJiminyMapCardKeyOfBeginningsLines), gCardEve00Frame0, gCardEve00Palette, gCardEve00Tiles, gMapCardBorderBlackFrame0, gMapCardBorderPalettes, gMapCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameKeyOfGuidance, gJiminyMapCardKeyOfGuidanceLines, ARRAY_COUNT(gJiminyMapCardKeyOfGuidanceLines), gCardEve01Frame0, gCardEve01Palette, gCardEve01Tiles, gMapCardBorderBlackFrame0, gMapCardBorderPalettes, gMapCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameKeyToTruth, gJiminyMapCardKeyToTruthLines, ARRAY_COUNT(gJiminyMapCardKeyToTruthLines), gCardEve02Frame0, gCardEve02Palette, gCardEve02Tiles, gMapCardBorderBlackFrame0, gMapCardBorderPalettes, gMapCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameKeyToRewards, gJiminyMapCardKeyToRewardsLines, ARRAY_COUNT(gJiminyMapCardKeyToRewardsLines), gCardRoom23Frame0, gCardRoom23Palette, gCardRoom23Tiles, gMapCardBorderBlackFrame0, gMapCardBorderPalettes, gMapCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry10Details[1] = {
    { gJiminyPremiumCardsName, gJiminyPremiumCardsLines, ARRAY_COUNT(gJiminyPremiumCardsLines), gJiminyPremiumCardFrame0, gJiminyPremiumCardPalette, gJiminyPremiumCardTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry16Details[22] = {
    { gCardNameSoulEater, gJiminyRikuCardSoulEaterLines, ARRAY_COUNT(gJiminyRikuCardSoulEaterLines), gCardWep20Frame0, gCardWep20Palette, gCardWep20Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameKing, gJiminyRikuCardKingLines, ARRAY_COUNT(gJiminyRikuCardKingLines), gCardSmn15Frame0, gMickeyPalette, gCardSmn15Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameShadow, gJiminyEnemyCardShadowLines, ARRAY_COUNT(gJiminyEnemyCardShadowLines), gCardEmy00Frame0, gEmy00Palette, gCardEmy00Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameLargeBody, gJiminyEnemyCardLargeBodyLines, ARRAY_COUNT(gJiminyEnemyCardLargeBodyLines), gCardEmy38Frame0, gEmy38Palette, gCardEmy38Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNamePowerwild, gJiminyEnemyCardPowerwildLines, ARRAY_COUNT(gJiminyEnemyCardPowerwildLines), gCardEmy15Frame0, gEmy15Palette, gCardEmy15Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameFatBandit, gJiminyEnemyCardFatBanditLines, ARRAY_COUNT(gJiminyEnemyCardFatBanditLines), gCardEmy39Frame0, gEmy39Palette, gCardEmy39Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameSearchGhost, gJiminyEnemyCardSearchGhostLines, ARRAY_COUNT(gJiminyEnemyCardSearchGhostLines), gCardEmy22Frame0, gEmy22Palette, gCardEmy22Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameSeaNeon, gJiminyEnemyCardSeaNeonLines, ARRAY_COUNT(gJiminyEnemyCardSeaNeonLines), gCardEmy06Frame0, gEmy06Palette, gCardEmy06Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameWightKnight, gJiminyEnemyCardWightKnightLines, ARRAY_COUNT(gJiminyEnemyCardWightKnightLines), gCardEmy25Frame0, gEmy25Palette, gCardEmy25Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNamePirate, gJiminyEnemyCardPirateLines, ARRAY_COUNT(gJiminyEnemyCardPirateLines), gCardEmy27Frame0, gEmy27Palette, gCardEmy27Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameDefender, gJiminyEnemyCardDefenderLines, ARRAY_COUNT(gJiminyEnemyCardDefenderLines), gCardEmy44Frame0, gEmy44Palette, gCardEmy44Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameGuardArmor, gJiminyEnemyCardGuardArmorLines, ARRAY_COUNT(gJiminyEnemyCardGuardArmorLines), gCardBos01Frame0, gBoss01objPalette, gCardBos01Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameParasiteCage, gJiminyEnemyCardParasiteCageLines, ARRAY_COUNT(gJiminyEnemyCardParasiteCageLines), gCardBos04Frame0, gBosPcBgPalettes, gCardBos04Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameTrickmaster, gJiminyEnemyCardTrickmasterLines, ARRAY_COUNT(gJiminyEnemyCardTrickmasterLines), gCardBos03Frame0, gBoss03objPalettes, gCardBos03Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameDarkside, gJiminyEnemyCardDarksideLines, ARRAY_COUNT(gJiminyEnemyCardDarksideLines), gCardBos00Frame0, gBoss00ObjPalette, gCardBos00Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameHades, gJiminyEnemyCardHadesLines, ARRAY_COUNT(gJiminyEnemyCardHadesLines), gCardBos11Frame0, gCardBos11Palette, gCardBos11Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameJafar, gJiminyEnemyCardJafarLines, ARRAY_COUNT(gJiminyEnemyCardJafarLines), gCardBos05Frame0, gCardBos05Palette, gCardBos05Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameOogieBoogie, gJiminyEnemyCardOogieBoogieLines, ARRAY_COUNT(gJiminyEnemyCardOogieBoogieLines), gCardBos02Frame0, gBoss02objPalette, gCardBos02Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameUrsula, gJiminyEnemyCardUrsulaLines, ARRAY_COUNT(gJiminyEnemyCardUrsulaLines), gCardBos06Frame0, gCardBos06Palette, gCardBos06Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameHook, gJiminyEnemyCardHookLines, ARRAY_COUNT(gJiminyEnemyCardHookLines), gCardBos10Frame0, gCardBos10Palette, gCardBos10Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameDragonMaleficent, gJiminyEnemyCardDragonMaleficentLines, ARRAY_COUNT(gJiminyEnemyCardDragonMaleficentLines), gCardBos07Frame0, gCardBos07Palette, gCardBos07Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameLexaeus, gJiminyEnemyCardLexaeusLines, ARRAY_COUNT(gJiminyEnemyCardLexaeusLines), gCardBos17Frame0, gCardBos17Palette, gCardBos17Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry11Details[25] = {
    { gCharacterNameSora, gJiminyCharacterSoraLines, ARRAY_COUNT(gJiminyCharacterSoraLines), gSor1fl00Frame0, gSoraPalette, gSor1fl00Tiles, gCardNpcEx00Frame0, gCardNpcEx00Palette, gCardNpcEx00Tiles, NULL, NULL, NULL, 0, 0, -3, -3 },
    { gCardNameDonaldDuck, gJiminyCharacterDonaldDuckLines, ARRAY_COUNT(gJiminyCharacterDonaldDuckLines), gDonaFl00Frame0, gDonaldPalette, gDonaFl00Tiles, gCardSmn02Frame0, gDonaldPalette, gCardSmn02Tiles, NULL, NULL, NULL, 0, 0, 0, -9 },
    { gCardNameGoofy, gJiminyCharacterGoofyLines, ARRAY_COUNT(gJiminyCharacterGoofyLines), gGoofyFl00Frame0, gGoofyPalette, gGoofyFl00Tiles, gCardSmn01Frame0, gGoofyPalette, gCardSmn01Tiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCharacterNameJiminyCricket, gJiminyCharacterJiminyCricketLines, ARRAY_COUNT(gJiminyCharacterJiminyCricketLines), gJimFl00Frame0, gJiminyPalette, gJimFl00Tiles, gCardNpcEx01Frame0, gCardNpcEx01Palette, gCardNpcEx01Tiles, NULL, NULL, NULL, 0, 0, -3, -11 },
    { gEnemyNameRiku, gJiminyCharacterRikuLines, ARRAY_COUNT(gJiminyCharacterRikuLines), gRikuFl00Frame0, gRikuPalette, gRikuFl00Tiles, gCardNpcEx03Frame0, gCardNpcEx03Palette, gCardNpcEx03Tiles, NULL, NULL, NULL, 0, 0, -3, 0 },
    { gCharacterNameKairi, gJiminyCharacterKairiLines, ARRAY_COUNT(gJiminyCharacterKairiLines), gKairF00Frame0, gKairiPalette, gKairF00Tiles, gCardNpcEx05Frame0, gCardNpcEx05Palette, gCardNpcEx05Tiles, NULL, NULL, NULL, 0, 0, -1, -3 },
    { gCardNameSimba, gJiminyCharacterSimbaLines, ARRAY_COUNT(gJiminyCharacterSimbaLines), gShinba10Frame0, gShinbaPalette, gShinba10Tiles, gCardSmn03Frame0, gCardSmn03Palette, gCardSmn03Tiles, NULL, NULL, NULL, 0, 0, -2, -7 },
    { gCardNameDumbo, gJiminyCharacterDumboLines, ARRAY_COUNT(gJiminyCharacterDumboLines), gSmnDumboFrame0, gDamboPalette, gSmnDumboTiles, gCardSmn06Frame0, gCardSmn06Palette, gCardSmn06Tiles, NULL, NULL, NULL, 0, 0, 8, -7 },
    { gCardNameBambi, gJiminyCharacterBambiLines, ARRAY_COUNT(gJiminyCharacterBambiLines), gBanb00Frame7, gBanbPalette, gBanb00Tiles, gCardSmn05Frame0, gBanbPalette, gCardSmn05Tiles, NULL, NULL, NULL, 0, 0, -2, -7 },
    { gCardNameMushu, gJiminyCharacterMushuLines, ARRAY_COUNT(gJiminyCharacterMushuLines), gMushuF00Frame1, gMushuPalette, gMushuF00Tiles, gCardSmn08Frame0, gMushuPalette, gCardSmn08Tiles, NULL, NULL, NULL, 0, 0, 0, -12 },
    { gCharacterNameMoogles, gJiminyCharacterMooglesLines, ARRAY_COUNT(gJiminyCharacterMooglesLines), gMoguFl00Frame0, gMoguPalette, gMoguFl00Tiles, gCardNpcEx02Frame0, gCardNpcEx02Palette, gCardNpcEx02Tiles, NULL, NULL, NULL, 0, 0, -2, -8 },
    { gCharacterNameLeon, gJiminyCharacterLeonLines, ARRAY_COUNT(gJiminyCharacterLeonLines), gReonFl00Frame0, gReonPalette, gReonFl00Tiles, gCardNpcEx09Frame0, gCardNpcEx09Palette, gCardNpcEx09Tiles, NULL, NULL, NULL, 0, 0, -3, 7 },
    { gCharacterNameYuffie, gJiminyCharacterYuffieLines, ARRAY_COUNT(gJiminyCharacterYuffieLines), gYuffieFl00Frame0, gYuffiePalette, gYuffieFl00Tiles, gCardNpcEx10Frame0, gCardNpcEx10Palette, gCardNpcEx10Tiles, NULL, NULL, NULL, 0, 0, -1, 2 },
    { gCharacterNameAerith, gJiminyCharacterAerithLines, ARRAY_COUNT(gJiminyCharacterAerithLines), gEarF00Frame0, gEarisPalette, gEarF00Tiles, gCardNpcEx06Frame0, gCardNpcEx06Palette, gCardNpcEx06Tiles, NULL, NULL, NULL, 0, 0, -3, 4 },
    { gCharacterNameCid, gJiminyCharacterCidLines, ARRAY_COUNT(gJiminyCharacterCidLines), gShidoF00Frame0, gShidoPalette, gShidoF00Tiles, gCardNpcEx07Frame0, gCardNpcEx07Palette, gCardNpcEx07Tiles, NULL, NULL, NULL, 0, 0, -2, 5 },
    { gCardNameCloud, gJiminyCharacterCloudLines, ARRAY_COUNT(gJiminyCharacterCloudLines), gCroudF00Frame0, gCroudPalette, gCroudF00Tiles, gCardSmn09Frame0, gCardSmn09Palette, gCardSmn09Tiles, NULL, NULL, NULL, 0, 0, -3, 2 },
    { gCharacterNameTidus, gJiminyCharacterTidusLines, ARRAY_COUNT(gJiminyCharacterTidusLines), gTidusFl00Frame0, gTidusPalette, gTidusFl00Tiles, gCardNpcDi01Frame0, gCardNpcDi01Palette, gCardNpcDi01Tiles, NULL, NULL, NULL, 0, 0, -2, -3 },
    { gCharacterNameWakka, gJiminyCharacterWakkaLines, ARRAY_COUNT(gJiminyCharacterWakkaLines), gWakkaF00Frame0, gWakkaPalette, gWakkaF00Tiles, gCardNpcDi02Frame0, gCardNpcDi02Palette, gCardNpcDi02Tiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCharacterNameSelphie, gJiminyCharacterSelphieLines, ARRAY_COUNT(gJiminyCharacterSelphieLines), gSelphieFl00Frame0, gSelphiePalette, gSelphieFl00Tiles, gCardNpcDi03Frame0, gCardNpcDi03Palette, gCardNpcDi03Tiles, NULL, NULL, NULL, 0, 0, -2, -3 },
    { gCharacterNameNamine, gJiminyCharacterNamineLines, ARRAY_COUNT(gJiminyCharacterNamineLines), gNamiF00Frame0, gNaminePalette, gNamiF00Tiles, gCardNpcCom01Frame0, gCardNpcCom01Palette, gCardNpcCom01Tiles, NULL, NULL, NULL, 0, 0, -1, -4 },
    { gCharacterNameRikuReplica, gJiminyCharacterRikuReplicaLines, ARRAY_COUNT(gJiminyCharacterRikuReplicaLines), gNiseFl00Frame0, gNiserikuPalette, gNiseFl00Tiles, gCardBos12Frame0, gCardBos12Palette, gCardBos12Tiles, NULL, NULL, NULL, 0, 0, -3, 0 },
    { gEnemyNameAxel, gJiminyCharacterAxelLines, ARRAY_COUNT(gJiminyCharacterAxelLines), gAcceleFl00Frame0, gAccelePalette, gAcceleFl00Tiles, gCardBos13Frame0, gCardBos13Palette, gCardBos13Tiles, NULL, NULL, NULL, 0, 0, -3, 6 },
    { gEnemyNameLarxene, gJiminyCharacterLarxeneLines, ARRAY_COUNT(gJiminyCharacterLarxeneLines), gLaxineFl00Frame0, gLaxinePalette, gLaxineFl00Tiles, gCardBos14Frame0, gCardBos14Palette, gCardBos14Tiles, NULL, NULL, NULL, 0, 0, -2, 5 },
    { gEnemyNameVexen, gJiminyCharacterVexenLines, ARRAY_COUNT(gJiminyCharacterVexenLines), gVixenFl00Frame0, gVixenPalette, gVixenFl00Tiles, gCardBos15Frame0, gCardBos15Palette, gCardBos15Tiles, NULL, NULL, NULL, 0, 0, -3, 6 },
    { gEnemyNameMarluxia, gJiminyCharacterMarluxiaLines, ARRAY_COUNT(gJiminyCharacterMarluxiaLines), gMaruxha2Fl00Frame0, gMaruxhaPalette, gMaruxha2Fl00Tiles, gCardBos16Frame0, gCardBos16Palette, gCardBos16Tiles, NULL, NULL, NULL, 0, 0, -3, 6 },
};

static const JiminyDetail sJiminyEntry12Details[40] = {
    { gCharacterNameAlice, gJiminyCharacterAliceLines, ARRAY_COUNT(gJiminyCharacterAliceLines), gAliceFl00Frame0, gAlicePalette, gAliceFl00Tiles, gCardNpcAw01Frame0, gCardNpcAw01Palette, gCardNpcAw01Tiles, NULL, NULL, NULL, 0, 0, -1, -1 },
    { gCharacterNameQueenOfHearts, gJiminyCharacterQueenOfHeartsLines, ARRAY_COUNT(gJiminyCharacterQueenOfHeartsLines), gQenF00Frame0, gQeenPalette, gQenF00Tiles, gCardNpcAw02Frame0, gCardNpcAw02Palette, gCardNpcAw02Tiles, NULL, NULL, NULL, 0, 0, -3, 4 },
    { gCharacterNameWhiteRabbit, gJiminyCharacterWhiteRabbitLines, ARRAY_COUNT(gJiminyCharacterWhiteRabbitLines), gUsagif00Frame0, gUsagi00Palette, gUsagif00Tiles, gCardNpcAw05Frame0, gCardNpcAw05Palette, gCardNpcAw05Tiles, NULL, NULL, NULL, 0, 0, 0, -10 },
    { gCharacterNameCardOfHearts, gJiminyCharacterCardOfHeartsLines, ARRAY_COUNT(gJiminyCharacterCardOfHeartsLines), gTrumpH00Frame0, gTrumpHPalette, gTrumpH00Tiles, gCardNpcAw04Frame0, gTrumpHPalette, gCardNpcAw04Tiles, NULL, NULL, NULL, 0, 0, 0, 2 },
    { gCharacterNameCardOfSpades, gJiminyCharacterCardOfSpadesLines, ARRAY_COUNT(gJiminyCharacterCardOfSpadesLines), gTrumpS00Frame0, gTrumpSPalette, gTrumpS00Tiles, gCardNpcAw03Frame0, gTrumpSPalette, gCardNpcAw03Tiles, NULL, NULL, NULL, 0, 0, 0, 2 },
    { gCharacterNameCheshireCat, gJiminyCharacterCheshireCatLines, ARRAY_COUNT(gJiminyCharacterCheshireCatLines), gCheshireFrame0, gCheshirePalette, gCheshireTiles, gCardNpcAw06Frame0, gCardNpcAw06Palette, gCardNpcAw06Tiles, NULL, NULL, NULL, 0, 0, -4, -12 },
    { gCharacterNameHercules, gJiminyCharacterHerculesLines, ARRAY_COUNT(gJiminyCharacterHerculesLines), gHeraclesFl00Frame0, gHeraclesPalette, gHeraclesFl00Tiles, gCardNpcHe02Frame0, gCardNpcHe02Palette, gCardNpcHe02Tiles, NULL, NULL, NULL, 0, 0, 0, 13 },
    { gCharacterNamePhiloctetes, gJiminyCharacterPhiloctetesLines, ARRAY_COUNT(gJiminyCharacterPhiloctetesLines), gPhilFl00Frame0, gPhilPalette, gPhilFl00Tiles, gCardNpcHe01Frame0, gCardNpcHe01Palette, gCardNpcHe01Tiles, NULL, NULL, NULL, 0, 0, -2, -10 },
    { gEnemyNameHades, gJiminyCharacterHadesLines, ARRAY_COUNT(gJiminyCharacterHadesLines), gHadesFl00Frame0, gHadesPalette, gHadesFl00Tiles, gCardBos11Frame0, gCardBos11Palette, gCardBos11Tiles, NULL, NULL, NULL, 0, 0, 0, 18 },
    { gCardNameAladdin, gJiminyCharacterAladdinLines, ARRAY_COUNT(gJiminyCharacterAladdinLines), gAladdin00Frame0, gAladdinPalette, gAladdin00Tiles, gCardSmn10Frame0, gAladdinPalette, gCardSmn10Tiles, NULL, NULL, NULL, 0, 0, -2, 6 },
    { gCardNameGenie, gJiminyCharacterGenieLines, ARRAY_COUNT(gJiminyCharacterGenieLines), gGenie01Frame2, gGeniePalette, gGenie01Tiles, gCardSmn04Frame0, gCardSmn04Palette, gCardSmn04Tiles, NULL, NULL, NULL, 0, 0, 2, 1 },
    { gCharacterNameJasmine, gJiminyCharacterJasmineLines, ARRAY_COUNT(gJiminyCharacterJasmineLines), gJasmineF00Frame0, gJasminePalette, gJasmineF00Tiles, gCardNpcAl01Frame0, gCardNpcAl01Palette, gCardNpcAl01Tiles, NULL, NULL, NULL, 0, 0, -4, 5 },
    { gCharacterNameIago, gJiminyCharacterIagoLines, ARRAY_COUNT(gJiminyCharacterIagoLines), gIagoFrame0, gIagoPalette, gIagoTiles, gCardNpcAl03Frame0, gCardNpcAl03Palette, gCardNpcAl03Tiles, NULL, NULL, NULL, 0, 0, -2, -10 },
    { gEnemyNameJafar, gJiminyCharacterJafarLines, ARRAY_COUNT(gJiminyCharacterJafarLines), gJafferFl00Frame0, gJafferPalette, gJafferFl00Tiles, gCardNpcAl02Frame0, gCardNpcAl02Palette, gCardNpcAl02Tiles, NULL, NULL, NULL, 0, 0, -3, 19 },
    { gCharacterNameJafarGenie, gJiminyCharacterJafarGenieLines, ARRAY_COUNT(gJiminyCharacterJafarGenieLines), NULL, NULL, NULL, gCardBos05Frame0, gCardBos05Palette, gCardBos05Tiles, gJiminyJafarGenieBgMap, gJiminyJafarGenieBgPalette, gJiminyJafarGenieBgTiles, sizeof(gJiminyJafarGenieBgPalette), sizeof(gJiminyJafarGenieBgTiles), 0, 0 },
    { gCardNameJack, gJiminyCharacterJackLines, ARRAY_COUNT(gJiminyCharacterJackLines), gJackFl00Frame0, gJackPalette, gJackFl00Tiles, gCardSmn12Frame0, gJackPalette, gCardSmn12Tiles, NULL, NULL, NULL, 0, 0, 2, 11 },
    { gCharacterNameSally, gJiminyCharacterSallyLines, ARRAY_COUNT(gJiminyCharacterSallyLines), gSariFl00Frame0, gSariPalette, gSariFl00Tiles, gCardNpcNm01Frame0, gCardNpcNm01Palette, gCardNpcNm01Tiles, NULL, NULL, NULL, 0, 0, -2, 7 },
    { gCharacterNameDrFinkelstein, gJiminyCharacterDrFinkelsteinLines, ARRAY_COUNT(gJiminyCharacterDrFinkelsteinLines), gFinklF00Frame0, gFinklPalette, gFinklF00Tiles, gCardNpcNm02Frame0, gCardNpcNm02Palette, gCardNpcNm02Tiles, NULL, NULL, NULL, 0, 0, -1, -7 },
    { gEnemyNameOogieBoogie, gJiminyCharacterOogieBoogieLines, ARRAY_COUNT(gJiminyCharacterOogieBoogieLines), gBugiFl00Frame0, gBugiPalette, gBugiFl00Tiles, gCardBos02Frame0, gBoss02objPalette, gCardBos02Tiles, NULL, NULL, NULL, 0, 0, -4, 9 },
    { gCharacterNamePinocchio, gJiminyCharacterPinocchioLines, ARRAY_COUNT(gJiminyCharacterPinocchioLines), gPinoF00Frame0, gPinokioPalette, gPinoF00Tiles, gCardNpcPi01Frame0, gCardNpcPi01Palette, gCardNpcPi01Tiles, NULL, NULL, NULL, 0, 0, -2, -8 },
    { gCharacterNameGeppetto, gJiminyCharacterGeppettoLines, ARRAY_COUNT(gJiminyCharacterGeppettoLines), gGeppettoF00Frame0, gGeppettoPalette, gGeppettoF00Tiles, gCardNpcPi02Frame0, gCardNpcPi02Palette, gCardNpcPi02Tiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameAriel, gJiminyCharacterArielLines, ARRAY_COUNT(gJiminyCharacterArielLines), gArielF00Frame0, gArielPalette, gArielF00Tiles, gCardSmn11Frame0, gArielPalette, gCardSmn11Tiles, NULL, NULL, NULL, 0, 0, -4, 3 },
    { gCharacterNameSebastian, gJiminyCharacterSebastianLines, ARRAY_COUNT(gJiminyCharacterSebastianLines), gSebastianFl00Frame0, gSebastianPalette, gSebastianFl00Tiles, gCardNpcLm01Frame0, gCardNpcLm01Palette, gCardNpcLm01Tiles, NULL, NULL, NULL, 0, 0, -1, -13 },
    { gCharacterNameFlounder, gJiminyCharacterFlounderLines, ARRAY_COUNT(gJiminyCharacterFlounderLines), gFlounderFl00Frame0, gFlounderPalette, gFlounderFl00Tiles, gCardNpcLm02Frame0, gCardNpcLm02Palette, gCardNpcLm02Tiles, NULL, NULL, NULL, 0, 0, -1, -17 },
    { gEnemyNameUrsula, gJiminyCharacterUrsulaLines, ARRAY_COUNT(gJiminyCharacterUrsulaLines), gArthraFl00Frame0, gArthraPalette, gArthraFl00Tiles, gCardBos06Frame0, gCardBos06Palette, gCardBos06Tiles, NULL, NULL, NULL, 0, 0, -3, 6 },
    { gCardNamePeterPan, gJiminyCharacterPeterPanLines, ARRAY_COUNT(gJiminyCharacterPeterPanLines), gPeterFl00Frame0, gPeterPalette, gPeterFl00Tiles, gCardSmn13Frame0, gPeterPalette, gCardSmn13Tiles, NULL, NULL, NULL, 0, 0, -3, 5 },
    { gCardNameTinkerBell, gJiminyCharacterTinkerBellLines, ARRAY_COUNT(gJiminyCharacterTinkerBellLines), gTinkF00Frame2, gTinkPalette, gTinkF00Tiles, gCardSmn07Frame0, gTinkPalette, gCardSmn07Tiles, NULL, NULL, NULL, 0, 0, -5, -10 },
    { gCharacterNameWendy, gJiminyCharacterWendyLines, ARRAY_COUNT(gJiminyCharacterWendyLines), gWendyFl00Frame0, gWendyPalette, gWendyFl00Tiles, gCardNpcPp01Frame0, gCardNpcPp01Palette, gCardNpcPp01Tiles, NULL, NULL, NULL, 0, 0, -1, 4 },
    { gCharacterNameHook, gJiminyCharacterHookLines, ARRAY_COUNT(gJiminyCharacterHookLines), gHookF00Frame0, gHookPalette, gHookF00Tiles, gCardBos10Frame0, gCardBos10Palette, gCardBos10Tiles, NULL, NULL, NULL, 0, 0, -1, 17 },
    { gCardNameBeast, gJiminyCharacterBeastLines, ARRAY_COUNT(gJiminyCharacterBeastLines), gBeastFl00Frame0, gBeastPalette, gBeastFl00Tiles, gCardSmn14Frame0, gBeastPalette, gCardSmn14Tiles, NULL, NULL, NULL, 0, 0, -1, 10 },
    { gCharacterNameBelle, gJiminyCharacterBelleLines, ARRAY_COUNT(gJiminyCharacterBelleLines), gBellFl00Frame0, gBellPalette, gBellFl00Tiles, gCardNpcPc01Frame0, gCardNpcPc01Palette, gCardNpcPc01Tiles, NULL, NULL, NULL, 0, 0, -4, 5 },
    { gCharacterNameMaleficent, gJiminyCharacterMaleficentLines, ARRAY_COUNT(gJiminyCharacterMaleficentLines), gMarefF00Frame0, gMarefPalette, gMarefF00Tiles, gCardNpcPc02Frame0, gCardNpcPc02Palette, gCardNpcPc02Tiles, NULL, NULL, NULL, 0, 0, -12, 16 },
    { gEnemyNameDragonMaleficent, gJiminyCharacterDragonMaleficentLines, ARRAY_COUNT(gJiminyCharacterDragonMaleficentLines), NULL, NULL, NULL, gCardBos07Frame0, gCardBos07Palette, gCardBos07Tiles, gJiminyDragonMaleficentBgMap, gJiminyDragonMaleficentBgPalette, gJiminyDragonMaleficentBgTiles, sizeof(gJiminyDragonMaleficentBgPalette), sizeof(gJiminyDragonMaleficentBgTiles), 0, 0 },
    { gCharacterNameWinnieThePooh, gJiminyCharacterWinnieThePoohLines, ARRAY_COUNT(gJiminyCharacterWinnieThePoohLines), gPoohFl06Frame0, gPoohPalette, gPoohFl06Tiles, gCardNpcPo01Frame0, gCardNpcPo01Palette, gCardNpcPo01Tiles, NULL, NULL, NULL, 0, 0, -2, -6 },
    { gCharacterNamePiglet, gJiminyCharacterPigletLines, ARRAY_COUNT(gJiminyCharacterPigletLines), gPigletStandFrontFrame0, gPigletPalette, gPigletStandFrontTiles, gCardNpcPo03Frame0, gCardNpcPo03Palette, gCardNpcPo03Tiles, NULL, NULL, NULL, 0, 0, -2, -10 },
    { gCharacterNameOwl, gJiminyCharacterOwlLines, ARRAY_COUNT(gJiminyCharacterOwlLines), gOwlFl00Frame0, gOwlPalette, gOwlFl00Tiles, gCardNpcPo06Frame0, gCardNpcPo06Palette, gCardNpcPo06Tiles, NULL, NULL, NULL, 0, 0, -3, -7 },
    { gCharacterNameRoo, gJiminyCharacterRooLines, ARRAY_COUNT(gJiminyCharacterRooLines), gRooFl00Frame0, gRooPalette, gRooFl00Tiles, gCardNpcPo07Frame0, gCardNpcPo07Palette, gCardNpcPo07Tiles, NULL, NULL, NULL, 0, 0, -1, -13 },
    { gCharacterNameEeyore, gJiminyCharacterEeyoreLines, ARRAY_COUNT(gJiminyCharacterEeyoreLines), gEeyoreFl00Frame0, gEeyorePalette, gEeyoreFl00Tiles, gCardNpcPo04Frame0, gCardNpcPo04Palette, gCardNpcPo04Tiles, NULL, NULL, NULL, 0, 0, 5, -13 },
    { gCharacterNameTigger, gJiminyCharacterTiggerLines, ARRAY_COUNT(gJiminyCharacterTiggerLines), gTiggerFl00Frame0, gTiggerPalette, gTiggerFl00Tiles, gCardNpcPo02Frame0, gCardNpcPo02Palette, gCardNpcPo02Tiles, NULL, NULL, NULL, 0, 0, -3, -3 },
    { gCharacterNameRabbit, gJiminyCharacterRabbitLines, ARRAY_COUNT(gJiminyCharacterRabbitLines), gRabbitFl00Frame0, gRabbitPalettes, gRabbitFl00Tiles, gCardNpcPo05Frame0, gCardNpcPo05Palette, gCardNpcPo05Tiles, NULL, NULL, NULL, 0, 0, -1, -3 },
};

static const JiminyDetail sJiminyEntry13Details[35] = {
    { gEnemyNameShadow, gJiminyHeartlessShadowLines, ARRAY_COUNT(gJiminyHeartlessShadowLines), gEmy00L00Frame0, gEmy00Palette, gEmy00L00Tiles, gCardEmy00Frame0, gEmy00Palette, gCardEmy00Tiles, NULL, NULL, NULL, 0, 0, 2, -12 },
    { gEnemyNameSoldier, gJiminyHeartlessSoldierLines, ARRAY_COUNT(gJiminyHeartlessSoldierLines), gEmy14Ll01Frame1, gEmy14Palette, gEmy14Ll01Tiles, gCardEmy14Frame0, gEmy14Palette, gCardEmy14Tiles, NULL, NULL, NULL, 0, 0, -8, 2 },
    { gEnemyNameLargeBody, gJiminyHeartlessLargeBodyLines, ARRAY_COUNT(gJiminyHeartlessLargeBodyLines), gEmy3800Frame0, gEmy38Palette, gEmy3800Tiles, gCardEmy38Frame0, gEmy38Palette, gCardEmy38Tiles, NULL, NULL, NULL, 0, 0, -6, 6 },
    { gEnemyNameRedNocturne, gJiminyHeartlessRedNocturneLines, ARRAY_COUNT(gJiminyHeartlessRedNocturneLines), gEmy01L00Frame16, gEmy01Palette, gEmy01L00Tiles, gCardEmy01Frame0, gEmy01Palette, gCardEmy01Tiles, NULL, NULL, NULL, 0, 0, -2, -8 },
    { gEnemyNameBlueRhapsody, gJiminyHeartlessBlueRhapsodyLines, ARRAY_COUNT(gJiminyHeartlessBlueRhapsodyLines), gEmy01L00Frame3, gEmy02Palette, gEmy01L00Tiles, gCardEmy01Frame0, gEmy02Palette, gCardEmy01Tiles, NULL, NULL, NULL, 0, 0, -1, -11 },
    { gEnemyNameYellowOpera, gJiminyHeartlessYellowOperaLines, ARRAY_COUNT(gJiminyHeartlessYellowOperaLines), gEmy01L00Frame0, gEmy03Palette, gEmy01L00Tiles, gCardEmy01Frame0, gEmy03Palette, gCardEmy01Tiles, NULL, NULL, NULL, 0, 0, -2, -11 },
    { gEnemyNameGreenRequiem, gJiminyHeartlessGreenRequiemLines, ARRAY_COUNT(gJiminyHeartlessGreenRequiemLines), gEmy01L00Frame15, gEmy04Palette, gEmy01L00Tiles, gCardEmy01Frame0, gEmy04Palette, gCardEmy01Tiles, NULL, NULL, NULL, 0, 0, -2, -8 },
    { gEnemyNamePowerwild, gJiminyHeartlessPowerwildLines, ARRAY_COUNT(gJiminyHeartlessPowerwildLines), gEmy1500Frame7, gEmy15Palette, gEmy1500Tiles, gCardEmy15Frame0, gEmy15Palette, gCardEmy15Tiles, NULL, NULL, NULL, 0, 0, 0, -6 },
    { gEnemyNameBouncywild, gJiminyHeartlessBouncywildLines, ARRAY_COUNT(gJiminyHeartlessBouncywildLines), gEmy1600Frame0, gEmy16Palette, gEmy1600Tiles, gCardEmy16Frame0, gEmy16Palette, gCardEmy16Tiles, NULL, NULL, NULL, 0, 0, -1, -9 },
    { gEnemyNameAirSoldier, gJiminyHeartlessAirSoldierLines, ARRAY_COUNT(gJiminyHeartlessAirSoldierLines), gEmy1800Frame0, gEmy18Palette, gEmy1800Tiles, gCardEmy18Frame0, gEmy18Palette, gCardEmy18Tiles, NULL, NULL, NULL, 0, 0, -1, 3 },
    { gEnemyNameBandit, gJiminyHeartlessBanditLines, ARRAY_COUNT(gJiminyHeartlessBanditLines), gEmy1900Frame5, gEmy19Palette, gEmy1900Tiles, gCardEmy19Frame0, gEmy19Palette, gCardEmy19Tiles, NULL, NULL, NULL, 0, 0, -1, -6 },
    { gEnemyNameFatBandit, gJiminyHeartlessFatBanditLines, ARRAY_COUNT(gJiminyHeartlessFatBanditLines), gEmy3900Frame0, gEmy39Palette, gEmy3900Tiles, gCardEmy39Frame0, gEmy39Palette, gCardEmy39Tiles, NULL, NULL, NULL, 0, 0, -1, 4 },
    { gEnemyNameBarrelSpider, gJiminyHeartlessBarrelSpiderLines, ARRAY_COUNT(gJiminyHeartlessBarrelSpiderLines), gEmy2100Frame0, gEmy21Palette, gEmy2100Tiles, gCardEmy21Frame0, gEmy21Palette, gCardEmy21Tiles, NULL, NULL, NULL, 0, 0, 1, -10 },
    { gEnemyNameSearchGhost, gJiminyHeartlessSearchGhostLines, ARRAY_COUNT(gJiminyHeartlessSearchGhostLines), gEmy2200Frame9, gEmy22Palette, gEmy2200Tiles, gCardEmy22Frame0, gEmy22Palette, gCardEmy22Tiles, NULL, NULL, NULL, 0, 0, 2, 6 },
    { gEnemyNameSeaNeon, gJiminyHeartlessSeaNeonLines, ARRAY_COUNT(gJiminyHeartlessSeaNeonLines), gEmy0600Frame2, gEmy06Palette, gEmy0600Tiles, gCardEmy06Frame0, gEmy06Palette, gCardEmy06Tiles, NULL, NULL, NULL, 0, 0, -2, -3 },
    { gEnemyNameScrewdiver, gJiminyHeartlessScrewdiverLines, ARRAY_COUNT(gJiminyHeartlessScrewdiverLines), gEmy2300Frame0, gEmy23Palette, gEmy2300Tiles, gCardEmy23Frame0, gEmy23Palette, gCardEmy23Tiles, NULL, NULL, NULL, 0, 0, 1, 2 },
    { gEnemyNameAquatank, gJiminyHeartlessAquatankLines, ARRAY_COUNT(gJiminyHeartlessAquatankLines), gEmy4100Frame0, gEmy41Palette, gEmy4100Tiles, gCardEmy41Frame0, gEmy41Palette, gCardEmy41Tiles, NULL, NULL, NULL, 0, 0, -2, 11 },
    { gEnemyNameWightKnight, gJiminyHeartlessWightKnightLines, ARRAY_COUNT(gJiminyHeartlessWightKnightLines), gEmy2500Frame3, gEmy25Palette, gEmy2500Tiles, gCardEmy25Frame0, gEmy25Palette, gCardEmy25Tiles, NULL, NULL, NULL, 0, 0, 4, 0 },
    { gEnemyNameGargoyle, gJiminyHeartlessGargoyleLines, ARRAY_COUNT(gJiminyHeartlessGargoyleLines), gEmy2600Frame0, gEmy26Palette, gEmy2600Tiles, gCardEmy26Frame0, gEmy26Palette, gCardEmy26Tiles, NULL, NULL, NULL, 0, 0, -4, 9 },
    { gEnemyNamePirate, gJiminyHeartlessPirateLines, ARRAY_COUNT(gJiminyHeartlessPirateLines), gEmy2700Frame0, gEmy27Palette, gEmy2700Tiles, gCardEmy27Frame0, gEmy27Palette, gCardEmy27Tiles, NULL, NULL, NULL, 0, 0, 10, -10 },
    { gEnemyNameAirPirate, gJiminyHeartlessAirPirateLines, ARRAY_COUNT(gJiminyHeartlessAirPirateLines), gEmy2800Frame0, gEmy28Palette, gEmy2800Tiles, gCardEmy28Frame0, gEmy28Palette, gCardEmy28Tiles, NULL, NULL, NULL, 0, 0, 5, 5 },
    { gEnemyNameDarkball, gJiminyHeartlessDarkballLines, ARRAY_COUNT(gJiminyHeartlessDarkballLines), gEmy2900Frame0, gEmy29Palette, gEmy2900Tiles, gCardEmy29Frame0, gEmy29Palette, gCardEmy29Tiles, NULL, NULL, NULL, 0, 0, -3, 7 },
    { gEnemyNameDefender, gJiminyHeartlessDefenderLines, ARRAY_COUNT(gJiminyHeartlessDefenderLines), gEmy4400Frame0, gEmy44Palette, gEmy4400Tiles, gCardEmy44Frame0, gEmy44Palette, gCardEmy44Tiles, NULL, NULL, NULL, 0, 0, 3, 1 },
    { gEnemyNameWyvern, gJiminyHeartlessWyvernLines, ARRAY_COUNT(gJiminyHeartlessWyvernLines), gEmy3000Frame0, gEmy30Palette, gEmy3000Tiles, gCardEmy30Frame0, gEmy30Palette, gCardEmy30Tiles, NULL, NULL, NULL, 0, 0, 2, -5 },
    { gEnemyNameWizard, gJiminyHeartlessWizardLines, ARRAY_COUNT(gJiminyHeartlessWizardLines), gEmy3100Frame6, gEmy31Palette, gEmy3100Tiles, gCardEmy31Frame0, gEmy31Palette, gCardEmy31Tiles, NULL, NULL, NULL, 0, 0, 5, -3 },
    { gEnemyNameNeoshadow, gJiminyHeartlessNeoshadowLines, ARRAY_COUNT(gJiminyHeartlessNeoshadowLines), gEmy3700Frame1, gEmy37Palette, gEmy3700Tiles, gCardEmy37Frame0, gEmy37Palette, gCardEmy37Tiles, NULL, NULL, NULL, 0, 0, -1, 4 },
    { gEnemyNameWhiteMushroom, gJiminyHeartlessWhiteMushroomLines, ARRAY_COUNT(gJiminyHeartlessWhiteMushroomLines), gEmy07Fl00Frame0, gEmy07Palette, gEmy07Fl00Tiles, gCardEmy07Frame0, gEmy07Palette, gCardEmy07Tiles, NULL, NULL, NULL, 0, 0, -2, -6 },
    { gEnemyNameBlackFungus, gJiminyHeartlessBlackFungusLines, ARRAY_COUNT(gJiminyHeartlessBlackFungusLines), gEmy07Fl10tFrame7, gEmy07bPalette, gEmy07Fl10tTiles, gCardEmy08Frame0, gEmy07bPalette, gCardEmy08Tiles, NULL, NULL, NULL, 0, 0, -2, -6 },
    { gEnemyNameCreeperPlant, gJiminyHeartlessCreeperPlantLines, ARRAY_COUNT(gJiminyHeartlessCreeperPlantLines), gEmy8300Frame0, gEmy83Palette, gEmy8300Tiles, gCardEmy83Frame0, gEmy83Palette, gCardEmy83Tiles, NULL, NULL, NULL, 0, 0, 0, -4 },
    { gEnemyNameTornadoStep, gJiminyHeartlessTornadoStepLines, ARRAY_COUNT(gJiminyHeartlessTornadoStepLines), gEmy8100Frame0, gEmy81Palette, gEmy8100Tiles, gCardEmy81Frame0, gEmy81Palette, gCardEmy81Tiles, NULL, NULL, NULL, 0, 0, -2, -3 },
    { gEnemyNameCrescendo, gJiminyHeartlessCrescendoLines, ARRAY_COUNT(gJiminyHeartlessCrescendoLines), gEmy8201Frame2, gEmy82Palette, gEmy8201Tiles, gCardEmy82Frame0, gEmy82Palette, gCardEmy82Tiles, NULL, NULL, NULL, 0, 0, 4, -10 },
    { gEnemyNameGuardArmor, gJiminyHeartlessGuardArmorLines, ARRAY_COUNT(gJiminyHeartlessGuardArmorLines), NULL, NULL, NULL, gCardBos01Frame0, gBoss01objPalette, gCardBos01Tiles, gJiminyGuardArmorBgMap, gJiminyGuardArmorBgPalette, gJiminyGuardArmorBgTiles, sizeof(gJiminyGuardArmorBgPalette), sizeof(gJiminyGuardArmorBgTiles), 3, -3 },
    { gEnemyNameParasiteCage, gJiminyHeartlessParasiteCageLines, ARRAY_COUNT(gJiminyHeartlessParasiteCageLines), NULL, NULL, NULL, gCardBos04Frame0, gBosPcBgPalettes, gCardBos04Tiles, gJiminyParasiteCageBgMap, gJiminyParasiteCageBgPalette, gJiminyParasiteCageBgTiles, sizeof(gJiminyParasiteCageBgPalette), sizeof(gJiminyParasiteCageBgTiles), 0, 0 },
    { gEnemyNameTrickmaster, gJiminyHeartlessTrickmasterLines, ARRAY_COUNT(gJiminyHeartlessTrickmasterLines), NULL, NULL, NULL, gCardBos03Frame0, gBoss03objPalettes, gCardBos03Tiles, gJiminyTrickmasterBgMap, gJiminyTrickmasterBgPalette, gJiminyTrickmasterBgTiles, sizeof(gJiminyTrickmasterBgPalette), sizeof(gJiminyTrickmasterBgTiles), 0, 3 },
    { gEnemyNameDarkside, gJiminyHeartlessDarksideLines, ARRAY_COUNT(gJiminyHeartlessDarksideLines), NULL, NULL, NULL, gCardBos00Frame0, gBoss00ObjPalette, gCardBos00Tiles, gJiminyDarksideBgMap, gJiminyDarksideBgPalette, gJiminyDarksideBgTiles, sizeof(gJiminyDarksideBgPalette), sizeof(gJiminyDarksideBgTiles), 0, 0 },
};

static const JiminyDetail sJiminyEntry18Details[14] = {
    { gEnemyNameRiku, gJiminyRikuCharacterRikuLines, ARRAY_COUNT(gJiminyRikuCharacterRikuLines), gRikuFl00Frame0, gRikuPalette, gRikuFl00Tiles, gCardNpcEx03Frame0, gCardNpcEx03Palette, gCardNpcEx03Tiles, NULL, NULL, NULL, 0, 0, -3, 0 },
    { gCardNameKing, gJiminyRikuCharacterKingLines, ARRAY_COUNT(gJiminyRikuCharacterKingLines), gMickeyFl00Frame0, gMickeyPalette, gMickeyFl00Tiles, gCardSmn15Frame0, gMickeyPalette, gCardSmn15Tiles, NULL, NULL, NULL, 0, 0, -2, -6 },
    { gCharacterNameSora, gJiminyRikuCharacterSoraLines, ARRAY_COUNT(gJiminyRikuCharacterSoraLines), gSor1fl00Frame0, gSoraPalette, gSor1fl00Tiles, gCardNpcEx00Frame0, gCardNpcEx00Palette, gCardNpcEx00Tiles, NULL, NULL, NULL, 0, 0, -3, -3 },
    { gCharacterNameKairi, gJiminyRikuCharacterKairiLines, ARRAY_COUNT(gJiminyRikuCharacterKairiLines), gKairF00Frame0, gKairiPalette, gKairF00Tiles, gCardNpcEx05Frame0, gCardNpcEx05Palette, gCardNpcEx05Tiles, NULL, NULL, NULL, 0, 0, -1, -3 },
    { gCharacterNameNamine, gJiminyRikuCharacterNamineLines, ARRAY_COUNT(gJiminyRikuCharacterNamineLines), gNamiF00Frame0, gNaminePalette, gNamiF00Tiles, gCardNpcCom01Frame0, gCardNpcCom01Palette, gCardNpcCom01Tiles, NULL, NULL, NULL, 0, 0, -1, -4 },
    { gCharacterNameRikuReplica, gJiminyRikuCharacterRikuReplicaLines, ARRAY_COUNT(gJiminyRikuCharacterRikuReplicaLines), gNiseFl00Frame0, gNiserikuPalette, gNiseFl00Tiles, gCardBos12Frame0, gCardBos12Palette, gCardBos12Tiles, NULL, NULL, NULL, 0, 0, -3, 0 },
    { gEnemyNameAnsem, gJiminyRikuCharacterAnsemLines, ARRAY_COUNT(gJiminyRikuCharacterAnsemLines), gAnsemFl00Frame0, gAnsemPalette, gAnsemFl00Tiles, gCardBos18Frame0, gCardBos18Palette, gCardBos18Tiles, NULL, NULL, NULL, 0, 0, -3, 9 },
    { gEnemyNameVexen, gJiminyRikuCharacterVexenLines, ARRAY_COUNT(gJiminyRikuCharacterVexenLines), gVixenFl00Frame0, gVixenPalette, gVixenFl00Tiles, gCardBos15Frame0, gCardBos15Palette, gCardBos15Tiles, NULL, NULL, NULL, 0, 0, -3, 6 },
    { gEnemyNameLexaeus, gJiminyRikuCharacterLexaeusLines, ARRAY_COUNT(gJiminyRikuCharacterLexaeusLines), gRexeusFl00Frame0, gRexeusPalette, gRexeusFl00Tiles, gCardBos17Frame0, gCardBos17Palette, gCardBos17Tiles, NULL, NULL, NULL, 0, 0, -5, 14 },
    { gCharacterNameZexion, gJiminyRikuCharacterZexionLines, ARRAY_COUNT(gJiminyRikuCharacterZexionLines), gXexionFl00Frame0, gXexionPalette, gXexionFl00Tiles, gCardNpcCom02Frame0, gCardNpcCom02Palette, gCardNpcCom02Tiles, NULL, NULL, NULL, 0, 0, -1, 6 },
    { gEnemyNameAxel, gJiminyRikuCharacterAxelLines, ARRAY_COUNT(gJiminyRikuCharacterAxelLines), gAcceleFl00Frame0, gAccelePalette, gAcceleFl00Tiles, gCardBos13Frame0, gCardBos13Palette, gCardBos13Tiles, NULL, NULL, NULL, 0, 0, -3, 6 },
    { gEnemyNameMarluxia, gJiminyRikuCharacterMarluxiaLines, ARRAY_COUNT(gJiminyRikuCharacterMarluxiaLines), gMaruxha2Fl00Frame0, gMaruxhaPalette, gMaruxha2Fl00Tiles, gCardBos16Frame0, gCardBos16Palette, gCardBos16Tiles, NULL, NULL, NULL, 0, 0, -3, 6 },
    { gEnemyNameLarxene, gJiminyRikuCharacterLarxeneLines, ARRAY_COUNT(gJiminyRikuCharacterLarxeneLines), gLaxineFl00Frame0, gLaxinePalette, gLaxineFl00Tiles, gCardBos14Frame0, gCardBos14Palette, gCardBos14Tiles, NULL, NULL, NULL, 0, 0, -2, 5 },
    { gCharacterNameDiZ, gJiminyRikuCharacterDiZLines, ARRAY_COUNT(gJiminyRikuCharacterDiZLines), gDizFl00Frame0, gDizPalette, gDizFl00Tiles, gCardNpcCom03Frame0, gCardNpcCom03Palette, gCardNpcCom03Tiles, NULL, NULL, NULL, 0, 0, -2, 8 },
};

static const JiminyDetail sJiminyEntry19Details[6] = {
    { gCharacterNameMaleficent, gJiminyRikuCharacterMaleficentLines, ARRAY_COUNT(gJiminyRikuCharacterMaleficentLines), gMarefF00Frame0, gMarefPalette, gMarefF00Tiles, gCardNpcPc02Frame0, gCardNpcPc02Palette, gCardNpcPc02Tiles, NULL, NULL, NULL, 0, 0, -12, 16 },
    { gCharacterNameJafarGenie, gJiminyRikuCharacterJafarGenieLines, ARRAY_COUNT(gJiminyRikuCharacterJafarGenieLines), NULL, NULL, NULL, gCardBos05Frame0, gCardBos05Palette, gCardBos05Tiles, gJiminyJafarGenieBgMap, gJiminyJafarGenieBgPalette, gJiminyJafarGenieBgTiles, sizeof(gJiminyJafarGenieBgPalette), sizeof(gJiminyJafarGenieBgTiles), 0, 0 },
    { gEnemyNameUrsula, gJiminyRikuCharacterUrsulaLines, ARRAY_COUNT(gJiminyRikuCharacterUrsulaLines), gArthraFl00Frame0, gArthraPalette, gArthraFl00Tiles, gCardBos06Frame0, gCardBos06Palette, gCardBos06Tiles, NULL, NULL, NULL, 0, 0, -3, 6 },
    { gEnemyNameHades, gJiminyRikuCharacterHadesLines, ARRAY_COUNT(gJiminyRikuCharacterHadesLines), gHadesFl00Frame0, gHadesPalette, gHadesFl00Tiles, gCardBos11Frame0, gCardBos11Palette, gCardBos11Tiles, NULL, NULL, NULL, 0, 0, 0, 18 },
    { gEnemyNameOogieBoogie, gJiminyRikuCharacterOogieBoogieLines, ARRAY_COUNT(gJiminyRikuCharacterOogieBoogieLines), gBugiFl00Frame0, gBugiPalette, gBugiFl00Tiles, gCardBos02Frame0, gBoss02objPalette, gCardBos02Tiles, NULL, NULL, NULL, 0, 0, -4, 9 },
    { gCharacterNameHook, gJiminyRikuCharacterHookLines, ARRAY_COUNT(gJiminyRikuCharacterHookLines), gHookF00Frame0, gHookPalette, gHookF00Tiles, gCardBos10Frame0, gCardBos10Palette, gCardBos10Tiles, NULL, NULL, NULL, 0, 0, -1, 17 },
};

static const JiminyDetail sJiminyEntry20Details[33] = {
    { gEnemyNameShadow, gJiminyHeartlessShadowLines, ARRAY_COUNT(gJiminyHeartlessShadowLines), gEmy00L00Frame0, gEmy00Palette, gEmy00L00Tiles, gCardEmy00Frame0, gEmy00Palette, gCardEmy00Tiles, NULL, NULL, NULL, 0, 0, 2, -12 },
    { gEnemyNameSoldier, gJiminyHeartlessSoldierLines, ARRAY_COUNT(gJiminyHeartlessSoldierLines), gEmy14Ll01Frame1, gEmy14Palette, gEmy14Ll01Tiles, gCardEmy14Frame0, gEmy14Palette, gCardEmy14Tiles, NULL, NULL, NULL, 0, 0, -8, 2 },
    { gEnemyNameLargeBody, gJiminyHeartlessLargeBodyLines, ARRAY_COUNT(gJiminyHeartlessLargeBodyLines), gEmy3800Frame0, gEmy38Palette, gEmy3800Tiles, gCardEmy38Frame0, gEmy38Palette, gCardEmy38Tiles, NULL, NULL, NULL, 0, 0, -6, 6 },
    { gEnemyNameRedNocturne, gJiminyHeartlessRedNocturneLines, ARRAY_COUNT(gJiminyHeartlessRedNocturneLines), gEmy01L00Frame16, gEmy01Palette, gEmy01L00Tiles, gCardEmy01Frame0, gEmy01Palette, gCardEmy01Tiles, NULL, NULL, NULL, 0, 0, -2, -8 },
    { gEnemyNameBlueRhapsody, gJiminyHeartlessBlueRhapsodyLines, ARRAY_COUNT(gJiminyHeartlessBlueRhapsodyLines), gEmy01L00Frame3, gEmy02Palette, gEmy01L00Tiles, gCardEmy01Frame0, gEmy02Palette, gCardEmy01Tiles, NULL, NULL, NULL, 0, 0, -1, -11 },
    { gEnemyNameYellowOpera, gJiminyHeartlessYellowOperaLines, ARRAY_COUNT(gJiminyHeartlessYellowOperaLines), gEmy01L00Frame0, gEmy03Palette, gEmy01L00Tiles, gCardEmy01Frame0, gEmy03Palette, gCardEmy01Tiles, NULL, NULL, NULL, 0, 0, -2, -11 },
    { gEnemyNameGreenRequiem, gJiminyHeartlessGreenRequiemLines, ARRAY_COUNT(gJiminyHeartlessGreenRequiemLines), gEmy01L00Frame15, gEmy04Palette, gEmy01L00Tiles, gCardEmy01Frame0, gEmy04Palette, gCardEmy01Tiles, NULL, NULL, NULL, 0, 0, -2, -8 },
    { gEnemyNamePowerwild, gJiminyHeartlessPowerwildLines, ARRAY_COUNT(gJiminyHeartlessPowerwildLines), gEmy1500Frame7, gEmy15Palette, gEmy1500Tiles, gCardEmy15Frame0, gEmy15Palette, gCardEmy15Tiles, NULL, NULL, NULL, 0, 0, 0, -6 },
    { gEnemyNameBouncywild, gJiminyHeartlessBouncywildLines, ARRAY_COUNT(gJiminyHeartlessBouncywildLines), gEmy1600Frame0, gEmy16Palette, gEmy1600Tiles, gCardEmy16Frame0, gEmy16Palette, gCardEmy16Tiles, NULL, NULL, NULL, 0, 0, -1, -9 },
    { gEnemyNameAirSoldier, gJiminyHeartlessAirSoldierLines, ARRAY_COUNT(gJiminyHeartlessAirSoldierLines), gEmy1800Frame0, gEmy18Palette, gEmy1800Tiles, gCardEmy18Frame0, gEmy18Palette, gCardEmy18Tiles, NULL, NULL, NULL, 0, 0, -1, 3 },
    { gEnemyNameBandit, gJiminyHeartlessBanditLines, ARRAY_COUNT(gJiminyHeartlessBanditLines), gEmy1900Frame5, gEmy19Palette, gEmy1900Tiles, gCardEmy19Frame0, gEmy19Palette, gCardEmy19Tiles, NULL, NULL, NULL, 0, 0, -1, -6 },
    { gEnemyNameFatBandit, gJiminyHeartlessFatBanditLines, ARRAY_COUNT(gJiminyHeartlessFatBanditLines), gEmy3900Frame0, gEmy39Palette, gEmy3900Tiles, gCardEmy39Frame0, gEmy39Palette, gCardEmy39Tiles, NULL, NULL, NULL, 0, 0, -1, 4 },
    { gEnemyNameBarrelSpider, gJiminyHeartlessBarrelSpiderLines, ARRAY_COUNT(gJiminyHeartlessBarrelSpiderLines), gEmy2100Frame0, gEmy21Palette, gEmy2100Tiles, gCardEmy21Frame0, gEmy21Palette, gCardEmy21Tiles, NULL, NULL, NULL, 0, 0, 1, -10 },
    { gEnemyNameSearchGhost, gJiminyHeartlessSearchGhostLines, ARRAY_COUNT(gJiminyHeartlessSearchGhostLines), gEmy2200Frame9, gEmy22Palette, gEmy2200Tiles, gCardEmy22Frame0, gEmy22Palette, gCardEmy22Tiles, NULL, NULL, NULL, 0, 0, 2, 6 },
    { gEnemyNameSeaNeon, gJiminyHeartlessSeaNeonLines, ARRAY_COUNT(gJiminyHeartlessSeaNeonLines), gEmy0600Frame2, gEmy06Palette, gEmy0600Tiles, gCardEmy06Frame0, gEmy06Palette, gCardEmy06Tiles, NULL, NULL, NULL, 0, 0, -2, -3 },
    { gEnemyNameScrewdiver, gJiminyHeartlessScrewdiverLines, ARRAY_COUNT(gJiminyHeartlessScrewdiverLines), gEmy2300Frame0, gEmy23Palette, gEmy2300Tiles, gCardEmy23Frame0, gEmy23Palette, gCardEmy23Tiles, NULL, NULL, NULL, 0, 0, 1, 2 },
    { gEnemyNameAquatank, gJiminyHeartlessAquatankLines, ARRAY_COUNT(gJiminyHeartlessAquatankLines), gEmy4100Frame0, gEmy41Palette, gEmy4100Tiles, gCardEmy41Frame0, gEmy41Palette, gCardEmy41Tiles, NULL, NULL, NULL, 0, 0, -2, 11 },
    { gEnemyNameWightKnight, gJiminyHeartlessWightKnightLines, ARRAY_COUNT(gJiminyHeartlessWightKnightLines), gEmy2500Frame3, gEmy25Palette, gEmy2500Tiles, gCardEmy25Frame0, gEmy25Palette, gCardEmy25Tiles, NULL, NULL, NULL, 0, 0, 4, 0 },
    { gEnemyNameGargoyle, gJiminyHeartlessGargoyleLines, ARRAY_COUNT(gJiminyHeartlessGargoyleLines), gEmy2600Frame0, gEmy26Palette, gEmy2600Tiles, gCardEmy26Frame0, gEmy26Palette, gCardEmy26Tiles, NULL, NULL, NULL, 0, 0, -4, 9 },
    { gEnemyNamePirate, gJiminyHeartlessPirateLines, ARRAY_COUNT(gJiminyHeartlessPirateLines), gEmy2700Frame0, gEmy27Palette, gEmy2700Tiles, gCardEmy27Frame0, gEmy27Palette, gCardEmy27Tiles, NULL, NULL, NULL, 0, 0, 10, -10 },
    { gEnemyNameAirPirate, gJiminyHeartlessAirPirateLines, ARRAY_COUNT(gJiminyHeartlessAirPirateLines), gEmy2800Frame0, gEmy28Palette, gEmy2800Tiles, gCardEmy28Frame0, gEmy28Palette, gCardEmy28Tiles, NULL, NULL, NULL, 0, 0, 5, 5 },
    { gEnemyNameDarkball, gJiminyHeartlessDarkballLines, ARRAY_COUNT(gJiminyHeartlessDarkballLines), gEmy2900Frame0, gEmy29Palette, gEmy2900Tiles, gCardEmy29Frame0, gEmy29Palette, gCardEmy29Tiles, NULL, NULL, NULL, 0, 0, -3, 7 },
    { gEnemyNameDefender, gJiminyHeartlessDefenderLines, ARRAY_COUNT(gJiminyHeartlessDefenderLines), gEmy4400Frame0, gEmy44Palette, gEmy4400Tiles, gCardEmy44Frame0, gEmy44Palette, gCardEmy44Tiles, NULL, NULL, NULL, 0, 0, 3, 1 },
    { gEnemyNameWyvern, gJiminyHeartlessWyvernLines, ARRAY_COUNT(gJiminyHeartlessWyvernLines), gEmy3000Frame0, gEmy30Palette, gEmy3000Tiles, gCardEmy30Frame0, gEmy30Palette, gCardEmy30Tiles, NULL, NULL, NULL, 0, 0, 2, -5 },
    { gEnemyNameWizard, gJiminyHeartlessWizardLines, ARRAY_COUNT(gJiminyHeartlessWizardLines), gEmy3100Frame6, gEmy31Palette, gEmy3100Tiles, gCardEmy31Frame0, gEmy31Palette, gCardEmy31Tiles, NULL, NULL, NULL, 0, 0, 5, -3 },
    { gEnemyNameNeoshadow, gJiminyHeartlessNeoshadowLines, ARRAY_COUNT(gJiminyHeartlessNeoshadowLines), gEmy3700Frame1, gEmy37Palette, gEmy3700Tiles, gCardEmy37Frame0, gEmy37Palette, gCardEmy37Tiles, NULL, NULL, NULL, 0, 0, -1, 4 },
    { gEnemyNameCreeperPlant, gJiminyHeartlessCreeperPlantLines, ARRAY_COUNT(gJiminyHeartlessCreeperPlantLines), gEmy8300Frame0, gEmy83Palette, gEmy8300Tiles, gCardEmy83Frame0, gEmy83Palette, gCardEmy83Tiles, NULL, NULL, NULL, 0, 0, 0, -4 },
    { gEnemyNameTornadoStep, gJiminyHeartlessTornadoStepLines, ARRAY_COUNT(gJiminyHeartlessTornadoStepLines), gEmy8100Frame0, gEmy81Palette, gEmy8100Tiles, gCardEmy81Frame0, gEmy81Palette, gCardEmy81Tiles, NULL, NULL, NULL, 0, 0, -2, -3 },
    { gEnemyNameCrescendo, gJiminyHeartlessCrescendoLines, ARRAY_COUNT(gJiminyHeartlessCrescendoLines), gEmy8201Frame2, gEmy82Palette, gEmy8201Tiles, gCardEmy82Frame0, gEmy82Palette, gCardEmy82Tiles, NULL, NULL, NULL, 0, 0, 4, -10 },
    { gEnemyNameGuardArmor, gJiminyRikuHeartlessGuardArmorLines, ARRAY_COUNT(gJiminyRikuHeartlessGuardArmorLines), NULL, NULL, NULL, gCardBos01Frame0, gBoss01objPalette, gCardBos01Tiles, gJiminyGuardArmorBgMap, gJiminyGuardArmorBgPalette, gJiminyGuardArmorBgTiles, sizeof(gJiminyGuardArmorBgPalette), sizeof(gJiminyGuardArmorBgTiles), 3, -3 },
    { gEnemyNameParasiteCage, gJiminyRikuHeartlessParasiteCageLines, ARRAY_COUNT(gJiminyRikuHeartlessParasiteCageLines), NULL, NULL, NULL, gCardBos04Frame0, gBosPcBgPalettes, gCardBos04Tiles, gJiminyParasiteCageBgMap, gJiminyParasiteCageBgPalette, gJiminyParasiteCageBgTiles, sizeof(gJiminyParasiteCageBgPalette), sizeof(gJiminyParasiteCageBgTiles), 0, 0 },
    { gEnemyNameTrickmaster, gJiminyRikuHeartlessTrickmasterLines, ARRAY_COUNT(gJiminyRikuHeartlessTrickmasterLines), NULL, NULL, NULL, gCardBos03Frame0, gBoss03objPalettes, gCardBos03Tiles, gJiminyTrickmasterBgMap, gJiminyTrickmasterBgPalette, gJiminyTrickmasterBgTiles, sizeof(gJiminyTrickmasterBgPalette), sizeof(gJiminyTrickmasterBgTiles), 0, 3 },
    { gEnemyNameDarkside, gJiminyRikuHeartlessDarksideLines, ARRAY_COUNT(gJiminyRikuHeartlessDarksideLines), NULL, NULL, NULL, gCardBos00Frame0, gBoss00ObjPalette, gCardBos00Tiles, gJiminyDarksideBgMap, gJiminyDarksideBgPalette, gJiminyDarksideBgTiles, sizeof(gJiminyDarksideBgPalette), sizeof(gJiminyDarksideBgTiles), 0, 0 },
};

#elif defined(VERSION_JP)

static const JiminyDetail sJiminyEntry01Details[17] = {
    { gJiminyStoryTale1Name, gJiminyStoryTale1Lines, ARRAY_COUNT(gJiminyStoryTale1Lines), gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gJiminyStoryTale2Name, gJiminyStoryTale2Lines, ARRAY_COUNT(gJiminyStoryTale2Lines), gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gJiminyStoryTale3Name, gJiminyStoryTale3Lines, ARRAY_COUNT(gJiminyStoryTale3Lines), gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gJiminyStoryTale4Name, gJiminyStoryTale4Lines, ARRAY_COUNT(gJiminyStoryTale4Lines), gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gWorldNameTraverseTown, gJiminyStoryTraverseTownLines, ARRAY_COUNT(gJiminyStoryTraverseTownLines), gWorldImageTraverseTownFrame0, gWorldImageTraverseTownPalette, gWorldImageTraverseTownTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gWorldNameWonderland, gJiminyStoryWonderlandLines, ARRAY_COUNT(gJiminyStoryWonderlandLines), gWorldImageWonderlandFrame0, gWorldImageWonderlandPalette, gWorldImageWonderlandTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gWorldNameOlympusColiseum, gJiminyStoryOlympusColiseumLines, ARRAY_COUNT(gJiminyStoryOlympusColiseumLines), gWorldImageOlympusColiseumFrame0, gWorldImageOlympusColiseumPalette, gWorldImageOlympusColiseumTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gWorldNameAgrabah, gJiminyStoryAgrabahLines, ARRAY_COUNT(gJiminyStoryAgrabahLines), gWorldImageAgrabahFrame0, gWorldImageAgrabahPalette, gWorldImageAgrabahTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gWorldNameHalloweenTown, gJiminyStoryHalloweenTownLines, ARRAY_COUNT(gJiminyStoryHalloweenTownLines), gWorldImageHalloweenTownFrame0, gWorldImageHalloweenTownPalette, gWorldImageHalloweenTownTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gWorldNameMonstro, gJiminyStoryMonstroLines, ARRAY_COUNT(gJiminyStoryMonstroLines), gWorldImageMonstroFrame0, gWorldImageMonstroPalette, gWorldImageMonstroTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, -8, 0 },
    { gWorldNameAtlantica, gJiminyStoryAtlanticaLines, ARRAY_COUNT(gJiminyStoryAtlanticaLines), gWorldImageAtlanticaFrame0, gWorldImageAtlanticaPalette, gWorldImageAtlanticaTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gWorldNameNeverLand, gJiminyStoryNeverLandLines, ARRAY_COUNT(gJiminyStoryNeverLandLines), gWorldImageNeverLandFrame0, gWorldImageNeverLandPalette, gWorldImageNeverLandTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gWorldNameHollowBastion, gJiminyStoryHollowBastionLines, ARRAY_COUNT(gJiminyStoryHollowBastionLines), gWorldImageHollowBastionFrame0, gWorldImageHollowBastionPalette, gWorldImageHollowBastionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gWorldName100AcreWood, gJiminyStory100AcreWoodLines, ARRAY_COUNT(gJiminyStory100AcreWoodLines), gWorldImage100AcreWoodFrame0, gWorldImage100AcreWoodPalette, gWorldImage100AcreWoodTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gWorldNameTwilightTown, gJiminyStoryTwilightTownLines, ARRAY_COUNT(gJiminyStoryTwilightTownLines), gWorldImageTwilightTownFrame0, gWorldImageTwilightTownPalette, gWorldImageTwilightTownTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gWorldNameDestinyIslands, gJiminyStoryDestinyIslandsLines, ARRAY_COUNT(gJiminyStoryDestinyIslandsLines), gWorldImageDestinyIslandsFrame0, gWorldImageDestinyIslandsPalette, gWorldImageDestinyIslandsTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gWorldNameCastleOblivion, gJiminyStoryCastleOblivionLines, ARRAY_COUNT(gJiminyStoryCastleOblivionLines), gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry15Details[6] = {
    { gJiminyRikuStoryTale1Name, gJiminyRikuStoryTale1Lines, ARRAY_COUNT(gJiminyRikuStoryTale1Lines), gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gJiminyRikuStoryTale2Name, gJiminyRikuStoryTale2Lines, ARRAY_COUNT(gJiminyRikuStoryTale2Lines), gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gJiminyRikuStoryTale3Name, gJiminyRikuStoryTale3Lines, ARRAY_COUNT(gJiminyRikuStoryTale3Lines), gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gJiminyRikuStoryTale4Name, gJiminyRikuStoryTale4Lines, ARRAY_COUNT(gJiminyRikuStoryTale4Lines), gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gJiminyRikuStoryTale5Name, gJiminyRikuStoryTale5Lines, ARRAY_COUNT(gJiminyRikuStoryTale5Lines), gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gJiminyRikuStoryTale6Name, gJiminyRikuStoryTale6Lines, ARRAY_COUNT(gJiminyRikuStoryTale6Lines), gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry04Details[17] = {
    { gCardNameKingdomKey, gJiminyAttackCardKingdomKeyLines, ARRAY_COUNT(gJiminyAttackCardKingdomKeyLines), gCardWep01Frame0, gCardWep01Palette, gCardWep01Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameThreeWishes, gJiminyAttackCardThreeWishesLines, ARRAY_COUNT(gJiminyAttackCardThreeWishesLines), gCardWep04Frame0, gCardWep04Palette, gCardWep04Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameCrabclaw, gJiminyAttackCardCrabclawLines, ARRAY_COUNT(gJiminyAttackCardCrabclawLines), gCardWep05Frame0, gCardWep05Palette, gCardWep05Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNamePumpkinhead, gJiminyAttackCardPumpkinheadLines, ARRAY_COUNT(gJiminyAttackCardPumpkinheadLines), gCardWep06Frame0, gCardWep06Palette, gCardWep06Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameFairyHarp, gJiminyAttackCardFairyHarpLines, ARRAY_COUNT(gJiminyAttackCardFairyHarpLines), gCardWep07Frame0, gCardWep07Palette, gCardWep07Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameWishingStar, gJiminyAttackCardWishingStarLines, ARRAY_COUNT(gJiminyAttackCardWishingStarLines), gCardWep08Frame0, gCardWep08Palette, gCardWep08Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameSpellbinder, gJiminyAttackCardSpellbinderLines, ARRAY_COUNT(gJiminyAttackCardSpellbinderLines), gCardWep09Frame0, gCardWep09Palette, gCardWep09Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameMetalChocobo, gJiminyAttackCardMetalChocoboLines, ARRAY_COUNT(gJiminyAttackCardMetalChocoboLines), gCardWep10Frame0, gCardWep10Palette, gCardWep10Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameOlympia, gJiminyAttackCardOlympiaLines, ARRAY_COUNT(gJiminyAttackCardOlympiaLines), gCardWep03Frame0, gCardWep03Palette, gCardWep03Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameLionheart, gJiminyAttackCardLionheartLines, ARRAY_COUNT(gJiminyAttackCardLionheartLines), gCardWep11Frame0, gCardWep11Palette, gCardWep11Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameLadyLuck, gJiminyAttackCardLadyLuckLines, ARRAY_COUNT(gJiminyAttackCardLadyLuckLines), gCardWep12Frame0, gCardWep12Palette, gCardWep12Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameDivineRose, gJiminyAttackCardDivineRoseLines, ARRAY_COUNT(gJiminyAttackCardDivineRoseLines), gCardWep13Frame0, gCardWep13Palette, gCardWep13Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameOathkeeper, gJiminyAttackCardOathkeeperLines, ARRAY_COUNT(gJiminyAttackCardOathkeeperLines), gCardWep14Frame0, gCardWep14Palette, gCardWep14Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameOblivion, gJiminyAttackCardOblivionLines, ARRAY_COUNT(gJiminyAttackCardOblivionLines), gCardWep15Frame0, gCardWep15Palette, gCardWep15Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameDiamondDust, gJiminyAttackCardDiamondDustLines, ARRAY_COUNT(gJiminyAttackCardDiamondDustLines), gCardWep18Frame0, gCardWep18Palette, gCardWep18Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameOneWingedAngel, gJiminyAttackCardOneWingedAngelLines, ARRAY_COUNT(gJiminyAttackCardOneWingedAngelLines), gCardWep19Frame0, gCardWep19Palette, gCardWep19Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameUltimaWeapon, gJiminyAttackCardUltimaWeaponLines, ARRAY_COUNT(gJiminyAttackCardUltimaWeaponLines), gCardWep16Frame0, gCardWep16Palette, gCardWep16Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry05Details[14] = {
    { gCardNameFire, gJiminyMagicCardFireLines, ARRAY_COUNT(gJiminyMagicCardFireLines), gCardMgc01Frame0, gCardMgc01Palette, gCardMgc01Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameBlizzard, gJiminyMagicCardBlizzardLines, ARRAY_COUNT(gJiminyMagicCardBlizzardLines), gCardMgc02Frame0, gCardMgc02Palette, gCardMgc02Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameThunder, gJiminyMagicCardThunderLines, ARRAY_COUNT(gJiminyMagicCardThunderLines), gCardMgc03Frame0, gCardMgc03Palette, gCardMgc03Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameCure, gJiminyMagicCardCureLines, ARRAY_COUNT(gJiminyMagicCardCureLines), gCardMgc04Frame0, gCardMgc04Palette, gCardMgc04Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameGravity, gJiminyMagicCardGravityLines, ARRAY_COUNT(gJiminyMagicCardGravityLines), gCardMgc05Frame0, gCardMgc05Palette, gCardMgc05Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameStop, gJiminyMagicCardStopLines, ARRAY_COUNT(gJiminyMagicCardStopLines), gCardMgc06Frame0, gCardMgc06Palette, gCardMgc06Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameAero, gJiminyMagicCardAeroLines, ARRAY_COUNT(gJiminyMagicCardAeroLines), gCardMgc07Frame0, gCardMgc07Palette, gCardMgc07Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameSimba, gJiminyMagicCardSimbaLines, ARRAY_COUNT(gJiminyMagicCardSimbaLines), gCardSmn03Frame0, gCardSmn03Palette, gCardSmn03Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameDumbo, gJiminyMagicCardDumboLines, ARRAY_COUNT(gJiminyMagicCardDumboLines), gCardSmn06Frame0, gCardSmn06Palette, gCardSmn06Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameBambi, gJiminyMagicCardBambiLines, ARRAY_COUNT(gJiminyMagicCardBambiLines), gCardSmn05Frame0, gBanbPalette, gCardSmn05Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameMushu, gJiminyMagicCardMushuLines, ARRAY_COUNT(gJiminyMagicCardMushuLines), gCardSmn08Frame0, gMushuPalette, gCardSmn08Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameGenie, gJiminyMagicCardGenieLines, ARRAY_COUNT(gJiminyMagicCardGenieLines), gCardSmn04Frame0, gCardSmn04Palette, gCardSmn04Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameTinkerBell, gJiminyMagicCardTinkerBellLines, ARRAY_COUNT(gJiminyMagicCardTinkerBellLines), gCardSmn07Frame0, gTinkPalette, gCardSmn07Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameCloud, gJiminyMagicCardCloudLines, ARRAY_COUNT(gJiminyMagicCardCloudLines), gCardSmn09Frame0, gCardSmn09Palette, gCardSmn09Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry06Details[7] = {
    { gCardNamePotion, gJiminyItemCardPotionLines, ARRAY_COUNT(gJiminyItemCardPotionLines), gCardItm01Frame0, gCardItm01Palette, gCardItm01Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameHiPotion, gJiminyItemCardHiPotionLines, ARRAY_COUNT(gJiminyItemCardHiPotionLines), gCardItm02Frame0, gCardItm02Palette, gCardItm02Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameMegaPotion, gJiminyItemCardMegaPotionLines, ARRAY_COUNT(gJiminyItemCardMegaPotionLines), gCardItm03Frame0, gCardItm03Palette, gCardItm03Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameEther, gJiminyItemCardEtherLines, ARRAY_COUNT(gJiminyItemCardEtherLines), gCardItm04Frame0, gCardItm04Palette, gCardItm04Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameMegaEther, gJiminyItemCardMegaEtherLines, ARRAY_COUNT(gJiminyItemCardMegaEtherLines), gCardItm05Frame0, gCardItm05Palette, gCardItm05Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameElixir, gJiminyItemCardElixirLines, ARRAY_COUNT(gJiminyItemCardElixirLines), gCardItm06Frame0, gCardItm06Palette, gCardItm06Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameMegalixir, gJiminyItemCardMegalixirLines, ARRAY_COUNT(gJiminyItemCardMegalixirLines), gCardItm07Frame0, gCardItm07Palette, gCardItm07Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry07Details[7] = {
    { gCardNameDonaldDuck, gJiminyFriendCardDonaldDuckLines, ARRAY_COUNT(gJiminyFriendCardDonaldDuckLines), gCardSmn02Frame0, gDonaldPalette, gCardSmn02Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameGoofy, gJiminyFriendCardGoofyLines, ARRAY_COUNT(gJiminyFriendCardGoofyLines), gCardSmn01Frame0, gGoofyPalette, gCardSmn01Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameAladdin, gJiminyFriendCardAladdinLines, ARRAY_COUNT(gJiminyFriendCardAladdinLines), gCardSmn10Frame0, gAladdinPalette, gCardSmn10Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameJack, gJiminyFriendCardJackLines, ARRAY_COUNT(gJiminyFriendCardJackLines), gCardSmn12Frame0, gJackPalette, gCardSmn12Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameAriel, gJiminyFriendCardArielLines, ARRAY_COUNT(gJiminyFriendCardArielLines), gCardSmn11Frame0, gArielPalette, gCardSmn11Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNamePeterPan, gJiminyFriendCardPeterPanLines, ARRAY_COUNT(gJiminyFriendCardPeterPanLines), gCardSmn13Frame0, gPeterPalette, gCardSmn13Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameBeast, gJiminyFriendCardBeastLines, ARRAY_COUNT(gJiminyFriendCardBeastLines), gCardSmn14Frame0, gBeastPalette, gCardSmn14Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry08Details[49] = {
    { gEnemyNameShadow, gJiminyEnemyCardShadowLines, ARRAY_COUNT(gJiminyEnemyCardShadowLines), gCardEmy00Frame0, gEmy00Palette, gCardEmy00Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameSoldier, gJiminyEnemyCardSoldierLines, ARRAY_COUNT(gJiminyEnemyCardSoldierLines), gCardEmy14Frame0, gEmy14Palette, gCardEmy14Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameLargeBody, gJiminyEnemyCardLargeBodyLines, ARRAY_COUNT(gJiminyEnemyCardLargeBodyLines), gCardEmy38Frame0, gEmy38Palette, gCardEmy38Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameRedNocturne, gJiminyEnemyCardRedNocturneLines, ARRAY_COUNT(gJiminyEnemyCardRedNocturneLines), gCardEmy01Frame0, gEmy01Palette, gCardEmy01Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameBlueRhapsody, gJiminyEnemyCardBlueRhapsodyLines, ARRAY_COUNT(gJiminyEnemyCardBlueRhapsodyLines), gBlueRhapsodyCardFrame0, gEmy02Palette, gBlueRhapsodyCardTiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameYellowOpera, gJiminyEnemyCardYellowOperaLines, ARRAY_COUNT(gJiminyEnemyCardYellowOperaLines), gYellowOperaCardFrame0, gEmy03Palette, gYellowOperaCardTiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameGreenRequiem, gJiminyEnemyCardGreenRequiemLines, ARRAY_COUNT(gJiminyEnemyCardGreenRequiemLines), gGreenRequiemCardFrame0, gEmy04Palette, gGreenRequiemCardTiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNamePowerwild, gJiminyEnemyCardPowerwildLines, ARRAY_COUNT(gJiminyEnemyCardPowerwildLines), gCardEmy15Frame0, gEmy15Palette, gCardEmy15Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameBouncywild, gJiminyEnemyCardBouncywildLines, ARRAY_COUNT(gJiminyEnemyCardBouncywildLines), gCardEmy16Frame0, gEmy16Palette, gCardEmy16Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameAirSoldier, gJiminyEnemyCardAirSoldierLines, ARRAY_COUNT(gJiminyEnemyCardAirSoldierLines), gCardEmy18Frame0, gEmy18Palette, gCardEmy18Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameBandit, gJiminyEnemyCardBanditLines, ARRAY_COUNT(gJiminyEnemyCardBanditLines), gCardEmy19Frame0, gEmy19Palette, gCardEmy19Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameFatBandit, gJiminyEnemyCardFatBanditLines, ARRAY_COUNT(gJiminyEnemyCardFatBanditLines), gCardEmy39Frame0, gEmy39Palette, gCardEmy39Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameBarrelSpider, gJiminyEnemyCardBarrelSpiderLines, ARRAY_COUNT(gJiminyEnemyCardBarrelSpiderLines), gCardEmy21Frame0, gEmy21Palette, gCardEmy21Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameSearchGhost, gJiminyEnemyCardSearchGhostLines, ARRAY_COUNT(gJiminyEnemyCardSearchGhostLines), gCardEmy22Frame0, gEmy22Palette, gCardEmy22Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameSeaNeon, gJiminyEnemyCardSeaNeonLines, ARRAY_COUNT(gJiminyEnemyCardSeaNeonLines), gCardEmy06Frame0, gEmy06Palette, gCardEmy06Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameScrewdiver, gJiminyEnemyCardScrewdiverLines, ARRAY_COUNT(gJiminyEnemyCardScrewdiverLines), gCardEmy23Frame0, gEmy23Palette, gCardEmy23Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameAquatank, gJiminyEnemyCardAquatankLines, ARRAY_COUNT(gJiminyEnemyCardAquatankLines), gCardEmy41Frame0, gEmy41Palette, gCardEmy41Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameWightKnight, gJiminyEnemyCardWightKnightLines, ARRAY_COUNT(gJiminyEnemyCardWightKnightLines), gCardEmy25Frame0, gEmy25Palette, gCardEmy25Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameGargoyle, gJiminyEnemyCardGargoyleLines, ARRAY_COUNT(gJiminyEnemyCardGargoyleLines), gCardEmy26Frame0, gEmy26Palette, gCardEmy26Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNamePirate, gJiminyEnemyCardPirateLines, ARRAY_COUNT(gJiminyEnemyCardPirateLines), gCardEmy27Frame0, gEmy27Palette, gCardEmy27Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameAirPirate, gJiminyEnemyCardAirPirateLines, ARRAY_COUNT(gJiminyEnemyCardAirPirateLines), gCardEmy28Frame0, gEmy28Palette, gCardEmy28Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameDarkball, gJiminyEnemyCardDarkballLines, ARRAY_COUNT(gJiminyEnemyCardDarkballLines), gCardEmy29Frame0, gEmy29Palette, gCardEmy29Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameDefender, gJiminyEnemyCardDefenderLines, ARRAY_COUNT(gJiminyEnemyCardDefenderLines), gCardEmy44Frame0, gEmy44Palette, gCardEmy44Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameWyvern, gJiminyEnemyCardWyvernLines, ARRAY_COUNT(gJiminyEnemyCardWyvernLines), gCardEmy30Frame0, gEmy30Palette, gCardEmy30Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameWizard, gJiminyEnemyCardWizardLines, ARRAY_COUNT(gJiminyEnemyCardWizardLines), gCardEmy31Frame0, gEmy31Palette, gCardEmy31Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameNeoshadow, gJiminyEnemyCardNeoshadowLines, ARRAY_COUNT(gJiminyEnemyCardNeoshadowLines), gCardEmy37Frame0, gEmy37Palette, gCardEmy37Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameWhiteMushroom, gJiminyEnemyCardWhiteMushroomLines, ARRAY_COUNT(gJiminyEnemyCardWhiteMushroomLines), gCardEmy07Frame0, gEmy07Palette, gCardEmy07Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameBlackFungus, gJiminyEnemyCardBlackFungusLines, ARRAY_COUNT(gJiminyEnemyCardBlackFungusLines), gCardEmy08Frame0, gEmy07bPalette, gCardEmy08Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameCreeperPlant, gJiminyEnemyCardCreeperPlantLines, ARRAY_COUNT(gJiminyEnemyCardCreeperPlantLines), gCardEmy83Frame0, gEmy83Palette, gCardEmy83Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameTornadoStep, gJiminyEnemyCardTornadoStepLines, ARRAY_COUNT(gJiminyEnemyCardTornadoStepLines), gCardEmy81Frame0, gEmy81Palette, gCardEmy81Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameCrescendo, gJiminyEnemyCardCrescendoLines, ARRAY_COUNT(gJiminyEnemyCardCrescendoLines), gCardEmy82Frame0, gEmy82Palette, gCardEmy82Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameGuardArmor, gJiminyEnemyCardGuardArmorLines, ARRAY_COUNT(gJiminyEnemyCardGuardArmorLines), gCardBos01Frame0, gBoss01objPalette, gCardBos01Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameParasiteCage, gJiminyEnemyCardParasiteCageLines, ARRAY_COUNT(gJiminyEnemyCardParasiteCageLines), gCardBos04Frame0, gBosPcBgPalettes, gCardBos04Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameTrickmaster, gJiminyEnemyCardTrickmasterLines, ARRAY_COUNT(gJiminyEnemyCardTrickmasterLines), gCardBos03Frame0, gBoss03objPalettes, gCardBos03Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameDarkside, gJiminyEnemyCardDarksideLines, ARRAY_COUNT(gJiminyEnemyCardDarksideLines), gCardBos00Frame0, gBoss00ObjPalette, gCardBos00Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameCardSoldier, gJiminyEnemyCardCardSoldierLines, ARRAY_COUNT(gJiminyEnemyCardCardSoldierLines), gCardNpcAw04Frame0, gTrumpHPalette, gCardNpcAw04Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameHades, gJiminyEnemyCardHadesLines, ARRAY_COUNT(gJiminyEnemyCardHadesLines), gCardBos11Frame0, gCardBos11Palette, gCardBos11Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameJafar, gJiminyEnemyCardJafarLines, ARRAY_COUNT(gJiminyEnemyCardJafarLines), gCardBos05Frame0, gCardBos05Palette, gCardBos05Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameOogieBoogie, gJiminyEnemyCardOogieBoogieLines, ARRAY_COUNT(gJiminyEnemyCardOogieBoogieLines), gCardBos02Frame0, gBoss02objPalette, gCardBos02Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameUrsula, gJiminyEnemyCardUrsulaLines, ARRAY_COUNT(gJiminyEnemyCardUrsulaLines), gCardBos06Frame0, gCardBos06Palette, gCardBos06Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameHook, gJiminyEnemyCardHookLines, ARRAY_COUNT(gJiminyEnemyCardHookLines), gCardBos10Frame0, gCardBos10Palette, gCardBos10Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameDragonMaleficent, gJiminyEnemyCardDragonMaleficentLines, ARRAY_COUNT(gJiminyEnemyCardDragonMaleficentLines), gCardBos07Frame0, gCardBos07Palette, gCardBos07Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameRiku, gJiminyEnemyCardRikuLines, ARRAY_COUNT(gJiminyEnemyCardRikuLines), gCardBos12Frame0, gCardBos12Palette, gCardBos12Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameAxel, gJiminyEnemyCardAxelLines, ARRAY_COUNT(gJiminyEnemyCardAxelLines), gCardBos13Frame0, gCardBos13Palette, gCardBos13Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameLarxene, gJiminyEnemyCardLarxeneLines, ARRAY_COUNT(gJiminyEnemyCardLarxeneLines), gCardBos14Frame0, gCardBos14Palette, gCardBos14Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameVexen, gJiminyEnemyCardVexenLines, ARRAY_COUNT(gJiminyEnemyCardVexenLines), gCardBos15Frame0, gCardBos15Palette, gCardBos15Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameMarluxia, gJiminyEnemyCardMarluxiaLines, ARRAY_COUNT(gJiminyEnemyCardMarluxiaLines), gCardBos16Frame0, gCardBos16Palette, gCardBos16Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameLexaeus, gJiminyEnemyCardLexaeusLines, ARRAY_COUNT(gJiminyEnemyCardLexaeusLines), gCardBos17Frame0, gCardBos17Palette, gCardBos17Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameAnsem, gJiminyEnemyCardAnsemLines, ARRAY_COUNT(gJiminyEnemyCardAnsemLines), gCardBos18Frame0, gCardBos18Palette, gCardBos18Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry09Details[26] = {
    { gRoomNameTranquilDarkness, gJiminyMapCardTranquilDarknessLines, ARRAY_COUNT(gJiminyMapCardTranquilDarknessLines), gCardRoom02Frame0, gCardRoom02Palette, gCardRoom02Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameTeemingDarkness, gJiminyMapCardTeemingDarknessLines, ARRAY_COUNT(gJiminyMapCardTeemingDarknessLines), gCardRoom01Frame0, gCardRoom01Palette, gCardRoom01Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameFeebleDarkness, gJiminyMapCardFeebleDarknessLines, ARRAY_COUNT(gJiminyMapCardFeebleDarknessLines), gCardRoom07Frame0, gCardRoom07Palette, gCardRoom07Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameAlmightyDarkness, gJiminyMapCardAlmightyDarknessLines, ARRAY_COUNT(gJiminyMapCardAlmightyDarknessLines), gCardRoom08Frame0, gCardRoom08Palette, gCardRoom08Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameSleepingDarkness, gJiminyMapCardSleepingDarknessLines, ARRAY_COUNT(gJiminyMapCardSleepingDarknessLines), gCardRoom05Frame0, gCardRoom05Palette, gCardRoom05Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameLoomingDarkness, gJiminyMapCardLoomingDarknessLines, ARRAY_COUNT(gJiminyMapCardLoomingDarknessLines), gCardRoom04Frame0, gCardRoom04Palette, gCardRoom04Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNamePremiumRoom, gJiminyMapCardPremiumRoomLines, ARRAY_COUNT(gJiminyMapCardPremiumRoomLines), gCardRoom20Frame0, gCardRoom20Palette, gCardRoom20Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameWhiteRoom, gJiminyMapCardWhiteRoomLines, ARRAY_COUNT(gJiminyMapCardWhiteRoomLines), gCardRoom21Frame0, gCardRoom21Palette, gCardRoom21Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameBlackRoom, gJiminyMapCardBlackRoomLines, ARRAY_COUNT(gJiminyMapCardBlackRoomLines), gCardRoom22Frame0, gCardRoom22Palette, gCardRoom22Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameMartialWaking, gJiminyMapCardMartialWakingLines, ARRAY_COUNT(gJiminyMapCardMartialWakingLines), gCardRoom13Frame0, gCardRoom13Palette, gCardRoom13Tiles, gMapCardBorderGreenFrame0, gMapCardBorderPalettes, gMapCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameSorcerousWaking, gJiminyMapCardSorcerousWakingLines, ARRAY_COUNT(gJiminyMapCardSorcerousWakingLines), gCardRoom12Frame0, gCardRoom12Palette, gCardRoom12Tiles, gMapCardBorderGreenFrame0, gMapCardBorderPalettes, gMapCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameAlchemicWaking, gJiminyMapCardAlchemicWakingLines, ARRAY_COUNT(gJiminyMapCardAlchemicWakingLines), gCardRoom14Frame0, gCardRoom14Palette, gCardRoom14Tiles, gMapCardBorderGreenFrame0, gMapCardBorderPalettes, gMapCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameMeetingGround, gJiminyMapCardMeetingGroundLines, ARRAY_COUNT(gJiminyMapCardMeetingGroundLines), gCardRoom15Frame0, gCardRoom15Palette, gCardRoom15Tiles, gMapCardBorderGreenFrame0, gMapCardBorderPalettes, gMapCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameStagnantSpace, gJiminyMapCardStagnantSpaceLines, ARRAY_COUNT(gJiminyMapCardStagnantSpaceLines), gCardRoom19Frame0, gCardRoom19Palette, gCardRoom19Tiles, gMapCardBorderGreenFrame0, gMapCardBorderPalettes, gMapCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameStrongInitiative, gJiminyMapCardStrongInitiativeLines, ARRAY_COUNT(gJiminyMapCardStrongInitiativeLines), gCardRoom17Frame0, gCardRoom17Palette, gCardRoom17Tiles, gMapCardBorderGreenFrame0, gMapCardBorderPalettes, gMapCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameLastingDaze, gJiminyMapCardLastingDazeLines, ARRAY_COUNT(gJiminyMapCardLastingDazeLines), gCardRoom18Frame0, gCardRoom18Palette, gCardRoom18Tiles, gMapCardBorderGreenFrame0, gMapCardBorderPalettes, gMapCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameCalmBounty, gJiminyMapCardCalmBountyLines, ARRAY_COUNT(gJiminyMapCardCalmBountyLines), gCardRoom09Frame0, gCardRoom09Palette, gCardRoom09Tiles, gMapCardBorderBlueFrame0, gMapCardBorderPalettes, gMapCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameGuardedTrove, gJiminyMapCardGuardedTroveLines, ARRAY_COUNT(gJiminyMapCardGuardedTroveLines), gCardRoom03Frame0, gCardRoom03Palette, gCardRoom03Tiles, gMapCardBorderBlueFrame0, gMapCardBorderPalettes, gMapCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameFalseBounty, gJiminyMapCardFalseBountyLines, ARRAY_COUNT(gJiminyMapCardFalseBountyLines), gCardRoom10Frame0, gCardRoom10Palette, gCardRoom10Tiles, gMapCardBorderBlueFrame0, gMapCardBorderPalettes, gMapCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameMomentsReprieve, gJiminyMapCardMomentsReprieveLines, ARRAY_COUNT(gJiminyMapCardMomentsReprieveLines), gCardRoom06Frame0, gCardRoom06Palette, gCardRoom06Tiles, gMapCardBorderBlueFrame0, gMapCardBorderPalettes, gMapCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameMinglingWorlds, gJiminyMapCardMinglingWorldsLines, ARRAY_COUNT(gJiminyMapCardMinglingWorldsLines), gCardRoom16Frame0, gCardRoom16Palette, gCardRoom16Tiles, gMapCardBorderBlueFrame0, gMapCardBorderPalettes, gMapCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameMoogleRoom, gJiminyMapCardMoogleRoomLines, ARRAY_COUNT(gJiminyMapCardMoogleRoomLines), gCardRoom11Frame0, gCardRoom11Palette, gCardRoom11Tiles, gMapCardBorderBlueFrame0, gMapCardBorderPalettes, gMapCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameKeyOfBeginnings, gJiminyMapCardKeyOfBeginningsLines, ARRAY_COUNT(gJiminyMapCardKeyOfBeginningsLines), gCardEve00Frame0, gCardEve00Palette, gCardEve00Tiles, gMapCardBorderBlackFrame0, gMapCardBorderPalettes, gMapCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameKeyOfGuidance, gJiminyMapCardKeyOfGuidanceLines, ARRAY_COUNT(gJiminyMapCardKeyOfGuidanceLines), gCardEve01Frame0, gCardEve01Palette, gCardEve01Tiles, gMapCardBorderBlackFrame0, gMapCardBorderPalettes, gMapCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameKeyToTruth, gJiminyMapCardKeyToTruthLines, ARRAY_COUNT(gJiminyMapCardKeyToTruthLines), gCardEve02Frame0, gCardEve02Palette, gCardEve02Tiles, gMapCardBorderBlackFrame0, gMapCardBorderPalettes, gMapCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gRoomNameKeyToRewards, gJiminyMapCardKeyToRewardsLines, ARRAY_COUNT(gJiminyMapCardKeyToRewardsLines), gCardRoom23Frame0, gCardRoom23Palette, gCardRoom23Tiles, gMapCardBorderBlackFrame0, gMapCardBorderPalettes, gMapCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry10Details[1] = {
    { gJiminyPremiumCardsName, gJiminyPremiumCardsLines, ARRAY_COUNT(gJiminyPremiumCardsLines), gJiminyPremiumCardFrame0, gJiminyPremiumCardPalette, gJiminyPremiumCardTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry16Details[22] = {
    { gCardNameSoulEater, gJiminyRikuCardSoulEaterLines, ARRAY_COUNT(gJiminyRikuCardSoulEaterLines), gCardWep20Frame0, gCardWep20Palette, gCardWep20Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameKing, gJiminyRikuCardKingLines, ARRAY_COUNT(gJiminyRikuCardKingLines), gCardSmn15Frame0, gMickeyPalette, gCardSmn15Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameShadow, gJiminyEnemyCardShadowLines, ARRAY_COUNT(gJiminyEnemyCardShadowLines), gCardEmy00Frame0, gEmy00Palette, gCardEmy00Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameLargeBody, gJiminyEnemyCardLargeBodyLines, ARRAY_COUNT(gJiminyEnemyCardLargeBodyLines), gCardEmy38Frame0, gEmy38Palette, gCardEmy38Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNamePowerwild, gJiminyEnemyCardPowerwildLines, ARRAY_COUNT(gJiminyEnemyCardPowerwildLines), gCardEmy15Frame0, gEmy15Palette, gCardEmy15Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameFatBandit, gJiminyEnemyCardFatBanditLines, ARRAY_COUNT(gJiminyEnemyCardFatBanditLines), gCardEmy39Frame0, gEmy39Palette, gCardEmy39Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameSearchGhost, gJiminyEnemyCardSearchGhostLines, ARRAY_COUNT(gJiminyEnemyCardSearchGhostLines), gCardEmy22Frame0, gEmy22Palette, gCardEmy22Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameSeaNeon, gJiminyEnemyCardSeaNeonLines, ARRAY_COUNT(gJiminyEnemyCardSeaNeonLines), gCardEmy06Frame0, gEmy06Palette, gCardEmy06Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameWightKnight, gJiminyEnemyCardWightKnightLines, ARRAY_COUNT(gJiminyEnemyCardWightKnightLines), gCardEmy25Frame0, gEmy25Palette, gCardEmy25Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNamePirate, gJiminyEnemyCardPirateLines, ARRAY_COUNT(gJiminyEnemyCardPirateLines), gCardEmy27Frame0, gEmy27Palette, gCardEmy27Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameDefender, gJiminyEnemyCardDefenderLines, ARRAY_COUNT(gJiminyEnemyCardDefenderLines), gCardEmy44Frame0, gEmy44Palette, gCardEmy44Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameGuardArmor, gJiminyEnemyCardGuardArmorLines, ARRAY_COUNT(gJiminyEnemyCardGuardArmorLines), gCardBos01Frame0, gBoss01objPalette, gCardBos01Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameParasiteCage, gJiminyEnemyCardParasiteCageLines, ARRAY_COUNT(gJiminyEnemyCardParasiteCageLines), gCardBos04Frame0, gBosPcBgPalettes, gCardBos04Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameTrickmaster, gJiminyEnemyCardTrickmasterLines, ARRAY_COUNT(gJiminyEnemyCardTrickmasterLines), gCardBos03Frame0, gBoss03objPalettes, gCardBos03Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameDarkside, gJiminyEnemyCardDarksideLines, ARRAY_COUNT(gJiminyEnemyCardDarksideLines), gCardBos00Frame0, gBoss00ObjPalette, gCardBos00Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameHades, gJiminyEnemyCardHadesLines, ARRAY_COUNT(gJiminyEnemyCardHadesLines), gCardBos11Frame0, gCardBos11Palette, gCardBos11Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameJafar, gJiminyEnemyCardJafarLines, ARRAY_COUNT(gJiminyEnemyCardJafarLines), gCardBos05Frame0, gCardBos05Palette, gCardBos05Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameOogieBoogie, gJiminyEnemyCardOogieBoogieLines, ARRAY_COUNT(gJiminyEnemyCardOogieBoogieLines), gCardBos02Frame0, gBoss02objPalette, gCardBos02Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameUrsula, gJiminyEnemyCardUrsulaLines, ARRAY_COUNT(gJiminyEnemyCardUrsulaLines), gCardBos06Frame0, gCardBos06Palette, gCardBos06Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameHook, gJiminyEnemyCardHookLines, ARRAY_COUNT(gJiminyEnemyCardHookLines), gCardBos10Frame0, gCardBos10Palette, gCardBos10Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameDragonMaleficent, gJiminyEnemyCardDragonMaleficentLines, ARRAY_COUNT(gJiminyEnemyCardDragonMaleficentLines), gCardBos07Frame0, gCardBos07Palette, gCardBos07Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gEnemyNameLexaeus, gJiminyEnemyCardLexaeusLines, ARRAY_COUNT(gJiminyEnemyCardLexaeusLines), gCardBos17Frame0, gCardBos17Palette, gCardBos17Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry11Details[25] = {
    { gCharacterNameSora, gJiminyCharacterSoraLines, ARRAY_COUNT(gJiminyCharacterSoraLines), gSor1fl00Frame0, gSoraPalette, gSor1fl00Tiles, gCardNpcEx00Frame0, gCardNpcEx00Palette, gCardNpcEx00Tiles, NULL, NULL, NULL, 0, 0, -3, -3 },
    { gCardNameDonaldDuck, gJiminyCharacterDonaldDuckLines, ARRAY_COUNT(gJiminyCharacterDonaldDuckLines), gDonaFl00Frame0, gDonaldPalette, gDonaFl00Tiles, gCardSmn02Frame0, gDonaldPalette, gCardSmn02Tiles, NULL, NULL, NULL, 0, 0, 0, -9 },
    { gCardNameGoofy, gJiminyCharacterGoofyLines, ARRAY_COUNT(gJiminyCharacterGoofyLines), gGoofyFl00Frame0, gGoofyPalette, gGoofyFl00Tiles, gCardSmn01Frame0, gGoofyPalette, gCardSmn01Tiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCharacterNameJiminyCricket, gJiminyCharacterJiminyCricketLines, ARRAY_COUNT(gJiminyCharacterJiminyCricketLines), gJimFl00Frame0, gJiminyPalette, gJimFl00Tiles, gCardNpcEx01Frame0, gCardNpcEx01Palette, gCardNpcEx01Tiles, NULL, NULL, NULL, 0, 0, -3, -11 },
    { gEnemyNameRiku, gJiminyCharacterRikuLines, ARRAY_COUNT(gJiminyCharacterRikuLines), gRikuFl00Frame0, gRikuPalette, gRikuFl00Tiles, gCardNpcEx03Frame0, gCardNpcEx03Palette, gCardNpcEx03Tiles, NULL, NULL, NULL, 0, 0, -3, 0 },
    { gCharacterNameKairi, gJiminyCharacterKairiLines, ARRAY_COUNT(gJiminyCharacterKairiLines), gKairF00Frame0, gKairiPalette, gKairF00Tiles, gCardNpcEx05Frame0, gCardNpcEx05Palette, gCardNpcEx05Tiles, NULL, NULL, NULL, 0, 0, -1, -3 },
    { gCardNameSimba, gJiminyCharacterSimbaLines, ARRAY_COUNT(gJiminyCharacterSimbaLines), gShinba10Frame0, gShinbaPalette, gShinba10Tiles, gCardSmn03Frame0, gCardSmn03Palette, gCardSmn03Tiles, NULL, NULL, NULL, 0, 0, -2, -7 },
    { gCardNameDumbo, gJiminyCharacterDumboLines, ARRAY_COUNT(gJiminyCharacterDumboLines), gSmnDumboFrame0, gDamboPalette, gSmnDumboTiles, gCardSmn06Frame0, gCardSmn06Palette, gCardSmn06Tiles, NULL, NULL, NULL, 0, 0, 8, -7 },
    { gCardNameBambi, gJiminyCharacterBambiLines, ARRAY_COUNT(gJiminyCharacterBambiLines), gBanb00Frame7, gBanbPalette, gBanb00Tiles, gCardSmn05Frame0, gBanbPalette, gCardSmn05Tiles, NULL, NULL, NULL, 0, 0, -2, -7 },
    { gCardNameMushu, gJiminyCharacterMushuLines, ARRAY_COUNT(gJiminyCharacterMushuLines), gMushuF00Frame1, gMushuPalette, gMushuF00Tiles, gCardSmn08Frame0, gMushuPalette, gCardSmn08Tiles, NULL, NULL, NULL, 0, 0, 0, -12 },
    { gCharacterNameMoogles, gJiminyCharacterMooglesLines, ARRAY_COUNT(gJiminyCharacterMooglesLines), gMoguFl00Frame0, gMoguPalette, gMoguFl00Tiles, gCardNpcEx02Frame0, gCardNpcEx02Palette, gCardNpcEx02Tiles, NULL, NULL, NULL, 0, 0, -2, -8 },
    { gCharacterNameLeon, gJiminyCharacterLeonLines, ARRAY_COUNT(gJiminyCharacterLeonLines), gReonFl00Frame0, gReonPalette, gReonFl00Tiles, gCardNpcEx09Frame0, gCardNpcEx09Palette, gCardNpcEx09Tiles, NULL, NULL, NULL, 0, 0, -3, 7 },
    { gCharacterNameYuffie, gJiminyCharacterYuffieLines, ARRAY_COUNT(gJiminyCharacterYuffieLines), gYuffieFl00Frame0, gYuffiePalette, gYuffieFl00Tiles, gCardNpcEx10Frame0, gCardNpcEx10Palette, gCardNpcEx10Tiles, NULL, NULL, NULL, 0, 0, -1, 2 },
    { gCharacterNameAerith, gJiminyCharacterAerithLines, ARRAY_COUNT(gJiminyCharacterAerithLines), gEarF00Frame0, gEarisPalette, gEarF00Tiles, gCardNpcEx06Frame0, gCardNpcEx06Palette, gCardNpcEx06Tiles, NULL, NULL, NULL, 0, 0, -3, 4 },
    { gCharacterNameCid, gJiminyCharacterCidLines, ARRAY_COUNT(gJiminyCharacterCidLines), gShidoF00Frame0, gShidoPalette, gShidoF00Tiles, gCardNpcEx07Frame0, gCardNpcEx07Palette, gCardNpcEx07Tiles, NULL, NULL, NULL, 0, 0, -2, 5 },
    { gCardNameCloud, gJiminyCharacterCloudLines, ARRAY_COUNT(gJiminyCharacterCloudLines), gCroudF00Frame0, gCroudPalette, gCroudF00Tiles, gCardSmn09Frame0, gCardSmn09Palette, gCardSmn09Tiles, NULL, NULL, NULL, 0, 0, -3, 2 },
    { gCharacterNameTidus, gJiminyCharacterTidusLines, ARRAY_COUNT(gJiminyCharacterTidusLines), gTidusFl00Frame0, gTidusPalette, gTidusFl00Tiles, gCardNpcDi01Frame0, gCardNpcDi01Palette, gCardNpcDi01Tiles, NULL, NULL, NULL, 0, 0, -2, -3 },
    { gCharacterNameWakka, gJiminyCharacterWakkaLines, ARRAY_COUNT(gJiminyCharacterWakkaLines), gWakkaF00Frame0, gWakkaPalette, gWakkaF00Tiles, gCardNpcDi02Frame0, gCardNpcDi02Palette, gCardNpcDi02Tiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCharacterNameSelphie, gJiminyCharacterSelphieLines, ARRAY_COUNT(gJiminyCharacterSelphieLines), gSelphieFl00Frame0, gSelphiePalette, gSelphieFl00Tiles, gCardNpcDi03Frame0, gCardNpcDi03Palette, gCardNpcDi03Tiles, NULL, NULL, NULL, 0, 0, -2, -3 },
    { gCharacterNameNamine, gJiminyCharacterNamineLines, ARRAY_COUNT(gJiminyCharacterNamineLines), gNamiF00Frame0, gNaminePalette, gNamiF00Tiles, gCardNpcCom01Frame0, gCardNpcCom01Palette, gCardNpcCom01Tiles, NULL, NULL, NULL, 0, 0, -1, -4 },
    { gCharacterNameRikuReplica, gJiminyCharacterRikuReplicaLines, ARRAY_COUNT(gJiminyCharacterRikuReplicaLines), gNiseFl00Frame0, gNiserikuPalette, gNiseFl00Tiles, gCardBos12Frame0, gCardBos12Palette, gCardBos12Tiles, NULL, NULL, NULL, 0, 0, -3, 0 },
    { gEnemyNameAxel, gJiminyCharacterAxelLines, ARRAY_COUNT(gJiminyCharacterAxelLines), gAcceleFl00Frame0, gAccelePalette, gAcceleFl00Tiles, gCardBos13Frame0, gCardBos13Palette, gCardBos13Tiles, NULL, NULL, NULL, 0, 0, -3, 6 },
    { gEnemyNameLarxene, gJiminyCharacterLarxeneLines, ARRAY_COUNT(gJiminyCharacterLarxeneLines), gLaxineFl00Frame0, gLaxinePalette, gLaxineFl00Tiles, gCardBos14Frame0, gCardBos14Palette, gCardBos14Tiles, NULL, NULL, NULL, 0, 0, -2, 5 },
    { gEnemyNameVexen, gJiminyCharacterVexenLines, ARRAY_COUNT(gJiminyCharacterVexenLines), gVixenFl00Frame0, gVixenPalette, gVixenFl00Tiles, gCardBos15Frame0, gCardBos15Palette, gCardBos15Tiles, NULL, NULL, NULL, 0, 0, -3, 6 },
    { gEnemyNameMarluxia, gJiminyCharacterMarluxiaLines, ARRAY_COUNT(gJiminyCharacterMarluxiaLines), gMaruxha2Fl00Frame0, gMaruxhaPalette, gMaruxha2Fl00Tiles, gCardBos16Frame0, gCardBos16Palette, gCardBos16Tiles, NULL, NULL, NULL, 0, 0, -3, 6 },
};

static const JiminyDetail sJiminyEntry12Details[40] = {
    { gCharacterNameAlice, gJiminyCharacterAliceLines, ARRAY_COUNT(gJiminyCharacterAliceLines), gAliceFl00Frame0, gAlicePalette, gAliceFl00Tiles, gCardNpcAw01Frame0, gCardNpcAw01Palette, gCardNpcAw01Tiles, NULL, NULL, NULL, 0, 0, -1, -1 },
    { gCharacterNameQueenOfHearts, gJiminyCharacterQueenOfHeartsLines, ARRAY_COUNT(gJiminyCharacterQueenOfHeartsLines), gQenF00Frame0, gQeenPalette, gQenF00Tiles, gCardNpcAw02Frame0, gCardNpcAw02Palette, gCardNpcAw02Tiles, NULL, NULL, NULL, 0, 0, -3, 4 },
    { gCharacterNameWhiteRabbit, gJiminyCharacterWhiteRabbitLines, ARRAY_COUNT(gJiminyCharacterWhiteRabbitLines), gUsagif00Frame0, gUsagi00Palette, gUsagif00Tiles, gCardNpcAw05Frame0, gCardNpcAw05Palette, gCardNpcAw05Tiles, NULL, NULL, NULL, 0, 0, 0, -10 },
    { gCharacterNameCardOfHearts, gJiminyCharacterCardOfHeartsLines, ARRAY_COUNT(gJiminyCharacterCardOfHeartsLines), gTrumpH00Frame0, gTrumpHPalette, gTrumpH00Tiles, gCardNpcAw04Frame0, gTrumpHPalette, gCardNpcAw04Tiles, NULL, NULL, NULL, 0, 0, 0, 2 },
    { gCharacterNameCardOfSpades, gJiminyCharacterCardOfSpadesLines, ARRAY_COUNT(gJiminyCharacterCardOfSpadesLines), gTrumpS00Frame0, gTrumpSPalette, gTrumpS00Tiles, gCardNpcAw03Frame0, gTrumpSPalette, gCardNpcAw03Tiles, NULL, NULL, NULL, 0, 0, 0, 2 },
    { gCharacterNameCheshireCat, gJiminyCharacterCheshireCatLines, ARRAY_COUNT(gJiminyCharacterCheshireCatLines), gCheshireFrame0, gCheshirePalette, gCheshireTiles, gCardNpcAw06Frame0, gCardNpcAw06Palette, gCardNpcAw06Tiles, NULL, NULL, NULL, 0, 0, -4, -12 },
    { gCharacterNameHercules, gJiminyCharacterHerculesLines, ARRAY_COUNT(gJiminyCharacterHerculesLines), gHeraclesFl00Frame0, gHeraclesPalette, gHeraclesFl00Tiles, gCardNpcHe02Frame0, gCardNpcHe02Palette, gCardNpcHe02Tiles, NULL, NULL, NULL, 0, 0, 0, 13 },
    { gCharacterNamePhiloctetes, gJiminyCharacterPhiloctetesLines, ARRAY_COUNT(gJiminyCharacterPhiloctetesLines), gPhilFl00Frame0, gPhilPalette, gPhilFl00Tiles, gCardNpcHe01Frame0, gCardNpcHe01Palette, gCardNpcHe01Tiles, NULL, NULL, NULL, 0, 0, -2, -10 },
    { gEnemyNameHades, gJiminyCharacterHadesLines, ARRAY_COUNT(gJiminyCharacterHadesLines), gHadesFl00Frame0, gHadesPalette, gHadesFl00Tiles, gCardBos11Frame0, gCardBos11Palette, gCardBos11Tiles, NULL, NULL, NULL, 0, 0, 0, 18 },
    { gCardNameAladdin, gJiminyCharacterAladdinLines, ARRAY_COUNT(gJiminyCharacterAladdinLines), gAladdin00Frame0, gAladdinPalette, gAladdin00Tiles, gCardSmn10Frame0, gAladdinPalette, gCardSmn10Tiles, NULL, NULL, NULL, 0, 0, -2, 6 },
    { gCardNameGenie, gJiminyCharacterGenieLines, ARRAY_COUNT(gJiminyCharacterGenieLines), gGenie01Frame2, gGeniePalette, gGenie01Tiles, gCardSmn04Frame0, gCardSmn04Palette, gCardSmn04Tiles, NULL, NULL, NULL, 0, 0, 2, 1 },
    { gCharacterNameJasmine, gJiminyCharacterJasmineLines, ARRAY_COUNT(gJiminyCharacterJasmineLines), gJasmineF00Frame0, gJasminePalette, gJasmineF00Tiles, gCardNpcAl01Frame0, gCardNpcAl01Palette, gCardNpcAl01Tiles, NULL, NULL, NULL, 0, 0, -4, 5 },
    { gCharacterNameIago, gJiminyCharacterIagoLines, ARRAY_COUNT(gJiminyCharacterIagoLines), gIagoFrame0, gIagoPalette, gIagoTiles, gCardNpcAl03Frame0, gCardNpcAl03Palette, gCardNpcAl03Tiles, NULL, NULL, NULL, 0, 0, -2, -10 },
    { gEnemyNameJafar, gJiminyCharacterJafarLines, ARRAY_COUNT(gJiminyCharacterJafarLines), gJafferFl00Frame0, gJafferPalette, gJafferFl00Tiles, gCardNpcAl02Frame0, gCardNpcAl02Palette, gCardNpcAl02Tiles, NULL, NULL, NULL, 0, 0, -3, 19 },
    { gCharacterNameJafarGenie, gJiminyCharacterJafarGenieLines, ARRAY_COUNT(gJiminyCharacterJafarGenieLines), NULL, NULL, NULL, gCardBos05Frame0, gCardBos05Palette, gCardBos05Tiles, gJiminyJafarGenieBgMap, gJiminyJafarGenieBgPalette, gJiminyJafarGenieBgTiles, sizeof(gJiminyJafarGenieBgPalette), sizeof(gJiminyJafarGenieBgTiles), 0, 0 },
    { gCardNameJack, gJiminyCharacterJackLines, ARRAY_COUNT(gJiminyCharacterJackLines), gJackFl00Frame0, gJackPalette, gJackFl00Tiles, gCardSmn12Frame0, gJackPalette, gCardSmn12Tiles, NULL, NULL, NULL, 0, 0, 2, 11 },
    { gCharacterNameSally, gJiminyCharacterSallyLines, ARRAY_COUNT(gJiminyCharacterSallyLines), gSariFl00Frame0, gSariPalette, gSariFl00Tiles, gCardNpcNm01Frame0, gCardNpcNm01Palette, gCardNpcNm01Tiles, NULL, NULL, NULL, 0, 0, -2, 7 },
    { gCharacterNameDrFinkelstein, gJiminyCharacterDrFinkelsteinLines, ARRAY_COUNT(gJiminyCharacterDrFinkelsteinLines), gFinklF00Frame0, gFinklPalette, gFinklF00Tiles, gCardNpcNm02Frame0, gCardNpcNm02Palette, gCardNpcNm02Tiles, NULL, NULL, NULL, 0, 0, -1, -7 },
    { gEnemyNameOogieBoogie, gJiminyCharacterOogieBoogieLines, ARRAY_COUNT(gJiminyCharacterOogieBoogieLines), gBugiFl00Frame0, gBugiPalette, gBugiFl00Tiles, gCardBos02Frame0, gBoss02objPalette, gCardBos02Tiles, NULL, NULL, NULL, 0, 0, -4, 9 },
    { gCharacterNamePinocchio, gJiminyCharacterPinocchioLines, ARRAY_COUNT(gJiminyCharacterPinocchioLines), gPinoF00Frame0, gPinokioPalette, gPinoF00Tiles, gCardNpcPi01Frame0, gCardNpcPi01Palette, gCardNpcPi01Tiles, NULL, NULL, NULL, 0, 0, -2, -8 },
    { gCharacterNameGeppetto, gJiminyCharacterGeppettoLines, ARRAY_COUNT(gJiminyCharacterGeppettoLines), gGeppettoF00Frame0, gGeppettoPalette, gGeppettoF00Tiles, gCardNpcPi02Frame0, gCardNpcPi02Palette, gCardNpcPi02Tiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { gCardNameAriel, gJiminyCharacterArielLines, ARRAY_COUNT(gJiminyCharacterArielLines), gArielF00Frame0, gArielPalette, gArielF00Tiles, gCardSmn11Frame0, gArielPalette, gCardSmn11Tiles, NULL, NULL, NULL, 0, 0, -4, 3 },
    { gCharacterNameSebastian, gJiminyCharacterSebastianLines, ARRAY_COUNT(gJiminyCharacterSebastianLines), gSebastianFl00Frame0, gSebastianPalette, gSebastianFl00Tiles, gCardNpcLm01Frame0, gCardNpcLm01Palette, gCardNpcLm01Tiles, NULL, NULL, NULL, 0, 0, -1, -13 },
    { gCharacterNameFlounder, gJiminyCharacterFlounderLines, ARRAY_COUNT(gJiminyCharacterFlounderLines), gFlounderFl00Frame0, gFlounderPalette, gFlounderFl00Tiles, gCardNpcLm02Frame0, gCardNpcLm02Palette, gCardNpcLm02Tiles, NULL, NULL, NULL, 0, 0, -1, -17 },
    { gEnemyNameUrsula, gJiminyCharacterUrsulaLines, ARRAY_COUNT(gJiminyCharacterUrsulaLines), gArthraFl00Frame0, gArthraPalette, gArthraFl00Tiles, gCardBos06Frame0, gCardBos06Palette, gCardBos06Tiles, NULL, NULL, NULL, 0, 0, -3, 6 },
    { gCardNamePeterPan, gJiminyCharacterPeterPanLines, ARRAY_COUNT(gJiminyCharacterPeterPanLines), gPeterFl00Frame0, gPeterPalette, gPeterFl00Tiles, gCardSmn13Frame0, gPeterPalette, gCardSmn13Tiles, NULL, NULL, NULL, 0, 0, -3, 5 },
    { gCardNameTinkerBell, gJiminyCharacterTinkerBellLines, ARRAY_COUNT(gJiminyCharacterTinkerBellLines), gTinkF00Frame2, gTinkPalette, gTinkF00Tiles, gCardSmn07Frame0, gTinkPalette, gCardSmn07Tiles, NULL, NULL, NULL, 0, 0, -5, -10 },
    { gCharacterNameWendy, gJiminyCharacterWendyLines, ARRAY_COUNT(gJiminyCharacterWendyLines), gWendyFl00Frame0, gWendyPalette, gWendyFl00Tiles, gCardNpcPp01Frame0, gCardNpcPp01Palette, gCardNpcPp01Tiles, NULL, NULL, NULL, 0, 0, -1, 4 },
    { gCharacterNameHook, gJiminyCharacterHookLines, ARRAY_COUNT(gJiminyCharacterHookLines), gHookF00Frame0, gHookPalette, gHookF00Tiles, gCardBos10Frame0, gCardBos10Palette, gCardBos10Tiles, NULL, NULL, NULL, 0, 0, -1, 17 },
    { gCardNameBeast, gJiminyCharacterBeastLines, ARRAY_COUNT(gJiminyCharacterBeastLines), gBeastFl00Frame0, gBeastPalette, gBeastFl00Tiles, gCardSmn14Frame0, gBeastPalette, gCardSmn14Tiles, NULL, NULL, NULL, 0, 0, -1, 10 },
    { gCharacterNameBelle, gJiminyCharacterBelleLines, ARRAY_COUNT(gJiminyCharacterBelleLines), gBellFl00Frame0, gBellPalette, gBellFl00Tiles, gCardNpcPc01Frame0, gCardNpcPc01Palette, gCardNpcPc01Tiles, NULL, NULL, NULL, 0, 0, -4, 5 },
    { gCharacterNameMaleficent, gJiminyCharacterMaleficentLines, ARRAY_COUNT(gJiminyCharacterMaleficentLines), gMarefF00Frame0, gMarefPalette, gMarefF00Tiles, gCardNpcPc02Frame0, gCardNpcPc02Palette, gCardNpcPc02Tiles, NULL, NULL, NULL, 0, 0, -12, 16 },
    { gEnemyNameDragonMaleficent, gJiminyCharacterDragonMaleficentLines, ARRAY_COUNT(gJiminyCharacterDragonMaleficentLines), NULL, NULL, NULL, gCardBos07Frame0, gCardBos07Palette, gCardBos07Tiles, gJiminyDragonMaleficentBgMap, gJiminyDragonMaleficentBgPalette, gJiminyDragonMaleficentBgTiles, sizeof(gJiminyDragonMaleficentBgPalette), sizeof(gJiminyDragonMaleficentBgTiles), 0, 0 },
    { gCharacterNameWinnieThePooh, gJiminyCharacterWinnieThePoohLines, ARRAY_COUNT(gJiminyCharacterWinnieThePoohLines), gPoohFl06Frame0, gPoohPalette, gPoohFl06Tiles, gCardNpcPo01Frame0, gCardNpcPo01Palette, gCardNpcPo01Tiles, NULL, NULL, NULL, 0, 0, -2, -6 },
    { gCharacterNamePiglet, gJiminyCharacterPigletLines, ARRAY_COUNT(gJiminyCharacterPigletLines), gPigletStandFrontFrame0, gPigletPalette, gPigletStandFrontTiles, gCardNpcPo03Frame0, gCardNpcPo03Palette, gCardNpcPo03Tiles, NULL, NULL, NULL, 0, 0, -2, -10 },
    { gCharacterNameOwl, gJiminyCharacterOwlLines, ARRAY_COUNT(gJiminyCharacterOwlLines), gOwlFl00Frame0, gOwlPalette, gOwlFl00Tiles, gCardNpcPo06Frame0, gCardNpcPo06Palette, gCardNpcPo06Tiles, NULL, NULL, NULL, 0, 0, -3, -7 },
    { gCharacterNameRoo, gJiminyCharacterRooLines, ARRAY_COUNT(gJiminyCharacterRooLines), gRooFl00Frame0, gRooPalette, gRooFl00Tiles, gCardNpcPo07Frame0, gCardNpcPo07Palette, gCardNpcPo07Tiles, NULL, NULL, NULL, 0, 0, -1, -13 },
    { gCharacterNameEeyore, gJiminyCharacterEeyoreLines, ARRAY_COUNT(gJiminyCharacterEeyoreLines), gEeyoreFl00Frame0, gEeyorePalette, gEeyoreFl00Tiles, gCardNpcPo04Frame0, gCardNpcPo04Palette, gCardNpcPo04Tiles, NULL, NULL, NULL, 0, 0, 5, -13 },
    { gCharacterNameTigger, gJiminyCharacterTiggerLines, ARRAY_COUNT(gJiminyCharacterTiggerLines), gTiggerFl00Frame0, gTiggerPalette, gTiggerFl00Tiles, gCardNpcPo02Frame0, gCardNpcPo02Palette, gCardNpcPo02Tiles, NULL, NULL, NULL, 0, 0, -3, -3 },
    { gCharacterNameRabbit, gJiminyCharacterRabbitLines, ARRAY_COUNT(gJiminyCharacterRabbitLines), gRabbitFl00Frame0, gRabbitPalettes, gRabbitFl00Tiles, gCardNpcPo05Frame0, gCardNpcPo05Palette, gCardNpcPo05Tiles, NULL, NULL, NULL, 0, 0, -1, -3 },
};

static const JiminyDetail sJiminyEntry13Details[35] = {
    { gEnemyNameShadow, gJiminyHeartlessShadowLines, ARRAY_COUNT(gJiminyHeartlessShadowLines), gEmy00L00Frame0, gEmy00Palette, gEmy00L00Tiles, gCardEmy00Frame0, gEmy00Palette, gCardEmy00Tiles, NULL, NULL, NULL, 0, 0, 2, -12 },
    { gEnemyNameSoldier, gJiminyHeartlessSoldierLines, ARRAY_COUNT(gJiminyHeartlessSoldierLines), gEmy14Ll01Frame1, gEmy14Palette, gEmy14Ll01Tiles, gCardEmy14Frame0, gEmy14Palette, gCardEmy14Tiles, NULL, NULL, NULL, 0, 0, -8, 2 },
    { gEnemyNameLargeBody, gJiminyHeartlessLargeBodyLines, ARRAY_COUNT(gJiminyHeartlessLargeBodyLines), gEmy3800Frame0, gEmy38Palette, gEmy3800Tiles, gCardEmy38Frame0, gEmy38Palette, gCardEmy38Tiles, NULL, NULL, NULL, 0, 0, -6, 6 },
    { gEnemyNameRedNocturne, gJiminyHeartlessRedNocturneLines, ARRAY_COUNT(gJiminyHeartlessRedNocturneLines), gEmy01L00Frame16, gEmy01Palette, gEmy01L00Tiles, gCardEmy01Frame0, gEmy01Palette, gCardEmy01Tiles, NULL, NULL, NULL, 0, 0, -2, -8 },
    { gEnemyNameBlueRhapsody, gJiminyHeartlessBlueRhapsodyLines, ARRAY_COUNT(gJiminyHeartlessBlueRhapsodyLines), gEmy01L00Frame3, gEmy02Palette, gEmy01L00Tiles, gCardEmy01Frame0, gEmy02Palette, gCardEmy01Tiles, NULL, NULL, NULL, 0, 0, -1, -11 },
    { gEnemyNameYellowOpera, gJiminyHeartlessYellowOperaLines, ARRAY_COUNT(gJiminyHeartlessYellowOperaLines), gEmy01L00Frame0, gEmy03Palette, gEmy01L00Tiles, gCardEmy01Frame0, gEmy03Palette, gCardEmy01Tiles, NULL, NULL, NULL, 0, 0, -2, -11 },
    { gEnemyNameGreenRequiem, gJiminyHeartlessGreenRequiemLines, ARRAY_COUNT(gJiminyHeartlessGreenRequiemLines), gEmy01L00Frame15, gEmy04Palette, gEmy01L00Tiles, gCardEmy01Frame0, gEmy04Palette, gCardEmy01Tiles, NULL, NULL, NULL, 0, 0, -2, -8 },
    { gEnemyNamePowerwild, gJiminyHeartlessPowerwildLines, ARRAY_COUNT(gJiminyHeartlessPowerwildLines), gEmy1500Frame7, gEmy15Palette, gEmy1500Tiles, gCardEmy15Frame0, gEmy15Palette, gCardEmy15Tiles, NULL, NULL, NULL, 0, 0, 0, -6 },
    { gEnemyNameBouncywild, gJiminyHeartlessBouncywildLines, ARRAY_COUNT(gJiminyHeartlessBouncywildLines), gEmy1600Frame0, gEmy16Palette, gEmy1600Tiles, gCardEmy16Frame0, gEmy16Palette, gCardEmy16Tiles, NULL, NULL, NULL, 0, 0, -1, -9 },
    { gEnemyNameAirSoldier, gJiminyHeartlessAirSoldierLines, ARRAY_COUNT(gJiminyHeartlessAirSoldierLines), gEmy1800Frame0, gEmy18Palette, gEmy1800Tiles, gCardEmy18Frame0, gEmy18Palette, gCardEmy18Tiles, NULL, NULL, NULL, 0, 0, -1, 3 },
    { gEnemyNameBandit, gJiminyHeartlessBanditLines, ARRAY_COUNT(gJiminyHeartlessBanditLines), gEmy1900Frame5, gEmy19Palette, gEmy1900Tiles, gCardEmy19Frame0, gEmy19Palette, gCardEmy19Tiles, NULL, NULL, NULL, 0, 0, -1, -6 },
    { gEnemyNameFatBandit, gJiminyHeartlessFatBanditLines, ARRAY_COUNT(gJiminyHeartlessFatBanditLines), gEmy3900Frame0, gEmy39Palette, gEmy3900Tiles, gCardEmy39Frame0, gEmy39Palette, gCardEmy39Tiles, NULL, NULL, NULL, 0, 0, -1, 4 },
    { gEnemyNameBarrelSpider, gJiminyHeartlessBarrelSpiderLines, ARRAY_COUNT(gJiminyHeartlessBarrelSpiderLines), gEmy2100Frame0, gEmy21Palette, gEmy2100Tiles, gCardEmy21Frame0, gEmy21Palette, gCardEmy21Tiles, NULL, NULL, NULL, 0, 0, 1, -10 },
    { gEnemyNameSearchGhost, gJiminyHeartlessSearchGhostLines, ARRAY_COUNT(gJiminyHeartlessSearchGhostLines), gEmy2200Frame9, gEmy22Palette, gEmy2200Tiles, gCardEmy22Frame0, gEmy22Palette, gCardEmy22Tiles, NULL, NULL, NULL, 0, 0, 2, 6 },
    { gEnemyNameSeaNeon, gJiminyHeartlessSeaNeonLines, ARRAY_COUNT(gJiminyHeartlessSeaNeonLines), gEmy0600Frame2, gEmy06Palette, gEmy0600Tiles, gCardEmy06Frame0, gEmy06Palette, gCardEmy06Tiles, NULL, NULL, NULL, 0, 0, -2, -3 },
    { gEnemyNameScrewdiver, gJiminyHeartlessScrewdiverLines, ARRAY_COUNT(gJiminyHeartlessScrewdiverLines), gEmy2300Frame0, gEmy23Palette, gEmy2300Tiles, gCardEmy23Frame0, gEmy23Palette, gCardEmy23Tiles, NULL, NULL, NULL, 0, 0, 1, 2 },
    { gEnemyNameAquatank, gJiminyHeartlessAquatankLines, ARRAY_COUNT(gJiminyHeartlessAquatankLines), gEmy4100Frame0, gEmy41Palette, gEmy4100Tiles, gCardEmy41Frame0, gEmy41Palette, gCardEmy41Tiles, NULL, NULL, NULL, 0, 0, -2, 11 },
    { gEnemyNameWightKnight, gJiminyHeartlessWightKnightLines, ARRAY_COUNT(gJiminyHeartlessWightKnightLines), gEmy2500Frame3, gEmy25Palette, gEmy2500Tiles, gCardEmy25Frame0, gEmy25Palette, gCardEmy25Tiles, NULL, NULL, NULL, 0, 0, 4, 0 },
    { gEnemyNameGargoyle, gJiminyHeartlessGargoyleLines, ARRAY_COUNT(gJiminyHeartlessGargoyleLines), gEmy2600Frame0, gEmy26Palette, gEmy2600Tiles, gCardEmy26Frame0, gEmy26Palette, gCardEmy26Tiles, NULL, NULL, NULL, 0, 0, -4, 9 },
    { gEnemyNamePirate, gJiminyHeartlessPirateLines, ARRAY_COUNT(gJiminyHeartlessPirateLines), gEmy2700Frame0, gEmy27Palette, gEmy2700Tiles, gCardEmy27Frame0, gEmy27Palette, gCardEmy27Tiles, NULL, NULL, NULL, 0, 0, 10, -10 },
    { gEnemyNameAirPirate, gJiminyHeartlessAirPirateLines, ARRAY_COUNT(gJiminyHeartlessAirPirateLines), gEmy2800Frame0, gEmy28Palette, gEmy2800Tiles, gCardEmy28Frame0, gEmy28Palette, gCardEmy28Tiles, NULL, NULL, NULL, 0, 0, 5, 5 },
    { gEnemyNameDarkball, gJiminyHeartlessDarkballLines, ARRAY_COUNT(gJiminyHeartlessDarkballLines), gEmy2900Frame0, gEmy29Palette, gEmy2900Tiles, gCardEmy29Frame0, gEmy29Palette, gCardEmy29Tiles, NULL, NULL, NULL, 0, 0, -3, 7 },
    { gEnemyNameDefender, gJiminyHeartlessDefenderLines, ARRAY_COUNT(gJiminyHeartlessDefenderLines), gEmy4400Frame0, gEmy44Palette, gEmy4400Tiles, gCardEmy44Frame0, gEmy44Palette, gCardEmy44Tiles, NULL, NULL, NULL, 0, 0, 3, 1 },
    { gEnemyNameWyvern, gJiminyHeartlessWyvernLines, ARRAY_COUNT(gJiminyHeartlessWyvernLines), gEmy3000Frame0, gEmy30Palette, gEmy3000Tiles, gCardEmy30Frame0, gEmy30Palette, gCardEmy30Tiles, NULL, NULL, NULL, 0, 0, 2, -5 },
    { gEnemyNameWizard, gJiminyHeartlessWizardLines, ARRAY_COUNT(gJiminyHeartlessWizardLines), gEmy3100Frame6, gEmy31Palette, gEmy3100Tiles, gCardEmy31Frame0, gEmy31Palette, gCardEmy31Tiles, NULL, NULL, NULL, 0, 0, 5, -3 },
    { gEnemyNameNeoshadow, gJiminyHeartlessNeoshadowLines, ARRAY_COUNT(gJiminyHeartlessNeoshadowLines), gEmy3700Frame1, gEmy37Palette, gEmy3700Tiles, gCardEmy37Frame0, gEmy37Palette, gCardEmy37Tiles, NULL, NULL, NULL, 0, 0, -1, 4 },
    { gEnemyNameWhiteMushroom, gJiminyHeartlessWhiteMushroomLines, ARRAY_COUNT(gJiminyHeartlessWhiteMushroomLines), gEmy07Fl00Frame0, gEmy07Palette, gEmy07Fl00Tiles, gCardEmy07Frame0, gEmy07Palette, gCardEmy07Tiles, NULL, NULL, NULL, 0, 0, -2, -6 },
    { gEnemyNameBlackFungus, gJiminyHeartlessBlackFungusLines, ARRAY_COUNT(gJiminyHeartlessBlackFungusLines), gEmy07Fl10tFrame7, gEmy07bPalette, gEmy07Fl10tTiles, gCardEmy08Frame0, gEmy07bPalette, gCardEmy08Tiles, NULL, NULL, NULL, 0, 0, -2, -6 },
    { gEnemyNameCreeperPlant, gJiminyHeartlessCreeperPlantLines, ARRAY_COUNT(gJiminyHeartlessCreeperPlantLines), gEmy8300Frame0, gEmy83Palette, gEmy8300Tiles, gCardEmy83Frame0, gEmy83Palette, gCardEmy83Tiles, NULL, NULL, NULL, 0, 0, 0, -4 },
    { gEnemyNameTornadoStep, gJiminyHeartlessTornadoStepLines, ARRAY_COUNT(gJiminyHeartlessTornadoStepLines), gEmy8100Frame0, gEmy81Palette, gEmy8100Tiles, gCardEmy81Frame0, gEmy81Palette, gCardEmy81Tiles, NULL, NULL, NULL, 0, 0, -2, -3 },
    { gEnemyNameCrescendo, gJiminyHeartlessCrescendoLines, ARRAY_COUNT(gJiminyHeartlessCrescendoLines), gEmy8201Frame2, gEmy82Palette, gEmy8201Tiles, gCardEmy82Frame0, gEmy82Palette, gCardEmy82Tiles, NULL, NULL, NULL, 0, 0, 4, -10 },
    { gEnemyNameGuardArmor, gJiminyHeartlessGuardArmorLines, ARRAY_COUNT(gJiminyHeartlessGuardArmorLines), NULL, NULL, NULL, gCardBos01Frame0, gBoss01objPalette, gCardBos01Tiles, gJiminyGuardArmorBgMap, gJiminyGuardArmorBgPalette, gJiminyGuardArmorBgTiles, sizeof(gJiminyGuardArmorBgPalette), sizeof(gJiminyGuardArmorBgTiles), 3, -3 },
    { gEnemyNameParasiteCage, gJiminyHeartlessParasiteCageLines, ARRAY_COUNT(gJiminyHeartlessParasiteCageLines), NULL, NULL, NULL, gCardBos04Frame0, gBosPcBgPalettes, gCardBos04Tiles, gJiminyParasiteCageBgMap, gJiminyParasiteCageBgPalette, gJiminyParasiteCageBgTiles, sizeof(gJiminyParasiteCageBgPalette), sizeof(gJiminyParasiteCageBgTiles), 0, 0 },
    { gEnemyNameTrickmaster, gJiminyHeartlessTrickmasterLines, ARRAY_COUNT(gJiminyHeartlessTrickmasterLines), NULL, NULL, NULL, gCardBos03Frame0, gBoss03objPalettes, gCardBos03Tiles, gJiminyTrickmasterBgMap, gJiminyTrickmasterBgPalette, gJiminyTrickmasterBgTiles, sizeof(gJiminyTrickmasterBgPalette), sizeof(gJiminyTrickmasterBgTiles), 0, 3 },
    { gEnemyNameDarkside, gJiminyHeartlessDarksideLines, ARRAY_COUNT(gJiminyHeartlessDarksideLines), NULL, NULL, NULL, gCardBos00Frame0, gBoss00ObjPalette, gCardBos00Tiles, gJiminyDarksideBgMap, gJiminyDarksideBgPalette, gJiminyDarksideBgTiles, sizeof(gJiminyDarksideBgPalette), sizeof(gJiminyDarksideBgTiles), 0, 0 },
};

static const JiminyDetail sJiminyEntry18Details[14] = {
    { gEnemyNameRiku, gJiminyRikuCharacterRikuLines, ARRAY_COUNT(gJiminyRikuCharacterRikuLines), gRikuFl00Frame0, gRikuPalette, gRikuFl00Tiles, gCardNpcEx03Frame0, gCardNpcEx03Palette, gCardNpcEx03Tiles, NULL, NULL, NULL, 0, 0, -3, 0 },
    { gCardNameKing, gJiminyRikuCharacterKingLines, ARRAY_COUNT(gJiminyRikuCharacterKingLines), gMickeyFl00Frame0, gMickeyPalette, gMickeyFl00Tiles, gCardSmn15Frame0, gMickeyPalette, gCardSmn15Tiles, NULL, NULL, NULL, 0, 0, -2, -6 },
    { gCharacterNameSora, gJiminyRikuCharacterSoraLines, ARRAY_COUNT(gJiminyRikuCharacterSoraLines), gSor1fl00Frame0, gSoraPalette, gSor1fl00Tiles, gCardNpcEx00Frame0, gCardNpcEx00Palette, gCardNpcEx00Tiles, NULL, NULL, NULL, 0, 0, -3, -3 },
    { gCharacterNameKairi, gJiminyRikuCharacterKairiLines, ARRAY_COUNT(gJiminyRikuCharacterKairiLines), gKairF00Frame0, gKairiPalette, gKairF00Tiles, gCardNpcEx05Frame0, gCardNpcEx05Palette, gCardNpcEx05Tiles, NULL, NULL, NULL, 0, 0, -1, -3 },
    { gCharacterNameNamine, gJiminyRikuCharacterNamineLines, ARRAY_COUNT(gJiminyRikuCharacterNamineLines), gNamiF00Frame0, gNaminePalette, gNamiF00Tiles, gCardNpcCom01Frame0, gCardNpcCom01Palette, gCardNpcCom01Tiles, NULL, NULL, NULL, 0, 0, -1, -4 },
    { gCharacterNameRikuReplica, gJiminyRikuCharacterRikuReplicaLines, ARRAY_COUNT(gJiminyRikuCharacterRikuReplicaLines), gNiseFl00Frame0, gNiserikuPalette, gNiseFl00Tiles, gCardBos12Frame0, gCardBos12Palette, gCardBos12Tiles, NULL, NULL, NULL, 0, 0, -3, 0 },
    { gEnemyNameAnsem, gJiminyRikuCharacterAnsemLines, ARRAY_COUNT(gJiminyRikuCharacterAnsemLines), gAnsemFl00Frame0, gAnsemPalette, gAnsemFl00Tiles, gCardBos18Frame0, gCardBos18Palette, gCardBos18Tiles, NULL, NULL, NULL, 0, 0, -3, 9 },
    { gEnemyNameVexen, gJiminyRikuCharacterVexenLines, ARRAY_COUNT(gJiminyRikuCharacterVexenLines), gVixenFl00Frame0, gVixenPalette, gVixenFl00Tiles, gCardBos15Frame0, gCardBos15Palette, gCardBos15Tiles, NULL, NULL, NULL, 0, 0, -3, 6 },
    { gEnemyNameLexaeus, gJiminyRikuCharacterLexaeusLines, ARRAY_COUNT(gJiminyRikuCharacterLexaeusLines), gRexeusFl00Frame0, gRexeusPalette, gRexeusFl00Tiles, gCardBos17Frame0, gCardBos17Palette, gCardBos17Tiles, NULL, NULL, NULL, 0, 0, -5, 14 },
    { gCharacterNameZexion, gJiminyRikuCharacterZexionLines, ARRAY_COUNT(gJiminyRikuCharacterZexionLines), gXexionFl00Frame0, gXexionPalette, gXexionFl00Tiles, gCardNpcCom02Frame0, gCardNpcCom02Palette, gCardNpcCom02Tiles, NULL, NULL, NULL, 0, 0, -1, 6 },
    { gEnemyNameAxel, gJiminyRikuCharacterAxelLines, ARRAY_COUNT(gJiminyRikuCharacterAxelLines), gAcceleFl00Frame0, gAccelePalette, gAcceleFl00Tiles, gCardBos13Frame0, gCardBos13Palette, gCardBos13Tiles, NULL, NULL, NULL, 0, 0, -3, 6 },
    { gEnemyNameMarluxia, gJiminyRikuCharacterMarluxiaLines, ARRAY_COUNT(gJiminyRikuCharacterMarluxiaLines), gMaruxha2Fl00Frame0, gMaruxhaPalette, gMaruxha2Fl00Tiles, gCardBos16Frame0, gCardBos16Palette, gCardBos16Tiles, NULL, NULL, NULL, 0, 0, -3, 6 },
    { gEnemyNameLarxene, gJiminyRikuCharacterLarxeneLines, ARRAY_COUNT(gJiminyRikuCharacterLarxeneLines), gLaxineFl00Frame0, gLaxinePalette, gLaxineFl00Tiles, gCardBos14Frame0, gCardBos14Palette, gCardBos14Tiles, NULL, NULL, NULL, 0, 0, -2, 5 },
    { gCharacterNameDiZ, gJiminyRikuCharacterDiZLines, ARRAY_COUNT(gJiminyRikuCharacterDiZLines), gDizFl00Frame0, gDizPalette, gDizFl00Tiles, gCardNpcCom03Frame0, gCardNpcCom03Palette, gCardNpcCom03Tiles, NULL, NULL, NULL, 0, 0, -2, 8 },
};

static const JiminyDetail sJiminyEntry19Details[6] = {
    { gCharacterNameMaleficent, gJiminyRikuCharacterMaleficentLines, ARRAY_COUNT(gJiminyRikuCharacterMaleficentLines), gMarefF00Frame0, gMarefPalette, gMarefF00Tiles, gCardNpcPc02Frame0, gCardNpcPc02Palette, gCardNpcPc02Tiles, NULL, NULL, NULL, 0, 0, -12, 16 },
    { gCharacterNameJafarGenie, gJiminyRikuCharacterJafarGenieLines, ARRAY_COUNT(gJiminyRikuCharacterJafarGenieLines), NULL, NULL, NULL, gCardBos05Frame0, gCardBos05Palette, gCardBos05Tiles, gJiminyJafarGenieBgMap, gJiminyJafarGenieBgPalette, gJiminyJafarGenieBgTiles, sizeof(gJiminyJafarGenieBgPalette), sizeof(gJiminyJafarGenieBgTiles), 0, 0 },
    { gEnemyNameUrsula, gJiminyRikuCharacterUrsulaLines, ARRAY_COUNT(gJiminyRikuCharacterUrsulaLines), gArthraFl00Frame0, gArthraPalette, gArthraFl00Tiles, gCardBos06Frame0, gCardBos06Palette, gCardBos06Tiles, NULL, NULL, NULL, 0, 0, -3, 6 },
    { gEnemyNameHades, gJiminyRikuCharacterHadesLines, ARRAY_COUNT(gJiminyRikuCharacterHadesLines), gHadesFl00Frame0, gHadesPalette, gHadesFl00Tiles, gCardBos11Frame0, gCardBos11Palette, gCardBos11Tiles, NULL, NULL, NULL, 0, 0, 0, 18 },
    { gEnemyNameOogieBoogie, gJiminyRikuCharacterOogieBoogieLines, ARRAY_COUNT(gJiminyRikuCharacterOogieBoogieLines), gBugiFl00Frame0, gBugiPalette, gBugiFl00Tiles, gCardBos02Frame0, gBoss02objPalette, gCardBos02Tiles, NULL, NULL, NULL, 0, 0, -4, 9 },
    { gCharacterNameHook, gJiminyRikuCharacterHookLines, ARRAY_COUNT(gJiminyRikuCharacterHookLines), gHookF00Frame0, gHookPalette, gHookF00Tiles, gCardBos10Frame0, gCardBos10Palette, gCardBos10Tiles, NULL, NULL, NULL, 0, 0, -1, 17 },
};

static const JiminyDetail sJiminyEntry20Details[33] = {
    { gEnemyNameShadow, gJiminyHeartlessShadowLines, ARRAY_COUNT(gJiminyHeartlessShadowLines), gEmy00L00Frame0, gEmy00Palette, gEmy00L00Tiles, gCardEmy00Frame0, gEmy00Palette, gCardEmy00Tiles, NULL, NULL, NULL, 0, 0, 2, -12 },
    { gEnemyNameSoldier, gJiminyHeartlessSoldierLines, ARRAY_COUNT(gJiminyHeartlessSoldierLines), gEmy14Ll01Frame1, gEmy14Palette, gEmy14Ll01Tiles, gCardEmy14Frame0, gEmy14Palette, gCardEmy14Tiles, NULL, NULL, NULL, 0, 0, -8, 2 },
    { gEnemyNameLargeBody, gJiminyHeartlessLargeBodyLines, ARRAY_COUNT(gJiminyHeartlessLargeBodyLines), gEmy3800Frame0, gEmy38Palette, gEmy3800Tiles, gCardEmy38Frame0, gEmy38Palette, gCardEmy38Tiles, NULL, NULL, NULL, 0, 0, -6, 6 },
    { gEnemyNameRedNocturne, gJiminyHeartlessRedNocturneLines, ARRAY_COUNT(gJiminyHeartlessRedNocturneLines), gEmy01L00Frame16, gEmy01Palette, gEmy01L00Tiles, gCardEmy01Frame0, gEmy01Palette, gCardEmy01Tiles, NULL, NULL, NULL, 0, 0, -2, -8 },
    { gEnemyNameBlueRhapsody, gJiminyHeartlessBlueRhapsodyLines, ARRAY_COUNT(gJiminyHeartlessBlueRhapsodyLines), gEmy01L00Frame3, gEmy02Palette, gEmy01L00Tiles, gCardEmy01Frame0, gEmy02Palette, gCardEmy01Tiles, NULL, NULL, NULL, 0, 0, -1, -11 },
    { gEnemyNameYellowOpera, gJiminyHeartlessYellowOperaLines, ARRAY_COUNT(gJiminyHeartlessYellowOperaLines), gEmy01L00Frame0, gEmy03Palette, gEmy01L00Tiles, gCardEmy01Frame0, gEmy03Palette, gCardEmy01Tiles, NULL, NULL, NULL, 0, 0, -2, -11 },
    { gEnemyNameGreenRequiem, gJiminyHeartlessGreenRequiemLines, ARRAY_COUNT(gJiminyHeartlessGreenRequiemLines), gEmy01L00Frame15, gEmy04Palette, gEmy01L00Tiles, gCardEmy01Frame0, gEmy04Palette, gCardEmy01Tiles, NULL, NULL, NULL, 0, 0, -2, -8 },
    { gEnemyNamePowerwild, gJiminyHeartlessPowerwildLines, ARRAY_COUNT(gJiminyHeartlessPowerwildLines), gEmy1500Frame7, gEmy15Palette, gEmy1500Tiles, gCardEmy15Frame0, gEmy15Palette, gCardEmy15Tiles, NULL, NULL, NULL, 0, 0, 0, -6 },
    { gEnemyNameBouncywild, gJiminyHeartlessBouncywildLines, ARRAY_COUNT(gJiminyHeartlessBouncywildLines), gEmy1600Frame0, gEmy16Palette, gEmy1600Tiles, gCardEmy16Frame0, gEmy16Palette, gCardEmy16Tiles, NULL, NULL, NULL, 0, 0, -1, -9 },
    { gEnemyNameAirSoldier, gJiminyHeartlessAirSoldierLines, ARRAY_COUNT(gJiminyHeartlessAirSoldierLines), gEmy1800Frame0, gEmy18Palette, gEmy1800Tiles, gCardEmy18Frame0, gEmy18Palette, gCardEmy18Tiles, NULL, NULL, NULL, 0, 0, -1, 3 },
    { gEnemyNameBandit, gJiminyHeartlessBanditLines, ARRAY_COUNT(gJiminyHeartlessBanditLines), gEmy1900Frame5, gEmy19Palette, gEmy1900Tiles, gCardEmy19Frame0, gEmy19Palette, gCardEmy19Tiles, NULL, NULL, NULL, 0, 0, -1, -6 },
    { gEnemyNameFatBandit, gJiminyHeartlessFatBanditLines, ARRAY_COUNT(gJiminyHeartlessFatBanditLines), gEmy3900Frame0, gEmy39Palette, gEmy3900Tiles, gCardEmy39Frame0, gEmy39Palette, gCardEmy39Tiles, NULL, NULL, NULL, 0, 0, -1, 4 },
    { gEnemyNameBarrelSpider, gJiminyHeartlessBarrelSpiderLines, ARRAY_COUNT(gJiminyHeartlessBarrelSpiderLines), gEmy2100Frame0, gEmy21Palette, gEmy2100Tiles, gCardEmy21Frame0, gEmy21Palette, gCardEmy21Tiles, NULL, NULL, NULL, 0, 0, 1, -10 },
    { gEnemyNameSearchGhost, gJiminyHeartlessSearchGhostLines, ARRAY_COUNT(gJiminyHeartlessSearchGhostLines), gEmy2200Frame9, gEmy22Palette, gEmy2200Tiles, gCardEmy22Frame0, gEmy22Palette, gCardEmy22Tiles, NULL, NULL, NULL, 0, 0, 2, 6 },
    { gEnemyNameSeaNeon, gJiminyHeartlessSeaNeonLines, ARRAY_COUNT(gJiminyHeartlessSeaNeonLines), gEmy0600Frame2, gEmy06Palette, gEmy0600Tiles, gCardEmy06Frame0, gEmy06Palette, gCardEmy06Tiles, NULL, NULL, NULL, 0, 0, -2, -3 },
    { gEnemyNameScrewdiver, gJiminyHeartlessScrewdiverLines, ARRAY_COUNT(gJiminyHeartlessScrewdiverLines), gEmy2300Frame0, gEmy23Palette, gEmy2300Tiles, gCardEmy23Frame0, gEmy23Palette, gCardEmy23Tiles, NULL, NULL, NULL, 0, 0, 1, 2 },
    { gEnemyNameAquatank, gJiminyHeartlessAquatankLines, ARRAY_COUNT(gJiminyHeartlessAquatankLines), gEmy4100Frame0, gEmy41Palette, gEmy4100Tiles, gCardEmy41Frame0, gEmy41Palette, gCardEmy41Tiles, NULL, NULL, NULL, 0, 0, -2, 11 },
    { gEnemyNameWightKnight, gJiminyHeartlessWightKnightLines, ARRAY_COUNT(gJiminyHeartlessWightKnightLines), gEmy2500Frame3, gEmy25Palette, gEmy2500Tiles, gCardEmy25Frame0, gEmy25Palette, gCardEmy25Tiles, NULL, NULL, NULL, 0, 0, 4, 0 },
    { gEnemyNameGargoyle, gJiminyHeartlessGargoyleLines, ARRAY_COUNT(gJiminyHeartlessGargoyleLines), gEmy2600Frame0, gEmy26Palette, gEmy2600Tiles, gCardEmy26Frame0, gEmy26Palette, gCardEmy26Tiles, NULL, NULL, NULL, 0, 0, -4, 9 },
    { gEnemyNamePirate, gJiminyHeartlessPirateLines, ARRAY_COUNT(gJiminyHeartlessPirateLines), gEmy2700Frame0, gEmy27Palette, gEmy2700Tiles, gCardEmy27Frame0, gEmy27Palette, gCardEmy27Tiles, NULL, NULL, NULL, 0, 0, 10, -10 },
    { gEnemyNameAirPirate, gJiminyHeartlessAirPirateLines, ARRAY_COUNT(gJiminyHeartlessAirPirateLines), gEmy2800Frame0, gEmy28Palette, gEmy2800Tiles, gCardEmy28Frame0, gEmy28Palette, gCardEmy28Tiles, NULL, NULL, NULL, 0, 0, 5, 5 },
    { gEnemyNameDarkball, gJiminyHeartlessDarkballLines, ARRAY_COUNT(gJiminyHeartlessDarkballLines), gEmy2900Frame0, gEmy29Palette, gEmy2900Tiles, gCardEmy29Frame0, gEmy29Palette, gCardEmy29Tiles, NULL, NULL, NULL, 0, 0, -3, 7 },
    { gEnemyNameDefender, gJiminyHeartlessDefenderLines, ARRAY_COUNT(gJiminyHeartlessDefenderLines), gEmy4400Frame0, gEmy44Palette, gEmy4400Tiles, gCardEmy44Frame0, gEmy44Palette, gCardEmy44Tiles, NULL, NULL, NULL, 0, 0, 3, 1 },
    { gEnemyNameWyvern, gJiminyHeartlessWyvernLines, ARRAY_COUNT(gJiminyHeartlessWyvernLines), gEmy3000Frame0, gEmy30Palette, gEmy3000Tiles, gCardEmy30Frame0, gEmy30Palette, gCardEmy30Tiles, NULL, NULL, NULL, 0, 0, 2, -5 },
    { gEnemyNameWizard, gJiminyHeartlessWizardLines, ARRAY_COUNT(gJiminyHeartlessWizardLines), gEmy3100Frame6, gEmy31Palette, gEmy3100Tiles, gCardEmy31Frame0, gEmy31Palette, gCardEmy31Tiles, NULL, NULL, NULL, 0, 0, 5, -3 },
    { gEnemyNameNeoshadow, gJiminyHeartlessNeoshadowLines, ARRAY_COUNT(gJiminyHeartlessNeoshadowLines), gEmy3700Frame1, gEmy37Palette, gEmy3700Tiles, gCardEmy37Frame0, gEmy37Palette, gCardEmy37Tiles, NULL, NULL, NULL, 0, 0, -1, 4 },
    { gEnemyNameCreeperPlant, gJiminyHeartlessCreeperPlantLines, ARRAY_COUNT(gJiminyHeartlessCreeperPlantLines), gEmy8300Frame0, gEmy83Palette, gEmy8300Tiles, gCardEmy83Frame0, gEmy83Palette, gCardEmy83Tiles, NULL, NULL, NULL, 0, 0, 0, -4 },
    { gEnemyNameTornadoStep, gJiminyHeartlessTornadoStepLines, ARRAY_COUNT(gJiminyHeartlessTornadoStepLines), gEmy8100Frame0, gEmy81Palette, gEmy8100Tiles, gCardEmy81Frame0, gEmy81Palette, gCardEmy81Tiles, NULL, NULL, NULL, 0, 0, -2, -3 },
    { gEnemyNameCrescendo, gJiminyHeartlessCrescendoLines, ARRAY_COUNT(gJiminyHeartlessCrescendoLines), gEmy8201Frame2, gEmy82Palette, gEmy8201Tiles, gCardEmy82Frame0, gEmy82Palette, gCardEmy82Tiles, NULL, NULL, NULL, 0, 0, 4, -10 },
    { gEnemyNameGuardArmor, gJiminyRikuHeartlessGuardArmorLines, ARRAY_COUNT(gJiminyRikuHeartlessGuardArmorLines), NULL, NULL, NULL, gCardBos01Frame0, gBoss01objPalette, gCardBos01Tiles, gJiminyGuardArmorBgMap, gJiminyGuardArmorBgPalette, gJiminyGuardArmorBgTiles, sizeof(gJiminyGuardArmorBgPalette), sizeof(gJiminyGuardArmorBgTiles), 3, -3 },
    { gEnemyNameParasiteCage, gJiminyRikuHeartlessParasiteCageLines, ARRAY_COUNT(gJiminyRikuHeartlessParasiteCageLines), NULL, NULL, NULL, gCardBos04Frame0, gBosPcBgPalettes, gCardBos04Tiles, gJiminyParasiteCageBgMap, gJiminyParasiteCageBgPalette, gJiminyParasiteCageBgTiles, sizeof(gJiminyParasiteCageBgPalette), sizeof(gJiminyParasiteCageBgTiles), 0, 0 },
    { gEnemyNameTrickmaster, gJiminyRikuHeartlessTrickmasterLines, ARRAY_COUNT(gJiminyRikuHeartlessTrickmasterLines), NULL, NULL, NULL, gCardBos03Frame0, gBoss03objPalettes, gCardBos03Tiles, gJiminyTrickmasterBgMap, gJiminyTrickmasterBgPalette, gJiminyTrickmasterBgTiles, sizeof(gJiminyTrickmasterBgPalette), sizeof(gJiminyTrickmasterBgTiles), 0, 3 },
    { gEnemyNameDarkside, gJiminyRikuHeartlessDarksideLines, ARRAY_COUNT(gJiminyRikuHeartlessDarksideLines), NULL, NULL, NULL, gCardBos00Frame0, gBoss00ObjPalette, gCardBos00Tiles, gJiminyDarksideBgMap, gJiminyDarksideBgPalette, gJiminyDarksideBgTiles, sizeof(gJiminyDarksideBgPalette), sizeof(gJiminyDarksideBgTiles), 0, 0 },
};

#elif defined(VERSION_EU)

static const JiminyDetail sJiminyEntry01Details[17] = {
    { &gJiminyStoryTale1NameByLanguage, &gJiminyStoryTale1Text, gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gJiminyStoryTale2NameByLanguage, &gJiminyStoryTale2Text, gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gJiminyStoryTale3NameByLanguage, &gJiminyStoryTale3Text, gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gJiminyStoryTale4NameByLanguage, &gJiminyStoryTale4Text, gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gWorldNameTraverseTownByLanguage, &gJiminyStoryTraverseTownText, gWorldImageTraverseTownFrame0, gWorldImageTraverseTownPalette, gWorldImageTraverseTownTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gWorldNameWonderlandByLanguage, &gJiminyStoryWonderlandText, gWorldImageWonderlandFrame0, gWorldImageWonderlandPalette, gWorldImageWonderlandTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gWorldNameOlympusColiseumByLanguage, &gJiminyStoryOlympusColiseumText, gWorldImageOlympusColiseumFrame0, gWorldImageOlympusColiseumPalette, gWorldImageOlympusColiseumTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gWorldNameAgrabahByLanguage, &gJiminyStoryAgrabahText, gWorldImageAgrabahFrame0, gWorldImageAgrabahPalette, gWorldImageAgrabahTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gWorldNameHalloweenTownByLanguage, &gJiminyStoryHalloweenTownText, gWorldImageHalloweenTownFrame0, gWorldImageHalloweenTownPalette, gWorldImageHalloweenTownTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gWorldNameMonstroByLanguage, &gJiminyStoryMonstroText, gWorldImageMonstroFrame0, gWorldImageMonstroPalette, gWorldImageMonstroTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, -8, 0 },
    { &gWorldNameAtlanticaByLanguage, &gJiminyStoryAtlanticaText, gWorldImageAtlanticaFrame0, gWorldImageAtlanticaPalette, gWorldImageAtlanticaTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gWorldNameNeverLandByLanguage, &gJiminyStoryNeverLandText, gWorldImageNeverLandFrame0, gWorldImageNeverLandPalette, gWorldImageNeverLandTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gWorldNameHollowBastionByLanguage, &gJiminyStoryHollowBastionText, gWorldImageHollowBastionFrame0, gWorldImageHollowBastionPalette, gWorldImageHollowBastionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gWorldName100AcreWoodByLanguage, &gJiminyStory100AcreWoodText, gWorldImage100AcreWoodFrame0, gWorldImage100AcreWoodPalette, gWorldImage100AcreWoodTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gWorldNameTwilightTownByLanguage, &gJiminyStoryTwilightTownText, gWorldImageTwilightTownFrame0, gWorldImageTwilightTownPalette, gWorldImageTwilightTownTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gWorldNameDestinyIslandsByLanguage, &gJiminyStoryDestinyIslandsText, gWorldImageDestinyIslandsFrame0, gWorldImageDestinyIslandsPalette, gWorldImageDestinyIslandsTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gWorldNameCastleOblivionByLanguage, &gJiminyStoryCastleOblivionText, gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry15Details[6] = {
    { &gJiminyRikuStoryTale1NameByLanguage, &gJiminyRikuStoryTale1Text, gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gJiminyRikuStoryTale2NameByLanguage, &gJiminyRikuStoryTale2Text, gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gJiminyRikuStoryTale3NameByLanguage, &gJiminyRikuStoryTale3Text, gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gJiminyRikuStoryTale4NameByLanguage, &gJiminyRikuStoryTale4Text, gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gJiminyRikuStoryTale5NameByLanguage, &gJiminyRikuStoryTale5Text, gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gJiminyRikuStoryTale6NameByLanguage, &gJiminyRikuStoryTale6Text, gWorldImageCastleOblivionFrame0, gWorldImageCastleOblivionPalette, gWorldImageCastleOblivionTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry04Details[17] = {
    { &gCardNameKingdomKeyByLanguage, &gJiminyAttackCardKingdomKeyText, gCardWep01Frame0, gCardWep01Palette, gCardWep01Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameThreeWishesByLanguage, &gJiminyAttackCardThreeWishesText, gCardWep04Frame0, gCardWep04Palette, gCardWep04Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameCrabclawByLanguage, &gJiminyAttackCardCrabclawText, gCardWep05Frame0, gCardWep05Palette, gCardWep05Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNamePumpkinheadByLanguage, &gJiminyAttackCardPumpkinheadText, gCardWep06Frame0, gCardWep06Palette, gCardWep06Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameFairyHarpByLanguage, &gJiminyAttackCardFairyHarpText, gCardWep07Frame0, gCardWep07Palette, gCardWep07Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameWishingStarByLanguage, &gJiminyAttackCardWishingStarText, gCardWep08Frame0, gCardWep08Palette, gCardWep08Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameSpellbinderByLanguage, &gJiminyAttackCardSpellbinderText, gCardWep09Frame0, gCardWep09Palette, gCardWep09Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameMetalChocoboByLanguage, &gJiminyAttackCardMetalChocoboText, gCardWep10Frame0, gCardWep10Palette, gCardWep10Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameOlympiaByLanguage, &gJiminyAttackCardOlympiaText, gCardWep03Frame0, gCardWep03Palette, gCardWep03Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameLionheartByLanguage, &gJiminyAttackCardLionheartText, gCardWep11Frame0, gCardWep11Palette, gCardWep11Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameLadyLuckByLanguage, &gJiminyAttackCardLadyLuckText, gCardWep12Frame0, gCardWep12Palette, gCardWep12Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameDivineRoseByLanguage, &gJiminyAttackCardDivineRoseText, gCardWep13Frame0, gCardWep13Palette, gCardWep13Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameOathkeeperByLanguage, &gJiminyAttackCardOathkeeperText, gCardWep14Frame0, gCardWep14Palette, gCardWep14Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameOblivionByLanguage, &gJiminyAttackCardOblivionText, gCardWep15Frame0, gCardWep15Palette, gCardWep15Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameDiamondDustByLanguage, &gJiminyAttackCardDiamondDustText, gCardWep18Frame0, gCardWep18Palette, gCardWep18Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameOneWingedAngelByLanguage, &gJiminyAttackCardOneWingedAngelText, gCardWep19Frame0, gCardWep19Palette, gCardWep19Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameUltimaWeaponByLanguage, &gJiminyAttackCardUltimaWeaponText, gCardWep16Frame0, gCardWep16Palette, gCardWep16Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry05Details[14] = {
    { &gCardNameFireByLanguage, &gJiminyMagicCardFireText, gCardMgc01Frame0, gCardMgc01Palette, gCardMgc01Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameBlizzardByLanguage, &gJiminyMagicCardBlizzardText, gCardMgc02Frame0, gCardMgc02Palette, gCardMgc02Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameThunderByLanguage, &gJiminyMagicCardThunderText, gCardMgc03Frame0, gCardMgc03Palette, gCardMgc03Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameCureByLanguage, &gJiminyMagicCardCureText, gCardMgc04Frame0, gCardMgc04Palette, gCardMgc04Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameGravityByLanguage, &gJiminyMagicCardGravityText, gCardMgc05Frame0, gCardMgc05Palette, gCardMgc05Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameStopByLanguage, &gJiminyMagicCardStopText, gCardMgc06Frame0, gCardMgc06Palette, gCardMgc06Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameAeroByLanguage, &gJiminyMagicCardAeroText, gCardMgc07Frame0, gCardMgc07Palette, gCardMgc07Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameSimbaByLanguage, &gJiminyMagicCardSimbaText, gCardSmn03Frame0, gCardSmn03Palette, gCardSmn03Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameDumboByLanguage, &gJiminyMagicCardDumboText, gCardSmn06Frame0, gCardSmn06Palette, gCardSmn06Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameBambiByLanguage, &gJiminyMagicCardBambiText, gCardSmn05Frame0, gBanbPalette, gCardSmn05Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameMushuByLanguage, &gJiminyMagicCardMushuText, gCardSmn08Frame0, gMushuPalette, gCardSmn08Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameGenieByLanguage, &gJiminyMagicCardGenieText, gCardSmn04Frame0, gCardSmn04Palette, gCardSmn04Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameTinkerBellByLanguage, &gJiminyMagicCardTinkerBellText, gCardSmn07Frame0, gTinkPalette, gCardSmn07Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameCloudByLanguage, &gJiminyMagicCardCloudText, gCardSmn09Frame0, gCardSmn09Palette, gCardSmn09Tiles, gCardBorderBlueFrame0, gCard00Palette, gCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry06Details[7] = {
    { &gCardNamePotionByLanguage, &gJiminyItemCardPotionText, gCardItm01Frame0, gCardItm01Palette, gCardItm01Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameHiPotionByLanguage, &gJiminyItemCardHiPotionText, gCardItm02Frame0, gCardItm02Palette, gCardItm02Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameMegaPotionByLanguage, &gJiminyItemCardMegaPotionText, gCardItm03Frame0, gCardItm03Palette, gCardItm03Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameEtherByLanguage, &gJiminyItemCardEtherText, gCardItm04Frame0, gCardItm04Palette, gCardItm04Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameMegaEtherByLanguage, &gJiminyItemCardMegaEtherText, gCardItm05Frame0, gCardItm05Palette, gCardItm05Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameElixirByLanguage, &gJiminyItemCardElixirText, gCardItm06Frame0, gCardItm06Palette, gCardItm06Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameMegalixirByLanguage, &gJiminyItemCardMegalixirText, gCardItm07Frame0, gCardItm07Palette, gCardItm07Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry07Details[7] = {
    { &gCardNameDonaldDuckByLanguage, &gJiminyFriendCardDonaldDuckText, gCardSmn02Frame0, gDonaldPalette, gCardSmn02Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameGoofyByLanguage, &gJiminyFriendCardGoofyText, gCardSmn01Frame0, gGoofyPalette, gCardSmn01Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameAladdinByLanguage, &gJiminyFriendCardAladdinText, gCardSmn10Frame0, gAladdinPalette, gCardSmn10Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameJackByLanguage, &gJiminyFriendCardJackText, gCardSmn12Frame0, gJackPalette, gCardSmn12Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameArielByLanguage, &gJiminyFriendCardArielText, gCardSmn11Frame0, gArielPalette, gCardSmn11Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNamePeterPanByLanguage, &gJiminyFriendCardPeterPanText, gCardSmn13Frame0, gPeterPalette, gCardSmn13Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameBeastByLanguage, &gJiminyFriendCardBeastText, gCardSmn14Frame0, gBeastPalette, gCardSmn14Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry08Details[49] = {
    { &gEnemyNameShadowByLanguage, &gJiminyEnemyCardShadowText, gCardEmy00Frame0, gEmy00Palette, gCardEmy00Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameSoldierByLanguage, &gJiminyEnemyCardSoldierText, gCardEmy14Frame0, gEmy14Palette, gCardEmy14Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameLargeBodyByLanguage, &gJiminyEnemyCardLargeBodyText, gCardEmy38Frame0, gEmy38Palette, gCardEmy38Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameRedNocturneByLanguage, &gJiminyEnemyCardRedNocturneText, gCardEmy01Frame0, gEmy01Palette, gCardEmy01Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameBlueRhapsodyByLanguage, &gJiminyEnemyCardBlueRhapsodyText, gBlueRhapsodyCardFrame0, gEmy02Palette, gBlueRhapsodyCardTiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameYellowOperaByLanguage, &gJiminyEnemyCardYellowOperaText, gYellowOperaCardFrame0, gEmy03Palette, gYellowOperaCardTiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameGreenRequiemByLanguage, &gJiminyEnemyCardGreenRequiemText, gGreenRequiemCardFrame0, gEmy04Palette, gGreenRequiemCardTiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNamePowerwildByLanguage, &gJiminyEnemyCardPowerwildText, gCardEmy15Frame0, gEmy15Palette, gCardEmy15Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameBouncywildByLanguage, &gJiminyEnemyCardBouncywildText, gCardEmy16Frame0, gEmy16Palette, gCardEmy16Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameAirSoldierByLanguage, &gJiminyEnemyCardAirSoldierText, gCardEmy18Frame0, gEmy18Palette, gCardEmy18Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameBanditByLanguage, &gJiminyEnemyCardBanditText, gCardEmy19Frame0, gEmy19Palette, gCardEmy19Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameFatBanditByLanguage, &gJiminyEnemyCardFatBanditText, gCardEmy39Frame0, gEmy39Palette, gCardEmy39Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameBarrelSpiderByLanguage, &gJiminyEnemyCardBarrelSpiderText, gCardEmy21Frame0, gEmy21Palette, gCardEmy21Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameSearchGhostByLanguage, &gJiminyEnemyCardSearchGhostText, gCardEmy22Frame0, gEmy22Palette, gCardEmy22Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameSeaNeonByLanguage, &gJiminyEnemyCardSeaNeonText, gCardEmy06Frame0, gEmy06Palette, gCardEmy06Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameScrewdiverByLanguage, &gJiminyEnemyCardScrewdiverText, gCardEmy23Frame0, gEmy23Palette, gCardEmy23Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameAquatankByLanguage, &gJiminyEnemyCardAquatankText, gCardEmy41Frame0, gEmy41Palette, gCardEmy41Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameWightKnightByLanguage, &gJiminyEnemyCardWightKnightText, gCardEmy25Frame0, gEmy25Palette, gCardEmy25Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameGargoyleByLanguage, &gJiminyEnemyCardGargoyleText, gCardEmy26Frame0, gEmy26Palette, gCardEmy26Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNamePirateByLanguage, &gJiminyEnemyCardPirateText, gCardEmy27Frame0, gEmy27Palette, gCardEmy27Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameAirPirateByLanguage, &gJiminyEnemyCardAirPirateText, gCardEmy28Frame0, gEmy28Palette, gCardEmy28Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameDarkballByLanguage, &gJiminyEnemyCardDarkballText, gCardEmy29Frame0, gEmy29Palette, gCardEmy29Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameDefenderByLanguage, &gJiminyEnemyCardDefenderText, gCardEmy44Frame0, gEmy44Palette, gCardEmy44Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameWyvernByLanguage, &gJiminyEnemyCardWyvernText, gCardEmy30Frame0, gEmy30Palette, gCardEmy30Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameWizardByLanguage, &gJiminyEnemyCardWizardText, gCardEmy31Frame0, gEmy31Palette, gCardEmy31Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameNeoshadowByLanguage, &gJiminyEnemyCardNeoshadowText, gCardEmy37Frame0, gEmy37Palette, gCardEmy37Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameWhiteMushroomByLanguage, &gJiminyEnemyCardWhiteMushroomText, gCardEmy07Frame0, gEmy07Palette, gCardEmy07Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameBlackFungusByLanguage, &gJiminyEnemyCardBlackFungusText, gCardEmy08Frame0, gEmy07bPalette, gCardEmy08Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameCreeperPlantByLanguage, &gJiminyEnemyCardCreeperPlantText, gCardEmy83Frame0, gEmy83Palette, gCardEmy83Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameTornadoStepByLanguage, &gJiminyEnemyCardTornadoStepText, gCardEmy81Frame0, gEmy81Palette, gCardEmy81Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameCrescendoByLanguage, &gJiminyEnemyCardCrescendoText, gCardEmy82Frame0, gEmy82Palette, gCardEmy82Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameGuardArmorByLanguage, &gJiminyEnemyCardGuardArmorText, gCardBos01Frame0, gBoss01objPalette, gCardBos01Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameParasiteCageByLanguage, &gJiminyEnemyCardParasiteCageText, gCardBos04Frame0, gBosPcBgPalettes, gCardBos04Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameTrickmasterByLanguage, &gJiminyEnemyCardTrickmasterText, gCardBos03Frame0, gBoss03objPalettes, gCardBos03Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameDarksideByLanguage, &gJiminyEnemyCardDarksideText, gCardBos00Frame0, gBoss00ObjPalette, gCardBos00Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameCardSoldierByLanguage, &gJiminyEnemyCardCardSoldierText, gCardNpcAw04Frame0, gTrumpHPalette, gCardNpcAw04Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameHadesByLanguage, &gJiminyEnemyCardHadesText, gCardBos11Frame0, gCardBos11Palette, gCardBos11Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameJafarByLanguage, &gJiminyEnemyCardJafarText, gCardBos05Frame0, gCardBos05Palette, gCardBos05Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameOogieBoogieByLanguage, &gJiminyEnemyCardOogieBoogieText, gCardBos02Frame0, gBoss02objPalette, gCardBos02Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameUrsulaByLanguage, &gJiminyEnemyCardUrsulaText, gCardBos06Frame0, gCardBos06Palette, gCardBos06Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameHookByLanguage, &gJiminyEnemyCardHookText, gCardBos10Frame0, gCardBos10Palette, gCardBos10Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameDragonMaleficentByLanguage, &gJiminyEnemyCardDragonMaleficentText, gCardBos07Frame0, gCardBos07Palette, gCardBos07Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameRikuByLanguage, &gJiminyEnemyCardRikuText, gCardBos12Frame0, gCardBos12Palette, gCardBos12Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameAxelByLanguage, &gJiminyEnemyCardAxelText, gCardBos13Frame0, gCardBos13Palette, gCardBos13Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameLarxeneByLanguage, &gJiminyEnemyCardLarxeneText, gCardBos14Frame0, gCardBos14Palette, gCardBos14Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameVexenByLanguage, &gJiminyEnemyCardVexenText, gCardBos15Frame0, gCardBos15Palette, gCardBos15Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameMarluxiaByLanguage, &gJiminyEnemyCardMarluxiaText, gCardBos16Frame0, gCardBos16Palette, gCardBos16Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameLexaeusByLanguage, &gJiminyEnemyCardLexaeusText, gCardBos17Frame0, gCardBos17Palette, gCardBos17Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameAnsemByLanguage, &gJiminyEnemyCardAnsemText, gCardBos18Frame0, gCardBos18Palette, gCardBos18Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry09Details[26] = {
    { &gRoomNameTranquilDarknessByLanguage, &gJiminyMapCardTranquilDarknessText, gCardRoom02Frame0, gCardRoom02Palette, gCardRoom02Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gRoomNameTeemingDarknessByLanguage, &gJiminyMapCardTeemingDarknessText, gCardRoom01Frame0, gCardRoom01Palette, gCardRoom01Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gRoomNameFeebleDarknessByLanguage, &gJiminyMapCardFeebleDarknessText, gCardRoom07Frame0, gCardRoom07Palette, gCardRoom07Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gRoomNameAlmightyDarknessByLanguage, &gJiminyMapCardAlmightyDarknessText, gCardRoom08Frame0, gCardRoom08Palette, gCardRoom08Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gRoomNameSleepingDarknessByLanguage, &gJiminyMapCardSleepingDarknessText, gCardRoom05Frame0, gCardRoom05Palette, gCardRoom05Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gRoomNameLoomingDarknessByLanguage, &gJiminyMapCardLoomingDarknessText, gCardRoom04Frame0, gCardRoom04Palette, gCardRoom04Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gRoomNamePremiumRoomByLanguage, &gJiminyMapCardPremiumRoomText, gCardRoom20Frame0, gCardRoom20Palette, gCardRoom20Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gRoomNameWhiteRoomByLanguage, &gJiminyMapCardWhiteRoomText, gCardRoom21Frame0, gCardRoom21Palette, gCardRoom21Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gRoomNameBlackRoomByLanguage, &gJiminyMapCardBlackRoomText, gCardRoom22Frame0, gCardRoom22Palette, gCardRoom22Tiles, gMapCardBorderRedFrame0, gMapCardBorderPalettes, gMapCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gRoomNameMartialWakingByLanguage, &gJiminyMapCardMartialWakingText, gCardRoom13Frame0, gCardRoom13Palette, gCardRoom13Tiles, gMapCardBorderGreenFrame0, gMapCardBorderPalettes, gMapCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gRoomNameSorcerousWakingByLanguage, &gJiminyMapCardSorcerousWakingText, gCardRoom12Frame0, gCardRoom12Palette, gCardRoom12Tiles, gMapCardBorderGreenFrame0, gMapCardBorderPalettes, gMapCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gRoomNameAlchemicWakingByLanguage, &gJiminyMapCardAlchemicWakingText, gCardRoom14Frame0, gCardRoom14Palette, gCardRoom14Tiles, gMapCardBorderGreenFrame0, gMapCardBorderPalettes, gMapCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gRoomNameMeetingGroundByLanguage, &gJiminyMapCardMeetingGroundText, gCardRoom15Frame0, gCardRoom15Palette, gCardRoom15Tiles, gMapCardBorderGreenFrame0, gMapCardBorderPalettes, gMapCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gRoomNameStagnantSpaceByLanguage, &gJiminyMapCardStagnantSpaceText, gCardRoom19Frame0, gCardRoom19Palette, gCardRoom19Tiles, gMapCardBorderGreenFrame0, gMapCardBorderPalettes, gMapCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gRoomNameStrongInitiativeByLanguage, &gJiminyMapCardStrongInitiativeText, gCardRoom17Frame0, gCardRoom17Palette, gCardRoom17Tiles, gMapCardBorderGreenFrame0, gMapCardBorderPalettes, gMapCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gRoomNameLastingDazeByLanguage, &gJiminyMapCardLastingDazeText, gCardRoom18Frame0, gCardRoom18Palette, gCardRoom18Tiles, gMapCardBorderGreenFrame0, gMapCardBorderPalettes, gMapCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gRoomNameCalmBountyByLanguage, &gJiminyMapCardCalmBountyText, gCardRoom09Frame0, gCardRoom09Palette, gCardRoom09Tiles, gMapCardBorderBlueFrame0, gMapCardBorderPalettes, gMapCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gRoomNameGuardedTroveByLanguage, &gJiminyMapCardGuardedTroveText, gCardRoom03Frame0, gCardRoom03Palette, gCardRoom03Tiles, gMapCardBorderBlueFrame0, gMapCardBorderPalettes, gMapCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gRoomNameFalseBountyByLanguage, &gJiminyMapCardFalseBountyText, gCardRoom10Frame0, gCardRoom10Palette, gCardRoom10Tiles, gMapCardBorderBlueFrame0, gMapCardBorderPalettes, gMapCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gRoomNameMomentsReprieveByLanguage, &gJiminyMapCardMomentsReprieveText, gCardRoom06Frame0, gCardRoom06Palette, gCardRoom06Tiles, gMapCardBorderBlueFrame0, gMapCardBorderPalettes, gMapCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gRoomNameMinglingWorldsByLanguage, &gJiminyMapCardMinglingWorldsText, gCardRoom16Frame0, gCardRoom16Palette, gCardRoom16Tiles, gMapCardBorderBlueFrame0, gMapCardBorderPalettes, gMapCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gRoomNameMoogleRoomByLanguage, &gJiminyMapCardMoogleRoomText, gCardRoom11Frame0, gCardRoom11Palette, gCardRoom11Tiles, gMapCardBorderBlueFrame0, gMapCardBorderPalettes, gMapCardBorderBlueTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gRoomNameKeyOfBeginningsByLanguage, &gJiminyMapCardKeyOfBeginningsText, gCardEve00Frame0, gCardEve00Palette, gCardEve00Tiles, gMapCardBorderBlackFrame0, gMapCardBorderPalettes, gMapCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gRoomNameKeyOfGuidanceByLanguage, &gJiminyMapCardKeyOfGuidanceText, gCardEve01Frame0, gCardEve01Palette, gCardEve01Tiles, gMapCardBorderBlackFrame0, gMapCardBorderPalettes, gMapCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gRoomNameKeyToTruthByLanguage, &gJiminyMapCardKeyToTruthText, gCardEve02Frame0, gCardEve02Palette, gCardEve02Tiles, gMapCardBorderBlackFrame0, gMapCardBorderPalettes, gMapCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gRoomNameKeyToRewardsByLanguage, &gJiminyMapCardKeyToRewardsText, gCardRoom23Frame0, gCardRoom23Palette, gCardRoom23Tiles, gMapCardBorderBlackFrame0, gMapCardBorderPalettes, gMapCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry10Details[1] = {
    { &gJiminyPremiumCardsNameByLanguage, &gJiminyPremiumCardsText, gJiminyPremiumCardFrame0, gJiminyPremiumCardPalette, gJiminyPremiumCardTiles, NULL, NULL, NULL, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry16Details[22] = {
    { &gCardNameSoulEaterByLanguage, &gJiminyRikuCardSoulEaterText, gCardWep20Frame0, gCardWep20Palette, gCardWep20Tiles, gCardBorderRedFrame0, gCard00Palette, gCardBorderRedTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameKingByLanguage, &gJiminyRikuCardKingText, gCardSmn15Frame0, gMickeyPalette, gCardSmn15Tiles, gCardBorderGreenFrame0, gCard00Palette, gCardBorderGreenTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameShadowByLanguage, &gJiminyEnemyCardShadowText, gCardEmy00Frame0, gEmy00Palette, gCardEmy00Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameLargeBodyByLanguage, &gJiminyEnemyCardLargeBodyText, gCardEmy38Frame0, gEmy38Palette, gCardEmy38Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNamePowerwildByLanguage, &gJiminyEnemyCardPowerwildText, gCardEmy15Frame0, gEmy15Palette, gCardEmy15Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameFatBanditByLanguage, &gJiminyEnemyCardFatBanditText, gCardEmy39Frame0, gEmy39Palette, gCardEmy39Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameSearchGhostByLanguage, &gJiminyEnemyCardSearchGhostText, gCardEmy22Frame0, gEmy22Palette, gCardEmy22Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameSeaNeonByLanguage, &gJiminyEnemyCardSeaNeonText, gCardEmy06Frame0, gEmy06Palette, gCardEmy06Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameWightKnightByLanguage, &gJiminyEnemyCardWightKnightText, gCardEmy25Frame0, gEmy25Palette, gCardEmy25Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNamePirateByLanguage, &gJiminyEnemyCardPirateText, gCardEmy27Frame0, gEmy27Palette, gCardEmy27Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameDefenderByLanguage, &gJiminyEnemyCardDefenderText, gCardEmy44Frame0, gEmy44Palette, gCardEmy44Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameGuardArmorByLanguage, &gJiminyEnemyCardGuardArmorText, gCardBos01Frame0, gBoss01objPalette, gCardBos01Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameParasiteCageByLanguage, &gJiminyEnemyCardParasiteCageText, gCardBos04Frame0, gBosPcBgPalettes, gCardBos04Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameTrickmasterByLanguage, &gJiminyEnemyCardTrickmasterText, gCardBos03Frame0, gBoss03objPalettes, gCardBos03Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameDarksideByLanguage, &gJiminyEnemyCardDarksideText, gCardBos00Frame0, gBoss00ObjPalette, gCardBos00Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameHadesByLanguage, &gJiminyEnemyCardHadesText, gCardBos11Frame0, gCardBos11Palette, gCardBos11Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameJafarByLanguage, &gJiminyEnemyCardJafarText, gCardBos05Frame0, gCardBos05Palette, gCardBos05Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameOogieBoogieByLanguage, &gJiminyEnemyCardOogieBoogieText, gCardBos02Frame0, gBoss02objPalette, gCardBos02Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameUrsulaByLanguage, &gJiminyEnemyCardUrsulaText, gCardBos06Frame0, gCardBos06Palette, gCardBos06Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameHookByLanguage, &gJiminyEnemyCardHookText, gCardBos10Frame0, gCardBos10Palette, gCardBos10Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameDragonMaleficentByLanguage, &gJiminyEnemyCardDragonMaleficentText, gCardBos07Frame0, gCardBos07Palette, gCardBos07Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gEnemyNameLexaeusByLanguage, &gJiminyEnemyCardLexaeusText, gCardBos17Frame0, gCardBos17Palette, gCardBos17Tiles, gCardBorderBlackFrame0, gCard00Palette, gCardBorderBlackTiles, NULL, NULL, NULL, 0, 0, 0, 0 },
};

static const JiminyDetail sJiminyEntry11Details[25] = {
    { &gCharacterNameSoraByLanguage, &gJiminyCharacterSoraText, gSor1fl00Frame0, gSoraPalette, gSor1fl00Tiles, gCardNpcEx00Frame0, gCardNpcEx00Palette, gCardNpcEx00Tiles, NULL, NULL, NULL, 0, 0, -3, -3 },
    { &gCardNameDonaldDuckByLanguage, &gJiminyCharacterDonaldDuckText, gDonaFl00Frame0, gDonaldPalette, gDonaFl00Tiles, gCardSmn02Frame0, gDonaldPalette, gCardSmn02Tiles, NULL, NULL, NULL, 0, 0, 0, -9 },
    { &gCardNameGoofyByLanguage, &gJiminyCharacterGoofyText, gGoofyFl00Frame0, gGoofyPalette, gGoofyFl00Tiles, gCardSmn01Frame0, gGoofyPalette, gCardSmn01Tiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCharacterNameJiminyCricketByLanguage, &gJiminyCharacterJiminyCricketText, gJimFl00Frame0, gJiminyPalette, gJimFl00Tiles, gCardNpcEx01Frame0, gCardNpcEx01Palette, gCardNpcEx01Tiles, NULL, NULL, NULL, 0, 0, -3, -11 },
    { &gEnemyNameRikuByLanguage, &gJiminyCharacterRikuText, gRikuFl00Frame0, gRikuPalette, gRikuFl00Tiles, gCardNpcEx03Frame0, gCardNpcEx03Palette, gCardNpcEx03Tiles, NULL, NULL, NULL, 0, 0, -3, 0 },
    { &gCharacterNameKairiByLanguage, &gJiminyCharacterKairiText, gKairF00Frame0, gKairiPalette, gKairF00Tiles, gCardNpcEx05Frame0, gCardNpcEx05Palette, gCardNpcEx05Tiles, NULL, NULL, NULL, 0, 0, -1, -3 },
    { &gCardNameSimbaByLanguage, &gJiminyCharacterSimbaText, gShinba10Frame0, gShinbaPalette, gShinba10Tiles, gCardSmn03Frame0, gCardSmn03Palette, gCardSmn03Tiles, NULL, NULL, NULL, 0, 0, -2, -7 },
    { &gCardNameDumboByLanguage, &gJiminyCharacterDumboText, gSmnDumboFrame0, gDamboPalette, gSmnDumboTiles, gCardSmn06Frame0, gCardSmn06Palette, gCardSmn06Tiles, NULL, NULL, NULL, 0, 0, 8, -7 },
    { &gCardNameBambiByLanguage, &gJiminyCharacterBambiText, gBanb00Frame7, gBanbPalette, gBanb00Tiles, gCardSmn05Frame0, gBanbPalette, gCardSmn05Tiles, NULL, NULL, NULL, 0, 0, -2, -7 },
    { &gCardNameMushuByLanguage, &gJiminyCharacterMushuText, gMushuF00Frame1, gMushuPalette, gMushuF00Tiles, gCardSmn08Frame0, gMushuPalette, gCardSmn08Tiles, NULL, NULL, NULL, 0, 0, 0, -12 },
    { &gCharacterNameMooglesByLanguage, &gJiminyCharacterMooglesText, gMoguFl00Frame0, gMoguPalette, gMoguFl00Tiles, gCardNpcEx02Frame0, gCardNpcEx02Palette, gCardNpcEx02Tiles, NULL, NULL, NULL, 0, 0, -2, -8 },
    { &gCharacterNameLeonByLanguage, &gJiminyCharacterLeonText, gReonFl00Frame0, gReonPalette, gReonFl00Tiles, gCardNpcEx09Frame0, gCardNpcEx09Palette, gCardNpcEx09Tiles, NULL, NULL, NULL, 0, 0, -3, 7 },
    { &gCharacterNameYuffieByLanguage, &gJiminyCharacterYuffieText, gYuffieFl00Frame0, gYuffiePalette, gYuffieFl00Tiles, gCardNpcEx10Frame0, gCardNpcEx10Palette, gCardNpcEx10Tiles, NULL, NULL, NULL, 0, 0, -1, 2 },
    { &gCharacterNameAerithByLanguage, &gJiminyCharacterAerithText, gEarF00Frame0, gEarisPalette, gEarF00Tiles, gCardNpcEx06Frame0, gCardNpcEx06Palette, gCardNpcEx06Tiles, NULL, NULL, NULL, 0, 0, -3, 4 },
    { &gCharacterNameCidByLanguage, &gJiminyCharacterCidText, gShidoF00Frame0, gShidoPalette, gShidoF00Tiles, gCardNpcEx07Frame0, gCardNpcEx07Palette, gCardNpcEx07Tiles, NULL, NULL, NULL, 0, 0, -2, 5 },
    { &gCardNameCloudByLanguage, &gJiminyCharacterCloudText, gCroudF00Frame0, gCroudPalette, gCroudF00Tiles, gCardSmn09Frame0, gCardSmn09Palette, gCardSmn09Tiles, NULL, NULL, NULL, 0, 0, -3, 2 },
    { &gCharacterNameTidusByLanguage, &gJiminyCharacterTidusText, gTidusFl00Frame0, gTidusPalette, gTidusFl00Tiles, gCardNpcDi01Frame0, gCardNpcDi01Palette, gCardNpcDi01Tiles, NULL, NULL, NULL, 0, 0, -2, -3 },
    { &gCharacterNameWakkaByLanguage, &gJiminyCharacterWakkaText, gWakkaF00Frame0, gWakkaPalette, gWakkaF00Tiles, gCardNpcDi02Frame0, gCardNpcDi02Palette, gCardNpcDi02Tiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCharacterNameSelphieByLanguage, &gJiminyCharacterSelphieText, gSelphieFl00Frame0, gSelphiePalette, gSelphieFl00Tiles, gCardNpcDi03Frame0, gCardNpcDi03Palette, gCardNpcDi03Tiles, NULL, NULL, NULL, 0, 0, -2, -3 },
    { &gCharacterNameNamineByLanguage, &gJiminyCharacterNamineText, gNamiF00Frame0, gNaminePalette, gNamiF00Tiles, gCardNpcCom01Frame0, gCardNpcCom01Palette, gCardNpcCom01Tiles, NULL, NULL, NULL, 0, 0, -1, -4 },
    { &gCharacterNameRikuReplicaByLanguage, &gJiminyCharacterRikuReplicaText, gNiseFl00Frame0, gNiserikuPalette, gNiseFl00Tiles, gCardBos12Frame0, gCardBos12Palette, gCardBos12Tiles, NULL, NULL, NULL, 0, 0, -3, 0 },
    { &gEnemyNameAxelByLanguage, &gJiminyCharacterAxelText, gAcceleFl00Frame0, gAccelePalette, gAcceleFl00Tiles, gCardBos13Frame0, gCardBos13Palette, gCardBos13Tiles, NULL, NULL, NULL, 0, 0, -3, 6 },
    { &gEnemyNameLarxeneByLanguage, &gJiminyCharacterLarxeneText, gLaxineFl00Frame0, gLaxinePalette, gLaxineFl00Tiles, gCardBos14Frame0, gCardBos14Palette, gCardBos14Tiles, NULL, NULL, NULL, 0, 0, -2, 5 },
    { &gEnemyNameVexenByLanguage, &gJiminyCharacterVexenText, gVixenFl00Frame0, gVixenPalette, gVixenFl00Tiles, gCardBos15Frame0, gCardBos15Palette, gCardBos15Tiles, NULL, NULL, NULL, 0, 0, -3, 6 },
    { &gEnemyNameMarluxiaByLanguage, &gJiminyCharacterMarluxiaText, gMaruxha2Fl00Frame0, gMaruxhaPalette, gMaruxha2Fl00Tiles, gCardBos16Frame0, gCardBos16Palette, gCardBos16Tiles, NULL, NULL, NULL, 0, 0, -3, 6 },
};

static const JiminyDetail sJiminyEntry12Details[40] = {
    { &gCharacterNameAliceByLanguage, &gJiminyCharacterAliceText, gAliceFl00Frame0, gAlicePalette, gAliceFl00Tiles, gCardNpcAw01Frame0, gCardNpcAw01Palette, gCardNpcAw01Tiles, NULL, NULL, NULL, 0, 0, -1, -1 },
    { &gCharacterNameQueenOfHeartsByLanguage, &gJiminyCharacterQueenOfHeartsText, gQenF00Frame0, gQeenPalette, gQenF00Tiles, gCardNpcAw02Frame0, gCardNpcAw02Palette, gCardNpcAw02Tiles, NULL, NULL, NULL, 0, 0, -3, 4 },
    { &gCharacterNameWhiteRabbitByLanguage, &gJiminyCharacterWhiteRabbitText, gUsagif00Frame0, gUsagi00Palette, gUsagif00Tiles, gCardNpcAw05Frame0, gCardNpcAw05Palette, gCardNpcAw05Tiles, NULL, NULL, NULL, 0, 0, 0, -10 },
    { &gCharacterNameCardOfHeartsByLanguage, &gJiminyCharacterCardOfHeartsText, gTrumpH00Frame0, gTrumpHPalette, gTrumpH00Tiles, gCardNpcAw04Frame0, gTrumpHPalette, gCardNpcAw04Tiles, NULL, NULL, NULL, 0, 0, 0, 2 },
    { &gCharacterNameCardOfSpadesByLanguage, &gJiminyCharacterCardOfSpadesText, gTrumpS00Frame0, gTrumpSPalette, gTrumpS00Tiles, gCardNpcAw03Frame0, gTrumpSPalette, gCardNpcAw03Tiles, NULL, NULL, NULL, 0, 0, 0, 2 },
    { &gCharacterNameCheshireCatByLanguage, &gJiminyCharacterCheshireCatText, gCheshireFrame0, gCheshirePalette, gCheshireTiles, gCardNpcAw06Frame0, gCardNpcAw06Palette, gCardNpcAw06Tiles, NULL, NULL, NULL, 0, 0, -4, -12 },
    { &gCharacterNameHerculesByLanguage, &gJiminyCharacterHerculesText, gHeraclesFl00Frame0, gHeraclesPalette, gHeraclesFl00Tiles, gCardNpcHe02Frame0, gCardNpcHe02Palette, gCardNpcHe02Tiles, NULL, NULL, NULL, 0, 0, 0, 13 },
    { &gCharacterNamePhiloctetesByLanguage, &gJiminyCharacterPhiloctetesText, gPhilFl00Frame0, gPhilPalette, gPhilFl00Tiles, gCardNpcHe01Frame0, gCardNpcHe01Palette, gCardNpcHe01Tiles, NULL, NULL, NULL, 0, 0, -2, -10 },
    { &gEnemyNameHadesByLanguage, &gJiminyCharacterHadesText, gHadesFl00Frame0, gHadesPalette, gHadesFl00Tiles, gCardBos11Frame0, gCardBos11Palette, gCardBos11Tiles, NULL, NULL, NULL, 0, 0, 0, 18 },
    { &gCardNameAladdinByLanguage, &gJiminyCharacterAladdinText, gAladdin00Frame0, gAladdinPalette, gAladdin00Tiles, gCardSmn10Frame0, gAladdinPalette, gCardSmn10Tiles, NULL, NULL, NULL, 0, 0, -2, 6 },
    { &gCardNameGenieByLanguage, &gJiminyCharacterGenieText, gGenie01Frame2, gGeniePalette, gGenie01Tiles, gCardSmn04Frame0, gCardSmn04Palette, gCardSmn04Tiles, NULL, NULL, NULL, 0, 0, 2, 1 },
    { &gCharacterNameJasmineByLanguage, &gJiminyCharacterJasmineText, gJasmineF00Frame0, gJasminePalette, gJasmineF00Tiles, gCardNpcAl01Frame0, gCardNpcAl01Palette, gCardNpcAl01Tiles, NULL, NULL, NULL, 0, 0, -4, 5 },
    { &gCharacterNameIagoByLanguage, &gJiminyCharacterIagoText, gIagoFrame0, gIagoPalette, gIagoTiles, gCardNpcAl03Frame0, gCardNpcAl03Palette, gCardNpcAl03Tiles, NULL, NULL, NULL, 0, 0, -2, -10 },
    { &gEnemyNameJafarByLanguage, &gJiminyCharacterJafarText, gJafferFl00Frame0, gJafferPalette, gJafferFl00Tiles, gCardNpcAl02Frame0, gCardNpcAl02Palette, gCardNpcAl02Tiles, NULL, NULL, NULL, 0, 0, -3, 19 },
    { &gCharacterNameJafarGenieByLanguage, &gJiminyCharacterJafarGenieText, NULL, NULL, NULL, gCardBos05Frame0, gCardBos05Palette, gCardBos05Tiles, gJiminyJafarGenieBgMap, gJiminyJafarGenieBgPalette, gJiminyJafarGenieBgTiles, sizeof(gJiminyJafarGenieBgPalette), sizeof(gJiminyJafarGenieBgTiles), 0, 0 },
    { &gCardNameJackByLanguage, &gJiminyCharacterJackText, gJackFl00Frame0, gJackPalette, gJackFl00Tiles, gCardSmn12Frame0, gJackPalette, gCardSmn12Tiles, NULL, NULL, NULL, 0, 0, 2, 11 },
    { &gCharacterNameSallyByLanguage, &gJiminyCharacterSallyText, gSariFl00Frame0, gSariPalette, gSariFl00Tiles, gCardNpcNm01Frame0, gCardNpcNm01Palette, gCardNpcNm01Tiles, NULL, NULL, NULL, 0, 0, -2, 7 },
    { &gCharacterNameDrFinkelsteinByLanguage, &gJiminyCharacterDrFinkelsteinText, gFinklF00Frame0, gFinklPalette, gFinklF00Tiles, gCardNpcNm02Frame0, gCardNpcNm02Palette, gCardNpcNm02Tiles, NULL, NULL, NULL, 0, 0, -1, -7 },
    { &gEnemyNameOogieBoogieByLanguage, &gJiminyCharacterOogieBoogieText, gBugiFl00Frame0, gBugiPalette, gBugiFl00Tiles, gCardBos02Frame0, gBoss02objPalette, gCardBos02Tiles, NULL, NULL, NULL, 0, 0, -4, 9 },
    { &gCharacterNamePinocchioByLanguage, &gJiminyCharacterPinocchioText, gPinoF00Frame0, gPinokioPalette, gPinoF00Tiles, gCardNpcPi01Frame0, gCardNpcPi01Palette, gCardNpcPi01Tiles, NULL, NULL, NULL, 0, 0, -2, -8 },
    { &gCharacterNameGeppettoByLanguage, &gJiminyCharacterGeppettoText, gGeppettoF00Frame0, gGeppettoPalette, gGeppettoF00Tiles, gCardNpcPi02Frame0, gCardNpcPi02Palette, gCardNpcPi02Tiles, NULL, NULL, NULL, 0, 0, 0, 0 },
    { &gCardNameArielByLanguage, &gJiminyCharacterArielText, gArielF00Frame0, gArielPalette, gArielF00Tiles, gCardSmn11Frame0, gArielPalette, gCardSmn11Tiles, NULL, NULL, NULL, 0, 0, -4, 3 },
    { &gCharacterNameSebastianByLanguage, &gJiminyCharacterSebastianText, gSebastianFl00Frame0, gSebastianPalette, gSebastianFl00Tiles, gCardNpcLm01Frame0, gCardNpcLm01Palette, gCardNpcLm01Tiles, NULL, NULL, NULL, 0, 0, -1, -13 },
    { &gCharacterNameFlounderByLanguage, &gJiminyCharacterFlounderText, gFlounderFl00Frame0, gFlounderPalette, gFlounderFl00Tiles, gCardNpcLm02Frame0, gCardNpcLm02Palette, gCardNpcLm02Tiles, NULL, NULL, NULL, 0, 0, -1, -17 },
    { &gEnemyNameUrsulaByLanguage, &gJiminyCharacterUrsulaText, gArthraFl00Frame0, gArthraPalette, gArthraFl00Tiles, gCardBos06Frame0, gCardBos06Palette, gCardBos06Tiles, NULL, NULL, NULL, 0, 0, -3, 6 },
    { &gCardNamePeterPanByLanguage, &gJiminyCharacterPeterPanText, gPeterFl00Frame0, gPeterPalette, gPeterFl00Tiles, gCardSmn13Frame0, gPeterPalette, gCardSmn13Tiles, NULL, NULL, NULL, 0, 0, -3, 5 },
    { &gCardNameTinkerBellByLanguage, &gJiminyCharacterTinkerBellText, gTinkF00Frame2, gTinkPalette, gTinkF00Tiles, gCardSmn07Frame0, gTinkPalette, gCardSmn07Tiles, NULL, NULL, NULL, 0, 0, -5, -10 },
    { &gCharacterNameWendyByLanguage, &gJiminyCharacterWendyText, gWendyFl00Frame0, gWendyPalette, gWendyFl00Tiles, gCardNpcPp01Frame0, gCardNpcPp01Palette, gCardNpcPp01Tiles, NULL, NULL, NULL, 0, 0, -1, 4 },
    { &gCharacterNameHookByLanguage, &gJiminyCharacterHookText, gHookF00Frame0, gHookPalette, gHookF00Tiles, gCardBos10Frame0, gCardBos10Palette, gCardBos10Tiles, NULL, NULL, NULL, 0, 0, -1, 17 },
    { &gCardNameBeastByLanguage, &gJiminyCharacterBeastText, gBeastFl00Frame0, gBeastPalette, gBeastFl00Tiles, gCardSmn14Frame0, gBeastPalette, gCardSmn14Tiles, NULL, NULL, NULL, 0, 0, -1, 10 },
    { &gCharacterNameBelleByLanguage, &gJiminyCharacterBelleText, gBellFl00Frame0, gBellPalette, gBellFl00Tiles, gCardNpcPc01Frame0, gCardNpcPc01Palette, gCardNpcPc01Tiles, NULL, NULL, NULL, 0, 0, -4, 5 },
    { &gCharacterNameMaleficentByLanguage, &gJiminyCharacterMaleficentText, gMarefF00Frame0, gMarefPalette, gMarefF00Tiles, gCardNpcPc02Frame0, gCardNpcPc02Palette, gCardNpcPc02Tiles, NULL, NULL, NULL, 0, 0, -12, 16 },
    { &gEnemyNameDragonMaleficentByLanguage, &gJiminyCharacterDragonMaleficentText, NULL, NULL, NULL, gCardBos07Frame0, gCardBos07Palette, gCardBos07Tiles, gJiminyDragonMaleficentBgMap, gJiminyDragonMaleficentBgPalette, gJiminyDragonMaleficentBgTiles, sizeof(gJiminyDragonMaleficentBgPalette), sizeof(gJiminyDragonMaleficentBgTiles), 0, 0 },
    { &gCharacterNameWinnieThePoohByLanguage, &gJiminyCharacterWinnieThePoohText, gPoohFl06Frame0, gPoohPalette, gPoohFl06Tiles, gCardNpcPo01Frame0, gCardNpcPo01Palette, gCardNpcPo01Tiles, NULL, NULL, NULL, 0, 0, -2, -6 },
    { &gCharacterNamePigletByLanguage, &gJiminyCharacterPigletText, gPigletStandFrontFrame0, gPigletPalette, gPigletStandFrontTiles, gCardNpcPo03Frame0, gCardNpcPo03Palette, gCardNpcPo03Tiles, NULL, NULL, NULL, 0, 0, -2, -10 },
    { &gCharacterNameOwlByLanguage, &gJiminyCharacterOwlText, gOwlFl00Frame0, gOwlPalette, gOwlFl00Tiles, gCardNpcPo06Frame0, gCardNpcPo06Palette, gCardNpcPo06Tiles, NULL, NULL, NULL, 0, 0, -3, -7 },
    { &gCharacterNameRooByLanguage, &gJiminyCharacterRooText, gRooFl00Frame0, gRooPalette, gRooFl00Tiles, gCardNpcPo07Frame0, gCardNpcPo07Palette, gCardNpcPo07Tiles, NULL, NULL, NULL, 0, 0, -1, -13 },
    { &gCharacterNameEeyoreByLanguage, &gJiminyCharacterEeyoreText, gEeyoreFl00Frame0, gEeyorePalette, gEeyoreFl00Tiles, gCardNpcPo04Frame0, gCardNpcPo04Palette, gCardNpcPo04Tiles, NULL, NULL, NULL, 0, 0, 5, -13 },
    { &gCharacterNameTiggerByLanguage, &gJiminyCharacterTiggerText, gTiggerFl00Frame0, gTiggerPalette, gTiggerFl00Tiles, gCardNpcPo02Frame0, gCardNpcPo02Palette, gCardNpcPo02Tiles, NULL, NULL, NULL, 0, 0, -3, -3 },
    { &gCharacterNameRabbitByLanguage, &gJiminyCharacterRabbitText, gRabbitFl00Frame0, gRabbitPalettes, gRabbitFl00Tiles, gCardNpcPo05Frame0, gCardNpcPo05Palette, gCardNpcPo05Tiles, NULL, NULL, NULL, 0, 0, -1, -3 },
};

static const JiminyDetail sJiminyEntry13Details[35] = {
    { &gEnemyNameShadowByLanguage, &gJiminyHeartlessShadowText, gEmy00L00Frame0, gEmy00Palette, gEmy00L00Tiles, gCardEmy00Frame0, gEmy00Palette, gCardEmy00Tiles, NULL, NULL, NULL, 0, 0, 2, -12 },
    { &gEnemyNameSoldierByLanguage, &gJiminyHeartlessSoldierText, gEmy14Ll01Frame1, gEmy14Palette, gEmy14Ll01Tiles, gCardEmy14Frame0, gEmy14Palette, gCardEmy14Tiles, NULL, NULL, NULL, 0, 0, -8, 2 },
    { &gEnemyNameLargeBodyByLanguage, &gJiminyHeartlessLargeBodyText, gEmy3800Frame0, gEmy38Palette, gEmy3800Tiles, gCardEmy38Frame0, gEmy38Palette, gCardEmy38Tiles, NULL, NULL, NULL, 0, 0, -6, 6 },
    { &gEnemyNameRedNocturneByLanguage, &gJiminyHeartlessRedNocturneText, gEmy01L00Frame16, gEmy01Palette, gEmy01L00Tiles, gCardEmy01Frame0, gEmy01Palette, gCardEmy01Tiles, NULL, NULL, NULL, 0, 0, -2, -8 },
    { &gEnemyNameBlueRhapsodyByLanguage, &gJiminyHeartlessBlueRhapsodyText, gEmy01L00Frame3, gEmy02Palette, gEmy01L00Tiles, gCardEmy01Frame0, gEmy02Palette, gCardEmy01Tiles, NULL, NULL, NULL, 0, 0, -1, -11 },
    { &gEnemyNameYellowOperaByLanguage, &gJiminyHeartlessYellowOperaText, gEmy01L00Frame0, gEmy03Palette, gEmy01L00Tiles, gCardEmy01Frame0, gEmy03Palette, gCardEmy01Tiles, NULL, NULL, NULL, 0, 0, -2, -11 },
    { &gEnemyNameGreenRequiemByLanguage, &gJiminyHeartlessGreenRequiemText, gEmy01L00Frame15, gEmy04Palette, gEmy01L00Tiles, gCardEmy01Frame0, gEmy04Palette, gCardEmy01Tiles, NULL, NULL, NULL, 0, 0, -2, -8 },
    { &gEnemyNamePowerwildByLanguage, &gJiminyHeartlessPowerwildText, gEmy1500Frame7, gEmy15Palette, gEmy1500Tiles, gCardEmy15Frame0, gEmy15Palette, gCardEmy15Tiles, NULL, NULL, NULL, 0, 0, 0, -6 },
    { &gEnemyNameBouncywildByLanguage, &gJiminyHeartlessBouncywildText, gEmy1600Frame0, gEmy16Palette, gEmy1600Tiles, gCardEmy16Frame0, gEmy16Palette, gCardEmy16Tiles, NULL, NULL, NULL, 0, 0, -1, -9 },
    { &gEnemyNameAirSoldierByLanguage, &gJiminyHeartlessAirSoldierText, gEmy1800Frame0, gEmy18Palette, gEmy1800Tiles, gCardEmy18Frame0, gEmy18Palette, gCardEmy18Tiles, NULL, NULL, NULL, 0, 0, -1, 3 },
    { &gEnemyNameBanditByLanguage, &gJiminyHeartlessBanditText, gEmy1900Frame5, gEmy19Palette, gEmy1900Tiles, gCardEmy19Frame0, gEmy19Palette, gCardEmy19Tiles, NULL, NULL, NULL, 0, 0, -1, -6 },
    { &gEnemyNameFatBanditByLanguage, &gJiminyHeartlessFatBanditText, gEmy3900Frame0, gEmy39Palette, gEmy3900Tiles, gCardEmy39Frame0, gEmy39Palette, gCardEmy39Tiles, NULL, NULL, NULL, 0, 0, -1, 4 },
    { &gEnemyNameBarrelSpiderByLanguage, &gJiminyHeartlessBarrelSpiderText, gEmy2100Frame0, gEmy21Palette, gEmy2100Tiles, gCardEmy21Frame0, gEmy21Palette, gCardEmy21Tiles, NULL, NULL, NULL, 0, 0, 1, -10 },
    { &gEnemyNameSearchGhostByLanguage, &gJiminyHeartlessSearchGhostText, gEmy2200Frame9, gEmy22Palette, gEmy2200Tiles, gCardEmy22Frame0, gEmy22Palette, gCardEmy22Tiles, NULL, NULL, NULL, 0, 0, 2, 6 },
    { &gEnemyNameSeaNeonByLanguage, &gJiminyHeartlessSeaNeonText, gEmy0600Frame2, gEmy06Palette, gEmy0600Tiles, gCardEmy06Frame0, gEmy06Palette, gCardEmy06Tiles, NULL, NULL, NULL, 0, 0, -2, -3 },
    { &gEnemyNameScrewdiverByLanguage, &gJiminyHeartlessScrewdiverText, gEmy2300Frame0, gEmy23Palette, gEmy2300Tiles, gCardEmy23Frame0, gEmy23Palette, gCardEmy23Tiles, NULL, NULL, NULL, 0, 0, 1, 2 },
    { &gEnemyNameAquatankByLanguage, &gJiminyHeartlessAquatankText, gEmy4100Frame0, gEmy41Palette, gEmy4100Tiles, gCardEmy41Frame0, gEmy41Palette, gCardEmy41Tiles, NULL, NULL, NULL, 0, 0, -2, 11 },
    { &gEnemyNameWightKnightByLanguage, &gJiminyHeartlessWightKnightText, gEmy2500Frame3, gEmy25Palette, gEmy2500Tiles, gCardEmy25Frame0, gEmy25Palette, gCardEmy25Tiles, NULL, NULL, NULL, 0, 0, 4, 0 },
    { &gEnemyNameGargoyleByLanguage, &gJiminyHeartlessGargoyleText, gEmy2600Frame0, gEmy26Palette, gEmy2600Tiles, gCardEmy26Frame0, gEmy26Palette, gCardEmy26Tiles, NULL, NULL, NULL, 0, 0, -4, 9 },
    { &gEnemyNamePirateByLanguage, &gJiminyHeartlessPirateText, gEmy2700Frame0, gEmy27Palette, gEmy2700Tiles, gCardEmy27Frame0, gEmy27Palette, gCardEmy27Tiles, NULL, NULL, NULL, 0, 0, 10, -10 },
    { &gEnemyNameAirPirateByLanguage, &gJiminyHeartlessAirPirateText, gEmy2800Frame0, gEmy28Palette, gEmy2800Tiles, gCardEmy28Frame0, gEmy28Palette, gCardEmy28Tiles, NULL, NULL, NULL, 0, 0, 5, 5 },
    { &gEnemyNameDarkballByLanguage, &gJiminyHeartlessDarkballText, gEmy2900Frame0, gEmy29Palette, gEmy2900Tiles, gCardEmy29Frame0, gEmy29Palette, gCardEmy29Tiles, NULL, NULL, NULL, 0, 0, -3, 7 },
    { &gEnemyNameDefenderByLanguage, &gJiminyHeartlessDefenderText, gEmy4400Frame0, gEmy44Palette, gEmy4400Tiles, gCardEmy44Frame0, gEmy44Palette, gCardEmy44Tiles, NULL, NULL, NULL, 0, 0, 3, 1 },
    { &gEnemyNameWyvernByLanguage, &gJiminyHeartlessWyvernText, gEmy3000Frame0, gEmy30Palette, gEmy3000Tiles, gCardEmy30Frame0, gEmy30Palette, gCardEmy30Tiles, NULL, NULL, NULL, 0, 0, 2, -5 },
    { &gEnemyNameWizardByLanguage, &gJiminyHeartlessWizardText, gEmy3100Frame6, gEmy31Palette, gEmy3100Tiles, gCardEmy31Frame0, gEmy31Palette, gCardEmy31Tiles, NULL, NULL, NULL, 0, 0, 5, -3 },
    { &gEnemyNameNeoshadowByLanguage, &gJiminyHeartlessNeoshadowText, gEmy3700Frame1, gEmy37Palette, gEmy3700Tiles, gCardEmy37Frame0, gEmy37Palette, gCardEmy37Tiles, NULL, NULL, NULL, 0, 0, -1, 4 },
    { &gEnemyNameWhiteMushroomByLanguage, &gJiminyHeartlessWhiteMushroomText, gEmy07Fl00Frame0, gEmy07Palette, gEmy07Fl00Tiles, gCardEmy07Frame0, gEmy07Palette, gCardEmy07Tiles, NULL, NULL, NULL, 0, 0, -2, -6 },
    { &gEnemyNameBlackFungusByLanguage, &gJiminyHeartlessBlackFungusText, gEmy07Fl10tFrame7, gEmy07bPalette, gEmy07Fl10tTiles, gCardEmy08Frame0, gEmy07bPalette, gCardEmy08Tiles, NULL, NULL, NULL, 0, 0, -2, -6 },
    { &gEnemyNameCreeperPlantByLanguage, &gJiminyHeartlessCreeperPlantText, gEmy8300Frame0, gEmy83Palette, gEmy8300Tiles, gCardEmy83Frame0, gEmy83Palette, gCardEmy83Tiles, NULL, NULL, NULL, 0, 0, 0, -4 },
    { &gEnemyNameTornadoStepByLanguage, &gJiminyHeartlessTornadoStepText, gEmy8100Frame0, gEmy81Palette, gEmy8100Tiles, gCardEmy81Frame0, gEmy81Palette, gCardEmy81Tiles, NULL, NULL, NULL, 0, 0, -2, -3 },
    { &gEnemyNameCrescendoByLanguage, &gJiminyHeartlessCrescendoText, gEmy8201Frame2, gEmy82Palette, gEmy8201Tiles, gCardEmy82Frame0, gEmy82Palette, gCardEmy82Tiles, NULL, NULL, NULL, 0, 0, 4, -10 },
    { &gEnemyNameGuardArmorByLanguage, &gJiminyHeartlessGuardArmorText, NULL, NULL, NULL, gCardBos01Frame0, gBoss01objPalette, gCardBos01Tiles, gJiminyGuardArmorBgMap, gJiminyGuardArmorBgPalette, gJiminyGuardArmorBgTiles, sizeof(gJiminyGuardArmorBgPalette), sizeof(gJiminyGuardArmorBgTiles), 3, -3 },
    { &gEnemyNameParasiteCageByLanguage, &gJiminyHeartlessParasiteCageText, NULL, NULL, NULL, gCardBos04Frame0, gBosPcBgPalettes, gCardBos04Tiles, gJiminyParasiteCageBgMap, gJiminyParasiteCageBgPalette, gJiminyParasiteCageBgTiles, sizeof(gJiminyParasiteCageBgPalette), sizeof(gJiminyParasiteCageBgTiles), 0, 0 },
    { &gEnemyNameTrickmasterByLanguage, &gJiminyHeartlessTrickmasterText, NULL, NULL, NULL, gCardBos03Frame0, gBoss03objPalettes, gCardBos03Tiles, gJiminyTrickmasterBgMap, gJiminyTrickmasterBgPalette, gJiminyTrickmasterBgTiles, sizeof(gJiminyTrickmasterBgPalette), sizeof(gJiminyTrickmasterBgTiles), 0, 3 },
    { &gEnemyNameDarksideByLanguage, &gJiminyHeartlessDarksideText, NULL, NULL, NULL, gCardBos00Frame0, gBoss00ObjPalette, gCardBos00Tiles, gJiminyDarksideBgMap, gJiminyDarksideBgPalette, gJiminyDarksideBgTiles, sizeof(gJiminyDarksideBgPalette), sizeof(gJiminyDarksideBgTiles), 0, 0 },
};

static const JiminyDetail sJiminyEntry18Details[14] = {
    { &gEnemyNameRikuByLanguage, &gJiminyRikuCharacterRikuText, gRikuFl00Frame0, gRikuPalette, gRikuFl00Tiles, gCardNpcEx03Frame0, gCardNpcEx03Palette, gCardNpcEx03Tiles, NULL, NULL, NULL, 0, 0, -3, 0 },
    { &gCardNameKingByLanguage, &gJiminyRikuCharacterKingText, gMickeyFl00Frame0, gMickeyPalette, gMickeyFl00Tiles, gCardSmn15Frame0, gMickeyPalette, gCardSmn15Tiles, NULL, NULL, NULL, 0, 0, -2, -6 },
    { &gCharacterNameSoraByLanguage, &gJiminyRikuCharacterSoraText, gSor1fl00Frame0, gSoraPalette, gSor1fl00Tiles, gCardNpcEx00Frame0, gCardNpcEx00Palette, gCardNpcEx00Tiles, NULL, NULL, NULL, 0, 0, -3, -3 },
    { &gCharacterNameKairiByLanguage, &gJiminyRikuCharacterKairiText, gKairF00Frame0, gKairiPalette, gKairF00Tiles, gCardNpcEx05Frame0, gCardNpcEx05Palette, gCardNpcEx05Tiles, NULL, NULL, NULL, 0, 0, -1, -3 },
    { &gCharacterNameNamineByLanguage, &gJiminyRikuCharacterNamineText, gNamiF00Frame0, gNaminePalette, gNamiF00Tiles, gCardNpcCom01Frame0, gCardNpcCom01Palette, gCardNpcCom01Tiles, NULL, NULL, NULL, 0, 0, -1, -4 },
    { &gCharacterNameRikuReplicaByLanguage, &gJiminyRikuCharacterRikuReplicaText, gNiseFl00Frame0, gNiserikuPalette, gNiseFl00Tiles, gCardBos12Frame0, gCardBos12Palette, gCardBos12Tiles, NULL, NULL, NULL, 0, 0, -3, 0 },
    { &gEnemyNameAnsemByLanguage, &gJiminyRikuCharacterAnsemText, gAnsemFl00Frame0, gAnsemPalette, gAnsemFl00Tiles, gCardBos18Frame0, gCardBos18Palette, gCardBos18Tiles, NULL, NULL, NULL, 0, 0, -3, 9 },
    { &gEnemyNameVexenByLanguage, &gJiminyRikuCharacterVexenText, gVixenFl00Frame0, gVixenPalette, gVixenFl00Tiles, gCardBos15Frame0, gCardBos15Palette, gCardBos15Tiles, NULL, NULL, NULL, 0, 0, -3, 6 },
    { &gEnemyNameLexaeusByLanguage, &gJiminyRikuCharacterLexaeusText, gRexeusFl00Frame0, gRexeusPalette, gRexeusFl00Tiles, gCardBos17Frame0, gCardBos17Palette, gCardBos17Tiles, NULL, NULL, NULL, 0, 0, -5, 14 },
    { &gCharacterNameZexionByLanguage, &gJiminyRikuCharacterZexionText, gXexionFl00Frame0, gXexionPalette, gXexionFl00Tiles, gCardNpcCom02Frame0, gCardNpcCom02Palette, gCardNpcCom02Tiles, NULL, NULL, NULL, 0, 0, -1, 6 },
    { &gEnemyNameAxelByLanguage, &gJiminyRikuCharacterAxelText, gAcceleFl00Frame0, gAccelePalette, gAcceleFl00Tiles, gCardBos13Frame0, gCardBos13Palette, gCardBos13Tiles, NULL, NULL, NULL, 0, 0, -3, 6 },
    { &gEnemyNameMarluxiaByLanguage, &gJiminyRikuCharacterMarluxiaText, gMaruxha2Fl00Frame0, gMaruxhaPalette, gMaruxha2Fl00Tiles, gCardBos16Frame0, gCardBos16Palette, gCardBos16Tiles, NULL, NULL, NULL, 0, 0, -3, 6 },
    { &gEnemyNameLarxeneByLanguage, &gJiminyRikuCharacterLarxeneText, gLaxineFl00Frame0, gLaxinePalette, gLaxineFl00Tiles, gCardBos14Frame0, gCardBos14Palette, gCardBos14Tiles, NULL, NULL, NULL, 0, 0, -2, 5 },
    { &gCharacterNameDiZByLanguage, &gJiminyRikuCharacterDiZText, gDizFl00Frame0, gDizPalette, gDizFl00Tiles, gCardNpcCom03Frame0, gCardNpcCom03Palette, gCardNpcCom03Tiles, NULL, NULL, NULL, 0, 0, -2, 8 },
};

static const JiminyDetail sJiminyEntry19Details[6] = {
    { &gCharacterNameMaleficentByLanguage, &gJiminyRikuCharacterMaleficentText, gMarefF00Frame0, gMarefPalette, gMarefF00Tiles, gCardNpcPc02Frame0, gCardNpcPc02Palette, gCardNpcPc02Tiles, NULL, NULL, NULL, 0, 0, -12, 16 },
    { &gCharacterNameJafarGenieByLanguage, &gJiminyRikuCharacterJafarGenieText, NULL, NULL, NULL, gCardBos05Frame0, gCardBos05Palette, gCardBos05Tiles, gJiminyJafarGenieBgMap, gJiminyJafarGenieBgPalette, gJiminyJafarGenieBgTiles, sizeof(gJiminyJafarGenieBgPalette), sizeof(gJiminyJafarGenieBgTiles), 0, 0 },
    { &gEnemyNameUrsulaByLanguage, &gJiminyRikuCharacterUrsulaText, gArthraFl00Frame0, gArthraPalette, gArthraFl00Tiles, gCardBos06Frame0, gCardBos06Palette, gCardBos06Tiles, NULL, NULL, NULL, 0, 0, -3, 6 },
    { &gEnemyNameHadesByLanguage, &gJiminyRikuCharacterHadesText, gHadesFl00Frame0, gHadesPalette, gHadesFl00Tiles, gCardBos11Frame0, gCardBos11Palette, gCardBos11Tiles, NULL, NULL, NULL, 0, 0, 0, 18 },
    { &gEnemyNameOogieBoogieByLanguage, &gJiminyRikuCharacterOogieBoogieText, gBugiFl00Frame0, gBugiPalette, gBugiFl00Tiles, gCardBos02Frame0, gBoss02objPalette, gCardBos02Tiles, NULL, NULL, NULL, 0, 0, -4, 9 },
    { &gCharacterNameHookByLanguage, &gJiminyRikuCharacterHookText, gHookF00Frame0, gHookPalette, gHookF00Tiles, gCardBos10Frame0, gCardBos10Palette, gCardBos10Tiles, NULL, NULL, NULL, 0, 0, -1, 17 },
};

static const JiminyDetail sJiminyEntry20Details[33] = {
    { &gEnemyNameShadowByLanguage, &gJiminyHeartlessShadowText, gEmy00L00Frame0, gEmy00Palette, gEmy00L00Tiles, gCardEmy00Frame0, gEmy00Palette, gCardEmy00Tiles, NULL, NULL, NULL, 0, 0, 2, -12 },
    { &gEnemyNameSoldierByLanguage, &gJiminyHeartlessSoldierText, gEmy14Ll01Frame1, gEmy14Palette, gEmy14Ll01Tiles, gCardEmy14Frame0, gEmy14Palette, gCardEmy14Tiles, NULL, NULL, NULL, 0, 0, -8, 2 },
    { &gEnemyNameLargeBodyByLanguage, &gJiminyHeartlessLargeBodyText, gEmy3800Frame0, gEmy38Palette, gEmy3800Tiles, gCardEmy38Frame0, gEmy38Palette, gCardEmy38Tiles, NULL, NULL, NULL, 0, 0, -6, 6 },
    { &gEnemyNameRedNocturneByLanguage, &gJiminyHeartlessRedNocturneText, gEmy01L00Frame16, gEmy01Palette, gEmy01L00Tiles, gCardEmy01Frame0, gEmy01Palette, gCardEmy01Tiles, NULL, NULL, NULL, 0, 0, -2, -8 },
    { &gEnemyNameBlueRhapsodyByLanguage, &gJiminyHeartlessBlueRhapsodyText, gEmy01L00Frame3, gEmy02Palette, gEmy01L00Tiles, gCardEmy01Frame0, gEmy02Palette, gCardEmy01Tiles, NULL, NULL, NULL, 0, 0, -1, -11 },
    { &gEnemyNameYellowOperaByLanguage, &gJiminyHeartlessYellowOperaText, gEmy01L00Frame0, gEmy03Palette, gEmy01L00Tiles, gCardEmy01Frame0, gEmy03Palette, gCardEmy01Tiles, NULL, NULL, NULL, 0, 0, -2, -11 },
    { &gEnemyNameGreenRequiemByLanguage, &gJiminyHeartlessGreenRequiemText, gEmy01L00Frame15, gEmy04Palette, gEmy01L00Tiles, gCardEmy01Frame0, gEmy04Palette, gCardEmy01Tiles, NULL, NULL, NULL, 0, 0, -2, -8 },
    { &gEnemyNamePowerwildByLanguage, &gJiminyHeartlessPowerwildText, gEmy1500Frame7, gEmy15Palette, gEmy1500Tiles, gCardEmy15Frame0, gEmy15Palette, gCardEmy15Tiles, NULL, NULL, NULL, 0, 0, 0, -6 },
    { &gEnemyNameBouncywildByLanguage, &gJiminyHeartlessBouncywildText, gEmy1600Frame0, gEmy16Palette, gEmy1600Tiles, gCardEmy16Frame0, gEmy16Palette, gCardEmy16Tiles, NULL, NULL, NULL, 0, 0, -1, -9 },
    { &gEnemyNameAirSoldierByLanguage, &gJiminyHeartlessAirSoldierText, gEmy1800Frame0, gEmy18Palette, gEmy1800Tiles, gCardEmy18Frame0, gEmy18Palette, gCardEmy18Tiles, NULL, NULL, NULL, 0, 0, -1, 3 },
    { &gEnemyNameBanditByLanguage, &gJiminyHeartlessBanditText, gEmy1900Frame5, gEmy19Palette, gEmy1900Tiles, gCardEmy19Frame0, gEmy19Palette, gCardEmy19Tiles, NULL, NULL, NULL, 0, 0, -1, -6 },
    { &gEnemyNameFatBanditByLanguage, &gJiminyHeartlessFatBanditText, gEmy3900Frame0, gEmy39Palette, gEmy3900Tiles, gCardEmy39Frame0, gEmy39Palette, gCardEmy39Tiles, NULL, NULL, NULL, 0, 0, -1, 4 },
    { &gEnemyNameBarrelSpiderByLanguage, &gJiminyHeartlessBarrelSpiderText, gEmy2100Frame0, gEmy21Palette, gEmy2100Tiles, gCardEmy21Frame0, gEmy21Palette, gCardEmy21Tiles, NULL, NULL, NULL, 0, 0, 1, -10 },
    { &gEnemyNameSearchGhostByLanguage, &gJiminyHeartlessSearchGhostText, gEmy2200Frame9, gEmy22Palette, gEmy2200Tiles, gCardEmy22Frame0, gEmy22Palette, gCardEmy22Tiles, NULL, NULL, NULL, 0, 0, 2, 6 },
    { &gEnemyNameSeaNeonByLanguage, &gJiminyHeartlessSeaNeonText, gEmy0600Frame2, gEmy06Palette, gEmy0600Tiles, gCardEmy06Frame0, gEmy06Palette, gCardEmy06Tiles, NULL, NULL, NULL, 0, 0, -2, -3 },
    { &gEnemyNameScrewdiverByLanguage, &gJiminyHeartlessScrewdiverText, gEmy2300Frame0, gEmy23Palette, gEmy2300Tiles, gCardEmy23Frame0, gEmy23Palette, gCardEmy23Tiles, NULL, NULL, NULL, 0, 0, 1, 2 },
    { &gEnemyNameAquatankByLanguage, &gJiminyHeartlessAquatankText, gEmy4100Frame0, gEmy41Palette, gEmy4100Tiles, gCardEmy41Frame0, gEmy41Palette, gCardEmy41Tiles, NULL, NULL, NULL, 0, 0, -2, 11 },
    { &gEnemyNameWightKnightByLanguage, &gJiminyHeartlessWightKnightText, gEmy2500Frame3, gEmy25Palette, gEmy2500Tiles, gCardEmy25Frame0, gEmy25Palette, gCardEmy25Tiles, NULL, NULL, NULL, 0, 0, 4, 0 },
    { &gEnemyNameGargoyleByLanguage, &gJiminyHeartlessGargoyleText, gEmy2600Frame0, gEmy26Palette, gEmy2600Tiles, gCardEmy26Frame0, gEmy26Palette, gCardEmy26Tiles, NULL, NULL, NULL, 0, 0, -4, 9 },
    { &gEnemyNamePirateByLanguage, &gJiminyHeartlessPirateText, gEmy2700Frame0, gEmy27Palette, gEmy2700Tiles, gCardEmy27Frame0, gEmy27Palette, gCardEmy27Tiles, NULL, NULL, NULL, 0, 0, 10, -10 },
    { &gEnemyNameAirPirateByLanguage, &gJiminyHeartlessAirPirateText, gEmy2800Frame0, gEmy28Palette, gEmy2800Tiles, gCardEmy28Frame0, gEmy28Palette, gCardEmy28Tiles, NULL, NULL, NULL, 0, 0, 5, 5 },
    { &gEnemyNameDarkballByLanguage, &gJiminyHeartlessDarkballText, gEmy2900Frame0, gEmy29Palette, gEmy2900Tiles, gCardEmy29Frame0, gEmy29Palette, gCardEmy29Tiles, NULL, NULL, NULL, 0, 0, -3, 7 },
    { &gEnemyNameDefenderByLanguage, &gJiminyHeartlessDefenderText, gEmy4400Frame0, gEmy44Palette, gEmy4400Tiles, gCardEmy44Frame0, gEmy44Palette, gCardEmy44Tiles, NULL, NULL, NULL, 0, 0, 3, 1 },
    { &gEnemyNameWyvernByLanguage, &gJiminyHeartlessWyvernText, gEmy3000Frame0, gEmy30Palette, gEmy3000Tiles, gCardEmy30Frame0, gEmy30Palette, gCardEmy30Tiles, NULL, NULL, NULL, 0, 0, 2, -5 },
    { &gEnemyNameWizardByLanguage, &gJiminyHeartlessWizardText, gEmy3100Frame6, gEmy31Palette, gEmy3100Tiles, gCardEmy31Frame0, gEmy31Palette, gCardEmy31Tiles, NULL, NULL, NULL, 0, 0, 5, -3 },
    { &gEnemyNameNeoshadowByLanguage, &gJiminyHeartlessNeoshadowText, gEmy3700Frame1, gEmy37Palette, gEmy3700Tiles, gCardEmy37Frame0, gEmy37Palette, gCardEmy37Tiles, NULL, NULL, NULL, 0, 0, -1, 4 },
    { &gEnemyNameCreeperPlantByLanguage, &gJiminyHeartlessCreeperPlantText, gEmy8300Frame0, gEmy83Palette, gEmy8300Tiles, gCardEmy83Frame0, gEmy83Palette, gCardEmy83Tiles, NULL, NULL, NULL, 0, 0, 0, -4 },
    { &gEnemyNameTornadoStepByLanguage, &gJiminyHeartlessTornadoStepText, gEmy8100Frame0, gEmy81Palette, gEmy8100Tiles, gCardEmy81Frame0, gEmy81Palette, gCardEmy81Tiles, NULL, NULL, NULL, 0, 0, -2, -3 },
    { &gEnemyNameCrescendoByLanguage, &gJiminyHeartlessCrescendoText, gEmy8201Frame2, gEmy82Palette, gEmy8201Tiles, gCardEmy82Frame0, gEmy82Palette, gCardEmy82Tiles, NULL, NULL, NULL, 0, 0, 4, -10 },
    { &gEnemyNameGuardArmorByLanguage, &gJiminyRikuHeartlessGuardArmorText, NULL, NULL, NULL, gCardBos01Frame0, gBoss01objPalette, gCardBos01Tiles, gJiminyGuardArmorBgMap, gJiminyGuardArmorBgPalette, gJiminyGuardArmorBgTiles, sizeof(gJiminyGuardArmorBgPalette), sizeof(gJiminyGuardArmorBgTiles), 3, -3 },
    { &gEnemyNameParasiteCageByLanguage, &gJiminyRikuHeartlessParasiteCageText, NULL, NULL, NULL, gCardBos04Frame0, gBosPcBgPalettes, gCardBos04Tiles, gJiminyParasiteCageBgMap, gJiminyParasiteCageBgPalette, gJiminyParasiteCageBgTiles, sizeof(gJiminyParasiteCageBgPalette), sizeof(gJiminyParasiteCageBgTiles), 0, 0 },
    { &gEnemyNameTrickmasterByLanguage, &gJiminyRikuHeartlessTrickmasterText, NULL, NULL, NULL, gCardBos03Frame0, gBoss03objPalettes, gCardBos03Tiles, gJiminyTrickmasterBgMap, gJiminyTrickmasterBgPalette, gJiminyTrickmasterBgTiles, sizeof(gJiminyTrickmasterBgPalette), sizeof(gJiminyTrickmasterBgTiles), 0, 3 },
    { &gEnemyNameDarksideByLanguage, &gJiminyRikuHeartlessDarksideText, NULL, NULL, NULL, gCardBos00Frame0, gBoss00ObjPalette, gCardBos00Tiles, gJiminyDarksideBgMap, gJiminyDarksideBgPalette, gJiminyDarksideBgTiles, sizeof(gJiminyDarksideBgPalette), sizeof(gJiminyDarksideBgTiles), 0, 0 },
};
#endif

#include "jiminy_placeholders.inc"
#ifdef PLATFORM_PC
#include "port/port_hdsprites.h"
#endif
static JiminyWork* sJiminyWork;

enum JiminyState {
    JIMINY_STATE_BARS_IN,
    JIMINY_STATE_TITLE_IN,
    JIMINY_STATE_TITLE_OUT,
    JIMINY_STATE_BARS_OUT,
    JIMINY_STATE_EXIT_TO_MENU,
    JIMINY_STATE_EXIT_TO_MAP,
    JIMINY_STATE_OPEN_LIST,
    JIMINY_STATE_LIST,
    JIMINY_STATE_OPEN_DETAIL,
    JIMINY_STATE_DETAIL
};

enum JiminyEntryState {
    JIMINY_ENTRY_STATE_INCOMPLETE,
    JIMINY_ENTRY_STATE_NEW,
    JIMINY_ENTRY_STATE_COMPLETE,
    JIMINY_ENTRY_STATE_HIDDEN
};

enum JiminyDetailLayout {
    JIMINY_DETAIL_LAYOUT_STORY,
    JIMINY_DETAIL_LAYOUT_CARD,
    JIMINY_DETAIL_LAYOUT_CHARACTER
};

void JiminyFreeRows() {
    s32 i;
    s32 j;

    for (i = 0; i < 8; i++) {
        sJiminyWork->rowStates[i] = JIMINY_ENTRY_STATE_INCOMPLETE;
        FreeTextSlots(sJiminyWork->lines[i].textSlots, ARRAY_COUNT(sJiminyWork->lines[i].textSlots));

        for (j = 0; j < 48; j++) {
            if (sJiminyWork->lines[i].textSlots[j].tiles != NULL) {
                sJiminyWork->lines[i].textSlots[j].tiles = NULL;
            }
        }
    }
}

void JiminyInitCursor(s16 x, s16 y, s16 rowHeight) {
    sJiminyWork->moveDelay = 0;
    sJiminyWork->x4 = x << 8;
    sJiminyWork->y5 = (y + sJiminyWork->cursorRow * rowHeight) << 8;
}

void JiminyUpdateCursor(s16 x, s16 y, s16 rowHeight) {
    s32 targetY;

    targetY = (y + sJiminyWork->cursorRow * rowHeight) << 8;
    ApproachValueHalf(&sJiminyWork->y5, targetY);

    if (sJiminyWork->moveDelay > 0) {
        sJiminyWork->moveDelay--;
    }
}

u16 GetJiminyTextLength(const u16* text) {
    s32 n;
    const u16* str;

    str = text;
    n = 0;

    while (1) {
#ifdef VERSION_US
        if (str[n] == 0) {
#else
        if (((u8*)str)[n] == 0) {
#endif
#ifdef VERSION_JP
            return n / 2;
#else
            return n;
#endif
        }

        n++;
    }
}

s32 GetJiminyEntryState(s32 idx) {
    const JiminyEntry* entry;
    s32 allSet;
    s32 noneSet;
    s32 allComplete;
    s32 allHidden;
    s32 i;

    entry = &sJiminyEntries[idx];

    if (entry->flags != NULL) {
        allSet = TRUE;
        noneSet = TRUE;

        for (i = 0; i < (u16)entry->count; i++) {
            if (IsJiminyFlagNew(entry->flags[i])) {
                return JIMINY_ENTRY_STATE_NEW;
            }

            if (!IsJiminyFlagSet(entry->flags[i])) {
                allSet = FALSE;
            } else {
                noneSet = FALSE;
            }
        }

        if (allSet) {
            return JIMINY_ENTRY_STATE_COMPLETE;
        }

        if (noneSet) {
            return JIMINY_ENTRY_STATE_HIDDEN;
        }

        return JIMINY_ENTRY_STATE_INCOMPLETE;
    }

    if (entry->children == NULL) {
        return JIMINY_ENTRY_STATE_HIDDEN;
    }

    allComplete = TRUE;
    allHidden = TRUE;

    for (i = 0; i < (u16)entry->count; i++) {
        switch (GetJiminyEntryState(entry->children[i])) {
        case JIMINY_ENTRY_STATE_NEW:
            return JIMINY_ENTRY_STATE_NEW;
        case JIMINY_ENTRY_STATE_INCOMPLETE:
            allComplete = FALSE;
            allHidden = FALSE;
            break;
        case JIMINY_ENTRY_STATE_COMPLETE:
            allHidden = FALSE;
            break;
        case JIMINY_ENTRY_STATE_HIDDEN:
            allComplete = FALSE;
            break;
        }
    }

    if (allComplete) {
        return JIMINY_ENTRY_STATE_COMPLETE;
    }

    if (allHidden) {
        return JIMINY_ENTRY_STATE_HIDDEN;
    }

    return JIMINY_ENTRY_STATE_INCOMPLETE;
}

void JiminyLoadHiddenRow(s32 row, const u16* const* itemTexts) {
    s16 length;

    length = GetJiminyTextLength(itemTexts[row]);
    length--;

    if (length < 0) {
        length = 0;
    }

    if (length > 12) {
        length = 12;
    }

    sJiminyWork->textSlotCounts[row] = LoadTextSlots(gJiminyHiddenTexts[length], sJiminyWork->lines[row].textSlots);
}

void JiminyLoadRows(s16 visibleRows, s16 itemCount, const u16* const* itemTexts, const u16* itemFlags, const u16* itemChildren, s16 listX, s16 listY, s16 rowHeight) {
    s16 n;
    s32 i;

    n = visibleRows > itemCount ? itemCount : visibleRows;

    if (itemFlags == NULL) {
        for (i = 0; i < n; i++) {
            if (itemChildren != NULL) {
                sJiminyWork->rowStates[i] = GetJiminyEntryState(itemChildren[i]);

                if (sJiminyWork->rowStates[i] == JIMINY_ENTRY_STATE_HIDDEN) {
                    JiminyLoadHiddenRow(i, itemTexts);
                } else {
                    sJiminyWork->textSlotCounts[i] =
                        LoadTextSlots(itemTexts[i], sJiminyWork->lines[i].textSlots);
                }
            } else {
                sJiminyWork->textSlotCounts[i] =
                    LoadTextSlots(itemTexts[i], sJiminyWork->lines[i].textSlots);
            }
        }
    } else {
        for (i = 0; i < n; i++) {
            if (IsJiminyFlagSet(itemFlags[i])) {
                sJiminyWork->textSlotCounts[i] =
                    LoadTextSlots(itemTexts[i], sJiminyWork->lines[i].textSlots);

                if (IsJiminyFlagNew(itemFlags[i])) {
                    sJiminyWork->rowStates[i] = JIMINY_ENTRY_STATE_NEW;
                }
            } else {
                JiminyLoadHiddenRow(i, itemTexts);
            }
        }
    }
}

void JiminyReloadRows() {
    s16 firstItem;

    firstItem = sJiminyWork->cursor - sJiminyWork->cursorRow;
    JiminyFreeRows();

    if (sJiminyWork->itemFlags != NULL) {
        JiminyLoadRows(sJiminyWork->visibleRows, sJiminyWork->itemCount,
            sJiminyWork->itemTexts + firstItem, sJiminyWork->itemFlags + firstItem, NULL,
            sJiminyWork->listX, sJiminyWork->listY, sJiminyWork->rowHeight);
    } else {
        JiminyLoadRows(sJiminyWork->visibleRows, sJiminyWork->itemCount,
            sJiminyWork->itemTexts + firstItem, NULL, sJiminyWork->itemChildren + firstItem,
            sJiminyWork->listX, sJiminyWork->listY, sJiminyWork->rowHeight);
    }
}

void JiminyReloadPlainRows() {
    s16 firstItem;

    firstItem = sJiminyWork->cursor - sJiminyWork->cursorRow;
    JiminyFreeRows();
    JiminyLoadRows(sJiminyWork->visibleRows, sJiminyWork->itemCount,
        sJiminyWork->itemTexts + firstItem, NULL, NULL,
        sJiminyWork->listX, sJiminyWork->listY, sJiminyWork->rowHeight);
}

void JiminyOpenList(s16 visibleRows, s16 itemCount, const u16* const* itemTexts, const u16* itemFlags, const u16* itemChildren, s16 listX, s16 listY, s16 rowHeight) {
#ifdef VERSION_EU
    s32 i;
#endif
    sJiminyWork->listX = listX;
    sJiminyWork->listY = listY;
    sJiminyWork->rowHeight = rowHeight;
    sJiminyWork->itemCount = itemCount;
    sJiminyWork->visibleRows = visibleRows;

#ifdef VERSION_EU
    for (i = 0; i < itemCount; i++) {
        sJiminyWork->resolvedTexts[i] = GetLocalizedString(itemTexts[i]);
    }

    sJiminyWork->itemTexts = sJiminyWork->resolvedTexts;
#else
    sJiminyWork->itemTexts = itemTexts;
#endif
    sJiminyWork->itemFlags = itemFlags;
    sJiminyWork->itemChildren = itemChildren;
    sJiminyWork->x = listX + 56;
    sJiminyWork->x2 = listX + 56;
    sJiminyWork->y = listY - 10;
    sJiminyWork->y2 = listY + rowHeight * (visibleRows - 1) + 12;
    sJiminyWork->flags = (sJiminyWork->flags & ~(JIMINY_FLAG_SHOW_TITLE | JIMINY_FLAG_SCROLL_UP | JIMINY_FLAG_SCROLL_DOWN)) | JIMINY_FLAG_SHOW_CURSOR;
    sJiminyWork->shownChars = 0;
    JiminyInitCursor(sJiminyWork->listX - 24, sJiminyWork->listY - 4,
        sJiminyWork->rowHeight);
    JiminyReloadRows();
    sJiminyWork->frame = 0;
}

u8 JiminyHandleListInput() {
    if (FadeIsActive()) {
        return TRUE;
    }

    if (sJiminyWork->shownChars < sJiminyWork->charCount) {
        AnimChange(&sJiminyWork->anim, 1, ANIM_FLAG_LOOP);

        if (!FadeIsActive()) {
            if (sJiminyWork->stateTimer % 5 == 0) {
                sJiminyWork->shownChars++;
            }
        }
    } else {
        AnimChange(&sJiminyWork->anim, 0, ANIM_FLAG_LOOP);
    }

    if (sJiminyWork->cursorRow < sJiminyWork->cursor) {
        sJiminyWork->flags |= JIMINY_FLAG_SCROLL_UP;
    } else {
        sJiminyWork->flags &= ~JIMINY_FLAG_SCROLL_UP;
    }

    if (sJiminyWork->visibleRows - sJiminyWork->cursorRow <
        sJiminyWork->itemCount - sJiminyWork->cursor) {
        sJiminyWork->flags |= JIMINY_FLAG_SCROLL_DOWN;
    } else {
        sJiminyWork->flags &= ~JIMINY_FLAG_SCROLL_DOWN;
    }

    if (sJiminyWork->moveDelay <= 0) {
        if (GetKeysRepeat() & DPAD_UP) {
            if (sJiminyWork->cursor > 0) {
                sJiminyWork->moveDelay = 1;
                sJiminyWork->cursor--;
                m4aSongNumStart(SONG_SYS_CLICK);

                if (sJiminyWork->cursorRow > 0) {
                    sJiminyWork->cursorRow--;
                } else {
                    JiminyReloadRows();
                }
            }
        } else if (GetKeysRepeat() & DPAD_DOWN) {
            if (sJiminyWork->cursor < sJiminyWork->itemCount - 1) {
                sJiminyWork->moveDelay = 1;
                sJiminyWork->cursor++;
                m4aSongNumStart(SONG_SYS_CLICK);

                if (sJiminyWork->cursorRow < sJiminyWork->visibleRows - 1) {
                    sJiminyWork->cursorRow++;
                } else {
                    JiminyReloadRows();
                }
            }
        }
    }

    JiminyUpdateCursor(sJiminyWork->listX - 24, sJiminyWork->listY - 4,
        sJiminyWork->rowHeight);

    if (GetKeysPressed() & START_BUTTON) {
        sJiminyWork->stateTimer = 0;
        sJiminyWork->state = JIMINY_STATE_EXIT_TO_MAP;
        m4aSongNumStart(SONG_SYS_CLOSE);
        return TRUE;
    }

    return FALSE;
}

void mode_jiminy_0() {
    s32 i;
    s32 j;
    const JiminyEntry* entry;

    sJiminyWork = EwramAlloc(sizeof(JiminyWork));
    SetBgMode0();
    SetupBg(0, 0, 0x1D, 0);
    SetupBg(1, 0, 0x1E, 0);
    SetupBg(2, 0, 0x1F, 0);
    SetupBg(3, 0, 0x1C, 0x0D);
    SetBgPriority(0, 0);
    SetBgPriority(1, 1);
    SetBgPriority(2, 0);
    SetBgPriority(3, 0);
    LoadBgTiles(1, gJiminyBgTiles, sizeof(gJiminyBgTiles));

#ifdef VERSION_EU
    switch (gLanguage) {
    case LANGUAGE_FRENCH:
        RequestDma3Copy(gJiminyBgTitleTilesFrench, (u8*)GetBgCharBase(1) + 0x2000, sizeof(gJiminyBgTitleTilesFrench));
        break;
    case LANGUAGE_SPANISH:
        RequestDma3Copy(gJiminyBgTitleTilesSpanish, (u8*)GetBgCharBase(1) + 0x2000, sizeof(gJiminyBgTitleTilesSpanish));
        break;
    case LANGUAGE_ITALIAN:
        RequestDma3Copy(gJiminyBgTitleTilesItalian, (u8*)GetBgCharBase(1) + 0x2000, sizeof(gJiminyBgTitleTilesItalian));
        break;
    case LANGUAGE_GERMAN:
        // @bug Reads past the end of gJiminyBgTitleTilesGerman.
        RequestDma3Copy(gJiminyBgTitleTilesGerman, (u8*)GetBgCharBase(1) + 0x2000, 0x1000);
        break;
    }
#endif
    LoadBgPalette(1, gJiminyBgPalette, sizeof(gJiminyBgPalette));
    LoadBgMap(1, gJiminyCoverMap, sizeof(gJiminyCoverMap));
    LoadBgMap(2, gJiminyMessageMap, sizeof(gJiminyMessageMap));

#ifdef VERSION_EU
    if (gLanguage == LANGUAGE_ENGLISH) {
        sJiminyWork->tiles = LoadObjTiles(gJiminyTitleTiles, sizeof(gJiminyTitleTiles));
    } else {
        sJiminyWork->tiles = LoadObjTiles(gJiminyTitleLocalizedTiles, sizeof(gJiminyTitleLocalizedTiles));
    }
#else
    sJiminyWork->tiles = LoadObjTiles(gJiminyTitleTiles, sizeof(gJiminyTitleTiles));
#endif
    sJiminyWork->palette = LoadObjPalette(gJiminyTitlePalette, sizeof(gJiminyTitlePalette));
    FadeSetPaletteExcluded(sJiminyWork->palette->index + 0x10, TRUE);
    sJiminyWork->tiles2 = LoadObjTiles(gJiminyBinderRingTiles, sizeof(gJiminyBinderRingTiles));
    sJiminyWork->palette2 = LoadObjPalette(gJiminyBinderRingPalette, sizeof(gJiminyBinderRingPalette));
    sJiminyWork->palette3 = LoadObjPalette(gJiminyRootListPalette, sizeof(gJiminyRootListPalette));
    sJiminyWork->tiles5 = LoadObjTiles(gJiminyScrollArrowTiles, sizeof(gJiminyScrollArrowTiles));
    sJiminyWork->palette6 = LoadObjPalette(gJiminyScrollArrowPalette, sizeof(gJiminyScrollArrowPalette));
    sJiminyWork->tiles6 = LoadObjTiles(gJiminyRowMarkTiles, sizeof(gJiminyRowMarkTiles));
    sJiminyWork->palette7 = LoadObjPalette(gJiminyListPalette, sizeof(gJiminyListPalette));
    sJiminyWork->x3 = -0x8000;
    sJiminyWork->y3 = -0x800;
    sJiminyWork->y4 = 0xA000;
    sJiminyWork->state = JIMINY_STATE_BARS_IN;
    sJiminyWork->stateTimer = 0;
    sJiminyWork->shownChars = 0;
    sJiminyWork->cursor = 0;
    sJiminyWork->cursorRow = 0;

    if (gGameState.flags & GAME_FLAG_RIKU) {
        sJiminyWork->tiles3 = AllocObjTiles(0x1000, gTalk2700Tiles);
        sJiminyWork->palette4 = LoadObjPalette(gTalk2700Palette, sizeof(gTalk2700Palette));
        AnimInit(&sJiminyWork->anim, gTalk2700Anims, gTalk2700Frames);
        AnimStart(&sJiminyWork->anim, 0, ANIM_FLAG_LOOP);
    } else {
        sJiminyWork->tiles3 = AllocObjTiles(0x1000, gTalk0600Tiles);
        sJiminyWork->palette4 = LoadObjPalette(gTalk0600Palette, sizeof(gTalk0600Palette));
        AnimInit(&sJiminyWork->anim, gTalk0600Anims, gTalk0600Frames);
        AnimStart(&sJiminyWork->anim, 0, ANIM_FLAG_LOOP);
    }

    if (!FadeIsActive()) {
        sJiminyWork->tiles4 = AllocObjTiles(0x200, gJiminyCursorTiles);
        sJiminyWork->palette5 = LoadObjPalette(gJiminyCursorPalette, sizeof(gJiminyCursorPalette));
        AnimInit(&sJiminyWork->anim2, gJiminyCursorAnims, gJiminyCursorFrames);
        AnimStart(&sJiminyWork->anim2, 2, ANIM_FLAG_LOOP);
    }

    sJiminyWork->tiles7 = AllocObjTiles(0x2000, NULL);
#ifdef VERSION_EU
    sJiminyWork->palette8 = LoadObjPalette(gPooAltImagePalettes, sizeof(gPooAltImagePalettes));
#else
    sJiminyWork->palette8 = LoadObjPalette(gWorldImageWonderlandPalette, sizeof(gWorldImageWonderlandPalette));
#endif
    sJiminyWork->tiles8 = AllocObjTiles(0x800, NULL);
    sJiminyWork->palette9 = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
    sJiminyWork->detailTimer = 0;
    sJiminyWork->unk_D38 = 0x100;
    FadeStartIn(FADE_MODE_BLACK, 0x10);

    for (i = 0; i < 8; i++) {
        InitTextSlots(sJiminyWork->lines[i].textSlots, ARRAY_COUNT(sJiminyWork->lines[i].textSlots));
    }

    InitMsgGlyphSprites(0);
#ifdef VERSION_JP
    sJiminyWork->charCount = LayoutMsgGlyphsSjis(0x400, 0x2600, gJiminyChooseEntryText);
#elif defined(VERSION_EU)
    sJiminyWork->charCount = LayoutMsgGlyphs(0x200, 0x2400, GetLocalizedString(&gJiminyChooseEntryTextByLanguage));
#else
    sJiminyWork->charCount = LayoutMsgGlyphs(0x200, 0x2400, gJiminyChooseEntryText);
#endif

    for (j = 0; j <= 0x14; j++) {
        sJiminyWork->pairs[j].cursor = 0;
        sJiminyWork->pairs[j].cursorRow = 0;
    }

    if (gGameState.flags & GAME_FLAG_RIKU) {
        sJiminyWork->entry = 14;
        entry = sJiminyEntries;
        entry += 14;
        JiminyOpenList(3, entry->count, entry->names, entry->flags, entry->children, 0x80, 0x40, 0x18);
        sJiminyWork->flags = JIMINY_FLAG_SHOW_TITLE;
    } else {
        sJiminyWork->entry = 0;
        entry = sJiminyEntries;
        JiminyOpenList(3, entry->count, entry->names, entry->flags, entry->children, 0x80, 0x40, 0x18);
        sJiminyWork->flags = (JIMINY_FLAG_SHOW_MESSAGE | JIMINY_FLAG_SHOW_TITLE);
    }
}

void mode_jiminy_1() {
#ifdef PLATFORM_PC
    PortSideStyle(PORT_SIDE_GROUND_BANDS); /* the lists (and back from a page) */
#endif
    s32 i;
    u16 flags;
    u16 alpha;
    const JiminyEntry* openEntry;
    JiminyPair* openPair;
    const JiminyEntry* listEntry;
    JiminyPair* listPair;

    switch (sJiminyWork->state) {
    case JIMINY_STATE_BARS_IN:
        if (sJiminyWork->stateTimer == 0) {
            sJiminyWork->steps = 16;
        }

        ApproachValue(&sJiminyWork->y3, 0, sJiminyWork->steps);
        ApproachValue(&sJiminyWork->y4, 0x9800, sJiminyWork->steps);
        sJiminyWork->steps--;

        if (sJiminyWork->steps <= 0) {
            sJiminyWork->state = JIMINY_STATE_TITLE_IN;
            sJiminyWork->stateTimer = 0;
        } else {
            sJiminyWork->stateTimer++;
        }

        break;
    case JIMINY_STATE_TITLE_IN:
        if (sJiminyWork->stateTimer == 0) {
            sJiminyWork->steps = 16;
        }

        ApproachValue(&sJiminyWork->x3, 0, sJiminyWork->steps);
        sJiminyWork->steps--;

        if (sJiminyWork->steps <= 0) {
            sJiminyWork->state = JIMINY_STATE_OPEN_LIST;
            sJiminyWork->stateTimer = 0;
        } else {
            sJiminyWork->stateTimer++;
        }

        break;
    case JIMINY_STATE_TITLE_OUT:
        if (sJiminyWork->stateTimer == 0) {
            flags = sJiminyWork->flags | JIMINY_FLAG_SHOW_TITLE;
            sJiminyWork->flags = flags & ~JIMINY_FLAG_SHOW_CURSOR;
            sJiminyWork->steps = 16;
            LoadBgMap(1, gJiminyCoverMap, sizeof(gJiminyCoverMap));
        }

        ApproachValue(&sJiminyWork->x3, -0x8000, sJiminyWork->steps);
        sJiminyWork->steps--;

        if (sJiminyWork->steps <= 0) {
            sJiminyWork->state = JIMINY_STATE_BARS_OUT;
            sJiminyWork->stateTimer = 0;
        } else {
            sJiminyWork->stateTimer++;
        }

        break;
    case JIMINY_STATE_BARS_OUT:
        if (sJiminyWork->stateTimer == 0) {
            sJiminyWork->steps = 16;
        }

        ApproachValue(&sJiminyWork->y3, -0x800, sJiminyWork->steps);
        ApproachValue(&sJiminyWork->y4, 0xA000, sJiminyWork->steps);
        sJiminyWork->steps--;

        if (sJiminyWork->steps <= 0) {
            sJiminyWork->state = JIMINY_STATE_EXIT_TO_MENU;
            sJiminyWork->stateTimer = 0;
        } else {
            sJiminyWork->stateTimer++;
        }

        break;
    case JIMINY_STATE_EXIT_TO_MENU:
        if (sJiminyWork->stateTimer == 0) {
            FadeStartOut(FADE_MODE_BLACK, 16);
            FadeLock();
        }

        if (FadeGetAmount() > 30) {
            ReturnToMap(TRUE);
        }

        break;
    case JIMINY_STATE_EXIT_TO_MAP:
        if (sJiminyWork->stateTimer == 0) {
            FadeStartOut(FADE_MODE_BLACK, 16);
            FadeLock();
        }

        if (FadeGetAmount() > 30) {
            ReturnToMap(FALSE);
        }

        break;
    case JIMINY_STATE_OPEN_LIST:
        openEntry = &sJiminyEntries[sJiminyWork->entry];
        openPair = &sJiminyWork->pairs[sJiminyWork->entry];
#ifdef VERSION_JP
        sJiminyWork->charCount = LayoutMsgGlyphsSjis(0x400, 0x2600, gJiminyChooseEntryText);
#else
        sJiminyWork->charCount = LayoutMsgGlyphs(0x200, 0x2400, LOCALIZED_STRING(gJiminyChooseEntryText));
#endif
        DisableBg(3);
        DisableBg(0);
        sJiminyWork->cursor = openPair->cursor;
        sJiminyWork->cursorRow = openPair->cursorRow;

        if (sJiminyWork->entry == 0 || sJiminyWork->entry == 14) {
            if (gGameState.flags & GAME_FLAG_RIKU) {
                LoadBgMap(1, gJiminyRikuRootMap, sizeof(gJiminyRikuRootMap));
            } else {
                LoadBgMap(1, openEntry->map, 0x800);
            }

            LoadObjPaletteBank(sJiminyWork->palette3->index, gJiminyRootListPalette);
            JiminyOpenList(3, openEntry->count, openEntry->names, openEntry->flags, openEntry->children, 0x80, 0x40, 0x18);
        } else {
            LoadBgMap(1, openEntry->map, 0x800);
            LoadObjPaletteBank(sJiminyWork->palette3->index, gJiminyListPalette);
#ifdef VERSION_JP
            JiminyOpenList(8, openEntry->count, openEntry->names, openEntry->flags, openEntry->children, 0x70, 0x1A, 0x10);
#else
            JiminyOpenList(4, openEntry->count, openEntry->names, openEntry->flags, openEntry->children, 0x70, 0x3A, 0x10);
#endif
        }

        sJiminyWork->state = JIMINY_STATE_LIST;
    case JIMINY_STATE_LIST:
        listEntry = &sJiminyEntries[sJiminyWork->entry];
        listPair = &sJiminyWork->pairs[sJiminyWork->entry];

        if (JiminyHandleListInput()) {
            break;
        }

        listPair->cursor = sJiminyWork->cursor;
        listPair->cursorRow = sJiminyWork->cursorRow;

        if (GetKeysPressed() & B_BUTTON) {
            sJiminyWork->stateTimer = 0;

            if (listEntry->parent == JIMINY_ENTRY_NONE) {
                sJiminyWork->state = JIMINY_STATE_TITLE_OUT;
            } else {
                sJiminyWork->state = JIMINY_STATE_OPEN_LIST;
                sJiminyWork->entry = listEntry->parent;
                FadeStartIn(FADE_MODE_BLACK, 5);
                FadeLock();
            }

            m4aSongNumStart(SONG_SYS_CLOSE);
            break;
        }

        if (GetKeysPressed() & A_BUTTON) {
            u32 ok;

            ok = TRUE;

            if (listEntry->flags != NULL) {
                ok = IsJiminyFlagSet(listEntry->flags[sJiminyWork->cursor]) != FALSE;
            } else {
                if (sJiminyWork->rowStates[sJiminyWork->cursorRow] == JIMINY_ENTRY_STATE_HIDDEN) {
                    ok = FALSE;
                }
            }

            if (ok) {
                m4aSongNumStart(SONG_SYS_KETTEI);

                if (listEntry->children != NULL) {
                    sJiminyWork->state = JIMINY_STATE_OPEN_LIST;
                    sJiminyWork->entry = listEntry->children[sJiminyWork->cursor];
                    sJiminyWork->stateTimer = 0;
                    FadeStartIn(FADE_MODE_BLACK, 5);
                    FadeLock();
                    break;
                } else {
                    FadeStartIn(FADE_MODE_BLACK, 5);
                    FadeLock();
                    sJiminyWork->stateTimer = 0;
                    sJiminyWork->state = JIMINY_STATE_OPEN_DETAIL;
                    sJiminyWork->detailIndex = sJiminyWork->cursor;
                    sJiminyWork->detailTable = listEntry->detail;
                    SetModeUpdate(JiminyDetailUpdate);
                    break;
                }
            }
        }

        sJiminyWork->stateTimer++;
        break;
    }

    alpha = abs(SIN(sJiminyWork->frame * 2)) * 15 >> 8;
    gBldCnt = (BLDCNT_TGT1_OBJ | BLDCNT_TGT2_BG0 | BLDCNT_TGT2_BG1 | BLDCNT_TGT2_BG2 | BLDCNT_TGT2_BG3);
    SetBlendAlpha(alpha, 16 - alpha);

    if (sJiminyWork->flags & JIMINY_FLAG_SHOW_MESSAGE) {
        EnableBg(2);
        DrawMsgGlyphs(sJiminyWork->shownChars);
        DrawSprite(0x23, 0x76, AnimUpdate(&sJiminyWork->anim), sJiminyWork->tiles3,
            sJiminyWork->palette4, NULL, 0, 0);
    } else {
        DisableBg(2);
    }

    for (i = 0; sJiminyWork->lines[i].textSlots[0].tiles != NULL; i++) {
        if (i >= sJiminyWork->visibleRows) {
            break;
        }

        DrawTextSlots(sJiminyWork->listX, sJiminyWork->listY + sJiminyWork->rowHeight * i,
            sJiminyWork->lines[i].textSlots, sJiminyWork->palette3, 0, sJiminyWork->textSlotCounts[i]);

        if (sJiminyWork->state == JIMINY_STATE_LIST) {
            switch (sJiminyWork->rowStates[i]) {
            case JIMINY_ENTRY_STATE_NEW:
#ifdef VERSION_EU
                switch (gLanguage) {
                case LANGUAGE_ENGLISH:
                    DrawSprite(0xD9, sJiminyWork->listY + sJiminyWork->rowHeight * i, gJiminyRowMarkFrame2,
                        sJiminyWork->tiles6, sJiminyWork->palette7, NULL, SPRITE_FLAG_BLEND, 0xFFFE);
                    break;
                case LANGUAGE_FRENCH:
                    DrawSprite(0xD9, sJiminyWork->listY + sJiminyWork->rowHeight * i, gJiminyRowMarkFrame3,
                        sJiminyWork->tiles6, sJiminyWork->palette7, NULL, SPRITE_FLAG_BLEND, 0xFFFE);
                    break;
                case LANGUAGE_SPANISH:
                    DrawSprite(0xD9, sJiminyWork->listY + sJiminyWork->rowHeight * i, gJiminyRowMarkFrame4,
                        sJiminyWork->tiles6, sJiminyWork->palette7, NULL, SPRITE_FLAG_BLEND, 0xFFFE);
                    break;
                case LANGUAGE_ITALIAN:
                    DrawSprite(0xD9, sJiminyWork->listY + sJiminyWork->rowHeight * i, gJiminyRowMarkFrame5,
                        sJiminyWork->tiles6, sJiminyWork->palette7, NULL, SPRITE_FLAG_BLEND, 0xFFFE);
                    break;
                case LANGUAGE_GERMAN:
                default:
                    DrawSprite(0xD9, sJiminyWork->listY + sJiminyWork->rowHeight * i, gJiminyRowMarkFrame6,
                        sJiminyWork->tiles6, sJiminyWork->palette7, NULL, SPRITE_FLAG_BLEND, 0xFFFE);
                    break;
                }
#else
                DrawSprite(0xD9, sJiminyWork->listY + sJiminyWork->rowHeight * i, gJiminyRowMarkFrame2,
                    sJiminyWork->tiles6, sJiminyWork->palette7, NULL, SPRITE_FLAG_BLEND, 0);
#endif
                break;
            case JIMINY_ENTRY_STATE_COMPLETE:
#ifdef VERSION_EU
                DrawSprite(0xD9, sJiminyWork->listY + sJiminyWork->rowHeight * i - 2, gJiminyRowMarkFrame1,
                    sJiminyWork->tiles6, sJiminyWork->palette7, NULL, SPRITE_FLAG_BLEND, 0xFFFE);
#else
                DrawSprite(0xD9, sJiminyWork->listY + sJiminyWork->rowHeight * i - 2, gJiminyRowMarkFrame1,
                    sJiminyWork->tiles6, sJiminyWork->palette7, NULL, SPRITE_FLAG_BLEND, 0);
#endif
                break;
            }
        }
    }

    if ((sJiminyWork->flags & JIMINY_FLAG_SHOW_TITLE)
#ifdef PLATFORM_PC
        /* the title's HD banner: the sprite stays over the list's BG, which carries the title once it has slid in */
        || (sJiminyWork->state == JIMINY_STATE_LIST && sJiminyWork->tiles != NULL &&
            PortHdSpriteAny(((ObjTiles*)sJiminyWork->tiles)->src))
#endif
    ) {
#ifdef VERSION_EU
        switch (gLanguage) {
        case LANGUAGE_ENGLISH:
            if (gGameState.flags & GAME_FLAG_RIKU) {
                DrawMenuHeader(sJiminyWork->x3 >> 8, 0, gJiminyTitleFrame3, sJiminyWork->tiles,
                    sJiminyWork->palette, NULL, 0, 0);
            } else {
                DrawMenuHeader(sJiminyWork->x3 >> 8, 0, gJiminyTitleFrame0, sJiminyWork->tiles,
                    sJiminyWork->palette, NULL, 0, 0);
            }

            DrawSprite(0x58, 0x98, gJiminyBinderRingFrame0, sJiminyWork->tiles2, sJiminyWork->palette2, NULL, 0, 0);
            DrawSprite(0x80, sJiminyWork->y3 >> 8, gJiminyTitleFrame1, sJiminyWork->tiles,
                sJiminyWork->palette, NULL, 0, 1);
            DrawSprite(0x80, sJiminyWork->y4 >> 8, gJiminyTitleFrame2, sJiminyWork->tiles,
                sJiminyWork->palette, NULL, 0, 1);
            break;
        case LANGUAGE_FRENCH:
            if (gGameState.flags & GAME_FLAG_RIKU) {
                DrawMenuHeader(sJiminyWork->x3 >> 8, 0, gJiminyTitleLocalizedFrame3, sJiminyWork->tiles,
                    sJiminyWork->palette, NULL, 0, 0);
            } else {
                DrawMenuHeader(sJiminyWork->x3 >> 8, 0, gJiminyTitleLocalizedFrame0, sJiminyWork->tiles,
                    sJiminyWork->palette, NULL, 0, 0);
            }

            DrawSprite(0x58, 0x98, gJiminyBinderRingFrame0, sJiminyWork->tiles2, sJiminyWork->palette2, NULL, 0, 0);
            DrawSprite(0x80, sJiminyWork->y3 >> 8, gJiminyTitleLocalizedFrame1, sJiminyWork->tiles,
                sJiminyWork->palette, NULL, 0, 1);
            DrawSprite(0x80, sJiminyWork->y4 >> 8, gJiminyTitleLocalizedFrame2, sJiminyWork->tiles,
                sJiminyWork->palette, NULL, 0, 1);
            break;
        case LANGUAGE_SPANISH:
            if (gGameState.flags & GAME_FLAG_RIKU) {
                DrawMenuHeader(sJiminyWork->x3 >> 8, 0, gJiminyTitleLocalizedFrame5, sJiminyWork->tiles,
                    sJiminyWork->palette, NULL, 0, 0);
            } else {
                DrawMenuHeader(sJiminyWork->x3 >> 8, 0, gJiminyTitleLocalizedFrame4, sJiminyWork->tiles,
                    sJiminyWork->palette, NULL, 0, 0);
            }

            DrawSprite(0x58, 0x98, gJiminyBinderRingFrame0, sJiminyWork->tiles2, sJiminyWork->palette2, NULL, 0, 0);
            DrawSprite(0x80, sJiminyWork->y3 >> 8, gJiminyTitleLocalizedFrame1, sJiminyWork->tiles,
                sJiminyWork->palette, NULL, 0, 1);
            DrawSprite(0x80, sJiminyWork->y4 >> 8, gJiminyTitleLocalizedFrame2, sJiminyWork->tiles,
                sJiminyWork->palette, NULL, 0, 1);
            break;
        case LANGUAGE_ITALIAN:
            if (gGameState.flags & GAME_FLAG_RIKU) {
                DrawMenuHeader(sJiminyWork->x3 >> 8, 0, gJiminyTitleLocalizedFrame7, sJiminyWork->tiles,
                    sJiminyWork->palette, NULL, 0, 0);
            } else {
                DrawMenuHeader(sJiminyWork->x3 >> 8, 0, gJiminyTitleLocalizedFrame6, sJiminyWork->tiles,
                    sJiminyWork->palette, NULL, 0, 0);
            }

            DrawSprite(0x58, 0x98, gJiminyBinderRingFrame0, sJiminyWork->tiles2, sJiminyWork->palette2, NULL, 0, 0);
            DrawSprite(0x80, sJiminyWork->y3 >> 8, gJiminyTitleLocalizedFrame1, sJiminyWork->tiles,
                sJiminyWork->palette, NULL, 0, 1);
            DrawSprite(0x80, sJiminyWork->y4 >> 8, gJiminyTitleLocalizedFrame2, sJiminyWork->tiles,
                sJiminyWork->palette, NULL, 0, 1);
            break;
        case LANGUAGE_GERMAN:
        default:
            if (gGameState.flags & GAME_FLAG_RIKU) {
                DrawMenuHeader(sJiminyWork->x3 >> 8, 0, gJiminyTitleLocalizedFrame9, sJiminyWork->tiles,
                    sJiminyWork->palette, NULL, 0, 0);
            } else {
                DrawMenuHeader(sJiminyWork->x3 >> 8, 0, gJiminyTitleLocalizedFrame8, sJiminyWork->tiles,
                    sJiminyWork->palette, NULL, 0, 0);
            }

            DrawSprite(0x58, 0x98, gJiminyBinderRingFrame0, sJiminyWork->tiles2, sJiminyWork->palette2, NULL, 0, 0);
            DrawSprite(0x80, sJiminyWork->y3 >> 8, gJiminyTitleLocalizedFrame1, sJiminyWork->tiles,
                sJiminyWork->palette, NULL, 0, 1);
            DrawSprite(0x80, sJiminyWork->y4 >> 8, gJiminyTitleLocalizedFrame2, sJiminyWork->tiles,
                sJiminyWork->palette, NULL, 0, 1);
            break;
        }
#else
        if (gGameState.flags & GAME_FLAG_RIKU) {
            DrawMenuHeader(sJiminyWork->x3 >> 8, 0, gJiminyTitleFrame3, sJiminyWork->tiles,
                sJiminyWork->palette, NULL, 0, 0);
        } else {
            DrawMenuHeader(sJiminyWork->x3 >> 8, 0, gJiminyTitleFrame0, sJiminyWork->tiles,
                sJiminyWork->palette, NULL, 0, 0);
        }

        DrawSprite(0x58, 0x98, gJiminyBinderRingFrame0, sJiminyWork->tiles2, sJiminyWork->palette2, NULL, 0, 0);
        DrawSprite(0x80, sJiminyWork->y3 >> 8, gJiminyTitleFrame1, sJiminyWork->tiles,
            sJiminyWork->palette, NULL, 0, 1);
        DrawSprite(0x80, sJiminyWork->y4 >> 8, gJiminyTitleFrame2, sJiminyWork->tiles,
            sJiminyWork->palette, NULL, 0, 1);
#endif
    }

    if (sJiminyWork->state == JIMINY_STATE_LIST) {
        if (sJiminyWork->flags & JIMINY_FLAG_SCROLL_UP) {
            DrawSprite(sJiminyWork->x, sJiminyWork->y - ((sJiminyWork->frame >> 3) & 3),
                gJiminyScrollArrowFrame0, sJiminyWork->tiles5, sJiminyWork->palette6, NULL, 0, 0);
        }

        if (sJiminyWork->flags & JIMINY_FLAG_SCROLL_DOWN) {
            DrawSprite(sJiminyWork->x2, sJiminyWork->y2 + ((sJiminyWork->frame >> 3) & 3),
                gJiminyScrollArrowFrame1, sJiminyWork->tiles5, sJiminyWork->palette6, NULL, 0, 0);
        }

        if (!FadeIsActive()) {
            if (sJiminyWork->flags & JIMINY_FLAG_SHOW_CURSOR) {
                if (sJiminyWork->moveDelay <= 0) {
                    DrawSprite(sJiminyWork->x4 >> 8, sJiminyWork->y5 >> 8,
                        AnimUpdate(&sJiminyWork->anim2), sJiminyWork->tiles4,
                        sJiminyWork->palette5, NULL, 0, 0);
                } else {
                    DrawSprite(sJiminyWork->x4 >> 8, sJiminyWork->y5 >> 8, gJiminyCursorFrame2,
                        sJiminyWork->tiles4, sJiminyWork->palette5, NULL, 0, 0);

                    if (sJiminyWork->moveDelay == 1) {
                        AnimReset(&sJiminyWork->anim2);
                    }
                }
            }
        }
    }

    UpdatePlayTime();
    sJiminyWork->frame++;
}

void JiminyOpenPlainList(s16 visibleRows, s16 itemCount, const u16* const* itemTexts, s16 listX, s16 listY, s16 rowHeight) {
    sJiminyWork->listX = listX;
    sJiminyWork->listY = listY;
    sJiminyWork->rowHeight = rowHeight;
    sJiminyWork->itemCount = itemCount;
    sJiminyWork->visibleRows = visibleRows;
    sJiminyWork->itemTexts = itemTexts;
    sJiminyWork->x = listX + 0x38;
    sJiminyWork->x2 = listX + 0x38;
    sJiminyWork->y = listY - 10;
    sJiminyWork->y2 = listY + rowHeight * (visibleRows - 1) + 12;
    sJiminyWork->cursor = 0;
    sJiminyWork->cursorRow = 0;
    sJiminyWork->moveDelay = 0;
    sJiminyWork->frame = 0;
    JiminyReloadPlainRows();
}

void SplitThreeDecimalDigits(s16 value, u8* out) {
    out[0] = value / 100;
    out[1] = value % 100 / 10;
    out[2] = value % 10;
}

void JiminyDetailUpdate() {
#ifdef PLATFORM_PC
    PortSideStyle(PORT_SIDE_GROUND_SHEETS); /* the open book: each side's own ground in the wide view */
#endif
    s32 count;
    const JiminyDetail* entries;
    s16 i;
    s16 unlocked;
    s16 selected;
    s16 width;
    u8 digits[3];
    u16* map0;
    u16* map1;
    u8* source;
    u16* nameMap;
    u8* dest;
    u16* paletteDest;

#ifdef VERSION_EU
    s32 wide = FALSE;
#endif

    switch ((u32)sJiminyWork->state) {
    case JIMINY_STATE_OPEN_DETAIL:
        switch ((u32)sJiminyWork->detailTable) {
        case 1:
            count = 17;
            entries = sJiminyEntry01Details;
            sJiminyWork->detailLayout = JIMINY_DETAIL_LAYOUT_STORY;
            break;
        case 2:
            count = 17;
            entries = sJiminyEntry04Details;
            sJiminyWork->detailLayout = JIMINY_DETAIL_LAYOUT_CARD;
            break;
        case 3:
            count = 14;
            entries = sJiminyEntry05Details;
            sJiminyWork->detailLayout = JIMINY_DETAIL_LAYOUT_CARD;
            break;
        case 4:
            count = 7;
            entries = sJiminyEntry06Details;
            sJiminyWork->detailLayout = JIMINY_DETAIL_LAYOUT_CARD;
            break;
        case 5:
            count = 7;
            entries = sJiminyEntry07Details;
            sJiminyWork->detailLayout = JIMINY_DETAIL_LAYOUT_CARD;
            break;
        case 6:
            count = 49;
            entries = sJiminyEntry08Details;
            sJiminyWork->detailLayout = JIMINY_DETAIL_LAYOUT_CARD;
            break;
        case 7:
            count = 26;
            entries = sJiminyEntry09Details;
            sJiminyWork->detailLayout = JIMINY_DETAIL_LAYOUT_CARD;
            break;
        case 8:
            count = 1;
            entries = sJiminyEntry10Details;
            sJiminyWork->detailLayout = JIMINY_DETAIL_LAYOUT_CARD;
            break;
        case 9:
            count = 25;
            entries = sJiminyEntry11Details;
            sJiminyWork->detailLayout = JIMINY_DETAIL_LAYOUT_CHARACTER;
            break;
        case 10:
            count = 40;
            entries = sJiminyEntry12Details;
            sJiminyWork->detailLayout = JIMINY_DETAIL_LAYOUT_CHARACTER;
            break;
        case 11:
            count = 35;
            entries = sJiminyEntry13Details;
            sJiminyWork->detailLayout = JIMINY_DETAIL_LAYOUT_CHARACTER;
            break;
        case 12:
            count = 6;
            entries = sJiminyEntry15Details;
            sJiminyWork->detailLayout = JIMINY_DETAIL_LAYOUT_STORY;
            break;
        case 13:
            count = 22;
            entries = sJiminyEntry16Details;
            sJiminyWork->detailLayout = JIMINY_DETAIL_LAYOUT_CARD;
            break;
        case 14:
            count = 14;
            entries = sJiminyEntry18Details;
            sJiminyWork->detailLayout = JIMINY_DETAIL_LAYOUT_CHARACTER;
            break;
        case 15:
            count = 6;
            entries = sJiminyEntry19Details;
            sJiminyWork->detailLayout = JIMINY_DETAIL_LAYOUT_CHARACTER;
            break;
        case 16:
            count = 33;
            entries = sJiminyEntry20Details;
            sJiminyWork->detailLayout = JIMINY_DETAIL_LAYOUT_CHARACTER;
            break;
        default:
            count = 25;
            entries = sJiminyEntry11Details;
            sJiminyWork->detailLayout = JIMINY_DETAIL_LAYOUT_CHARACTER;
            break;
        }

        sJiminyWork->detailCount = count;

        if (sJiminyWork->detailIndex >= (s16)count) {
            sJiminyWork->detailIndex = 0;
        }

        sJiminyWork->detail = &entries[sJiminyWork->detailIndex];

        if (sJiminyWork->itemFlags != NULL) {
            ClearJiminyFlagNew(sJiminyWork->itemFlags[sJiminyWork->detailIndex]);
            unlocked = 0;
            selected = 0;

            for (i = 0; i < count; i++) {
                if (IsJiminyFlagSet(sJiminyWork->itemFlags[i])) {
                    if (sJiminyWork->detailIndex == i) {
                        selected = unlocked;
                    }

                    unlocked++;
                }
            }

            sJiminyWork->nextDetail = sJiminyWork->detailIndex;
            sJiminyWork->prevDetail = sJiminyWork->detailIndex;
            i = sJiminyWork->detailIndex + 1;

            for (;;) {
                if (i >= count) {
                    i = 0;
                }

                if (i == sJiminyWork->detailIndex) {
                    break;
                }

                if (IsJiminyFlagSet(sJiminyWork->itemFlags[i])) {
                    sJiminyWork->nextDetail = i;
                    break;
                }

                i++;
            }

            i = sJiminyWork->detailIndex - 1;

            for (;;) {
                if (i < 0) {
                    i = count - 1;
                }

                if (i == sJiminyWork->detailIndex) {
                    break;
                }

                if (IsJiminyFlagSet(sJiminyWork->itemFlags[i])) {
                    sJiminyWork->prevDetail = i;
                    break;
                }

                i--;
            }
        } else {
            unlocked = count;
            selected = sJiminyWork->detailIndex;
            sJiminyWork->nextDetail = sJiminyWork->detailIndex + 1;

            if (sJiminyWork->nextDetail >= count) {
                sJiminyWork->nextDetail = 0;
            }

            sJiminyWork->prevDetail = sJiminyWork->detailIndex - 1;

            if (sJiminyWork->prevDetail < 0) {
                sJiminyWork->prevDetail = count - 1;
            }
        }

        switch (sJiminyWork->detailLayout) {
        case JIMINY_DETAIL_LAYOUT_STORY:
            map0 = gJiminyDetailOverlayMap;
            map1 = gJiminyStoryDetailMap;
#ifdef VERSION_JP
            sJiminyWork->charCount = LayoutMsgGlyphsSjis(0x400,
                0x1800, sJiminyWork->detail->name);
#elif defined(VERSION_EU)
            sJiminyWork->charCount = LayoutMsgGlyphs(0x400,
                0x1600, GetLocalizedString(sJiminyWork->detail->name));
#else
            sJiminyWork->charCount = LayoutMsgGlyphs(0x400,
                0x1600, sJiminyWork->detail->name);
#endif
#ifdef VERSION_EU
            JiminyOpenPlainList(4, GetLocalizedLineCount(sJiminyWork->detail->text), GetLocalizedLines(sJiminyWork->detail->text), 8, 0x3A, 16);
#else
#ifdef VERSION_JP
            JiminyOpenPlainList(7, sJiminyWork->detail->lineCount, sJiminyWork->detail->text, 8, 0x2A, 16);
#else
            JiminyOpenPlainList(4, sJiminyWork->detail->lineCount, sJiminyWork->detail->text, 8, 0x3A, 16);
#endif
#endif
            break;
        case JIMINY_DETAIL_LAYOUT_CARD:
            map0 = gJiminyDetailOverlayMap;
            map1 = gJiminyCardDetailMap;
#ifdef VERSION_JP
            sJiminyWork->charCount = LayoutMsgGlyphsSjis(0x400,
                0x1800, sJiminyWork->detail->name);
#elif defined(VERSION_EU)
            sJiminyWork->charCount = LayoutMsgGlyphs(0x400,
                0x1600, GetLocalizedString(sJiminyWork->detail->name));
#else
            sJiminyWork->charCount = LayoutMsgGlyphs(0x400,
                0x1600, sJiminyWork->detail->name);
#endif
#ifdef VERSION_EU
            JiminyOpenPlainList(4, GetLocalizedLineCount(sJiminyWork->detail->text), GetLocalizedLines(sJiminyWork->detail->text), 8, 0x3A, 16);
#else
#ifdef VERSION_JP
            JiminyOpenPlainList(7, sJiminyWork->detail->lineCount, sJiminyWork->detail->text, 8, 0x2A, 16);
#else
            JiminyOpenPlainList(4, sJiminyWork->detail->lineCount, sJiminyWork->detail->text, 8, 0x3A, 16);
#endif
#endif
            break;
        case JIMINY_DETAIL_LAYOUT_CHARACTER:
        default:
            map0 = gJiminyCharacterDetailOverlayMap;
            map1 = gJiminyCharacterDetailMap;
#ifdef VERSION_JP
            sJiminyWork->charCount = LayoutMsgGlyphsSjis(0x2800,
                0x1800, sJiminyWork->detail->name);
#elif defined(VERSION_EU)
            sJiminyWork->charCount = LayoutMsgGlyphs(0x2800,
                0x1600, GetLocalizedString(sJiminyWork->detail->name));
#else
            sJiminyWork->charCount = LayoutMsgGlyphs(0x2800,
                0x1600, sJiminyWork->detail->name);
#endif
#ifdef VERSION_EU
            JiminyOpenPlainList(4, GetLocalizedLineCount(sJiminyWork->detail->text), GetLocalizedLines(sJiminyWork->detail->text), 8, 0x3A, 16);
#else
            JiminyOpenPlainList(
#ifdef VERSION_JP
                6,
#else
                4,
#endif
                sJiminyWork->detail->lineCount, sJiminyWork->detail->text, 8, 0x3A, 16);
#endif
            break;
        }

        LoadBgMap(0, map0, 0x800);
        LoadBgMap(1, map1, 0x800);

        if (sJiminyWork->detail->tiles != NULL) {
#ifdef VERSION_EU
            if (sJiminyWork->detail->palette == gWorldImage100AcreWoodPalette && IsPooAltImageActive()) {
                LoadObjPaletteBank(sJiminyWork->palette8->index, gPooAltImagePalettes[0]);
                LoadObjPaletteBank(sJiminyWork->palette8->index + 1, gPooAltImagePalettes[1]);
                SetObjTileSource(sJiminyWork->tiles7, gPooAltImageTiles);
            } else
#endif
            {
                LoadObjPaletteBank(sJiminyWork->palette8->index, sJiminyWork->detail->palette);
                SetObjTileSource(sJiminyWork->tiles7, sJiminyWork->detail->tiles);
            }
        }

        if (sJiminyWork->detail->tiles2 != NULL) {
            LoadObjPaletteBank(sJiminyWork->palette9->index, sJiminyWork->detail->palette2);
            SetObjTileSource(sJiminyWork->tiles8, sJiminyWork->detail->tiles2);
        }

        if (sJiminyWork->detailLayout == JIMINY_DETAIL_LAYOUT_CHARACTER) {
            nameMap = gJiminyCharacterNamePlateMap;
            dest = (u8*)GetBgScreenBase(0) + 0x8E;
        } else {
            nameMap = gJiminyNamePlateMap;
            dest = (u8*)GetBgScreenBase(0) + 0x80;
        }

        if (sJiminyWork->detailLayout < JIMINY_DETAIL_LAYOUT_CHARACTER) {
#ifdef VERSION_JP
            switch (sJiminyWork->charCount) {
            case 1: nameMap += 0x60; break;
            case 2: nameMap += 0x120; break;
            case 3: nameMap += 0x180; break;
            case 4: nameMap += 0x240; break;
            case 5: nameMap += 0x2A0; break;
            case 6: nameMap += 0x300; break;
            case 7: nameMap += 0x360; break;
            case 8: nameMap += 0x70; break;
            case 9: nameMap += 0xD0; break;
            case 10: nameMap += 0x130; break;
            default: nameMap += 0x190; break;
            }
#else
            width = GetMsgTextWidth(
#ifdef VERSION_EU
                GetLocalizedString(sJiminyWork->detail->name)
#else
                sJiminyWork->detail->name
#endif
            );

            switch ((width + 12) / 8) {
            case 0: nameMap += 0x120; break;
            case 1: nameMap += 0x120; break;
            case 2: nameMap += 0x120; break;
            case 3: nameMap += 0x120; break;
            case 4: nameMap += 0x180; break;
            case 5: nameMap += 0x1E0; break;
            case 6: nameMap += 0x240; break;
            case 7: nameMap += 0x2A0; break;
            case 8: nameMap += 0x300; break;
            case 9: nameMap += 0x360; break;
            case 10: nameMap += 0x10; break;
            case 11: nameMap += 0x70; break;
            case 12: nameMap += 0xD0; break;
            case 13: nameMap += 0x130; break;
#ifdef VERSION_EU
            case 14: nameMap += 0x190; break;
            case 15: nameMap += 0x1F0; break;
            default: nameMap += 0x1F0; wide = TRUE; break;
#else
            default: nameMap += 0x190; break;
#endif
            }
#endif
        } else {
#ifdef VERSION_JP
            width = sJiminyWork->charCount;
#else
            width = (s16)GetMsgTextWidth(
#ifdef VERSION_EU
                GetLocalizedString(sJiminyWork->detail->name)
#else
                sJiminyWork->detail->name
#endif
            ) / 8;
#endif

            switch (width) {
            case 0: break;
            case 1: break;
            case 2: nameMap += 0x60; break;
            case 3: nameMap += 0xC0; break;
            case 4: nameMap += 0x120; break;
            case 5: nameMap += 0x180; break;
            case 6: nameMap += 0x1E0; break;
#ifdef VERSION_JP
            case 7: nameMap += 0x2A0; break;
            case 8: nameMap += 0x300; break;
            case 9: nameMap += 0x360; break;
            case 10: nameMap += 0x10; break;
            case 11: nameMap += 0xD0; break;
            case 12: nameMap += 0xD0; break;
#else
            case 7: nameMap += 0x240; break;
            case 8: nameMap += 0x2A0; break;
            case 9: nameMap += 0x300; break;
            case 10: nameMap += 0x360; break;
            case 11: nameMap += 0x10; break;
            case 12: nameMap += 0x70; break;
#endif
            case 13: nameMap += 0xD0; break;
            default: nameMap += 0x130; break;
            }
        }

        RequestDma3Copy(nameMap, dest, 0x20);
        RequestDma3Copy(nameMap + 0x20, dest + 0x40, 0x20);
        RequestDma3Copy(nameMap + 0x40, dest + 0x80, 0x20);

#ifdef VERSION_EU
        if (wide) {
            RequestDma3Copy(nameMap, dest + 2, 0x20);
            RequestDma3Copy(nameMap + 0x20, dest + 0x42, 0x20);
            RequestDma3Copy(nameMap + 0x40, dest + 0x82, 0x20);
        }
#endif

        source = gJiminyDigitTiles;
        SplitThreeDecimalDigits(selected + 1, digits);
        dest = (u8*)GetBgCharBase(0) + 0x20;
        RequestDma3Copy(source + digits[0] * 0x20, dest, 0x20);
        dest = (u8*)GetBgCharBase(0) + 0x40;
        RequestDma3Copy(source + digits[1] * 0x20, dest, 0x20);
        dest = (u8*)GetBgCharBase(0) + 0x60;
        RequestDma3Copy(source + digits[2] * 0x20, dest, 0x20);
        SplitThreeDecimalDigits(unlocked, digits);
        dest = (u8*)GetBgCharBase(0) + 0x80;
        RequestDma3Copy(source + digits[0] * 0x20, dest, 0x20);
        dest = (u8*)GetBgCharBase(0) + 0xA0;
        RequestDma3Copy(source + digits[1] * 0x20, dest, 0x20);
        dest = (u8*)GetBgCharBase(0) + 0xC0;
        RequestDma3Copy(source + digits[2] * 0x20, dest, 0x20);
        sJiminyWork->state = JIMINY_STATE_DETAIL;
        DisableBg(2);

        if (sJiminyWork->detail->bgTiles != NULL) {
            EnableBg(3);
            RequestDma3Copy(sJiminyWork->detail->bgTiles,
                (u8*)GetBgCharBase(3) + 0x4000, sJiminyWork->detail->tileSize);

            switch ((u16)sJiminyWork->detail->paletteSize) {
            case 0x60:
                paletteDest = (u16*)(BG_PLTT + 13 * PLTT_SIZE_4BPP);
                LoadPalette(sJiminyWork->detail->bgPalette, paletteDest, 0x60);
                break;
            case 0x40:
                paletteDest = (u16*)(BG_PLTT + 14 * PLTT_SIZE_4BPP);
                LoadPalette(sJiminyWork->detail->bgPalette, paletteDest, 0x40);
                break;
            case 0x20:
            default:
                paletteDest = (u16*)(BG_PLTT + 15 * PLTT_SIZE_4BPP);
                LoadPalette(sJiminyWork->detail->bgPalette, paletteDest, 0x20);
                break;
            }

            LoadBgMap(3, sJiminyWork->detail->bgMap, 0x800);
        } else {
            DisableBg(3);
        }

        sJiminyWork->detailTimer = 5;
        SetBlendAlpha(0, 16);
    case JIMINY_STATE_DETAIL:
        if (sJiminyWork->cursor > 0) {
            sJiminyWork->flags |= JIMINY_FLAG_SCROLL_UP;
        } else {
            sJiminyWork->flags &= ~JIMINY_FLAG_SCROLL_UP;
        }

        if (sJiminyWork->visibleRows < sJiminyWork->itemCount - sJiminyWork->cursor) {
            sJiminyWork->flags |= JIMINY_FLAG_SCROLL_DOWN;
        } else {
            sJiminyWork->flags &= ~JIMINY_FLAG_SCROLL_DOWN;
        }

        if (FadeIsActive()) {
            break;
        }

        if (sJiminyWork->moveDelay <= 0) {
            if (GetKeysRepeat() & DPAD_UP) {
                if (sJiminyWork->cursor > 0) {
                    sJiminyWork->moveDelay = 1;
                    sJiminyWork->cursor--;
                    m4aSongNumStart(SONG_SYS_CLICKI04B);
                    JiminyReloadPlainRows();
                }
            } else if (GetKeysRepeat() & DPAD_DOWN) {
                if (sJiminyWork->visibleRows < sJiminyWork->itemCount - sJiminyWork->cursor) {
                    sJiminyWork->moveDelay = 1;
                    sJiminyWork->cursor++;
                    m4aSongNumStart(SONG_SYS_CLICKI04B);
                    JiminyReloadPlainRows();
                }
            }
        } else {
            sJiminyWork->moveDelay = 0;
        }

        if (sJiminyWork->detailIndex != sJiminyWork->nextDetail) {
            if (GetKeysRepeat() & L_BUTTON) {
                sJiminyWork->state = JIMINY_STATE_OPEN_DETAIL;
                sJiminyWork->stateTimer = 0;
                sJiminyWork->detailIndex = sJiminyWork->prevDetail;
                m4aSongNumStart(SONG_SYS_CANSEL);
                break;
            } else if (GetKeysRepeat() & R_BUTTON) {
                sJiminyWork->state = JIMINY_STATE_OPEN_DETAIL;
                sJiminyWork->stateTimer = 0;
                sJiminyWork->detailIndex = sJiminyWork->nextDetail;
                m4aSongNumStart(SONG_SYS_CANSEL);
                break;
            }
        }

        if (GetKeysPressed() & B_BUTTON) {
            sJiminyWork->stateTimer = 0;
            sJiminyWork->state = JIMINY_STATE_OPEN_LIST;
            FadeStartIn(FADE_MODE_BLACK, 5);
            FadeLock();
            SetModeUpdate(mode_jiminy_1);
            m4aSongNumStart(SONG_SYS_CLOSE);
        } else if (GetKeysPressed() & START_BUTTON) {
            sJiminyWork->stateTimer = 0;
            sJiminyWork->state = JIMINY_STATE_EXIT_TO_MAP;
            m4aSongNumStart(SONG_SYS_CLOSE);
        }

        break;
    case JIMINY_STATE_EXIT_TO_MAP:
        if (sJiminyWork->stateTimer == 0) {
            FadeStartOut(FADE_MODE_BLACK, 16);
            FadeLock();
        }

        if (FadeGetAmount() > 30) {
            ReturnToMap(FALSE);
        }

        break;
    }

    DrawMsgGlyphs(sJiminyWork->charCount);

    for (i = 0; sJiminyWork->lines[i].textSlots[0].tiles != NULL && i < sJiminyWork->visibleRows; i++) {
        DrawTextSlots(sJiminyWork->listX, sJiminyWork->listY + sJiminyWork->rowHeight * i,
            sJiminyWork->lines[i].textSlots, sJiminyWork->palette3, 0, sJiminyWork->textSlotCounts[i]);
    }

    if (sJiminyWork->flags & JIMINY_FLAG_SCROLL_UP) {
        DrawSprite(sJiminyWork->x, sJiminyWork->y - ((sJiminyWork->frame >> 3) & 3) + 4,
            gJiminyScrollArrowFrame0, sJiminyWork->tiles5, sJiminyWork->palette6, NULL, 0, 0);
    }

    if (sJiminyWork->flags & JIMINY_FLAG_SCROLL_DOWN) {
        DrawSprite(sJiminyWork->x2, sJiminyWork->y2 + ((sJiminyWork->frame >> 3) & 3),
            gJiminyScrollArrowFrame1, sJiminyWork->tiles5, sJiminyWork->palette6, NULL, 0, 0);
    }

    if (sJiminyWork->detailIndex != sJiminyWork->nextDetail) {
        DrawSprite(-((sJiminyWork->frame >> 3) & 3) + 0x9A, 5, gJiminyScrollArrowFrame2,
            sJiminyWork->tiles5, sJiminyWork->palette6, NULL, 0, 0);
        DrawSprite(0xE0 + ((sJiminyWork->frame >> 3) & 3), 5, gJiminyScrollArrowFrame3,
            sJiminyWork->tiles5, sJiminyWork->palette6, NULL, 0, 0);
    }

    switch (sJiminyWork->detailLayout) {
    case JIMINY_DETAIL_LAYOUT_STORY:
        if (sJiminyWork->detail->tiles != NULL) {
            DrawSprite(sJiminyWork->detail->x + 0xC8, sJiminyWork->detail->y + 0x5C,
                sJiminyWork->detail->sprite, sJiminyWork->tiles7, sJiminyWork->palette8, NULL, SPRITE_FLAG_BLEND, 1);
        }

        break;
    case JIMINY_DETAIL_LAYOUT_CARD:
        if (sJiminyWork->detail->tiles != NULL) {
            DrawSprite(0xC2, 0x5E, sJiminyWork->detail->sprite,
                sJiminyWork->tiles7, sJiminyWork->palette8, NULL, SPRITE_FLAG_BLEND, 1);
        }

        if (sJiminyWork->detail->tiles2 != NULL && sJiminyWork->detail->sprite2 != NULL) {
            DrawSprite(0xC2, 0x5E, sJiminyWork->detail->sprite2,
                sJiminyWork->tiles8, sJiminyWork->palette9, NULL, SPRITE_FLAG_BLEND, 0);
        }

        break;
    case JIMINY_DETAIL_LAYOUT_CHARACTER:
        if (sJiminyWork->detail->tiles != NULL) {
            DrawSprite(sJiminyWork->detail->x + 0xC4, sJiminyWork->detail->y + 0x74,
                sJiminyWork->detail->sprite, sJiminyWork->tiles7, sJiminyWork->palette8, NULL, SPRITE_FLAG_BLEND, 1);
        }

        if (sJiminyWork->detail->tiles2 != NULL) {
            DrawSprite(0x14, 0x25, sJiminyWork->detail->sprite2,
                sJiminyWork->tiles8, sJiminyWork->palette9, NULL, SPRITE_PRIORITY(1) | SPRITE_FLAG_BLEND, 0);
        }

        break;
    }

    if ((s16)sJiminyWork->detailTimer > 0) {
        sJiminyWork->detailTimer--;
    }

    UpdatePlayTime();
    sJiminyWork->frame++;
}

void mode_jiminy_2() {
    FreeMsgGlyphSprites();
    ReleaseObjTiles(sJiminyWork->tiles);
    ReleaseObjPalette(sJiminyWork->palette);
    ReleaseObjTiles(sJiminyWork->tiles2);
    ReleaseObjPalette(sJiminyWork->palette2);
    ReleaseObjPalette(sJiminyWork->palette3);
    ReleaseObjTiles(sJiminyWork->tiles3);
    ReleaseObjPalette(sJiminyWork->palette4);
    ReleaseObjTiles(sJiminyWork->tiles4);
    ReleaseObjPalette(sJiminyWork->palette5);
    ReleaseObjTiles(sJiminyWork->tiles5);
    ReleaseObjPalette(sJiminyWork->palette6);
    ReleaseObjTiles(sJiminyWork->tiles6);
    ReleaseObjPalette(sJiminyWork->palette7);
    ReleaseObjTiles(sJiminyWork->tiles7);
    ReleaseObjPalette(sJiminyWork->palette8);
    ReleaseObjTiles(sJiminyWork->tiles8);
    ReleaseObjPalette(sJiminyWork->palette9);
    JiminyFreeRows();
    EwramFree(sJiminyWork);
}

Mode gModeJiminy = {
    "mode_jiminy",
    (ModeInitFunc)mode_jiminy_0,
    mode_jiminy_1,
    mode_jiminy_2,
};
