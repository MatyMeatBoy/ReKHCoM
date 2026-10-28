"""Actual glyph producers and PPU admission slice; CPU refusals, no GPU ACK claim."""
import argparse, hashlib, json, subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
def function(source, signature):
    start=source.index(signature);end=source.index('{',start)+1;depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]

PRE=r'''
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
typedef struct {int id;} SDL_Texture;
typedef struct {int id;} SDL_Renderer;
typedef struct {SDL_Texture*tex;int ready,w,h,left,top,advance;} HdGlyph;
typedef struct {uint8_t*a;int w,h,left,top,advance;} Bitmap;
static int sGlyphUploadFailed,mode,allocs,frees,creates,uploads,destroys,logs;
static uint32_t uploadedPixel;
static SDL_Texture texture;
static void* SDL_malloc(size_t n){allocs++;return mode==1?NULL:malloc(n);}
static void SDL_free(void*p){if(p)frees++;free(p);}
static SDL_Texture* SDL_CreateTexture(void*r,int a,int b,int w,int h){creates++;return mode==2?NULL:&texture;}
static int SDL_UpdateTexture(void*t,void*r,void*p,int pitch){uploads++;uploadedPixel=*(uint32_t*)p;return mode!=3;}
static void SDL_DestroyTexture(void*t){if(!t)abort();destroys++;}
static void SDL_SetTextureBlendMode(void*t,int n){}
static void SDL_SetTextureScaleMode(void*t,int n){}
static void PortLog(const char*t,...){logs++;}
#define SDL_PIXELFORMAT_ARGB8888 1
#define SDL_TEXTUREACCESS_STATIC 1
#define SDL_BLENDMODE_BLEND 1
#define SDL_SCALEMODE_NEAREST 1
static struct {int loaded;} sFace[2]={{1},{1}};
static void LoadCustomFont(void){}
static struct {int hdText;} gPortConfig={1};
static int sHdText;
'''
MAIN=r'''
int main(int argc,char**argv){
 mode=atoi(argv[1]);SDL_Renderer r={0};HdGlyph g={0};Bitmap b={0};
 b.w=b.h=1;b.advance=7;b.a=malloc(1);*b.a=128;g.ready=1;
 if(mode==4)sGlyphUploadFailed=1;
 if(mode==5)b.w=b.h=0;
 GlyphTexture(&r,&g,&b);BeginFrame();
 if(b.a)return 10;
 if(mode==0){if(!g.tex||!g.ready||g.w!=1||g.h!=1||g.advance!=7||uploadedPixel!=0x80ffffffu||!sHdText||creates!=1||uploads!=1||destroys||logs)return 11;}
 else if(mode==5){if(g.tex||!g.ready||!sHdText||allocs||creates||uploads||logs)return 12;}
 else {
  if(g.tex||g.ready||sHdText||!sGlyphUploadFailed)return 13;
  if(creates!=(mode==2||mode==3)||uploads!=(mode==3)||destroys!=(mode==3)||logs!=(mode!=4))return 14;
  int previous=allocs;b.w=b.h=1;b.a=malloc(1);g.ready=1;
  GlyphTexture(&r,&g,&b);BeginFrame();
  if(g.tex||g.ready||sHdText||allocs!=previous||logs!=(mode!=4))return 15;
  /* Same reset as source shutdown; a fresh generation may upload again. */
  sGlyphUploadFailed=0;mode=0;b.a=malloc(1);g.ready=1;
  GlyphTexture(&r,&g,&b);BeginFrame();if(!g.tex||!g.ready||!sHdText)return 16;
 }
 if(g.tex)SDL_DestroyTexture(g.tex);
 printf("PASS mode=%s allocs=%d creates=%d uploads=%d destroys=%d logs=%d\n",argv[1],allocs,creates,uploads,destroys,logs);return 0;
}
'''
def run(out):
    tools=next(p/'toolchain' for p in ROOT.parents if (p/'toolchain').is_dir())
    binary=next(tools.glob('llvm-mingw-*/bin'))
    ppu=(ROOT/'port/src/port_ppu.c').read_text(encoding='utf-8')
    begin=ppu.index('    sHdText = gPortConfig.hdText;');end=ppu.index('\n#endif',begin)+len('\n#endif')
    gate=ppu[begin:end]
    rows=[]
    for name,path,define in [('gdi','port/src/port_hdtext.c','_WIN32'),('stb','port/ps5/guest/src/hdtext_ps5.c','PORT_PS5_GUEST')]:
        source=(ROOT/path).read_text(encoding='utf-8')
        # Shutdown, not an automatic per-frame retry, owns the next generation.
        shutdown=function(source,'void PortHdTextShutdown(void)')
        if 'sGlyphUploadFailed = 0;' not in shutdown and 'sGlyphUploadFailed' in source:
            raise RuntimeError('Upload-failure latch must be reset by shutdown')
        getter=function(source,'int PortPortableHdReady(void)') if 'int PortPortableHdReady(void)' in source else ''
        unit=PRE+getter+'\n'+function(source,'static void GlyphTexture(')+'\nstatic void BeginFrame(void){\n'+gate+'\n}\n'+MAIN
        fixture=out/(name+'.c');fixture.write_text(unit,encoding='utf-8')
        for arch in ['i686','x86_64']:
            exe=out/(name+'-'+arch+'.exe')
            subprocess.run([str(binary/(arch+'-w64-mingw32-clang.exe')),'-std=c11','-O0','-D'+define,str(fixture),'-o',str(exe)],check=True)
            for mode in range(6):
                result=subprocess.run([str(exe),str(mode)],capture_output=True,text=True,timeout=5)
                rows.append(dict(producer=name,arch=arch,mode=mode,exit=result.returncode,stdout=result.stdout,stderr=result.stderr,source_sha256=hashlib.sha256(source.encode()).hexdigest()))
    (out/'RESULTS.json').write_text(json.dumps(rows,indent=2),encoding='utf-8')
    failed=[r for r in rows if r['exit']]
    print(json.dumps(dict(cases=len(rows),failed=len(failed),result='FAIL' if failed else 'PASS')))
    if failed:raise SystemExit(1)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--out',type=Path,required=True);args=p.parse_args()
    args.out.mkdir(parents=True,exist_ok=True);run(args.out)
