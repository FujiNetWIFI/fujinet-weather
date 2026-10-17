#!/usr/bin/env python3

import sys
from PIL import Image


PIXELS_PER_BYTE = 6     # 1 byte = 6 pixels + "color" attributes
RLE_ESC = 0xE1          # RLE escape byte: RLE_ESC, count, value
RLE_MIN_RUN = 4         # shorter runs are stored as they are (except RLE_ESC itself)


class PMD_Bitmap:
    def __init__(self, width, height, label=None):
        self.label = label
        self.width = width      # in bytes, i.e. 1 byte = 6 pixels + "color" attributes
        self.height = height    # in pixels
        self.data = None

    def write(self, fout):
        rle_data = rle_encode(self.data) if self.data else b''
        if self.label:
            print(f"// {self.width} x {self.height} bytes, RLE {len(rle_data)} bytes", file=fout)
            print(f"unsigned char {self.label}[] = {{", file=fout)
        print(f"    {self.width}, {self.height}, // width in bytes, height in pixels", file=fout)
        if rle_data:
            fout.write(to_hex(rle_data, 12))
        if self.label:
            print("};", file=fout)
        return len(rle_data)

def rle_encode(data):
    """compress runs of identical bytes to RLE_ESC, count, value"""
    result = bytearray()
    i = 0
    while i < len(data):
        v = data[i]
        n = 1
        while i + n < len(data) and data[i + n] == v and n < 255:
            n += 1
        if n >= RLE_MIN_RUN or v == RLE_ESC:
            result.extend((RLE_ESC, n, v))
        else:
            result.extend([v] * n)
        i += n
    return result

def to_hex(data, chunk_size):
    """format data as C array initializer, chunk_size bytes per line"""
    lines = []
    for i in range(0, len(data), chunk_size):
        chunk = data[i:i+chunk_size]
        lines.append("    " + ", ".join(f"0x{n:02X}" for n in chunk) + ",\n")
    return ''.join(lines)

def get_byte_from_pixels(pix, x, y, rgbindex=1):
    """convert 6 pixels in a row starting at [x, y] to a PMD 85 video RAM byte"""
    n = 0
    bright = []
    for i in range(PIXELS_PER_BYTE):
        p = pix[x+i, y][rgbindex]
        if p >= 64:             # skip black pixels
            n |= 1 << i         # on PMD 85 bit 0 is leftmost on screen
            bright.append(p >= 192)  # True for full bright, False for reduced bright
    ca = 0x40 if bright and sum(bright)/len(bright) < 0.5 else 0 # brightness/color bit
    return ca | n

def get_bitmap_from_pixels(pix, x, y, w, h, label=None):
    """convert w x h pixels area starting at [x, y] to PMD_Bitmap, data stored column by column"""
    if w % PIXELS_PER_BYTE:
        rounded = (w + PIXELS_PER_BYTE - 1) // PIXELS_PER_BYTE * PIXELS_PER_BYTE
        print(f"Warning: {label or 'bitmap'} width {w} is not multiple of {PIXELS_PER_BYTE}, rounded up to {rounded}")
        w = rounded
    if w // PIXELS_PER_BYTE > 255 or h > 255:
        print(f"Warning: {label or 'bitmap'} size {w} x {h} px does not fit into bitmap header")

    bitmap = PMD_Bitmap(w // PIXELS_PER_BYTE, h, label)
    bitmap.data = bytearray()
    for col in range(x, x + w, PIXELS_PER_BYTE):
        for row in range(y, y + h):
            bitmap.data.append(get_byte_from_pixels(pix, col, row, 1)) # 1 = green pixels
    return bitmap

def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} input_file output_file")
        return 1

    in_name, out_name = sys.argv[1], sys.argv[2]

    im = Image.open(in_name).convert('RGB')
    pix = im.load()

    print("Image:     ", im.width, "x", im.height, "px")

    bitmaps = []
    # weather icons, 4x3 grid (last cell is blank)
    for i in range(11):
        bitmaps.append(get_bitmap_from_pixels(pix, 72*(i % 4), 53*(i // 4), 72, 53, f"bmp_weather{i}"))
    for d in range(10):
        bitmaps.append(get_bitmap_from_pixels(pix, 18*d, 162, 18, 54, f"bmp_digit{d}"))
    bitmaps.append(get_bitmap_from_pixels(pix, 180, 162+34, 12, 20, "bmp_dot"))
    bitmaps.append(get_bitmap_from_pixels(pix, 180, 162, 12, 22, "bmp_degree"))
    bitmaps.append(get_bitmap_from_pixels(pix, 180, 162+22, 12, 12, "bmp_minus"))
    bitmaps.append(get_bitmap_from_pixels(pix, 198, 162, 18, 54, "bmp_fahrenheit"))
    bitmaps.append(get_bitmap_from_pixels(pix, 216, 162, 18, 54, "bmp_celsius"))

    raw_size = rle_size = 0
    with open(out_name, "w") as fout:
        for bitmap in bitmaps:
            raw_size += len(bitmap.data)
            rle_size += bitmap.write(fout)
    print("Data:      ", raw_size, "bytes, RLE", rle_size, "bytes")

if __name__ == "__main__":
    raise SystemExit(main())
