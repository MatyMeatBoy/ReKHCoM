"""Live menu profiles must classify Moogle's entrance before its first visible frame."""
from pathlib import Path
import subprocess,tempfile
ROOT=Path(__file__).resolve().parents[2]
def main():
 s=(ROOT/'src/mode.c').read_text(encoding='utf8')
 start=s.index('int PortMoogleMenu(void) {');end=s.index('/* The menu screens drawn',start)
 style_start=s.index('    PortSideStyle(mode == &gModeAllmap')
 style=s[style_start:s.index(';',style_start)+1]
 code=r'''
#include <assert.h>
typedef int Mode;
static Mode gModeStatus,gModeMapinspect,gModeDeck,gModeMsTop,gModeMsShop,gModeMsCharge,gModeOther;
static Mode gModeAllmap,gModeWORLDSELECT,gModeWorldwarp,gModeJiminy,gModeMenuNew,gModeMenuLoad;
static Mode* gCurrentMode;
enum { PORT_SIDE_MENU=4,PORT_SIDE_PATTERN=5,PORT_SIDE_EDGE_FADE=7,
       PORT_SIDE_BLUR_BANDS=8,PORT_SIDE_GROUND_BANDS=9,PORT_SIDE_SCENE_BANDS=11 };
static int actualStyle;
static void PortSideStyle(int style) {actualStyle=style;}
'''+s[start:end]+'\nstatic void Entry(Mode* mode) { gCurrentMode=mode;\n'+style+'\n}\n'+r'''
int main(void) {
 Mode* menus[]={&gModeMsTop,&gModeMsShop,&gModeMsCharge};
 /* The source classification is independent of warm texture caches and of
    the 45-frame/8-frame header sliding timer: test every entry frame. */
 for(int warm=0;warm<2;warm++)for(int frame=0;frame<60;frame++)for(int i=0;i<3;i++) {
  Entry(menus[i]);assert(PortMoogleMenu()==(i==2?2:1));
  assert(PortMenuBackgroundOnly());
  assert(actualStyle==PORT_SIDE_MENU); /* Real entry path must agree with live classifiers. */
 }
 gCurrentMode=&gModeStatus;assert(PortMenuBackgroundOnly() && !PortMoogleMenu());
 gCurrentMode=&gModeMapinspect;assert(PortMenuBackgroundOnly());
 gCurrentMode=&gModeDeck;assert(PortMenuBackgroundOnly());
 gCurrentMode=&gModeOther;assert(!PortMenuBackgroundOnly() && !PortMoogleMenu());
 Entry(&gModeOther);assert(actualStyle==PORT_SIDE_EDGE_FADE);
 Entry(&gModeAllmap);assert(actualStyle==PORT_SIDE_PATTERN);
 Entry(&gModeJiminy);assert(actualStyle==PORT_SIDE_GROUND_BANDS);
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
