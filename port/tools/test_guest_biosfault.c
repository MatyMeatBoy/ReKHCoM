/* The x86-64 BIOS stand-in for the game's NULL accesses (port/ps5/shared/guest_biosfault.h, issue #564).
   Decoder cases, then real NULL reads and writes from code marked as guest text under the Windows handler.
   Build: clang -O2 -I. port/tools/test_guest_biosfault.c -o test_guest_biosfault.exe (x86_64 Windows). */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "port/windows64/guest_divzero_windows.h"
#include "port/windows64/guest_biosfault_windows.h"

static int sFailures;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); sFailures++; } } while (0)

static int Decode(const unsigned char* code, size_t length, const uint64_t regs[16], uint64_t fault, int* reg,
                  uint64_t* delta) {
    PortGuestDivisionBounds bounds = { (uintptr_t)code, (uintptr_t)code + length };
    return PortGuestBiosDecode((uintptr_t)code, bounds, regs, fault, reg, delta);
}

static void DecoderCases(void) {
    uint64_t regs[16] = { 0 }, delta;
    int reg;
    static const unsigned char cmpR14[] = { 0x67, 0x41, 0x83, 0x7e, 0x28, 0x02 };   /* cmp dword [r14d+0x28],2 (#564) */
    static const unsigned char movRax[] = { 0x8b, 0x40, 0x08 };                     /* mov eax,[rax+8] */
    static const unsigned char baseIndex[] = { 0x48, 0x8b, 0x04, 0xc8 };            /* mov rax,[rax+rcx*8] */
    static const unsigned char indexOnly[] = { 0x8b, 0x04, 0x8d, 0x10, 0, 0, 0 };   /* mov eax,[rcx*4+0x10] */
    static const unsigned char ripRel[] = { 0x8b, 0x05, 0x10, 0, 0, 0 };            /* mov eax,[rip+0x10] */
    static const unsigned char movsd[] = { 0xf3, 0xa5 };                            /* rep movsd */
    static const unsigned char regReg[] = { 0x01, 0xc0 };                           /* add eax,eax */
    static const unsigned char storeR15[] = { 0x66, 0x41, 0x89, 0x47, 0x04 };       /* mov [r15+4],ax */
    static const unsigned char movzx[] = { 0x0f, 0xb6, 0x41, 0x05 };                /* movzx eax,byte [rcx+5] */
    static const unsigned char fsRead[] = { 0x64, 0x8b, 0x00 };                     /* mov eax,fs:[rax] */
    static const unsigned char stack[] = { 0x8b, 0x44, 0x24, 0x08 };                /* mov eax,[rsp+8] */

    CHECK(Decode(cmpR14, sizeof(cmpR14), regs, 0x28, &reg, &delta) && reg == 14 && delta == PORT_GUEST_BIOS_STANDIN);
    CHECK(Decode(movRax, sizeof(movRax), regs, 8, &reg, &delta) && reg == 0);
    CHECK(Decode(baseIndex, sizeof(baseIndex), regs, 0, &reg, &delta) && reg == 0);
    CHECK(Decode(indexOnly, sizeof(indexOnly), regs, 0x10, &reg, &delta) && reg == 1 &&
          delta == PORT_GUEST_BIOS_STANDIN / 4);
    CHECK(!Decode(ripRel, sizeof(ripRel), regs, 0x10, &reg, &delta));
    regs[6] = 0x10;
    CHECK(Decode(movsd, sizeof(movsd), regs, 0x10, &reg, &delta) && reg == 6);
    regs[6] = 0;
    CHECK(!Decode(regReg, sizeof(regReg), regs, 0, &reg, &delta));
    CHECK(Decode(storeR15, sizeof(storeR15), regs, 4, &reg, &delta) && reg == 15);
    CHECK(Decode(movzx, sizeof(movzx), regs, 5, &reg, &delta) && reg == 1);
    CHECK(!Decode(fsRead, sizeof(fsRead), regs, 0, &reg, &delta));
    CHECK(!Decode(stack, sizeof(stack), regs, 8, &reg, &delta));
    CHECK(!Decode(movRax, sizeof(movRax), regs, 0x200, &reg, &delta));      /* not this operand's address */
    CHECK(!Decode(movRax, sizeof(movRax), regs, 0x10008, &reg, &delta));    /* past the BIOS area */
    regs[0] = 0x02000000;
    CHECK(!Decode(movRax, sizeof(movRax), regs, 8, &reg, &delta));          /* a real pointer: no match */
    {
        PortGuestDivisionBounds outside = { (uintptr_t)movRax + 1, (uintptr_t)movRax + 3 };
        regs[0] = 0;
        CHECK(!PortGuestBiosDecode((uintptr_t)movRax, outside, regs, 8, &reg, &delta)); /* not guest text */
    }
}

