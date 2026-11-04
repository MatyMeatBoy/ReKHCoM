/**
 * mode_eventselect.c
 * Debug Event Select and Event Effects
 */

#include "macros.h"
#include "registration_data.h"
#include "msg_api.h"
#include "card_api.h"
#include "m4a_song.h"
#include "fade.h"
#include "engine_math.h"
#include "mode_event.h"
#include "mode_eventselect.h"
#include "gba/keys.h"
#include "sprites_evt.h"
#include "sprites_hum.h"
#include "sprites_level_up.h"
#include "sprites_smn.h"
#include "obj_resource_types.h"
#include "malloc.h"
#include "anim.h"
#include "display.h"
#include "event_chara_types.h"
#include "event_index_data.h"
#include "evt_object_types.h"
#include "evt_types.h"
#include "key.h"
#include "m4a.h"
#include "mode.h"
#include "msg_types.h"
#include "obj.h"
#include "obj_api.h"
#include "taskpool.h"
#include "text.h"
#include "types.h"
#include <stddef.h>
#include "debug_text.h"
#include "sprite_palettes.h"
#include "debug_font.h"
#include "event_ids.h"
#include "songs.h"
#include "gba/defines.h"

static const s16 sSoraEventIds[147] = {
    EVENT_000_1F_ENTRANCE_PART1,
    EVENT_001_1F_ENTRANCE_PART2,
    EVENT_002_1F_TRAVERSE_TOWN_E0_1,
    EVENT_003_1F_TRAVERSE_TOWN_E0_2,
    EVENT_004_1F_TRAVERSE_TOWN_E1_1,
    EVENT_005_1F_TRAVERSE_TOWN_E1_2,
    EVENT_006_1F_TRAVERSE_TOWN_E2,
    EVENT_007_1F_TRAVERSE_TOWN_E3,
    EVENT_008_1F_BATTLE_GUARDARMOR,
    EVENT_009_1F_TRAVERSE_TOWN_E4,
    EVENT_010_1F_GOAL_1,
    EVENT_011_1F_GOAL_2,
    EVENT_012_2F_ENTRANCE,
    EVENT_013_2F_GOAL,
    EVENT_014_2F_DEMO,
    EVENT_015_3F_ENTRANCE,
    EVENT_016_3F_GOAL,
    EVENT_017_3F_DEMO,
    EVENT_018_4F_ENTRANCE,
    EVENT_019_4F_GOAL,
    EVENT_020_4F_DEMO,
    EVENT_021_5F_ENTRANCE,
    EVENT_022_5F_GOAL,
    EVENT_023_5F_DEMO,
    EVENT_024_6F_ENTRANCE,
    EVENT_025_6F_GOAL_1,
    EVENT_026_6F_GOAL_2,
    EVENT_027_6F_GOAL_3,
    EVENT_028_6F_DEMO,
    EVENT_029_7F_ENTRANCE,
    EVENT_030_7F_GOAL_1,
    EVENT_031_7F_GOAL_2,
    EVENT_032_8F_ENTRANCE,
    EVENT_033_8F_GOAL_1,
    EVENT_034_8F_GOAL_2,
    EVENT_035_8F_DEMO,
    EVENT_036_9F_ENTRANCE,
    EVENT_037_9F_GOAL,
    EVENT_038_9F_DEMO,
    EVENT_039_10F_ENTRANCE,
    EVENT_040_10F_GOAL_1,
    EVENT_041_10F_GOAL_2,
    EVENT_042_10F_DEMO,
    EVENT_043_11F_ENTRANCE,
    EVENT_044_11F_TWILIGHT_TOWN_E0,
    EVENT_045_11F_TWILIGHT_TOWN_E1,
    EVENT_046_11F_TWILIGHT_TOWN_E1_2,
    EVENT_047_11F_DEMO_1,
    EVENT_048_11F_GOAL_1,
    EVENT_049_11F_GOAL_2,
    EVENT_050_11F_GOAL_3,
    EVENT_051_11F_DEMO_2,
    EVENT_052_12F_ENTRANCE,
    EVENT_053_12F_DESTINY_ISLAND_E0,
    EVENT_054_12F_DESTINY_ISLAND_E1,
    EVENT_055_12F_DESTINY_ISLAND_E2,
    EVENT_056_12F_DESTINY_ISLAND_E2_2,
    EVENT_057_12F_DESTINY_ISLAND_E3,
    EVENT_058_12F_GOAL,
    EVENT_059_12F_GOAL_2,
    EVENT_060_12F_GOAL_3,
    EVENT_061_13F_ENTRANCE,
    EVENT_062_12F_GOAL,
    EVENT_063_13F_CASTLE_OBLIVION_E1,
    EVENT_064_13F_CASTLE_OBLIVION_E1_2,
    EVENT_065_13F_CASTLE_OBLIVION_E1_3,
    EVENT_066_13F_CASTLE_OBLIVION_LAST1,
    EVENT_067_13F_CASTLE_OBLIVION_LAST2,
    EVENT_068_13F_CASTLE_OBLIVION_LAST3,
    EVENT_069_13F_CASTLE_OBLIVION_LAST4,
    EVENT_070_13F_CASTLE_OBLIVION_LAST5,
    EVENT_071_13F_CASTLE_OBLIVION_LAST6,
    EVENT_072_13F_CASLTE_OBLIVION_LAST7,
    EVENT_073_13F_CAPSULE_ROOM_ENDING,
    EVENT_074_MONSTORO_E0,
    EVENT_075_MONSTORO_E1,
    EVENT_076_MONSTORO_E2,
    EVENT_077_MONSTORO_E2_2,
    EVENT_078_MONSTORO_E2_3,
    EVENT_080_MONSTORO_E2_5,
    EVENT_081_MONSTORO_E3,
    EVENT_082_MONSTORO_E3_SUCCESS,
    EVENT_083_MONSTORO_E3_FAILURE_1,
    EVENT_084_MONSTORO_E3_FAILURE_2,
    EVENT_085_MONSTORO_E3_RETRY,
    EVENT_086_MONSTORO_E3_END,
    EVENT_087_HALLOWEEN_TOWN_E0,
    EVENT_088_HALLOWEEN_TOWN_E0_2,
    EVENT_089_HALLOWEEN_TOWN_E1,
    EVENT_090_HALLOWEEN_TOWN_E2,
    EVENT_091_HALLOWEEN_TOWN_E3,
    EVENT_092_HALLOWEEN_TOWN_E3,
    EVENT_093_HALLOWEEN_TOWN_END,
    EVENT_094_WONDERLAND_E0,
    EVENT_095_WONDERLAND_E1,
    EVENT_096_WONDERLAND_E1_2,
    EVENT_097_WONDERLAND_E2,
    EVENT_098_WONDERLAND_E3,
    EVENT_099_WONDERLAND_BOSS,
    EVENT_100_WONDERLAND_END,
    EVENT_101_ATLANTICA_E0,
    EVENT_102_ATLANTICA_E1,
    EVENT_103_ATLANTICA_E2,
    EVENT_104_ATLANTICA_E3,
    EVENT_105_ATLANTICA_BOSS,
    EVENT_106_ATLANTICA_END,
    EVENT_107_AGRABAH_E0,
    EVENT_108_AGRABAH_E0_2,
    EVENT_109_AGRABAH_E1,
    EVENT_110_AGRABAH_E2,
    EVENT_111_AGRABAH_E2_2,
    EVENT_112_AGRABAH_E3,
    EVENT_113_AGRABAH_BOSS,
    EVENT_114_AGRABAH_END,
    EVENT_115_NEVERLAND_E0,
    EVENT_116_NEVERLAND_E1,
    EVENT_117_NEVERLAND_E2,
    EVENT_118_NEVERLAND_E3,
    EVENT_119_NEVERLAND_END,
    EVENT_120_COLISEUM_E0,
    EVENT_121_COLISEUM_E1,
    EVENT_122_COLISEUM_E2,
    EVENT_123_COLISEUM_E2_2,
    EVENT_124_COLISEUM_E3,
    EVENT_125_COLISEUM_BOSS,
    EVENT_126_COLISEUM_END,
    EVENT_129_HOLLOWBASTION_E0,
    EVENT_130_HOLLOWBASTION_E1,
    EVENT_131_HOLLOWBASTION_E2,
    EVENT_132_HOLLOWBASTION_E3,
    EVENT_133_HOLLOWBASTION_END,
    EVENT_134_100ACREWOOD_START,
    EVENT_135_100ACREWOOD_START_RETRY,
    EVENT_136_100ACREWOOD_LV1,
    EVENT_137_100ACREWOOD_LV2,
    EVENT_138_100ACREWOOD_LV2_RETRY,
    EVENT_139_100ACREWOOD_LV3,
    EVENT_140_100ACREWOOD_LV4,
    EVENT_141_100ACREWOOD_LV5,
    EVENT_142_100ACREWOOD_LV6,
    EVENT_143_100ACREWOOD_END_1ST_COMP,
    EVENT_144_100ACREWOOD_END_1ST_NO,
    EVENT_145_100ACREWOOD_END_COMP,
    EVENT_146_100ACREWOOD_END_NO,
    EVENT_147_100ACREWOOD_END_COMPCOMP,
    EVENT_148_100ACREWOOD_END_SORAONLY,
    EVENT_END,
};

