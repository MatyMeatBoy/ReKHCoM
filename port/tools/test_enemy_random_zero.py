"""Exercise the complete enemy-card initializer with real 32-bit game types.

Synthetic valid card tables isolate index zero from invalid enemy kinds. The
oracle differs only at that remainder, using the cartridge's zero result.
This is not evidence that story scripts currently call this API with zero.
"""
from pathlib import Path
import argparse
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def extract(text):
    start = text.index('void EnemyUsecardRandomInit(')
    end = text.index('\nvoid UseEnemyCard(', start)
    return text[start:end]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--source-ref')
    ap.add_argument('--expect-zero-crash', action='store_true')
    args = ap.parse_args()
    path = 'src/card/card_enemy.c'
    text = (subprocess.check_output(['git', '-C', str(ROOT), 'show',
                                    args.source_ref + ':' + path], text=True)
            if args.source_ref else (ROOT / path).read_text())
    actual = extract(text)
    old = 'GetRandom() % (s16)work->args.index'
    new = 'MOD0(GetRandom(), (s16)work->args.index)'
    needle = new if new in actual else old
    assert actual.count(needle) == 1
    oracle = actual.replace('EnemyUsecardRandomInit(', 'OracleInit(', 1)
    oracle = oracle.replace(needle, 'OracleRemainder(GetRandom(), (s16)work->args.index)')
    unit = r'''
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "card.h"
#include "boss_card_data.h"
#include "card_def_data.h"
#include "engine_math.h"
static const s32 ids[10]={0,1,2,3,4,5,6,7,8,9};
const s32* gEnemyCardIds[]={ids,ids,ids,ids};
const u8 gEnemyCardCounts[]={1,3,9,10};
const CardDef gCardDefs[10]={{.value=0},{.value=1},{.value=2},{.value=3},
 {.value=4},{.value=5},{.value=6},{.value=7},{.value=8},{.value=9}};
static unsigned calls, seed;
u16 GetRandom(void){return (u16)(seed+0x9e37u*calls++);}
static unsigned OracleRemainder(unsigned a,short b){return b?a%(unsigned)b:0;}
''' + actual + oracle + r'''
static void check(unsigned variant,unsigned index,unsigned value){
 CardDisplayArgs arg;CardDisplayWork got,want;unsigned actualCalls;
 memset(&arg,0x39,sizeof(arg));arg.variant=variant;arg.index=index;
 memset(&got,0xa5,sizeof(got));want=got;
 seed=value;calls=0;EnemyUsecardRandomInit(&got,&arg);actualCalls=calls;
 seed=value;calls=0;OracleInit(&want,&arg);
 assert(calls==actualCalls);assert(!memcmp(&got,&want,sizeof(got)));
 assert(got.cardDef>=gCardDefs && got.cardDef<gCardDefs+10);
 assert(got.bobAngle==(u8)(value+0x9e37u*(actualCalls-1)));
}
int main(int argc,char**argv){
 static const unsigned seeds[]={0,1,2,7,255,32767,65534,65535};
 unsigned v,i,s,count=0;
 /* Nonzero controls execute first, in the same binary as the zero case. */
 for(v=0;v<4;v++)for(i=1;i<256;i++)for(s=0;s<8;s++){
  check(v,i,seeds[s]);count++;
 }
 printf("PASS %u nonzero full-state/RNG controls\n",count);fflush(stdout);
 if(argc>1)return 0;
 for(v=0;v<4;v++)for(s=0;s<8;s++)check(v,0,seeds[s]);
 puts("PASS 32 zero-index full-state/RNG controls (valid synthetic card tables)");
 return 0;
}
'''
    compiler = next((ROOT.parent / 'toolchain').glob('llvm-mingw-*/bin/i686-w64-mingw32-clang.exe'))
    includes = [ROOT / 'include', ROOT / 'port/include']
    includes += [p for p in (ROOT / 'include').iterdir() if p.is_dir() and p.name != 'gba']
    with tempfile.TemporaryDirectory(prefix='rekhcom-enemy-zero-') as tmp:
        tmp = Path(tmp)
        source, exe = tmp / 'probe.c', tmp / 'probe.exe'
        source.write_text(unit, encoding='utf-8')
        subprocess.run([str(compiler), '-O2', '-std=gnu11', '-DPLATFORM_PC',
                        '-DVERSION_EU', '-funsigned-char', '-fwrapv',
                        '-mno-ms-bitfields', *[f'-I{p}' for p in includes],
                        str(source), '-o', str(exe)], check=True)
        subprocess.run([str(exe), 'nonzero-only'], check=True)
        result = subprocess.run([str(exe)])
        if args.expect_zero_crash:
            assert (result.returncode & 0xffffffff) == 0xc0000094, result.returncode
            print('EXPECTED RED: old zero-index remainder raises integer divide-by-zero')
        else:
            result.check_returncode()


if __name__ == '__main__':
    main()