/* "guest" code: the handler only acts inside sGuestDivision */
__attribute__((noinline, section(".text$guest1"))) static uint32_t GuestRead(const volatile uint32_t* p) { return p[10]; }
__attribute__((noinline, section(".text$guest2"))) static void GuestWrite(volatile uint32_t* p) { p[3] = 0x12345678u; }
__attribute__((noinline, section(".text$guest3"))) static const volatile uint32_t* GuestChase(const volatile uint32_t* const volatile* p) {
    return (const volatile uint32_t*)(uintptr_t)*(const volatile uint32_t*)p; /* the loaded value replaces the base */
}
static void* volatile sNull; /* a NULL the compiler cannot fold into an absolute address */
__attribute__((noinline)) static uint32_t HostRead(const volatile uint32_t* p) { return p[1]; }

static LONG CALLBACK Unhandled(EXCEPTION_POINTERS* ep) {
    printf("unhandled exception %08lx at %p (expected only with --host-null)\n", ep->ExceptionRecord->ExceptionCode, ep->ExceptionRecord->ExceptionAddress);
    fflush(stdout);
    ExitProcess(3);
    return EXCEPTION_CONTINUE_SEARCH;
}

int main(int argc, char** argv) {
    uintptr_t lo, hi;
    void* standIn;
    DecoderCases();

    standIn = VirtualAlloc((void*)(uintptr_t)PORT_GUEST_BIOS_STANDIN, PORT_GUEST_BIOS_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    CHECK(standIn == (void*)(uintptr_t)PORT_GUEST_BIOS_STANDIN);
    if (!standIn) return 1;
    PortGuestBiosFill();
    lo = (uintptr_t)GuestRead; hi = lo;
    if ((uintptr_t)GuestWrite < lo) lo = (uintptr_t)GuestWrite;
    if ((uintptr_t)GuestChase < lo) lo = (uintptr_t)GuestChase;
    if ((uintptr_t)GuestWrite > hi) hi = (uintptr_t)GuestWrite;
    if ((uintptr_t)GuestChase > hi) hi = (uintptr_t)GuestChase;
    sGuestDivision.begin = lo;
    sGuestDivision.end = hi + 64;
    AddVectoredExceptionHandler(1, GuestBiosHandler);
    SetUnhandledExceptionFilter(Unhandled);

    CHECK(GuestRead(sNull) == PORT_GUEST_BIOS_WORD);
    GuestWrite(sNull);
    CHECK(((volatile uint32_t*)(uintptr_t)PORT_GUEST_BIOS_STANDIN)[3] == PORT_GUEST_BIOS_WORD); /* write ignored */
    CHECK(GuestChase(sNull) == (const volatile uint32_t*)(uintptr_t)PORT_GUEST_BIOS_WORD);
    CHECK(sGuestBiosRedirects == 3);
    if (argc > 1 && !strcmp(argv[1], "--host-null")) {
        HostRead(sNull); /* outside the guest text: must stay a crash */
        printf("FAIL: a host NULL read was redirected\n");
        return 1;
    }
    printf(sFailures ? "guest BIOS stand-in: %d failure(s)\n" : "guest BIOS stand-in: decoder and handler tests passed\n", sFailures);
    return sFailures != 0;
}
