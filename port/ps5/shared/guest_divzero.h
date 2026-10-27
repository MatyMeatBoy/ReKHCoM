/* Cartridge __{u,}{div,mod}si3 zero-divisor compatibility for the x32 image.
 * No instruction is changed on the normal path. At fault time, prove a zero
 * register/mapped-memory operand before reproducing the cartridge result. */
#ifndef REKHCOM_GUEST_DIVZERO_H
#define REKHCOM_GUEST_DIVZERO_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define PORT_GUEST_DIVISION_MAGIC 0x31564447u /* GDV1, optional after the original 36-byte header */
typedef struct PortGuestDivisionBounds { uintptr_t begin, end; } PortGuestDivisionBounds;
#define PORT_GUEST_DIVISION_FULL 1
typedef struct PortGuestDivisionMemory {
    PortGuestDivisionBounds range[9];
    unsigned count;
} PortGuestDivisionMemory;
typedef struct PortGuestDivisionInstruction { unsigned length, bytes; } PortGuestDivisionInstruction;

/* These are the regions both x32 hosts actually map before boot. No host/DLL
 * memory is readable by the decoder. Build the descriptor only after ALL maps
 * succeed; the last mapping includes guest data, heap and its own stack. */
static PortGuestDivisionMemory PortGuestDivisionMappedMemory(uintptr_t begin, uintptr_t end) {
    PortGuestDivisionMemory memory = {{{0x02000000,0x02040000},{0x03000000,0x03010000},
        {0x04000000,0x04010000},{0x05000000,0x05010000},{0x06000000,0x06020000},
        {0x07000000,0x07010000},{0x08000000,0x0a000000},{0x0e000000,0x0e010000},{0,0}},0};
    if (begin && end > begin) { memory.range[8].begin=begin; memory.range[8].end=end; memory.count=9; }
    return memory;
}
static int PortGuestDivisionRead(const PortGuestDivisionMemory* memory, uint64_t address,
                                 unsigned bytes, uint64_t* value) {
    unsigned i, j;
    if (!memory || memory->count > 9 || address > UINTPTR_MAX) return 0;
    for (i=0;i<memory->count;i++) {
        const PortGuestDivisionBounds* range=&memory->range[i];
        if (address >= range->begin && address <= range->end && bytes <= range->end-address) {
            const volatile unsigned char* p=(const volatile unsigned char*)(uintptr_t)address;
            *value=0;
            for (j=0;j<bytes;j++) *value |= (uint64_t)p[j] << (j*8);
            return 1;
        }
    }
    return 0;
}

static PortGuestDivisionBounds PortGuestDivisionImage(const void* image, size_t size, uintptr_t base,
                                                     uintptr_t imageEnd, uintptr_t boot) {
    PortGuestDivisionBounds bounds = {0, 0};
    uint32_t meta[3];
    if (size < 48 || size > UINTPTR_MAX - base) return bounds;
    memcpy(meta, (const unsigned char*)image + 36, sizeof(meta));
    if (meta[0] != PORT_GUEST_DIVISION_MAGIC || meta[1] < base + 48 || meta[1] >= meta[2] ||
        meta[2] > base + size || meta[2] > imageEnd || boot < meta[1] || boot >= meta[2]) return bounds;
    bounds.begin = meta[1]; bounds.end = meta[2];
    return bounds;
}

/* Register order is the x86 encoding: AX,CX,DX,BX,SP,BP,SI,DI,R8..R15.
 * Address calculation follows long mode, including address32 EIP-relative and
 * SIB rules. FS/GS are rejected: their host TLS bases are outside the flat GBA
 * contract. Nonzero divisors/overflow and unrelated opcodes remain faults. */
static PortGuestDivisionInstruction PortGuestDivisionDecode(uintptr_t rip, PortGuestDivisionBounds bounds,
        const uint64_t regs[16], const PortGuestDivisionMemory* memory) {
    PortGuestDivisionInstruction no={0,0}, result;
    const unsigned char* code;
    size_t available;
    unsigned at=0, rex=0, word=0, address32=0, opcode, modrm, mod, rm, bytes;
    uint64_t value=0, address=0, index=0;
    int relative=0;
    if (!bounds.begin || rip < bounds.begin || rip >= bounds.end) return no;
    code = (const unsigned char*)rip;
    available = bounds.end - rip;
    if (available > 15) available=15;
    while (at<available) {
        unsigned p=code[at];
        if (p>=0x40 && p<=0x4f) rex=p;
        else if (p==0x66) { word=1; rex=0; }
        else if (p==0x67) { address32=1; rex=0; }
        else if (p==0x2e || p==0x36 || p==0x3e || p==0x26 || p==0xf2 || p==0xf3) rex=0;
        else break;
        at++;
    }
    if (available-at<2) return no;
    opcode=code[at++];modrm=code[at++];
    if ((opcode!=0xf6 && opcode!=0xf7) || ((modrm>>3)&7)<6) return no;
    bytes=opcode==0xf6?1:(rex&8)?8:word?2:4;
    mod=modrm>>6;rm=modrm&7;
    if (mod==3) {
        if (bytes==1 && !rex && rm>=4) value=regs[rm-4]>>8;
        else value=regs[rm+((rex&1)?8:0)];
    } else {
        unsigned displacement=mod==1?1:mod==2?4:0;
        if (rm==4) {
            unsigned sib, base, ix;
            if (at==available) return no;
            sib=code[at++];base=sib&7;ix=(sib>>3)&7;
            if (ix!=4 || (rex&2)) index=regs[ix+((rex&2)?8:0)] << (sib>>6);
            if (mod==0 && base==5) displacement=4;
            else address=regs[base+((rex&1)?8:0)];
        } else if (mod==0 && rm==5) { displacement=4;relative=1; }
        else address=regs[rm+((rex&1)?8:0)];
        if (displacement>available-at) return no;
        if (displacement==1) address+=(uint64_t)(int64_t)(int8_t)code[at++];
        else if (displacement==4) {
            uint32_t d=(uint32_t)code[at]|(uint32_t)code[at+1]<<8|(uint32_t)code[at+2]<<16|(uint32_t)code[at+3]<<24;
            address+=(uint64_t)(int64_t)(int32_t)d;at+=4;
        }
        address+=index;
        if (relative) address+=(uint64_t)rip+at;
        if (address32) address=(uint32_t)address;
        if (!PortGuestDivisionRead(memory,address,bytes,&value)) return no;
    }
    if (bytes<8) value &= ((uint64_t)1<<(bytes*8))-1;
    if (value) return no;
    result.length=at;result.bytes=bytes;return result;
}
static void PortGuestDivisionResult(unsigned bytes, uint64_t* ax, uint64_t* dx) {
    if (bytes==1) *ax &= ~UINT64_C(0xffff); /* AL quotient, AH remainder */
    else if (bytes==2) { *ax &= ~UINT64_C(0xffff);*dx &= ~UINT64_C(0xffff); }
    else { *ax=0;*dx=0; } /* EAX/EDX writes also zero-extend. */
}
#endif
