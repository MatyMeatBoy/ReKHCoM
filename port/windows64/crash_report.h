/* Private Windows64 reporter; included after host logging/path helpers. */
static char sMachine[1024];
static uint32_t sCpuLeaf1Ecx;
static uint32_t sGuestEnd;
static void CrashLine(FILE* f, const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(f, fmt, ap);
    va_end(ap);
    fputc('\n', f);
}
static void CrashWhere(FILE* f, const char* what, uint64_t a) {
    HMODULE mod = NULL;
    char name[MAX_PATH] = "";
    if (a >= PS5G_BASE && a < sGuestEnd) {
        CrashLine(f, "%s %016llx  guest+%08llx", what, (unsigned long long)a, (unsigned long long)(a - PS5G_BASE));
    } else if (a >= 0x02000000u && a < 0x10000000u) {
        CrashLine(f, "%s %016llx  GBA memory", what, (unsigned long long)a);
    } else if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                  (LPCSTR)(uintptr_t)a, &mod) && mod && GetModuleFileNameA(mod, name, sizeof(name))) {
        const char* base = strrchr(name, '\\');
        CrashLine(f, "%s %016llx  %s+%llx", what, (unsigned long long)a, base ? base + 1 : name,
                  (unsigned long long)(a - (uintptr_t)mod));
    } else {
        CrashLine(f, "%s %016llx", what, (unsigned long long)a);
    }
}
static void CrashWriteSnapshot(EXCEPTION_POINTERS* ep) {
    FILE* f = sCrashPath[0] ? fopen(sCrashPath, "a") : NULL;
    if (f) {
        const EXCEPTION_RECORD* r = ep->ExceptionRecord;
        const CONTEXT* c = ep->ContextRecord;
        SYSTEMTIME t;
        GetLocalTime(&t);
        CrashLine(f, "== CRASH (64-bit host) %04u-%02u-%02u %02u:%02u:%02u, %.1f s after start", t.wYear, t.wMonth, t.wDay, t.wHour,
                  t.wMinute, t.wSecond, SDL_GetTicks() / 1000.0);
        fputs(sMachine, f);
        CrashLine(f, "exception %08lx", (unsigned long)r->ExceptionCode);
        CrashWhere(f, "at", (uint64_t)(uintptr_t)r->ExceptionAddress);
        if (r->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && r->NumberParameters >= 2)
            CrashLine(f, "%s %016llx", r->ExceptionInformation[0] == 8 ? "executing" : r->ExceptionInformation[0] ? "writing" : "reading",
                      (unsigned long long)r->ExceptionInformation[1]);
        CrashLine(f, "rax %016llx rbx %016llx rcx %016llx rdx %016llx", c->Rax, c->Rbx, c->Rcx, c->Rdx);
        CrashLine(f, "rsi %016llx rdi %016llx rbp %016llx rsp %016llx", c->Rsi, c->Rdi, c->Rbp, c->Rsp);
        CrashLine(f, "r8  %016llx r9  %016llx r10 %016llx r11 %016llx", c->R8, c->R9, c->R10, c->R11);
        CrashLine(f, "r12 %016llx r13 %016llx r14 %016llx r15 %016llx", c->R12, c->R13, c->R14, c->R15);
        {
            /* code addresses on the stack (no unwind data for the guest): guest offsets and modules only */
            const uint64_t* sp = (const uint64_t*)(uintptr_t)c->Rsp;
            MEMORY_BASIC_INFORMATION mi;
            int found = 0;
            if (VirtualQuery(sp, &mi, sizeof(mi)) && mi.State == MEM_COMMIT && !(mi.Protect & (PAGE_NOACCESS | PAGE_GUARD))) {
                uintptr_t end = (uintptr_t)mi.BaseAddress + mi.RegionSize;
                for (uintptr_t at = (uintptr_t)sp, i = 0;
                     at <= end && end - at >= sizeof(uint64_t) && i < 2048 && found < 24;
                     at += sizeof(uint64_t), i++) {
                    uint64_t v;
                    /* A damaged/unaligned stack pointer may leave fewer than
                     * eight readable bytes before the next protected page. */
                    memcpy(&v, (const void*)at, sizeof(v));
                    HMODULE mod = NULL;
                    if ((v >= PS5G_BASE && v < sGuestEnd) ||
                        (v > 0x10000000u && GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                                               (LPCSTR)(uintptr_t)v, &mod) && mod)) {
                        CrashWhere(f, "stack", v);
                        found++;
                    }
                }
            }
        }
        fclose(f);
    }
}

/* All CRT, SDL and loader work runs on this native OS thread, never on the
 * x32 guest's foreign stack. Keep the faulting thread alive while reading its
 * stack; a bounded wait ensures disk/loader failure cannot hang termination. */
