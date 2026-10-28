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
        for version, expected in [('2.2', '02.02'), ('1.9.16', '01.09'), ('2.2-pre3', '02.02')]:
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

    def test_current_header_reaches_package(self):
        header = (ROOT / 'port/include/port/port_version.h').read_text(encoding='utf-8')
        major, minor = re.search(r'#define REKHCOM_VERSION\s+"v(\d+)\.(\d+)', header).groups()
        expected = f'{int(major):02d}.{int(minor):02d}'
        script = build.package_script(['eu'])
        self.assertIn('APP_VER=' + expected, script)
        self.assertIn('VERSION=' + expected, script)


if __name__ == '__main__':
    unittest.main()
