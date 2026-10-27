"""Run the real PortLog producer across restarts; never use a player's data folder."""
from pathlib import Path
import subprocess
import tempfile

R = Path(__file__).resolve().parents[2]


def contents(path):
    return ''.join(line for line in path.read_text().splitlines(keepends=True)
                   if not line.startswith('# session:'))


def main():
    source = (R / 'port/src/port_main.c').read_text(encoding='utf-8')
    start = source.index('void PortLog(const char* fmt, ...) {')
    end = source.index('\nvoid PortFatal(', start)
    header = R / 'port/include/port/port_logfile.h'
    code = '#include <stdio.h>\n#include <stdarg.h>\n'
    if header.exists():
        code += '#include "port/port_logfile.h"\n'
    code += 'static FILE* sLog;\nstatic const char* PortConfigPath(const char* p) { return p; }\n'
    code += source[start:end]
    code += 'int main(int argc, char** argv) { PortLog("session %s", argv[1]); PortLog("end %s", argv[1]); return 0; }\n'
    cc = next((R.parent / 'toolchain').glob('llvm-mingw-*/bin/x86_64-w64-mingw32-clang.exe'))
    with tempfile.TemporaryDirectory(prefix='rekhcom-log-history-') as name:
        folder = Path(name)
        src, exe = folder / 'log.c', folder / 'log.exe'
        src.write_text(code, encoding='utf-8')
        subprocess.run([str(cc), '-I', str(R/'port/include'), str(src), '-o', str(exe)], check=True)
        # Guest builds have no wall-clock libc. Their native host records time.
        guest = folder/'guest.c'
        guest.write_text('#include "port/port_logfile.h"\nvoid probe(void) { PortOpenSessionLog("guest.log"); }\n')
        clang = cc.with_name('clang.exe')
        resource = subprocess.check_output([str(clang), '-print-resource-dir'], text=True).strip()
        subprocess.run([str(clang), '--target=x86_64-linux-gnux32', '-ffreestanding', '-nostdinc',
                        '-isystem', str(Path(resource)/'include'),
                        '-I', str(R/'port/ps5/guest/include'), '-I', str(R/'port/include'),
                        '-fsyntax-only', str(guest)], check=True)
        for n in range(8):
            subprocess.run([str(exe), str(n)], cwd=folder, check=True, stderr=subprocess.DEVNULL)
        assert contents(folder/'kh_com.log') == 'session 7\nend 7\n'
        for previous in range(1, 6):
            path = folder / f'kh_com.log.{previous}'
            assert path.exists(), f'previous session log missing: {path.name}'
            assert contents(path) == f'session {7-previous}\nend {7-previous}\n'
        assert not (folder/'kh_com.log.6').exists(), 'history must stay bounded'
        # A blocked rotation must append, never truncate the last evidence.
        (folder/'kh_com.log.5').unlink()
        (folder/'kh_com.log.5').mkdir()
        (folder/'kh_com.log.5/keep').write_text('do not remove')
        subprocess.run([str(exe), '8'], cwd=folder, check=True, stderr=subprocess.DEVNULL)
        assert contents(folder/'kh_com.log') == 'session 7\nend 7\nsession 8\nend 8\n'
        assert (folder/'kh_com.log.5/keep').read_text() == 'do not remove'
    print('PASS: current + five previous logs; rotation errors preserve evidence')


if __name__ == '__main__':
    main()
