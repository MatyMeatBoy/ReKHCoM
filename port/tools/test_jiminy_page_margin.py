"""Real native-paper margin renderer: ownership, rules, bounds and failed draws."""
from pathlib import Path
import argparse,subprocess,tempfile
from test_mapcard_definition_bounds import function
root=Path(__file__).resolve().parents[2];p=argparse.ArgumentParser();p.add_argument('--cc',type=Path,required=True);a=p.parse_args()
main=(root/'port/src/port_main.c').read_text(encoding='utf8')
edge=function(main.replace('static float PixelEdge','float PixelEdge'),'PixelEdge')
unit=r'''#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>
#include <string.h>
#include "port/jiminy_paper.h"
typedef uint8_t Uint8;
typedef struct SDL_FRect{float x,y,w,h;} SDL_FRect;
typedef struct SDL_Renderer{int unused;} SDL_Renderer;
static SDL_Renderer renderer,*sRenderer=&renderer;
static struct {int width,height;} gPortScreen={240,160};
static uint32_t native[240*160],composite[240*160],*gPortFrame=native,*sComposite=composite;
static PortJiminyPageRow rows[160];
static int ready=1,count,attempt,failAt,restored;
static uint32_t color,paint[4096];static SDL_FRect rectangles[4096];
#define SDL_floorf floorf
static const PortJiminyPageRow* PortPpuJiminyPageRows(void){return ready?rows:0;}
static int SDL_SetRenderDrawColor(SDL_Renderer*r,int red,int green,int blue,int alpha){(void)r;assert(alpha==255);color=0xff000000u|(red<<16)|(green<<8)|blue;return 1;}
static int SDL_RenderFillRect(SDL_Renderer*r,const SDL_FRect*rect){
 (void)r;attempt++;if(attempt==failAt)return 0;
 assert(count<4096&&rect->x>=0&&rect->y>=0&&rect->x+rect->w<=1400.01f&&rect->y+rect->h<=720.01f);
 paint[count]=color;rectangles[count++]=*rect;return 1;
}
static void DrawJournalPicture(const SDL_FRect*src,const SDL_FRect*dst){(void)src;(void)dst;restored++;}
'''+edge+'\n'+(root/'port/include/port/jiminy_margin_draw.inc').read_text(encoding='utf8')+r'''
static uint32_t pixel(float x,float y){uint32_t c=0;for(int i=0;i<count;i++){SDL_FRect*r=&rectangles[i];if(x>=r->x&&x<r->x+r->w&&y>=r->y&&y<r->y+r->h)c=paint[i];}return c;}
static void reset(void){
 count=attempt=failAt=restored=0;ready=1;memset(native,0,sizeof(native));memset(composite,0,sizeof(composite));
 for(int y=0;y<160;y++)rows[y]=(PortJiminyPageRow){0xffefeff7,0xffa5adb5,0xffd6d6d6,0xff9cc65a,65535};
}
int main(void){
 SDL_FRect src={0,0,240,160},dst={10,20,960,640};
 reset();rows[80].owned&=~(1<<11);composite[81*240+236]=0xff123456;
 rows[72].fill=0xffa5adb5;
 assert(RepairJiminyPageMargin(&src,&dst,1400)&&!restored);
 assert(count<100); /* flat paper/rules must batch, not flood the draw bridge */
 assert(pixel(1000,400)==0xffefeff7); /* green margin becomes native paper */
 assert(pixel(1000,20+72*4)==0xffa5adb5); /* authored horizontal rule */
 assert(pixel(1356,400)==0xffa5adb5); /* right guide moved, never stretched */
 assert(pixel(950,400)==0xffefeff7); /* old vertical guide erased only on BG1 */
 assert(!pixel(950,340)&&!pixel(954,344)); /* object and composite edit survive */
 assert(pixel(1385,400)==0xffd6d6d6&&pixel(1395,400)==0xff9cc65a);
 reset();ready=0;assert(!RepairJiminyPageMargin(&src,&dst,1400)&&!count&&!restored);
 reset();rows[90].fill=0;assert(!RepairJiminyPageMargin(&src,&dst,1400)&&!count&&!restored);
 reset();assert(!RepairJiminyPageMargin(&src,&dst,970)&&!count&&!restored);
 reset();src.x=-1;assert(!RepairJiminyPageMargin(&src,&dst,1400)&&!count);src.x=0;
 reset();gPortScreen.width=239;assert(!RepairJiminyPageMargin(&src,&dst,1400)&&!count);gPortScreen.width=240;
 for(int failure=1;failure<12;failure++){
  reset();failAt=failure;assert(!RepairJiminyPageMargin(&src,&dst,1400)&&restored==1);
 }
 puts("PASS actual paper/rule/border rendering; BG1-only tail repair protects objects and UI; invalid/missing/narrow data skip; draw rejection restores picture");
}
'''
with tempfile.TemporaryDirectory(prefix='jiminy-page-margin-') as folder:
 c=Path(folder)/'test.c';exe=Path(folder)/'test.exe';c.write_text(unit,encoding='utf8')
 subprocess.run([str(a.cc.resolve()),'-std=c11','-O2','-Wall','-Wno-unused-function','-I'+str(root/'port/include'),str(c),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
