"""No aligned SSE access to cartridge data in the 64-bit guest (#277).

Cartridge data sits at its ROM address (0x08000000-0x0A000000), aligned to 4 at most. An instruction that requires
16-byte alignment (movaps, movdqa, SSE arithmetic with a memory operand) on such an address faults: the Steam Deck
crashed opening the first door (LoadMapSelectKindPalette). ROM_ALIGN on the generated declarations prevents it; this
scans the built guest for any rip-relative aligned access into the ROM that is not 16-byte aligned.
    python port/tools/test_rom_alignment.py [--region eu]   (after port/linux64/build_linux64.py)
"""
import argparse, re, subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ALIGNED = re.compile(r'\b(movaps|movapd|movdqa|vmovaps|vmovapd|vmovdqa|pand|pandn|por|pxor|padd\w|psub\w|pcmp\w+|'
                     r'pmin\w+|pmax\w+|pavg\w|pmul\w+|psad\w+|pack\w+|punpck\w+|pshufb|andps|andnps|orps|xorps|addps|'
                     r'subps|mulps|divps|minps|maxps|cmpps|sqrtps)\b')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--region', nargs='+', default=['eu', 'us', 'jp'])
    args = p.parse_args()
    objdump = next((ROOT.parent / 'toolchain').glob('llvm-mingw-*/bin/llvm-objdump.exe'), None) or 'llvm-objdump'
    checked = 0
    for region in args.region:
        elf = ROOT / f'port/ps5/build/guest-{region}/guest.elf'
        if not elf.exists():
            continue
        text = subprocess.run([str(objdump), '-d', '--no-show-raw-insn', str(elf)], capture_output=True, text=True,
                              check=True).stdout
        bad, fn = [], None
        for line in text.splitlines():
            m = re.match(r'^[0-9a-f]+ <(.+)>:', line)
            if m:
                fn = m.group(1)
            elif ALIGNED.search(line):
                t = re.search(r'# (0x[0-9a-f]+)', line)
                if t and 0x8000000 <= int(t.group(1), 16) < 0xA000000 and int(t.group(1), 16) % 16:
                    bad.append(f'{fn}: {line.strip()}')
        assert not bad, f'{region}: aligned access to unaligned ROM data:\n' + '\n'.join(bad[:20])
        checked += 1
    assert checked, 'no guest.elf built: run port/linux64/build_linux64.py first'
    print(f'PASS: {checked} guest image(s), no aligned SSE access to unaligned cartridge data')


if __name__ == '__main__':
    main()
