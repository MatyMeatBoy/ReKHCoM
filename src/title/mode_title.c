/**
 * mode_title.c
 * Title Screen
 */

#ifdef PLATFORM_PC
#include "engine.h"
#endif
#include "macros.h"
#include "registration_data.h"
#include "title_api.h"
#include "map_api.h"
#include "system_state.h"
#include "sprites_title.h"
#include "game_state.h"
#include "gba/keys.h"
#include "gba/io_reg.h"
#include "mode_pooh_api.h"
#include "card_api.h"
#include "malloc.h"
#include "fade.h"
#include "songs.h"
#include "display.h"
#include "engine_math.h"
#include "gba/defines.h"
#include "intr.h"
#include "key.h"
#include "m4a.h"
#include "m4a_song.h"
#include "mode.h"
#include "mode_sio_api.h"
#include "save_api.h"
#include "save_types.h"
#include <stddef.h>
#include "taskpool.h"
#include "types.h"

s32 gTitleBgScale EWRAM_COMMON(4);
s32 gTitleBgX EWRAM_COMMON(4);
s32 gTitleBgY EWRAM_COMMON(4);

enum TitleState {
    TITLE_STATE_FADE_IN,
    TITLE_STATE_ZOOM,
    TITLE_STATE_WHITE_OUT,
    TITLE_STATE_LOGO_IN,
    TITLE_STATE_CROSSFADE_BG,
    TITLE_STATE_PRESS_START,
    TITLE_STATE_MENU_IN,
    TITLE_STATE_MENU_OUT,
    TITLE_STATE_MENU,
    TITLE_STATE_FADE_OUT
};

static u32 sTitleState;
#ifdef PLATFORM_PC
u32* PortTitleStatePtr(void) { return &sTitleState; } /* port_ra_state.c */
#endif
static TaskPool sTitleTaskPool;
static Task* sTitleMenuTask;
static Task* sTitleLogoTask;
static Task* sTitleObjTask;
static u16 sTitleTimer;
static s16 sTitleMenuChoice;
static u8* sTitlePaletteBuffer;
static u16 sTitleBlendStep;
static u8 sTitleCancelled;

void TitleCopyToPaletteBuffer(u16 slot, void* src, u16 size) {
    RequestDma3Copy(src, sTitlePaletteBuffer + slot * 32, size);
}

void TitleLoadPaletteBuffer() {
    LoadPalette(sTitlePaletteBuffer, (void*)PLTT, PLTT_SIZE);
}

void TitleExitToChoice() {
    if (sTitleCancelled) {
        ModeRequest(&gModeTitle, 0);
        return;
    }

    switch (sTitleMenuChoice) {
    case TITLE_MENU_RESUME:
        SaveLoadSystem();
        SaveClearSystem();
        RequestMapMode();
        return;
    case TITLE_MENU_CONTINUE:
        ModeRequest(&gModeMenuLoad, 0);
        return;
    case TITLE_MENU_LINK_BATTLE:
        ClearSioBattleFileLoaded();
        ModeRequest(&gModeSioBattle, SIO_BATTLE_ENTRY_OPEN);
        return;
    case TITLE_MENU_NEW_GAME_SORA:
        SetupSoraNewGame();
        ModeRequest(&gModeMenuNew, 0);
        return;
    case TITLE_MENU_NEW_GAME_RIKU:
        SetupRikuNewGame();
        ModeRequest(&gModeMenuNew, 0);
        return;
    case TITLE_MENU_NEW_GAME:
    default:
        ModeRequest(&gModeMenuNew, 0);
        return;
    }
}

