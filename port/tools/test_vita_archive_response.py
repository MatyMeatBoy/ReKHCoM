"""Exercise the real archiver with a long object list; no SDK/VM/ROM/compiler."""
import importlib.util
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('vita_archive_builder', ROOT / 'port/vita/build_vita.py')
build = importlib.util.module_from_spec(spec)
spec.loader.exec_module(build)
AR = str(build.AR) if Path(build.AR).is_file() else shutil.which('llvm-ar')


@unittest.skipUnless(AR, 'llvm-ar is required for archive consumer checks')
class ArchiveResponseTests(unittest.TestCase):
    def test_all_members_survive_a_windows_sized_command(self):
        with tempfile.TemporaryDirectory(prefix='vita archive ') as folder:
            out = Path(folder)
            # Arbitrary object payloads are valid ar members. Test argument and
            # archive fidelity here; the full guest build tests ARM object code.
            objects = []
            for i in range(800):
                obj = out / f'guest unit {i:04d}.o'
                obj.write_bytes(f'fixture member {i}\n'.encode())
                objects.append(obj)
            archive = out / 'guest-eu.a'
            self.assertGreater(len(subprocess.list2cmdline([AR, 'rcs', str(archive), *map(str, objects)])), 32767)
            with patch.object(build, 'AR', AR):
                build.archive_guest(archive, objects)
            members = subprocess.check_output([AR, 't', str(archive)], encoding='utf-8').splitlines()
            self.assertEqual(members, [p.name for p in objects])
            for i in (0, 399, 799):
                actual = subprocess.check_output([AR, 'p', str(archive), objects[i].name])
                self.assertEqual(actual, objects[i].read_bytes())

    def test_rebuild_drops_stale_members(self):
        with tempfile.TemporaryDirectory(prefix='vita rebuild ') as folder:
            out = Path(folder)
            one, two = out / 'first.o', out / 'second.o'
            one.write_bytes(b'first')
            two.write_bytes(b'second')
            archive = out / 'guest.a'
            with patch.object(build, 'AR', AR):
                build.archive_guest(archive, [one, two])
                build.archive_guest(archive, [two])
            self.assertEqual(subprocess.check_output([AR, 't', str(archive)], encoding='utf-8').splitlines(), ['second.o'])


if __name__ == '__main__':
    unittest.main()
