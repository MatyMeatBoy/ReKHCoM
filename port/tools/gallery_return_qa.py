"""Bounded gallery-return QA. EU x86 only; hidden real renderer; private fixtures.

Build uses pristine production C plus forced-include presentation interception.
No launcher or release target is built. Readback invalidates performance claims.
"""
import argparse
import csv
import hashlib
import itertools
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
HEADER = 'port/tools/gallery_present_recorder.h'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def call(argv, **kwargs):
    subprocess.run([str(x) for x in argv], check=True, **kwargs)


def instrument(ninja):
    text = ninja.read_text()
    sources = sorted(p for p in (ROOT / 'port/src').glob('*.c')
                     if re.search(r'\bSDL_RenderPresent\s*\(', p.read_text(encoding='utf-8')))
    assert sources, 'No present callers discovered'
    # Per-edge flags: leave game and unrelated port TUs untouched.
    for source in sources:
        relative = source.relative_to(ROOT).as_posix()
        edge = re.search(r'^build [^\n]+: cc_port ' + re.escape(relative) + r'\n', text, re.M)
        assert edge, relative
        text = text[:edge.end()] + f'  port_cflags = $port_cflags -include {HEADER}\n' + text[edge.end():]
    link = re.search(r'^build \$builddir/khcom\.exe: link[^\n]*(?:\n  [^\n]*)*', text, re.M)
    assert link
    text = text[:link.end()] + ' $builddir/port/gallery_present_recorder.o' + text[link.end():]
    text = text.replace(': cc_port port/src/port_ui.c\n', ': cc_port port/tools/gallery_ui_probe.c\n')
    text += '\nbuild $builddir/port/gallery_present_recorder.o: cc_port port/tools/gallery_present_recorder.c\n'
    ninja.write_text(text)
    return [p.relative_to(ROOT).as_posix() for p in sources]


def build(a):
    out = ROOT / 'build/gallery-return-eu'
    (ROOT / 'roms').mkdir(exist_ok=True)
    rom = ROOT / 'roms/B8CP.gba'
    if not rom.exists():
        shutil.copy2(a.rom, rom)
    call([sys.executable, 'tools/baserom.py', 'eu'], cwd=ROOT)
    call([sys.executable, 'port/configure_pc.py', '--version', 'eu', '--out', out.relative_to(ROOT),
          '--toolchain', a.toolchain, '--sdl', a.sdl], cwd=ROOT)
    sources = instrument(out / 'build.ninja')
    with (a.work / 'build.log').open('w', encoding='utf-8') as log:
        call([a.ninja, '-f', out / 'build.ninja', '-j', '2',
              'build/gallery-return-eu/khcom.exe', 'build/gallery-return-eu/SDL3.dll'],
             cwd=ROOT, stdout=log, stderr=subprocess.STDOUT)
    manifest = dict(source_commit=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
                    region='eu', architecture='i686', scope='instrumented QA, no performance evidence',
                    exe_sha256=sha(out / 'khcom.exe'), sdl_sha256=sha(out / 'SDL3.dll'),
                    rom_sha256=sha(a.rom), compiler_sha256=sha(a.toolchain / 'bin/i686-w64-mingw32-clang.exe'),
                    hooked_sources=sources, recorder_sha256=sha(ROOT / 'port/tools/gallery_present_recorder.c'),
                    header_sha256=sha(ROOT / HEADER), ninja_sha256=sha(out / 'build.ninja'))
    manifest['ui_probe_sha256'] = sha(ROOT / 'port/tools/gallery_ui_probe.c')
    (a.work / 'build-manifest.json').write_text(json.dumps(manifest, indent=2))


def cases():
    for context, tab, route in itertools.product(('title', 'game'), (0, 2), ('finish', 'skip')):
        yield dict(context=context, tab=tab, route=route, resource='valid')
    yield dict(context='title', tab=1, route='finish', resource='valid')
    for resource in ('absent', 'corrupt'):
        yield dict(context='title', tab=2, route='skip', resource=resource)


def fingerprint(directory):
    names = ['kh_com.sav', 'kh_com.ini', 'scenes_seen.dat', 'kh_com_recom.ini', 'rebel_mode.txt']
    names += [p.name for p in directory.glob('achievements_*.dat')]
    return {n: sha(directory / n) if (directory / n).exists() else None for n in sorted(set(names))}


