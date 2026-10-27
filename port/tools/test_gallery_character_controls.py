"""Compile the production gallery consumer; no ROM, window, audio or save files."""
from pathlib import Path
import os
import re
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def function(source, name):
    match = re.search(r'(?m)^(?:static )?(?:void|int)\s+' + name + r'\([^;\n]*\)\s*\{', source)
    assert match, name
    start = match.start()
    end = match.end()
    depth = 1
    while depth:
        depth += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def main():
    source = (ROOT / 'port/src/port_scene_gallery_menu.inc').read_text(encoding='utf8')
    ui = (ROOT / 'port/src/port_ui.c').read_text(encoding='utf8')
    declarations = source[source.index('static int sSceneChoice'):source.index('static int SceneAt')]
    names = ['SceneResetTriggers', 'SceneKey', 'SceneCount', 'SceneRawAt', 'SceneRiku', 'SceneBuildView',
             'SceneViewCount', 'SceneAt', 'SceneImportedAt', 'SceneProviderHd', 'SceneMarkBroken',
             'SceneMenuOpen']
    for optional in ['SceneChangeView', 'SceneCharacterHelp']:
        if optional + '(' in source:
            names.append(optional)
    core = declarations + '\n' + '\n'.join(function(source, name) for name in names)
    core += '\n' + function(source, 'SceneMenuInput') + '\n' + function(source, 'PortUiGalleryResume')
    core += '\n' + function(ui, 'UiPadButton')
    core += '\n' + function(ui, 'UiListTriggerReserved')
    base = next(parent for parent in ROOT.parents if (parent / 'toolchain').is_dir())
    cc = next((base / 'toolchain').glob('llvm-mingw-*/bin/i686-w64-mingw32-clang.exe'))
    sdl = base / 'toolchain/SDL3-3.4.18/i686-w64-mingw32'
    out = ROOT / 'build/gallery-character-controls'
    out.mkdir(parents=True, exist_ok=True)
    (out / 'gallery-consumer.inc').write_text(core, encoding='utf8')
    shutil.copy2(sdl / 'bin/SDL3.dll', out / 'SDL3.dll')
    env = {key: value for key, value in os.environ.items() if not key.startswith(('KHCOM_', 'SDL_'))}
    env.update(SDL_VIDEO_DRIVER='dummy', SDL_AUDIO_DRIVER='dummy')
    for region in ['EU', 'US', 'JP']:
        exe = out / ('probe-' + region.lower() + '.exe')
        subprocess.run([str(cc), '-O2', '-std=gnu11', '-Wall', '-Wextra', '-Wno-unused-function',
                        '-Wno-unused-variable', '-Wno-unused-but-set-variable', '-Wno-unused-but-set-global',
                        '-DVERSION_' + region, '-DPLATFORM_PC', '-mno-ms-bitfields',
                        '-I' + str(out), '-I' + str(ROOT / 'port/include'), '-iquote', str(ROOT / 'include'),
                        '-I' + str(sdl / 'include'), str(ROOT / 'port/tools/gallery_character_conformance.c'),
                        '-L' + str(sdl / 'lib'), '-lSDL3', '-o', str(exe)], check=True)
        subprocess.run([str(exe)], cwd=out, env=env, check=True, timeout=15)
        targeted = dict(env, KHCOM_GALLERY_EVENT='149')
        subprocess.run([str(exe), '--targeted'], cwd=out, env=targeted, check=True, timeout=15)


if __name__ == '__main__':
    main()
