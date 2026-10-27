#!/usr/bin/env python3
import argparse
import math
import os
import subprocess
import sys
import tempfile
from pathlib import Path

import yaml

import m4a_assets
import sprite_sheet

YAML_LOADER = getattr(yaml, "CSafeLoader", yaml.SafeLoader)


def load_yaml(text):
    return yaml.load(text, Loader=YAML_LOADER)

ROOT = Path(__file__).resolve().parent.parent
MANIFEST_DIR = ROOT / "config" / "assets"
GBAGFX = ROOT / "tools" / "gbagfx" / "gbagfx"
VERSIONS = ("us", "jp", "eu")
SOURCE_EXT = {"tiles4": "png", "tiles8": "png", "palette": "pal", "tilemap": "bin", "raw": "bin", "sprite_sheet": "png",
              "glyphs": "png", **m4a_assets.SOURCE_EXT}
BINARY_EXT = {"tiles4": "4bpp", "tiles8": "8bpp", "palette": "gbapal", "tilemap": "bin", "raw": "bin",
              "sprite_sheet": "4bpp", "glyphs": "bin", **m4a_assets.BINARY_EXT}
GLYPH_GREYS = {1: [(0, 0, 0), (255, 255, 255)], 2: [(0, 0, 0), (85, 85, 85), (170, 170, 170), (255, 255, 255)]}
GBAGFX_MAX_COLORS = 256
ASM_KINDS = ("sprite", "anim", "array", "string")
SCALAR_SIZES = {"u8": 1, "s8": 1, "u16": 2, "s16": 2, "u32": 4, "s32": 4}
FORMAT_CTYPES = {"tilemap": "u16", "palette": "u16"}
CONST_KINDS = ("struct", "array", "string")
DIRECTIVES = {1: ".byte", 2: ".hword", 4: ".4byte"}
VALUES_PER_LINE = {1: 16, 2: 8, 4: 4}


class ManifestError(Exception):
    pass


class Manifest:
    def __init__(self, path):
        self.path = Path(path)
        with self.path.open() as handle:
            doc = load_yaml(handle)
        for key in ("group", "types", "objects", "entries"):
            if key not in doc:
                raise ManifestError(f"{self.path}: missing {key}")
        self.group = doc["group"]
        self.types = doc["types"]
        self.objects = doc["objects"]
        self.entries = doc["entries"]
        self.by_name = {}
        for entry in self.entries:
            if entry["name"] in self.by_name:
                raise ManifestError(f"{self.path}: duplicate entry {entry['name']}")
            self.by_name[entry["name"]] = entry

    def present(self, entry, version):
        return version in entry

    def symbol(self, entry, version):
        return entry[version].get("symbol", entry.get("symbol", entry["name"]))

    def kind(self, entry):
        if "record" not in entry:
            return None
        if entry["record"] not in self.types:
            raise ManifestError(f"{self.group}: {entry['name']} has the unknown record type {entry['record']}")
        return self.types[entry["record"]].get("kind", "struct")

    def data(self, entry, version, key):
        return entry[version].get(key, entry.get(key))

    def elements(self, entry, version):
        return self.data(entry, version, "elements")

    def values(self, entry, version):
        values = self.data(entry, version, "values")
        if values and isinstance(values[0], list):
            return [value for row in values for value in row]
        return values

    def string_bytes(self, entry, version):
        rtype = self.types[entry["record"]]
        try:
            return self.data(entry, version, "text").encode(rtype["encoding"]) + b"\0"
        except UnicodeError as error:
            raise ManifestError(f"{self.group}: {entry['name']}: {error}")

    def align(self, entry, version):
        if "align" in entry:
            return entry["align"]
        kind = self.kind(entry)
        if kind == "struct":
            return record_layout(self.types[entry["record"]], version)[2]
        if kind == "array":
            return max(scalar_size(self.types[entry["record"]]["type"]), self.types[entry["record"]].get("align", 1))
        if kind == "string":
            return self.types[entry["record"]].get("align", 1)
        return 1

    def size(self, entry, version):
        kind = self.kind(entry)
        if kind is None:
            return entry[version]["size"]
        if kind == "struct":
            size = record_size(self.types[entry["record"]], version)
            elements = self.elements(entry, version)
            return size if elements is None else size * len(elements)
        if kind == "array":
            return scalar_size(self.types[entry["record"]]["type"]) * len(self.values(entry, version))
        if kind == "string":
            return len(self.string_bytes(entry, version))
        if kind == "table":
            return 4 * len(self.data(entry, version, "items"))
        if kind == "sprite":
            count = len(self.data(entry, version, "oam"))
            return 2 + 6 * count + (2 if count else 0)
        if kind == "anim":
            return 6 + 4 * len(self.data(entry, version, "frames"))
        raise ManifestError(f"{self.group}: unknown record kind {kind}")

    def fields(self, entry, version):
        fields = dict(entry.get("fields", {}))
        fields.update(entry[version].get("fields", {}))
        return fields

    def source(self, entry, version):
        if "record" in entry or entry.get("format") == "fill":
            return None
        return ROOT / "assets" / version / self.group / f"{entry['name']}.{SOURCE_EXT[entry['format']]}"

    def binary(self, entry, version):
        if ("record" in entry or entry.get("tiles") or entry.get("format") in m4a_assets.INCLUDED
                or entry.get("format") == "fill"):
            return None
        return ROOT / "build" / version / "gen" / self.group / f"{entry['name']}.{BINARY_EXT[entry['format']]}"

    def sources(self, entry, version):
        source = self.source(entry, version)
        if source is None:
            return []
        if entry["format"] == "sprite_sheet" and sprite_sheet.layer_count(self.sheet(entry, version)) > 1:
            return [source, source.with_name(f"{entry['name']}.layers.png")]
        return [source]

    def frames(self, entry, version):
        return self.data(entry, version, "frames")

    def animations(self, entry, version):
        return self.data(entry, version, "animations")

    def item_symbol(self, item, version):
        return item.get(version, {}).get("symbol", item["symbol"])

    def sheet(self, entry, version):
        try:
            return {"cell": self.data(entry, version, "cell"), "anchor": self.data(entry, version, "anchor"),
                    "columns": self.data(entry, version, "columns"),
                    "frames": [[sprite_sheet.piece_from_yaml(piece) for piece in frame["pieces"]]
                               for frame in self.frames(entry, version)]}
        except sprite_sheet.SheetError as error:
            raise ManifestError(f"{self.group}: {entry['name']}: {error}")

    def names(self, entry, version):
        if entry.get("format") != "sprite_sheet":
            return {}
        items = self.frames(entry, version) + self.animations(entry, version)
        return {item["symbol"]: self.item_symbol(item, version) for item in items}

    def symbols(self, entry, version):
        if entry.get("format") != "sprite_sheet":
            return [self.symbol(entry, version)]
        block = [] if entry.get("tiles") else [self.symbol(entry, version)]
        return list(self.names(entry, version).values()) + block

    def anim_type(self):
        names = [name for name, rtype in self.types.items() if rtype.get("kind") == "anim"]
        if not names:
            raise ManifestError(f"{self.group}: a sprite sheet with animations needs an anim record type")
        return names[0]

    def members(self, version):
        objects = self.objects.get(version, [])
        found = {obj["name"]: [] for obj in objects}
        for entry in self.entries:
            if version not in entry:
                continue
            address = entry[version]["address"]
            owner = [obj for obj in objects if any(lo <= address < hi for lo, hi, _data in spans(obj))]
            if len(owner) != 1:
                raise ManifestError(f"{self.group}: {entry['name']} is in {len(owner)} {version} objects")
            found[owner[0]["name"]].append(entry)
        for obj in objects:
            members = sorted(found[obj["name"]], key=lambda e: e[version]["address"])
            for lo, hi, data in spans(obj):
                position = lo
                for entry in members:
                    if not lo <= entry[version]["address"] < hi:
                        continue
                    position = aligned(position, self.align(entry, version))
                    if entry[version]["address"] != position:
                        raise ManifestError(f"{self.group}: {obj['name']} has a gap before {entry['name']} at {position:#x} in {version}")
                    position += self.size(entry, version)
                    kind = self.kind(entry)
                    if "data_start" in obj and (kind == "table") != data:
                        raise ManifestError(f"{self.group}: {obj['name']} holds {entry['name']} in its {'.data' if data else 'payload'} range, which takes {'only' if data else 'no'} table records")
                    if obj["name"].endswith(".c") and entry.get("format") in ("sprite_sheet", "fill"):
                        raise ManifestError(f"{self.group}: {obj['name']} holds {entry['format']} {entry['name']} but is not an assembler object")
                    if obj["name"].endswith(".c") and (kind in ASM_KINDS or kind == "struct" and self.elements(entry, version) is not None):
                        raise ManifestError(f"{self.group}: {obj['name']} holds a {kind} record but is not an assembler object")
                if not position <= hi < position + 4:
                    raise ManifestError(f"{self.group}: {obj['name']} ends at {position:#x}, not {hi:#x}, in {version}")
            found[obj["name"]] = members
        return found


