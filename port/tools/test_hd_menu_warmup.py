"""Small real-cache/lifetime probes; no ROM, player data, visible window or sweep."""
import argparse
from pathlib import Path
import os
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def function(source, marker):
    start = source.index(marker)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=ROOT / 'build/hd-menu-warmup')
    parser.add_argument('--shutdown-ref', help='RED check: old sprite shutdown from this Git revision')
    args = parser.parse_args()
    out = args.out.resolve()
    out.mkdir(parents=True, exist_ok=True)
    tc = ROOT.parent / 'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin'
    sdl = ROOT.parent / 'toolchain/SDL3-3.4.18/i686-w64-mingw32'
    compiler = tc / 'i686-w64-mingw32-clang.exe'
    source = (ROOT / 'port/src/port_hdsprites.c').read_text()
    if args.shutdown_ref:
        source = subprocess.check_output(['git', 'show', args.shutdown_ref + ':port/src/port_hdsprites.c'],
                                         cwd=ROOT, text=True)
    unit = r'''
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef int SDL_Texture;
static int live;
static SDL_Texture* texture(void){ live++; return malloc(sizeof(int)); }
static void SDL_DestroyTexture(SDL_Texture* p){if(p){live--;free(p);}}
#define SDL_free free
#define SDL_memset memset
#define HD_SPRITE_COUNT 2
static struct {SDL_Texture *image,*white;size_t bytes;int present;} sState[2];
static size_t sBytes;
static SDL_Texture *sRingFull,*sRingEmpty,*sArt,*sArtWhite,*sMenuLightImage,*sMenuLightWhite;
static unsigned char *sMenuLightSamples,*sMenuLightPixels;
static int sMenuLightTried,sMenuLightPhase,sRingTried,sArtTried,sArtW,sArtH,sArtCount,sBarCount,sDrawCount[3];
static void* sRenderer;
'''
    for marker in ('static void MenuLightRelease(', 'static void Unload(', 'void PortHdSpritesShutdown('):
        unit += function(source, marker) + '\n'
    unit += r'''
int main(void){
 sArt=texture();sArtWhite=texture();sRingFull=texture();sRingEmpty=texture();
 sMenuLightImage=texture();sMenuLightWhite=texture();
 sMenuLightSamples=malloc(4);sMenuLightPixels=malloc(4);
 sArtTried=sArtW=sArtH=sArtCount=sBarCount=1;sRenderer=(void*)1;
 PortHdSpritesShutdown();
 assert(live==0 && !sArt && !sArtWhite && !sRenderer && !sArtTried && !sArtCount && !sBarCount);
 PortHdSpritesShutdown();assert(live==0);
 puts("PASS poster/light shutdown releases and resets; repeat shutdown safe");return 0;
}
'''
    (out / 'sprites.c').write_text(unit)
    subprocess.run([str(compiler), str(out / 'sprites.c'), '-o', str(out / 'sprites.exe')], check=True)
    subprocess.run([str(out / 'sprites.exe')], check=True, timeout=10)
    unit = '#include "' + (ROOT / 'port/src/port_hdtext.c').as_posix() + '"\n'
    unit += r'''
#include <assert.h>
#include <stdio.h>
PortConfig gPortConfig;
PortStutterProbe gPortStutter;
uint32_t PortFrameCount(void){return 0;}
int PortPpuHudDigits(PortHudDigit* out,int max){return 0;}
int PortPpuHudDigitVisible(int digit,int x,int y){return 0;}
int PortPpuTextGlyphs(PortTextGlyph* out,int max,int hud){return 0;}
const uint16_t* PortLatinGlyphWidths(void){static uint16_t w[256];for(int i=0;i<256;i++)w[i]=6;return w;}
const char* PortConfigPath(const char* name){return name;}
void PortLog(const char* format,...){(void)format;}
static int textures(void){int n=0;for(int j=0;j<2;j++){
 for(int i=0;i<256;i++)n+=sFonts[3][j].glyphs[i].tex!=NULL;
 for(int i=0;i<HD_TEXT_JP_CACHE;i++)n+=sFonts[3][j].jpGlyphs[i].tex!=NULL;
}return n;}
int main(void){
 SDL_SetHint(SDL_HINT_VIDEO_DRIVER,"dummy");assert(SDL_Init(SDL_INIT_VIDEO));
 SDL_Surface* surface=SDL_CreateSurface(240,160,SDL_PIXELFORMAT_XRGB8888);assert(surface);
 SDL_Renderer* renderer=SDL_CreateSoftwareRenderer(surface);assert(renderer);
 gPortConfig.hdFont=2;
 PortHdTextWarmUi(renderer,3.375f);assert(textures()==0);
 gPortConfig.hdTitle=1;
 PortHdTextWarmUi(renderer,3.375f);int first=textures();assert(first>80);
 PortHdTextWarmUi(renderer,3.375f);assert(textures()==first);
 PortHdTextWarmUi(renderer,6.75f);int both=textures();assert(both>first);
 PortHdTextWarmUi(renderer,3.375f);assert(textures()==both);
 FontForScale(4.0f,0);FontForScale(5.0f,0);
 int kept=0;for(int s=0;s<(int)(sizeof(sFonts)/sizeof(sFonts[0]));s++)
  for(int j=0;j<2;j++)kept|=sFonts[s][j].font && sFonts[s][j].key%100000==54;
 assert(kept); /* gameplay font churn must not invalidate the prepared menu */
 PortHdTextWarmUi(renderer,6.75f);PortHdTextWarmUi(renderer,3.375f);
 assert(textures()==both); /* title -> Options remains hot after gameplay */
 PortHdTextShutdown();assert(textures()==0 && !sDc && !sCustomTried);
 PortHdTextShutdown();
 PortHdTextWarmUi(renderer,3.375f);assert(textures()==first);
 PortHdTextShutdown();SDL_DestroyRenderer(renderer);SDL_DestroySurface(surface);SDL_Quit();
 printf("PASS HD-off skips font cache; idempotent warmup textures=%d/%d; shutdown/reload safe\n",first,both);
 return 0;
}
'''
    (out / 'fonts.c').write_text(unit)
    subprocess.run([str(compiler), '-std=gnu11', '-I' + str(ROOT / 'port/include'), '-I' + str(sdl / 'include'),
                    str(out / 'fonts.c'), '-L' + str(sdl / 'lib'), '-lSDL3', '-lgdi32', '-o', str(out / 'fonts.exe')], check=True)
    dll = out / 'SDL3.dll'
    if not dll.exists():
        import shutil
        shutil.copy2(sdl / 'bin/SDL3.dll', dll)
    subprocess.run([str(out / 'fonts.exe')], cwd=out, check=True, timeout=30)


if __name__ == '__main__':
    main()
