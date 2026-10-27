"""Small native C tests for device recognition and ini precedence; no game build needed."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]

class DeviceProfiles(unittest.TestCase):
    def test_classifier_and_real_config_parser(self):
        config = (ROOT / 'port/src/port_config.c').read_text()
        load = config[config.index('void PortConfigLoad(void) {'):config.index('static void PortConfigLoadFile(void) {')]
        self.assertLess(load.index('PortConfigDeviceDefaults();'), load.index('PortConfigLoadFile();'))
        defaults = config[config.index('static void PortConfigDeviceDefaults(void)'):config.index('\nvoid PortConfigLoad(void)')]
        parser = config[config.index('static void PortConfigLoadFile(void) {'):]
        parser = parser[:parser.index('    fclose(f);') + len('    fclose(f);')] + '\n}\n'
        code = PREAMBLE + defaults + parser + HARNESS
        with tempfile.TemporaryDirectory() as d:
            d = Path(d)
            source = d / 'profiles.c'
            source.write_text(code)
            cc = os.environ.get('CC') or shutil.which('cc') or shutil.which('clang')
            if not cc and Path('C:/msys64/ucrt64/bin/gcc.exe').exists():
                cc = 'C:/msys64/ucrt64/bin/gcc.exe'
            self.assertIsNotNone(cc, 'C compiler required')
            exe = d / 'profiles.exe'
            env = dict(os.environ)
            env['PATH'] = str(Path(cc).parent) + os.pathsep + env.get('PATH', '')
            subprocess.run([cc, '-std=c99', '-Wall', '-Wextra', '-Werror', '-I', str(ROOT / 'port/include'), str(source), '-o', str(exe)], env=env, check=True)
            for case in ['matrix', 'generic', 'deck', 'user', 'default', 'partial', 'unknown']:
                subprocess.run([str(exe), case], cwd=d, env=env, check=True)
            host = (ROOT / 'port/include/port/port_device_profile_host.h').read_text()
            helper = host[host.index('static void PortDeviceReadBiosValue('):host.index('static void PortDeviceReadWindowsBios(')]
            source.write_text(BIOS_PREAMBLE + helper + BIOS_HARNESS)
            subprocess.run([cc, '-std=c99', '-fshort-wchar', '-Wall', '-Wextra', '-Werror', str(source), '-o', str(exe)], env=env, check=True)
            subprocess.run([str(exe)], cwd=d, env=env, check=True)


PREAMBLE = r'''
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "port/port_device_profile.h"
#define PORT_PS5_GUEST 1
#define SDL_strncmp strncmp
#define SDL_strcmp strcmp
#define SDL_atoi atoi
#define SDL_clamp(v,a,b) ((v)<(a)?(a):(v)>(b)?(b):(v))
#define SDL_strlcpy(d,s,n) snprintf(d,n,"%s",s)
static const char* query;
static int logs;
static const char* SDL_getenv(const char* name) { assert(!strcmp(name,"KHCOM_HOST_DEVICE_PROFILE")); return query; }
static void PortLog(const char* fmt, ...) { (void)fmt; logs++; }
static const char* PortConfigPath(const char* file) { return file; }
static char* Trim(char* text) { text[strcspn(text,"\r\n")] = 0; return text; }
static int sUpgradedSettings;
static struct { int fullscreen, padLayout, hdText, discordPresence, assetsSeen, netStats, hdSubtitles, hdMovies, hdMusic; char onlineName[64]; } gPortConfig;
typedef struct { const char* name; int* value; int min,max; } Key;
static const Key sKeys[] = {
    { "fullscreen", &gPortConfig.fullscreen, 0, 1 },
    { "pad_layout", &gPortConfig.padLayout, 0, 2 },
    { "hd_text", &gPortConfig.hdText, 0, 1 }
};
'''
HARNESS = r'''
static void check(PortDevicePlatform p, const char* env, const char* vendor, const char* product, int deck) {
    PortDeviceResult result = PortDeviceClassify(p,env,vendor,product);
    assert((result.profile == PORT_DEVICE_STEAM_DECK) == deck);
}
int main(int argc, char** argv) {
    assert(argc == 2);
    if (!strcmp(argv[1],"matrix")) {
        check(PORT_DEVICE_LINUX,NULL,"Valve","Jupiter",1);
        check(PORT_DEVICE_LINUX,NULL,"Valve","Galileo",1);
        check(PORT_DEVICE_LINUX,"1",NULL,NULL,1);
        check(PORT_DEVICE_WINDOWS,"1",NULL,NULL,1);
        check(PORT_DEVICE_OTHER,"1","Valve","Jupiter",0);
        check(PORT_DEVICE_WINDOWS,NULL,"Valve","Jupiter",1);
        check(PORT_DEVICE_WINDOWS,NULL,"Valve","Galileo",1);
        check(PORT_DEVICE_WINDOWS,NULL,"Other","Jupiter",0);
        check(PORT_DEVICE_LINUX,NULL,"Other","Jupiter",0);
        check(PORT_DEVICE_LINUX,NULL,"Valve","Other",0);
        check(PORT_DEVICE_LINUX,NULL,NULL,NULL,0);
        check(PORT_DEVICE_LINUX,"",NULL,NULL,0);
        check(PORT_DEVICE_LINUX,"true",NULL,NULL,0);
        check(PORT_DEVICE_LINUX,NULL,"valve","Jupiter",0);
        check(PORT_DEVICE_LINUX,NULL,"Valve","Jupiter clone",0);
        check(PORT_DEVICE_LINUX,"01",NULL,NULL,0);
        check(PORT_DEVICE_LINUX,"1 ",NULL,NULL,0);
        check(PORT_DEVICE_LINUX,"0","Valve","Galileo",1);
        return 0;
    }
    remove("kh_com.ini"); remove("kh_com.default.ini");
    query = !strcmp(argv[1],"generic") ? "generic:unrecognized-hardware" : "steam-deck:steam-env";
    if (!strcmp(argv[1],"unknown")) query = NULL;
    if (!strcmp(argv[1],"user") || !strcmp(argv[1],"default") || !strcmp(argv[1],"partial")) {
        FILE* f = fopen(!strcmp(argv[1],"default") ? "kh_com.default.ini" : "kh_com.ini","w");
        assert(f);
        fputs(!strcmp(argv[1],"partial") ? "pad_layout=2\nhd_text=1\n" : "fullscreen=0\npad_layout=1\nhd_text=1\n",f);
        fclose(f);
    }
    PortConfigDeviceDefaults(); PortConfigLoadFile();
    assert(gPortConfig.fullscreen == (!strcmp(argv[1],"deck") || !strcmp(argv[1],"partial")));
    assert(gPortConfig.padLayout == (!strcmp(argv[1],"partial") ? 2 : (!strcmp(argv[1],"user") || !strcmp(argv[1],"default"))));
    assert(gPortConfig.hdText == (!strcmp(argv[1],"partial") || !strcmp(argv[1],"user") || !strcmp(argv[1],"default")));
    gPortConfig.fullscreen = 0;
    PortConfigDeviceDefaults();
    assert(gPortConfig.fullscreen == 0 && logs == 1);
    remove("kh_com.ini"); remove("kh_com.default.ini");
    return 0;
}
'''


BIOS_PREAMBLE = r"""
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
typedef wchar_t WCHAR;
typedef uint32_t DWORD;
typedef int (*PortDeviceRegGetValue)(void*,const WCHAR*,const WCHAR*,DWORD,DWORD*,void*,DWORD*);
#define HKEY_LOCAL_MACHINE NULL
#define RRF_RT_REG_SZ 2
#define RRF_ZEROONFAILURE 0x20000000
#define RRF_SUBKEY_WOW6464KEY 0x10000
#define ERROR_SUCCESS 0
#define REG_SZ 1
static int mode;
static int Provider(void* key,const WCHAR* path,const WCHAR* name,DWORD flags,DWORD* type,void* data,DWORD* bytes) {
    static const WCHAR value[] = L"Valve";
    (void)key; (void)path; (void)name;
    assert(flags == (RRF_RT_REG_SZ | RRF_ZEROONFAILURE | RRF_SUBKEY_WOW6464KEY));
    memcpy(data,value,sizeof(value)); *bytes = sizeof(value); *type = REG_SZ;
    if (mode == 1) return 2; /* absent value/access denied */
    if (mode == 2) *type = 4;
    if (mode == 3) *bytes = 3;
    if (mode == 4) *bytes = 260;
    if (mode == 5) ((WCHAR*)data)[5] = L'x';
    if (mode == 6) ((WCHAR*)data)[1] = 0;
    if (mode == 7) ((WCHAR*)data)[1] = 0x100;
    if (mode == 8) *bytes = 0;
    return ERROR_SUCCESS;
}
"""
BIOS_HARNESS = r"""
int main(void) {
    char out[128];
    PortDeviceReadBiosValue(Provider,L"SystemManufacturer",out,sizeof(out));
    assert(strcmp(out,"Valve") == 0);
    for (mode = 1; mode <= 8; mode++) {
        strcpy(out,"previous");
        PortDeviceReadBiosValue(Provider,L"SystemManufacturer",out,sizeof(out));
        assert(out[0] == 0);
    }
    PortDeviceReadBiosValue(NULL,L"SystemManufacturer",out,sizeof(out));
    assert(out[0] == 0);
    mode = 0;
    PortDeviceReadBiosValue(Provider,L"SystemManufacturer",out,3);
    assert(out[0] == 0);
    return 0;
}
"""

if __name__ == '__main__':
    unittest.main()
