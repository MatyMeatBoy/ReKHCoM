/* Low guest stacks and emulated TLS for the desktop x32 PPU. Host stacks and
 * native TLS remain owned by SDL/OS; no platform FS/GS register is changed. */
#ifdef PORT_LINUX64_GUEST
#include <SDL3/SDL.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "bridge_calls.h"
#include "bridge_threads.h"

unsigned char rk_tstacks[BRIDGE_X32_WORKERS * BRIDGE_X32_STACK_SIZE]
    __attribute__((aligned(BRIDGE_X32_STACK_SIZE)));
extern int ps5_thread(uint32_t fn,uint32_t arg,uint32_t stackTop);
static int sThreads;

/* Clang's x32 emutls descriptor: four pointer-sized words, including a
 * template for initialized TLS. Each slot owns its allocation for life. */
typedef struct { uintptr_t size,align; void** objects; const void* initial; } GuestTls;
static int sTlsLock;
static unsigned GuestSlot(void) {
    uintptr_t offset=(uintptr_t)__builtin_frame_address(0)-(uintptr_t)rk_tstacks;
    return offset<sizeof(rk_tstacks) ? 1+(offset>>BRIDGE_X32_STACK_SHIFT) : 0;
}
void* __emutls_get_address(GuestTls* control) {
    unsigned slot=GuestSlot();
    void** objects=__atomic_load_n(&control->objects,__ATOMIC_ACQUIRE);
    void* result=objects ? __atomic_load_n(&objects[slot],__ATOMIC_ACQUIRE) : NULL;
    if (result) return result;
    while (__atomic_exchange_n(&sTlsLock,1,__ATOMIC_ACQUIRE))
        while (__atomic_load_n(&sTlsLock,__ATOMIC_RELAXED)) {}
    objects=control->objects;
    if (!objects) {
        objects=calloc(BRIDGE_X32_WORKERS+1,sizeof(*objects));
        if (!objects) abort();
        __atomic_store_n(&control->objects,objects,__ATOMIC_RELEASE);
    }
    result=objects[slot];
    if (!result) {
        uintptr_t align=control->align ? control->align : 1;
        void* raw;
        if ((align&(align-1)) || control->size>SIZE_MAX-(align-1)) abort();
        raw=malloc(control->size+align-1);
        if (!raw) abort();
        result=(void*)(((uintptr_t)raw+align-1)&~(align-1));
        if (control->initial) memcpy(result,control->initial,control->size);
        else memset(result,0,control->size);
        __atomic_store_n(&objects[slot],result,__ATOMIC_RELEASE);
    }
    __atomic_store_n(&sTlsLock,0,__ATOMIC_RELEASE);
    return result;
}

static int ThreadBridge(void) {
    static int available=-1;
    if (available<0) available=h_thread(BRIDGE_THREADS_PROBE,0,0,0,0)==BRIDGE_THREADS_MAGIC;
    return available;
}
int SDL_GetNumLogicalCPUCores(void) {
    int n=ThreadBridge() ? h_thread(0,0,0,0,0) : 1;
    return n<1 ? 1 : n>BRIDGE_X32_WORKERS+1 ? BRIDGE_X32_WORKERS+1 : n;
}
SDL_Thread* SDL_CreateThread(SDL_ThreadFunction fn,const char* name,void* data) {
    unsigned slot=(unsigned)sThreads+1;
    (void)name;
    if (!ThreadBridge() || !fn || slot>BRIDGE_X32_WORKERS) return NULL;
    if (!h_thread(1,(uint32_t)(uintptr_t)ps5_thread,(uint32_t)(uintptr_t)fn,(uint32_t)(uintptr_t)data,
                  (uint32_t)(uintptr_t)(rk_tstacks+slot*BRIDGE_X32_STACK_SIZE))) return NULL;
    sThreads=(int)slot;
    return (SDL_Thread*)(uintptr_t)slot;
}
void SDL_DetachThread(SDL_Thread* thread) { (void)thread; }
SDL_Semaphore* SDL_CreateSemaphore(Uint32 initial) {
    int sem=ThreadBridge() ? h_thread(2,initial,0,0,0) : 0;
    return sem>0 ? (SDL_Semaphore*)(uintptr_t)sem : NULL;
}
void SDL_WaitSemaphore(SDL_Semaphore* sem) { h_thread(3,(uint32_t)(uintptr_t)sem,0,0,0); }
void SDL_SignalSemaphore(SDL_Semaphore* sem) { h_thread(4,(uint32_t)(uintptr_t)sem,0,0,0); }

/* Private executable regression: native SDL workers really enter x32 stacks,
 * initialize separate TLS, and cross the host bridge concurrently. */
static _Thread_local unsigned sProbeInitial=0x12345678;
static _Thread_local unsigned sProbeZero;
static _Thread_local unsigned char sProbeAligned[32] __attribute__((aligned(32)));
static SDL_Semaphore *sProbeGo,*sProbeDone;
static SDL_AtomicInt sProbeFailed;
static int ProbeBody(void* data) {
    unsigned expected=(unsigned)(uintptr_t)data,i;
    char value[32];
    if (GuestSlot()!=expected || sProbeInitial!=0x12345678 || sProbeZero ||
        ((uintptr_t)sProbeAligned&31)) SDL_SetAtomicInt(&sProbeFailed,1);
    sProbeInitial=expected;sProbeZero=expected+100;
    if (expected) SDL_WaitSemaphore(sProbeGo);
    for(i=0;i<2000;i++) {
        (void)h_ticks_us();
        h_getenv("KHCOM_PPU_THREAD_TEST",value,sizeof(value));
        if (sProbeInitial!=expected || sProbeZero!=expected+100 || GuestSlot()!=expected)
            SDL_SetAtomicInt(&sProbeFailed,1);
    }
    if (expected) SDL_SignalSemaphore(sProbeDone);
    return 0;
}
int PortGuestThreadSelfTest(void) {
    int i;
    if (!ThreadBridge() || !(sProbeGo=SDL_CreateSemaphore(0)) || !(sProbeDone=SDL_CreateSemaphore(0))) return 1;
    for(i=1;i<=2;i++) if(!SDL_CreateThread(ProbeBody,"probe",(void*)(uintptr_t)i)) return 2;
    for(i=1;i<=2;i++) SDL_SignalSemaphore(sProbeGo);
    ProbeBody(NULL);
    for(i=1;i<=2;i++) SDL_WaitSemaphore(sProbeDone);
    if(SDL_GetAtomicInt(&sProbeFailed)) return 3;
    h_log("PASS x32 low-stack TLS alignment/templates/isolation and concurrent bridge calls");
    return 0;
}
#endif
