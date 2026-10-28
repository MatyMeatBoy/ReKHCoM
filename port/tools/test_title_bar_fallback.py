"""Execute the actual PPU queue/bar functions with controlled queue acceptance."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.find(signature + ' {')
    if start < 0:
        return ''
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


PREFIX = r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define GBA_SCREEN_W 240
#define GBA_SCREEN_H 160
#define TRANSPARENT 0xffff
#define PPU_MIN(a,b) ((a)<(b)?(a):(b))
#define PPU_MAX(a,b) ((a)>(b)?(a):(b))
static uint16_t regs[64], oam[512], palette[512];
#define IO16(n) regs[(n)/2]
#define OAM16 oam
#define PLTT16 palette
static struct {int width,height,originX,originY;} gPortScreen;
static struct {int hdTitle;} gPortConfig;
static int gPortOverlayW=240,sTitleBands,sEnvTitleBandsOff,sTitleBarHd;
static int sMainHdAny,sScreenHdAny,sTitleLogoBgHd,sTitleArtHd,sTitleLogoBg;
static int sHudLayer,sHudCanvasFallback,sBottom=160,sRight=240;
static int sOverlay,sScreenBg[4],sHdText;
static uint8_t sOccBg[240*160],sOccBgLayer[240*160];
static uint16_t sOccObj[240*160];
static int sAffX[2],sAffY[2];
static uint8_t gBgPaletteBank[4],sMainHdPart[128],sMainHdScreen[128];
static int16_t sMainHdBox[128][4];
static struct {int layer;} sHdPieces[64]; static int sHdPieceCount;
static struct {int hud,screen;} sOamTag[128];
static struct {int x,y;} sOamExt[128];
typedef struct {uint16_t id;int16_t x,y,w,h;uint8_t flags,bank;
 const void *tiles,*frame;const uint16_t* palette;} OamHdPart;
static OamHdPart sOamHd[128];
static const int sObjSize[3][4][2]={{{8,8},{16,16},{32,32},{64,64}},{{16,8}},{{8,16}}};
const unsigned char gTitlePressStartTiles[1]={1};
const unsigned char gTitlePressStartFrenchTiles[1]={2},gTitlePressStartGermanTiles[1]={3};
const unsigned char gTitlePressStartItalianTiles[1]={4},gTitlePressStartSpanishTiles[1]={5};
const unsigned char gTitleLogoBgTiles[1]={6},gTitleLogoBgPalette[1]={0};
static uint16_t sBgLine[4][240];static uint32_t sColorLut[65536];
static int accepted[128],rows;
static void TitleArtQueue(void){}
static void PortHdSpritesBegin(int layer){(void)layer;}
static void QueueStillHdParts(void){}
static int SpriteOnScreen(int screen){return screen;}
static int PortHdSpriteFind(const void* t,const void* f){return 0;}
static int PortHdSpriteQueue(int l,int id,const float*x,const float*y,int a,int b,int c,int d,int e,int f,int g){return 0;}
static void PortPaletteFade(const uint16_t*a,const uint16_t*b,int n,int*d,int*w){*d=*w=0;}
static int QueueHdPart(int l,int i,int hx,int hy,int ow,int oh,int bw,int bh,int af,
 int16_t pa,int16_t pb,int16_t pc,int16_t pd,int alpha){return accepted[i];}
static uint16_t Blend(uint16_t c,uint16_t p,int a,int b){return c;}
static uint16_t Brighten(uint16_t c,int n){return c;}
static uint16_t Darken(uint16_t c,int n){return c;}
static void PortHdTitleBarRow(int y,int x,int n,const uint32_t*p){rows++;}
'''

MAIN = r'''
static void reset(void){
 memset(regs,0,sizeof(regs));memset(oam,0,sizeof(oam));memset(sOamHd,0,sizeof(sOamHd));
 memset(accepted,0,sizeof(accepted));memset(sOamTag,0,sizeof(sOamTag));
 for(int i=0;i<128;i++)oam[i*4]=0x200;
 for(int x=0;x<240;x++)sBgLine[0][x]=TRANSPARENT;
 for(int x=40;x<100;x++)sBgLine[0][x]=3;
 gPortScreen.width=240;gPortScreen.height=160;gPortConfig.hdTitle=1;
 sTitleBands=1;sEnvTitleBandsOff=0;sTitleLogoBg=0;IO16(0)=0x1000;rows=0;
}
static void part(int i,const void*tiles,int id,int ok){
 oam[i*4]=143;oam[i*4+1]=50;sOamExt[i].x=50;sOamExt[i].y=143;
 sOamHd[i].tiles=tiles;sOamHd[i].id=id;sOamHd[i].w=8;sOamHd[i].h=8;accepted[i]=ok;
}
static int check(const char*name,int want){
 /* DrawTextBg produces a fresh line before each real TitleBarLine call. */
 for(int x=0;x<240;x++)sBgLine[0][x]=TRANSPARENT;
 for(int x=40;x<100;x++)sBgLine[0][x]=3;
 QueueMainHdParts();
 if(sTitleBarHd!=want){fprintf(stderr,"FAIL %s bar=%d want=%d\n",name,sTitleBarHd,want);return 1;}
 TitleBarLine(143,143);
 if((rows>0)!=want || (!want && sBgLine[0][50]!=3)){
  fprintf(stderr,"FAIL %s native/handover rows=%d\n",name,rows);return 1;}
 return 0;
}
int main(void){int failed=0;
 const void*langs[]={gTitlePressStartTiles,
#ifdef VERSION_EU
 gTitlePressStartFrenchTiles,gTitlePressStartGermanTiles,gTitlePressStartItalianTiles,gTitlePressStartSpanishTiles,
#endif
 };
 for(unsigned l=0;l<sizeof(langs)/sizeof(langs[0]);l++){
  reset();part(127,langs[l],0,0);failed+=check("missing/corrupt id0",0);
  reset();part(127,langs[l],1,0);failed+=check("resource known, queue rejected",0);
  reset();part(127,langs[l],1,1);part(126,langs[l],1,0);failed+=check("partial acceptance",0);
  reset();part(127,langs[l],1,1);part(126,langs[l],1,1);failed+=check("all accepted",1);
  reset();part(127,langs[l],0,0);oam[127*4]=0x200;failed+=check("disabled words",1);
 }
 reset();failed+=check("blink no active words",1);
 reset();part(127,gTitleLogoBgTiles,1,0);failed+=check("unrelated rejected sprite",1);
 reset();IO16(0)=0;failed+=check("OBJ disabled bar phase",1);
 reset();gPortConfig.hdTitle=0;part(127,gTitlePressStartTiles,1,1);failed+=check("original/GBA",0);
 reset();sTitleBands=0;failed+=check("title left",0);
 reset();part(127,gTitlePressStartTiles,1,1);failed+=check("accepted frame",1);
 accepted[127]=0;rows=0;failed+=check("next frame rejection",0);
 printf("PPU title bar checks: %s\n",failed?"FAIL":"PASS");return failed?1:0;
}
'''


class TitleBarFallback(unittest.TestCase):
    def test_actual_queue_and_native_bar_handover(self):
        source = (ROOT / 'port/src/port_ppu.c').read_text(encoding='utf-8')
        pieces = [function(source, 'static int TitlePressStartTiles(const void* tiles)'),
                  function(source, 'static int TitleBarWordsReady(void)'),
                  function(source, 'static void QueueMainHdParts(void)'),
                  function(source, 'static void TitleBarLine(int y, int gy)')]
        self.assertTrue(pieces[2] and pieces[3])
        cc = shutil.which('clang')
        if not cc:
            tc = next(p / 'toolchain' for p in ROOT.parents if (p / 'toolchain').is_dir())
            cc = str(sorted(tc.glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))[-1])
        with tempfile.TemporaryDirectory(prefix='ppu-title-bar-') as tmp:
            c = Path(tmp) / 'actual.c'; exe = Path(tmp) / 'actual.exe'
            c.write_text(PREFIX + '\n'.join(pieces) + MAIN, encoding='utf-8')
            for region in ('EU', 'US', 'JP'):
                with self.subTest(region=region):
                    subprocess.run([cc, '-std=c11', '-O2', '-DVERSION_'+region, str(c), '-o', str(exe)], check=True)
                    run = subprocess.run([str(exe)], capture_output=True, text=True, timeout=10)
                    self.assertEqual(run.returncode, 0, run.stdout + run.stderr)


if __name__ == '__main__':
    unittest.main()
