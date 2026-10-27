
/* Full production LoadArm64Image contract probe, no guest execution.
 * Windows VirtualAlloc adapter keeps all pages RW. mprotect verifies ranges
 * and rejects WX requests but DOES NOT emulate Linux page protection. */
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#error This test adapter requires Windows; it does not emulate Linux mmap/mprotect.
#endif
#include <windows.h>
#include "bridge.h"
#define PROT_NONE 0
#define PROT_READ 1
#define PROT_WRITE 2
#define PROT_EXEC 4
#define MAP_PRIVATE 2
#define MAP_ANONYMOUS 32
#define MAP_FAILED ((void*)-1)
#define _SC_PAGESIZE 30
static uint8_t file[8192];
static size_t fileBytes=sizeof(file),mappedBytes;
static void*mapped;
static int protections,maps;
static long sysconf(int x){(void)x;return 4096;}
static void* mmap(void*p,size_t n,int prot,int flags,int fd,size_t offset){
 (void)prot;(void)flags;(void)fd;(void)offset;maps++;
 void*r=VirtualAlloc(p,n,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
 if(!r)return MAP_FAILED;
 mapped=r;mappedBytes=n;return r;
}
static int munmap(void*p,size_t n){(void)n;if(mapped==p)mapped=NULL;return VirtualFree(p,0,MEM_RELEASE)?0:-1;}
static int mprotect(void*p,size_t n,int prot){
 assert(mapped&&(uintptr_t)p>=(uintptr_t)mapped&&(uintptr_t)p+n<=(uintptr_t)mapped+mappedBytes);
 assert((prot&(PROT_WRITE|PROT_EXEC))!=(PROT_WRITE|PROT_EXEC));protections++;return 0;
}
static void Log(const char*s,...){(void)s;}
static void* SDL_LoadFile(const char*p,size_t*n){(void)p;*n=fileBytes;void*q=malloc(*n);assert(q);memcpy(q,file,*n);return q;}
#define SDL_free free
#include "macho_loader.inc"
static void init(void){
 memset(file,0,sizeof(file));fileBytes=sizeof(file);
 ArmMachHeader*h=(void*)file;
 *h=(ArmMachHeader){.magic=0xfeedface,.cpu=0x0200000c,.type=2,.ncmds=4,.cmds=3*sizeof(ArmMachSegment)+sizeof(ArmMachRebase),.flags=0x200000};
 ArmMachSegment*s=(void*)(h+1);
 s[0]=(ArmMachSegment){.cmd=1,.size=sizeof(*s),.name="__TEXT",.addr=0x50000000,.length=4096,.offset=0,.bytes=512,.prot=5};
 s[1]=(ArmMachSegment){.cmd=1,.size=sizeof(*s),.name="__DATA",.addr=0x50001000,.length=8192,.prot=3};
 s[2]=(ArmMachSegment){.cmd=1,.size=sizeof(*s),.name="__RKHDR",.addr=0x50003000,.length=4096,.offset=4096,.bytes=sizeof(Ps5GuestHeader),.prot=1};
 uint8_t rb[]={0x11,0x22,8,0x55,0x30,4,0x51,0};memcpy(file+4352,rb,sizeof(rb));
 ArmMachRebase*r=(void*)(s+3);
 *r=(ArmMachRebase){.cmd=0x80000022,.size=sizeof(*r),.offset=4352,.bytes=sizeof(rb)};
 Ps5GuestHeader*g=(void*)(file+4096);
 *g=(Ps5GuestHeader){.magic=PS5G_MAGIC,.boot=0x50000000,.hostTable=0x50001000,.heapStart=0x50001800,.heapEnd=0x50002000,.stackTop=0x50003000,.biosArea=0x50001000};
}
int main(void){
 Ps5GuestHeader h;
 _putenv("KHCOM_IMAGE_MOVE="); /* child-local control; no inherited relocation preference */
 init();assert(LoadArm64Image("synthetic",&h));
 assert(h.boot==0x50000000&&h.hostTable==0x50001000&&h.stackTop==0x50003000&&h.imageEnd==0x50004000);
 assert(protections==6);munmap(mapped,mappedBytes);
 _putenv("KHCOM_IMAGE_MOVE=1");init();assert(LoadArm64Image("synthetic",&h));
 assert(h.boot!=0x50000000&&h.hostTable==h.boot+4096&&h.stackTop==h.boot+12288&&h.imageEnd==h.boot+16384);
 munmap(mapped,mappedBytes);
 _putenv("KHCOM_IMAGE_MOVE=");init();ArmMachHeader*mh=(void*)file;ArmMachSegment*s=(void*)(mh+1);
 s[1].addr=s[0].addr;int before=maps;assert(!LoadArm64Image("overlap",&h)&&maps==before);
 init();s=(void*)(file+sizeof(ArmMachHeader));s[0].offset=8190;s[0].bytes=512;
 before=maps;assert(!LoadArm64Image("file-oob",&h)&&maps==before);
 init();Ps5GuestHeader*g=(void*)(file+4096);g->hostTable=0x50003ff0;
 assert(!LoadArm64Image("table-oob",&h)&&!mapped);
 puts("PASS full loader synthetic: preferred/moved image, header slide, overlap/file/table bounds; no ARM execution");return 0;
}
