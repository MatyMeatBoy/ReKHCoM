#!/usr/bin/env python3
"""HD menus battery: every menu with an HD title, in each language, checked on a frame where the menu has settled
(not only while it slides in: several menus move their title to a BG copy then, DEBUG_WORKFLOW.md), plus the HP
gauge at several HP levels. The game logs what it draws in HD on the checked frames (KHCOM_HDDRAWLOG); a case passes
when the title's HD sprite is among them and the process ended cleanly. A contact sheet of the checked frames' top
strip goes with the report, for the eye. Silent (dummy audio), each case in its own folder, four at a time.

    python port/tools/test_hd_menus.py --hd <hd sprites folder> [--build build/pc-hdtest-eu] [--langs es,en,fr,de,it]
                                       [--only menu,deck,...] [--save <kh_com.sav>] [--out build/hdmenus]
"""
import argparse
import json
import os
import re
import shutil
import subprocess
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SAVE = ROOT.parent / 'jugar/ReKHCoM-v1.9.15-completo/kh_com.sav'
WALK = ','.join(f'{f}-{f + 3}:A' for f in range(200, 1400, 50))  # language, title, continue: into the field

# the title's HD sprite per language: (Spanish, English, French, German, Italian); None where there is none
SUFFIX = {'es': 'Spanish', 'en': '', 'fr': 'French', 'de': 'German', 'it': 'Italian'}


def banner(base, lang, short=False, shared=()):
    if lang in shared:
        return f'{base}Banner'
    if short:  # gMapInspectBarEs / Fr / De / It
        return f'{base}{dict(es="Es", en="", fr="Fr", de="De", it="It")[lang]}Banner'
    return f'{base}{SUFFIX[lang]}Banner'


def pause_case(entry, check):
    """START in the field, `entry` presses of DOWN, A; checked `check` frames after A."""
    f = 1600
    keys = [WALK, f'{f}-{f + 3}:START'] + [f'{f + 60 + d * 20}-{f + 63 + d * 20}:DOWN' for d in range(entry)]
    a = f + 80 + entry * 20
    keys.append(f'{a}-{a + 3}:A')
    return {'KHCOM_KEYS': ','.join(keys)}, a + check


CASES = {
    'menu': (lambda: ({'KHCOM_KEYS': WALK + ',1600-1603:START'}, 1700),
             lambda l: banner('gMapMenuTitle', l, shared=('en', 'fr', 'it'))),
    'deck': (lambda: pause_case(0, 260), lambda l: banner('gDeckTitleBanner', l)),
    'mapcards': (lambda: pause_case(2, 260), lambda l: banner('gMapInspectBar', l, short=True)),
    'worldcards': (lambda: pause_case(3, 260), lambda l: banner('gWorldInspectBar', l)),
    'status': (lambda: pause_case(4, 260), lambda l: banner('gStatusBar', l, shared=('en', 'de'))),
    'journal': (lambda: pause_case(5, 260),
                lambda l: {'es': 'gJiminyTitleLocalizedFrame4Banner', 'en': 'gJiminyTitleBanner',
                           'fr': 'gJiminyTitleLocalizedFrame0Banner', 'it': 'gJiminyTitleLocalizedFrame6Banner'}.get(l)),
    'load': (lambda: ({'KHCOM_KEYS': ','.join(f'{f}-{f + 3}:A' for f in range(200, 600, 50))}, 760),
             lambda l: banner('gMenuLoadTitle', l)),
    'save': (lambda: ({'KHCOM_KEYS': WALK, 'KHCOM_MODE': '1500:mapsave'}, 1760), lambda l: banner('gMapSaveTitle', l)),
    'worldselect': (lambda: ({'KHCOM_KEYS': WALK, 'KHCOM_MODE': '1500:mode_worldselect'}, 1900),
                    lambda l: banner('gWorldselectTitle', l)),
    'worldwarp': (lambda: ({'KHCOM_KEYS': WALK, 'KHCOM_MODE': '1500:mode_worldwarp'}, 1760),
                  lambda l: None if l == 'fr' else banner('gWorldWarpBar', l)),
    'shop': (lambda: ({'KHCOM_KEYS': WALK, 'KHCOM_MODE': '1500:mode_ms_top'}, 1900),
             lambda l: None if l == 'fr' else banner('gMsTopBar', l)),
    'rikudeck': (lambda: ({'KHCOM_MODE': '60:roomriku:10800', 'KHCOM_KEYS': '400-403:START,' + ','.join(
                              f'{f}-{f + 3}:A' for f in range(480, 780, 60))}, 1100), lambda l: banner('gRikuDeckTitleBanner', l)),  # Reverse/Rebirth: its tutorial, then VIEW DECK
    'premire': (lambda: ({'KHCOM_KEYS': WALK, 'KHCOM_MODE': '1450:WORLDSELECT'}, 2000),
                lambda l: banner('gPremireChanceTitle', l)),  # the elite bonus screen, through the debug mode
    'levelup': (lambda: ({'KHCOM_BATTLE': '0:1:0', 'KHCOM_KEYS': '60-62:A', 'KHCOM_MODE': '400:levelup'}, 1000),
                lambda l: banner('gLevelUpHeader', l)),
    'synthesis': (lambda: ({'KHCOM_MODE': '60:room:10901,120:fieldfreeze,200:doorpose:0', 'KHCOM_KEYS': '240-242:A'},
                           470), lambda l: banner('gMapSelectTitle', l)),
}
GAUGE = [100, 75, 50, 25, 10, 3]