void TitleShowLogo(u16 frames) {
    if ((gGameState.flags & GAME_FLAG_RIKU_TITLE) != 0) {
        LoadBgPalette(1, gTitleRikuBgPalette, sizeof(gTitleRikuBgPalette));
        TitleCopyToPaletteBuffer(0, gTitleRikuBgPalette, sizeof(gTitleRikuBgPalette));
    } else {
        LoadBgPalette(1, gTitleSoraBgPalette, sizeof(gTitleSoraBgPalette));
        TitleCopyToPaletteBuffer(0, gTitleSoraBgPalette, sizeof(gTitleSoraBgPalette));
    }

    EnableBg(0);
    DisableBg(1);
    DisableBg(2);
    TASK_CREATE_IF_INACTIVE(sTitleLogoTask, &sTitleTaskPool, &gTaskDescTitleLogo, NULL);
    TASK_CREATE_IF_INACTIVE(sTitleObjTask, &sTitleTaskPool, &gTaskDescTitleObj, NULL);
    FadeStartIn(FADE_MODE_ADD_WHITE, frames);
}

void TitleFinishIntro() {
#ifdef VERSION_EU
    switch (gLanguage) {
    case LANGUAGE_ENGLISH:
    case LANGUAGE_FRENCH:
    case LANGUAGE_ITALIAN:
    case LANGUAGE_SPANISH:
    case 5:
    case 6:
#endif
        LoadBgMap(0, gTitlePressStartBarMap, sizeof(gTitlePressStartBarMap));
#ifdef VERSION_EU
        break;
    case LANGUAGE_GERMAN:
        LoadBgMap(0, gTitlePressStartBarGermanMap, sizeof(gTitlePressStartBarGermanMap));
        break;
    }
#endif

#ifdef PLATFORM_PC
    PortTitleBands(1);
    PortTitleArtFadeFirst(0); /* BG0 takes the press-start bar */
#endif
    SetBgBlend(0, 5, 16);
    EnableBg(1);
    m4aSongNumStart(SONG_BGM_TITLE);
}

void TitleFadeOut() {
    m4aMPlayFadeOut(gMPlayTable[gSongTable[SONG_BGM_TITLE].ms].info, 5);
    FadeStartOut(FADE_MODE_BLACK, 90);
    BackdropFadeStartOut(0, 90);
    sTitleState = TITLE_STATE_FADE_OUT;
}

#ifdef PLATFORM_PC
void PortUiIntroSkipReset(void);
#endif

