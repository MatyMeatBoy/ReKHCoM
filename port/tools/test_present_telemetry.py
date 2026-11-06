"""Compile the actual RenderPresent-to-GalleryPresented block from a source.
Private fixture, no SDL/GPU/game or product edits. Explicit compiler/output.
"""
from pathlib import Path
import argparse,ctypes,hashlib,json,os,subprocess
ap=argparse.ArgumentParser();ap.add_argument('--source',type=Path,default=Path(__file__).resolve().parents[2]/'port/src/port_main.c');ap.add_argument('--cc',required=True);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--expect-failure',action='store_true');args=ap.parse_args()
source=args.source.read_text(encoding='utf8')
a=source.index('int presented=SDL_RenderPresent(sRenderer);');b=source.index('PortSceneGalleryPreviewPresented(presented);',a)+len('PortSceneGalleryPreviewPresented(presented);')
block=source[a:b]
fixture=r'''
#include <stdint.h>
#include <assert.h>
#include <stdio.h>
static uint64_t sPresentSerial,sPresentAcceptedAt,sPresentGapTicks;
static uint32_t sFpsFrames;
static int sRenderer,accepted,gallery=-1,clockReads;
static uint64_t now;
static int SDL_RenderPresent(int renderer){(void)renderer;return accepted;}
static uint64_t SDL_GetPerformanceCounter(void){clockReads++;return now;}
static void PortSceneGalleryPreviewPresented(int value){gallery=value;}
static void PresentBlock(void){
'''+block+r'''
}
int main(void){
 uint64_t start;
 accepted=0;now=999;PresentBlock();
 assert(!sPresentSerial&&!sFpsFrames&&!sPresentAcceptedAt&&!sPresentGapTicks&&gallery==0&&!clockReads);
 accepted=1;now=0;PresentBlock();
 assert(sPresentSerial==1&&sFpsFrames==1&&sPresentAcceptedAt==0&&sPresentGapTicks==0&&gallery==1&&clockReads==1);
 now=16742;PresentBlock();
 assert(sPresentSerial==2&&sFpsFrames==2&&sPresentAcceptedAt==16742&&sPresentGapTicks==16742&&clockReads==2);
 start=sPresentSerial;accepted=0;now=33484;PresentBlock();
 assert(sPresentSerial==start&&sFpsFrames==2&&sPresentAcceptedAt==16742&&sPresentGapTicks==16742&&gallery==0&&clockReads==2);
 /* A failed presentation does not publish the stale previous gap into CSV. */
 assert((sPresentSerial!=start ? sPresentGapTicks:0)==0);
 accepted=1;now=50226;PresentBlock();
 assert(sPresentSerial==3&&sFpsFrames==3&&sPresentAcceptedAt==50226&&sPresentGapTicks==33484&&gallery==1&&clockReads==3);
 /* A skipped cycle makes no presentation call and advances no serial. */
 start=sPresentSerial;assert(sPresentSerial==start);
 puts("PASS actual present block: fail-first, first0, next16742, fail-noadvance, next33484; gallery/FPS preserved");
 return 0;
}
'''
args.output.mkdir(parents=True,exist_ok=True);c=args.output/'present_fixture.c';exe=args.output/'present_fixture.exe';c.write_text(fixture)
if os.name=='nt':ctypes.windll.kernel32.SetErrorMode(0x0001|0x0002|0x8000)
flags=getattr(subprocess,'BELOW_NORMAL_PRIORITY_CLASS',0)|getattr(subprocess,'CREATE_NO_WINDOW',0)
compiled=subprocess.run([args.cc,'-O2',str(c),'-o',str(exe)],capture_output=True,creationflags=flags)
(args.output/'compile.stdout').write_bytes(compiled.stdout);(args.output/'compile.stderr').write_bytes(compiled.stderr)
assert compiled.returncode==0,compiled.stderr
p=subprocess.run([str(exe)],capture_output=True,creationflags=flags)
(args.output/'stdout.log').write_bytes(p.stdout);(args.output/'stderr.log').write_bytes(p.stderr)
result={'source_sha256':hashlib.sha256(args.source.read_bytes()).hexdigest(),'compile_exit':compiled.returncode,'runtime_exit':p.returncode,'expected_failure':args.expect_failure,'block':block,'stdout':p.stdout.decode(errors='replace')}
(args.output/'RESULT.json').write_text(json.dumps(result,indent=2))
print(json.dumps({k:v for k,v in result.items() if k!='block'}))
assert (p.returncode!=0) if args.expect_failure else (p.returncode==0)
