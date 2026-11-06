#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif
#include "port/port_scene130_library.h"

int main(int argc,char** argv) {
    int x,y,sx,sy,tx,ty;
    PortScene130LibrarySource(192,321,&sx,&sy);
    assert(sx==288 && sy==273); /* floor division when u-v is negative */
    assert(!PortScene130LibrarySource(0,296,&sx,&sy));
    assert(sx==64 && sy==264); /* complete left baseboard before carpet */
    assert(!PortScene130LibrarySource(-8,304,&sx,&sy));
    assert(sx==120 && sy==240);
    assert(PortScene130LibrarySource(0,312,&sx,&sy)); /* diagonal624 is floor */
    for (y=0;y<200;y+=7) {
        PortScene130LibrarySource(-128,y,&sx,&sy);
        PortScene130LibrarySource(-192,y+32,&tx,&ty);
        assert(sx==tx && sy==ty);
        PortScene130LibrarySource(768,y,&sx,&sy);
        PortScene130LibrarySource(832,y+32,&tx,&ty);
        assert(sx==tx && sy==ty);
    }
    for (y=640;y<768;y+=7) for (x=-256;x<768;x+=11) {
        PortScene130LibrarySource(x,y,&sx,&sy);
        PortScene130LibrarySource(x+96,y+48,&tx,&ty);
        assert(sx==tx && sy==ty);
        PortScene130LibrarySource(x+96,y-48,&tx,&ty);
        assert(sx==tx && sy==ty);
        assert(sx>=192 && sx<384 && sy>=272 && sy<368);
    }
    if (argc>1 && !strcmp(argv[1],"--dump")) {
#ifdef _WIN32
        _setmode(_fileno(stdout),0x8000); /* binary mode, no CRLF translation */
#endif
        for(y=-128;y<640;y++) for(x=-256;x<768;x++) {
            int16_t pair[2];
            PortScene130LibrarySource(x,y,&sx,&sy);
            assert(sx>=0 && sy>=0 && sx<512 && sy<512);
            pair[0]=(int16_t)sx;pair[1]=(int16_t)sy;
            assert(fwrite(pair,sizeof(pair),1,stdout)==1);
        }
    } else puts("PASS library source: carpet lattice, shelf diagonals, signed floor division");
    return 0;
}
