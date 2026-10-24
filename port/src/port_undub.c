/* Japanese battle voices from an independently verified original cartridge.
 * No patch or audio data is shipped. The map contains addresses and lengths.
 * Keep samples at fixed ROM addresses so save-state pointers remain valid. */
#include <SDL3/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "port/port.h"

typedef struct { uint32_t pointer, dest, source, size; } UndubSample;
typedef struct { uint32_t dest, source; int header, oldHeader; } UndubSong;
#include "port/port_undub_map.h"

#ifndef VERSION_JP
/* The Japanese cartridge is read in pieces when the voices are copied, never held whole (32 MB did not fit beside
   the game on the Vita, report in #44); sPath is the dump found and verified. */
static char sPath[512];
static int sChecked;
#endif

static uint32_t Read32(const uint8_t* p) {
    return p[0] | (uint32_t)p[1] << 8 | (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}
static void Write32(uint8_t* p, uint32_t n) {
    p[0] = n; p[1] = n >> 8; p[2] = n >> 16; p[3] = n >> 24;
}

#ifndef VERSION_JP
#if !__STDC_HOSTED__
#include "bridge_calls.h"
#endif

/* path is the original Japanese cartridge: its header, its size and its CRC, read 64 KB at a time */
static int IsJapanese(const char* path) {
    static uint8_t buf[0x10000];
    uint32_t table[256], crc = ~0u;
    size_t total = 0, got, k;
    unsigned i, j;
    FILE* f = fopen(path, "rb");
    if (!f)
        return 0;
    for (i = 0; i < 256; i++) {
        uint32_t c = i;
        for (j = 0; j < 8; j++) c = (c >> 1) ^ (0xEDB88320u & (0u - (c & 1)));
        table[i] = c;
    }
    while ((got = fread(buf, 1, sizeof(buf), f)) > 0) {
        if (total == 0 && (got < 0xB0 || memcmp(buf + 0xAC, "B8CJ", 4) != 0))
            break;
        for (k = 0; k < got; k++) crc = (crc >> 8) ^ table[(crc ^ buf[k]) & 255];
        total += got;
        if (total > GBA_ROM_SIZE)
            break;
    }
    fclose(f);
    return total == GBA_ROM_SIZE && ~crc == 0x8D5A0D84u;
}

/* every .gba of a folder until one is the Japanese cartridge (the consoles take a dump of any name) */
static int FindIn(const char* folder) {
    char path[512];
#if !__STDC_HOSTED__
    char name[256];
    int dir = h_dir_open(folder);
    if (dir < 0)
        return 0;
    while (h_dir_next(dir, name, sizeof(name))) {
        size_t n = strlen(name);
        if (n > 4 && strcasecmp(name + n - 4, ".gba") == 0) {
            SDL_snprintf(path, sizeof(path), "%s/%s", folder, name);
            if (IsJapanese(path)) {
                SDL_snprintf(sPath, sizeof(sPath), "%s", path);
                h_dir_close(dir);
                return 1;
            }
        }
    }
    h_dir_close(dir);
#else
    int count = 0, i, found = 0;
    char** names = SDL_GlobDirectory(folder, "*.gba", SDL_GLOB_CASEINSENSITIVE, &count);
    for (i = 0; names && i < count && !found; i++) {
        SDL_snprintf(path, sizeof(path), "%s/%s", folder, names[i]);
        if (IsJapanese(path)) {
            SDL_snprintf(sPath, sizeof(sPath), "%s", path);
            found = 1;
        }
    }
    SDL_free(names);
    if (found)
        return 1;
#endif
    return 0;
}
#endif

int PortUndubAvailable(void) {
#ifdef VERSION_JP
    return 0; /* already Japanese */
#else
    static const char* names[] = { "roms/B8CJ.gba", "../../roms/B8CJ.gba", "B8CJ.gba" };
    char folder[512];
    size_t i;
    if (sChecked) return sPath[0] != 0;
    sChecked = 1;
    for (i = 0; i < SDL_arraysize(names); i++)
        if (IsJapanese(PortConfigPath(names[i]))) {
            SDL_snprintf(sPath, sizeof(sPath), "%s", PortConfigPath(names[i]));
            return 1;
        }
    /* any name: the data folder (the consoles' ROM folder) and its roms folder */
    SDL_snprintf(folder, sizeof(folder), "%s", PortConfigPath("roms"));
    if (FindIn(folder))
        return 1;
    SDL_snprintf(folder, sizeof(folder), "%s", PortConfigPath(""));
    for (i = SDL_strlen(folder); i > 0 && (folder[i - 1] == '/' || folder[i - 1] == '\\'); i--)
        folder[i - 1] = 0;
    return FindIn(folder[0] ? folder : ".");
#endif
}

#ifndef VERSION_JP
/* n bytes of the Japanese cartridge from offset at into out */
static int ReadJapanese(FILE* f, uint32_t at, void* out, uint32_t n) {
    return fseek(f, (long)at, SEEK_SET) == 0 && fread(out, 1, n, f) == n;
}
#endif

void PortUndubApply(void) {
#ifndef VERSION_JP
    uint8_t* rom = (uint8_t*)(uintptr_t)GBA_ROM;
    size_t i;
    unsigned samples = 0, songs = 0;
    FILE* f;
    if (!gPortConfig.undub) return;
    if (!PortUndubAvailable() || (f = fopen(sPath, "rb")) == NULL) {
        PortLog("undub: original Japanese ROM unavailable; original audio retained");
        return;
    }
    for (i = 0; i < SDL_arraysize(kUndubSamples); i++) {
        const UndubSample* s = &kUndubSamples[i];
        if (s->source && !ReadJapanese(f, s->source, rom + s->dest, s->size))
            continue; /* the English voice stays where it points */
        if (!s->source)
            memset(rom + s->dest, 0, s->size); /* removed English-only voice */
        Write32(rom + s->pointer, GBA_ROM + s->dest);
        samples++;
    }
    for (i = 0; i < SDL_arraysize(kUndubSongs); i++) {
        const UndubSong* s = &kUndubSongs[i];
        uint8_t* from = malloc((size_t)s->header + 12);
        uint8_t* to = rom + s->dest;
        int k, out = 0, running = -1;
        if (from == NULL || !ReadJapanese(f, s->source, from, (uint32_t)s->header + 12)) {
            free(from);
            continue;
        }
        /* Voice tracks contain only single-byte MIDI commands/arguments.
         * Remove redundant note opcodes and centre-pan commands, as Undub
         * does. The original header stays pinned for native gSongTable. */
        for (k = 0; k < s->header; k++) {
            int b = from[k];
            if (b == 0xBF && k + 1 < s->header && from[k + 1] == 0x40) { k++; continue; }
            if (b >= 0xCF && b == running) continue;
            if (b >= 0xBD) running = b;
            to[out++] = b;
            if (b == 0xB1) break;
        }
        memset(to + out, 0, s->oldHeader - out);
        memcpy(to + s->oldHeader, from + s->header, 12);
        /* The voice group and track pointers are relative to each region. */
#ifdef VERSION_US
        const uint32_t groupDelta = 0x26AF4;
#else
        const uint32_t groupDelta = 0x96A20;
#endif
        Write32(to + s->oldHeader + 4, Read32(from + s->header + 4) + groupDelta);
        Write32(to + s->oldHeader + 8, GBA_ROM + s->dest);
        free(from);
        songs++;
    }
    fclose(f);
    PortLog("undub: %u Japanese samples, %u voice tracks loaded (%s)", samples, songs, sPath);
#endif
}
