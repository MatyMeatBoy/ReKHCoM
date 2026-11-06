#include <assert.h>
#include "port/port_preliminar_v1_tiles.h"
int main(void) {
    PortPreliminarRecipe trees={0,{97,2,2,2}};
    int x,y,sx,sy,selected;
    for(y=-16;y<81;y++) for(x=-16;x<81;x++) {
        sx=x;sy=y;
        selected=PortPreliminarTileSource(&trees,&sx,&sy);
        assert(selected==(x>=0 && x<64 && y>=0 && y<64));
        if(selected) assert(sx==x && sy==y);
    }
    return 0;
}
