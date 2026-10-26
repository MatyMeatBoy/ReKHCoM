"""Exercise the launcher's Steam artwork copier against fake library folders."""
from pathlib import Path
import subprocess
import tempfile
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]


def main():
    out = ROOT / 'build/steam-artwork-test'
    out.mkdir(parents=True, exist_ok=True)
    source = (ROOT / 'port/decompui/decompui.cpp').read_text(encoding='utf-8')
    marker = 'static void SteamArtwork('
    assert marker in source, 'Steam artwork needs one copier for the capsule and upper hero'
    begin = source.index(marker)
    end = source.index('/* ---- "Add to Steam"', begin)
    unit = '#include <filesystem>\n#include <fstream>\n#include <string>\nnamespace fs=std::filesystem;\nstatic fs::path sRoot;\n'
    unit += 'static bool SteamHdLogo(const fs::path& to) { if (!fs::exists(sRoot/"hd-logo")) return false; fs::copy_file(sRoot/"hd-logo",to,fs::copy_options::overwrite_existing); return true; }\n'
    unit += source[begin:end]
    unit += '\nint main(int argc,char** argv) { sRoot=fs::u8path(argv[1]); SteamArtwork(fs::u8path(argv[2]),"12345"); }\n'
    (out/'artwork.cpp').write_text(unit, encoding='utf-8')
    compiler = ROOT.parent/'toolchain/llvm-mingw-20261006-ucrt-x86_64/bin/i686-w64-mingw32-clang++.exe'
    subprocess.run([str(compiler), '-std=c++17', '-static', str(out/'artwork.cpp'), '-o', str(out/'artwork.exe')], check=True)
    with tempfile.TemporaryDirectory() as folder:
        root=Path(folder)/'game'; grid=Path(folder)/'grid'
        (root/'assets').mkdir(parents=True); grid.mkdir()
        for name in ('boxart.jpg','steam_header.jpg','steam_hero.jpg','logo.png'):
            (root/'assets'/name).write_bytes(name.encode())
        (grid/'unrelated.jpg').write_bytes(b'keep me')
        for hd in (False,True):
            if hd: (root/'hd-logo').write_bytes(b'HD logo')
            subprocess.run([str(out/'artwork.exe'),str(root),str(grid)],check=True)
            assert (grid/'12345.jpg').read_bytes()==b'steam_header.jpg'
            assert (grid/'12345_hero.jpg').read_bytes()==b'steam_hero.jpg', 'Hero must use its own wide background, not the capsule'
            assert (grid/'12345p.jpg').read_bytes()==b'boxart.jpg'
            assert (grid/'12345_logo.png').read_bytes()==(b'HD logo' if hd else b'logo.png')
            assert (grid/'unrelated.jpg').read_bytes()==b'keep me'
            print('PASS Steam capsule and distinct hero with '+('HD' if hd else 'default')+' logo')

    hero=ROOT/'port/decompui/assets/steam_hero.jpg'
    header=ROOT/'port/decompui/assets/steam_header.jpg'
    with Image.open(hero) as image:
        assert image.size==(3840,1240), 'Hero must preserve the existing wide battle capture'
    assert hero.read_bytes()!=header.read_bytes(), 'Hero and capsule must be distinct assets'
    for script in ('port/tools/core_assets.py','port/tools/package_release.py','port/configure_pc.py'):
        assert 'steam_hero.jpg' in (ROOT/script).read_text(encoding='utf-8'), script
    ui=(ROOT/'port/src/port_ui.c').read_text(encoding='utf-8')
    block=ui.split('static const char* const sHelpNg[LANG_COUNT] = {',1)[1].split('};',1)[0]
    assert block.count('↑↓')==6 and block.count('←→')==6, 'All six new-game help strings need valid UTF-8 arrows'
    print('PASS hero dimensions, packaging references, and six-language help arrows')


if __name__ == '__main__':
    main()
