/**
 * mode_ms_top.c
 * Moogle Shop Top Menu
 */

#include "mode_ms_top.h"
#include "registration_data.h"
#include "system_state.h"
#include "mode_ms.h"
#include "mode_ms_top_api.h"
#include "anim.h"
#include "sprites_btl.h"
#include "sprites_evt.h"
#include "sprites_moogle_shop.h"
#include "sprites_sora.h"
#include "gba/keys.h"
#include "gba/io_reg.h"
#include "mode_ms_api.h"
#include "fade.h"
#include "songs.h"
#include "player_progression.h"
#include "card_api.h"
#include "display.h"
#include "engine_math.h"
#include "game_state.h"
#include "key.h"
#include "m4a_song.h"
#include "map_api.h"
#include "map_types.h"
#include "mode.h"
#include "obj.h"
#include "obj_api.h"
#include "player_progression_types.h"
#include "taskpool.h"
#include "types.h"
#include <stddef.h>
#include "card_msgwin.h"
#include "sprite_palettes.h"
#include "card_message_data.h"
#include "jiminy_records_index_data.h"
#ifdef PLATFORM_PC
#include "port/port_hdsprites.h"
#endif

enum MsTopState {
    MS_TOP_STATE_BARS_IN,
    MS_TOP_STATE_TITLE_IN,
    MS_TOP_STATE_CHECK_INTRO,
    MS_TOP_STATE_INTRO,
    MS_TOP_STATE_CHECK_FREE_PACK,
    MS_TOP_STATE_FREE_PACK_MESSAGE,
    MS_TOP_STATE_FREE_PACK,
    MS_TOP_STATE_SHOW_OPTION,
    MS_TOP_STATE_SELECT,
    MS_TOP_STATE_TITLE_OUT,
    MS_TOP_STATE_BARS_OUT,
    MS_TOP_STATE_EXIT
};

enum MsTopDir {
    MS_TOP_DIR_NONE,
    MS_TOP_DIR_LEFT,
    MS_TOP_DIR_RIGHT
};

static s16 sMsTopCursor;
static void* sMsTopNextMode;
static struct ObjTiles* sMsTopBarTiles;
static struct ObjPalette* sMsTopBarPalette;
static struct ObjTiles* sMsTopArrowTiles;
static struct ObjPalette* sMsTopArrowPalette;
static AnimState sMsTopArrowAnim;
static struct ObjTiles* sMsTopSoraTiles;
static struct ObjPalette* sMsTopSoraPalette;
static AnimState sMsTopSoraAnim;
static struct ObjTiles* sMsTopShadowTiles;
static struct ObjPalette* sMsTopShadowPalette;
static void* sMsTopWarpTiles[2];
static void* sMsTopWarpPalettes[2];
static AnimState sMsTopWarpAnims[2];
static struct ObjTiles* sMsTopMoogleTiles;
static struct ObjPalette* sMsTopMooglePalette;
static AnimState sMsTopMoogleAnim;
static s16 sMsTopScrollDir;
static s16 sMsTopScrollSteps;
static s16 sMsTopMoogleWalkDir;
static s32 sMsTopBg1ScrollX;
static s32 sMsTopBg0ScrollX;
static s32 sMsTopObjScrollX;
static s32 sMsTopMoogleX;
static s16 sMsTopState;
static s16 sMsTopSteps;
static s32 sMsTopBarY[2];
static s32 sMsTopBarX;
static u8 sMsTopBarVisible;
static TaskPool sMsTopTaskPool;
static s16 sMsTopPendingMessage;
static s16 sMsTopPendingOptionMessage;
static u8 sMsTopMessageStarted;
static s16 sMsTopIntroIndex;

