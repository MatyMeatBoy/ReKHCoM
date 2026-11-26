"""KHCOM_SAVE_ASYNC: the production Linux SRAM code with a deferred stand-in for the writer thread.

The thread runs only when the test lets it, so the game can change SRAM in between. Real temporary files; no ROM,
installed save or game build. CC selects a C compiler.
"""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'port/android/native/port_memory_android.c'


class SaveAsyncTests(unittest.TestCase):
    def test_periodic_writer(self):
        source = SOURCE.read_text(encoding='utf-8')
        source = source[source.index('/* The bank the SRAM was loaded from'):]
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            exe, code = directory / 'async.exe', directory / 'async.c'
            code.write_text(PREAMBLE + source + HARNESS, encoding='utf-8')
            subprocess.run([os.environ.get('CC', 'clang'), '-std=c99', '-Wall', '-Wextra', '-Werror',
                            '-Wno-deprecated-declarations', '-DPORT_LINUX', str(code), '-o', str(exe)], check=True)
            for case in ['off', 'on', 'slow', 'fail', 'nothread', 'sync', 'bank', 'gallery', 'exit']:
                with self.subTest(case=case):
                    work = directory / case
                    work.mkdir()
                    result = subprocess.run([str(exe), case], cwd=work, capture_output=True, text=True, timeout=15)
                    self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
                    if case == 'exit':
                        self.assertEqual((work / 'kh_com.sav').read_bytes(), b'\x33' * 65536)


PREAMBLE = r"""
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define GBA_SRAM_SIZE 65536
#define MAX_PATH 4096
#define PORT_SAVE_NAME "kh_com.sav"
#define PORT_SAVE_BANKS 5
#define PORT_ENV_ONCE(out, getter, name) ((out) = getter(name))
static unsigned char memory[GBA_SRAM_SIZE], sSramShadow[GBA_SRAM_SIZE];
#define GBA_SRAM ((uintptr_t)memory)
static int sSramLoaded, notices, gallery, failWrite, noThread, started, waited;
static const char* asyncValue;
static struct {int saveBank;}gPortConfig={1};
static const char* PortConfigPath(const char* name) {return name;}
int PortSceneGalleryActive(void) {return gallery;}
void PortUiSaved(void) {notices++;}
void PortRebelEditedSave(void) {}
static void PortLog(const char* fmt,...) {(void)fmt;}
typedef struct { int value; } SDL_AtomicInt;
typedef struct { int (*run)(void*); int ran; } SDL_Thread;
static SDL_Thread job;
static const char* SDL_getenv(const char* name) {(void)name;return asyncValue;}
static void SDL_SetAtomicInt(SDL_AtomicInt* a,int v) {a->value=v;}
static int SDL_GetAtomicInt(SDL_AtomicInt* a) {return a->value;}
static SDL_Thread* SDL_CreateThread(int (*run)(void*),const char* name,void* data) {
    (void)name;(void)data;if(noThread)return NULL;started++;job.run=run;job.ran=0;return &job;
}
static void Run(void) {if(!job.ran){job.ran=1;job.run(NULL);}}
static void SDL_WaitThread(SDL_Thread* t,int* status) {(void)t;(void)status;waited++;Run();}
static FILE* open_file(const char* name,const char* mode) {
    if(failWrite&&strchr(mode,'w'))return NULL;
    return fopen(name,mode);
}
static int replace_file(const char* from,const char* to) {
#ifdef _WIN32
    remove(to); /* The test host's CRT rename cannot replace an existing file. */
#endif
    return rename(from,to);
}
#define fopen open_file
#define rename replace_file
void PortSramLoad(void);
void PortSramSyncIfDirty(void);
"""

