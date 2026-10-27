"""Exercise the actual guest environment cache; no game/ROM/device required."""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
CC=next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
PRE=r'''
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
static void* allocations[10000]; static size_t sizes[10000]; static int count, calls, failAt, live, freed[10000], queries; static size_t liveBytes, peakBytes;
static void* alloc(size_t n){calls++;if(calls==failAt)return NULL;void*p=malloc(n);assert(p&&count<10000);allocations[count]=p;sizes[count++]=n;live++;liveBytes+=n;if(liveBytes>peakBytes)peakBytes=liveBytes;return p;}
static void release(void*p){if(!p)return;for(int i=0;i<count;i++)if(allocations[i]==p){assert(!freed[i]);freed[i]=1;memset(p,0xcd,sizes[i]);live--;liveBytes-=sizes[i];return;}assert(0);}
static char* duplicate(const char*s){size_t n=strlen(s)+1;char*p=alloc(n);if(p)memcpy(p,s,n);return p;}
static int h_getenv(const char*n,char*out,int cap){
 queries++;
 if(!strcmp(n,"KHCOM_QUIT"))return snprintf(out,cap,"1000");
 int size=!strcmp(n,"LONG")?1000:!strncmp(n,"BUDGET_",7)?700000:!strcmp(n,"OVERSIZE")?2000000:0;
 if(!size)return -1;
 int len=size<cap-1?size:cap-1;memset(out,'x',len);out[len]=0;return len;
}
#define malloc alloc
#define free release
#define strdup duplicate
/* CACHE */
#undef malloc
#undef free
#undef strdup
int main(int argc,char**argv){
 const char*test=argc>1?argv[1]:"saturation";
 if(!strcmp(test,"oom")){
  failAt=calls+1;assert(!GetenvLocked("KHCOM_QUIT"));assert(!live);
  failAt=calls+2;assert(!GetenvLocked("KHCOM_QUIT"));assert(!live);
  failAt=0;const char*p=GetenvLocked("KHCOM_QUIT");assert(p&&!strcmp(p,"1000"));
  int prior=live;failAt=calls+3;assert(!GetenvLocked("LONG"));assert(live==prior&&!strcmp(p,"1000"));
  failAt=0;const char*q=GetenvLocked("LONG");assert(q&&strlen(q)==1000);assert(GetenvLocked("LONG")==q);
  assert(!GetenvLocked(NULL)&&!GetenvLocked(""));return 0;
 }
 const char*held=GetenvLocked("KHCOM_QUIT");assert(held&&!strcmp(held,"1000"));char key[40];
 if(!strcmp(test,"limits")){
  int baseline=live;assert(!GetenvLocked("OVERSIZE"));assert(live==baseline);
  int successes=0;
  for(int i=0;i<12;i++){snprintf(key,sizeof(key),"BUDGET_%d",i);const char*p=GetenvLocked(key);if(p){assert(strlen(p)==700000);successes++;}}
  assert(successes==3&&peakBytes<=4u*1024*1024&&!strcmp(held,"1000"));return 0;
 }
 for(int i=0;i<1500;i++){snprintf(key,sizeof(key),"ABSENT_%d",i);assert(!GetenvLocked(key));}
 assert(!memcmp(held,"1000",5));assert(GetenvLocked("KHCOM_QUIT")==held);
 int before=calls;assert(!GetenvLocked("NEW_AFTER_FULL"));assert(calls==before);assert(peakBytes<=4u*1024*1024);
 return 0;
}
'''
def main():
 source=(ROOT/'port/ps5/guest/src/libc.c').read_text(encoding='utf-8')
 cache=source[source.index('static char* GetenvLocked(const char* name) {'):source.index('\nvoid exit(',source.index('static char* GetenvLocked(const char* name) {'))]
 with tempfile.TemporaryDirectory() as td:
  p=Path(td)/'test.c';p.write_text(PRE.replace('/* CACHE */',cache),encoding='utf-8');exe=p.with_suffix('.exe')
  subprocess.run([str(CC),'-std=c11',str(p),'-o',str(exe)],check=True)
  for case in ['saturation','oom','limits']:
   subprocess.run([str(exe),case],check=True,timeout=10,creationflags=0x08004000)
 print('PASS: actual guest environment cache retains pointers after saturation; transactional OOM/retry, long values and 4 MiB budget')
if __name__=='__main__':main()
