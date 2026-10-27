"""Serial hidden/silent HD vs native profiling; output never touches a player's install.

Measures existing frame instrumentation and process RSS/private bytes. These are
process metrics, not dedicated GPU memory. Paths/options/build hashes are retained.
"""
import argparse
import csv
import ctypes
import datetime
import hashlib
import json
import math
import os
from pathlib import Path
import re
import shutil
import statistics
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
SCENARIOS = {
    'title': {'KHCOM_MODE': '60:mode_title'},
    'monstro-battle': {'KHCOM_BATTLE': '30:5:0'},
    'monstro-battle2': {'KHCOM_BATTLE': '31:5:0'},
    'monstro-opening': {'KHCOM_MODE': '60:room:10303,120:fieldfreeze,200:eventplay:74'},
    'traverse-door': {'KHCOM_MODE': '60:room:10901,120:fieldfreeze,200:doorpose:1'},
}


def memory(process):
    if os.name == 'nt':
        from ctypes import wintypes
        class Counters(ctypes.Structure):
            _fields_ = [('cb', wintypes.DWORD), ('faults', wintypes.DWORD)] + [
                (name, ctypes.c_size_t) for name in ('peak_rss', 'rss', 'peak_paged', 'paged',
                    'peak_nonpaged', 'nonpaged', 'commit', 'peak_commit', 'private')]
        counters = Counters(); counters.cb = ctypes.sizeof(counters)
        api = ctypes.windll.psapi.GetProcessMemoryInfo
        api.argtypes = [wintypes.HANDLE, ctypes.c_void_p, wintypes.DWORD]
        if api(int(process._handle), ctypes.byref(counters), counters.cb):
            return int(counters.rss), int(counters.private), int(counters.peak_rss)
    elif Path(f'/proc/{process.pid}/status').exists():
        status = Path(f'/proc/{process.pid}/status').read_text()
        return linux_memory(status)
    return 0, None, 0


def percentile(values, p):
    return sorted(values)[min(len(values)-1, int((len(values)-1)*p))] if values else None


def linux_memory(status):
    # VmData is virtual data size, not private committed/resident memory.
    values = {k: int(v)*1024 for k,v in re.findall(r'^(VmRSS|VmHWM):\s+(\d+)', status, re.M)}
    return values.get('VmRSS', 0), None, values.get('VmHWM', 0)


def summarize_frames(rows, expected_frames, warm_start=240, min_warm=120):
    if not rows:
        raise ValueError('empty frame log')
    parsed = []
    for row in rows:
        frame, logic = int(row['frame']), int(row['logic'])
        work, interval = float(row['render_work_ms']), float(row['interval_ms'])
        if frame < 0 or logic < 0 or not all(math.isfinite(n) for n in (work, interval)) or work < 0 or interval <= 0:
            raise ValueError('non-finite/negative frame measurements')
        parsed.append((frame, logic, work, interval))
    if any(b[0] != a[0]+1 for a,b in zip(parsed, parsed[1:])):
        raise ValueError('duplicate, unordered or missing frame samples')
    # VBlank logging omits its first interval; Quit runs before the next write.
    if parsed[0][0] > 3 or parsed[-1][0] < expected_frames-1 or parsed[-1][0] > expected_frames+1 or len(parsed) < expected_frames-2:
        raise ValueError('truncated frame coverage')
    cold = [r[2] for r in parsed if r[0] < warm_start]
    warm = [r for r in parsed if r[0] >= warm_start]
    if len(warm) < min_warm:
        raise ValueError('insufficient warm samples')
    result = dict(frames=len(parsed), cold_samples=len(cold), warm_samples=len(warm),
                  cold_max_ms=max(cold, default=0),
                  logic_steps=sorted(set(b[1]-a[1] for a,b in zip(warm, warm[1:]))))
    for name, values in [('work', [r[2] for r in warm]), ('interval', [r[3] for r in warm]), ('cold_work', cold)]:
        result[name+'_mean_ms'] = statistics.mean(values) if values else None
        for label, q in [('p50', .5), ('p95', .95), ('p99', .99)]:
            result[name+'_'+label+'_ms'] = percentile(values, q)
    return result


def validate_pacing(metrics, fast=False, quality_capture=False):
    if not fast and not quality_capture and metrics['logic_steps'] != [1]:
        raise ValueError('warm logic advances differ from one; cannot score normal pacing')


