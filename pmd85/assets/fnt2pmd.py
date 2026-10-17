#!/usr/bin/env python3
"""Transpose groups of 8 input bytes into 8 output bytes, then optionally
reorder the resulting chunks before writing them to disk.

For each chunk of 8 input bytes:
  - output byte 0 contains bit 7 from all 8 input bytes
  - output byte 1 contains bit 6 from all 8 input bytes
  - ...
  - output byte 7 contains bit 0 from all 8 input bytes

Bits from the first input byte are placed into the highest bit position of the
corresponding output byte; bits from the 8th input byte are placed into the
lowest bit position.

The chunk ordering is intentionally buffered so that the output can be written
at a different chunk index than the input chunk index. This is useful when an
ANTIC-style input order must be converted to ATASCII-style output order.
"""

import sys


def transpose_chunk(chunk: bytes) -> bytes:
    if len(chunk) < 8:
        chunk += b"\x00" * (8 - len(chunk))
    chunk = chunk[:8]

    out = bytearray(8)

    # output byte index follows the source bit position: byte 0 = bit 7,
    # byte 1 = bit 6, ..., byte 7 = bit 0.
    for src_bit in range(8):
        out_byte_index = 7 - src_bit
        for src_index in range(8):
            bit = (chunk[src_index] >> (7 - src_bit)) & 0x01
            if bit:
                out[out_byte_index] |= 1 << (7 - src_index)

    return bytes(reversed(out))


def antic_to_atascii_chunk_index(index: int, total_chunks: int) -> int:
    """Return the output position for a chunk.
    """
    upper = index & ~0x7F
    index &= 0x7F
    if index < 96:
        index -= 64
        if index < 0:
            index += 96
    return upper | index


def main() -> int:
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} input_file output_file")
        return 1

    in_name, out_name = sys.argv[1], sys.argv[2]

    chunks = []
    with open(in_name, "rb") as fin:
        while True:
            chunk = fin.read(8)
            if not chunk:
                break
            chunks.append(transpose_chunk(chunk))

    total = len(chunks)
    out = [None] * total
    for idx, chunk in enumerate(chunks):
        out_pos = antic_to_atascii_chunk_index(idx, total)
        if out_pos < 0 or out_pos >= total:
            raise ValueError(f"Chunk index {idx} maps out of range: {out_pos}")
        out[out_pos] = chunk

    with open(out_name, "wb") as fout:
        for chunk in out:
            fout.write(chunk)

    print(f"Processed {in_name} -> {out_name} ({total} chunks)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
