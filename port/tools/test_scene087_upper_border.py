"""Small C resolver test: extend scene087's upper vine, never its props."""
import ctypes
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]

class Scene087UpperBorder(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        compiler=os.environ.get('CC') or shutil.which('clang') or shutil.which('gcc')
        if not compiler:raise unittest.SkipTest('Set CC to a native C compiler')
        cls.temp=tempfile.TemporaryDirectory(prefix='scene087-border-')
        root=Path(cls.temp.name);code=root/'resolver.c'
        code.write_text('''#include "port_preliminar_v1_tiles.h"
#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#else
#define EXPORT __attribute__((visibility("default")))
#endif
EXPORT int resolve(int bg,int x,int y,int* sx,int* sy) {
    PortPreliminarRecipe p={0};p.v[0]=87;p.v[1]=bg;p.v[4]=32;p.v[5]=32;
    *sx=x;*sy=y;return PortPreliminarPixelSource(&p,x,y,sx,sy);
}
EXPORT int eligible(int x,int y) {
    return PortPreliminar087VineRun(x,y);
}
EXPORT int resolve_tile(int x,int y,int* sx,int* sy) {
    PortPreliminarRecipe p={0};p.v[0]=87;p.v[1]=3;p.v[4]=32;p.v[5]=32;
    *sx=x;*sy=y;return PortPreliminarTileSource(&p,sx,sy);
}
''')
        lib=root/'resolver.dll'
        flags=['-shared','-O0']+(['-fPIC'] if os.name!='nt' else [])
        link=['-lm'] if os.name!='nt' else []
        result=subprocess.run([compiler,*flags,'-I',str(ROOT/'port/include/port'),str(code),'-o',str(lib),*link],capture_output=True,text=True,creationflags=getattr(subprocess,'BELOW_NORMAL_PRIORITY_CLASS',0))
        if result.returncode:raise RuntimeError(result.stdout+result.stderr)
        cls.lib=ctypes.CDLL(str(lib));cls.resolve=cls.lib.resolve
        cls.resolve.argtypes=[ctypes.c_int]*3+[ctypes.POINTER(ctypes.c_int)]*2
        cls.resolve.restype=ctypes.c_int
        cls.eligible=cls.lib.eligible
        cls.eligible.argtypes=[ctypes.c_int,ctypes.c_int]
        cls.eligible.restype=ctypes.c_int
        cls.resolve_tile=cls.lib.resolve_tile
        cls.resolve_tile.argtypes=[ctypes.c_int]*2+[ctypes.POINTER(ctypes.c_int)]*2
        cls.resolve_tile.restype=ctypes.c_int

    @classmethod
    def tearDownClass(cls):
        if os.name!='nt':cls.temp.cleanup()
        else:cls.temp._finalizer.detach() # mapped DLL remains owned by this process

    def source(self,bg,x,y):
        sx,sy=ctypes.c_int(),ctypes.c_int()
        ok=self.resolve(bg,x,y,ctypes.byref(sx),ctypes.byref(sy))
        return ok,sx.value,sy.value

    def test_native_upper_vine_reaches_both_sides(self):
        for x,y in ((-1,56),(-32,72),(-65,89),(256,-72),(287,-88),(320,-104)):
            d=2*y+x;sx=x%32
            with self.subTest(x=x,y=y):
                self.assertEqual(self.source(2,x,y),(1,sx,int((d-sx)/2)))

    def test_central_pumpkins_and_native_art_are_not_remapped(self):
        for bg in (2,3):
            for y in range(256):
                for x in range(256):
                    self.assertEqual(self.source(bg,x,y),(0,x,y))

    def test_upper_extension_is_limited_to_the_vine_band(self):
        for bg in (2,3):
            for x,y in ((-32,50),(-32,90),(256,-96),(256,-40)):
                self.assertEqual(self.source(bg,x,y)[0],0)

    def test_lower_lit_ledge_keeps_its_authored_tile_path(self):
        for bg in (2,3):
            for x,y in ((-16,224),(-64,248),(16,260)):
                self.assertEqual(self.source(bg,x,y),(0,x,y))

    def test_dispatch_catches_any_run_crossing_the_upper_band(self):
        for y in range(-128,257):
            for d in range(80,161):
                x=d-2*y
                self.assertEqual(bool(self.eligible(x,y)),any(96<=2*y+x+j<144 for j in range(8)))

    def test_stone_uses_the_same_two_to_one_boundary_as_the_vine(self):
        for x,y,expected in ((-32,20,(4,0)),(-31,20,(5,0)),(64,-20,(0,10)),(65,-20,(1,10))):
            sx,sy=ctypes.c_int(),ctypes.c_int()
            self.assertEqual(self.resolve_tile(x,y,ctypes.byref(sx),ctypes.byref(sy)),1)
            self.assertEqual((sx.value,sy.value),expected)

    def test_stone_border_and_vine_share_the_original_pixel_phase(self):
        for x,y in ((-1,56),(-32,72),(-65,89),(256,-72),(287,-88),(320,-104)):
            sx=x%32
            self.assertEqual(self.source(3,x,y),(1,sx,int((2*y+x-sx)/2)))

if __name__=='__main__':unittest.main()
