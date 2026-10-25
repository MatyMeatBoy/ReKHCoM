/**
 * bos_md.c
 * Dragon Maleficent Boss
 */

#include "system_state.h"
#include "display.h"
#include "fade.h"
#include "bos_md.h"
#include "anim.h"
#include "sprites_bos5.h"
#include "sprites_worldinspect.h"
#include "chara_types.h"
#include "prize_types.h"
#include "btl_api.h"
#include "songs.h"
#include "battle_actor.h"
#include "battle_actor_types.h"
#include "battle_work.h"
#include "btl_collision.h"
#include "btl_effect.h"
#include "card_api.h"
#include "engine_math.h"
#include "gba/defines.h"
#include "m4a_song.h"
#include "obj.h"
#include "obj_api.h"
#include "taskpool.h"
#include "types.h"
#include <stddef.h>
#include "bos_ga.h"
#include "default_bg_map.h"
#include "sprite_palettes.h"
#include "enemy_ids.h"
#include "macros.h"

#define BOS_MD_VIEW_X_MAX_EXTRA 96

static const EmyKind sBosMdEmyKind = { ENEMY_DRAGON_MALEFICENT, 1000, 16, 16, 0, 60, EMY_KIND_FLAG_NO_COLLIDER };

static const MdMapData sMdMapData = {
    gBosMdBgTiles, sizeof(gBosMdBgTiles), gBosMdBgPalettes, sizeof(gBosMdBgPalettes), { gBosMdBgMap0, gBosMdBgMap1, gBosMdBgMap2, gBosMdBgMap3 }
};

static const MdFrameDef sMdFrameDefs[41] = {
    {
        32, 0, gBosMdFrame0Tiles, sizeof(gBosMdFrame0Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame0Map },
        { { 0, 112, 0xFF90, gBosMdFrame0ArmTiles, sizeof(gBosMdFrame0ArmTiles), gBosMdFrame0ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame2 } },
        { { -100, 0, -45, 0 } },
    },
    {
        32, 0, gBosMdFrame0Tiles, 4480,
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame0Map },
        { { 0, 112, 0xFF90, gBosMdFrame0ArmTiles, sizeof(gBosMdFrame0ArmTiles), gBosMdFrame0ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame2 } },
        { { -100, 0, -45, 0 } },
    },
    {
        32, 0, gBosMdFrame2Tiles, sizeof(gBosMdFrame2Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame2Map },
        { { 0, 112, 0xFF90, gBosMdFrame2ArmTiles, sizeof(gBosMdFrame2ArmTiles), gBosMdFrame2ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame3 } },
        { { -101, 0, -34, 0 } },
    },
    {
        32, 0, gBosMdFrame3Tiles, sizeof(gBosMdFrame3Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame3Map },
        { { 0, 112, 0xFF90, gBosMdFrame3ArmTiles, sizeof(gBosMdFrame3ArmTiles), gBosMdFrame3ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame1 } },
        { { -103, 0, -28, 0 } },
    },
    {
        0, 65521, gBosMdFrame4Tiles, sizeof(gBosMdFrame4Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame4Map },
        { { 0xFFF8, 112, 0xFF90, gBosMdFrame4ArmTiles, sizeof(gBosMdFrame4ArmTiles), gBosMdFrame4ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame1 } },
        { { -131, 0, -108, 0 } },
    },
    {
        3, 65524, gBosMdFrame5Tiles, sizeof(gBosMdFrame5Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame5Map },
        { { 0, 112, 0xFF90, gBosMdFrame5ArmTiles, sizeof(gBosMdFrame5ArmTiles), gBosMdFrame5ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame3 } },
        { { -142, 0, -105, 0 } },
    },
    {
        4, 65529, gBosMdFrame6Tiles, sizeof(gBosMdFrame6Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame6Map },
        { { 0, 112, 0xFF8F, gBosMdFrame6ArmTiles, sizeof(gBosMdFrame6ArmTiles), gBosMdFrame6ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame2 } },
        { { -145, 0, -94, 0 } },
    },
    {
        43, 65504, gBosMdFrame7Tiles, sizeof(gBosMdFrame7Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame7Map },
        { { 0, 112, 0xFF90, gBosMdFrame7ArmTiles, sizeof(gBosMdFrame7ArmTiles), gBosMdFrame7ArmFrame0 }, { 11, 56, 0xFFE0, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame0 } },
        { { -67, 0, -126, 0 } },
    },
    {
        43, 65528, gBosMdFrame8Tiles, sizeof(gBosMdFrame8Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame8Map },
        { { 0, 112, 0xFF90, gBosMdFrame8ArmTiles, sizeof(gBosMdFrame8ArmTiles), gBosMdFrame8ArmFrame0 }, { 24, 56, 0xFFE2, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame4 } },
        { { -46, 0, -154, 0 } },
    },
    {
        31, 0, gBosMdFrame9Tiles, sizeof(gBosMdFrame9Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame9Map },
        { { 0, 112, 0xFF90, gBosMdFrame9ArmTiles, sizeof(gBosMdFrame9ArmTiles), gBosMdFrame9ArmFrame0 }, { 2, 56, 0xFFF2, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame6 } },
        { { -105, 0, -135, 0 } },
    },
    {
        29, 65531, gBosMdFrame10Tiles, sizeof(gBosMdFrame10Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame10Map },
        { { 0, 112, 0xFF90, gBosMdFrame10ArmTiles, sizeof(gBosMdFrame10ArmTiles), gBosMdFrame10ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame2 } },
        { { -123, 0, -42, 0 } },
    },
    {
        25, 65527, gBosMdFrame11Tiles, sizeof(gBosMdFrame11Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame11Map },
        { { 0, 112, 0xFF91, gBosMdFrame11ArmTiles, sizeof(gBosMdFrame11ArmTiles), gBosMdFrame11ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame1 } },
        { { -123, 0, -30, 0 } },
    },
    {
        22, 65519, gBosMdFrame12Tiles, sizeof(gBosMdFrame12Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame12Map },
        { { 0xFFFF, 112, 0xFF90, gBosMdFrame12ArmTiles, sizeof(gBosMdFrame12ArmTiles), gBosMdFrame12ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame1 } },
        { { -129, 0, -36, 0 } },
    },
    {
        36, 65518, gBosMdFrame13Tiles, sizeof(gBosMdFrame13Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame13Map },
        { { 0, 112, 0xFF90, gBosMdFrame13ArmTiles, sizeof(gBosMdFrame13ArmTiles), gBosMdFrame13ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame1 } },
        { { -115, 0, -31, 0 } },
    },
    {
        35, 65533, gBosMdFrame14Tiles, sizeof(gBosMdFrame14Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame14Map },
        { { 0, 112, 0xFF90, gBosMdFrame14ArmTiles, sizeof(gBosMdFrame14ArmTiles), gBosMdFrame14ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame1 } },
        { { -98, 0, -49, 0 } },
    },
    {
        35, 65530, gBosMdFrame4Tiles, sizeof(gBosMdFrame4Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame4Map },
        { { 0, 112, 0xFF90, gBosMdFrame15ArmTiles, sizeof(gBosMdFrame15ArmTiles), gBosMdFrame15ArmFrame0 }, { 20, 56, 0xFFCF, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame4 } },
        { { -94, 0, -100, 0 } },
    },
    {
        45, 65535, gBosMdFrame5Tiles, sizeof(gBosMdFrame5Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame5Map },
        { { 0, 112, 0xFF90, gBosMdFrame16ArmTiles, sizeof(gBosMdFrame16ArmTiles), gBosMdFrame16ArmFrame0 }, { 19, 56, 0xFFED, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame5 } },
        { { -104, 0, -94, 0 } },
    },
    {
        48, 6, gBosMdFrame6Tiles, sizeof(gBosMdFrame6Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame6Map },
        { { 0, 112, 0xFF90, gBosMdFrame17ArmTiles, sizeof(gBosMdFrame17ArmTiles), gBosMdFrame17ArmFrame0 }, { 17, 56, 0xFFF3, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame6 } },
        { { -105, 0, -79, 0 } },
    },
    {
        56, 11, gBosMdFrame18Tiles, sizeof(gBosMdFrame18Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame18Map },
        { { 0, 112, 0xFF90, gBosMdFrame18ArmTiles, sizeof(gBosMdFrame18ArmTiles), gBosMdFrame18ArmFrame0 }, { 23, 56, 0xFFF8, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame1 } },
        { { -103, 0, -57, 0 } },
    },
    {
        35, 65533, gBosMdFrame19Tiles, sizeof(gBosMdFrame19Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame19Map },
        { { 0, 112, 0xFF9A, gBosMdFrame19ArmTiles, sizeof(gBosMdFrame19ArmTiles), gBosMdFrame19ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame1 } },
        { { -98, 0, -78, 0 } },
    },
    {
        36, 65535, gBosMdFrame20Tiles, sizeof(gBosMdFrame20Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame20Map },
        { { 0, 112, 0xFF92, gBosMdFrame20ArmTiles, sizeof(gBosMdFrame20ArmTiles), gBosMdFrame20ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame3 } },
        { { -102, 0, -59, 0 } },
    },
    {
        38, 0, gBosMdFrame21Tiles, sizeof(gBosMdFrame21Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame21Map },
        { { 0, 112, 0xFF89, gBosMdFrame21ArmTiles, sizeof(gBosMdFrame21ArmTiles), gBosMdFrame21ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame3 } },
        { { -102, 0, -52, 0 } },
    },
    {
        37, 0, gBosMdFrame22Tiles, sizeof(gBosMdFrame22Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame22Map },
        { { 0, 112, 0xFF8D, gBosMdFrame22ArmTiles, sizeof(gBosMdFrame22ArmTiles), gBosMdFrame22ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame2 } },
        { { -100, 0, -50, 0 } },
    },
    {
        5, 5, gBosMdFrame3Tiles, sizeof(gBosMdFrame3Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame3Map },
        { { 0xFFFF, 112, 0xFF90, gBosMdFrame23ArmTiles, sizeof(gBosMdFrame23ArmTiles), gBosMdFrame23ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame1 } },
        { { -128, 0, -22, 0 } },
    },
    {
        41, 65504, gBosMdFrame24Tiles, sizeof(gBosMdFrame24Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame24Map },
        { { 0, 112, 0xFF90, gBosMdFrame7ArmTiles, sizeof(gBosMdFrame7ArmTiles), gBosMdFrame7ArmFrame0 }, { 13, 56, 0xFFDF, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame0 } },
        { { -108, 0, -88, 0 } },
    },
    {
        46, 65499, gBosMdFrame7Tiles, sizeof(gBosMdFrame7Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame7Map },
        { { 11, 112, 0xFF85, gBosMdFrame19ArmTiles, sizeof(gBosMdFrame19ArmTiles), gBosMdFrame19ArmFrame0 }, { 29, 56, 0xFFBA, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame4 } },
        { { -62, 0, -126, 0 } },
    },
    {
        60, 65491, gBosMdFrame26Tiles, sizeof(gBosMdFrame26Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame26Map },
        { { 21, 112, 0xFF70, gBosMdFrame20ArmTiles, sizeof(gBosMdFrame20ArmTiles), gBosMdFrame20ArmFrame0 }, { 40, 56, 0xFFC4, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame5 } },
        { { -3, 0, -168, 0 } },
    },
    {
        36, 65527, gBosMdFrame9Tiles, sizeof(gBosMdFrame9Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame9Map },
        { { 0, 112, 0xFF89, gBosMdFrame21ArmTiles, sizeof(gBosMdFrame21ArmTiles), gBosMdFrame21ArmFrame0 }, { 17, 56, 0xFFF3, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame6 } },
        { { -103, 0, -142, 0 } },
    },
    {
        1, 65530, gBosMdFrame18Tiles, sizeof(gBosMdFrame18Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame18Map },
        { { 0, 112, 0xFF8F, gBosMdFrame6ArmTiles, sizeof(gBosMdFrame6ArmTiles), gBosMdFrame6ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame1 } },
        { { -158, 0, -75, 0 } },
    },
    {
        36, 65518, gBosMdFrame13Tiles, sizeof(gBosMdFrame13Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame13Map },
        { { 0, 112, 0xFF90, gBosMdFrame13ArmTiles, sizeof(gBosMdFrame13ArmTiles), gBosMdFrame13ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame1 } },
        { { -115, 0, -31, 0 } },
    },
    {
        35, 65533, gBosMdFrame14Tiles, sizeof(gBosMdFrame14Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame14Map },
        { { 0, 112, 0xFF90, gBosMdFrame14ArmTiles, sizeof(gBosMdFrame14ArmTiles), gBosMdFrame14ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame1 } },
        { { -98, 0, -49, 0 } },
    },
    {
        0, 65525, gBosMdFrame26Tiles, sizeof(gBosMdFrame26Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame26Map },
        { { 0xFFF8, 112, 0xFF90, gBosMdFrame4ArmTiles, sizeof(gBosMdFrame4ArmTiles), gBosMdFrame4ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame1 } },
        { { -62, 0, -136, 0 } },
    },
    {
        0, 65530, gBosMdFrame32Tiles, sizeof(gBosMdFrame32Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame32Map },
        { { 0, 112, 0xFF90, gBosMdFrame5ArmTiles, sizeof(gBosMdFrame5ArmTiles), gBosMdFrame5ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame1 } },
        { { -98, 0, -135, 0 } },
    },
    {
        0, 65535, gBosMdFrame24Tiles, sizeof(gBosMdFrame24Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame24Map },
        { { 0, 112, 0xFF8F, gBosMdFrame6ArmTiles, sizeof(gBosMdFrame6ArmTiles), gBosMdFrame6ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame1 } },
        { { -149, -20, -38, 0 } },
    },
    {
        0, 65535, gBosMdFrame34Tiles, sizeof(gBosMdFrame34Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame34Map },
        { { 0, 112, 0xFF90, gBosMdFrame34ArmTiles, sizeof(gBosMdFrame34ArmTiles), gBosMdFrame34ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame1 } },
        { { -140, 17, -24, 0 } },
    },
    {
        0, 65532, gBosMdFrame35Tiles, sizeof(gBosMdFrame35Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame35Map },
        { { 0, 112, 0xFF90, gBosMdFrame35ArmTiles, sizeof(gBosMdFrame35ArmTiles), gBosMdFrame35ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame1 } },
        { { -171, 0, -78, 0 } },
    },
    {
        0, 14, gBosMdFrame36Tiles, sizeof(gBosMdFrame36Tiles),
        { gDefaultBgMap, gDefaultBgMap, gBosMdFrame36LeftMap, gBosMdFrame36Map },
        { { 0, 112, 0xFF90, gBosMdFrame36ArmTiles, sizeof(gBosMdFrame36ArmTiles), gBosMdFrame36ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame1 } },
        { { -190, -20, -33, 0 } },
    },
    {
        3, 12, gBosMdFrame37Tiles, sizeof(gBosMdFrame37Tiles),
        { gDefaultBgMap, gDefaultBgMap, gBosMdFrame37LeftMap, gBosMdFrame37Map },
        { { 0, 112, 0xFF90, gBosMdFrame23ArmTiles, sizeof(gBosMdFrame23ArmTiles), gBosMdFrame23ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame1 } },
        { { -181, 17, -25, 0 } },
    },
    {
        31, 2, gBosMdFrame38Tiles, sizeof(gBosMdFrame38Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame38Map },
        { { 0, 112, 0xFF90, gBosMdFrame38ArmTiles, sizeof(gBosMdFrame38ArmTiles), gBosMdFrame38ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame2 } },
        { { -86, 0, -26, 0 } },
    },
    {
        32, 65530, gBosMdFrame7Tiles, sizeof(gBosMdFrame7Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame7Map },
        { { 0, 112, 0xFF90, gBosMdFrame0ArmTiles, sizeof(gBosMdFrame0ArmTiles), gBosMdFrame0ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame1 } },
        { { -79, 0, -98, 0 } },
    },
    {
        32, 4, gBosMdFrame13Tiles, sizeof(gBosMdFrame13Tiles),
        { gDefaultBgMap, gDefaultBgMap, gDefaultBgMap, gBosMdFrame13Map },
        { { 0, 112, 0xFF90, gBosMdFrame38ArmTiles, sizeof(gBosMdFrame38ArmTiles), gBosMdFrame38ArmFrame0 }, { 0, 56, 0xFFEE, gBosMdForearmTiles, sizeof(gBosMdForearmTiles), gBosMdForearmFrame1 } },
        { { -119, 19, -27, 0 } },
    },
};

