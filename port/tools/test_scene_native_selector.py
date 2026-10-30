"""Compile actual native scene selector cases: three labels and bidirectional wrapping."""
import argparse,re,subprocess,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--cc',required=True);a=ap.parse_args()
 s=(ROOT/'port/src/port_ui.c').read_text(encoding='utf-8')
 table=re.search(r'static const char\* const sSceneFraming\[.*?\n};',s,re.S).group()
 cases=re.findall(r'case ITEM_SCENE_FRAMING:[ \t]*\n([ \t]*[^\n]*gPortSceneFraming[^\n]*)\n\s*break;',s,re.S)
 value=next(c for c in cases if 'SDL_strlcpy' in c);change=next(c for c in cases if 'gPortSceneFraming =' in c)
 wrap=re.search(r'static int Wrap\(.*?\n}',s,re.S).group()
 code=r'''
#include <stdio.h>
#include <string.h>
#define LANG_COUNT 6
#define ITEM_SCENE_FRAMING 0
#define SDL_clamp(v,l,h) ((v)<(l)?(l):(v)>(h)?(h):(v))
static int gPortSceneFraming;
static void SDL_strlcpy(char*out,const char*in,size_t n){snprintf(out,n,"%s",in);}
TABLE
WRAP
static void change(int dir){switch(0){case ITEM_SCENE_FRAMING: CHANGE break;}}
static void value(int l,char*out,size_t size){switch(0){case ITEM_SCENE_FRAMING: VALUE break;}}
int main(void){char out[100];
 if(sizeof(sSceneFraming)/sizeof(sSceneFraming[0])!=3)return 1;
 for(int l=0;l<6;l++)for(int i=0;i<3;i++){gPortSceneFraming=i;value(l,out,sizeof(out));if(!out[0])return 2;}
 gPortSceneFraming=0;change(-1);if(gPortSceneFraming!=2)return 3;value(4,out,sizeof(out));if(strcmp(out,"Plano original"))return 4;
 change(1);if(gPortSceneFraming!=0)return 5;change(1);if(gPortSceneFraming!=1)return 6;change(1);if(gPortSceneFraming!=2)return 7;change(1);if(gPortSceneFraming!=0)return 8;
 change(-1);change(-1);if(gPortSceneFraming!=1)return 9;
 puts("PASS native selector: all3 values/six languages, forward/backward wrapping and original-shot label");return 0;}
'''.replace('TABLE',table).replace('WRAP',wrap).replace('CHANGE',change).replace('VALUE',value)
 with tempfile.TemporaryDirectory(prefix='scene-native-selector-') as tmp:
  p=Path(tmp);(p/'test.c').write_text(code,encoding='utf-8')
  subprocess.run([a.cc,'-std=c99','-static',str(p/'test.c'),'-o',str(p/'test.exe')],check=True)
  subprocess.run([str(p/'test.exe')],check=True)
if __name__=='__main__':main()
