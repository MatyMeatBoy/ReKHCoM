/*
 * GBA address space for the native build.
 *
 * Every region is committed at its hardware address, so the game's register
 * macros, VRAM/OAM pointers and the pointers baked into ROM data are valid
 * as-is. This requires a 32-bit process with a fixed image base, which keeps
 * the 0x02000000-0x0E010000 range free when the program starts.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>

#include "port/port.h"
#include "port/rom_relocs.h"

/*
 * The GBA address range is part of the executable image: an uninitialised
 * section placed after the program's own data (".bss$z" sorts last) and large
 * enough to span 0x02000000-0x0E010000. Windows maps the image before any DLL,
 * heap or system view, so the range is guaranteed to be ours, already
 * committed and zero-filled.
 */
#define GBA_SPACE_END 0x0E010000u
#define GBA_SPACE_SIZE (GBA_SPACE_END - 0x00400000u)

#ifndef PORT_GBA_RELOC
/* Its own uninitialised section: no file space, whatever follows it. */
#pragma clang section bss = "gbaspace"
__attribute__((used, aligned(4096))) static uint8_t sGbaSpace[GBA_SPACE_SIZE];
#pragma clang section bss = ""
#endif

/* Stand-in for the BIOS area behind BIOS_PTR(): reads see the open-bus value
   left by the BIOS (the opcode after the SWI return), writes are undone every
   frame. */
__attribute__((aligned(16))) uint8_t gPortBiosArea[0x10000];

void PortBiosAreaRefresh(void) {
    uint32_t* p = (uint32_t*)gPortBiosArea;
    size_t i;

    for (i = 0; i < sizeof(gPortBiosArea) / 4; i++)
        p[i] = 0xE3A02004;
}

static uint8_t sSramShadow[GBA_SRAM_SIZE];
static int sSramLoaded;

#ifdef PORT_GBA_RELOC
/* The relocatable build (include/gba/gba_space.h) keeps its regions in arrays elsewhere. Its Windows check build
   reserves the hardware range without access, so any access the game still makes through a bus address faults and
   the stray-pointer handler (port_biosfault.c) logs it and sends it to the array, as a console's handler would. */
int PortMemoryInit(void) {
    uint32_t a, reserved = 0, total = 0;
    /* in 64 KiB pieces: libraries and heaps loaded before this keep the pieces they took */
    for (a = 0x02000000u; a < GBA_SPACE_END; a += 0x10000u, total++)
        reserved += VirtualAlloc((void*)(uintptr_t)a, 0x10000u, MEM_RESERVE, PAGE_NOACCESS) != NULL;
    PortLog("GBA space: %u of %u pieces of the bus range reserved", (unsigned)reserved, (unsigned)total);
    PortLog("GBA space: relocatable, EWRAM %p VRAM %p ROM %p", (void*)gGbaEwram, (void*)gGbaVram, (void*)gGbaRom);
    return 1;
}
#else
int PortMemoryInit(void) {
    uint32_t start = (uint32_t)(uintptr_t)sGbaSpace;
    uint32_t end = start + GBA_SPACE_SIZE;

    if (start > GBA_EWRAM || end < GBA_SPACE_END) {
        PortFatal("The executable's GBA memory section is at 0x%08X-0x%08X and does not cover "
                  "0x%08X-0x%08X. The program was linked incorrectly.",
                  start, end, GBA_EWRAM, GBA_SPACE_END);
        return 0;
    }
    return 1;
}
#endif

const char* gPortRomPath;

static int TryRom(const char* path) {
    FILE* f = fopen(path, "rb");
    uint8_t header[0xC0];
    long size;

    if (f == NULL)
        return 0;
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size != (long)GBA_ROM_SIZE || fread(header, 1, sizeof(header), f) != sizeof(header)
        || memcmp(header + 0xAC, PORT_ROM_CODE, 4) != 0) {
        fclose(f);
        return 0;
    }
    fseek(f, 0, SEEK_SET);
    if (fread((void*)(uintptr_t)GBA_ROM, 1, GBA_ROM_SIZE, f) != GBA_ROM_SIZE) {
        fclose(f);
        return 0;
    }
    fclose(f);
    PortAchievementsRomLoaded((const uint8_t*)(uintptr_t)GBA_ROM, GBA_ROM_SIZE);
    PortLog("ROM: %s", path);
    return 1;
}

