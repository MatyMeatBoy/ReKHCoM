"""Exercise real foreground-layer branches for two native small-card corners."""
from pathlib import Path
import json, os, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/'port/src/port_ppu.c').read_text(encoding='utf-8')
def block(marker, text=source):
    start=text.index(marker);i=text.index('{',start)+1;depth=1
    while depth:depth+=(text[i]=='{')-(text[i]=='}');i+=1
    return text[start:i]
helpers=''
for marker in ['static int SmallCardNativeCorner(', 'static int SmallCardCornerAt(']:
    if marker in source:helpers+=block(marker)+'\n'
world=block('static void HdOverLayer(')
hud=source[source.index('void PortPpuRenderHud('):]
hud=block('if (sHdCover && (size_t)w * h <= sHdCoverSize && sHdCover[py * w + px]',hud)
clear=block('if ((hdSprite || hdPart) && sHudOverUsed)') if 'if ((hdSprite || hdPart) && sHudOverUsed)' in source else ''
recipes=[r.split('\t') for r in (ROOT/'port/data/hd_sprites.tsv').read_text().splitlines() if '\t' in r]
recipes=[r for r in recipes if r[1]=='6']
assert recipes and all(r[4:8]==['-8','-20','16','32'] and 'place=1,16,15,31' in r[9] for r in recipes)
cards=[r[0] for r in recipes]
corpus=os.environ.get('KHCOM_CARD_CORNER_INVENTORY')
colors=[]
if corpus:
    for row in json.loads(Path(corpus).read_text())['cards']:
        def argb(c):return 0xff000000|(c[0]<<16)|(c[1]<<8)|c[2]
        colors.append((argb(row['row16_corner_left']),argb(row['row16_corner_right'])))
else:
    colors=[(0xff000000|(i*35041&0xffffff),0xff000000|(i*91991&0xffffff)) for i in range(len(cards))]
