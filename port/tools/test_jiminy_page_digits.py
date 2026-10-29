"""Exercise the actual journal page emitter and decimal loader without a ROM."""
import argparse
from pathlib import Path
import subprocess
import tempfile
from test_mapcard_definition_bounds import function

ROOT=Path(__file__).resolve().parents[2]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc',type=Path,required=True)
    args=parser.parse_args()
    journal=(ROOT/'src/menu/mode_jiminy.c').read_text(encoding='utf-8')
    start=journal.index('        source = gJiminyDigitTiles;')
    emit=journal[start:journal.index('        sJiminyWork->state =',start)]
    loader=function((ROOT/'src/menu/mode_ms_top.c').read_text(encoding='utf-8'),'LoadDecimalDigitTiles')
    split=function(journal,'SplitThreeDecimalDigits')
    code=r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint8_t u8;typedef uint16_t u16;typedef uint32_t u32;typedef int16_t s16;typedef int32_t s32;
static u8 memory[1024],gJiminyDigitTiles[320];
static int copies,tags,codes[32];
static void* GetBgCharBase(int bg){(void)bg;return memory;}
static void RequestDma3Copy(const void*src,void*dst,u16 size){memcpy(dst,src,size);copies++;}
static void PortRegisterBgTextTile(void*dst,int code,int style,const void*src){
 (void)style;if(memcmp(dst,src,32))__builtin_trap();codes[((u8*)dst-memory)/32]=code;tags++;
}
'''+loader+'\n'+split+'\n'+r'''
static void Emit(int selected,int unlocked){u8* source;u8* dest;u8 digits[3];
'''+emit+r'''
}
int main(void){
 int value,i,total;for(i=0;i<10;i++)memset(gJiminyDigitTiles+i*32,i,32);
 for(value=1;value<=999;value++){
  total=1000-value;copies=tags=0;memset(memory,0xff,sizeof(memory));memset(codes,0,sizeof(codes));
  Emit(value-1,total);
  if(copies!=6)return 95;
  for(i=0;i<6;i++){
   int number=i<3?value:total,position=i%3,divisor=position==0?100:position==1?10:1;
   int digit=number/divisor%10;
   if(memcmp(memory+(i+1)*32,gJiminyDigitTiles+digit*32,32))return 96;
   if(codes[i+1]!='0'+digit){puts("FAIL journal page digits lack current HD metadata");return 97;}
  }
  if(tags!=6)return 98;
 }
 puts("PASS999 current/total pairs, six original DMA copies and six current HD tags");return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix='jiminy-digits-') as temp:
        root=Path(temp);unit=root/'probe.c';exe=root/'probe.exe';unit.write_text(code)
        subprocess.run([str(args.cc.resolve()),'-DPLATFORM_PC','-std=c11','-O2',str(unit),'-o',str(exe)],check=True)
        return subprocess.run([str(exe)],timeout=10).returncode

if __name__=='__main__':raise SystemExit(main())
