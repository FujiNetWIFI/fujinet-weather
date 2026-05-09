PRODUCT = weather
PRODUCT_UPPER = WEATHER
PLATFORMS += adam
PLATFORMS += apple2
PLATFORMS += atari-pas
PLATFORMS += c64
PLATFORMS += coco
#PLATFORMS += vic20

# You can run 'make <platform>' to build for a specific platform,
# or 'make <platform>/<target>' for a platform-specific target.
# Example shortcuts:
#   make coco        → build for coco (CoCo 1/2)
#   make coco3       → build for CoCo 3
#   make coco-dist   → combined disk with loader + both CoCo binaries
#   make apple2/disk → build the 'disk' target for apple2

# SRC_DIRS may use the literal %PLATFORM% token.
# It expands to the chosen PLATFORM plus any of its combos.
SRC_DIRS = %PLATFORM%/src %PLATFORM%

# FUJINET_LIB can be
# - a version number such as 4.7.6 
# - a directory which contains the libs for each platform
# - a zip file with an archived fujinet-lib
# - a URL to a git repo
# - empty which will use whatever is the latest
# - undefined, no fujinet-lib will be used
FUJINET_LIB = 4.10.0

# HIRESTXT_LIB can be
# - a version number such as 0.5.0.2
# - a directory which contains the built library
# - a URL to a git repo
# - empty which will use whatever is the latest
# - undefined, no hirestxt-mod will be used
# Only used for coco/dragon builds.
#HIRESTXT_LIB =

# Define extra dirs ("combos") that expand with a platform.
# Format: platform+=combo1,combo2
PLATFORM_COMBOS = \
  vic20+=vic20-rom \
  atari-pas+=atari

CFLAGS_EXTRA_COCO = -Wno-const

ifeq ($(MAKE_COCO3),COCO3)
	CFLAGS_EXTRA_COCO += -DCOCO3
	LDFLAGS_EXTRA_COCO = --org=1000 --limit=7E00
endif

ifeq ($(PLATFORM),apple2)
	CFLAGS_EXTRA_APPLE2  += -DAPPLE2
	LDFLAGS_EXTRA_APPLE2 += --start-addr 0x4000 --ld-args -D,__HIMEM__=0xBF00 -m $(BUILD_EXEC).map
endif

# Support 'make coco3'
coco3:
	$(MAKE) coco MAKE_COCO3=COCO3

# Generated CoCo 3 charset header (4bpp), built from the MSDOS port's font.
COCO3_CHARSET_H = coco/src/charset_coco3.h

$(COCO3_CHARSET_H): coco/support/cvt_msdos_font.py msdos/src/ascii.c
	python3 coco/support/cvt_msdos_font.py

# Generated CoCo 3 weather icons header (4bpp inline 32x32 icons, 4608 bytes).
COCO3_ICONS_H = coco/src/charset_coco3_icons.h

$(COCO3_ICONS_H): coco/support/cvt_icons.py
	python3 coco/support/cvt_icons.py

# Only regenerate during COCO3 builds.
ifeq ($(MAKE_COCO3),COCO3)
coco/r2r:: $(COCO3_CHARSET_H) $(COCO3_ICONS_H)
endif

atari: atari-pas

include mekkogx/toplevel-rules.mk

# If you need to add extra platform-specific steps, do it below:
#   coco/r2r:: coco/custom-step1
#   coco/r2r:: coco/custom-step2
# or
#   apple2/disk: apple2/custom-step1 apple2/custom-step2

ATARI_DISK = r2r/atari-pas/weather.atr
ATARI_DISK_DIR = _cache/atari-pas/disk

atari-pas/r2r:: $(ATARI_DISK)

$(ATARI_DISK): r2r/atari-pas/weather.xex $(wildcard atari/disk/*)
	rm -rf $(ATARI_DISK_DIR)
	mkdir -p $(ATARI_DISK_DIR)
	cp atari/disk/* $(ATARI_DISK_DIR)/
	cp r2r/atari-pas/weather.xex $(ATARI_DISK_DIR)/AUTORUN.SYS
	dir2atr -S -b Dos25 $@ $(ATARI_DISK_DIR)

clean::
	rm -f atari/tmp*

# Variables for coco-dist
R2R_PRODUCT = r2r/coco/$(PRODUCT)
COCO_DISK = $(R2R_PRODUCT).dsk

# Combined CoCo 1/2 + CoCo 3 disk, with loader that auto-detects model.
coco-dist:
	$(MAKE) clean
	rm -rf build
	$(MAKE) coco
	mv $(R2R_PRODUCT).bin $(R2R_PRODUCT)1.bin

	rm -rf build
	$(MAKE) coco3
	mv $(R2R_PRODUCT).bin $(R2R_PRODUCT)3.bin

	cmoc -DPRODUCT=\"$(PRODUCT_UPPER)\" -o $(R2R_PRODUCT).bin support/coco/loader.c

	$(RM) $(COCO_DISK)
	decb dskini $(COCO_DISK)
	echo RUNM\"$(PRODUCT_UPPER)\" > build/coco/autoexec.bas
	decb copy -t -0 build/coco/autoexec.bas $(COCO_DISK),AUTOEXEC.BAS
	decb copy -b -2 $(R2R_PRODUCT).bin $(COCO_DISK),$(PRODUCT_UPPER).BIN
	decb copy -b -2 $(R2R_PRODUCT)1.bin $(COCO_DISK),$(PRODUCT_UPPER)1.BIN
	decb copy -b -2 $(R2R_PRODUCT)3.bin $(COCO_DISK),$(PRODUCT_UPPER)3.BIN
