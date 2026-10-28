"""Focused production movie gain contract; CPU/SDL dummy, no device listening claim."""
from pathlib import Path
import re
import json
import os
import subprocess

ROOT = Path(__file__).resolve().parents[2]

def probe():
    out = ROOT / 'build/movie-master-volume'
    out.mkdir(parents=True, exist_ok=True)
    source = (ROOT / 'port/src/port_hdmovie.c').read_text()
    start = source.rindex('static int PlayFile(const char* name) {')
    guest = source[start:source.index('\n}', start)+2]
    code = r'''
#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "port/port_audio_continuity.h"
static SDL_AudioStream *sAudio;
static PortAudioContinuity sAudioContinuity;
#define Log(...) ((void)0)
#define PortLog(...) ((void)0)
#include "audio_bridge_sdl.h"
static int checks;
#define CHECK(x) do { checks++; if(!(x)){fprintf(stderr,"failed line %d: %s\n",__LINE__,#x);return 1;} }while(0)
static int oldHost, calls, actualMaster, outcome=1, refreshed;
static struct {int masterVolume,hdSubtitles;} gPortConfig;
static int sStartAt;
static int h_thread(int op,uint32_t a,uint32_t b,uint32_t c,uint32_t d){
 return oldHost ? oldHost-1 : HostAudioCommand(op,a,b,c,d);
}
static int h_hd_movie(const char*m,const char*a,const char*b,const char*f){
 calls++;actualMaster=PortMovieVolumeTake(&sMovieVolume);return outcome;
}
static void MoviePath(const char*n,char*p,size_t cap){SDL_strlcpy(p,n,cap);}
static void SubsOrder(const char**o){o[0]="en";o[1]="es";}
static void MovieSubtitlePath(const char*n,const char*l,char*p,size_t cap){SDL_strlcpy(p,n,cap);}
static const char*PortConfigPath(const char*p){return p;}
void PortBridgeRefreshTextureEpoch(void){refreshed++;}
GUEST
int main(void){
 CHECK(SDL_Init(SDL_INIT_AUDIO));
 SDL_AudioSpec spec={SDL_AUDIO_F32,2,48000};
 SDL_AudioStream*audio=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,NULL,NULL);
 CHECK(audio!=NULL);
 for(int master=0;master<=100;master+=50){
   for(int step=0;step<=4;step++){
     float fade=step*.25f;
     CHECK(SDL_SetAudioStreamGain(audio,PortMovieGain(master,fade)));
     CHECK(fabsf(SDL_GetAudioStreamGain(audio)-master*.01f*fade)<.00001f);
     float pcm[64];for(int i=0;i<64;i++)pcm[i]=.5f;
     CHECK(SDL_PutAudioStreamData(audio,pcm,sizeof(pcm)));
     CHECK(SDL_GetAudioStreamQueued(audio)>=sizeof(pcm));
     CHECK(SDL_PauseAudioStreamDevice(audio));
     CHECK(SDL_GetAudioStreamGain(audio)==PortMovieGain(master,fade));
     CHECK(SDL_ResumeAudioStreamDevice(audio));
     CHECK(SDL_GetAudioStreamGain(audio)==PortMovieGain(master,fade));
     SDL_PauseAudioStreamDevice(audio);SDL_ClearAudioStream(audio);
   }
   gPortConfig.masterVolume=master;oldHost=0;calls=0;
   CHECK(PlayFile("fixture.mp4")==1&&calls==1&&actualMaster==master&&!sMovieVolume.pending);
   CHECK(PortMovieVolumeTake(&sMovieVolume)==100); /* old guest on new host */
   for(int legacy=1;legacy<=3;legacy++){
     oldHost=legacy;calls=0;
     CHECK(PlayFile("fixture.mp4")== (master==100));
     CHECK(calls==(master==100));
   }
 }
 oldHost=0;gPortConfig.masterVolume=50;
 for(int result=0;result<=2;result++){
   outcome=result;CHECK(PlayFile("fixture.mp4")==!!result);
   CHECK(actualMaster==50&&!sMovieVolume.pending);
   CHECK(PortMovieVolumeTake(&sMovieVolume)==100);
 }
 CHECK(HostAudioCommand(BRIDGE_MOVIE_MASTER,0,0,0,0)==1);
 CHECK(HostAudioCommand(BRIDGE_MOVIE_PROBE,0,0,0,0)==BRIDGE_MOVIE_MAGIC);
 CHECK(PortMovieVolumeTake(&sMovieVolume)==0); /* probe has no lifecycle side effect */
 CHECK(!sAudioBridgeControls); /* movie commands do not negotiate/change game stream */
 CHECK(HostAudioCommand(BRIDGE_MOVIE_MASTER,0,0,0,0)==1);
 CHECK(HostAudioCommand(BRIDGE_MOVIE_MASTER,101,0,0,0)==0);
 CHECK(PortMovieVolumeTake(&sMovieVolume)==100);
 CHECK(HostAudioCommand(BRIDGE_MOVIE_MASTER,0xffffffff,0,0,0)==0);
 CHECK(HostAudioCommand(0x212,0,0,0,0)==-2);
 CHECK(PortMovieGain(-1,1)==0&&PortMovieGain(101,1)==1);
 CHECK(PortMovieGain(50,-1)==0&&PortMovieGain(50,2)==.5f);
 SDL_DestroyAudioStream(audio);SDL_Quit();
 printf("{\"checks\":%d,\"backend\":\"SDL dummy\",\"pcm_clock_preserved\":true}\n",checks);
 return 0;
}
'''.replace('GUEST', guest)
    src = out / 'probe.c'
    src.write_text(code)
    tc = ROOT.parent / 'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe'
    sdl = ROOT.parent / 'toolchain/SDL3-3.4.18/x86_64-w64-mingw32'
    cmd = [str(tc), '-O2', '-DPORT_LINUX64_GUEST', str(src), '-I'+str(ROOT/'port/include'),
           '-I'+str(ROOT/'port/ps5/shared'), '-I'+str(sdl/'include'), '-L'+str(sdl/'lib'),
           '-lSDL3', '-o', str(out/'probe.exe')]
    subprocess.run(cmd, check=True)
    env = dict(os.environ, SDL_AUDIO_DRIVER='dummy', PATH=str(sdl/'bin')+os.pathsep+os.environ['PATH'])
    result = subprocess.run([str(out/'probe.exe')], env=env, capture_output=True, text=True, check=True)
    report = json.loads(result.stdout)
    report['scope'] = 'Actual guest wrapper, shared host command/state and real SDL gain/queued PCM. No codec/device listening or platform runtime claim.'
    (out/'RESULT.json').write_text(json.dumps(report, indent=2)+'\n')
    print(result.stdout.strip())

