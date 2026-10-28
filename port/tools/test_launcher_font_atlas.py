"""Actual release-mode ImGui consumer: unchecked baseline versus atlas recovery."""
from pathlib import Path
import argparse, hashlib, json, os, shutil, subprocess

ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser();parser.add_argument('--work',type=Path,required=True)
parser.add_argument('--target',choices=['i686-w64-windows-gnu','x86_64-w64-windows-gnu'])
parser.add_argument('--font',type=Path,help='Optional known valid custom font control')
args=parser.parse_args();out=args.work.resolve();out.mkdir(parents=True,exist_ok=True)
source=(ROOT/'port/decompui/decompui.cpp').read_text(encoding='utf-8')
start=source.find('static bool FinalizeFonts()')
helper=''
if start>=0:
    end=source.index('{',start)+1;depth=1
    while depth:
        depth += (source[end]=='{')-(source[end]=='}');end+=1
    helper=source[start:end]
unit=r'''
#include "imgui.h"
#include "imgui_internal.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <cmath>
#ifdef _WIN32
#include <windows.h>
#endif
static ImFont *sFontBody,*sFontSmall,*sFontTitle,*sFontMenu,*sFontMenuSmall;
static float Px(float size){return size;}
static std::string fontError;
static bool SDL_SetError(const char* error){fontError=error;return false;}
static int rejectedBuilds;
static bool RejectBuild(ImFontAtlas*){rejectedBuilds++;return false;}
HELPER
int main(int argc,char**argv){
#ifdef _WIN32
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
#endif
 int scenario=argc>1?std::atoi(argv[1]):0;
 ImGui::CreateContext();auto&io=ImGui::GetIO();io.IniFilename=nullptr;
 io.DisplaySize=ImVec2(640,360);io.DeltaTime=1.0f/60;
 ImFontConfig normal;normal.SizePixels=18;
 ImFont* original=scenario==7?io.Fonts->AddFontFromFileTTF(argv[2],18):io.Fonts->AddFontDefault(&normal);
 if(!original)return 50;
 sFontBody=sFontSmall=sFontTitle=sFontMenu=sFontMenuSmall=original;
 unsigned char bad[256]={0,1,0,0};
 if(scenario==1){ImFontConfig merge;merge.FontDataOwnedByAtlas=false;merge.MergeMode=true;io.Fonts->AddFontFromMemoryTTF(bad,sizeof(bad),16,&merge);}
 if(scenario==2){io.Fonts->Clear();sFontBody=sFontSmall=sFontTitle=sFontMenu=sFontMenuSmall=nullptr;}
 if(scenario==3)sFontSmall=nullptr;
 ImFont stale;if(scenario==4)sFontMenu=&stale;
 ImFontBuilderIO reject={};reject.FontBuilder_Build=RejectBuild;
 if(scenario==5)io.Fonts->FontBuilderIO=&reject;
 if(scenario==6)io.FontDefault=&stale;
 bool ready=GUARD;
 std::printf("scenario=%d ready=%d rejected=%d\n",scenario,ready,rejectedBuilds);std::fflush(stdout);
 if(scenario==5){if(ready||rejectedBuilds!=2||fontError!="Neither configured nor incorporated launcher font atlas could be built")return 51;ImGui::DestroyContext();return 0;}
 if(!ready)return 52;
 unsigned char*pixels=nullptr;int width=0,height=0;io.Fonts->GetTexDataAsRGBA32(&pixels,&width,&height);
 // This is the real release-mode fault in the old consumer: NewFrame dereferences
 // ContainerAtlas after a failed Build. Do not replace it with a mock assertion.
 ImGui::NewFrame();ImGui::TextUnformatted("Font consumer ready");ImGui::EndFrame();
 if(!pixels||width<=0||height<=0||!io.Fonts->IsBuilt())return 53;
 ImFont*roles[]={sFontBody,sFontSmall,sFontTitle,sFontMenu,sFontMenuSmall};
 for(ImFont*f:roles){bool member=false;for(ImFont*inside:io.Fonts->Fonts)member|=inside==f;
  if(!member||!f->IsLoaded()||f->ContainerAtlas!=io.Fonts||!(f->FontSize>0))return 54;}
 if((scenario==0||scenario==7)&&(sFontBody!=original||std::fabs(sFontBody->FontSize-18)>0.001f))return 55;
 unsigned hash=2166136261u;for(int i=0;i<width*height*4;i++)hash=(hash^pixels[i])*16777619u;
 std::printf("font_pixels=%08x size=%.1f configured_font_preserved=%d\n",hash,sFontBody->FontSize,(scenario==0||scenario==7)&&sFontBody==original);
 ImGui::DestroyContext();return 0;
}
'''.replace('HELPER',helper).replace('GUARD','FinalizeFonts()' if helper else 'true')
cpp=out/'consumer.cpp';cpp.write_text(unit,encoding='utf-8')
imgui=ROOT/'port/decompui/third_party/imgui'
compiler=shutil.which('clang++') or shutil.which('g++')
if not compiler:
    tools=next(p/'toolchain' for p in ROOT.parents if (p/'toolchain').is_dir())
    compiler=str(next(tools.glob('llvm-mingw-*/bin/clang++.exe')))
exe=out/('consumer.exe' if os.name=='nt' else 'consumer')
flags=['-std=c++17','-O2','-DNDEBUG',*(['--target='+args.target] if args.target else []),'-I'+str(imgui)]
sources=[imgui/name for name in ['imgui.cpp','imgui_draw.cpp','imgui_widgets.cpp','imgui_tables.cpp']]
identity=subprocess.check_output([compiler,'--version'])+str(flags).encode()
for path in sorted(imgui.glob('*.h'))+sources:
    identity+=path.name.encode()+hashlib.sha256(path.read_bytes()).digest()
cache=out.parent/('imgui-cache-'+hashlib.sha256(identity).hexdigest()[:20]);cache.mkdir(exist_ok=True)
objects=[]
for path in sources:
    obj=cache/(path.stem+'.o');record=obj.with_suffix('.sha256')
    if not obj.exists() or not record.exists() or hashlib.sha256(obj.read_bytes()).hexdigest()!=record.read_text():
        pending=obj.with_suffix('.pending.o')
        subprocess.run([compiler,*flags,'-c',str(path),'-o',str(pending)],check=True)
        pending.replace(obj);record.write_text(hashlib.sha256(obj.read_bytes()).hexdigest())
    objects.append(str(obj))
command=[compiler,*flags,'-static',str(cpp),*objects,'-o',str(exe)]
subprocess.run(command,check=True)
rows=[]
for scenario in range(8 if args.font else 7):
    run=subprocess.run([str(exe),str(scenario),*([str(args.font.resolve())] if args.font else [])],capture_output=True,text=True,timeout=5,cwd=out)
    rows.append(dict(scenario=scenario,exit=run.returncode,output=run.stdout,stderr=run.stderr))
    print(json.dumps(rows[-1]),flush=True)
result=dict(guard_present=bool(helper),cases=rows,result='PASS' if all(r['exit']==0 for r in rows) else 'FAIL',
            scope='Actual ImGui1.91.9b NewFrame with NDEBUG; font failures/roles bounded, no device/GPU claim')
(out/'RESULTS.json').write_text(json.dumps(result,indent=2),encoding='utf-8')
if result['result']!='PASS':raise SystemExit(1)
if source.index('if (!FinalizeFonts())')>source.index('ImGui_ImplSDL3_InitForOpenGL(window, gl)'):
    raise RuntimeError('Atlas guard must precede backend startup and both frame loops')
