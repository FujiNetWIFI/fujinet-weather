# fujinet-weather
Weather application for FujiNet on Atari, ADAM, Apple II, C64, CoCo, MS-DOS, PMD 85, Amiga and VIC-20.

# Building

`make <platform>` builds one platform into `r2r/<platform>/`; `make all` builds every platform in `PLATFORMS`. fujinet-lib is fetched automatically.

### Atari
- Mad-Pascal: https://mads.atari8.info/doc/en/index.html
- `make atari`

### ADAM
- z88dk: https://github.com/z88dk/z88dk/wiki
- `make adam`

### Apple II
- cc65:
  - git clone https://github.com/cc65/cc65.git
  - make clean && make PREFIX=/usr/local && sudo make install PREFIX=/usr/local
- `make apple2`

### C64
- cc65 (see Apple II)
- `make c64`

### CoCo
- CMOC and lwtools: http://perso.b2b2c.ca/~sarrazip/dev/cmoc.html, http://www.lwtools.ca/
- `make coco` (CoCo 1/2), `make coco3`, or `make coco-dist` for one disk with both and a loader that picks by model.
- The CoCo 3 icon header is checked in, so the build does not require the source PNGs. To regenerate it, run `make coco3-icons` (needs the Pillow Python package) with `MM_ICONS_DIR` set to the directory containing the Meteomatics `wsymbol_*.png` files.

### MS-DOS
- Open Watcom 2
- `./make-exp msdos` (builds against fujinet-lib-experimental, which has the RS-232 support)

### PMD 85
- z88dk (see ADAM)
- `make pmd85`

### Amiga
- Workbench 1.3 and later, over FujiNet NIO; m68k-amigaos-gcc (bebbo) and a
  fujinet-nio-workspace checkout. See [amiga/README.md](amiga/README.md).

### VIC-20
- cc65 (see Apple II)
- Not part of the main build; `make -C vic20-rom`