static const s16 sRikuEventIds[49] = {
    EVENT_149_RIKU_B12F_OPNING,
    EVENT_150_RIKU_B12F_ENTRANCE,
    EVENT_151_RIKU_B12F_E0,
    EVENT_152_RIKU_B12F_E1,
    EVENT_153_RIKU_B12F_E2,
    EVENT_154_RIKU_B12F_E3,
    EVENT_155_RIKU_B12F_GOAL,
    EVENT_156_RIKU_B12F_GOAL_2,
    EVENT_157_RIKU_B12F_DEMO,
    EVENT_158_RIKU_B11F_ENTRANCE,
    EVENT_159_RIKU_B11F_DEMO,
    EVENT_160_RIKU_B10F_GOAL,
    EVENT_161_RIKU_B10F_GOAL_2,
    EVENT_162_RIKU_B9F_DEMO,
    EVENT_163_RIKU_B8F_GOAL,
    EVENT_164_RIKU_B8F_GOAL_2,
    EVENT_165_RIKU_B8F_DEMO,
    EVENT_166_RIKU_B7F_ENTRANCE,
    EVENT_167_RIKU_B7F_DEMO,
    EVENT_168_RIKU_B6F_DEMO,
    EVENT_169_RIKU_B5F_DEMO,
    EVENT_170_RIKU_B4F_ENTRANCE,
    EVENT_171_RIKU_B4F_GOAL,
    EVENT_172_RIKU_B4F_GOAL_2,
    EVENT_173_RIKU_B4F_DARK,
    EVENT_174_RIKU_B4F_GOAL_3,
    EVENT_175_RIKU_B4F_DEMO,
    EVENT_176_RIKU_B3F_ENTRANCE,
    EVENT_177_RIKU_B3F_E0,
    EVENT_178_RIKU_B3F_E1,
    EVENT_179_RIKU_B3F_E2,
    EVENT_180_RIKU_B3F_BOSS,
    EVENT_181_RIKU_B3F_E2_2,
    EVENT_182_RIKU_B3F_LIGHT,
    EVENT_183_RIKU_B3F_E2_3,
    EVENT_184_RIKU_B3F_DEMO,
    EVENT_185_RIKU_B2F_ENTRANCE,
    EVENT_186_RIKU_B2F_E0,
    EVENT_187_RIKU_B2F_E1,
    EVENT_188_RIKU_B2F_E1_2,
    EVENT_189_RIKU_B2F_E2,
    EVENT_190_RIKU_B2F_GOAL,
    EVENT_191_RIKU_B1F_ENTRANCE,
    EVENT_192_RIKU_B1F_E0,
    EVENT_193_RIKU_B1F_LAST1,
    EVENT_194_RIKU_B1F_LAST2,
    EVENT_195_RIKU_ENDING,
    EVENT_196_DAMI_JUUJIRO,
    EVENT_END,
};

