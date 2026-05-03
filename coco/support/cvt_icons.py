#!/usr/bin/env python3
"""
Convert meteomatics weather PNGs to CoCo 3 inline icons.

Format: 32x32 px at 2bpp (4 colors per icon) with a per-icon 4-byte
palette table.  Each icon picks the 4 best CoCo 3 16-color palette
indices for its image, so overall the screen still uses 16 colors --
sun icons get yellow, rain icons get blue, etc. -- but each individual
icon stays at 4 colors so its pixel data is half the size of 4bpp.

Source:  /home/rich/Downloads/mm_api_symbols/wsymbol_*.png  (64x64 RGBA)
Output:  coco/src/charset_coco3_icons.h

Layout: icon = 16 tiles (4x4 grid) of 8 rows x 2 bytes/row at 2bpp.
        16 * 16 = 256 bytes pixel data + 4 bytes palette = 260 bytes/icon.
        9 icons * 260 = 2340 bytes inline.
"""
import os
from PIL import Image

HERE = os.path.dirname(os.path.abspath(__file__))
MM_DIR = "/home/rich/Downloads/mm_api_symbols"
HEADER_DST = os.path.abspath(os.path.join(HERE, "..", "src", "charset_coco3_icons.h"))

ICONS = [
    ("wsymbol_0001_sunny.png",                 "clear"),
    ("wsymbol_0002_sunny_intervals.png",       "few_clouds"),
    ("wsymbol_0003_white_cloud.png",           "scattered_clouds"),
    ("wsymbol_0043_mostly_cloudy.png",         "broken_clouds"),
    ("wsymbol_0009_light_rain_showers.png",    "shower_rain"),
    ("wsymbol_0018_cloudy_with_heavy_rain.png","rain"),
    ("wsymbol_0024_thunderstorms.png",         "thunderstorm"),
    ("wsymbol_0011_light_snow_showers.png",    "snow"),
    ("wsymbol_0007_fog.png",                   "fog"),
]

# CoCo 3 GIME RGB6 palette -> 8-bit RGB (matches gfx.c).
GIME_PAL = [
     0,   #  0 black
     7,   #  1 dark gray
    56,   #  2 light gray
    63,   #  3 white
    28,   #  4 teal
    11,   #  5 light blue
     1,   #  6 dark blue
     9,   #  7 sea blue
    25,   #  8 foam
    27,   #  9 light foam
     4,   # 10 dark red
    36,   # 11 red
    38,   # 12 red orange
    52,   # 13 orange
    54,   # 14 yellow
    63,   # 15 white (duplicate)
]

def gime_to_rgb(g):
    r = ((g >> 5) & 1) * 2 + ((g >> 2) & 1)
    gg = ((g >> 4) & 1) * 2 + ((g >> 1) & 1)
    b = ((g >> 3) & 1) * 2 + ((g >> 0) & 1)
    return (r * 85, gg * 85, b * 85)

PAL_RGB = [gime_to_rgb(c) for c in GIME_PAL]

def color_dist(rgb1, rgb2):
    dr = rgb1[0] - rgb2[0]
    dg = rgb1[1] - rgb2[1]
    db = rgb1[2] - rgb2[2]
    return dr*dr*2 + dg*dg*4 + db*db*3

def nearest_pal_idx(rgb):
    """Closest CoCo 3 palette index to the given RGB."""
    best = 0
    best_d = 1 << 30
    for i, p in enumerate(PAL_RGB):
        d = color_dist(rgb, p)
        if d < best_d:
            best_d = d
            best = i
    return best

ICON_SIZE   = 32
TILES_SIDE  = 4
TILES_TOTAL = TILES_SIDE * TILES_SIDE
TILE_BYTES  = 16   # 8 rows x 2 bytes/row at 2bpp
ICON_PIXEL_BYTES = TILES_TOTAL * TILE_BYTES   # 256

def pick_icon_palette(img):
    """Choose 4 CoCo 3 palette indices that best represent this icon.
    Slot 0 is always palette 0 (black) for transparent pixels.  Slots
    1..3 are picked by quantizing the icon's opaque pixels to 3 colors,
    each rounded to the nearest CoCo 3 palette entry."""
    # Build a list of opaque pixels' nearest CoCo 3 palette indices.
    counts = [0] * 16
    px = img.load()
    for y in range(ICON_SIZE):
        for x in range(ICON_SIZE):
            r, g, b, a = px[x, y]
            if a < 64:
                continue
            counts[nearest_pal_idx((r, g, b))] += 1
    # Pick the 3 most common non-zero palette indices for slots 1..3.
    ranked = sorted(range(16), key=lambda i: counts[i], reverse=True)
    chosen = []
    for idx in ranked:
        if idx == 0:
            continue
        if counts[idx] == 0:
            break
        chosen.append(idx)
        if len(chosen) == 3:
            break
    while len(chosen) < 3:
        chosen.append(3)   # pad with white if image had <3 distinct colors
    return [0, chosen[0], chosen[1], chosen[2]]

