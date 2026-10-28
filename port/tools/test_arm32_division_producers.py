"""Producer graph boundaries; real compiler/ABI tests are separate pilots."""
import importlib.util
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

HERE = Path(__file__).resolve().parent


def load_builder():
    spec = importlib.util.spec_from_file_location('arm32_linux_builder', HERE/'build_linux.py')
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class LinuxProducer(unittest.TestCase):
    def graph(self, arch):
        builder = load_builder()
        with tempfile.TemporaryDirectory(prefix='arm32-producer-') as folder:
            root = Path(folder)
            for name, text in {
                'include/btl/example.h': '', 'src/example.c': 'int game(int a,int b){return a%b;}',
                'port/src/example.c': 'int host(void){return 1;}',
                'port/src/port_symbols.s': '.globl _FixedCount\n.set _FixedCount, 26\n',
                'build/eu/pc/rom_symbols.s': '.globl _Rom\n.set _Rom, 0x08000000\n',
                'build/eu/pc/rom_relocs.c': '', 'build/eu/gen/example.h': '',
            }.items():
                path = root/name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(text)
            out = root/'build'/('linux-'+arch+'-eu')
            # Only the unrelated generated-ROM preparation is stubbed. The
            # production Ninja writer, target flags and graph edges execute.
            with patch.object(builder, 'ROOT', root), patch.object(builder.subprocess, 'check_call'):
                builder.configure('eu', arch, root/'sysroot-base', out)
            return (out/'build.ninja').read_text().replace('$\n', '').replace('\n    ', ' ')

    def test_armhf_game_boundary_and_invalidation(self):
        text = self.graph('armhf')
        self.assertIn('compile_cartridge_game.py --fingerprint', text)
        game = next(line for line in text.splitlines() if 'game/example.o:' in line)
        self.assertIn('cartridge_division_ir.py', game)
        self.assertIn('compile_cartridge_game.py', game)
        port = next(line for line in text.splitlines() if 'port/example.o:' in line)
        self.assertNotIn('cartridge_division', port)
        self.assertIn('cc_port', port)
        self.assertIn('-mfloat-abi=hard -mthumb', text)

    def test_i386_keeps_direct_compiler(self):
        text = self.graph('i386')
        self.assertNotIn('compile_cartridge_game', text)
        self.assertNotIn('cartridge_division_ir', text)
        self.assertIn('command = $cc $game_cflags -MD -MF $out.d -c $in -o $out', text)


if __name__ == '__main__':
    unittest.main()
