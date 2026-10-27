"""Real UpdateMapcardGfx must tolerate the native allocator returning no palette.

The original GBA reads BIOS/open bus at NULL+6; native 64-bit hosts fault.
No GPU/ROM or signal recovery is used in this regression.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT=Path(__file__).resolve().parents[2]


def main():
    source=(ROOT/'src/card/card_worldselect.c').read_text(encoding='utf-8')
    start=source.index('void UpdateMapcardGfx(MapcardWork* work) {')
    body=source[start:source.index('\n}',start)+2]
    code=r'''
#include <stdint.h>
#include <string.h>
#include <windows.h>
typedef uint16_t u16;
typedef struct { int pad; unsigned short count,index; } ObjPalette;
typedef struct { void* tiles; void* palette; int tilesSize; } MapCardDef;
typedef MapCardDef MapCardBackDef;
typedef struct { MapCardDef* cardDef; MapCardBackDef* cardBack; void* tiles,*tiles2,*tiles3; ObjPalette *palette,*palette2; u16 flags; } MapcardWork;
static unsigned char gPortBiosArea[64] __attribute__((aligned(16)));
#define BIOS_PTR(p) ((p)!=0?(p):(__typeof__(p))(void*)gPortBiosArea)
#define TRUE 1
#define MAPCARD_FLAG_GFX_LOADED 1
static int gCardValueDigitTiles[4], calls, mode, excluded[32];
static ObjPalette palette={0,1,4};
static int IsMapcardOnScreen(MapcardWork* w) { return 1; }
static void* LoadObjTiles(void* p,int size) {return (void*)1;}
static ObjPalette* LoadObjPalette(void* p,int size) {calls++; return mode==0 || (mode==1 && calls==1)?0:&palette;}
static void FadeSetPaletteExcluded(unsigned slot,int value) {if(slot<32)excluded[slot]++;}
static void ReleaseObjTiles(void* p){}
static void ReleaseObjPalette(ObjPalette* p){}
'''+body+r'''
int main(void) {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    MapCardDef def={0,0,32}; MapcardWork w={0}; w.cardDef=&def;w.cardBack=&def;
    memset(gPortBiosArea,0xcc,sizeof(gPortBiosArea));
    for(mode=0;mode<3;mode++) {
        w.flags=0;calls=0;memset(excluded,0,sizeof(excluded));
        UpdateMapcardGfx(&w);
        if(!(w.flags&1) || excluded[20]!=mode)return 1;
    }
    return 0;
}
'''
    cc=next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
    with tempfile.TemporaryDirectory(prefix='rekhcom-mapcard-palette-') as name:
        folder=Path(name); src=folder/'test.c';exe=folder/'test.exe'
        src.write_text(code)
        subprocess.run([str(cc),str(src),'-o',str(exe)],check=True)
        result=subprocess.run([str(exe)],timeout=10,creationflags=0x08004000)
        assert result.returncode==0, f'palette exhaustion crashed/changed valid palette behavior: {result.returncode}'
    print('PASS: mapcard palette exhaustion follows GBA BIOS semantics; valid palettes unchanged')


if __name__=='__main__':
    main()
