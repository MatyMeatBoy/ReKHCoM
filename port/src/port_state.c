/*
 * Save states.
 *
 * A state is taken and restored at one fixed point: the main loop's wait for
 * the vertical blank, where the native call stack is always the same
 * (main -> AgbMain -> VBlankIntrWait). It holds the GBA memory regions, the
 * game's globals (the .data/.bss group from port/game_prefix.h) and the
 * platform layer's own hardware state. The executable has a fixed image base,
 * so every pointer inside the state stays valid for the same build.
 *
 * SRAM is deliberately not part of a state: loading one never touches the
 * player's real save file.
 */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define ImageStart() ((const uint8_t*)GetModuleHandleA(NULL) + 0x1000)
#define MakeDir(path) CreateDirectoryA(path, NULL)
#else
/* Linux: the executable is linked at a fixed base (no PIE), like Windows. */
#include <stdint.h>
#include <sys/stat.h>
#define MAX_PATH 1024
extern const uint8_t __executable_start[];
#define ImageStart() (__executable_start)
#define MakeDir(path) mkdir(path, 0755)
#endif
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "port/port.h"
#include "port/port_state.h"
#include "port/port_ui.h"

extern void AgbMain(void);

#define STATE_MAGIC 0x5453484Bu /* "KHST" */
#define STATE_VERSION 1

typedef struct Region {
    uint32_t base;
    uint32_t size;
} Region;

static const Region sRegions[] = {
    { GBA_EWRAM, GBA_EWRAM_SIZE }, { GBA_IWRAM, GBA_IWRAM_SIZE }, { GBA_IO, GBA_IO_SIZE },
    { GBA_PLTT, GBA_PLTT_SIZE },   { GBA_VRAM, GBA_VRAM_SIZE },   { GBA_OAM, GBA_OAM_SIZE },
};

int gPortInMainLoop;
int gPortStateSlot = 1;
static int sPendingSave, sPendingLoad;
static int sQuiet, sLastResult; /* PortStateRequestQuiet: no notice; the result kept */
static char sPendingPath[MAX_PATH];
static FILE* sFile;
static int sOk;

/* States hold raw pointers into the game's code and data, so they are only
   valid for an identical game image: hash the game's code (linked before
   the platform layer, which may change freely) and its globals' layout. */
static uint32_t BuildId(void) {
    static uint32_t sId;
    char *data, *bss;
    size_t dataSize, bssSize;
    const uint8_t* p;
    const uint8_t* end = (const uint8_t*)(uintptr_t)PortStateService;
    uint32_t h = 2166136261u;

    if (sId)
        return sId;
    PortGameSections(&data, &dataSize, &bss, &bssSize);
    for (p = ImageStart(); p < end; p++)
        h = (h ^ *p) * 16777619u;
    h ^= (uint32_t)(uintptr_t)data ^ (uint32_t)dataSize * 31u ^ (uint32_t)(uintptr_t)bss ^ (uint32_t)bssSize * 7u;
    sId = h ^ STATE_VERSION;
    return sId;
}

static void VisitWrite(void* p, size_t n) {
    if (sOk && fwrite(p, 1, n, sFile) != n)
        sOk = 0;
}

/* A bounded staging allocation keeps damaged files away from live state.
   64 MiB exceeds the current GBA/platform snapshot and is an explicit budget. */
#define STATE_MAX_PAYLOAD (64u * 1024u * 1024u)
static size_t sPayloadSize, sPayloadOffset;
static unsigned char* sPayload;
static void VisitCount(void* p, size_t n) {
    (void)p;
    if (n > STATE_MAX_PAYLOAD - sPayloadSize) sOk = 0;
    else sPayloadSize += n;
}
static void VisitApply(void* p, size_t n) {
    memcpy(p, sPayload + sPayloadOffset, n);
    sPayloadOffset += n;
}

static void VisitAll(PortStateVisitor visit) {
    size_t i;

    for (i = 0; i < sizeof(sRegions) / sizeof(sRegions[0]); i++)
        visit((void*)(uintptr_t)sRegions[i].base, sRegions[i].size);
    {
        char *data, *bss;
        size_t dataSize, bssSize;
        PortGameSections(&data, &dataSize, &bss, &bssSize);
        visit(data, dataSize);
        visit(bss, bssSize);
    }
    PortDmaVisitState(visit);
    PortPpuVisitState(visit);
    PortAudioVisitState(visit);
}

const char* PortStatePath(int slot) {
    static char path[MAX_PATH];

    MakeDir(PortConfigPath("states"));
    MakeDir(PortConfigPath("states/native32"));
    SDL_snprintf(path, sizeof(path), "%s", PortConfigPath("states/native32/slot0.khst"));
    path[strlen(path) - 6] = (char)('0' + slot % 10);
    return path;
}

static void MakeParentDir(const char* path) {
    char dir[MAX_PATH];
    char* slash;

    SDL_strlcpy(dir, path, sizeof(dir));
    slash = SDL_strrchr(dir, '/');
    if (SDL_strrchr(dir, '\\') > slash)
        slash = SDL_strrchr(dir, '\\');
    if (slash != NULL) {
        *slash = 0;
        MakeDir(dir);
    }
}

