"""Compile and execute the actual launcher network-settings consumer, without GUI."""
import argparse
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--cxx', required=True)
    args = ap.parse_args()
    source = (ROOT/'port/decompui/decompui.cpp').read_text(encoding='utf-8')
    start = source.index('static void NetlinkRows() {')
    end = source.index('\nstatic void PageExtras()', start)
    function = source[start:end]
    code = r'''
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include "port/decompui/netlink_preset.h"
namespace fs=std::filesystem;
static fs::path sRoot;
static int desired=-1,observed=-1;
static const char* T(const char* es,const char*){return es;}
static int Px(int v){return v;}
static bool CycleRow(const char* label,const char*,int& value,int count,const char*const*){
    if(std::string(label).find("Enlace en red") == 0){
        observed=value;
        if(desired>=0){assert(desired<count);value=desired;return true;}
    }
    return false;
}
static int Ini(const char*){return 0;}
static void SaveIni(){}
static std::map<std::string,int> sIni;
static int sFontSmall,kMuted,kAccent;
static const int ImGuiInputTextFlags_CharsUppercase=1,ImGuiInputTextFlags_CharsNoBlank=2;
namespace ImGui {
static void AlignTextToFramePadding(){}static void SameLine(){}
static void TextUnformatted(const char*){}static void SetNextItemWidth(int){}
static bool InputText(const char*,char*,size_t,int=0){return false;}
static bool InputInt(const char*,int*,int,int){return false;}
static void PushFont(int){}static void PopFont(){}
static void TextColored(int,const char*,const char*){}
}
int main(int argc,char**argv){
    sRoot=argv[1]; desired=std::atoi(argv[3]);
    {std::ofstream out(sRoot/"netlink.cfg");out<<argv[2]<<"\n";}
    NetlinkRows();
    assert(observed==std::atoi(argv[4]));
    std::ifstream in(sRoot/"netlink.cfg");std::string spec;std::getline(in,spec);
    assert(spec==argv[5]);
}
'''
    # Inject the real function before the test's main, not a reimplementation.
    code = code.replace('#include <cassert>', '#include <cassert>\n#include <map>')
    code = code.replace('int main(int argc', function+'\nint main(int argc')
    with tempfile.TemporaryDirectory(prefix='netlink-preset-') as tmp:
        work = Path(tmp)
        cpp, exe = work/'consumer.cpp', work/'consumer.exe'
        cpp.write_text(code, encoding='utf-8')
        subprocess.run([args.cxx, '-static', '-std=c++17', '-I', str(ROOT), str(cpp), '-o', str(exe)], check=True)
        beta = 'lobby:45.8.207.150:47800'
        for before, selection, parsed, after in [
                ('off', 9, 0, beta), (beta, -1, 9, beta),
                (beta, 0, 9, 'off'), ('lobby:custom.example:47801', -1, 8, 'lobby:custom.example:47801'),
                ('steam:host', -1, 1, 'steam:host'), ('lobby:steam', -1, 7, 'lobby:steam'),
                ('relay:custom:47801:ABC:host', -1, 5, 'relay:custom:47801:ABC:host')]:
            subprocess.run([str(exe), str(work), before, str(selection), str(parsed), after], check=True)
        print('PASS: 7 production launcher preset/save/reopen/legacy cases')


if __name__ == '__main__':
    main()
