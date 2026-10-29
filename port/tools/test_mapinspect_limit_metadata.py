"""Production BG registration and map-counter emitter, without ROM/SDL.

Checks fixed99 metadata uses its own original pixels and the generated digits'
palette, while all original DMA copies/native tiles remain unchanged.
"""
import argparse
import json
from pathlib import Path
import subprocess
from test_mapcard_definition_bounds import function

ROOT=Path(__file__).resolve().parents[2]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();out=args.out.resolve();out.mkdir(parents=True,exist_ok=True)
    ppu=(ROOT/'port/src/port_ppu.c').read_text(encoding='utf-8')
    kind=ppu[ppu.index('typedef struct BgTextTile {'):ppu.index('} BgTextTile;')+len('} BgTextTile;')]
    registration=function(ppu,'PortRegisterBgTextTile')
    alias=function(ppu,'PortRegisterBgTextTileLike') if 'void PortRegisterBgTextTileLike(' in ppu else ''
    digits=function((ROOT/'src/menu/mode_ms_top.c').read_text(encoding='utf-8'),'LoadDecimalDigitTiles')
    counter=function((ROOT/'src/menu/mode_mapinspect.c').read_text(encoding='utf-8'),'MapInspectDrawCardTotal')
    pre=r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;typedef int32_t s32;
#define GBA_VRAM 0x06000000u
static unsigned char vram[65536],gMapInspectTiles[0x3000],gMapInspectTotalDigitTiles[320];
static int sMapInspectCardTotal,copies;
static void* GetBgCharBase(int bg){(void)bg;return (void*)(uintptr_t)GBA_VRAM;}
static void RequestDma3Copy(const void* src,void* dst,u16 size){
 uintptr_t at=(uintptr_t)dst-GBA_VRAM;
 if(at+size>sizeof(vram))__builtin_trap();memcpy(vram+at,src,size);copies++;
}
'''+kind+r'''
static BgTextTile sBgTextTiles[2048],sBgTextTilesPrev[2048];
'''
    main=r'''
int main(void){
 int value,i;unsigned char native[64];
 memset(gMapInspectTotalDigitTiles,0x33,sizeof(gMapInspectTotalDigitTiles));
 for(i=0;i<10;i++){
  memset(gMapInspectTotalDigitTiles+i*32,0x11,10);
  memset(gMapInspectTotalDigitTiles+i*32+10,0x22,8);
 }
 memcpy(gMapInspectTiles+0x13c0,gMapInspectTotalDigitTiles+9*32,32);
 memcpy(gMapInspectTiles+0x13e0,gMapInspectTotalDigitTiles+9*32,32);
 gMapInspectTiles[0x13c0+21]=0x33;gMapInspectTiles[0x13e0+22]=0x33;
 memcpy(native,gMapInspectTiles+0x13c0,64);
 for(value=0;value<=99;value++){
  memset(sBgTextTiles,0,sizeof(sBgTextTiles));memset(sBgTextTilesPrev,0,sizeof(sBgTextTilesPrev));
  memset(vram,0x77,sizeof(vram));memcpy(vram+0x13c0,native,64);copies=0;
  /* A previous menu owned this slot. Generic slash discovery skips a
     nonempty tag, even though its old fingerprint no longer matches. */
  sBgTextTiles[0x54].code='X';memset(sBgTextTiles[0x54].source,0xff,32);
  sMapInspectCardTotal=value;MapInspectDrawCardTotal();
  if(copies!=4 || memcmp(vram+0x13c0,native,64))return 96;
  if(sBgTextTiles[0x1d].code!='0'+value/10 || sBgTextTiles[0x1e].code!='0'+value%10)return 96;
  for(i=0;i<2;i++){
   BgTextTile *fixed=&sBgTextTiles[0x9e + i],*like=&sBgTextTiles[0x1f + i];
   if(fixed->code!='9' || fixed->style!=like->style || fixed->faceIndex!=like->faceIndex ||
      fixed->shadowIndex!=like->shadowIndex || fixed->panelIndex!=like->panelIndex ||
      memcmp(fixed->source,native+i*32,32)){
    printf("FAIL value=%d fixed=%x code=%u (expected9 with native signature)\n",value,0x9e + i,fixed->code);return 97;
   }
  }
  if(sBgTextTiles[0x9d].code || sBgTextTiles[0xa0].code)return 96;
  if(sBgTextTiles[0x54].code!='/' || memcmp(sBgTextTiles[0x54].source,gMapInspectTiles+0xa80,32)){
   puts("FAIL stale slash slot did not receive current counter meaning/signature");return 98;
  }
 }
 puts("PASS100 totals, fixed9 signatures/palette, original DMA/native pixels and neighbouring metadata");return 0;
}
'''
    unit=out/'probe.c';unit.write_text(pre+registration+'\n'+alias+'\n'+digits+'\n'+counter+'\n'+main)
    p=subprocess.run([str(args.cc.resolve()),'-O2','-std=c11','-DPLATFORM_PC',str(unit),'-o',str(out/'probe.exe')],capture_output=True,text=True)
    (out/'compile.log').write_text(p.stdout+p.stderr)
    if p.returncode:print(p.stderr);return p.returncode
    p=subprocess.run([str(out/'probe.exe')],capture_output=True,text=True,timeout=5)
    row=dict(exit=p.returncode,output=p.stdout,scope='Actual production emitters/metadata functions; synthetic tile pixels and immediate DMA. No game/font/device rendering claim.')
    (out/'RESULT.json').write_text(json.dumps(row,indent=2));print(json.dumps(row));return p.returncode


if __name__=='__main__':raise SystemExit(main())
