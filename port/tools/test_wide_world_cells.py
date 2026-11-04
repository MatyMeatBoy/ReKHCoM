"""Compare actual PC cell sampling with cartridge row/column tile streaming.

Compiles map_cell.c unchanged into an isolated fixture. Exercises room edges,
transparent layers and cells with absent BG1/BG2 maps; no game is launched.
"""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    target = ROOT / 'build/codex-wide-cells'
    target.mkdir(parents=True, exist_ok=True)
    source = target / 'cells.c'
    original = (ROOT / 'src/map/map_cell.c').read_text()
    def function(name):
        import re
        match = re.search(r'^(?:static )?[A-Za-z_][^\n;=]*\b' + name + r'\([^\n]*\) \{', original, re.M)
        assert match, name
        end, depth = match.end(), 1
        while depth:
            depth += (original[end] == '{') - (original[end] == '}')
            end += 1
        return original[match.start():end] + '\n'
    # Exact production bodies, no synthetic reimplementation of the reader.
    bodies = original[:original.index('#ifdef PLATFORM_PC')]
    # the PC-only helpers MapWorldTile uses (top-edge close, edge strip, plain base tiles; KHCOM_TOPCLOSE gate)
    bodies += '#include <stdlib.h>\n#include <stdio.h>\n' + original[original.index('static int sMapTopClose'):original.index('static int MapWorldTile(')]
    bodies += ''.join(function(name) for name in ('MapGetCell', 'MapWorldTile', 'MapBuildBgRow', 'MapBuildBgColumn'))
    source.write_text(bodies + r'''
#include <stdio.h>
static MapCell cells[20];
static u16 pieces[20][3][64];
static u16 outside1[1024], outside2[1024];
static MapRoomDef room;
int main(void) {
    int i, b, n, x, y, checks = 0;
    u16 strips[3][32];
    sMapCells = cells; sMapCols = 5; sMapRows = 4; sMapRoomDef = &room;
    room.map = outside1; room.map2 = outside2;
    outside1[0x110] = 0x7123; outside2[0x340] = 0x8456;
    for (i = 0; i < 20; i++) {
        for (b = 0; b < 3; b++) for (n = 0; n < 64; n++)
            pieces[i][b][n] = 1 + i * 192 + b * 64 + n;
        cells[i].bg1Map = i % 3 ? pieces[i][0] : NULL;
        cells[i].bg2Map = i % 4 ? pieces[i][1] : NULL;
        cells[i].bg3Map = pieces[i][2];
    }
    for (y = -16; y <= 24; y++) for (x = -16; x <= 36; x++) {
        MapBuildBgRow(strips[2], strips[1], strips[0], x, y);
        for (n = 0; n < 32; n++) for (b = 1; b <= 3; b++) {
            int actual = MapWorldTile(b, x + n, y);
            if (actual != strips[b - 1][n]) {
                printf("FAIL row bg%d (%d,%d) got%d expected%d\n", b,x+n,y,actual,strips[b-1][n]);
                return 1;
            }
            checks++;
        }
        MapBuildBgColumn(strips[2], strips[1], strips[0], x, y);
        for (n = 0; n < 32; n++) for (b = 1; b <= 3; b++) {
            int actual = MapWorldTile(b, x, y + n);
            if (actual != strips[b - 1][n]) {
                printf("FAIL column bg%d (%d,%d)\n", b,x,y+n); return 1;
            }
            checks++;
        }
    }
    puts("PASS actual MapWorldTile matches original streams incl edges/holes/missing layers");
    printf("%d tile comparisons\n", checks);
    return 0;
}
''')
    cc = next((ROOT.parent / 'toolchain').glob('llvm-mingw-*/bin/i686-w64-mingw32-clang.exe'))
    includes = [ROOT / 'include', ROOT / 'build/eu/gen', ROOT / 'port/include']
    includes += [path for path in (ROOT / 'include').iterdir() if path.is_dir()]
    command = [str(cc), '-O0', '-std=gnu89', '-w', '-DVERSION_EU', '-DPLATFORM_PC',
               '-mno-ms-bitfields', '-funsigned-char', '-ffunction-sections', '-fdata-sections',
               '-include', 'port/game_prefix.h']
    for path in includes:
        command += ['-I', str(path)]
    command += [str(source), '-Wl,--gc-sections', '-o', str(target / 'cells.exe')]
    subprocess.run(command, cwd=ROOT, check=True)
    result = subprocess.run([str(target / 'cells.exe')], capture_output=True, text=True)
    (target / 'report.txt').write_text(result.stdout + result.stderr)
    print(result.stdout, end='')
    return result.returncode


if __name__ == '__main__':
    raise SystemExit(main())
