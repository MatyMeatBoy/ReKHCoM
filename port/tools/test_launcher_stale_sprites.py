"""The launcher finds HD sprites an updated recipe left behind (ReKHCoM.exe --hd-stale), without the HD install.

An update that changes a sprite's recipe (26/10: the battle notices' proportion) left older imports refused by the
game, which then drew the cartridge's Card Break. At its first start the launcher now imports the sprites again;
this checks the detection it relies on, with synthetic imports in a private folder:
  - an unmarked (older) import: one inventory migration, regardless of proportion;
  - the same picture at the recipe's proportion and a current inventory: 0;
  - a marked import whose mark differs from the current row: 1; matching mark: 0.
    python port/tools/test_launcher_stale_sprites.py [--exe build/pc/ReKHCoM.exe]
"""
import argparse, re, shutil, subprocess, tempfile
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]


def fnv(row):
    h = 2166136261
    for b in row.encode('utf-8'):
        h = ((h ^ b) * 16777619) & 0xFFFFFFFF
    return h


def stale(exe, folder):
    out = subprocess.run([str(folder / exe.name), '--hd-stale'], cwd=folder, capture_output=True, text=True, timeout=60)
    return int(re.search(r'(\d+) stale HD sprites', out.stderr + out.stdout).group(1))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', type=Path, default=ROOT / 'build/pc/ReKHCoM.exe')
    a = p.parse_args()
    row = next(l for l in (ROOT / 'port/data/hd_sprites.tsv').read_text(encoding='utf-8').splitlines()
               if l.startswith('gBtlPopCardBreakTiles\t'))
    rows = [l for l in (ROOT / 'port/data/hd_sprites.tsv').read_text(encoding='utf-8').splitlines()
            if l and not l.startswith('#') and l.split('\t')[9] != 'raw']
    inventory = '__recipe_inventory_v2__\t1\n' + ''.join(
        f'{l.split(chr(9))[0]}\t{fnv(l)}\n' for l in rows if l != row)
    f = row.split('\t')
    pad = re.search(r'pad=(\d+),(\d+)', f[9])
    want = (int(f[6]) + int(pad[1]) + int(pad[2])) / int(f[7])
    with tempfile.TemporaryDirectory() as tmp:
        d = Path(tmp)
        for name in (a.exe.name, 'SDL3.dll'):
            shutil.copy2(a.exe.parent / name, d / name)
        sprites = d / 'hd/sprites'
        sprites.mkdir(parents=True)
        png = sprites / 'gBtlPopCardBreakTiles.png'
        Image.new('RGBA', (int(f[6]) * 8, int(f[7]) * 8)).save(png)  # the proportion before pad=
        assert stale(a.exe, d) == 1, 'old-proportion picture not found'
        Image.new('RGBA', (round(want * 128), 128)).save(png)
        assert stale(a.exe, d) == 1, 'unmarked current-proportion import needs inventory migration'
        (sprites / 'recipes.txt').write_text(inventory + f'gBtlPopCardBreakTiles\t{fnv(row) ^ 1}\n')
        assert stale(a.exe, d) == 1, 'changed recipe mark not found'
        (sprites / 'recipes.txt').write_text(inventory + f'gBtlPopCardBreakTiles\t{fnv(row)}\n')
        assert stale(a.exe, d) == 0, 'matching recipe mark counted'
    print('PASS: launcher migrates legacy imports and compares current recipe inventories')


if __name__ == '__main__':
    main()
