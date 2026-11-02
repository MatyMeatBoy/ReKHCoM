"""Manifest contract for the pinned SDLActivity (no device or native execution)."""
from pathlib import Path
import unittest
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[2]
A = '{http://schemas.android.com/apk/res/android}'
# SDL 3.4.18 template, plus density already handled by this application.
SDL_CONFIGS = set('layoutDirection locale grammaticalGender fontScale fontWeightAdjustment '
                  'orientation uiMode screenLayout screenSize smallestScreenSize keyboard '
                  'keyboardHidden navigation density'.split())

class ManifestLifecycle(unittest.TestCase):
    def setUp(self):
        manifest = ET.parse(ROOT / 'port/android/app/src/main/AndroidManifest.xml')
        self.activities = {a.get(A+'name').rsplit('.', 1)[-1]: a
                           for a in manifest.findall('./application/activity')}

    def test_configuration_changes_reach_existing_sdl_activity(self):
        for name in ('LauncherActivity', 'GameActivity'):
            with self.subTest(activity=name):
                handled = set(self.activities[name].get(A+'configChanges', '').split('|'))
                self.assertFalse(SDL_CONFIGS - handled,
                                 f'{name} can recreate SDLActivity for {sorted(SDL_CONFIGS-handled)}')

    def test_launcher_game_isolation_and_task_behavior_stay_explicit(self):
        launcher, game = (self.activities[n] for n in ('LauncherActivity', 'GameActivity'))
        self.assertIsNone(launcher.get(A+'process'))
        self.assertEqual(game.get(A+'process'), ':game')
        self.assertEqual(launcher.get(A+'exported'), 'true')
        self.assertEqual(game.get(A+'exported'), 'false')
        for activity in (launcher, game):
            self.assertEqual(activity.get(A+'launchMode', 'standard'), 'standard')
            self.assertIsNone(activity.get(A+'taskAffinity'))
        self.assertEqual(launcher.get(A+'screenOrientation'), 'sensorLandscape')
        self.assertEqual(game.get(A+'screenOrientation'), 'landscape')

if __name__ == '__main__':
    unittest.main()
