"""Redistributable runtime assets shared by clean packages and Windows resources."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def core_assets():
    files = {
        'assets/boxart.jpg': ROOT / 'port/decompui/assets/boxart.jpg',
        'assets/logo.png': ROOT / 'port/decompui/assets/logo.png',
        'assets/steam_header.jpg': ROOT / 'port/decompui/assets/steam_header.jpg',  # Add to Steam: the wide capsule
        'assets/steam_hero.jpg': ROOT / 'port/decompui/assets/steam_hero.jpg',  # Steam library background
        'assets/achievements/badges.png': ROOT / 'port/assets/achievements/badges.png',
        'assets/hd/title/art/sora.png': ROOT / 'port/assets/hd/title/art/sora.png',  # the HD title's illustration (mod)
    }
    for word in sorted((ROOT / 'port/assets/hd/title/words').glob('*.png')):  # the HD title's words (make_title_words.py)
        files['assets/hd/title/words/' + word.name] = word
    for name in ('enemy-hp-frame-v2.png', 'label-hp-v2.png', 'label-vt-v2.png',
                 'label-lp-v1.png', 'label-pv-v3.png'):
        files['assets/hd-hud/pilot/' + name] = ROOT / 'port/assets/hd-hud/pilot' / name
    manifest = ROOT / 'port/assets/hd-hud/sprites/manifest.json'
    if manifest.is_file():
        from review_hd_hud import approved_pieces
        for piece in approved_pieces(manifest):
            files['assets/hd-hud/sprites/' + piece['file']] = manifest.parent / piece['file']
    return files
