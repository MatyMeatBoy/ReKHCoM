#ifndef PORT_OAM_H
#define PORT_OAM_H

#include <stdint.h>

/* The cartridge's OAM and SpriteWork ABI stay 128 entries. Expanded views
 * can expose additional parts of those entries; store them outside GBA RAM.
 * The renderer has an explicit 512-part budget, not an unbounded allocation. */
#define PORT_OAM_NATIVE 128
#define PORT_OAM_CAPACITY 512
#define PORT_OAM_SLOT_MASK (PORT_OAM_CAPACITY - 1)
#define PORT_OAM_PRIORITY_SHIFT 9
#if PORT_OAM_CAPACITY != (1 << PORT_OAM_PRIORITY_SHIFT) || 4 * PORT_OAM_CAPACITY >= 65535
#error OAM occupancy keys require a matching power-of-two capacity below the empty sentinel
#endif

static inline uint16_t* PortOamSlot(uint16_t* native, uint16_t* extra, int slot) {
    if (slot < 0 || slot >= PORT_OAM_CAPACITY) return 0;
    return slot < PORT_OAM_NATIVE ? native + slot * 4 : extra + (slot - PORT_OAM_NATIVE) * 4;
}

uint16_t* PortOamPartBuffer(int slot);
void PortOamTagsReset(void);

#endif
