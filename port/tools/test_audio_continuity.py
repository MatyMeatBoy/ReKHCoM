"""Actual SDL consumer/producer PCM underflow and recovery, without a device."""
from pathlib import Path
import os, subprocess
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'build/audio-continuity-tests';OUT.mkdir(parents=True,exist_ok=True)
c=r'''
#include <SDL3/SDL.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
static int failPut;
static bool InjectPut(SDL_AudioStream*s,const void*p,int bytes){
 if(failPut){failPut=0;return false;}return SDL_PutAudioStreamData(s,p,bytes);
}
#define SDL_PutAudioStreamData InjectPut
#include "port/port_audio_continuity.h"
#undef SDL_PutAudioStreamData
static PortAudioContinuity state;
static float in[4096],out[8192];
static Sint16 sinput[4096],soutput[8192];
static void f32_case(int partial) {
 SDL_AudioSpec spec={SDL_AUDIO_F32,2,48000};
 SDL_AudioStream*s=SDL_CreateAudioStream(&spec,&spec);assert(s);
 assert(PortAudioContinuityInit(&state,s));
 for(int i=0;i<4096;i++)in[i]=0.6f;
 assert(PortAudioContinuityPut(&state,s,in,sizeof(in)));
 assert(SDL_GetAudioStreamData(s,out,sizeof(in))==sizeof(in));assert(!memcmp(in,out,sizeof(in)));
 int gap=partial?32:512;
 assert(SDL_GetAudioStreamData(s,out,gap*8)==gap*8); /* baseline returns zero: abrupt starvation */
 float last=.6f,maxstep=0;
 for(int i=0;i<gap;i++){
   float step=fabsf(out[i*2]-last);if(step>maxstep)maxstep=step;
   assert(out[i*2]==out[i*2+1]);last=out[i*2];
 }
 assert(maxstep<=.00251f);
 assert(PortAudioContinuityUnderruns(&state,s)==1);
 if(!partial){
   assert(last==0);
   assert(SDL_GetAudioStreamData(s,out,512*8)==512*8);
   for(int i=0;i<1024;i++)assert(out[i]==0); /* no repeated block/noise */
 }
 for(int i=0;i<4096;i++)in[i]=-.4f;
 assert(PortAudioContinuityPut(&state,s,in,sizeof(in)));
 int tail=partial?240-gap:0;
 int frames=2048+tail;
 assert(SDL_GetAudioStreamData(s,out,frames*8)==frames*8);
 for(int i=0;i<frames;i++){
   float step=fabsf(out[i*2]-last);if(step>maxstep)maxstep=step;last=out[i*2];
   assert(out[i*2]==out[i*2+1]);
 }
 assert(maxstep<=.00251f);assert(last==-.4f);
 assert(PortAudioContinuityClear(&state,s));
 assert(SDL_GetAudioStreamData(s,out,512*8)==512*8);
 for(int i=0;i<1024;i++)assert(out[i]==0); /* reset cannot replay the previous tail */
 SDL_DestroyAudioStream(s);
 printf("PASS F32 %s underrun: max adjacent jump %.7f; untouched steady PCM; reset silent\n",partial?"partial":"complete",maxstep);
}
static void s16_case(void) {
 SDL_AudioSpec spec={SDL_AUDIO_S16,2,48000};
 SDL_AudioStream*s=SDL_CreateAudioStream(&spec,&spec);assert(s);assert(PortAudioContinuityInit(&state,s));
 for(int i=0;i<4096;i++)sinput[i]=16384;
 assert(PortAudioContinuityPut(&state,s,sinput,sizeof(sinput)));
 assert(SDL_GetAudioStreamData(s,soutput,sizeof(sinput))==sizeof(sinput));assert(!memcmp(sinput,soutput,sizeof(sinput)));
 assert(SDL_GetAudioStreamData(s,soutput,512*4)==512*4);
 int last=16384,maxstep=0;
 for(int i=0;i<512;i++){int d=abs((int)soutput[i*2]-last);if(d>maxstep)maxstep=d;last=soutput[i*2];}
 assert(maxstep<=69 && last==0);
 for(int i=0;i<4096;i++)sinput[i]=-12000;
 assert(PortAudioContinuityPut(&state,s,sinput,sizeof(sinput)));
 assert(SDL_GetAudioStreamData(s,soutput,sizeof(sinput))==sizeof(sinput));
 for(int i=0;i<2048;i++){int d=abs((int)soutput[i*2]-last);if(d>maxstep)maxstep=d;last=soutput[i*2];}
 assert(maxstep<=69 && last==-12000);
 SDL_DestroyAudioStream(s);printf("PASS S16: max adjacent jump %d/32768, steady PCM exact\n",maxstep);
}
static void boundary_cases(void){
 SDL_AudioSpec spec={SDL_AUDIO_F32,2,48000};SDL_AudioStream*s=SDL_CreateAudioStream(&spec,&spec);assert(s);
 assert(PortAudioContinuityInit(&state,s));
 for(int i=0;i<4096;i++)in[i]=.6f;
 assert(PortAudioContinuityPut(&state,s,in,sizeof(in)));
 assert(SDL_GetAudioStreamData(s,out,sizeof(in))==sizeof(in));
 failPut=1;for(int i=0;i<4096;i++)in[i]=-.4f;
 assert(!PortAudioContinuityPut(&state,s,in,sizeof(in)));
 assert(SDL_GetAudioStreamData(s,out,512*8)==512*8);assert(out[0]>.59f); /* failed data cannot replace the tail */
 failPut=1;assert(!PortAudioContinuityPut(&state,s,in,sizeof(in)));
 assert(SDL_GetAudioStreamData(s,out,512*8)==512*8);
 for(int i=0;i<1024;i++)assert(out[i]==0); /* rejected fade-in cannot resurrect stale data */
 assert(PortAudioContinuityPut(&state,s,in,sizeof(in)));
 assert(PortAudioContinuityClear(&state,s));
 for(int i=0;i<4096;i++)in[i]=.6f;
 assert(PortAudioContinuityPut(&state,s,in,sizeof(in)));
 assert(SDL_GetAudioStreamData(s,out,sizeof(in))==sizeof(in));
 failPut=1;assert(SDL_GetAudioStreamData(s,out,512*8)==0); /* failed callback enqueue returns without spinning */
 assert(SDL_GetAudioStreamData(s,out,512*8)==512*8);
 for(int i=0;i<1024;i++)assert(out[i]==0); /* failed consumer request must not replay its old tail later */
 assert(PortAudioContinuityClear(&state,s));
 assert(PortAudioContinuityPut(&state,s,in,sizeof(in)));
 assert(SDL_GetAudioStreamData(s,out,sizeof(in))==sizeof(in));
 assert(SDL_GetAudioStreamData(s,out,32*8)==32*8);
 assert(SDL_SetAudioStreamFrequencyRatio(s,1.2f));
 assert(PortAudioContinuityPut(&state,s,in,sizeof(in)));
 assert(SDL_GetAudioStreamQueued(s)==(2048+240-32)*8); /* rate changes do not restart/lengthen the active ramp */
 assert(PortAudioContinuityClear(&state,s));
 assert(SDL_LockAudioStream(s));PortAudioContinuityGet(&state,s,2147483647,2147483647);SDL_UnlockAudioStream(s);
 assert(SDL_GetAudioStreamQueued(s)==2048*8); /* huge demand is bounded by fixed scratch */
 SDL_DestroyAudioStream(s);puts("PASS: failed producer/callback/recovery, rate-change ramp and bounded demand");
}
int main(void){f32_case(0);f32_case(1);s16_case();boundary_cases();return 0;}
'''
(OUT/'test.c').write_text(c)
cc=next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/clang.exe'))
sdl=ROOT.parent/'toolchain/SDL3-3.4.18/x86_64-w64-mingw32'
subprocess.run([str(cc),'-O2',str(OUT/'test.c'),'-I'+str(ROOT/'port/include'),'-I'+str(sdl/'include'),'-L'+str(sdl/'lib'),'-lSDL3','-o',str(OUT/'test.exe')],check=True)
env=dict(os.environ,PATH=str(sdl/'bin')+os.pathsep+os.environ['PATH'])
p=subprocess.run([str(OUT/'test.exe')],env=env,text=True,capture_output=True)
print(p.stdout,p.stderr)
assert p.returncode==0,p.returncode
from test_hd_audio_flow import analyse
normal=[[n,1,1,1,100,n*804,0,0] for n in range(12)]
assert analyse(normal)['fallback_underruns']==0
masked=[r[:-1]+[int(r[0]>=9)] for r in normal]
assert analyse(masked)['underruns']==0 and analyse(masked)['fallback_underruns']==1
assert not analyse([r[:7] for r in normal])['fallback_observed']
print('PASS: synthesized queue cannot hide underflows in HD flow reports; legacy logs marked unobserved')
