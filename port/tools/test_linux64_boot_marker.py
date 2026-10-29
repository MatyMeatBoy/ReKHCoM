"""Compile the real marker/guest-entry boundary and inject filesystem failures."""
from pathlib import Path
import argparse
import json
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--work', type=Path, required=True)
    ap.add_argument('--source', type=Path, default=ROOT / 'port/linux64/linux64_host.c')
    args = ap.parse_args()
    out = args.work.resolve()
    out.mkdir(parents=True, exist_ok=True)
    text = args.source.read_text(encoding='utf8')
    declaration = text.index('    int (*boot)(uint32_t) =')
    gate = '    if (!PortStartupDeadlineEnter(&startupDeadline)) return 75;'
    begin = text.index(gate, declaration) if gate in text[declaration:] else text.index('    if (bootStatus)', declaration)
    end = text.index('    Log("startup: guest core returned', begin)
    block = text[begin:end]
    probe = r'''
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#undef assert
#define assert(x) do { if (!(x)) { fprintf(stderr,"contract failed: %s\n",#x); exit(91); } } while(0)
static const char *fault;
static int enters, removes, quits, startupDeadline;
static bool PortStartupDeadlineEnter(int *d){(void)d;return strcmp(fault,"deadline");}
static void StartupStage(const char*a,const char*b){printf("%s:%s\n",a,b);}
#define Log(...) ((void)0)
static FILE *Open(const char*a,const char*b){(void)a;(void)b;return !strcmp(fault,"open")?NULL:(FILE*)1;}
static int Put(const char*a,FILE*b){(void)a;(void)b;return !strcmp(fault,"write")?-1:0;}
static int Close(FILE*f){(void)f;return !strcmp(fault,"close")?-1:0;}
static int Remove(const char*p){(void)p;removes++;return 0;}
static void SDL_Quit(void){quits++;}
static int boot(unsigned n){(void)n;enters++;return 20;}
#define fopen Open
#define fputs Put
#define fclose Close
#define remove Remove
static int run(void){
 const char *bootStatus=!strcmp(fault,"no-marker")?NULL:"marker";
 struct {unsigned stackTop;} h={0};
BLOCK
 return rc;
}
int main(int argc,char**argv){fault=argc>1?argv[1]:"success";int rc=run();
 bool fail=!strcmp(fault,"open")||!strcmp(fault,"write")||!strcmp(fault,"close");
 bool deadline=!strcmp(fault,"deadline");
 assert(rc==(deadline?75:fail?4:20));assert(enters==(!fail&&!deadline));
 assert(removes==(!strcmp(fault,"write")||!strcmp(fault,"close")));
 assert(quits==fail);return 0;}
'''.replace('BLOCK', block)
    (out / 'probe.c').write_text(probe, encoding='utf8', newline='\n')
    cc = ROOT.parent / 'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe'
    subprocess.run([str(cc), '-O2', str(out / 'probe.c'), '-o', str(out / 'probe.exe')], check=True)
    cases = []
    for mode in ('success', 'open', 'write', 'close', 'no-marker', 'deadline'):
        p = subprocess.run([str(out / 'probe.exe'), mode], capture_output=True, text=True, timeout=5)
        assert p.returncode == 0, (mode, p.returncode, p.stderr)
        cases.append(dict(case=mode, exit=p.returncode, log=p.stdout))
    (out / 'RESULT.json').write_text(json.dumps(dict(result='PASS', cases=cases), indent=2), encoding='utf8')
    print('PASS6 actual marker/guest gate: failed open/write/close and lost deadline never enter guest')


if __name__ == '__main__':
    main()
