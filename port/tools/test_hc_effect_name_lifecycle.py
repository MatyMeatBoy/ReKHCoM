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
    ap.add_argument('--compare-gba', action='store_true', help='compare all task side effects with original source using the GBA zero quotient')
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
    reference = subprocess.check_output(['git', 'show', '038d92627bf2792465dd7eb143b7da6e0c856638:src/card/mode_premire.c'], cwd=ROOT, text=True, encoding='utf-8')
    start = reference.index('u8 HCEffectName_1('); end = reference.index('\nu8 IsHcEffectNameShuffling(', start)
    original_reference = reference[start:end].strip()
    for region in ('eu', 'us', 'jp'):
        assert preprocess(original_reference, region) == preprocess(consumer, region), 'non-PC consumer changed: ' + region
    reference = original_reference.replace('u8 HCEffectName_1(', 'u8 ReferenceHCEffectName_1(')
    for ptr in ('gBtlWork', 'gRikuBtlWork'):
        old = '(((s16)' + ptr + '->hcEffectCount << 16) / div)'
        assert reference.count(old) == 1
        reference = reference.replace(old, 'GbaQuotient(((s16)' + ptr + '->hcEffectCount << 16), div)')
    producers = '\n'.join(extract(ROOT / ('src/card/' + file), name) for file, name in (
        ('card_battle.c', 'ApplySoraHcEffect'), ('card_battle_riku.c', 'ApplyRikuHcEffect')))
    pre = r'''
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <windows.h>
#include "types.h"
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
static int taskUpdateCalls;
#define SetTaskUpdate(...) (++taskUpdateCalls)
static s32 GbaQuotient(s32 a,s32 b){return b==0?0:a/b;}
static void SplitFourDigits(int value,u8*dst){(void)value;(void)dst;digitCalls++;}
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FAIL line%d: %s\n",__LINE__,#c);exit(97);}}while(0)
'''
    tests = r'''
static int cases;
static void Reset(int side,HcEffectNameWork*w){
 memset(&sora,0,sizeof(sora));memset(&riku,0,sizeof(riku));memset(&state,0,sizeof(state));memset(w,0,sizeof(*w));
 w->side=side;w->effect=HC_EFFECT_INCREMENTOR;w->visible=1;w->blinkInterval=32;digitCalls=0;taskUpdateCalls=0;
}
int main(void){
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
 for(int side=0;side<2;side++){
  HcEffectNameWork w;CardBattleWork card={0};
  /* No effect remains after its real producer applies Dispel. The existing
     task must follow its original zero-quotient lifecycle checks. */
  Reset(side,&w);
  if(side==0){state.soraHcEffect=HC_EFFECT_DISPEL;ApplySoraHcEffect(&card);}
  else{state.rikuHcEffect=HC_EFFECT_DISPEL;ApplyRikuHcEffect(&card);}
  CHECK((side==0?sora.hcEffect:riku.hcEffect)==HC_EFFECT_NONE);
  w.effect=side==0?state.soraHcEffect:state.rikuHcEffect;
  printf("case side%d producer-dispel -> NONE\n",side);fflush(stdout);
  CHECK(HCEffectName_1(&w,NULL)==0);CHECK(digitCalls==0);
  CHECK((side==0?state.soraHcEffect:state.rikuHcEffect)==HC_EFFECT_NONE);cases++;
  Reset(side,&w);CHECK(HCEffectName_1(&w,NULL)==0);cases++;
  /* Battle end can clear the effective effect while an old positive counter
     and name task remain for this update. */
  Reset(side,&w);sora.hcEffectCount=riku.hcEffectCount=1;
  state.soraHcEffect=state.rikuHcEffect=HC_EFFECT_INCREMENTOR;
  CHECK(HCEffectName_1(&w,NULL)==1);CHECK(w.blinkInterval==2);cases++;
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
    if args.compare_gba:
        tests = r'''
