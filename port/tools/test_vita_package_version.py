"""Vita packaging commands only; no SDK, VM, game inputs or compiler."""
import importlib.util
from pathlib import Path
import re
import shlex
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('build_vita', ROOT / 'port/vita/build_vita.py')
build = importlib.util.module_from_spec(spec)
spec.loader.exec_module(build)


class PackageVersionTests(unittest.TestCase):
    def test_release_version_reaches_sfo_command(self):
        for version, expected in [('2.2', '02.20'), ('2.2.5', '02.25'), ('2.3', '02.30'), ('99.9.9', '99.99')]:
            with self.subTest(version=version), patch.object(build.build_switch, 'version', return_value=version):
                script = build.package_script(['eu', 'us', 'jp'])
                command = shlex.split(next(line for line in script.splitlines()
                                           if line.startswith('vita-mksfoex ')))
                self.assertIn('APP_VER=' + expected, command)
                self.assertIn('VERSION=' + expected, command)
                self.assertIn('TITLE_ID=RKHC00001', command)
                self.assertEqual(command[-2:], ['ReKHCoM', 'param.sfo'])
                self.assertIn('for r in eu us jp;', script)
                self.assertNotIn('APPVERSION', script)

    def test_unrepresentable_release_stops_before_package_commands(self):
        for version in ('1.9.16','2.2-pre3','100.0.0','2.10.0','2.2.10'):
            with self.subTest(version=version), patch.object(build.build_switch, 'version', return_value=version):
                with self.assertRaises(ValueError):build.package_script(['eu'])

    def test_current_header_reaches_package(self):
        header = (ROOT / 'port/include/port/port_version.h').read_text(encoding='utf-8')
        release = re.search(r'#define REKHCOM_VERSION\s+"([^"]+)"', header).group(1)
        from sony_package_version import sfo_release_version
        expected = sfo_release_version(release)
        script = build.package_script(['eu'])
        self.assertIn('APP_VER=' + expected, script)
        self.assertIn('VERSION=' + expected, script)


if __name__ == '__main__':
    unittest.main()
