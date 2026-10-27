/**
 * mode.c
 * Game Mode Manager
 */

#ifdef PLATFORM_PC
#include "engine.h"
#endif
#include "registration_data.h"
#include "chara_api.h"
#include "display.h"
#include "m4a_song.h"
#include "sio_api.h"
#include "obj_api.h"
#include "fade.h"
#include "gba/syscall.h"
#include "key.h"
#include "malloc.h"
#include "main.h"
#include "mode.h"
#include "gba/keys.h"
#include "sroll_api.h"
#include "gba/defines.h"
#include "gba/io_reg.h"
#include "system_state.h"
#include <stddef.h>
#include "types.h"

Mode* gCurrentMode;
void (*gCurrentModeUpdate)();
u16 gDebugModeIndex;
Mode* gPendingMode;
s32 gPendingModeArg;
vu8 gModeFlags;
u16 gModeBlankColor;
void (*gModeTransitionCallback)();
void (*gModeVBlankCallback)();

#ifdef VERSION_EU
u32 gUnkEu_030074AC;
#endif

static Mode* sDebugModes[] = {
    &gModeDebug,
    &gModeJiminy,
#ifndef VERSION_EU
    &gModeDeckExchange,
#endif
    &gModePremire,
    &gModeWORLDSELECT,
    &gModeDeck,
    &gModeRikuBtlTutorial,
    &gModeRikuDeckTutorial,
#ifdef VERSION_EU
    &gModeTextCheck,
#endif
    &gModeWLogo,
    &gModeSioBattle,
    &gModeSioBtlConnect,
    &gModeSioBtlOption,
    &gModeSioBtlCardget,
    &gModeSioError,
#ifndef VERSION_EU
    &gModeSioChgConnect,
    &gModeSioChgCard,
#endif
    &gModeSioDbgFlg,
    &gModeTitle,
    &gModeCopyright1,
    &gModeCopyright2,
    &gModeMapChk,
    &gModeWorldselect,
    &gModeWorldinspect,
    &gModeWorldwarp,
    &gModeMsTop,
    &gModeMapinspect,
};

void ModeBlankDisplay() {
    REG_DISPCNT &= ~(DISPCNT_BG_ALL_ON | DISPCNT_OBJ_ON);
    *(vu16*)PLTT = gModeBlankColor;
}

#ifdef PLATFORM_PC
extern Mode gModeDeck, gModeStatus, gModeJiminy, gModeAllmap, gModeMapinspect, gModeWorldinspect,
    gModeWorldselect, gModeWORLDSELECT, gModeWorldwarp, gModeMsTop, gModeMsShop, gModeMsCharge;

int PortMoogleMenu(void) {
    /* Distinct profiles: the charge screen has tabs reaching its right edge. */
    return gCurrentMode == &gModeMsTop || gCurrentMode == &gModeMsShop ? 1 :
           gCurrentMode == &gModeMsCharge ? 2 : 0;
}

/* Entry styling and live edge composition must classify the same modes. */
static int ModeUsesMenuBackground(Mode* mode) {
    /* menus whose own BG panels reach the edge: sprites there (NEW badges,
       the pointing glove) must never be carried into the margin */
    return mode == &gModeStatus || mode == &gModeMapinspect || mode == &gModeDeck
        /* The entrance already draws its BG bands while the title slides in.
           Classify it like the shop: foreground/fade differences at the edge
           must not hide those bands until the generic sliding timer expires. */
        || mode == &gModeMsTop || mode == &gModeMsShop || mode == &gModeMsCharge;
}

/* Derive this from the live mode so loading a state needs no extra flag. */
int PortMenuBackgroundOnly(void) {
    return ModeUsesMenuBackground(gCurrentMode);
}

/* The menu screens drawn over the whole GBA picture: their backgrounds end
   at its edges, so they keep the GBA framing in the wide view. */
static int ModeIsFullScreenMenu(const Mode* mode) {
    return mode == &gModeDeck || mode == &gModeStatus || mode == &gModeJiminy || mode == &gModeAllmap
        || mode == &gModeMapinspect || mode == &gModeWorldinspect || mode == &gModeWorldselect
        || mode == &gModeWORLDSELECT || mode == &gModeWorldwarp || mode == &gModeMsTop
        || mode == &gModeMsShop || mode == &gModeMsCharge
        || mode == &gModeMenuNew || mode == &gModeMenuLoad;
}
#endif

