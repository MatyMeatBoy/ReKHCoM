"""Current visible bottom bands, never a timed cache, continue into menu margins."""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
def main():
 s=(ROOT/'port/src/port_main.c').read_text(encoding='utf8')
 marker='static uint32_t MenuVisibleBottomBand('
 assert marker in s,'No visible-native bottom band path (reproduced status frame212 has black margin)'
 start=s.index(marker);a=s.index('{',start);end=a+1;depth=1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 code=r'''
#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#define GBA_SCREEN_H 160
'''+s[start:end]+r'''
int main(void) {
 uint32_t row[240];
 for(int side=-1;side<=1;side+=2)for(int warm=0;warm<2;warm++)for(int frame=0;frame<60;frame++) {
  for(int x=0;x<240;x++)row[x]=0xFF0055FFu;
  int edge=side<0?0:239;
  assert(MenuVisibleBottomBand(row,240,edge,side,152)==0xFF0055FFu);
  /* Upper panels, tabs and header slides retain their own existing logic. */
  assert(!MenuVisibleBottomBand(row,240,edge,side,31));
  assert(!MenuVisibleBottomBand(row,240,edge,side,131));
  assert(!MenuVisibleBottomBand(row,240,edge,side,160));
  row[edge+(side<0?63:-63)]=0xFF112233u;
  assert(!MenuVisibleBottomBand(row,240,edge,side,152)); /* badges/lettering */
  row[edge+(side<0?63:-63)]=0xFF0055FFu;
  assert(!MenuVisibleBottomBand(row,32,edge,side,152)); /* no out-of-bounds sample */
  for(int x=0;x<240;x++)row[x]=0xFF000000u;
  assert(MenuVisibleBottomBand(row,240,edge,side,152)==0xFF000000u); /* genuinely hidden */
 }
 return 0;
}
'''
 cc=next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
 with tempfile.TemporaryDirectory(prefix='rekhcom-bottom-band-') as d:
  p=Path(d)/'test.c';e=Path(d)/'test.exe';p.write_text(code)
  subprocess.run([str(cc),'-Wall','-Wextra','-Werror',str(p),'-o',str(e)],check=True)
  r=subprocess.run([str(e)],creationflags=0x08004000,timeout=10);assert r.returncode==0
 print('PASS: visible bottom band, nonuniform badges, hidden/fading rows, both sides and bounds')
if __name__=='__main__':main()
