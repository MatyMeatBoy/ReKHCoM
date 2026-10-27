"""Real temporary-file writes with failures below the production SRAM/UI logic.

No ROM, installed save, game build or renderer is used. CC selects a C compiler.
"""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SOURCES = ['port/src/port_memory.c', 'port/android/native/port_memory_android.c',
           'port/ps5/guest/src/port_memory_ps5.c', 'port/web/native/port_memory_web.c']


class SaveBankFlushTests(unittest.TestCase):
    def test_failed_write_cannot_discard_dirty_bank_or_switch_its_ui(self):
        ui = (ROOT / 'port/src/port_ui.c').read_text(encoding='utf-8')
        start = ui.index('        case ITEM_SAVEBANK: {')
        ui = ui[start:ui.index('        case ITEM_UNDUB:', start)]
        ui = 'static void ChangeBank(int dir) { switch(0) {\n' + ui.replace('case ITEM_SAVEBANK:', 'case 0:') + '} }\n'
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            flags = ['-std=c99', '-Wall', '-Wextra', '-Werror']
            if os.environ.get('SAVE_SANITIZE') == '1':
                flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all']
            for index, path in enumerate(SOURCES):
                source = (ROOT / path).read_text(encoding='utf-8')
                source = source[source.index('/* The bank the SRAM was loaded from'):]
                exe = directory / f'bank-{index}.exe'
                code = directory / f'bank-{index}.c'
                code.write_text(PREAMBLE + source + ui + HARNESS, encoding='utf-8')
                subprocess.run([os.environ.get('CC', 'clang'), *flags, str(code), '-o', str(exe)], check=True)
                for action in ['direct', 'ui']:
                    for failure in range(5):
                        with self.subTest(source=path, action=action, failure=failure):
                            work = directory / f'run-{index}-{action}-{failure}'
                            work.mkdir()
                            result = subprocess.run([str(exe), action, str(failure)], cwd=work,
                                                    capture_output=True, text=True, timeout=15)
                            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)


PREAMBLE = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>
#define GBA_SRAM_SIZE 65536
#define MAX_PATH 4096
#define PORT_SAVE_NAME "kh_com.sav"
#define PORT_SAVE_BANKS 5
#define PORT_MODE_CHOOSE 99
#define LANG_COUNT 6
#define MOVEFILE_REPLACE_EXISTING 1
#define SDL_snprintf snprintf
#define EM_ASM(...) ((void)0)
#define EMSCRIPTEN_KEEPALIVE
static unsigned char memory[GBA_SRAM_SIZE], sSramShadow[GBA_SRAM_SIZE];
#define GBA_SRAM ((uintptr_t)memory)
static int sSramLoaded, failure, notices, loads, videos, failedLogs;
static struct {int saveBank, graphicsMode;}gPortConfig={1,7};
static const char* PortConfigPath(const char* name) {return name;}
static int PortSceneGalleryActive(void) {return 0;}
static void PortUiSaved(void) {}
static void PortRebelEditedSave(void) {}
static void PortLog(const char* fmt,...) {if(strstr(fmt,"cannot switch bank"))failedLogs++;}
static FILE* open_file(const char* name,const char* mode) {
    if(failure==1&&strchr(mode,'w')){errno=EACCES;return NULL;}
    return fopen(name,mode);
}
static size_t write_file(const void* ptr,size_t size,size_t count,FILE* f) {
    if(failure==2) return fwrite(ptr,size,count/2,f);
    return fwrite(ptr,size,count,f);
}
static int close_file(FILE* f) {int r=fclose(f);return failure==3?EOF:r;}
static int replace_file(const char* from,const char* to) {
    if(failure==4){errno=EACCES;return -1;}
#ifdef _WIN32
    remove(to); /* The test host's CRT rename cannot replace an existing file. */
#endif
    return rename(from,to);
}
#define fopen open_file
#define fwrite write_file
#define fclose close_file
#define rename replace_file
#define h_rename replace_file
#define MoveFileExA(from,to,flags) (replace_file(from,to)==0)
void PortSramLoad(void);
void PortSramSyncIfDirty(void);
static int Wrap(int n,int max) {return(n%max+max)%max;}
static int PortDebugModeAvailable(void) {return 1;}
static void PortConfigSave(void) {}
static int BankCopy(const char* a,const char*b) {(void)a;(void)b;return 1;}
static void PortConfigLoad(void) {loads++;gPortConfig.graphicsMode=8;}
static void PortVideoApplyConfig(void) {videos++;}
static void PortUiNotice(const char* p) {(void)p;notices++;}
static int Lang(void) {return 0;}
'''

HARNESS = r'''
#undef fopen
#undef fwrite
#undef fclose
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
    int fail=argc>2?atoi(argv[2]):0;
    int ui=argc>1&&!strcmp(argv[1],"ui");
    (void)PortUiSaved;(void)PortRebelEditedSave;(void)PortUiNotice;
    disk_write("kh_com.sav",0x11);disk_write("kh_com_bank2.sav",0x22);
    PortSramLoad();memset(memory,0x33,sizeof(memory));failure=fail;
    if(ui)ChangeBank(1);else{gPortConfig.saveBank=2;
        check(PortSramSwitchBank()==!fail,"switch return value must report persistence failure");}
    if(fail) {
        check(sSramBank==1,"failed flush must not change loaded bank");
        check(memory[0]==0x33&&memory[GBA_SRAM_SIZE-1]==0x33,"dirty SRAM must survive failed switch");
        check(sSramShadow[0]==0x11,"failed write must stay dirty for retry");
        check(failedLogs==1,"failed switch must log the retained pending save");
        disk_check("kh_com.sav",0x11);disk_check("kh_com_bank2.sav",0x22);
        if(ui){check(gPortConfig.saveBank==1,"UI must retain old selected bank");
            check(loads==0&&videos==0&&gPortConfig.graphicsMode==7,"UI must not load target bank settings");
            check(notices==1,"rejected bank switch must report failure");}
        failure=0;PortSramSyncIfDirty();disk_check("kh_com.sav",0x33);
        gPortConfig.saveBank=2;PortSramSwitchBank();
    }
    check(sSramBank==2&&memory[0]==0x22,"successful switch loads target bank");
    check(!memcmp(memory,sSramShadow,sizeof(memory)),"successful load refreshes shadow");
    disk_check("kh_com.sav",0x33);disk_check("kh_com_bank2.sav",0x22);
    if(ui&&!fail)check(gPortConfig.saveBank==2&&loads==1&&videos==1,"successful UI switch remains functional");
    return 0;
}
'''

if __name__ == '__main__':
    unittest.main()