static const WarpDef sWarpDefs[2] = {
    {&gModeMsShop, gMsTopRoomBgMap, sizeof(gMsTopRoomBgMap), 104, 48, 1, 136, 80, 0, 48, 66, {{64, 64, gMsTopPacksPalette, sizeof(gMsTopPacksPalette), gMsTopPacksTiles, sizeof(gMsTopPacksTiles), gMsTopPacksAnims, gMsTopPacksFrames, 0}, {192, 84, gMoguPalette, sizeof(gMoguPalette), gMsChargeMoogleTiles, sizeof(gMsChargeMoogleTiles), gMsChargeMoogleAnims, gMsChargeMoogleFrames, 1}}},
    {&gModeMsCharge, gMsTopSpotlightBgMap, sizeof(gMsTopSpotlightBgMap), 76, 48, 0, 112, 80, SPRITE_FLAG_HFLIP, 32, 32, {{32, 64, gMsTopPacksDimPalette, sizeof(gMsTopPacksDimPalette), gMsTopPacksDimTiles, sizeof(gMsTopPacksDimTiles), gMsTopPacksDimAnims, gMsTopPacksDimFrames, 0}, {160, 84, gMsTopMoogleDimPalette, sizeof(gMsTopMoogleDimPalette), gMsChargeMoogleTiles, sizeof(gMsChargeMoogleTiles), gMsChargeMoogleAnims, gMsChargeMoogleFrames, 0}}},
};

static const u16 sMsTopIntroMessages[3] = {
    CARD_MSG_MS_TOP_INTRO_0,
    CARD_MSG_MS_TOP_INTRO_1,
    CARD_MSG_MS_TOP_INTRO_2,
};

#ifdef VERSION_EU
static void* sMsTopBgMapsByLanguage[5] = {
    gMsTopBgMap,
    gMsTopBgFrenchMap,
    gMsTopBgGermanMap,
    gMsTopBgItalianMap,
    gMsTopBgSpanishMap,
};

static void* sMsTopBarBgMapsByLanguage[5] = {
    gMsTopBarBgMap,
    gMsTopBarBgFrenchMap,
    gMsTopBarBgGermanMap,
    gMsTopBarBgItalianMap,
    gMsTopBarBgSpanishMap,
};

static void* sMsTopTitleSpritesByLanguage[5] = {
    gMsTopBarFrame0,
    gMsTopBarFrenchFrame0,
    gMsTopBarGermanFrame0,
    gMsTopBarItalianFrame0,
    gMsTopBarSpanishFrame0,
};

static void* sMsTopTopBarSpritesByLanguage[5] = {
    gMsTopBarFrame1,
    gMsTopBarFrenchFrame1,
    gMsTopBarGermanFrame1,
    gMsTopBarItalianFrame1,
    gMsTopBarSpanishFrame1,
};

static void* sMsTopBottomBarSpritesByLanguage[5] = {
    gMsTopBarFrame2,
    gMsTopBarFrenchFrame2,
    gMsTopBarGermanFrame2,
    gMsTopBarItalianFrame2,
    gMsTopBarSpanishFrame2,
};
#endif

Mode gModeMsTop = {
    "mode_ms_top",
    (ModeInitFunc)mode_ms_top_0,
    mode_ms_top_1,
    mode_ms_top_2,
};

u32 GetMooglePoints() {
    return gGameState.progression.mooglePoints;
}

void SetMooglePoints(u32 points) {
    gGameState.progression.mooglePoints = points;
}

u8 SpendMooglePoints(u32 points) {
    u8 ok = FALSE;

    if (GetMooglePoints() >= points) {
        SetMooglePoints(GetMooglePoints() - points);
        ok = TRUE;
    }

    return ok;
}

u8 AddMooglePoints(u32 points) {
    points += GetMooglePoints();

    if (points > 99999) {
        SetMooglePoints(99999);
        return FALSE;
    }

    SetMooglePoints(points);
    return TRUE;
}

void LoadDecimalDigitTiles(u32 value, u8* glyphs, u8* dst, u16 stride, u16 count) {
    s32 i;
    u32 digit;

    for (i = 0; i < count; i++) {
        digit = value % 10;
        value /= 10;
        RequestDma3Copy(&glyphs[digit * stride], &dst[(count - 1 - i) * stride], stride);
    }
}

void UpdateMsTopMooglePalette() {
    s32 dim;
    s16 x;
    s32 screenX;
    s32 base;

    x = sWarpDefs[0].x3 + ((sMsTopMoogleX - sMsTopBg0ScrollX) >> 8);
    dim = FALSE;
    screenX = x;
    base = (-sMsTopBg1ScrollX) >> 8;

    if (screenX <= base + 0x1C || screenX >= base + 0x1C + 0x62) {
        dim = TRUE;
    }

    ReleaseObjPalette(sMsTopMooglePalette);
    sMsTopMooglePalette = LoadObjPalette(!dim ? gMoguPalette : gMsTopMoogleDimPalette, 0x20);
}

