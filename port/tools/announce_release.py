#!/usr/bin/env python3
"""Release announcements (user 25/10): the posts for X, Reddit and Patreon of a big release, written from the
CHANGELOG's section into build/announce/<version>/ (one file per place), ready to paste. Nothing is posted from here:
publishing goes through the user's OK (Codex posts where a browser is needed).

    python port/tools/announce_release.py v2.0              # the CHANGELOG section of v2.0
    python port/tools/announce_release.py v2.0 --spanish    # also the Spanish posts
"""
import argparse
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
REPO = 'https://github.com/MatyMeatBoy/ReKHCoM'
X_LIMIT = 280


def section(version):
    text = (ROOT / 'CHANGELOG.md').read_text(encoding='utf-8')
    m = re.search(r'^## ' + re.escape(version) + r'\b.*?$(.*?)(?=^## |\Z)', text, re.M | re.S)
    if not m:
        raise SystemExit(f'CHANGELOG.md has no "## {version}" section')
    return [l[2:].strip() for l in m.group(1).splitlines() if l.startswith('- ')]


def x_post(version, items, link):
    head = f'ReKHCoM {version} is out: Kingdom Hearts: Chain of Memories, native on PC and more.'
    body, lines = '', []
    for item in items:
        short = re.sub(r'\s*\(#\d+\)', '', item.split('. ')[0]).rstrip('.')
        candidate = '\n'.join(lines + ['· ' + short])
        if len(head) + len(candidate) + len(link) + 4 > X_LIMIT:
            break
        lines.append('· ' + short)
    body = '\n'.join(lines)
    return f'{head}\n{body}\n{link}'.strip()


def long_post(version, items, link, title_word='Changes'):
    out = [f'# ReKHCoM {version}', '',
           'ReKHCoM is a native PC port of Kingdom Hearts: Chain of Memories (GBA), built from the decompilation. '
           'You need your own cartridge dump.', '', f'## {title_word}', '']
    out += [f'- {i}' for i in items]
    out += ['', f'Download: {link}', '']
    return '\n'.join(out)


def main():
    ap = argparse.ArgumentParser(description=__doc__.split('\n')[0])
    ap.add_argument('version')
    ap.add_argument('--spanish', action='store_true')
    args = ap.parse_args()
    items = section(args.version)
    link = f'{REPO}/releases/tag/{args.version}'
    out = ROOT / 'build/announce' / args.version
    out.mkdir(parents=True, exist_ok=True)
    (out / 'x.txt').write_text(x_post(args.version, items, link) + '\n', encoding='utf-8')
    (out / 'reddit.md').write_text(long_post(args.version, items, link), encoding='utf-8')
    (out / 'patreon.md').write_text(long_post(args.version, items, link, 'What is new') +
                                    '\nThank you for supporting the project!\n', encoding='utf-8')
    if args.spanish:
        (out / 'x_es.txt').write_text(f'¡Salió ReKHCoM {args.version}! Kingdom Hearts: Chain of Memories nativo en PC y '
                                      f'más.\n{link}\n', encoding='utf-8')
    for f in sorted(out.iterdir()):
        print(f, len(f.read_text(encoding='utf-8')), 'chars')


if __name__ == '__main__':
    main()
