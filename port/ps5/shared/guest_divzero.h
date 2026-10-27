/* Cartridge __{u,}{div,mod}si3 zero-divisor compatibility for the x32 image.
 * No instruction is changed on the normal path. Only fault-time register
 * DIV/IDIV r32 with a proven zero operand can be emulated. */
#ifndef REKHCOM_GUEST_DIVZERO_H
#define REKHCOM_GUEST_DIVZERO_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define PORT_GUEST_DIVISION_MAGIC 0x31564447u /* GDV1, optional after the original 36-byte header */
typedef struct PortGuestDivisionBounds { uintptr_t begin, end; } PortGuestDivisionBounds;

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
 * Memory operands, 8/16/64-bit divisions, noncanonical prefixes and all
 * nonzero operands (including INT_MIN/-1 overflow) stay real exceptions. */
static unsigned PortGuestDivisionLength(uintptr_t rip, PortGuestDivisionBounds bounds, const uint64_t regs[16]) {
    const unsigned char* code;
    size_t available;
    unsigned at = 0, high = 0, operand, modrm;
    if (!bounds.begin || rip < bounds.begin || rip >= bounds.end) return 0;
    code = (const unsigned char*)rip;
    available = bounds.end - rip;
    if (code[0] == 0x40 || code[0] == 0x41) { high = (code[0] & 1) * 8; at++; }
    if (available - at < 2 || code[at] != 0xf7) return 0;
    modrm = code[at + 1];
    if ((modrm & 0xc0) != 0xc0 || ((modrm >> 3) & 7) < 6) return 0;
    operand = (modrm & 7) + high;
    return (uint32_t)regs[operand] == 0 ? at + 2 : 0;
}
#endif
