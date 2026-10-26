"""Offline release-gate regressions for the real HD-scene harness."""
import contextlib
import io
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

import test_hd_scenes as scenes

EVENT = 'EVENT_035_8F_DEMO'
MOVIE = 'zz_event_084'


class HdSceneGateTests(unittest.TestCase):
    def check_case(self, result, imported=True, selected=EVENT):
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            (root / 'scenes').mkdir()
            if imported:
                (root / 'scenes' / (MOVIE + '.mp4')).touch()
            with patch.object(scenes, 'ROOT', root), \
                 patch.object(sys, 'argv', ['test_hd_scenes.py', '--hd', str(root), '--jobs', '1', '--only', selected]), \
                 patch.object(scenes, 'event_numbers', return_value={EVENT: 35}), \
                 patch.object(scenes, 'scene_map', return_value=([(EVENT, MOVIE)], [], [])), \
                 patch.object(scenes, 'event_lists', return_value=[(0, 35, EVENT)]), \
                 patch.object(scenes, 'run', return_value=result), contextlib.redirect_stdout(io.StringIO()):
                return scenes.main()

    def test_clean_playback_passes(self):
        self.assertEqual(self.check_case((0, False, [35], [MOVIE])), 0)

    def test_wrong_movie_fails_the_process(self):
        self.assertEqual(self.check_case((0, False, [35], ['wrong_movie'])), 1)

    def test_timeout_cannot_pass_after_opening_the_movie(self):
        self.assertEqual(self.check_case(('timeout', False, [35], [MOVIE])), 1)

    def test_logged_crash_fails_the_process(self):
        self.assertEqual(self.check_case((0, True, [35], [MOVIE])), 1)

    def test_missing_import_is_not_a_successful_empty_suite(self):
        self.assertEqual(self.check_case((0, False, [35], [MOVIE]), imported=False), 1)

    def test_unmatched_selection_is_not_a_successful_empty_suite(self):
        self.assertEqual(self.check_case((0, False, [35], [MOVIE]), selected='not-an-event'), 1)


if __name__ == '__main__':
    unittest.main()