static const MdAnimFrame sMdIdleFrames[4] = { { 1, 18 }, { 2, 12 }, { 3, 24 }, { 2, 12 } };

static const MdAnimFrame sMdIdleNearFrames[4] = { { 4, 18 }, { 5, 12 }, { 6, 24 }, { 5, 12 } };

static const MdAnimFrame sMdFireBreathStartFrames[4] = { { 7, 12 }, { 8, 30 }, { 9, 3 }, { 10, 6 } };

static const MdAnimFrame sMdFireBreathFrames[2] = { { 11, 24 }, { 12, 6 } };

static const MdAnimFrame sMdFireBreathEndFrames[2] = { { 13, 6 }, { 14, 6 } };

static const MdAnimFrame sMdQuakeDoubleFrames[10] = { { 15, 6 }, { 16, 3 }, { 17, 3 }, { 18, 24 }, { 1, 6 }, { 19, 6 }, { 20, 3 }, { 21, 3 }, { 22, 24 }, { 1, 6 } };

static const MdAnimFrame sMdQuakeHeavyFrames[8] = { { 23, 6 }, { 24, 6 }, { 25, 6 }, { 26, 30 }, { 27, 3 }, { 28, 6 }, { 29, 6 }, { 30, 6 } };

static const MdAnimFrame sMdBiteNearFrames[6] = { { 39, 3 }, { 31, 24 }, { 32, 3 }, { 33, 3 }, { 34, 24 }, { 40, 3 } };

static const MdAnimFrame sMdBiteFarFrames[8] = { { 39, 3 }, { 31, 24 }, { 32, 3 }, { 35, 3 }, { 36, 3 }, { 37, 24 }, { 34, 6 }, { 40, 6 } };

static const MdAnimFrame sMdBiteDoubleFrames[11] = { { 39, 3 }, { 31, 24 }, { 32, 3 }, { 33, 3 }, { 34, 24 }, { 35, 6 }, { 32, 12 }, { 36, 3 }, { 37, 24 }, { 34, 6 }, { 38, 6 } };

