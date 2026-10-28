"""Pure report contracts; does not start a game, process monitor or build."""
import math
import unittest
import profile_hd_resources as profile
import profile_serial_plan as plan
from pathlib import Path

def rows():
    return [dict(frame=str(i), interval_ms=str(i), render_work_ms=str(i/2), logic=str(i)) for i in range(2, 7)]

class ReportContract(unittest.TestCase):
    def test_complete_finite_samples_have_all_percentiles(self):
        result = profile.summarize_frames(rows(), 6, 3, 3)
        self.assertEqual(result['work_p50_ms'], 2)
        self.assertEqual(result['warm_samples'], 4)
        self.assertEqual(result['logic_steps'], [1])
        self.assertEqual(result['interval_p99_ms'], 5)
    def test_empty_truncated_duplicate_gap_or_nan_cannot_pass(self):
        bad = [[], rows()[:-2], rows()+rows()[-1:], [rows()[0],*rows()[2:]]]
        corrupt = rows();corrupt[2]['render_work_ms'] = 'nan';bad.append(corrupt)
        for value in bad:
            with self.subTest(value=value), self.assertRaises(ValueError):
                profile.summarize_frames(value, 6, 3, 3)
    def test_linux_virtual_data_is_not_private_bytes(self):
        self.assertEqual(profile.linux_memory('VmRSS: 10 kB\nVmHWM: 12 kB\nVmData: 9000 kB\n'), (10240, None, 12288))
    def test_missing_mode_and_degraded_hd_are_explicit_failures(self):
        with self.assertRaises(ValueError):profile.validate_case_log('Settings: ok', 'monstro-battle', False)
        with self.assertRaises(ValueError):profile.validate_case_log('harness: frame 242 mode mode_battle\nHD sprites: rejected x recipe', 'monstro-battle', True)
        profile.validate_case_log('harness: frame 242 mode mode_battle', 'monstro-battle', False)

    def test_guest_real_unusable_font_is_strict_failure_but_labelled_fallback_allowed(self):
        text='harness: frame 242 mode mode_battle\nHD text: apk:/fonts/KHMenu.otf is not a usable font'
        with self.assertRaises(ValueError):profile.validate_case_log(text,'monstro-battle',True)
        profile.validate_case_log(text,'monstro-battle',False)
        with self.assertRaises(ValueError):profile.validate_case_log('harness: frame 242 mode mode_battle','monstro-battle',True)

    def test_pacing_never_passes_skipped_or_paused_logic(self):
        for steps in ([0],[2],[0,1],[]):
            with self.subTest(steps=steps),self.assertRaises(ValueError):profile.validate_pacing({'logic_steps':steps})
        profile.validate_pacing({'logic_steps':[1]})
        profile.validate_pacing({'logic_steps':[0]},fast=True)

    def test_map_debug_index_cannot_substitute_effective_worldid(self):
        prefix='harness: frame 242 mode Event\n'
        wrong=prefix+'harness: progress flags=0 world=1 room=0\n'*2
        with self.assertRaises(ValueError):profile.validate_case_log(wrong,'monstro-opening',False,True)
        correct=prefix+'harness: progress flags=0 world=5 room=0\n'*2
        profile.validate_case_log(correct,'monstro-opening',False,True)
        self.assertIn('10303',profile.SCENARIOS['monstro-opening']['KHCOM_MODE'])

    def test_controlled_profile_changes_only_four_hd_flags(self):
        off=profile.controlled_settings(False);on=profile.controlled_settings(True)
        self.assertEqual({k for k in off if off[k]!=on[k]}, {'hd_hud','hd_text','hd_cards','hd_title'})
        self.assertEqual((off['view_mode'],off['zoom'],off['vsync'],off['experimental_frame_skip']), (4,8,0,0))
        self.assertEqual(off['graphics_mode'],off['graphics_mode_applied'])
        self.assertEqual(off['hd_movies'],0)
    def test_effective_settings_must_be_observed_and_match(self):
        line='Settings: graphics mode 1, view 4, fullscreen 0, vsync 0, frame skip 0, HD movies 0 music 0 scenes 0 HUD 1 text 1'
        profile.validate_effective_settings(line,True)
        for bad in ('',line.replace('view 4','view 3'),line.replace('HUD 1','HUD 0'),line.replace('frame skip 0','frame skip 1')):
            with self.subTest(value=bad),self.assertRaises(ValueError):profile.validate_effective_settings(bad,True)
    def test_quality_capture_uses_real_host_counter_and_valid_bmp(self):
        import struct,tempfile
        with tempfile.TemporaryDirectory() as temp:
            folder=Path(temp)
            with self.assertRaises(ValueError):profile.quality_capture(folder,True,450)
            header=b'BM'+struct.pack('<IHHI',70,0,0,54)+struct.pack('<IiiHHIIiiII',40,2,2,1,24,0,16,0,0,0,0)
            (folder/'host_shot_450.bmp').write_bytes(header+b'\0'*16)
            result=profile.quality_capture(folder,True,450)
            self.assertEqual(result['domain'],'host-present counter; not guest logic frame')
            self.assertEqual(result['dimensions'],[2,2])
            (folder/'host_shot_450.bmp').write_bytes(header)
            with self.assertRaises(ValueError):profile.quality_capture(folder,True,450)
            self.assertEqual(profile.quality_environment(False,450), {'KHCOM_PRESENTSHOTS':'450-450'})
            self.assertEqual(profile.quality_environment(True,450), {'KHCOM_HOST_SHOT':'450'})

    def test_serial_plan_alternates_and_uses_explicit_host_guest(self):
        result=plan.commands(*[Path(n) for n in ('build32','build64','host64','guest','rom','sprites','assets','out')])
        self.assertEqual(len(result),8)
        self.assertEqual([('--guest' in c) for c in result],[False,True,True,False,False,True,False,True])
        self.assertTrue(all('--fast' not in c and '--software' not in c for c in result))
        self.assertEqual(plan.quote("space's $path"), "'space''s $path'")

if __name__ == '__main__':unittest.main()
