"""Actual FreeType/Web Raster producers with CPU upload-refusal fixtures.

No renderer, browser, driver or console-shim acceptance is certified here.
"""
import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
PREAMBLE = r'''
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
typedef struct {int id;} SDL_Texture;
typedef struct {int id;} SDL_Renderer;
typedef long FT_Fixed;
typedef struct {FT_Fixed xx,xy,yx,yy;} FT_Matrix;
typedef struct {int width,rows,pitch,pixel_mode;unsigned char* buffer;} FT_Bitmap;
typedef struct {FT_Bitmap bitmap;int bitmap_left,bitmap_top;struct {long x;} advance;} Slot;
typedef struct {Slot* glyph;} Face;
typedef Face* FT_Face;
typedef struct {SDL_Texture* tex;int ready,w,h,left,top,advance;} HdGlyph;
typedef struct {FT_Face font;int style;float cap,scale,condense;} HdFont;
static FT_Face japanese,fallback;
static int ready=1,gPortWebHdReady=1,load_error,create_ok=1,upload_ok=1;
static int creates,uploads,destroys,logs;
static uint8_t scratch[512*512];
static SDL_Texture texture={1};
#define FT_LOAD_RENDER 1
#define FT_LOAD_TARGET_NORMAL 2
#define FT_PIXEL_MODE_GRAY 2
#define SDL_PIXELFORMAT_ARGB8888 1
#define SDL_TEXTUREACCESS_STATIC 1
#define SDL_BLENDMODE_BLEND 1
#define SDL_SCALEMODE_NEAREST 1
#define SDL_max(a,b) ((a)>(b)?(a):(b))
#define SDL_malloc malloc
#define SDL_free free
static void PortLog(const char* text,...) {logs++;}
static unsigned Unicode(int code,int jp) {return code;}
static int FT_Get_Char_Index(FT_Face f,unsigned cp) {return f!=NULL;}
static int FT_Set_Pixel_Sizes(FT_Face f,int a,int b) {return 0;}
static void FT_Set_Transform(FT_Face f,FT_Matrix* m,void* p) {}
static int FT_Load_Char(FT_Face f,unsigned cp,int flags) {return load_error;}
static const uint16_t* PortLatinGlyphWidths(void) {static uint16_t w[256];return w;}
static int WebRaster(int style,float cap,float width,int code,int jp,void* pixels,int* m) {
    if(load_error)return 0; m[0]=m[1]=m[2]=1;m[3]=m[4]=0;return 1;
}
static SDL_Texture* SDL_CreateTexture(SDL_Renderer* r,int a,int b,int c,int d) {
    creates++;return create_ok?&texture:NULL;
}
static int SDL_UpdateTexture(SDL_Texture* t,void* rect,const void* p,int pitch) {uploads++;return upload_ok;}
static void SDL_DestroyTexture(SDL_Texture* t) {if(!t)abort();destroys++;}
static int SDL_SetTextureBlendMode(SDL_Texture* t,int mode) {return 1;}
static int SDL_SetTextureScaleMode(SDL_Texture* t,int mode) {return 1;}
'''
MAIN = r'''
int main(int argc,char** argv) {
    int mode=atoi(argv[1]);unsigned char pixel=255;Slot slot={0};
    slot.bitmap.width=slot.bitmap.rows=slot.bitmap.pitch=1;
    slot.bitmap.pixel_mode=FT_PIXEL_MODE_GRAY;slot.bitmap.buffer=&pixel;
    Face face={&slot};HdFont font={&face,0,7,1,1};HdGlyph g={0};SDL_Renderer renderer={0};
    load_error=mode==1;create_ok=mode!=2;upload_ok=mode!=3;
    if(mode==4)READY=0;
    Raster(&renderer,&font,&g,'H',0);
    if(mode==0) {
        if(!READY||!g.ready||!g.tex||creates!=1||uploads!=1||destroys||logs)return 10;
        Raster(&renderer,&font,&g,'H',0);
        if(creates!=1||uploads!=1)return 11; /* Valid cached glyph unchanged. */
        SDL_DestroyTexture(g.tex);if(destroys!=1)return 12;
    } else if(mode==1) {
        if(READY||g.tex||creates||uploads||destroys)return 13;
    } else if(mode==2||mode==3) {
        if(READY||g.ready||g.tex||creates!=1||uploads!=(mode==3)||destroys!=(mode==3)||logs!=1)return 14;
        Raster(&renderer,&font,&g,'H',0);
        if(creates!=1||logs!=1||destroys!=(mode==3))return 15; /* No failure loop. */
        READY=1;create_ok=upload_ok=1; /* Explicit recovery/new readiness generation. */
        Raster(&renderer,&font,&g,'H',0);
        if(!READY||!g.ready||!g.tex||creates!=2||logs!=1)return 16;
        SDL_DestroyTexture(g.tex);if(destroys!=1+(mode==3))return 17;
    } else if(mode==4) {
        if(g.ready||g.tex||creates||uploads||destroys||logs)return 18;
    }
    printf("PASS mode=%d creates=%d uploads=%d destroys=%d logs=%d\n",mode,creates,uploads,destroys,logs);
    return 0;
}
'''


def run(out):
    if os.name == 'nt':
        tools = next(p / 'toolchain' for p in ROOT.parents if (p / 'toolchain').is_dir())
        binary = next(tools.glob('llvm-mingw-*/bin'))
        compilers = [(arch, binary / (arch + '-w64-mingw32-clang.exe'))
                     for arch in ('i686', 'x86_64')]
    else:
        compiler = shutil.which('clang') or shutil.which('cc')
        if not compiler:
            raise RuntimeError('C compiler required')
        compilers = [('native', Path(compiler))]
    results = []
    for name, relative, ready in (
        ('freetype', 'port/portable/hdtext_freetype.c', 'ready'),
        ('web', 'port/web/native/hdtext_web.c', 'gPortWebHdReady'),
    ):
        source = (ROOT / relative).read_text(encoding='utf-8')
        start = source.index('static void Raster(')
        end = source.index('\nstatic HdGlyph* Glyph(', start)
        fixture = out / (name + '.c')
        fixture.write_text(PREAMBLE + source[start:end] + MAIN.replace('READY', ready), encoding='utf-8')
        for arch, compiler in compilers:
            exe = out / (name + '-' + arch + ('.exe' if os.name == 'nt' else ''))
            subprocess.run([str(compiler), '-std=c11', '-O0', str(fixture), '-o', str(exe)], check=True)
            for mode in range(5):
                actual = subprocess.run([str(exe), str(mode)], capture_output=True, text=True)
                results.append(dict(producer=name, arch=arch, mode=mode,
                                    exit=actual.returncode, stdout=actual.stdout, stderr=actual.stderr))
    (out / 'RESULTS.json').write_text(json.dumps(results, indent=2), encoding='utf-8')
    failed = [r for r in results if r['exit']]
    if failed:
        raise RuntimeError(f'{len(failed)} actual-producer cases failed: {failed}')
    print(f'PASS {len(results)} actual Raster cases; CPU fixtures only')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--out', type=Path)
    args = parser.parse_args()
    if args.out:
        args.out.mkdir(parents=True, exist_ok=True)
        run(args.out)
    else:
        with tempfile.TemporaryDirectory(prefix='font-upload-') as directory:
            run(Path(directory))
