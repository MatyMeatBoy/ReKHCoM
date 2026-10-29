"""Regression for a journal footer reading next-frame palettes after VBlank."""
from pathlib import Path
import argparse,subprocess,tempfile
from test_mapcard_definition_bounds import function
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--cc',type=Path,required=True);a=p.parse_args()
ppu=(ROOT/'port/src/port_ppu.c').read_text(encoding='utf-8');hd=(ROOT/'port/src/port_hdsprites.c').read_text(encoding='utf-8')
unit=r'''#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#define SDL_min(a,b) ((a)<(b)?(a):(b))
#define SDL_max(a,b) ((a)>(b)?(a):(b))
#define SDL_abs abs
static uint16_t live[256],sJiminyHeaderPalette[16],flags;
static int sTextLatched=1,sJiminyHeaderBlank;
#define PLTT16 live
#define IO16(x) flags
const uint16_t gJiminyBgPalette[96]={[81]=0x421,[82]=0x187,[83]=0x4321,[84]=0x3eef,[85]=0x6abc,[86]=0x1111,[87]=0x1234,[88]=0x3456,[89]=0x7777,[90]=0x4def,[91]=0x112,[92]=0x7654,[93]=0x4523,[94]=0x7832,[95]=0x3b21};
'''+function(hd.replace('static int FadeFits', 'int FadeFits'),'FadeFits')+'\n'+function(hd,'PortPaletteFade')+'\n'+function(ppu.replace('static void KeepJiminyHeaderPalette', 'void KeepJiminyHeaderPalette'),'KeepJiminyHeaderPalette')+'\n'+function(ppu,'PortPpuJiminyFade')+r'''
int main(void){int dark,white,badDark,badWhite;
 memcpy(live+80,gJiminyBgPalette+80,32);KeepJiminyHeaderPalette();
 memset(live+80,0,32);PortPpuJiminyFade(&dark,&white);PortPaletteFade(gJiminyBgPalette+80,live+80,16,&badDark,&badWhite);
 assert(dark==0&&white==0&&badDark==255&&badWhite==0);
 KeepJiminyHeaderPalette();PortPpuJiminyFade(&dark,&white);assert(dark==255&&white==0);
 for(int i=80;i<96;i++)live[i]=0x7fff;KeepJiminyHeaderPalette();PortPpuJiminyFade(&dark,&white);assert(dark==0&&white==255);
 memset(live+80,0,32);flags=0x80;KeepJiminyHeaderPalette();flags=0;PortPpuJiminyFade(&dark,&white);assert(dark==0&&white==255);
 sTextLatched=0;PortPpuJiminyFade(&dark,&white);assert(dark==0&&white==0);
 puts("PASS presented header palette survives next-frame mutation; black/white/forced-blank/unlatched controls");
}
'''
with tempfile.TemporaryDirectory(prefix='rekhcom-jiminy-fade-') as d:
 d=Path(d);c=d/'test.c';exe=d/'test.exe';c.write_text(unit,encoding='utf-8')
 subprocess.run([str(a.cc.resolve()),'-std=c11','-O2','-Wall',str(c),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
