/* Windows x64 host: the game's NULL accesses read the BIOS stand-in as on the GBA (port/ps5/shared/guest_biosfault.h).
   Fault-time only. Uses the guest text bounds of guest_divzero_windows.h. */
#include "../ps5/shared/guest_biosfault.h"
static struct { int reg; uint64_t delta, target; DWORD thread; int write; } sGuestBiosPending = { -1, 0, 0, 0, 0 };
static volatile LONG sGuestBiosRedirects;
static volatile uint64_t sGuestBiosFirstRip;
static DWORD64* GuestBiosReg(CONTEXT* c, int r) {
    switch (r) {
        case 0: return &c->Rax; case 1: return &c->Rcx; case 2: return &c->Rdx; case 3: return &c->Rbx;
        case 4: return &c->Rsp; case 5: return &c->Rbp; case 6: return &c->Rsi; case 7: return &c->Rdi;
        case 8: return &c->R8; case 9: return &c->R9; case 10: return &c->R10; case 11: return &c->R11;
        case 12: return &c->R12; case 13: return &c->R13; case 14: return &c->R14; default: return &c->R15;
    }
}
static LONG CALLBACK GuestBiosHandler(EXCEPTION_POINTERS* ep) {
    EXCEPTION_RECORD* r = ep->ExceptionRecord;
    CONTEXT* c = ep->ContextRecord;
    if (r->ExceptionCode == EXCEPTION_SINGLE_STEP) {
        DWORD64* v;
        if (sGuestBiosPending.reg < 0 || sGuestBiosPending.thread != GetCurrentThreadId())
            return EXCEPTION_CONTINUE_SEARCH;
        v = GuestBiosReg(c, sGuestBiosPending.reg);
        /* put the register back unless the instruction loaded something else into it */
        if (*v - sGuestBiosPending.target + PORT_GUEST_BIOS_SIZE < 2 * PORT_GUEST_BIOS_SIZE)
            *v -= sGuestBiosPending.delta;
        if (sGuestBiosPending.write)
            PortGuestBiosFill(); /* the GBA ignores writes to the BIOS */
        sGuestBiosPending.reg = -1;
        c->EFlags &= ~0x100u;
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    if (r->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && r->NumberParameters >= 2 && r->ExceptionInformation[0] != 8 &&
        sGuestBiosPending.reg < 0 && (uintptr_t)r->ExceptionAddress == (uintptr_t)c->Rip) {
        uint64_t regs[16], delta;
        int reg, i;
        for (i = 0; i < 16; i++)
            regs[i] = *GuestBiosReg(c, i);
        if (!PortGuestBiosDecode((uintptr_t)c->Rip, sGuestDivision, regs, r->ExceptionInformation[1], &reg, &delta))
            return EXCEPTION_CONTINUE_SEARCH;
        *GuestBiosReg(c, reg) += delta;
        sGuestBiosPending.reg = reg;
        sGuestBiosPending.delta = delta;
        sGuestBiosPending.target = regs[reg] + delta;
        sGuestBiosPending.thread = GetCurrentThreadId();
        sGuestBiosPending.write = r->ExceptionInformation[0] == 1;
        if (InterlockedIncrement(&sGuestBiosRedirects) == 1)
            sGuestBiosFirstRip = c->Rip;
        c->EFlags |= 0x100u; /* trap after this one instruction */
        return EXCEPTION_CONTINUE_EXECUTION;
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
