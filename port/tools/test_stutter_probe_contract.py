"""Compile real stutter wrappers/macros with a deterministic clock; no SDL/GPU."""
import argparse
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def function(text, signature):
    start = text.index(signature + ' {')
    end = text.index('{', start) + 1
    depth = 1
    while depth:
        depth += (text[end] == '{') - (text[end] == '}')
        end += 1
    return text[start:end]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--cc', type=Path, required=True)
    args = parser.parse_args()
    header = (ROOT/'port/include/port/port.h').read_text(encoding='utf-8')
    sprites = (ROOT/'port/src/port_hdsprites.c').read_text(encoding='utf-8')
    text = (ROOT/'port/src/port_hdtext.c').read_text(encoding='utf-8')
    main_source = (ROOT/'port/src/port_main.c').read_text(encoding='utf-8')
    probe = header[header.index('typedef struct PortStutterProbe {'):header.index('/* ---- audio')]
    logic = next(line.strip() for line in main_source.splitlines() if 'double logic = logicStart' in line)
    finish = next(line.strip() for line in main_source.splitlines() if 'logicStart = SDL_GetPerformanceCounter();' in line)
    source = r'''
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"contract: %s\n",#x); exit(97); } } while(0)
static uint64_t now;
static int clockCalls, loads, glyphCalls;
static uint64_t SDL_GetPerformanceCounter(void) { clockCalls++; return now++; }
''' + probe + r'''
PortStutterProbe gPortStutter;
static struct { const char* name; } kHdSprites[] = { {"heavy"}, {"light"} };
static int Load(int id) { loads++; now += id ? 10 : 100; return id == 0; }
typedef struct { int ready; } HdGlyph;
typedef struct { HdGlyph glyphs[256]; } HdFont;
typedef struct { int unused; } SDL_Renderer;
static HdGlyph* GlyphNow(SDL_Renderer* renderer, HdFont* font, int code) {
    (void)renderer; glyphCalls++; now+=20; font->glyphs[code&255].ready=1;
    return &font->glyphs[code&255];
}
''' + function(sprites, 'static int LoadCounted(int i)') + '\n' + function(text, 'static HdGlyph* Glyph(SDL_Renderer* renderer, HdFont* f, int code)') + r'''
static uint64_t logicStart, sCycleEnd;
static void EndLog(void) {
''' + finish + r'''
}
static double Logic(uint64_t cycleStart) {
    double ms=1.0;
''' + logic + r'''
    return logic;
}
int main(void) {
    HdFont font={0};
    CHECK(LoadCounted(0)); CHECK(!LoadCounted(1));
    CHECK(Glyph(NULL,&font,65)->ready);
    CHECK(clockCalls==0 && !gPortStutter.hdLoads && !gPortStutter.glyphs);
    CHECK(loads==2 && glyphCalls==1);
    gPortStutter.on=1; now=0;
    CHECK(LoadCounted(0)); CHECK(!LoadCounted(1));
    CHECK(gPortStutter.hdLoads==2 && gPortStutter.hdLoadTicks==112);
    CHECK(gPortStutter.worstLoadTicks==101 && !strcmp(gPortStutter.worstLoad,"heavy"));
    CHECK(Glyph(NULL,&font,66)->ready && gPortStutter.glyphs==1 && gPortStutter.glyphTicks==21);
    int before=clockCalls; CHECK(Glyph(NULL,&font,66)->ready && clockCalls==before);
    sCycleEnd=100; now=146; EndLog(); CHECK(Logic(150)==4.0);
    (void)sCycleEnd;
    puts("PASS real stutter wrappers/macros: disabled no clock/counters; enabled totals/slowest/cache; logic excludes previous log/autosave");
}
'''
    with tempfile.TemporaryDirectory(prefix='stutter-contract-') as temp:
        folder=Path(temp)
        cases=[('valid',source,0),
               ('clock-off',source.replace('if (!gPortStutter.on)\n        return Load(i);', 'if (0)\n        return Load(i);',1),97),
               ('wrong-logic',source.replace(finish,'logicStart = sCycleEnd;',1),97)]
        for name,code,expected in cases:
            unit=folder/(name+'.c'); exe=folder/(name+'.exe')
            unit.write_text(code,encoding='utf-8')
            subprocess.run([str(args.cc),'-std=c11','-Wall','-Wextra','-Werror',str(unit),'-o',str(exe)],check=True)
            result=subprocess.run([str(exe)],capture_output=True,timeout=10)
            assert result.returncode==expected,(name,result.returncode,result.stdout,result.stderr)
            if expected: print('PASS negative control:',name)
            else: print(result.stdout.decode().strip())


if __name__=='__main__': main()
