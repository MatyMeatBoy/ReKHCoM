"""Run production miniz/HD pack installer against complete and interrupted variants."""
import os
from pathlib import Path
import shutil
import struct
import subprocess
import tempfile
import zipfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    compiler = os.environ.get('CC') or shutil.which('clang') or shutil.which('gcc')
    if not compiler:
        compiler = str(sorted((ROOT.parent / 'toolchain').glob('llvm-mingw-*/bin/clang.exe'))[-1])
    with tempfile.TemporaryDirectory(prefix='khcom-media-pack-') as folder:
        root = Path(folder)
        harness = root / 'test.c'
        harness.write_text('#include <stdio.h>\n#include <stdarg.h>\nvoid Log(const char*f,...){va_list a;va_start(a,f);vfprintf(stderr,f,a);fputc(10,stderr);va_end(a);}\nint HdPackInstall(const char*,void(*)(const char*,...));\nint main(int n,char**v){if(n!=2)return 2;printf("%d\\n",HdPackInstall(v[1],Log));return 0;}\n')
        exe = root / ('test.exe' if os.name == 'nt' else 'test')
        miniz = ROOT / 'port/ps5/third_party/miniz'
        subprocess.run([compiler, '-std=gnu11', '-O0', '-static', '-I' + str(miniz),
                        str(harness), str(ROOT / 'port/portable/hd_pack.c'),
                        *[str(miniz / name) for name in ('miniz.c', 'miniz_tdef.c', 'miniz_tinfl.c', 'miniz_zip.c')],
                        '-o', str(exe)], check=True)
        data = root / 'data'
        data.mkdir()
        def package(generation, marker, incomplete=False, mixed=False):
            movie = struct.pack('>I4sI4s', 8, b'moov', 16, b'mdat') + marker * 8
            records = ['movies/ROP.mp4 24']
            if incomplete or mixed:
                records.append(('scenes/zz_event_001.mp4' if mixed else 'scenes/zz_event_151.mp4') + ' 24')
            meta = f'REKHCOM_MEDIA_V1 en user aabb 1 1 {int(mixed)} {len(records)}\n' + '\n'.join(records) + '\n'
            with zipfile.ZipFile(data / 'ReKHCoM-HD.zip', 'w', zipfile.ZIP_STORED) as z:
                # Deliberately first: the installer must defer publication.
                z.writestr('hd/media/en/active.txt', f'REKHCOM_MEDIA_ACTIVE_V1\nmovies {generation}\nscenes {generation if mixed else chr(45)}\n')
                z.writestr(f'hd/media/en/{generation}/media.meta', meta)
                z.writestr(f'hd/media/en/{generation}/movies/ROP.mp4', movie)
                if mixed and not incomplete:
                    z.writestr(f'hd/media/en/{generation}/scenes/zz_event_001.mp4', movie)
                z.writestr('hd/media/en/.stage-private/movies/ROP.mp4', b'must not copy')
                z.writestr('hd/media/en/g-nope/../../user.sav', b'must not copy')
            return (data / 'ReKHCoM-HD.zip').stat().st_size
        last_log = ''
        def run():
            nonlocal last_log
            result = subprocess.run([str(exe), str(data)], text=True, capture_output=True, check=True)
            last_log = result.stderr
            return result.stdout.strip()
        old_size = package('g-old', b'O')
        assert run() == '1'
        active = data / 'hd/media/en/active.txt'
        old = active.read_bytes()
        assert not (data / 'hd/media/en/.stage-private').exists()
        assert not (data / 'hd/media/user.sav').exists()
        assert run() == '0', 'unchanged central-directory fingerprint must skip'
        package('g-bad', b'B', incomplete=True)
        assert run() == '-1', 'incomplete media transfer must report failure, not permit deleting its pack'
        assert active.read_bytes() == old, 'incomplete transferred generation replaced old active'
        assert 'zip can be deleted now' not in last_log and 'keep ' in last_log
        new_size = package('g-new', b'N')
        assert old_size == new_size, 'fixture must prove same-size package replacement'
        assert run() == '1'
        assert 'g-new' in active.read_text()
        assert (data / 'hd/media/en/g-old/movies/ROP.mp4').read_bytes().endswith(b'O' * 8)
        assert (data / 'hd/media/en/g-new/movies/ROP.mp4').read_bytes().endswith(b'N' * 8)
        package('g-both', b'M', mixed=True)
        assert run() == '1', 'mixed movie/story manifest must validate both components without NULL comparison'
        assert active.read_text().count('g-both') == 2
        before_partial = active.read_bytes()
        package('g-partial', b'P', incomplete=True, mixed=True)
        assert run() == '-1', 'incomplete mixed transfer must not report success'
        assert active.read_bytes() == before_partial, 'partial mixed restore must preserve active movies AND scenes'
        assert 'zip can be deleted now' not in last_log and 'keep ' in last_log
        assert (data / 'hd/media/en/g-both/scenes/zz_event_001.mp4').exists()
    print('PASS production pack: deferred activation, incomplete preservation, strict paths and same-size update')


if __name__ == '__main__':
    main()
