"""Check portable catalogue integrity and execute the production region-to-asset resolver."""
import json,subprocess,sys
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'port/tools'))
from core_assets import gallery_posters
posters=gallery_posters();assert len(posters)==199
for p in posters:
 with Image.open(p) as image:
  assert image.size==(360,240)
  assert any(lo!=hi for lo,hi in image.convert('RGB').getextrema()),f'Blank poster: {p.name}'
print('PASS 199 nonblank portable engine PNGs, dimensions and SHA256 manifest')
source=(ROOT/'port/src/port_scene_preview.c').read_text(encoding='utf-8')
start=source.index('int PortScenePosterPreview(');code=source[start:]
prefix=r'''#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "evt/event_ids.h"
#include "port/port_scene_gallery.h"
#define SDL_snprintf snprintf
#define SDL_free free
static char requested[128];
static void* PortLoadAsset(const char*name,size_t*size){snprintf(requested,sizeof(requested),"%s",name);*size=1;return malloc(1);}
static int PortScenePosterDecode(const void*data,size_t size,uint32_t*out,int*w,int*h){return 1;}
'''
test=r'''
int main(){uint32_t pixel;int w,h;int actual[]={EVENT_000_1F_ENTRANCE_PART1,EVENT_129_HOLLOWBASTION_E0,EVENT_149_RIKU_B12F_OPNING,EVENT_195_RIKU_ENDING,PORT_SCENE_MOVIE(1),PORT_SCENE_MOVIE(5)};int canonical[]={0,129,149,195,513,517};char expected[128];
for(int i=0;i<6;i++){snprintf(expected,sizeof(expected),"assets/gallery/gba/scene-%03d.png",canonical[i]);assert(PortScenePosterPreview(actual[i],&pixel,&w,&h));assert(!strcmp(requested,expected));}
assert(!PortScenePosterPreview(-1,&pixel,&w,&h));assert(!PortScenePosterPreview(512,&pixel,&w,&h));assert(!PortScenePosterPreview(518,&pixel,&w,&h));puts("PASS production region asset resolver, movie IDs and invalid ID guards");}
'''
out=Path('D:/01Claude/ReKHCoM-build/previews-gba-20261026/region-probe');out.mkdir(parents=True,exist_ok=True);(out/'probe.c').write_text(prefix+code+test,encoding='utf-8')
compiler=ROOT.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/i686-w64-mingw32-clang.exe'
for region in ('EU','US','JP'):
 exe=out/f'probe-{region}.exe'
 subprocess.run([str(compiler),'-std=gnu11','-O1','-DVERSION_'+region,'-I'+str(ROOT/'include'),'-I'+str(ROOT/'port/include'),str(out/'probe.c'),'-o',str(exe)],check=True)
 subprocess.run([str(exe)],check=True)
 print('PASS',region,flush=True)