static const char sEventSelectCursorText[] = "\x81\xa8";

static const char sEventSelectBlankText[] = "\x81\x40";

static const char sEventSelectSoraLabel[] = "\x82\x64\x82\x75\x82\x64\x82\x6d\x82\x73\x81\x40\x82\x72\x82\x6e\x82\x71\x82\x60";

static const char sEventSelectRikuLabel[] = "\x82\x64\x82\x75\x82\x64\x82\x6d\x82\x73\x81\x40\x82\x71\x82\x68\x82\x6a\x82\x74";

static const char sEventSelectNoLabel[] = "\x82\x6d\x82\x8f\x81\x40\x81\x81";

static u16 sUnk_02034A92;
static s16 sEventSelectIndex;
static u8 sEventSelectList;
EventSoundMix* gEventSoundMix EWRAM_COMMON(4);

enum EventSelectList {
    EVENT_SELECT_LIST_SORA,
    EVENT_SELECT_LIST_RIKU
};

#ifdef PLATFORM_PC
/* Test harness (KHCOM_EVENT=list:index): preselect an entry of the menu. */
void PortEventSelectPreset(int list, int index) {
    sEventSelectList = list ? EVENT_SELECT_LIST_RIKU : EVENT_SELECT_LIST_SORA;
    sEventSelectIndex = index;
}
#endif

s16 GetEventListLength(u8 list) {
    s16 n = 0;

    switch (list) {
    case EVENT_SELECT_LIST_SORA:
        while (sSoraEventIds[n] != EVENT_END) {
            n++;
        }

        break;
    case EVENT_SELECT_LIST_RIKU:
        while (sRikuEventIds[n] != EVENT_END) {
            n++;
        }

        break;
    }

    return n;
}

void mode_eventselect_0() {
    SetBgMode0();
    func_08085FB0();
    InitSoraDecks();
    InitMapCardInventory();
    SetupBg(0, 0, 30, 0);
    EnableBg(0);
    DebugTextInit(0, 0x8000, 0x800);
    DebugTextLoadPalette(0, gDebugFontPalette, sizeof(gDebugFontPalette), 0);
}

void mode_eventselect_1() {
    if (GetKeysRepeat() & DPAD_UP) {
        if (sEventSelectList != EVENT_SELECT_LIST_SORA) {
            sEventSelectList--;
        } else {
            sEventSelectList = EVENT_SELECT_LIST_RIKU;
        }
    }

    if (GetKeysRepeat() & DPAD_DOWN) {
        if (sEventSelectList == EVENT_SELECT_LIST_SORA) {
            sEventSelectList++;
        } else {
            sEventSelectList = EVENT_SELECT_LIST_SORA;
        }
    }

    switch (sEventSelectList) {
    case EVENT_SELECT_LIST_SORA:
        DebugTextPrint(0, 0, 2, sEventSelectCursorText);
        DebugTextPrint(0, 10, 2, sEventSelectBlankText);
        DebugTextPrint(0, 20, 2, sEventSelectBlankText);
        break;
    case EVENT_SELECT_LIST_RIKU:
        DebugTextPrint(0, 0, 2, sEventSelectBlankText);
        DebugTextPrint(0, 10, 2, sEventSelectCursorText);
        DebugTextPrint(0, 20, 2, sEventSelectBlankText);
        break;
    }

    if (GetKeysRepeat() & DPAD_RIGHT) {
        sEventSelectIndex++;
    }

    if (GetKeysRepeat() & DPAD_LEFT) {
        sEventSelectIndex--;
    }

    if (GetEventListLength(sEventSelectList) - 1 < sEventSelectIndex) {
        sEventSelectIndex = 0;
    }

    if (sEventSelectIndex < 0) {
        sEventSelectIndex = GetEventListLength(sEventSelectList) - 1;
    }

    DebugTextPrint(10, 0, 2, sEventSelectSoraLabel);
    DebugTextPrint(10, 10, 2, sEventSelectRikuLabel);
    DebugTextPrint(20, 40, 2, sEventSelectNoLabel);
    DebugTextPrintNumber(100, 40, 2, sEventSelectIndex + 1);

    switch (sEventSelectList) {
    case EVENT_SELECT_LIST_SORA:
        DebugTextPrint(20, 80, 2, gEventNames[sSoraEventIds[sEventSelectIndex]]);
        break;
    case EVENT_SELECT_LIST_RIKU:
        DebugTextPrint(20, 80, 2, gEventNames[sRikuEventIds[sEventSelectIndex]]);
        break;
    }

    if (GetKeysPressed() & A_BUTTON) {
        switch (sEventSelectList) {
        case EVENT_SELECT_LIST_SORA:
#ifdef VERSION_EU
            ModeRequest(&gModeEventDebug, sSoraEventIds[sEventSelectIndex] | 0x8000);
#else
            RequestEventMode(sSoraEventIds[sEventSelectIndex]);
#endif
            break;
        case EVENT_SELECT_LIST_RIKU:
#ifdef VERSION_EU
            ModeRequest(&gModeEventDebug, sRikuEventIds[sEventSelectIndex] | 0x8000);
#else
            RequestEventMode(sRikuEventIds[sEventSelectIndex]);
#endif
            break;
        }
    }

    if (GetKeysPressed() & B_BUTTON) {
        ModeRequest(&gModeDebug, 0);
    }

    DebugTextDraw(0);
    DebugTextClear();
}