static const MdAnimFrame sMdDefeatFrames[1] = { { 1, 32767 } };

static const MdAnimDef sMdAnimDefs[11] = {
    { sMdIdleFrames, ARRAY_COUNT(sMdIdleFrames) },
    { sMdIdleNearFrames, ARRAY_COUNT(sMdIdleNearFrames) },
    { sMdFireBreathStartFrames, ARRAY_COUNT(sMdFireBreathStartFrames) },
    { sMdFireBreathFrames, ARRAY_COUNT(sMdFireBreathFrames) },
    { sMdFireBreathEndFrames, ARRAY_COUNT(sMdFireBreathEndFrames) },
    { sMdQuakeDoubleFrames, ARRAY_COUNT(sMdQuakeDoubleFrames) },
    { sMdQuakeHeavyFrames, ARRAY_COUNT(sMdQuakeHeavyFrames) },
    { sMdBiteNearFrames, ARRAY_COUNT(sMdBiteNearFrames) },
    { sMdBiteFarFrames, ARRAY_COUNT(sMdBiteFarFrames) },
    { sMdBiteDoubleFrames, ARRAY_COUNT(sMdBiteDoubleFrames) },
    { sMdDefeatFrames, ARRAY_COUNT(sMdDefeatFrames) },
};

TaskDesc gTaskDescBosMd = {
    "task_bos_md",
    (TaskInitFunc)task_bos_md_0,
    (TaskUpdateFunc)task_bos_md_1,
    (TaskDrawFunc)task_bos_md_2,
    (TaskDestroyFunc)task_bos_md_3,
    sizeof(MdWork),
};

static TaskDesc sTaskDescBosMdMap = {
    "task_bos_md_map",
    (TaskInitFunc)task_bos_md_map_0,
    (TaskUpdateFunc)task_bos_md_map_1,
    NULL,
    NULL,
    sizeof(MdMapWork),
};

static const MdFirePoint sMdFirePoints0[4] = { { 88, 288, 240, 0 }, { 56, 312, 330, 0 }, { 72, 336, 420, 0 }, { 104, 360, 510, 0 } };

static const MdFirePoint sMdFirePoints1[4] = { { 128, 304, 0, 0 }, { 104, 324, 0, 0 }, { 144, 344, 0, 0 }, { 112, 364, 0, 0 } };

static const MdFirePoint sMdFirePoints2[4] = { { 128, 288, 0, 0 }, { 156, 312, 0, 0 }, { 172, 336, 0, 0 }, { 144, 360, 0, 0 } };

enum BosMdFirePattern {
    BOS_MD_FIRE_PATTERN_POINTS_0,
    BOS_MD_FIRE_PATTERN_POINTS_1,
    BOS_MD_FIRE_PATTERN_POINTS_2,
    BOS_MD_FIRE_PATTERN_RING,
    BOS_MD_FIRE_PATTERN_ROWS,
    BOS_MD_FIRE_PATTERN_COLUMN
};

static const MdFireDef sMdFireDefs[6] = {
    { sMdFirePoints0, ARRAY_COUNT(sMdFirePoints0) },
    { sMdFirePoints1, ARRAY_COUNT(sMdFirePoints1) },
    { sMdFirePoints2, ARRAY_COUNT(sMdFirePoints2) },
    { sMdFirePoints0, ARRAY_COUNT(sMdFirePoints0) },
    { sMdFirePoints0, ARRAY_COUNT(sMdFirePoints0) },
    { sMdFirePoints0, ARRAY_COUNT(sMdFirePoints0) },
};

static const EmyKind sBosMdFireEmyKind = { ENEMY_DRAGON_MALEFICENT, 1000, 16, 16, 0, 60, EMY_KIND_FLAG_NO_COLLIDER };

static TaskDesc sTaskDescBosMdFire = {
    "task_bos_md_fire",
    (TaskInitFunc)task_bos_md_fire_0,
    (TaskUpdateFunc)task_bos_md_fire_1,
    (TaskDrawFunc)task_bos_md_fire_2,
    (TaskDestroyFunc)task_bos_md_fire_3,
    sizeof(MdFireWork),
};

static TaskDesc sTaskDescBosMdDai = {
    "task_bos_md_dai",
    (TaskInitFunc)task_bos_md_dai_0,
    (TaskUpdateFunc)task_bos_md_dai_1,
    (TaskDrawFunc)task_bos_md_dai_2,
    (TaskDestroyFunc)task_bos_md_dai_3,
    sizeof(MdDaiWork),
};

static TaskDesc sTaskDescBosMdHahen = {
    "task_bos_md_hahen",
    (TaskInitFunc)task_bos_md_hahen_0,
    (TaskUpdateFunc)task_bos_md_hahen_1,
    (TaskDrawFunc)task_bos_md_hahen_2,
    (TaskDestroyFunc)task_bos_md_hahen_3,
    sizeof(MdHahenWork),
};

void BosMdRequestState(MdWork* work, s32 state) {
    u16 flags;

    work->nextState = state;
    flags = work->flags | MD_FLAG_STATE_REQUESTED;
    work->flags = flags;
}

void BosMdSetBgMap(MdWork* work, u16 index) {
    SetBgMapBlocks(1, (void*)sMdFrameDefs[index].blocks, 2, 2);
}

void BosMdLoadBgTiles(MdWork* work, u16 index) {
    LoadBgTiles(1, sMdFrameDefs[index].tiles, sMdFrameDefs[index].tilesSize);
}

void BosMdSetFrame(MdWork* work, u16 id) {
    s32 n;

    if (!work->bgVisible) {
        return;
    }

    BosMdLoadBgTiles(work, id);
    BosMdSetBgMap(work, id);
    work->bgOffsetX = sMdFrameDefs[id].bgOffsetX;
    work->bgOffsetY = sMdFrameDefs[id].bgOffsetY;

    for (n = 0; n < 2; n++) {
        work->gfx[n].sprite = sMdFrameDefs[id].desc[n].sprite;

        if (work->gfx[n].src != sMdFrameDefs[id].desc[n].src && n == 0) {
            work->gfx[n].src = sMdFrameDefs[id].desc[n].src;
            UpdateSpriteFrameTiles(work->gfx[n].tiles, work->gfx[n].sprite, work->gfx[n].src);
        }

        work->gfx[n].x = sMdFrameDefs[id].desc[n].x;
        work->gfx[n].y = sMdFrameDefs[id].desc[n].y;
        work->gfx[n].z = sMdFrameDefs[id].desc[n].z;
    }
}

void MdAnimStart(MdWork* work, s16 id) {
    MdAnim* anim;
    const MdAnimDef* defs;
    const MdAnimDef* def;
    const MdAnimFrame* frames;

    anim = &work->anim;
    anim->animId = id;
    defs = sMdAnimDefs;
    def = defs + id;
    frames = def->frames;
    anim->frames = frames;
    anim->frameCount = def->frameCount;
    anim->frame = 0;
    anim->timer = frames->duration;
    BosMdSetFrame(work, frames->gfxIndex);
}

void MdAnimUpdate(MdWork* work) {
    MdAnim* anim;

    anim = &work->anim;
    anim->timer--;

    if (anim->timer < 0) {
        anim->frame++;

        if (anim->frame >= anim->frameCount) {
            anim->frame = 0;
        }

        anim->timer = anim->frames[anim->frame].duration;
        BosMdSetFrame(work, anim->frames[anim->frame].gfxIndex);
    }
}

u8 BosMdAnimIsLastFrame(MdWork* work) {
    MdAnim* anim;

    anim = &work->anim;

    if (anim->frame >= anim->frameCount - 1) {
        return TRUE;
    }

    return FALSE;
}

enum BosMdState {
    BOS_MD_STATE_IDLE,
    BOS_MD_STATE_BITE,
    BOS_MD_STATE_QUAKE,
    BOS_MD_STATE_FIRE_BREATH,
    BOS_MD_STATE_DEFEATED
};

enum BosMdPhase {
    BOS_MD_PHASE_ENTER,
    BOS_MD_PHASE_UPDATE,
    BOS_MD_PHASE_EXIT
};

u8 BosMdUpdateIdle(MdWork* work) {
    if (work->flags & MD_FLAG_STATE_REQUESTED) {
        work->statePhase = BOS_MD_PHASE_EXIT;
    }

    switch (work->statePhase) {
    case BOS_MD_PHASE_ENTER:
        if (gBtlWork->actor->x > 0x8000) {
            MdAnimStart(work, 1);
        } else {
            MdAnimStart(work, 0);
        }

        if (gBtlWork->flags & 0x100000) {
            work->timer = ((GetRandom() & 3) + (GetRandom() & 3)) / 2 * 20 + 60;
#ifdef PLATFORM_PC
        /* @bug: a frame before the maximum HP is set divides by zero (the gauges' #40 class) */
        } else if (work->sub[0].maxHp > 0 && work->sub[0].hp * 10 / work->sub[0].maxHp > 4) {
#else
        } else if (work->sub[0].hp * 10 / work->sub[0].maxHp > 4) {
#endif
            work->timer = (GetRandom() % 5 + GetRandom() % 5) / 2 * 30 + 30;
        } else {
            work->timer = (GetRandom() % 6 + GetRandom() % 6) / 2 * 10 + 30;
        }

        break;
    case BOS_MD_PHASE_UPDATE:
        if (BosMdAnimIsLastFrame(work) && gBtlWork->actor->x > 0x8000) {
            MdAnimStart(work, 1);
        }

        if (gBtlWork->phase != BTL_PHASE_START) {
            work->timer--;

            if (work->timer <= 0) {
                RequestEnemyCardUse(&work->sub[0]);
                BosMdRequestState(work, BOS_MD_STATE_IDLE);
            }
        }

        break;
    case BOS_MD_PHASE_EXIT:
        break;
    }

    if (work->statePhase == BOS_MD_PHASE_ENTER) {
        work->statePhase = BOS_MD_PHASE_UPDATE;
    }

    if (work->statePhase == BOS_MD_PHASE_EXIT) {
        work->state = work->nextState;
        work->statePhase = BOS_MD_PHASE_ENTER;
        work->flags &= ~MD_FLAG_STATE_REQUESTED;
    }

    return 1;
}

