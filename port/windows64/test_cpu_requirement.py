"""Compile only native hosts; mask CPUID in a test build, then exercise real main.

No guest build, ROM, real saves or windows. A synthetic guest returns 81.
The CPUID override is a forced include in this test only, never a shipped flag.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import struct
import subprocess
import sys

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'port/windows64'))
from prepare_movie_core import prepare


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--work',type=Path,required=True)
    p.add_argument('--toolchain',type=Path,required=True)
    p.add_argument('--sdl',type=Path,required=True)
    a=p.parse_args();a.work=a.work.resolve();a.work.mkdir(parents=True,exist_ok=True)
    prepare(ROOT,a.work)
    # Preserve the CPU's real other feature bits; only clear SSE4.1.
    override=a.work/'no-sse41.h'
    override.write_text('#include <intrin.h>\nstatic void CpuWithoutSse41(int r[4],int leaf){__cpuid(r,leaf);if(leaf==1)r[2]&=~(1u<<19);}\n#define __cpuid CpuWithoutSse41\n')
    cc=a.toolchain/'bin/clang.exe';sdl=a.sdl/'x86_64-w64-mingw32'
    flags=[str(cc),'--target=x86_64-w64-mingw32','-O2',f'-I{ROOT/"port/ps5/shared"}',
           f'-I{ROOT/"port/ps5/third_party/stb"}',f'-I{sdl/"include"}',f'-I{a.work}',f'-L{sdl/"lib"}',
           '-lmfplat','-lmfreadwrite','-lmfuuid','-lole32','-lpropsys','-lgdi32']
    blob=bytearray(0x3000)
    struct.pack_into('<9I',blob,0,0x35474B52,0x10003000,0x10001000,0x10002000,0x10003000,0x10008000,0x10010000,0x10010000,0)
    blob[0x1000:0x1006]=bytes.fromhex('b851000000c3')
    image=a.work/'synthetic.bin';image.write_bytes(blob)
    marker=a.work/'entered.txt'
    sentinels={n:b'private test sentinel\n' for n in ('kh_com.sav','kh_com.ini','scenes_seen.dat')}
    for n,b in sentinels.items():(a.work/n).write_bytes(b)
    env={k:v for k,v in os.environ.items() if not k.startswith(('SDL_','KHCOM_'))}
    env.update(SDL_VIDEO_DRIVER='dummy',SDL_AUDIO_DRIVER='dummy',KHCOM_HOST_HIDDEN='1')
    results=[]
    for missing in (False,True):
        name='no-sse41' if missing else 'native-cpu';host=a.work/f'{name}.exe'
        command=flags+(['-include',str(override)] if missing else [])+[str(ROOT/'port/windows64/windows64_host.c'),'-lSDL3','-lm','-o',str(host)]
        subprocess.run(command,check=True,cwd=ROOT)
        shutil.copy2(sdl/'bin/SDL3.dll',a.work/'SDL3.dll')
        marker.write_text('stale')
        run=subprocess.run([str(host),str(image),'--data',str(a.work),'--boot-status',str(marker)],
                           cwd=a.work,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,
                           creationflags=0x08004000,timeout=20)
        (a.work/f'{name}.log').write_bytes(run.stdout)
        print(name,'exit',run.returncode,'marker',marker.exists(),flush=True)
        assert run.returncode==(5 if missing else 81),run.stdout.decode(errors='replace')
        assert marker.exists()!=missing,'CPU rejection must occur before guest entry, removing stale marker'
        if missing:
            assert b'SSE4.1' in run.stdout and b'32' in run.stdout,run.stdout
        assert all((a.work/n).read_bytes()==b for n,b in sentinels.items()),'persistent data changed'
        results.append(dict(case=name,exit=run.returncode,entered=marker.exists(),sha256=hashlib.sha256(host.read_bytes()).hexdigest()))
    (a.work/'cpu-results.json').write_text(json.dumps(results,indent=2))
    print('PASS real native host: CPUID supported/unsupported, no guest entry on rejection, persistent data preserved')


if __name__=='__main__':main()
