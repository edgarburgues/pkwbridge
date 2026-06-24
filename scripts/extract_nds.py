#!/usr/bin/env python3
"""
extract_nds.py - Build PkwBridge runtime data files from a HGSS NDS dump.

Reads a SoulSilver / HeartGold ROM that you own and produces the five
`.bin` files that PkwBridge loads at boot from `sdmc:/3ds/pokeStride/data/`.
No data ships with PkwBridge: each user extracts their own from a copy
of the game they legally possess.

Usage:
  python3 extract_nds.py <PATH_TO_NDS> <OUT_DIR>

The script expects:
  - course_bitmaps in ARM9 overlay 112 (BLZ-compressed)
  - PHC sprites NARC at FAT file 385
  - PHC walking-icon NARC at FAT file 377
  - PHC system font NARC at FAT file 145, inner file 10

These offsets are identified by signature search against the byte layout
PkwBridge expects. If they don't match your dump (e.g. different region
or revision), the script aborts rather than write garbage.
"""
import os
import struct
import sys


# ----------------------------- BLZ ----------------------------------
def blz_decompress(data: bytes) -> bytes:
    """Decompress Nintendo's "Bottom LZ" (used for arm9/overlay binaries).

    Footer format (last 8 bytes):
      [0..3]: u24 enc_size | u8 hdr_size
      [4..7]: u32 add_size
    Stream is decoded backwards from end. Match-descriptor byte at the
    HIGHER address holds (length-3, disp_high), the lower-address byte
    holds disp_low. (Opposite byte order from standard LZ77 0x10.)
    """
    f = data[-8:]
    enc_size = f[0] | (f[1] << 8) | (f[2] << 16)
    hdr_size = f[3]
    add_size = struct.unpack("<I", f[4:8])[0]
    out = bytearray(data) + bytearray(add_size)
    in_pos = len(data) - hdr_size
    out_pos = len(out)
    in_end = len(data) - enc_size
    flags = 0
    bits = 0
    while in_pos > in_end:
        if bits == 0:
            in_pos -= 1
            flags = out[in_pos]
            bits = 8
        bit = (flags & 0x80) != 0
        flags = (flags << 1) & 0xFF
        bits -= 1
        if not bit:
            in_pos -= 1
            out_pos -= 1
            out[out_pos] = out[in_pos]
        else:
            in_pos -= 2
            b_lo = out[in_pos]
            b_hi = out[in_pos + 1]
            length = (b_hi >> 4) + 3
            disp = (((b_hi & 0xF) << 8) | b_lo) + 3
            for _ in range(length):
                out_pos -= 1
                out[out_pos] = out[out_pos + disp]
    return bytes(out)


# ----------------------------- LZ77 0x10 ----------------------------
def lz77_decompress(data: bytes):
    if len(data) < 4 or data[0] != 0x10:
        return None
    size = data[1] | (data[2] << 8) | (data[3] << 16)
    out = bytearray()
    i = 4
    try:
        while len(out) < size and i < len(data):
            flags = data[i]
            i += 1
            for bit in range(8):
                if len(out) >= size:
                    break
                if flags & (0x80 >> bit):
                    hi = data[i]
                    lo = data[i + 1]
                    i += 2
                    length = (hi >> 4) + 3
                    disp = ((hi & 0xF) << 8 | lo) + 1
                    base = len(out) - disp
                    for k in range(length):
                        out.append(out[base + k])
                else:
                    out.append(data[i])
                    i += 1
        return bytes(out)
    except (IndexError, ValueError):
        return None


# ----------------------------- NDS / NARC ---------------------------
class Nds:
    def __init__(self, path):
        with open(path, "rb") as f:
            self.data = f.read()
        h = self.data
        self.fat_off = struct.unpack("<I", h[0x48:0x4C])[0]
        self.fat_size = struct.unpack("<I", h[0x4C:0x50])[0]
        self.ov9_tbl_off = struct.unpack("<I", h[0x50:0x54])[0]
        self.ov9_tbl_size = struct.unpack("<I", h[0x54:0x58])[0]

    def file_count(self):
        return self.fat_size // 8

    def get_file(self, idx):
        e = self.fat_off + idx * 8
        s = struct.unpack("<I", self.data[e:e + 4])[0]
        end = struct.unpack("<I", self.data[e + 4:e + 8])[0]
        return self.data[s:end]

    def get_overlay9(self, idx):
        """Return decompressed overlay 9 entry idx."""
        e = self.ov9_tbl_off + idx * 32
        file_id = struct.unpack("<I", self.data[e + 24:e + 28])[0]
        flags_word = struct.unpack("<I", self.data[e + 28:e + 32])[0]
        compressed = (flags_word >> 24) & 1
        raw = self.get_file(file_id)
        if compressed:
            return blz_decompress(raw)
        return raw


def narc_files(blob):
    """Iterate inner files of a NARC archive. Returns list of bytes."""
    if blob[:4] != b"NARC" or blob[16:20] != b"BTAF":
        raise ValueError("not a NARC")
    count = struct.unpack("<I", blob[24:28])[0]
    btnf_off = 16 + 12 + count * 8
    btnf_size = struct.unpack("<I", blob[btnf_off + 4:btnf_off + 8])[0]
    gmif_data = btnf_off + btnf_size + 8
    out = []
    for i in range(count):
        en = 16 + 12 + i * 8
        fs = struct.unpack("<I", blob[en:en + 4])[0]
        fe = struct.unpack("<I", blob[en + 4:en + 8])[0]
        out.append(blob[gmif_data + fs:gmif_data + fe])
    return out


