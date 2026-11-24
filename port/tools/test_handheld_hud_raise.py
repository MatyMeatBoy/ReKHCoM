"""Exercise actual HUD anchor math when 480p shares the world canvas."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
source = (root / 'port/src/port_ppu.c').read_text(encoding='utf8')

def function(name):
    start = source.rfind('\n', 0, source.index(name + '(')) + 1
    opening = source.index('{', start)
    end, depth = opening + 1, 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]

program = r'''
#include <stdio.h>
#include <stdint.h>
typedef struct { int ayq; } Tag;
static Tag sEntryTag[128];
static int sHudDepth=1,sHudAy=3,sHudAx=-1,gPortSpriteShiftY,gPortHudScale,sBattleView=1;
static struct { int cardHudRaise; } gPortConfig={8};
static int fixed=1;
static int PortFixed480p(void){return fixed;}
static int PortWideMarginY(void){return 40;}
static const char* getenv(const char* name){(void)name;return "1";}
#define PORT_ENV_ONCE(var, fn, key) ((var) = fn(key))
#define CHECK(x) do {if(!(x)){fprintf(stderr,"FAIL line %d: %s\n",__LINE__,#x);return 1;}} while(0)
''' + '\n'.join(function(n) for n in ('FollowQ', 'PortSpriteShiftY')) + r'''
int main(void){
 CHECK(PortSpriteShiftY(0,140)==24); /* played card/stock: 40 - 16 */
 CHECK(PortSpriteShiftY(0,24)==-40); /* top stock stays in place */
 sHudAy=1;gPortSpriteShiftY=40;
 CHECK(PortSpriteShiftY(0,140)==24); /* enemy/reload */
 sHudAx=2;CHECK(PortSpriteShiftY(0,24)==40); /* enemy stock stays at top */
 CHECK(PortSpriteShiftY(0,140)==24);sHudAx=-1;
 sHudAy=-1;gPortSpriteShiftY=-40;CHECK(PortSpriteShiftY(0,24)==-40);
 sHudAy=3;gPortHudScale=2;CHECK(PortSpriteShiftY(0,140)==40); /* separate HUD handles raise */
 gPortHudScale=0;fixed=0;CHECK(PortSpriteShiftY(0,140)==40); /* other platforms unchanged */
 fixed=1;sBattleView=0;CHECK(PortSpriteShiftY(0,140)==40);
 puts("PASS handheld HUD anchors");return 0;
}
'''
base = next(p for p in root.parents if (p / 'toolchain').is_dir())
cc = next((base / 'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
with tempfile.TemporaryDirectory(prefix='handheld-hud-') as directory:
    path = Path(directory)
    (path / 'test.c').write_text(program, encoding='utf8')
    subprocess.run([str(cc), '-std=gnu11', str(path / 'test.c'), '-o', str(path / 'test.exe')], check=True)
    subprocess.run([str(path / 'test.exe')], check=True)