def spans(obj):
    out = [(obj["start"], obj["end"], False)]
    if "data_start" in obj:
        out.append((obj["data_start"], obj["data_end"], True))
    return out


def in_data(obj, entry, version):
    return "data_start" in obj and obj["data_start"] <= entry[version]["address"] < obj["data_end"]


def aligned(position, align):
    return (position + align - 1) // align * align


def scalar_size(ctype):
    return SCALAR_SIZES.get(ctype, 4 if ctype.endswith("*") else None)


def record_layout(rtype, version):
    placed, size, align, unit = [], 0, rtype.get("align", 1), None
    for field in rtype["fields"]:
        if version not in field.get("versions", VERSIONS):
            continue
        width = scalar_size(field["type"])
        if width is None:
            raise ManifestError(f"unknown field type {field['type']}")
        bits = field.get("bits")
        if bits:
            if unit is None or unit[1] != width or unit[2] + bits > 8 * width:
                size = aligned(size, width)
                unit = [size, width, 0]
                size += width
            placed.append((field, unit[0], width, 1, unit[2]))
            unit[2] += bits
        else:
            unit = None
            size = aligned(size, width)
            count = field.get("count", 1)
            placed.append((field, size, width, count, None))
            size += width * count
        align = max(align, width)
    return placed, aligned(size, align), align


def record_size(rtype, version):
    return record_layout(rtype, version)[1]


def element_values(manifest, entry, layout, element):
    names = [field["name"] for field, *_rest in layout]
    if isinstance(element, dict):
        missing = [name for name in names if name not in element]
        unknown = sorted(set(element) - set(names))
        if missing or unknown:
            raise ManifestError(f"{manifest.group}: {entry['name']} lacks {missing[:3]} or has unknown fields {unknown[:3]}")
        return element
    if len(element) != len(names):
        raise ManifestError(f"{manifest.group}: {entry['name']} element {element} has {len(element)} values for {len(names)} fields")
    return dict(zip(names, element))


def field_list(manifest, entry, field, count, value):
    if isinstance(value, str) and field.get("encoding"):
        data = value.encode(field["encoding"])
        if len(data) > count:
            raise ManifestError(f"{manifest.group}: {entry['name']}.{field['name']} is longer than {count} bytes")
        return list(data.ljust(count, b"\0"))
    if not isinstance(value, list) or len(value) != count:
        raise ManifestError(f"{manifest.group}: {entry['name']}.{field['name']} needs {count} values")
    return value


