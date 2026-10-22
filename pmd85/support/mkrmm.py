#!/usr/bin/env python3
"""Create an auto-booting PMD 85 ROM module image (.rmm) from a z88dk binary.

The PMD 85 monitor reads the start of the ROM module and, if the first byte
is 0xCD (CALL), runs it. The boot code placed there calls the monitor's ROM
transfer routine, which copies the module to RAM and returns right behind
its arguments:

    0000  CD 00 8C    call 8C00h     ; monitor ROM transfer routine
    0003  ss ss       dw source      ; address in ROM module
    0005  cc cc       dw count       ; transfer size, see transfer_count()
    0007  dd dd       dw destination ; address in RAM
    0009  F3 C3 xx xx di / jp program ; first 4 bytes of the z88dk binary

The z88dk binary starts with "di / jp program" at 0000 followed by the RST
vectors. The boot code takes bytes 0..8, the original 4 bytes move to 9..12
and the rest of the binary (from offset 13) stays where it is.
"""

import sys

ROM_TRANSFER = 0x8C00
BOOT_SIZE = 9           # call + 3 words
ENTRY_SIZE = 4          # di / jp program
FIRST_MODEL_FIX = 0x100 # see transfer_count()
ROM_MODULE_SIZE = 0x8000 # 32 KB
RAM_TOP = 0x8000        # monitor ROM starts here, module is loaded to 0000h


def transfer_count(size):
    """Transfer size argument for the monitor's ROM transfer routine.

    The routine expects the number of bytes to transfer minus 1. The monitor
    of the first PMD 85 model has a bug and transfers 256 bytes less, so 256
    is added: the first model then copies exactly `size` bytes, later models
    copy 256 bytes more.
    """
    return size - 1 + FIRST_MODEL_FIX


def word(value):
    return bytes((value & 0xFF, value >> 8))


def main():
    if len(sys.argv) != 3:
        sys.exit(f"usage: {sys.argv[0]} input.bin output.rmm")
    src, dst = sys.argv[1], sys.argv[2]

    with open(src, "rb") as f:
        data = f.read()

    if len(data) <= BOOT_SIZE + ENTRY_SIZE:
        sys.exit(f"{src}: too short ({len(data)} bytes)")
    if len(data) > ROM_MODULE_SIZE:
        sys.exit(f"{src}: {len(data)} bytes do not fit into a "
                 f"{ROM_MODULE_SIZE // 1024} KB ROM module")
    if len(data) + FIRST_MODEL_FIX > RAM_TOP:
        # not a problem in general, writes to the ROM area are ignored
        print(f"warning: {src}: later models copy {FIRST_MODEL_FIX} bytes more, "
              f"the transfer will run into the ROM area at {RAM_TOP:#06x}",
              file=sys.stderr)
    if data[0] != 0xF3 or data[1] != 0xC3:
        sys.exit(f"{src}: does not start with 'di / jp nnnn', not a z88dk pmd85 binary?")
    dropped = data[ENTRY_SIZE:BOOT_SIZE + ENTRY_SIZE]
    if dropped.strip(b"\x00\xc9"):
        sys.exit(f"{src}: bytes {ENTRY_SIZE}..{BOOT_SIZE + ENTRY_SIZE - 1} are in use "
                 f"({dropped.hex(' ')}), the boot code would overwrite them")

    count = transfer_count(len(data))
    boot = b"\xcd" + word(ROM_TRANSFER) + word(0x0000) + word(count) + word(0x0000)
    assert len(boot) == BOOT_SIZE

    with open(dst, "wb") as f:
        f.write(boot + data[:ENTRY_SIZE] + data[BOOT_SIZE + ENTRY_SIZE:])

    print(f"{dst}: {len(data)} bytes, transfer size {count:#06x}")


if __name__ == "__main__":
    main()
