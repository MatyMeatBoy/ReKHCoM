"""Run the real launcher GL bootstrap with context/current failures, without GPU."""
import argparse
from pathlib import Path
import os, shutil, subprocess, tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--source', type=Path, default=ROOT/'port/decompui/decompui.cpp')
    args = ap.parse_args()
    source = args.source.read_text(encoding='utf-8')
    start = source.index('    SDL_GLContext gl = SDL_GL_CreateContext(window);')
    end = source.index('    SDL_GL_SetSwapInterval(1);', start) + len('    SDL_GL_SetSwapInterval(1);')
    body = source[start:end]
    helper = ''
    if 'static int LauncherVideoFailed(' in source:
        a = source.index('static int LauncherVideoFailed(')
        helper = source[a:source.index('\nint main(', a)]
    prefix = r'''
#include <cstdio>
#include <cstring>
struct SDL_Window {};
using SDL_GLContext = void*;
static SDL_Window windowObject;
static void* sRaUi = &windowObject;
static int scenario, invalidCall, swapCalls, destroyGL, destroyWindow, quitCalls, raDestroy, diagnostics;
#define SDL_MESSAGEBOX_ERROR 1
namespace ImGui { static void* GetCurrentContext(){return nullptr;} static void DestroyContext(){} }
static void ImGui_ImplSDL3_Shutdown(){}
static void ImGui_ImplOpenGL3_Shutdown(){}
static const char* SDL_GetError(){return "injected startup failure";}
static void SDL_Log(const char*,...){}
static bool SDL_ShowSimpleMessageBox(int,const char*,const char* message,SDL_Window*){
    if(std::strstr(message,"injected startup failure")) diagnostics++;return true;
}
static SDL_GLContext SDL_GL_CreateContext(SDL_Window*){return scenario==1 ? nullptr : &windowObject;}
static bool SDL_GL_MakeCurrent(SDL_Window*,SDL_GLContext gl){if(!gl)invalidCall++;return scenario!=2;}
#define GL_VERSION 1
static SDL_GLContext SDL_GL_GetCurrentContext(){return &windowObject;}
static const unsigned char* glGetString(int){return (const unsigned char*)"3.0";}
static bool SDL_SetError(const char*){return false;}
static bool SDL_GL_SetSwapInterval(int){swapCalls++;return true;}
static void SDL_GL_DestroyContext(SDL_GLContext gl){if(!gl)invalidCall++;destroyGL++;}
static void SDL_DestroyWindow(SDL_Window*){destroyWindow++;}
static void SDL_Quit(){quitCalls++;}
static void KhRaDestroy(void*){raDestroy++;}
'''
    unit = prefix + helper + '\nstatic int Bootstrap(SDL_Window* window){\n' + body + '\nreturn 0;\n}\n' + r'''
int main(){
    for(scenario=0;scenario<3;scenario++){
        sRaUi=&windowObject;invalidCall=swapCalls=destroyGL=destroyWindow=quitCalls=raDestroy=diagnostics=0;
        int result=Bootstrap(&windowObject);
        bool bad=scenario ? result!=1 || swapCalls!=0 || invalidCall || diagnostics!=1 ||
                            destroyGL!=(scenario==2) || destroyWindow!=1 || quitCalls!=1 || raDestroy!=1 || sRaUi!=nullptr
                          : result!=0 || swapCalls!=1 || invalidCall || diagnostics || destroyGL || destroyWindow || quitCalls || raDestroy;
        if(bad){std::fprintf(stderr,"scenario%d result%d downstream%d invalid%d diagnostics%d cleanup%d/%d/%d/%d\n",scenario,result,swapCalls,invalidCall,diagnostics,destroyGL,destroyWindow,quitCalls,raDestroy);return 97;}
    }
    std::puts("PASS actual launcher GL bootstrap: success, context creation failure, make-current failure; no downstream GL after failure; diagnostic and cleanup once");
}
'''
    cc = os.environ.get('CXX') or shutil.which('clang++') or shutil.which('g++')
    if not cc:
        cc = str(next((ROOT.parents[1]/'toolchain').glob('llvm-mingw-*/bin/clang++.exe')))
    with tempfile.TemporaryDirectory(prefix='launcher-gl-') as temp:
        path=Path(temp); cpp=path/'consumer.cpp';cpp.write_text(unit,encoding='utf-8')
        exe=path/('consumer.exe' if os.name=='nt' else 'consumer')
        subprocess.run([cc,'-std=c++17','-static',str(cpp),'-o',str(exe)],check=True)
        result=subprocess.run([str(exe)],capture_output=True,text=True,timeout=5)
        print(result.stdout,end='')
        assert result.returncode==0,(result.returncode,result.stderr)


if __name__=='__main__':main()
