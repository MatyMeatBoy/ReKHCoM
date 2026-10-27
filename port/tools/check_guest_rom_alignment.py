"""Reject aligned SIMD accesses to misaligned cartridge symbols in an x32 guest ELF."""
import argparse
from pathlib import Path
import re
import subprocess

ROOT=Path(__file__).resolve().parents[2]


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('elf',type=Path)
    args=parser.parse_args()
    tool=next((ROOT.parent/'toolchain').glob('llvm-mingw-*/bin/llvm-objdump.exe'))
    code=subprocess.check_output([str(tool),'-d','--no-show-raw-insn',str(args.elf)],text=True)
    failures=[]
    for line in code.splitlines():
        # These instructions require alignment even on processors that accept
        # ordinary unaligned integer reads. Other SIMD reads are audited separately.
        if not re.search(r'\b(?:v?movap[sd]|v?movdqa(?:32|64)?)\b',line):
            continue
        target=re.search(r'#\s*0x([0-9a-fA-F]+)',line)
        if target:
            address=int(target.group(1),16)
            if 0x08000000<=address<0x0A000000 and address%16:
                failures.append(line.strip())
    for failure in failures[:25]:
        print(failure)
    print(f'{len(failures)} unsafe aligned SIMD accesses to ROM in {args.elf.name}')
    raise SystemExit(bool(failures))


if __name__=='__main__':
    main()
