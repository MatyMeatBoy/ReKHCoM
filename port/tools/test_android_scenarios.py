"""Many harness scenarios on one Android emulator/device, using the separate --smoke app.

The smoke APK (build_android.py --abi x86 --smoke --smoke-env KHCOM_FAST=1) reads
files/smoke_env.txt (KEY=VALUE lines) before the game starts, so a single install runs
menus, fixtures, bosses and the playthrough bot. A scenario passes when the game process
exits by itself (KHCOM_QUIT) with no native crash and no CRASH/FATAL line in kh_com.log.
Platforms without fault recovery (Android, web, Linux ARM) die on a stray NULL access that
the desktop ports silently recover, so this is the check for those.

    python port/tools/test_android_scenarios.py --serial emulator-5554 --regions us \
        --groups menus fixtures bosses issues bot [--only name,name]
"""
import argparse, json, re, subprocess, sys, time
from pathlib import Path

R = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(R / 'port/tools'))
import test_battles

ADB = str(R.parent / 'toolchain/android/sdk/platform-tools/adb.exe')
PKG = 'com.rekhcom.android.smoke'
CODES = {'eu': 'B8CP', 'us': 'B8CE', 'jp': 'B8CJ'}
OUT = R / 'build/android-scenarios'
UBSAN = set()

# Mode_Premire is left out: the level-up screen opened with no battle before it reads gBtlWork NULL (a harness
# artifact, debug_kit.py lists it as expected)
MENU_MODES = ['mode_status', 'mode_worldselect', 'Mode_Deck', 'mode_jiminy', 'mode_ms_top',
              'mode_ms_shop', 'mode_ms_charge', 'mode_allmap', 'mode_worldinspect', 'mode_worldwarp', 'mode_pooh',
              'mode_StaffRoll', 'Continue', 'mode_wLogo', 'mode_mapinspect', 'mode_eventselect', 'mode_title',
              'WORLDSELECT', 'Mode_riku_deckTutorial']
# mode_vsbattle is left out too: the link battle opened with no link partner reads gLinkPartnerDeck NULL (the game
# only enters it after the cable exchange; debug_kit.py HARNESS_ONLY)

FIXTURES = {
    'room-castle': 'KHCOM_MODE=60:room:10901,120:fieldfreeze',
    # doorselect is left out: it opens the door choice in a room without that door, a harness artifact
    # (debug_kit.py HARNESS_ONLY); the desktop ports recover from it, Android does not
    'doortutorial': 'KHCOM_MODE=60:room:10901,200:doortutorial',
    'castle': 'KHCOM_MODE=60:castle',
    'obtain': 'KHCOM_MODE=60:room:10901,200:obtain',
    'deckuse': 'KHCOM_MODE=60:deckuse:592',  # deck 1, 20 copies of card 1 (DECK|COUNT<<2|CARD<<9)
    'jiminy-fixture': 'KHCOM_MODE=60:jiminy',
    'mapsave': 'KHCOM_MODE=60:room:10901,200:mapsave',
    'moogle': 'KHCOM_MODE=60:room:10901,200:moogle',
    'rikuopening': 'KHCOM_MODE=60:rikuopening',
    'roomriku': 'KHCOM_MODE=60:roomriku:10901',
    'levelup': 'KHCOM_BATTLE=0:1:0;KHCOM_MODE=400:levelup',
    'battlewin': 'KHCOM_BATTLE=0:1:0;KHCOM_MODE=400:battlewin',
    'bosskill': 'KHCOM_BATTLE=10:1:0;KHCOM_MODE=400:bosskill',
    'playerkill': 'KHCOM_BATTLE=0:1:0;KHCOM_MODE=400:playerkill',
}