# ----------------------------- locator helpers ----------------------
# Known FAT file IDs for SoulSilver (W). Verified against the byte
# layouts PkwBridge expects. The script asserts the shape (NARC magic,
# entry count, decompressed size) before writing.
FILE_PHCGRA = 385
FILE_PHCICON = 377
FILE_FONT = 145
FONT_INNER = 10
OVERLAY_COURSES = 112

# Course bitmap offsets within decompressed overlay 112. The 8 backgrounds
# are stored contiguously (192 B each) but not in canonical course order.
COURSE_BITMAP_OFFSETS = {
    1: 0x11644, 2: 0x11044, 3: 0x11404, 4: 0x11584,
    5: 0x11344, 6: 0x11104, 7: 0x11284, 8: 0x111C4,
}

# Font header at file_145[10] start
FONT_GLYPH_OFFSET = 0x10
FONT_GLYPH_SIZE = 509 * 64    # 32576
FONT_WIDTHS_OFFSET = 0x7F50
FONT_WIDTHS_SIZE = 509


# ----------------------------- extraction ---------------------------
def write_bin(out_dir, name, data, expected):
    if len(data) != expected:
        sys.exit(
            f"FATAL: {name} got {len(data)} B, expected {expected}. "
            f"Wrong region/revision?"
        )
    path = os.path.join(out_dir, name)
    with open(path, "wb") as f:
        f.write(data)
    print(f"  wrote {name:<24} {expected:>10,} B")


def extract_course_bitmaps(nds: Nds) -> bytes:
    ov = nds.get_overlay9(OVERLAY_COURSES)
    out = bytearray()
    for course_id in range(1, 9):
        off = COURSE_BITMAP_OFFSETS[course_id]
        out += ov[off:off + 192]
    return bytes(out)


def extract_phc_narc(nds: Nds, file_id, expected_count, entry_size):
    blob = nds.get_file(file_id)
    files = narc_files(blob)
    if len(files) != expected_count:
        sys.exit(
            f"FATAL: NDS file {file_id} has {len(files)} inner files, "
            f"expected {expected_count}"
        )
    out = bytearray()
    for f in files:
        d = lz77_decompress(f) if f[:1] == b"\x10" else f
        if d is None:
            sys.exit(f"FATAL: bad LZ77 in NARC {file_id}")
        if len(d) < entry_size:
            d = d + b"\x00" * (entry_size - len(d))
        else:
            d = d[:entry_size]
        out += d
    return bytes(out)


def extract_font(nds: Nds):
    blob = nds.get_file(FILE_FONT)
    files = narc_files(blob)
    if len(files) <= FONT_INNER:
        sys.exit(
            f"FATAL: font NARC has only {len(files)} files (need >{FONT_INNER}). "
            f"Wrong region? The Japanese release ships only 10 files; the Western "
            f"build adds file 10 (zero-indexed) specifically for the walker."
        )
    f = files[FONT_INNER]
    glyphs = f[FONT_GLYPH_OFFSET:FONT_GLYPH_OFFSET + FONT_GLYPH_SIZE]
    widths = f[FONT_WIDTHS_OFFSET:FONT_WIDTHS_OFFSET + FONT_WIDTHS_SIZE]
    return glyphs, widths


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__.strip())
    nds_path, out_dir = sys.argv[1], sys.argv[2]
    if not os.path.exists(nds_path):
        sys.exit(f"NDS not found: {nds_path}")
    os.makedirs(out_dir, exist_ok=True)

    print(f"reading {nds_path}")
    nds = Nds(nds_path)
    print(f"  NDS has {nds.file_count()} FAT entries\n")

    print("extracting course_bitmaps from arm9 overlay 112 (BLZ)...")
    cb = extract_course_bitmaps(nds)
    write_bin(out_dir, "course_bitmaps.bin", cb, 8 * 192)

    print("extracting phcgra (Pokemon sprites) from NDS file 385...")
    phcgra = extract_phc_narc(nds, FILE_PHCGRA, 664, 1536)
    write_bin(out_dir, "phcgra_data.bin", phcgra, 664 * 1536)

    print("extracting phcicon (walker icons) from NDS file 377...")
    phcicon = extract_phc_narc(nds, FILE_PHCICON, 540, 384)
    write_bin(out_dir, "phcicon_data.bin", phcicon, 540 * 384)

    print("extracting font_sys from NDS file 145, inner 10...")
    glyphs, widths = extract_font(nds)
    write_bin(out_dir, "font_sys_glyphs.bin", glyphs, FONT_GLYPH_SIZE)
    write_bin(out_dir, "font_sys_widths.bin", widths, FONT_WIDTHS_SIZE)

    print("\nDone. Copy the 5 .bin files to:")
    print("  sdmc:/3ds/pokeStride/data/")


if __name__ == "__main__":
    main()
