/* Native MS64 caller verifies the nonvolatiles absent from SysV.
   Stack alignment and 32-byte shadow space match a normal Windows caller. */
.text
.globl Windows64CheckPreserved
Windows64CheckPreserved:
    pushq %rdi
    pushq %rsi
    subq $72, %rsp
    movdqu %xmm6, 32(%rsp)
    movdqu %xmm15, 48(%rsp)
    movq $0x12345678, %rdi
    movq $0x23456789, %rsi
    pcmpeqd %xmm6, %xmm6
    pcmpeqd %xmm15, %xmm15
    movl $0x3456789a, 64(%rsp)
    call Windows64InvokeProbe
    xorl %eax, %eax
    cmpq $0x12345678, %rdi
    jne 1f
    cmpq $0x23456789, %rsi
    jne 1f
    pmovmskb %xmm6, %edx
    cmpl $65535, %edx
    jne 1f
    pmovmskb %xmm15, %edx
    cmpl $65535, %edx
    jne 1f
    cmpl $0x3456789a, 64(%rsp)
    jne 1f
    movl $1, %eax
1:
    movdqu 32(%rsp), %xmm6
    movdqu 48(%rsp), %xmm15
    addq $72, %rsp
    popq %rsi
    popq %rdi
    ret
