"""Compile actual shared-shake functions in isolated fixtures; no game/ROM.

Requires an explicitly authorized C window and explicit output directory.
The source is extracted from src/btl/btl_map.c, never copied as a stale oracle.
"""
from pathlib import Path
import argparse,subprocess,re,os,json,hashlib

ROOT=Path(__file__).resolve().parents[2]

def function(source,name):
    start=source.index('void '+name+'(');end=source.index('{',start)+1;depth=1
    while depth:depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--cc',required=True,type=Path);ap.add_argument('--output',required=True,type=Path)
    args=ap.parse_args();out=args.output.resolve();out.mkdir(parents=True,exist_ok=True)
    source=(ROOT/'src/btl/btl_map.c').read_text()
    pattern=re.search(r'static const s8 sBtlMapShakePattern\[32\] = \{.*?\};',source,re.S)[0]
    funcs='\n'.join(function(source,n)for n in ('BtlMapResetShake','BtlMapStartShake','BtlMapUpdateShake'))
    c=r'''
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <windows.h>
#endif
typedef signed char s8;typedef unsigned char u8;typedef short s16;
typedef unsigned short u16;typedef int s32;
#define TRUE 1
#define FALSE 0
typedef struct BtlWork {u8 prefix[24];u8 rotation;u8 suffix[8];} BtlWork;
BtlWork* gBtlWork;
static u8 sBtlMapShakeActive;static u16 sBtlMapShakeStep;static s32 sBtlMapShakeOffset;
''' + pattern+'\n'+funcs+r'''
int main(int argc,char** argv) {
    BtlWork live,expected;int has_work,active,i;
#ifdef _WIN32
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
#endif
    setvbuf(stdout,NULL,_IONBF,0);
    assert(argc==3);has_work=atoi(argv[1]);active=atoi(argv[2]);
    memset(&live,0x5a,sizeof(live));live.rotation=77;
    memcpy(&expected,&live,sizeof(live));
    gBtlWork=has_work?&live:NULL;
    assert(offsetof(BtlWork,rotation)==24);
    BtlMapResetShake();assert(!sBtlMapShakeActive&&!sBtlMapShakeStep&&!sBtlMapShakeOffset);
    if(active)BtlMapStartShake();
    puts("step,offset,active,rotation");
    for(i=0;i<36;i++) {
        BtlMapUpdateShake();
        printf("%d,%d,%d,%d\n",i+1,sBtlMapShakeOffset,sBtlMapShakeActive,gBtlWork?gBtlWork->rotation:-1);
        if(active&&i<31)assert(sBtlMapShakeActive);
        else assert(!sBtlMapShakeActive&&!sBtlMapShakeOffset);
    }
    if(has_work&&active)expected.rotation=0;
    assert(!memcmp(&live,&expected,sizeof(live)));
    BtlMapResetShake();assert(!sBtlMapShakeActive&&!sBtlMapShakeStep&&!sBtlMapShakeOffset);
    return 0;
}
'''
    src=out/'shake_fixture.c';exe=out/('shake_fixture.exe'if os.name=='nt'else'shake_fixture')
    src.write_text(c,encoding='utf8')
    flags=(subprocess.CREATE_NO_WINDOW|subprocess.BELOW_NORMAL_PRIORITY_CLASS)if os.name=='nt'else 0
    compile_cmd=[str(args.cc),'-O2','-fwrapv',str(src),'-o',str(exe)]
    build=subprocess.run(compile_cmd,capture_output=True,creationflags=flags)
    (out/'compile.stdout').write_bytes(build.stdout);(out/'compile.stderr').write_bytes(build.stderr)
    build.check_returncode();runs=[]
    for has_work,active in ((0,0),(1,0),(0,1),(1,1)):
        name=('live'if has_work else'null')+('-active'if active else'-idle')
        run=subprocess.run([str(exe),str(has_work),str(active)],capture_output=True,creationflags=flags)
        (out/(name+'.csv')).write_bytes(run.stdout);(out/(name+'.stderr')).write_bytes(run.stderr)
        runs.append(dict(case=name,exit_code=run.returncode,stdout_rows=max(0,len(run.stdout.splitlines())-1)))
    report=dict(source_sha256=hashlib.sha256(source.encode()).hexdigest(),functions_sha256=hashlib.sha256(funcs.encode()).hexdigest(),compile_command=compile_cmd,compile_exit=build.returncode,exe_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),runs=runs,all_pass=all(x['exit_code']==0 for x in runs),priority='BELOW_NORMAL',game=False,ROM=False)
    (out/'RESULT.json').write_text(json.dumps(report,indent=2));print(json.dumps(report))
    return 0 if report['all_pass']else 1

if __name__=='__main__':raise SystemExit(main())
