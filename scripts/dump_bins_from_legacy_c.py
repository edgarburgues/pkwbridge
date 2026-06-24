#!/usr/bin/env python3
"""
dump_bins_from_legacy_c.py — generate the runtime .bin files that
PkwBridge loads at boot from sdmc:/3ds/pokeStride/data/.

Takes as input the legacy C source files that previously embedded the
bulk asset data (font_sys.c, course_bitmaps.c, phcgra_data.c,
phcicon_data.c) and writes plain binary blobs:

  course_bitmaps.bin    8 * 192 = 1536 B
  font_sys_glyphs.bin   509 * 64 = 32576 B
  font_sys_widths.bin   509 B
  phcgra_data.bin       664 * 1536 = 1,019,904 B
  phcicon_data.bin      540 * 384 = 207,360 B

This is a transitional tool used during the public-release refactor.
A proper extractor that reads the data from the user's own HGSS NDS
dump (rather than from these C files) is TODO — see
scripts/extract_nds.py.

Usage:
  python3 dump_bins_from_legacy_c.py <SRC_DIR> <OUT_DIR>

where SRC_DIR contains the original .c files (e.g.
08-copia/02-tools/pkwbridge/source/) and OUT_DIR is where the .bin
files will be written.
"""
import os
import re
import sys


def parse_hex_bytes(text):
    """All 0xNN bytes in order. Comments / structure ignored."""
    return bytes(int(m, 16) for m in re.findall(r"0x([0-9A-Fa-f]{2})", text))


def write_bin(out_dir, name, data, expected):
    path = os.path.join(out_dir, name)
    if len(data) < expected:
        sys.exit(f"FATAL: {name} got {len(data)} B, expected {expected}")
    with open(path, "wb") as f:
        f.write(data[:expected])
    print(f"  wrote {name:<24} {expected:>10,} B")


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__.strip())
    src_dir, out_dir = sys.argv[1], sys.argv[2]
    os.makedirs(out_dir, exist_ok=True)

    print(f"reading from {src_dir}")
    print(f"writing to   {out_dir}\n")

    # course_bitmaps.c: 8 * 192 B, all bytes after the array opening
    with open(os.path.join(src_dir, "course_bitmaps.c")) as f:
        cb_bytes = parse_hex_bytes(f.read())
    write_bin(out_dir, "course_bitmaps.bin", cb_bytes, 8 * 192)

    # font_sys.c: widths (509 B) then glyphs (509*64 B). Both decimal AND
    # hex appear in the file, so split by the array name. Each array is
    # framed by `= {` ... `};`.
    with open(os.path.join(src_dir, "font_sys.c")) as f:
        font_text = f.read()
    # The widths come first in our file. Find the two array bodies.
    matches = list(re.finditer(r"=\s*\{(.*?)\};", font_text, re.DOTALL))
    if len(matches) < 2:
        sys.exit("FATAL: font_sys.c lacks the 2 expected array bodies")
    widths_body = matches[0].group(1)
    glyphs_body = matches[1].group(1)
    # widths use decimal: parse integers
    widths_ints = [int(x) for x in re.findall(r"\b(\d+)\b", widths_body)]
    widths_bin = bytes(widths_ints[:509])
    write_bin(out_dir, "font_sys_widths.bin", widths_bin, 509)
    # glyphs are hex bytes
    glyphs_bin = parse_hex_bytes(glyphs_body)
    write_bin(out_dir, "font_sys_glyphs.bin", glyphs_bin, 509 * 64)

    # phcgra_data.c: 664 * 1536 hex bytes
    with open(os.path.join(src_dir, "phcgra_data.c")) as f:
        phcgra_bytes = parse_hex_bytes(f.read())
    write_bin(out_dir, "phcgra_data.bin", phcgra_bytes, 664 * 1536)

    # phcicon_data.c: 540 * 384 hex bytes BEFORE the species_to_index
    # lookup table (which uses decimal ints, would skew the count).
    with open(os.path.join(src_dir, "phcicon_data.c")) as f:
        phcicon_text = f.read()
    # Split at the species_to_index table — only take the part before it.
    cut = phcicon_text.find("phcicon_species_to_index")
    if cut < 0:
        sys.exit("FATAL: phcicon_data.c lacks species_to_index sentinel")
    phcicon_bytes = parse_hex_bytes(phcicon_text[:cut])
    write_bin(out_dir, "phcicon_data.bin", phcicon_bytes, 540 * 384)

    print("\nAll .bin files written. Copy them to your SD at")
    print("  sdmc:/3ds/pokeStride/data/")


if __name__ == "__main__":
    main()
