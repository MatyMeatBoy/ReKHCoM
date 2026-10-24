"""Builds one test file per GBA save format around a real save (public specs only).

Used by test_save_formats.py and runnable alone to leave the files in a folder:

    python port/tools/save_formats.py --base <kh_com.sav> --out <folder>

Every builder returns the bytes of a file the importer must read back to the base save.
The layouts are the public ones: mGBA's sharkport.c (SharkPortSave / ADVSAVEG), mGBA's
state PNG (gbAs / gbAx, zlib) and raw extdata table, VBA's gzip state, RetroArch's RZIP,
DeSmuME's .dsv footer, the 3DS Virtual Console AGBSAVE header, libretro VBA's 136 KB file.
"""
import argparse
import gzip
import io
import random
import struct
import zipfile
import zlib
from pathlib import Path

KH_USED_END = 0x6B70  # end of the last block; the rest of the 64 KB file is erased
SIG_EU = b"KHCOM_BACKUP_VER00000013"
SIG_US = b"KHCOM_BACKUP_VER00000012"
BLOCKS = [(0x10, 0x40), (0x50, 0x40), (0x90, 0xF14), (0xFA4, 0xF14), (0x1EC0, 0x418), (0x22D8, 0x418),
          (0x26F0, 0x418), (0x2B08, 0x418), (0x2F20, 0xF14), (0x3E34, 0xF14), (0x4D48, 0xF14), (0x5C5C, 0xF14)]


def checksum(block):
    total = 0
    for i in range(0, len(block) - 1, 2):
        total += block[i] | block[i + 1] << 8
    if len(block) & 1:
        total += block[-1]
    total = (total & 0xFFFF) + (total >> 16)
    return (~(total + (total >> 16))) & 0xFFFF


def seal(data, at, size):
    data[at + 24:at + 26] = b"\0\0"
    data[at + 24:at + 26] = struct.pack("<H", checksum(bytes(data[at:at + size])))


def to_region(save, region):
    """Same save written by another cartridge: signature and header language, resealed."""
    out = bytearray(save)
    for at, size in BLOCKS:
        if bytes(out[at:at + 24]) not in (SIG_EU, SIG_US) or checksum(bytes(out[at:at + size])) != 0:
            continue  # like the game: a block that is not valid stays as it is
        out[at:at + 24] = SIG_EU if region == 0 else SIG_US
        if size == 0x40 and region != 0:
            out[at + 0x1E:at + 0x20] = b"\0\0"
        seal(out, at, size)
    return bytes(out)


def noise(n, seed):
    rng = random.Random(seed)
    return bytes(rng.getrandbits(8) for _ in range(n))


def png(chunks):
    def chunk(kind, data):
        return struct.pack(">I", len(data)) + kind + data + struct.pack(">I", zlib.crc32(kind + data))
    ihdr = struct.pack(">IIBBBBB", 240, 160, 8, 2, 0, 0, 0)
    idat = zlib.compress(b"".join(b"\0" + b"\x20\x40\x80" * 240 for _ in range(160)))
    out = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", idat)
    for kind, data in chunks:
        out += chunk(kind, data)
    return out + chunk(b"IEND", b"")


def sharkport(sram, title=b"KH COM\0\0\0\0\0\0"):
    header = bytearray(0x1C)
    header[:12] = title
    header[0x12] = 0x5A
    header[0x13] = 0x38
    header[0x14] = 1
    total = 0
    for byte in bytes(header) + sram:
        total = (total + (byte << (total % 24))) & 0xFFFFFFFF
    date = b"10/24/2026 04:00:00 PM"
    out = struct.pack("<I", 13) + b"SharkPortSave" + struct.pack("<I", 0x000F0000)
    out += struct.pack("<I", len(title)) + title + struct.pack("<I", len(date)) + date + struct.pack("<I", 0)
    out += struct.pack("<I", 0x1C + len(sram)) + bytes(header) + sram + struct.pack("<I", total)
    return out


def adv_save_g(sram):
    head = bytearray(0x430)
    head[:8] = b"ADVSAVEG"
    head[0x0C:0x18] = b"KH COM\0\0\0\0\0\0"
    head[0x1C:0x20] = struct.pack("<I", 2)  # SRAM
    head[0x2C:0x40] = b"Kingdom Hearts CoM"
    head[0x42C:0x430] = b"xV4\x12"
    return bytes(head) + sram


def mgba_extdata_state(sram):
    state = noise(0x30000, 11)
    table = struct.pack("<IiQ", 1, 0, 0)  # placeholder, replaced below
    items = [(2, sram)]  # EXTDATA_SAVEDATA
    header_len = 16 * (len(items) + 1)
    offset = len(state) + header_len
    out = bytearray(state)
    for tag, data in items:
        out += struct.pack("<IiQ", tag, len(data), offset)
        offset += len(data)
    out += struct.pack("<IiQ", 0, 0, 0)
    for _, data in items:
        out += data
    del table
    return bytes(out)


def rzip(payload, chunk=0x8000):
    out = b"#RZIPv\x01#" + struct.pack("<I", chunk) + struct.pack("<Q", len(payload))
    for at in range(0, len(payload), chunk):
        comp = zlib.compress(payload[at:at + chunk])
        out += struct.pack("<I", len(comp)) + comp
    return out


def dsv(sram):
    footer = b"|<--Snip above here to create a raw sav by excluding this DeSmuME savedata footer:"
    footer += struct.pack("<IIIII", len(sram), 1, 0, 0, len(sram)) + b"|-DESMUME SAVE-|"
    return sram + footer


def reorder(data, group):
    out = bytearray(data)
    for at in range(0, len(out) - group + 1, group):
        out[at:at + group] = bytes(reversed(out[at:at + group]))
    return bytes(out)


