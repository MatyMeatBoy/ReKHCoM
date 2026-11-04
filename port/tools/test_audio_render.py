#!/usr/bin/env python3
"""ROM-free regression for interpolation at PCM sample and loop boundaries."""
from pathlib import Path
import subprocess
r=Path(__file__).resolve().parents[2];
import os
os.chdir(r)
cc=next((r.parent/'toolchain').glob('llvm-mingw-*/bin/i686-w64-mingw32-clang.exe'))
c=[str(cc),'-O2','-std=gnu11','-ffunction-sections','-fdata-sections','-Wl,--gc-sections','-DPLATFORM_PC','-DVERSION_EU','-Iport/include','-I'+str(r.parent/'toolchain/SDL3-3.4.18/i686-w64-mingw32/include')]
for p in [Path('include')]+[p for p in Path('include').iterdir() if p.is_dir() and p.name!='gba']:c+=['-iquote',str(p)]
subprocess.run(c+['port/tools/mixer_conformance.c','-o','build/mixer_conformance.exe'],check=True)
subprocess.run([str(r/'build/mixer_conformance.exe')],check=True) # absolute: Windows CreateProcess does not resolve a forward-slash relative path

# Integrate the duty waveform independently, one eighth at a time.
source=(r/'port/src/port_audio.c').read_text()
duty=source[source.index('static double DutyIntegral'):source.index('static uint16_t RegFreq')]
prefix=source[source.index('static const uint8_t sDuty'):source.index('static const uint8_t sNrx4')]
reference=r"""
static double Reference(int duty,double p) {
    double periods=floor(p/8.0),remainder=p-periods*8.0,sum=0;
    int highs=0;
    for(int k=0;k<8;k++)highs+=sDuty[duty][k];
    for(int k=0;k<8 && remainder>0;k++){
        double part=remainder>=1.0?1.0:remainder;
        sum+=sDuty[duty][k]?part:-part;remainder-=part;
    }
    return periods*(2*highs-8)+sum;
}
int main(void){
    double worst=0;
    for(int d=0;d<4;d++)for(int i=-100000;i<100000;i++){
        double p=i/7919.0,error=fabs(Reference(d,p)-DutyIntegral(d,p));
        if(error>worst)worst=error;
    }
    printf("800000 duty integrals: maximum error %.17g\n",worst);
    return worst>1e-12;
}
"""
out=r/'build/audio-tests';out.mkdir(exist_ok=True)
(out/'duty.c').write_text('#include <math.h>\n#include <stdint.h>\n#include <stdio.h>\n#define SDL_floor floor\n'+prefix+duty+reference)
subprocess.run([str(cc),'-O2',str(out/'duty.c'),'-o',str(out/'duty.exe')],check=True)
subprocess.run([str(out/'duty.exe')],check=True)
