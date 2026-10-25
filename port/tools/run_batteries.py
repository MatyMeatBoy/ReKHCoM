"""Runs the long test batteries one after another, resumable, outside the session that starts them.

    python port/tools/run_batteries.py --detach     # starts itself through WMI and returns at once

A battery started as a child of an agent session dies when that session ends (Windows ends the whole process tree:
adb fails with 0xC0000142, sh cannot fork, ninja gets 0x40010004). With --detach the process is created by WMI,
outside that tree, so it keeps going; each step runs with --resume, so starting it again carries on where it stopped.
Progress: build/batteries.log; a step's own output is in its log (build/debugkit/*.txt, build/android-scenarios).
"""
import argparse
import json
import os
import re
import subprocess
import sys
import time
from pathlib import Path
from workdir import WORK_DIR  # the heavy work folder (REKHCOM_WORK)

ROOT = Path(__file__).resolve().parents[2]
LOG = ROOT / 'build/batteries.log'
SDK = ROOT.parent / 'toolchain/android/sdk'
ADB = SDK / 'platform-tools/adb.exe'
PY = sys.executable
SCN = 'port/tools/test_android_scenarios.py'

# Small steps, each one suite of one version, so whoever picks the work up (Claude or Codex, sessions end on token
# limits) sees how far it got with --status and carries on: every case keeps its result on disk.
# Desktop first (what the user asked for, 21/10), then the Android emulator: both together do not fit in the RAM.
KIT = [PY, 'port/tools/debug_kit.py', 'run', '--resume', '--jobs', '2']
STEPS = [('desktop build', [PY, 'port/tools/debug_kit.py', 'run', '--build', '--versions', 'eu,us,jp', '--suites', ''])]
# scenes = the Android runner's menus and fixtures on the desktop build: every stray access the desktop recovers
# there ("bios:") closes the 64-bit Android app, so it runs before the emulator (21/10: the debug event select)
for _suite in ('scenes', 'events', 'wide', 'rooms', 'battles', 'riku', 'monkey', 'skip'):
    for _v in ('eu', 'us', 'jp'):
        STEPS.append((f'desktop {_suite} {_v}', KIT + ['--versions', _v, '--suites', _suite]))
STEPS += [
    ('android', None),
    # the players' reports (#22/#34 image moved, #33 Trickmaster scene) first, on the image loaded away from home
    ('android eu move', [PY, SCN, '--serial', 'emulator-5554', '--regions', 'eu', '--groups', 'issues', 'menus',
                         'fixtures', 'bosses', '--resume', '--slow', '6', '--env', 'KHCOM_IMAGE_MOVE=1']),
    ('android eu bosses', [PY, SCN, '--serial', 'emulator-5554', '--regions', 'eu', '--groups', 'bosses',
                           '--from-battle', '30', '--resume', '--slow', '6']),
    ('android eu bosses riku', [PY, SCN, '--serial', 'emulator-5554', '--regions', 'eu', '--groups', 'bosses',
                                '--riku', '--resume', '--slow', '6']),
    ('android us jp menus bosses', [PY, SCN, '--serial', 'emulator-5554', '--regions', 'us', 'jp', '--groups',
                                    'menus', 'bosses', '--resume', '--slow', '6']),
]


# --hd: the same desktop batteries with the HD mod on (debug_kit.py --hd), their own log and marks
HD = '--hd' in sys.argv
if HD:
    LOG = ROOT / 'build/batteries-hd.log'
    STEPS = [(n, c + ['--hd']) for n, c in STEPS if n.startswith('desktop')]


def progress(name):
    """'done/total' for a step, read from the cases it left on disk."""
    parts = name.split()
    if parts[0] == 'desktop' and len(parts) == 3:
        suite, v = parts[1], parts[2]
        if (ROOT / f'build/debugkit{"-hd" if HD else ""}/{suite}-{v}.done').exists():
            return 'done'
        if suite in ('events', 'wide'):
            cases = ROOT / 'build/eventtest' / (v + ('_wide' if suite == 'wide' else ''))
        elif suite == 'skip':
            cases = ROOT / 'build/frameskip' / v / 'skip'
        elif suite == 'rooms':
            cases = ROOT / f'build/roomcases_{v}'
        else:
            return 'running or not started'
        return f'{len(list(cases.glob("*/result.json")))} cases finished' if cases.is_dir() else 'not started'
    if parts[0] == 'android' and len(parts) > 1:
        tag = ('-'.join(p for p in parts[1:] if p in ('eu', 'us', 'jp')) + ('-riku' if 'riku' in parts else '')
               + ('-khcom_image_move' if 'move' in parts else ''))
        rep = ROOT / f'build/android-scenarios/report-{tag}.json'
        if rep.exists():
            rows = json.loads(rep.read_text())
            return f'{sum(r["status"] == "ok" for r in rows)} ok, {sum(r["status"] != "ok" for r in rows)} not ok'
        return 'not started'
    return ''