void mode_title_0() {
#ifdef PLATFORM_PC
    PortUiIntroSkipReset(); /* the logos' skip prompt, if it was still up */
    PortTitleLogoBg(1);     /* BG2 is the logo: HD with the HD title */
    PortTitleBands(0);
    PortSideStyle(PORT_SIDE_COLOR); /* side bars in the screen's background colour */
#endif
    sTitleCancelled = FALSE;
    ResetGameState();
#ifdef PLATFORM_PC
    /* Riku's illustration already ends in white within the native canvas. */
    PortTitleArtFade(!(gGameState.flags & GAME_FLAG_RIKU_TITLE));
    PortTitleArtFadeFirst(!(gGameState.flags & GAME_FLAG_RIKU_TITLE));
#endif
    SaveLoadHeader();
    InitMapCardInventory();
    ResetSelectedMapCard();
    sTitleMenuChoice = TITLE_MENU_NEW_GAME;
    sTitlePaletteBuffer = EwramAlloc(0x400);
    SetBgMode1();
    SetupBg(0, 0, 0x1D, 0);
    SetBgPriority(0, 3);

    if (gGameState.flags & GAME_FLAG_RIKU_TITLE) {
#ifdef VERSION_EU
        LoadBgMapLz77(0, gTitleRikuIntroBgMap);
#else
        LoadBgMap(0, gTitleRikuIntroBgMap, sizeof(gTitleRikuIntroBgMap));
#endif
    } else {
#ifdef VERSION_EU
        LoadBgMapLz77(0, gTitleSoraIntroBgMap);
#else
        LoadBgMap(0, gTitleSoraIntroBgMap, sizeof(gTitleSoraIntroBgMap));
#endif
    }

    DisableBg(0);
    SetupBg(1, 0, 0x1E, 0);
    SetBgPriority(1, 3);

    if (gGameState.flags & GAME_FLAG_RIKU_TITLE) {
#ifdef VERSION_EU
        LoadBgTilesLz77(1, gTitleRikuBgTiles);
        LoadBgMapLz77(1, gTitleRikuBgMap);
#else
        LoadBgTiles(1, gTitleRikuBgTiles, sizeof(gTitleRikuBgTiles));
        LoadBgMap(1, gTitleRikuBgMap, sizeof(gTitleRikuBgMap));
#endif
    } else {
#ifdef VERSION_EU
        LoadBgTilesLz77(1, gTitleSoraBgTiles);
        LoadBgMapLz77(1, gTitleSoraBgMap);
#else
        LoadBgTiles(1, gTitleSoraBgTiles, sizeof(gTitleSoraBgTiles));
        LoadBgMap(1, gTitleSoraBgMap, sizeof(gTitleSoraBgMap));
#endif
    }

    DisableBg(1);
    BackdropFadeReset();
    BackdropFadeSetColor(0, 0, 0);
    SetupBg(2, 2, 0x1F, 0xB);
    SetBgSize(2, BGCNT_AFF256x256);
    SetBgPriority(2, 2);
    LoadBgTiles(2, gTitleLogoBgTiles, sizeof(gTitleLogoBgTiles));
    LoadBgPalette(2, gTitleLogoBgPalette, sizeof(gTitleLogoBgPalette));
    LoadBgMap(2, gTitleLogoBgMap, 0x400);
    gTitleBgX = 0x7800;
    gTitleBgY = 0x5A00;
    gTitleBgScale = 0x1000;
    SetBgAffine(2, 0, gTitleBgScale >> 4, gTitleBgScale >> 4, gTitleBgX, gTitleBgY);
    TaskPoolInit(&sTitleTaskPool, 4);
    sTitleLogoTask = NULL;
    sTitleObjTask = NULL;
    FadeStartIn(FADE_MODE_BLACK, 0x4C);
    sTitleState = TITLE_STATE_FADE_IN;
    m4aSongNumStart(SONG_SND_0);
    sTitleTimer = 0x1E;
}

#ifdef PLATFORM_PC
void PortUiTitleHint(void);
void PortUiOpenOptions(void);
int PortUiNeedsModeChoice(void);
void PortUiOpenModeChoice(void);
#endif

