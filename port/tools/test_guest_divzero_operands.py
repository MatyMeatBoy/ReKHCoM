"""Exercise real register/memory DIV/IDIV widths through the Windows adapter."""
from pathlib import Path
import argparse, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[2]
CODE=r'''
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "port/windows64/guest_divzero_windows.h"
static unsigned char *p;static unsigned n;
static void B(unsigned b){p[n++]=(unsigned char)b;}
static void D(uint32_t v){memcpy(p+n,&v,4);n+=4;}
static void Q(uint64_t v){memcpy(p+n,&v,8);n+=8;}
int main(int argc,char**argv){
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);
 if(argc!=6)return 3;
 int bytes=atoi(argv[1]),kind=atoi(argv[2]),sign=atoi(argv[3]),value=atoi(argv[4]),reject=atoi(argv[5]);
 p=VirtualAlloc((void*)0x10000000,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);if(!p)return 4;
 uint64_t target=(uintptr_t)p+256,stored=(uint64_t)value;memcpy(p+256,&stored,8);
 B(0xb8);D(123);B(0x31);B(0xd2); /* dividend AX/DX:AX/EDX:EAX/RDX:RAX */
 if(kind==0){B(0xb9);D(value);}else{B(0x48);B(0xb9);Q(kind==4?target-127:kind==6?target+0x100000000ull:kind==9?target+128:kind==10?target+0x1234:target);}
 if(kind==7){B(0x41);B(0xb8);D(value);} /* r8 operand */
 if(kind==11){B(0x49);B(0xbd);Q(target);} /* r13 base, mod01 disp0 */
 if(bytes==2)B(0x66);
 if(kind==3||kind==6)B(0x67);
 if(bytes==8||kind==7||kind==11)B(0x40|(bytes==8?8:0)|((kind==7||kind==11)?1:0));
 B(bytes==1?0xf6:0xf7);
 unsigned op=(sign?7:6)<<3;
 if(kind==0)B(0xc1|op);
 else if(kind==1)B(1|op);
 else if(kind==2||kind==3){B(5|op);D((uint32_t)(target-((uintptr_t)p+n+4)));}
 else if(kind==4){B(0x44|op);B(0x11);B(127);} /* [rcx+rdx+127] */
 else if(kind==5){B(4|op);B(0x85);D((uint32_t)(target-123*4));} /* [rax*4+disp32] */
 else if(kind==6){B(4|op);B(0x11);} /* address-size32 truncates high rcx */
 else if(kind==7)B(0xc0|op);
 else if(kind==8)B(0xc4|op); /* AH: zero high byte of initial AX=123 */
 else if(kind==9){B(0x41|op);B(128);} /* [rcx-128] */
 else if(kind==10){B(0x81|op);D((uint32_t)-0x1234);} /* [rcx-0x1234] */
 else if(kind==11){B(0x45|op);B(0);} /* [r13+0] */
 else return 5;
 B(0x48);B(0xc1);B(0xe2);B(0x20);B(0x48);B(0x09);B(0xd0);B(0xc3);
 sGuestDivision.begin=(uintptr_t)p;sGuestDivision.end=(uintptr_t)p+n;
#ifdef PORT_GUEST_DIVISION_FULL
 sGuestDivisionMemory.count=reject?0:1;
 sGuestDivisionMemory.range[0].begin=(uintptr_t)p;sGuestDivisionMemory.range[0].end=(uintptr_t)p+4096;
#endif
 AddVectoredExceptionHandler(1,GuestDivisionHandler);
 uint64_t actual=((uint64_t(*)(void))p)();
 uint64_t expected=value?(bytes==1?0x13d:((uint64_t)1<<32)|61):0;
 if(actual!=expected){fprintf(stderr,"got%llx expected%llx\n",(unsigned long long)actual,(unsigned long long)expected);return 97;}
 VirtualFree(p,0,MEM_RELEASE);return 0;
}
'''
def main():
 ap=argparse.ArgumentParser(description=__doc__)
 ap.add_argument('--emit-linux',type=Path,help='emit equivalent real-signal fork runner for an existing Linux runtime')
 args=ap.parse_args()
 if args.emit_linux:
  code=CODE
  replacements={
   '#define WIN32_LEAN_AND_MEAN\n#include <windows.h>':'#define _GNU_SOURCE\n#include <sys/mman.h>\n#include <sys/wait.h>\n#include <unistd.h>\n#define SetErrorMode(x) ((void)0)\n#define SEM_FAILCRITICALERRORS 0\n#define SEM_NOGPFAULTERRORBOX 0\n#define SEM_NOOPENFILEERRORBOX 0',
   '#include "port/windows64/guest_divzero_windows.h"':'#include "port/linux64/guest_divzero_linux.h"',
   'int main(int argc,char**argv){':'static int RunOperand(int argc,char**argv){',
   'p=VirtualAlloc((void*)0x10000000,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);if(!p)return 4;':'p=mmap((void*)0x10000000,4096,PROT_READ|PROT_WRITE|PROT_EXEC,MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);if(p==MAP_FAILED)return 4;',
   'AddVectoredExceptionHandler(1,GuestDivisionHandler);':'if(!GuestDivisionInstall())return 6;',
   'VirtualFree(p,0,MEM_RELEASE);':'munmap(p,4096);',
  }
  for old,new in replacements.items():
   assert code.count(old)==1,old
   code=code.replace(old,new)
  code+=r'''
int main(void){
 int checked=0;
 for(int width=1;width<=8;width*=2)for(int kind=0;kind<12;kind++){
  if(kind==8 && width!=1)continue;
  for(int sign=0;sign<=1;sign++)for(int value=0;value<=2;value+=2){
   if(kind==8 && value)continue;
   for(int reject=0;reject<2;reject++){
    if(reject && (value || kind==0 || kind==7 || kind==8))continue;
    char w[8],k[8],s[8],v[8],r[8];snprintf(w,8,"%d",width);snprintf(k,8,"%d",kind);
    snprintf(s,8,"%d",sign);snprintf(v,8,"%d",value);snprintf(r,8,"%d",reject);
    char* argv[]={"probe",w,k,s,v,r};pid_t child=fork();if(child<0)return 20;
    if(!child)_exit(RunOperand(6,argv));
    int status;if(waitpid(child,&status,0)!=child)return 21;
    if(reject?(!WIFSIGNALED(status)||WTERMSIG(status)!=SIGFPE):(!WIFEXITED(status)||WEXITSTATUS(status)!=0)){
     fprintf(stderr,"width%d kind%d sign%d value%d reject%d status%x\n",width,kind,sign,value,reject,status);return 22;
    }
    checked++;
   }
  }
 }
 printf("PASS Linux %d real register/memory DIV/IDIV width/addressing/normal/bounds cases\n",checked);return 0;
}
'''
  args.emit_linux.parent.mkdir(parents=True,exist_ok=True)
  args.emit_linux.write_text(code)
  print(args.emit_linux)
  return
 cc=ROOT.parents[1]/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-clang.exe'
 with tempfile.TemporaryDirectory(prefix='guest-div-operands-') as tmp:
  folder=Path(tmp);unit=folder/'actual.c';exe=folder/'actual.exe';unit.write_text(CODE)
  subprocess.run([str(cc),'-std=c11','-O2','-I'+str(ROOT),str(unit),'-o',str(exe)],check=True)
  checked=0
  for width in (1,2,4,8):
   for kind in range(12):
    if kind==8 and width!=1:continue
    for signed in (0,1):
     for value in (0,2):
      if kind==8 and value:continue
      args=[str(width),str(kind),str(signed),str(value),'0']
      r=subprocess.run([str(exe),*args],capture_output=True,text=True,timeout=5)
      assert r.returncode==0,(args,hex(r.returncode&0xffffffff),r.stderr)
      checked+=1
     if kind not in (0,7,8):
      args=[str(width),str(kind),str(signed),'0','1']
      r=subprocess.run([str(exe),*args],capture_output=True,text=True,timeout=5)
      assert r.returncode&0xffffffff==0xc0000094,('unapproved memory must remain fatal',args,r.returncode,r.stderr)
      checked+=1
  print('PASS',checked,'real DIV/IDIV operand cases: 8/16/32/64-bit, registers/high-byte/REX, memory/SIB/RIP/address32; normal controls and memory bounds')
if __name__=='__main__':main()