def say(text):
    with open(LOG, 'a', encoding='utf-8') as f:
        f.write(time.strftime('%d/%m %H:%M:%S ') + text + '\n')


def running_pid(pid):
    """Whether a process with this id is alive (Windows: tasklist)."""
    r = subprocess.run(['tasklist', '/FI', f'PID eq {pid}', '/NH'], capture_output=True, text=True)
    return str(pid) in r.stdout


def emulator():
    """The Android 14 emulator (Pixel6_API34), started if it is not running, ready when it says boot completed."""
    def booted():
        r = subprocess.run([str(ADB), '-s', 'emulator-5554', 'shell', 'getprop', 'sys.boot_completed'],
                           capture_output=True, text=True)
        return r.stdout.strip() == '1'
    if booted():
        return True
    # the AVD (13 GB) lives outside the user profile: without ANDROID_AVD_HOME the emulator finds none and never boots
    env = dict(os.environ)
    for home in (ROOT.parent / 'toolchain/android/avd', (WORK_DIR / 'android-avd')):
        if 'ANDROID_AVD_HOME' not in env and (home / 'Pixel6_API34.ini').exists():
            env['ANDROID_AVD_HOME'] = str(home)
    subprocess.Popen([str(SDK / 'emulator/emulator.exe'), '-avd', 'Pixel6_API34', '-no-window', '-no-audio',
                      '-no-snapshot', '-gpu', 'swiftshader_indirect', '-memory', '3072'],
                     stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, creationflags=0x08000000, env=env)
    for _ in range(120):
        time.sleep(5)
        if booted():
            time.sleep(20)
            return True
    return False


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('--detach', action='store_true')
    ap.add_argument('--status', action='store_true', help='how far each step got, and the last lines of the log')
    ap.add_argument('--hd', action='store_true', help='the desktop batteries with the HD mod on (build/batteries-hd.log)')
    args = ap.parse_args()
    if args.status:
        for name, _ in STEPS:
            print(f'{name:32s} {progress(name)}')
        if LOG.exists():
            print(''.join(LOG.read_text(encoding='utf-8').splitlines(True)[-5:]), end='')
        return
    if args.detach:
        cmd = f'"{PY}" "{Path(__file__).resolve()}"' + (' --hd' if HD else '')
        ps = (f"$s=New-CimInstance -ClassName Win32_ProcessStartup -ClientOnly -Property @{{ShowWindow=[uint16]0}};"
              f"$r=Invoke-CimMethod -ClassName Win32_Process -MethodName Create -Arguments "
              f"@{{CommandLine='{cmd}';CurrentDirectory='{ROOT}';ProcessStartupInformation=$s}};"
              f"'pid ' + $r.ProcessId + ' result ' + $r.ReturnValue")
        print(subprocess.run(['powershell', '-NoProfile', '-Command', ps], capture_output=True, text=True).stdout.strip())
        return
    # One run at a time: two of them wrote the same suite logs at once (25/10: interleaved, unreadable results)
    lock = LOG.with_suffix('.lock')
    if lock.exists():
        try:
            other = int(lock.read_text().strip())
        except ValueError:
            other = 0
        if other and other != os.getpid() and running_pid(other):
            print(f'batteries already running (pid {other}); --status shows how far they got')
            return
    lock.write_text(str(os.getpid()))
    say('start')
    for name, cmd in STEPS:
        if cmd is None:
            ok = emulator()
            say(f'emulator {"ready" if ok else "did not boot"}')
            if not ok:
                break
            continue
        say(f'{name}: running')
        with open(ROOT / 'build' / ('battery-' + re.sub(r'[^a-z0-9]+', '_', name) + '.log'),
                  'w', encoding='utf-8') as out:  # this run only (appending mixed old runs into the results)
            code = subprocess.run(cmd, cwd=ROOT, stdout=out, stderr=subprocess.STDOUT,
                                  creationflags=0x08000000).returncode
        say(f'{name}: exit {code}')
    # the emulator takes 3 GB and the CPU: closed as soon as the batteries end (the user plays meanwhile)
    subprocess.run([str(ADB), '-s', 'emulator-5554', 'emu', 'kill'], capture_output=True)
    lock.unlink(missing_ok=True)
    say('BATTERIES-DONE')


if __name__ == '__main__':
    main()
