"""Exercise the actual PPU selection prefix for default and explicit controls."""
from pathlib import Path
import os,shutil,subprocess,tempfile

root=Path(__file__).resolve().parents[2]
source=(root/'port/src/port_ppu.c').read_text(encoding='utf8')
start=source.index('static int PpuInnerBegin(void) {')
end=source.index('    if (sPpuWorkers < 0)',start)
prefix=source[start:end]+'    return sPpuInnerOn;\n}\n'
fixture='''#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#undef _WIN32
#ifdef TEST_WIN32
#define _WIN32 1
#endif
static int sPpuInnerOn=-1;
static const char* setting;
static const char* SDL_getenv(const char* name){(void)name;return setting;}
#define SDL_atoi atoi
#define PortLog(...) ((void)0)
'''+prefix+'''
int main(void){
assert(PpuInnerBegin()==EXPECT_DEFAULT);
sPpuInnerOn=-1;setting="0";assert(PpuInnerBegin()==0);
sPpuInnerOn=-1;setting="1";assert(PpuInnerBegin()==1);
puts("PASS actual inner-row default and explicit controls");return 0;}
'''
cc=os.environ.get('CC') or shutil.which('cc') or str(root.parent.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-clang.exe')
with tempfile.TemporaryDirectory() as directory:
    d=Path(directory);(d/'default.c').write_text(fixture)
    for name,flags,expected in [('windows32',['-DTEST_WIN32'],1),('windows-reloc',['-DTEST_WIN32','-DPORT_GBA_RELOC'],1),('desktop-x32',['-DPORT_LINUX64_GUEST'],1),('switch',['-DPORT_SWITCH'],1),('other',[],0)]:
        exe=d/(name+'.exe')
        subprocess.run([cc,*flags,f'-DEXPECT_DEFAULT={expected}',str(d/'default.c'),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