def record_chunks(manifest, entry, version):
    kind = manifest.kind(entry)
    rtype = manifest.types[entry["record"]]
    if kind == "array":
        width = scalar_size(rtype["type"])
        return [(index * width, width, value) for index, value in enumerate(manifest.values(entry, version))]
    if kind == "string":
        return [(index, 1, value) for index, value in enumerate(manifest.string_bytes(entry, version))]
    if kind == "table":
        return [(4 * index, 4, item or 0) for index, item in enumerate(manifest.data(entry, version, "items"))]
    if kind != "struct":
        raise ManifestError(f"{manifest.group}: {entry['name']} is a {kind} record, which has no field layout")
    layout, size, _align = record_layout(rtype, version)
    elements = manifest.elements(entry, version)
    elements = [manifest.fields(entry, version)] if elements is None else elements
    out = []
    for index, element in enumerate(elements):
        values = element_values(manifest, entry, layout, element)
        base = index * size
        units = {}
        for field, offset, width, count, shift in layout:
            value = values[field["name"]]
            if shift is not None:
                if not isinstance(value, int):
                    raise ManifestError(f"{manifest.group}: {entry['name']}.{field['name']} is a bit field and needs a number")
                packed = units.get(offset, (width, 0))[1] | ((value & ((1 << field["bits"]) - 1)) << shift)
                units[offset] = (width, packed)
            elif "count" in field:
                out += [(base + offset + i * width, width, item)
                        for i, item in enumerate(field_list(manifest, entry, field, count, value))]
            else:
                out.append((base + offset, width, value))
        out += [(base + offset, width, packed) for offset, (width, packed) in units.items()]
    return sorted(out, key=lambda chunk: chunk[0])


def expression(value, resolve):
    name, sign, rest = value.partition("+") if "+" in value else value.partition("-")
    return resolve(name.strip()) + (f"{sign}{rest.strip()}" if sign else "")


def value_text(value, width, resolve):
    if value is None:
        return "0"
    if isinstance(value, int):
        return str(value & ((1 << (8 * width)) - 1))
    if width != 4:
        raise ManifestError(f"symbol {value} does not fit a {width}-byte field")
    return expression(value, resolve)


def chunk_lines(chunks, size, resolve):
    lines, position, run = [], 0, None

    def flush():
        if run:
            lines.append(f"\t{DIRECTIVES[run[0]]} {', '.join(run[1])}")

    for offset, width, value in chunks:
        if offset < position:
            raise ManifestError(f"record fields overlap at offset {offset}")
        if offset > position:
            flush()
            run = (1, ["0"] * (offset - position))
            position = offset
        text = value_text(value, width, resolve)
        if run is None or run[0] != width or len(run[1]) >= VALUES_PER_LINE[width]:
            flush()
            run = (width, [])
        run[1].append(text)
        position = offset + width
    flush()
    if size > position:
        lines.append(f"\t.byte {', '.join(['0'] * (size - position))}")
    return lines


def record_lines(manifest, entry, version, resolve):
    return chunk_lines(record_chunks(manifest, entry, version), manifest.size(entry, version), resolve)


def check_record(manifest, entry, version, data, addresses):
    expected = bytearray(len(data))
    known = bytearray(b"\1" * len(data))
    for offset, width, value in record_chunks(manifest, entry, version):
        if isinstance(value, str):
            name, sign, rest = value.partition("+") if "+" in value else value.partition("-")
            if name.strip() not in addresses:
                known[offset:offset + width] = bytes(width)
                continue
            value = addresses[name.strip()] + (int(rest, 0) * (-1 if sign == "-" else 1) if sign else 0)
        expected[offset:offset + width] = ((value or 0) & ((1 << (8 * width)) - 1)).to_bytes(width, "little")
    for offset, (want, got, check) in enumerate(zip(expected, data, known)):
        if check and want != got:
            raise ManifestError(f"{manifest.group}: {entry['name']} differs from the {version} ROM at offset {offset:#x}")


def load_manifests(directory=MANIFEST_DIR):
    return [Manifest(path) for path in sorted(Path(directory).glob("*.yaml"))]


def entry_lookup(manifests):
    lookup = {}
    for manifest in manifests:
        for entry in manifest.entries:
            if entry["name"] in lookup:
                raise ManifestError(f"{manifest.group}: {entry['name']} is also an entry of {lookup[entry['name']].group}")
            lookup[entry["name"]] = manifest
    return lookup


def lender(lookup, entry, version):
    name = entry["tiles"]
    if name not in lookup or version not in lookup[name].by_name[name]:
        raise ManifestError(f"{entry['name']}: borrows tiles from {name}, which has no {version} entry")
    owner = lookup[name]
    target = owner.by_name[name]
    if target.get("tiles") or target.get("format") not in ("tiles4", "sprite_sheet") or target[version].get("compress"):
        raise ManifestError(f"{entry['name']}: {name} is not an uncompressed tile block of its own")
    if entry.get("tiles_offset", 0) % 32:
        raise ManifestError(f"{entry['name']}: tiles_offset {entry['tiles_offset']:#x} is not a whole number of tiles")
    return owner, target


def plan(version, manifests=None):
    manifests = load_manifests() if manifests is None else manifests
    lookup = {e["name"]: m for m in manifests for e in m.entries}
    out = {}
    for manifest in manifests:
        members = manifest.members(version)
        gen = ROOT / "build" / version / "gen"
        objects = {}
        for obj in manifest.objects.get(version, []):
            objects[obj["name"]] = {"source": gen / obj["name"], "members": members[obj["name"]],
                                    "binaries": [manifest.binary(e, version) for e in members[obj["name"]]
                                                 if manifest.binary(e, version) is not None],
                                    "includes": m4a_assets.includes(members[obj["name"]],
                                                                    lambda e: manifest.source(e, version))}
        sources = {path for e in manifest.entries if version in e and "record" not in e
                   for path in manifest.sources(e, version)}
        for e in manifest.entries:
            if version in e and e.get("format") == "sprite_sheet" and e.get("tiles"):
                owner, target = lender(lookup, e, version)
                sources.update(owner.sources(target, version))
        sources = sorted(sources, key=str)
        binaries = sorted({path for name, obj in objects.items() if name.endswith(".s")
                           for path in obj["binaries"]}, key=str)
        out[manifest.group] = {"manifest": manifest, "objects": objects, "header": gen / f"{manifest.group}.h",
                               "sources": sources, "binaries": binaries}
    return out