void UpdateMsTopWarpGfx() {
    s16 i;
    s32 dim;
    s16 x;
    s32 screenX;
    s32 base;

    for (i = 0; i <= 1; i++) {
        x = sWarpDefs[0].gfx[i].x - (sMsTopBg0ScrollX >> 8);
        dim = 0;
        screenX = x;
        base = (-sMsTopBg1ScrollX) >> 8;

        if (screenX <= base + 0x18 || screenX >= base + 0x18 + 0x6A) {
            dim = 1;
        }

        if (sMsTopWarpPalettes[i] != NULL) {
            ReleaseObjPalette(sMsTopWarpPalettes[i]);
        }

        if (sMsTopWarpTiles[i] != NULL) {
            ReleaseObjTiles(sMsTopWarpTiles[i]);
        }

        sMsTopWarpPalettes[i] = LoadObjPalette(sWarpDefs[dim].gfx[i].palette, sWarpDefs[dim].gfx[i].paletteSize);
        sMsTopWarpTiles[i] = LoadObjTiles(sWarpDefs[dim].gfx[i].tiles, sWarpDefs[dim].gfx[i].tilesSize);
        AnimInit(&sMsTopWarpAnims[i], sWarpDefs[dim].gfx[i].anims, sWarpDefs[dim].gfx[i].gfxTable);
        AnimStart(&sMsTopWarpAnims[i], sWarpDefs[dim].gfx[i].animId, ANIM_FLAG_LOOP);
    }
}

void SetMsTopWarpAnim(s16 cursor) {
    AnimStart(&sMsTopArrowAnim, sWarpDefs[cursor].animId, ANIM_FLAG_LOOP);
}

void QueueMsTopIntroMessage() {
    if (sMsTopPendingMessage < 0) {
        if (sMsTopIntroIndex <= 2) {
            sMsTopPendingMessage = sMsTopIntroMessages[sMsTopIntroIndex];
            sMsTopIntroIndex++;
        }
    }
}

void MsTopHandleInput() {
    s16 prev;
    u16 keys;

    prev = sMsTopCursor;
    keys = GetKeysPressed();

    if (keys & A_BUTTON) {
        if (sMsTopCursor == 1) {
            AnimStart(&sMsTopWarpAnims[sMsTopCursor], 2, ANIM_FLAG_LOOP);
        }

        sMsTopNextMode = sWarpDefs[sMsTopCursor].mode;
        m4aSongNumStart(SONG_SYS_KETTEI);
        FadeStartOut(FADE_MODE_BLACK, 16);
        FadeLock();
        sMsTopState = MS_TOP_STATE_EXIT;
    } else if (keys & B_BUTTON) {
        sMsTopNextMode = NULL;
        m4aSongNumStart(SONG_SYS_CLOSE);
        sMsTopBarVisible = TRUE;
#ifdef VERSION_EU
        LoadBgMap(2, sMsTopBgMapsByLanguage[gLanguage], 0x500);
#else
        LoadBgMap(2, gMsTopBgMap, sizeof(gMsTopBgMap));
#endif
        sMsTopSteps = 16;
        sMsTopState = MS_TOP_STATE_TITLE_OUT;
    } else if (keys & START_BUTTON) {
        sMsTopNextMode = NULL;
        m4aSongNumStart(SONG_SYS_CLOSE);
        sMsTopBarVisible = TRUE;
#ifdef VERSION_EU
        LoadBgMap(2, sMsTopBgMapsByLanguage[gLanguage], 0x500);
#else
        LoadBgMap(2, gMsTopBgMap, sizeof(gMsTopBgMap));
#endif
        FadeStartOut(FADE_MODE_BLACK, 16);
        FadeLock();
        sMsTopState = MS_TOP_STATE_EXIT;
    } else if ((keys & DPAD_LEFT) && sMsTopScrollDir != MS_TOP_DIR_LEFT && sMsTopCursor != 0) {
        sMsTopCursor = 0;
        sMsTopScrollDir = MS_TOP_DIR_LEFT;
        sMsTopScrollSteps = 30 - sMsTopScrollSteps;
        sMsTopMoogleWalkDir = MS_TOP_DIR_LEFT;
        AnimStart(&sMsTopMoogleAnim, 1, ANIM_FLAG_LOOP);
    } else if ((keys & DPAD_RIGHT) && sMsTopScrollDir != MS_TOP_DIR_RIGHT && sMsTopCursor != 1) {
        sMsTopCursor = 1;
        sMsTopScrollDir = MS_TOP_DIR_RIGHT;
        sMsTopScrollSteps = 30 - sMsTopScrollSteps;
        sMsTopMoogleWalkDir = MS_TOP_DIR_RIGHT;
        AnimStart(&sMsTopMoogleAnim, 1, ANIM_FLAG_LOOP);
    }

    if (sMsTopCursor != prev) {
        if (IsMessageWindowOpen()) {
            CloseMessageWindow();
        }

        sMsTopPendingOptionMessage = sMsTopCursor == 0 ? CARD_MSG_MS_TOP_SHOP_OPTION : CARD_MSG_MS_TOP_CHARGE_OPTION;
        SetMsTopWarpAnim(sMsTopCursor);
        m4aSongNumStart(SONG_SYS_CLICK);
    }
}

