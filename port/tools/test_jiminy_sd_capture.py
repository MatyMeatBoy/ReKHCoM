"""Presented journal name edges must remain available with HD artwork off."""
import argparse,subprocess,tempfile
from pathlib import Path
from test_mapcard_definition_bounds import function
root=Path(__file__).resolve().parents[2];p=argparse.ArgumentParser();p.add_argument('--cc',type=Path,required=True);p.add_argument('--baseline-ref');a=p.parse_args()
src=subprocess.check_output(['git','show',a.baseline_ref+':port/src/port_ppu.c'],text=True,encoding='utf8') if a.baseline_ref else (root/'port/src/port_ppu.c').read_text(encoding='utf8')
unit='''#include <stdint.h>
#include <assert.h>
#include <stdio.h>
typedef const uint32_t* EdgePtr;
static int sJiminyDetailCapture,sJiminyPaperCapture;
static const void* sJiminyBookFrame;
static uint32_t sJiminyNameEdges[48];
'''+function(src.replace('const uint32_t* PortPpuJiminyNameEdges','EdgePtr PortPpuJiminyNameEdges'),'PortPpuJiminyNameEdges')+'''
int main(void){
 sJiminyDetailCapture=1;sJiminyPaperCapture=0;sJiminyBookFrame=0;
 assert(PortPpuJiminyNameEdges()==sJiminyNameEdges); /* SD, valid detail */
 sJiminyPaperCapture=1;assert(PortPpuJiminyNameEdges()==sJiminyNameEdges);
 sJiminyDetailCapture=0;assert(!PortPpuJiminyNameEdges()); /* previous frame must not leak */
 sJiminyPaperCapture=0;sJiminyBookFrame=sJiminyNameEdges;assert(!PortPpuJiminyNameEdges());
 puts("PASS SD and HD presented detail edges; non-detail frames reject stale metadata");
}
'''
with tempfile.TemporaryDirectory(prefix='jiminy-sd-edges-') as folder:
 c=Path(folder)/'test.c';exe=Path(folder)/'test.exe';c.write_text(unit,encoding='utf8')
 subprocess.run([str(a.cc.resolve()),'-std=c11','-O2',str(c),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
