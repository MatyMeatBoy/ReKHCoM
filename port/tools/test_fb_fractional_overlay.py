"""Exercise the real direct overlay against SDL's nearest/alpha arithmetic."""
from pathlib import Path
import subprocess
import sys
import tempfile
import os

root = Path(__file__).resolve().parents[2]
source = (root / 'port/linux/fbdev_linux.c').read_text()


def function(signature):
    start = source.index(signature)
    opening = source.index('{', start)
    depth = 1
    for i in range(opening + 1, len(source)):
        depth += (source[i] == '{') - (source[i] == '}')
        if not depth:
            return source[start:i + 1]
    raise ValueError(signature)


program = r'''
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#define SDL_getenv getenv
typedef struct { int x,y,w,h; } SDL_Rect;
typedef struct { void *pixels; int pitch; } SDL_Surface;
typedef struct { int unused; } SDL_Renderer;
static SDL_Surface *sRenderSurface;
static bool PortFbBeginGlyphBatch(SDL_Renderer *renderer) { (void)renderer; return true; }
static void PortLog(const char *fmt, ...) { (void)fmt; }
''' + function('static uint32_t ScaleGlyphRgb(uint32_t rgb, unsigned factor) {') + '\n' + function('static void BlendOverlayRowGroups(') + '\n' + function('static bool UseOverlayRowGroups(') + '\n' + function('bool PortFbBlendOverlayScaled(') + '\n' + function('bool PortFbBlendOverlayScaledSparse(') + r'''
static unsigned rng=7331;
static unsigned rnd(void) { rng=rng*1664525u+1013904223u; return rng; }
static unsigned channel(unsigned s,unsigned d,unsigned a) { return s*a/255+d*(255-a)/255; }
int main(void) {
    uint32_t *pixels=malloc(640*480*4), *expected=malloc(640*480*4), *src=malloc(240*160*4);
    SDL_Surface surface={pixels,640*4}; sRenderSurface=&surface;
    if (!pixels || !expected || !src) return 2;
    SDL_Rect invalid={-1,0,1,1}, valid={0,0,1,1};
    if (PortFbBlendOverlayScaledSparse(NULL,src,1,1,&invalid)) return 7;
    if (PortFbBlendOverlayScaledSparse(NULL,NULL,1,1,&valid)) return 8;
    if (PortFbBlendOverlayScaledSparse(NULL,src,0,1,&valid)) return 9;
    for (int t=0;t<80;t++) {
        int sw=1+rnd()%240, sh=1+rnd()%160;
        SDL_Rect r={rnd()%20,rnd()%20,0,0};
        r.w=1+rnd()%(640-r.x); r.h=1+rnd()%(480-r.y);
        if (t % 4 == 3) { sw=240; r.x=0; r.w=640; }
        if (t % 8 >= 6) { sw=240; sh=160; r.x=0; r.w=640; r.y=26; r.h=427; }
        for (int i=0;i<sw*sh;i++) {
            src[i]=rnd();
            if (t%4==0 && (i/sw)%3) src[i]&=0x00ffffffu;
            if (t%4==1 && (i%sw)<sw/3) src[i]&=0x00ffffffu;
            if (t%4==2) src[i]|=0xff000000u;
            /* long transparent and opaque runs with odd ends */
            if (t%8>=5) { int part=((i%sw)/7+i/sw/3)%3; if (part==0) src[i]&=0x00ffffffu; if (part==1) src[i]|=0xff000000u; }
        }
        for (int i=0;i<640*480;i++) pixels[i]=expected[i]=rnd();
        uint64_t dx=((uint64_t)sw<<16)/r.w,dy=((uint64_t)sh<<16)/r.h,py=dy/2;
        for (int y=0;y<r.h;y++,py+=dy) {
            uint64_t px=dx/2;
            for (int x=0;x<r.w;x++,px+=dx) {
                int at=(r.y+y)*640+r.x+x;
                uint32_t s=src[(py>>16)*sw+(px>>16)],d=expected[at]; unsigned a=s>>24;
                if (a==255) expected[at]=s;
                else if (a) expected[at]=(channel((s>>16)&255,(d>>16)&255,a)<<16)|
                                               (channel((s>>8)&255,(d>>8)&255,a)<<8)|channel(s&255,d&255,a);
            }
        }
        if (!PortFbBlendOverlayScaled(NULL,src,sw,sh,&r)) return 3;
        for (int i=0;i<640*480;i++) if ((pixels[i]&0xffffff)!=(expected[i]&0xffffff)) return 4;
        /* Black destinations (with any unused alpha byte) take their own path. */
        for (int i=0;i<640*480;i++) pixels[i]=rnd();
        for (int i=0;i<640*480;i+=2) pixels[i]&=0xff000000u;
        memcpy(expected,pixels,640*480*4);
        uint64_t sy=dy/2;
        for (int y=0;y<r.h;y++,sy+=dy) {
            uint64_t sx=dx/2;
            for (int x=0;x<r.w;x++,sx+=dx) {
                int at=(r.y+y)*640+r.x+x;
                uint32_t s=src[(sy>>16)*sw+(sx>>16)],d=expected[at]; unsigned a=s>>24;
                if (a==255) expected[at]=s;
                else if (a) expected[at]=(channel((s>>16)&255,(d>>16)&255,a)<<16)|
                    (channel((s>>8)&255,(d>>8)&255,a)<<8)|channel(s&255,d&255,a);
            }
        }
        if (!PortFbBlendOverlayScaledSparse(NULL,src,sw,sh,&r)) return 5;
        for (int i=0;i<640*480;i++) if ((pixels[i]&0xffffff)!=(expected[i]&0xffffff)) return 6;
    }
    puts("PASS 80 fractional overlay sizes and alpha workloads");
    free(src); free(expected); free(pixels); return 0;
}
'''
with tempfile.TemporaryDirectory() as directory:
    src = Path(directory) / 'test.c'
    exe = Path(directory) / 'test.exe'
    src.write_text(program)
    subprocess.run([sys.argv[1] if len(sys.argv) > 1 else 'clang', '-O2', str(src), '-o', str(exe)], check=True)
    subprocess.run([str(exe)], check=True)
    subprocess.run([str(exe)], check=True, env={**os.environ, 'KHCOM_FBDEV_OVERLAY_240_RUNS': '1'})
    subprocess.run([str(exe)], check=True, env={**os.environ, 'KHCOM_FBDEV_SPARSE_RUNS': '1'})
    subprocess.run([str(exe)], check=True, env={**os.environ, 'KHCOM_FBDEV_OVERLAY_ROW_GROUPS': '1'})