def quantize_to_icon(img, icon_pal):
    """Map every pixel of the icon to its 2bpp slot (0..3) by picking
    whichever of the 4 chosen CoCo 3 palette colors is closest."""
    pal_rgb = [PAL_RGB[i] for i in icon_pal]
    px = img.load()
    grid = [[0]*ICON_SIZE for _ in range(ICON_SIZE)]
    for y in range(ICON_SIZE):
        for x in range(ICON_SIZE):
            r, g, b, a = px[x, y]
            if a < 64:
                grid[y][x] = 0
                continue
            best = 0
            best_d = 1 << 30
            for i, pr in enumerate(pal_rgb):
                d = color_dist((r, g, b), pr)
                if d < best_d:
                    best_d = d
                    best = i
            grid[y][x] = best
    return grid

def grid_to_tiles(grid):
    tiles = []
    for ty in range(TILES_SIDE):
        for tx in range(TILES_SIDE):
            tile = []
            for r in range(8):
                row = grid[ty*8 + r]
                start = tx * 8
                p = row[start:start+8]
                # 8 pixels at 2bpp -> 2 bytes (4 px each, MSB first).
                b0 = (p[0] << 6) | (p[1] << 4) | (p[2] << 2) | p[3]
                b1 = (p[4] << 6) | (p[5] << 4) | (p[6] << 2) | p[7]
                tile.append(b0)
                tile.append(b1)
            tiles.append(tile)
    return tiles

def convert_icon(path):
    img = Image.open(path).convert("RGBA").resize(
        (ICON_SIZE, ICON_SIZE), Image.LANCZOS)
    icon_pal = pick_icon_palette(img)
    grid     = quantize_to_icon(img, icon_pal)
    tiles    = grid_to_tiles(grid)
    return icon_pal, tiles

def main():
    icons = [convert_icon(os.path.join(MM_DIR, fname))
             for fname, _ in ICONS]

    with open(HEADER_DST, "w") as f:
        f.write("/* Generated by coco/support/cvt_icons.py - do not edit. */\n")
        f.write("/* 32x32 px icons in 2bpp + per-icon 4-color palette. */\n\n")
        f.write("#ifndef CHARSET_COCO3_ICONS_H\n#define CHARSET_COCO3_ICONS_H\n\n")
        f.write(f"#define ICONS_COCO3_N      {len(ICONS)}\n")
        f.write(f"#define ICON_TILES_SIDE    {TILES_SIDE}\n")
        f.write(f"#define ICON_TILES_TOTAL   {TILES_TOTAL}\n")
        f.write(f"#define ICON_TILE_BYTES    {TILE_BYTES}\n\n")
        # Per-icon palettes
        f.write("/* 4-color CoCo 3 palette indices for each icon (slot 0 = bg). */\n")
        f.write(f"static const unsigned char icon_palette[{len(ICONS)}][4] = {{\n")
        for idx, (icon_pal, _tiles) in enumerate(icons):
            _, name = ICONS[idx]
            pal_str = ", ".join(f"{p:2d}" for p in icon_pal)
            f.write(f"    {{ {pal_str} }},  /* {idx}: {name} */\n")
        f.write("};\n\n")
        # Pixel data
        f.write(f"static const unsigned char charset_coco3_icons"
                f"[{len(ICONS)}][{TILES_TOTAL}][{TILE_BYTES}] = {{\n")
        for idx, (_pal, tiles) in enumerate(icons):
            _, name = ICONS[idx]
            f.write(f"    /* {idx}: {name} */\n    {{\n")
            for tnum, tile in enumerate(tiles):
                hex_bytes = ", ".join(f"0x{b:02X}" for b in tile)
                f.write(f"        /* t{tnum:02d} */ {{ {hex_bytes} }},\n")
            f.write("    },\n")
        f.write("};\n\n#endif\n")

    print(f"wrote {HEADER_DST} "
          f"({len(ICONS)} icons, "
          f"{len(ICONS)*ICON_PIXEL_BYTES + len(ICONS)*4} bytes inline)")

if __name__ == "__main__":
    main()
