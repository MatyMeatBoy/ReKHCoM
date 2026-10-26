"""Render the production GBA button with the user's ROM glyphs, with/without HD collection.

Requires configured reference builds (their symbol files) and original ROMs. For example:
    python port/tools/test_gba_key.py --reference-root . --regions eu us jp
Generated ROM-derived fixtures and BMP evidence stay in build/gba-key-test.
"""
import argparse
from pathlib import Path
import os
import re
import shutil
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    start = source.index('static void ' + name + '(')
    return source[start:source.index('\n}\n', start) + 3]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--reference-root', type=Path, default=ROOT)
    parser.add_argument('--regions', nargs='+', choices=['eu', 'us', 'jp'], default=['eu', 'us', 'jp'])
    args = parser.parse_args()
    out = ROOT / 'build/gba-key-test'
    out.mkdir(parents=True, exist_ok=True)
    cc = next((ROOT.parent / 'toolchain').glob('llvm-mingw-*/bin/i686-w64-mingw32-clang.exe'))
    sdl = ROOT.parent / 'toolchain/SDL3-3.4.18/i686-w64-mingw32'
    shutil.copy2(sdl / 'bin/SDL3.dll', out / 'SDL3.dll')
    ui = (ROOT / 'port/src/port_ui.c').read_text(encoding='utf-8')
    for region in args.regions:
        symbols = (args.reference_root / f'build/{region}/pc/rom_symbols.s').read_text()
        rom = (args.reference_root / 'roms' / {'eu': 'B8CP.gba', 'us': 'B8CE.gba', 'jp': 'B8CJ.gba'}[region]).read_bytes()
        def address(name):
            return int(re.search(r'\.set _' + name + r', (0x[0-9a-fA-F]+)', symbols).group(1), 16) - 0x08000000
        tiles_base, table = address('gMsgLatinFontTiles'), address('gMsgLatinFontFrames')
        tiles = bytearray()
        for code in (ord('A'), ord('B')):
            frame = struct.unpack_from('<I', rom, table + code * 4)[0] - 0x08000000
            tile = struct.unpack_from('<H', rom, frame + 6)[0] & 1023
            tiles.extend(rom[tiles_base + tile * 32:tiles_base + tile * 32 + 128])
        unit = r'''
#include <SDL3/SDL.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint32_t image[40*24], baseline[40*24];
static uint32_t* sFb=image;
static uint32_t* sHdUiFrame;
static int sHdUiCount,sHdUiRun,sHdUiInText;
static struct {int code,x,y,alive,pw,run;uint32_t face,shadow;} sHdUiGlyphs[2048];
static void Pixel(int x,int y,uint32_t c){if(x>=0&&x<40&&y>=0&&y<24)image[y*40+x]=c;}
static int GlyphWidth(int code){(void)code;return 6;}
static const uint16_t a[]={1,0,0,0},b[]={1,0,0,4};
static const void* gMsgLatinFontFrames[256]={[65]=a,[66]=b};
'''
        unit += 'static const uint8_t gMsgLatinFontTiles[]={' + ','.join(map(str, tiles)) + '};\n'
        unit += function(ui, 'DrawGlyph') + '\n' + function(ui, 'GbaKey')
        unit += r'''
static void shot(const char* name){
    SDL_Surface* s=SDL_CreateSurfaceFrom(40,24,SDL_PIXELFORMAT_ARGB8888,image,40*4);
    assert(s);assert(SDL_SaveBMP(s,name));SDL_DestroySurface(s);
}
int main(void){
    for(int hd=0;hd<2;hd++){
        memset(image,0,sizeof(image));sHdUiFrame=hd?image:NULL;sHdUiCount=0;
        GbaKey('A',10,12);GbaKey('B',29,12);
        shot(hd?"key-hd.bmp":"key-gba.bmp");
        assert(sHdUiCount==0); /* key letter is part of the raster sprite, not an HD text run */
        for(int center=10;center<30;center+=19){
            int x0=40,y0=24,x1=-1,y1=-1;
            for(int y=0;y<24;y++)for(int x=center-6;x<=center+6;x++)if(image[y*40+x]==0xFFF4F0FCu){
                if(x<x0)x0=x;if(x>x1)x1=x;if(y<y0)y0=y;if(y>y1)y1=y;
            }
            assert(x1>=x0&&y1>=y0);
            assert(abs(x0+x1-2*center)<=1&&abs(y0+y1-24)<=1);
        }
        if(!hd)memcpy(baseline,image,sizeof(image));
        else assert(!memcmp(baseline,image,sizeof(image)));
    }
    puts("GBA A/B: visible letter centred, HD/GBA pixels identical and no deferred HD glyph PASS");return 0;
}
'''
        source = out / f'key-{region}.c'
        source.write_text(unit, encoding='utf-8')
        exe = out / f'key-{region}.exe'
        subprocess.run([str(cc), '-O2', '-std=gnu11', '-I' + str(sdl / 'include'), str(source),
                        '-L' + str(sdl / 'lib'), '-lSDL3', '-o', str(exe)], check=True)
        subprocess.run([str(exe)], cwd=out, env=dict(os.environ, SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy'), check=True)
        for mode in ['gba', 'hd']:
            shutil.copy2(out / f'key-{mode}.bmp', out / f'{region}-{mode}.bmp')
        print(region + ': PASS')


if __name__ == '__main__':
    main()