static HANDLE sCrashReady, sCrashDone;
static LONG sCrashCaptured;
static EXCEPTION_RECORD sCrashRecord;
static CONTEXT sCrashContext;
static uintptr_t sCrashHostBegin, sCrashHostEnd, sCrashSdlBegin, sCrashSdlEnd;
static DWORD WINAPI CrashWorker(void* unused) {
    EXCEPTION_POINTERS ep = { &sCrashRecord, &sCrashContext };
    (void)unused;
    if (WaitForSingleObject(sCrashReady, INFINITE) == WAIT_OBJECT_0) {
        CrashWriteSnapshot(&ep);
        SetEvent(sCrashDone);
    }
    return 0;
}
static void CrashCopy(volatile unsigned char* dst, const volatile unsigned char* src, size_t size) {
    while (size--) *dst++ = *src++;
}
static LONG WINAPI CrashFilter(EXCEPTION_POINTERS* ep) {
    if (sCrashReady && !InterlockedCompareExchange(&sCrashCaptured, 1, 0)) {
        CrashCopy((volatile unsigned char*)&sCrashRecord, (const volatile unsigned char*)ep->ExceptionRecord,
                  sizeof(sCrashRecord));
        CrashCopy((volatile unsigned char*)&sCrashContext, (const volatile unsigned char*)ep->ContextRecord,
                  sizeof(sCrashContext));
        sCrashRecord.ExceptionRecord = NULL; /* no borrowed exception-chain pointers */
        SetEvent(sCrashReady);
        WaitForSingleObject(sCrashDone, 2000);
    }
    return EXCEPTION_CONTINUE_SEARCH; /* never resume the original fault */
}
static LONG WINAPI CrashVectored(EXCEPTION_POINTERS* ep) {
    uintptr_t at = (uintptr_t)ep->ExceptionRecord->ExceptionAddress;
    if ((ep->ExceptionRecord->ExceptionCode & 0xF0000000u) == 0xC0000000u &&
        ((at >= PS5G_BASE && at < sGuestEnd) ||
         (at >= sCrashHostBegin && at < sCrashHostEnd) ||
         (sCrashSdlBegin && at >= sCrashSdlBegin && at < sCrashSdlEnd)))
        CrashFilter(ep);
    return EXCEPTION_CONTINUE_SEARCH;
}
/* Called only at normal start-up. No loader lookup is allowed in the handler. */
static uintptr_t CrashModuleEnd(HMODULE module) {
    const IMAGE_DOS_HEADER* dos = (const IMAGE_DOS_HEADER*)module;
    if (!module) return 0;
    const IMAGE_NT_HEADERS* nt = (const IMAGE_NT_HEADERS*)((const char*)module + dos->e_lfanew);
    return (uintptr_t)module + nt->OptionalHeader.SizeOfImage;
}
static void CrashWorkerBegin(void) {
    HANDLE thread;
    sCrashHostBegin = (uintptr_t)GetModuleHandleA(NULL);
    sCrashHostEnd = CrashModuleEnd((HMODULE)sCrashHostBegin);
    sCrashSdlBegin = (uintptr_t)GetModuleHandleA("SDL3.dll");
    sCrashSdlEnd = CrashModuleEnd((HMODULE)sCrashSdlBegin);
    sCrashDone = CreateEventW(NULL, TRUE, FALSE, NULL);
    sCrashReady = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (sCrashDone && sCrashReady && (thread = CreateThread(NULL, 0, CrashWorker, NULL, 0, NULL)) != NULL) {
        CloseHandle(thread);
        return;
    }
    if (sCrashReady) CloseHandle(sCrashReady);
    if (sCrashDone) CloseHandle(sCrashDone);
    sCrashReady = sCrashDone = NULL; /* preserve original exception even without reporting */
}
static void CrashReportBegin(void) {
    OSVERSIONINFOW v = { sizeof(v) };
    typedef LONG(WINAPI * RtlGetVersionFn)(OSVERSIONINFOW*);
    RtlGetVersionFn get = (RtlGetVersionFn)(void*)GetProcAddress(GetModuleHandleA("ntdll.dll"), "RtlGetVersion");
    MEMORYSTATUSEX m = { sizeof(m) };
    char cpu[64] = "";
    DWORD size = sizeof(cpu);
    snprintf(sCrashPath, sizeof(sCrashPath), "%s/kh_com.log", sData);
    if (get) get(&v);
    GlobalMemoryStatusEx(&m);
    RegGetValueA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System\\CentralProcessor\\0", "ProcessorNameString",
                 RRF_RT_REG_SZ, NULL, cpu, &size);
    sCpuLeaf1Ecx = PortWindowsCpuLeaf1Ecx();
    snprintf(sMachine, sizeof(sMachine),
             "host built " __DATE__ ", Windows %lu.%lu build %lu\nCPU %s, %d logical cores, SSE4.1 %s, AVX %s\n"
             "RAM %llu MB, free %llu MB\nfolder %s\n",
             v.dwMajorVersion, v.dwMinorVersion, v.dwBuildNumber, cpu, SDL_GetNumLogicalCPUCores(),
             PortWindowsCpuHasSse41(sCpuLeaf1Ecx) ? "yes" : "no", (sCpuLeaf1Ecx >> 28) & 1 ? "yes" : "no", m.ullTotalPhys >> 20,
             m.ullAvailPhys >> 20, sData);
    CrashWorkerBegin();
    SetUnhandledExceptionFilter(CrashFilter);
    AddVectoredExceptionHandler(0, CrashVectored);
}
