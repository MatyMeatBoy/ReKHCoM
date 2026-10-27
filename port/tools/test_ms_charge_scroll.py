"""Execute the complete production MsChargeDraw and tab-count consumer on CPU.

Only rendering/animation are test doubles. No SDL, ROM, GPU or player data.
Every run gets a new child directory under --work, including faulting baselines.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

from prepare_synthesis_logic import extract

ROOT = Path(__file__).resolve().parents[2]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--work', type=Path, required=True)
    ap.add_argument('--source', type=Path, default=ROOT / 'src/menu/ms_charge.c')
    args = ap.parse_args()
    args.work.mkdir(parents=True, exist_ok=True)
    out = Path(tempfile.mkdtemp(prefix='ms-charge-', dir=args.work))
    source = args.source.read_text(encoding='utf-8')
    draw = extract(args.source, 'MsChargeDraw')
    count = extract(args.source, 'GetMsChargeTabCount')
    declarations = source[source.index('enum MsChargeState {'):source.index('\ns16 GetMsChargeTabStart(')]
    pre = r'''
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include "ms_charge_api.h"
/* Opaque renderer-only state is never inspected by the real scroll path. */
typedef int AnimState;
typedef int TextSlot;
static unsigned char gMsChargeScrollbarFrame0[1];
static s16 gSineTable[256];
#define CARD_CATEGORY_ENEMY 3
#define SPRITE_PRIORITY(x) (x)
#define SPRITE_FLAG_HFLIP 1
static int yObserved, scrollbarCalls;
static void RecordDraw(int x,int y,const void* frame){
 (void)x;if(frame==gMsChargeScrollbarFrame0){yObserved=y;scrollbarCalls++;}
}
#define DrawSprite(x,y,frame,...) RecordDraw(x,y,frame)
#define DrawTextSlots(...) ((void)0)
#define GetTextSlotsWidth(...) 0
#define ApproachValueHalf(...) ((void)0)
static void* AnimUpdate(AnimState* unused){(void)unused;return NULL;}
static MsCard card;
MsCard* GetMsChargeSelectedCard(void){return &card;}
'''
    main_c = r'''
static int cases;
static void CheckCase(int tab,int count,int scroll){
 int range=(count+2)/3-4;
 /* Geometry contract: no scrolling means top origin. Out-of-range transient
    state retains that origin; positive valid ranges retain the original ratio. */
 int expected=range>0 && scroll>=0 && scroll<=range ? 40+84*scroll/range : 40;
 for(int i=0;i<4;i++)sMsChargeCategoryEntryCount[i]=0;
 sMsChargeCategoryEntryCount[tab==4?0:tab]=count;
 sMsChargeTab=tab;sMsChargeGridScroll=scroll;
 sMsChargeMenuState=MS_CHARGE_MENU_STATE_TAB;
 sMsChargeState=MS_CHARGE_STATE_FADE_IN;
 yObserved=-1000;scrollbarCalls=0;
 MsChargeDraw();cases++;
 if(scrollbarCalls!=1 || yObserved!=expected){
  fprintf(stderr,"count%d tab%d scroll%d got%d calls%d expected%d\n",count,tab,scroll,yObserved,scrollbarCalls,expected);exit(97);
 }
}
int main(void){
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
 /* These reachable one-page counts fault on the unguarded production code. */
 for(int n=10;n<=12;n++)CheckCase(4,n,0);
 for(int tab=0;tab<=4;tab++)for(int n=0;n<=286;n++){
  int range=(n+2)/3-4;
  for(int scroll=-1;scroll<=98;scroll++)CheckCase(tab,n,scroll);
  if(range>0){CheckCase(tab,n,range);CheckCase(tab,n,range+1);}
 }
 printf("PASS: %d full MsChargeDraw consumer cases; counts0..286, tabs0..4, scroll-1..98\n",cases);
 return 0;
}
'''
    unit = out / 'consumer.c'
    unit.write_text(pre + declarations + '\n' + count + '\n' + draw + '\n' + main_c, encoding='utf-8')
    compiler = ROOT.parents[1] / 'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe'
    exe = out / 'consumer.exe'
    subprocess.run([str(compiler), '--target=x86_64-w64-mingw32', '-std=gnu11', '-O1', '-DPLATFORM_PC',
                    '-iquote', str(ROOT / 'include'), '-iquote', str(ROOT / 'include/menu'),
                    str(unit), '-o', str(exe)], check=True)
    result = subprocess.run([str(exe)], capture_output=True, timeout=15)
    report = dict(source=str(args.source.resolve()),
                  source_sha256=hashlib.sha256(args.source.read_bytes()).hexdigest(),
                  draw_sha256=hashlib.sha256(draw.encode()).hexdigest(),
                  count_sha256=hashlib.sha256(count.encode()).hexdigest(),
                  source_commit=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
                  defines=['PLATFORM_PC'], exit=result.returncode, exit_hex=f'0x{result.returncode & 0xffffffff:08X}',
                  stdout=result.stdout.decode('utf-8', errors='replace'), stderr=result.stderr.decode('utf-8', errors='replace'))
    (out / 'result.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(out)
    print(json.dumps(report, indent=2))
    if result.returncode:
        raise SystemExit(1)


if __name__ == '__main__':
    main()
