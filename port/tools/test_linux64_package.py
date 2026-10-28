"""Linux packages must contain the x86-64 host, not depend on a warm build tree."""
import importlib.util
from pathlib import Path
import sys
import tempfile
import tarfile
import zipfile
import runpy
import ast
import re
import json
import unittest
from unittest.mock import patch

TOOLS = Path(__file__).resolve().parent
sys.path.insert(0, str(TOOLS))
spec = importlib.util.spec_from_file_location('build_linux', TOOLS / 'build_linux.py')
linux = importlib.util.module_from_spec(spec)
spec.loader.exec_module(linux)


class Linux64PackageTests(unittest.TestCase):
    def test_unified_archive_and_release_staging_contract(self):
        """Exercise real archive writers/stager, with ROM-free fake binaries."""
        with tempfile.TemporaryDirectory() as folder, patch.object(linux, 'ROOT', Path(folder)):
            root = Path(folder)
            def write(name, data=b'fixture'):
                path = root / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_bytes(data)
            for name in ['bin64/rekhcom64', 'lib64/libSDL3.so.0'] + [f'bin64/guest-{r}.bin' for r in ('eu', 'us', 'jp')]:
                write('build/linux64/out/' + name)
            write('port/include/port/port_zoom.h', b'#define PORT_ZOOM_CONFIG_VERSION 2\n')
            write('port/include/port/port_version.h', b'#define REKHCOM_VERSION "v2.1"\n')
            write('port/linux/ReKHCoM Launcher.sh', b'#!/bin/sh\n')
            write('port/linux/launcher/rekhcom_launcher.py', b'# launcher fixture\n')
            write('port/data/hd_sprites.tsv', b'fixture\n')
            write('port/linux/LINUX.md')
            write('port/linux/Setup Steam Deck.sh', b'#!/bin/sh\n')
            write('port/linux/anbernic/ReKHCoM.sh', b'#!/bin/sh\n')
            # Old outputs are retained; generating a new version must not delete them.
            write('build/releases/ReKHCoM-v2.0-steamdeck.tar.gz', b'old release')
            def fake_bundle(arch, base, versions, dest):
                for region in versions:
                    path = dest / 'bin' / f'khcom_{region}'
                    path.parent.mkdir(parents=True, exist_ok=True)
                    path.write_bytes(b'32-bit fixture')
                (dest / 'roms').mkdir()
                (dest / 'lib').mkdir()
                (dest / 'lib/rekhcom').write_bytes(b'loader')
                (dest / 'ReKHCoM.sh').write_bytes(b'#!/bin/sh\n')
                linux.install_runtime_version(dest)
                if arch == 'i386':
                    import shutil
                    shutil.copytree(root / 'build/linux64/out', dest, dirs_exist_ok=True)
            with patch.object(linux, 'bundle', side_effect=fake_bundle) as bundle:
                archives = linux.package('v2.1', root, root)
            expected = {'ReKHCoM-v2.1-linux-x86-x64-desktop-steamdeck.tar.gz',
                        'ReKHCoM-v2.1-anbernic-beta.zip'}
            self.assertEqual({p.name for p in archives}, expected)
            self.assertEqual(bundle.call_count, 2)
            self.assertEqual((root / 'build/releases/ReKHCoM-v2.0-steamdeck.tar.gz').read_bytes(), b'old release')
            with tarfile.open(next(p for p in archives if p.name.endswith('.tar.gz'))) as tar:
                members = {m.name: m for m in tar.getmembers()}
                for region in ('eu', 'us', 'jp'):
                    self.assertIn(f'bin64/guest-{region}.bin', members)
                for name in ['ReKHCoM.sh', 'lib/rekhcom', 'bin64/rekhcom64', 'lib64/libSDL3.so.0'] + [f'bin/khcom_{r}' for r in ('eu', 'us', 'jp')]:
                    self.assertEqual(members[name].mode, 0o755, name)
                self.assertEqual(members['LINUX.md'].mode, 0o644)
                self.assertEqual(members['Setup Steam Deck.sh'].mode, 0o755)
                self.assertEqual(members['roms'].mode, 0o755)
                self.assertEqual(tar.extractfile('ReKHCoM.version').read(), b'v2.1\n')
                self.assertNotIn('kh_com.default.ini', members, 'Deck detection must not ship a forced preset')
                self.assertFalse(any(Path(n).suffix in ('.gba', '.sav', '.ini', '.khst') for n in members))
                self.assertFalse(any('arm64' in n for n in members))
            with zipfile.ZipFile(next(p for p in archives if p.name.endswith('.zip'))) as archive:
                self.assertEqual(archive.getinfo('ReKHCoM.sh').external_attr >> 16 & 0o777, 0o755)
                self.assertEqual(archive.read('rekhcom/ReKHCoM.version'), b'v2.1\n')
            # Run the production release stager against this fixture repository.
            write('port/tools/stage_release.py', (TOOLS / 'stage_release.py').read_bytes())
            write('build/releases/ReKHCoM-v2.1-android-beta.apk')
            write('build/releases/ReKHCoM-v2.1-windows-x86-x64.zip', b'combined Windows fixture')
            write('build/releases/ReKHCoM-v2.1-windows-x86.zip', b'legacy Windows fixture')
            write('build/releases/PPSA98511.zip')
            with patch.object(sys, 'argv', ['stage_release.py', 'v2.1']):
                runpy.run_path(str(root / 'port/tools/stage_release.py'), run_name='__main__')
            staged = {p.name for p in (root / 'build/publish/v2.1/main').iterdir()}
            self.assertIn('ReKHCoM-v2.1-linux-x86-x64-desktop-steamdeck.tar.gz', staged)
            self.assertIn('ReKHCoM-v2.1-linux-armhf-anbernic-beta.zip', staged)
            self.assertIn('ReKHCoM-v2.1-android-beta.apk', staged)
            self.assertIn('ReKHCoM-v2.1-windows-x86-x64.zip', staged)
            self.assertNotIn('ReKHCoM-v2.1-windows-x86.zip', staged)
            self.assertFalse(any(n.endswith('-steamdeck-i386.tar.gz') for n in staged))
            # The published names must still give Quiver exactly one Linux pick.
            source = ast.parse((TOOLS / 'release_check.py').read_text(encoding='utf-8'))
            functions = {'any_of', 'auxiliary', 'excluded', 'windows', 'linux_x64',
                         'android', 'asset_matches_filter', 'quiver'}
            statements = [node for node in source.body if
                          isinstance(node, ast.FunctionDef) and node.name in functions or
                          isinstance(node, ast.Assign) and any(isinstance(t, ast.Name) and t.id == 'NON_WIN' for t in node.targets)]
            checks = []
            namespace = {'re': re, 'WIN_FILTER': json.loads((TOOLS.parents[1] / 'docs/quiver/submission-draft.json').read_text(encoding='utf-8'))['launcher']['releaseAssetFilter'],
                         'check': lambda ok, text: checks.append((ok, text))}
            exec(compile(ast.Module(body=statements, type_ignores=[]), 'release_check.py', 'exec'), namespace)
            picks = [n for n in staged if namespace['linux_x64'](n.lower())]
            self.assertEqual(picks, ['ReKHCoM-v2.1-linux-x86-x64-desktop-steamdeck.tar.gz'])
            namespace['quiver'](staged)
            self.assertTrue(all(ok for ok, text in checks), checks)
            for name in ('ReKHCoM-v2.0-windows-x86.zip', 'ReKHCoM-v2.1-windows-x86-x64.zip'):
                self.assertTrue(namespace['asset_matches_filter'](name, namespace['WIN_FILTER']))
            for name in ('PPSA98511.zip', 'ReKHCoM-v2.1-windows-x86-x64.zip.sha256', 'ReKHCoM-v2.1-linux-x86-x64-desktop-steamdeck.tar.gz'):
                self.assertFalse(namespace['asset_matches_filter'](name, namespace['WIN_FILTER']))

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