pre=r'''
#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
typedef struct {int id,x,y,w,h;} OamHdPart;
static OamHdPart sOamHd[128];
static unsigned char sMainHdPart[128],sMainHdScreen[128];
static int16_t sMainHdBox[128][4];
static uint16_t oam[512];
#define OAM16 oam
static int PortHdSpriteKeepsCartridge(int id){return id==1;}
static uint8_t cover[32*64],*sHdCover=cover;
static size_t sHdCoverSize=sizeof(cover);
static uint32_t sHudOver[32*64];
static int sHudOverUsed;
'''
code=pre+helpers+world+r'''
static void HudWrite(int i,int hdPart,int hdSprite,int tx,int ty,uint32_t argb){
 int px=0,py=0,w=1,h=1;
 for(int once=0;once<1;once++){
'''+clear+r'''
 if(hdSprite)continue;
'''+hud+r'''
 }
}
static uint32_t HudPixel(int ready,int tx,int ty,uint32_t argb){
 sHudOverUsed=0;sHdCover[0]=ready;sHudOver[0]=0;
 HudWrite(0,ready,0,tx,ty,argb);
 return sHudOverUsed?sHudOver[0]:0;
}
static const uint32_t colors[][2]={'''+','.join('{%du,%du}'%p for p in colors)+r'''};
int main(void){
 static uint8_t bg[32*64];static uint16_t owner[32*64];
 static uint32_t picture[32*64],over[32*64];
 int checked=0;
 for(int card=0;card<(int)(sizeof(colors)/sizeof(colors[0]));card++)
 for(int ready=0;ready<2;ready++)for(int group=0;group<2;group++)for(int screen=0;screen<2;screen++)
 for(int flip=0;flip<4;flip++) {
   memset(sMainHdPart,0,sizeof(sMainHdPart));memset(oam,0,sizeof(oam));
   sOamHd[0].id=group?1:2;sOamHd[0].x=-8;sOamHd[0].y=-20;sOamHd[0].w=16;sOamHd[0].h=32;
   sMainHdPart[0]=ready;sMainHdScreen[0]=screen;
   sMainHdBox[0][0]=sMainHdBox[0][1]=0;sMainHdBox[0][2]=16;sMainHdBox[0][3]=32;
   oam[1]=(flip&1?0x1000:0)|(flip&2?0x2000:0);
   memset(bg,4,sizeof(bg));memset(owner,0,sizeof(owner));memset(over,0,sizeof(over));
   for(int y=0;y<32;y++)for(int x=0;x<16;x++)picture[y*16+x]=colors[card][x==14];
   int used=0;HdOverLayer(screen,16,32,bg,owner,picture,over,&used);
   for(int y=0;y<32;y++)for(int x=0;x<16;x++) {
     int tx=flip&1?15-x:x,ty=flip&2?31-y:y;
     int corner=ready&&group&&ty==16&&(tx==1||tx==14);
     assert(over[y*16+x]==(corner?picture[y*16+x]:0));
     assert(HudPixel(ready,tx,ty,picture[y*16+x])==(corner?picture[y*16+x]:0));
     checked+=2;
   }
 }
 /* Real GBA affine source mappings: doubled identity, 2x and 180-degree.
    These have explicit expected corner locations, not a copy of the helper. */
 for(int transform=0;transform<3;transform++) {
   memset(sMainHdPart,0,sizeof(sMainHdPart));memset(oam,0,sizeof(oam));
   sOamHd[0]=(OamHdPart){1,-8,-20,16,32};sMainHdPart[0]=1;sMainHdScreen[0]=0;
   sMainHdBox[0][0]=sMainHdBox[0][1]=0;sMainHdBox[0][2]=32;sMainHdBox[0][3]=64;
   oam[0]=0x300;oam[3]=oam[15]=transform==1?128:transform==2?(uint16_t)-256:256;
   memset(bg,4,sizeof(bg));memset(owner,0,sizeof(owner));memset(over,0,sizeof(over));
   for(int k=0;k<32*64;k++)picture[k]=0xff123400u+k;
   int used=0;HdOverLayer(0,32,64,bg,owner,picture,over,&used);
   for(int y=0;y<64;y++)for(int x=0;x<32;x++) {
     int expected=transform==0?(y==32&&(x==9||x==22)):
                  transform==1?((y==32||y==33)&&(x==2||x==3||x==28||x==29)):
                               (y==32&&(x==10||x==23));
     assert(over[y*32+x]==(expected?picture[y*32+x]:0));
   }
   /* A different visible OAM owner cannot donate the hidden card's corner. */
   memset(owner,0xff,sizeof(owner));memset(over,0,sizeof(over));used=0;
   HdOverLayer(0,32,64,bg,owner,picture,over,&used);assert(!used);
 }
 /* A partial OAM piece contains only the right corner. */
 memset(oam,0,sizeof(oam));sOamHd[0]=(OamHdPart){1,0,-4,8,8};
 sMainHdBox[0][2]=sMainHdBox[0][3]=8;
 memset(owner,0,sizeof(owner));memset(over,0,sizeof(over));
 int used=0;HdOverLayer(0,8,8,bg,owner,picture,over,&used);
 for(int k=0;k<64;k++)assert(over[k]==(k==6?picture[k]:0));
 /* Real back-to-front HUD ordering: a lower card's preserved corner must
    not remain above the illustration of the next visible HD card. */
 for(int front=0;front<4;front++) {
   sOamHd[127]=sOamHd[126]=(OamHdPart){1,-8,-20,16,32};
   sHudOverUsed=0;sHudOver[0]=0;sHdCover[0]=1;
   HudWrite(127,1,0,1,16,0xff001122u);assert(sHudOver[0]==0xff001122u);
   if(front==0)HudWrite(126,1,0,5,20,0xff334455u); /* front HD illustration */
   if(front==1)HudWrite(126,1,0,14,16,0xff334455u); /* front's own native corner */
   if(front==2)HudWrite(126,0,0,5,20,0xff334455u); /* front native fallback */
   if(front==3)HudWrite(126,0,1,5,20,0xff334455u); /* another HD sprite path */
   assert(sHudOver[0]==((front==1||front==2)?0xff334455u:0));
 }
 printf("PASS: %d world/screen/HUD native corner pixels; queued/off, colours, flips\n",checked);
}
'''
cc=next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
with tempfile.TemporaryDirectory(prefix='small-card-corner-') as d:
    c=Path(d)/'test.c';exe=Path(d)/'test.exe';c.write_text(code)
    subprocess.run([str(cc),'-std=c99',str(c),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True,timeout=15,creationflags=0x08004000)
print(f'PASS: {len(colors)} card colour pairs, {len(cards)} current small-card recipes; foreground owns only two original cells')
