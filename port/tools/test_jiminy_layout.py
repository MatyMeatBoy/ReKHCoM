"""Actual journal geometry and shared glyph-readiness contracts, without a ROM."""
from pathlib import Path
import argparse,subprocess,tempfile
from test_mapcard_definition_bounds import function
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--cc',type=Path,required=True);a=p.parse_args()
ready=function((ROOT/'port/include/port/port_hdtext_ui.inc').read_text(encoding='utf-8'),'PortHdTextUiGlyphReady')
unit=r'''#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <math.h>
#include "port/jiminy_layout.h"
typedef struct SDL_Renderer {int unused;} SDL_Renderer;
typedef struct HdGlyph {void*tex;int ready,w,h,advance;} HdGlyph;
typedef struct HdFont {void*font;} HdFont;
static struct {int hdTitle;} gPortConfig;
static HdFont font;static HdGlyph glyph;static int acknowledged=1;
static HdFont* FontForScale(float scale,int style){(void)scale;(void)style;return &font;}
static HdGlyph* Glyph(SDL_Renderer*r,HdFont*f,int c){(void)r;(void)f;(void)c;return &glyph;}
static HdGlyph* GlyphJp(SDL_Renderer*r,HdFont*f,uint16_t c){return Glyph(r,f,c);}
static int PortBridgeTextureReady(void* texture){return texture&&acknowledged;}
'''+ready+r'''
int main(void){
 PortJiminyLayout b;int w,h;SDL_Renderer renderer={0};
 assert(!PortJiminyFit(0,864,&b));assert(!PortJiminyFit(1280,0,&b));assert(!PortJiminyFit(-1,900,&b));assert(!PortJiminyFit(1280,720,0));
 for(w=1;w<=4096;w+=37)for(h=1;h<=2160;h+=43){
  assert(PortJiminyFit(w,h,&b));assert(isfinite(b.scale)&&b.scale>0);
  assert(b.x>=-0.001f&&b.y>=-0.001f);
  assert(b.x+b.width<=w+0.001f&&b.footerY+b.footerHeight<=h+0.001f);
  assert(fabsf(b.footerY-(b.y+b.height))<0.001f&&b.footerHeight>0);
  assert(fabsf(b.width*2-b.height*3)<0.01f);
  assert(fabsf(b.x*2+b.width-w)<0.001f);
 }
 gPortConfig.hdTitle=1;font.font=(void*)1;glyph=(HdGlyph){(void*)1,1,4,7,5};
 assert(PortHdTextUiGlyphReady(&renderer,'A',4));
 assert(!PortHdTextUiGlyphReady(0,'A',4));assert(!PortHdTextUiGlyphReady(&renderer,'A',0));
 gPortConfig.hdTitle=0;assert(!PortHdTextUiGlyphReady(&renderer,'A',4));gPortConfig.hdTitle=1;
 font.font=0;assert(!PortHdTextUiGlyphReady(&renderer,'A',4));font.font=(void*)1;
 glyph.ready=0;assert(!PortHdTextUiGlyphReady(&renderer,'A',4));glyph.ready=1;
 glyph.tex=0;assert(!PortHdTextUiGlyphReady(&renderer,'A',4));
 glyph.w=glyph.h=0;assert(PortHdTextUiGlyphReady(&renderer,' ',4));
 glyph.advance=0;assert(!PortHdTextUiGlyphReady(&renderer,' ',4));glyph.advance=5;
 glyph.w=4;glyph.h=7;glyph.tex=(void*)1;acknowledged=0;
#ifdef PORT_SDL_BRIDGE_TEXTURE_READY
 assert(!PortHdTextUiGlyphReady(&renderer,'A',4));
#else
 assert(PortHdTextUiGlyphReady(&renderer,'A',4));
#endif
 acknowledged=1;assert(PortHdTextUiGlyphReady(&renderer,'A',4));
 puts("PASS journal geometry across wide/tall/tiny outputs; glyph absence/upload rejection/space/bridge acknowledgement");
}
'''
with tempfile.TemporaryDirectory(prefix='rekhcom-jiminy-layout-') as d:
 d=Path(d);c=d/'test.c';c.write_text(unit,encoding='utf-8')
 for bridge in [False,True]:
  exe=d/f'test-{bridge}.exe';cmd=[str(a.cc.resolve()),'-std=c11','-O2','-Wall','-Wno-unused-function','-I'+str(ROOT/'port/include'),str(c),'-o',str(exe)]
  if bridge:cmd+=['-DPORT_SDL_BRIDGE_TEXTURE_READY']
  subprocess.run(cmd,check=True);subprocess.run([str(exe)],check=True)
