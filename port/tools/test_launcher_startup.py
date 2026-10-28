"""Fault-inject the actual launcher startup blocks without a GPU or Android device."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]

def section(source, start, end):
    a = source.index(start)
    return source[a:source.index(end, a)]

def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--source', type=Path, default=ROOT/'port/decompui/decompui.cpp')
    args = ap.parse_args()
    source = args.source.read_text(encoding='utf-8')
    helper = section(source, 'static int LauncherVideoFailed(', '\nint main(')
    init = section(source, '    if (!SDL_Init(SDL_INIT_VIDEO', '#ifdef LAUNCHER_ANDROID')
    window = section(source, '    SDL_Window* window = SDL_CreateWindow(', '    SDL_SetWindowMinimumSize(')
    gl = section(source, '    SDL_GLContext gl = SDL_GL_CreateContext(window);', '    LoadBoxArt();')
    prefix = r''' 
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <string>
struct SDL_Window {};
using SDL_GLContext = void*;
static SDL_Window windowObject;
static void* sRaUi;
static int scenario, invalidCall, calls[10], destroyed[6], popupCalls;
static bool context, platform, renderer, popupFail;
static std::string error, logged, popup;
#define SDL_INIT_VIDEO 1
#define SDL_INIT_GAMEPAD 2
#define SDL_WINDOW_OPENGL 1
#define SDL_WINDOW_RESIZABLE 2
#define SDL_WINDOW_HIDDEN 4
#define SDL_MESSAGEBOX_ERROR 1
#define IMGUI_CHECKVERSION() ((void)0)
#define ImGuiConfigFlags_NavEnableKeyboard 1
#define ImGuiConfigFlags_NavEnableGamepad 2
struct ImGuiIO { const char* IniFilename; int ConfigFlags; };
static ImGuiIO io;
namespace ImGui {
void CreateContext(){context=true;}
void* GetCurrentContext(){return context ? &windowObject : nullptr;}
ImGuiIO& GetIO(){return io;}
void DestroyContext(){if(!context||platform||renderer)invalidCall++;context=false;destroyed[0]++;error="context cleanup clobber";}
}
static const char* SDL_GetError(){return error.c_str();}
static const char* SDL_GetAndroidInternalStoragePath(){error="storage lookup clobber";return ".";}
static void SDL_ClearError(){error.clear();}
static void SDL_Log(const char* fmt,...){char b[2048];va_list ap;va_start(ap,fmt);vsnprintf(b,sizeof(b),fmt,ap);va_end(ap);logged+=b;error="log clobber";}
static bool SDL_ShowSimpleMessageBox(int,const char*,const char* message,SDL_Window*){popup=message;popupCalls++;error="popup unavailable";return !popupFail;}
static bool SDL_SetError(const char* message){error=message;return false;}
static bool pass(int n){calls[n]++;if(scenario==n){error=n>=8 ? "" : "injected original failure";return false;}return true;}
static bool SDL_Init(int){return pass(1);}
static SDL_Window* SDL_CreateWindow(const char*,int,int,int){return pass(2)? &windowObject:nullptr;}
static SDL_GLContext SDL_GL_CreateContext(SDL_Window*){return pass(3)? &windowObject:nullptr;}
static bool SDL_GL_MakeCurrent(SDL_Window*,SDL_GLContext gl){if(!gl)invalidCall++;return pass(4);}
static SDL_GLContext SDL_GL_GetCurrentContext(){return pass(5)? &windowObject:nullptr;}
#define GL_VERSION 1
static const unsigned char* glGetString(int){if(!pass(6))return nullptr;return (const unsigned char*)(scenario==7?"":"OpenGL ES 3.0");}
static bool SDL_GL_SetSwapInterval(int){return true;}
static void SDL_GL_DestroyContext(SDL_GLContext gl){if(!gl||context)invalidCall++;destroyed[1]++;error="GL cleanup clobber";}
static void SDL_DestroyWindow(SDL_Window* w){if(!w)invalidCall++;destroyed[2]++;error="window cleanup clobber";}
static void SDL_Quit(){destroyed[3]++;error="quit clobber";}
static void KhRaDestroy(void* p){if(!p)invalidCall++;destroyed[4]++;}
static void ApplyStyle(){}
static void LoadFonts(){}
static bool ImGui_ImplSDL3_InitForOpenGL(SDL_Window*,SDL_GLContext){platform=pass(8);return platform;}
static bool ImGui_ImplOpenGL3_Init(const char*){if(!platform)invalidCall++;renderer=pass(9);return renderer;}
static void ImGui_ImplSDL3_Shutdown(){if(!platform||renderer)invalidCall++;platform=false;destroyed[5]++;error="backend cleanup clobber";}
static void ImGui_ImplOpenGL3_Shutdown(){if(!renderer)invalidCall++;renderer=false;}
'''
    unit = prefix + helper + '\nstatic int Bootstrap(){\n' + init + r'''
    sRaUi=&windowObject;
    int windowW=1180, windowH=760;
    const char* shotDir=nullptr;
''' + window + gl + '\nreturn 0;\n}\n' + r'''
int main(){
 int failures=0;
 const char* stages[]={"","SDL initialization","window creation","OpenGL context creation","OpenGL context activation","OpenGL current context","OpenGL version query","OpenGL version query","ImGui SDL backend initialization","ImGui OpenGL backend initialization"};
 for(int mode=0;mode<2;mode++)for(scenario=0;scenario<=9;scenario++){
  std::remove("launcher-startup-error.log");std::remove("launcher-startup-error.log.pending");
  invalidCall=popupCalls=0;context=platform=renderer=false;sRaUi=nullptr;popupFail=mode;
  for(int& n:calls)n=0;for(int& n:destroyed)n=0;error="stale unrelated error";logged.clear();popup.clear();
  int result=Bootstrap();
  bool bad=invalidCall || result!=(scenario?1:0);
  if(scenario){
   bad|=popupCalls!=1 || popup.find(stages[scenario])==std::string::npos || logged.find(stages[scenario])==std::string::npos;
   if(scenario<=4)bad|=popup.find("injected original failure")==std::string::npos || logged.find("injected original failure")==std::string::npos;
   else if(scenario>=8)bad|=popup.find("no SDL error detail")==std::string::npos || popup.find("stale unrelated error")!=std::string::npos;
   bad|=destroyed[0]!=(scenario>=8) || destroyed[1]!=(scenario>=4) || destroyed[2]!=(scenario>=3) || destroyed[3]!=1 || destroyed[4]!=(scenario>=2) || destroyed[5]!=(scenario==9);
   bad|=sRaUi!=nullptr || context || platform || renderer;
   for(int n=scenario+1;n<=9;n++)bad|=calls[n]!=0;
#ifdef LAUNCHER_ANDROID
   char saved[2048]={0};FILE* file=std::fopen("launcher-startup-error.log","rb");
   if(file){std::fread(saved,1,sizeof(saved)-1,file);std::fclose(file);}
   bad|=!file || std::string(saved).find(stages[scenario])==std::string::npos;
   if(scenario<=4)bad|=std::string(saved).find("injected original failure")==std::string::npos;
#endif
  }else{bad|=popupCalls!=0 || !platform || !renderer;for(int n:destroyed)bad|=n!=0;}
  if(bad){std::fprintf(stderr,"scenario%d popupFail%d result%d invalid%d popup:%s cleanup:%d/%d/%d/%d/%d/%d\n",scenario,mode,result,invalidCall,popup.c_str(),destroyed[0],destroyed[1],destroyed[2],destroyed[3],destroyed[4],destroyed[5]);failures++;}
 }
 if(failures)return 97;
 std::puts("PASS 20 actual-source startup cases: nine failure boundaries + success, popup available/unavailable, original error retained, partial cleanup once, no downstream init");
}
'''
    cc = os.environ.get('CXX') or shutil.which('clang++') or shutil.which('g++')
    if not cc:
        cc = str(next((ROOT.parents[1]/'toolchain').glob('llvm-mingw-*/bin/clang++.exe')))
    with tempfile.TemporaryDirectory(prefix='launcher-startup-') as temp:
        path=Path(temp); cpp=path/'consumer.cpp';cpp.write_text(unit,encoding='utf-8')
        exe=path/('consumer.exe' if os.name=='nt' else 'consumer')
        for android in (False, True):
            subprocess.run([cc,'-std=c++17','-static',*(['-DLAUNCHER_ANDROID'] if android else []),str(cpp),'-o',str(exe)],check=True)
            result=subprocess.run([str(exe)],cwd=path,capture_output=True,text=True,timeout=5)
            print(('Android GLES' if android else 'desktop GL')+': '+result.stdout,end='')
            assert result.returncode==0,(result.returncode,result.stderr)

if __name__=='__main__':main()