void MsTopDraw() {
    s32 i;
    u16 flags;

#ifdef PLATFORM_PC
    /* the title's HD banner: the sprite stays over the BG copy the shop switches to once it has slid in */
    if (!sMsTopBarVisible && sMsTopBarTiles != NULL &&
        PortHdSpriteFind(sMsTopBarTiles->src,
#ifdef VERSION_EU
                         sMsTopTitleSpritesByLanguage[gLanguage]
#else
                         gMsTopBarFrame0
#endif
                         ))
        DrawSprite(0, 0,
#ifdef VERSION_EU
                   sMsTopTitleSpritesByLanguage[gLanguage],
#else
                   gMsTopBarFrame0,
#endif
                   sMsTopBarTiles, sMsTopBarPalette, NULL, SPRITE_PRIORITY(1), 0x7D0); /* over BG2 */
#endif
    if (sMsTopBarVisible) {
#ifdef VERSION_EU
        DrawSprite(sMsTopBarX >> 8, 0, sMsTopTitleSpritesByLanguage[gLanguage], sMsTopBarTiles, sMsTopBarPalette, NULL, SPRITE_PRIORITY(2), 0x7D0);
        DrawSprite(0x80, sMsTopBarY[0] >> 8, sMsTopTopBarSpritesByLanguage[gLanguage], sMsTopBarTiles, sMsTopBarPalette, NULL, SPRITE_PRIORITY(2), 0x7D1);
        DrawSprite(0x80, sMsTopBarY[1] >> 8, sMsTopBottomBarSpritesByLanguage[gLanguage], sMsTopBarTiles, sMsTopBarPalette, NULL, SPRITE_PRIORITY(2), 0x7D1);
#else
        DrawSprite(sMsTopBarX >> 8, 0, gMsTopBarFrame0, sMsTopBarTiles, sMsTopBarPalette, NULL, SPRITE_PRIORITY(2), 0x7D0);
        DrawSprite(0x80, sMsTopBarY[0] >> 8, gMsTopBarFrame1, sMsTopBarTiles, sMsTopBarPalette, NULL, SPRITE_PRIORITY(2), 0x7D1);
        DrawSprite(0x80, sMsTopBarY[1] >> 8, gMsTopBarFrame2, sMsTopBarTiles, sMsTopBarPalette, NULL, SPRITE_PRIORITY(2), 0x7D1);
#endif
    }

    flags = SPRITE_PRIORITY(2);

    switch (sMsTopMoogleWalkDir) {
    case MS_TOP_DIR_NONE:
        if (sMsTopCursor == 0) {
            flags |= SPRITE_FLAG_HFLIP;
        }

        break;
    case MS_TOP_DIR_LEFT:
        break;
    case MS_TOP_DIR_RIGHT:
        flags |= SPRITE_FLAG_HFLIP;
        break;
    }

    DrawSprite(sWarpDefs[0].x3 + ((sMsTopMoogleX - sMsTopBg0ScrollX) >> 8), sWarpDefs[0].y3,
        AnimUpdate(&sMsTopMoogleAnim), sMsTopMoogleTiles, sMsTopMooglePalette, NULL, flags, 0x834);
    DrawSprite((sMsTopObjScrollX >> 8) + sWarpDefs[0].x, sWarpDefs[0].y,
        AnimUpdate(&sMsTopArrowAnim), sMsTopArrowTiles, sMsTopArrowPalette, NULL, SPRITE_PRIORITY(2), 0x7D0);

    DrawSprite(sWarpDefs[0].x2 + (sMsTopObjScrollX >> 8), sWarpDefs[0].y2,
        AnimUpdate(&sMsTopSoraAnim), sMsTopSoraTiles, sMsTopSoraPalette, NULL,
        SPRITE_PRIORITY(2) | sWarpDefs[sMsTopCursor].flags, 0x7D0);
    DrawSprite(sWarpDefs[0].x2 + (sMsTopObjScrollX >> 8), sWarpDefs[0].y2,
        gBtlShadowFrame0, sMsTopShadowTiles, sMsTopShadowPalette, NULL,
        SPRITE_PRIORITY(2) | sWarpDefs[sMsTopCursor].flags, 0x7D1);

    for (i = 0; i <= 1; i++) {
        DrawSprite(sWarpDefs[0].gfx[i].x - (sMsTopBg0ScrollX >> 8), sWarpDefs[0].gfx[i].y,
            AnimUpdate(&sMsTopWarpAnims[i]), sMsTopWarpTiles[i], sMsTopWarpPalettes[i], NULL, SPRITE_PRIORITY(2), 0x7D0);
    }

    if (sMsTopState == MS_TOP_STATE_FREE_PACK) {
        DrawMooglePackOpening();
    }

    TaskPoolDraw(&sMsTopTaskPool);
    SetBgScroll(0, (u16)(sMsTopBg0ScrollX >> 8), 0);
    SetBgScroll(1, (u16)(sMsTopBg1ScrollX >> 8), 0);
}

