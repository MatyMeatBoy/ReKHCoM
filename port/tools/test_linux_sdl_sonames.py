"""Real ELF fixtures: copied Windows sysroot links must retain runtime SONAMEs."""
from pathlib import Path
import argparse, importlib.util, subprocess, tempfile

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--cc', type=Path, required=True)
p.add_argument('--readelf', type=Path, required=True)
a = p.parse_args()
spec = importlib.util.spec_from_file_location('subject', Path(__file__).with_name('linux_sdl_sonames.py'))
subject = importlib.util.module_from_spec(spec)
spec.loader.exec_module(subject)

with tempfile.TemporaryDirectory(prefix='linux-sdl-sonames-') as td:
    root = Path(td)
    libs = root / 'sysroot/usr/lib/x86_64-linux-gnu'
    libs.mkdir(parents=True)
    c = root / 'stub.c'
    c.write_text('int fixture(void) { return 1; }\n')
    def library(name, soname):
        cmd = [str(a.cc), '--target=x86_64-linux-gnu', '-fuse-ld=lld', '-nostdlib',
               '-shared', '-fPIC', str(c), '-o', str(libs / name)]
        if soname is not None:
            cmd += ['-Wl,-soname,' + soname]
        subprocess.run(cmd, check=True)
    library('libX11.so', 'libX11.so.6')
    library('libXext.so', 'libXext.so.6')
    config = root / 'SDL_build_config.h'
    initial = ('#define SDL_VIDEO_DRIVER_X11_DYNAMIC "libX11.so"\n'
               '#define SDL_VIDEO_DRIVER_X11_DYNAMIC_XEXT "libXext.so"\n'
               '#define SDL_AUDIO_DRIVER_ALSA_DYNAMIC "libasound.so"\n'
               '/* #undef SDL_VIDEO_DRIVER_X11_DYNAMIC_XCURSOR */\n')
    config.write_text(initial)
    rows = subject.normalize_x11_sonames(config, root / 'sysroot', a.readelf)
    text = config.read_text()
    assert '"libX11.so.6"' in text and '"libXext.so.6"' in text
    assert '"libasound.so"' in text and '#undef SDL_VIDEO_DRIVER_X11_DYNAMIC_XCURSOR' in text
    assert len(rows) == 2 and all(row['changed'] for row in rows)
    # CMake may regenerate this file; rerunning must be stable either way.
    (libs / 'libX11.so.6').write_bytes((libs / 'libX11.so').read_bytes())
    (libs / 'libXext.so.6').write_bytes((libs / 'libXext.so').read_bytes())
    assert not any(row['changed'] for row in subject.normalize_x11_sonames(config, root / 'sysroot', a.readelf))
    assert config.read_text() == text
    # Validate every active entry before changing any of the generated header.
    for bad in ('missing', 'no-soname', 'unsafe-soname'):
        config.write_text(initial)
        path = libs / 'libXext.so'
        path.unlink()
        if bad == 'no-soname': library(path.name, None)
        elif bad == 'unsafe-soname': library(path.name, '../libXext.so.6')
        try:
            subject.normalize_x11_sonames(config, root / 'sysroot', a.readelf)
        except (ValueError, FileNotFoundError): pass
        else: raise AssertionError('accepted ' + bad)
        assert config.read_text() == initial, 'partially rewrote header on failure'
        if not path.exists(): library(path.name, 'libXext.so.6')
    print('PASS real ELF SONAMEs: copied aliases, idempotence, inactive/audio preservation, missing/invalid metadata and atomic validation')
