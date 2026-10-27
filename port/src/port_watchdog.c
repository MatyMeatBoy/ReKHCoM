/*
 * Development aid: when KHCOM_WATCHDOG is set, a background thread samples
 * the game thread every few seconds and logs its call stack (frame-pointer
 * walk) so hangs can be located with llvm-addr2line.
 */
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdlib.h>

#include "port/port.h"

static HANDLE sGameThread;

static DWORD WINAPI Watch(LPVOID arg) {
    int period = atoi((const char*)arg);

    if (period <= 0)
        period = 3;
    for (;;) {
        CONTEXT ctx;
        int i;

        Sleep(period * 1000);
        SuspendThread(sGameThread);
        ctx.ContextFlags = CONTEXT_CONTROL | CONTEXT_INTEGER;
        if (GetThreadContext(sGameThread, &ctx)) {
            uint32_t ebp = ctx.Ebp;
            PortLog("watchdog: eip=%08lx", (unsigned long)ctx.Eip);
            for (i = 0; i < 24 && ebp >= 0x10000; i++) {
                uint32_t ret = 0, next = 0;
                if (IsBadReadPtr((void*)(uintptr_t)ebp, 8))
                    break;
                next = ((uint32_t*)(uintptr_t)ebp)[0];
                ret = ((uint32_t*)(uintptr_t)ebp)[1];
                PortLog("watchdog:   %08lx", (unsigned long)ret);
                if (next <= ebp)
                    break;
                ebp = next;
            }
        }
        ResumeThread(sGameThread);
    }
    return 0;
}

static void LogStack(uint32_t eip, uint32_t ebp) {
    int i;

    PortLog("  at %08lx", (unsigned long)eip);
    for (i = 0; i < 32 && ebp >= 0x10000; i++) {
        uint32_t next, ret;
        if (IsBadReadPtr((void*)(uintptr_t)ebp, 8))
            break;
        next = ((uint32_t*)(uintptr_t)ebp)[0];
        ret = ((uint32_t*)(uintptr_t)ebp)[1];
        PortLog("  at %08lx", (unsigned long)ret);
        if (next <= ebp)
            break;
        ebp = next;
    }
}

static LONG WINAPI CrashHandler(EXCEPTION_POINTERS* info) {
    EXCEPTION_RECORD* r = info->ExceptionRecord;
    CONTEXT* c = info->ContextRecord;

    PortLog("CRASH: exception %08lx at %08lx (access %s %08lx)", (unsigned long)r->ExceptionCode,
            (unsigned long)c->Eip, r->NumberParameters > 0 && r->ExceptionInformation[0] ? "write" : "read",
            r->NumberParameters > 1 ? (unsigned long)r->ExceptionInformation[1] : 0ul);
    PortLog("  eax=%08lx ebx=%08lx ecx=%08lx edx=%08lx esi=%08lx edi=%08lx ebp=%08lx esp=%08lx",
            c->Eax, c->Ebx, c->Ecx, c->Edx, c->Esi, c->Edi, c->Ebp, c->Esp);
    {
        HMODULE mod = NULL;
        char name[MAX_PATH];
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                               (LPCSTR)(uintptr_t)c->Eip, &mod)
            && GetModuleFileNameA(mod, name, sizeof(name)))
            PortLog("  in %s +%08lx", name, (unsigned long)(c->Eip - (uintptr_t)mod));
    }
    LogStack(c->Eip, c->Ebp);
    {
        /* release builds keep no frame chain: list the stack words that point
           into the executable's code, so the caller can still be symbolized */
        uintptr_t base = (uintptr_t)GetModuleHandleA(NULL);
        IMAGE_NT_HEADERS* nt = (IMAGE_NT_HEADERS*)(base + ((IMAGE_DOS_HEADER*)base)->e_lfanew);
        uintptr_t lo = base + nt->OptionalHeader.BaseOfCode;
        uintptr_t hi = lo + nt->OptionalHeader.SizeOfCode;
        const uint32_t* sp = (const uint32_t*)(uintptr_t)c->Esp;
        int i, n = 0;
        for (i = 0; i < 512 && n < 24; i++) {
            if (IsBadReadPtr(sp + i, 4))
                break;
            if (sp[i] >= lo && sp[i] < hi) {
                PortLog("  stack %08lx", (unsigned long)sp[i]);
                n++;
            }
        }
    }
    return EXCEPTION_CONTINUE_SEARCH;
}

/* The machine, at start (#360): with the crash lines below, kh_com.log is the one file a report needs. */
static void LogMachine(void) {
    OSVERSIONINFOW v = { sizeof(v) };
    typedef LONG(WINAPI * RtlGetVersionFn)(OSVERSIONINFOW*);
    RtlGetVersionFn get = (RtlGetVersionFn)(void*)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlGetVersion");
    MEMORYSTATUSEX m = { sizeof(m) };
    char cpu[64] = "";
    DWORD size = sizeof(cpu);
    SYSTEM_INFO si;
    if (get) get(&v);
    GlobalMemoryStatusEx(&m);
    GetNativeSystemInfo(&si);
    RegGetValueA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", "ProcessorNameString",
                 RRF_RT_REG_SZ, NULL, cpu, &size);
    PortLog("System: Windows %lu.%lu build %lu %s, CPU %s, %lu cores, RAM %llu MB (free %llu MB), 32-bit game",
            v.dwMajorVersion, v.dwMinorVersion, v.dwBuildNumber,
            si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? "x64" : "x86", cpu,
            (unsigned long)si.dwNumberOfProcessors, m.ullTotalPhys >> 20, m.ullAvailPhys >> 20);
}

void PortCrashHandlerInstall(void) {
    LogMachine();
    SetUnhandledExceptionFilter(CrashHandler);
}

void PortWatchdogStart(void) {
    const char* env = getenv("KHCOM_WATCHDOG");

    if (env == NULL)
        return;
    DuplicateHandle(GetCurrentProcess(), GetCurrentThread(), GetCurrentProcess(), &sGameThread, 0, FALSE,
                    DUPLICATE_SAME_ACCESS);
    CreateThread(NULL, 0, Watch, (LPVOID)env, 0, NULL);
}
