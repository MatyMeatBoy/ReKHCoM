#!/usr/bin/env python3
"""Generate the asset glue for the native PC build.

The PC port maps the original cartridge image at its GBA address (0x08000000),
so asset data is used straight from the ROM instead of being re-encoded and
assembled. This script reuses the decomp's own manifests to produce:

  build/<ver>/gen/*.h            asset declarations the C sources include
  build/<ver>/gen/*.c / *.inc    asset units and text fragments that are C code
  build/<ver>/pc/rom_symbols.s   every assembler-only asset symbol, pinned to its ROM address
  build/<ver>/pc/rom_relocs.c    ROM words that must point at native code/data instead
"""
import argparse
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools"))

import assetgen  # noqa: E402
import baserom  # noqa: E402
import sprite_sheet  # noqa: E402
import m4a_assets  # noqa: E402
import textgen  # noqa: E402

ROM_BASE = 0x08000000


class Gen:
    def __init__(self, version, rom):
        self.version = version
        self.rom = rom
        self.gen = ROOT / "build" / version / "gen"
        self.pc = ROOT / "build" / version / "pc"
        self.gen.mkdir(parents=True, exist_ok=True)
        self.pc.mkdir(parents=True, exist_ok=True)
        self.rom_symbols = {}   # name -> address, defined in ROM
        self.native_refs = []   # (rom address of word, symbol name)

    def word(self, address):
        return struct.unpack_from("<I", self.rom, address - ROM_BASE)[0]

    def bytes_at(self, address, size):
        return self.rom[address - ROM_BASE:address - ROM_BASE + size]

    def pin(self, name, address):
        old = self.rom_symbols.get(name)
        if old is not None and old != address:
            raise SystemExit(f"error: {name} pinned at both {old:#x} and {address:#x}")
        self.rom_symbols[name] = address

    # ---- text -------------------------------------------------------------

    def text(self):
        textgen.decode(self.version, self.rom)
        charmap = textgen.load_charmap(self.version)
        for path in sorted(textgen.POOL_DIR.glob("*.yaml")):
            textgen.generate(self.version, str(path))
            pool = textgen.Pool(path)
            if not pool.present(self.version):
                continue
            encoded = textgen.read_source(pool, self.version, charmap)
            for unit in pool.units(self.version):
                if unit.kind != "object":
                    continue
                placed, _end = unit.layout(lambda entry, _start: encoded[entry["name"]])
                for entry, start, _size in placed:
                    self.pin(entry["name"], start)

    # ---- graphics / audio / tables -----------------------------------------

    def sheet_block(self, manifest, entry):
        """Pin a sprite sheet's frames and animations, return its tile block."""
        version = self.version
        address = entry[version]["address"]
        data = self.bytes_at(address, entry[version]["size"])
        frames = manifest.frames(entry, version)
        anims = manifest.animations(entry, version)
        oam, _anims, block = sprite_sheet.parse_records(data, len(frames), len(anims))
        position = address
        for frame, attrs in zip(frames, oam):
            self.pin(manifest.item_symbol(frame, version), position)
            position += 2 + 6 * len(attrs) + (2 if attrs else 0)
        for anim in anims:
            self.pin(manifest.item_symbol(anim, version), position)
            position += 6 + 4 * len(anim["frames"])
        if not entry.get("tiles"):
            self.pin(manifest.symbol(entry, version), position)
        return None, None, block

    def assets(self):
        version = self.version
        manifests = assetgen.load_manifests()
        resolve = assetgen.resolver(manifests, version)
        units = {line.split()[0] for line in (ROOT / "config" / version / "units.txt").read_text().splitlines()
                 if line.strip() and not line.startswith("#")}
        pending_tables = []
        for manifest in manifests:
            members_by_object = manifest.members(version)
            sheets = {}
            for name, members in members_by_object.items():
                if name not in units:
                    continue
                if name.endswith(".c"):
                    for entry in members:
                        if "record" not in entry:
                            source = manifest.source(entry, version)
                            source.parent.mkdir(parents=True, exist_ok=True)
                            textgen.write_if_changed(source, self.bytes_at(entry[version]["address"],
                                                                           entry[version]["size"]))
                    assetgen.emit_c(manifest, version, members, self.gen / name, manifests)
                    continue
                for entry in members:
                    if entry.get("format") == "sprite_sheet":
                        sheets[entry["name"]] = self.sheet_block(manifest, entry)
                        continue
                    address = entry[version]["address"]
                    if entry.get("format") == "song":
                        # Track data comes first; the SongHeader the symbol names is at the end.
                        data = self.bytes_at(address, entry[version]["size"])
                        address += m4a_assets.song_header(data, address)
                    self.pin(manifest.symbol(entry, version), address)
                    kind = manifest.kind(entry)
                    if kind == "table":
                        pending_tables.append((manifest, entry))
                    elif kind == "struct":
                        pending_tables.append((manifest, entry))
            assetgen.emit_header(manifest, version, members_by_object, self.gen / f"{manifest.group}.h", sheets)

        # Table/struct words that refer to symbols the ROM image does not hold
        # (functions, RAM, data compiled from C) are patched at startup.
        for manifest, entry in pending_tables:
            address = entry[version]["address"]
            if manifest.kind(entry) == "table":
                for index, item in enumerate(manifest.data(entry, version, "items")):
                    if not item:
                        continue
                    self.reference(address + 4 * index, resolve(item))
            else:
                rtype = manifest.types[entry["record"]]
                values = manifest.fields(entry, version)
                elements = manifest.elements(entry, version)
                records = [values] if elements is None else elements
                record_size = assetgen.record_size(rtype, version)
                for index, record in enumerate(records):
                    for field, offset in self.pointer_fields(rtype, version):
                        value = record.get(field["name"]) if isinstance(record, dict) else None
                        if isinstance(value, str):
                            self.reference(address + index * record_size + offset, resolve(value))

    def pointer_fields(self, rtype, version):
        placed, _size, _align = assetgen.record_layout(rtype, version)
        for field, offset, _width, count, bit in placed:
            if field["type"].endswith("*") and count == 1 and bit is None:
                yield field, offset

    def reference(self, rom_address, symbol):
        if symbol in self.rom_symbols:
            return
        self.native_refs.append((rom_address, symbol))

    def fixed(self):
        path = ROOT / "config" / self.version / "symbols.txt"
        for line in path.read_text().splitlines():
            line = line.split("#")[0].strip()
            if line:
                name, addr = (x.strip() for x in line.split("="))
                self.pin(name, int(addr, 16))

    # ---- output -------------------------------------------------------------

    def write_rom_alignment(self):
        """ROM addresses obey the cartridge layout, not a native ABI's array padding.

        SysV x86-64 assumes arrays >=16 bytes are 16-byte aligned unless their
        declaration says otherwise. Pinned GBA palettes can be only 4 aligned:
        aligned SSE reads then fault even though the asset exists (#277).
        Keep the actual alignment rather than disabling SIMD for the whole game.
        """
        declaration = re.compile(r'^(\s*extern\b.*?)\b([A-Za-z_]\w*)((?:\[[^\]\n]*\])*)\s*;\s*$')
        for path in self.gen.rglob('*.h'):
            source = path.read_text(encoding='utf-8')
            lines = []
            for line in source.splitlines(keepends=True):
                match = declaration.match(line)
                address = self.rom_symbols.get(match.group(2)) if match else None
                if address is not None:
                    alignment = min(16, address & -address) if address else 1
                    line = line.rstrip().removesuffix(';') + f' __attribute__((aligned({alignment})));\n'
                lines.append(line)
            textgen.write_if_changed(path, ''.join(lines))

    def write(self):
        native = [(a, s) for a, s in self.native_refs if s not in self.rom_symbols]
        lines = ["/* generated by port/tools/gen_pc_assets.py */", "\t.section .rdata,\"dr\""]
        for name, address in sorted(self.rom_symbols.items(), key=lambda kv: (kv[1], kv[0])):
            lines.append(f"\t.globl _{name}")
            lines.append(f"\t.set _{name}, {address:#010x}")
        textgen.write_if_changed(self.pc / "rom_symbols.s", "\n".join(lines) + "\n")

        # The relocatable build (PORT_GBA_RELOC, include/gba/gba_space.h): every GBA region is an array of the
        # program, defined here with the symbols pinned inside them (the ROM's and port/src/port_symbols.s's), so
        # each is an offset from an array of the same file. Values below the GBA space are constants, kept as such.
        regions = [("gGbaEwram", 0x02000000, 0x40000), ("gGbaIwram", 0x03000000, 0x8000), ("gGbaIo", 0x04000000, 0x400),
                   ("gGbaPltt", 0x05000000, 0x400), ("gGbaVram", 0x06000000, 0x18000), ("gGbaOam", 0x07000000, 0x400),
                   ("gGbaSram", 0x0E000000, 0x10000), ("gGbaRom", ROM_BASE, 0x2000000)]
        lines = ["/* generated by port/tools/gen_pc_assets.py */", "\t.section .bss$gbaspace,\"bw\""]
        for name, _base, size in regions:
            lines += ["\t.balign 4096", f"\t.globl _{name}", f"_{name}:", f"\t.space {size:#x}"]
        fixed = {}
        for line in (ROOT / "port/src/port_symbols.s").read_text().splitlines():
            parts = line.split()
            if len(parts) == 3 and parts[0] == ".set":
                sym, value = parts[1].rstrip(",").lstrip("_"), int(line.split(",")[1], 16)
                fixed[sym] = value
        for name, address in sorted({**self.rom_symbols, **fixed}.items(), key=lambda kv: (kv[1], kv[0])):
            where = next(((r, address - base) for r, base, size in regions if base <= address < base + size), None)
            lines.append(f"\t.globl _{name}")
            lines.append(f"\t.set _{name}, _{where[0]} + {where[1]:#x}" if where else f"\t.set _{name}, {address:#010x}")
        textgen.write_if_changed(self.pc / "rom_symbols_reloc.s", "\n".join(lines) + "\n")

        lines = ["/* generated by port/tools/gen_pc_assets.py */", '#include "port/rom_relocs.h"', ""]
        for symbol in sorted({s for _a, s in native}):
            lines.append(f"extern char {symbol}[];")
        lines += ["", "const PortRomReloc gPortRomRelocs[] = {"]
        for address, symbol in sorted(native):
            lines.append(f"    {{ {address:#010x}, {symbol} }},")
        lines += ["    { 0, 0 },", "};", ""]
        textgen.write_if_changed(self.pc / "rom_relocs.c", "\n".join(lines))
        print(f"{self.version}: {len(self.rom_symbols)} ROM symbols, {len(native)} native relocations")


