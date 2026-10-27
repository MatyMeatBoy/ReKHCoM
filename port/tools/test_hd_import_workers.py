"""Real production Parallel workers must join and propagate exceptions to their owner."""
from pathlib import Path
import os, shutil, subprocess, tempfile
ROOT=Path(__file__).resolve().parents[2]
def main():
 s=(ROOT/'port/decompui/hd_import.inc').read_text(encoding='utf-8')
 begin=s.index('struct ImportRuntime {');end=s.index('\n#include "hd_media_scenes.inc"',begin)
 compress=s[s.index('static void Compress('):s.index('static void StartCompress(')]
 producer=s[s.index('static void Run(fs::path source'):s.index('static bool MovieRate(')]
 assert 'sJob.running' not in producer+compress, 'Only the owning guard may publish completion'
 prefix='''#include <thread>
#include <fstream>
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
static std::atomic<bool> holdCleanup{false},inCleanup{false},releaseCleanup{false},failCompression{false};
static int MFStartup(int,int){return 0;}
static void MFShutdown(){if(holdCleanup){inCleanup=true;while(!releaseCleanup)std::this_thread::yield();}}
static const char* T(const char*,const char*en){return en;}
#define SUCCEEDED(x) ((x)>=0)
static std::atomic<int> comCount{0};
static int CoInitializeEx(void*,int){++comCount;return 0;}
static void CoUninitialize(){--comCount;}
struct Latch{std::atomic<bool> value{false},restart{false};std::atomic<int> terminals{0};operator bool()const{return value;}
void operator=(bool v){value=v;if(!v){++terminals;if(restart.exchange(false))value=true;}}};
struct Job{std::atomic<bool> cancel{false};Latch running;std::string status;int slotPart[4]{},partial[4]{};void Done(int,int){} void Plan(int,int){} void Say(const std::string&s){status=s;}}sJob;
static thread_local int tSlot,tUnits;
static int Units(const fs::path&){return 1;}
static int MoviePart(const fs::path&){return 0;}
'''
 stubs='''
static int sUnitShift;
struct Quality{int a,b;};
static void RestoreLeftovers(const fs::path&){}
static int Compression(int q){if(failCompression)throw std::runtime_error("injected producer failure");return q;}
static int Workers(int){return 1;}
static bool AtOrUnder(const fs::path&,int){return false;}
static bool TranscodeBest(const fs::path&,const fs::path&,Quality){return false;}
static void InstalledAdd(const fs::path&,const std::string&,int,uintmax_t){}
'''
 test='''int main(int argc,char**argv){if(argc!=2)return 3;
fs::path data=argv[1];fs::create_directories(data/"hd/movies");fs::create_directories(data/"hd/scenes");
for(bool fail:{false,true}){
 sJob.running=true;sJob.running.terminals=0;sJob.cancel=false;sJob.running.restart=false;
 holdCleanup=true;inCleanup=false;releaseCleanup=false;failCompression=fail;
 std::thread owner;
 LaunchImport([&]{Compress(data,0);},[&](auto callback){owner=std::thread(callback);});
 auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
 while(!inCleanup && std::chrono::steady_clock::now()<deadline)std::this_thread::yield();
 bool early=!inCleanup || !sJob.running || sJob.running.terminals!=0;
 // Simulate the GUI starting the next job immediately at the terminal store.
 sJob.running.restart=true;releaseCleanup=true;owner.join();holdCleanup=false;
 if(early || !sJob.running || sJob.running.terminals!=1 || comCount!=0)return 20;
 if(fail && (!sJob.cancel || sJob.status.find("Import failed:")!=0))return 21;
}
failCompression=false;sJob.running.restart=false;sJob.running=false;sJob.cancel=false;
std::vector<fs::path> files={"a","b","c"};bool caught=false;
try{Parallel(files,3,[](const fs::path&){throw std::runtime_error("injected");});}
catch(const std::runtime_error&){caught=true;}
if(!caught || !sJob.cancel || comCount!=0)return 4;
sJob.cancel=false;sJob.running=true;
LaunchImport([]{ImportRuntime runtime;throw std::runtime_error("injected");});
auto deadline=std::chrono::steady_clock::now()+std::chrono::seconds(2);
while(sJob.running && std::chrono::steady_clock::now()<deadline)std::this_thread::yield();
if(sJob.running || !sJob.cancel || comCount!=0 || sJob.status.find("Import failed:")!=0)return 5;
for(int kind=0;kind<3;kind++){sJob.running.terminals=0;sJob.running=true;sJob.cancel=false;GuardImport([kind]{if(kind==0)throw std::bad_alloc();if(kind==1)throw std::system_error(std::make_error_code(std::errc::permission_denied));throw 7;});if(sJob.running || !sJob.cancel || sJob.running.terminals!=1)return 6;}
sJob.cancel=false;sJob.running=true;sJob.running.terminals=0;sJob.running.restart=true;
LaunchImport([]{},[](const auto&){throw std::system_error(std::make_error_code(std::errc::resource_unavailable_try_again));});
if(!sJob.running || !sJob.cancel || sJob.running.terminals!=1 || sJob.status.find("could not start worker")==std::string::npos)return 7;
sJob.cancel=false;sJob.running.restart=false;sJob.running=true;sJob.running.terminals=0;GuardImport([]{});return sJob.running || sJob.cancel || sJob.running.terminals!=1 ? 8:0;}
'''
 with tempfile.TemporaryDirectory(prefix='khcom-workers-') as folder:
  temp=Path(folder);unit=temp/'worker.cpp';unit.write_text(prefix+s[begin:end]+stubs+compress+test,encoding="utf-8")
  compiler=os.environ.get('CXX') or shutil.which('clang++') or shutil.which('g++')
  if not compiler:compiler=str(next((ROOT.parent.parent/'toolchain').glob('llvm-mingw-*/bin/clang++.exe')))
  exe=temp/('test.exe' if os.name=='nt' else 'test')
  subprocess.run([compiler,'-std=c++17','-static',str(unit),'-o',str(exe)],check=True)
  run=subprocess.run([str(exe),str(temp/"data")],capture_output=True,text=True,timeout=5)
  assert run.returncode==0,(run.returncode,run.stderr)
  print('PASS production workers: Parallel join/COM cleanup; real std::thread failure diagnostic/done; memory/system/unknown exceptions; actual Compress cleanup barrier and single terminal/next-job safety; success')
if __name__=='__main__':main()