def run(build, hd, save, out, name, lang, env_extra, check, expect):
    work = ROOT / 'build' / f'hdm_{name}_{lang}'  # two levels under the repository, where the exe finds ../../roms
    shutil.rmtree(work, ignore_errors=True)
    work.mkdir(parents=True)
    for f in ('khcom.exe', 'SDL3.dll'):
        shutil.copy2(build / f, work / f)
    for save_name in ('kh_com.sav', 'kh_com_us.sav', 'kh_com_jp.sav'):  # each version reads its own (23/10)
        shutil.copy2(save, work / save_name)
    (work / 'kh_com.ini').write_text('config_version=2\nview_mode=4\n')
    env = dict(os.environ, SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy', KHCOM_FAST='1',
               KHCOM_CONFIG='hd_cards=1;hd_text=1', KHCOM_HD_SPRITES=str(hd), KHCOM_WINDOW='1920x1080',
               KHCOM_HDDRAWLOG=str(check), KHCOM_OUTPUT_SHOT=str(check), KHCOM_QUIT=str(check + 20), **env_extra)
    try:
        code = subprocess.run([str(work / 'khcom.exe'), '--language', lang], cwd=str(work), env=env, timeout=400,
                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL).returncode
    except subprocess.TimeoutExpired:
        code = 'timeout'
    log = (work / 'kh_com.log').read_text(errors='replace') if (work / 'kh_com.log').exists() else ''
    drawn = re.findall(rf'hd draw {check} (\S+)', log)
    gauge = re.findall(rf'hd gauge {check} hp (\d+)/(\d+)', log)
    crashed = 'CRASH' in log or 'fatal' in log.lower()
    if expect == 'gauge':
        ok = code == 0 and not crashed and bool(gauge)
    else:
        ok = code == 0 and not crashed and expect in drawn
    return dict(case=name, lang=lang, ok=ok, code=code, expect=expect, drawn=sorted(set(drawn)), gauge=gauge,
                shot=str(work / f'out_{check:05d}.bmp'))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--hd', required=True)
    ap.add_argument('--build', default=str(ROOT / 'build/pc-hdtest-eu'))
    ap.add_argument('--langs', default='es,en,fr,de,it')
    ap.add_argument('--only')
    ap.add_argument('--save', default=str(SAVE))
    ap.add_argument('--out', default=str(ROOT / 'build/hdmenus'))
    args = ap.parse_args()
    # the game runs in its own folder: a relative HD path would point nowhere and every case would fall back
    build, hd, save, out = Path(args.build), Path(args.hd).resolve(), Path(args.save), Path(args.out)
    out.mkdir(parents=True, exist_ok=True)
    only = set(args.only.split(',')) if args.only else None
    jobs = []
    for name, (setup, expect) in CASES.items():
        if only and name not in only:
            continue
        for lang in args.langs.split(','):
            e = expect(lang)
            if e is None:
                continue
            env, check = setup()
            jobs.append((name, lang, env, check, e))
    if not only or 'gauge' in only:
        for p in GAUGE:
            jobs.append((f'gauge{p}', 'es', {'KHCOM_BATTLE': '0:1:0', 'KHCOM_KEYS': '60-62:A',
                                            'KHCOM_MODE': f'300:hudhp:{p}'}, 700, 'gauge'))
    with ThreadPoolExecutor(4) as pool:
        results = list(pool.map(lambda j: run(build, hd, save, out, *j), jobs))
    bad = [r for r in results if not r['ok']]
    for r in results:
        print(('ok  ' if r['ok'] else 'FAIL'), f"{r['case']:12} {r['lang']}  expect {r['expect']}",
              '' if r['ok'] else f"code {r['code']} drawn {r['drawn'][:6]}")
    (out / 'report.json').write_text(json.dumps(results, indent=1), encoding='utf-8')
    try:
        from PIL import Image, ImageDraw
        tiles = []
        for r in results:
            if Path(r['shot']).exists():
                im = Image.open(r['shot']).convert('RGB')
                crop = im.crop((0, 0, 960, 230 if r['case'].startswith('gauge') else 64))
                ImageDraw.Draw(crop).text((700, 4), f"{r['case']} {r['lang']} {'ok' if r['ok'] else 'FAIL'}",
                                          fill=(255, 255, 0) if r['ok'] else (255, 0, 0))
                tiles.append(crop)
        if tiles:
            sheet = Image.new('RGB', (960, sum(t.height for t in tiles)))
            y = 0
            for t in tiles:
                sheet.paste(t, (0, y))
                y += t.height
            sheet.save(out / 'sheet.png')
    except ImportError:
        pass
    print(f'test_hd_menus: {len(results) - len(bad)}/{len(results)} ok -> {out}')
    return 1 if bad else 0


if __name__ == '__main__':
    raise SystemExit(main())
