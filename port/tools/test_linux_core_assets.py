"""Exercise the real bundle asset consumer without compiling/running a game."""
import hashlib
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import build_linux
from core_assets import core_assets


class LinuxCoreAssets(unittest.TestCase):
    def test_both_bundle_architectures_ship_exact_shared_core_assets(self):
        # Native dependency scanning/strip is outside this resource-packaging test.
        expected = core_assets()
        self.assertIn('assets/hd/title/words/gTitlePressStartFrame3.png', expected)
        for arch in ('i386', 'armhf'):
            with self.subTest(arch=arch), tempfile.TemporaryDirectory(prefix='linux-assets-') as tmp:
                base = Path(tmp) / 'runtime'
                libs = base / 'sysroot/lib' / build_linux.LIBDIRS[arch]
                libs.mkdir(parents=True)
                for name in ('libSDL3.so.0', build_linux.LOADER[arch]):
                    (libs / name).write_bytes(b'not executed: packaging fixture')
                (base / 'sdl-src-LICENSE.txt').write_text('test runtime placeholder')
                (base / 'packages.txt').write_text('test runtime placeholder')
                dest = Path(tmp) / 'bundle'
                with patch.object(build_linux, 'DLOPEN', []), \
                     patch.object(build_linux, 'needed', return_value=[]), \
                     patch.object(build_linux.subprocess, 'check_call'):
                    build_linux.bundle(arch, base, [], dest)
                missing = [name for name in expected if not (dest / name).is_file()]
                self.assertEqual(missing, [], 'Native bundle omitted shared runtime assets')
                for name, source in expected.items():
                    self.assertEqual(hashlib.sha256((dest / name).read_bytes()).digest(),
                                     hashlib.sha256(source.read_bytes()).digest(), name)


if __name__ == '__main__':
    unittest.main()
