"""Compile the production host argument consumer; no SDL, guest, or VM required.

python port/tools/test_linux64_language.py --cc /path/to/cc
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
CC = 'cc'


class Linux64LanguageTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory(prefix='linux64 args ')
        cls.addClassCleanup(cls.temp.cleanup)
        root = Path(cls.temp.name)
        source = (ROOT / 'port/linux64/linux64_host.c').read_text()
        # Compile the actual startup consumer, ending before SDL initialization.
        start = source.index('    const char* bootStatus =')
        end = source.index('    {', source.index('    setenv("KHCOM_DATA"', start))
        consumer = source[start:end]
        parser = ROOT / 'port/linux64/linux64_args.h'
        include = '#include "linux64_args.h"\n' if parser.exists() else ''
        guest_source = (ROOT / 'port/ps5/guest/src/platform_ps5.c').read_text()
        guest_consumer = guest_source[guest_source.index('int guest_main(void) {'):]
        probe = root / 'consumer.c'
        probe.write_text('''#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
static int setenv(const char* name, const char* value, int overwrite) {
    if (!overwrite && getenv(name)) return 0;
    return _putenv_s(name, value);
}
#endif
''' + include + '''
static const char* gPortRomPath;
static void WriteDefaultSettings(void) {}
static int h_getenv(const char* name, char* out, size_t capacity) {
    const char* value = getenv(name);
    if (!value) return 0;
    snprintf(out, capacity, "%s", value);
    return (int)strlen(value);
}
static int GameMain(int argc, char** argv) {
    printf("guest_language=%s\\n", argc == 3 ? argv[2] : "");
    printf("guest_rom=%s\\n", gPortRomPath ? gPortRomPath : "");
    return 0;
}
#define main GameMain
''' + guest_consumer + '''
#undef main
static char sData[1024] = ".";
int main(int argc, char** argv) {
''' + consumer + '''
    const char* language = getenv("KHCOM_LANGUAGE");
    printf("data=%s\\nboot=%s\\nlanguage=%s\\nrom=%s\\n", sData,
           bootStatus ? bootStatus : "", language ? language : "",
           getenv("KHCOM_ROM") ? getenv("KHCOM_ROM") : "");
    return guest_main();
}
''')
        cls.exe = root / ('consumer.exe' if os.name == 'nt' else 'consumer')
        subprocess.run([CC, '-std=c11', '-Wall', '-Wextra', '-Werror',
                        '-I', str(parser.parent), str(probe), '-o', str(cls.exe)], check=True)

    def run_consumer(self, *args, language='inherited'):
        env = dict(os.environ, KHCOM_LANGUAGE=language, KHCOM_ROM='/selected/eu.gba')
        result = subprocess.run([str(self.exe), 'guest-eu.bin', *args], env=env,
                                text=True, capture_output=True, check=True)
        return dict(line.split('=', 1) for line in result.stdout.splitlines())

    def test_language_last_wins_and_host_paths_survive(self):
        actual = self.run_consumer('--data', '/my data', '--boot-status', '/boot marker',
                                   '--', '--data-dir', '/ignored data', '--language', 'en',
                                   '--language', 'es')
        self.assertEqual(actual, dict(data='/my data', boot='/boot marker',
                                      language='es', rom='/selected/eu.gba', guest_language='es',
                                      guest_rom='/selected/eu.gba'))

    def test_game_arguments_cannot_override_host_or_rom(self):
        actual = self.run_consumer('--data', '/real', '--boot-status', '/real marker',
                                   '--', '--data', '/other', '--boot-status', '/other marker',
                                   '--rom', '/another/us.gba', '--language', 'fr')
        self.assertEqual(actual, dict(data='/real', boot='/real marker',
                                      language='fr', rom='/selected/eu.gba', guest_language='fr',
                                      guest_rom='/selected/eu.gba'))

    def test_missing_language_and_unknown_arguments_preserve_environment(self):
        for args in ((), ('--', '--language'), ('--', '--unknown', 'es')):
            with self.subTest(args=args):
                self.assertEqual(self.run_consumer(*args)['language'], 'inherited')


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('--cc', default='cc')
    args = ap.parse_args()
    CC = args.cc
    unittest.main(argv=[__file__], verbosity=2)
