"""Offline regressions for real Android smoke failures, without booting a device."""
from pathlib import Path
import subprocess
import tempfile
import unittest
from unittest.mock import patch

import test_android_scenarios as scenarios


class ScenarioResultsTests(unittest.TestCase):
    def run_case(self, host, log='mode: mode_title (frame 0)\n', alive=(True, False)):
        processes = iter(alive)
        ticks = iter(range(100))

        def adb(*args, **kwargs):
            if args[:2] == ('shell', 'pidof'):
                return subprocess.CompletedProcess(args, 0 if next(processes, False) else 1, b'', b'')
            return subprocess.CompletedProcess(args, 0, b'', b'')

        def private(*args):
            result = host if args == ('cat', 'files/kh_com_host.log') else log if args == ('cat', 'files/kh_com.log') else ''
            return subprocess.CompletedProcess(args, 0, result.encode(), b'')

        with tempfile.TemporaryDirectory() as folder, \
                patch.object(scenarios, 'OUT', Path(folder)), \
                patch.object(scenarios, 'adb', side_effect=adb), \
                patch.object(scenarios, 'private', side_effect=private), \
                patch.object(scenarios, 'keep_evidence') as evidence, \
                patch.object(scenarios.time, 'sleep'), \
                patch.object(scenarios.time, 'monotonic', side_effect=lambda: next(ticks)):
            result = scenarios.run('us', 'battle-123-BATTLE_EVENT_AGRABAH_1', 100, 10, 'KHCOM_BATTLE=123:4:0')
        return result, evidence

    def test_missing_guest_is_bad_apk_even_when_process_exits(self):
        # Actual host output from the 25/10 US Agrabah battery failure.
        host = ('the game image guest-us.macho is missing from the APK\n'
                'The game image could not be loaded (the game image guest-us.macho is missing from the APK).\n')
        (status, detail, _), evidence = self.run_case(host, log='')
        self.assertEqual(status, 'BAD-APK')
        self.assertIn('guest-us.macho is missing', detail)
        evidence.assert_called_once()

    def test_missing_guest_does_not_wait_for_timeout(self):
        (status, detail, _), evidence = self.run_case('the game image guest-jp.macho is missing from the APK\n',
                                                   log='', alive=(True,) * 20)
        self.assertEqual(status, 'BAD-APK')
        self.assertIn('guest-jp.macho', detail)
        evidence.assert_called_once()

    def test_nonzero_host_game_exit_is_not_success(self):
        (status, detail, _), evidence = self.run_case('the game ended (1)\n')
        self.assertEqual(status, 'GAME-ERROR')
        self.assertIn('(1)', detail)
        evidence.assert_called_once()

    def test_empty_game_log_is_not_a_pass(self):
        (status, _, _), evidence = self.run_case('', log='')
        self.assertEqual(status, 'BAD-SCENARIO')
        evidence.assert_called_once()

    def test_clean_host_exit_still_passes(self):
        (status, _, _), evidence = self.run_case('starting the game (250 bridge entries)\nthe game ended (0)\n')
        self.assertEqual(status, 'ok')
        evidence.assert_not_called()

    def test_legacy_clean_process_exit_still_passes(self):
        (status, _, _), evidence = self.run_case('')
        self.assertEqual(status, 'ok')
        evidence.assert_not_called()


if __name__ == '__main__':
    unittest.main()
