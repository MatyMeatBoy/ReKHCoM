"""Gallery transition matrix, with isolated reusable workers and explicit unavailable/N/A results.

Codec fixtures are synthetic local MP4s, not story footage. Saves/ROMs are copied and never modified at source.
Writes one bounded log per case, fingerprints, exit codes, and an append-only results.jsonl for resuming.
"""
import argparse
import concurrent.futures
import hashlib
import itertools
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import threading
import time

ROOT = Path(__file__).resolve().parents[2]
MOVIE_FILES = {513: ('ROP', 0), 514: (None, 0), 515: ('zz_event_123', 170),
               516: ('zz_event_151', 222), 517: ('zz_event_631', 0)}


def key(case):
    return '-'.join(str(case[k]) for k in ('mode', 'context', 'actor', 'tab', 'event', 'resource', 'scenes', 'movies', 'exit', 'recom', 'window'))


def cases(which):
    for mode, context, actor, tab, resource, scenes, route, kind in itertools.product(
            (1, 2, 3), ('title', 'game'), ('sora', 'riku'), (0, 1, 2), ('absent', 'valid', 'error'),
            (0, 1), ('finish', 'skip', 'cancel'), ('mapped', 'native')):
        event = (10 if actor == 'sora' else 155) if kind == 'mapped' else (2 if actor == 'sora' else 150)
        yield dict(mode=mode, context=context, actor=actor, tab=tab, event=event, resource=resource,
                   scenes=scenes, movies=1, exit=route, recom=0, window='240x160', kind=kind)
    if which == 'full':
        for mode, context, tab, resource, scenes, movies, route, event in itertools.product(
                (1, 2, 3), ('title', 'game'), (0, 1, 2), ('absent', 'valid', 'error'),
                (0, 1), (0, 1), ('finish', 'skip', 'cancel'), range(513, 518)):
            yield dict(mode=mode, context=context, actor='riku' if event == 517 else 'sora', tab=tab,
                       event=event, resource=resource, scenes=scenes, movies=movies, exit=route,
                       recom=0, window='240x160', kind='cinematic')
        for mode, context, tab, resource, scenes, movies, route, event in itertools.product(
                (1, 2, 3), ('title', 'game'), (1, 2), ('valid', 'error'), (0, 1), (0, 1),
                ('finish', 'skip', 'cancel'), (56, 73)):
            yield dict(mode=mode, context=context, actor='sora', tab=tab, event=event, resource=resource,
                       scenes=scenes, movies=movies, exit=route, recom=0, window='240x160', kind='alias-event')
        for mode, context, actor, recom, scenes, route in itertools.product(
                (1, 2, 3), ('title', 'game'), ('sora', 'riku'), (0, 1), (0, 1), ('finish', 'skip', 'cancel')):
            yield dict(mode=mode, context=context, actor=actor, tab=2, event=0 if actor == 'sora' else 193,
                       resource='valid', scenes=scenes, movies=1, exit=route, recom=recom,
                       window='240x160', kind='extras')



def equivalent_groups(jobs):
    """Collapse only inputs that cannot reach a different production branch.

    Keep graphics modes, title/game, actor, tab, logical version and both HD flags.
    An excluded/unavailable row cannot receive playback-exit input; GBA and unmapped
    entries never consult an HD resource. Record every grouped ID, never invent PASS.
    """
    groups = {}
    for case in jobs:
        normalized = case.copy()
        movie, _ = movie_for(case['event'])
        excluded = (case['tab'] == 1 and not movie or
                    case['event'] in (515, 516) and case['tab'] in (1, 2) and case['resource'] != 'absent')
        unavailable = case['tab'] == 1 and case['resource'] == 'absent'
        reasons = []
        if case['tab'] == 0 or not movie:
            normalized['resource'] = 'not-consulted'
            reasons.append('HD resource never consulted by this native provider')
        if excluded or unavailable:
            normalized['exit'] = 'not-started'
            reasons.append('Excluded/unavailable row never starts playback')
        signature = key(normalized)
        group = groups.setdefault(signature, dict(case=case, equivalents=[], reasons=reasons))
        group['equivalents'].append(key(case))
    return list(groups.values())


