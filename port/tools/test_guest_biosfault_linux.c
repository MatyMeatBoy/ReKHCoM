/* Linux x86-64 counterpart of test_guest_biosfault.c: real NULL reads and writes from code marked as guest text go
   to the BIOS stand-in through SIGSEGV + single-step SIGTRAP; a NULL read outside it still crashes.
   Build: clang --target=x86_64-linux-gnu --sysroot=<amd64 sysroot> -fuse-ld=lld -O2 -I. this.c (run on Linux). */
#define _GNU_SOURCE
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <ucontext.h>
#include "port/linux64/guest_divzero_linux.h"
#include "port/linux64/guest_biosfault_linux.h"

static int sFailures;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); sFailures++; } } while (0)

__attribute__((noinline, section("guesttext"))) static uint32_t GuestRead(const volatile uint32_t* p) { return p[10]; }
__attribute__((noinline, section("guesttext"))) static void GuestWrite(volatile uint32_t* p) { p[3] = 0x12345678u; }
__attribute__((noinline, section("guesttext"))) static uint8_t GuestByte(const volatile uint8_t* p, long i) { return p[i * 4 + 1]; }
__attribute__((noinline)) static uint32_t HostRead(const volatile uint32_t* p) { return p[1]; }
extern char __start_guesttext[], __stop_guesttext[];
static void* volatile sNull; /* a NULL the compiler cannot fold into an absolute address */

int main(int argc, char** argv) {
    void* standIn;
    setvbuf(stdout, NULL, _IONBF, 0);
    standIn = mmap((void*)(uintptr_t)PORT_GUEST_BIOS_STANDIN, PORT_GUEST_BIOS_SIZE, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    CHECK(standIn == (void*)(uintptr_t)PORT_GUEST_BIOS_STANDIN);
    if (standIn != (void*)(uintptr_t)PORT_GUEST_BIOS_STANDIN) return 1;
    PortGuestBiosFill();
    sGuestDivision.begin = (uintptr_t)__start_guesttext;
    sGuestDivision.end = (uintptr_t)__stop_guesttext;
    CHECK(GuestDivisionInstall());
    CHECK(GuestBiosInstall());

    printf("guest text %p..%p read at %p\n", (void*)sGuestDivision.begin, (void*)sGuestDivision.end, (void*)GuestRead);
    CHECK(GuestRead(sNull) == PORT_GUEST_BIOS_WORD);
    GuestWrite(sNull);
    CHECK(((volatile uint32_t*)(uintptr_t)PORT_GUEST_BIOS_STANDIN)[3] == PORT_GUEST_BIOS_WORD); /* write ignored */
    CHECK(GuestByte(sNull, 2) == ((PORT_GUEST_BIOS_WORD >> 8) & 0xFF));
    CHECK(sGuestBiosRedirects == 3);
    if (argc > 1 && !strcmp(argv[1], "--host-null")) {
        fflush(stdout);
        HostRead(sNull); /* outside the guest text: must stay a crash (SIGSEGV through the previous action) */
        printf("FAIL: a host NULL read was redirected\n");
        return 1;
    }
    printf(sFailures ? "guest BIOS stand-in (Linux): %d failure(s)\n" : "guest BIOS stand-in (Linux): handler tests passed\n", sFailures);
    return sFailures != 0;
}
