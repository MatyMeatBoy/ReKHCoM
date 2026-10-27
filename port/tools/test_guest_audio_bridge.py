"""Read-only production guest/SDL-host contract probe; no game or device quality claim."""
import json, os, re, subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
REPO = ROOT
OUT = ROOT / 'build/audio-bridge-contract'
OUT.mkdir(parents=True,exist_ok=True)
shim = (REPO / 'port/ps5/guest/src/sdl_shim.c').read_text()
host = (REPO / 'port/windows64/windows64_host.c').read_text()
ring = (REPO / 'port/ps4/ps4_host.c').read_text()
audio = (REPO / 'port/src/port_audio.c').read_text()

def function(src, signature):
    start = src.index(signature)
    opening = src.index('{', start)
    level = 1
    i = opening + 1
    while level:
        level += (src[i] == '{') - (src[i] == '}')
        i += 1
    return src[start:i]

guest = '\n'.join(function(shim, sig) for sig in [
    'static void SetRatio(', 'SDL_AudioStream* SDL_OpenAudioDeviceStream(',
    'bool SDL_PauseAudioStreamDevice(', 'bool SDL_ResumeAudioStreamDevice(',
    'bool SDL_SetAudioStreamGain(', 'bool SDL_SetAudioStreamFrequencyRatio(',
    'bool SDL_SetAudioStreamFormat(', 'bool SDL_GetAudioStreamFormat(', 'bool SDL_PutAudioStreamData(',
    'int SDL_GetAudioStreamQueued('])
