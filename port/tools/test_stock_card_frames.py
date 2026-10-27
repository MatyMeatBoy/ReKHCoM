"""HUD pixel compositor must retain small-card frames under their HD window.
Extract both production HD cover branches; no ROM, GPU or game build needed.
"""
from pathlib import Path
import subprocess
import tempfile
ROOT=Path(__file__).resolve().parents[2]

def block(text, marker):
    start=text.index(marker); brace=text.index('{',start); end=brace+1; depth=1
    while depth:
        depth+=(text[end]=='{')-(text[end]=='}');end+=1
    return text[start:end]

def main():
    source=(ROOT/'port/src/port_ppu.c').read_text(encoding='utf8')
    hud=source[source.index('void PortPpuRenderHud('):]
    hide=block(hud,'if (hdPart) {')
    over=block(hud,'if (sHdCover && (size_t)w * h <= sHdCoverSize && sHdCover[py * w + px]')
    sprites=(ROOT/'port/src/port_hdsprites.c').read_text(encoding='utf8')
    keeps=block(sprites,'int PortHdSpriteKeepsCartridge(')
    code=PREAMBLE+keeps+'\nstatic void Pixel(int id,int queued,int foreground) {\n'
    code+='int i=0,hdPart=queued,px=0,py=0,w=1,h=1; uint32_t argb=0xFF123456u; sOamHd[0].id=id;\n'
    code+='for(int once=0;once<1;once++) {\n'+hide+'\n'+over+'\nbuf[0]=argb;\n}\n(void)foreground;(void)i;\n}\n'+HARNESS
    cc=next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
    with tempfile.TemporaryDirectory(prefix='rekhcom-stock-frame-') as d:
        src=Path(d)/'test.c';exe=Path(d)/'test.exe';src.write_text(code)
        subprocess.run([str(cc),'-std=c99','-Wall','-Wextra','-Werror',str(src),'-o',str(exe)],check=True)
        r=subprocess.run([str(exe)],timeout=10,creationflags=0x08004000)
        assert r.returncode==0,f'HUD stock frame pixel failed: {r.returncode:#x}'
    print('PASS: small-card frame pixels survive HD queue; replacement/fallback and foreground cover preserved')

PREAMBLE=r'''
#include <assert.h>
#include <stdint.h>
#include <string.h>
#define HD_SPRITE_COUNT 2
static const struct {int group;} kHdSprites[2]={{6},{1}};
static struct {int id;} sOamHd[1];
static uint8_t cover[1],*sHdCover=cover;
static size_t sHdCoverSize=1;
static uint32_t buf[1],sHudOver[1];
static int sHudOverUsed;
'''
HARNESS=r'''
int main(void) {
    /* Repeated 1/2/3-card queues share this pixel path for player and opponents.
       queue=0 models absent, rejected, released or not yet reloaded HD texture. */
    for(int actor=0;actor<3;actor++)for(int pile=1;pile<=3;pile++)for(int card=0;card<pile;card++)for(int cycle=0;cycle<3;cycle++) {
        for(int queued=0;queued<=1;queued++) {
            buf[0]=cover[0]=sHudOverUsed=0; sHudOver[0]=0;
            Pixel(1,queued,0);
            assert(buf[0]==0xFF123456u && !sHudOverUsed);
            assert(cover[0]==queued);
        }
    }
    buf[0]=cover[0]=sHudOverUsed=0;
    Pixel(2,1,0); assert(!buf[0] && cover[0] && !sHudOverUsed);
    Pixel(0,0,1); assert(sHudOverUsed && sHudOver[0]==0xFF123456u && !buf[0]);
    return 0;
}
'''
if __name__=='__main__':main()

