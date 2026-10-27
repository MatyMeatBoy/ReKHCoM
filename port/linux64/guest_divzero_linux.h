/* Linux x86-64 counterpart: genuine zero-operand guest DIV/IDIV only. */
#include <signal.h>
#include <ucontext.h>
#include "../ps5/shared/guest_divzero.h"
static PortGuestDivisionBounds sGuestDivision;
static struct sigaction sGuestDivisionPrevious;
static unsigned char sGuestDivisionStack[65536];
static void GuestDivisionSignal(int sig, siginfo_t* info, void* opaque) {
    ucontext_t* context = (ucontext_t*)opaque;
    greg_t* g = context->uc_mcontext.gregs;
    unsigned length = 0;
    if (sig == SIGFPE && info->si_code == FPE_INTDIV) {
        uint64_t regs[16] = {g[REG_RAX],g[REG_RCX],g[REG_RDX],g[REG_RBX],g[REG_RSP],g[REG_RBP],g[REG_RSI],g[REG_RDI],
                             g[REG_R8],g[REG_R9],g[REG_R10],g[REG_R11],g[REG_R12],g[REG_R13],g[REG_R14],g[REG_R15]};
        length = PortGuestDivisionLength((uintptr_t)g[REG_RIP], sGuestDivision, regs);
    }
    if (length) { g[REG_RAX]=0; g[REG_RDX]=0; g[REG_RIP]+=length; return; }
    /* sa_handler/sa_sigaction share storage: SIG_DFL/IGN remain sentinels
     * even if an earlier action retained SA_SIGINFO. Never call either. */
    if (sGuestDivisionPrevious.sa_handler == SIG_DFL || sGuestDivisionPrevious.sa_handler == SIG_IGN) {
        signal(sig, SIG_DFL); raise(sig);
    } else if (sGuestDivisionPrevious.sa_flags & SA_SIGINFO)
        sGuestDivisionPrevious.sa_sigaction(sig, info, opaque);
    else
        sGuestDivisionPrevious.sa_handler(sig);
}
static int GuestDivisionInstall(void) {
    stack_t stack = {0}, previous;
    struct sigaction action = {0};
    int installStack;
    if (sigaltstack(NULL, &previous) != 0) return 0;
    installStack = (previous.ss_flags & SS_DISABLE) != 0;
    stack.ss_sp=sGuestDivisionStack; stack.ss_size=sizeof(sGuestDivisionStack);
    action.sa_sigaction=GuestDivisionSignal; action.sa_flags=SA_SIGINFO|SA_ONSTACK;
    sigemptyset(&action.sa_mask);
    if (installStack && sigaltstack(&stack, NULL) != 0) return 0;
    if (sigaction(SIGFPE, &action, &sGuestDivisionPrevious) == 0) return 1;
    if (installStack) sigaltstack(&previous, NULL);
    return 0;
}
