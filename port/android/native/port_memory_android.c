/*
 * GBA address space for the native build.
 *
 * Every region is committed at its hardware address, so the game's register
 * macros, VRAM/OAM pointers and the pointers baked into ROM data are valid
 * as-is. It reserves the hardware range in a 32-bit process (Android, Linux); it keeps
 * the 0x02000000-0x0E010000 range free when the program starts.
 */
#include <sys/mman.h>
#include <unistd.h>
#define MAX_PATH 1024
#include <stdio.h>
#include <string.h>

#include "port/port.h"
#include "port/rom_relocs.h"

/* Keep original 32-bit GBA pointers, reserving virtual space without
 * replacing existing mappings. Only hardware regions receive physical pages. */
_Static_assert(sizeof(void*) == 4, "the native core requires a 32-bit ABI");

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

/* The range is claimed as soon as this library is loaded (the activity's onCreate), before the graphics driver, the
   audio stack and the Java heap's growth can land in it: on some phones (Google Pixel, issue #22) they took part of
   it by the time the game started. PortMemoryInit then only commits the regions. */
static int sEarlyReserved;

__attribute__((constructor)) static void ReserveGbaSpaceEarly(void) {
    const uintptr_t low = GBA_EWRAM, high = GBA_SRAM + GBA_SRAM_SIZE;
    void* space = mmap((void*)low, high - low, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (space == (void*)low)
        sEarlyReserved = 1;
    else if (space != MAP_FAILED)
        munmap(space, high - low);
}

int PortMemoryInit(void) {
    static const struct { uintptr_t base; size_t bytes; } regions[] = {
        {GBA_EWRAM,GBA_EWRAM_SIZE},{GBA_IWRAM,GBA_IWRAM_SIZE},
        {GBA_IO,GBA_IO_SIZE},{GBA_PLTT,GBA_PLTT_SIZE},
        {GBA_VRAM,GBA_VRAM_SIZE},{GBA_OAM,GBA_OAM_SIZE},
        {GBA_ROM,GBA_ROM_SIZE},{GBA_SRAM,GBA_SRAM_SIZE}
    };
    const uintptr_t low=GBA_EWRAM, high=GBA_SRAM+GBA_SRAM_SIZE;
    size_t page=(size_t)sysconf(_SC_PAGESIZE), i;
    char taken[512]="";
    void* space=sEarlyReserved ? (void*)low : mmap((void*)low,high-low,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    PortLog("GBA space: %s", sEarlyReserved ? "reserved when the library loaded" : "reserved at start");
    if (space!=(void*)low) {
        FILE* maps=fopen("/proc/self/maps","r");
        char line[256];
        int placed=0;
        PortLog("GBA space: mmap returned %p", space);
        /* What already lies in the hardware range (diagnostics on devices). */
        while(maps && fgets(line,sizeof(line),maps)) {
            unsigned long a=0,b=0;
            if(sscanf(line,"%lx-%lx",&a,&b)==2 && a<high && b>low) {
                PortLog("GBA space taken: %s",line);
                if (strlen(taken)+strlen(line)<sizeof(taken)) strcat(taken,line);
            }
        }
        if(maps) fclose(maps);
        if (space!=MAP_FAILED) munmap(space,high-low);
        /* Some devices keep a library or allocator mapping in the unused gaps
           between the hardware regions. The game only touches the regions
           themselves, so reserve each one on its own at its exact address. */
        for(i=0;i<sizeof(regions)/sizeof(regions[0]);i++) {
            uintptr_t start=regions[i].base & ~(uintptr_t)(page-1);
            size_t bytes=(regions[i].base+regions[i].bytes-start+page-1)&~(page-1);
            void* part=mmap((void*)start,bytes,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
            if (part!=(void*)start) {
                PortLog("GBA region %u cannot be reserved at %p (got %p)",(unsigned)i,(void*)start,part);
                if (part!=MAP_FAILED) munmap(part,bytes);
                break;
            }
            placed++;
        }
        if (placed!=(int)(sizeof(regions)/sizeof(regions[0]))) {
            for(i=0;i<(size_t)placed;i++) {
                uintptr_t start=regions[i].base & ~(uintptr_t)(page-1);
                munmap((void*)start,(regions[i].base+regions[i].bytes-start+page-1)&~(page-1));
            }
            /* the mappings in the way go in the message itself: Android keeps the log out of the player's reach */
            PortFatal("Cannot reserve the GBA address space 0x02000000-0x0E010000.\n"
                      "Please restart the phone and try again; if it still fails, send a screenshot of this message "
                      "(issue #22).\n\nIn use:\n%s", taken[0] ? taken : "(nothing listed)");
            return 0;
        }
        PortLog("GBA space: reserved region by region");
    }
    for(i=0;i<sizeof(regions)/sizeof(regions[0]);i++) {
        uintptr_t start=regions[i].base & ~(uintptr_t)(page-1);
        size_t bytes=(regions[i].base+regions[i].bytes-start+page-1)&~(page-1);
        if(mprotect((void*)start,bytes,PROT_READ|PROT_WRITE)!=0) {
            PortFatal("Cannot commit GBA region %u",(unsigned)i);
            return 0;
        }
    }
    return 1;
}

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
    PortLog("ROM: %s", path);
    return 1;
}

static int FindRom(void) {
    static const char* const names[] = { "baserom.gba", "roms/" PORT_ROM_CODE ".gba",
                                         "../../roms/" PORT_ROM_CODE ".gba" /* development tree */ };
    size_t i;

    if (gPortRomPath != NULL && TryRom(gPortRomPath))
        return 1;
    for (i = 0; i < sizeof(names) / sizeof(names[0]); i++) {
        if (TryRom(PortConfigPath(names[i])))
            return 1;
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
    /* ROM words that referenced code or data now compiled natively. */
    for (r = gPortRomRelocs; r->romAddress != 0; r++)
        *(uint32_t*)(uintptr_t)r->romAddress = (uint32_t)(uintptr_t)r->target;
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
        if (rename(tmp, PortConfigPath(BankName(sSramBank))) == 0)
        {
            extern void PortUiSaved(void);
            memcpy(sSramShadow, sram, GBA_SRAM_SIZE);
            PortUiSaved(); /* Quit the game: how long since the last save */
        }
    }
}
