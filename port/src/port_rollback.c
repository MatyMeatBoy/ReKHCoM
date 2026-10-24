/*
 * Rollback for the network link (src/sio/sio.c, port/progress/ONLINE_ESTUDIO.md).
 *
 * The link plays the other player's packet `delay` frames late and guesses it when it is later than that, so the
 * game never waits for the network. A guess that turns out wrong is taken back here: at the main loop's wait for
 * the vertical blank (the one place the native stack is always the same, as for save states) every frame of the
 * link session leaves a snapshot of the game's memory and the emulated hardware in a ring; the frame the wrong
 * guess spoiled is restored and the game plays the frames from there to now again, with the keys it had, without
 * drawing, sounding or pacing them (port_main.c VBlankCycle), all inside the current real frame.
 *
 * Snapshots are kept by "timeline frame" (counted at every wait of the session and taken back with the memory) and
 * tagged with the link frame the game is about to play.
 */
#include <SDL3/SDL.h>
#include <stdlib.h>
#include <string.h>

#include "port/port.h"
#include "port/port_state.h"
#include "port/port_netlink.h"

#define RB_FRAMES 32

int NetRollbackSession(void);
uint32_t NetLinkFrame(void);
uint32_t NetLinkMade(void);
uint32_t NetTakeRollback(void);

typedef struct {
    uint32_t base;
    uint32_t size;
} Region;

static const Region sRegions[] = {
    { GBA_EWRAM, GBA_EWRAM_SIZE }, { GBA_IWRAM, GBA_IWRAM_SIZE }, { GBA_IO, GBA_IO_SIZE },
    { GBA_PLTT, GBA_PLTT_SIZE },   { GBA_VRAM, GBA_VRAM_SIZE },   { GBA_OAM, GBA_OAM_SIZE },
};

static uint8_t* sSnap[RB_FRAMES];
static uint32_t sSnapTimeline[RB_FRAMES], sSnapLink[RB_FRAMES];
static uint8_t sSnapValid[RB_FRAMES];
static uint16_t sKeys[RB_FRAMES]; /* the keys each timeline frame played with */
static size_t sSnapSize;
static uint8_t* sCursor;
static uint32_t sTimeline; /* part of the snapshot: taken back with the game */
static int sResim, sSession, sFailed;
static uint32_t sReplayed, sLastDepth;

/* port_ui.c's network line: how many frames the last rollback played again */
int PortRollbackLastDepth(void) {
    return (int)sLastDepth;
}

/* Tests (KHCOM_ROLLBACK_TEST=<n>): every n link frames the game is rolled back 6 frames although no guess was wrong,
   and every frame played again is compared with what it was the first time: the first byte that differs is logged
   with its region (data/bss offsets: match them with the exe's symbols), so state the snapshots miss shows up. */
static int sTestEvery = -1;
static uint8_t* sOld[RB_FRAMES];
static int sOldValid[RB_FRAMES];
static const char* sVisitName;
static size_t sVisitOffset;
static int sDiffLogged;

static void VisitCompare(void* p, size_t n) {
    size_t i;
    const uint8_t* a = sCursor;
    const uint8_t* b = (const uint8_t*)p;
    if (sDiffLogged < 12 && strcmp(sVisitName, "audio") != 0) /* the mixer is not played again (no sound) */
        for (i = 0; i < n; i++)
            if (a[i] != b[i]) {
                PortLog("rollback test: frame played again differs in %s+0x%x (%u bytes in), was %02x now %02x",
                        sVisitName, (unsigned)i, (unsigned)(sVisitOffset + i), a[i], b[i]);
                sDiffLogged++;
                break;
            }
    sCursor += n;
    sVisitOffset += n;
}

static void VisitSize(void* p, size_t n) {
    (void)p;
    sSnapSize += n;
}
static void VisitSave(void* p, size_t n) {
    memcpy(sCursor, p, n);
    sCursor += n;
}
static void VisitLoad(void* p, size_t n) {
    memcpy(p, sCursor, n);
    sCursor += n;
}

static void VisitAll(PortStateVisitor visit) {
    static const char* const names[] = { "ewram", "iwram", "io", "pltt", "vram", "oam" };
    size_t i;
    for (i = 0; i < sizeof(sRegions) / sizeof(sRegions[0]); i++) {
        sVisitName = names[i];
        visit((void*)(uintptr_t)sRegions[i].base, sRegions[i].size);
    }
    {
        char *data, *bss;
        size_t dataSize, bssSize;
        PortGameSections(&data, &dataSize, &bss, &bssSize);
        sVisitName = "data";
        visit(data, dataSize);
        sVisitName = "bss";
        visit(bss, bssSize);
    }
    sVisitName = "dma";
    PortDmaVisitState(visit);
    sVisitName = "ppu";
    PortPpuVisitState(visit);
    sVisitName = "audio";
    PortAudioVisitState(visit);
    sVisitName = "timeline";
    visit(&sTimeline, sizeof(sTimeline));
}

