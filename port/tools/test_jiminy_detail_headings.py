"""Run the real journal heading registration for every layout/language/fade."""
import argparse,subprocess,tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--cc',type=Path,required=True);p.add_argument('--baseline-ref');a=p.parse_args()
s=subprocess.check_output(['git','show',a.baseline_ref+':src/menu/mode_jiminy.c'],text=True,encoding='utf8') if a.baseline_ref else (root/'src/menu/mode_jiminy.c').read_text(encoding='utf8')
start=s.index('#if defined(PLATFORM_PC) && (defined(VERSION_EU) || defined(VERSION_US))',s.index('case JIMINY_STATE_EXIT_TO_MAP:',s.index('void mode_jiminy_')))
end=s.index('    DrawMsgGlyphs(sJiminyWork->charCount);',start)
block=s[start:end]
unit=r'''#include <assert.h>
#include <stdio.h>
#include <stdint.h>
enum {JIMINY_DETAIL_LAYOUT_STORY,JIMINY_DETAIL_LAYOUT_CARD,JIMINY_DETAIL_LAYOUT_CHARACTER};
enum {LANGUAGE_ENGLISH,LANGUAGE_FRENCH,LANGUAGE_SPANISH,LANGUAGE_ITALIAN,LANGUAGE_GERMAN};
enum {FADE_MODE_BLACK,FADE_MODE_WHITE,FADE_MODE_ADD_WHITE};
static unsigned char gJiminyBgTiles[1],gJiminyBgTitleTilesFrench[1],gJiminyBgTitleTilesSpanish[1],gJiminyBgTitleTilesItalian[1],gJiminyBgTitleTilesGerman[1];
static uint16_t gJiminyStoryDetailMap[1],gJiminyCardDetailMap[1],gJiminyCharacterDetailMap[1],gJiminyDetailOverlayMap[1];
static struct {int detailLayout;} work,*sJiminyWork=&work;
static struct {int amount,mode;} fade,*gFadeWork=&fade;
static int gLanguage,count,dark[2],white[2];static const void *shownTiles[2],*shownMap[2];
static void PortHdStillSprite(const void*tiles,const void*frame,int x,int y,int d,int w){assert(!x&&!y&&count<2);shownTiles[count]=tiles;shownMap[count]=frame;dark[count]=d;white[count]=w;count++;}
static void actual(void){
'''+block+r'''}
int main(void){
 const void*maps[]={gJiminyStoryDetailMap,gJiminyCardDetailMap,gJiminyCharacterDetailMap};
 const void*tiles[]={gJiminyBgTiles,gJiminyBgTitleTilesFrench,gJiminyBgTitleTilesSpanish,gJiminyBgTitleTilesItalian,gJiminyBgTitleTilesGerman};
 for(int layout=0;layout<3;layout++)for(int lang=0;lang<5;lang++)for(int mode=0;mode<3;mode++)for(int level=0;level<=31;level++){
  work.detailLayout=layout;gLanguage=lang;fade.mode=mode;fade.amount=level<<8;count=0;
  actual();
#if defined(VERSION_JP) || !defined(PLATFORM_PC)
  assert(!count);continue;
#endif
  assert(count==2&&shownMap[0]==maps[layout]&&shownMap[1]==gJiminyDetailOverlayMap);
  for(int i=0;i<2;i++) {
#ifdef VERSION_EU
  assert(shownTiles[i]==tiles[lang]);
#else
  assert(shownTiles[i]==tiles[0]);
#endif
  assert(dark[i]==(mode==FADE_MODE_BLACK?level*255/31:0));assert(white[i]==(mode!=FADE_MODE_BLACK?level*255/31:0));
  }
 }
 puts("PASS actual heading registration/language/fades or native-only regional path");
}
'''
with tempfile.TemporaryDirectory(prefix='rekhcom-journal-headings-') as d:
 d=Path(d);c=d/'test.c';c.write_text(unit,encoding='utf8')
 for region in ('EU','US','JP','native'):
  flags=['-DVERSION_US'] if region=='native' else ['-DPLATFORM_PC','-DVERSION_'+region]
  exe=d/(region+'.exe');subprocess.run([str(a.cc.resolve()),'-std=c11','-O2',*flags,str(c),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