def refresh_generated_alignment(version):
    """Upgrade cached glue without rereading/re-encoding the cartridge."""
    symbols = ROOT / 'build' / version / 'pc/rom_symbols.s'
    if not symbols.is_file() or not (symbols.parent/'rom_symbols_reloc.s').is_file():
        return False
    if not (ROOT/'build'/version/'gen').is_dir():
        return False
    pinned = {name: int(address, 16) for name, address in
              re.findall(r'\.set _([A-Za-z_][A-Za-z0-9_]*), (0x[0-9a-fA-F]+)', symbols.read_text())}
    if not pinned:
        return False
    gen = Gen(version, b'')
    gen.rom_symbols = pinned
    gen.write_rom_alignment()
    return True


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--version", default="eu", choices=list(baserom.versions()))
    parser.add_argument('--ensure', action='store_true', help='Reuse generated assets and upgrade pinned declarations')
    args = parser.parse_args()
    from gen_scene_framing import generate
    generate(ROOT, ROOT / "build" / args.version / "gen/scene_framing_grants.h")
    if args.ensure and refresh_generated_alignment(args.version):
        return 0
    _source, rom = baserom.load(args.version)
    if rom is None:
        sys.exit(baserom.missing_message(args.version, purpose="the PC build"))
    gen = Gen(args.version, rom)
    gen.fixed()
    gen.text()
    gen.assets()
    gen.write_rom_alignment()
    gen.write()
    return 0


if __name__ == "__main__":
    sys.exit(main())
