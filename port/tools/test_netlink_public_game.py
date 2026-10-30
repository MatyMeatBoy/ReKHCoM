"""Opt-in actual-game smoke through a relay; writes only a fresh evidence folder.

Two hidden game processes use owned ROM read-only. Not cross-region validation.
No global process cleanup; on timeout only the children started here are stopped.
"""
import argparse
import json
import os
from pathlib import Path
import re
import secrets
import shutil
import subprocess
import time


def below_normal(process):
    # The game raises its own priority at startup. Keep this test's own children
    # below normal as requested for shared-machine QA; never touch other games.
    if os.name == 'nt' and process.poll() is None:
        import ctypes
        kernel = ctypes.WinDLL('kernel32', use_last_error=True)
        kernel.SetPriorityClass.argtypes = [ctypes.c_void_p, ctypes.c_ulong]
        kernel.SetPriorityClass.restype = ctypes.c_int
        if not kernel.SetPriorityClass(ctypes.c_void_p(int(process._handle)), 0x4000):
            raise ctypes.WinError(ctypes.get_last_error())


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--build', type=Path, required=True)
    ap.add_argument('--rom', type=Path, required=True)
    ap.add_argument('--out', type=Path, required=True)
    ap.add_argument('--server', default='45.8.207.150')
    ap.add_argument('--port', type=int, default=47800)
    ap.add_argument('--frames', type=int, default=2400)
    ap.add_argument('--impaired', action='store_true')
    ap.add_argument('--lobby', action='store_true')
    a = ap.parse_args()
    a.out.mkdir(parents=True, exist_ok=False)
    room = 'QA'+secrets.token_hex(5).upper()
    processes, outputs = [], []
    try:
        for i in range(2):
            work = a.out/('p%d' % (i+1))
            work.mkdir()
            for name in ('khcom.exe', 'SDL3.dll'):
                shutil.copy2(a.build/name, work/name)
            (work/'kh_com.ini').write_text('config_version=2\ngraphics_mode=0\nnet_delay=4\nnet_stats=1\nonline_name=QA%d\n' % (i+1))
            keys = ','.join('%d-%d:A' % (f, f+3) for f in range(200, 1450, 40))
            if i == 0:
                keys += ',1600-1700:RIGHT,1800-1900:LEFT'
            env = dict(os.environ, SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy',
                       KHCOM_NETLINK='relay:%s:%d:%s:%s' % (a.server, a.port, room, 'host' if i == 0 else 'join'),
                       KHCOM_NETLINK_TRACE='1', KHCOM_NETLINK_DELAY='4', KHCOM_NETLINK_GIVEUP='25',
                       KHCOM_PPU_THREADS='1', KHCOM_MODE='60:mode_sio_battle:1', KHCOM_KEYS=keys, KHCOM_QUIT=str(a.frames))
            if a.impaired:
                env.update(KHCOM_NETLINK_LAG='4', KHCOM_NETLINK_JITTER='6', KHCOM_NETLINK_LOSS='5')
            if a.lobby:
                env.pop('KHCOM_NETLINK')
                env.update(KHCOM_LOBBY='lobby:%s:%d' % (a.server, a.port),
                           KHCOM_LOBBY_KEYS='40:3' if i == 0 else '400:1,410:3',
                           KHCOM_LOBBY_SHOTS='20,390')
            output = (work/'stdout.txt').open('w')
            outputs.append(output)
            processes.append(subprocess.Popen([str((work/'khcom.exe').resolve()), '--language', 'es',
                                               '--rom', str(a.rom.resolve())], cwd=work, env=env,
                                              stdout=output, stderr=subprocess.STDOUT,
                                              creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0) |
                                              getattr(subprocess, 'BELOW_NORMAL_PRIORITY_CLASS', 0)))
            if i == 0:
                time.sleep(1)
        deadline = time.monotonic()+180
        play_logs = None
        while any(p.poll() is None for p in processes) and time.monotonic() < deadline:
            if play_logs is None and any(p.poll() is not None for p in processes):
                # Global game frames differ while the host waits in the lobby.
                # The first KHCOM_QUIT deliberately pulls that player's cable;
                # evaluate play before that departure, and report teardown apart.
                play_logs = [(a.out/('p%d' % (i+1))/'kh_com.log').read_text(encoding='utf-8', errors='replace')
                             for i in range(2)]
            for process in processes:
                below_normal(process)
            time.sleep(.5)
        result = {'room': room, 'endpoint': '%s:%d/udp' % (a.server, a.port),
                  'impaired': a.impaired, 'lobby': a.lobby, 'players': [],
                  'scope': 'two local actual-game processes through public VPS, not physical cross-region proof'}
        for i, process in enumerate(processes):
            log_path = a.out/('p%d' % (i+1))/'kh_com.log'
            log = log_path.read_text(encoding='utf-8', errors='replace') if log_path.exists() else ''
            play = play_logs[i] if play_logs else log
            checks = re.findall(r'netlink: (\d+) state checks, (\d+) desync', play)
            battles = re.findall(r'netlink: battle (\d+) frames', play)
            if a.lobby:
                picked = re.search(r'the lobby picked relay:[^\r\n]+:([A-Z0-9]+):(?:host|join)', log)
                if i == 0:
                    result['room'] = picked[1] if picked else None
                elif not picked or picked[1] != result['room']:
                    raise AssertionError('the lobby did not select the same room')
            player = {'exit': process.poll(), 'checks': checks[-1] if checks else None,
                      'battle_frames': battles[-1] if battles else None,
                      'desync': 'netlink: DESYNC' in log, 'connected': 'netlink: connected' in play,
                      'link_error_during_play': 'mode: mode_sioError' in play,
                      'wait_abandoned_during_play': 'the player gave up waiting' in play,
                      'post_peer_departure_link_error': 'mode: mode_sioError' in log and 'mode: mode_sioError' not in play}
            player['pass'] = player['exit'] == 0 and player['connected'] and bool(checks) and \
                             int(checks[-1][0]) >= 10 and int(checks[-1][1]) == 0 and not player['desync'] and \
                             not player['link_error_during_play'] and not player['wait_abandoned_during_play'] and bool(battles)
            result['players'].append(player)
        result['pass'] = all(p['pass'] for p in result['players'])
        (a.out/'RESULT.json').write_text(json.dumps(result, indent=2))
        print(json.dumps(result), flush=True)
        return 0 if result['pass'] else 1
    finally:
        for process in processes:
            if process.poll() is None:
                process.terminate()
                process.wait(timeout=10)
        for output in outputs:
            output.close()


if __name__ == '__main__':
    raise SystemExit(main())