static int FindRom(void) {
    WIN32_FIND_DATAA fd;
    HANDLE h;
    char path[MAX_PATH];
    static const char* const names[] = { "baserom.gba", "roms/" PORT_ROM_CODE ".gba",
                                         "../../roms/" PORT_ROM_CODE ".gba" /* development tree */ };
    size_t i;

    if (gPortRomPath != NULL && TryRom(gPortRomPath))
        return 1;
    for (i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        if (TryRom(PortConfigPath(names[i])))
            return 1;
    }
    /* Any dump of this cartridge version next to the executable. */
    h = FindFirstFileA(PortConfigPath("*.gba"), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            snprintf(path, sizeof(path), "%s", PortConfigPath(fd.cFileName));
            if (TryRom(path)) {
                FindClose(h);
                return 1;
            }
        } while (FindNextFileA(h, &fd));
        FindClose(h);
    }
    return 0;
}

int PortRomLoad(void) {
    const PortRomReloc* r;

    if (!FindRom()) {
        PortFatal("Kingdom Hearts: Chain of Memories (" PORT_ROM_REGION ") ROM not found.\n\n"
                  "Place your cartridge dump (" PORT_ROM_CODE ", 32 MiB) next to the executable "
                  "as \"baserom.gba\".");
        return 0;
    }
#ifdef PORT_GBA_RELOC
    {
        extern int GbaSpaceMoveRomPointers(void);
        PortLog("GBA space: %d ROM pointers moved", GbaSpaceMoveRomPointers());
    }
#endif
    /* ROM words that referenced code or data now compiled natively. */
    for (r = gPortRomRelocs; r->romAddress != 0; r++)
        *(uint32_t*)(uintptr_t)(GBA_ROM + (r->romAddress - 0x08000000u)) = (uint32_t)(uintptr_t)r->target;
    return 1;
}

/* The bank the SRAM was loaded from (gPortConfig.saveBank can change first). */
static int sSramBank = 1;

static const char* BankName(int bank) {
    static char name[64];
    size_t len = strlen(PORT_SAVE_NAME) - 4; /* without ".sav" */

    if (bank <= 1)
        return PORT_SAVE_NAME;
    snprintf(name, sizeof(name), "%.*s_bank%d.sav", (int)len, PORT_SAVE_NAME, bank);
    return name;
}

const char* PortSaveName(void) {
    return BankName(sSramBank);
}

void PortSramSwitchBank(void) {
    if (gPortConfig.saveBank == sSramBank)
        return;
    PortSramSyncIfDirty();
    PortSramLoad();
}

void PortSramLoad(void) {
    FILE* f;
    sSramBank = gPortConfig.saveBank < 1 ? 1 : gPortConfig.saveBank > PORT_SAVE_BANKS ? PORT_SAVE_BANKS : gPortConfig.saveBank;
    f = fopen(PortConfigPath(BankName(sSramBank)), "rb");
    uint8_t* sram = (uint8_t*)(uintptr_t)GBA_SRAM;

    memset(sram, 0xFF, GBA_SRAM_SIZE);
    if (f != NULL) {
        size_t n = fread(sram, 1, GBA_SRAM_SIZE, f);
        fclose(f);
        PortLog("Save: loaded %u bytes from %s", (unsigned)n, BankName(sSramBank));
    }
    memcpy(sSramShadow, sram, GBA_SRAM_SIZE);
    sSramLoaded = 1;
}

void PortSramSyncIfDirty(void) {
    const uint8_t* sram = (const uint8_t*)(uintptr_t)GBA_SRAM;
    char tmp[MAX_PATH];
    FILE* f;

    if (!sSramLoaded || memcmp(sSramShadow, sram, GBA_SRAM_SIZE) == 0)
        return;
    snprintf(tmp, sizeof(tmp), "%s.tmp", PortConfigPath(BankName(sSramBank)));
    f = fopen(tmp, "wb");
    if (f == NULL)
        return;
    /* Always close exactly once, including failed writes / failed close. */
    int written = fwrite(sram, 1, GBA_SRAM_SIZE, f) == GBA_SRAM_SIZE;
    int closed = fclose(f) == 0;
    if (written && closed) {
        if (MoveFileExA(tmp, PortConfigPath(BankName(sSramBank)), MOVEFILE_REPLACE_EXISTING))
        {
            extern void PortUiSaved(void);
            memcpy(sSramShadow, sram, GBA_SRAM_SIZE);
            PortUiSaved(); /* Quit the game: how long since the last save */
        }
    }
}
