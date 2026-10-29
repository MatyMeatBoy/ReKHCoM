"""Run actual inventory functions against real32-bit types and guarded tables.

No ROM, SDL or save file. Controlled definitions/counts expose a definition
overread and phantom reserved-slot entries; this is not a reported game crash.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def function(text, name):
    import re
    match = re.search(r'^\w+\s+' + name + r'\([^;]*?\)\s*\{', text, re.M)
    assert match, name
    start = text.index('{', match.start())
    level, end = 1, start + 1
    while level:
        level += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[match.start():end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--opt-level', choices=('0', '2'), default='0')
    parser.add_argument('--baseline-ref', help='read production functions from an existing Git revision')
    args = parser.parse_args()
    out = args.out.resolve(); out.mkdir(parents=True, exist_ok=True)
    def read_source(name):
        if args.baseline_ref:
            return subprocess.check_output(['git', '-C', str(ROOT), 'show',
                                            args.baseline_ref + ':' + name], text=True, encoding='utf-8')
        return (ROOT / name).read_text(encoding='utf-8')
    selection = read_source('src/card/card_worldselect.c')
    menu = read_source('src/menu/mode_mapinspect.c')
    names = ('CountRegularMapCards', 'CountMapCards', 'CountOwnedMapCardKinds',
             'ListOwnedMapCardKinds', 'CountZeroValueMapCards', 'CountMapCardsOfKind',
             'AddMapCard', 'RemoveMapCard', 'HasMapCard')
    bodies = [function(selection, name) for name in names]
    bodies.append(function(menu, 'MapInspectBuildInventory'))
    prefix = r'''
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include "card.h"
#include "card_ids.h"
#include "ms_types.h"
#include "jiminy_records_index_data.h"
static MapCardDef (*defs)[260];
static u8 (*counts)[270];
#define gMapCardDefs (*defs)
#define gMapCardCounts (*counts)
#define MAP_CARD_INVENTORY_EMPTY 27
static MapCardInventoryEntry entries[27];
static MapCardInventoryEntry* sMapCardInventoryEntries=entries;
static u16 sMapInspectCategoryStart[4],sMapInspectCategoryEntryCount[4];
static s16 sMapInspectCategoryCardCount[4],sMapInspectCardTotal;
static const u16 sMapCardCategoryTypes[4]={MAP_CARD_COLOR_RED,MAP_CARD_COLOR_GREEN,MAP_CARD_COLOR_BLUE,MAP_CARD_COLOR_GOLD};
_Static_assert(sizeof(entries)==0x2f4,"actual cartridge inventory layout");
_Static_assert(sizeof(void*)==4,"probe must use game pointer width");
static void Fill16(void* dst,u16 value,unsigned size){u16* p=dst;while(size>=2){*p++=value;size-=2;}}
#define DmaFill16(channel,value,dst,size) Fill16(dst,value,size)
void SetJiminyFlag(u32 flag){(void)flag;}
static void* Guarded(unsigned bytes){
 SYSTEM_INFO info;DWORD old;unsigned space;unsigned char* base;
 GetSystemInfo(&info);space=(bytes+info.dwPageSize-1)/info.dwPageSize*info.dwPageSize;
 base=VirtualAlloc(NULL,space+info.dwPageSize,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
 if(!base || !VirtualProtect(base+space,info.dwPageSize,PAGE_NOACCESS,&old))ExitProcess(95);
 return base+space-bytes;
}
static void Init(void){
 unsigned i;defs=Guarded(sizeof(*defs));counts=Guarded(sizeof(*counts));
 memset(defs,0,sizeof(*defs));memset(counts,0,sizeof(*counts));
 for(i=0;i<260;i++){gMapCardDefs[i].kind=i/10;gMapCardDefs[i].color=sMapCardCategoryTypes[(i/10)%4];}
}
'''
    cases = r'''
int main(int argc,char**argv){
 int i;MapSelectKindEntry list[27];
 SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);
 if(argc!=2)return 94;Init();
 if(!strcmp(argv[1],"inventory")){
  for(i=0;i<260;i+=10)gMapCardCounts[i]=1;
  MapInspectBuildInventory();
  if(sMapInspectCardTotal!=20 || sMapInspectCategoryStart[3]!=20 ||
     sMapInspectCategoryEntryCount[0]!=7 || sMapInspectCategoryEntryCount[1]!=7 ||
     sMapInspectCategoryEntryCount[2]!=6 || sMapInspectCategoryEntryCount[3]!=6 ||
     entries[26].cardType!=27)return 97;
 }else if(!strcmp(argv[1],"reserved-list")){
  memset(gMapCardCounts+260,255,10);memset(list,255,sizeof(list));
  if(CountOwnedMapCardKinds()!=0)return 97;
  ListOwnedMapCardKinds(list);if(list[0].baseCardId!=0xffff)return 97;
  if(CountZeroValueMapCards()!=0 || CountMapCards()!=0)return 97;
 }else if(!strcmp(argv[1],"invalid-add")){
  const u16 ids[]={260,269,270,65535};
  for(i=0;i<4;i++)if(AddMapCard(ids[i]))return 97;
  for(i=0;i<270;i++)if(gMapCardCounts[i])return 97;
 }else if(!strcmp(argv[1],"invalid-query-remove")){
  const u16 ids[]={260,269,270,65535};
  memset(gMapCardCounts+260,255,10);
  for(i=0;i<4;i++)if(HasMapCard(ids[i]) || RemoveMapCard(ids[i]) || CountMapCardsOfKind(ids[i]))return 97;
  for(i=260;i<270;i++)if(gMapCardCounts[i]!=255)return 97;
 }else if(!strcmp(argv[1],"valid-cards")){
  for(i=0;i<260;i+=10)if(!AddMapCard(i) || !HasMapCard(i) || CountMapCardsOfKind(i)!=1)return 97;
  if(CountMapCards()!=26 || CountOwnedMapCardKinds()!=26 || CountZeroValueMapCards()!=26)return 97;
  memset(list,255,sizeof(list));ListOwnedMapCardKinds(list);
  for(i=0;i<26;i++)if(list[i].baseCardId!=i*10 || list[i].count!=1)return 97;
  if(list[26].baseCardId!=0xffff)return 97;
  for(i=0;i<260;i+=10)if(!RemoveMapCard(i) || HasMapCard(i))return 97;
  /* Existing regular-card capacity and gold-key exception stay intact. */
  for(i=0;i<11;i++)gMapCardCounts[i]=9;
  if(CountRegularMapCards()!=99 || AddMapCard(0) || !AddMapCard(250) ||
     gMapCardCounts[0]!=9 || gMapCardCounts[250]!=1 || CountMapCards()!=100)return 97;
  if(!RemoveMapCard(0) || !AddMapCard(0) || gMapCardCounts[0]!=9)return 97;
 }else return 94;
 printf("PASS %s\n",argv[1]);return 0;
}
'''
    unit = out / 'probe.c'; unit.write_text(prefix + '\n'.join(bodies) + cases, encoding='utf-8')
    includes = [ROOT / 'include', ROOT / 'build/us/gen',
                *(p for p in (ROOT / 'include').iterdir() if p.is_dir() and p.name != 'gba')]
    command = [str(args.cc.resolve()), '-O'+args.opt_level, '-std=gnu11', '-mno-ms-bitfields',
               '-fno-strict-aliasing', '-fwrapv', '-DPLATFORM_PC', '-DVERSION_US']
    for path in includes: command += ['-iquote', str(path)]
    command += [str(unit), '-o', str(out / 'probe.exe')]
    result = subprocess.run(command, capture_output=True, text=True)
    (out / 'compile.log').write_text(result.stdout + result.stderr)
    if result.returncode: print(result.stderr); return result.returncode
    rows = []
    for case in ('inventory', 'reserved-list', 'invalid-add', 'invalid-query-remove', 'valid-cards'):
        p = subprocess.run([str(out / 'probe.exe'), case], capture_output=True, timeout=5,
                           creationflags=0x08004000)
        rows.append(dict(case=case, exit=p.returncode, output=p.stdout.decode(errors='replace'),
                         pass_=p.returncode == 0, guarded_access_fault=p.returncode & 0xffffffff == 0xc0000005))
    record = dict(result='PASS' if all(r['pass_'] for r in rows) else 'FAIL', cases=rows,
                  production_source=args.baseline_ref or 'working tree', optimization=args.opt_level,
                  source_sha256=[hashlib.sha256(b.encode()).hexdigest() for b in bodies],
                  scope='Actual source functions/32-bit game types; controlled guarded definition/count arrays. No ROM/save/device/game reproduction. Reserved serialized counts remain untouched.')
    (out / 'RESULT.json').write_text(json.dumps(record, indent=2) + '\n')
    print(json.dumps(record)); return 0 if record['result'] == 'PASS' else 1


if __name__ == '__main__':
    raise SystemExit(main())
