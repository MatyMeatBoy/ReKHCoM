"""Execute the production Launch consumer with fake CPU/process boundaries.

No GUI, game, ROM or launcher build. Assert selected child, Force64 notice and
unchanged persisted preference. Missing files keep their own diagnostic.
"""
import argparse
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[2]


def function(source,signature):
    start=source.index(signature+' {');brace=source.index('{',start);end=brace+1;depth=1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--work',type=Path,required=True)
    p.add_argument('--toolchain',type=Path,required=True)
    a=p.parse_args();a.work=a.work.resolve();a.work.mkdir(parents=True,exist_ok=True)
    code=r'''#include <windows.h>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <cstdint>
#include <cassert>
#include "port/decompui/runtime_selection.h"
namespace fs=std::filesystem;
struct SDL_Window{};struct SDL_Event{};
static void SDL_ShowWindow(SDL_Window*){}static void SDL_RaiseWindow(SDL_Window*){}
static void SDL_HideWindow(SDL_Window*){}static int SDL_PollEvent(SDL_Event*){return 0;}
struct VersionState{fs::path exe;};static VersionState sState[1];
struct Version{const char*id;};static Version kVersions[]={{"eu"}};
static fs::path sRoot;static int sSelected,sRuntimeMode,persisted;
static bool sLaunch32Once,sRuntime64Entered,sQuitNow;
static uint32_t sRuntime64Problem,testEcx;
static std::string s64FailedVersion,notice;
static std::wstring attempted;
static std::vector<std::wstring> attempts;
#define REKHCOM_VERSION "cpu-test"
#define REKHCOM_EXIT_QUIT_ALL 81
static void SaveIni(){}static void LoadIni(){}
static void SaveLauncherState(){persisted=sRuntimeMode;}
static fs::path RomPath(int){return sRoot/"unused.gba";}
static const char*T(const char*es,const char*){return es;}
static void Notice(const char*text,bool){notice=text;}
static std::wstring Quote(const std::wstring&s){return L"\""+s+L"\"";}
static void FakeSystem(SYSTEM_INFO*s){s->wProcessorArchitecture=PROCESSOR_ARCHITECTURE_AMD64;}
template<typename...Args>static BOOL FakeCreate(LPCWSTR exe,Args...){attempted=exe;attempts.push_back(exe);return FALSE;}
#define GetNativeSystemInfo FakeSystem
#define CreateProcessW FakeCreate
#define PortWindowsCpuLeaf1Ecx() testEcx
'''
    code+=function((ROOT/'port/decompui/decompui.cpp').read_text(encoding='utf-8'),'static void Launch(SDL_Window* window)')
    code+=r'''
int main(int argc,char**argv){
 assert(argc==2);sRoot=fs::u8path(argv[1]);fs::create_directories(sRoot/"bin64");
 sState[0].exe=sRoot/"game32.exe";
 for(const char*n:{"rekhcom64.exe","guest-eu.bin","SDL3.dll"})std::ofstream(sRoot/"bin64"/n)<<"fixture";
 for(bool cpu:{false,true})for(int mode:{0,1,2}){
  sRuntimeMode=mode;testEcx=cpu?(1u<<19):0;attempted.clear();attempts.clear();notice.clear();persisted=-1;
  Launch(nullptr);
  assert(sRuntimeMode==mode && persisted==mode);
  if(mode==2 && !cpu){assert(attempted.empty());assert(notice.find("SSE4.1")!=std::string::npos);}
  else {assert(!attempts.empty());assert(fs::path(attempts.front()).filename()==((cpu && mode!=1)?"rekhcom64.exe":"game32.exe"));if(!cpu)assert(attempts.size()==1);}
 }
 fs::remove(sRoot/"bin64/guest-eu.bin");
 sRuntimeMode=2;testEcx=1u<<19;attempted.clear();notice.clear();Launch(nullptr);
 assert(attempted.empty() && notice.find("bin64")!=std::string::npos && notice.find("SSE4.1")==std::string::npos);
 assert(sRuntimeMode==2 && persisted==2);
}
'''
    source=a.work/'launcher-cpu-test.cpp';source.write_text(code,encoding='utf-8')
    exe=a.work/'launcher-cpu-test.exe'
    subprocess.run([str(a.toolchain/'bin/clang++.exe'),'-std=c++17','-static','-I',str(ROOT),str(source),'-o',str(exe)],check=True)
    subprocess.run([str(exe),str(a.work/'launcher-fixtures')],check=True)
    print('PASS production Launch consumer: CPU false/true x Auto/Force32/Force64; unchanged persisted choice; missing-file diagnostic')


if __name__=='__main__':main()