def zipped(name, data, method):
    buf = io.BytesIO()
    with zipfile.ZipFile(buf, "w", method) as z:
        z.writestr(name, data)
    return buf.getvalue()


def nocash(sram, packed=False):
    # NO$GBA "Raw" setting stores the bytes after its 0x4C-byte header, flag 0 = uncompressed
    head = b"NocashGbaBackupMediaSavDataFile\x1a" + struct.pack("<I", 0x10000) + b"\0" * 12 + b"SRAM" + \
        struct.pack("<I", 1 if packed else 0) + struct.pack("<II", len(sram), len(sram))
    return head.ljust(0x4C, b"\0") + sram


def build(base):
    """name -> (extension, bytes, expectation): 'same' (back to the base), 'region' (other cartridge's
    save, converted), 'blocks:N' (readable, fewer valid blocks) or 'fail' (documented as unsupported)."""
    assert len(base) == 0x10000
    sram = base[:0x8000]
    out = {}
    out["port_raw_64k"] = ("sav", base, "same")
    out["sram_32k"] = ("sav", sram, "same")
    out["sram_32k_srm"] = ("srm", sram, "same")
    out["trimmed_to_last_block"] = ("sav", base[:KH_USED_END], "same")
    out["flash_128k_padded"] = ("sav", base + b"\xFF" * 0x10000, "same")
    out["flash_128k_second_bank"] = ("sav", b"\xFF" * 0x10000 + base, "same")
    out["flash_128k_mirrored"] = ("sav", base + base, "same")
    out["zero_padded_128k"] = ("sav", sram + b"\0" * 0x18000, "same")
    out["libretro_vba_136k"] = ("srm", base + b"\xFF" * 0x10000 + b"\xFF" * 0x2000, "same")
    out["eeprom_style_8k_prefix"] = ("sav", b"\xFF" * 0x2000 + base, "same")
    out["swap16"] = ("sav", reorder(base, 2), "same")
    out["swap32"] = ("sav", reorder(base, 4), "same")
    out["reversed_64bit_blocks"] = ("sav", reorder(base, 8), "same")
    out["reversed_64bit_32k"] = ("sav", reorder(sram, 8), "same")
    out["sharkport_sps"] = ("sps", sharkport(sram), "same")
    out["sharkport_xps"] = ("xps", sharkport(sram, b"KH CoM\0\0\0\0\0\0"), "same")
    out["gameshark_sp_gsv"] = ("gsv", adv_save_g(sram), "same")
    out["gameshark_64k_payload"] = ("gsv", adv_save_g(base), "same")
    out["vba_gzip_sgm"] = ("sgm", gzip.compress(struct.pack("<I", 11) + noise(0x2000, 1) + base + b"\xFF" * 0x10000 +
                                                 noise(0x40000, 2)), "same")
    out["gzip_raw_sav"] = ("sav.gz", gzip.compress(base), "same")
    out["zlib_raw_sav"] = ("sav", zlib.compress(base), "same")
    out["mgba_ss_png_gbAx"] = ("ss1", png([(b"gbAs", zlib.compress(noise(0x30000, 3))),
                                          (b"gbAx", struct.pack("<II", 2, len(sram)) + zlib.compress(sram))]), "same")
    out["mgba_ss_raw_extdata"] = ("ss1", mgba_extdata_state(sram), "same")
    out["retroarch_rzip_state"] = ("state", rzip(noise(0x20000, 4) + base + noise(0x20000, 5)), "same")
    out["zip_deflate"] = ("zip", zipped("kh_com.sav", base, zipfile.ZIP_DEFLATED), "same")
    out["zip_stored"] = ("zip", zipped("Kingdom Hearts - Chain of Memories.sav", sram, zipfile.ZIP_STORED), "same")
    out["zip_of_gzip_state"] = ("zip", zipped("state.sgm", gzip.compress(noise(0x1000, 6) + base), zipfile.ZIP_DEFLATED),
                                "same")
    out["dsv_desmume_footer"] = ("dsv", dsv(sram), "same")
    out["agbsave_3ds_512_header"] = ("sav", b"AGBSAVE\0".ljust(512, b"\0") + sram, "same")
    out["nocash_raw_setting"] = ("sav", nocash(sram), "same")
    out["unknown_header_garbage"] = ("bin", noise(777, 7) + base + noise(333, 8), "same")
    out["unknown_container_with_zlib_part"] = ("bin", b"MYEMUSAV" + noise(100, 9) + zlib.compress(b"\0" * 5000 + base),
                                               "same")
    out["us_cartridge_raw"] = ("sav", to_region(base, 1), "region")
    out["jp_cartridge_sram_32k"] = ("sav", to_region(base, 2)[:0x8000], "region")
    out["us_cartridge_sharkport"] = ("sps", sharkport(to_region(base, 1)[:0x8000]), "region")
    two = bytearray(base)
    two_other = bytearray(base)
    two_other[0x2F20 + 0x100] ^= 1  # a different image: ambiguous, never guessed
    seal(two_other, 0x2F20, 0xF14)
    out["two_different_saves_ambiguous"] = ("sav", bytes(two) + bytes(two_other), "fail")
    out["nocash_compressed"] = ("sav", nocash(b"\x01\x02\x03" * 100, True), "fail")
    out["empty_flash"] = ("sav", b"\xFF" * 0x10000, "fail")
    out["not_a_save"] = ("bin", noise(65536, 10), "fail")
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--base", type=Path, required=True)
    ap.add_argument("--out", type=Path, required=True)
    args = ap.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    for name, (ext, data, expect) in build(args.base.read_bytes()).items():
        (args.out / f"{name}.{ext}").write_bytes(data)
        print(f"{name}.{ext}\t{len(data)}\t{expect}")


if __name__ == "__main__":
    main()
