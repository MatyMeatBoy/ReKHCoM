#include <assert.h>
#include <stdio.h>
#include "port/port_preliminar_v1_tiles.h"

int main(void) {
    const PortPreliminarRecipe* trees = PortPreliminarFind(0x56184dbeu, 2, 2, 2);
    const PortPreliminarRecipe* grass = PortPreliminarFind(0xe030d62bu, 3, 2, 2);
    assert(trees && grass);

    for (int y = -16; y < 81; y++) {
        for (int x = -16; x < 81; x++) {
            int sx = x, sy = y;
            int accepted = PortPreliminarTileSourceForPreview(trees, &sx, &sy, 0);
            assert(accepted == (x >= 8 && x < 48 && y >= 0 && y < 40));
            assert(sx == x && sy == y);

            accepted = PortPreliminarTileSourceForPreview(trees, &sx, &sy, 1);
            assert(accepted == (x >= 0 && x < 64 && y >= 0 && y < 64));
            assert(sx == x && sy == y);

            int gx = x, gy = y;
            int grassAccepted = PortPreliminarTileSource(grass, &gx, &gy);
            assert(PortPreliminarTileSourceForPreview(grass, &sx, &sy, 1) == grassAccepted);
            assert(sx == gx && sy == gy);
        }
    }
    puts("Scene 097: approved BG2 bounds, full preview BG2 and identical BG3 PASS");
    return 0;
}
