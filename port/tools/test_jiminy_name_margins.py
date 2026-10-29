"""Actual name-margin renderer uses presented BG0 ownership, not title length."""
import argparse,subprocess,tempfile
from pathlib import Path
from test_mapcard_definition_bounds import function
root=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--cc',type=Path,required=True);a=p.parse_args()
s=(root/'port/src/port_main.c').read_text(encoding='utf8')
edge=function(s.replace('static float PixelEdge','float PixelEdge'),'PixelEdge')
render=function(s.replace('static void DrawJournalNameMargins','void DrawJournalNameMargins'),'DrawJournalNameMargins')
prefix=r'''#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "port/jiminy_paper.h"
typedef uint8_t Uint8;
typedef struct SDL_FRect {float x,y,w,h;} SDL_FRect;
typedef struct SDL_Renderer {int unused;} SDL_Renderer;
static SDL_Renderer renderer,*sRenderer=&renderer;
static uint32_t edges[48],colour,paint[48];static SDL_FRect box[48];static int ready=1,count;
#define SDL_max(a,b) ((a)>(b)?(a):(b))
#define SDL_floorf floorf
static const uint32_t* PortPpuJiminyNameEdges(void){return ready?edges:0;}
static int SDL_SetRenderDrawColor(SDL_Renderer*r,int red,int green,int blue,int alpha){(void)r;assert(alpha==255);colour=0xff000000u|(red<<16)|(green<<8)|blue;return 1;}
static int SDL_RenderFillRect(SDL_Renderer*r,const SDL_FRect*b){(void)r;assert(count<48&&b->w>0&&b->h>0);box[count]=*b;paint[count++]=colour;return 1;}
'''
unit=prefix+edge+render+r'''
int main(void){
 SDL_FRect dst={100,0,960,640};uint32_t yellow=0xfffac600u,green=0xff9cc65au;
 for(int layer=0;layer<6;layer++)for(int blend=0;blend<2;blend++)for(int x=-1;x<=240;x++)for(int y=15;y<=40;y++){
  memset(edges,0,sizeof(edges));PortJiminyRecordNameEdge(edges,layer,blend,x,y,yellow);
  int expected=layer==0&&!blend&&(x==0||x==239)&&y>=16&&y<40,seen=0;
  for(int i=0;i<48;i++)seen+=edges[i]!=0;assert(seen==expected);
 }
 memset(edges,0,sizeof(edges));edges[24]=yellow; /* right PAGE below label; never extend it to the left */
 DrawJournalNameMargins(&dst,1200,green);assert(!count);
 edges[5]=yellow;DrawJournalNameMargins(&dst,1200,green);
 assert(count==2&&paint[0]==yellow&&paint[1]==green&&box[0].y==84&&box[1].y==84&&box[0].w==100&&box[1].x==1060&&box[1].w==140);
 count=0;edges[29]=0xff000000u;DrawJournalNameMargins(&dst,1200,green);assert(count==2&&paint[1]==0xff000000u);
 count=0;ready=0;DrawJournalNameMargins(&dst,1200,green);assert(!count);
 ready=1;count=0;DrawJournalNameMargins(&dst,1060,green);assert(count==1);
 count=0;dst.x=0;DrawJournalNameMargins(&dst,960,green);assert(!count);
 puts("PASS BG0/alpha/edge ownership and bounds; short/long independent, right PAGE unchanged, native colours/black, absent metadata and zero-width margins");
}
'''
with tempfile.TemporaryDirectory(prefix='rekhcom-jiminy-name-') as d:
 d=Path(d);c=d/'test.c';exe=d/'test.exe';c.write_text(unit,encoding='utf8')
 subprocess.run([str(a.cc.resolve()),'-std=c11','-O2','-I'+str(root/'port/include'),str(c),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
