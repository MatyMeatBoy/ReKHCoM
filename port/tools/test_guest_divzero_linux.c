#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include "port/linux64/guest_divzero_linux.h"

static int forwarded;
static void Prior(int sig) { if(sig==SIGFPE)forwarded++; }
static int Child(int mode) {
    unsigned char* p=mmap((void*)0x10000000,4096,PROT_READ|PROT_WRITE|PROT_EXEC,
                         MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0);
    if(p==MAP_FAILED)return 4;
    unsigned char body[]={0xb8,123,0,0,0,0x31,0xd2,0xb9,0,0,0,0,0xf7,0xf1,0x48,0xc1,0xe2,0x20,0x48,0x09,0xd0,0xc3};
    if(mode==1)body[13]=0xf9;
    if(mode==2){body[1]=0;body[4]=0x80;body[5]=0x99;body[6]=0x90;memset(body+8,0xff,4);body[13]=0xf9;}
    if(mode==3)body[8]=2;
    memcpy(p,body,sizeof(body));
    sGuestDivision.begin=(uintptr_t)p;sGuestDivision.end=(uintptr_t)p+sizeof(body);
    if(mode==4)sGuestDivision.begin=(uintptr_t)p+64;
    if(!GuestDivisionInstall())return 5;
    struct sigaction action;stack_t stack;
    if(sigaction(SIGFPE,NULL,&action) || !(action.sa_flags&SA_ONSTACK) || sigaltstack(NULL,&stack) ||
       stack.ss_sp!=sGuestDivisionStack || stack.ss_flags&SS_DISABLE)return 6;
    uint64_t (*call)(void)=(uint64_t(*)(void))p;
    uint64_t result=call();
    return result==(mode==3?(((uint64_t)1<<32)|61):0)?0:97;
}
int main(void) {
    for(int mode=0;mode<5;mode++) {
        pid_t pid=fork();if(pid<0)return 7;if(!pid)_exit(Child(mode));
        int status;if(waitpid(pid,&status,0)!=pid)return 8;
        int rejected=mode==2 || mode==4;
        if(rejected ? !WIFSIGNALED(status) || WTERMSIG(status)!=SIGFPE : !WIFEXITED(status) || WEXITSTATUS(status)!=0)return 9;
        printf("PASS Linux real signal mode%d, status%x\n",mode,status);
    }
    /* Non-kernel/wrong-kind signals never rewrite context or enter DIV decoding. */
    ucontext_t context,original;siginfo_t info;
    memset(&context,0xa5,sizeof(context));original=context;memset(&info,0,sizeof(info));info.si_code=SI_USER;
    memset(&sGuestDivisionPrevious,0,sizeof(sGuestDivisionPrevious));sGuestDivisionPrevious.sa_handler=Prior;
    GuestDivisionSignal(SIGFPE,&info,&context);
    if(forwarded!=1 || memcmp(&context,&original,sizeof(context)))return 10;
    puts("PASS Linux wrong si_code forwarded without context mutation");return 0;
}
