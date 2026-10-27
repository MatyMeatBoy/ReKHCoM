"""Small configuration contract checks; no SDL/game compilation."""
import importlib.util
from pathlib import Path
import unittest
import sys
import tempfile
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('linuxarm64', ROOT / 'port/linuxarm64/build_linuxarm64.py')
build = importlib.util.module_from_spec(spec)
spec.loader.exec_module(build)


class LinuxArm64BuildTest(unittest.TestCase):
    def test_linux_guest_configuration(self):
        original = build.build_switch.common_flags
        try:
            build.configure_guest()
            flags = build.build_switch.common_flags()
            self.assertNotIn('-DPORT_SWITCH', flags)
            self.assertIn('-DPORT_LINUX64_GUEST', flags)
            self.assertIn('--target=arm64_32-apple-watchos', flags)
            self.assertFalse(build.build_switch.PPU_THREADS)
            self.assertFalse(build.build_switch.TOUCH_CONTROLS)
            self.assertEqual(build.build_switch.BUILD, ROOT / 'build/linuxarm64')
            build.configure_guest()
            self.assertEqual(build.build_switch.common_flags().count('-DPORT_LINUX64_GUEST'), 1)
        finally:
            build.build_switch.common_flags = original

    def test_host_target_sysroot_and_output_contract_without_building(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'base/sdl/lib').mkdir(parents=True)
            (root / 'base/sdl/lib/libSDL3.so.0').write_bytes(b'synthetic runtime')
            with patch.object(build, 'BASE', root / 'base'), patch.object(build, 'OUT', root / 'out'), \
                 patch.object(build.subprocess, 'check_call') as run:
                build.build_host()
            args = run.call_args.args[0]
            self.assertIn('--target=aarch64-linux-gnu', args)
            self.assertIn('--sysroot=' + str(root / 'base/sysroot'), args)
            self.assertIn('-Wl,-rpath,$ORIGIN/../libarm64', args)
            self.assertEqual(Path(args[-1]), root / 'out/binarm64/rekhcom-arm64')
            self.assertEqual((root / 'out/libarm64/libSDL3.so.0').read_bytes(), b'synthetic runtime')
            self.assertNotIn('-DPORT_SWITCH', args)

    def test_missing_arm64_sdl_does_not_invoke_compiler(self):
        with tempfile.TemporaryDirectory() as directory, patch.object(build, 'BASE', Path(directory)), \
             patch.object(build.subprocess, 'check_call') as run:
            with self.assertRaisesRegex(SystemExit, 'ARM64 SDL3 missing'):
                build.build_host()
            run.assert_not_called()

    def test_arm64_preparation_is_explicit_not_new_default_work(self):
        spec = importlib.util.spec_from_file_location('arm64_prepare_probe', ROOT / 'port/tools/linux_prepare.py')
        prep = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(prep)
        with patch.object(sys, 'argv', ['linux_prepare.py']), patch.object(prep, 'prepare') as run:
            prep.main()
        self.assertEqual([c.args[0] for c in run.call_args_list], ['i386', 'amd64', 'armhf'])
        with patch.object(sys, 'argv', ['linux_prepare.py', '--arch', 'arm64', '--jobs', '1']), \
             patch.object(prep, 'prepare') as run:
            prep.main()
        self.assertEqual(run.call_args.args[0], 'arm64')
        self.assertEqual(run.call_args.args[2], 1)


if __name__ == '__main__':
    unittest.main()
