"""The JP UI queue must give the shared Japanese glyph backends Shift-JIS.

Runs the real queue producer and native CP932 conversion without a renderer.
"""
from pathlib import Path
import subprocess,tempfile
R=Path(__file__).resolve().parents[2]
s=(R/'port/src/port_ui.c').read_text(encoding='utf-8');a=s.index('static void DrawGlyph(int code,');b=s.index('    const uint16_t* frame;',a);producer=s[a:b]+'}\n'
s=(R/'port/src/port_hdtext.c').read_text(encoding='utf-8');a=s.index('int PortCodepointToSjis(unsigned cp) {');b=s.index('\n}',a)+2;conversion=s[a:b]
code=r'''
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#define VERSION_JP 1
#define JP_CHAR 0x1000000
static struct { int code,x,y,alive,pw,run;uint32_t face,shadow; } sHdUiGlyphs[2048];
static int sHdUiCount,sHdUiInText,sHdUiRun;
static uint32_t fb,*sFb=&fb,*sHdUiFrame=&fb;
static int GlyphWidth(int code){(void)code;return 10;}
'''
code+=conversion+producer+r'''
int main(void){
 DrawGlyph(JP_CHAR|0x4E00,0,0,0,0);
 if(sHdUiGlyphs[0].code!=0x88EA){fprintf(stderr,"FAIL: JP glyph queue has %x, expected CP932 88ea\n",sHdUiGlyphs[0].code);return 1;}
 DrawGlyph('A',0,0,0,0);
 if(sHdUiGlyphs[1].code!='A')return 2;
 puts("PASS: JP UI queue converts Unicode to Shift-JIS; Latin preserved");return 0;
}
'''
cc=next((R.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
with tempfile.TemporaryDirectory(prefix='rekhcom-jp-ui-') as folder:
 folder=Path(folder);src=folder/'jp.c';exe=folder/'jp.exe';src.write_text(code,encoding='utf-8');subprocess.run([str(cc),str(src),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