/* Config paths use the platform CRT encoding, as fopen does. */
static int ReplaceState(const char* from, const char* to) {
#ifdef _WIN32
    return MoveFileExA(from, to, MOVEFILE_REPLACE_EXISTING) ? 0 : -1;
#else
    return rename(from, to);
#endif
}

static int Save(const char* path) {
    uint32_t header[3] = { STATE_MAGIC, STATE_VERSION, BuildId() };
    char temporary[MAX_PATH];
    if (SDL_snprintf(temporary, sizeof(temporary), "%s.tmp", path) >= (int)sizeof(temporary))
        return 0;
    MakeParentDir(path);
    sFile = fopen(temporary, "wb");
    if (sFile == NULL) return 0;
    sOk = 1;
    VisitWrite(header, sizeof(header));
    VisitAll(VisitWrite);
    if (fclose(sFile) != 0) sOk = 0;
    if (sOk && ReplaceState(temporary, path) != 0) sOk = 0;
    if (!sOk) remove(temporary);
    return sOk;
}

static int Load(const char* path) {
    uint32_t header[3];
    sFile = fopen(path, "rb");
    if (sFile == NULL) return 0;
    if (fread(header, 1, sizeof(header), sFile) != sizeof(header)) {
        fclose(sFile);
        return 0;
    }
    if (header[0] != STATE_MAGIC || header[1] != STATE_VERSION || header[2] != BuildId()) {
        fclose(sFile);
        PortLog("state: %s is from another build", path);
        return -1;
    }
    sOk = 1;
    sPayloadSize = 0;
    VisitAll(VisitCount);
    sPayload = sOk ? malloc(sPayloadSize ? sPayloadSize : 1) : NULL;
    if (sPayload == NULL) { fclose(sFile); return 0; }
    sOk = fread(sPayload, 1, sPayloadSize, sFile) == sPayloadSize;
    if (sOk && fgetc(sFile) != EOF) sOk = 0;
    if (ferror(sFile)) sOk = 0;
    if (fclose(sFile) != 0) sOk = 0;
    if (!sOk) { free(sPayload); sPayload = NULL; return 0; }
    /* VisitAll describes fixed regions; no live values are changed until the
       complete exact-size payload and stream have been validated. */
    sPayloadOffset = 0;
    PortAchievementsInvalidate("Save state loaded");
    VisitAll(VisitApply);
    free(sPayload);
    sPayload = NULL;
    PortAudioReset();
    return 1;
}

void PortStateRequestSave(const char* path) {
    SDL_strlcpy(sPendingPath, path ? path : PortStatePath(gPortStateSlot), sizeof(sPendingPath));
    sPendingSave = 1;
}

void PortStateRequestLoad(const char* path) {
    SDL_strlcpy(sPendingPath, path ? path : PortStatePath(gPortStateSlot), sizeof(sPendingPath));
    if (!path) {
        FILE* current = fopen(sPendingPath, "rb");
        if (current) fclose(current);
        else {
            SDL_snprintf(sPendingPath, sizeof(sPendingPath), "%s", PortConfigPath("states/slot0.khst"));
            sPendingPath[strlen(sPendingPath) - 6] = (char)('0' + gPortStateSlot % 10);
        }
    }
    sPendingLoad = 1;
}

/* The same, for the platform's own use (rematches): no notice on screen. */
void PortStateRequestQuiet(int load, const char* path) {
    SDL_strlcpy(sPendingPath, path, sizeof(sPendingPath));
    if (load) sPendingLoad = 1; else sPendingSave = 1;
    sQuiet = 1;
}

/* 1 while a request waits; then the last result (> 0 done) */
int PortStateBusy(void) {
    return sPendingSave || sPendingLoad;
}
int PortStateLastResult(void) {
    return sLastResult;
}

/* Called at the top of every frame; acts only at the main loop's wait. */
void PortStateService(void) {
    char msg[96];
    int r;

    if (!gPortInMainLoop || (!sPendingSave && !sPendingLoad))
        return;
    if (sPendingSave) {
        sPendingSave = 0;
        r = Save(sPendingPath);
        sLastResult = r;
        PortLog("state: save %s -> %s", sPendingPath, r ? "ok" : "failed");
        SDL_snprintf(msg, sizeof(msg), r ? "State %d saved" : "State %d: save failed", gPortStateSlot);
        if (!sQuiet)
            PortUiNotice(msg);
    }
    if (sPendingLoad) {
        sPendingLoad = 0;
        r = Load(sPendingPath);
        sLastResult = r;
        PortLog("state: load %s -> %d", sPendingPath, r);
        SDL_snprintf(msg, sizeof(msg),
                     r > 0 ? "State %d loaded" : (r < 0 ? "State %d: other version" : "State %d: empty"),
                     gPortStateSlot);
        if (!sQuiet)
            PortUiNotice(msg);
    }
    sQuiet = 0;
}
