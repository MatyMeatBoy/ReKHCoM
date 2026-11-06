/* Actual SDL host implementation: bounded pool, semaphores and SysV callback.
 * Build at a fixed low image base: protocol callback addresses are uint32. */
#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#define THREAD_HOST_SDL 1
#define Log(...) ((void)0)
#include "../ps5/shared/thread_host.h"

static SDL_AtomicInt completed;
static int go,done;
static int __attribute__((sysv_abi)) Entry(uint32_t fn,uint32_t arg,uint32_t stack) {
    assert(fn==123 && arg<7 && stack==0x1000+arg*16);
    H_thread(3,go,0,0,0);
    SDL_AddAtomicInt(&completed,1);
    H_thread(4,done,0,0,0);
    return 0;
}
int main(void) {
    int i,n;
    assert(SDL_Init(0));
    _putenv_s("KHCOM_PPU_THREADS","8");
    assert(H_thread(0,0,0,0,0)==8); /* RED on the old single-core host. */
    _putenv_s("KHCOM_PPU_THREADS","1");
    assert(H_thread(0,0,0,0,0)==1);
    _putenv_s("KHCOM_PPU_THREADS","999");
    assert(H_thread(0,0,0,0,0)==8);
    _putenv_s("KHCOM_PPU_THREADS","8");
    go=H_thread(2,0,0,0,0);done=H_thread(2,0,0,0,0);
    assert(go>0 && done>0);
    assert((uintptr_t)Entry<=UINT32_MAX);
    for(i=0;i<7;i++) assert(H_thread(1,(uint32_t)(uintptr_t)Entry,123,i,0x1000+i*16));
    assert(!H_thread(1,(uint32_t)(uintptr_t)Entry,123,7,0));
    for(i=0;i<7;i++) H_thread(4,go,0,0,0);
    for(i=0;i<7;i++) H_thread(3,done,0,0,0);
    assert(SDL_GetAtomicInt(&completed)==7);
    for(n=2;n<16;n++) assert(H_thread(2,0,0,0,0)>0);
    assert(!H_thread(2,0,0,0,0));
    puts("PASS actual SDL thread host bounds, semaphore barrier and SysV callback");
    return 0;
}