# what players reported, kept as cases so they stay fixed; run them also with --env KHCOM_IMAGE_MOVE=1 (#22, #34:
# phones where 0x01000000 is taken load the 64-bit image elsewhere)
ISSUES = {
    'issue33-trickmaster-scene': 'KHCOM_EVENT=0:98',  # EVENT_099_WONDERLAND_BOSS, into the battle
    'issue-100acre-lv1': 'KHCOM_EVENT=0:133',  # EVENT_136: the BUS_PTR sites the desktop battery found
}


def adb(*args, check=True):
    # adb.exe itself can fail to start (0xC0000142 / 0xC000026B: Windows refusing new processes for a moment):
    # that is not the game's result, so it is retried instead of ending the battery
    for attempt in range(10):
        r = subprocess.run([ADB, '-s', SERIAL, *map(str, args)], capture_output=True)
        if r.returncode not in (0xC0000142, 0xC000026B, -1073741502, -1073741205):
            break
        time.sleep(5 + attempt * 5)
    if check and r.returncode:
        raise subprocess.CalledProcessError(r.returncode, r.args, r.stdout, r.stderr)
    return r


def private(*args):
    return adb('shell', 'run-as', PKG, *args, check=False)


def battle_keys(frames):
    keys = ['60-63:A']
    dirs = ('RIGHT', 'LEFT', 'UP', 'DOWN')
    for n, f in enumerate(range(300, frames, 24)):
        keys.append(f'{f}-{f + 3}:A')
        if n % 4 == 0:
            keys.append(f'{f + 6}-{f + 20}:{dirs[(n // 4) % 4]}')
    return ','.join(keys)


def scenarios(groups, only, riku, from_battle=0):
    out = []
    if 'menus' in groups:
        for m in MENU_MODES:
            out.append((f'menu-{m}', 400, 90, f'KHCOM_MODE=60:{m}\nKHCOM_KEYS=200-203:A,260-263:DOWN,300-303:A,340-343:B'))
    if 'fixtures' in groups:
        for name, env in FIXTURES.items():
            out.append((f'fx-{name}', 900, 120, env.replace(';', '\n') + '\nKHCOM_KEYS=300-303:A,360-363:A,420-423:A'))
    if 'bosses' in groups or 'battles' in groups:
        stages = test_battles.stage_table()
        entries = test_battles.battle_entries()
        if 'battles' not in groups:
            # the debug menu lists each world's battles in order; the last two of a world hold its bosses
            last = {}
            for i, world, battle in entries:
                last.setdefault(re.sub(r'_[0-9]+$', '', battle), []).append((i, world, battle))
            entries = [e for group in last.values() for e in group[-2:]]
        for i, world, battle in entries:
            if i < from_battle:
                continue
            stage = next((k for k, s in enumerate(stages) if s.replace('BATTLE_STAGE_', 'WORLD_') == world), 0)
            out.append((f'battle-{i:03d}-{battle}', 1800, 150,
                        f'KHCOM_BATTLE={i}:{stage}:{int(riku)}\nKHCOM_KEYS={battle_keys(1800)}'))
    if 'issues' in groups:
        keys = ','.join(f'{f}-{f + 3}:A' for f in range(30, 2400, 40))
        for name, env in ISSUES.items():
            out.append((name, 2400, 200, f'{env}\nKHCOM_KEYS={keys}'))
    if 'story' in groups:
        # every story event that starts a battle and goes on into another event once won (test_event_battles.py;
        # issue #46: Android closed right after Agrabah's first event battle)
        import test_event_battles as teb
        for event, battle, after in teb.chains():
            e = teb.env_for(event)
            out.append((f'story-{event:03d}-{battle}', teb.FRAMES, 900,
                        f'KHCOM_MODE={e["KHCOM_MODE"]}\nKHCOM_KEYS={e["KHCOM_KEYS"]}'))
    if 'bot' in groups:
        out.append(('bot-playthrough', 60000, 1500, 'KHCOM_BOT=1\nKHCOM_SHOTS=5000,15000,30000,45000,60000'))
    if only:
        out = [s for s in out if any(o in s[0] for o in only)]
    return out


