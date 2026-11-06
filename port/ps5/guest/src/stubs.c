/* One stub per host function: loads the table index and jumps to the
 * trampoline. The index is the entry's position in bridge_list.h, the same
 * order as the host's table (HF_*). */
#include "bridge.h"

#define STR2(x) #x
#define STR(x) STR2(x)
#define X(name) \
    __asm__(".text\n.globl h_" #name "\n.type h_" #name ",@function\nh_" #name ":\n\tmovl $" STR(__COUNTER__) ", %eax\n\tjmp __host_call\n");
#include "bridge_list.h"
#undef X

uint64_t gHostTable[HF_COUNT];
#ifdef PORT_LINUX64_GUEST
uint64_t gHostRsp[8]; /* main + seven low-stack PPU workers */
#else
uint64_t gHostRsp;
#endif
