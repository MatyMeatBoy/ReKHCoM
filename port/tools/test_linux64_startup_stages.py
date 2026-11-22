"""Fault/nonreturn tests of the actual Linux64 startup block; no SDL/device runtime."""
from pathlib import Path
import argparse,json,os,subprocess
ROOT=Path(__file__).resolve().parents[2]

def main():
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--work',type=Path,required=True);args=ap.parse_args()
 out=args.work.resolve();out.mkdir(parents=True,exist_ok=True)
 source=(ROOT/'port/linux64/linux64_host.c').read_text(encoding='utf-8')
 if '/* Startup-only diagnostics' not in source:raise RuntimeError('missing flushed startup milestones in production host')
 helpers=source[source.index('/* Startup-only diagnostics'):source.index('static void StartupDeadlineExpired')]
 start=source.index('    StartupStage("sdl-info", "begin");')
 end=source.index('    Log("renderer: %s (64-bit host)", SDL_GetRendererName(sRenderer));',start)+len('    Log("renderer: %s (64-bit host)", SDL_GetRendererName(sRenderer));')
 block=source[start:end]
 startup=(ROOT/'port/include/port/sdl_startup.h').read_text(encoding='utf-8')
 startup=startup.replace('#include <SDL3/SDL.h>', '')
 freeblock=source[source.index('    StartupStage("image-scratch-release", "begin");'):source.index('    StartupStage("image-scratch-release", "done");')+len('    StartupStage("image-scratch-release", "done");')]
 pre=r'''
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#define SDL_INIT_VIDEO 1
#define SDL_INIT_AUDIO 2
#define SDL_INIT_GAMEPAD 4
#define SDL_WINDOW_RESIZABLE 1
#define SDL_WINDOW_HIGH_PIXEL_DENSITY 2
#define SDL_WINDOW_HIDDEN 4
#define SDL_VERSION 3004018
typedef unsigned SDL_InitFlags;
#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif
static int ProbeClock(int c,struct timespec*t){(void)c;t->tv_sec=123;t->tv_nsec=456000;return 0;}
#define clock_gettime ProbeClock
static long getpid(void){return 1234;}
static void *sWindow,*sRenderer;
static const char *failStage,*stopStage,*error="original injected error";
static void Log(const char*fmt,...){va_list ap;va_start(ap,fmt);vfprintf(stderr,fmt,ap);fputc('\n',stderr);va_end(ap);error="logging clobbered error";}
static int operation(const char*n){if(stopStage&&!strcmp(stopStage,n))_Exit(77);return !(failStage&&!strcmp(failStage,n));}
static void ProbeFree(void*p){(void)p;operation("image-scratch-release");}
#define free ProbeFree
static int SDL_GetVersion(void){return 3004018;}
static const char*SDL_GetCurrentVideoDriver(void){return "controlled-video";}
static const char*SDL_GetCurrentAudioDriver(void){return "controlled-audio";}
static void StartupLibraries(void){}
static void KhLoadGamepadMappings(void){}
static const char*SDL_GetError(void){return error;}
static bool SDL_Init(SDL_InitFlags flags){if(flags!=5)_Exit(99);error="original injected error";return operation("sdl-init");}
static bool SDL_InitSubSystem(SDL_InitFlags flags){if(flags!=2)_Exit(95);error="original injected error";return operation("sdl-audio-init");}
static bool SDL_ClearError(void){error="";return true;}
typedef struct { int unused; } PortStartupDeadline;
static void StartupDeadlineExpired(void){}
static bool PortStartupDeadlineBegin(PortStartupDeadline*d,unsigned ms,void(*f)(void)){(void)d;(void)f;if(ms!=30000)_Exit(97);error="original injected error";return operation("window-deadline");}
static void PortStartupDeadlineCancel(PortStartupDeadline*d){(void)d;}
static void*SDL_CreateWindow(const char*n,int w,int h,int f){(void)n;(void)w;(void)h;if(f!=7)_Exit(96);error="original injected error";return operation("window")?(void*)1:NULL;}
static bool SDL_ShowWindow(void*w){(void)w;error="original injected error";return operation("window-show");}
static void*SDL_CreateRenderer(void*w,const char*n){(void)w;(void)n;error="original injected error";return operation("renderer")?(void*)2:NULL;}
static bool SDL_SetRenderVSync(void*r,int v){(void)r;if(v!=(getenv("KHCOM_FAST")?0:1))_Exit(98);error="original injected error";return operation("vsync");}
static const char*SDL_GetRendererName(void*r){(void)r;return "controlled-renderer";}
'''
 code=pre+startup+helpers+r'''
int main(int argc,char**argv){setvbuf(stderr,NULL,_IOFBF,8192);if(argc>1)failStage=argv[1];if(argc>2)stopStage=argv[2];
 void*tmp=(void*)3;
FREEBLOCK
BLOCK
 return 0;
}
'''.replace('FREEBLOCK',freeblock).replace('BLOCK',block)
 (out/'probe.c').write_text(code,encoding='utf-8')
 cc=next(p/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe' for p in ROOT.parents
         if (p/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe').is_file())
 subprocess.run([str(cc),'-O2',str(out/'probe.c'),'-o',str(out/'probe.exe')],check=True,capture_output=True)
 rows=[]
 for label,argv,expected in [('success',[],0),('uncapped',[],0),('init-false',['sdl-init'],4),('audio-init-false',['sdl-audio-init'],0),('deadline-false',['window-deadline'],4),('window-null',['window'],4),('show-false',['window-show'],4),('renderer-null',['renderer'],4),('vsync-false',['vsync'],0),*[(s+'-no-return',['-',s],77) for s in ('image-scratch-release','sdl-init','window','window-show','renderer','vsync')]]:
  env={k:v for k,v in os.environ.items() if k!='KHCOM_FAST'}
  if label=='uncapped':env['KHCOM_FAST']='1'
  p=subprocess.run([str(out/'probe.exe'),*argv],env=env,capture_output=True,text=True,timeout=5);log=p.stderr
  if p.returncode!=expected:raise RuntimeError((label,p.returncode,log))
  if label.endswith('no-return'):
   stage=argv[1]
   if f'stage={stage} status=begin' not in log or f'stage={stage} status=done' in log:raise RuntimeError((label,'lost/untrue milestone',log))
  elif expected==4 or label=='vsync-false':
   if 'original injected error' not in log:raise RuntimeError((label,'error clobbered',log))
  elif label=='audio-init-false':
   if ('SDL audio unavailable, continuing without sound: original injected error' not in log
       or 'stage=renderer status=done' not in log):raise RuntimeError((label,'optional failure aborted startup or lost its cause',log))
  elif 'controlled-video' not in log or 'linked=3004018' not in log:raise RuntimeError(('missing linked SDL/driver info',log))
  downstream={'init-false':'window','deadline-false':'window','window-null':'window-show','show-false':'renderer','renderer-null':'vsync'}.get(label)
  if downstream and f'stage={downstream} status=begin' in log:raise RuntimeError((label,'called downstream after failure',log))
  rows.append({'case':label,'exit':p.returncode,'log':log.splitlines()})
 (out/'RESULTS.json').write_text(json.dumps({'result':'PASS','cases':rows,'scope':'Actual source helper/block; buffered stderr plus _Exit proves begin milestones flushed. No SDL/device/game/VM execution.'},indent=2)+'\n')
 print(f'PASS{len(rows)} production startup cases, required failures, optional audio and flushed nonreturns')

if __name__=='__main__':main()
