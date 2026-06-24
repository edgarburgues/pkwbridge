# PkwBridge data extractors

PkwBridge loads its bulk asset data (font glyphs, course backgrounds,
Pokemon sprites, walker icons) at boot from
`sdmc:/3ds/pokeStride/data/`. The files are NOT distributed with the
homebrew because they are extracted from copyrighted material. Each
user generates them locally from a copy of the game they legally own.

## Required `.bin` files

| File | Size | Source |
|---|---|---|
| `course_bitmaps.bin` | 1,536 B | HGSS arm9 overlay 112 (BLZ) — 8 × 192 B route backgrounds |
| `font_sys_glyphs.bin` | 32,576 B | HGSS NARC at NDS file 145, inner 10 (509 × 64 B PHC font) |
| `font_sys_widths.bin` | 509 B | Same NARC inner file (per-glyph advance widths) |
| `phcgra_data.bin` | 1,019,904 B | HGSS NARC at NDS file 385 (664 × 1536 B Pokemon sprites) |
| `phcicon_data.bin` | 207,360 B | HGSS NARC at NDS file 377 (540 × 384 B walker icons) |

## Without these files

The app still works — every write site is guarded by an
`app_data_*_loaded()` check. When data is missing, PkwBridge does not
overwrite the corresponding region of the pweep, so the walker keeps
its previous visual content (the template's background, sprites,
fonts). You lose visual updates on route change but Send / Claim /
Return still operate.

## `extract_nds.py` — extract from your HGSS dump

```
python3 scripts/extract_nds.py /path/to/SoulSilver.nds /path/to/output_dir/
```

Reads a SoulSilver / HeartGold NDS ROM and writes all five `.bin`
files. Includes:

  - NDS FAT/overlay parser
  - BLZ decompressor (for arm9 overlays)
  - LZ77 0x10 decompressor (for NARC inner files)
  - NARC archive parser

Tested against SoulSilver (Western). Other regions (HeartGold, JP, EU)
may use different FAT file IDs — the script aborts with a clear error
if the layout doesn't match instead of writing garbage.

## `dump_bins_from_legacy_c.py`

Reads asset data embedded in C source files and dumps it back out as
`.bin`. This is a niche helper; most users should use `extract_nds.py`
instead. Run:

```
python3 scripts/dump_bins_from_legacy_c.py <SRC_DIR> <OUT_DIR>
```
