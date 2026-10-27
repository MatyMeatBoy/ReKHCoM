"""Compile the real LAD parser functions into a tiny CPU-only regression harness.

Set CC to a C compiler; optional HD_LAYOUT_SANITIZE=1 enables ASan/UBSan.
No SDL, game build, assets, renderer, or installed game is required.
"""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class LayoutBoundsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cc = os.environ.get('CC') or shutil.which('clang') or shutil.which('gcc')
        if not cc:
            raise RuntimeError('Set CC to a C compiler to run the LAD parser harness')
        source = (ROOT / 'port/src/port_hdlayout.c').read_text(encoding='utf-8')
        readers = source[source.index('static int32_t I32('):source.index('static float F32(')]
        # Parser block is kept together; compile the production implementation verbatim.
        start = source.index('/* The layout\'s sequence n:')
        parser = source[start:source.index('\nint PortHdLayoutGroupFrames(', start)]
        groups = source[source.index('int PortHdLayoutGroupFrames('):source.index('\ntypedef struct Ctx')]
        cls.tmp = tempfile.TemporaryDirectory()
        temp = Path(cls.tmp.name)
        harness = temp / 'layout.c'
        harness.write_text('#include <stdint.h>\n#include <stddef.h>\n#include <limits.h>\n#include <stdlib.h>\n#include <string.h>\n#include <stdio.h>\n'
            + source[source.index('typedef struct HdLayout {'):source.index('typedef struct HdLayoutTexture {')]
            + readers + '\n' + parser + '\n'
            'static HdLayout current;\nstatic const HdLayout* Layout(int n) { (void)n; return &current; }\n'
            + groups + '\n' + HARNESS, encoding='utf-8')
        cls.exe = temp / 'layout.exe'
        flags = ['-std=c99', '-Wall', '-Wextra', '-Werror', '-O1', '-g']
        if os.environ.get('HD_LAYOUT_SANITIZE') == '1':
            flags += ['-fsanitize=address,undefined', '-fno-sanitize-recover=all']
        subprocess.run([cc, *flags, str(harness), '-o', str(cls.exe)], check=True)

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def test_valid_sequence_and_frames(self):
        self.run_case('valid')

    def run_case(self, case):
        result = subprocess.run([str(self.exe), case], capture_output=True, text=True)
        self.assertEqual(result.returncode, 0, case + ': ' + result.stdout + result.stderr)

    def test_all_truncation_lengths(self):
        self.run_case('truncations')

    def test_all_table_bounds(self):
        self.run_case('table_bounds')

    def test_malformed_layouts(self):
        for case in ['short_lad', 'negative_table', 'overflow_table', 'overflow_count',
                     'negative_start', 'end_start', 'reverse_end', 'negative_end',
                     'bad_magic', 'short_seq', 'sequence_index', 'negative_sequence']:
            with self.subTest(case=case):
                self.run_case(case)

    def test_malformed_sequence_tables_and_indices(self):
        for case in ['negative_offset', 'negative_count', 'overflow_offset', 'overflow_size',
                     'table_at_end', 'part_sprite', 'negative_sprite', 'group_part',
                     'negative_part', 'animation_group', 'negative_group', 'group_animation',
                     'negative_animation', 'short_sprite', 'short_part', 'short_group',
                     'short_animation', 'short_agroup']:
            with self.subTest(case=case):
                self.run_case(case)


