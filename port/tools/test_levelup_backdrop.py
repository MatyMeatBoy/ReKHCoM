"""Compile the real level-up PC setup: only its stats BG may become screen UI.

This CPU check complements real boss reward captures; it does not certify pixels.
"""
from pathlib import Path
import argparse
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', type=Path, required=True)
    args = parser.parse_args()
    source = (ROOT / 'src/card/card_level_up.c').read_text()
    body = source.split('void Level_Up_0(LevelUpWork* work) {', 1)[1]
    setup = body.split('#ifdef PLATFORM_PC', 1)[1].split('#endif', 1)[0]
    close = source.split('void Level_Up_3(LevelUpWork* work) {', 1)[1]
    cleanup = close.split('#ifdef PLATFORM_PC', 1)[1].split('#endif', 1)[0]
    names = re.findall(r'^\s*(BATTLE_[A-Z0-9_]+)(?:\s*[,=]|\s*$)',
                       (ROOT / 'include/btl/battle_ids.h').read_text(), re.M)
    ids = ','.join(names)
    probe = r'''
#include <stdio.h>
#include <string.h>
#include "battle_ids.h"
struct { int battleId; unsigned flags; } battle, *gBtlWork=&battle;
static int splits[4], bands[4], clips[4];
void PortBgSplit(int bg,int value) { splits[bg]=value; }
void PortBgSplitBands(int bg,int value) { bands[bg]=value; }
void PortBgClip(int bg,int value) { clips[bg]=value; }
static void setup(void) { SETUP }
static void cleanup(void) { CLEANUP }
int main(void) {
    int ids[]={IDS}; unsigned i; int bad=0;
    for(i=0;i<sizeof(ids)/sizeof(ids[0]);i++) {
        int bg, stats=ids[i]==BATTLE_URSULA?0:1;
        memset(splits,0,sizeof(splits)); memset(bands,0,sizeof(bands));
        memset(clips,0,sizeof(clips)); battle.battleId=ids[i]; setup();
        for(bg=0;bg<4;bg++) {
            if(splits[bg]!=(bg==stats?146:0) || bands[bg]!=(bg==stats) || clips[bg]!=(bg==stats)) {
                fprintf(stderr,"battle%d bg%d must%s be stats UI: split%d bands%d clip%d\n",
                        ids[i],bg,bg==stats?"":" not",splits[bg],bands[bg],clips[bg]); bad++;
            }
        }
        cleanup();
        for(bg=0;bg<4;bg++) if(splits[bg] || bands[bg]) {
            fprintf(stderr,"battle%d bg%d leaves split UI after close\n",ids[i],bg); bad++;
        }
    }
    if(bad) return 1;
    printf("PASS actual Level_Up_0 PC setup: %u battle IDs x4 BG roles\n",i);
    return 0;
}
'''.replace('SETUP', setup).replace('CLEANUP', cleanup).replace('IDS', ids)
    with tempfile.TemporaryDirectory(prefix='khcom-levelup-') as directory:
        directory = Path(directory)
        cfile, exe = directory / 'probe.c', directory / 'probe.exe'
        cfile.write_text(probe)
        subprocess.run([str(args.cc), '-std=c11', '-O1', '-I'+str(ROOT/'include/btl'),
                        str(cfile), '-o', str(exe)], check=True)
        subprocess.run([str(exe)], check=True)


if __name__ == '__main__':
    main()
