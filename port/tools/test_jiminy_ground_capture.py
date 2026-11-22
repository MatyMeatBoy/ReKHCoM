"""Compile the actual PPU ownership capture for the journal outer ground."""
from pathlib import Path
import argparse,subprocess,tempfile
root=Path(__file__).resolve().parents[2];p=argparse.ArgumentParser();p.add_argument('--cc',type=Path,required=True);a=p.parse_args()
src=(root/'port/src/port_ppu.c').read_text(encoding='utf-8');start=src.index('        if (sJiminyDetailCapture && objects && gx >= 0 && gx < 3 &&');end=src.index('        if (sJiminyDetailCapture && objects && gx >= 200',start);capture=src[start:end]
unit=r'''#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "port/jiminy_paper.h"
static PortJiminyPageRow sJiminyPageRows[160];
static int sJiminyDetailCapture;
static void capture(int objects,int gx,int gyLine,int topLayer,int blend){
'''+capture+r'''}
int main(void){
 int xs[]={-1,0,1,2,3,239},ys[]={-1,15,16,80,159,160};
 for(int d=0;d<2;d++)for(int o=0;o<2;o++)for(int layer=-1;layer<6;layer++)for(int blend=0;blend<2;blend++)
  for(unsigned x=0;x<sizeof(xs)/sizeof(xs[0]);x++)for(unsigned y=0;y<sizeof(ys)/sizeof(ys[0]);y++){
   memset(sJiminyPageRows,0,sizeof(sJiminyPageRows));sJiminyDetailCapture=d;
   capture(o,xs[x],ys[y],layer,blend);
   int expected=d&&o&&layer==1&&!blend&&xs[x]>=0&&xs[x]<3&&ys[y]>=16&&ys[y]<160;
   for(int row=0;row<160;row++)assert(sJiminyPageRows[row].leftOwned==(expected&&row==ys[y]?(1u<<xs[x]):0));
  }
 assert(sizeof(PortJiminyPageRow)==20);
 puts("PASS actual PPU capture: presented detail only, three ground columns, opaque BG1, valid rows; header/objects/blends and boundaries preserved");
}
'''
with tempfile.TemporaryDirectory(prefix='jiminy-ground-')as folder:
 c=Path(folder)/'test.c';exe=Path(folder)/'test.exe';c.write_text(unit,encoding='utf-8');subprocess.run([str(a.cc.resolve()),'-std=c11','-O2','-Wall','-I'+str(root/'port/include'),str(c),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
