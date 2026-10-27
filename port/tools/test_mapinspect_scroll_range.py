"""Exercise the production map-card scrollbar calculation at page boundaries.

No ROM, SDL, GPU or crash recovery: zero scroll range must never divide.
"""
import argparse
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--cc', type=Path, required=True)
    args = ap.parse_args()
    source = (ROOT/'src/menu/mode_mapinspect.c').read_text(encoding='utf8')
    start = source.index('    maxScroll = (GetMapInspectTabCount(sMapInspectTab) + 2) / 3 - 4;')
    body = source[start:source.index('    DrawSprite(72, barOffset + 40,', start)]
    code = r'''
#include <stdio.h>
#include <windows.h>
typedef short s16;
static int count;
static s16 sMapInspectTab, sMapInspectGridScroll;
static int GetMapInspectTabCount(int tab) { (void)tab; return count; }
static int Offset(void) {
    s16 maxScroll, barOffset;
''' + body + r'''
    return barOffset;
}
int main(void) {
    int scroll, checked=0;
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);
    for (count=0;count<=27;count++) for (scroll=-1;scroll<=27;scroll++) {
        int range=(count+2)/3-4;
        int expected=range>0 && scroll>=0 && scroll<=range ? 84*scroll/range : 0;
        sMapInspectGridScroll=scroll;
        if (Offset()!=expected) return 97;
        checked++;
    }
    printf("PASS production scrollbar: %d inventory/scroll pairs, including10/11/12 types\n",checked);
    return 0;
}
'''
    with tempfile.TemporaryDirectory(prefix='mapinspect-range-') as tmp:
        folder=Path(tmp); unit=folder/'range.c'; exe=folder/'range.exe'
        unit.write_text(code, encoding='utf8')
        subprocess.run([str(args.cc.resolve()),'-std=c11','-DPLATFORM_PC','-O2',str(unit),'-o',str(exe)],check=True)
        result=subprocess.run([str(exe)],capture_output=True,timeout=10,creationflags=0x08004000)
        print(result.stdout.decode().strip())
        assert result.returncode==0, f'production scrollbar failed with exit{result.returncode:#x}'


if __name__=='__main__': main()
