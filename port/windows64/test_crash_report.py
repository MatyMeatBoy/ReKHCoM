"""Actual Windows exceptions on native and foreign stacks; no SDL/game/ROM needed."""
import argparse
import pathlib
import subprocess
import tempfile
import time
import unittest

HERE = pathlib.Path(__file__).resolve().parent


def reporter_source():
    if ARGS.baseline:
        source = subprocess.check_output(
            ['git', 'show', f'{ARGS.baseline}:port/windows64/windows64_host.c'], cwd=HERE, text=True)
        return source[source.index('static char sMachine['):source.index('static void CrashReportNote(')]
    header = HERE / 'crash_report.h'
    if header.exists():
        return header.read_text()
    source = (HERE / 'windows64_host.c').read_text()
    return source[source.index('static char sMachine['):source.index('static void CrashReportNote(')]


PREAMBLE = r'''
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#define PS5G_BASE 0x10000000u
static char sCrashPath[1100];
static char sData[1024];
static unsigned SDL_GetTicks(void) { return 1; }
static int SDL_GetNumLogicalCPUCores(void) { return 1; }
static unsigned PortWindowsCpuLeaf1Ecx(void) { return 0; }
static int PortWindowsCpuHasSse41(unsigned x) { return 0; }
static int sFailThread;
static int sSuspendThread;
static HANDLE WINAPI ProbeCreateThread(LPSECURITY_ATTRIBUTES a, SIZE_T n, LPTHREAD_START_ROUTINE f,
                                      LPVOID p, DWORD flags, LPDWORD id) {
    return sFailThread ? NULL : CreateThread(a, n, f, p, flags | (sSuspendThread ? CREATE_SUSPENDED : 0), id);
}
#define CreateThread ProbeCreateThread
'''
MAIN = r'''
__attribute__((noreturn)) void FaultForeign(void* top);
__attribute__((noreturn)) void GuestForeign(void* top, void* code);
static void FaultNative(void) { volatile int zero = 0; volatile int numerator = 7; volatile int value = numerator / zero; (void)value; }
int main(int argc, char** argv) {
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    snprintf(sData, sizeof(sData), "%s", argv[1]);
    sFailThread = !strcmp(argv[2], "no-worker");
    sSuspendThread = !strcmp(argv[2], "blocked-worker");
    CrashReportBegin();
    if (!strcmp(argv[2], "foreign") || !strcmp(argv[2], "guest") || sFailThread || sSuspendThread) {
        char* stack = VirtualAlloc((void*)0x1d026000, 4*1024*1024, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
        if (!stack) return 10;
        if (!strcmp(argv[2], "guest")) {
            const unsigned char divide[] = {0xb8,1,0,0,0,0x31,0xd2,0x31,0xc9,0xf7,0xf1,0x0f,0x0b};
            void* code = VirtualAlloc((void*)PS5G_BASE, 4096, MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE);
            if (!code) return 12;
            memcpy(code, divide, sizeof(divide));
            FlushInstructionCache(GetCurrentProcess(), code, sizeof(divide));
            sGuestEnd = PS5G_BASE + 4096;
            GuestForeign(stack + 4*1024*1024 - 256, code);
        }
        FaultForeign(stack + 4*1024*1024 - 256);
    }
    if (!strcmp(argv[2], "repeat")) {
        EXCEPTION_RECORD r = {0}; CONTEXT c = {0}; EXCEPTION_POINTERS ep = {&r, &c};
        r.ExceptionCode = EXCEPTION_INT_DIVIDE_BY_ZERO;
        r.ExceptionAddress = (void*)FaultNative;
        c.Rip = (uintptr_t)FaultNative;
        CrashFilter(&ep);
        r.ExceptionCode = EXCEPTION_ACCESS_VIOLATION;
        CrashFilter(&ep);
        return 0;
    }
    FaultNative();
    return 11;
}
'''
ASM = r'''
.text
.globl FaultForeign
FaultForeign:
    movq %rcx, %rsp
    andq $-16, %rsp
    subq $32, %rsp
    movl $1, %eax
    xorl %edx, %edx
    xorl %ecx, %ecx
    divl %ecx
    ud2
.globl GuestForeign
GuestForeign:
    movq %rcx, %rsp
    andq $-16, %rsp
    subq $32, %rsp
    jmpq *%rdx
'''


class CrashReporterTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory(prefix='rekhcom-crash-report-')
        cls.root = pathlib.Path(cls.tmp.name)
        c = cls.root / 'probe.c'
        c.write_text(PREAMBLE + reporter_source() + MAIN)
        asm = cls.root / 'fault.S'
        asm.write_text(ASM)
        cls.exe = cls.root / 'probe.exe'
        subprocess.run([ARGS.cc, '-O2', str(c), str(asm), '-ladvapi32', '-o', str(cls.exe)], check=True)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def run_case(self, mode, missing=False):
        folder = self.root / self.id().split('.')[-1]
        folder.mkdir()
        if missing:
            folder = folder / ('x' * 200) / ('y' * 200) / 'absent'
        p = subprocess.run([str(self.exe), str(folder), mode], timeout=10, capture_output=True)
        return p.returncode & 0xffffffff, (folder / 'kh_com.log')

    def assert_original(self, mode):
        code, log = self.run_case(mode)
        self.assertEqual(code, 0xc0000094)
        self.assertTrue(log.exists(), 'first exception report missing')
        text = log.read_text()
        self.assertIn('exception c0000094', text)
        self.assertNotIn('exception c00000fd', text)
        self.assertIn('rsp ', text)

    def test_native_fault(self):
        self.assert_original('native')

    def test_foreign_stack_fault(self):
        self.assert_original('foreign')

    def test_guest_address_foreign_stack_fault(self):
        code, log = self.run_case('guest')
        self.assertEqual(code, 0xc0000094)
        text = log.read_text()
        self.assertIn('exception c0000094', text)
        self.assertIn('guest+00000009', text)

    def test_first_exception_only(self):
        code, log = self.run_case('repeat')
        self.assertEqual(code, 0)
        self.assertEqual(log.read_text().count('exception '), 1)
        self.assertIn('exception c0000094', log.read_text())

    def test_unwritable_long_path_preserves_exit(self):
        code, log = self.run_case('foreign', missing=True)
        self.assertEqual(code, 0xc0000094)
        self.assertFalse(log.exists())

    def test_worker_creation_failure_preserves_exit(self):
        code, log = self.run_case('no-worker')
        self.assertEqual(code, 0xc0000094)
        self.assertFalse(log.exists())

    def test_blocked_worker_has_bounded_wait(self):
        started = time.monotonic()
        code, log = self.run_case('blocked-worker')
        elapsed = time.monotonic() - started
        self.assertEqual(code, 0xc0000094)
        self.assertGreater(elapsed, 1.8)
        self.assertLess(elapsed, 6)
        self.assertFalse(log.exists())


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--cc', required=True)
    parser.add_argument('--baseline', help='Extract the reporter from a revision to demonstrate the regression')
    ARGS, remaining = parser.parse_known_args()
    unittest.main(argv=[__file__, *remaining])
