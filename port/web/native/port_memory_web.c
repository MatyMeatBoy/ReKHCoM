/* GBA hardware addresses live in sparse wasm32 linear memory. */
#include <emscripten.h>
#include <emscripten/heap.h>
#include <stdio.h>
#include <string.h>
#define MAX_PATH 1024
#include "port/port.h"
#include "port/rom_relocs.h"
_Static_assert(sizeof(void*)==4,"The browser core requires wasm32");
__attribute__((aligned(16))) uint8_t gPortBiosArea[0x10000];
static uint8_t sSramShadow[GBA_SRAM_SIZE];
static int sSramLoaded,romReady;
void PortBiosAreaRefresh(void) {
    uint32_t* p=(uint32_t*)gPortBiosArea;
    for(size_t i=0;i<sizeof(gPortBiosArea)/4;i++)p[i]=0xE3A02004;
}
int PortMemoryInit(void) {
    if(emscripten_get_heap_size()<GBA_SRAM+GBA_SRAM_SIZE ||
       (uintptr_t)&sSramLoaded<GBA_SRAM+GBA_SRAM_SIZE) {
        PortFatal("Browser memory layout overlaps GBA memory");return 0;
    }
    return 1; /* Linear memory is zero-filled; never touch the unused gaps. */
}
EMSCRIPTEN_KEEPALIVE int PortWebRomReady(unsigned size) {
    romReady=size==GBA_ROM_SIZE && !memcmp((void*)(GBA_ROM+0xAC),PORT_ROM_CODE,4);
    return romReady;
}
const char* gPortRomPath;

static int FindRom(void) {
    /* JS verifies SHA and writes directly at GBA_ROM: no MEMFS ROM duplicate. */
    if(!romReady)return 0;
    PortLog("ROM: browser local file (" PORT_ROM_CODE ")");return 1;
}
int PortRomLoad(void) {
    const PortRomReloc* r;

    if (!FindRom()) {
        PortFatal("Kingdom Hearts: Chain of Memories (" PORT_ROM_REGION ") ROM not found.\n\n"
                  "Select your cartridge dump (" PORT_ROM_CODE ", 32 MiB) in the browser "
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
EMSCRIPTEN_KEEPALIVE int PortWebActiveBank(void) {return sSramBank;}

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

static int SramSyncCurrent(void);

int PortSramSwitchBank(void) {
    if (gPortConfig.saveBank == sSramBank)
        return 1;
    if (!SramSyncCurrent()) {
        PortLog("Save: cannot switch bank %d to %d: current SRAM could not be persisted; pending save retained",
                sSramBank, gPortConfig.saveBank);
        return 0;
    }
    PortSramLoad();
    return 1;
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

static int SramSyncCurrent(void) {
    extern int PortSceneGalleryActive(void);
    if (PortSceneGalleryActive()) return 0; /* replay restores SRAM before normal syncing resumes */
    const uint8_t* sram = (const uint8_t*)(uintptr_t)GBA_SRAM;
    char tmp[MAX_PATH];
    FILE* f;

    if (!sSramLoaded || memcmp(sSramShadow, sram, GBA_SRAM_SIZE) == 0)
        return 1;
    snprintf(tmp, sizeof(tmp), "%s.tmp", PortConfigPath(BankName(sSramBank)));
    f = fopen(tmp, "wb");
    if (f == NULL)
        return 0;
    /* Always close exactly once, including failed writes / failed close. */
    int written = fwrite(sram, 1, GBA_SRAM_SIZE, f) == GBA_SRAM_SIZE;
    int closed = fclose(f) == 0;
    if (written && closed) {
        if (rename(tmp, PortConfigPath(BankName(sSramBank))) == 0) {
            memcpy(sSramShadow, sram, GBA_SRAM_SIZE);
            EM_ASM({if(Module.persistSoon)Module.persistSoon();});
            return 1;
        }
    }
    return 0;
}

void PortSramSyncIfDirty(void) {
    (void)SramSyncCurrent();
}
