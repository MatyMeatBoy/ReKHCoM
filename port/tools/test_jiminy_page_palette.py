"""Actual journal colour effects and native SD paper/guide roles."""
import argparse,re,struct,subprocess,tempfile
from pathlib import Path
from test_mapcard_definition_bounds import function
root=Path(__file__).resolve().parents[2];p=argparse.ArgumentParser();p.add_argument('--cc',type=Path,required=True);p.add_argument('--rom',type=Path);p.add_argument('--symbols',type=Path);a=p.parse_args()
src=(root/'port/src/port_ppu.c').read_text(encoding='utf8')
parts='\n'.join(function(src.replace('static uint16_t '+n,'uint16_t '+n).replace('static uint32_t '+n,'uint32_t '+n),n) for n in ('Brighten','Darken','JiminyBgColor'))
unit='''#include <stdint.h>
#include <assert.h>
#include <stdio.h>
#define TRANSPARENT 0xffff
static uint32_t sColorLut[32768];
'''+parts+'''
int main(void){
 for(int i=0;i<32768;i++)sColorLut[i]=0xff000000u|i;
 for(int c=0;c<32768;c+=211)for(int effect=0;effect<4;effect++)for(int amount=0;amount<=16;amount+=8){
  assert(JiminyBgColor(c,0,2,effect,amount)==sColorLut[c]);
  assert(JiminyBgColor(c,1,0,effect,amount)==sColorLut[c]);
  uint32_t expected=effect==1?0:sColorLut[effect==2?Brighten(c,amount):effect==3?Darken(c,amount):c];
  assert(JiminyBgColor(c,1,2,effect,amount)==expected);
 }
 assert(!JiminyBgColor(TRANSPARENT,0,0,0,0));
 assert(JiminyBgColor(0x1234,1,2,3,16)==0xff000000u);
 assert(JiminyBgColor(0x1234,1,2,2,16)==sColorLut[32767]);
 puts("PASS actual native fades/window-effect guards/alpha rejection; opaque black distinct from missing metadata");
}
'''
with tempfile.TemporaryDirectory(prefix='jiminy-page-palette-') as folder:
 c=Path(folder)/'test.c';exe=Path(folder)/'test.exe';c.write_text(unit,encoding='utf8');subprocess.run([str(a.cc.resolve()),'-std=c11','-O2',str(c),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
if bool(a.rom)!=bool(a.symbols):p.error('Use --rom and --symbols together')
if a.rom:
 data=a.rom.read_bytes();symbols={k:int(v,16) for k,v in re.findall(r'\.set _(\w+), (0x[0-9a-f]+)',a.symbols.read_text())};tiles=symbols['gJiminyBgTiles']-0x8000000
 for name in ('gJiminyStoryDetailMap','gJiminyCardDetailMap','gJiminyCharacterDetailMap'):
  def pixel(x,y):
   e=struct.unpack_from('<H',data,symbols[name]-0x8000000+2*((y//8)*32+x//8))[0];sx=7-x%8 if e&1024 else x%8;sy=7-y%8 if e&2048 else y%8;b=data[tiles+(e&1023)*32+sy*4+sx//2];return(e>>12)*16+(b>>4 if sx&1 else b&15)
  assert {pixel(200,y) for y in range(24,160)}=={59,63},(name,'Paper probe contains texture/shadow')
  assert [y for y in range(24,160) if pixel(200,y)==59]==[47,141],name
  assert all(pixel(232,y)==133 for y in range(42,147)),name
  assert all(pixel(x,80)==63 for x in range(220,224)),name
  assert all(pixel(x,80)>=128 for x in range(224,240)),(name,'Shaded edge bounds changed')
  assert all(136<=pixel(x,y)<=142 for y in range(20,24) for x in range(224,240)),(name,'Top paper edge overlaps PAGE foreground')
 print('PASS all three native SD maps: inner paper/rules, guide and shaded-edge roles verified without stretching edge texture')
