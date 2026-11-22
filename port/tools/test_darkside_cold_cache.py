"""Exercise the actual Darkside layer/prepare functions before worker reads.

Synthetic map identities and row preparation only; no ROM or graphics backend.
"""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


def function(source, signature):
    start = source.index(signature)
    brace = source.index('{', start)
    depth = 1
    end = brace + 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


class ColdCache(unittest.TestCase):
    def test_latch_publishes_complete_cache_before_workers(self):
        source = (ROOT/'port/src/port_darkside.c').read_text(encoding='utf8')
        code = r'''
#include <assert.h>
#include <stddef.h>
#define BEACH_W 256
static int sPrepared, sShoreRow = -1, sPeriod[64], rows, walks, enabled = 1;
static short sShoreRight[2048], sShoreLeft[2048];
static unsigned short gBosDsdBgMap0[1], gBosDsdBgMap2[1], gDefaultBgMap[1];
static const void* gBosDsdFrameMaps[47][4];
static int PortDarksideBeachOn(void) { return enabled; }
static void PrepareRow(int row) { ++rows; sPeriod[row] = row == 5 ? -1 : 1; }
static void Walk(short* out, int edge, int dir) { ++walks; out[0] = (short)(edge + dir); }
PREPARE
LAYER
int main(void) {
    const void* beach[4] = {gBosDsdBgMap0,gDefaultBgMap,gBosDsdBgMap2,gDefaultBgMap};
    const void* other[4] = {gDefaultBgMap,gDefaultBgMap,gDefaultBgMap,gDefaultBgMap};
    assert(PortDarksideLayer(NULL) == 0 && rows == 0);
    assert(PortDarksideLayer(other) == 0 && rows == 0);
    enabled = 0;
    assert(PortDarksideLayer(beach) == 0 && rows == 0);
    enabled = 1;
    /* Main's latch calls this before dispatching rows. No worker may build it. */
    assert(PortDarksideLayer(beach) == 1);
    assert(sPrepared && rows == 64 && walks == 2 && sShoreRow == 5);
    assert(sShoreRight[0] == 256 && sShoreLeft[0] == -1);
    for (int worker = 0; worker < 8; ++worker) {
        Prepare();
        assert(PortDarksideLayer(beach) == 1);
    }
    assert(rows == 64 && walks == 2); /* all warm calls are read-only */
}
'''.replace('PREPARE', function(source, 'static void Prepare(void)'))
        code = code.replace('LAYER', function(source, 'int PortDarksideLayer('))
        compiler = ROOT.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe'
        with tempfile.TemporaryDirectory(prefix='darkside cold cache ') as temporary:
            p = Path(temporary)
            (p/'test.c').write_text(code, encoding='utf8')
            subprocess.run([str(compiler), '-std=c11', '-O2', str(p/'test.c'), '-o', str(p/'test.exe')], check=True)
            subprocess.run([str(p/'test.exe')], check=True, timeout=10)


if __name__ == '__main__':
    unittest.main()
