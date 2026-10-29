"""Old imports must notice newly added recipes, including absent output files.

Run the actual launcher's read-only --hd-stale command in a private profile.
No ROM or remaster installation is needed.
"""
import argparse
from pathlib import Path
import shutil
import subprocess
import tempfile

from PIL import Image

from test_launcher_stale_sprites import ROOT, fnv, stale


def check_producer(cxx):
    source = (ROOT/'port/decompui/hd_import.inc').read_text(encoding='utf-8')
    functions = source[source.index('static uint32_t RecipeHash('):source.index('static int StaleSprites(')]
    prefix = r'''
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>
namespace fs = std::filesystem;
static const char* kHdSpriteRecipe[] = {
 "present\t6\tframe\ttiles\t-8\t-20\t16\t32\towned:tex\tscale=8",
 "missing\t6\tframe\ttiles\t-8\t-20\t16\t32\towned:tex\tscale=8",
 "raw_layout\t3\t-\t-\t0\t0\t0\t0\towned:layout\traw"
};
'''
    with tempfile.TemporaryDirectory(prefix='recipe-producer-') as temp:
        folder = Path(temp)
        unit = folder/'producer.cpp'
        unit.write_text(prefix+functions+'\nint main(int argc,char**argv){WriteRecipeMarks(argv[1]);}\n')
        exe = folder/'producer.exe'
        built = subprocess.run([str(cxx.resolve()),'-std=c++17','-static',str(unit),'-o',str(exe)],
                               capture_output=True,text=True)
        assert built.returncode == 0, built.stderr
        (folder/'present.png').write_bytes(b'only the existing output')
        subprocess.run([str(exe),str(folder)],check=True,timeout=10)
        marks = dict(line.split('\t') for line in (folder/'recipes.txt').read_text().splitlines())
        assert set(marks) == {'__recipe_inventory_v2__','present','missing'}, marks
        assert marks['__recipe_inventory_v2__'] == '1'
        assert int(marks['missing']) == fnv('missing\t6\tframe\ttiles\t-8\t-20\t16\t32\towned:tex\tscale=8')
    print('PASS: actual C++ producer records absent attempts and excludes raw layouts')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--cxx', type=Path, help='also compile and exercise the actual C++ inventory writer')
    args = parser.parse_args()
    if args.cxx:
        check_producer(args.cxx)
    rows = [line for line in (ROOT/'port/data/hd_sprites.tsv').read_text(encoding='utf-8').splitlines()
            if line and not line.startswith('#') and line.split('\t')[9] != 'raw']
    by_name = {line.split('\t')[0]: line for line in rows}
    card = 'gTranquilDarknessSmallCardHD'
    existing = 'gBtlPopCardBreakTiles'
    with tempfile.TemporaryDirectory(prefix='recipe-inventory-') as temp:
        root = Path(temp)
        for name in (args.exe.name, 'SDL3.dll'):
            shutil.copyfile(args.exe.parent/name, root/name)
        sprites = root/'hd/sprites'
        sprites.mkdir(parents=True)
        assert stale(args.exe, root) == 0, 'empty directory is not an existing import'
        # A legacy import knows its existing files. The added small-card PNG
        # does not exist, so checking only existing PNG hashes loses this update.
        Image.new('RGBA', (128,32)).save(sprites/(existing+'.png'))
        marks = sprites/'recipes.txt'
        marks.write_text(f'{existing}\t{fnv(by_name[existing])}\n')
        assert stale(args.exe, root) > 0, 'legacy import missed newly added recipes'
        inventory = '__recipe_inventory_v2__\t1\n'
        marks.write_text(inventory + ''.join(f'{name}\t{fnv(line)}\n' for name,line in by_name.items()))
        assert stale(args.exe, root) == 0, 'known unavailable regional/source rows should not cause a repair loop'
        marks.write_text(inventory + ''.join(f'{name}\t{fnv(line)}\n'
                                           for name,line in by_name.items() if name != card))
        assert stale(args.exe, root) == 1, 'missing output of a newly added recipe was not noticed'
        # Changed recipes are retried even if the earlier source lacked that
        # texture. Missing output alone is otherwise an ordinary native fallback.
        marks.write_text(inventory + ''.join(f'{name}\t{fnv(line) ^ (name == card)}\n'
                                           for name,line in by_name.items()))
        assert stale(args.exe, root) == 1, 'changed absent recipe was not noticed'
    print('PASS: legacy migration, added/changed absent recipes, known missing-source fallback, empty profile')


if __name__ == '__main__':
    main()