def movie_for(event):
    if event in MOVIE_FILES:
        return MOVIE_FILES[event]
    rows = dict((int(n), movie) for n, movie in re.findall(r'\{ EVENT_(\d+)[A-Z0-9_]*, "([^"]+)"',
                (ROOT / 'port/src/port_hdscenes.inc').read_text(encoding='utf-8')))
    return rows.get(event), 0


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def prepare(pool, exe, rom):
    pool.mkdir(parents=True, exist_ok=True)
    for name, source in [('khcom.exe', exe), ('SDL3.dll', exe.parent / 'SDL3.dll'), ('baserom.gba', rom)]:
        target = pool / name
        if not target.exists() or target.stat().st_mtime != source.stat().st_mtime:
            shutil.copy2(source, target)
    shutil.copytree(exe.parent / 'assets', pool / 'assets', dirs_exist_ok=True)


def run_case(case, pool, work, fixtures, save, repeats=2, screenshots=False):
    ident = key(case)
    movie, offset = movie_for(case['event'])
    not_visible = case['tab'] == 1 and not movie
    alias = case['event'] in (515, 516) and case['tab'] in (1, 2) and case['resource'] != 'absent'
    not_visible = not_visible or alias
    # Remove only this worker's generated cache/media; linked source fixtures are unlinked before any replacement.
    for path in list(pool.glob('scene-preview-*.rgb')) + list(pool.glob('gallery_*.bmp')) + list((pool / 'hd').rglob('*.mp4')):
        path.unlink()
    for path in pool.glob('achievements_*.dat'):
        path.unlink()
    if save:
        shutil.copy2(save, pool / 'kh_com.sav')
    else:
        (pool / 'kh_com.sav').write_bytes(b'\xff' * 65536)
    before_save = sha(pool / 'kh_com.sav')
    (pool / 'scenes_seen.dat').write_bytes(b'\xff' * 128)
    provider_movie = movie or ('zz_event_012' if case['actor'] == 'sora' else 'zz_event_503')
    if case['resource'] != 'absent':
        target = pool / 'hd/scenes' / (provider_movie + '.mp4')
        target.parent.mkdir(parents=True, exist_ok=True)
        if case['resource'] == 'error':
            target.write_bytes(b'Invalid local codec fixture')
        else:
            os.link(fixtures[offset], target)
        if case['kind'] == 'extras':
            extra = pool / 'hd/scenes' / ('zz_event_001.mp4' if case['actor'] == 'sora' else 'zz_event_614.mp4')
            os.link(fixtures[0], extra)
    (pool / 'kh_com.ini').write_text(
        f"config_version=11\ngraphics_mode={case['mode']}\ngraphics_mode_applied={case['mode']}\n"
        f"hd_title={int(case['mode'] == 1)}\nhd_text={int(case['mode'] != 3)}\nhd_hud=0\nhd_cards=0\n"
        f"hd_scenes={case['scenes']}\nhd_movies={case['movies']}\nhd_music=0\nassets_seen=255\n"
        f"ps2_mode={case['recom']}\nps2_difficulty=2\nfullscreen=0\nview_mode={0 if case['mode'] == 3 else 4}\n"
        "game_language=5\nzoom=8\nvsync=0\nsave_bank=1\n", encoding='utf-8')
    events = [] if not_visible else [f'{30 + 40 * i}:120' for i in range(repeats)]
    close_tick = 30 if not_visible else 30 + 40 * repeats
    events.append(f'{close_tick}:-1')
    env = {k: v for k, v in os.environ.items() if not k.startswith('KHCOM_')}
    env.update(SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy', KHCOM_FAST='1', KHCOM_BACKGROUND_TEST='1', KHCOM_PPU_THREADS='1',
               KHCOM_WINDOW=case['window'], KHCOM_MODE=f"1:{'roomriku' if case['actor'] == 'riku' else 'room'}:20000" if case['context'] == 'game' else '1:mode_title',
               KHCOM_OPTIONS_AT='120', KHCOM_SCENES_SHOT=str(case['tab'] + 1), KHCOM_GALLERY_EVENT=str(case['event'] - 2 if 129 <= case['event'] < 512 else case['event']),
               KHCOM_GALLERY_KEYS=','.join(events), KHCOM_GALLERY_AUDIT='1', KHCOM_QUIT='8000',
               KHCOM_GALLERY_QUIT_AFTER=str(close_tick), KHCOM_ACHIEVEMENTS_LIVE='1')
    if case['recom']:
        env['KHCOM_RECOM'] = '1'
    if screenshots:
        env['KHCOM_GALLERY_SHOTS'] = '25,' + str(close_tick - 5)
        env['KHCOM_GALLERY_PREVIEWTEST'] = '1'
    hd = movie and case['tab'] in (1, 2) and case['resource'] == 'valid'
    if hd and case['exit'] in ('skip', 'cancel'):
        env['KHCOM_HDMOVIE_PAUSETEST'] = '0'
        env['KHCOM_HDMOVIE_PAUSE_ACTION'] = case['exit']
    if not hd:
        env['KHCOM_GALLERY_AUTO_KEYS'] = case['exit']
        if case['exit'] != 'finish':
            env['KHCOM_GALLERY_NATIVE_PAUSE'] = 'cancel' if case['exit'] == 'cancel' else 'skip'
    log_path = work / 'logs' / (ident + '.log')
    log_path.parent.mkdir(exist_ok=True)
    started = time.monotonic()
    try:
        with log_path.open('w', encoding='utf-8') as log:
            run = subprocess.run([str(pool / 'khcom.exe')], cwd=pool, env=env, stdout=log,
                                 stderr=subprocess.STDOUT, timeout=180)
        text = log_path.read_text(encoding='utf-8', errors='replace')
        if run.returncode:
            raise AssertionError(f'exit={run.returncode}')
        if re.search(r'bios:|CRASH|FATAL', text):
            raise AssertionError('Invalid memory access/crash/fatal in log')
        audits = re.findall(r'scene gallery audit: game=(\d) map=(\d) config=(\d) files=(\d) recom=(\d) rank=(\d)', text)
        if any(a != ('1',) * 6 for a in audits):
            raise AssertionError('Replay modified game/map/config/achievement/scene progress or Re:CoM state: ' + str(audits))
        unavailable = case['tab'] == 1 and case['resource'] == 'absent'
        if not_visible:
            if not re.search(r'scene gallery selection: event=\d+ found=0', text):
                raise AssertionError('Expected excluded/deduplicated scene remains visible in the actual UI catalogue')
            if audits:
                raise AssertionError('A different scene played while verifying the excluded entry')
        if not unavailable and not not_visible and len(audits) < repeats:
            raise AssertionError(f'Expected {repeats} completed audits; got {len(audits)}')
        if sha(pool / 'kh_com.sav') != before_save:
            raise AssertionError('SRAM file changed')
        if not not_visible and case['resource'] == 'valid' and hd and text.count('scene gallery: HD ') < repeats:
            raise AssertionError('Expected imported HD provider was not played')
        if not not_visible and not unavailable and not hd and (case['resource'] != 'error' or case['tab'] == 2) and text.count('restored original state; returning to the list') < repeats:
            raise AssertionError('Native scene never returned to its list')
        if f'tick={close_tick} key=-1' not in text:
            raise AssertionError('Gallery did not remain navigable through the final close')
        result = dict(id=ident, case=case, status='N/A' if not_visible else 'PASS', reason=('Already included in the complete mapped HD event' if alias else 'No HD mapping; excluded from the HD-only list') if not_visible else '', seconds=round(time.monotonic() - started, 3),
                      audits=len(audits), sram_sha256=before_save, log=str(log_path),
                      availability='excluded by actual UI catalogue' if not_visible else 'blocked: HD not imported' if unavailable else 'played')
        if screenshots:
            dest = work / 'shots' / ident
            dest.mkdir(parents=True, exist_ok=True)
            for path in pool.glob('gallery_*.bmp'):
                shutil.copy2(path, dest / path.name)
            result['shots'] = str(dest)
        return result
    except Exception as error:
        return dict(id=ident, case=case, status='FAIL', error=str(error), log=str(log_path),
                    seconds=round(time.monotonic() - started, 3))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', required=True, type=Path)
    p.add_argument('--rom', required=True, type=Path)
    p.add_argument('--work', required=True, type=Path)
    p.add_argument('--save', required=True, type=Path, help='Initialized save copy; blank SRAM legitimately changes during boot')
    p.add_argument('--suite', choices=('pilot', 'core', 'full', 'relevant'), default='pilot')
    p.add_argument('--workers', type=int, default=2)
    p.add_argument('--resume', action='store_true')
    a = p.parse_args()
    if not 1 <= a.workers <= 2:
        p.error('--workers must be 1 or 2 (bounded renderer memory budget)')
    a.exe, a.rom, a.work = a.exe.resolve(), a.rom.resolve(), a.work.resolve()
    a.work.mkdir(parents=True, exist_ok=True)
    fixtures = {}
    for offset in (0, 170, 222):
        path = a.work / f'codec-offset-{offset}.mp4'
        if not path.exists():
            subprocess.run(['ffmpeg', '-v', 'error', '-nostdin', '-f', 'lavfi', '-i', 'testsrc2=s=160x90:r=5',
                            '-t', str(offset + 1.5), '-an', '-c:v', 'libx264', '-preset', 'ultrafast',
                            '-crf', '35', '-y', str(path)], check=True)
        fixtures[offset] = path
    jobs = list(cases('full' if a.suite in ('full', 'relevant') else 'core'))
    if a.suite == 'pilot':
        wanted = []
        for mode in (1, 2, 3):
            for actor in ('sora', 'riku'):
                for kind, tab, route, resource in [('mapped', 0, 'finish', 'valid'), ('native', 2, 'cancel', 'valid'),
                                                  ('mapped', 2, 'skip', 'valid'), ('mapped', 1, 'finish', 'error')]:
                    wanted.append(next(c for c in jobs if (c['mode'], c['actor'], c['kind'], c['tab'], c['exit'], c['resource'], c['context'], c['scenes']) ==
                                       (mode, actor, kind, tab, route, resource, 'game', 1)))
        jobs = wanted
    grouped = equivalent_groups(jobs) if a.suite == 'relevant' else [dict(case=c, equivalents=[key(c)], reasons=[]) for c in jobs]
    coverage = {key(g['case']): dict(equivalents=g['equivalents'], reasons=g['reasons']) for g in grouped}
    jobs = [g['case'] for g in grouped]
    results = a.work / 'results.jsonl'
    manifest = dict(exe_sha256=sha(a.exe), rom_sha256=sha(a.rom), suite=a.suite,
                    save_sha256=sha(a.save) if a.save else None,
                    fixtures={str(o): sha(path) for o, path in fixtures.items()}, coverage=coverage)
    manifest_path = a.work / 'manifest.json'
    if results.exists():
        if not a.resume:
            p.error('Existing results require --resume or a fresh --work directory')
        if not manifest_path.exists():
            p.error('Cannot resume without the original manifest')
        previous = json.loads(manifest_path.read_text(encoding='utf-8'))
        if any(previous.get(k) != v for k, v in manifest.items()):
            p.error('Resume fingerprints differ: use a fresh work directory for this binary/configuration')
    done = set()
    if a.resume and results.exists():
        done = {r['id'] for line in results.read_text(encoding='utf-8').splitlines() if (r := json.loads(line))['status'] in ('PASS', 'N/A')}
    jobs = [c for c in jobs if key(c) not in done]
    manifest['cases_total'] = len(jobs) + len(done)
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding='utf-8')
    pools = [a.work / f'worker-{i}' for i in range(a.workers)]
    for pool in pools:
        prepare(pool, a.exe, a.rom)
    lock = threading.Lock()
    counts = dict(PASS=0, FAIL=0, **{'N/A': 0})
    pending = iter(jobs)
    def worker(pool):
        while True:
            with lock:
                case = next(pending, None)
            if case is None:
                return
            result = run_case(case, pool, a.work, fixtures, a.save, screenshots=a.suite == 'pilot')
            result['exe_sha256'] = manifest['exe_sha256']
            result['equivalent_inputs'] = coverage[result['id']]
            with lock:
                with results.open('a', encoding='utf-8') as f:
                    f.write(json.dumps(result, ensure_ascii=False) + '\n')
                counts[result['status']] += 1
                if result['status'] == 'FAIL' or sum(counts.values()) % 25 == 0:
                    print(counts, result['id'], result.get('error', ''), flush=True)
    with concurrent.futures.ThreadPoolExecutor(a.workers) as executor:
        list(executor.map(worker, pools))
    print(json.dumps(counts), flush=True)
    return int(counts['FAIL'] != 0)


if __name__ == '__main__':
    raise SystemExit(main())
