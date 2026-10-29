#!/usr/bin/env python3
"""Exercise the actual accepted-scene list and Trickmaster gate without ROM/SDL."""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
# Exact accepted Atlas versions: 8072e837 (081), 4bdd5835 (097), c6f2bb42 (Trickmaster).
# Negative controls have no approved default recipe and must not be promoted.
ACCEPTED = [4,7,44,45,53,54,55,81,87,90,95,97,98,99,102,104,110,112,115,116,117,118,120,121,122,124]
PENDING = [75,76,86,129,131,132]

def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--cc',default=os.environ.get('CC','cc'));a=ap.parse_args()
    source=(ROOT/'port/src/port_ppu.c').read_text(encoding='utf-8')
    declaration=re.search(r'static const int kPreliminarApproved\[\] = \{.*?\};',source,re.S).group()
    gate=re.search(r'int tmBoard\s*=\s*(.*?);',source,re.S).group(1)
    functions='\n'.join(re.search(r'int '+n+r'\(void\)\s*\{.*?\n\}',source,re.S).group() for n in ['PortArenasV2','PortArenasApproved'])
    code='#include <stdlib.h>\n#include <stdio.h>\n'+declaration+'\n'+functions+"""
struct { int valid; const void* const* map; } sBgLatch[4];
static int matched;
static int PortTrickmasterBackdropMatches(const void* const* p) { (void)p; return matched; }
int main(int argc,char**argv) {
 int bg=0,i,found=0,scene,expected;
 if(argc!=5)return 2;
 scene=atoi(argv[1]);sBgLatch[0].valid=atoi(argv[2]);matched=atoi(argv[3]);expected=atoi(argv[4]);
 if(scene>=0) {
  for(i=0;i<(int)(sizeof(kPreliminarApproved)/sizeof(kPreliminarApproved[0]));i++)found|=kPreliminarApproved[i]==scene;
  if(found!=expected){fprintf(stderr,"scene %d default=%d expected=%d\\n",scene,found,expected);return 1;}
 }else {
"""+' int tmBoard = '+gate+';\n'+"""
  if(!!tmBoard!=expected){fprintf(stderr,"trickmaster gate=%d expected=%d\\n",!!tmBoard,expected);return 1;}
 }
 return 0;
}
"""
    with tempfile.TemporaryDirectory(prefix='atlas-approved-contract-') as td:
        d=Path(td);(d/'test.c').write_text(code);exe=d/('test.exe' if os.name=='nt' else 'test')
        subprocess.run([a.cc,'-std=c11',str(d/'test.c'),'-o',str(exe)],check=True)
        failures=[]
        for scene in ACCEPTED+PENDING:
            result=subprocess.run([str(exe),str(scene),'0','0',str(int(scene in ACCEPTED))]);failures.append(result.returncode)
        for switch in [None,'1','0']:
            env=dict(os.environ);env.pop('KHCOM_ARENAS_V2',None)
            if switch is not None:env['KHCOM_ARENAS_V2']=switch
            for valid in [0,1]:
                for match in [0,1]:
                    result=subprocess.run([str(exe),'-1',str(valid),str(match),str(int(switch!='0' and valid and match))],env=env);failures.append(result.returncode)
        assert not any(failures), f'{sum(bool(x) for x in failures)} activation contracts failed'
    print('PASS:26 accepted scenes,6 pending negative controls,12 actual Trickmaster gate cases')

if __name__=='__main__':main()
