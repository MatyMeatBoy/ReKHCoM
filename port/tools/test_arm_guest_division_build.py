"""Exercise the actual ARM guest producer with real Clang and isolated sources."""
import argparse
import contextlib
import importlib.util
import io
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'port/switch'))
import build_switch as builder

CC=builder.CLANG
class ProducerTests(unittest.TestCase):
    def setUp(self):
        self.temporary=tempfile.TemporaryDirectory(prefix='arm-guest-division-')
        self.root=Path(self.temporary.name)
        self.original=(builder.ROOT,builder.CLANG,dict(os.environ))
        self.original_lower=builder.lower_cartridge_division
        self.original_fingerprint=builder.division_fingerprint
        builder.ROOT=self.root;builder.CLANG=str(CC)
        os.environ.pop('REBUILD',None)
        (self.root/'src').mkdir();(self.root/'port').mkdir()
        self.header=self.root/'src/local header.h';self.header.write_text('#define VALUE 7\n')
        self.game=self.root/'src/game.c';self.game.write_text('#include "local header.h"\nint game(int b){return VALUE%b;}\n')
        self.host=self.root/'port/host.c';self.host.write_text('int host(int n,int d){return n%d;}\n')
        self.flags=['--target=armv7a-none-eabihf','-mcpu=cortex-a9','-mthumb','-O2','-ffreestanding','-fwrapv']
        self.obj=self.root/'out/game.o';self.hostobj=self.root/'out/host.o'
        self.units=[(self.game,self.obj,self.flags),(self.host,self.hostobj,self.flags)]
        self.commands=[]
        self.real_run=subprocess.run
        def capture(command,*args,**kwargs):
            self.commands.append([str(x) for x in command])
            return self.real_run(command,*args,**kwargs)
        builder.subprocess.run=capture
    def tearDown(self):
        builder.subprocess.run=self.real_run
        builder.lower_cartridge_division=self.original_lower
        builder.division_fingerprint=self.original_fingerprint
        builder.ROOT,builder.CLANG,environment=self.original
        os.environ.clear();os.environ.update(environment)
        self.temporary.cleanup()
    def compile_count(self):
        return sum('-c' in x or '-emit-llvm' in x for x in self.commands)
    def test_game_only_lowering_and_cache(self):
        builder.compile_all(1,self.units)
        ir=self.obj.with_suffix('.gba.ll')
        self.assertTrue(ir.is_file(),'game producer did not emit cartridge LLVM IR')
        self.assertIn('@__rekhcom_gba_srem32',ir.read_text())
        self.assertFalse(self.hostobj.with_suffix('.gba.ll').exists())
        self.assertEqual(self.compile_count(),3)
        first=self.compile_count();builder.compile_all(1,self.units)
        self.assertEqual(self.compile_count(),first,'same inputs should reuse objects')
        self.obj.with_suffix('.gba-ir.sha256').unlink()
        builder.compile_all(1,self.units)
        self.assertEqual(self.compile_count(),first+2,'old game object lacking lowering marker must rebuild')
    def test_header_dependencies_and_flags(self):
        builder.compile_all(1,self.units)
        before=self.compile_count()
        self.assertIn('local\\ header.h',self.obj.with_suffix('.d').read_text())
        self.assertNotIn('gba.ll',self.obj.with_suffix('.d').read_text(),'backend must not overwrite frontend dependencies')
        self.header.write_text('#define VALUE 11\n')
        future=self.obj.stat().st_mtime+2;os.utime(self.header,(future,future))
        builder.compile_all(1,self.units)
        self.assertEqual(self.compile_count(),before+2)
        # Restore ordinary times after the forced newer-header check.
        os.utime(self.header,(0,0))
        before=self.compile_count()
        changed=[(self.game,self.obj,[*self.flags,'-DCONFIG_CHANGED=1']),self.units[1]]
        builder.compile_all(1,changed)
        self.assertEqual(self.compile_count(),before+2,'changed game flags must invalidate cache')
    def test_failed_compile_cannot_mark_cache_valid(self):
        builder.compile_all(1,self.units)
        self.assertTrue(self.obj.with_suffix('.gba-ir.sha256').is_file())
        self.game.write_text('not valid C\n')
        os.utime(self.game,(self.obj.stat().st_mtime+2,)*2)
        with contextlib.redirect_stdout(io.StringIO()),contextlib.redirect_stderr(io.StringIO()),self.assertRaises(SystemExit):builder.compile_all(1,self.units)
        self.assertFalse(self.obj.with_suffix('.gba-ir.sha256').exists())
    def test_helper_revision_invalidates_game_cache(self):
        builder.compile_all(1,self.units)
        before=self.compile_count()
        builder.division_fingerprint=lambda:'test-new-helper-revision'
        builder.compile_all(1,self.units)
        self.assertEqual(self.compile_count(),before+2)
    def test_source_content_changed_with_preserved_mtime(self):
        builder.compile_all(1,self.units)
        before=self.compile_count();saved=self.game.stat()
        self.game.write_text('#include "local header.h"\nint game(int b){return VALUE%b+1;}\n')
        os.utime(self.game,ns=(saved.st_atime_ns,saved.st_mtime_ns))
        builder.compile_all(1,self.units)
        self.assertEqual(self.compile_count(),before+2,'source identity must not rely on copy timestamps')
    def test_backend_failure_preserves_header_deps_and_invalidates_cache(self):
        builder.compile_all(1,self.units)
        builder.lower_cartridge_division=lambda text:'invalid LLVM IR\n'
        os.environ['REBUILD']='1'
        with contextlib.redirect_stdout(io.StringIO()),contextlib.redirect_stderr(io.StringIO()),self.assertRaises(SystemExit):builder.compile_all(1,[self.units[0]])
        self.assertFalse(self.obj.with_suffix('.gba-ir.sha256').exists())
        self.assertIn('local\\ header.h',self.obj.with_suffix('.d').read_text())

if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--cc',type=Path)
    args=parser.parse_args();CC=args.cc or CC
    result=unittest.TextTestRunner(verbosity=2).run(unittest.defaultTestLoader.loadTestsFromTestCase(ProducerTests))
    raise SystemExit(0 if result.wasSuccessful() else 1)
