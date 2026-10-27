"""Live menu profiles must classify Moogle's entrance before its first visible frame."""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
def main():
 s=(ROOT/'src/mode.c').read_text(encoding='utf8')
 start=s.index('int PortMoogleMenu(void) {');end=s.index('/* The menu screens drawn',start)
 code=r'''
#include <assert.h>
typedef int Mode;
static Mode gModeStatus,gModeMapinspect,gModeDeck,gModeMsTop,gModeMsShop,gModeMsCharge,gModeOther;
static Mode* gCurrentMode;
'''+s[start:end]+r'''
int main(void) {
 Mode* menus[]={&gModeMsTop,&gModeMsShop,&gModeMsCharge};
 /* The source classification is independent of warm texture caches and of
    the 45-frame/8-frame header sliding timer: test every entry frame. */
 for(int warm=0;warm<2;warm++)for(int frame=0;frame<60;frame++)for(int i=0;i<3;i++) {
  gCurrentMode=menus[i];assert(PortMoogleMenu()==(i==2?2:1));
  assert(PortMenuBackgroundOnly());
 }
 gCurrentMode=&gModeStatus;assert(PortMenuBackgroundOnly() && !PortMoogleMenu());
 gCurrentMode=&gModeMapinspect;assert(PortMenuBackgroundOnly());
 gCurrentMode=&gModeDeck;assert(PortMenuBackgroundOnly());
 gCurrentMode=&gModeOther;assert(!PortMenuBackgroundOnly() && !PortMoogleMenu());
 return 0;
}
'''
 cc=next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
 with tempfile.TemporaryDirectory(prefix='rekhcom-menu-profile-') as d:
  p=Path(d)/'test.c';e=Path(d)/'test.exe';p.write_text(code,encoding='utf8')
  subprocess.run([str(cc),'-Wall','-Wextra','-Werror',str(p),'-o',str(e)],check=True)
  r=subprocess.run([str(e)],creationflags=0x08004000,timeout=10)
  assert r.returncode==0,f'Moogle entry profile failed: {r.returncode:#x}'
 print('PASS: live Moogle entry/shop/charge profiles and existing submenu/background guards')
if __name__=='__main__':main()
