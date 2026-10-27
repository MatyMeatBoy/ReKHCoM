/* Portable loader logic probe: no guest instructions are executed. */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#ifdef _WIN32
#include <windows.h>
#define PROT_NONE 0
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4
#define MAP_PRIVATE 2
#define MAP_ANONYMOUS 32
#define MAP_FAILED ((void *)-1)
#define _SC_PAGESIZE 30
static long sysconf(int key) { (void)key; return 4096; }
static void *mmap(void *at, size_t n, int prot, int flags, int fd, size_t offset) {
    (void)at; (void)n; (void)prot; (void)flags; (void)fd; (void)offset; return MAP_FAILED;
}
static int munmap(void *p, size_t n) { (void)p; (void)n; return 0; }
static int mprotect(void *p, size_t n, int prot) { (void)p; (void)n; (void)prot; return 0; }
#else
#include <sys/mman.h>
#endif
#include "../ps5/shared/bridge.h"
static void Log(const char *fmt, ...) { (void)fmt; }
static void *SDL_LoadFile(const char *path, size_t *n) { (void)path; *n = 0; return NULL; }
#define SDL_free free
#include "../linuxarm64/macho_loader.inc"
int main(void) {
    uint64_t value;
    const uint8_t valid[] = {0xe5, 0x8e, 0x26};
    const uint8_t *p = valid;
    assert(ArmUleb(&p, valid + sizeof(valid), &value) && value == 624485);
    const uint8_t shortValue[] = {0x80}; p = shortValue;
    assert(!ArmUleb(&p, shortValue + sizeof(shortValue), &value));
    const uint8_t overflow[] = {255,255,255,255,255,255,255,255,255,2}; p = overflow;
    assert(!ArmUleb(&p, overflow + sizeof(overflow), &value));
#ifdef _WIN32
    uint32_t *word = VirtualAlloc((void *)0x50000000, 65536, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    assert(word == (void *)0x50000000);
#else
    uint32_t *word = mmap((void *)0x50000000, 65536, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    assert(word == (void *)0x50000000);
#endif
    ArmMachSegment segment = {.cmd = 1, .name = "__DATA", .addr = 0x50000000, .length = 4};
    const ArmMachSegment *segments[] = {&segment};
    const uint8_t rebase[] = {0x11, 0x20, 0, 0x51, 0};
    *word = 0x1234;
    assert(ArmRebase(rebase, rebase + sizeof(rebase), segments, 1, 0));
    assert(*word == 0x1234);
    /* slide changes both target address and pointer contents */
    segment.addr -= 0x1000;
    assert(ArmRebase(rebase, rebase + sizeof(rebase), segments, 1, 0x1000));
    assert(*word == 0x2234);
    const uint8_t badSeg[] = {0x11,0x21,0,0x51,0};
    assert(!ArmRebase(badSeg, badSeg + sizeof(badSeg), segments, 1, 0));
    const uint8_t outOfBounds[] = {0x11,0x20,4,0x51,0};
    assert(!ArmRebase(outOfBounds, outOfBounds + sizeof(outOfBounds), segments, 1, 0));
    const uint8_t badOpcode[] = {0x90};
    assert(!ArmRebase(badOpcode, badOpcode + 1, segments, 1, 0));
    Ps5GuestHeader header;
    assert(!LoadArm64Image("missing", &header));
#ifdef _WIN32
    VirtualFree(word, 0, MEM_RELEASE);
#else
    munmap(word, 65536);
#endif
    puts("ARM64 loader logic: ULEB, bounds, opcode, pointer slide, missing image PASS");
    return 0;
}
