"""Bounded preview observer: disabled clocks/IO, immutable context and overflow."""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--cc', type=Path, required=True)
    p.add_argument('--work', type=Path, required=True)
    a = p.parse_args()
    a.work.mkdir(parents=True, exist_ok=True)
    out = Path(tempfile.mkdtemp(prefix='trace-', dir=a.work)).resolve()
    repo = Path(__file__).resolve().parents[2]
    code = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(c) do{if(!(c)){fprintf(stderr,"failed: %s\n",#c);exit(97);}}while(0)
static int enable,clocks,opens;
static int sPreviewSerial=3,sPreviewEvent=10,sPreviewHd=1,sPreviewFallback=1;
static char sPreviewMediaPath[1024]="immutable/en/g-old/scene.mp4";
static const char* SDL_getenv(const char*n){(void)n;return enable?"1":NULL;}
static uint64_t SDL_GetPerformanceCounter(void){clocks++;return (uint64_t)clocks*100;}
static uint64_t SDL_GetPerformanceFrequency(void){return 1000;}
static unsigned PortFrameCount(void){return 42;}
static FILE* Open(const char*p,const char*m){opens++;return fopen(p,m);}
#define fopen Open
'''
    code += 'static const char* PortConfigPath(const char*n){(void)n;return '+json.dumps((out/'preview_trace.csv').as_posix())+';}\n'
    code += '#include "port/preview_trace.h"\n'
    code += r'''
int main(void){
 PreviewTrace("request");PreviewTraceDraw(1);PortSceneGalleryPreviewPresented(1);PreviewTraceFlush();
 CHECK(clocks==0&&opens==0&&sPreviewTraceCount==0&&!sPreviewTracePending);
 enable=1;sPreviewTraceEnabled=-1;PreviewTrace("request");PreviewTraceDraw(0);
 CHECK(!sPreviewTracePending);PreviewTraceDraw(1);CHECK(sPreviewTracePending);
 PortSceneGalleryPreviewPresented(1);CHECK(!sPreviewTracePending);
 int n=clocks;PreviewTraceDraw(1);PortSceneGalleryPreviewPresented(1);CHECK(clocks==n);
 unsigned oldGeneration=sPreviewTrace[0].generation;
 sPreviewFallback=0;PreviewTrace("cpu-hd-ready-polled");PreviewTraceDraw(1);PortSceneGalleryPreviewPresented(1);
 CHECK(sPreviewTrace[sPreviewTraceCount-1].fallback==0);
 sPreviewSerial++;sPreviewMediaPath[10]='j';PreviewTrace("request");
 CHECK(sPreviewTrace[sPreviewTraceCount-1].generation!=oldGeneration&&sPreviewTrace[0].serial==3);
 while(sPreviewTraceCount<PREVIEW_TRACE_CAPACITY)PreviewTrace("request");
 n=clocks;for(int i=0;i<4;i++)PreviewTrace("overflow");CHECK(clocks==n&&sPreviewTraceDropped==4&&opens==0);
 PreviewTraceFlush();CHECK(opens==1);puts("PASS observer-off, context, acceptance stages, deduplication and bounded overflow");return 0;}
'''
    unit = out/'probe.c'
    unit.write_text(code, encoding='utf8')
    subprocess.run([str(a.cc), '-std=c11', '-O0', '-I'+str(repo/'port/include'), str(unit), '-o', str(out/'probe.exe')], check=True)
    r = subprocess.run([str(out/'probe.exe')], capture_output=True, timeout=10)
    (out/'run.log').write_bytes(r.stdout+r.stderr)
    print((r.stdout+r.stderr).decode(errors='replace'), end='')
    if r.returncode:
        raise SystemExit(r.returncode)
    csv = (out/'preview_trace.csv').read_text(encoding='utf8').splitlines()
    assert len(csv) == 258 and 'dropped=4' in csv[0]
    assert csv[2].split(',')[1] == 'request' and csv[3].split(',')[1] == 'draw-rejected'
    # Prove the disabled-clock assertion detects a broken observer, in a
    # private include tree rather than mutating production source.
    include = out/'broken/port'
    include.mkdir(parents=True)
    header = (repo/'port/include/port/preview_trace.h').read_text(encoding='utf8')
    guard = '    if (!PreviewTraceEnabled()) return;'
    assert guard in header
    (include/'preview_trace.h').write_text(header.replace(guard, '    (void)PreviewTraceEnabled();', 1), encoding='utf8')
    subprocess.run([str(a.cc), '-std=c11', '-O0', '-I'+str(out/'broken'), str(unit), '-o', str(out/'broken.exe')], check=True)
    broken = subprocess.run([str(out/'broken.exe')], capture_output=True, timeout=10)
    (out/'broken-off.log').write_bytes(broken.stdout+broken.stderr)
    assert broken.returncode == 97 and b'clocks==0' in broken.stderr
    print('PASS negative control: observer-off clock regression rejected')


if __name__ == '__main__':
    main()
