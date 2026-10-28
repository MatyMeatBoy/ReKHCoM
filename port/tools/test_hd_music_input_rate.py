"""Probe real guest rate notification, host dispatch and HD mixer consumption."""
from pathlib import Path
import subprocess,tempfile

ROOT=Path(__file__).resolve().parents[2]
shared=ROOT/'port/ps5/shared'
music=(shared/'hd_music_host.h').read_text()
shim=(ROOT/'port/ps5/guest/src/sdl_shim.c').read_text()
def function(source,signature):
    start=source.index(signature)
    return source[start:source.index('\n}',start)+2]

pre=r'''
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "audio_bridge.h"
static int sHdInputRate;
'''+function(music,'static int HdMusicInputRate(')+r'''
#define THREAD_HOST_AUDIO_RATE(rate) HdMusicInputRate(rate)
#include "thread_host.h"
static int h_thread(int op,uint32_t a,uint32_t b,uint32_t c,uint32_t d){return H_thread(op,a,b,c,d);}
static double actualRatio;
static void h_audio_ratio(double r){actualRatio=r;}
typedef struct {int inRate,outRate;float ratio;} SDL_AudioStream;
typedef struct {int freq;} SDL_AudioSpec;
'''+function(shim,'static void SetRatio(')+function(shim,'bool SDL_SetAudioStreamFormat(')+r'''
static struct {int playing;void*vorbis;unsigned rate;int gain;float a[2],b[2];double frac;}sHd;
static unsigned steps,delivered;
static int16_t original[1024*2];
static int HdNext(float*out){steps++;out[0]=out[1]=0;return 1;}
static void Sink(const int16_t*p,uint32_t n){
 if(memcmp(p,original,n*4))abort();
 delivered+=n;
}
'''+function(music,'static void HdMusicMix(')+function(music,'static void HdMusicPush(')
post=r'''
static void Run(unsigned rate,unsigned trackRate){
 steps=delivered=0;sHd.frac=0;sHd.rate=trackRate;
 for(unsigned left=rate;left;){unsigned n=left<1024?left:1024;HdMusicPush(original,n,48000,Sink);left-=n;}
 if(delivered!=rate)abort();
 if(steps+1<trackRate || steps>trackRate+1){
  fprintf(stderr,"input %u track %u consumed %u\n",rate,trackRate,steps);exit(2);
 }
}
int main(void){
 for(unsigned i=0;i<2048;i++)original[i]=(int16_t)(i*17-15000);
 sHd.playing=1;sHd.vorbis=(void*)1;sHd.gain=0;
 unsigned rates[]={32768,48000,63072,65536};
 float ratios[]={.997f,1.0f,1.003f};
 SDL_AudioStream stream={48000,48000,1.0f};
 for(unsigned r=0;r<4;r++){
  SDL_AudioSpec spec={rates[r]};
  for(unsigned p=0;p<3;p++){
   stream.ratio=ratios[p];
   /* Force the real format-change branch, as the first game queue does. */
   stream.inRate=1;SDL_SetAudioStreamFormat(&stream,&spec,NULL);
   if(sHdInputRate!=(int)rates[r])return 3;
   double expected=(double)stream.ratio*rates[r]/48000;
   if(actualRatio<expected-1e-8 || actualRatio>expected+1e-8)return 4;
   Run(rates[r],44100);Run(rates[r],48000);
  }
 }
 int kept=sHdInputRate;
 if(H_thread(BRIDGE_AUDIO_INPUT_RATE,0,0,0,0) ||
    H_thread(BRIDGE_AUDIO_INPUT_RATE,384001,0,0,0) ||
    H_thread(BRIDGE_AUDIO_INPUT_RATE,UINT32_MAX,0,0,0) || sHdInputRate!=kept)return 5;
 if(H_thread(0,0,0,0,0)!=1 || H_thread(0x205,0,0,0,0)!=0)return 6;
 /* An older guest sends no rate command and retains its host-open contract. */
 sHdInputRate=0;Run(48000,48000);
 /* Disabled HD passes original PCM through byte-for-byte at every rate. */
 sHd.playing=0;delivered=0;HdMusicPush(original,1024,65536,Sink);
 if(delivered!=1024)return 7;
 puts("PASS 24 input/track/PLL contracts; GBA PCM exact; invalid rates rejected; legacy/core/disabled-HD preserved");
}
'''
for host in ('windows64/windows64_host.c','linux64/linux64_host.c','android64/host/android64_host.c',
             'ps4/ps4_host.c','ps5/app/ps5_host.c','switch/host/switch_host.c'):
    text=(ROOT/'port'/host).read_text()
    assert text.index('#include "hd_music_host.h"')<text.index('#include "thread_host.h"'),host
assert 'h_thread(BRIDGE_AUDIO_INPUT_RATE, (uint32_t)sAudio.inRate' in function(shim,'SDL_AudioStream* SDL_OpenAudioDeviceStream(')
for script,count in (('switch/build_switch.py',1),('vita/build_vita.py',2)):
    text=(ROOT/'port'/script).read_text()
    assert text.count("PS5 / 'shared/audio_bridge.h'")==count,script
cc=next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
with tempfile.TemporaryDirectory(prefix='hd-rate-') as tmp:
    path=Path(tmp);src=path/'probe.c';exe=path/'probe.exe'
    src.write_text(pre+post)
    subprocess.run([str(cc),'-I',str(shared),str(src),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS all six bridge hosts include the shared rate dispatcher; physical devices remain unverified')