u8 BosMdUpdateBite(MdWork* work) {
    s32 actorX;
    u16 roll;

    if (work->flags & MD_FLAG_STATE_REQUESTED) {
        work->statePhase = BOS_MD_PHASE_EXIT;
    }

    switch (work->statePhase) {
        case BOS_MD_PHASE_ENTER:
            actorX = gBtlWork->actor->x;

            if (actorX > 0xA800) {
                switch (GetRandom() % 3) {
                case 0:
                    MdAnimStart(work, 7);
                    break;
                case 1:
                    MdAnimStart(work, 8);
                    break;
                case 2:
                    MdAnimStart(work, 9);
                    break;
                }
            } else if (actorX > 0x7000) {
                roll = GetRandom() % 100;

                if (roll <= 59) {
                    MdAnimStart(work, 7);
                } else if (roll <= 89) {
                    MdAnimStart(work, 8);
                } else {
                    MdAnimStart(work, 9);
                }
            } else if (actorX > 0x3800) {
                roll = GetRandom() % 100;

                if (roll <= 59) {
                    MdAnimStart(work, 8);
                } else if (roll <= 89) {
                    MdAnimStart(work, 7);
                } else {
                    MdAnimStart(work, 9);
                }
            } else {
                switch (GetRandom() % 3) {
                case 0:
                    MdAnimStart(work, 7);
                    break;
                case 1:
                    MdAnimStart(work, 8);
                    break;
                case 2:
                    MdAnimStart(work, 9);
                    break;
                }
            }

            break;
        case BOS_MD_PHASE_UPDATE:
            switch (work->anim.frames[work->anim.frame].gfxIndex) {
            case 33:
            case 34:
            case 36:
            case 37:
                if (ApplyAttackBox(251, work->sub[0].x, work->sub[0].y,
                                  work->sub[0].z, 40, 20, 24) != ATTACK_RESULT_NONE) {
                    m4aSongNumStart(SONG_BTL_DRGN_BITE);
                }

                break;
            }

            if (BosMdAnimIsLastFrame(work)) {
                BosMdRequestState(work, BOS_MD_STATE_IDLE);
            }

            break;
        case BOS_MD_PHASE_EXIT:
            ClearBtlObjActionFlags(&work->sub[0]);
            break;
    }

    if (work->statePhase == BOS_MD_PHASE_ENTER) {
        work->statePhase = BOS_MD_PHASE_UPDATE;
    }

    if (work->statePhase == BOS_MD_PHASE_EXIT) {
        work->state = work->nextState;
        work->statePhase = BOS_MD_PHASE_ENTER;
        work->flags &= ~MD_FLAG_STATE_REQUESTED;
    }

    return 1;
}

enum BosMdQuakeStep {
    BOS_MD_QUAKE_STEP_FIRST_SLAM,
    BOS_MD_QUAKE_STEP_SECOND_SLAM,
    BOS_MD_QUAKE_STEP_DONE
};

u8 BosMdUpdateQuake(MdWork* work) {
    s32 gfxIndex;

    if (work->flags & MD_FLAG_STATE_REQUESTED) {
        work->statePhase = BOS_MD_PHASE_EXIT;
    }

    switch (work->statePhase) {
        case BOS_MD_PHASE_ENTER:
            if (gBtlWork->actor->x > 0xA800) {
                if (GetRandom() % 100 <= 59) {
                    MdAnimStart(work, 6);
                } else {
                    MdAnimStart(work, 5);
                }
            } else {
                if (GetRandom() % 100 <= 79) {
                    MdAnimStart(work, 5);
                } else {
                    MdAnimStart(work, 6);
                }
            }

            work->step = BOS_MD_QUAKE_STEP_FIRST_SLAM;
            break;
        case BOS_MD_PHASE_UPDATE:
            switch (work->step) {
            case BOS_MD_QUAKE_STEP_FIRST_SLAM:
                gfxIndex = work->anim.frames[work->anim.frame].gfxIndex;

                if (gfxIndex == 18) {
                    ApplyAttackBox(252, gBtlWork->viewX, gBtlWork->viewY, 0,
                                  256, 256, 1);
                    m4aSongNumStart(SONG_BTL_DRGN_RUMB);
                    BtlMapStartShake();
                    work->signals |= MD_SIGNAL_QUAKE_SLAM;
                    work->step = BOS_MD_QUAKE_STEP_SECOND_SLAM;
                } else if (gfxIndex == 28) {
                    ApplyAttackBox(254, gBtlWork->viewX, gBtlWork->viewY, 0,
                                  256, 256, 1);
                    m4aSongNumStart(SONG_BTL_DRGN_RUMB);
                    BtlMapStartShake();
                    work->signals |= MD_SIGNAL_QUAKE_SLAM;
                    work->step = BOS_MD_QUAKE_STEP_DONE;
                }

                break;
            case BOS_MD_QUAKE_STEP_SECOND_SLAM:
                gfxIndex = work->anim.frames[work->anim.frame].gfxIndex;

                if (gfxIndex == 22) {
                    MakeOpponentsHittable();
                    ApplyAttackBox(252, gBtlWork->viewX, gBtlWork->viewY, 0,
                                  256, 256, 1);
                    m4aSongNumStart(SONG_BTL_DRGN_RUMB);
                    BtlMapStartShake();
                    work->signals |= MD_SIGNAL_QUAKE_SLAM;
                    work->step = BOS_MD_QUAKE_STEP_DONE;
                }

                break;
            }

            if (BosMdAnimIsLastFrame(work)) {
                BosMdRequestState(work, BOS_MD_STATE_IDLE);
            }

            break;
        case BOS_MD_PHASE_EXIT:
            ClearBtlObjActionFlags(&work->sub[0]);
            break;
    }

    if (work->statePhase == BOS_MD_PHASE_ENTER) {
        work->statePhase = BOS_MD_PHASE_UPDATE;
    }

    if (work->statePhase == BOS_MD_PHASE_EXIT) {
        work->state = work->nextState;
        work->statePhase = BOS_MD_PHASE_ENTER;
        work->flags &= ~MD_FLAG_STATE_REQUESTED;
    }

    return 1;
}

enum BosMdFireBreathStep {
    BOS_MD_FIRE_BREATH_STEP_WINDUP,
    BOS_MD_FIRE_BREATH_STEP_BREATHE,
    BOS_MD_FIRE_BREATH_STEP_RECOVER
};