void mode_ms_top_0(u32 flags) {
    s32 i;

    SpriteReset();
    FadeStartIn(FADE_MODE_BLACK, 16);
    SetBgMode0();
    gBldCnt = (BLDCNT_TGT1_BG1 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG0);
    gBldAlpha = BLDALPHA_BLEND(16, 16);
    SetupBg(0, 0, 28, 0);
    SetupBg(1, 0, 29, 0);
    SetupBg(2, 0, 30, 0);
    SetupBg(3, 3, 31, 14);
    SetBgPriority(0, 3);
    SetBgPriority(1, 2);
    SetBgPriority(2, 1);
    SetBgPriority(3, 0);

    if (flags & 1) {
        ClearMoogleRoomFlags();
    }

    if (flags & 2) {
        sMsTopBarVisible = FALSE;
        sMsTopState = MS_TOP_STATE_SHOW_OPTION;
    } else {
        sMsTopBarVisible = TRUE;
        sMsTopState = MS_TOP_STATE_BARS_IN;
        sMsTopSteps = 16;
        sMsTopBarY[0] = -0x800;
        sMsTopBarY[1] = 0xA800;
        sMsTopBarX = -0x8000;
        sMsTopCursor = 0;
    }

    sMsTopScrollDir = MS_TOP_DIR_NONE;
    sMsTopScrollSteps = 0;
    sMsTopMoogleWalkDir = MS_TOP_DIR_NONE;

    if (sMsTopCursor == 0) {
        sMsTopBg1ScrollX = 0;
        sMsTopBg0ScrollX = 0;
        sMsTopObjScrollX = 0;
        sMsTopMoogleX = 0;
    } else {
        sMsTopBg1ScrollX = -0x6100;
        sMsTopBg0ScrollX = 0x2100;
        sMsTopObjScrollX = -0x1C00;
        sMsTopMoogleX = 0xBC00;
    }

    sMsTopPendingMessage = -1;
    sMsTopPendingOptionMessage = -1;
    sMsTopMessageStarted = FALSE;
    LoadBgPalette(0, gMsTopBgPalette, sizeof(gMsTopBgPalette));
    LoadBgTiles(0, gMsTopBgTiles, sizeof(gMsTopBgTiles));
    LoadDecimalDigitTiles(GetMooglePoints(), gMsTopPointsDigitTiles, (u8*)GetBgCharBase(0) + 0x20, 0x20, 5);
    LoadBgMap(0, gMsTopRoomBgMap, sizeof(gMsTopRoomBgMap));
    LoadBgMap(1, gMsTopSpotlightBgMap, sizeof(gMsTopSpotlightBgMap));

    if (sMsTopBarVisible) {
#ifdef VERSION_EU
        LoadBgMap(2, sMsTopBgMapsByLanguage[gLanguage], 0x500);
#else
        LoadBgMap(2, gMsTopBgMap, sizeof(gMsTopBgMap));
#endif
    } else {
#ifdef VERSION_EU
        LoadBgMap(2, sMsTopBarBgMapsByLanguage[gLanguage], 0x500);
#else
        LoadBgMap(2, gMsTopBarBgMap, sizeof(gMsTopBarBgMap));
#endif
    }

    for (i = 0; i < 2; i++) {
        sMsTopWarpPalettes[i] = NULL;
        sMsTopWarpTiles[i] = NULL;
    }

    sMsTopBarPalette = LoadObjPalette(gMsTopBarPalette, sizeof(gMsTopBarPalette));
    sMsTopBarTiles = LoadObjTiles(gMsTopBarTiles, sizeof(gMsTopBarTiles));
    sMsTopArrowPalette = LoadObjPalette(gMsTopArrowPalette, sizeof(gMsTopArrowPalette));
    sMsTopArrowTiles = LoadObjTiles(gMsTopArrowTiles, sizeof(gMsTopArrowTiles));
    AnimInit(&sMsTopArrowAnim, gMsTopArrowAnims, gMsTopArrowFrames);
    sMsTopSoraPalette = LoadObjPalette(gSoraPalette, sizeof(gSoraPalette));
    sMsTopSoraTiles = LoadObjTiles(gSor1ll00Tiles, sizeof(gSor1ll00Tiles));
    AnimInit(&sMsTopSoraAnim, gSor1ll00Anims, gSor1ll00Frames);
    AnimStart(&sMsTopSoraAnim, 0, ANIM_FLAG_LOOP);
    sMsTopShadowPalette = LoadObjPalette(gBStatesPalette, sizeof(gBStatesPalette));
    sMsTopShadowTiles = LoadObjTiles(gBtlShadowTiles, sizeof(gBtlShadowTiles));
    sMsTopMooglePalette = LoadObjPalette(gMoguPalette, sizeof(gMoguPalette));
    sMsTopMoogleTiles = LoadObjTiles(gMoguFl00Tiles, sizeof(gMoguFl00Tiles));
    AnimInit(&sMsTopMoogleAnim, gMoguFl00Anims, gMoguFl00Frames);
    AnimStart(&sMsTopMoogleAnim, 0, ANIM_FLAG_LOOP);
    SetMsTopWarpAnim(sMsTopCursor);
    UpdateMsTopWarpGfx();
    UpdateMsTopMooglePalette();
    TaskPoolInit(&sMsTopTaskPool, 1);
    EnableBg(0);
    EnableBg(1);
    EnableBg(2);
    DisableBg(3);
}