HARNESS = r"""
#undef fopen
static void check(int ok,const char* text) {if(!ok){fprintf(stderr,"%s\n",text);exit(1);}}
static void disk_write(const char* name,int byte) {
    unsigned char b[GBA_SRAM_SIZE];memset(b,byte,sizeof(b));
    FILE*f=fopen(name,"wb");check(f!=NULL,"fixture open");
    check(fwrite(b,1,sizeof(b),f)==sizeof(b),"fixture write");check(!fclose(f),"fixture close");
}
static void disk_check(const char* name,int byte) {
    unsigned char b[GBA_SRAM_SIZE];FILE*f=fopen(name,"rb");int i;
    check(f!=NULL,"saved file remains present");
    check(fread(b,1,sizeof(b),f)==sizeof(b),"saved file remains complete");fclose(f);
    for(i=0;i<GBA_SRAM_SIZE;i++)check(b[i]==byte,"unexpected saved byte");
}
int main(int argc,char**argv) {
    const char* c=argc>1?argv[1]:"";
    disk_write("kh_com.sav",0x11);disk_write("kh_com_bank2.sav",0x22);
    asyncValue=!strcmp(c,"off")?NULL:"1";
    PortSramSyncPeriodic(); /* nothing loaded yet: nothing may be written */
    disk_check("kh_com.sav",0x11);
    PortSramLoad();memset(memory,0x33,sizeof(memory));
    if(!strcmp(c,"off")) {
        PortSramSyncPeriodic();disk_check("kh_com.sav",0x33);
        check(!started&&notices==1,"disabled flag keeps the synchronous write");
    } else if(!strcmp(c,"on")) {
        PortSramSyncPeriodic();disk_check("kh_com.sav",0x11);
        memset(memory,0x44,sizeof(memory)); /* the game goes on while the card is busy */
        Run();disk_check("kh_com.sav",0x33);check(!notices,"notice waits for the game's thread");
        PortSramSyncPeriodic();check(notices==1,"finished save is reported once");
        Run();disk_check("kh_com.sav",0x44);
        PortSramSyncPeriodic();check(notices==2,"second save reported");
        Run();PortSramSyncPeriodic();check(notices==2,"clean SRAM writes nothing");
    } else if(!strcmp(c,"slow")) {
        PortSramSyncPeriodic();PortSramSyncPeriodic();PortSramSyncPeriodic();
        check(started==1&&!waited,"a busy writer is neither joined nor doubled");
    } else if(!strcmp(c,"fail")) {
        failWrite=1;PortSramSyncPeriodic();Run();disk_check("kh_com.sav",0x11);
        check(sSramShadow[0]==0x11&&!notices,"failed write stays dirty");
        failWrite=0;PortSramSyncPeriodic();Run();disk_check("kh_com.sav",0x33);
    } else if(!strcmp(c,"nothread")) {
        noThread=1;PortSramSyncPeriodic();disk_check("kh_com.sav",0x33);check(notices==1,"fallback saves");
    } else if(!strcmp(c,"sync")) {
        PortSramSyncPeriodic();memset(memory,0x44,sizeof(memory));
        PortSramSyncIfDirty(); /* quit: older snapshot first, then the newest SRAM */
        check(waited==1&&notices==2,"immediate sync joins the writer");disk_check("kh_com.sav",0x44);
    } else if(!strcmp(c,"bank")) {
        PortSramSyncPeriodic();gPortConfig.saveBank=2;check(PortSramSwitchBank(),"switch");
        disk_check("kh_com.sav",0x33);disk_check("kh_com_bank2.sav",0x22);
        check(memory[0]==0x22&&sSramShadow[0]==0x22,"new bank loaded after the writer ended");
    } else if(!strcmp(c,"gallery")) {
        gallery=1;PortSramSyncPeriodic();check(!started,"replay SRAM is never written");
        disk_check("kh_com.sav",0x11);
    } else if(!strcmp(c,"exit")) {
        PortSramSyncPeriodic();check(!job.ran,"still pending");exit(0); /* atexit finishes it */
    } else check(0,"unknown case");
    return 0;
}
"""

if __name__ == '__main__':
    unittest.main()