def run_gbagfx(*args):
    subprocess.run([str(GBAGFX)] + [str(a) for a in args], check=True, stdout=subprocess.DEVNULL)


def gba_to_rgb(entry):
    return tuple(((entry >> shift) & 0x1F) * 255 // 31 for shift in (0, 5, 10))


def write_jasc(path, data):
    lines = ["JASC-PAL", "0100", str(len(data) // 2)]
    for offset in range(0, len(data), 2):
        entry = data[offset] | (data[offset + 1] << 8)
        lines.append("%d %d %d" % gba_to_rgb(entry))
    Path(path).write_text("\r\n".join(lines) + "\r\n")


def glyph_geometry(entry, size):
    width, height = entry["glyph"]
    bpp = entry["bpp"]
    row = width * bpp // 8
    if bpp not in GLYPH_GREYS or width * bpp % 8 or size % (row * height):
        raise ManifestError(f"{entry['name']}: {size} bytes are not whole {width}x{height} {bpp}bpp glyphs")
    count = size // (row * height)
    columns = entry.get("columns", 16)
    return width, height, bpp, row, count, columns, (count + columns - 1) // columns


def glyphs_to_png(entry, data):
    width, height, bpp, row, count, columns, rows = glyph_geometry(entry, len(data))
    sheet = columns * width
    pixels = bytearray(sheet * rows * height)
    per, mask = 8 // bpp, (1 << bpp) - 1
    for glyph in range(count):
        left, top, base = (glyph % columns) * width, (glyph // columns) * height, glyph * row * height
        for y in range(height):
            for x in range(width):
                value = data[base + y * row + x // per] >> (8 - bpp * (x % per + 1))
                pixels[(top + y) * sheet + left + x] = value & mask
    return sprite_sheet.write_png(sheet, rows * height, pixels, GLYPH_GREYS[bpp])


def png_to_glyphs(entry, png, size):
    width, height, bpp, row, count, columns, rows = glyph_geometry(entry, size)
    try:
        sheet, sheet_height, pixels = sprite_sheet.read_png(png)
    except sprite_sheet.SheetError as error:
        raise ManifestError(f"{entry['name']}: {error}")
    if (sheet, sheet_height) != (columns * width, rows * height):
        raise ManifestError(f"{entry['name']}: the glyph sheet is {sheet}x{sheet_height}, the description needs {columns * width}x{rows * height}")
    per = 8 // bpp
    out = bytearray(size)
    for glyph in range(count):
        left, top, base = (glyph % columns) * width, (glyph // columns) * height, glyph * row * height
        for y in range(height):
            for x in range(width):
                value = pixels[(top + y) * sheet + left + x]
                if value >> bpp:
                    raise ManifestError(f"{entry['name']}: glyph {glyph} uses colour {value}, beyond {bpp}bpp")
                out[base + y * row + x // per] |= value << (8 - bpp * (x % per + 1))
    return bytes(out)


def jasc_colors(data):
    lines = data.decode("ascii").replace("\r\n", "\n").split("\n")
    if lines[:2] != ["JASC-PAL", "0100"]:
        raise ManifestError("not a JASC-PAL 0100 palette")
    count = int(lines[2])
    colors = [tuple(int(x) for x in line.split()) for line in lines[3:3 + count]]
    if len(colors) != count or any(len(c) != 3 or not all(0 <= x <= 255 for x in c) for c in colors):
        raise ManifestError(f"the palette does not list {count} RGB colours")
    return colors


def read_source(entry, source, tmp, size=None):
    fmt = entry["format"]
    if not source.exists():
        raise ManifestError(f"{source} is missing; run python3 tools/extract_assets.py")
    if fmt == "glyphs":
        return png_to_glyphs(entry, source.read_bytes(), size)
    if fmt == "palette" and int(source.read_bytes().split(b"\n", 3)[2]) > GBAGFX_MAX_COLORS:
        return b"".join(((r // 8) | ((g // 8) << 5) | ((b // 8) << 10)).to_bytes(2, "little")
                        for r, g, b in jasc_colors(source.read_bytes()))
    if fmt in ("palette", "tiles4", "tiles8"):
        out = tmp / f"{entry['name']}.{BINARY_EXT[fmt]}"
        run_gbagfx(source, out)
        return out.read_bytes()
    if fmt in m4a_assets.FORMATS:
        try:
            return m4a_assets.encode(fmt, source.read_bytes())
        except m4a_assets.M4aError as error:
            raise ManifestError(f"{source.relative_to(ROOT)}: {error}")
    return source.read_bytes()


def compress(data, method, tmp, name):
    if method != "lz77":
        raise ManifestError(f"{name}: unsupported compression {method}")
    plain = tmp / f"{name}.plain"
    packed = tmp / f"{name}.plain.lz"
    plain.write_bytes(data)
    run_gbagfx(plain, packed)
    return packed.read_bytes()


def encode(manifest, entry, version, tmp, source=None):
    source = manifest.source(entry, version) if source is None else source
    data = read_source(entry, source, tmp, entry[version]["size"])
    method = entry[version].get("compress")
    if method:
        data = compress(data, method, tmp, entry["name"])
    if len(data) != entry[version]["size"]:
        raise ManifestError(f"{entry['name']}: {version} source encodes to {len(data)} bytes, manifest says {entry[version]['size']}")
    return data


def decode_one(manifest, entry, version, data, tmp, palette_bytes):
    fmt = entry["format"]
    method = entry[version].get("compress")
    if method == "lz77":
        packed = tmp / f"{entry['name']}.bin.lz"
        plain = tmp / f"{entry['name']}.bin"
        packed.write_bytes(data)
        run_gbagfx(packed, plain)
        data = plain.read_bytes()
    elif method:
        raise ManifestError(f"{entry['name']}: unsupported compression {method}")
    if fmt in ("tilemap", "raw"):
        return data
    if fmt == "glyphs":
        return glyphs_to_png(entry, data)
    if fmt == "palette":
        out = tmp / f"{entry['name']}.pal"
        write_jasc(out, data)
        return out.read_bytes()
    depth = 4 if fmt == "tiles4" else 8
    unit = 32 if depth == 4 else 64
    if len(data) % unit:
        raise ManifestError(f"{entry['name']}: {len(data)} bytes is not a whole number of {depth}bpp tiles")
    tiles = len(data) // unit
    width = entry.get("width", 1)
    if tiles % width:
        raise ManifestError(f"{entry['name']}: width {width} does not divide {tiles} tiles")
    colors = 16 if depth == 4 else 256
    pal_path = tmp / f"{entry['name']}.hint.gbapal"
    pal_path.write_bytes((palette_bytes + bytes(colors * 2))[:colors * 2])
    raw = tmp / f"{entry['name']}.{BINARY_EXT[fmt]}"
    png = tmp / f"{entry['name']}.png"
    raw.write_bytes(data)
    run_gbagfx(raw, png, "-width", width, "-palette", pal_path)
    return png.read_bytes()


def borrowed_block(lookup, entry, version, tmp):
    owner, target = lender(lookup, entry, version)
    if target["format"] == "sprite_sheet":
        return encode_sheet(owner, target, version)[2][entry.get("tiles_offset", 0):]
    return encode(owner, target, version, tmp)[entry.get("tiles_offset", 0):]


def encode_sheet(manifest, entry, version, main=None, extra=None, borrowed=None):
    layout = manifest.sheet(entry, version)
    if main is None:
        paths = manifest.sources(entry, version)
        for path in paths:
            if not path.exists():
                raise ManifestError(f"{path} is missing; run python3 tools/extract_assets.py")
        main = paths[0].read_bytes()
        extra = paths[1].read_bytes() if len(paths) > 1 else None
    animations = manifest.animations(entry, version)
    try:
        layers = sprite_sheet.read_sheets(layout, main, extra)
        if entry.get("tiles"):
            if borrowed is None:
                raise ManifestError(f"{entry['name']}: {version} needs the tiles of {entry['tiles']}")
            oam, block = sprite_sheet.encode_view(layout, layers, borrowed), b""
        else:
            oam, block = sprite_sheet.encode(layout, layers)
        size = len(sprite_sheet.record_bytes(oam, animations)) + len(block)
    except sprite_sheet.SheetError as error:
        raise ManifestError(f"{entry['name']}: {version} {error}")
    if size != entry[version]["size"]:
        raise ManifestError(f"{entry['name']}: {version} sheet encodes to {size} bytes, manifest says {entry[version]['size']}")
    return oam, animations, block


def decode_sheet(manifest, entry, version, data, palette_bytes, borrowed=None):
    layout = manifest.sheet(entry, version)
    animations = manifest.animations(entry, version)
    try:
        oam, anims, block = sprite_sheet.parse_records(data, len(layout["frames"]), len(animations))
        if anims != [{"fields": anim["fields"], "frames": anim["frames"]} for anim in animations]:
            raise ManifestError(f"{entry['name']}: {version} animation headers differ from the description")
        if entry.get("tiles"):
            if block or borrowed is None:
                raise ManifestError(f"{entry['name']}: {version} borrows tiles but holds a block or has no lender")
            layout, layers = sprite_sheet.decode_view(oam, borrowed, layout)
        else:
            layout, layers = sprite_sheet.decode(oam, block, layout)
        main, extra = sprite_sheet.write_sheets(layout, layers, palette_bytes)
    except sprite_sheet.SheetError as error:
        raise ManifestError(f"{entry['name']}: {version} {error}")
    oam_out, animations, block_out = encode_sheet(manifest, entry, version, main, extra, borrowed)
    if sprite_sheet.record_bytes(oam_out, animations) + block_out != data:
        raise ManifestError(f"{entry['name']}: {version} sheet does not re-encode to the ROM bytes")
    return dict(zip(manifest.sources(entry, version), (main, extra)))


def decode(manifest, version, rom, rom_base=0x08000000, lookup=None):
    written = checked = 0
    palettes = {name: owner.by_name[name] for name, owner in lookup.items()} if lookup else manifest.by_name

    def rom_bytes(entry):
        address = entry[version]["address"]
        size = entry[version]["size"] if "size" in entry[version] else manifest.size(entry, version)
        data = rom[address - rom_base:address - rom_base + size]
        if len(data) != size:
            raise ManifestError(f"{entry['name']}: {version} extent is outside the ROM")
        return data

    addresses = {e["name"]: e[version]["address"] for other in (set(lookup.values()) if lookup else [manifest])
                 for e in other.entries if version in e and e.get("format") != "sprite_sheet"}
    for entry in manifest.entries:
        if version in entry and manifest.kind(entry) in ("struct", "array", "string", "table"):
            check_record(manifest, entry, version, rom_bytes(entry), addresses)
            checked += 1
        elif version in entry and entry.get("format") == "fill":
            if rom_bytes(entry) != bytes([entry["value"]]) * entry[version]["size"]:
                raise ManifestError(f"{manifest.group}: {entry['name']} is not a {entry['value']:#x} fill in the {version} ROM")
            checked += 1
    audio = None
    with tempfile.TemporaryDirectory() as tmpdir:
        tmp = Path(tmpdir)
        probe_dir = tmp / "probe"
        probe_dir.mkdir()
        for entry in manifest.entries:
            if "record" in entry or version not in entry or entry.get("format") == "fill":
                continue
            data = rom_bytes(entry)
            palette = palettes.get(entry.get("palette"))
            palette_bytes = rom_bytes(palette) if palette is not None and version in palette else b""
            if entry["format"] == "sprite_sheet":
                borrowed = None
                if entry.get("tiles"):
                    owner, target = lender(lookup or {e["name"]: manifest for e in manifest.entries}, entry, version)
                    borrowed = rom_bytes(target)
                    if target["format"] == "sprite_sheet":
                        frames, anims = owner.frames(target, version), owner.animations(target, version)
                        borrowed = sprite_sheet.parse_records(borrowed, len(frames), len(anims))[2]
                    borrowed = borrowed[entry.get("tiles_offset", 0):]
                files = decode_sheet(manifest, entry, version, data, palette_bytes, borrowed)
            elif entry["format"] in m4a_assets.FORMATS:
                try:
                    audio = audio or m4a_assets.Context(set(lookup.values()) if lookup else [manifest], version, rom_bytes)
                    decoded = audio.decode(entry, manifest.symbol(entry, version), entry[version]["address"], data)
                except m4a_assets.M4aError as error:
                    raise ManifestError(f"{manifest.group}: {version} {error}")
                files = {manifest.source(entry, version): decoded}
            else:
                decoded = decode_one(manifest, entry, version, data, tmp, palette_bytes)
                source = manifest.source(entry, version)
                probe = probe_dir / source.name
                probe.write_bytes(decoded)
                if encode(manifest, entry, version, tmp, probe) != data:
                    raise ManifestError(f"{entry['name']}: {version} {entry['format']} source does not re-encode to the ROM bytes")
                files = {source: decoded}
            checked += 1
            for source, decoded in files.items():
                if not source.exists() or source.read_bytes() != decoded:
                    source.parent.mkdir(parents=True, exist_ok=True)
                    source.write_bytes(decoded)
                    written += 1
    return checked, written


def field_versions(field):
    return field.get("versions", list(VERSIONS))


def c_value(manifest, resolve, field, value):
    ctype = field["type"]
    if field.get("count"):
        return "{ " + ", ".join(str(v) for v in value) + " }"
    if ctype.endswith("*"):
        if value == 0 or value is None:
            return "0"
        return f"({ctype}){resolve(value)}"
    return str(value)


def resolver(all_manifests, version):
    lookup = {}
    for other in all_manifests:
        for entry in other.entries:
            if version in entry:
                lookup[entry["name"]] = other.symbol(entry, version)
                lookup.update(other.names(entry, version))

    def resolve(name):
        return lookup.get(name, name)

    return resolve


def emit_c(manifest, version, members, out_path, all_manifests):
    resolve = resolver(all_manifests, version)
    defined = {manifest.symbol(e, version) for e in members}
    referenced = set()
    for entry in members:
        kind = manifest.kind(entry)
        if kind == "table":
            referenced.update(resolve(item) for item in manifest.data(entry, version, "items"))
        elif kind == "struct":
            rtype = manifest.types[entry["record"]]
            fields = manifest.fields(entry, version)
            for field in rtype["fields"]:
                if version not in field_versions(field) or not field["type"].endswith("*"):
                    continue
                value = fields[field["name"]]
                if value not in (0, None):
                    referenced.add(resolve(value))
    lines = []
    headers = sorted({manifest.types[e["record"]].get("header") for e in members if "record" in e} - {None}) or ["types.h"]
    for header in headers:
        lines.append(f'#include "{header}"')
    lines.append("")
    for name in sorted(referenced - defined):
        lines.append(f"extern u8 {name}[];")
    if referenced - defined:
        lines.append("")
    declared = False
    for entry in members:
        symbol = manifest.symbol(entry, version)
        if "record" not in entry:
            lines.append(f"extern const u8 {symbol}[{manifest.size(entry, version)}];")
            declared = True
        elif manifest.kind(entry) == "table" and symbol in referenced:
            ctype = manifest.types[entry["record"]]["type"]
            lines.append(f"extern {ctype} {symbol}[{len(manifest.data(entry, version, 'items'))}];")
            declared = True
    if declared:
        lines.append("")
    for entry in members:
        symbol = manifest.symbol(entry, version)
        kind = manifest.kind(entry)
        if kind == "table":
            ctype = manifest.types[entry["record"]]["type"]
            items = [f"({ctype}){resolve(item)}" for item in manifest.data(entry, version, "items")]
            lines.append(f"{ctype} {symbol}[{len(items)}] = {{")
            for item in items:
                lines.append(f"    {item},")
            lines.append("};")
        elif kind == "struct":
            rtype = manifest.types[entry["record"]]
            fields = manifest.fields(entry, version)
            lines.append(f"const {entry['record']} {symbol} = {{")
            for field in rtype["fields"]:
                if version not in field_versions(field):
                    continue
                lines.append(f"    {c_value(manifest, resolve, field, fields[field['name']])},")
            lines.append("};")
        else:
            data = manifest.source(entry, version).read_bytes()
            if entry["format"] != "raw" or entry[version].get("compress"):
                raise ManifestError(f"{entry['name']}: only raw entries may sit inside a C object")
            if len(data) != entry[version]["size"]:
                raise ManifestError(f"{entry['name']}: {version} source is {len(data)} bytes, manifest says {entry[version]['size']}")
            lines.append(f"const u8 {symbol}[{len(data)}] = {{")
            for offset in range(0, len(data), 24):
                chunk = data[offset:offset + 24]
                lines.append("    " + ", ".join(str(b) for b in chunk) + ",")
            lines.append("};")
        lines.append("")
    write_if_changed(out_path, "\n".join(lines).rstrip("\n") + "\n")


def words(values):
    return ", ".join(str(v) for v in values)


def emit_sheet(manifest, entry, version, sheet, lines):
    oam, animations, block = sheet
    for frame, attrs_list in zip(manifest.frames(entry, version), oam):
        symbol = manifest.item_symbol(frame, version)
        lines.append(f"\t.global {symbol}")
        lines.append(f"{symbol}:")
        lines.append(f"\t.hword {len(attrs_list)}")
        for attrs in attrs_list:
            lines.append(f"\t.hword {words(attrs)}")
        if attrs_list:
            lines.append("\t.hword 0")
    for anim in animations:
        symbol = manifest.item_symbol(anim, version)
        lines.append(f"\t.global {symbol}")
        lines.append(f"{symbol}:")
        lines.append(f"\t.hword {anim['fields']['originX']}, {anim['fields']['originY']}, {len(anim['frames'])}")
        for frame in anim["frames"]:
            lines.append(f"\t.hword {words(frame)}")
    binary = manifest.binary(entry, version)
    if binary is None:
        return
    write_if_changed(binary, block)
    symbol = manifest.symbol(entry, version)
    lines.append(f"\t.global {symbol}")
    lines.append(f"{symbol}:")
    lines.append(f'\t.incbin "{binary.relative_to(ROOT).as_posix()}"')


def emit_s(manifest, version, members, out_path, tmp, obj, sheets, resolve):
    lines = m4a_assets.prelude(members) + ["\t.section .rodata"]
    position = obj["start"]
    if position % 4 == 0:
        lines.append("\t.balign 4")
    elif position % 2 == 0:
        lines.append("\t.balign 2")
    tables = [e for e in members if in_data(obj, e, version)]
    for entry in members:
        if in_data(obj, entry, version):
            continue
        symbol = manifest.symbol(entry, version)
        pad = aligned(position, manifest.align(entry, version)) - position
        if pad:
            lines.append(f"\t.byte {words([0] * pad)}")
            position += pad
        if entry.get("format") == "sprite_sheet":
            emit_sheet(manifest, entry, version, sheets[entry["name"]], lines)
            position += manifest.size(entry, version)
            continue
        if entry.get("format") in m4a_assets.INCLUDED:
            source = manifest.source(entry, version)
            if not source.exists():
                raise ManifestError(f"{source.relative_to(ROOT)} is missing; run python3 tools/extract_assets.py")
            lines.append(f'\t.include "{source.relative_to(ROOT).as_posix()}"')
            position += manifest.size(entry, version)
            continue
        lines.append(f"\t.global {symbol}")
        lines.append(f"{symbol}:")
        kind = manifest.kind(entry)
        if kind == "sprite":
            oam = manifest.data(entry, version, "oam")
            lines.append(f"\t.hword {len(oam)}")
            for attrs in oam:
                lines.append(f"\t.hword {words(attrs)}")
            if oam:
                lines.append("\t.hword 0")
        elif kind == "anim":
            fields = manifest.fields(entry, version)
            frames = manifest.data(entry, version, "frames")
            lines.append(f"\t.hword {fields['originX']}, {fields['originY']}, {len(frames)}")
            for frame in frames:
                lines.append(f"\t.hword {words(frame)}")
        elif kind is not None:
            lines += record_lines(manifest, entry, version, resolve)
        elif entry.get("format") == "fill":
            lines.append(f"\t.fill {entry[version]['size']}, 1, {entry['value']}")
        else:
            binary = manifest.binary(entry, version)
            data = encode(manifest, entry, version, tmp)
            binary.parent.mkdir(parents=True, exist_ok=True)
            write_if_changed(binary, data)
            lines.append(f'\t.incbin "{binary.relative_to(ROOT).as_posix()}"')
        position += manifest.size(entry, version)
    if obj["end"] > position:
        lines.append(f"\t.byte {words([0] * (obj['end'] - position))}")
    if "data_start" in obj:
        lines.append("\t.section .data")
        position = obj["data_start"]
        if position % 4 == 0:
            lines.append("\t.balign 4")
        for entry in tables:
            symbol = manifest.symbol(entry, version)
            lines.append(f"\t.global {symbol}")
            lines.append(f"{symbol}:")
            items = [resolve(item) if item else 0 for item in manifest.data(entry, version, "items")]
            lines.append(f"\t.4byte {words(items)}")
            position += manifest.size(entry, version)
        if obj["data_end"] > position:
            lines.append(f"\t.byte {words([0] * (obj['data_end'] - position))}")
    write_if_changed(out_path, "\n".join(lines) + "\n")


def qualifier(item, kind=None):
    return "const " if item.get("const", kind in CONST_KINDS) else ""


def scalar_count(manifest, entry, version, ctype):
    size = manifest.size(entry, version)
    if size % SCALAR_SIZES[ctype]:
        raise ManifestError(f"{manifest.group}: {entry['name']} is {size} bytes in {version}, not a whole number of {ctype}")
    return size // SCALAR_SIZES[ctype]


def dimensions(manifest, entry, version, count, unit):
    shape = manifest.data(entry, version, "shape") or [count]
    if math.prod(shape) != count:
        raise ManifestError(f"{manifest.group}: {entry['name']} has shape {shape} but {count} {unit} in {version}")
    return "".join(f"[{n}]" for n in shape)


def emit_header(manifest, version, members_by_object, out_path, sheets):
    guard = "GUARD_GEN_" + manifest.group.upper() + "_H"
    lines = [f"#ifndef {guard}", f"#define {guard}", ""]
    headers = {manifest.types[e.get("record", e.get("type"))].get("header") for members in members_by_object.values()
               for e in members if ("record" in e or "type" in e) and e.get("declare") is not False}
    if any(manifest.animations(e, version) for members in members_by_object.values()
           for e in members if e.get("format") == "sprite_sheet"):
        headers.add(manifest.types[manifest.anim_type()].get("header"))
    headers = sorted(headers - {None})
    if not headers:
        headers = ["types.h"]
    for header in headers:
        lines.append(f'#include "{header}"')
    lines.append("")
    for name, members in members_by_object.items():
        for entry in members:
            if entry.get("format") == "sprite_sheet":
                for frame in manifest.frames(entry, version):
                    if frame.get("declare") is not False:
                        lines.append(f"extern {qualifier(frame)}u16 {manifest.item_symbol(frame, version)}[];")
                for anim in manifest.animations(entry, version):
                    if anim.get("declare") is not False:
                        lines.append(f"extern {qualifier(anim)}{manifest.anim_type()} {manifest.item_symbol(anim, version)};")
                if entry.get("declare") is not False and not entry.get("tiles"):
                    lines.append(f"extern {qualifier(entry)}u8 {manifest.symbol(entry, version)}[{len(sheets[entry['name']][2])}];")
                continue
            if entry.get("declare") is False:
                continue
            symbol = manifest.symbol(entry, version)
            kind = manifest.kind(entry)
            const = qualifier(entry, kind)
            if kind == "table":
                ctype = manifest.types[entry["record"]]["type"]
                lines.append(f"extern {ctype} {symbol}[{len(manifest.data(entry, version, 'items'))}];")
            elif kind == "struct" and "ctype" in entry:
                count = scalar_count(manifest, entry, version, entry["ctype"])
                lines.append(f"extern {const}{entry['ctype']} {symbol}{dimensions(manifest, entry, version, count, entry['ctype'])};")
            elif kind == "struct":
                if not manifest.types[entry["record"]].get("header"):
                    raise ManifestError(f"{manifest.group}: {entry['name']} is declared but {entry['record']} has no header")
                elements = manifest.elements(entry, version)
                count = "" if elements is None else dimensions(manifest, entry, version, len(elements), entry["record"])
                lines.append(f"extern {const}{entry['record']} {symbol}{count};")
            elif kind == "array":
                rtype = manifest.types[entry["record"]]
                values = len(manifest.values(entry, version))
                columns = rtype.get("columns")
                shape = f"[{values}]" if not columns else f"[{values // columns}][{columns}]"
                lines.append(f"extern {const}{rtype.get('ctype', rtype['type'])} {symbol}{shape};")
            elif kind == "string":
                lines.append(f"extern {const}{manifest.types[entry['record']].get('ctype', 'char')} {symbol}[];")
            elif kind == "anim":
                lines.append(f"extern {const}{entry['record']} {symbol};")
            elif kind == "sprite":
                lines.append(f"extern {const}u16 {symbol}[];")
            elif name.endswith(".s") and "type" in entry:
                lines.append(f"extern {const}{entry['type']} {symbol};")
            elif name.endswith(".s"):
                ctype = entry.get("ctype", FORMAT_CTYPES.get(entry.get("format"), "u8"))
                count = scalar_count(manifest, entry, version, ctype)
                lines.append(f"extern {const}{ctype} {symbol}{dimensions(manifest, entry, version, count, ctype)};")
    lines += ["", "#endif"]
    write_if_changed(out_path, "\n".join(lines) + "\n")


def write_if_changed(path, content):
    path = Path(path)
    data = content if isinstance(content, bytes) else content.encode()
    if path.exists() and path.read_bytes() == data:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    tmp = path.with_name(path.name + ".tmp")
    tmp.write_bytes(data)
    os.replace(tmp, path)


def check_symbols(manifest, version, members_by_object):
    ledger = set()
    for line in (ROOT / "config" / version / "symbols.txt").read_text().splitlines():
        line = line.split("#")[0].strip()
        if line:
            ledger.add(line.split("=")[0].strip())
    clashes = sorted(symbol for members in members_by_object.values()
                     for e in members for symbol in manifest.symbols(e, version) if symbol in ledger)
    if clashes:
        raise ManifestError(f"{manifest.group}: {version} symbols.txt still defines {', '.join(clashes[:5])}")


def check_names(manifests, version):
    owners = {}
    for manifest in manifests:
        for entry in manifest.entries:
            if version not in entry:
                continue
            keys = []
            if entry.get("format") == "sprite_sheet":
                keys = [item["symbol"] for item in manifest.frames(entry, version) + manifest.animations(entry, version)]
                if len(keys) != len(set(keys)):
                    raise ManifestError(f"{manifest.group}: {entry['name']} names an item twice in {version}")
            for symbol in set(keys) | set(manifest.symbols(entry, version)):
                owner = owners.setdefault(symbol, entry["name"])
                if owner != entry["name"]:
                    raise ManifestError(f"{version}: {symbol} is defined by both {owner} and {entry['name']}")


def generate(version, manifest_path, all_manifests=None):
    manifest = Manifest(manifest_path)
    all_manifests = load_manifests() if all_manifests is None else all_manifests
    members_by_object = manifest.members(version)
    if not GBAGFX.exists():
        raise ManifestError(f"{GBAGFX.relative_to(ROOT)} is missing; run sh tools/fetch_gbagfx.sh")
    check_names(all_manifests, version)
    check_symbols(manifest, version, members_by_object)
    gen = ROOT / "build" / version / "gen"
    gen.mkdir(parents=True, exist_ok=True)
    objects = {obj["name"]: obj for obj in manifest.objects.get(version, [])}
    lookup = {e["name"]: m for m in all_manifests for e in m.entries}
    with tempfile.TemporaryDirectory() as tmpdir:
        tmp = Path(tmpdir)
        sheets = {e["name"]: encode_sheet(manifest, e, version,
                                          borrowed=borrowed_block(lookup, e, version, tmp) if e.get("tiles") else None)
                  for members in members_by_object.values() for e in members if e.get("format") == "sprite_sheet"}
        needs = any("data_start" in obj for obj in objects.values()) or any(
            manifest.kind(e) not in (None, "sprite", "anim") for name, members in members_by_object.items()
            if name.endswith(".s") for e in members)
        resolve = resolver(all_manifests, version) if needs else None
        for name, members in members_by_object.items():
            if name.endswith(".c"):
                emit_c(manifest, version, members, gen / name, all_manifests)
            else:
                emit_s(manifest, version, members, gen / name, tmp, objects[name], sheets, resolve)
    emit_header(manifest, version, members_by_object, gen / f"{manifest.group}.h", sheets)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("version", choices=VERSIONS)
    parser.add_argument("manifest")
    args = parser.parse_args()
    try:
        generate(args.version, args.manifest)
    except (ManifestError, subprocess.CalledProcessError) as error:
        sys.exit(f"error: {error}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
