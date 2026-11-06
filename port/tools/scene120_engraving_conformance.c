/* Coordinates and source indices only; no cartridge bytes in this fixture. */
#include <assert.h>
#include "port/port_scene120_engraving.h"
int main(void) {
    int x,y,i;
    assert(PortScene120EngravingEntry(96,-32)==(0x4000|564));
    assert(PortScene120EngravingEntry(128,-48)==(0x4000|504));
    assert(PortScene120EngravingEntry(96,-1)==(0x4000|660));
    for (y=-64;y<256;y++) for (x=-32;x<288;x++) {
        int entry=PortScene120EngravingEntry(x,y);
        int selected=x>=96 && x<160 && y>=-48 && y<0 && (x>=128 || y>=-32);
        assert((entry>=0)==selected);
        if(selected) assert((entry>>12)==4 && (entry&1023)>=500 && (entry&1023)<668);
    }
    for(i=-1;i<=16;i++) assert(!!PortScene120EngravingInk(i)==(i==8 || i==10 || i==11 || i==12));
    return 0;
}