u8 BosMdUpdateFireBreath(MdWork* work) {
    MdFireArg fireArg;

    if (work->flags & MD_FLAG_STATE_REQUESTED) {
        work->statePhase = BOS_MD_PHASE_EXIT;
    }

    switch (work->statePhase) {
        case BOS_MD_PHASE_ENTER:
            work->signals = work->signals | MD_SIGNAL_CLEAR_FIRES;
            MdAnimStart(work, 2);
            work->step = BOS_MD_FIRE_BREATH_STEP_WINDUP;
            break;
        case BOS_MD_PHASE_UPDATE:
            switch (work->step) {
                case BOS_MD_FIRE_BREATH_STEP_WINDUP:
                    if (BosMdAnimIsLastFrame(work)) {
                        MdAnimStart(work, 3);
                        BgFxStartDragonFire(work->sub[0].x, work->sub[0].y,
                                      work->sub[0].z + 0x1200, Q_8_8(2));
                        m4aSongNumStart(SONG_EF_DRGN_FIRE);
                        work->step = BOS_MD_FIRE_BREATH_STEP_BREATHE;
                    }

                    break;
                case BOS_MD_FIRE_BREATH_STEP_BREATHE:
                    if (ApplyAttackBox(253, work->sub[0].x,
                                      work->sub[0].y + 0x1800, 0, 72, 48, 1) != ATTACK_RESULT_NONE) {
                        m4aSongNumStart(SONG_SND_714);
                    }

                    if (!BgFxIsActive()) {
                        work->signals &= ~MD_SIGNAL_CLEAR_FIRES;
                        fireArg.pool = &work->tasks;
                        fireArg.index = 0;
                        fireArg.flags = &work->signals;

#ifdef PLATFORM_PC
                        /* @bug: as BOS_MD_PHASE_ENTER, no division by a maximum HP still 0 (#40 class) */
                        if (work->sub[0].maxHp > 0 && work->sub[0].hp * 10 / work->sub[0].maxHp > 4) {
#else
                        if (work->sub[0].hp * 10 / work->sub[0].maxHp > 4) {
#endif
                            switch (GetRandom() % 3) {
                            case 0:
                                fireArg.pattern = BOS_MD_FIRE_PATTERN_POINTS_0;
                                break;
                            case 1:
                                fireArg.pattern = BOS_MD_FIRE_PATTERN_POINTS_1;
                                break;
                            case 2:
                                fireArg.pattern = BOS_MD_FIRE_PATTERN_POINTS_2;
                                break;
                            }
                        } else {
                            switch (GetRandom() % 3) {
                            case 0:
                                fireArg.pattern = BOS_MD_FIRE_PATTERN_RING;
                                break;
                            case 1:
                                fireArg.pattern = BOS_MD_FIRE_PATTERN_ROWS;
                                break;
                            case 2:
                                fireArg.pattern = BOS_MD_FIRE_PATTERN_COLUMN;
                                break;
                            }
                        }

                        TaskCreate(&work->tasks, &sTaskDescBosMdFire, &fireArg);
                        MdAnimStart(work, 4);
                        work->step = BOS_MD_FIRE_BREATH_STEP_RECOVER;
                    }

                    break;
                case BOS_MD_FIRE_BREATH_STEP_RECOVER:
                    if (BosMdAnimIsLastFrame(work)) {
                        BosMdRequestState(work, BOS_MD_STATE_IDLE);
                    }

                    break;
            }

            break;
        case BOS_MD_PHASE_EXIT:
            ClearBtlObjActionFlags(&work->sub[0]);
            break;
    }

    if (work->statePhase == BOS_MD_PHASE_ENTER) {
        work->statePhase = BOS_MD_PHASE_UPDATE;
    }

    if (work->statePhase == BOS_MD_PHASE_EXIT) {
        work->state = work->nextState;
        work->statePhase = BOS_MD_PHASE_ENTER;
        work->flags &= ~MD_FLAG_STATE_REQUESTED;
    }

    return 1;
}

enum BosMdDefeatStep {
    BOS_MD_DEFEAT_STEP_WAIT_FADE,
    BOS_MD_DEFEAT_STEP_DEATH_FX,
    BOS_MD_DEFEAT_STEP_DEATH_FLASH
};

u8 BosMdUpdateDefeat(MdWork* work) {
    u8 result;
    PrizeCardArg prize;

    result = 1;

    if (work->flags & MD_FLAG_STATE_REQUESTED) {
        work->statePhase = BOS_MD_PHASE_EXIT;
    }

    switch (work->statePhase) {
        case BOS_MD_PHASE_ENTER:
            MdAnimStart(work, 10);
            BeginBossDefeat(&work->sub[0]);
            work->signals |= MD_SIGNAL_CLEAR_FIRES;
            work->step = BOS_MD_DEFEAT_STEP_WAIT_FADE;
            break;
        case BOS_MD_PHASE_UPDATE:
            switch (work->step) {
            case BOS_MD_DEFEAT_STEP_WAIT_FADE:
                if (!FadeIsActive()) {
                    BgFxStartBossDeath(work->sub[0].x,
                                  work->sub[0].y + work->sub[0].z);
                    FadeToAmount(FADE_MODE_BLACK, gBtlWork->fadeAmount, 8);
                    work->timer = 120;
                    work->step = BOS_MD_DEFEAT_STEP_DEATH_FX;
                }

                break;
            case BOS_MD_DEFEAT_STEP_DEATH_FX:
                work->timer--;

                if (work->timer <= 0) {
                    BgFxStartBossDeathFlash();
                    work->step = BOS_MD_DEFEAT_STEP_DEATH_FLASH;
                }

                break;
            case BOS_MD_DEFEAT_STEP_DEATH_FLASH:
                if (work->bgVisible && FadeGetAmount() == 31) {
                    DisableBg(1);
                    work->bgVisible = FALSE;
                }

                if (!BgFxIsActive()) {
                    prize.x = work->sub[0].x;
                    prize.y = work->sub[0].y;
                    prize.z = work->sub[0].z;
                    CreateBossPrizeCardTask(&gBtlWork->taskPools[0], &prize);
                    DropBossPrizes(&work->sub[0]);
                    EndBossDefeat();
                    result = 0;
                }

                break;
            }

            break;
        case BOS_MD_PHASE_EXIT:
            break;
    }

    if (work->statePhase == BOS_MD_PHASE_ENTER) {
        work->statePhase = BOS_MD_PHASE_UPDATE;
    }

    if (work->statePhase == BOS_MD_PHASE_EXIT) {
        work->state = work->nextState;
        work->statePhase = BOS_MD_PHASE_ENTER;
        work->flags &= ~MD_FLAG_STATE_REQUESTED;
    }

    return result;
}

void BosMdChooseAttack(MdWork* work) {
    s32 actorX;
    u16 roll;

    actorX = gBtlWork->actor->x;

    if (actorX > 0xA800) {
        roll = GetRandom() % 100;

        if (roll <= 59) {
            BosMdRequestState(work, BOS_MD_STATE_QUAKE);
        } else if (roll <= 89) {
            BosMdRequestState(work, BOS_MD_STATE_FIRE_BREATH);
        } else {
            BosMdRequestState(work, BOS_MD_STATE_BITE);
        }
    } else if (actorX > 0x7000) {
        roll = GetRandom() % 100;

        if (roll <= 59) {
            BosMdRequestState(work, BOS_MD_STATE_BITE);
        } else if (roll <= 89) {
            BosMdRequestState(work, BOS_MD_STATE_FIRE_BREATH);
        } else {
            BosMdRequestState(work, BOS_MD_STATE_QUAKE);
        }
    } else if (actorX > 0x3800) {
        roll = GetRandom() % 100;

        if (roll <= 59) {
            BosMdRequestState(work, BOS_MD_STATE_BITE);
        } else if (roll <= 89) {
            BosMdRequestState(work, BOS_MD_STATE_FIRE_BREATH);
        } else {
            BosMdRequestState(work, BOS_MD_STATE_QUAKE);
        }
    } else {
        roll = GetRandom() % 100;

        if (roll <= 59) {
            BosMdRequestState(work, BOS_MD_STATE_QUAKE);
        } else if (roll <= 89) {
            BosMdRequestState(work, BOS_MD_STATE_FIRE_BREATH);
        } else {
            BosMdRequestState(work, BOS_MD_STATE_BITE);
        }
    }
}

enum BosMdHurtState {
    BOS_MD_HURT_STATE_NONE,
    BOS_MD_HURT_STATE_HIT = 2
};

void BosMdHandleReaction(MdWork* work) {
    s16 i;

    for (i = 0; i < 1; i++) {
        BtlObj* sub = &work->sub[i];

        switch (UpdateBtlObjReaction(sub)) {
        case BTL_REACTION_CARD_ACTION:
            BosMdChooseAttack(work);
            break;
        case BTL_REACTION_HURT:
        case BTL_REACTION_STUNNED:
        case BTL_REACTION_GRAVITY:
            work->hurtTimer = 30;
            work->hurtState[i] = BOS_MD_HURT_STATE_HIT;
            break;
        case BTL_REACTION_DEFEATED:
        case BTL_REACTION_GRAVITY_DEFEATED:
            SetBtlObjUnhittable(sub, TRUE);
            BosMdRequestState(work, BOS_MD_STATE_DEFEATED);
            break;
        case BTL_REACTION_CARD_BROKEN:
            BosMdRequestState(work, BOS_MD_STATE_IDLE);
            ClearBtlObjActionFlags(sub);
            break;
        }
    }
}

void BosMdEndHurt(MdWork* work) {
    s16 i;

    for (i = 0; i < 1; i++) {
        BtlObj* sub = &work->sub[i];

        if (work->hurtState[i] == BOS_MD_HURT_STATE_HIT && work->hurtTimer == 0) {
            work->hurtState[i] = BOS_MD_HURT_STATE_NONE;
            ClearBtlObjActionFlags(sub);
        }
    }
}

void task_bos_md_0(MdWork* work, void* arg) {
    s16 i;

    TaskCreate(&gBtlWork->taskPools[1], &sTaskDescBosMdMap, (void*)&sMdMapData);
    gBtlWork->flags &= 0xFFFFFFFFFFEFFFFF;
    work->state = BOS_MD_STATE_IDLE;
    work->nextState = BOS_MD_STATE_IDLE;
    work->flags = 0;
    work->statePhase = BOS_MD_PHASE_ENTER;
    work->timer = 0;
    work->stepsLeft = 0;
    work->hurtTimer = 0;
    work->signals = 0;
    work->bgVisible = TRUE;

    for (i = 0; i < 1; i++) {
        work->hurtState[i] = BOS_MD_HURT_STATE_NONE;
    }

    for (i = 0; i < 2; i++) {
        work->gfx[i].tiles = NULL;
        work->gfx[i].src = NULL;
        work->gfx[i].sprite = NULL;
        work->gfx[i].x = 0;
        work->gfx[i].y = 0;
    }

    work->gfx[0].tiles = AllocSpriteFrameTiles(2432);
    work->gfx[1].tiles = LoadObjTiles(gBosMdForearmTiles, sizeof(gBosMdForearmTiles));
    gBtlWork->bossX = 0x11000;
    gBtlWork->bossY = 0x15000;
    gBtlWork->bossZ = 0;
    SetBattleActorPosition(0x7800, gBtlWork->bossY, 0);

    for (i = 0; i < 1; i++) {
        InitEnemyBtlObj(&work->sub[i], &sBosMdEmyKind, gBtlWork->bossX,
                      gBtlWork->bossY, gBtlWork->bossZ);
#ifdef VERSION_EU
        ColliderInit(&work->sub[i].collider, COLLIDER_TYPE_BOSS, 16, 24);
#else
        ColliderInit(&work->sub[i].collider, COLLIDER_TYPE_BOSS, 16, 16);
#endif

        if (i == 0) {
            work->sub[i].flags |= 0x400;
        } else {
            work->sub[i].flags |= BTLOBJ_FLAG_NEVER_USES_CARDS;
        }

        work->sub[i].flags |= BTLOBJ_FLAG_FACING_LEFT;
    }

    BosMdSetFrame(work, 0);
    MdAnimStart(work, 0);
    LoadPalette(gBosMdPalette, (void*)PLTT, sizeof(gBosMdPalette));
    SetBtlPaletteFadeExcluded(0, TRUE);
    work->bgPalette = gBosMdPalette;
    work->palette = LoadObjPalette(gBosMdPalette, sizeof(gBosMdPalette));
    SetBtlPaletteFadeExcluded(work->palette->index + 16, TRUE);
    work->palette2 = LoadObjPalette(gHitFlashPalette, sizeof(gHitFlashPalette));
    SetBtlPaletteFadeExcluded(work->palette2->index + 16, TRUE);
    TaskPoolInit(&work->tasks, 6);
    TaskPoolInit(&work->tasks2, 1);
    TaskPoolInit(&work->tasks3, 8);
    ScrollBgMapTo(1, (gBtlWork->viewX >> 8) + 72 - work->bgOffsetX,
                  (gBtlWork->viewY >> 8) + 48 - work->bgOffsetY);
}

s32 task_bos_md_1(MdWork* work) {
    void* args[2];
    s32 result;
    s16 i;

    result = 1;
    BosMdHandleReaction(work);

    switch (work->state) {
    case BOS_MD_STATE_IDLE:
        result = BosMdUpdateIdle(work);
        break;
    case BOS_MD_STATE_BITE:
        result = BosMdUpdateBite(work);
        break;
    case BOS_MD_STATE_QUAKE:
        result = BosMdUpdateQuake(work);
        break;
    case BOS_MD_STATE_FIRE_BREATH:
        result = BosMdUpdateFireBreath(work);
        break;
    case BOS_MD_STATE_DEFEATED:
        result = BosMdUpdateDefeat(work);
        break;
    }

    BosMdEndHurt(work);

    if (ConsumeGimmickFlag(0)) {
        args[0] = &work->tasks3;
        args[1] = &work->signals;
        TaskCreate(&work->tasks2, &sTaskDescBosMdDai, args);
    }

    MdAnimUpdate(work);

    if (work->hurtTimer > 0) {
        work->hurtTimer--;
    }

    for (i = 0; i < 1; i++) {
        work->sub[i].x = gBtlWork->bossX
            + sMdFrameDefs[work->anim.frames[work->anim.frame].gfxIndex]
                  .pos[i].x * 256;
        work->sub[i].y = gBtlWork->bossY
            + sMdFrameDefs[work->anim.frames[work->anim.frame].gfxIndex]
                  .pos[i].y * 256;
        work->sub[i].z = gBtlWork->bossZ
            + sMdFrameDefs[work->anim.frames[work->anim.frame].gfxIndex]
                  .pos[i].z * 256;
        ColliderSetPosition(&work->sub[i].collider, work->sub[i].x, work->sub[i].y,
                      work->sub[i].z);
    }

    TaskPoolUpdate(&work->tasks);
    TaskPoolUpdate(&work->tasks2);
    TaskPoolUpdate(&work->tasks3);
    return result;
}

void task_bos_md_2(MdWork* work) {
    s16 x;
    s16 y;
    void* pal;
    void* daiPool;
    void* hahenPool;
    void* firePool;
    s32 i;

    if (!work->bgVisible) {
        return;
    }

    if (StepHitFlash(&work->sub[0])) {
        if (work->bgPalette != gHitFlashPalette) {
            LoadPalette(gHitFlashPalette, (void*)PLTT, sizeof(gHitFlashPalette));
            work->bgPalette = gHitFlashPalette;
        }

        pal = work->palette2;
    } else {
        if (work->bgPalette != gBosMdPalette) {
            LoadPalette(gBosMdPalette, (void*)PLTT, sizeof(gBosMdPalette));
            work->bgPalette = gBosMdPalette;
        }

        pal = work->palette;
    }

    x = (gBtlWork->viewX >> 8) - (work->bgOffsetX - 72);
    y = (gBtlWork->viewY >> 8) - (work->bgOffsetY - 48);
    ScrollBgMapTo(1, x, y);
    firePool = &work->tasks;
    daiPool = &work->tasks2;
    hahenPool = &work->tasks3;

    for (i = 0; i < 2; i++) {
        s32 wx;
        s32 wy;
        u16 flags;

        wx = (work->gfx[i].x + 224) * 256;
        wy = (work->gfx[i].y + 256) * 256;
        WorldToScreen(&x, &y, wx, wy, work->gfx[i].z * 256);
        flags = GetBattleSpritePriorityFlags(wy);
        DrawSprite(x, y, work->gfx[i].sprite, work->gfx[i].tiles, pal, NULL, flags,
                   -4100 - (wy >> 6));
    }

    TaskPoolDraw(firePool);
    TaskPoolDraw(daiPool);
    TaskPoolDraw(hahenPool);
}

void task_bos_md_3(MdWork* work) {
    void* firePool;
    void* daiPool;
    void* hahenPool;
    s32 i;

    DisableBg(1);

    for (i = 0; i < 1; i++) {
        ColliderUnregister(&work->sub[i].collider);
        ReleaseEnemyBtlObj(&work->sub[i]);
    }

    firePool = &work->tasks;
    daiPool = &work->tasks2;
    hahenPool = &work->tasks3;

    for (i = 0; i < 2; i++) {
        if (work->gfx[i].tiles != NULL) {
            ReleaseObjTiles(work->gfx[i].tiles);
        }
    }

    ReleaseObjPalette(work->palette);
    ReleaseObjPalette(work->palette2);
    TaskPoolDestroy(firePool);
    TaskPoolDestroy(daiPool);
    TaskPoolDestroy(hahenPool);
}

void task_bos_md_map_0(MdMapWork* work, MdMapData* arg) {
    LoadBgTiles(0, arg->tiles, arg->tilesSize);
    LoadBgPalette(0, arg->palette, arg->paletteSize);
    SetBgMapBlocks(0, &arg->map, 2, 2);
    gBtlWork->scale = Q_8_8(1);
    gBtlWork->zoomScale = Q_8_8(1);
    gBtlWork->x = 0x10000;
    gBtlWork->y = 0x14000;
    gBtlWork->viewX = 0x10000;
    gBtlWork->viewY = 0x14000;
    gBtlWork->x2 = 0x10000;
    gBtlWork->y2 = 0x14000;
    gBtlWork->zoomX = 0x10000;
    gBtlWork->zoomY = 0x14000;
    gBtlWork->zoomSteps = 15;
    gBtlWork->rotation = 0;
    BtlMapResetShake();
    ScrollBgMapTo(0, gBtlWork->viewX >> 8, gBtlWork->viewY >> 8);
}

s32 task_bos_md_map_1(MdMapWork* work) {
    s32 dx;
    s32 dy;

    BtlMapUpdateShake();
    dx = (gBtlWork->x2 - gBtlWork->x) >> 3;
    dy = (gBtlWork->y2 - gBtlWork->y) >> 3;

    if (dx > 1280) {
        dx = 1280;
    } else if (dx < -1280) {
        dx = -1280;
    }

    gBtlWork->x += dx;
    gBtlWork->y += dy;
    gBtlWork->viewX = gBtlWork->x;
    gBtlWork->viewY = gBtlWork->y;

    if (gBtlWork->viewX - (DISPLAY_WIDTH / 2) * 256 < gBtlWork->xMin * 256) {
        gBtlWork->viewX = (gBtlWork->xMin + DISPLAY_WIDTH / 2) * 256;
    } else if (gBtlWork->viewX + (DISPLAY_WIDTH / 2) * 256 > (gBtlWork->xMax + BOS_MD_VIEW_X_MAX_EXTRA) * 256) {
        gBtlWork->viewX = (gBtlWork->xMax + BOS_MD_VIEW_X_MAX_EXTRA - DISPLAY_WIDTH / 2) * 256;
    }

    if (gBtlWork->viewY + (DISPLAY_HEIGHT / 2) * 256 < gBtlWork->yMin * 256) {
        gBtlWork->viewY = (gBtlWork->yMin - DISPLAY_HEIGHT / 2) * 256;
    } else if (gBtlWork->viewY + (DISPLAY_HEIGHT / 2) * 256 > gBtlWork->yMax * 256) {
        gBtlWork->viewY = (gBtlWork->yMax - DISPLAY_HEIGHT / 2) * 256;
    }

    gBtlWork->viewY += BtlMapGetShake();
    ScrollBgMapTo(0, (gBtlWork->viewX >> 8) + 8, (gBtlWork->viewY >> 8) + 40);
    return 1;
}

enum BosMdFireState {
    BOS_MD_FIRE_STATE_APPEAR,
    BOS_MD_FIRE_STATE_ACTIVE,
    BOS_MD_FIRE_STATE_INERT,
    BOS_MD_FIRE_STATE_HURT,
    BOS_MD_FIRE_STATE_VANISH
};

void BosMdFireHandleReaction(MdFireWork* work) {
    BtlObj* sub;

    sub = &work->sub;

    switch (UpdateBtlObjReaction(sub)) {
    case BTL_REACTION_HURT:
    case BTL_REACTION_STUNNED:
    case BTL_REACTION_GRAVITY:
        work->flashTimer = 30;
        work->state = BOS_MD_FIRE_STATE_HURT;
        break;
    case BTL_REACTION_DEFEATED:
    case BTL_REACTION_GRAVITY_DEFEATED:
        if (GetRandom() % 100 <= 49) {
            if ((gBtlWork->flags & 0x100000) == 0) {
                DropGimmickCard(0, sub->x, sub->y, sub->z);
            }
        }

        SetBtlObjUnhittable(sub, TRUE);
        work->scaleSteps = 30;
        work->state = BOS_MD_FIRE_STATE_VANISH;
        break;
    }
}

enum BosMdFireMotion {
    BOS_MD_FIRE_MOTION_WAIT,
    BOS_MD_FIRE_MOTION_SHOOT,
    BOS_MD_FIRE_MOTION_CIRCLE,
    BOS_MD_FIRE_MOTION_SWAY
};

u8 BosMdFireUpdateMotion(MdFireWork* work) {
    u8 result;
    BtlObj* sub;
    u8 angle;

    result = 1;
    sub = &work->sub;

    if ((*work->flags & MD_SIGNAL_CLEAR_FIRES) && work->state != BOS_MD_FIRE_STATE_VANISH) {
        SetBtlObjUnhittable(sub, TRUE);
        work->scaleSteps = 30;
        work->state = BOS_MD_FIRE_STATE_VANISH;
    }

    switch (work->state) {
        case BOS_MD_FIRE_STATE_APPEAR:
            work->scaleSteps--;

            if (work->scaleSteps <= 0) {
                SetBtlObjUnhittable(sub, FALSE);
                work->state = BOS_MD_FIRE_STATE_ACTIVE;
            }

            break;
        case BOS_MD_FIRE_STATE_ACTIVE:
            switch (work->motion) {
                case BOS_MD_FIRE_MOTION_WAIT:
                    if (work->timer > 0) {
                        work->timer--;

                        if (work->timer <= 0) {
                            switch (work->pattern) {
                            case BOS_MD_FIRE_PATTERN_POINTS_0:
                            case BOS_MD_FIRE_PATTERN_POINTS_1:
                            case BOS_MD_FIRE_PATTERN_POINTS_2:
                            case BOS_MD_FIRE_PATTERN_ROWS:
                                angle = BosGaGetAngle(work->x, work->y,
                                                  gBtlWork->actor->x,
                                                  gBtlWork->actor->y);
                                work->vx = gSineTable[angle] * 3;
                                work->vy = -gSineTable[angle + 0x40] * 3;
                                work->timer = 90;
                                work->motion = BOS_MD_FIRE_MOTION_SHOOT;
                                break;
                            case BOS_MD_FIRE_PATTERN_RING:
                                work->motion = BOS_MD_FIRE_MOTION_CIRCLE;
                                break;
                            case BOS_MD_FIRE_PATTERN_COLUMN:
                                work->motion = BOS_MD_FIRE_MOTION_SWAY;
                                break;
                            }
                        }
                    }

                    break;
                case BOS_MD_FIRE_MOTION_SHOOT:
                    work->x += work->vx;
                    work->y += work->vy;
                    work->timer--;

#ifdef VERSION_EU
                    if (work->timer <= 0 || work->y <= 0x12FFF) {
#else
                    if (work->timer <= 0 || work->y <= 0x117FF) {
#endif
                        SetBtlObjUnhittable(sub, TRUE);
                        work->scaleSteps = 30;
                        work->state = BOS_MD_FIRE_STATE_VANISH;
                    }

                    break;
                case BOS_MD_FIRE_MOTION_CIRCLE:
                    work->angle++;
                    work->x = gSineTable[work->angle] * 40 + work->centerX;
                    work->y = -gSineTable[work->angle + 0x40] * 40 + work->centerY;
                    break;
                case BOS_MD_FIRE_MOTION_SWAY:
                    work->angle++;
                    work->x = gSineTable[work->angle] * 32 + work->centerX;
                    break;
            }

            if (work->contactCooldown > 0) {
                work->contactCooldown--;
            } else if (ColliderIsTouchingType(&work->sub.collider, COLLIDER_TYPE_PLAYER)) {
                m4aSongNumStart(SONG_SND_714);
                gBtlWork->actor->flags |= BTLOBJ_FLAG_HAZARD_PENDING;
                work->contactCooldown = 60;
            }

            break;
        case BOS_MD_FIRE_STATE_INERT:
            break;
        case BOS_MD_FIRE_STATE_HURT:
            if (work->flashTimer == 0) {
                work->state = BOS_MD_FIRE_STATE_ACTIVE;
                ClearBtlObjActionFlags(sub);
            }

            break;
        case BOS_MD_FIRE_STATE_VANISH:
            work->scaleSteps--;

            if (work->scaleSteps <= 0) {
                result = 0;
            }

            break;
    }

    return result;
}

void BosMdFirePlace(MdFireWork* work) {
    const MdFirePoint* point;

    switch (work->pattern) {
    case BOS_MD_FIRE_PATTERN_POINTS_0:
    case BOS_MD_FIRE_PATTERN_POINTS_1:
    case BOS_MD_FIRE_PATTERN_POINTS_2:
        point = sMdFireDefs[work->pattern].points + work->index;
        work->x = point->x * 256;
        work->y = point->y * 256;
        work->timer = point->delay;
        work->motion = BOS_MD_FIRE_MOTION_WAIT;
        break;
    case BOS_MD_FIRE_PATTERN_RING:
        work->angle = work->index * 256 / 6;
        work->centerX = 0x8000;
        work->centerY = 0x14800;
        work->x = gSineTable[work->angle] * 40 + work->centerX;
        work->y = -gSineTable[work->angle + 0x40] * 40 + work->centerY;
        work->timer = 60;
        work->motion = BOS_MD_FIRE_MOTION_WAIT;
        break;
    case BOS_MD_FIRE_PATTERN_ROWS:
        work->x = GetRandom() % 96 * 256 + 0x9800;
        work->y = work->index * 4096 + 0x11800;
        work->timer = work->index * 60 + 240;
        work->motion = BOS_MD_FIRE_MOTION_WAIT;
        break;
    case BOS_MD_FIRE_PATTERN_COLUMN:
        work->angle = 0;
        work->centerX = 0x9800;
        work->x = gSineTable[work->angle] * 32 + work->centerX;
        work->y = work->index * 4096 + 0x11800;
        work->timer = work->index * 256 / 6 + 60;
        work->motion = BOS_MD_FIRE_MOTION_WAIT;
        break;
    }
}

void task_bos_md_fire_0(MdFireWork* work, MdFireArg* arg) {
    MdFireArg fireArg;
    s16 i;
    s16 n;

    work->flashTimer = 0;
    work->contactCooldown = 0;
    work->scale = Q_8_8(0.1);
    work->scaleSteps = 30;
    work->state = BOS_MD_FIRE_STATE_APPEAR;
    work->z = 0;
    work->pattern = arg->pattern;
    work->index = arg->index;
    work->flags = arg->flags;
    BosMdFirePlace(work);
    InitEnemyBtlObj(&work->sub, &sBosMdFireEmyKind, work->x, work->y, work->z);
    ColliderInit(&work->sub.collider, COLLIDER_TYPE_ENEMY, 16, 16);
    ColliderSetPosition(&work->sub.collider, work->sub.x, work->sub.y,
                  work->sub.z);
    work->sub.flags |= BTLOBJ_FLAG_NEVER_USES_CARDS;
    work->sub.hp = 20;
    work->sub.maxHp = 20;
    SetBtlObjUnhittable(&work->sub, TRUE);
    work->palette = LoadObjPalette(gBosMdFirePalette, sizeof(gBosMdFirePalette));
    work->palette2 = LoadObjPalette(gHitFlashPalette, sizeof(gHitFlashPalette));
    work->tiles = LoadObjTiles(gBosMdFireTiles, sizeof(gBosMdFireTiles));
    AnimInit(&work->anim, gBosMdFireAnims, gBosMdFireFrames);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    fireArg.pool = NULL;
    fireArg.pattern = arg->pattern;
    fireArg.flags = arg->flags;

    if (arg->pool != NULL) {
        if (work->pattern <= BOS_MD_FIRE_PATTERN_POINTS_2) {
            n = sMdFireDefs[work->pattern].count;
        } else {
            n = 6;
        }

        for (i = 1; i < n; i++) {
            fireArg.index = i;
            TaskCreate(arg->pool, &sTaskDescBosMdFire, &fireArg);
        }
    }
}

u8 task_bos_md_fire_1(MdFireWork* work) {
    u8 result;

    BosMdFireHandleReaction(work);
    result = BosMdFireUpdateMotion(work);

    if (work->flashTimer > 0) {
        work->flashTimer--;
    }

    work->sub.x = work->x;
    work->sub.y = work->y;
    work->sub.z = work->z;
    ColliderSetPosition(&work->sub.collider, work->sub.x, work->sub.y,
                  work->sub.z);
    return result;
}

void task_bos_md_fire_2(MdFireWork* work) {
    s16 x;
    s16 y;
    void* pal;
    ObjAffine* affine;
    u16 flags;

    if (work->flashTimer > 0 && (gFrameCounter & 1)) {
        pal = work->palette2;
    } else {
        pal = work->palette;
    }

    WorldToScreen(&x, &y, work->x, work->y, work->z);
    flags = GetBattleSpritePriorityFlags(work->y);

    if (work->state == BOS_MD_FIRE_STATE_APPEAR) {
        ApproachValue(&work->scale, Q_8_8(1), work->scaleSteps);
        affine = AllocObjAffine(0, work->scale, work->scale, FALSE);
    } else if (work->state == BOS_MD_FIRE_STATE_VANISH) {
        ApproachValue(&work->scale, Q_8_8(0.1), work->scaleSteps);
        affine = AllocObjAffine(0, work->scale, work->scale, FALSE);
    } else {
        affine = NULL;
    }

    DrawSprite(x, y, AnimUpdate(&work->anim), work->tiles, pal,
                  affine, flags, -4100 - (work->y >> 8) * 4);
}

void task_bos_md_fire_3(MdFireWork* work) {
    ColliderUnregister(&work->sub.collider);
    ReleaseEnemyBtlObj(&work->sub);
    ReleaseObjPalette(work->palette);
    ReleaseObjPalette(work->palette2);
    ReleaseObjTiles(work->tiles);
}

enum BosMdDaiState {
    BOS_MD_DAI_STATE_DROP_BOTTOM,
    BOS_MD_DAI_STATE_DROP_MIDDLE,
    BOS_MD_DAI_STATE_DROP_TOP,
    BOS_MD_DAI_STATE_STACKED
};

void task_bos_md_dai_0(MdDaiWork* work, void** args) {
    Collider* collider;

    gBtlWork->flags |= 0x100000;
    work->flags = args[1];
    work->pool = args[0];
    work->level = 0;
    work->state = BOS_MD_DAI_STATE_DROP_BOTTOM;
    work->x = 0x8000;
    work->y = 0x14F00;
    work->z = 0;
    work->dropSteps = 20;
    work->dropZ = -40960;
    collider = &work->collider;
    ColliderInit(collider, COLLIDER_TYPE_PLATFORM, 24, 24);
    ColliderSetPosition(collider, work->x, work->y, work->z);
    ColliderSetDisabled(collider, TRUE);
    work->palette = LoadObjPalette(gBosMdDaiPalette, sizeof(gBosMdDaiPalette));
    work->tiles = LoadObjTiles(gBosMdDaiTiles, sizeof(gBosMdDaiTiles));
}

s32 task_bos_md_dai_1(MdDaiWork* work) {
    s32 result;
    s32 args[3];
    s16 i;
    s16 n;

    result = 1;

    switch (work->state) {
    case BOS_MD_DAI_STATE_DROP_BOTTOM:
        ApproachValue(&work->dropZ, 0, work->dropSteps);
        work->dropSteps--;

        if (work->dropSteps <= 0) {
            ColliderSetDisabled(&work->collider, FALSE);
            ColliderSetHeight(&work->collider, 8);
            work->dropZ = work->z - 0xA000;
            m4aSongNumStart(SONG_BTL_DRGN_GIMIC);
            work->level = 1;
            work->dropSteps = 20;
            work->state = BOS_MD_DAI_STATE_DROP_MIDDLE;
        }

        break;
    case BOS_MD_DAI_STATE_DROP_MIDDLE:
        ApproachValue(&work->dropZ, -3584, work->dropSteps);
        work->dropSteps--;

        if (work->dropSteps <= 0) {
            ColliderSetDisabled(&work->collider, FALSE);
            ColliderSetHeight(&work->collider, 16);
            work->dropZ = work->z - 0xA000;
            m4aSongNumStart(SONG_BTL_DRGN_GIMIC);
            work->level = 2;
            work->dropSteps = 20;
            work->state = BOS_MD_DAI_STATE_DROP_TOP;
        }

        break;
    case BOS_MD_DAI_STATE_DROP_TOP:
        ApproachValue(&work->dropZ, -7168, work->dropSteps);
        work->dropSteps--;

        if (work->dropSteps <= 0) {
            ColliderSetDisabled(&work->collider, FALSE);
            ColliderSetHeight(&work->collider, 24);
            m4aSongNumStart(SONG_BTL_DRGN_GIMIC);
            work->level = 3;
            *work->flags &= ~MD_SIGNAL_QUAKE_SLAM;
            work->state = BOS_MD_DAI_STATE_STACKED;
        }

        break;
    case BOS_MD_DAI_STATE_STACKED:
        if (*work->flags & MD_SIGNAL_QUAKE_SLAM) {
            *work->flags &= ~MD_SIGNAL_QUAKE_SLAM;
            args[0] = work->x;
            args[1] = work->y;
            args[2] = -((work->level - 1) * 7 << 9);
            n = GetRandom() % 3 + 3;

            for (i = 0; i < n; i++) {
                TaskCreate(work->pool, &sTaskDescBosMdHahen, args);
            }

            work->level--;

            if (work->level <= 0) {
                ColliderSetDisabled(&work->collider, TRUE);
                result = 0;
            } else {
                ColliderSetHeight(&work->collider, work->level * 8);
            }
        }

        break;
    }

    return result;
}

void task_bos_md_dai_2(MdDaiWork* work) {
    s16 x;
    s16 y;
    u16 flags;

    WorldToScreen(&x, &y, work->x, work->y, work->z + work->dropZ);
    flags = GetBattleSpritePriorityFlags(work->y);

    if (work->state <= BOS_MD_DAI_STATE_DROP_TOP) {
        DrawSprite(x, y + 24, gBosMdDaiFrame0, work->tiles, work->palette, NULL,
                      flags, -4100 - (work->y >> 8) * 4);
        DrawSprite(x, y, gBosMdDaiFrame1, work->tiles, work->palette, NULL, flags,
                      -4100 - (work->y >> 8) * 4);
        DrawSprite(x + 8, y - 16, gBosMdDaiFrame0, work->tiles, work->palette, NULL,
                      flags, -4100 - (work->y >> 8) * 4);
    }

    WorldToScreen(&x, &y, work->x, work->y, work->z);
    flags = GetBattleSpritePriorityFlags(work->y);

    if (work->level > 0) {
        DrawSprite(x, y, gBosMdDaiFrames[work->level + 1], work->tiles,
                      work->palette, NULL, flags,
                      -4100 - (work->y >> 8) * 4);
    }
}

void task_bos_md_dai_3(MdDaiWork* work) {
    ColliderUnregister(&work->collider);
    ReleaseObjPalette(work->palette);
    ReleaseObjTiles(work->tiles);
    gBtlWork->flags &= 0xFFFFFFFFFFEFFFFF;
}

void task_bos_md_hahen_0(MdHahenWork* work, s32* src) {
    u8 angle;
    s32 speed;

    work->x = src[0];
    work->y = src[1];
    work->z = src[2];
    angle = GetRandom();
    speed = (GetRandom() & 0x1FF) + 0x100;
    work->vx = -gSineTable[angle + 0x40] * speed >> 8;
    work->vy = gSineTable[angle] * speed >> 8;
    work->vz = -((GetRandom() & 0x1FF) + 0x100);
    work->timer = 3;
    work->palette = LoadObjPalette(gBosMdDaiPalette, sizeof(gBosMdDaiPalette));
    work->tiles = LoadObjTiles(gBosMdDaiTiles, sizeof(gBosMdDaiTiles));
    work->gfx = gBosMdDaiFrames[GetRandom() % 2];
}

s32 task_bos_md_hahen_1(MdHahenWork* work) {
    s32 result;

    result = 1;
    work->x += work->vx;
    work->y += work->vy;

#ifdef VERSION_EU
    if (work->y <= 0x12FFF) {
#else
    if (work->y <= 0x117FF) {
#endif
        work->vy = -work->vy;
    }

    work->z += work->vz;
    work->vz += 102;

    if (work->z > 0) {
        work->z = 0;
        work->vz = -(work->vz * 8 / 10);
        work->timer--;

        if ((s16)work->timer <= 0) {
            result = 0;
        }
    }

    return result;
}

void task_bos_md_hahen_2(MdHahenWork* work) {
    s16 x;
    s16 y;
    u16 flags;
    s32 oddFrame;

    oddFrame = gFrameCounter & 1;

    if (oddFrame != 0) {
        return;
    }

    WorldToScreen(&x, &y, work->x, work->y, work->z);
    flags = GetBattleSpritePriorityFlags(work->y);
    DrawSprite(x, y, work->gfx, work->tiles, work->palette,
                  NULL, flags, -4100 - (work->y >> 8) * 4);
}

void task_bos_md_hahen_3(MdHahenWork* work) {
    ReleaseObjPalette(work->palette);
    ReleaseObjTiles(work->tiles);
}
