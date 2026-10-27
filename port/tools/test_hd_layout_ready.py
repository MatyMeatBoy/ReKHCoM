"""CPU harness: unavailable HD resources must keep the native gauge visible."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class LayoutReadyTests(unittest.TestCase):
    def test_native_fallback_until_complete_hd_resources(self):
        source = (ROOT / 'port/src/port_hdlayout.c').read_text(encoding='utf-8')
        core = source[source.index('typedef struct HdLayout {'):source.index('\ntypedef struct Ctx')]
        player = source[source.index('#define GAUGE_LEN'):source.index('\nstatic void GaugeParts(')]
        enemy = source[source.index('#define ENEMY_LAYER'):source.index('\nvoid PortHdEnemyGaugeDraw(')]
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            test = directory / 'ready.c'
            test.write_text(PREAMBLE + core + player + enemy + HARNESS, encoding='utf-8')
            exe = directory / 'ready.exe'
            for name in ['gHudGaugeTex0', 'gHudGaugeTex1', 'gHudRikuGaugeTex0']:
                (directory / name).write_bytes(b'corrupt image')
            flags = ['-std=c99', '-Wall', '-Wextra', '-Werror']
            if os.environ.get('HD_LAYOUT_SANITIZE') == '1':
                flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all']
            subprocess.run([os.environ.get('CC', 'clang'), *flags, str(test), '-o', str(exe)], check=True)
            for case in ['unprepared', 'decode', 'surface', 'upload', 'valid', 'face',
                         'riku', 'riku_face', 'sequence', 'player_group', 'enemy_group', 'empty_group']:
                with self.subTest(case=case):
                    result = subprocess.run([str(exe), case], cwd=directory, capture_output=True, text=True)
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


PREAMBLE = r'''
#include <stdint.h>
#include <stddef.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
typedef int SDL_Renderer;
typedef int SDL_Texture;
typedef int SDL_Surface;
static int renderer, surface, texture, failure, failFace;
static unsigned char image[4];
#define SDL_PIXELFORMAT_RGBA32 1
#define SDL_BLENDMODE_BLEND 1
#define SDL_snprintf snprintf
#define SDL_strcmp strcmp
#define SDL_free free
#define SDL_max(a,b) ((a)>(b)?(a):(b))
#define SDL_clamp(x,a,b) ((x)<(a)?(a):((x)>(b)?(b):(x)))
static struct { int hdCards; } gPortConfig = {1};
static unsigned PortFrameCount(void) { return 123; }
static void* SDL_LoadFile(const char* p, size_t* n) {(void)p;(void)n;return NULL;}
static const char* PortHdSpritePath(char* p,size_t n,const char* name) {snprintf(p,n,"%s",name);return p;}
static unsigned char* stbi_load(const char* p,int* w,int* h,int* n,int c) {
    (void)c;*w=*h=1;*n=4;
    return failure==1 || (failFace && strstr(p,"Tex0")) ? NULL : image;
}
static void stbi_image_free(void* p) {(void)p;}
static SDL_Surface* SDL_CreateSurfaceFrom(int w,int h,int fmt,void* px,int pitch) {
    (void)w;(void)h;(void)fmt;(void)px;(void)pitch;return failure==2?NULL:&surface;
}
static SDL_Texture* SDL_CreateTextureFromSurface(SDL_Renderer* r,SDL_Surface* s) {
    (void)r;(void)s;return failure==3?NULL:&texture;
}
static void SDL_DestroySurface(SDL_Surface* s) {(void)s;}
static void SDL_SetTextureBlendMode(SDL_Texture* t,int mode) {(void)t;(void)mode;}
''' + (ROOT / 'port/include/port/port_hdlayout.h').read_text(encoding='utf-8').replace('#ifdef SDL_h_', '#if 0')

HARNESS = r'''
/* Valid complete SEQD tables; group zero references a real animation/part. */
static void put(unsigned char* p,int o,int v) {uint32_t x=(uint32_t)v;p[o]=x;p[o+1]=x>>8;p[o+2]=x>>16;p[o+3]=x>>24;}
static void seq(unsigned char* p,int groups) {
    int i;put(p,0,0x44514553);
    put(p,8,1);put(p,12,48);put(p,16,1);put(p,20,92);
    put(p,24,1);put(p,28,112);put(p,112,0x10000);
    put(p,32,1);put(p,36,116);put(p,128,470);
    put(p,40,groups);put(p,44,260);
    for(i=0;i<groups;i++)put(p,260+i*36,0x10000);
}
int main(int argc,char**argv) {
    unsigned char bytes[2000]={0};const char* c=argc>1?argv[1]:"valid";
    int expect=1;
    int riku=!strcmp(c,"riku")||!strcmp(c,"riku_face");
    (void)F32; (void)kEnemyPlaceGroup;
    put(bytes,0,0x4459414c);put(bytes,24,2);put(bytes,28,32);
    put(bytes,32,40);put(bytes,36,1000);seq(bytes+40,4);seq(bytes+1000,16);
    if(!strcmp(c,"sequence")){put(bytes,24,1);expect=0;}
    if(!strcmp(c,"player_group")){put(bytes+1000,40,14);expect=0;}
    if(!strcmp(c,"enemy_group")){put(bytes+1000,40,6);expect=0;}
    if(!strcmp(c,"empty_group")){put(bytes+1000,260+14*36,0);expect=0;}
    sLayouts[HD_LAYOUT_GAUGE]=(HdLayout){bytes,sizeof(bytes),0};
    assert(ValidateLayout(&sLayouts[HD_LAYOUT_GAUGE]));
    if(!strcmp(c,"unprepared")){assert(!PortHdGauge(100,100,0));assert(!PortHdEnemyGauge(1,100,100,bytes));return 0;}
    failure=!strcmp(c,"decode")?1:!strcmp(c,"surface")?2:!strcmp(c,"upload")?3:0;
    failFace=!strcmp(c,"face")||!strcmp(c,"riku_face");
    /* The native phase requests resources; only the presentation phase uploads. */
    assert(!PortHdGauge(100,100,riku));
    assert(!PortHdEnemyGauge(1,100,100,bytes));
    PortHdLayoutPrepare(&renderer);
    assert(!PortHdLayoutReady(HD_LAYOUT_GAUGE,-1));
    assert(!PortHdLayoutReady(HD_LAYOUT_GAUGE,HD_LAYOUT_TEXTURE_COUNT));
    if(failure)expect=0;
    assert(PortHdGauge(100,100,riku)==(expect&&!failFace&&strcmp(c,"enemy_group")!=0));
    if(!strcmp(c,"player_group")||!strcmp(c,"empty_group"))expect=1;
    assert(PortHdEnemyGauge(1,100,100,bytes)==expect);
    return 0;
}
'''

if __name__ == '__main__':
    unittest.main()