def validate_case_log(log, scenario, strict_hd, require_progress=False):
    expected = {'title':'mode_title', 'monstro-battle':'mode_battle', 'monstro-battle2':'mode_battle',
                'monstro-opening':'Event'}
    if scenario in expected and not re.search(r'harness: frame \d+ mode '+re.escape(expected[scenario])+r'(?:\s|$)', log):
        raise ValueError('requested scenario mode was not reached: '+scenario)
    if scenario == 'traverse-door' and 'harness: door pose side=' not in log:
        raise ValueError('door fixture unavailable')
    progress = re.findall(r'harness: progress [^\n]*world=(\d+)[^\n]*', log)
    if require_progress and len(progress) < 2:
        raise ValueError('effective progress/world snapshots missing')
    expected_world = {'monstro-opening':5, 'traverse-door':10}.get(scenario)
    if require_progress and expected_world is not None and any(int(world) != expected_world for world in progress):
        raise ValueError('effective WorldId differs from workload: '+scenario)
    if strict_hd and re.search(r'HD sprites:.*reject|HD text:.*(?:missing|failed|unavailable)', log, re.I):
        raise ValueError('HD resources degraded; use --allow-degraded only for an explicitly labelled fallback run')


def sha256(path):
    digest = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024*1024), b''):
            digest.update(block)
    return digest.hexdigest()


