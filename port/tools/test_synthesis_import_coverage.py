"""Import every current room01 requirement through the production C++ consumer.

Private decoded texture required; never stages the legacy Pillow fallback.
Writes 405 PNGs, 20 contact sheets and hashes in a new isolated evidence folder.
No game, SDL, GPU, audio, installed assets or saves are touched.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

import numpy as np
from PIL import Image, ImageDraw

import khhd_sprites
import synthesis_requirement_hd as synthesis

ROOT = Path(__file__).resolve().parents[2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--texture', type=Path, required=True)
    parser.add_argument('--work', type=Path, required=True)
    parser.add_argument('--baseline-sprites', type=Path, help='Read-only current room01 import to check unchanged glyphs')
    args = parser.parse_args()
    original = sha(args.texture)
    args.work.mkdir(parents=True, exist_ok=True)
    out = Path(tempfile.mkdtemp(prefix='room01-coverage-', dir=args.work))
    assets = out / 'sprites'
    assets.mkdir()
    texture = Image.open(args.texture).convert('RGBA')
    pixels = np.asarray(texture)
    source = (ROOT / 'port/decompui/hd_import.inc').read_text(encoding='utf-8')
    struct = source[source.index('struct Rgba {'):source.index('static uint16_t U16(')]
    functions = source[source.index('static Rgba Resample('):source.index('\nstruct Job {')]
    prelude = '''#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <cstring>
#include <functional>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <filesystem>
'''
    main_cpp = r'''
int main(int argc,char** argv){
 if(argc!=6)return 2;
 Rgba texture; texture.Resize(atoi(argv[2]),atoi(argv[3]));
 std::ifstream raw(argv[1],std::ios::binary);
 raw.read((char*)texture.px.data(),texture.px.size());if(!raw)return 3;
 std::ifstream rows(argv[4]);std::string line;
 while(std::getline(rows,line)){
  if(line.empty()||line[0]=='#')continue;
  std::vector<std::string> f;size_t p=0;
  for(;;){size_t q=line.find('\t',p);f.push_back(line.substr(p,q-p));if(q==std::string::npos)break;p=q+1;}
  if(f.size()!=10)return 4;
  Rgba result;
  if(!ApplyRecipe(texture,f[9],atoi(f[6].c_str()),atoi(f[7].c_str()),result,
       [](const std::string&,Rgba&){return false;}))return 5;
  std::ofstream file(std::filesystem::u8path(argv[5])/ (f[0]+".rgba"),std::ios::binary);
  int size[2]={result.w,result.h};file.write((char*)size,sizeof(size));
  file.write((char*)result.px.data(),result.px.size());if(!file)return 6;
 }
 return 0;
}
'''
    unit = out / 'import-consumer.cpp'
    unit.write_text(prelude + struct + functions + main_cpp, encoding='utf-8')
    compiler = ROOT.parents[1] / 'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/clang++.exe'
    exe = out / 'import-consumer.exe'
    subprocess.run([str(compiler), '-std=c++17', '-O1', '-static', str(unit), '-o', str(exe)], check=True)
    rows = synthesis.recipe_rows()
    committed = (ROOT / 'port/data/hd_sprites.tsv').read_text(encoding='utf-8').splitlines()
    assert all(row in committed for row in rows), 'recipes must be regenerated before testing'
    recipe = out / 'recipes.tsv'
    recipe.write_text('\n'.join(rows) + '\n', encoding='utf-8')
    raw_texture = out / 'texture.rgba'
    raw_texture.write_bytes(pixels.tobytes())
    subprocess.run([str(exe), str(raw_texture), str(texture.width), str(texture.height), str(recipe), str(assets)],
                   check=True, timeout=90)
    raw_texture.unlink()
    pieces = []
    for _, name, box, rule, value in synthesis.specs():
        raw = assets / (name + '.rgba')
        blob = raw.read_bytes()
        width, height = np.frombuffer(blob[:8], dtype='<i4')
        native = np.frombuffer(blob[8:], dtype=np.uint8).reshape(height, width, 4)
        assert (width, height) == (box[2] * 8, box[3] * 8), name
        ops = synthesis.recom_body(box, value) if rule is None else synthesis.recom_requirement(box, rule, value)
        reference = khhd_sprites.apply(pixels, ops, box)
        delta = np.abs(native.astype(np.int16) - reference.astype(np.int16))
        assert delta.max() <= 1, (name, 'native/reference mismatch', int(delta.max()))
        if rule is None and ';erase=' in ops:
            baseline = khhd_sprites.apply(pixels, ops.split(';erase=')[0] + ';scale=8', box)
            assert np.array_equal(reference[:287], baseline[:287]), (name, 'changed outside bottom strip')
        elif args.baseline_sprites:
            baseline = np.asarray(Image.open(args.baseline_sprites / (name + '.png')).convert('RGBA'))
            assert np.array_equal(reference, baseline), (name, 'changed approved requirement')
        alpha = native[:, :, 3]
        assert alpha.max() > 32 and not alpha[0].any() and not alpha[-1].any(), (name, 'clipped/empty')
        assert not alpha[:, 0].any() and not alpha[:, -1].any(), (name, 'clipped sides')
        path = assets / (name + '.png')
        Image.fromarray(native, 'RGBA').save(path)
        raw.unlink()
        pieces.append(dict(name=name, size=[int(width), int(height)], sha256=sha(path),
                           native_reference_max_delta=int(delta.max())))
    assert len(pieces) == 405 and len({p['name'] for p in pieces}) == 405
    # All 2,000 combinations, composed at their actual OAM origins; sheets
    # show imported textures, not screenshots and not evidence of GPU filtering.
    for color in synthesis.COLORS:
        body = Image.open(assets / f'synthesis_body_{color}.png').convert('RGBA')
        for rule in synthesis.RULES:
            sheet = Image.new('RGB', (1280, 1760), (42, 42, 65))
            draw = ImageDraw.Draw(sheet)
            for value in range(100):
                name = f'synthesis_{rule}_{value:02d}'
                spec = next(s for s in synthesis.specs() if s[1] == name)
                requirement = Image.open(assets / (name + '.png')).convert('RGBA')
                card = body.copy()
                x, y = (spec[2][0] + 16) * 8, (spec[2][1] + 20) * 8
                card.alpha_composite(requirement, (x, y))
                x, y = value % 10 * 128, value // 10 * 176
                thumb = card.resize((128, 156), Image.Resampling.LANCZOS)
                sheet.paste(thumb, (x, y + 18), thumb)
                draw.text((x + 3, y + 2), f'{value} {rule} {color}', fill='white')
            sheet.save(out / f'all-{rule}-{color}.png')
    assert sha(args.texture) == original, 'source texture changed'
    report = dict(source_commit=subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip(),
                  texture=str(args.texture.resolve()), texture_sha256=original,
                  generator_sha256=sha(ROOT / 'port/tools/synthesis_requirement_hd.py'),
                  harness_sha256=sha(Path(__file__)),
                  import_functions_sha256=hashlib.sha256(functions.encode()).hexdigest(),
                  recipes_sha256=sha(recipe), exe_sha256=sha(exe), pieces=pieces,
                  combinations=2000, contact_sheets=20, scope='CPU production importer + reference; no GPU/game QA')
    (out / 'results.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
    print(out)
    print('PASS:405 native imports/reference parity;2000 visual combinations;source unchanged')


if __name__ == '__main__':
    main()