void mode_ms_top_1() {
    UpdatePlayTime();

    switch (sMsTopState) {
    case MS_TOP_STATE_BARS_IN:
        ApproachValue(&sMsTopBarY[0], 0, sMsTopSteps);
        ApproachValue(&sMsTopBarY[1], 0x9800, sMsTopSteps);

        if (--sMsTopSteps <= 0) {
            sMsTopSteps = 16;
            sMsTopState = MS_TOP_STATE_TITLE_IN;
        }

        break;
    case MS_TOP_STATE_TITLE_IN:
        ApproachValue(&sMsTopBarX, 0, sMsTopSteps);

        if (--sMsTopSteps <= 0) {
            sMsTopBarVisible = FALSE;
#ifdef VERSION_EU
            LoadBgMap(2, sMsTopBarBgMapsByLanguage[gLanguage], 0x500);
#else
            LoadBgMap(2, gMsTopBarBgMap, sizeof(gMsTopBarBgMap));
#endif
            sMsTopState = MS_TOP_STATE_CHECK_INTRO;
        }

        break;
    case MS_TOP_STATE_CHECK_INTRO:
        if ((gGameState.progression.tutorialFlags & TUTORIAL_FLAG_MOOGLE_SHOP) == 0) {
            sMsTopIntroIndex = 0;
            sMsTopState = MS_TOP_STATE_INTRO;
        } else {
            sMsTopState = MS_TOP_STATE_CHECK_FREE_PACK;
        }

        break;
    case MS_TOP_STATE_INTRO:
        QueueMsTopIntroMessage();

        if (sMsTopIntroIndex > 2 && sMsTopPendingMessage < 0 && !IsMessageWindowOpen()) {
            SetJiminyFlag(JIMINY_RECORD_CHARACTER_MOOGLES);
            gGameState.progression.tutorialFlags |= TUTORIAL_FLAG_MOOGLE_SHOP;
            sMsTopState = MS_TOP_STATE_CHECK_FREE_PACK;
        }

        break;
    case MS_TOP_STATE_CHECK_FREE_PACK:
        if (GetMoogleFreePackFlag(gMapFloorState.room) == 0) {
            sMsTopPendingMessage = CARD_MSG_MS_TOP_FREE_PACK;
            sMsTopState = MS_TOP_STATE_FREE_PACK_MESSAGE;
        } else {
            sMsTopState = MS_TOP_STATE_SHOW_OPTION;
        }

        break;
    case MS_TOP_STATE_FREE_PACK_MESSAGE:
        if (!IsMessageWindowOpen()) {
            if (gGameState.floor <= 5) {
                RollMooglePackCards(0, 0);
            } else if (gGameState.floor <= 9) {
                RollMooglePackCards(0, 1);
            } else {
                RollMooglePackCards(0, 2);
            }

            InitMooglePackOpening(120, 80);
            FadeSetPaletteExcluded(13, TRUE);
            FadeToAmount(FADE_MODE_BLACK, 16, 8);
            sMsTopState = MS_TOP_STATE_FREE_PACK;
        }

        break;
    case MS_TOP_STATE_FREE_PACK:
        if (!UpdateMooglePackOpening(1)) {
            ReleaseMooglePackOpening();
            SetMoogleFreePackFlag(gMapFloorState.room);
            SetupBg(3, 3, 31, 14);
            DisableBg(3);
            LoadDecimalDigitTiles(GetMooglePoints(), gMsTopPointsDigitTiles, (u8*)GetBgCharBase(0) + 0x20, 0x20, 5);
            FadeToOriginal(FADE_MODE_BLACK, 8);
            sMsTopState = MS_TOP_STATE_SHOW_OPTION;
        }

        break;
    case MS_TOP_STATE_SHOW_OPTION:
        sMsTopPendingOptionMessage = sMsTopCursor == 0 ? CARD_MSG_MS_TOP_SHOP_OPTION : CARD_MSG_MS_TOP_CHARGE_OPTION;
        sMsTopState = MS_TOP_STATE_SELECT;
        break;
    case MS_TOP_STATE_SELECT:
        MsTopHandleInput();
        break;
    case MS_TOP_STATE_TITLE_OUT:
        ApproachValue(&sMsTopBarX, -0x8000, sMsTopSteps);

        if (--sMsTopSteps <= 0) {
            sMsTopSteps = 16;
            sMsTopState = MS_TOP_STATE_BARS_OUT;
        }

        break;
    case MS_TOP_STATE_BARS_OUT:
        ApproachValue(&sMsTopBarY[0], -0x800, sMsTopSteps);
        ApproachValue(&sMsTopBarY[1], 0xA800, sMsTopSteps);

        if (--sMsTopSteps <= 0) {
            FadeStartOut(FADE_MODE_BLACK, 16);
            FadeLock();
            sMsTopState = MS_TOP_STATE_EXIT;
        }

        break;
    case MS_TOP_STATE_EXIT:
        if (!FadeIsActive()) {
            if (sMsTopNextMode != NULL) {
                ModeRequest(sMsTopNextMode, 0);
            } else {
                RequestMapMode();
            }
        }

        break;
    }

    switch (sMsTopScrollDir) {
    case MS_TOP_DIR_NONE:
        break;
    case MS_TOP_DIR_LEFT:
        UpdateMsTopWarpGfx();
        ApproachValue(&sMsTopBg1ScrollX, 0, sMsTopScrollSteps);
        ApproachValue(&sMsTopBg0ScrollX, 0, sMsTopScrollSteps);
        ApproachValue(&sMsTopObjScrollX, 0, sMsTopScrollSteps);

        if (--sMsTopScrollSteps <= 0) {
            sMsTopScrollDir = MS_TOP_DIR_NONE;
        }

        break;
    case MS_TOP_DIR_RIGHT:
        UpdateMsTopWarpGfx();
        ApproachValue(&sMsTopBg1ScrollX, -0x6100, sMsTopScrollSteps);
        ApproachValue(&sMsTopBg0ScrollX, 0x2100, sMsTopScrollSteps);
        ApproachValue(&sMsTopObjScrollX, -0x1C00, sMsTopScrollSteps);

        if (--sMsTopScrollSteps <= 0) {
            sMsTopScrollDir = MS_TOP_DIR_NONE;
        }

        break;
    }

    switch (sMsTopMoogleWalkDir) {
    case MS_TOP_DIR_NONE:
        break;
    case MS_TOP_DIR_LEFT:
        UpdateMsTopMooglePalette();
        sMsTopMoogleX -= 0x180;

        if (sMsTopMoogleX <= 0) {
            AnimStart(&sMsTopMoogleAnim, 0, ANIM_FLAG_LOOP);
            sMsTopMoogleX = 0;
            sMsTopMoogleWalkDir = MS_TOP_DIR_NONE;
        }

        break;
    case MS_TOP_DIR_RIGHT:
        UpdateMsTopMooglePalette();
        sMsTopMoogleX += 0x180;

        if (sMsTopMoogleX >= 0xBC00) {
            AnimStart(&sMsTopMoogleAnim, 0, ANIM_FLAG_LOOP);
            sMsTopMoogleX = 0xBC00;
            sMsTopMoogleWalkDir = MS_TOP_DIR_NONE;
        }

        break;
    }

    if (sMsTopPendingOptionMessage >= 0) {
        if (sMsTopMessageStarted) {
            if (!IsMessageWindowOpen()) {
                ShowPersistentCardMessage(&sMsTopTaskPool, 3, sMsTopPendingOptionMessage);
                sMsTopPendingOptionMessage = -1;
            }
        } else {
            ShowPersistentCardMessage(&sMsTopTaskPool, 3, sMsTopPendingOptionMessage);
            sMsTopPendingOptionMessage = -1;
            sMsTopMessageStarted = TRUE;
        }
    }

    if (sMsTopPendingMessage >= 0) {
        if (sMsTopMessageStarted) {
            if (!IsMessageWindowOpen()) {
                CreateCardMessageTask(&sMsTopTaskPool, 3, sMsTopPendingMessage);
                sMsTopPendingMessage = -1;
            }
        } else {
            CreateCardMessageTask(&sMsTopTaskPool, 3, sMsTopPendingMessage);
            sMsTopPendingMessage = -1;
            sMsTopMessageStarted = TRUE;
        }
    }

    TaskPoolUpdate(&sMsTopTaskPool);
    MsTopDraw();
}

void mode_ms_top_2() {
    s32 i;

    ReleaseObjPalette(sMsTopBarPalette);
    ReleaseObjTiles(sMsTopBarTiles);
    ReleaseObjPalette(sMsTopArrowPalette);
    ReleaseObjTiles(sMsTopArrowTiles);
    ReleaseObjPalette(sMsTopSoraPalette);
    ReleaseObjTiles(sMsTopSoraTiles);
    ReleaseObjPalette(sMsTopShadowPalette);
    ReleaseObjTiles(sMsTopShadowTiles);

    for (i = 0; i < 2; i++) {
        ReleaseObjPalette(sMsTopWarpPalettes[i]);
        ReleaseObjTiles(sMsTopWarpTiles[i]);
    }

    ReleaseObjPalette(sMsTopMooglePalette);
    ReleaseObjTiles(sMsTopMoogleTiles);
    TaskPoolDestroy(&sMsTopTaskPool);
}