def tree_fingerprint(path):
    files = []
    for entry in sorted(Path(path).rglob('*')):
        if entry.is_file(): files.append((entry.relative_to(path).as_posix(), entry.stat().st_size, sha256(entry)))
    return dict(files=len(files), bytes=sum(f[1] for f in files),
                sha256=hashlib.sha256(json.dumps(files, separators=(',', ':')).encode()).hexdigest())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, default=ROOT/'build/pc-eu')
    parser.add_argument('--hd-sprites', type=Path, required=True)
    parser.add_argument('--assets', type=Path, default=ROOT/'port/assets')
    parser.add_argument('--exe', type=Path, help='explicit host executable (default BUILD/khcom.exe)')
    parser.add_argument('--guest', type=Path, help='Windows64 guest image; required for that host')
    parser.add_argument('--rom', type=Path, default=ROOT/'roms/B8CP.gba')
    parser.add_argument('--warm-start', type=int, default=240)
    parser.add_argument('--min-warm', type=int, default=120)
    parser.add_argument('--allow-degraded', action='store_true')
    parser.add_argument('--quality-capture', action='store_true', help='separate QA run; screenshot IO makes timing non-comparable')
    parser.add_argument('--output-root', type=Path, default=ROOT/'build/hd-resource-profile')
    parser.add_argument('--scenarios', default='title,monstro-battle,monstro-battle2,traverse-door')
    parser.add_argument('--frames', type=int, default=900)
    parser.add_argument('--fast', action='store_true', help='throughput only; interval is not real-time frame pacing')
    parser.add_argument('--software', action='store_true', help='no GPU; results are software-renderer only')
    args=parser.parse_args()
    build=args.build.resolve()
    source_exe=(args.exe or build/'khcom.exe').resolve()
    if args.frames < args.warm_start+args.min_warm+2 or args.min_warm < 2 or args.warm_start < 2:
        parser.error('frames must cover cold and requested warm samples')
    if any(case not in SCENARIOS for case in args.scenarios.split(',')):
        parser.error('unknown scenario')
    if not args.hd_sprites.is_dir() or not (args.assets/'fonts/KHMenu.otf').is_file():
        parser.error('complete explicit sprite directory and common assets/fonts/KHMenu.otf required')
    for path in [source_exe, build/'SDL3.dll', args.rom, *([args.guest] if args.guest else [])]:
        if not path.is_file(): parser.error('missing input: '+str(path))
    out=args.output_root.resolve()/datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f')
    out.mkdir(parents=True)
    shutil.copytree(args.assets, out/'assets')
    report={'schema':2,'exe_sha256':sha256(source_exe),
            'dll_sha256':sha256(build/'SDL3.dll'), 'rom_sha256':sha256(args.rom),
            'guest_sha256':sha256(args.guest) if args.guest else None,
            'assets_fingerprint':tree_fingerprint(args.assets), 'sprites_fingerprint':tree_fingerprint(args.hd_sprites),
            'memory_scope':'process RSS/working set; Windows PrivateUsage commit, unavailable on Linux status; NOT VRAM',
            'sample_scope':'VBlank outer work includes event pump/modal waits; not per-menu-frame or HD movie decode timing',
            'cold_scope':'first process frames; OS/filesystem cache not flushed or claimed cold',
            'paced':not args.fast, 'timing_comparable':not args.fast and not args.quality_capture,
            'quality_capture':args.quality_capture, 'quality_pair_verified':False, 'percentile_method':'lower order statistic: floor((n-1)*q)',
            'build':str(build),'options':vars(args).copy(),'cases':[]}
    report['options']={k:str(v) if isinstance(v,Path) else v for k,v in report['options'].items()}
    for scenario in args.scenarios.split(','):
        for hd in (0,1):
            folder=out/f'{scenario}-hd{hd}'; folder.mkdir()
            shutil.copy2(source_exe,folder/'khcom.exe')
            shutil.copy2(build/'SDL3.dll',folder/'SDL3.dll')
            if args.guest: shutil.copy2(args.guest,folder/'guest.bin')
            (folder/'kh_com.ini').write_text('config_version=11\ngraphics_mode=1\ngraphics_mode_applied=1\n'
                f'view_mode=4\nzoom=8\nvsync=0\nfullscreen=0\nexperimental_frame_skip=0\n'
                f'hd_hud={hd}\nhd_text={hd}\nhd_cards={hd}\nhd_title={hd}\nhd_music=0\nhd_movies=0\nhd_scenes=0\n')
            env={k:v for k,v in os.environ.items() if not k.startswith(('KHCOM_','SDL_'))}
            env.update(SDL_AUDIO_DRIVER='dummy', KHCOM_HIDDEN='1', KHCOM_BACKGROUND_TEST='1',
                KHCOM_HOST_HIDDEN='1', KHCOM_ASSETS=str((out/'assets').resolve()), KHCOM_WINDOW='1920x1080', KHCOM_PPU_THREADS='1', KHCOM_HD_HUD_DEV='1',
                KHCOM_FRAMELOG='frames.csv', KHCOM_QUIT=str(args.frames),KHCOM_TRACE_MODES='1',
                KHCOM_KEYS=','.join(f'{f}-{f+2}:A' for f in range(240,args.frames,40)), **SCENARIOS[scenario])
            initial_mode=env.get('KHCOM_MODE', '')
            env['KHCOM_MODE']=(initial_mode+',' if initial_mode else '')+f'{args.warm_start}:progress,{args.frames-2}:progress'
            # Do not enter title submenus: their modal input loop does not use
            # the gameplay frame limit. Keep this benchmark on the title scene.
            if scenario == 'title':
                env['KHCOM_KEYS']=''
            if args.quality_capture: env['KHCOM_PRESENTSHOTS']=f'{args.frames-1}-{args.frames-1}'
            if args.fast: env['KHCOM_FAST']='1'
            if args.software: env['SDL_VIDEO_DRIVER']='dummy'
            if args.hd_sprites: env['KHCOM_HD_SPRITES']=str(args.hd_sprites.resolve())
            began=time.monotonic(); peak_rss=0; peak_private=None
            with (folder/'stderr.txt').open('w') as stderr:
                command=[str(folder/'khcom.exe')]
                if args.guest: command += [str(folder/'guest.bin'), '--data', str(folder)]
                command += ['--rom', str(args.rom.resolve()), '--language','es']
                process=subprocess.Popen(command,cwd=folder,env=env,stdout=stderr,stderr=stderr,
                    creationflags=0x08004000 if os.name=='nt' else 0)
                with (folder/'memory.csv').open('w',newline='') as memfile:
                    writer=csv.writer(memfile); writer.writerow(('seconds','rss_bytes','private_bytes'))
                    while process.poll() is None:
                        rss,private,high=memory(process)
                        peak_rss=max(peak_rss,rss,high)
                        if private is not None: peak_private=max(peak_private or 0,private)
                        writer.writerow((round(time.monotonic()-began,3),rss,private))
                        if time.monotonic()-began>180:
                            process.kill(); process.wait(); raise RuntimeError(f'{scenario}: timeout; {folder}')
                        time.sleep(.05)
            log=(folder/'kh_com.log').read_text(errors='replace')
            assert process.returncode==0 and 'CRASH' not in log, (scenario,process.returncode,folder)
            validate_case_log(log, scenario, bool(hd and not args.allow_degraded), require_progress=True)
            if hd and 'HD text: font ' not in log:
                raise ValueError('HD text requested but no loaded font evidence: '+str(folder))
            if args.quality_capture and not (folder/f'present_{args.frames-1:05d}.bmp').is_file():
                raise ValueError('quality capture unavailable: '+str(folder))
            with (folder/'frames.csv').open() as stream:
                metrics=summarize_frames(list(csv.DictReader(stream)), args.frames, args.warm_start, args.min_warm)
            validate_pacing(metrics, args.fast, args.quality_capture)
            case=dict(scenario=scenario,hd=hd,seconds=round(time.monotonic()-began,3),
                **metrics, peak_rss_mib=peak_rss/1048576 if peak_rss else None,
                peak_private_mib=peak_private/1048576 if peak_private is not None else None,
                allow_degraded=args.allow_degraded,
                settings=next((line for line in log.splitlines() if line.startswith('Settings:')),''),
                renderer=next((line for line in log.splitlines() if line.startswith('Renderer:')),''),
                host_renderer=next((line for line in (folder/'stderr.txt').read_text(errors='replace').splitlines() if 'renderer:' in line.lower()),''),
                modes=[line for line in log.splitlines() if line.startswith('harness: frame')],
                effective_progress=[line for line in log.splitlines() if line.startswith('harness: progress ')],
                fixture=dict(SCENARIOS[scenario]),
                evidence_sha256={p.name:sha256(p) for p in folder.iterdir() if p.is_file() and p.suffix in ('.log','.csv','.bmp','.txt')})
            report['cases'].append(case)
            (out/'report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
            print(json.dumps(case),flush=True)
    print(f'Report: {out}',flush=True)


if __name__=='__main__':
    main()
