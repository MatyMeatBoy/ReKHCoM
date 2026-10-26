"""ROM-free controls reset and input regressions against the production C code."""
from pathlib import Path
import os
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
cc = next((ROOT.parent / 'toolchain').glob('llvm-mingw-*/bin/i686-w64-mingw32-clang.exe'))
sdl = ROOT.parent / 'toolchain/SDL3-3.4.18/i686-w64-mingw32'
out = ROOT / 'build/controller-bindings-test'
out.mkdir(parents=True, exist_ok=True)
shutil.copy2(sdl / 'bin/SDL3.dll', out / 'SDL3.dll')
# Compile the unchanged production reset function, without linking the entire game's UI.
ui = (ROOT / 'port/src/port_ui.c').read_text(encoding='utf-8')
reset = ui[ui.index('static void ResetControlBindings(void) {'):ui.index('static void Change(int item, int dir) {')]
(out / 'reset_control_bindings.inc').write_text(reset, encoding='utf-8')
env = dict(os.environ, SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy')
for name in ['controller_bindings_conformance', 'input_controls_conformance']:
    game = [f for d in ['', 'bos', 'btl', 'card', 'debug', 'evt', 'lib', 'map', 'menu', 'msg', 'poo', 'sio', 'sroll', 'title']
            for f in ('-iquote', str(ROOT / 'include' / d))]
    exe = out / (name + '.exe')
    subprocess.run([str(cc), '-O2', '-std=gnu11', '-I' + str(out),
                    *game, '-DVERSION_EU', '-DPLATFORM_PC', '-mno-ms-bitfields', '-I' + str(ROOT / 'port/include'),
                    '-I' + str(sdl / 'include'), str(ROOT / 'port/tools' / (name + '.c')),
                    '-L' + str(sdl / 'lib'), '-lSDL3', '-o', str(exe)], check=True)
    subprocess.run([str(exe)], cwd=out, env=env, check=True)
