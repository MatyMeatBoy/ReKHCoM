"""Run the actual wall placement scan with valid and out-of-room cells.

The cartridge reads BIOS at NULL when scanning above row zero. No native
exception handler is installed: portable builds must explicitly model the read.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    source = (ROOT / 'src/map/map.c').read_text(encoding='utf8')
    start = source.index('s32 MapWallFaceIsUnreserved(')
    body = source[start:source.index('\n}', start) + 2]
    code = r'''
#include <stdint.h>
#include <string.h>
#include <windows.h>
typedef int s32;
typedef int16_t s16;
typedef uint16_t u16;
typedef struct {u16 flags; uint8_t type, pad;} MapCell;
static struct {int rows;} room;
static void* gPortBiosArea;
static MapCell cells[8][4];
static int outside, visits;
#define gMapRoomState (&room)
#define BIOS_PTR(p) ((p) != 0 ? (p) : (__typeof__(p))gPortBiosArea)
#define TRUE 1
#define FALSE 0
#define MAP_CELL_FLAG_GMK_RESERVED 0x100
#define MAP_CELL_TYPE_BACK_WALL_FACE 7
#define MAP_CELL_TYPE_RIGHT_WALL_FACE 9
__attribute__((noinline)) static MapCell* MapCellAt(s16 x, s16 y) {
    visits++;
    if (x < 0 || x >= 4 || y < 0 || y >= room.rows) {outside++; return 0;}
    return &cells[y][x];
}
''' + body + r'''
int main(void) {
    uint32_t bios = 0xE3A02004;
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    gPortBiosArea = &bios;
    room.rows=8;
    for(int y=0;y<8;y++)for(int x=0;x<4;x++)cells[y][x].type=7;
    /* A three-column wall reaches y=-1: BIOS type 0xA0 terminates each column. */
    if(!MapWallFaceIsUnreserved(0,2,3) || outside!=3)return 1;
    outside=visits=0;
    cells[1][1].flags=0x100;
    if(MapWallFaceIsUnreserved(0,2,3) || outside!=1)return 2;
    cells[1][1].flags=0;
    for(int x=0;x<4;x++)cells[1][x].type=0;
    outside=visits=0;
    if(!MapWallFaceIsUnreserved(0,2,3) || outside)return 3;
    if(bios!=0xE3A02004)return 4;
    return 0;
}
'''
    cc = next((ROOT.parent / 'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
    with tempfile.TemporaryDirectory(prefix='rekhcom-wall-bios-') as folder:
        src = Path(folder) / 'test.c'; exe = Path(folder) / 'test.exe'
        src.write_text(code, encoding='utf8')
        subprocess.run([str(cc), '-O2', str(src), '-o', str(exe)], check=True)
        result = subprocess.run([str(exe)], timeout=10, creationflags=0x08004000)
        assert result.returncode == 0, f'wall scan failed: {result.returncode:#x}'
    print('PASS: wall boundary follows BIOS reads; reserved and valid cells retain behavior')


if __name__ == '__main__':
    main()