void mode_eventselect_2() {
    DebugTextDestroy();
}

void Hanabira_0(EffectWork* work, EventCharaWork* chara) {
    s32 i;

    TaskPoolInit(&work->tasks, 16);

    for (i = 15; i >= 0; i--) {
        TaskCreate(&work->tasks, &gTaskDescHanabiraC, chara);
    }
}

s32 Hanabira_1(EffectWork* work) {
    TaskPoolUpdate(&work->tasks);

    return 1;
}

void Hanabira_2(EffectWork* work) {
    TaskPoolDraw(&work->tasks);
}

void Hanabira_3(EffectWork* work) {
    TaskPoolDestroy(&work->tasks);
}

enum HanabiraCState {
    HANABIRA_C_STATE_RISE,
    HANABIRA_C_STATE_FLUTTER
};

void Hanabira_c_0(EffectWork* work, EventCharaWork* chara) {
    EvtObj* obj;

    work->actor = chara;
    obj = &chara->obj;
    work->palette = LoadObjPalette(gMaruxhaBtEffPalette, sizeof(gMaruxhaBtEffPalette));
    work->tiles = LoadObjTiles(gMaruxhaBtEff2Tiles, sizeof(gMaruxhaBtEff2Tiles));
    work->x = obj->x;
    work->y = obj->y;
    work->z = obj->z - 0x3000;
    work->vx = GetRandom() % 717 - 358;
    work->vz = -(GetRandom() % 539 + 102);
    AnimInit(&work->anim, gMaruxhaBtEff2Anims, gMaruxhaBtEff2Frames);
    AnimStart(&work->anim, GetRandom() & 1, ANIM_FLAG_LOOP);
    work->state = HANABIRA_C_STATE_RISE;
}

s32 Hanabira_c_1(EffectWork* work) {
    s32 decayed;
    s32 randomValue;

    switch (work->state) {
    case HANABIRA_C_STATE_RISE:
        work->x += work->vx;
        work->z += work->vz;
        work->vz += 17;

        if (work->vz > 256) {
            work->state = HANABIRA_C_STATE_FLUTTER;
        }

        break;
    case HANABIRA_C_STATE_FLUTTER:
        work->x += work->vx;
        work->z += work->vz;
        work->vz = (decayed = work->vz - 12) - (randomValue = GetRandom()) % 9;

        if (work->vz < 0) {
            work->vz = GetRandom() % 181 + 204;

            if (work->vx > 0) {
                work->vx = -(GetRandom() % 257 + 128);
            } else {
                work->vx = GetRandom() % 257 + 128;
            }
        }

        if (work->z >= 0) {
            return 0;
        }

        break;
    }

    work->gfx = AnimUpdate(&work->anim);

    return 1;
}

void Hanabira_c_2(EffectWork* work) {
    s32 x;
    s32 y;
    s32 baseY;

    x = (work->x >> 8) - (gEventState->x >> 8);
    baseY = work->y >> 8;
    y = baseY + (work->z >> 8) - (gEventState->y >> 8);
    DrawSprite(x, y, work->gfx, work->tiles, work->palette, NULL, SPRITE_PRIORITY(2), -0x1004 - baseY * 4);
}

