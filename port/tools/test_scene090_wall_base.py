"""Scene090: native center, straight wall footing and original floor boundary.

Compiles only a small coordinate resolver, never the game or ROM assets.
"""
import ctypes
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]


class Scene090WallBase(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler = os.environ.get('CC') or shutil.which('clang') or shutil.which('gcc')
        if not compiler:
            raise unittest.SkipTest('Set CC to a native C compiler')
        cls.temp = tempfile.TemporaryDirectory(prefix='scene090-wall-')
        directory = Path(cls.temp.name)
        code = directory / 'resolver.c'
        code.write_text('''#include "port_preliminar_v1_tiles.h"
__declspec(dllexport) int resolve(int bg,int x,int y,int* sx,int* sy) {
    PortPreliminarRecipe p={0};
    p.v[0]=90; p.v[1]=bg; p.v[4]=64; p.v[5]=64;
    *sx=x; *sy=y; return PortPreliminarTileSource(&p,sx,sy);
}
''')
        library = directory / 'resolver.dll'
        result = subprocess.run([compiler, '-shared', '-O0', '-I', str(ROOT / 'port/include/port'),
                                 str(code), '-o', str(library)], capture_output=True, text=True,
                                creationflags=getattr(subprocess, 'BELOW_NORMAL_PRIORITY_CLASS', 0))
        if result.returncode:
            raise RuntimeError(result.stdout + result.stderr)
        cls.library = ctypes.CDLL(str(library))
        cls.resolve = cls.library.resolve
        cls.resolve.argtypes = [ctypes.c_int] * 3 + [ctypes.POINTER(ctypes.c_int)] * 2
        cls.resolve.restype = ctypes.c_int

    @classmethod
    def tearDownClass(cls):
        # Windows holds an imported DLL until process exit; preserve temp data
        # for that process rather than forcibly removing a mapped file.
        if os.name != 'nt':
            cls.temp.cleanup()
        else:
            cls.temp._finalizer.detach()

    def source(self, x, y, bg=3):
        sx, sy = ctypes.c_int(), ctypes.c_int()
        accepted = self.resolve(bg, x, y, ctypes.byref(sx), ctypes.byref(sy))
        return accepted, sx.value, sy.value

    def test_wall_footing_uses_clean_original_lower_rows(self):
        # Columns46/47 row26 contain a grave. Columns48/49 rows25..27
        # contain the clean masonry and its authentic bottom edge instead.
        for x in (64, 65, 66, 79, 100, 143):
            for y in (29, 30, 31):
                with self.subTest(x=x, y=y):
                    self.assertEqual(self.source(x, y), (1, 48 + (x - 64) % 2, y - 4))

    def test_native_center_and_unique_props_are_unchanged(self):
        for bg in (1, 3):
            for y in range(64):
                for x in range(64):
                    self.assertEqual(self.source(x, y, bg)[1:], (x, y))

    def test_floor_still_starts_after_the_wall_base(self):
        for x in (64, 65, 100, 143):
            self.assertEqual(self.source(x, 32), (1, 48 + (x - 48) % 16, 48))

    def test_other_layers_are_not_extended(self):
        self.assertEqual(self.source(64, 30, 1)[0], 0)


if __name__ == '__main__':
    unittest.main()
