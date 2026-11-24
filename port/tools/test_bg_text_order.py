"""Verify the production background glyph ordering over sparse, mixed layers."""
from pathlib import Path
import subprocess, tempfile

root=Path(__file__).resolve().parents[2]
text=(root/'port/src/port_ppu.c').read_text(encoding='utf8')
name='BuildBgTextOrder'
start=text.rfind('\n',0,text.index(name+'('))+1
opening=text.index('{',start);end=opening+1;depth=1
while depth:
    depth+=(text[end]=='{')-(text[end]=='}');end+=1
function=text[start:end]
program=r'''
#include <stdint.h>
#include <stdio.h>
typedef struct {uint8_t bg,priority,screen;} BgTextGlyph;
static BgTextGlyph sBgTextGlyphs[1024];static int sBgTextCount;
''' + function + r'''
int main(void){
 unsigned seed=0x1234567;int order[1024],start[16],end[16],reference[1024];
 for(int trial=0;trial<3000;trial++){
  sBgTextCount=trial%1025;
  for(int i=0;i<sBgTextCount;i++){
   seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;
   sBgTextGlyphs[i].priority=seed%5;sBgTextGlyphs[i].bg=(seed>>4)%5;sBgTextGlyphs[i].screen=(seed>>8)&1;
  }
  for(int layer=0;layer<=2;layer+=2){
   int count=0;BuildBgTextOrder(layer,order,start,end);
   for(int p=3;p>=0;p--)for(int b=3;b>=0;b--){
    int g=(3-p)*4+3-b;if(start[g]!=count)return 1;
    for(int j=0;j<sBgTextCount;j++)if(sBgTextGlyphs[j].priority==p && sBgTextGlyphs[j].bg==b && (sBgTextGlyphs[j].screen?2:0)==layer)reference[count++]=j;
    if(end[g]!=count)return 2;
   }
   for(int j=0;j<count;j++)if(order[j]!=reference[j])return 3;
  }
 }
 puts("PASS: background glyph buckets preserve original draw order");return 0;
}
'''
base=next(p for p in root.parents if (p/'toolchain').is_dir())
cc=next((base/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
with tempfile.TemporaryDirectory(prefix='bg-text-order-') as directory:
    p=Path(directory);(p/'test.c').write_text(program)
    subprocess.run([str(cc),'-O2','-std=gnu11',str(p/'test.c'),'-o',str(p/'test.exe')],check=True)
    subprocess.run([str(p/'test.exe')],check=True)
