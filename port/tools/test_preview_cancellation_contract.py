"""Deterministic preview lifecycle contracts from production C, with no SDL link.

Worker completion is scheduled explicitly. There are no real threads, windows,
sleeps, clocks, GPU calls or performance measurements. Runtime data stays under
the caller's --work in a fresh private subdirectory. This is not device QA.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile


def function(source, signature):
    start = source.index(signature + ' {')
    end = source.index('{', start) + 1
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


PRELUDE = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "port/port_hd_poster.h"
/* Keep deliberate negative controls silent: no Windows CRT assertion dialog. */
#undef assert
#define assert(condition) do { if (!(condition)) { fprintf(stderr,"contract assertion: %s\n",#condition); exit(97); } } while (0)
typedef uint64_t Uint64;
typedef struct { int value; } SDL_AtomicInt;
typedef struct { int unused; } SDL_Texture;
typedef struct { int unused; } SDL_Renderer;
typedef struct { int unused; } SDL_Surface;
typedef struct { Uint64 modify_time; size_t size; } SDL_PathInfo;
typedef struct SDL_Thread { int (*entry)(void*); void* data; int exists, finished; } SDL_Thread;
static SDL_Thread queued;
static Uint64 ticks;
static int generation, decodeFail, threadFail, missingMedia, pendingWaitAllowed, joins, pendingJoins;
static int starts, decoded, destroys, active;
static size_t allocations,allocationCalls;
static void Complete(void);
static void* ProbeMalloc(size_t size) { allocationCalls++;void* p=malloc(size); if(p)allocations++; return p; }
static void ProbeFree(void* p) { if(p){assert(allocations);allocations--;free(p);} }
#define SDL_malloc ProbeMalloc
#define SDL_free ProbeFree
#define SDL_memcpy memcpy
#define SDL_strcmp strcmp
#define SDL_snprintf snprintf
#define SDL_PIXELFORMAT_ARGB8888 0
static size_t SDL_strlcpy(char* dst,const char* src,size_t size){size_t n=strlen(src);if(size){size_t take=n<size-1?n:size-1;memcpy(dst,src,take);dst[take]=0;}return n;}
static Uint64 SDL_GetTicks(void){return ticks;}
static const char* SDL_getenv(const char* name){(void)name;return NULL;}
static Uint64 SDL_GetPerformanceCounter(void){assert(!"observer-off clock");return 0;}
static Uint64 SDL_GetPerformanceFrequency(void){assert(!"observer-off frequency");return 1;}
static const char* PortConfigPath(const char* name){(void)name;assert(!"observer-off path/file IO");return "";}
static int SDL_GetAtomicInt(SDL_AtomicInt* a){return a->value;}
static void SDL_SetAtomicInt(SDL_AtomicInt* a,int value){a->value=value;}
static SDL_Thread* SDL_CreateThread(int(*entry)(void*),const char* name,void* data){
 (void)name;if(threadFail)return NULL;assert(!queued.exists);queued=(SDL_Thread){entry,data,1,0};starts++;return &queued;
}
static void SDL_WaitThread(SDL_Thread* thread,int* status){
 assert(thread==&queued && queued.exists);joins++;
 if(!thread->finished){assert(pendingWaitAllowed);pendingJoins++;Complete();}
 queued.exists=0;if(status)*status=0;
}
static void SDL_DestroyTexture(SDL_Texture* texture){if(texture)destroys++;}
static SDL_Surface* SDL_CreateSurfaceFrom(int w,int h,int format,void* pixels,int pitch){(void)w;(void)h;(void)format;(void)pixels;(void)pitch;return NULL;}
static bool SDL_SavePNG(SDL_Surface* surface,const char* path){(void)surface;(void)path;assert(!"No file IO in this fixture");return false;}
static void SDL_DestroySurface(SDL_Surface* surface){assert(!surface);}
static bool SDL_GetPathInfo(const char* path,SDL_PathInfo* info){(void)path;(void)info;return false;}
static void* SDL_LoadFile(const char* path,size_t* size){(void)path;(void)size;assert(!"No file IO in this fixture");return NULL;}
static void PortLog(const char* format,...){(void)format;}
static uint32_t GbaColor(int event){return 0xff000055u|((uint32_t)event<<8);}
static uint32_t HdColor(int event,int gen){return 0xff0000ccu|((uint32_t)event<<8)|((uint32_t)gen<<16);}
const char* PortHdGalleryMovieName(int event){return event==10?"event10":"event11";}
int PortHdGalleryMovieOffset(int event){(void)event;return 0;}
int PortHdScenePreviewPath(const char* movie,char* out,size_t size){if(missingMedia){if(size)out[0]=0;return 0;}snprintf(out,size,"g%d/%s.mp4",generation,movie);return 1;}
int PortHdMovieResolvePath(const char* movie,char* out,size_t size){return PortHdScenePreviewPath(movie,out,size);}
int PortHdScenePreviewFile(const char* path,uint32_t* out,int w,int h,int second){
 int gen=-1,event=-1;(void)second;assert(!active);active=1;decoded++;
 assert(sscanf(path,"g%d/event%d.mp4",&gen,&event)==2);
 for(int i=0;i<w*h;i++)out[i]=HdColor(event,gen);
 active=0;return !decodeFail;
}
int PortScenePosterPreview(int event,uint32_t* out,int* w,int* h){*w=*h=2;for(int i=0;i<4;i++)out[i]=GbaColor(event);return 1;}
int PortScenePosterDecode(const void* data,size_t size,uint32_t* out,int* w,int* h){(void)data;(void)size;(void)out;(void)w;(void)h;return 0;}
int PortSceneBackgroundPreview(int event,uint32_t* out,int w,int h){(void)event;(void)out;(void)w;(void)h;assert(!"Poster fixture is available");return 0;}
static void* sReplayMemory;
static int sReplay,sReplayFallback;
enum { GALLERY_IDLE };
static int sOpenSound,sCloseSound,sOpen,sClosing,sReturn;
'''

SCENARIOS = r'''
static void Complete(void){assert(queued.exists&&!queued.finished);queued.entry(queued.data);queued.finished=1;}
static void Reset(void){
 PortSceneGalleryPreviewCancel();if(queued.exists&&!queued.finished)Complete();PortSceneGalleryPreviewPoll();
 PortSceneGalleryPreviewCacheClear();assert(!queued.exists&&!allocations);
 ticks=0;generation=decodeFail=threadFail=missingMedia=pendingWaitAllowed=0;
}
static void Start(int event){
 assert(PortSceneGalleryPreview(event,1));assert(sPreviewFallback&&sPreview[0]==GbaColor(event));
 ticks+=75;assert(PortSceneGalleryPreview(event,1));assert(queued.exists&&!queued.finished);
}
static void CheckGba(int event){assert(sPreviewReady&&sPreview[0]==GbaColor(event)&&sPreviewW==2&&sPreviewH==2);}
int main(void){
 /* ForceClose must not join a pending decoder; late completion cannot reopen it. */
 Reset();Start(10);int before=joins;SDL_Texture texture;sPreviewTexture=&texture;
 sOpen=sOpenSound=sCloseSound=sClosing=sReturn=1;PortUiForceClose();
 assert(joins==before&&queued.exists&&!queued.finished&&!sOpen&&!sClosing&&!sReturn);
 assert(!sPreviewReady&&!sPreviewTexture&&sPreviewEvent==-1);
 Complete();PortSceneGalleryPreviewPoll();assert(!sPreviewReady&&!queued.exists&&!sPreviewJob.pixels);
 puts("PASS force-close does not join a pending decoder; late result cannot reopen selection");

 /* Reopening the same event must still invalidate its previous session's job. */
 Reset();Start(10);PortSceneGalleryPreviewCancel();assert(PortSceneGalleryPreview(10,1));
 Complete();PortSceneGalleryPreviewPoll();CheckGba(10);assert(sPreviewFallback);
 ticks+=75;assert(PortSceneGalleryPreview(10,1));Complete();PortSceneGalleryPreviewPoll();
 assert(sPreviewReady&&!sPreviewFallback&&sPreview[0]==HdColor(10,0));
 puts("PASS reopened same-event gallery rejects previous-session result");

 /* HD -> GBA while pending: result cannot overwrite the cartridge provider. */
 Reset();Start(10);assert(PortSceneGalleryPreview(11,0));CheckGba(11);
 Complete();PortSceneGalleryPreviewPoll();CheckGba(11);assert(!sPreviewHd);
 puts("PASS pending HD result cannot replace a later GBA selection");

 /* Same event, HD -> GBA -> HD without closing: old HD still has an old serial. */
 Reset();Start(10);assert(PortSceneGalleryPreview(10,0));assert(PortSceneGalleryPreview(10,1));
 Complete();PortSceneGalleryPreviewPoll();CheckGba(10);assert(sPreviewFallback);
 ticks+=75;assert(PortSceneGalleryPreview(10,1));Complete();PortSceneGalleryPreviewPoll();
 assert(sPreview[0]==HdColor(10,0)&&!sPreviewFallback);
 puts("PASS same-event HD/GBA/HD cycle rejects the first HD request");

 /* Rapid event change starts no second worker and later serves the final choice. */
 Reset();Start(10);int count=starts;assert(PortSceneGalleryPreview(11,1));ticks+=75;
 assert(PortSceneGalleryPreview(11,1));assert(starts==count);Complete();PortSceneGalleryPreviewPoll();CheckGba(11);
 assert(PortSceneGalleryPreview(11,1));assert(starts==count+1);Complete();PortSceneGalleryPreviewPoll();
 assert(sPreview[0]==HdColor(11,0)&&!sPreviewJob.thread&&!sPreviewJob.pixels);
 puts("PASS rapid HD selection keeps one worker and presents only the final result");

 /* Completion immediately before a generation-changing request tests Poll ordering. */
 Reset();Start(10);Complete();generation=1;assert(PortSceneGalleryPreview(10,1));
 CheckGba(10);assert(sPreviewFallback&&strstr(sPreviewMediaPath,"g1/"));
 ticks+=75;assert(PortSceneGalleryPreview(10,1));Complete();PortSceneGalleryPreviewPoll();
 assert(sPreview[0]==HdColor(10,1)&&!sPreviewFallback);
 puts("PASS completed old-generation result is not returned by a new-generation request");

 /* Corrupt/cancelled decode retains fallback and does not spin retry attempts. */
 Reset();decodeFail=1;Start(10);count=starts;Complete();PortSceneGalleryPreviewPoll();CheckGba(10);
 ticks+=1000;assert(PortSceneGalleryPreview(10,1));assert(starts==count&&sPreviewFallback);
 Reset();threadFail=1;assert(PortSceneGalleryPreview(10,1));ticks+=75;assert(PortSceneGalleryPreview(10,1));
 assert(!sPreviewJob.thread&&!sPreviewJob.pixels);CheckGba(10);
 size_t attempts=allocationCalls;ticks+=1000;assert(PortSceneGalleryPreview(10,1));assert(allocationCalls==attempts);
 puts("PASS decode/thread failure keeps fallback and releases job ownership");

 /* Debounce uses elapsed selection time; no worker is started at 74ms. */
 Reset();assert(PortSceneGalleryPreview(10,1));count=starts;
 ticks=74;assert(PortSceneGalleryPreview(10,1));assert(starts==count&&!queued.exists);
 ticks=75;assert(PortSceneGalleryPreview(10,1));assert(starts==count+1&&queued.exists);
 Complete();PortSceneGalleryPreviewPoll();assert(!sPreviewFallback);
 puts("PASS immediate fallback precedes the exact 75ms worker debounce boundary");

 /* Missing imported media never starts a decoder and still owns a valid GBA still. */
 Reset();missingMedia=1;assert(PortSceneGalleryPreview(10,1));CheckGba(10);count=starts;
 ticks=75;assert(PortSceneGalleryPreview(10,1));assert(starts==count&&!queued.exists&&sPreviewFallback);
 ticks=1000;assert(PortSceneGalleryPreview(10,1));assert(starts==count&&sPreviewFallback);
 puts("PASS missing HD media preserves fallback without queued work or repeated retries");

 /* Process shutdown deliberately joins; this is distinct from menu closure. */
 Reset();Start(10);pendingWaitAllowed=1;before=pendingJoins;PortSceneGalleryFinalShutdown();
 assert(pendingJoins==before+1&&!queued.exists&&!sPreviewJob.thread&&!sPreviewJob.pixels&&!allocations);
 assert(!sPreviewReady&&!sGbaPreviewBytes);
 puts("PASS final shutdown joins pending work deliberately and releases all owned buffers");
 Reset();assert(destroys==1&&!active);return 0;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--repo', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--cc', type=Path, required=True)
    parser.add_argument('--negative-controls', action='store_true', help='Require private deliberately broken variants to fail their corresponding assertion')
    args = parser.parse_args()
    repo = args.repo.resolve()
    source = (repo/'port/src/port_scene_gallery.c').read_text(encoding='utf-8')
    ui = (repo/'port/src/port_ui.c').read_text(encoding='utf-8')
    begin = source.index('#define PREVIEW_W')
    end = source.index('/* Output-resolution poster rendering', begin)
    code = PRELUDE + source[begin:end]
    code += function(source, 'void PortSceneGalleryShutdown(void)')
    code += function(source, 'void PortSceneGalleryFinalShutdown(void)')
    code += function(ui, 'void PortUiForceClose(void)') + SCENARIOS
    args.work.mkdir(parents=True, exist_ok=True)
    output = Path(tempfile.mkdtemp(prefix='contract-', dir=args.work))
    unit = output/'probe.c'; executable = output/'probe.exe'
    unit.write_text(code, encoding='utf-8')
    subprocess.run([str(args.cc), '-std=c11', '-O0', '-I'+str(repo/'port/include'),
                    str(unit), '-o', str(executable)], check=True)
    run = subprocess.run([str(executable)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                         timeout=10, creationflags=0x08004000)
    (output/'run.log').write_bytes(run.stdout)
    print(run.stdout.decode(errors='replace'), end='')
    assert run.returncode == 0, f'contract fixture exit {run.returncode}'
    controls = []
    if args.negative_controls:
        serial_guard = 'job->serial == sPreviewSerial && '
        close_start = 'void PortSceneGalleryPreviewCancel(void) {\n'
        assert code.count(serial_guard) == code.count(close_start) == 1, 'Production anchors changed'
        variants = [
            ('stale-serial', code.replace(serial_guard, '', 1), b'GbaColor(event)'),
            ('cold-join', code.replace(close_start, close_start+'    if (sPreviewJob.thread) SDL_WaitThread(sPreviewJob.thread, NULL);\n', 1), b'pendingWaitAllowed'),
        ]
        for name, mutant, expected in variants:
            unit = output/(name+'.c'); binary = output/(name+'.exe')
            unit.write_text(mutant, encoding='utf-8')
            subprocess.run([str(args.cc), '-std=c11', '-O0', '-I'+str(repo/'port/include'),
                            str(unit), '-o', str(binary)], check=True)
            broken = subprocess.run([str(binary)], stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                    timeout=10, creationflags=0x08004000)
            (output/(name+'.log')).write_bytes(broken.stdout)
            assert broken.returncode != 0 and expected in broken.stdout, broken.stdout.decode(errors='replace')
            controls.append(dict(name=name, expected_failure=True, exit=broken.returncode))
            print('PASS negative control:', name, '(deliberately broken private test copy rejected)')
    manifest = dict(source_commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=repo,text=True).strip(),
                    gallery_sha256=hashlib.sha256(source.encode()).hexdigest(),
                    ui_sha256=hashlib.sha256(ui.encode()).hexdigest(),
                    executable_sha256=hashlib.sha256(executable.read_bytes()).hexdigest(),
                    source_text_encoding='UTF-8 with normalized LF',
                    negative_controls=controls,
                    scope='deterministic production C lifecycle model; no real threads/SDL/GPU/performance evidence')
    (output/'manifest.json').write_text(json.dumps(manifest, indent=2))
    print('Evidence:', output)


if __name__ == '__main__':
    main()
