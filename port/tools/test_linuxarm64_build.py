"""Small configuration contract checks; no SDL/game compilation."""
import importlib.util
from pathlib import Path
import unittest

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
        finally:
            build.build_switch.common_flags = original


if __name__ == '__main__':
    unittest.main()
