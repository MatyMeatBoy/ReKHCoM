"""SFO release encoding and real producer commands; no SDK/build/runtime."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
from sony_package_version import sfo_release_version

ROOT=Path(__file__).resolve().parents[2]

class SfoReleaseTests(unittest.TestCase):
    def test_requested_and_boundary_versions(self):
        for source,expected in [('2.2.5','02.25'),('v2.2.5','02.25'),('2.2','02.20'),('2.3','02.30'),('0.0.0','00.00'),('99.9.9','99.99')]:
            with self.subTest(source=source):self.assertEqual(sfo_release_version(source),expected)

    def test_reject_unrepresentable_or_nonstable_versions(self):
        for source in ('100.0.0','2.10.0','2.2.10','1.9.16','2.2-pre3','2.2.5.1','2','-1.2.3','2.-1.3','2.2.-1','',None):
            with self.subTest(source=source),self.assertRaises(ValueError):sfo_release_version(source)

    def test_all_bounded_triples_are_unique_and_ordered(self):
        versions=[sfo_release_version(f'{a}.{b}.{c}') for a in range(100) for b in range(10) for c in range(10)]
        self.assertEqual(len(set(versions)),10000)
        self.assertEqual(versions,sorted(versions))

    def test_ps4_reads_complete_header_version(self):
        spec=importlib.util.spec_from_file_location('sfo_ps4_producer',ROOT/'port/ps4/build_ps4.py')
        build=importlib.util.module_from_spec(spec);spec.loader.exec_module(build)
        with tempfile.TemporaryDirectory(prefix='sony-version-') as directory:
            base=Path(directory);header=base/'port/include/port/port_version.h';header.parent.mkdir(parents=True)
            with patch.object(build,'ROOT',base):
                for version,expected in [('v2.2.5','02.25'),('v2.3','02.30')]:
                    header.write_text('#define REKHCOM_VERSION "'+version+'"\n',encoding='utf-8')
                    self.assertEqual(build.version(),expected)
                header.write_text('#define REKHCOM_VERSION "v2.2.50"\n',encoding='utf-8')
                with self.assertRaises(ValueError):build.version()
                header.write_text('missing version\n',encoding='utf-8')
                with self.assertRaises(ValueError):build.version()

if __name__=='__main__':unittest.main()
