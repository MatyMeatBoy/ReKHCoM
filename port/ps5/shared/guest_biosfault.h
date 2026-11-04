/* The original game reads (and rarely writes) through NULL in a few places: on the GBA those accesses hit the BIOS
 * area, reads see its open-bus value and writes are ignored. The native 32-bit build redirects them in
 * port/src/port_biosfault.c; this is the x86-64 counterpart for the x32 guest hosts (issue #564: a sleight name
 * whose OBJ tiles failed to allocate crashed the 64-bit game where the 32-bit one went on).
 *
 * Only an access by guest code to the first 64 KiB is handled. Its memory operand is decoded, the effective address
 * must be the faulting one, and the base (or index) register is moved by PORT_GUEST_BIOS_STANDIN for one single
 * step, so the access lands in a stand-in of the BIOS area; the register is put back after the step unless the
 * instruction replaced it. Fault-time only: no logging, allocation, CRT or OS calls beyond the host's own. */
#ifndef REKHCOM_GUEST_BIOSFAULT_H
#define REKHCOM_GUEST_BIOSFAULT_H
#include <stdint.h>
#include "guest_divzero.h"

#define PORT_GUEST_BIOS_STANDIN 0x01FF0000u /* 64 KiB right below the GBA's EWRAM, mapped by the host */
#define PORT_GUEST_BIOS_SIZE 0x10000u
#define PORT_GUEST_BIOS_WORD 0xE3A02004u /* the open-bus word the BIOS leaves (port_memory_ps5.c) */

static void PortGuestBiosFill(void) {
    volatile uint32_t* p = (volatile uint32_t*)(uintptr_t)PORT_GUEST_BIOS_STANDIN;
    unsigned i;
    for (i = 0; i < PORT_GUEST_BIOS_SIZE / 4; i++)
        p[i] = PORT_GUEST_BIOS_WORD;
}

/* One-byte opcodes followed by a ModRM byte (x86-64: no 62/C4/C5, those are EVEX/VEX prefixes). */
static int PortGuestBiosModRm1(unsigned op) {
    if (op < 0x40)
        return (op & 7) < 4;
    switch (op) {
        case 0x63: case 0x69: case 0x6B: case 0xC0: case 0xC1: case 0xC6: case 0xC7: case 0xD0: case 0xD1:
        case 0xD2: case 0xD3: case 0xF6: case 0xF7: case 0xFE: case 0xFF:
            return 1;
    }
    return (op >= 0x80 && op <= 0x8F) || (op >= 0xD8 && op <= 0xDF);
}

/* Two-byte (0F xx) opcodes without a ModRM byte. */
static int PortGuestBiosNoModRm2(unsigned op) {
    return (op >= 0x05 && op <= 0x0B) || op == 0x0E || (op >= 0x30 && op <= 0x37) || op == 0x77
           || (op >= 0x80 && op <= 0x8F) || (op >= 0xA0 && op <= 0xA2) || (op >= 0xA8 && op <= 0xAA)
           || op >= 0xC8;
}

/* The register (0..15, RAX..R15) to move and by how much, so the access at `fault` by the instruction at `rip`
   lands at PORT_GUEST_BIOS_STANDIN + fault. 0 when the instruction is not one this can redirect exactly. */
static int PortGuestBiosDecode(uintptr_t rip, PortGuestDivisionBounds bounds, const uint64_t regs[16],
                               uint64_t fault, int* reg, uint64_t* delta) {
    const unsigned char* code;
    size_t available;
    unsigned at = 0, rex = 0, address32 = 0, op, modrm, mod, rm;
    int base = -1, index = -1;
    unsigned scale = 1;
    uint64_t address = 0;

    if (!bounds.begin || rip < bounds.begin || rip >= bounds.end || fault >= PORT_GUEST_BIOS_SIZE)
        return 0;
    code = (const unsigned char*)rip;
    available = bounds.end - rip;
    if (available > 15)
        available = 15;
    while (at < available) {
        unsigned p = code[at];
        if (p >= 0x40 && p <= 0x4F) rex = p;
        else if (p == 0x67) { address32 = 1; rex = 0; }
        else if (p == 0x66 || p == 0xF2 || p == 0xF3 || p == 0xF0 || p == 0x2E || p == 0x3E || p == 0x26 || p == 0x36)
            rex = 0;
        else
            break; /* FS/GS (64/65) and anything else end the prefixes; 64/65 are then refused as opcodes */
        at++;
    }
    if (at >= available)
        return 0;
    op = code[at++];
    if ((op >= 0xA4 && op <= 0xA7) || (op >= 0xAA && op <= 0xAF)) { /* string instructions: RSI source, RDI destination */
        base = (op == 0xAA || op == 0xAB || op == 0xAE || op == 0xAF) ? 7 : 6;
        address = regs[base];
        if (address32)
            address = (uint32_t)address;
        if (fault - address >= 64)
            return 0;
        *reg = base;
        *delta = PORT_GUEST_BIOS_STANDIN;
        return 1;
    }
    if (op == 0x0F) {
        if (at >= available)
            return 0;
        op = code[at++];
        if (op == 0x38 || op == 0x3A)
            at++;
        else if (PortGuestBiosNoModRm2(op))
            return 0;
    } else if (!PortGuestBiosModRm1(op)) {
        return 0;
    }
    if (at >= available)
        return 0;
    modrm = code[at++];
    mod = modrm >> 6;
    rm = modrm & 7;
    if (mod == 3)
        return 0;
    if (rm == 4) {
        unsigned sib, b, ix;
        if (at >= available)
            return 0;
        sib = code[at++];
        scale = 1u << (sib >> 6);
        ix = ((sib >> 3) & 7) | ((rex & 2) ? 8 : 0);
        b = (sib & 7) | ((rex & 1) ? 8 : 0);
        if (ix != 4)
            index = (int)ix;
        if (!((sib & 7) == 5 && mod == 0))
            base = (int)b;
        else if (available - at < 4)
            return 0;
        else
            address = (uint64_t)(int64_t)(int32_t)((uint32_t)code[at] | (uint32_t)code[at + 1] << 8 |
                                                   (uint32_t)code[at + 2] << 16 | (uint32_t)code[at + 3] << 24);
    } else if (rm == 5 && mod == 0) {
        return 0; /* RIP-relative: never a NULL access */
    } else {
        base = (int)(rm | ((rex & 1) ? 8 : 0));
    }
    if (mod == 1) {
        if (at >= available)
            return 0;
        address += (uint64_t)(int64_t)(int8_t)code[at];
    } else if (mod == 2) {
        if (available - at < 4)
            return 0;
        address += (uint64_t)(int64_t)(int32_t)((uint32_t)code[at] | (uint32_t)code[at + 1] << 8 |
                                                (uint32_t)code[at + 2] << 16 | (uint32_t)code[at + 3] << 24);
    }
    if (base >= 0)
        address += regs[base];
    if (index >= 0)
        address += regs[index] * scale;
    if (address32)
        address = (uint32_t)address;
    if (fault - address >= 64) /* the faulting access must be this operand (a wide one may fault past its start) */
        return 0;
    if (base == 4 || (base < 0 && index < 0))
        return 0; /* the stack pointer is never moved; an absolute address has no register */
    if (base >= 0 && base != index) {
        *reg = base;
        *delta = PORT_GUEST_BIOS_STANDIN;
        return 1;
    }
    if (index >= 0 && base < 0 && PORT_GUEST_BIOS_STANDIN % scale == 0) {
        *reg = index;
        *delta = PORT_GUEST_BIOS_STANDIN / scale;
        return 1;
    }
    return 0;
}
#endif
