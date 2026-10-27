#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "port/windows64/guest_divzero_windows.h"

static void Registers(CONTEXT* c, const uint64_t r[16]) {
    c->Rax=r[0];c->Rcx=r[1];c->Rdx=r[2];c->Rbx=r[3];c->Rsp=r[4];c->Rbp=r[5];c->Rsi=r[6];c->Rdi=r[7];
    c->R8=r[8];c->R9=r[9];c->R10=r[10];c->R11=r[11];c->R12=r[12];c->R13=r[13];c->R14=r[14];c->R15=r[15];
}
int main(void) {
    unsigned cases=0;
    const int prefixes[]={-1,0x40,0x41,0x48,0x66,0x67,0xf3};
    const uint64_t divisors[]={0,1,0xffffffffu,0x100000000ull};
    for(unsigned p=0;p<sizeof(prefixes)/sizeof(prefixes[0]);p++)
    for(unsigned opcode=0xf6;opcode<=0xf7;opcode++)
    for(unsigned modrm=0;modrm<256;modrm++)
    for(unsigned value=0;value<sizeof(divisors)/sizeof(divisors[0]);value++) {
        unsigned char code[3]; unsigned length=0;
        if(prefixes[p]>=0)code[length++]=(unsigned char)prefixes[p];
        code[length++]=(unsigned char)opcode;code[length++]=(unsigned char)modrm;
        uint64_t regs[16];for(int i=0;i<16;i++)regs[i]=0x1111111111111111ull+i;
        unsigned operand=(modrm&7)+(prefixes[p]==0x41?8:0);
        regs[operand]=divisors[value];
        CONTEXT context,expected;
        memset(&context,0xa5,sizeof(context));Registers(&context,regs);context.Rip=(uintptr_t)code;
        expected=context;
        EXCEPTION_RECORD record={0};record.ExceptionCode=EXCEPTION_INT_DIVIDE_BY_ZERO;record.ExceptionAddress=code;
        EXCEPTION_POINTERS ep={&record,&context};
        sGuestDivision.begin=(uintptr_t)code;sGuestDivision.end=(uintptr_t)code+length;
        int accepts=(prefixes[p]==-1 || prefixes[p]==0x40 || prefixes[p]==0x41) && opcode==0xf7 &&
                    (modrm&0xc0)==0xc0 && ((modrm>>3)&7)>=6 && (uint32_t)divisors[value]==0;
        if(accepts){expected.Rax=0;expected.Rdx=0;expected.Rip+=length;}
        LONG result=GuestDivisionHandler(&ep);
        if(result!=(accepts?EXCEPTION_CONTINUE_EXECUTION:EXCEPTION_CONTINUE_SEARCH) || memcmp(&context,&expected,sizeof(context)))return 10;
        /* Truncated instruction, guest data/heap, and unrelated exception stay untouched. */
        for(int reject=0;reject<4;reject++) {
            context=expected;context.Rip=(uintptr_t)code;expected=context;
            sGuestDivision.begin=(uintptr_t)code;sGuestDivision.end=(uintptr_t)code+length;
            record.ExceptionCode=EXCEPTION_INT_DIVIDE_BY_ZERO;record.ExceptionAddress=code;
            if(reject==0)sGuestDivision.end--;
            if(reject==1)sGuestDivision.begin=(uintptr_t)code+length;
            if(reject==2)record.ExceptionCode=EXCEPTION_ACCESS_VIOLATION;
            if(reject==3)record.ExceptionAddress=code+1;
            if(GuestDivisionHandler(&ep)!=EXCEPTION_CONTINUE_SEARCH || memcmp(&context,&expected,sizeof(context)))return 11;
        }
        cases++;
    }
    unsigned char image[256]={0};uint32_t meta[]={PORT_GUEST_DIVISION_MAGIC,0x10000030,0x10000080};
    memcpy(image+36,meta,sizeof(meta));
    PortGuestDivisionBounds valid=PortGuestDivisionImage(image,sizeof(image),0x10000000,0x10000100,0x10000040);
    if(valid.begin!=meta[1] || valid.end!=meta[2])return 12;
    for(int reject=0;reject<6;reject++){
        memcpy(image+36,meta,sizeof(meta));
        size_t size=sizeof(image);uintptr_t imageEnd=0x10000100,boot=0x10000040;
        if(reject==0)memset(image+36,0,4);
        if(reject==1)size=47;
        if(reject==2)size=64;
        if(reject==3)imageEnd=0x10000050;
        if(reject==4)boot=0x10000080;
        if(reject==5)memcpy(image+40,"\x24\x00\x00\x10",4);
        valid=PortGuestDivisionImage(image,size,0x10000000,imageEnd,boot);
        if(valid.begin || valid.end)return 13;
    }
    printf("PASS %u decoder/context cases plus text metadata and rejection controls\n",cases);
    return 0;
}