void ModeStart(Mode* mode, s32 arg) {
    gModeBlankColor = FadeGetColor();
#ifdef PLATFORM_PC
    /* The native renderer runs before the next VBlank. Blank now, before
       the new mode reuses VRAM/palettes, rather than rendering released
       scenery for one frame with the new mode's palette. */
    ModeBlankDisplay();
#endif
    VTransReset();
    BgReset();
    SpriteReset();
    FadeReset();
    MosaicReset();
    gCurrentMode = mode;

#ifdef PLATFORM_PC
    PortMenuPanelSide(-1);
    PortMenuHeaderSource(NULL, NULL, NULL);
    PortSideStyle(mode == &gModeAllmap ? PORT_SIDE_PATTERN :
                  ModeUsesMenuBackground(mode)
                      ? PORT_SIDE_MENU :
                  /* live backgrounds: the blurred reflection, with the bars carried on */
                  (mode == &gModeWORLDSELECT || mode == &gModeWorldwarp) ? PORT_SIDE_BLUR_BANDS
                  /* the journal: green paper, its title and footer bars carried on */
                  : mode == &gModeJiminy ? PORT_SIDE_GROUND_BANDS
                  /* the save files (new game, load): their scene and their bars */
                  : (mode == &gModeMenuNew || mode == &gModeMenuLoad) ? PORT_SIDE_SCENE_BANDS
                  : PORT_SIDE_EDGE_FADE); /* no stretched columns anywhere */
    PortSceneNarrow(ModeIsFullScreenMenu(mode));
    PortNarrowObjBands(mode == &gModeWorldselect);
    {
        /* the logos' "press again to skip" prompt never outlives them */
        extern Mode gModeCopyright1, gModeCopyright2;
        extern void PortUiIntroSkipReset(void);
        if (mode != &gModeCopyright1 && mode != &gModeCopyright2)
            PortUiIntroSkipReset();
    }
    {
        /* the battle's opening (the heart bursting) covers the whole screen */
        extern Mode gModeBattle;
        extern void PortCoverIntro(void);
        if (mode == &gModeBattle)
            PortCoverIntro();
    }
#endif
    if (mode->init != NULL) {
        mode->init(arg);
    }

    gCurrentModeUpdate = gCurrentMode->update;
    gModeFlags |= MODE_FLAG_STARTED;
}

