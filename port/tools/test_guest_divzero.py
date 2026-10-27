"""Execute real x32-style DIV/IDIV faults through the production Windows adapter."""
from pathlib import Path
import argparse, os, shutil, struct, subprocess, tempfile

ROOT=Path(__file__).resolve().parents[2]

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--cc',type=Path)
    args=ap.parse_args()
    header=ROOT/'port/windows64/guest_divzero_windows.h'
    adapter='#include "port/windows64/guest_divzero_windows.h"' if header.exists() else '''
typedef struct {uintptr_t begin,end;} PortGuestDivisionBounds;
static PortGuestDivisionBounds sGuestDivision;
static LONG CALLBACK GuestDivisionHandler(EXCEPTION_POINTERS*ep){(void)ep;return EXCEPTION_CONTINUE_SEARCH;}
'''
    code=r'''
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
''' + adapter + r'''
int main(int argc,char**argv){
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);
    if(argc!=2)return 3;
    int mode=atoi(argv[1]);
    unsigned char* p=VirtualAlloc((void*)0x10000000,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);
    if(!p)return 4;
    /* mov eax,123; xor edx,edx; mov ecx,divisor; div/idiv ecx; ret */
    unsigned char body[]={0xb8,123,0,0,0,0x31,0xd2,0xb9,0,0,0,0,0xf7,0xf1,0x48,0xc1,0xe2,0x20,0x48,0x09,0xd0,0xc3};
    if(mode==1)body[13]=0xf9;
    if(mode==2){body[1]=0;body[4]=0x80;body[5]=0x99;body[6]=0x90;memset(body+8,0xff,4);body[13]=0xf9;}
    if(mode==3)body[8]=2;
    memcpy(p,body,sizeof(body));
    sGuestDivision.begin=(uintptr_t)p;sGuestDivision.end=(uintptr_t)p+sizeof(body);
    if(mode==4)sGuestDivision.begin=(uintptr_t)p+64;
    AddVectoredExceptionHandler(1,GuestDivisionHandler);
    uint64_t (*call)(void)=(uint64_t(*)(void))p;
    uint64_t result=call();
    if(result!=(mode==3?(((uint64_t)1<<32)|61):0))return 97;
    VirtualFree(p,0,MEM_RELEASE);
    puts("PASS real guest division fault/control");return 0;
}
'''
    cc=args.cc or ROOT.parents[1]/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-clang.exe'
    with tempfile.TemporaryDirectory(prefix='guest-divzero-') as tmp:
        p=Path(tmp);unit=p/'fault.c';unit.write_text(code);exe=p/'fault.exe'
        subprocess.run([str(cc),'-std=c11','-O2','-I'+str(ROOT),str(unit),'-o',str(exe)],check=True)
        for mode,expected in [(0,0),(1,0),(2,0xc0000095),(3,0),(4,0xc0000094)]:
            r=subprocess.run([str(exe),str(mode)],capture_output=True,text=True,timeout=5)
            print('mode',mode,'exit',hex(r.returncode & 0xffffffff),r.stdout.strip())
            assert r.returncode & 0xffffffff==expected,(mode,r.returncode,r.stderr)
        decoder=p/'decoder.exe'
        subprocess.run([str(cc),'-std=c11','-O2','-I'+str(ROOT),str(ROOT/'port/tools/test_guest_divzero_decoder.c'),'-o',str(decoder)],check=True)
        r=subprocess.run([str(decoder)],capture_output=True,text=True,timeout=5)
        print(r.stdout,end='');assert r.returncode==0,(r.returncode,r.stderr)
        # Exercise the production guest linker script with a ROM-free image.
        assembly=p/'guest.s';assembly.write_text('.text\n.global ps5_boot\nps5_boot: ret\n.bss\n.global gHostTable\ngHostTable: .space 1024\n.global gPortBiosArea\ngPortBiosArea: .space 65536\n')
        obj=p/'guest.o';elf=p/'guest.elf';raw=p/'guest.bin';tc=Path(cc).parent
        subprocess.run([str(cc),'--target=x86_64-unknown-linux-gnux32','-c',str(assembly),'-o',str(obj)],check=True)
        subprocess.run([str(tc/'ld.lld.exe'),'-m','elf32_x86_64','-T',str(ROOT/'port/ps5/guest/guest.ld'),str(obj),'-o',str(elf)],check=True)
        subprocess.run([str(tc/'llvm-objcopy.exe'),'-O','binary',str(elf),str(raw)],check=True)
        words=struct.unpack_from('<12I',raw.read_bytes())
        assert words[0]==0x35474b52 and words[9:]==(0x31564447,0x10000030,0x10000031),words
        assert words[2]==words[10]
        print('PASS production guest linker: original header plus exact initialized text metadata')

if __name__=='__main__':main()