void Hanabira_c_3(EffectWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

void smoke_0(EffectWork* work, EventCharaWork* chara) {
    EvtObj* obj;

    work->actor = chara;
    obj = &chara->obj;
    work->x = obj->x;
    work->y = obj->y - 0x800;
    work->tiles = AllocObjTiles(128, NULL);
    work->palette = LoadObjPalette(gCommonObjPalette, sizeof(gCommonObjPalette));
    SetObjTileSource(work->tiles, gEventSmokeTiles);
    AnimInit(&work->anim, gEventSmokeAnims, gEventSmokeFrames);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    work->followFlip = TRUE;
    work->age = 0;
}

void Exclamation_0(EffectWork* work, EventCharaWork* chara) {
    EvtObj* obj;

    work->actor = chara;
    obj = &chara->obj;
    work->x = obj->x;
    work->y = obj->y;
    work->tiles = AllocObjTiles(128, NULL);
    work->palette = LoadObjPalette(gCommonObjPalette, sizeof(gCommonObjPalette));

    if (!FadeIsActive()) {
        FadeSetPaletteExcluded(BIOS_PTR((ObjPaletteHeader*)work->palette)->index + 16, TRUE);
    }

    SetObjTileSource(work->tiles, gFEventTiles);
    AnimInit(&work->anim, gFEventAnims, gFEventFrames);
    AnimStart(&work->anim, 0, 0);
    work->gfx = AnimGetGfx(&work->anim);
    work->followFlip = TRUE;
    work->age = 0;
}

void balloon_0(EffectWork* work, EventCharaWork* chara) {
    EvtObj* obj;

    work->actor = chara;
    obj = &chara->obj;
    work->x = obj->x;
    work->y = obj->y;
    work->tiles = AllocObjTiles(128, NULL);
    work->palette = LoadObjPalette(gCommonObjPalette, sizeof(gCommonObjPalette));
    SetObjTileSource(work->tiles, gFEventTiles);
    AnimInit(&work->anim, gFEventAnims, gFEventFrames);
    AnimStart(&work->anim, 1, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    work->followFlip = FALSE;
    work->age = 0;
}

s32 EffectUpdateObj(EffectWork* work) {
    work->gfx = AnimUpdate(&work->anim);

    if (!work->actor->callbackActive) {
        return 0;
    }

    return 1;
}

s32 Exclamation_1(EffectWork* work) {
    work->age++;
    work->gfx = AnimUpdate(&work->anim);

    if (!work->actor->callbackActive || work->age == 50) {
        return 0;
    }

    return 1;
}

void EffectDrawObj(EffectWork* work) {
    u16 flags;
    s32 y;

    flags = work->actor->obj.drawFlags;

    if (!work->followFlip) {
        flags &= ~SPRITE_FLAG_HFLIP;
    }

    DrawSprite((work->x >> 8) - (gEventState->x >> 8),
               (y = (work->y >> 8) + gEventCharaParams[work->actor->arg.chara].spriteYOffset) -
                   (gEventState->y >> 8),
               work->gfx, work->tiles, work->palette, NULL, flags, 50);
}

void EffectReleaseObj(EffectWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
}

void Question_0(EffectWork* work, EventCharaWork* chara) {
    EvtObj* obj;

    work->actor = chara;
    obj = &chara->obj;
    work->x = obj->x;
    work->y = obj->y;
    work->tiles = AllocObjTiles(128, NULL);
    work->palette = LoadObjPalette(gCommonObjPalette, sizeof(gCommonObjPalette));
    SetObjTileSource(work->tiles, gFEventTiles);
    AnimInit(&work->anim, gFEventAnims, gFEventFrames);
    AnimStart(&work->anim, 5, 0);
    work->gfx = AnimGetGfx(&work->anim);
    work->followFlip = FALSE;
    work->timer = 0;
    work->age = 0;
}

s32 Question_1(EffectWork* work) {
    work->gfx = AnimUpdate(&work->anim);
    work->timer++;

    if (work->timer == 12) {
        AnimStart(&work->anim, 6, ANIM_FLAG_LOOP);
    }

    if (!work->actor->callbackActive) {
        return 0;
    }

    work->age = 0;

    return 1;
}

void TinkerbellParticleInit(EffectWork* work, EventCharaWork* chara) {
    EvtObj* obj;
    s32 offsetX;
    s32 offsetY;
    s32 center;

    work->actor = chara;
    obj = &chara->obj;
    center = 0x400;
    offsetX = (GetRandom() % 9 << 8) - center;
    work->x = obj->x + offsetX;
    offsetY = (GetRandom() % 9 << 8) - center;
    work->y = obj->y + offsetY;
    work->z2 = obj->z;
    work->vx = GetRandom() % 232 + 76;
    work->tiles = AllocObjTiles(128, NULL);
    work->palette = LoadObjPalette(gCommonObjPalette, sizeof(gCommonObjPalette));
    SetObjTileSource(work->tiles, gSmnTinkEffTiles);
    AnimInit(&work->anim, gSmnTinkEffAnims, gSmnTinkEffFrames);
    AnimStart(&work->anim, GetRandom() % 3, 0);
    work->gfx = AnimGetGfx(&work->anim);
    work->followFlip = FALSE;
    work->age = 0;
    gEventState->particleCount++;
}

s32 TinkerbellParticleUpdate(EffectWork* work) {
    work->gfx = AnimUpdate(&work->anim);
    work->age++;
    work->z2 += 256;

    if (work->z2 > 0) {
        return 0;
    }

    return 1;
}

void TinkerbellParticleDraw(EffectWork* work) {
    u16 flags;

    flags = work->actor->obj.drawFlags;

    if (!work->followFlip) {
        flags &= ~SPRITE_FLAG_HFLIP;
    }

    DrawSprite((work->x >> 8) - (gEventState->x >> 8),
               ((work->y + work->z2) >> 8) - (gEventState->y >> 8),
               work->gfx, work->tiles, work->palette, NULL, flags,
               -0x1004 - (work->y >> 8) * 4);
}

void TinkerbellParticleDestroy(EffectWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    gEventState->particleCount--;
}

void GlowNose_0(EffectWork* work, EventCharaWork* chara) {
    EvtObj* obj;

    work->actor = chara;
    obj = &chara->obj;
    work->x = obj->x - 1536;
    work->y = obj->y + 3072;
    work->tiles = AllocObjTiles(128, NULL);
    work->palette = LoadObjPalette(gCommonObjPalette, sizeof(gCommonObjPalette));
    SetObjTileSource(work->tiles, gGlowNoseTiles);
    AnimInit(&work->anim, gGlowNoseAnims, gGlowNoseFrames);
    AnimStart(&work->anim, 0, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    work->followFlip = TRUE;
    work->age = 0;
}

s32 GlowNose_1(EffectWork* work) {
    work->gfx = AnimUpdate(&work->anim);
    work->age++;

    if (work->age > 44) {
        return 0;
    }

    return 1;
}

void GlowNose2_0(EffectWork* work, EventCharaWork* chara) {
    EvtObj* obj;

    work->actor = chara;
    obj = &chara->obj;

    switch (chara->arg.chara) {
    case EVENT_CHARA_ROBED_FIGURE:
        work->x = obj->x - 6144;
        work->y = obj->y + 8192;
        break;
    case EVENT_CHARA_AXEL:
        work->x = obj->x + 2048;
        work->y = obj->y + 2048;
        break;
    }

    work->tiles = AllocObjTiles(128, NULL);
    work->palette = LoadObjPalette(gCommonObjPalette, sizeof(gCommonObjPalette));
    SetObjTileSource(work->tiles, gGlowNoseTiles);
    AnimInit(&work->anim, gGlowNoseAnims, gGlowNoseFrames);
    AnimStart(&work->anim, 1, ANIM_FLAG_LOOP);
    work->gfx = AnimGetGfx(&work->anim);
    work->followFlip = TRUE;
    work->age = 0;
}

s32 GlowNose2_1(EffectWork* work) {
    work->gfx = AnimUpdate(&work->anim);
    work->age++;

    if (work->age > 8) {
        return 0;
    }

    return 1;
}

void down_0(EffectWork* work, EventCharaWork* chara) {
    EvtObj* obj;
    DownWork* down;
    u8 i;

    work->actor = chara;
    obj = &chara->obj;

    switch (chara->arg.chara) {
    case EVENT_CHARA_SORA:
        work->x = obj->x + 4096;
        work->y = obj->y - 6144;
        break;
    case EVENT_CHARA_GOOFY:
        work->x = obj->x - 2048;
        work->y = obj->y - 6144;
        break;
    case EVENT_CHARA_DONALD:
        work->x = obj->x + 3584;
        work->y = obj->y - 1024;
        break;
    }

    work->tiles = AllocSpriteFrameTiles(32);
    UpdateSpriteFrameTiles(work->tiles, gLvupLogoFrames[3], gLvupLogoTiles);
    work->palette = LoadObjPalette(gCard00Palette, sizeof(gCard00Palette));
    work->down = EwramAlloc(sizeof(DownWork));
    down = work->down;

    for (i = 0; i < ARRAY_COUNT(down->angle); i++) {
        down->angle[i] = i * 32;
        down->wobble[i] = 0;
    }
}

s32 down_1(EffectWork* work) {
    DownWork* down;
    u8 i;

    down = work->down;

    for (i = 0; i < ARRAY_COUNT(down->x); i++) {
        down->x[i] = SIN(down->angle[i]) * 8 + work->x;
        down->y[i] = -COS(down->angle[i]) * (down->wobble[i] + 4) +
                       work->y;
        down->angle[i] += 4;

        if (down->wobble[i] == 0) {
            down->wobble[i]++;
        } else {
            down->wobble[i] = 0;
        }
    }

    if (!work->actor->callbackActive) {
        return 0;
    }

    return 1;
}

s32 down_2(EffectWork* work) {
    DownWork* down;
    u16 flags;
    u8 i;

    flags = work->actor->obj.drawFlags;
    down = work->down;

    for (i = 0; i < ARRAY_COUNT(down->x); i++) {
        DrawSprite((down->x[i] >> 8) - (gEventState->x >> 8),
                   (down->y[i] >> 8) - (gEventState->y >> 8), NULL,
                   work->tiles, work->palette, NULL, flags, 50);
    }
}

void down_3(EffectWork* work) {
    ReleaseObjTiles(work->tiles);
    ReleaseObjPalette(work->palette);
    EwramFree(work->down);
}

void Tinkerbell_0(EffectWork* work, EventCharaWork* chara) {
    work->actor = chara;
    gEventState->particleCount = 0;
    work->timer = 0;
    TaskPoolInit(&work->tasks, 8);
}

s32 Tinkerbell_1(EffectWork* work) {
    work->timer++;

    if (work->timer == 5) {
        if (gEventState->particleCount <= 3) {
            TaskCreate(&work->tasks, &gTaskDescTinkerbellParticle, work->actor);
        }

        work->timer = 0;
    }

    TaskPoolUpdate(&work->tasks);

    return 1;
}

void Tinkerbell_2(EffectWork* work) {
    TaskPoolDraw(&work->tasks);
}

void Tinkerbell_3(EffectWork* work) {
    TaskPoolDestroy(&work->tasks);
}

void CreateTinkerbellTask(EventCharaWork* work) {
    TaskCreate(&work->tasks, &gTaskDescTinkerbell, work);
}

void CreateDownTask(EventCharaWork* work) {
    TaskCreate(&work->tasks, &gTaskDescDown, work);
}

void CreateSmokeTask(EventCharaWork* work) {
    TaskCreate(&work->tasks, &gTaskDescSmoke, work);
}

void CreateExclamationTask(EventCharaWork* work) {
    TaskCreate(&work->tasks, &gTaskDescExclamation, work);
}

void CreateBalloonTask(EventCharaWork* work) {
    TaskCreate(&work->tasks, &gTaskDescBalloon, work);
}

void CreateQuestionTask(EventCharaWork* work) {
    TaskCreate(&work->tasks, &gTaskDescQuestion, work);
}

void CreateGlowNoseTask(EventCharaWork* work) {
    TaskCreate(&work->tasks, &gTaskDescGlowNose, work);
}

void CreateGlowNose2Task(EventCharaWork* work) {
    TaskCreate(&work->tasks, &gTaskDescGlowNose2, work);
}

void CreateHanabiraTask(EventCharaWork* work) {
    TaskCreate(&work->tasks, &gTaskDescHanabira, work);
}

enum EvSoundFadeMode {
    EV_SOUND_FADE_MODE_NONE,
    EV_SOUND_FADE_MODE_IN,
    EV_SOUND_FADE_MODE_OUT
};

void EV_SOUND_0(EvSoundWork* work, u8* arg) {
    u8 i;

    work->eventId = arg[0];
    work->cue = 0;
    work->unk_06 = 0;
    work->fadeMode = EV_SOUND_FADE_MODE_NONE;
    work->volume = 256;
    work->soundCues = gEventSequenceDefs[work->eventId]->soundCues;
    gEventSoundMix = EwramAlloc(sizeof(EventSoundMix) * 64);

    for (i = 0; i < 64; i++) {
        gEventSoundMix[i].pan = 0;
        gEventSoundMix[i].volume = 256;
    }
}

s32 EV_SOUND_1(EvSoundWork* work) {
    const EvSoundCue* cue;
    MusicPlayerInfo* mp;
    u8 player;
    u8 fadePlayer;
    u8 i;

    if (work->soundCues == NULL) {
        return 0;
    }

    cue = &work->soundCues[work->cue];

    if (gEventState->frame == cue->frame) {
        if (cue->song != SONG_NONE) {
            if ((cue->flags & EV_SOUND_FLAG_STOP) == 0) {
                m4aSongNumStartOrContinue(cue->song);
                player = gSongTable[cue->song].ms;
                m4aMPlayImmInit(gMPlayTable[player].info);
                gEventSoundMix[player].pan = 0;
                gEventSoundMix[player].volume = 256;
            } else {
                m4aSongNumStop(cue->song);
            }
        } else {
            m4aMPlayAllStop();
        }

        if (cue->flags & EV_SOUND_FLAG_FADE_OUT) {
            m4aMPlayFadeOut(gMPlayTable[gSongTable[cue->song].ms].info, 5);
            work->fadeMode = EV_SOUND_FADE_MODE_OUT;
        }

        if (cue->flags & EV_SOUND_FLAG_FADE_IN) {
            fadePlayer = gSongTable[cue->song].ms;
            mp = gMPlayTable[fadePlayer].info;
            work->volume = 3;
            m4aMPlayVolumeControl(mp, 255, 3);
            work->fadeMode = EV_SOUND_FADE_MODE_IN;
        }

        if ((cue->flags & EV_SOUND_FLAG_END) == 0) {
            work->cue++;
        }
    }

    EvSoundUpdateFadeIn(work);

    for (i = 16; i <= 24; i++) {
        m4aMPlayPanpotControl(gMPlayTable[i].info, 255,
                              gEventSoundMix[i].pan);
        m4aMPlayVolumeControl(gMPlayTable[i].info, 255,
                              gEventSoundMix[i].volume);
    }

    return 1;
}

void EV_SOUND_2() {
}

void EV_SOUND_3() {
    EwramFree(gEventSoundMix);
}

void EvSoundUpdateFadeIn(EvSoundWork* work) {
    MusicPlayerInfo* mp;

    mp = gMPlayTable[0].info;

    if (work->fadeMode == EV_SOUND_FADE_MODE_IN) {
        work->volume += 2;

        if (work->volume > 255) {
            work->volume = 256;
        }

        m4aMPlayImmInit(mp);
        m4aMPlayVolumeControl(mp, 255, work->volume);
    }
}

void SetEventSoundPosition(u16 song, s16 x, s16 y) {
    u8 player;
    s32 sx;
    s16 dx;
    s16 pan;
    s16 dist;
    s16 axisDist;
    s16 total;
    u32 twiceDy;

    total = 0;

    if (gEventSoundMix == NULL) {
        return;
    }

    player = gSongTable[song].ms;
    m4aMPlayImmInit(gMPlayTable[player].info);

    if ((u16)x > DISPLAY_WIDTH) {
        gEventSoundMix[player].pan = total;
        gEventSoundMix[player].volume = total;
    }

    if ((u16)y > DISPLAY_HEIGHT) {
        gEventSoundMix[player].pan = total;
        gEventSoundMix[player].volume = total;
    }

    sx = x;
    dx = sx;
    dx -= DISPLAY_WIDTH / 2;
    pan = dx;

    if (pan > 127) {
        pan = 127;
    }

    if (pan < -128) {
        pan = -128;
    }

    gEventSoundMix[player].pan = pan;

    if (DISPLAY_WIDTH / 2 - sx >= 0) {
        axisDist = DISPLAY_WIDTH / 2 - sx;
    } else {
        axisDist = dx;
    }

    dist = axisDist;

    if (80 - y * 2 < 0) {
        twiceDy = y * 2 - 80;
        axisDist = twiceDy / 2;
    } else {
        twiceDy = 80 - y * 2;
        axisDist = twiceDy / 2;
    }

    total = dist + axisDist;

    if (total > 256) {
        total = 256;
    }

    gEventSoundMix[player].volume = 256 - total;

    if (gEventSoundMix[player].volume < 12) {
        gEventSoundMix[player].volume = 12;
    }
}

void Event_Debug_0(EventDebugWork* work) {
    work->tiles = LoadSmallFontTiles();
    work->palette = LoadSmallFontPalette();
}

s32 Event_Debug_1(EventDebugWork* work) {
    work->digitCount = FormatSmallFontDecimal(gEventState->frame, work->digits);
    return 1;
}

void Event_Debug_2(EventDebugWork* work) {
    DrawSmallFontString(0, 0, work->digits, work->tiles, work->palette, 0, work->digitCount);
}

void Event_Debug_3(EventDebugWork* work) {
    FreeSmallFontResources(work->tiles, work->palette);
}

Mode gModeEventselect = {
    "mode_eventselect",
    (ModeInitFunc)mode_eventselect_0,
    mode_eventselect_1,
    mode_eventselect_2,
};

TaskDesc gTaskDescHanabira = {
    "Hanabira",
    (TaskInitFunc)Hanabira_0,
    (TaskUpdateFunc)Hanabira_1,
    (TaskDrawFunc)Hanabira_2,
    (TaskDestroyFunc)Hanabira_3,
    sizeof(EffectWork),
};

TaskDesc gTaskDescHanabiraC = {
    "Hanabira_c",
    (TaskInitFunc)Hanabira_c_0,
    (TaskUpdateFunc)Hanabira_c_1,
    (TaskDrawFunc)Hanabira_c_2,
    (TaskDestroyFunc)Hanabira_c_3,
    sizeof(EffectWork),
};

TaskDesc gTaskDescSmoke = {
    "smoke",
    (TaskInitFunc)smoke_0,
    (TaskUpdateFunc)EffectUpdateObj,
    (TaskDrawFunc)EffectDrawObj,
    (TaskDestroyFunc)EffectReleaseObj,
    sizeof(EffectWork),
};

TaskDesc gTaskDescExclamation = {
    "Exclamation",
    (TaskInitFunc)Exclamation_0,
    (TaskUpdateFunc)Exclamation_1,
    (TaskDrawFunc)EffectDrawObj,
    (TaskDestroyFunc)EffectReleaseObj,
    sizeof(EffectWork),
};

TaskDesc gTaskDescBalloon = {
    "balloon",
    (TaskInitFunc)balloon_0,
    (TaskUpdateFunc)EffectUpdateObj,
    (TaskDrawFunc)EffectDrawObj,
    (TaskDestroyFunc)EffectReleaseObj,
    sizeof(EffectWork),
};

TaskDesc gTaskDescQuestion = {
    "Question",
    (TaskInitFunc)Question_0,
    (TaskUpdateFunc)Question_1,
    (TaskDrawFunc)EffectDrawObj,
    (TaskDestroyFunc)EffectReleaseObj,
    sizeof(EffectWork),
};

TaskDesc gTaskDescTinkerbellParticle = {
    "GlowNose",
    (TaskInitFunc)TinkerbellParticleInit,
    (TaskUpdateFunc)TinkerbellParticleUpdate,
    (TaskDrawFunc)TinkerbellParticleDraw,
    (TaskDestroyFunc)TinkerbellParticleDestroy,
    sizeof(EffectWork),
};

TaskDesc gTaskDescGlowNose = {
    "GlowNose",
    (TaskInitFunc)GlowNose_0,
    (TaskUpdateFunc)GlowNose_1,
    (TaskDrawFunc)EffectDrawObj,
    (TaskDestroyFunc)EffectReleaseObj,
    sizeof(EffectWork),
};

TaskDesc gTaskDescGlowNose2 = {
    "GlowNose",
    (TaskInitFunc)GlowNose2_0,
    (TaskUpdateFunc)GlowNose2_1,
    (TaskDrawFunc)EffectDrawObj,
    (TaskDestroyFunc)EffectReleaseObj,
    sizeof(EffectWork),
};

TaskDesc gTaskDescDown = {
    "down",
    (TaskInitFunc)down_0,
    (TaskUpdateFunc)down_1,
    (TaskDrawFunc)down_2,
    (TaskDestroyFunc)down_3,
    sizeof(EffectWork),
};

TaskDesc gTaskDescTinkerbell = {
    "Tinkerbell",
    (TaskInitFunc)Tinkerbell_0,
    (TaskUpdateFunc)Tinkerbell_1,
    (TaskDrawFunc)Tinkerbell_2,
    (TaskDestroyFunc)Tinkerbell_3,
    sizeof(EffectWork),
};

TaskDesc gTaskDescEvSound = {
    "EV_SOUND",
    (TaskInitFunc)EV_SOUND_0,
    (TaskUpdateFunc)EV_SOUND_1,
    (TaskDrawFunc)EV_SOUND_2,
    (TaskDestroyFunc)EV_SOUND_3,
    sizeof(EvSoundWork),
};

TaskDesc gTaskDescEventDebug = {
    "Event_Debug",
    (TaskInitFunc)Event_Debug_0,
    (TaskUpdateFunc)Event_Debug_1,
    (TaskDrawFunc)Event_Debug_2,
    (TaskDestroyFunc)Event_Debug_3,
    sizeof(EventDebugWork),
};
