#!/usr/bin/env python3
"""Renames functions in an IDA database from a table, through idalib.

    env/venv/bin/python harness/tools/ida_rename.py --idb env/ida/FalloutNV.exe.i64 TSV

TSV rows are `address<TAB>qualified name[<TAB>evidence]`; blank and `#` lines are skipped. Every target is first given
a placeholder name, so a name one row's target holds now is free when another row takes it. Template spellings
(`<`, `,`, ` `, `*`) are allowed for this session. A name another address holds (an overload) gets IDA's `_N` suffix,
as the sync gives it: counted as renamed. Prints `renamed N (K with _N), failed M` and each failure (its original name
is put back); exits 1 on any failure. The database is saved either way. Who may write the canonical database: RULES
"The IDA database".
"""

from __future__ import annotations

import argparse
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from idalib_open import open_idb  # noqa: E402


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--idb", required=True)
    parser.add_argument("table")
    args = parser.parse_args()
    rows = []
    for line in open(args.table):
        if line.strip() and not line.startswith("#"):
            address, name = line.rstrip("\n").split("\t")[:2]
            rows.append((int(address, 16), name))
    idapro = open_idb(args.idb)
    import ida_name

    for _, name in rows:
        for char in name:
            if not ida_name.is_ident_cp(ord(char)):
                ida_name.set_cp_validity(ida_name.UCDR_NAME, ord(char), ord(char) + 1, True)
    # a row that fails gets its original name back, not the placeholder
    original = {ea: ida_name.get_name(ea) for ea, _ in rows}
    for ea, _ in rows:
        ida_name.set_name(ea, f"rename_{ea:X}", ida_name.SN_FORCE | ida_name.SN_NOWARN)
    renamed = suffixed = failed = 0
    for ea, name in rows:
        ok = ida_name.set_name(ea, name, ida_name.SN_FORCE | ida_name.SN_NOWARN | ida_name.SN_NOCHECK)
        got = ida_name.get_name(ea)
        if ok and got == name:
            renamed += 1
        elif got.startswith(name) and re.fullmatch(r"_\d+", got[len(name):]):
            renamed += 1
            suffixed += 1
        else:
            failed += 1
            ida_name.set_name(ea, original[ea], ida_name.SN_FORCE | ida_name.SN_NOWARN | ida_name.SN_NOCHECK)
            print(f"{ea:#x}\twanted {name}\tgot {got}\tkept {ida_name.get_name(ea)}")
    idapro.close_database(True)
    print(f"renamed {renamed} ({suffixed} with _N), failed {failed}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
