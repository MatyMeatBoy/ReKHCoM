"""Real ROM color fixtures through ObjColor and the HUD digit producer; no game/GPU build."""
import argparse,hashlib,subprocess,sys,tempfile
from pathlib import Path
import yaml
ROOT=Path(__file__).resolve().parents[2]
BASE=next(p for p in ROOT.parents if (p/'toolchain').is_dir())
sys.path.insert(0,str(ROOT/'tools'))
import sprite_sheet

def block(s,needle):
 a=s.index(needle);b=s.index('{',a);e=b+1;n=1
 while n:n+=(s[e]=='{')-(s[e]=='}');e+=1
 return s[a:e]
def array(name,data):return 'static const unsigned char '+name+'[]={'+','.join(map(str,data))+'};\n'
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--rom',type=Path);ap.add_argument('--region',choices=['eu','us','jp'],default='eu');a=ap.parse_args()
 rom=(a.rom or BASE/'khcom-crash-diagnostics/roms'/({'eu':'B8CP.gba','us':'B8CE.gba','jp':'B8CJ.gba'}[a.region])).read_bytes()
 palettes=yaml.safe_load((ROOT/'config/assets/sprite_palettes.yaml').read_text())['entries']
 entry=next(e for e in palettes if e.get('symbol')=='gBStatesPalette');offset=entry[a.region]['address']-0x8000000;palette=rom[offset:offset+32]
 sheets=yaml.safe_load((ROOT/'config/assets/sprites_card_pictures.yaml').read_text())['entries']
 fixtures='';tiles=[]
 for name in ['gCardPremiumValueDigitTiles','gCardModifiedValueDigitTiles']:
  e=next(e for e in sheets if e['name']==name);part=e[a.region];off=part['address']-0x8000000
  frames,_,raw=sprite_sheet.parse_records(rom[off:off+part['size']],len(e['frames']),len(e['animations']))
  assert len(raw)==320 and all(f[0][2]==i for i,f in enumerate(frames))
  fixtures+=array(name,raw);tiles.append(name)
 fixtures+=array('paletteBytes',palette)
 s=(ROOT/'port/src/port_ppu.c').read_text();obj=block(s,'static int ObjColor(')
 hud=s[s.index('void PortPpuRenderHud('):]
 register=block(hud,'if (gPortConfig.hdHud && sHudDigitOwners')
 ink=next(l.strip() for l in hud.splitlines() if l.strip().startswith('digitInk = digit &&'))
 consume=block(hud,'if (digitInk) {');cover=block(hud,'if (hdPart) {')
 helper=block(s,'static int HudDigitKeepsPalette(') if 'static int HudDigitKeepsPalette(' in s else ''
 code=PREAMBLE+fixtures+helper+obj+'\n'+'''static int Run(const void* role,const unsigned char* bytes,int tile,int present) {
 int i=0,w=16,h=16,hdPart=present,opaque=0,native=0;
 PortHudDigit* digit=NULL;SpriteTag tagValue={1};const SpriteTag*tag=&tagValue;
 sOamHd[0].tiles=role;sOamHd[0].id=present; sHudDigitCount=0;
 memcpy(VRAM8+0x10000,bytes,320);memcpy(PLTT16+256,paletteBytes,32);
 '''+register+'''
 for(int sy=0;sy<8;sy++)for(int sx=0;sx<8;sx++) {
 int px=sx,py=sy,digitInk=0,col=ObjColor(0,tile,sx,sy,8,1);
 if(col<0)continue;opaque++;
 uint32_t argb=0xFF000000u|((col&31)*255/31<<16)|(((col>>5)&31)*255/31<<8)|((col>>10)&31)*255/31;
 '''+cover+'\n'+ink+'\n'+consume+'''
 native++;
 }
 free(sHudDigits);sHudDigits=NULL;
 return present ? (native!=0) : (native!=opaque);
}
int main(void) {
 if(HudDigitKeepsPalette(NULL))return 3;
 for(int hud=0;hud<2;hud++)for(int tile=0;tile<10;tile++)for(int present=0;present<2;present++) {
 gPortConfig.hdHud=hud;
 if(Run(gCardPremiumValueDigitTiles,gCardPremiumValueDigitTiles,tile,present))return 1;
 if(Run(gCardModifiedValueDigitTiles,gCardModifiedValueDigitTiles,tile,present))return 2;
 }
 return 0;
}
'''
 cc=next((BASE/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
 with tempfile.TemporaryDirectory(prefix='rekhcom-digit-palette-') as d:
  p=Path(d)/'test.c';e=Path(d)/'test.exe';p.write_text(code)
  subprocess.run([str(cc),'-std=c99',str(p),'-o',str(e)],check=True)
  result=subprocess.run([str(e)],timeout=10,creationflags=0x08004000)
  assert result.returncode==0,f'actual ROM {a.region} colored digit fallback lost ink: {result.returncode}'
 print('PASS:',a.region,'real ROM digit palettes/tiles, 0-9, HD present/missing; sha256',hashlib.sha256(rom).hexdigest()[:12])
PREAMBLE=r'''
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
static unsigned char VRAM8[0x20000];static uint16_t PLTT16[512];
static struct {int hdHud;}gPortConfig={1};
static uint8_t owner[256],*sHudDigitOwners=owner,sHdCover[256];
static size_t sHudDigitOwnerSize=256;
typedef struct {int x,y,w,h,code,slot;uint32_t color;}PortHudDigit;
typedef struct {int digit;}SpriteTag;
static PortHudDigit*sHudDigits;static int sHudDigitCount;
static struct {const void*tiles;int id;}sOamHd[1];
static int PortHdSpriteKeepsCartridge(int id){(void)id;return 0;}
'''
if __name__=='__main__':main()
