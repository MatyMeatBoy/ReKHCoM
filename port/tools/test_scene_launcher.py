"""Compile the launcher's actual scene framing row and verify its setting contract."""
import argparse
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]
def main():
    ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('--cxx',required=True);a=ap.parse_args()
    source=(ROOT/'port/decompui/decompui.cpp').read_text(encoding='utf-8')
    start=source.index('static bool SceneFramingRow() {');end=source.index('\n}\n',start)+3
    function=source[start:end]
    code=r'''
#include <cstdio>
#include <string>
static int value=0,desired=-1;
static const char* T(const char* es,const char*){return es;}
static int& Ini(const char* name){if(std::string(name)!="scene_framing")throw 1;return value;}
static bool CycleRow(const char*,const char* help,int& v,int count,const char*const* names){
    if(!help||count!=3||!names[0]||!names[1]||!names[2])throw 2;
    if(desired<0)return false;v=desired;return true;
}
FUNCTION
int main(){
    if(SceneFramingRow()||value!=0)return 1;
    desired=1;if(!SceneFramingRow()||value!=1)return 2;
    desired=-1;if(SceneFramingRow()||value!=1)return 3;
    desired=2;if(!SceneFramingRow()||value!=2)return 4;
    desired=-1;if(SceneFramingRow()||value!=2)return 5;
    desired=0;if(!SceneFramingRow()||value!=0)return 6;
    puts("PASS production launcher scene selector: default, all three choices, retained value and explanation");
}
'''.replace('FUNCTION',function)
    with tempfile.TemporaryDirectory(prefix='scene-launcher-') as tmp:
        src,exe=Path(tmp)/'test.cpp',Path(tmp)/'test.exe';src.write_text(code,encoding='utf-8')
        subprocess.run([a.cxx,'-std=c++17','-static',str(src),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
if __name__=='__main__':main()