#ifdef VERSION_EU
void ModeInit(u8 softReset) {
#else
void ModeInit() {
#endif
    gModeFlags = (MODE_FLAG_BLANK_PENDING | MODE_FLAG_DISPLAY_HELD);
    gModeBlankColor = 0;
    gDebugModeIndex = 0;

#ifdef VERSION_EU
    if (softReset) {
        ModeStart(&gModeCopyright1, 0);
    } else {
        ModeStart(&gModeLang, 0);
    }
#else
    ModeStart(&gModeCopyright1, 0);
#endif
    gPendingMode = NULL;
    gModeTransitionCallback = NULL;
    gModeVBlankCallback = NULL;
}

void ModeSetTransitionCallback(void (*init)(), void (*update)()) {
    if (init != NULL) {
        init();
    }

    gModeTransitionCallback = update;
    gModeFlags |= MODE_FLAG_TRANSITION_ACTIVE;
}

void ModeClearTransitionCallback() {
    gModeFlags &= ~MODE_FLAG_TRANSITION_ACTIVE;
    gModeTransitionCallback = NULL;
}

void ModeSetVBlankCallback(void (*fn)()) {
    gModeVBlankCallback = fn;
}

void ModeClearVBlankCallback() {
    gModeVBlankCallback = NULL;
}

u8 IsModeStarted() {
    if (gModeFlags & MODE_FLAG_STARTED) {
        return TRUE;
    }

    return FALSE;
}

void ModeRequest(Mode* mode, s32 arg) {
    gPendingMode = mode;
    gPendingModeArg = arg;
}

void ModeRequestHeapReset(Mode* mode, s32 arg) {
    gPendingMode = mode;
    gPendingModeArg = arg;
    gModeFlags |= MODE_FLAG_HEAP_RESET;
}

#ifdef VERSION_EU
void DoSoftReset() {
    gSoftResetMarker[0] = SOFT_RESET_MAGIC;
    SoftReset(RESET_ALL & ~RESET_IWRAM);
}
#endif

void ModeUpdate() {
    u8 displayHeld;

    if ((((GetKeysPressed() & START_BUTTON) && (GetKeysHeld() & SELECT_BUTTON) && (GetKeysHeld() & A_BUTTON) &&
             (GetKeysHeld() & B_BUTTON)) ||
            ((GetKeysHeld() & START_BUTTON) && (GetKeysPressed() & SELECT_BUTTON) && (GetKeysHeld() & A_BUTTON) &&
                (GetKeysHeld() & B_BUTTON))) &&
        !(gSystemFlags & SYSTEM_FLAG_NO_SOFT_RESET)) {
        if (SioIsConnected()) {
            SioLinkClose();
        }

        m4aMPlayAllStop();
#ifdef VERSION_EU
        ScanlineDmaReset();
        DoSoftReset();
#else
        SoftReset(RESET_ALL);
        ScanlineDmaReset();
#endif
    } else {
        if (gModeFlags & MODE_FLAG_TRANSITION_ACTIVE) {
            return;
        }

        displayHeld = gModeFlags & MODE_FLAG_DISPLAY_HELD;

        if (displayHeld != 0) {
            if (gModeFlags & MODE_FLAG_BLANK_PENDING) {
                return;
            }

            FadeUpdate();
            FlushDma3QueueWithCpu();
            gModeFlags &= ~MODE_FLAG_DISPLAY_HELD;
        } else if (gPendingMode != NULL) {
            if (gCurrentMode->exit != NULL) {
                gCurrentMode->exit();
            }

            if (gModeFlags & MODE_FLAG_HEAP_RESET) {
                gModeFlags &= ~MODE_FLAG_HEAP_RESET;
                EwramHeapInit(GetEwramHeapStart(), GetEwramHeapSize());
            }

            gModeFlags = (MODE_FLAG_BLANK_PENDING | MODE_FLAG_DISPLAY_HELD);
            ModeStart(gPendingMode, gPendingModeArg);
            gPendingMode = NULL;
        } else {
            if (gCurrentModeUpdate != NULL) {
                gCurrentModeUpdate();
            }

            FadeUpdate();
            MosaicUpdate();
            SortSprites();
        }
    }
}

void SetModeUpdate(void (*fn)()) {
    gCurrentModeUpdate = fn;
}

void ModeFlushDisplay() {
    if (gModeFlags & MODE_FLAG_BLANK_PENDING) {
        ModeBlankDisplay();
        gModeFlags &= ~MODE_FLAG_BLANK_PENDING;
    }

    if (!(gModeFlags & MODE_FLAG_DISPLAY_HELD)) {
        if (gSystemFlags & SYSTEM_FLAG_DMA3_FLUSH_CPU) {
            FlushDma3QueueWithCpu();
        } else {
            FlushDma3Queue();
        }

        UpdateSpriteOam();
        CommitDisplayRegs();
    }
}

void ModeRunVBlankCallbacks() {
    if ((gModeFlags & MODE_FLAG_DISPLAY_HELD) && gModeTransitionCallback != NULL) {
        gModeTransitionCallback();
    }

    if (gModeVBlankCallback != NULL) {
        gModeVBlankCallback();
    }
}

void ModeCallExit() {
    if (gCurrentMode->exit != NULL) {
        gCurrentMode->exit();
    }
}

const char* GetModeName() {
    return gCurrentMode->name;
}

void UpdateDebugModeSelect() {
    if (GetKeysHeld() & SELECT_BUTTON) {
        if (GetKeysPressed() & L_BUTTON) {
            gDebugModeIndex--;

            if ((s16)gDebugModeIndex < 0) {
#ifdef VERSION_EU
                gDebugModeIndex = 23;
#else
                gDebugModeIndex = 25;
#endif
            }

            ModeRequest(sDebugModes[(s16)gDebugModeIndex], 0);
        }

        if (GetKeysPressed() & R_BUTTON) {
            gDebugModeIndex++;

#ifdef VERSION_EU
            if (gDebugModeIndex > 23) {
#else
            if (gDebugModeIndex > 25) {
#endif
                gDebugModeIndex = 0;
            }

            ModeRequest(sDebugModes[(s16)gDebugModeIndex], 0);
        }
    }
}
