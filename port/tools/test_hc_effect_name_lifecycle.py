"""Run the production enemy-card name consumer and effect producers on native CPU.

Renderer/task hooks are test doubles; effect IDs, EU counts, lifecycle producer
functions and HCEffectName_1 are extracted from the current sources. No ROM/save.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

from gen_card_table import preprocess
from prepare_synthesis_logic import extract

ROOT = Path(__file__).resolve().parents[2]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--work', type=Path, required=True)
    ap.add_argument('--source', type=Path, default=ROOT / 'src/card/mode_premire.c')
    ap.add_argument('--side', choices=('both', 'sora', 'riku'), default='both')
    args = ap.parse_args()
    args.work.mkdir(parents=True, exist_ok=True)
    out = Path(tempfile.mkdtemp(prefix='hc-effect-', dir=args.work))
    labels = (ROOT / 'include/card/card_label_data.h').read_text(encoding='utf-8')
    enum = re.search(r'enum HcEffectId \{.*?\n\};', labels, re.S).group()
    catalog = preprocess((ROOT / 'src/card/card_catalog.c').read_text(encoding='utf-8'), 'eu')
    table = catalog[catalog.index('const HcEffectDef gHcEffectDefs'):]
    table = table[:table.index('\n};')]
    counts = [int(count) for _, count in re.findall(r'\{[^{}]*?,\s*(\d+),\s*(\d+),\s*\}', table)]
    assert len(counts) == 55 and [i for i, n in enumerate(counts) if not n] == [0]
    consumer = extract(args.source, 'HCEffectName_1')
    shuffling = extract(args.source, 'IsHcEffectNameShuffling')
    producers = '\n'.join(extract(ROOT / ('src/card/' + file), name) for file, name in (
        ('card_battle.c', 'ApplySoraHcEffect'), ('card_battle_riku.c', 'ApplyRikuHcEffect')))
    pre = r'''
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
typedef uint8_t u8; typedef uint16_t u16; typedef int16_t s16;
typedef uint32_t u32; typedef int32_t s32;
#define FALSE 0
#define TRUE 1
#define CARD_SIDE_SORA 0
#define CARD_SIDE_RIKU 1
#define CARD_LIST_MAIN 0
#define CARD_LIST_MAGIC 1
#define BTL_FLAG_HUM_BATTLE 1
#define BTL_FLAG_VS_BATTLE 2
#define BTL_FLAG_OPPONENT_CARD_ACTION 4
#define BTL_FLAG_PLAYER_CARD_ACTION 8
#define BTL_FLAG_STOCK_SEQUENCE 16
typedef struct { u32 hcEffect; s16 hcEffectCount; u32 flags; } Battle;
typedef struct { u16 soraHcEffect,rikuHcEffect; u8 soraHcEffectReplaced,rikuHcEffectReplaced; } CardBattleState;
typedef struct { s16 reloadCounts[2]; } CardBattleWork;
typedef struct { int side,effect,timer,blinkInterval,visible; u8 countThousands,countHundreds,countTens,countOnes; } HcEffectNameWork;
typedef struct { int count; } HcEffectDef;
typedef void (*TaskUpdateFunc)(void);
static Battle sora,riku; static Battle *gBtlWork=&sora,*gRikuBtlWork=&riku;
static CardBattleState state; static CardBattleState *gCardBattleState=&state;
static int digitCalls;
static void UpdateHcEffectNameShuffle(void){}
#define SetTaskUpdate(...) ((void)0)
static void SplitFourDigits(int value,u8*dst){(void)value;(void)dst;digitCalls++;}
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL line%d: %s\n",__LINE__,#c);exit(97);}}while(0)
'''
    tests = r'''
static int cases;
static void Reset(int side,HcEffectNameWork*w){
 memset(&sora,0,sizeof(sora));memset(&riku,0,sizeof(riku));memset(&state,0,sizeof(state));memset(w,0,sizeof(*w));
 w->side=side;w->effect=HC_EFFECT_INCREMENTOR;w->visible=1;w->blinkInterval=32;digitCalls=0;
}
int main(void){
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
 for(int side=0;side<2;side++){
  HcEffectNameWork w;CardBattleWork card={0};
  /* No effect remains after its real producer applies Dispel. The existing
     name task must terminate rather than divide by NONE.count (zero). */
  Reset(side,&w);
  if(side==0){state.soraHcEffect=HC_EFFECT_DISPEL;ApplySoraHcEffect(&card);}
  else{state.rikuHcEffect=HC_EFFECT_DISPEL;ApplyRikuHcEffect(&card);}
  CHECK((side==0?sora.hcEffect:riku.hcEffect)==HC_EFFECT_NONE);
  printf("case side%d producer-dispel -> NONE\n",side);fflush(stdout);
  CHECK(HCEffectName_1(&w,NULL)==0);CHECK(digitCalls==0);
  CHECK((side==0?state.soraHcEffect:state.rikuHcEffect)==HC_EFFECT_NONE);cases++;
  Reset(side,&w);CHECK(HCEffectName_1(&w,NULL)==0);cases++;
  /* Battle end can clear the effective effect while an old positive counter
     and name task remain for this update. */
  Reset(side,&w);sora.hcEffectCount=riku.hcEffectCount=1;
  state.soraHcEffect=state.rikuHcEffect=HC_EFFECT_INCREMENTOR;
  CHECK(HCEffectName_1(&w,NULL)==0);cases++;
  for(int remaining=0;remaining<=3;remaining++){
   Reset(side,&w);sora.hcEffect=riku.hcEffect=HC_EFFECT_INCREMENTOR;
   state.soraHcEffect=state.rikuHcEffect=HC_EFFECT_INCREMENTOR;
   sora.hcEffectCount=riku.hcEffectCount=remaining;
   CHECK(HCEffectName_1(&w,NULL)==(remaining>0));
   if(remaining>0){CHECK(w.blinkInterval==remaining*16);CHECK(digitCalls==1);}
   cases++;
  }
  Reset(side,&w);sora.hcEffect=riku.hcEffect=HC_EFFECT_INCREMENTOR;
  state.soraHcEffect=state.rikuHcEffect=HC_EFFECT_INCREMENTOR;
  state.soraHcEffectReplaced=state.rikuHcEffectReplaced=1;
  sora.hcEffectCount=riku.hcEffectCount=2;
  CHECK(HCEffectName_1(&w,NULL)==0);cases++;
 }
 printf("PASS %d actual producer/consumer lifecycle cases, Sora/Riku, NONE/Dispel/expired/replaced/positive counts\n",cases);return 0;
}
'''
    if args.side != 'both':
        side = 0 if args.side == 'sora' else 1
        tests = tests.replace('for(int side=0;side<2;side++)', f'for(int side={side};side<{side + 1};side++)')
    unit = out / 'consumer.c'
    unit.write_text(pre + enum + '\nstatic const HcEffectDef gHcEffectDefs[55]={' +
                    ','.join('{%d}' % n for n in counts) + '};\n' +
                    producers + '\n' + shuffling + '\n' + consumer + '\n' + tests, encoding='utf-8')
    base = next(p for p in ROOT.parents if (p / 'toolchain').is_dir())
    compiler = next((base / 'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
    exe = out / 'consumer.exe'
    subprocess.run([str(compiler), '-std=gnu11', '-O0', '-DPLATFORM_PC', '-DVERSION_EU',
                    str(unit), '-o', str(exe)], check=True)
    result = subprocess.run([str(exe)], capture_output=True, timeout=10, creationflags=0x08000000)
    report = dict(source=str(args.source.resolve()), source_sha256=hashlib.sha256(args.source.read_bytes()).hexdigest(),
                  source_commit=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
                  consumer_sha256=hashlib.sha256(consumer.encode()).hexdigest(), effect_counts=counts,
                  exit=result.returncode, exit_hex=f'0x{result.returncode & 0xffffffff:08X}',
                  stdout=result.stdout.decode(errors='replace'), stderr=result.stderr.decode(errors='replace'))
    (out / 'result.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(out);print(json.dumps(report, indent=2))
    if result.returncode:
        raise SystemExit(1)


if __name__ == '__main__':
    main()

