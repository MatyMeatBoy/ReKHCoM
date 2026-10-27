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
    'monstro-opening': {'KHCOM_EVENT': '0:74'},
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
        values = {k: int(v)*1024 for k,v in re.findall(r'^(VmRSS|VmHWM|VmData):\s+(\d+)', status, re.M)}
        return values.get('VmRSS',0), values.get('VmData',0), values.get('VmHWM',0)
    return 0, 0, 0


def percentile(values, p):
    return sorted(values)[min(len(values)-1, int((len(values)-1)*p))] if values else None


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, default=ROOT/'build/pc-eu')
    parser.add_argument('--hd-sprites', type=Path)
    parser.add_argument('--scenarios', default='title,monstro-battle,monstro-battle2,traverse-door')
    parser.add_argument('--frames', type=int, default=900)
    parser.add_argument('--fast', action='store_true', help='throughput only; interval is not real-time frame pacing')
    parser.add_argument('--software', action='store_true', help='no GPU; results are software-renderer only')
    args=parser.parse_args()
    build=args.build.resolve()
    out=ROOT/'build/hd-resource-profile'/datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f')
    out.mkdir(parents=True)
    report={'exe_sha256':hashlib.sha256((build/'khcom.exe').read_bytes()).hexdigest(),
            'build':str(build),'options':vars(args).copy(),'cases':[]}
    report['options']={k:str(v) if isinstance(v,Path) else v for k,v in report['options'].items()}
    for scenario in args.scenarios.split(','):
        for hd in (0,1):
            folder=out/f'{scenario}-hd{hd}'; folder.mkdir()
            for name in ('khcom.exe','SDL3.dll'):
                shutil.copy2(build/name,folder/name)
            (folder/'kh_com.ini').write_text('config_version=11\ngraphics_mode=1\ngraphics_mode_applied=1\n'
                f'view_mode=4\nzoom=8\nvsync=0\nfullscreen=0\nexperimental_frame_skip=0\n'
                f'hd_hud={hd}\nhd_text={hd}\nhd_cards={hd}\nhd_title={hd}\nhd_music=0\nhd_movies=0\nhd_scenes=0\n')
            env={k:v for k,v in os.environ.items() if not k.startswith(('KHCOM_','SDL_'))}
            env.update(SDL_AUDIO_DRIVER='dummy', KHCOM_HIDDEN='1', KHCOM_BACKGROUND_TEST='1',
                KHCOM_WINDOW='1920x1080', KHCOM_PPU_THREADS='1', KHCOM_HD_HUD_DEV='1',
                KHCOM_FRAMELOG='frames.csv', KHCOM_QUIT=str(args.frames),KHCOM_TRACE_MODES='1',
                KHCOM_PRESENTSHOTS=f'{args.frames-1}-{args.frames-1}',
                KHCOM_KEYS=','.join(f'{f}-{f+2}:A' for f in range(240,args.frames,40)), **SCENARIOS[scenario])
            # Do not enter title submenus: their modal input loop does not use
            # the gameplay frame limit. Keep this benchmark on the title scene.
            if scenario == 'title':
                env['KHCOM_KEYS']=''
            if args.fast: env['KHCOM_FAST']='1'
            if args.software: env['SDL_VIDEO_DRIVER']='dummy'
            if args.hd_sprites: env['KHCOM_HD_SPRITES']=str(args.hd_sprites.resolve())
            began=time.monotonic(); peak_rss=peak_private=0
            with (folder/'stderr.txt').open('w') as stderr:
                process=subprocess.Popen([str(folder/'khcom.exe'),'--rom',str(ROOT/'roms/B8CP.gba'),
                    '--language','es'],cwd=folder,env=env,stdout=stderr,stderr=stderr,
                    creationflags=0x08004000 if os.name=='nt' else 0)
                with (folder/'memory.csv').open('w',newline='') as memfile:
                    writer=csv.writer(memfile); writer.writerow(('seconds','rss_bytes','private_bytes'))
                    while process.poll() is None:
                        rss,private,high=memory(process)
                        peak_rss=max(peak_rss,rss,high); peak_private=max(peak_private,private)
                        writer.writerow((round(time.monotonic()-began,3),rss,private))
                        if time.monotonic()-began>180:
                            process.kill(); process.wait(); raise RuntimeError(f'{scenario}: timeout; {folder}')
                        time.sleep(.05)
            log=(folder/'kh_com.log').read_text(errors='replace')
            assert process.returncode==0 and 'CRASH' not in log, (scenario,process.returncode,folder)
            if scenario=='traverse-door':
                assert 'harness: door pose side=' in log, f'door fixture unavailable: {folder}'
            rows=list(csv.DictReader((folder/'frames.csv').open()))
            # Cold first 240 frames are reported separately, never hidden in a steady-state mean.
            cold=[float(r['render_work_ms']) for r in rows if int(r['frame'])<240]
            warm=[r for r in rows if int(r['frame'])>=240]
            work=[float(r['render_work_ms']) for r in warm]
            case=dict(scenario=scenario,hd=hd,frames=len(rows),seconds=round(time.monotonic()-began,3),
                cold_max_ms=max(cold,default=0),work_mean_ms=statistics.mean(work) if work else None,
                work_p95_ms=percentile(work,.95),work_p99_ms=percentile(work,.99),
                interval_p99_ms=percentile([float(r['interval_ms']) for r in warm],.99),
                peak_rss_mib=peak_rss/1048576,peak_private_mib=peak_private/1048576,
                settings=next((line for line in log.splitlines() if line.startswith('Settings:')),''),
                renderer=next((line for line in log.splitlines() if line.startswith('Renderer:')),''),
                modes=[line for line in log.splitlines() if line.startswith('harness: frame')])
            report['cases'].append(case)
            (out/'report.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
            print(json.dumps(case),flush=True)
    print(f'Report: {out}',flush=True)


if __name__=='__main__':
    main()
