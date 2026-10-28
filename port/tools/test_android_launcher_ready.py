"""Actual native status parser, completion consumer and region/Play helpers.

Pure C++ probe with a JNI string adapter; optional --files is an owned Java
importer's output. This is not an Android Activity/device test.
"""
from pathlib import Path
import argparse,subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
TOOLS=next(p/'toolchain' for p in ROOT.parents if (p/'toolchain').is_dir())
main=(ROOT/'port/decompui/decompui.cpp').read_text(encoding='utf-8')
android=(ROOT/'port/decompui/launcher_android.inc').read_text(encoding='utf-8')
def function(text,sig):
 a=text.index(sig)
 while ';' in text[a:text.index('{',a)]:a=text.index(sig,a+1)
 i=text.index('{',a)+1;depth=1
 while depth:depth+=(text[i]=='{')-(text[i]=='}');i+=1
 return text[a:i]
versions=main[main.index('struct Version {'):main.index('static void KeepSelection();')]
status=android[android.index('struct AndroidStatus {'):android.index('static AndroidStatus AndroidStatusNow()')]
poll_at=main.index('            static int seen = -1;',main.index('AndroidStatus st')-500)
poll_begin=main.rfind('        {',0,poll_at)
i=poll_begin+main[poll_begin:].index('{')+1;depth=1
while depth:depth+=(main[i]=='{')-(main[i]=='}');i+=1
poll=main[poll_begin:i]
code=r'''
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>
namespace fs=std::filesystem;
static std::string wire;
using jobject=void*;using jclass=void*;using jmethodID=int;using jstring=std::string*;
struct JNIEnv {
 jmethodID GetMethodID(jclass,const char*,const char*){return 1;}
 jstring CallObjectMethod(jobject,jmethodID){return &wire;}
 const char*GetStringUTFChars(jstring s,void*){return s->c_str();}
 void ReleaseStringUTFChars(jstring,const char*){}
 void DeleteLocalRef(jstring){}
};
template<class F>static bool AndroidActivityCall(F call){JNIEnv env;call(&env,nullptr,nullptr);return true;}
static const void*PortEmbeddedAssetData(const char*,size_t*){return nullptr;}
static int saves,scans,notices;static bool noticeError;static std::string noticeText;
static void SaveLauncherState(){saves++;}
static void LoadIni(){}
static void Notice(const std::string&t,bool e){notices++;noticeText=t;noticeError=e;}
namespace hd{static struct{bool running;std::string text;}sJob;}
'''+versions+function(main,'static std::string Sha1Hex(')+function(main,'static bool ReadFile(')+function(main,'static fs::path RomPath(')
code+=function(main,'static void ScanInstall(').replace('static void ScanInstall(', 'static void RealScanInstall(',1)
code+='\nstatic void ScanInstall(){scans++;RealScanInstall();}\n'
code+=function(main,'static bool GameReady(')+function(main,'static void KeepSelection(')+status+function(android,'static AndroidStatus AndroidStatusNow(')
code+='\nstatic void Poll()'+poll+r'''
int main(int argc,char**argv){
 for(unsigned mask=0;mask<8;mask++)for(int selected=0;selected<3;selected++){
  for(int v=0;v<3;v++)sState[v].installed=(mask>>v)&1;
  sSelected=selected;KeepSelection();
  int expected=selected;if(!(mask&(1u<<selected))&&mask){expected=0;while(!(mask&(1u<<expected)))expected++;}
  assert(sSelected==expected&&GameReady(sSelected)==bool(mask));
 }
 sRoot=argv[1];for(auto&s:sState)s.installed=false;sSelected=1;
 wire="0|0|0|";Poll();assert(!scans&&!GameReady(sSelected));
 wire="1|0|0|Copying";Poll();assert(hd::sJob.running&&!scans&&!GameReady(sSelected));
 wire="0|1|0|ROM installed|verified";Poll();assert(scans==1&&!hd::sJob.running&&noticeText=="ROM installed|verified");
 if(argc>2){assert(GameReady(sSelected)&&sSelected==0&&sState[0].installed);}
 else assert(!GameReady(sSelected));
 Poll();assert(scans==1); /* unchanged result serial must not rehash every frame */
 wire="0|2|1|Import rejected";Poll();assert(scans==2&&noticeError&&noticeText=="Import rejected");
 if(argc>2)assert(GameReady(sSelected));
 puts("PASS actual Android status/completion/ScanInstall/KeepSelection/GameReady;24 region masks; no device/JNI ABI claim");
}
'''
def run():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--files',type=Path);a=p.parse_args()
 with tempfile.TemporaryDirectory(prefix='android-launcher-ready-') as t:
  out=Path(t);src=out/'test.cpp';exe=out/'test.exe';src.write_text(code,encoding='utf-8')
  cc=next((TOOLS).glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang++.exe'))
  subprocess.run([str(cc),'-std=c++17','-O2','-static','-DLAUNCHER_ANDROID',str(src),'-o',str(exe)],check=True)
  files=a.files.resolve() if a.files else out/'files';files.mkdir(exist_ok=True)
  subprocess.run([str(exe),str(files),*(['owned'] if a.files else [])],check=True,timeout=20)
if __name__=='__main__':run()
