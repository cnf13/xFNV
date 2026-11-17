#!/usr/bin/env python3
"""Looks up records of a Fallout: New Vegas plugin (default env/run/Data/FalloutNV.esm) by editor id.

Usage: harness/tools/esm.py [--esm FILE] [--type TYPE[,TYPE...]] find REGEX
  prints "FORMID TYPE EDID ["FULL"]" for every record whose EDID matches REGEX (Python re.search, case-insensitive).
  FalloutNV.esm is load order 00, so the form id is what the console takes (`player.placeatme`, `additem`, `prid`;
  leading zeros may be dropped). `coc` takes a CELL's EDID, interior or exterior.

Format (TES4 family, FNV): a record header is 24 bytes (type, data size, flags, form id, version control, form
version, unknown); flag 0x40000 marks zlib-compressed data (a uint32 decompressed size, then the zlib stream). A GRUP
header is 24 bytes too (GRUP, size including the header, label, group type, stamp, unknown) and holds records and
groups. A subrecord is (type, uint16 size, data); an XXXX subrecord carries the uint32 size of the next one.
"""
import argparse
import os
import re
import signal
import struct
import sys
import zlib

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
DEFAULT_ESM = os.path.join(ROOT, "env", "run", "Data", "FalloutNV.esm")
COMPRESSED = 0x40000
HEADER = struct.Struct("<4sIIIIHH")


def subrecords(data):
    """Yields (type, bytes) for the subrecords of one record's (decompressed) data."""
    offset = 0
    pending = None
    while offset + 6 <= len(data):
        kind, size = struct.unpack_from("<4sH", data, offset)
        offset += 6
        if pending is not None:
            size, pending = pending, None
        if kind == b"XXXX":
            pending = struct.unpack_from("<I", data, offset)[0]
            offset += size
            continue
        yield kind, data[offset:offset + size]
        offset += size


def records(blob, start=0, end=None, types=None):
    """Yields (type, form id, data) for every record in blob[start:end], descending into GRUPs."""
    end = len(blob) if end is None else end
    offset = start
    while offset + HEADER.size <= end:
        kind, size, flags, form_id, _, _, _ = HEADER.unpack_from(blob, offset)
        if kind == b"GRUP":
            # size covers the group header; a top-level group's label is the record type it holds
            label = blob[offset + 8:offset + 12]
            group_type = flags
            if group_type == 0 and types is not None and label not in types:
                offset += size
                continue
            yield from records(blob, offset + HEADER.size, offset + size, types)
            offset += size
            continue
        data_start = offset + HEADER.size
        offset = data_start + size
        if types is not None and kind not in types:
            continue
        data = blob[data_start:offset]
        if flags & COMPRESSED:
            # raw inflate past the 2-byte zlib header: a few FalloutNV.esm records carry a wrong adler32
            data = zlib.decompressobj(-zlib.MAX_WBITS).decompress(data[6:])
        yield kind, form_id, data


def cstring(raw):
    return raw.split(b"\0", 1)[0].decode("cp1252", "replace")


def find(blob, pattern, types):
    expression = re.compile(pattern, re.IGNORECASE)
    for kind, form_id, data in records(blob, HEADER.size + struct.unpack_from("<I", blob, 4)[0], types=types):
        edid = full = None
        for sub, raw in subrecords(data):
            if sub == b"EDID":
                edid = cstring(raw)
            elif sub == b"FULL" and full is None:
                full = cstring(raw)
            if edid is not None and full is not None:
                break
        if edid is not None and expression.search(edid):
            yield form_id, kind.decode(), edid, full


def main():
    # quiet when piped into head
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--esm", default=DEFAULT_ESM)
    parser.add_argument("--type", help="comma list of record types to search (default: all)")
    commands = parser.add_subparsers(dest="command", required=True)
    find_parser = commands.add_parser("find", help="records whose EDID matches REGEX")
    find_parser.add_argument("regex")
    args = parser.parse_args()

    with open(args.esm, "rb") as file:
        blob = file.read()
    if blob[:4] != b"TES4":
        sys.exit(f"esm: {args.esm} is not a TES4 plugin")
    types = None if not args.type else {t.strip().upper().encode() for t in args.type.split(",")}
    count = 0
    for form_id, kind, edid, full in find(blob, args.regex, types):
        print(f"{form_id:08X} {kind} {edid}" + (f" \"{full}\"" if full else ""))
        count += 1
    if count == 0:
        sys.exit(1)


if __name__ == "__main__":
    main()
