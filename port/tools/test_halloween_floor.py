"""Compile actual affine sampling functions against synthetic registers/VRAM."""
from pathlib import Path
import subprocess
import tempfile,argparse,json,hashlib
R=Path(__file__).resolve().parents[2]
a=argparse.ArgumentParser(description='CPU-only production affine sampler regression; no game, GPU or proprietary inputs.')
a.add_argument('--out',type=Path);a.add_argument('--ppu-source',type=Path);a.add_argument('--expect-failure',action='store_true');args=a.parse_args()
temp=None if args.out else tempfile.TemporaryDirectory(prefix='halloween-floor-',ignore_cleanup_errors=True)
E=args.out or Path(temp.name);E.mkdir(parents=True,exist_ok=True)
base=next(p for p in R.parents if (p/'toolchain').is_dir())
cc=next((base/'toolchain').glob('llvm-mingw-*/bin/i686-w64-mingw32-clang.exe')).resolve()
source=args.ppu_source or R/'port/src/port_ppu.c'
ppu=source.read_text(encoding='utf8')
def fn(name):
 a=ppu.rfind('\n',0,ppu.index(name+'('))+1;b=ppu.index('{',a);e=b+1;d=1
 while d:d+=(ppu[e]=='{')-(ppu[e]=='}');e+=1
 return ppu[a:e]+'\n'
pre=r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define PPU_TLS
#define GBA_SCREEN_H 160
#define GBA_SCREEN_W 240
#define PORT_MAX_W 640
#define TRANSPARENT 0x8000
static uint8_t vram[0x20000];static uint16_t palette[512],io[256];
#define IO16(x) io[(x)/2]
#define VRAM8 vram
#define PLTT16 palette
static struct {int width,originX,wide;} gPortScreen;
static int sBattleView,sLeft,sRight,sJafarFight,sArenaBands[6],sDarksideLayer[4],sInWorld[640];
static int sHalloweenFloor=1;
static uint8_t gBtlBgHalloweenTownTiles[16384];
static int32_t sAffX[2],sAffY[2],sMosAffX[2],sMosAffY[2];
static int sMosNextGy[2]={0x7fffffff,0x7fffffff};
static uint16_t sBgLine[4][640];
#define CHECK(c) do {if(!(c)){fprintf(stderr,"FAIL %d: %s\n",__LINE__,#c);return 77;}}while(0)
'''
header=R/'port/include/port/port_halloween_floor.h'
if header.exists():pre+='\n#include "port/port_halloween_floor.h"\n'
identity='sHalloweenFloor=0;'
if 'sHalloweenFloor = sBattleView' in ppu:
 begin=ppu.index('sHalloweenFloor = sBattleView');identity=ppu[begin:ppu.index(';',begin)+1]
pre+='\nstatic void MatchLoadedFloor(void){'+identity+'}\n'
core=''.join(fn(n) for n in ['ArenaBandPixel','MosaicBlockStart','AffineCopyShown','AffineSingleCopy','DrawStraightAffine','DrawAffineBg'])
(E/'test.c').write_text(pre+core+(R/'port/tools/halloween_floor_conformance.c').read_text())
a=[str(cc),'-O2','-std=gnu11','-I'+str(R/'port/include'),str(E/'test.c'),'-o',str(E/'test.exe')]
s=subprocess.run(a,capture_output=True,text=True);(E/'compile.log').write_text(s.stdout+s.stderr);s.check_returncode()
s=subprocess.run([str(E/'test.exe')],capture_output=True,text=True,timeout=20,creationflags=0x08000000)
print(s.stdout+s.stderr,end='');(E/'result.log').write_text(s.stdout+s.stderr)
(E/'RESULT.json').write_text(json.dumps(dict(source=str(source),source_sha256=hashlib.sha256(source.read_bytes()).hexdigest(),exit=s.returncode,expected_failure=args.expect_failure,scope='Extracted production C affine sampler with synthetic VRAM, not game/GPU validation'),indent=2))
assert s.returncode==(77 if args.expect_failure else 0),s.returncode

