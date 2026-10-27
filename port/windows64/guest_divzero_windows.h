/* Fault-time only. No logging, allocation, CRT or OS calls on the guest stack. */
#include "../ps5/shared/guest_divzero.h"
static PortGuestDivisionBounds sGuestDivision;
static PortGuestDivisionMemory sGuestDivisionMemory;
static LONG CALLBACK GuestDivisionHandler(EXCEPTION_POINTERS* ep) {
    CONTEXT* c = ep->ContextRecord;
    PortGuestDivisionInstruction instruction;
    uint64_t regs[16];
    /* Windows can classify a zero high-byte DIV operand as integer overflow.
       Decode and prove zero for either #DE classification; a genuine nonzero
       quotient overflow is still rejected by the same operand check. */
    if ((ep->ExceptionRecord->ExceptionCode != EXCEPTION_INT_DIVIDE_BY_ZERO &&
         ep->ExceptionRecord->ExceptionCode != EXCEPTION_INT_OVERFLOW) ||
        (uintptr_t)ep->ExceptionRecord->ExceptionAddress != (uintptr_t)c->Rip) return EXCEPTION_CONTINUE_SEARCH;
    regs[0]=c->Rax; regs[1]=c->Rcx; regs[2]=c->Rdx; regs[3]=c->Rbx;
    regs[4]=c->Rsp; regs[5]=c->Rbp; regs[6]=c->Rsi; regs[7]=c->Rdi;
    regs[8]=c->R8; regs[9]=c->R9; regs[10]=c->R10; regs[11]=c->R11;
    regs[12]=c->R12; regs[13]=c->R13; regs[14]=c->R14; regs[15]=c->R15;
    instruction = PortGuestDivisionDecode((uintptr_t)c->Rip, sGuestDivision, regs, &sGuestDivisionMemory);
    if (!instruction.length) return EXCEPTION_CONTINUE_SEARCH;
    PortGuestDivisionResult(instruction.bytes, &regs[0], &regs[2]);
    c->Rax=regs[0];c->Rdx=regs[2];c->Rip+=instruction.length;
    return EXCEPTION_CONTINUE_EXECUTION;
}
