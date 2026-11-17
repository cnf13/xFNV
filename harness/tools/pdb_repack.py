"""Rewrite an MSF 7 (PDB) file with 4096-byte pages so llvm-pdbutil can read it.

The Xbox 360 debug PDB (resources/Fallout_Debug.pdb) uses 1024-byte pages; its stream directory then needs more
than one page of page numbers, which llvm-pdbutil refuses. The streams themselves are copied unchanged.

usage: pdb_repack.py IN OUT
"""

import struct
import sys

MAGIC = b"Microsoft C/C++ MSF 7.00\r\n\x1aDS\0\0\0"
PAGE = 4096


def pages(size: int, page: int) -> int:
    return (size + page - 1) // page


def read_streams(data: bytes) -> list[bytes | None]:
    if not data.startswith(MAGIC):
        sys.exit("pdb_repack: not an MSF 7 file")
    page, _, _, directory_bytes, _ = struct.unpack_from("<5I", data, len(MAGIC))

    def page_bytes(number: int) -> bytes:
        return data[number * page:(number + 1) * page]

    # the header lists the pages that hold the directory's page numbers
    directory_pages = pages(directory_bytes, page)
    map_pages = struct.unpack_from(f"<{pages(directory_pages * 4, page)}I", data, len(MAGIC) + 20)
    directory_numbers = struct.unpack_from(f"<{directory_pages}I", b"".join(page_bytes(n) for n in map_pages))
    directory = b"".join(page_bytes(n) for n in directory_numbers)[:directory_bytes]

    count = struct.unpack_from("<I", directory)[0]
    sizes = struct.unpack_from(f"<{count}I", directory, 4)
    offset = 4 + 4 * count
    streams: list[bytes | None] = []
    for size in sizes:
        if size == 0xFFFFFFFF:
            streams.append(None)
            continue
        numbers = struct.unpack_from(f"<{pages(size, page)}I", directory, offset)
        offset += 4 * len(numbers)
        streams.append(b"".join(page_bytes(n) for n in numbers)[:size])
    return streams


def write_msf(streams: list[bytes | None]) -> bytes:
    # page 0 header, pages 1 and 2 the free page maps, then the streams, the directory, its page list
    next_page = 3
    body = bytearray()
    directory = bytearray(struct.pack("<I", len(streams)))
    numbers: list[list[int]] = []
    for stream in streams:
        directory += struct.pack("<I", 0xFFFFFFFF if stream is None else len(stream))
        stream = stream or b""
        count = pages(len(stream), PAGE)
        numbers.append(list(range(next_page, next_page + count)))
        body += stream.ljust(count * PAGE, b"\0")
        next_page += count
    for stream_numbers in numbers:
        directory += struct.pack(f"<{len(stream_numbers)}I", *stream_numbers)

    directory_count = pages(len(directory), PAGE)
    directory_numbers = list(range(next_page, next_page + directory_count))
    body += bytes(directory).ljust(directory_count * PAGE, b"\0")
    next_page += directory_count
    if directory_count * 4 > PAGE:
        sys.exit("pdb_repack: directory too large for one page of page numbers")
    map_page = next_page
    body += struct.pack(f"<{directory_count}I", *directory_numbers).ljust(PAGE, b"\0")
    next_page += 1

    # every page is in use: the free page maps are all zero bits, which llvm-pdbutil accepts
    header = MAGIC + struct.pack("<6I", PAGE, 1, next_page, len(directory), 0, map_page)
    free_maps = bytes(2 * PAGE)
    return header.ljust(PAGE, b"\0") + free_maps + bytes(body)


def main() -> int:
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    with open(sys.argv[1], "rb") as source:
        streams = read_streams(source.read())
    with open(sys.argv[2], "wb") as target:
        target.write(write_msf(streams))
    print(f"pdb_repack: {len(streams)} streams")
    return 0


if __name__ == "__main__":
    sys.exit(main())
