"""Execute production gallery resume and fade consumers without a renderer/ROM."""
import os
from pathlib import Path
import subprocess
import tempfile
ROOT = Path(__file__).resolve().parents[2]

def function(source, signature):
    begin = source.index(signature)
    brace = source.index('{', begin)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[begin:end]

def main():
    menu = (ROOT / 'port/src/port_scene_gallery_menu.inc').read_text(encoding='utf-8')
    ui = (ROOT / 'port/src/port_ui.c').read_text(encoding='utf-8')
    code = r'''#include <assert.h>
#include <string.h>
#define MENU_FADE 8
#define PAGE_SCENES 3
static int sSceneChoice,sSceneScroll,sSceneHd,sSceneHarnessTick,sScenePause,sPage,sSceneViewTab,sMenuLightStart;
static int sOpen,sFade,sClosing,sReturn,suppressed,offline;
static unsigned char sSceneBroken[8];
static int PortScenePosterRendering(void){return offline;}
static int SceneAt(int i,void*p){(void)p;return i+100;}
static int SceneViewCount(void){return 8;}
static int MenuLightNow(void){return 19;}
void PortInputSuppressUntilRelease(void){suppressed++;}
static void PortUiOpenOptions(void){sOpen=1;sFade=0;sClosing=sReturn=0;}
'''
    code += function(menu, 'void PortUiGalleryResume(void)') + '\n'
    code += function(ui, 'int PortUiMenuCanvas(void)') + '\n'
    code += function(ui, 'int PortUiSceneFade(void)') + '\n'
    code += r'''
int main(void){
 for(int origin=0;origin<2;origin++)for(int hd=0;hd<3;hd++)for(int skip=0;skip<2;skip++){
  (void)origin;(void)skip;
  sOpen=0;sFade=0;sClosing=5;sReturn=8;sScenePause=1;
  sSceneChoice=4;sSceneScroll=2;sSceneHd=hd;sSceneHarnessTick=77;
  memset(sSceneBroken,0,sizeof(sSceneBroken));sSceneBroken[3]=1;
  int prior=suppressed;PortUiGalleryResume();
  assert(sOpen && sPage==PAGE_SCENES && !sScenePause);
  assert(sSceneChoice==4 && sSceneScroll==2 && sSceneHd==hd && sSceneHarnessTick==77);
  assert(sSceneBroken[3] && sSceneViewTab==-1 && sMenuLightStart==19);
  assert(PortUiMenuCanvas() && "first return frame must be gallery canvas, never restored title/field");
  assert(!PortUiSceneFade() && !sClosing && !sReturn);
  assert(suppressed==prior+1);
 }
 offline=1;sOpen=0;PortUiGalleryResume();assert(!sOpen);
 return 0;
}
'''
    toolchain = next(p / 'toolchain' for p in ROOT.parents if (p / 'toolchain').is_dir())
    cc = sorted(toolchain.glob('llvm-mingw-*/bin/clang.exe'))[-1]
    with tempfile.TemporaryDirectory(prefix='gallery-return-') as directory:
        c=Path(directory)/'test.c';exe=Path(directory)/'test.exe'
        c.write_text(code,encoding='utf-8')
        subprocess.run([str(cc),'-std=c11','-Wall','-Wextra','-Werror',str(c),'-o',str(exe)],check=True)
        subprocess.run([str(exe)],check=True)
    print('PASS production gallery resume/fade consumers: first frame, preserved selection, input latch, offline capture')
if __name__=='__main__': main()

