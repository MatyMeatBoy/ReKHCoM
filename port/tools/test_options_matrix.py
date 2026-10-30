"""Options persistence and availability across HD/enhanced/classic, Sora/Riku and mod presence.

Every run uses a fresh config/save directory and the opt-in production menu audit.
Physical monitor behavior is outside the dummy renderer's scope.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import time

import case_bin
from test_text_options import item_numbers

ROOT = Path(__file__).resolve().parents[2]
SETTINGS = {
    'DODGE': 'dodge_button', 'STOCK': 'stock_button', 'INVERT_CARD_SCROLL': 'invert_card_scroll',
    'CRT_OUTPUT': 'crt_output', 'ASPECT': 'aspect', 'ZOOM': 'zoom', 'HUD': 'hud_size',
    'FULLSCREEN': 'fullscreen', 'VSYNC': 'vsync', 'SCANLINES': 'scanlines', 'LCD': 'color_correction',
    'GBA_BORDER': 'gba_border', 'FPS': 'show_fps', 'MASTER': 'master_volume', 'MUSIC': 'music_volume', 'SFX': 'sfx_volume',
    'UNDUB': 'undub', 'DISCORD': 'discord_presence', 'HDTEXT': 'hd_text', 'HD_MOVIES': 'hd_movies',
    'HD_MUSIC': 'hd_music', 'HD_SCENES': 'hd_scenes', 'HD_CARDS': 'hd_cards',
    'BATTLE_ZOOM': 'experimental_battle_zoom', 'SMOOTHAUDIO': 'smooth_audio',
    'SAVE_SLOT_EXPERIMENTAL': 'experimental_save_point_slot_select', 'MOOGLE_NOTICE': 'experimental_moogle_notice',
    'UNSTOCK': 'experimental_unstock', 'FRAMESKIP': 'experimental_frame_skip', 'VIEW': 'view_mode',
}
ACTION_PAGES = {'CONTROLS_MENU': 4, 'ACHIEVEMENTS_MENU': 6, 'SCENES_MENU': 8, 'REMATCH_MENU': 5, 'CHEATS_MENU': 3}
SPECIAL = {'MODE_RESET', 'TITLE', 'QUIT', 'SAVEBANK', 'FS_RES', 'CLOSE', 'ACH_MODE', *ACTION_PAGES}
AUDIT = re.compile(r'options-audit: phase=(\w+) page=(\d+) item=(\d+) visible=(\d+) enabled=(\d+) value=\[(.*?)\]')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, required=True)
    parser.add_argument('--hd', type=Path, required=True)
    parser.add_argument('--roms', type=Path, default=ROOT / 'roms')
    parser.add_argument('--modes', nargs='+', type=int, choices=[1, 2, 3], default=[1, 2, 3])
    parser.add_argument('--actors', nargs='+', choices=['sora', 'riku'], default=['sora', 'riku'])
    parser.add_argument('--assets', nargs='+', type=int, choices=[0, 1], default=[0, 1])
    parser.add_argument('--timeout', type=int, default=120)
    parser.add_argument('--out', type=Path, help='isolated evidence directory (must not exist)')
    args = parser.parse_args()
    build, hd = args.build.resolve(), args.hd.resolve()
    if not hd.is_dir():
        parser.error('--hd must be an existing imported mod folder')
    out = args.out.resolve() if args.out else ROOT / 'build/options-matrix' / time.strftime('%Y%m%d_%H%M%S')
    out.mkdir(parents=True)
    items = item_numbers()
    names = {number: name for name, number in items.items()}
    results = []
    digest = hashlib.sha256((build / 'khcom.exe').read_bytes()).hexdigest()

    def run(context, name, route=(), extras=None, seed=None, files=None, expected_exit=0):
        mode, actor, assets = context
        work = out / f'mode{mode}-{actor}-assets{assets}' / name
        work.mkdir(parents=True)
        case_bin.place(build, out, work)
        shutil.copytree(ROOT / 'port/assets/fonts', work / 'assets/fonts')
        if assets:
            subprocess.run(['cmd', '/c', 'mklink', '/J', str(work / 'hd'), str(hd)],
                           check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            if (args.roms / 'B8CJ.gba').is_file():
                (work / 'roms').mkdir()
                shared_rom = out / '_roms/B8CJ.gba'
                if not shared_rom.exists():
                    shared_rom.parent.mkdir(exist_ok=True)
                    shutil.copy2(args.roms / 'B8CJ.gba', shared_rom)
                os.link(shared_rom, work / 'roms/B8CJ.gba')
        settings = dict(config_version=11, graphics_mode=mode, graphics_mode_applied=mode, assets_seen=-1,
            view_mode=0 if mode == 3 else 4, zoom=8, window_scale=2, fullscreen=0, vsync=0,
            hd_text=int(mode != 3), hd_font=0, hd_cards=0, hd_title=0, hd_music=0, hd_movies=0, hd_scenes=0,
            master_volume=100, music_volume=100, sfx_volume=100)
        settings.update(seed or {})
        (work / 'kh_com.ini').write_text(''.join(f'{key}={value}\n' for key, value in settings.items()))
        for filename, content in (files or {}).items():
            (work / filename).write_text(content)
        env = {k: v for k, v in os.environ.items() if not k.startswith(('KHCOM_', 'SDL_'))}
        hold = max([tick for tick, _, _ in route] or [3]) + 12
        env.update(SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy', KHCOM_FAST='1', KHCOM_PPU_THREADS='1', KHCOM_OPTIONS_AUDIT='1',
            KHCOM_OPTIONS_TOGGLE='120', KHCOM_OPTIONS_HOLD=str(hold), KHCOM_QUIT='185', KHCOM_TRACE_MODES='1',
            KHCOM_MODE=f'30:{"roomriku" if actor == "riku" else "room"}:10901,60:fieldfreeze', KHCOM_WINDOW='640x360')
        if route:
            env['KHCOM_OPTIONS_ACTIONS'] = ','.join(f'{tick}:{items[row]}:{direction}' for tick, row, direction in route)
        env.update(extras or {})
        if actor == 'title':
            env.pop('KHCOM_MODE', None)
        try:
            code = subprocess.run([str(work / 'khcom.exe'), '--rom', str((args.roms / 'B8CP.gba').resolve()), '--language', 'es'],
                cwd=work, env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=args.timeout).returncode
        except subprocess.TimeoutExpired:
            code = 'timeout'
        log = (work / 'kh_com.log').read_text(encoding='utf-8', errors='replace')
        saved = dict(line.split('=', 1) for line in (work / 'kh_com.ini').read_text().splitlines()
                     if '=' in line and not line.startswith('#'))
        rows = {}
        for phase, page, item, visible, enabled, value in AUDIT.findall(log):
            if phase == 'open':
                rows[names.get(int(item), 'UNKNOWN_' + item)] = dict(page=int(page), visible=int(visible), enabled=int(enabled), value=value)
        problems = []
        if code != expected_exit or re.search(r'bios:|CRASH|FATAL', log):
            problems.append(f'exit/fault {code}')
        if not rows:
            problems.append('missing Options audit metadata; rebuild with the audit hook')
        if assets:
            os.rmdir(work / 'hd')  # unlink this test's junction only, never the mod directory
        result = dict(context=dict(mode=mode, actor=actor, assets=assets), case=name, exit=code, problems=problems,
                      rows=rows, folder=str(work))
        return result, saved, log

    def record(result):
        result['passed'] = not result['problems']
        results.append(result)
        print(f"mode{result['context']['mode']}-{result['context']['actor']}-assets{result['context']['assets']} {result['case']}: "
              f"{'PASS' if result['passed'] else 'FAIL'} " + '; '.join(result['problems']), flush=True)
        (out / 'report.json').write_text(json.dumps(dict(build_sha256=digest, cases=results), indent=2))

    for mode in args.modes:
        for actor in args.actors:
            for assets in args.assets:
                context = (mode, actor, assets)
                baseline, original, _ = run(context, 'baseline')
                unknown = [name for name, row in baseline['rows'].items() if row['visible'] and name not in SETTINGS and name not in SPECIAL]
                baseline['problems'] += ['unclassified visible row ' + name for name in unknown]
                record(baseline)
                if baseline['problems']:
                    continue
                route = [(3+i*3, name, -1 if name in ['MASTER', 'MUSIC', 'SFX'] else 1)
                         for i, name in enumerate(name for name in SETTINGS if baseline['rows'].get(name, {}).get('visible'))]
                changed, saved, log = run(context, 'settings', route)
                mutations = {int(t): (int(enabled), before, after) for t, _, enabled, before, after in
                    re.findall(r'options-test: tick=(\d+) item=(\d+) enabled=(\d+) before=\[(.*?)\] after=\[(.*?)\]', log)}
                changed['tested_settings'] = []
                for tick, name, direction in route:
                    key = SETTINGS[name]
                    evidence = mutations.get(tick)
                    if evidence is None:
                        changed['problems'].append(name + ': action never reached')
                    elif baseline['rows'][name]['enabled']:
                        if evidence[1] == evidence[2] or saved.get(key) == original.get(key):
                            changed['problems'].append(name + ': enabled setting did not change and persist')
                    elif evidence[1] != evidence[2] or saved.get(key) != original.get(key):
                        changed['problems'].append(name + ': disabled setting changed')
                    changed['tested_settings'].append(dict(row=name, key=key, enabled=baseline['rows'][name]['enabled']))
                record(changed)
                for row, page in ACTION_PAGES.items():
                    if not baseline['rows'].get(row, {}).get('visible'):
                        continue
                    result, _, log = run(context, row.lower(), [(3, row, 1)])
                    if not re.search(r'options-audit: phase=changed page=' + str(page) + r'\b', log):
                        result['problems'].append(row + ': target page did not open')
                    record(result)
                for row in ['MODE_RESET', 'TITLE', 'QUIT']:
                    if not baseline['rows'].get(row, {}).get('visible'):
                        continue
                    result, saved, log = run(context, row.lower() + '-cancel', [(3, row, 1)], {'KHCOM_CONFIRM_KEYS': '6:3'})
                    if 'confirm-test: tick=6' not in log:
                        result['problems'].append('confirmation did not open/cancel')
                    if saved.get('graphics_mode') != original.get('graphics_mode'):
                        result['problems'].append('cancel changed graphics mode')
                    record(result)
                # These rows have effects beyond a single scalar value.
                result, saved, _ = run(context, 'bank-disabled', [(3, 'SAVEBANK', 1)])
                if saved.get('save_bank') != original.get('save_bank'):
                    result['problems'].append('gameplay changed save bank')
                record(result)
                result, saved, _ = run(context, 'achievement-mode-info', [(3, 'ACH_MODE', 1)])
                if saved.get('cheat_savestates') != original.get('cheat_savestates'):
                    result['problems'].append('informational achievement row changed cheats')
                record(result)
                for accept in [False, True]:
                    result, saved, log = run(context, 'font-' + ('accept' if accept else 'cancel'), extras={
                        'KHCOM_OPTIONS_FONTCHOICE': '1', 'KHCOM_OPTIONS_ACTIONS': '', 'KHCOM_CONFIRM_KEYS': f'3:1,6:{2 if accept else 3}'})
                    if saved.get('hd_font') != ('2' if accept else '0') or 'confirm-test: tick=6' not in log:
                        result['problems'].append('font selection did not commit/cancel correctly')
                    record(result)
                target = 3 if mode == 2 else 2
                start = 2 if mode == 1 and not assets else mode
                keys = [f'{6+i*3}:{1 if target > start else 0}' for i in range(abs(target-start))]
                keys += ['15:2', '18:1', '21:2', '24:1', '27:2']
                result, saved, _ = run(context, 'preset-accept', [(3, 'MODE_RESET', 1)],
                    {'KHCOM_CONFIRM_KEYS': ','.join(keys), 'KHCOM_OPTIONS_HOLD': '42'})
                if saved.get('graphics_mode') != str(target) or saved.get('graphics_mode_applied') != str(target):
                    result['problems'].append('confirmed preset did not persist')
                record(result)
                result, saved, log = run(context, 'resolution-confirm', extras={
                    'KHCOM_OPTIONS_CURSOR': str(items['FS_RES']),
                    'KHCOM_OPTIONS_KEYS': '3:1073741903,6:13,9:27'})
                if 'options-input: tick=6' not in log:
                    result['problems'].append('resolution confirm was not handled')
                record(result)
                cheats = {'CHEAT_RIKU': 'cheat_unlock_riku', 'CHEAT_ONEHIT': 'cheat_one_hit',
                          'CHEAT_INFHP': 'cheat_infinite_hp', 'CHEAT_GBA_MAP': 'debug_gba_map_logic',
                          'CHEAT_TITLE_BRIGHTNESS': 'experimental_title_brightness'}
                route = [(3, 'CHEATS_MENU', 1)] + [(6+i*3, row, 1) for i, row in enumerate(cheats)]
                result, saved, log = run(context, 'cheat-settings', route)
                for row, key in cheats.items():
                    if saved.get(key) != '1':
                        result['problems'].append(row + ': did not persist')
                record(result)
                result, saved, log = run(context, 'quick-states-confirm', [(3, 'CHEATS_MENU', 1), (6, 'CHEAT_STATES', 1)],
                    {'KHCOM_CONFIRM_KEYS': '9:1,12:2'})
                if saved.get('cheat_savestates') != '1':
                    result['problems'].append('quick states confirmation did not persist')
                record(result)
    # Save banks are only available before loading a game. Give bank 2 a known
    # config, then return to bank 1 and verify both settings remain distinct.
    for assets in args.assets:
        context = (2, 'title', assets)
        bank = 'config_version=11\ngraphics_mode=2\ngraphics_mode_applied=2\nsave_bank=2\nmaster_volume=30\nassets_seen=-1\n'
        result, saved, _ = run(context, 'bank-roundtrip', [(3, 'SAVEBANK', 1), (6, 'SAVEBANK', -1)],
            files={'kh_com_bank2.ini': bank})
        work = Path(result['folder'])
        if saved.get('save_bank') != '1' or saved.get('master_volume') != '100':
            result['problems'].append('return to original bank lost its config')
        if 'master_volume=30' not in (work / 'kh_com_bank2.ini').read_text():
            result['problems'].append('other bank config was overwritten')
        record(result)
    print(out)
    return 0 if results and all(result['passed'] for result in results) else 1


if __name__ == '__main__':
    raise SystemExit(main())
