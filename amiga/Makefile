# fnweather for the Amiga (Workbench 1.3+) over FujiNet NIO.
#
# Needs bebbo's m68k-amigaos-gcc and a built fujinet-nio-workspace:
#   make NIO_WORKSPACE=/path/to/fujinet-nio-workspace
# Only fujinet-nio-lib's Amiga library is linked (built on demand below);
# at run time fnweather talks to FujiNet through fujinet-nio.device.
#
#   make          build/fnweather + build/fnweather.info (Workbench tool icon)
#                 (nix13 runtime: Kickstart 1.3 and later)
#   make test     host-side tests of the JSON, time and URL code

NIO_WORKSPACE ?= $(HOME)/code/FujiBerryNIO/workspace
NIO_LIB       ?= $(NIO_WORKSPACE)/repos/fujinet-nio-lib

CC      := m68k-amigaos-gcc
HOST_CC ?= cc
# Same flags as the NIO core apps' WB1.3 build (nio-core-apps, AMIGA_CRT=nix13).
CFLAGS  := -std=c99 -Wall -Wextra -O2 -mcpu=68000 -msoft-float -mcrt=nix13 \
           -D__AMIGA__ -I$(NIO_LIB)/include
LDFLAGS := -mcpu=68000 -msoft-float -mcrt=nix13

SRCS := $(wildcard src/*.c)
OBJS := $(SRCS:src/%.c=build/obj/%.o)
NIO_LIB_FILE := $(NIO_LIB)/build/fujinet-nio-amiga.a

all: build/fnweather build/fnweather.info

build/fnweather.info: support/mkicon.py
	@mkdir -p build
	python3 support/mkicon.py $@ >/dev/null

build/fnweather: $(OBJS) $(NIO_LIB_FILE)
	$(CC) -o $@ $(OBJS) $(NIO_LIB_FILE) $(LDFLAGS) -lamiga

build/obj/%.o: src/%.c src/*.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(NIO_LIB_FILE):
	$(MAKE) -C $(NIO_LIB) amiga

test: build/host-test
	./build/host-test

build/host-test: tests/host_test.c src/json.c src/timeutil.c src/meteo.c src/*.h
	@mkdir -p build
	$(HOST_CC) -std=c99 -Wall -Wextra -Werror -Isrc -o $@ tests/host_test.c \
	    src/json.c src/timeutil.c src/meteo.c

clean:
	rm -rf build

.PHONY: all test clean
