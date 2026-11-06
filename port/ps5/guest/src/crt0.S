/* Entry, host-call trampoline, setjmp and the memory primitives of the guest
 * (x32: 32-bit pointers, 64-bit registers). */
    .text

/* int ps5_boot(uint32_t stackTop): called by the host on its own stack. Saves
 * the host's registers and stack, runs guest_main on the guest stack, returns
 * guest_main's result. */
    .globl ps5_boot
    .type ps5_boot,@function
ps5_boot:
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    subq $8, %rsp
    movq %rsp, gHostRsp
    movl %edi, %esp
    xorl %ebp, %ebp
    call guest_main
    movq gHostRsp, %rsp
    addq $8, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx
    ret

/* Entered through a stub with the table index in eax. Runs the host function
 * on the host's stack (the driver needs far more than the guest's) and comes
 * back to the guest stack. Arguments are in registers only. */
    .globl __host_call
    .type __host_call,@function
__host_call:
    movq %rsp, %r11
#ifdef PORT_LINUX64_GUEST
    /* Each low worker stack selects its own native host stack. Main is0. */
    movq %r11, %r10
    subq $rk_tstacks, %r10
    cmpq $3670016, %r10 /* seven512KiB stacks */
    jae .Lhost_main_stack
    shrq $19, %r10
    incq %r10
    movq gHostRsp(,%r10,8), %rsp
    jmp .Lhost_stack_ready
.Lhost_main_stack:
#endif
    movq gHostRsp, %rsp
#ifdef PORT_LINUX64_GUEST
.Lhost_stack_ready:
#endif
    pushq %r11
    pushq %r11
    movl %eax, %eax
    call *gHostTable(,%rax,8)
    popq %r11
    popq %r11
    movq %r11, %rsp
    ret

#ifdef PORT_LINUX64_GUEST
/* SysV native callback: enter fn(arg) on its aligned low guest stack. */
    .globl ps5_thread
    .type ps5_thread,@function
ps5_thread:
    pushq %rbx
    pushq %rbp
    pushq %r12
    pushq %r13
    pushq %r14
    pushq %r15
    subq $8, %rsp
    movl %edi, %r12d
    movl %esi, %r13d
    movl %edx, %r15d
    subl $1, %r15d
    subl $rk_tstacks, %r15d
    cmpl $3670016, %r15d
    jae .Lthread_invalid
    shrl $19, %r15d
    incl %r15d
    movq %rsp, gHostRsp(,%r15,8)
    movl %edx, %esp
    xorl %ebp, %ebp
    movl %r13d, %edi
    call *%r12
    movq gHostRsp(,%r15,8), %rsp
    jmp .Lthread_restore
.Lthread_invalid:
    movl $-1, %eax
.Lthread_restore:
    addq $8, %rsp
    popq %r15
    popq %r14
    popq %r13
    popq %r12
    popq %rbp
    popq %rbx
    ret
#endif

    .globl setjmp
    .type setjmp,@function
setjmp:
    movl %edi, %edi
    movq %rbx, 0(%rdi)
    movq %rbp, 8(%rdi)
    movq %r12, 16(%rdi)
    movq %r13, 24(%rdi)
    movq %r14, 32(%rdi)
    movq %r15, 40(%rdi)
    leaq 8(%rsp), %rdx
    movq %rdx, 48(%rdi)
    movq (%rsp), %rdx
    movq %rdx, 56(%rdi)
    xorl %eax, %eax
    ret

    .globl longjmp
    .type longjmp,@function
longjmp:
    movl %edi, %edi
    movl %esi, %eax
    testl %eax, %eax
    jnz 1f
    incl %eax
1:  movq 0(%rdi), %rbx
    movq 8(%rdi), %rbp
    movq 16(%rdi), %r12
    movq 24(%rdi), %r13
    movq 32(%rdi), %r14
    movq 40(%rdi), %r15
    movq 48(%rdi), %rsp
    jmp *56(%rdi)

    .globl memcpy
    .type memcpy,@function
memcpy:
    movl %edi, %edi
    movl %esi, %esi
    movl %edx, %ecx
    movq %rdi, %rax
    rep movsb
    ret

    .globl memset
    .type memset,@function
memset:
    movl %edi, %edi
    movl %edx, %ecx
    movq %rdi, %r8
    movl %esi, %eax
    rep stosb
    movq %r8, %rax
    ret

    .globl memmove
    .type memmove,@function
memmove:
    movl %edi, %edi
    movl %esi, %esi
    movl %edx, %ecx
    movq %rdi, %rax
    cmpq %rsi, %rdi
    jbe 1f
    leaq (%rsi,%rcx), %r8
    cmpq %r8, %rdi
    jae 1f
    leaq -1(%rdi,%rcx), %rdi
    leaq -1(%rsi,%rcx), %rsi
    std
    rep movsb
    cld
    ret
1:  rep movsb
    ret

    .section .note.GNU-stack,"",@progbits
