"""Real production Parallel workers must join and propagate exceptions to their owner."""
from pathlib import Path
import os, shutil, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[2]
def main():
 s=(ROOT/'port/decompui/hd_import.inc').read_text(encoding='utf-8')
 begin=s.index('struct ImportRuntime {');end=s.index('\n#include "hd_media_scenes.inc"',begin)
 prefix='''#include <thread>
#include <atomic>
#include <vector>
#include <functional>
#include <filesystem>
#include <mutex>
#include <stdexcept>
#include <exception>
#include <chrono>
#include <string>
namespace fs=std::filesystem;
using HRESULT=int;
#define COINIT_MULTITHREADED 0
#define MF_VERSION 0
#define MFSTARTUP_LITE 0
static int MFStartup(int,int){return 0;}
static void MFShutdown(){}
static const char* T(const char*,const char*en){return en;}
#define SUCCEEDED(x) ((x)>=0)
static std::atomic<int> comCount{0};
static int CoInitializeEx(void*,int){++comCount;return 0;}
static void CoUninitialize(){--comCount;}
struct Job{std::atomic<bool> cancel{false},running{false};std::string status;int slotPart[4]{},partial[4]{};void Done(int,int){} void Say(const std::string&s){status=s;}}sJob;
static thread_local int tSlot,tUnits;
static int Units(const fs::path&){return 1;}
static int MoviePart(const fs::path&){return 0;}
'''
 test='''int main(){std::vector<fs::path> files={"a","b","c"};bool caught=false;
try{Parallel(files,3,[](const fs::path&){throw std::runtime_error("injected");});}
catch(const std::runtime_error&){caught=true;}
if(!caught || !sJob.cancel || comCount!=0)return 4;
sJob.cancel=false;sJob.running=true;
LaunchImport([]{ImportRuntime runtime;throw std::runtime_error("injected");});
auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
while(sJob.running && std::chrono::steady_clock::now()<deadline)std::this_thread::yield();
if(sJob.running || !sJob.cancel || comCount!=0 || sJob.status.find("Import failed:")!=0)return 5;
for(int kind=0;kind<3;kind++){sJob.running=true;sJob.cancel=false;GuardImport([kind]{if(kind==0)throw std::bad_alloc();if(kind==1)throw std::system_error(std::make_error_code(std::errc::permission_denied));throw 7;});if(sJob.running || !sJob.cancel)return 6;}
sJob.cancel=false;sJob.running=true;
LaunchImport([]{},[](const auto&){throw std::system_error(std::make_error_code(std::errc::resource_unavailable_try_again));});
if(sJob.running || !sJob.cancel || sJob.status.find("could not start worker")==std::string::npos)return 7;
sJob.cancel=false;sJob.running=true;GuardImport([]{});return sJob.running || sJob.cancel ? 8:0;}
'''
 with tempfile.TemporaryDirectory(prefix='khcom-workers-') as folder:
  temp=Path(folder);unit=temp/'worker.cpp';unit.write_text(prefix+s[begin:end]+test,encoding="utf-8")
  compiler=os.environ.get('CXX') or shutil.which('clang++') or shutil.which('g++')
  if not compiler:compiler=str(next((ROOT.parent.parent/'toolchain').glob('llvm-mingw-*/bin/clang++.exe')))
  exe=temp/('test.exe' if os.name=='nt' else 'test')
  subprocess.run([compiler,'-std=c++17','-static',str(unit),'-o',str(exe)],check=True)
  run=subprocess.run([str(exe)],capture_output=True,text=True,timeout=5)
  assert run.returncode==0,(run.returncode,run.stderr)
  print('PASS production workers: Parallel join/COM cleanup; real std::thread failure diagnostic/done; memory/system/unknown exceptions; success')
if __name__=='__main__':main()