void mode_title_1() {
#ifdef PLATFORM_PC
    /* the first start: the graphics mode (HD, GBA enhanced, GBA classic) before anything else */
    if (PortUiNeedsModeChoice())
        PortUiOpenModeChoice();
    /* PC options: offered on the "press start" and menu screens. */
    if (sTitleState == TITLE_STATE_PRESS_START || sTitleState == TITLE_STATE_MENU) {
        PortUiTitleHint();
        if (GetKeysPressed() & SELECT_BUTTON)
            PortUiOpenOptions();
    }
#endif
    switch (sTitleState) {
    case TITLE_STATE_FADE_IN:
        if (FadeIsActive()) {
            break;
        }

        if (sTitleTimer != 0) {
            sTitleTimer--;

            if (sTitleTimer != 0) {
                break;
            }
        }

        sTitleState = TITLE_STATE_ZOOM;
        sTitleTimer = 100;
        break;
    case TITLE_STATE_ZOOM:
        if (gGameState.flags & GAME_FLAG_RIKU_TITLE) {
            ApproachValue(&gTitleBgX, 0x3F3F, sTitleTimer);
        } else {
            ApproachValue(&gTitleBgX, 0xB0C1, sTitleTimer);
        }

        ApproachValue(&gTitleBgY, 0x6A37, sTitleTimer);
        ApproachValue(&gTitleBgScale, 0xBD0, sTitleTimer);
        SetBgAffine(2, 0, gTitleBgScale / 16, gTitleBgScale / 16, gTitleBgX, gTitleBgY);
        sTitleTimer--;

        if (sTitleTimer == 0x46) {
            BackdropFadeStartOut(1, 0x46);

            if (sTitleTimer == 0x46) {
                FadeStartOut(FADE_MODE_WHITE, 0x46);
            }
        }

        if (sTitleTimer == 0) {
            sTitleState = TITLE_STATE_WHITE_OUT;
            sTitleTimer = 2;
        }

        break;
    case TITLE_STATE_WHITE_OUT:
        if (FadeIsActive()) {
            break;
        }

        sTitleTimer--;

        if (sTitleTimer != 0) {
            break;
        }

        TitleShowLogo(15);
        sTitleState = TITLE_STATE_LOGO_IN;
        sTitleTimer = 0x28;
        break;
    case TITLE_STATE_LOGO_IN:
        if (FadeIsActive()) {
            break;
        }

        if (sTitleTimer != 0) {
            sTitleTimer--;
            break;
        }

        sTitleState = TITLE_STATE_CROSSFADE_BG;
        sTitleBlendStep = 0;
        gBldCnt = (BLDCNT_TGT1_BG0 | BLDCNT_TGT1_BG1 | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG0 | BLDCNT_TGT2_BG1 | BLDCNT_TGT2_OBJ);
        gBldAlpha = BLDALPHA_BLEND(16, 0);
        sTitleTimer = 4;
        EnableBg(1);
        break;
    case TITLE_STATE_CROSSFADE_BG:
        if (sTitleTimer != 0) {
            sTitleTimer--;
            break;
        }

        sTitleTimer = 4;
        sTitleBlendStep++;
        gBldAlpha = (sTitleBlendStep << 8) | (16 - sTitleBlendStep);

        if (sTitleBlendStep > 15) {
            gBldCnt = 0;
            TitleFinishIntro();
            sTitleState = TITLE_STATE_PRESS_START;
        }

        break;
    case TITLE_STATE_PRESS_START:
        if (!IsTitleObjSlideDone()) {
            break;
        }

        if (!(GetKeysPressed() & START_BUTTON) && !(GetKeysPressed() & A_BUTTON)) {
            break;
        }

        m4aSongNumStart(SONG_SYS_KETTEI);

        if (SaveRepairSystem() == SAVE_OK) {
            sTitleMenuChoice = TITLE_MENU_RESUME;
        } else if (SaveRepairFileLarge(0) == SAVE_OK || SaveRepairFileLarge(1) == SAVE_OK) {
            sTitleMenuChoice = TITLE_MENU_CONTINUE;
        } else if (
#ifdef PLATFORM_PC
                   PortRikuAvailable(gGameState.flags & GAME_FLAG_SORA_CLEAR) &&
#else
                   (gGameState.flags & GAME_FLAG_SORA_CLEAR) &&
#endif
                   /* Existing Riku saves stay stored, but require access. */
                   (SaveRepairFileSmall(0) == SAVE_OK || SaveRepairFileSmall(1) == SAVE_OK)) {
            sTitleMenuChoice = TITLE_MENU_CONTINUE;
        } else {
            sTitleMenuChoice = TITLE_MENU_NEW_GAME;
        }

        TaskKill(&sTitleTaskPool, sTitleLogoTask);
        TaskKill(&sTitleTaskPool, sTitleObjTask);
        sTitleMenuTask = TaskCreate(&sTitleTaskPool, &gTaskDescTitleMenu, &sTitleMenuChoice);
        DisableBg(0);
        sTitleState = TITLE_STATE_MENU_IN;
        sTitleBlendStep = 0;
        gBldCnt = (BLDCNT_TGT1_OBJ | BLDCNT_EFFECT_BLEND | BLDCNT_TGT2_BG1);
        gBldAlpha = ((16 - sTitleBlendStep) << 8) | sTitleBlendStep;
        sTitleTimer = 4;
        break;
    case TITLE_STATE_MENU_IN:
        if (sTitleTimer != 0) {
            sTitleTimer--;
            break;
        }

        sTitleTimer = 4;
        sTitleBlendStep++;
        gBldAlpha = ((16 - sTitleBlendStep) << 8) | sTitleBlendStep;

        if (sTitleBlendStep > 15) {
            gBldCnt = 0;
            sTitleState = TITLE_STATE_MENU;
        }

        break;
    case TITLE_STATE_MENU_OUT:
        if (sTitleTimer != 0) {
            sTitleTimer--;
            break;
        }

        sTitleTimer = 1;
        sTitleBlendStep--;
        gBldAlpha = ((16 - sTitleBlendStep) << 8) | sTitleBlendStep;

        if (sTitleBlendStep > 15) {
            gBldCnt = 0;
            TaskKill(&sTitleTaskPool, sTitleMenuTask);
            sTitleLogoTask = TaskCreate(&sTitleTaskPool, &gTaskDescTitleLogo, NULL);
            sTitleObjTask = TaskCreate(&sTitleTaskPool, &gTaskDescTitleObj, NULL);
            EnableBg(0);
            sTitleState = TITLE_STATE_PRESS_START;
        }

        break;
    case TITLE_STATE_MENU:
        if ((GetKeysPressed() & START_BUTTON) || (GetKeysPressed() & A_BUTTON)) {
            switch (sTitleMenuChoice) {
            case TITLE_MENU_NEW_GAME:
            case TITLE_MENU_NEW_GAME_SORA:
            case TITLE_MENU_NEW_GAME_RIKU:
                m4aSongNumStart(SONG_SYS_START);
                break;
            default:
                m4aSongNumStart(SONG_SYS_KETTEI);
                break;
            }

            TitleFadeOut();
        } else if (GetKeysPressed() & B_BUTTON) {
            m4aSongNumStart(SONG_SYS_CLOSE);
            sTitleCancelled = TRUE;
            TitleFadeOut();
        }

        break;
    case TITLE_STATE_FADE_OUT:
        if (!FadeIsActive()) {
            TitleExitToChoice();
        }

        break;
    }

    if (!FadeIsActive() && sTitleState != TITLE_STATE_MENU_IN) {
        TaskPoolUpdate(&sTitleTaskPool);
    }

    TaskPoolDraw(&sTitleTaskPool);
    BackdropFadeUpdate();

    if (sTitleState <= TITLE_STATE_CROSSFADE_BG && (GetKeysPressed() & (A_BUTTON | START_BUTTON))) {
        m4aSongNumStart(SONG_SYS_CLICK);

        if (sTitleState <= TITLE_STATE_LOGO_IN) {
            TitleShowLogo(2);
        }

        TitleFinishIntro();
        sTitleState = TITLE_STATE_PRESS_START;
    }
}

void mode_title_2() {
#ifdef PLATFORM_PC
    PortTitleLogoBg(0);
    PortTitleArtFade(0);
    PortTitleArtFadeFirst(0);
    PortTitleBands(0);
#endif
    TaskPoolDestroy(&sTitleTaskPool);
    REG_IME = 0;
    REG_IE &= ~INTR_FLAG_VCOUNT;
    REG_DISPSTAT &= ~DISPSTAT_VCOUNT_INTR;
    REG_IME = 1;
    ResetVCountCallback();
    EwramFree(sTitlePaletteBuffer);
}

u8 IsTitleLogoShown() {
    if (sTitleState > TITLE_STATE_WHITE_OUT) {
        return TRUE;
    }

    return FALSE;
}

u8 IsTitleIntroDone() {
    if (sTitleState > TITLE_STATE_CROSSFADE_BG) {
        return TRUE;
    }

    return FALSE;
}

Mode gModeTitle = {
    "mode_title",
    (ModeInitFunc)mode_title_0,
    mode_title_1,
    mode_title_2,
};
