#!/usr/bin/env python3
"""Source checks that take seconds and catch what otherwise only fails in a console or Android build.

- The console guests (PS5, PS4, Switch, Vita, Android 64) build port/src with a small SDL shim
  (port/ps5/guest/include/SDL3/SDL.h): every SDLK_/SDL_EVENT_/SDL_GAMEPAD_/SDL_SCANCODE_ constant the port uses must be
  defined there, with SDL's own value (21/10: SDLK_AC_BACK and SDL_EVENT_MOUSE_BUTTON_DOWN broke the Android 64 build).
- Java string and char literals must close on their line (21/10: a script wrote real newlines into GameActivity's
  share-log strings and the APK did not build).
- The generated HD sprite tables (gen_hd_sprites_inc.py) match port/data/hd_sprites.tsv, and its rows pass
  check_hd_sprites.py.
- every port/tools/*.py parses (22/10: a newline written as a real line break broke the Android harness and was
  pushed; KNOWN_BROKEN lists the ones waiting on their owner).
- port/src compiles (syntax only) for the console guests, against the shim, as build_switch.py compiles it: a
  desktop-only SDL call outside its guard otherwise shows only in a console build (22/10: the sprite export).

Exit code 1 and one line per problem. release_check.py and build_android.py run it first.
"""
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SHIM = ROOT / 'port/ps5/guest/include/SDL3/SDL.h'
SDL = ROOT.parent / 'toolchain/android/SDL3-3.4.18/include/SDL3'
CONST = re.compile(r'\b(SDLK_\w+|SDL_EVENT_\w+|SDL_GAMEPAD_\w+|SDL_SCANCODE_\w+)\b')


def guest_sources():
    return sorted((ROOT / 'port/src').glob('*.c')) + [ROOT / 'port/android/native/touch_android.c']


def shim_problems():
    shim = SHIM.read_text(encoding='utf-8')
    defined = set(CONST.findall(shim))
    out = []
    for f in guest_sources():
        for name in sorted(set(CONST.findall(f.read_text(encoding='utf-8', errors='replace'))) - defined):
            out.append(f'{f.relative_to(ROOT)}: {name} is not in the console SDL shim ({SHIM.relative_to(ROOT)})')
    # the shim's keycodes must be SDL's own: the hosts pass SDL's values through
    real = {}
    if SDL.is_dir():
        for h in SDL.glob('*.h'):
            for name, value in re.findall(r'#define\s+(SDLK_\w+)\s+(0x[0-9A-Fa-f]+)u?', h.read_text(encoding='utf-8')):
                real[name] = int(value, 16)
    for name, value in re.findall(r'#define\s+(SDLK_\w+)\s+(0x[0-9A-Fa-f]+)u?', shim):
        if name in real and real[name] != int(value, 16):
            out.append(f'{SHIM.relative_to(ROOT)}: {name} is {value}, SDL has {real[name]:#x}')
    return out


def generated_problems():
    import subprocess
    r = subprocess.run([sys.executable, str(ROOT / 'port/tools/gen_hd_sprites_inc.py'), '--check'],
                       capture_output=True, text=True)
    return [l for l in r.stdout.splitlines() if l.strip()] if r.returncode else []


def hd_sprite_problems():
    """The HD sprite rows: unique names, symbols in every version, parts inside the box (check_hd_sprites.py)."""
    import subprocess
    r = subprocess.run([sys.executable, str(ROOT / 'port/tools/check_hd_sprites.py')], capture_output=True, text=True)
    return [l for l in r.stdout.splitlines() if l.strip() and not l.startswith('check_hd_sprites:')] if r.returncode else []


# Codex's scene review script: lines scrambled by a merge (18/10), used by nothing; PENDIENTES_VIGENTES.md asks Codex
KNOWN_BROKEN = set()


def tool_problems():
    """Every port/tools/*.py must at least parse."""
    import ast
    out = []
    for f in sorted((ROOT / 'port/tools').glob('*.py')):
        if f.name in KNOWN_BROKEN:
            continue
        try:
            ast.parse(f.read_text(encoding='utf-8'), str(f))
        except SyntaxError as e:
            out.append(f'{f.relative_to(ROOT)}:{e.lineno}: does not parse ({e.msg})')
    return out


def jp_glyph_problems():
    import subprocess
    r = subprocess.run([sys.executable, '-X', 'utf8', str(ROOT / 'port/tools/check_jp_glyphs.py')],
                       capture_output=True, text=True, encoding='utf-8')
    return [l for l in r.stdout.splitlines() if l.strip()] if r.returncode else []


def guest_compile_problems():
    import subprocess
    cc = ROOT.parent / 'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/clang.exe'
    if not cc.is_file():
        return []
    sys.path.insert(0, str(ROOT / 'port/ps5'))
    sys.path.insert(0, str(ROOT / 'port/switch'))
    import build_ps5
    import build_switch
    flags = [str(x) for x in build_switch.common_flags() if not str(x).startswith('-mcpu')]
    flags += ['-w', '-std=gnu11', '-DSTBI_NO_THREAD_LOCALS', f'-I{ROOT / "port/ps5/third_party/stb"}', f'-I{ROOT / "port/src"}',
              f'-I{ROOT / "port/android/native"}', '-DPLATFORM_PC', '-DVERSION_EU', '-DPORT_TOUCH']
    flags += [str(i) for i in build_ps5.game_includes('eu')]
    out = []
    for f in sorted((ROOT / 'port/src').glob('*.c')):
        if f.stem in build_ps5.PORT_EXCLUDED or f.name in build_ps5.PORT_PATCHES:
            continue
        r = subprocess.run([str(cc), '-fsyntax-only', *flags, str(f)], capture_output=True, text=True)
        out += [f'{f.relative_to(ROOT)} (console guest): {l}' for l in r.stderr.splitlines() if ': error:' in l][:3]
    return out


def java_problems():
    out = []
    for f in sorted((ROOT / 'port/android/app/src').rglob('*.java')):
        block = False
        for n, line in enumerate(f.read_text(encoding='utf-8').splitlines(), 1):
            i, quote = 0, None
            while i < len(line):
                c = line[i]
                if block:
                    if line.startswith('*/', i):
                        block = False
                        i += 1
                elif quote:
                    if c == '\\':
                        i += 1
                    elif c == quote:
                        quote = None
                elif line.startswith('//', i):
                    break
                elif line.startswith('/*', i):
                    block = True
                    i += 1
                elif c in '"\'':
                    quote = c
                i += 1
            if quote:
                out.append(f'{f.relative_to(ROOT)}:{n}: string or char literal not closed on its line')
    return out


def main():
    problems = (shim_problems() + java_problems() + generated_problems() + hd_sprite_problems() + tool_problems() +
                jp_glyph_problems() +
                guest_compile_problems())
    for p in problems:
        print(p)
    if problems:
        print(f'check_sources: {len(problems)} problem(s)')
        sys.exit(1)
    print('check_sources: ok')


if __name__ == '__main__':
    main()
