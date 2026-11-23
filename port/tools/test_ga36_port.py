"""ROM-free launcher, ARMv7 ABI audit and GA36 package checks."""
import importlib.util
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import zipfile


ROOT = Path(__file__).resolve().parents[2]
PORT = ROOT / 'port/linux/ga36'


def load_package():
    spec = importlib.util.spec_from_file_location('ga36_package', PORT / 'package.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class Ga36PortTest(unittest.TestCase):
    def setUp(self):
        self.scratch = tempfile.TemporaryDirectory(prefix='ga36 port ')
        self.addCleanup(self.scratch.cleanup)
        self.temp = Path(self.scratch.name)

    def test_audit_checks_arm_eabi_float_and_kernel_floor(self):
        package = load_package()
        image = self.temp / 'loader'
        header = bytearray(52)
        header[:7] = b'\x7fELF\x01\x01\x01'
        struct.pack_into('<H', header, 18, 40)
        struct.pack_into('<I', header, 36, 0x05000400)
        image.write_bytes(header)
        with patch.object(package.subprocess, 'check_output', return_value='OS: Linux, ABI: 3.2.0') as readelf:
            package.audit_elf(image, 'llvm-readelf', require_kernel_abi=True)
            readelf.return_value = '[Requesting program interpreter: lib/rekhcom]'
            with self.assertRaisesRegex(ValueError, 'libarmhf/rekhcom'):
                package.audit_elf(image, 'llvm-readelf', interpreter=True)
            readelf.return_value = '[Requesting program interpreter: libarmhf/rekhcom]'
            package.audit_elf(image, 'llvm-readelf', interpreter=True)
            readelf.return_value = 'OS: Linux, ABI: 4.14.0'
            with self.assertRaisesRegex(ValueError, 'kernel ABI'):
                package.audit_elf(image, 'llvm-readelf', require_kernel_abi=True)
            struct.pack_into('<I', header, 36, 0x05000200)
            image.write_bytes(header)
            with self.assertRaisesRegex(ValueError, 'hard-float'):
                package.audit_elf(image, 'llvm-readelf')

    def test_packager_stages_armhf_layout_and_executable_bits(self):
        package = load_package()
        archive = self.temp / 'ga36.zip'

        def bundle(arch, base, versions, game):
            self.assertEqual(arch, 'armhf')
            for region in versions:
                path = game / f'bin/khcom_{region}'
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(b'game fixture')
            (game / 'lib').mkdir()
            (game / 'lib/rekhcom').write_bytes(b'loader fixture')
            (game / 'lib/libc.so.6').write_bytes(b'libc fixture')
            (game / 'ReKHCoM.sh').write_bytes(b'#!/bin/sh\n')
            (game / 'roms').mkdir()

        with patch.object(package, 'bundle', side_effect=bundle), patch.object(package, 'audit_elf'):
            package.create_package(self.temp, archive, 'llvm-readelf')
        with zipfile.ZipFile(archive) as z:
            self.assertIn('rekhcomga36/roms/README.txt', z.namelist())
            self.assertIn('rekhcomga36/libarmhf/native-interpreter', z.namelist())
            self.assertFalse(any(name.endswith(('.gba', '.sav')) for name in z.namelist()))
            for name in ('ReKHCoM GA36.sh', 'rekhcomga36/binarmhf/khcom_jp', 'rekhcomga36/libarmhf/rekhcom'):
                self.assertEqual(z.getinfo(name).external_attr >> 16 & 0o777, 0o755)
            self.assertNotIn('rekhcomga36/lib/rekhcom', z.namelist())
        with self.assertRaises(FileExistsError):
            package.create_package(self.temp, archive, 'llvm-readelf')

    def test_launch_profile_and_required_framebuffer(self):
        shell = shutil.which('sh') if os.name != 'nt' else Path('C:/Program Files/Git/bin/sh.exe')
        if not shell or not Path(shell).is_file():
            self.skipTest('a POSIX shell is required')
        game = self.temp / 'rekhcomga36'
        game.mkdir()
        launcher = self.temp / 'ReKHCoM GA36.sh'
        launcher.write_bytes((PORT / 'ReKHCoM.sh').read_bytes().replace(b'\r\n', b'\n'))
        (game / 'ReKHCoM.sh').write_bytes(b'''printf '%s\\n' "$KHCOM_FBDEV" "$SDL_VIDEO_DRIVER" "$SDL_AUDIO_DRIVER" "$KHCOM_TALL_FIT" "$KHCOM_FIXED_FULLSCREEN" "$KHCOM_CONFIG" "$@" > result
exit 17
''')
        shim = self.temp / 'uname'
        shim.write_bytes(b'#!/bin/sh\nprintf "%s\\n" "$TEST_ARCH"\n')
        shim.chmod(0o755)
        posix_temp = subprocess.check_output([str(shell), '-c', 'cd "$1" && pwd -P', 'sh', str(self.temp)], text=True).strip()
        env = dict(os.environ, TEST_ARCH='armv7l', KHCOM_FBDEV='', SDL_VIDEO_DRIVER='', SDL_AUDIO_DRIVER='',
                   KHCOM_TALL_FIT='', KHCOM_CONFIG='zoom=6')
        result = subprocess.run([str(shell), '-c', 'export PATH="$1:$PATH"; shift; exec sh "$@"', 'sh',
                                 posix_temp, str(launcher), '--region', 'jp'], env=env, capture_output=True, text=True)
        self.assertEqual(result.returncode, 17, result.stderr)
        self.assertEqual((game / 'result').read_text().splitlines(), [
            '', 'offscreen', 'alsa', '0', '1', 'fullscreen=1;view_mode=4;aspect=5;zoom=4;battle_zoom=4;filter=0;zoom=6',
            '--force32', '--region', 'jp',
        ])
        env['KHCOM_FBDEV'] = posix_temp + '/missing-fb'
        result = subprocess.run([str(shell), '-c', 'export PATH="$1:$PATH"; shift; exec sh "$@"', 'sh',
                                 posix_temp, str(launcher)], env=env, capture_output=True, text=True)
        self.assertEqual(result.returncode, 2)
        self.assertIn('framebuffer', result.stderr)


if __name__ == '__main__':
    unittest.main()
