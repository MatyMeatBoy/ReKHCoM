"""Capture missing catalog views sequentially from an identified build, in private fresh data folders.

This produces evidence only: it never edits the review desk or player saves.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parents[3]
ADVANCE = ','.join(f'{f}-{f+2}:A' for f in range(100, 1200, 100))
MENU = '60:room:10901,120:fieldfreeze,180:'
CASES = {
    'portrait-sora': dict(title='Retratos de diálogo · Sora / León', frames=[600, 900, 1200],
        env=dict(KHCOM_EVENT='0:4', KHCOM_KEYS=ADVANCE), reference='test_portrait_clipping.py'),
    'portrait-riku': dict(title='Retratos de diálogo · Riku / DiZ', frames=[900, 1200],
        env=dict(KHCOM_EVENT='1:0', KHCOM_KEYS=ADVANCE), reference='test_portrait_clipping.py'),
    'title-original': dict(title='Título original · barras y brillo', frames=[600, 900],
        env=dict(KHCOM_MODE='60:mode_title', KHCOM_KEYS='700-702:START', KHCOM_MENU_FRAMECLOCK='1'),
        reference='test_title_bands.py'),
    'language': dict(title='Selección de idioma · márgenes', frames=[120], lang=None, env={},
        reference='test_ppu.py: lang'),
    'copyright': dict(title='Copyright · presentación', frames=[80], env={},
        reference='test_tas_boot_sync.py: mode_copyright1'),
    'worldcards': dict(title='Naipes mundo · selección y márgenes', frames=[700, 900],
        env=dict(KHCOM_MODE=MENU+'mode_worldinspect'), reference='refresh_readme_screenshots.py: worldcards'),
    'hollow17': dict(title='Hollow Bastion · borde superior de Riku B1 sala 3', frames=[1450, 1560],
        env=dict(KHCOM_MODE='300:roomriku:10700,800:roomshape:18,801:roomseed:138815524,900:roomgoto:3',
                 KHCOM_KEYS='1400-1520:LEFT'), reference='HANDOFF_CLAUDE_20261018.md: reproducción exacta #17'),
    'field': dict(title='Campo · HUD, minimapa y márgenes', frames=[260, 400],
        env=dict(KHCOM_MODE='60:room:10901,120:fieldfreeze', KHCOM_KEYS='130-170:UP,171-210:RIGHT'),
        reference='refresh_readme_screenshots.py: field'),
    'battle-original': dict(title='Combate original · HUD y arena', frames=[300, 600],
        env=dict(KHCOM_BATTLE='10:4:0', KHCOM_KEYS='60-63:A'), reference='refresh_readme_screenshots.py: battle'),
    'movie': dict(title='Película original · encuadre y subtítulos', frames=[480, 2850],
        env=dict(KHCOM_MOVIE='1'), reference='refresh_readme_screenshots.py: movie_extended / subtitles_hd'),
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def capture(name, case, build, rom, out, commit):
    work = out / name
    work.mkdir(parents=True, exist_ok=False)
    for file in ('khcom.exe', 'SDL3.dll'):
        shutil.copy2(build / file, work / file)
    (work / 'kh_com.ini').write_text('config_version=11\nview_mode=4\nzoom=6\nscale_mode=0\n'
        'hd_text=0\nhd_cards=0\nhd_hud=0\nhd_movies=0\nhd_scenes=0\nfullscreen=0\nvsync=0\n', encoding='utf-8')
    env = {k: v for k, v in os.environ.items() if not k.startswith(('KHCOM_', 'SDL_'))}
    frames = case['frames']
    env.update(SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy', KHCOM_FAST='1', KHCOM_TRACE_MODES='1',
               KHCOM_WINDOW='1920x1080', KHCOM_SHOTS=','.join(map(str, frames)),
               KHCOM_OUTPUT_SHOT=','.join(map(str, frames)), KHCOM_QUIT=str(max(frames)+5), **case['env'])
    command = [str(work / 'khcom.exe'), '--rom', str(rom), '--data-dir', str(work)]
    if case.get('lang', 'es'):
        command += ['--language', case.get('lang', 'es')]
    run = dict(case=name, title=case['title'], source_commit=commit, build_sha256=sha(build/'khcom.exe'),
               fixture_reference=case['reference'], fixture=case['env'], frames=frames,
               captured_at=datetime.now(timezone.utc).isoformat(), scope='EU, 1920x1080, x3, original art; fresh save',
               approval='pending; current capture does not inherit historical approval')
    result = subprocess.run(command, cwd=work, env=env, timeout=240,
                            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                            creationflags=0x08000000 if os.name == 'nt' else 0)
    log = (work / 'kh_com.log').read_text(encoding='utf-8', errors='replace')
    run['exit_code'] = result.returncode
    run['shots'] = [dict(path=str(work / f'out_{f:05d}.bmp'), sha256=sha(work/f'out_{f:05d}.bmp'))
                    for f in frames if (work/f'out_{f:05d}.bmp').is_file()]
    run['ok'] = result.returncode == 0 and len(run['shots']) == len(frames) and not any(
        marker in log for marker in ('CRASH', 'harness: no mode', 'bios:'))
    (work / 'capture.json').write_text(json.dumps(run, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    if not run['ok']:
        raise RuntimeError('Capture failed; retain evidence and investigate: ' + name)
    print(name + ': captured ' + str(len(frames)) + ' real frames', flush=True)
    return run


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build', type=Path, default=ROOT/'build/pc')
    p.add_argument('--rom', type=Path, default=ROOT/'roms/B8CP.gba')
    p.add_argument('--out', type=Path, required=True, help='Fresh output directory (never reuses a run)')
    p.add_argument('--only', nargs='+', choices=sorted(CASES), default=list(CASES))
    a = p.parse_args()
    commit = subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True).strip()
    dirty = subprocess.check_output(['git', '-C', str(ROOT), 'diff', '--name-only', 'HEAD', '--',
                                     'src', 'include', 'port/src', 'port/include'], text=True).strip()
    if dirty:
        p.error('Game sources differ from recorded commit; commit them or use a clean build checkout')
    out = a.out.resolve()
    out.mkdir(parents=True, exist_ok=False)
    results = [capture(name, CASES[name], a.build.resolve(), a.rom.resolve(), out, commit) for name in a.only]
    (out/'report.json').write_text(json.dumps(results, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')


if __name__ == '__main__':
    main()