def run(region, name, frames, timeout, env):
    private('rm', '-f', 'files/kh_com.log', 'files/kh_com_host.log', 'files/smoke_env.txt')
    text = f'KHCOM_FAST=1\nKHCOM_TRACE_MODES=1\nKHCOM_QUIT={frames}\n{env}\n'
    tmp = OUT / 'env.txt'
    tmp.write_text(text)
    adb('push', tmp, '/data/local/tmp/smoke_env.txt')
    private('cp', '/data/local/tmp/smoke_env.txt', 'files/smoke_env.txt')
    adb('shell', 'am', 'force-stop', PKG)
    adb('logcat', '-c')
    code = CODES[region]
    adb('shell', 'am', 'start', '--user', '0', '-n', PKG + '/com.rekhcom.android.GameActivity',
        '--es', 'region', region, '--es', 'rom', f'/data/user/0/{PKG}/files/roms/{code}.gba')
    start = time.monotonic()
    time.sleep(6)
    status = 'timeout'
    seen = False  # a process that has not started yet is not one that finished (the first start after an install is slow)
    while time.monotonic() - start < timeout:
        crashes = adb('logcat', '-d', '-b', 'crash').stdout.decode(errors='replace')
        if PKG in crashes and ('Fatal signal' in crashes or 'FATAL EXCEPTION' in crashes):
            status = 'CRASH'
            break
        host = private('cat', 'files/kh_com_host.log').stdout.decode(errors='replace')
        if 'crash: signal' in host:  # the 64-bit host's own crash report (android64_host.c, CrashSignal)
            status = 'CRASH'
            break
        alive = not adb('shell', 'pidof', PKG + ':game', check=False).returncode
        if alive:
            seen = True
        elif seen:
            status = 'ok'
            break
        # the 64-bit host returns from the game and Android may keep the process: its log says so
        if 'the game ended' in host:
            status = 'ok'
            break
        time.sleep(3)
    log = private('cat', 'files/kh_com.log').stdout.decode(errors='replace')
    detail = ''
    ubsan = sorted(set(re.findall(r'ReKHCoM-UBSAN.*?: (\w[\w-]* pc=0x[0-9a-f]+)', adb('logcat', '-d', '-s', 'ReKHCoM-UBSAN:W').stdout.decode(errors='replace'))))
    if ubsan:
        print('  ubsan:', ' '.join(ubsan), flush=True)
        UBSAN.update(ubsan)
    if status == 'CRASH':
        crashes = adb('logcat', '-d', '-b', 'crash').stdout.decode(errors='replace')
        detail = '\n'.join(l.split(': ', 1)[-1] for l in crashes.splitlines() if re.search(r'Fatal signal|#0[0-6] pc', l))
        host = private('cat', 'files/kh_com_host.log').stdout.decode(errors='replace')
        detail += ''.join('\n' + l for l in host.splitlines() if l.startswith(('crash:', '  #')))
    if 'KHCOM_BATTLE' not in env and 'mode mode_chkbtl' in log:
        # the APK's own debug battle ran: built without --smoke-env KHCOM_FAST=1, every case would test the wrong thing
        status, detail = 'BAD-APK', 'rebuild with: build_android.py --smoke --smoke-env KHCOM_FAST=1'
    elif status == 'ok' and re.search(r'CRASH|FATAL', log):
        status = 'LOGGED-CRASH'
    elif status == 'ok' and 'harness: no mode' in private('cat', 'files/kh_com.log').stdout.decode(errors='replace'):
        status = 'BAD-SCENARIO'
    if status != 'ok':
        keep_evidence(region, name, status, log)
    if status == 'timeout':
        adb('shell', 'am', 'force-stop', PKG)
    return status, detail, log[-300:]


