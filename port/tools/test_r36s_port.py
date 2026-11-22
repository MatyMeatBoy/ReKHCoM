"""ROM-free checks of the ARM64 handheld build, ZIP and launch entry."""
import importlib.util
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import zipfile


ROOT = Path(__file__).resolve().parents[2]
PORT = ROOT / 'port/linux/r36s'


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class R36SPortTest(unittest.TestCase):
    def setUp(self):
        self.scratch = tempfile.TemporaryDirectory(prefix='r36s port ')
        self.addCleanup(self.scratch.cleanup)
        self.temp = Path(self.scratch.name)

    def test_host_build_uses_armv8_focal_and_only_ships_sdl(self):
        build = load('r36s_build', ROOT / 'port/linuxarm64/build_linuxarm64.py')
        sysroot, sdl = self.temp / 'focal', self.temp / 'sdl'
        (sysroot / 'usr/lib/gcc/aarch64-linux-gnu/10').mkdir(parents=True)
        (sysroot / 'usr/lib/gcc/aarch64-linux-gnu/10/crtbegin.o').touch()
        (sdl / 'lib').mkdir(parents=True)
        (sdl / 'include/SDL3').mkdir(parents=True)
        (sdl / 'include/SDL3/SDL.h').touch()
        (sdl / 'lib/libSDL3.so.0').write_bytes(b'synthetic SDL')
        with patch.object(build, 'OUT', self.temp / 'out'), \
             patch.object(build.subprocess, 'check_call') as run:
            build.build_host(sysroot=sysroot, sdl=sdl)
        args = run.call_args.args[0]
        self.assertIn('-mcpu=cortex-a35', args)
        self.assertIn(f'--sysroot={sysroot}', args)
        self.assertEqual([p.name for p in (self.temp / 'out/libarm64').iterdir()], ['libSDL3.so.0'])

    def test_elf_validator_rejects_wrong_architecture_and_newer_glibc(self):
        package = load('r36s_package', PORT / 'package.py')
        report = ('Class: ELF64\nMachine: AArch64\nData: 2\'s complement, little endian\n'
                  '[Requesting program interpreter: /lib/ld-linux-aarch64.so.1]\n'
                  '(NEEDED) Shared library: [libSDL3.so.0]\nName: GLIBC_2.31')
        with patch.object(package.subprocess, 'check_output', return_value=report) as readelf:
            package.audit_elf(Path('rekhcom-arm64'), 'llvm-readelf', host=True)
            for old, new in (('AArch64', 'X86-64'), ('little endian', 'big endian'),
                             ('2.31', '2.34'), ('2.31', 'PRIVATE'),
                             ('/lib/ld-linux-aarch64.so.1', '/lib/ld-linux.so.2'),
                             ('libSDL3.so.0', 'libSDL2-2.0.so.0')):
                readelf.return_value = report.replace(old, new)
                with self.assertRaises(ValueError, msg=new):
                    package.audit_elf(Path('rekhcom-arm64'), 'llvm-readelf', host=True)
            readelf.return_value = report + '\n(NEEDED) Shared library: [libgbm.so.1]'
            with self.assertRaisesRegex(ValueError, 'libgbm.so.1'):
                package.audit_elf(Path('rekhcom-arm64'), 'llvm-readelf', host=True)

    def test_package_contains_only_expected_runtime_entries(self):
        package = load('r36s_package', PORT / 'package.py')
        runtime = self.temp / 'runtime'
        for name in package.RUNTIME_FILES:
            entry = runtime / name
            entry.parent.mkdir(parents=True, exist_ok=True)
            entry.write_bytes(b'fixture')
        (runtime / 'roms').mkdir()
        (runtime / 'roms/private.gba').write_bytes(b'must stay outside the archive')
        license_file = self.temp / 'SDL-LICENSE.txt'
        license_file.write_text('SDL license fixture', encoding='utf-8')
        archive = self.temp / 'rekhcom-r36s.zip'
        with patch.object(package, 'audit_elf', return_value='synthetic ELF report'), \
             patch.object(package, 'audit_guest'), patch.object(package, 'install_common_assets'):
            package.create_package(runtime, archive, license_file, 'llvm-readelf')
        with zipfile.ZipFile(archive) as payload:
            names = set(payload.namelist())
            self.assertIn('ReKHCoM R36S.sh', names)
            self.assertIn('rekhcomr36s/roms/README.txt', names)
            self.assertNotIn('rekhcomr36s/roms/private.gba', names)
            for name in ('ReKHCoM R36S.sh', 'rekhcomr36s/ReKHCoM.sh', 'rekhcomr36s/binarm64/rekhcom-arm64'):
                self.assertEqual(payload.getinfo(name).external_attr >> 16 & 0o777, 0o755)
            self.assertNotIn(b'\r', payload.read('ReKHCoM R36S.sh'))
        with self.assertRaises(FileExistsError):
            package.create_package(runtime, archive, license_file, 'llvm-readelf')

    def test_launcher_uses_portmaster_mapping_and_preserves_game_exit(self):
        bash = shutil.which('bash') if os.name != 'nt' else Path('C:/Program Files/Git/bin/bash.exe')
        if not bash or not Path(bash).is_file():
            self.skipTest('bash required')
        root = self.temp / 'roms2 ports'
        game = root / 'rekhcomr36s'
        portmaster = root / 'PortMaster'
        game.mkdir(parents=True)
        portmaster.mkdir()
        launcher = root / 'ReKHCoM R36S.sh'
        launcher.write_bytes((PORT / 'ReKHCoM.sh').read_bytes().replace(b'\r\n', b'\n'))
        (portmaster / 'control.txt').write_bytes(b'''get_controls() { sdl_controllerconfig=mapped; }
pm_finish() { printf 'finished\\n' >> "$GAMEDIR/result"; }
''')
        (game / 'ReKHCoM.sh').write_bytes(b'''printf '%s\\n' "$SDL_VIDEO_DRIVER" "$SDL_AUDIO_DRIVER" "$SDL_GAMECONTROLLERCONFIG" "$@" > result
exit 17
''')
        for video, mapping, expected in (('', '', ['kmsdrm', 'alsa', 'mapped']),
                                         ('dummy', 'override', ['dummy', 'alsa', 'override'])):
            env = dict(os.environ, SDL_VIDEO_DRIVER=video, SDL_AUDIO_DRIVER='', SDL_GAMECONTROLLERCONFIG=mapping)
            result = subprocess.run([str(bash), str(launcher), '--region', 'jp'], cwd=self.temp,
                                    env=env, capture_output=True, text=True)
            self.assertEqual(result.returncode, 17, result.stderr)
            self.assertEqual((game / 'result').read_text().splitlines(),
                             [*expected, '--force64', '--region', 'jp', 'finished'])


if __name__ == '__main__':
    unittest.main()
