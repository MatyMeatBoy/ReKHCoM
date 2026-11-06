"""An unused/text-only mosaic register must not disable independent PPU rows."""
from pathlib import Path
import subprocess,tempfile,os,shutil,re
R=Path(__file__).resolve().parents[2]
s=(R/'port/src/port_ppu.c').read_text(encoding='utf8')
start=s.index('static int PpuRowsIndependent(void) {');end=s.index('\nstatic int GlyphSame(',start)
production=s[start:end]
if 'PpuAffineMosaicDependent()' in production:
 a=s.index('static int PpuAffineMosaicDependent(void) {')
 production=s[a:start]+production
 inner=s[s.index('static int PpuInnerKeep(int gy) {'):s.index('/* The kept rows 0..gy1',s.index('static int PpuInnerKeep(int gy) {'))]
 assert 'PpuAffineMosaicDependent()' in inner,'HBlank must re-evaluate the same dependency'
code='''#include <stdint.h>
#include <assert.h>
#include <stdio.h>
static uint16_t io[64];
#define IO16(off) io[(off)/2]
static int sBgIsoLatched[4],sPpuStats[5];
static struct {int header;} sOamTag[1];
static int OamCount(void){return 0;}
'''+production+'''
int main(void){int mode,on2,on3,mos2,mos3,vertical;
for(mode=0;mode<8;mode++)for(on2=0;on2<2;on2++)for(on3=0;on3<2;on3++)
for(mos2=0;mos2<2;mos2++)for(mos3=0;mos3<2;mos3++)for(vertical=0;vertical<2;vertical++){
 IO16(0)=mode|(on2?0x400:0)|(on3?0x800:0);
 IO16(0xC)=mos2?0x40:0;IO16(0xE)=mos3?0x40:0;IO16(0x4C)=vertical?0x10:0;
 assert(!!PpuRowsIndependent()==!(vertical&&((mode!=0&&on2&&mos2)||(mode==2&&on3&&mos3))));
}
puts("PASS actual PPU row dependency:256 mode/enable/mosaic combinations; HBlank shares gate");return 0;
}
'''
cc=os.environ.get('CC') or shutil.which('cc')
if not cc:cc=str(R.parent.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-clang.exe')
with tempfile.TemporaryDirectory() as d:
 p=Path(d);(p/'mosaic.c').write_text(code)
 subprocess.run([cc,str(p/'mosaic.c'),'-o',str(p/'mosaic.exe')],check=True)
 subprocess.run([str(p/'mosaic.exe')],check=True)
