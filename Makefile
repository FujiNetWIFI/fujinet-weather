PRODUCT = weather
PLATFORMS += adam
PLATFORMS += apple2
PLATFORMS += atari-pas
PLATFORMS += c64
PLATFORMS += coco
#PLATFORMS += vic20

# You can run 'make <platform>' to build for a specific platform,
# or 'make <platform>/<target>' for a platform-specific target.
# Example shortcuts:
#   make coco        → build for coco
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