def check():
    native = (ROOT / 'port/src/port_hdmovie.c').read_text()
    android = (ROOT / 'port/portable/hdmovie_android.c').read_text()
    linux = (ROOT / 'port/linux64/hdmovie_ffmpeg.inc').read_text()
    # Check the actual initial open/resume sites, not a parallel gain implementation.
    for name, source in [('native-mf-and-ffmpeg', native), ('android-codec', android), ('linux64-ffmpeg', linux)]:
        opens = list(re.finditer(r'audio = .*SDL_OpenAudioDeviceStream\([^;]+;', source))
        if not opens:
            raise RuntimeError(f'{name}: no production movie stream')
        for opening in opens:
            tail = source[opening.end():]
            resume = tail.index('SDL_ResumeAudioStreamDevice(audio)')
            if 'SDL_SetAudioStreamGain(audio, PortMovieGain(masterVolume, 1.0f))' not in tail[:resume]:
                raise RuntimeError(f'{name}: movie resumes at default full volume')
    for source in [native, android]:
        for expression in re.findall(r'SDL_SetAudioStreamGain\(audio, ([^;]+);', source):
            if 'PortMovieGain(masterVolume,' not in expression:
                raise RuntimeError('fade overwrites master: ' + expression)

if __name__ == '__main__':
    check()
    probe()
    print('production movie initial gain/fade PASS')