static int Prepare(void) {
    int i;
    if (sSnap[0])
        return 1;
    if (sFailed)
        return 0;
    sSnapSize = 0;
    VisitAll(VisitSize);
    for (i = 0; i < RB_FRAMES; i++) {
        sSnap[i] = (uint8_t*)malloc(sSnapSize);
        if (!sSnap[i]) {
            sFailed = 1;
            PortLog("rollback: no memory for %d snapshots of %u bytes: the link waits instead", RB_FRAMES,
                    (unsigned)sSnapSize);
            while (i-- > 0) {
                free(sSnap[i]);
                sSnap[i] = NULL;
            }
            return 0;
        }
    }
    PortLog("rollback: %d snapshots of %u KB", RB_FRAMES, (unsigned)(sSnapSize / 1024));
    return 1;
}

int PortRollbackResimulating(void) {
    return sResim;
}

/* The game's frame is done and the next one's keys are in KEYINPUT: take back a wrong guess, keep this frame. */
void PortRollbackBoundary(int inMainLoop) {
    uint32_t to, link;
    int i, slot;

    if (!NetRollbackSession()) {
        if (sSession) {
            PortLog("rollback: link session over, %u frames played again", (unsigned)sReplayed);
            sSession = 0;
            sResim = 0;
            memset(sSnapValid, 0, sizeof(sSnapValid));
        }
        return;
    }
    if (!sSession) {
        sSession = 1;
        sTimeline = 0;
        memset(sSnapValid, 0, sizeof(sSnapValid));
    }
    if (!Prepare())
        return;
    to = inMainLoop ? NetTakeRollback() : 0xFFFFFFFF; /* elsewhere the native stack is not the snapshots' */
    if (sTestEvery < 0)
        sTestEvery = SDL_getenv("KHCOM_ROLLBACK_TEST") ? SDL_atoi(SDL_getenv("KHCOM_ROLLBACK_TEST")) : 0;
    if (sTestEvery > 0 && inMainLoop && to == 0xFFFFFFFF && !sResim && NetLinkFrame() > 8 &&
        NetLinkFrame() % (uint32_t)sTestEvery == 0) {
        to = NetLinkFrame() - 6;
        for (i = 0; i < RB_FRAMES; i++) { /* what the frames were, to compare while playing them again */
            if (!sOld[i])
                sOld[i] = (uint8_t*)malloc(sSnapSize);
            sOldValid[i] = sOld[i] && sSnapValid[i];
            if (sOldValid[i])
                memcpy(sOld[i], sSnap[i], sSnapSize);
        }
    }
    if (to != 0xFFFFFFFF) {
        /* the latest snapshot from before link frame `to` was played */
        int best = -1;
        for (i = 0; i < RB_FRAMES; i++)
            if (sSnapValid[i] && sSnapLink[i] <= to && (best < 0 || sSnapTimeline[i] > sSnapTimeline[best]))
                best = i;
        if (best < 0) {
            PortLog("rollback: link frame %u is older than the snapshots: not taken back", (unsigned)to);
        } else {
            uint32_t from = sTimeline;
            sCursor = sSnap[best];
            VisitAll(VisitLoad);
            for (i = 0; i < RB_FRAMES; i++) /* the frames after it are being played again */
                if (sSnapValid[i] && sSnapTimeline[i] > sTimeline)
                    sSnapValid[i] = 0;
            sResim = 1;
            sReplayed += from - sTimeline;
            sLastDepth = from - sTimeline;
            if (PortNetlinkTrace())
                PortLog("rollback: link frame %u, %u frames again", (unsigned)to, (unsigned)(from - sTimeline));
        }
    }
    link = NetLinkFrame();
    if (sResim && link >= NetLinkMade())
        sResim = 0; /* caught up: from here the frames are new */
    slot = (int)(sTimeline % RB_FRAMES);
    if (sResim)
        IO16(0x130) = sKeys[slot]; /* the keys this frame had the first time */
    else
        sKeys[slot] = IO16(0x130);
    if (inMainLoop) {
        if (sResim && sTestEvery > 0 && sOldValid[slot] && sSnapTimeline[slot] == sTimeline) {
            uint32_t keep = sTimeline;
            sCursor = sOld[slot];
            sVisitOffset = 0;
            VisitAll(VisitCompare);
            sTimeline = keep;
        }
        sCursor = sSnap[slot];
        VisitAll(VisitSave);
        sSnapTimeline[slot] = sTimeline;
        sSnapLink[slot] = link;
        sSnapValid[slot] = 1;
    } else {
        sSnapValid[slot] = 0;
    }
    sTimeline++;
}
