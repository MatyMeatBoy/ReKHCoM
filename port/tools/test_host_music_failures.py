"""Production host music producer/consumer with injected Vorbis faults; no device."""
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
source=(ROOT/'port/ps5/shared/hd_music_host.h').read_text(encoding='utf-8')
production=source[source.index('enum { HD_MUSIC_OPEN,'):source.rindex('#endif')]
preamble=r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define HD_MUSIC_LOG(...) ((void)0)
typedef int stb_vorbis;
static int cursor,decodeCalls,seekCalls,closeCalls,failDecode,failSeek,total=2000;
static int fake;
static int H_fopen(const char*p,int m){return strstr(p,".ogg")?1:-1;}
static int H_fseek(int h,int o,int w){return 0;}
static int H_ftell(int h){return 4;}
static int H_fread(int h,void*p,uint32_t n){memset(p,0,n);return (int)n;}
static void H_fclose(int h){}
static stb_vorbis* stb_vorbis_open_memory(const void*p,int n,int*e,void*a){return &fake;}
struct info {unsigned sample_rate;};
static struct info stb_vorbis_get_info(stb_vorbis*v){return (struct info){48000};}
static unsigned stb_vorbis_stream_length_in_samples(stb_vorbis*v){return total;}
static void stb_vorbis_close(stb_vorbis*v){closeCalls++;}
static int stb_vorbis_seek(stb_vorbis*v,unsigned at){seekCalls++;if(failSeek)return 0;cursor=at;return 1;}
static int stb_vorbis_get_samples_float_interleaved(stb_vorbis*v,int channels,float*b,int n){
 decodeCalls++;if(failDecode)return 0;int frames=n/2;
 if(frames>total-cursor)frames=total-cursor;
 for(int i=0;i<frames;i++){b[i*2]=b[i*2+1]=(float)cursor++;}
 return frames;
}
'''
cases=r'''
static int pushed;
static void push(const int16_t*f,uint32_t n){pushed+=n;}
static void reset(void){
 memset(&sHd,0,sizeof(sHd));cursor=0;decodeCalls=seekCalls=closeCalls=failDecode=failSeek=0;
 sHd.vorbis=&fake;sHd.rate=48000;sHd.loopStart=100;sHd.loopEnd=200;sHd.playing=1;
}
int main(void){
 float out[2];reset();cursor=200;sHd.pos=200;
 for(int i=0;i<1000;i++){HdNext(out);if(out[0]!=100+i%100){fprintf(stderr,"loop sample %d=%g\n",i,out[0]);return 1;}}
 if(sHd.pos>200||seekCalls!=10)return 2;
 reset();failDecode=1;
 for(int i=0;i<96000;i++)HdNext(out);
 if(decodeCalls>2||seekCalls>1||closeCalls||out[0]!=0||H_hd_music(HD_MUSIC_PLAY,"",1000)){
 fprintf(stderr,"no-progress decode=%d seek=%d close=%d\n",decodeCalls,seekCalls,closeCalls);return 3;}
 H_hd_music(HD_MUSIC_CLOSE,"",0);if(closeCalls!=1)return 4;
 reset();cursor=200;sHd.pos=200;failSeek=1;HdNext(out);
 if(decodeCalls||seekCalls!=1||H_hd_music(HD_MUSIC_PLAY,"",1000)||closeCalls)return 5;
 reset();failDecode=1;int16_t frames[4096]={0};pushed=0;
 HdMusicPush(frames,2048,48000,push);
 if(pushed!=2048||decodeCalls>2||seekCalls>1||closeCalls||H_hd_music(HD_MUSIC_PLAY,"",1000))return 6;
 reset();failDecode=1;
 if(H_hd_music(HD_MUSIC_OPEN,"test.ogg",0)||closeCalls!=2||sHd.vorbis||sHd.file)return 7;
 reset();
 if(!H_hd_music(HD_MUSIC_OPEN,"test.ogg",0)||!H_hd_music(HD_MUSIC_PLAY,"",1000))return 8;
 H_hd_music(HD_MUSIC_CLOSE,"",0);if(closeCalls!=2)return 9;
 return 0;
}
'''
compiler=ROOT.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe'
with tempfile.TemporaryDirectory(prefix='rekhcom-host-music-') as td:
 src=Path(td)/'test.c';exe=Path(td)/'test.exe'
 src.write_text(preamble+production+cases,encoding='utf-8')
 subprocess.run([str(compiler),str(src),'-o',str(exe)],check=True,capture_output=True)
 subprocess.run([str(exe)],check=True,capture_output=True)

# Compile the complete shared header against its real Vorbis implementation too.
with tempfile.TemporaryDirectory(prefix='rekhcom-host-header-') as td:
 src=Path(td)/'syntax.c'
 header=(ROOT/'port/ps5/shared/hd_music_host.h').as_posix()
 src.write_text('#include <stdint.h>\nstatic int H_fopen(const char*,int);\nstatic int H_fseek(int,int,int);\nstatic int H_ftell(int);\nstatic int H_fread(int,void*,uint32_t);\nstatic void H_fclose(int);\n'+f'#include "{header}"\n',encoding='utf-8')
 subprocess.run([str(compiler),'-fsyntax-only',str(src)],check=True,capture_output=True)
