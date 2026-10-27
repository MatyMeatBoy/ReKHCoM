"""Fault-inject decoder/device failures into the real feed + per-frame controller."""
from pathlib import Path
import subprocess
import tempfile

R = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    end = source.index('\n}', start) + 2
    return source[start:end]


def main():
    source = (R/'port/src/port_hdmusic.c').read_text(encoding='utf-8')
    feed = function(source, 'static int Feed(float gain)' if 'static int Feed(float gain)' in source else 'static void Feed(float gain)')
    frame = function(source, 'void PortHdMusicFrame(void)')
    code = r'''
#include <stdio.h>
#include <stdlib.h>
typedef struct { int status, fadeOI, fadeOV; } MusicPlayerInfo;
static MusicPlayerInfo gMPlayInfo_BGM={1,0,0};
static struct { int hdMusic,masterVolume; } gPortConfig={1,100};
static struct { float musicGain; } gPortMix={0.75f};
static int sSong=1,sMissing=-1,sRate=48000,sLevel,sHold=1,sPosition,sLoopStart,sLoopEnd=48000;
static int fakeStream, *sStream=&fakeStream, fakeVorbis, *sVorbis=&fakeVorbis;
static const char* kTracks[]={NULL,"test"};
static int mode, queued, closed, calls;
#define HD_MUSIC_LEVEL 0.33f
#define MUSICPLAYER_STATUS_TRACK 1
#define MUSICPLAYER_STATUS_PAUSE 2
#define FADE_VOL_SHIFT 0
#define FADE_VOL_MAX 256
static void PortLog(const char* fmt, ...) { (void)fmt; }
static int CurrentSong(void) { return 1; }
static void Close(void) { closed++; sSong=-1; }
static int Open(int song) { return 1; }
static void HdLog(int song,int state) {}
static void Pause(void) {}
static void Resume(void) {}
static int SDL_SetAudioStreamGain(int* s,float gain) {return 1;}
static int SDL_GetAudioStreamQueued(int* s) {return queued;}
static const char* SDL_GetError(void) {return "injected device failure";}
static int SDL_PutAudioStreamData(int* s,const void* p,int bytes) {
    if(mode==1)return 0; queued+=bytes; return 1;
}
static int stb_vorbis_seek(int* v,unsigned pos) {return 1;}
static int stb_vorbis_get_samples_float_interleaved(int* v,int channels,float* out,int count) {
    calls++; if(mode==0)return 0;
    int n=count/2; for(int i=0;i<n*2;i++)out[i]=0; return n;
}
'''
    code += feed + '\n' + frame + r'''
int main(int argc,char** argv) {
    mode=atoi(argv[1]);
    if(mode==3)sLoopEnd=1024;
    if(mode==4)sLoopEnd=0;
    PortHdMusicFrame();
    if(mode==0 || mode==1 || mode==4) {
        if(!closed || sMissing!=1 || gPortMix.musicGain!=0.75f) return 1;
        if(calls>32)return 2;
    } else {
        if(closed || gPortMix.musicGain!=0 || queued<48000/5*8) return 3;
    }
    return 0;
}
'''
    cc = next((R.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
    with tempfile.TemporaryDirectory(prefix='rekhcom-audio-fail-') as name:
        folder=Path(name); src=folder/'test.c'; exe=folder/'test.exe'
        src.write_text(code,encoding='utf-8')
        subprocess.run([str(cc),str(src),'-o',str(exe)],check=True)
        for mode, label in enumerate(('decoder stalls','device rejects data','normal track','short valid loop','empty loop')):
            try:
                result = subprocess.run([str(exe),str(mode)],timeout=2)
            except subprocess.TimeoutExpired:
                raise AssertionError(f'{label}: audio feed never returns') from None
            assert result.returncode==0, (label,result.returncode)
            print(f'PASS: {label}')


if __name__=='__main__':
    main()
