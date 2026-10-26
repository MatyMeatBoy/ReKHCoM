"""Linux packages must contain the x86-64 host, not depend on a warm build tree."""
import importlib.util
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

TOOLS = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location('build_linux', TOOLS / 'build_linux.py')
linux = importlib.util.module_from_spec(spec)
spec.loader.exec_module(linux)


class Linux64PackageTests(unittest.TestCase):
    def test_package_builds_linux64_before_packaging_with_requested_jobs(self):
        with patch.object(sys, 'argv', ['build_linux.py', '--package', 'v2.0', '--jobs', '2']), \
             patch.object(linux, 'configure'), patch.object(linux, 'package') as package, \
             patch.object(linux.subprocess, 'check_call') as run:
            linux.main()
        linux64 = [c for c in run.call_args_list if 'build_linux64.py' in str(c.args[0])]
        self.assertEqual(len(linux64), 1, 'Linux packaging must build its x86-64 host and all three images')
        command = linux64[0].args[0]
        self.assertEqual(command[command.index('--jobs') + 1], '2')
        package.assert_called_once()

    def test_package_rejects_missing_linux64_before_making_archives(self):
        with tempfile.TemporaryDirectory() as folder, patch.object(linux, 'ROOT', Path(folder)), \
             patch.object(linux, 'bundle', side_effect=AssertionError('must reject missing Linux64 before bundling')):
            header = Path(folder) / 'port/include/port/port_zoom.h'
            header.parent.mkdir(parents=True)
            header.write_text('#define PORT_ZOOM_CONFIG_VERSION 2\n')
            with self.assertRaisesRegex(SystemExit, 'Linux64'):
                linux.package('v2.0', Path(folder), Path(folder))
            self.assertFalse((Path(folder) / 'build/releases').exists())


if __name__ == '__main__':
    unittest.main()