static int cases;
static void Check(int side,int effective,int stateEffect,int workEffect,int replaced,int remaining){
 HcEffectNameWork initial={0},want,got;
 Battle bs={effective,remaining,0},br={effective,remaining,0},wantS,wantR;
 CardBattleState initialState={stateEffect,stateEffect,replaced,replaced},wantState;
 initial.side=side;initial.effect=workEffect;initial.blinkInterval=32;initial.visible=1;
 want=initial;sora=bs;riku=br;state=initialState;digitCalls=taskUpdateCalls=0;
 int expected=ReferenceHCEffectName_1(&want,NULL),digits=digitCalls,updates=taskUpdateCalls;
 wantS=sora;wantR=riku;wantState=state;
 got=initial;sora=bs;riku=br;state=initialState;digitCalls=taskUpdateCalls=0;
 int actual=HCEffectName_1(&got,NULL);
 if(actual!=expected || memcmp(&want,&got,sizeof(want)) || memcmp(&wantState,&state,sizeof(state)) ||
    memcmp(&wantS,&sora,sizeof(sora)) || memcmp(&wantR,&riku,sizeof(riku)) || digits!=digitCalls || updates!=taskUpdateCalls){
  fprintf(stderr,"MISMATCH side%d effective%d state%d work%d replaced%d remaining%d returns%d/%d effects%d/%d\n",
   side,effective,stateEffect,workEffect,replaced,remaining,actual,expected,
   side==0?state.soraHcEffect:state.rikuHcEffect,side==0?wantState.soraHcEffect:wantState.rikuHcEffect);exit(97);
 }
 cases++;
}
int main(void){
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
 const int ids[]={HC_EFFECT_NONE,HC_EFFECT_INCREMENTOR,HC_EFFECT_ATTACK_BRACER,HC_EFFECT_RANDOM_FLUSH,HC_EFFECT_DISPEL,HC_EFFECT_MIMIC,HC_EFFECT_INCREMENTOR_2};
 for(int side=0;side<2;side++){
  for(int a=0;a<7;a++)for(int b=0;b<7;b++)for(int c=0;c<7;c++)for(int replaced=0;replaced<2;replaced++){
   int n=gHcEffectDefs[ids[a]].count,values[]={0,1,n>0?n-1:0,n,n+1};
   for(int k=0;k<5;k++)Check(side,ids[a],ids[b],ids[c],replaced,values[k]);
  }
  for(int effect=1;effect<55;effect++)for(int remaining=0;remaining<=gHcEffectDefs[effect].count+1;remaining++)
   Check(side,effect,effect,effect,0,remaining);
  /* Producer states: Dispel in normal combat and Dispel against an existing effect. */
  for(int versus=0;versus<2;versus++){
   CardBattleWork card={0};sora=(Battle){HC_EFFECT_INCREMENTOR,1,versus?BTL_FLAG_HUM_BATTLE:0};
   riku=(Battle){HC_EFFECT_INCREMENTOR,1,0};state=(CardBattleState){HC_EFFECT_INCREMENTOR,HC_EFFECT_INCREMENTOR,0,0};
   if(side==0){state.soraHcEffect=HC_EFFECT_DISPEL;ApplySoraHcEffect(&card);}
   else{state.rikuHcEffect=HC_EFFECT_DISPEL;ApplyRikuHcEffect(&card);}
   Check(side,side==0?sora.hcEffect:riku.hcEffect,side==0?state.soraHcEffect:state.rikuHcEffect,
    HC_EFFECT_INCREMENTOR,1,side==0?sora.hcEffectCount:riku.hcEffectCount);
  }
 }
 printf("PASS %d production-consumer cases exactly match original GBA-zero-quotient task/state/visibility/blink/counter behavior\n",cases);return 0;
}
'''
    if args.side != 'both':
        side = 0 if args.side == 'sora' else 1
        tests = tests.replace('for(int side=0;side<2;side++)', f'for(int side={side};side<{side + 1};side++)')
    unit = out / 'consumer.c'
    unit.write_text(pre + enum + '\nstatic const HcEffectDef gHcEffectDefs[55]={' +
                    ','.join('{%d}' % n for n in counts) + '};\n' +
                    producers + '\n' + shuffling + '\n' + (reference if args.compare_gba else '') + '\n' + consumer + '\n' + tests, encoding='utf-8')
    base = next(p for p in ROOT.parents if (p / 'toolchain').is_dir())
    compiler = next((base / 'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
    exe = out / 'consumer.exe'
    subprocess.run([str(compiler), '-std=gnu11', '-O0', '-DPLATFORM_PC', '-DVERSION_EU', '-iquote', str(ROOT / 'include'),
                    str(unit), '-o', str(exe)], check=True)
    result = subprocess.run([str(exe)], capture_output=True, timeout=10, creationflags=0x08000000)
    report = dict(source=str(args.source.resolve()), source_sha256=hashlib.sha256(args.source.read_bytes()).hexdigest(),
                  source_commit=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
                  consumer_sha256=hashlib.sha256(consumer.encode()).hexdigest(), effect_counts=counts, compare_gba=args.compare_gba, non_pc_regions_unchanged=['eu','us','jp'],
                  reference_commit='038d92627bf2792465dd7eb143b7da6e0c856638', reference_sha256=hashlib.sha256(original_reference.encode()).hexdigest(),
                  exit=result.returncode, exit_hex=f'0x{result.returncode & 0xffffffff:08X}',
                  stdout=result.stdout.decode(errors='replace'), stderr=result.stderr.decode(errors='replace'))
    (out / 'result.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(out);print(json.dumps(report, indent=2))
    if result.returncode:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