def keep_evidence(region, name, status, log):
    """A case that is not ok is a bug until it is explained (DEBUG_WORKFLOW.md): its game log, host log, logcat and the
    screen as it was, in build/android-scenarios/cases/<region>-<name>/ (22/10: US 124 timed out with nothing kept, and
    became issue #46)."""
    d = OUT / 'cases' / re.sub(r'[^\w.-]+', '_', f'{region}-{name}')
    d.mkdir(parents=True, exist_ok=True)
    (d / 'status.txt').write_text(status + chr(10))
    (d / 'kh_com.log').write_text(log, encoding='utf-8', errors='replace')
    (d / 'kh_com_host.log').write_bytes(private('cat', 'files/kh_com_host.log').stdout)
    (d / 'logcat.txt').write_bytes(adb('logcat', '-d').stdout)
    shot = adb('exec-out', 'screencap', '-p', check=False).stdout
    if shot:
        (d / 'screen.png').write_bytes(shot)
    if status == 'timeout':  # where the game thread is: its native stack
        pid = adb('shell', 'pidof', PKG + ':game', check=False).stdout.decode().strip()
        if pid:
            (d / 'stack.txt').write_bytes(adb('shell', 'run-as', PKG, 'cat', f'/proc/{pid}/stack', check=False).stdout +
                                          adb('shell', 'debuggerd', '-b', pid, check=False).stdout)
    print(f'  evidence: {d}', flush=True)


def main():
    global SERIAL
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    p.add_argument('--serial', required=True)
    p.add_argument('--regions', nargs='+', choices=CODES, default=['us'])
    p.add_argument('--groups', nargs='+', default=['menus', 'fixtures', 'bosses'],
                   choices=['menus', 'fixtures', 'bosses', 'battles', 'issues', 'story', 'bot'])
    p.add_argument('--only', default='')
    p.add_argument('--riku', action='store_true')
    p.add_argument('--from-battle', type=int, default=0, help='skip battle entries below this index')
    p.add_argument('--resume', action='store_true', help="skip what the report file already has as ok")
    p.add_argument('--slow', type=float, default=1.0,
                   help='timeout multiplier: an x86_64 emulator runs the arm64 build translated, several times slower')
    p.add_argument('--env', nargs='*', default=[],
                   help='extra KEY=VALUE for every scenario (KHCOM_IMAGE_MOVE=1: the image loaded away from its linked address)')
    args = p.parse_args()
    SERIAL = args.serial
    OUT.mkdir(parents=True, exist_ok=True)
    report = []
    tag = '-'.join(args.regions) + ('-riku' if args.riku else '') + ''.join('-' + e.split('=')[0].lower() for e in args.env)
    done = set()
    if args.resume and (OUT / f'report-{tag}.json').exists():
        report = [r for r in json.loads((OUT / f'report-{tag}.json').read_text()) if r['status'] == 'ok']
        done = {(r['region'], r['name']) for r in report}
    for region in args.regions:
        code = CODES[region]
        private('mkdir', '-p', 'files/roms')
        adb('push', R / 'roms' / f'{code}.gba', f'/data/local/tmp/{code}.gba')
        private('cp', f'/data/local/tmp/{code}.gba', f'files/roms/{code}.gba')
        for name, frames, timeout, env in scenarios(args.groups, [o for o in args.only.split(',') if o], args.riku, args.from_battle):
            if (region, name) in done:
                continue
            status, detail, tail = run(region, name, frames, int(timeout * args.slow), '\n'.join([env, *args.env]))
            print(f'{region} {name}: {status}', flush=True)
            if status != 'ok':
                print(detail or tail, flush=True)
            report.append(dict(region=region, name=name, status=status, detail=detail))
            (OUT / f'report-{tag}.json').write_text(json.dumps(report, indent=1))
    (OUT / 'ubsan-sites.txt').write_text(chr(10).join(sorted(UBSAN)))
    bad = [r for r in report if r['status'] != 'ok']
    print(f'{len(report) - len(bad)}/{len(report)} scenarios ok')
    sys.exit(1 if bad else 0)


if __name__ == '__main__':
    main()