guest = re.sub(r'\bSDL_(\w+)\(', r'Guest_\1(', guest)
guest = re.sub(r'\bSDL_AudioStream\b', 'GuestStream', guest)
queue = re.sub(r'\bSDL_(\w+)\(', r'Guest_\1(', function(audio,'static void Queue('))
hostopen = function(host, 'static int H_audio_open(')
hostqueued = function(host, 'static uint32_t H_audio_queued(')
hostratio = function(host, 'static void H_audio_ratio(')
guest_struct = function(shim,'struct SDL_AudioStream {').replace('struct SDL_AudioStream','typedef struct GuestStream',1)+' GuestStream;'
audio_commands = ('#include "audio_bridge_sdl.h"\n' if (ROOT/'port/ps5/shared/audio_bridge_sdl.h').exists() else '')
thread_call = 'return H_thread(op,a,b,c,d);'
c = r'''
#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
static void Log(const char *fmt, ...) {(void)fmt;}
static void SetError(const char *s) {(void)s;}
static SDL_AudioStream *sAudio;
'''+('#include "port/port_audio_continuity.h"\nstatic PortAudioContinuity sAudioContinuity;\n' if (ROOT/'port/include/port/port_audio_continuity.h').exists() else '')+audio_commands+r'''
static int sGameRate=48000;
static int16_t lastSample;
'''+hostopen+'\n'+hostqueued+'\n'+hostratio+'\n'+function(host,'static void GamePush(')+'\n'+function(host,'static void H_audio_clear(')+'\n'+function((ROOT/'port/windows64/hdmovie_windows.inc').read_text(),'static void PortAudioMovieHold(')+r'''
static int HostGpuCommand(int o,uint32_t a,uint32_t b,uint32_t c,uint32_t d){return -2;}
static int H_thread_legacy(int o,uint32_t a,uint32_t b,uint32_t c,uint32_t d){return 0;}
'''+function(host,'static int H_thread(')+r'''
static int h_thread(int op,uint32_t a,uint32_t b,uint32_t c,uint32_t d){
#ifdef RING_BACKEND
 return 0;
#else
'''+thread_call+r'''
#endif
}
static double sRatio=1.0,sPos;
static int16_t sLast[2];
static unsigned sRead,sWrite,ringQueued;
static void RingPut(const int16_t *p,uint32_t n) {(void)p;ringQueued+=n;}
'''+function(ring,'static void GamePush(').replace('GamePush(', 'RingGamePush(')+r'''
static int h_audio_open(int n) {return H_audio_open(n);}
static void h_audio_push(const int16_t *p,uint32_t n) {
    if(n)lastSample=p[0];
#ifdef RING_BACKEND
    RingGamePush(p,n);
#else
    GamePush(p,n);
#endif
}
static uint32_t h_audio_queued(void) {
#ifdef RING_BACKEND
    return ringQueued;
#else
    return H_audio_queued();
#endif
}
static void h_audio_ratio(double r) {sRatio=r;H_audio_ratio(r);}
'''+guest_struct+r'''
static GuestStream sAudioGuest;
'''+guest.replace('sAudio.', 'sAudioGuest.').replace('return &sAudio;', 'return &sAudioGuest;')+r'''
static GuestStream *sStream;
static int sStreamRate,sStreamPriming=1;
static float sQueuedAvg=-1,sOut[4096];
#define GBA_FPS 59.727500569606
static int AudioLog(void) {return 0;}
#define PortLog Log
static void PortAudioReset(void){SDL_ClearAudioStream(sAudio);sStreamPriming=1;sQueuedAvg=-1;}
'''+queue.replace('Guest_min(', 'SDL_min(')+r'''
int main(void) {
    if(!SDL_Init(SDL_INIT_AUDIO))return 2;
    SDL_AudioSpec spec={SDL_AUDIO_F32,2,48000};
    GuestStream *g=Guest_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&spec,NULL,NULL);
    if(!g)return 3;
    int startsPaused=SDL_AudioDevicePaused(SDL_GetAudioStreamDevice(sAudio));
    Guest_PauseAudioStreamDevice(g);
    int pauseHonored=SDL_AudioDevicePaused(SDL_GetAudioStreamDevice(sAudio));
    SDL_PauseAudioStreamDevice(sAudio); /* deterministic queue observation, NOT quality evidence */
    float data[2048];for(int i=0;i<2048;i++)data[i]=0.5f;
    Guest_SetAudioStreamGain(g,0.25f);
    Guest_PutAudioStreamData(g,data,sizeof(data));
    int gainHonored=SDL_GetAudioStreamGain(sAudio)==0.25f;
    int rates[]={48000,63072,65536};
    printf("{\"audio_driver\":\"%s\",\"starts_paused\":%d,\"pause_honored\":%d,\"gain_honored\":%d,\"gain_sample\":%d,\"rates\":[",SDL_GetCurrentAudioDriver(),startsPaused,pauseHonored,gainHonored,lastSample);
    int mismatches=0;
    for(int i=0;i<3;i++){
        SDL_AudioSpec in={SDL_AUDIO_F32,2,rates[i]};
        Guest_SetAudioStreamFormat(g,&in,NULL);
        SDL_ClearAudioStream(sAudio);ringQueued=0;sPos=0;
        Guest_PutAudioStreamData(g,data,sizeof(data));
        int actual=sizeof(data);
        int reported=Guest_GetAudioStreamQueued(g);
        mismatches+=abs(actual-reported)>24;
        printf("%s{\"input_rate\":%d,\"actual_input_bytes\":%d,\"guest_reported_bytes\":%d}",i?",":"",rates[i],actual,reported);
    }
    printf("]");
#ifndef RING_BACKEND
    SDL_ClearAudioStream(sAudio);
    Guest_PauseAudioStreamDevice(g);
    sStream=g;sStreamRate=0;
    for(int frame=0;frame<90;frame++){
        Queue(1056,63072);
        SDL_Delay(frame%3==0?16:17);
    }
    printf(",\"steady_90frames_still_priming\":%d,\"steady_queued_bytes\":%d,\"steady_fallback_underruns\":%u",sStreamPriming,Guest_GetAudioStreamQueued(g),PortAudioContinuityUnderruns(&sAudioContinuity,sAudio));
#endif
    PortAudioMovieHold(1);PortAudioMovieHold(0);
    int movieKeepsPriming=SDL_AudioDevicePaused(SDL_GetAudioStreamDevice(sAudio));
    printf(",\"movie_keeps_guest_priming\":%d",movieKeepsPriming);
    printf("}\n");
    SDL_DestroyAudioStream(sAudio);SDL_Quit();
#ifdef RING_BACKEND
    return (startsPaused || pauseHonored || gainHonored || mismatches)?4:0;
#else
    return (!startsPaused || !pauseHonored || !gainHonored || mismatches || sStreamPriming || !movieKeepsPriming)?4:0;
#endif
}
'''
(OUT/'probe.c').write_text(c)
tc = next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/clang.exe'))
sdl = ROOT.parent/'toolchain/SDL3-3.4.18/x86_64-w64-mingw32'
cmd=[str(tc),'-O2',str(OUT/'probe.c'),'-I'+str(sdl/'include'),'-I'+str(ROOT/'port/include'),'-I'+str(ROOT/'port/ps5/shared'),'-L'+str(sdl/'lib'),'-lSDL3','-o',str(OUT/'probe.exe')]
driver=os.environ.get('KHCOM_TEST_AUDIO_DRIVER','dummy')
env=dict(os.environ, SDL_AUDIO_DRIVER=driver,PATH=str(sdl/'bin')+os.pathsep+os.environ['PATH'])
results={}
for backend,flags in [('windows-sdl',[]),('linux-sdl',[]),('android-sdl',[]),('ring',['-DRING_BACKEND','-DREKHCOM_PS4'])]:
    unit=c
    alt_path={'linux-sdl':'port/linux64/linux64_host.c','android-sdl':'port/android64/host/android64_host.c'}.get(backend)
    if alt_path:
        alt=(ROOT/alt_path).read_text()
        for signature in ['static int H_audio_open(', 'static uint32_t H_audio_queued(', 'static void H_audio_ratio(',
                          'static void GamePush(', 'static int H_thread(']:
            old=function(host,signature)
            assert unit.count(old)==1,(backend,signature)
            unit=unit.replace(old,function(alt,signature),1)
    (OUT/'probe.c').write_text(unit)
    subprocess.run(cmd+flags,check=True)
    p=subprocess.run([str(OUT/'probe.exe')],env=env,text=True,capture_output=True)
    print(backend,p.stdout,p.stderr)
    assert p.returncode==0,(backend,p.returncode)
    results[backend]=json.loads(p.stdout)
results['scope']='Production functions extracted without edits, real SDL device and actual PS4 GamePush with controlled RingPut sink; steady output is zero PCM (silent). Contracts only, not console device, audible quality or original-cause confirmation.'
results['source']=subprocess.check_output(['git','-C',str(REPO),'rev-parse','HEAD'],text=True).strip()
(OUT/('result-'+driver+'.json')).write_text(json.dumps(results,indent=2)+'\n')
