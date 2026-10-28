"""Real HD loader: named seam correction checkpoint, every other texel unchanged.

No ROM, SDL or GPU. Existing decoder/upload failure contracts stay separate.
"""
from pathlib import Path
import subprocess
import tempfile
import re
from test_hd_sprite_budget import ROOT, function, preamble

before = 'static SDL_Surface* SDL_CreateSurfaceFrom(int w,int h,int f,void*p,int pitch){return ++surfaces==failSurface?NULL:(void*)1;}'
after = ('static unsigned char captured[128*256*4];\n'
         'static SDL_Surface* SDL_CreateSurfaceFrom(int w,int h,int f,void*p,int pitch)'
         '{memcpy(captured,p,(size_t)w*h*4);return (void*)1;}')
assert before in preamble
code = preamble.replace(before, after)
code += function('static void Unload(') + '\n' + function('static int Load(int i) {')
code += r'''
static void check(const char* name,int group,int kind) {
 memset(sState,0,sizeof(sState));sBytes=0;creates=0;failTexture=0;
 width=decodedW=128;height=decodedH=256;infoOK=decodeOK=1;
 kHdSprites[0].name=name;kHdSprites[0].group=group;kHdSprites[0].w=16;kHdSprites[0].h=32;
 assert(Load(0));
 for(int y=0;y<256;y++)for(int x=0;x<128;x++) {
   unsigned char* p=captured+(y*128+x)*4;
   int changed=group==6&&kind==1&&x>=88&&x<96&&y>=120&&y<128;
   assert(p[0]==(changed?8:255));
   assert(p[1]==p[0]&&p[2]==p[0]&&p[3]==255);
 }
 Unload(0);
}
int main(void) {
 check("gKingdomKeySmallCardHD",6,1);
 check("gOlympiaSmallCardHD",6,0);
 check("gCureSmallCardHD",6,0);check("gSimbaSmallCardHD",6,0);
 check("gKingdomKeySmallCardHD",1,0);check("gOlympiaSmallCardHD",1,0);
 puts("PASS: named card seam pixels only; unrelated assets unchanged");
}
'''
old = 'return decodeOK?calloc((size_t)*w**h,4):NULL;'
new = ('if(!decodeOK)return NULL;unsigned char*p=malloc((size_t)*w**h*4);'
       'memset(p,255,(size_t)*w**h*4);return p;')
assert old in code
code = code.replace(old,new)
with tempfile.TemporaryDirectory(prefix='small-card-seam-') as folder:
    source=Path(folder)/'test.c';exe=Path(folder)/'test.exe';source.write_text(code)
    cc=next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
    subprocess.run([str(cc),str(source),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True,timeout=10,creationflags=0x08004000)
# Verify the actual value-to-art mapping instead of repeating one identical
# loader call ten times and labelling that digit coverage.
catalog=(ROOT/'src/card/card_catalog.c').read_text(encoding='utf-8')
rows=re.findall(r'\{\s*(gCardWep01Frame0[^{}]+?)\{[^{}]*\},\s*\}',catalog)
normal=[]
for row in rows:
    match=re.search(r'CARD_KIND_KINGDOM_KEY,\s*0x0,\s*(\d+),',row)
    if match:
        assert 'gKingdomKeySmallCardFrame0, gKingdomKeySmallCardTiles, gKingdomKeySmallCardPalette' in row
        normal.append(int(match[1]))
assert sorted(normal)==list(range(10)),normal
print('PASS: production loader changes only one seam cell; catalog values0..9 share this artwork; digit rendering not exercised')
