/* Pending Atlas approval: replacement FloorMotifIndex, not active gameplay. */
#include "oogie_ghost_contours.inc"
static int OogieFloorProposal(int x, int y) {
    if (y >= 472) {
        int depth = y - 472;
        /* The green carpet continues down both sides, narrowing with depth.
         * The live renderer shades it; no motif is sampled down the wall. */
        return depth < 64 && x >= 224 + depth / 2 && x < 528 - depth / 2 ? 159 : 0;
    }
    if (y < 408) return 0;
    if (x >= 224 && x < 256) return kOogieGhostLeft[y - 408][x - 224];
    if (x >= 496 && x < 528) return kOogieGhostRight[y - 408][x - 496];
    return 0; /* authored room pixels retain priority */
}
