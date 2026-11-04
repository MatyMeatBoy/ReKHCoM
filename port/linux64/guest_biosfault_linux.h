/* Linux x86-64 host: the game's NULL accesses read the BIOS stand-in as on the GBA (port/ps5/shared/guest_biosfault.h).
   Fault-time only. Uses the guest text bounds of guest_divzero_linux.h, after which it is included; SIGSEGV moves the
   register, SIGTRAP (the single step) puts it back. Anything else goes to the previous action. */
#include "../ps5/shared/guest_biosfault.h"
static struct sigaction sGuestBiosPreviousSegv, sGuestBiosPreviousTrap;
static struct { int reg; uint64_t delta, target; int write; } sGuestBiosPending = { -1, 0, 0, 0 };
static volatile long sGuestBiosRedirects;
static volatile uint64_t sGuestBiosFirstRip;
static greg_t* GuestBiosReg(greg_t* g, int r) {
    static const int map[16] = { REG_RAX, REG_RCX, REG_RDX, REG_RBX, REG_RSP, REG_RBP, REG_RSI, REG_RDI,
                                 REG_R8, REG_R9, REG_R10, REG_R11, REG_R12, REG_R13, REG_R14, REG_R15 };
    return &g[map[r & 15]];
}
static void GuestBiosChain(struct sigaction* previous, int sig, siginfo_t* info, void* opaque) {
    if (previous->sa_handler == SIG_DFL || previous->sa_handler == SIG_IGN) {
        signal(sig, SIG_DFL);
        raise(sig);
    } else if (previous->sa_flags & SA_SIGINFO)
        previous->sa_sigaction(sig, info, opaque);
    else
        previous->sa_handler(sig);
}
static void GuestBiosSignal(int sig, siginfo_t* info, void* opaque) {
    ucontext_t* context = (ucontext_t*)opaque;
    greg_t* g = context->uc_mcontext.gregs;
    if (sig == SIGTRAP && sGuestBiosPending.reg >= 0) {
        greg_t* v = GuestBiosReg(g, sGuestBiosPending.reg);
        if ((uint64_t)*v - sGuestBiosPending.target + PORT_GUEST_BIOS_SIZE < 2 * PORT_GUEST_BIOS_SIZE)
            *v = (greg_t)((uint64_t)*v - sGuestBiosPending.delta);
        if (sGuestBiosPending.write)
            PortGuestBiosFill(); /* the GBA ignores writes to the BIOS */
        sGuestBiosPending.reg = -1;
        g[REG_EFL] &= ~(greg_t)0x100;
        return;
    }
    if (sig == SIGSEGV && sGuestBiosPending.reg < 0) {
        uint64_t regs[16], delta;
        int reg, i;
        for (i = 0; i < 16; i++)
            regs[i] = (uint64_t)*GuestBiosReg(g, i);
        if (PortGuestBiosDecode((uintptr_t)g[REG_RIP], sGuestDivision, regs, (uint64_t)(uintptr_t)info->si_addr, &reg, &delta)) {
            *GuestBiosReg(g, reg) = (greg_t)(regs[reg] + delta);
            sGuestBiosPending.reg = reg;
            sGuestBiosPending.delta = delta;
            sGuestBiosPending.target = regs[reg] + delta;
            sGuestBiosPending.write = (g[REG_ERR] & 2) != 0;
            if (__sync_add_and_fetch(&sGuestBiosRedirects, 1) == 1)
                sGuestBiosFirstRip = (uint64_t)g[REG_RIP];
            g[REG_EFL] |= 0x100; /* trap after this one instruction */
            return;
        }
    }
    GuestBiosChain(sig == SIGTRAP ? &sGuestBiosPreviousTrap : &sGuestBiosPreviousSegv, sig, info, opaque);
}
/* After GuestDivisionInstall (its alternate stack serves both). */
static int GuestBiosInstall(void) {
    struct sigaction action = {0};
    action.sa_sigaction = GuestBiosSignal;
    action.sa_flags = SA_SIGINFO | SA_ONSTACK;
    sigemptyset(&action.sa_mask);
    if (sigaction(SIGSEGV, &action, &sGuestBiosPreviousSegv) != 0)
        return 0;
    if (sigaction(SIGTRAP, &action, &sGuestBiosPreviousTrap) != 0) {
        sigaction(SIGSEGV, &sGuestBiosPreviousSegv, NULL);
        return 0;
    }
    return 1;
}