def run_case(a, case):
    ident = f"{case['context']}-tab{case['tab']}-{case['route']}-{case['resource']}"
    work = a.work / ident
    existing = work / 'report.json'
    if existing.exists():
        previous = json.loads(existing.read_text())
        assert previous['case'] == case and not previous['errors'], 'Existing failed/different case: use fresh --work'
        assert sha(work/'khcom.exe') == sha(ROOT/'build/gallery-return-eu/khcom.exe'), 'Existing result uses different executable'
        assert previous['before']['kh_com.sav'] == sha(a.save), 'Existing result uses different save'
        assert not a.config or previous['before']['kh_com.ini'] == sha(a.config), 'Existing result uses different config'
        assert all(sha(work/item['file']) == item['sha256'] for item in previous['images']), 'Capture fingerprint changed'
        print(ident, 'reuse verified identical capture', flush=True)
        return previous
    work.mkdir()
    exe = ROOT / 'build/gallery-return-eu/khcom.exe'
    for source, target in [(exe, 'khcom.exe'), (exe.parent / 'SDL3.dll', 'SDL3.dll'),
                           (a.rom, 'baserom.gba'), (a.save, 'kh_com.sav')]:
        shutil.copy2(source, work / target)
    # Read-only core assets are embedded by the fresh build. Optional gallery
    # posters stay in the explicit shared immutable fixture tree, via junction.
    if a.assets:
        call(['cmd', '/c', 'mklink', '/J', work / 'assets', a.assets], stdout=subprocess.DEVNULL)
    movie = work / 'hd/scenes/zz_event_012.mp4'
    if case['resource'] != 'absent' and case['tab']:
        movie.parent.mkdir(parents=True)
        if case['resource'] == 'corrupt':
            movie.write_bytes(b'Invalid private QA fixture')
        else:
            shutil.copy2(a.work / 'fixture.mp4', movie)
    (work / 'scenes_seen.dat').write_bytes(b'\xff' * 128)
    config = (
        'config_version=11\ngraphics_mode=2\ngraphics_mode_applied=2\nhd_title=0\nhd_text=1\n'
        'hd_hud=0\nhd_cards=0\nhd_scenes=1\nhd_movies=1\nhd_music=0\nassets_seen=255\n'
        'ps2_mode=0\nps2_difficulty=2\nfullscreen=0\nview_mode=4\nzoom=8\nvsync=0\n'
        'game_language=5\nsave_bank=1\n')
    (work / 'kh_com.ini').write_text(a.config.read_text() if a.config else config, encoding='utf-8')
    before = fingerprint(work)
    env = {k: v for k, v in os.environ.items() if not k.startswith(('KHCOM_', 'SDL_'))}
    env.update(SDL_AUDIO_DRIVER='dummy', SDL_RENDER_DRIVER='direct3d11', SDL_JOYSTICK_ALLOW_BACKGROUND_EVENTS='1', KHCOM_HIDDEN='1',
               KHCOM_BACKGROUND_TEST='1', KHCOM_WINDOW='640x360', KHCOM_PPU_THREADS='1',
               KHCOM_MODE='1:room:20000' if case['context'] == 'game' else '1:mode_title',
               KHCOM_OPTIONS_AT='120', KHCOM_SCENES_SHOT=str(case['tab'] + 1), KHCOM_GALLERY_EVENT='10',
               KHCOM_GALLERY_KEYS='30:120,80:-1', KHCOM_GALLERY_SHOTS='29,31,32,33',
               KHCOM_GALLERY_AUDIT='1', KHCOM_GALLERY_QUIT_AFTER='80', KHCOM_QUIT='8000',
               KHCOM_ACHIEVEMENTS_LIVE='1', KHCOM_VPAD='')
    if case['tab'] and case['resource'] == 'valid':
        if case['route'] == 'skip':
            env.update(KHCOM_HDMOVIE_PAUSETEST='0', KHCOM_HDMOVIE_PAUSE_ACTION='skip')
    else:
        env['KHCOM_GALLERY_AUTO_KEYS'] = 'finish'
        if case['route'] == 'skip':
            env['KHCOM_GALLERY_NATIVE_PAUSE'] = 'skip'
            # GBA bits in AUTO_KEYS do not open the SDL pause overlay. Use
            # the existing Options-entry harness, then its real confirm path.
            env['KHCOM_OPTIONS_TOGGLE'] = '240'
    started = time.monotonic()
    with (work / 'run.log').open('w', encoding='utf-8') as log:
        result = subprocess.run([str(work / 'khcom.exe')], cwd=work, env=env,
                                stdout=log, stderr=subprocess.STDOUT, timeout=180)
    text = (work / 'run.log').read_text(errors='replace')
    rows = list(csv.DictReader((work / 'qa-frames.csv').open()))
    after = fingerprint(work)
    audits = re.findall(r'scene gallery audit: game=(\d) map=(\d) config=(\d) files=(\d) recom=(\d) rank=(\d)', text)
    transitions = [i for i in range(1, len(rows)) if rows[i-1]['gallery'] == '1' and rows[i]['gallery'] == '0']
    errors = []
    if result.returncode: errors.append(f'exit={result.returncode}')
    if re.search(r'bios:|CRASH|FATAL', text): errors.append('crash/invalid memory marker')
    expected_audits = 2 if case['resource'] == 'corrupt' else 1
    if audits != [('1',) * 6] * expected_audits: errors.append(f'expected {expected_audits} successful audits: {audits}')
    if before != after: errors.append('persistent file fingerprints changed')
    if len(transitions) != 1: errors.append(f'expected one captured return, got {len(transitions)}')
    if transitions:
        first = rows[transitions[0]]
        if first['canvas'] != '1' or first['fade'] != '0': errors.append('first return is not full gallery canvas')
        entered = next((i for i,r in enumerate(rows) if r['gallery']=='1'), None)
        if entered:
            selection = ('choice','scroll','tab','character','event')
            if any(rows[entered-1][k] != first[k] for k in selection): errors.append('selection/tab/scroll/character/event changed')
        if case['route']=='skip' and not any(r['pause']=='1' and r['step']=='1' for r in rows):
            errors.append('skip confirmation page not captured')
    if not rows or {r['renderer'] for r in rows} != {'direct3d11'}: errors.append('real D3D11 renderer not confirmed')
    if 'tick=80 key=-1' not in text: errors.append('gallery final navigation/close missing')
    hd_rows = [r for r in rows if r['caller'].endswith('port_hdmovie.c')]
    if case['tab'] and case['resource'] == 'valid' and not hd_rows: errors.append('HD present interception not demonstrated')
    if case['route'] == 'skip' and case['tab'] and case['resource'] == 'valid':
        # The pause loop is a distinct RenderPresent call site from movie playback.
        if len({r['line'] for r in hd_rows}) < 2: errors.append('HD pause presentation not intercepted')
    from PIL import Image
    images = []
    for path in sorted(work.glob('qa-*.bmp')):
        png = path.with_suffix('.png')
        with Image.open(path) as im: im.save(png)
        path.unlink()
        images.append(dict(file=png.name, sha256=sha(png)))
    report = dict(case=case, status='FAIL' if errors else 'PASS-functional-capture-pending-visual-review', errors=errors,
                  exe_sha256=sha(exe), rom_sha256=sha(a.rom),
                  process_seconds=round(time.monotonic()-started, 2), exit=result.returncode,
                  before=before, after=after, audits=audits, transitions=[rows[i] for i in transitions],
                  hd_present_count=len(hd_rows), hd_present_sites=sorted({r['line'] for r in hd_rows}),
                  images=images, env=env_filter(env), fixture_sha256=sha(movie) if movie.exists() else None,
                  source_commit=json.loads((a.work/'build-manifest.json').read_text())['source_commit'])
    (work / 'report.json').write_text(json.dumps(report, indent=2))
    print(ident, report['status'], errors, flush=True)
    return report


