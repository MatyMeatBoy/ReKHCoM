"""Round-trip game settings through the production launcher INI functions.

Game-only keys need not have launcher controls: SaveIni preserves unknown
numeric keys. Compile the real parser/writer and check that preservation,
including game changes made after the launcher read the file.
"""
import re
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    config = (ROOT / 'port/src/port_config.c').read_text(encoding='utf-8')
    launcher = (ROOT / 'port/decompui/decompui.cpp').read_text(encoding='utf-8')
    game = set(re.findall(r'\{ "([a-z0-9_]+)", &gPortConfig', config))
    start = launcher.index('struct Setting {')
    settings = launcher[start:launcher.index('/* The monitor\'s fullscreen resolutions', start)]
    table = settings[settings.index('kSettings[] = {'):settings.index('};', settings.index('kSettings[] = {'))]
    known = set(re.findall(r'\{ "([a-z0-9_]+)",', table))
    writer_start = launcher.index('static void SaveIni() {')
    writer = launcher[writer_start:launcher.index('\n}\n', writer_start) + 3]
    reset_start = launcher.index('if (ImGui::Button(T("Restablecer controles", "Reset controls"))) {')
    reset = launcher[launcher.index('\n', reset_start) + 1:launcher.index('\n    }', reset_start)]
    unit = '''#include <SDL3/SDL.h>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <map>
#include <string>
#include <cstring>
#include "port/port_zoom.h"
namespace fs = std::filesystem;
static fs::path sRoot;
static std::string sCapture;
'''
    unit += settings + '\n' + writer
    unit += '''
int main(int argc, char** argv) {
    sRoot = fs::u8path(argv[1]); LoadIni();
    std::string mode=argv[2];
    if(mode=="default") assert(Ini("pad_layout")==0);
    if(mode=="merge") {
        Ini("music_volume")=41; /* user's edit in the launcher */
        std::ofstream(sRoot/"kh_com.ini") << "music_volume=62\\npad_layout=1\\nassets_seen=11\\nfuture_setting=77\\n";
    }
    if(mode=="reset") {
        sCapture="pad_a";
        for(const Setting& setting:kSettings)
            if(std::strncmp(setting.key,"kb_",3)==0||std::strncmp(setting.key,"pad_",4)==0) Ini(setting.key)=999;
'''
    unit += reset
    unit += '''
        assert(sCapture.empty());
        for(const Setting& setting:kSettings)
            if(std::strncmp(setting.key,"kb_",3)==0||std::strncmp(setting.key,"pad_",4)==0) assert(Ini(setting.key)==setting.def);
    }
    SaveIni();
}
'''
    out = ROOT / 'build/settings-keys-test'
    out.mkdir(parents=True, exist_ok=True)
    (out / 'settings.cpp').write_text(unit, encoding='utf-8')
    compiler = next((ROOT.parent / 'toolchain').glob('llvm-mingw-*/bin/i686-w64-mingw32-clang++.exe'))
    sdl = ROOT.parent / 'toolchain/SDL3-3.4.18/i686-w64-mingw32/include'
    exe = out / 'settings.exe'
    subprocess.run([str(compiler), '-std=c++17', '-O1', '-static', '-I' + str(sdl), '-I' + str(ROOT / 'port/include'),
                    str(out / 'settings.cpp'), '-o', str(exe)], check=True)
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / 'kh_com.ini'
        def values():
            return dict(line.split('=', 1) for line in path.read_text(encoding='utf-8').splitlines()
                        if '=' in line and not line.startswith('#'))
        def run(mode):
            subprocess.run([str(exe), directory, mode], check=True)
        run('default')
        assert values()['pad_layout'] == '0'
        print('PASS default controller layout: auto, Nintendo forcing off')
        preserved = {key: str(100 + i) for i, key in enumerate(sorted(game - known))}
        preserved.update(future_setting='321', music_volume='99', pad_layout='2', online_name='Jugador con nombre')
        path.write_text('config_version=11\r\n' + ''.join(f'{key}={value}\r\n' for key, value in preserved.items()), encoding='utf-8')
        run('roundtrip')
        current = values()
        assert all(current.get(key) == value for key, value in preserved.items()), (preserved, current)
        print(f'PASS game-only keys ({len(game-known)}), future key and online name round-trip')
        run('merge')
        current = values()
        assert current['music_volume'] == '41', current
        assert current['pad_layout'] == '1' and current['assets_seen'] == '11' and current['future_setting'] == '77', current
        print('PASS game/launcher concurrent settings merge')
        run('reset')
        assert values()['pad_layout'] == '0'
        print('PASS actual launcher control reset: defaults and pending capture')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
