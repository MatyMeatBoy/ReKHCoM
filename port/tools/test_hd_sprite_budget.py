"""Run production sprite Load with injected decoder/texture failures, no SDL/GPU."""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'port/src/port_hdsprites.c').read_text(encoding='utf-8')

def function(signature):
    start = source.index(signature)
    while ';' in source[start:source.index('{', start)]:
        start = source.index(signature, start + 1)
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

preamble = r'''
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define HD_SPRITE_COUNT 3
#define SDL_max(a,b) ((a)>(b)?(a):(b))
#define PORT_STUTTER_NOW() 0
#define PORT_STUTTER_ADD(field, start) ((void)0)
#define SDL_snprintf snprintf
#define SDL_free free
static void* decodePointer;
static size_t decodedBytes,peakDecodedBytes;
static int failDecodeAlloc;
static void* TestCalloc(size_t count,size_t size){
 if(failDecodeAlloc)return NULL;
 assert(!decodePointer);decodePointer=calloc(count,size);
 if(decodePointer){decodedBytes=count*size;if(decodedBytes>peakDecodedBytes)peakDecodedBytes=decodedBytes;}
 return decodePointer;
}
#define calloc TestCalloc
static void TestDecodeFree(void*p){if(p==decodePointer){decodedBytes=0;decodePointer=NULL;}free(p);}
#define SDL_PIXELFORMAT_RGBA32 0
#define SDL_BLENDMODE_BLEND 0
typedef int SDL_Texture; typedef int SDL_Surface; typedef int SDL_ScaleMode;
typedef struct { int group; const char*name; int padL,w,padR,padT,h,padB; const void*tiles; const void*frame; int x0,y0; } HdSpriteDef; static HdSpriteDef kHdSprites[3]={{.name="test",.w=16,.h=16}};
static struct {SDL_Texture *image,*white;uint32_t lastUsed;size_t bytes;int textureH,present;} sState[3];
static size_t sBytes; static void*sRenderer;
static int width,height,decodedW,decodedH,infoOK,decodeOK,decodes,creates,destroys,failTexture,surfaces,failSurface;
static char* SpritePath(char*f,size_t n,const char*name){snprintf(f,n,"%s",name);return f;}
static void* TitleWordAsset(const char*n,size_t*z){*z=1;return malloc(1);}
static int stbi_info(const char*f,int*w,int*h,int*n){*w=width;*h=height;return infoOK;}
static int stbi_info_from_memory(const void*p,int z,int*w,int*h,int*n){return stbi_info("",w,h,n);}
static unsigned char* stbi_load(const char*f,int*w,int*h,int*n,int c){decodes++;*w=decodedW;*h=decodedH;return decodeOK?calloc((size_t)*w**h,4):NULL;}
static unsigned char* stbi_load_from_memory(const void*p,int z,int*w,int*h,int*n,int c){return stbi_load("",w,h,n,c);}
static void stbi_image_free(void*p){TestDecodeFree(p);}
static void PortLog(const char*f,...){ }
static uint32_t PortFrameCount(void){return 9;}
static int MenuLightName(const char*n){return 0;} static int MenuLightReady(void*r){return 0;}
static SDL_Surface* SDL_CreateSurfaceFrom(int w,int h,int f,void*p,int pitch){return ++surfaces==failSurface?NULL:(void*)1;}
static SDL_Texture* SDL_CreateTextureFromSurface(void*r,SDL_Surface*s){creates++;return creates==failTexture?NULL:(void*)1;}
static void SDL_DestroySurface(SDL_Surface*s){}
static void SDL_DestroyTexture(SDL_Texture*t){destroys++;}
static void SDL_SetTextureBlendMode(SDL_Texture*t,int m){}
static void SDL_SetTextureScaleMode(SDL_Texture*t,int m){}
static int PortSmoothScaleMode(void){return 0;}
'''
cases = r'''
static void reset(void){for(int i=0;i<3;i++)Unload(i);assert(!decodedBytes&&!decodePointer);memset(sState,0,sizeof(sState));sBytes=0;width=height=decodedW=decodedH=16;infoOK=decodeOK=1;decodes=creates=destroys=failTexture=failDecodeAlloc=surfaces=failSurface=0;peakDecodedBytes=0;kHdSprites[0].group=0;}
int main(void){
 size_t budget=BUDGET;
 reset();infoOK=0;assert(!Load(0));assert(!decodes&&!creates&&!sBytes);
 reset();width=height=4097;assert(!Load(0));assert(!decodes&&!creates);
 reset();width=32;assert(!Load(0));assert(!decodes&&!creates);
 reset();width=height=4096;assert(!Load(0));assert(!decodes&&!creates&&!sBytes);
 reset();sState[1].image=(void*)1;sState[1].white=(void*)1;sState[1].bytes=budget;sBytes=budget;sState[1].lastUsed=9;
 assert(!Load(0));assert(!decodes&&!creates&&sBytes==budget&&!destroys); /* active cache cannot be evicted */
 reset();sState[1].image=(void*)1;sState[1].white=(void*)1;sState[1].bytes=budget;sBytes=budget;sState[1].lastUsed=8;
 assert(Load(0));assert(sBytes==16*16*8&&destroys==2&&creates==2);
 reset();decodedW=32;assert(!Load(0));assert(decodes==1&&!creates&&!sBytes); /* changed between header and decode */
 reset();decodeOK=0;assert(!Load(0));assert(!creates&&!sBytes);
 reset();failTexture=2;assert(!Load(0));assert(destroys==1&&!sBytes&&!sState[0].image&&!sState[0].white);
 assert(!decodedBytes&&peakDecodedBytes==16*16*4);
 reset();failDecodeAlloc=1;assert(!Load(0)&&!creates&&!sBytes&&!decodedBytes);
 reset();failSurface=1;assert(!Load(0)&&!creates&&!sBytes&&!decodedBytes);
 reset();assert(Load(0)&&creates==2&&!decodedBytes&&peakDecodedBytes==16*16*4);
 int released=destroys;Unload(0);assert(!sBytes&&destroys==released+2);
 Unload(0);assert(!sBytes&&destroys==released+2);
 reset();kHdSprites[0].group=7;infoOK=0;assert(!Load(0));assert(!decodes&&!creates);
 reset();kHdSprites[0].group=7;assert(Load(0));assert(sBytes==16*16*8);
 puts("PASS sprite Load: header, dimensions, aspect, budget, pinned/evictable cache, changed file, decode/upload failures, memory assets");
}
'''
if __name__ == '__main__':
    compiler = next((ROOT.parent / 'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
    with tempfile.TemporaryDirectory(prefix='rekhcom-sprite-budget-') as folder:
        folder = Path(folder)
        unit = folder / 'sprite.c'
        unit.write_text(preamble + function('static void Unload(') + function('static int Load(') + cases, encoding='utf-8')
        for guest, budget in ((False, 32*1024*1024), (True, 12*1024*1024)):
            exe = folder / ('guest.exe' if guest else 'native.exe')
            subprocess.run([str(compiler), '-std=c11', f'-DBUDGET={budget}', *(['-DPORT_GBA_RELOC'] if guest else []), str(unit), '-o', str(exe)], check=True)
            subprocess.run([str(exe)], check=True)

