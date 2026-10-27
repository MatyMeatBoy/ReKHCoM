"""Exercise the shipped Linux wrapper with fake game executables, without a ROM.

    python port/tools/test_linux_launcher.py --shell /bin/sh

On Windows, --shell may name Git's sh.exe. On Linux, GLib's gio additionally
launches the installed desktop entry with its real Exec parser when available.
No Steam configuration or existing desktop entries are touched.
"""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SHELL = '/bin/sh'


class LauncherTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='rekhcom launcher ')
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        # These characters are legal on both hosts and exercise field-code and
        # shell expansion errors that a plain /home/deck/Games path would hide.
        self.game = self.root / "Game's $dollar `tick` %f space"
        if os.name != 'nt':
            self.game = self.game / 'quote" and back\\slash'
        self.game.mkdir(parents=True)
        for name in ('bin', 'lib', 'roms'):
            (self.game / name).mkdir()
        self.script = self.game / 'ReKHCoM.sh'
        self.script.write_bytes((ROOT / 'port/linux/ReKHCoM.sh').read_bytes().replace(b'\r\n', b'\n'))
        self.script.chmod(0o644)  # /bin/sh must not depend on executable bits.
        self.xdg = self.root / 'app data'
        self.env = dict(os.environ, XDG_DATA_HOME=self.posix(self.root) + '/app data', LD_LIBRARY_PATH='/inherited/libs')
        self.directory = self.posix(self.game)
        for region in ('eu', 'us', 'jp'):
            stub = self.game / 'bin' / ('khcom_' + region)
            stub.write_text('#!/bin/sh\nprintf "%s\\n" "$PWD" "$LD_LIBRARY_PATH" "$@" > args.txt\n'
                            'printf "%s\\n" "' + region + '" > region.txt\nexit 17\n', encoding='utf-8')
            stub.chmod(0o755)

    def posix(self, path):
        return subprocess.check_output([SHELL, '-c', 'cd "$1" && pwd -P', 'sh', str(path)],
                                       text=True).strip()

    def run_script(self, *args):
        return subprocess.run([SHELL, str(self.script), *args], cwd=self.root, env=self.env,
                              text=True, capture_output=True)

    def assert_game(self, region, args):
        captured = (self.game / 'args.txt').read_text(encoding='utf-8').splitlines()
        self.assertEqual(captured, [self.directory, self.directory + '/lib/sys:' + self.directory + '/lib:/inherited/libs',
                                   '--data-dir', self.directory, *args])
        self.assertEqual((self.game / 'region.txt').read_text().strip(), region)

    def fake64(self, code, entered=False):
        folder = self.game / 'bin64'
        folder.mkdir(exist_ok=True)
        (folder / 'guest-eu.bin').write_bytes(b'guest')
        host = folder / 'rekhcom64'
        host.write_text('#!/bin/sh\nprintf "64\\n" > ran64.txt\n' +
                        ('while [ "$#" -gt 0 ]; do if [ "$1" = --boot-status ]; then shift; printf ready > "$1"; fi; shift; done\n' if entered else '') +
                        f'exit {code}\n')
        host.chmod(0o755)

    def test_automatic_boot_failure_retries_32(self):
        self.fake64(4)
        self.assertEqual(self.run_script().returncode, 17)
        self.assertTrue((self.game / 'ran64.txt').exists())
        self.assertTrue((self.game / 'region.txt').exists())

    def test_guest_failure_and_quit_never_retry(self):
        for code, entered in ((4, True), (81, False), (0, False), (139, False)):
            self.fake64(code, entered)
            self.assertEqual(self.run_script().returncode, code)
            self.assertFalse((self.game / 'region.txt').exists())

    def test_forced_modes_and_persistent_preference(self):
        self.fake64(4)
        (self.game / 'decompui.ini').write_text('runtime_mode=1\n')
        self.assertEqual(self.run_script().returncode, 17)
        self.assertFalse((self.game / 'ran64.txt').exists())
        (self.game / 'region.txt').unlink()
        self.assertEqual(self.run_script('--force64').returncode, 4)
        self.assertFalse((self.game / 'region.txt').exists())
        (self.game / 'bin64/rekhcom64').unlink()
        self.assertEqual(self.run_script('--force64').returncode, 2)
        self.assertFalse((self.game / 'region.txt').exists())

    def test_no_rom_and_foreign_cwd(self):
        result = self.run_script('--language', 'es', 'argument with spaces', '$literal')
        self.assertEqual(result.returncode, 17)
        self.assert_game('eu', ['--language', 'es', 'argument with spaces', '$literal'])
        self.assertIn('ROM not found', (self.game / 'ReKHCoM.log').read_text())
        self.assertFalse((self.root / 'ReKHCoM.log').exists())

    def test_previous_five_logs_survive_restarts(self):
        for n in range(8):
            self.assertEqual(self.run_script().returncode, 17)
            with (self.game / 'ReKHCoM.log').open('a') as log:
                log.write(f'run-marker-{n}\n')
        for previous in range(1, 6):
            path = self.game / f'ReKHCoM.log.{previous}'
            self.assertTrue(path.exists(), f'previous log missing: {path.name}')
            self.assertIn(f'run-marker-{7-previous}\n', path.read_text())
        self.assertFalse((self.game / 'ReKHCoM.log.6').exists())

    def test_rotation_directory_keeps_existing_evidence(self):
        log = self.game / 'ReKHCoM.log'
        log.write_text('previous-crash-evidence\n')
        blocked = self.game / 'ReKHCoM.log.1'
        blocked.mkdir()
        (blocked/'keep').write_text('user data')
        self.assertEqual(self.run_script().returncode, 17)
        self.assertTrue(log.read_text().startswith('previous-crash-evidence\n'))
        self.assertIn('ReKHCoM.sh:', log.read_text())
        self.assertEqual(list(p.name for p in blocked.iterdir()), ['keep'])

    def test_header_selection_and_region_option(self):
        # Header-only fixtures are not cartridge dumps; they test dispatch only.
        for region, code in [('eu', b'B8CP'), ('us', b'B8CE'), ('jp', b'B8CJ')]:
            rom = self.game / 'roms' / (region + ' dummy header.gba')
            rom.write_bytes(bytes(172) + code)
        for region in ('eu', 'us', 'jp'):
            result = self.run_script('--region', region, '--language', 'es')
            self.assertEqual(result.returncode, 17)
            self.assert_game(region, ['--rom', self.directory + '/roms/' + region + ' dummy header.gba',
                                      '--language', 'es'])
        self.assertEqual(self.run_script().returncode, 17)
        self.assert_game('eu', ['--rom', self.directory + '/roms/eu dummy header.gba'])
        for bad in [('--region',), ('--region', '../../other')]:
            self.assertEqual(self.run_script(*bad).returncode, 2)

    def test_desktop_install_is_rom_independent_and_relocatable(self):
        result = self.run_script('--install-desktop')
        self.assertEqual(result.returncode, 0, result.stderr)
        entry = self.xdg / 'applications/rekhcom.desktop'
        text = entry.read_text(encoding='utf-8')
        self.assertIn('Exec=/bin/sh "', text)
        self.assertIn('%%f', text)
        self.assertIn('Path=', text)
        self.assertIn('Icon=', text)
        self.assertFalse((self.game / 'args.txt').exists())
        self.assertFalse((self.game / 'ReKHCoM.log').exists())
        if os.name != 'nt' and shutil.which('gio'):
            # Real GLib parsing, including spaces, quotes, percent and backslash.
            if shutil.which('desktop-file-validate'):
                subprocess.run(['desktop-file-validate', str(entry)], check=True)
            launched = subprocess.run(['gio', 'launch', str(entry)], cwd=self.root, env=self.env,
                                      capture_output=True, text=True)
            self.assertEqual(launched.returncode, 0, launched.stderr)
            import time
            for _ in range(100):
                if (self.game / 'region.txt').exists():
                    break
                time.sleep(0.02)
            self.assert_game('eu', [])
            print('GLib desktop entry parser and launch: passed', flush=True)
        moved = self.game.with_name(self.game.name + ' moved')
        self.game.rename(moved)
        self.game = moved
        self.script = moved / 'ReKHCoM.sh'
        self.assertEqual(self.run_script('--install-desktop').returncode, 0)
        self.assertIn('moved', entry.read_text())
        self.assertEqual(len(list(entry.parent.glob('*.desktop'))), 1)

    def test_setup_registers_without_starting_game_or_touching_steam(self):
        helper = self.game / 'Setup Steam Deck.sh'
        helper.write_bytes((ROOT / 'port/linux/Setup Steam Deck.sh').read_bytes().replace(b'\r\n', b'\n'))
        # No dialog dependency: silent mode prints complete instructions.
        result = subprocess.run([SHELL, str(helper), '--no-dialog'], cwd=self.root,
                                env=self.env, text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertTrue((self.xdg / 'applications/rekhcom.desktop').exists())
        self.assertIn('Non-Steam', result.stdout)
        self.assertIn('Start + Select', result.stdout)
        self.assertIn('Gaming Mode', result.stdout)
        self.assertFalse((self.game / 'args.txt').exists())
        self.assertFalse((self.game / 'ReKHCoM.log').exists())
        self.assertFalse((self.root / '.steam').exists())
        bad = subprocess.run([SHELL, str(helper), '--bad'], cwd=self.root,
                             env=self.env, text=True, capture_output=True)
        self.assertEqual(bad.returncode, 2)

    def test_manual_steam_command(self):
        result = self.run_script('--steam-help')
        self.assertEqual(result.returncode, 0)
        self.assertIn('Target: /bin/sh', result.stdout)
        options = next(line.split(': ', 1)[1] for line in result.stdout.splitlines()
                       if line.startswith('Launch Options: '))
        # Steam itself is not under test: verify the printed shell command
        # preserves the literal script path, then arguments after that path.
        command = '/bin/sh ' + options + ' --language "es space"'
        launched = subprocess.run([SHELL, '-c', command], cwd=self.root, env=self.env,
                                  capture_output=True, text=True)
        self.assertEqual(launched.returncode, 17, launched.stderr)
        self.assert_game('eu', ['--language', 'es space'])


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--shell', default=SHELL)
    args = parser.parse_args()
    SHELL = args.shell
    unittest.main(argv=[__file__], verbosity=2)
