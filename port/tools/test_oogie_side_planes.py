"""Compile the real Oogie renderer against deterministic, ROM-free tile fixtures."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]

class OogieSidePlanes(unittest.TestCase):
    def test_native_room_red_path_and_new_sides(self):
        compiler = os.environ.get('CC') or shutil.which('gcc')
        local = ROOT.parent.parent / 'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/x86_64-w64-mingw32-gcc.exe'
        if not compiler and local.is_file():
            compiler = str(local)
        if not compiler:
            self.skipTest('C compiler unavailable')
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            (directory / 'port').mkdir()
            (directory / 'port/port.h').write_text('#include <stdint.h>\nextern uint16_t testPalette[256];\n#define GBA_PLTT ((uintptr_t)testPalette)\n')
            (directory / 'port/port_battle_backdrop.h').write_text('')
            source = '#include <stdint.h>\n#include <assert.h>\nuint16_t testPalette[256];\n'
            source += 'const uint16_t gBosBoogieBgMaps[2048]={' + ','.join(['0x9000'] * 2048) + '};\n'
            source += 'const uint16_t gDefaultBgMap[1024]={0};\n'
            # One fixture tile cycles native white palette slots150..153; no cartridge data.
            source += 'const uint8_t gBosBoogieBgTiles[32512]={' + ','.join(['0x76','0x98'] * 16) + '};\n'
            source += '#include "' + (ROOT / 'port/src/port_oogie.c').as_posix() + '"\n'
            source += r'''
int main(void) {
    int x,y,oldWidth=0,newWidth=0;uint16_t color;
    for(x=0;x<256;x++)testPalette[x]=(uint16_t)x;
    for(y=192;y<472;y++)for(x=256;x<496;x++) {
        color=12345;assert(!PortOogieHallPixel(x,y,376,332,&color));assert(color==12345);
    }
    /* The approved centre wins over every side/figure pixel. */
    for(y=472;y<800;y++)for(x=64;x<688;x++) {
        int index=PathIndex(x,y);
        if(index<0)continue;
        assert(PortOogieHallPixel(x,y,376,332,&color));
        assert(color==(index?testPalette[index]:0x8000));
    }
    assert(PortOogieHallPixel(230,500,376,332,&color)&&color==159);
    assert(PortOogieHallPixel(520,500,376,332,&color)&&color==159);
    assert(PortOogieHallPixel(187,500,376,332,&color)&&color>=150&&color<=153);
    assert(PortOogieHallPixel(250,420,376,332,&color)&&color==146);
    assert(PortOogieHallPixel(503,445,376,332,&color)&&color==146);
    /* The reviewed straight front rim reaches both viewport edges. */
    for(y=400;y<408;y++)for(x=-1000;x<2000;x++)
        assert(FloorMotifIndex(x,y)==RoomIndex(ROOM_X,y));
    for(y=408;y<800;y++) {
        assert(PortOogieHallPixel(-1000,y,376,332,&color)&&color==0x8000);
        assert(PortOogieHallPixel(2000,y,376,332,&color)&&color==0x8000);
    }
    for(x=0;x<280;x++) {
        oldWidth+=FloorMotifIndex(x,500)==159;
        newWidth+=FloorMotifIndex(x,600)==159;
    }
    assert(oldWidth>0&&newWidth>=oldWidth);
    testPalette[159]=1023;
    assert(PortOogieHallPixel(230,500,376,332,&color)&&color==1023);
    return 0;
}
'''
            check = directory / 'check.c'
            check.write_text(source)
            executable = directory / ('check.exe' if os.name == 'nt' else 'check')
            subprocess.run([compiler,'-std=c11','-I',str(directory),'-I',str(ROOT/'port/include'),str(check),'-o',str(executable)],check=True,capture_output=True)
            result = subprocess.run([str(executable)],capture_output=True,text=True)
            self.assertEqual(result.returncode,0,result.stderr)

if __name__ == '__main__':
    unittest.main()
