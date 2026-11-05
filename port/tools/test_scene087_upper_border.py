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
__declspec(dllexport) int resolve(int bg,int x,int y,int* sx,int* sy) {
    PortPreliminarRecipe p={0};p.v[0]=87;p.v[1]=bg;p.v[4]=32;p.v[5]=32;
    *sx=x;*sy=y;return PortPreliminarPixelSource(&p,x,y,sx,sy);
}
''')
        lib=root/'resolver.dll'
        result=subprocess.run([compiler,'-shared','-O0','-I',str(ROOT/'port/include/port'),str(code),'-o',str(lib)],capture_output=True,text=True,creationflags=getattr(subprocess,'BELOW_NORMAL_PRIORITY_CLASS',0))
        if result.returncode:raise RuntimeError(result.stdout+result.stderr)
        cls.lib=ctypes.CDLL(str(lib));cls.resolve=cls.lib.resolve
        cls.resolve.argtypes=[ctypes.c_int]*3+[ctypes.POINTER(ctypes.c_int)]*2
        cls.resolve.restype=ctypes.c_int

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
        for x,y in ((-32,50),(-32,90),(256,-96),(256,-40)):
            self.assertEqual(self.source(2,x,y)[0],0)

    def test_lower_lit_ledge_keeps_its_existing_pixel_phase(self):
        for x,y in ((-16,224),(-64,248),(16,260)):
            d=2*y+x;sx=32+(x-32)%16
            self.assertEqual(self.source(2,x,y),(1,sx,int((d-sx)/2)))

if __name__=='__main__':unittest.main()