def env_filter(env):
    return {k: v for k,v in env.items() if k.startswith(('KHCOM_', 'SDL_'))}


def contact_sheets(work):
    """Every retained frame, labelled with presentation serial and live state."""
    from PIL import Image, ImageDraw
    for report_file in sorted(work.glob('*/report.json')):
        directory = report_file.parent
        report = json.loads(report_file.read_text())
        rows = {r['present']:r for r in csv.DictReader((directory/'qa-frames.csv').open())}
        files = sorted(directory.glob('qa-return-*.png'))
        files = sorted(directory.glob('qa-before-*.png')) + files
        sheet = Image.new('RGB', (1280, ((len(files)+7)//8)*112+30), '#ddd')
        draw = ImageDraw.Draw(sheet)
        draw.text((4,8), directory.name+' | D3D11 hidden, instrumented QA; ALL retained frames', fill='black')
        for i,path in enumerate(files):
            row = rows[str(int(path.stem.rsplit('-',1)[1]))]
            x=(i%8)*160; y=(i//8)*112+30
            with Image.open(path) as im: sheet.paste(im.convert('RGB').resize((160,90)),(x,y))
            draw.text((x+2,y+92), f"#{row['present']} G{row['gallery']} C{row['canvas']} F{row['fade']} P{row.get('pause','?')} S{row.get('step','?')}",fill='black')
        sheet.save(directory/'contact.png')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--build', action='store_true')
    p.add_argument('--work', type=Path, required=True)
    p.add_argument('--rom', type=Path, required=True)
    p.add_argument('--save', type=Path, required=True)
    p.add_argument('--assets', type=Path)
    p.add_argument('--config', type=Path, help='Fully serialized INI from an isolated boot; avoids mistaking serialization for mutation')
    p.add_argument('--toolchain', type=Path)
    p.add_argument('--sdl', type=Path)
    p.add_argument('--ninja', type=Path)
    p.add_argument('--case', help='one explicit context-tabN-route-resource; default first HD skip proof')
    p.add_argument('--all', action='store_true')
    a = p.parse_args()
    a.work = a.work.resolve(); a.work.mkdir(parents=True, exist_ok=True)
    if a.build: build(a)
    fixture = a.work / 'fixture.mp4'
    if not fixture.exists():
        call(['ffmpeg', '-v', 'error', '-nostdin', '-f', 'lavfi', '-i', 'testsrc2=s=320x180:r=30',
              '-t', '2', '-an', '-c:v', 'libx264', '-preset', 'ultrafast', '-pix_fmt', 'yuv420p', '-y', fixture])
    selected = list(cases()) if a.all else [c for c in cases() if
        f"{c['context']}-tab{c['tab']}-{c['route']}-{c['resource']}" == (a.case or 'title-tab2-skip-valid')]
    assert selected, 'Unknown case'
    reports = [run_case(a, c) for c in selected]
    (a.work / 'results.json').write_text(json.dumps(reports, indent=2))
    contact_sheets(a.work)
    if any(r['errors'] for r in reports): sys.exit(1)


if __name__ == '__main__': main()
