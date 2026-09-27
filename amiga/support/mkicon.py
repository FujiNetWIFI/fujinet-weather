#!/usr/bin/env python3
"""Write fnweather.info: a Workbench 1.3 tool icon (sun behind a cloud).

The .info format is the classic DiskObject: magic, a Gadget whose render
is one Image, then the planar image data and the Tool Types.  Everything is
big-endian; pointer fields only need to be non-zero where data follows.

Pens use the Workbench 1.3 palette: 0 blue (background), 1 white,
2 black, 3 orange.  Hires pixels on a 640x200 screen are twice as tall as
wide, so the shapes are drawn as ellipses twice as wide as they are tall.

Usage: mkicon.py <output.info>
"""
import struct
import sys

W, H = 48, 21
BLUE, WHITE, BLACK, ORANGE = 0, 1, 2, 3

TOOL_TYPES = ["UNITS=METRIC", "(PLACE=London)"]
STACK = 8192


def inside(x, y, cx, cy, rx, ry):
    return ((x - cx) / rx) ** 2 + ((y - cy) / ry) ** 2 <= 1.0


def draw():
    img = [[BLUE] * W for _ in range(H)]
    sun = (32.0, 6.5, 9.0, 4.6)
    # Rays: short dashes around the sun, as (x, y) pixel runs.
    rays = [(32, 0), (33, 0), (22, 1), (23, 1), (42, 1), (43, 1),
            (19, 6), (20, 6), (44, 6), (45, 6), (46, 6), (42, 12), (43, 12)]
    cloud_parts = [(13.0, 13.5, 9.0, 4.2), (22.0, 11.0, 9.5, 5.5),
                   (31.0, 14.0, 8.0, 3.8), (17.0, 16.0, 12.0, 3.0)]
    cloud_bottom = 18

    def in_cloud(x, y):
        return y <= cloud_bottom and any(inside(x, y, *c) for c in cloud_parts)

    for y in range(H):
        for x in range(W):
            if inside(x, y, *sun):
                img[y][x] = ORANGE
    for x, y in rays:
        img[y][x] = ORANGE
    for y in range(H):
        for x in range(W):
            if in_cloud(x, y):
                edge = any(not in_cloud(x + dx, y + dy)
                           for dx, dy in ((1, 0), (-1, 0), (0, 1), (0, -1)))
                img[y][x] = BLACK if edge else WHITE
    return img


def planes(img, depth=2):
    words = (W + 15) // 16
    out = bytearray()
    for plane in range(depth):
        for row in img:
            bits = [(pen >> plane) & 1 for pen in row] + [0] * (words * 16 - W)
            for w in range(words):
                value = 0
                for b in bits[w * 16:(w + 1) * 16]:
                    value = (value << 1) | b
                out += struct.pack(">H", value)
    return bytes(out)


def disk_object():
    gadget = struct.pack(
        ">IhhhhHHHIIIiIHI",
        0,            # NextGadget
        0, 0, W, H,   # LeftEdge, TopEdge, Width, Height
        0x0004,       # Flags: GADGIMAGE, highlight by complement
        0x0003,       # Activation: RELVERIFY | GADGIMMEDIATE
        0x0001,       # GadgetType: BOOLGADGET
        1,            # GadgetRender: an Image follows
        0,            # SelectRender
        0,            # GadgetText
        0,            # MutualExclude
        0,            # SpecialInfo
        0,            # GadgetID
        0,            # UserData
    )
    assert len(gadget) == 44
    header = struct.pack(">HH", 0xE310, 1) + gadget + struct.pack(
        ">BBIIiiIIi",
        3, 0,              # do_Type WBTOOL, pad
        0,                 # DefaultTool: none
        1,                 # ToolTypes: present
        -0x80000000,       # CurrentX: NO_ICON_POSITION
        -0x80000000,       # CurrentY
        0,                 # DrawerData (tools have none)
        0,                 # ToolWindow
        STACK,
    )
    assert len(header) == 78
    return header


def image_header(depth=2):
    # LeftEdge, TopEdge, Width, Height, Depth, ImageData, PlanePick, PlaneOnOff, NextImage
    return struct.pack(">hhhhhIBBI", 0, 0, W, H, depth, 1, (1 << depth) - 1, 0, 0)


def tool_types():
    out = struct.pack(">I", (len(TOOL_TYPES) + 1) * 4)
    for entry in TOOL_TYPES:
        data = entry.encode("latin-1") + b"\0"
        out += struct.pack(">I", len(data)) + data
    return out


def main():
    if len(sys.argv) != 2:
        sys.exit(__doc__)
    img = draw()
    blob = disk_object() + image_header() + planes(img) + tool_types()
    with open(sys.argv[1], "wb") as f:
        f.write(blob)
    for row in img:
        print("".join(" #@o"[p] for p in row).rstrip())


if __name__ == "__main__":
    main()
