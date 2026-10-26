"""Poster import is optional, bounded and atomic; it never edits a source movie."""
import importlib.util
import os
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch
from PIL import Image

TOOLS = Path(__file__).parent


class PosterImportTests(unittest.TestCase):
    def setUp(self):
        spec = importlib.util.spec_from_file_location('poster_import', TOOLS / 'khhd_posters.py')
        self.module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(self.module)
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.movie = Path(self.temp.name) / 'zz_event_123.mp4'
        self.movie.write_bytes(b'original source fixture')

    def fake_decoder(self, command, **kwargs):
        Image.new('RGB', (640, 360), '#A06020').save(command[-1], format='PNG')
        return subprocess.CompletedProcess(command, 0)

    def test_one_frame_bound_and_segment_offsets(self):
        m = self.module
        self.assertEqual(m.poster_seconds(self.movie), (1, 171))
        self.assertEqual(m.poster_seconds(Path('zz_event_151.mp4')), (1, 223))
        self.assertEqual(m.poster_seconds(Path('ROP.mp4')), (1,))
        with patch.object(m.subprocess, 'run', side_effect=self.fake_decoder) as run:
            self.assertTrue(m.generate_poster(self.movie, 171))
        command = run.call_args.args[0]
        self.assertEqual(command[command.index('-frames:v') + 1], '1')
        self.assertEqual(command[command.index('-ss') + 1], '171')
        self.assertIn('640:360', command[command.index('-vf') + 1])
        self.assertIn('-an', command)
        self.assertEqual(self.movie.read_bytes(), b'original source fixture')
        self.assertEqual(m.poster_path(self.movie, 171).name, 'zz_event_123.poster-v1-t171-640x360.png')

    def test_fresh_png_does_not_start_decoder(self):
        m = self.module
        with patch.object(m.subprocess, 'run', side_effect=self.fake_decoder):
            self.assertTrue(m.generate_poster(self.movie, 1))
        with patch.object(m.subprocess, 'run') as run:
            self.assertTrue(m.generate_poster(self.movie, 1))
            run.assert_not_called()

    def test_failure_preserves_old_poster_and_removes_partial(self):
        m = self.module
        poster = m.poster_path(self.movie, 1)
        poster.write_bytes(b'old poster retained')
        with patch.object(m.subprocess, 'run', side_effect=subprocess.TimeoutExpired('ffmpeg', 30)):
            self.assertFalse(m.generate_poster(self.movie, 1))
        self.assertEqual(poster.read_bytes(), b'old poster retained')
        self.assertEqual(list(Path(self.temp.name).glob('*.part.png')), [])

    def test_stale_png_is_rebuilt_and_marked_current(self):
        m = self.module
        poster = m.poster_path(self.movie, 1)
        Image.new('RGB', (640, 360)).save(poster)
        future = poster.stat().st_mtime_ns + 10_000_000_000
        os.utime(self.movie, ns=(future, future))
        with patch.object(m.subprocess, 'run', side_effect=self.fake_decoder) as run:
            self.assertTrue(m.generate_poster(self.movie, 1))
        run.assert_called_once()
        self.assertGreaterEqual(poster.stat().st_mtime_ns, self.movie.stat().st_mtime_ns)


if __name__ == '__main__':
    unittest.main()