HARNESS = r'''
static void put(unsigned char *b, int o, int32_t v) {
    uint32_t u = (uint32_t)v;
    b[o] = u; b[o+1] = u >> 8; b[o+2] = u >> 16; b[o+3] = u >> 24;
}
int main(int argc, char **argv) {
    unsigned char b[380] = {0}, *s = b + 36;
    HdLayout l = { b, sizeof(b), 0 };
    const char *c = argc > 1 ? argv[1] : "valid";
    const unsigned char *seq;
    int size = 0, index = 0, valid = strcmp(c, "valid") == 0;
    put(b, 0, 0x4459414c); put(b, 24, 1); put(b, 28, 32); put(b, 32, 36);
    put(s, 0, 0x44514553);
    put(s, 8, 1); put(s, 12, 48); /* sprites */
    put(s, 16, 1); put(s, 20, 92); /* parts */
    put(s, 24, 1); put(s, 28, 112); /* sprite groups */
    put(s, 32, 1); put(s, 36, 116); /* animations */
    put(s, 40, 1); put(s, 44, 260); /* animation groups */
    put(s, 112, 0x10000); put(s, 260, 0x10000); put(s, 128, 23);
    if (!strcmp(c,"truncations")) {
        int cut;
        for (cut = 0; cut < (int)sizeof(b); cut++) {
            unsigned char *exact = malloc(cut ? (size_t)cut : 1);
            HdLayout truncated = { exact, cut, 0 };
            int accepted;
            if (!exact) return 4;
            memcpy(exact, b, (size_t)cut);
            accepted = ValidateLayout(&truncated);
            free(exact);
            /* Last animation group is [296,332); remaining bytes are padding. */
            if (accepted != (cut >= 332)) {
                fprintf(stderr,"truncation %d accepted=%d\n",cut,accepted); return 5;
            }
        }
        return 0;
    }
    if (!strcmp(c,"table_bounds")) {
        static const int bad[] = { -1, INT_MIN, INT_MAX, 344 };
        unsigned char saved[344];
        int table, k;
        memcpy(saved,s,sizeof(saved));
        for (table=0; table<5; table++) {
            for (k=0; k<4; k++) {
                put(s,12+table*8,bad[k]);
                if (ValidateLayout(&l)) return 6;
                memcpy(s,saved,sizeof(saved));
            }
            put(s,8+table*8,INT_MAX);
            if (ValidateLayout(&l)) return 7;
            memcpy(s,saved,sizeof(saved));
            put(s,8+table*8,-1);
            if (ValidateLayout(&l)) return 8;
            memcpy(s,saved,sizeof(saved));
        }
        return 0;
    }
    if (!strcmp(c,"short_lad")) l.size = 31;
    if (!strcmp(c,"negative_table")) put(b,28,-4);
    if (!strcmp(c,"overflow_table")) put(b,28,INT_MAX);
    if (!strcmp(c,"overflow_count")) put(b,24,INT_MAX);
    if (!strcmp(c,"negative_start")) put(b,32,-1);
    if (!strcmp(c,"end_start")) put(b,32,sizeof(b));
    if (!strcmp(c,"reverse_end") || !strcmp(c,"negative_end")) {
        put(b,24,2); put(b,28,300); put(b,300,36);
        put(b,304,!strcmp(c,"reverse_end") ? 20 : INT_MIN);
    }
    if (!strcmp(c,"bad_magic")) put(s,0,0);
    if (!strcmp(c,"short_seq")) l.size = 36 + 47;
    if (!strcmp(c,"sequence_index")) index = 1;
    if (!strcmp(c,"negative_sequence")) index = -1;
    if (!strcmp(c,"negative_offset")) put(s,44,-1);
    if (!strcmp(c,"negative_count")) put(s,40,-1);
    if (!strcmp(c,"overflow_offset")) put(s,36,INT_MAX);
    if (!strcmp(c,"overflow_size")) put(s,32,INT_MAX);
    if (!strcmp(c,"table_at_end")) put(s,44,l.size-36);
    if (!strcmp(c,"part_sprite")) put(s,108,1);
    if (!strcmp(c,"negative_sprite")) put(s,108,-1);
    if (!strcmp(c,"group_part")) put(s,112,0x10001);
    if (!strcmp(c,"negative_part")) put(s,112,0x1ffff);
    if (!strcmp(c,"animation_group")) put(s,120,1);
    if (!strcmp(c,"negative_group")) put(s,120,-1);
    if (!strcmp(c,"group_animation")) put(s,260,0x10001);
    if (!strcmp(c,"negative_animation")) put(s,260,0x1ffff);
    if (!strcmp(c,"short_sprite")) put(s,12,l.size-36-43);
    if (!strcmp(c,"short_part")) put(s,20,l.size-36-19);
    if (!strcmp(c,"short_group")) put(s,28,l.size-36-3);
    if (!strcmp(c,"short_animation")) put(s,36,l.size-36-143);
    if (!strcmp(c,"short_agroup")) put(s,44,l.size-36-35);
    if (index == 0 && !!ValidateLayout(&l) != valid) {
        fprintf(stderr,"unexpected layout acceptance\n"); return 3;
    }
    seq = Sequence(&l,index,&size);
    if (!!seq != valid) { fprintf(stderr,"unexpected acceptance=%d\n",!!seq); return 1; }
    if (valid) {
        l.validated = 1; /* same cached path as a successfully loaded resource */
        if (!Sequence(&l,0,&size)) return 9;
        current = l;
        if (size != l.size-36 || PortHdLayoutGroupFrames(0,0,0) != 23 ||
            PortHdLayoutGroupFrames(0,0,1) != 0) return 2;
    }
    return 0;
}
'''

if __name__ == '__main__':
    unittest.main()
