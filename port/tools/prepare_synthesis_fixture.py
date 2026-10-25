"""Create a local Ninja override enabling only the controlled synthesis fixture.
Uses the existing own build; only card_worldselect.c gets the test define.
The normal configure_pc build never enables it. Does not compile or run.
"""
import argparse
from pathlib import Path
import re

if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ninja', type=Path, default=Path('build/pc-atlas/build.ninja'))
    args = parser.parse_args()
    original = args.ninja.read_text(encoding='utf-8')
    flat = re.sub(r'\$\n\s*', '', original)
    flags = re.search(r'^game_cflags = (.+)$', flat, re.M).group(1)
    pattern = r'(^build [^\n]+: cc_game src/card/card_worldselect.c\n)'
    fixture, count = re.subn(pattern, lambda m: m.group(1) +
                            '  game_cflags = ' + flags + ' -DPORT_SYNTHESIS_TEST_FIXTURE\n',
                            flat, flags=re.M)
    assert count == 1
    target = args.ninja.with_name('synthesis-fixture.ninja')
    target.write_text(fixture, encoding='utf-8')
    print(target)
