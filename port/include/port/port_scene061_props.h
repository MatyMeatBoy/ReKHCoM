#ifndef PORT_SCENE061_PROPS_H
#define PORT_SCENE061_PROPS_H
/* ROM placements: (16,160), (80,192), (144,224), (208,256).
 * Exclude the four authored slots: they keep their original draw route. */
static inline int PortScene061PropPosition(int slot, int* x, int* y) {
    if (slot >= 0 && slot < 4) return 0;
    *x = 16 + 64 * slot;
    *y = 160 + 32 * slot;
    return 1;
}
#endif
