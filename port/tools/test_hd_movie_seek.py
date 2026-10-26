"""Exercise the Windows player's production PCM queue after a keyframe seek.

No ROM, imported assets, window or audio device. MF's observed preroll timestamps
are supplied at its ReadSample boundary; the actual queue loop selects the PCM.
"""
import shutil
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / 'port/src/port_hdmovie.c').read_text(encoding='utf-8')
start = source.index('        /* keep about half a second of audio queued: it is the clock */')
end = source.index('        /* the presentation time:', start)
production = source[start:end]
preamble = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
typedef long long LONGLONG;
typedef unsigned long DWORD;
typedef unsigned char BYTE;
typedef int HRESULT;
typedef struct { int index; } IMFSample;
typedef IMFSample IMFMediaBuffer;
#define MF_SOURCE_READER_FIRST_AUDIO_STREAM 0
#define MF_SOURCE_READERF_ENDOFSTREAM 1
#define FAILED(hr) ((hr)<0)
#define SUCCEEDED(hr) ((hr)>=0)
static int sStartAt, packet, queued, firstValue, calls;
static LONGLONG timestamps[4];
static short pcm[4][2048];
static DWORD lengths[4];
static IMFSample samples[4]={{0},{1},{2},{3}};
static int SDL_GetAudioStreamQueued(void* audio) {(void)audio;return queued;}
static void SDL_PutAudioStreamData(void* audio,const void* data,int size) {
 (void)audio;if(!queued)firstValue=*(const short*)data;queued+=size;calls++;
}
static HRESULT IMFSourceReader_ReadSample(void* r,DWORD stream,int a,void* b,DWORD* flags,LONGLONG* ts,IMFSample** sample) {
 (void)r;(void)stream;(void)a;(void)b;
 if(packet==4){*flags=MF_SOURCE_READERF_ENDOFSTREAM;return 0;}
 *ts=timestamps[packet];*sample=&samples[packet++];return 0;
}
static HRESULT IMFSample_ConvertToContiguousBuffer(IMFSample*s,IMFMediaBuffer**b){*b=s;return 0;}
static HRESULT IMFMediaBuffer_Lock(IMFMediaBuffer*b,BYTE**data,void*x,DWORD*len){(void)x;*data=(BYTE*)pcm[b->index];*len=lengths[b->index];return 0;}
static void IMFMediaBuffer_Unlock(IMFMediaBuffer*b){(void)b;}
static void IMFMediaBuffer_Release(IMFMediaBuffer*b){(void)b;}
static void IMFSample_Release(IMFSample*s){(void)s;}
static LONGLONG fill(int seconds,LONGLONG first) {
 unsigned rate=48000,channels=2;void* audio=(void*)1;void* reader=0;int audioDone=0;HRESULT hr;
 LONGLONG audioPushed=(LONGLONG)seconds*10000000;
 sStartAt=seconds;packet=queued=calls=0;firstValue=-1;
 for(int i=0;i<4;i++){
  timestamps[i]=first+(LONGLONG)i*1024*10000000/rate;lengths[i]=4096;
  for(int j=0;j<1024;j++)for(int c=0;c<2;c++)pcm[i][j*2+c]=(short)(i*1024+j);
 }
'''
cases = r'''
 return audioPushed-(LONGLONG)queued*10000000/(rate*channels*2);
}
int main(void){
 /* Actual MF seek(222s) with 10s GOP: first PCM starts at 219.968s.
    All four packets are preroll and must never reach SDL. */
 fill(222,2199680000LL);
 if(queued){fprintf(stderr,"FAIL: seek 222 queued %d preroll bytes\n",queued);return 1;}
 fill(170,1699680000LL);
 if(firstValue!=1536){fprintf(stderr,"FAIL: crossing packet starts at PCM frame %d, expected 1536\n",firstValue);return 2;}
 LONGLONG clock=fill(222,2220100000LL);
 if(llabs(clock-2220100000LL)>2){fprintf(stderr,"FAIL: clock %lld ignores actual PCM timestamp\n",clock);return 3;}
 clock=fill(0,0);
 if(firstValue!=0 || llabs(clock)>2){fprintf(stderr,"FAIL: movie from zero changed\n");return 4;}
 puts("PASS: preroll discarded, crossing PCM trimmed, timestamps aligned, zero offset preserved");return 0;
}
'''
compiler = shutil.which('cc')
if not compiler:
    compiler = next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
with tempfile.TemporaryDirectory(prefix='rekhcom-seek-') as directory:
    directory=Path(directory); src=directory/'seek.c'; exe=directory/'seek.exe'
    src.write_text(preamble+production+cases,encoding='utf-8')
    subprocess.run([str(compiler),'-std=c11',str(src),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
