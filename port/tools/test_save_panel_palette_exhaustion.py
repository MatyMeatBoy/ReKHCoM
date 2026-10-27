"""Real save-menu palette readers: mixed allocator failures follow the GBA BIOS bus.
No game build, ROM, GPU or exception recovery is used.
"""
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    start = source.rfind('\n', 0, source.index(name + '(')) + 1
    brace = source.index('{', start)
    depth, end = 1, brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def main():
    source = (ROOT/'src/map/map.c').read_text(encoding='utf8')
    engine = (ROOT/'src/engine.c').read_text(encoding='utf8')
    types = (ROOT/'include/types.h').read_text(encoding='utf8')
    bios_macro = next(line for line in types.splitlines() if line.startswith('#define BIOS_PTR'))
    code = PREAMBLE + bios_macro + '\n'
    code += function(engine, 'FadeSetPaletteExcluded') + '\n'
    for name in ('MapSaveSetPanelPalettesExcluded', 'MapSaveSetCharaPalettesExcluded'):
        code += function(source, name) + '\n'
    code += HARNESS
    cc = next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
    with tempfile.TemporaryDirectory(prefix='rekhcom-save-palettes-') as directory:
        src = Path(directory)/'test.c'
        exe = Path(directory)/'test.exe'
        src.write_text(code, encoding='utf8')
        subprocess.run([str(cc), '-std=gnu11', '-Wall', '-Wextra', '-Werror', str(src), '-o', str(exe)], check=True)
        result = subprocess.run([str(exe)], timeout=10, creationflags=0x08004000)
        assert result.returncode == 0, f'save palette readers failed: {result.returncode:#x}'
    print('PASS: 256 palette mixes x enable/disable; BIOS unchanged; valid banks and bounds preserved')


PREAMBLE = r'''
#include <assert.h>
#include <stdint.h>
#include <string.h>
typedef uint8_t u8;
typedef uint16_t u16;
/* Guest/GBA resource prefix: a 32-bit src pointer, refCount, then index at +6. */
typedef struct { uint32_t src; u16 refCount, index; } FldRes;
typedef struct { FldRes *palette, *palette2, *palette3, *palette4, *palette5, *palette6, *palette7, *palette8; } MapSaveWork;
typedef struct { u8 excluded; } PaletteSlot;
static struct { PaletteSlot slots[32]; } fade;
static typeof(fade)* gFadeWork = &fade;
static unsigned char gPortBiosArea[64] __attribute__((aligned(16)));
'''
HARNESS = r'''
int main(void) {
    FldRes resources[8] = {{0}};
    const int panel[] = {0, 2, 3, 7};
    const int chars[] = {1, 4, 5, 6};
    unsigned char expected[32], bios[64];
    for (int i=0; i<64; i+=4) { uint32_t word=0xE3A02004u; memcpy(gPortBiosArea+i,&word,4); }
    memcpy(bios,gPortBiosArea,sizeof(bios));
    assert((uintptr_t)&resources[0].index-(uintptr_t)&resources[0] == 6);
    for (int i=0;i<8;i++) resources[i].index=(u16)i;
    for (int mask=0; mask<256; mask++) for (int enabled=0;enabled<2;enabled++) for (int which=0;which<2;which++) {
        MapSaveWork work;
        FldRes* p[8];
        for (int i=0;i<8;i++) p[i]=(mask&(1<<i))?&resources[i]:NULL;
        work=(MapSaveWork){p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]};
        memset(&fade,!enabled,sizeof(fade)); memset(expected,!enabled,sizeof(expected));
        if (!which) for (int i=11;i<=14;i++) expected[i]=(u8)enabled;
        for (int i=0;i<4;i++) { int slot=(which?chars:panel)[i]; if(p[slot]) expected[16+slot]=(u8)enabled; }
        if(which) MapSaveSetCharaPalettesExcluded(&work,(u8)enabled);
        else MapSaveSetPanelPalettesExcluded(&work,(u8)enabled);
        for(int i=0;i<32;i++) assert(fade.slots[i].excluded==expected[i]);
        assert(!memcmp(bios,gPortBiosArea,sizeof(bios)));
    }
    /* Preserve the production fade guard even for a real resource with a bad bank. */
    memset(&fade,0,sizeof(fade));
    FadeSetPaletteExcluded(32,1); FadeSetPaletteExcluded(0xE3B0,1);
    for(int i=0;i<32;i++) assert(!fade.slots[i].excluded);
    return 0;
}
'''

if __name__ == '__main__':
    main()
