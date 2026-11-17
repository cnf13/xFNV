"""Open an IDA database with idalib from env/venv/bin/python.

idalib (IDA Pro 9.3) needs IDADIR (it reads its licence from there) and its config from $HOME/.idapro, the real home:
harness/env.sh redirects HOME into env/home, so HOME falls back to /home/cnf when it has no .idapro. idalib opens
the .i64 in place and locks it to one process; a killed process leaves the unpacked .id0/.id1/.nam/.til beside the
.i64, and idalib then fails to open it (exit 1, no message) until those files are deleted (the .i64 itself is only
written on save).

Opening the canonical env/ida/FalloutNV.exe.i64 first takes a rollback snapshot of it in env/ida/snapshots/ (the
newest 5 kept); run this file directly to take one by hand before an xd://ida write.
"""

from __future__ import annotations

import os
import shutil
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
IDA_DIR = "/home/cnf/ida-pro-9.3"
IDA_HOME = "/home/cnf"
CANONICAL = os.path.realpath(os.path.join(ROOT, "env", "ida", "FalloutNV.exe.i64"))


def snapshot_canonical(real: str) -> None:
    """Copy the canonical to env/ida/snapshots/FalloutNV.exe.<stamp>.i64 and keep the newest 5 (RULES "The IDA
    database")."""
    folder = os.path.join(os.path.dirname(real), "snapshots")
    try:
        os.makedirs(folder, exist_ok=True)
        shutil.copyfile(real, os.path.join(folder, f"FalloutNV.exe.{time.strftime('%Y%m%d-%H%M%S')}.i64"))
    except OSError as error:
        sys.exit(f"cannot snapshot {real}: {error}")
    snapshots = sorted(name for name in os.listdir(folder) if name.endswith(".i64"))
    for name in snapshots[:-5]:
        os.remove(os.path.join(folder, name))


def open_idb(path: str):
    """Open the database (no auto-analysis, Hex-Rays initialised); refuse anything under resources/. Returns the
    idapro module (close with idapro.close_database(save=...))."""
    real = os.path.realpath(path)
    if real.startswith(os.path.join(os.path.realpath(ROOT), "resources") + os.sep):
        sys.exit(f"refusing to open {path}: work on a copy")
    if not os.path.exists(real):
        sys.exit(f"no database {path}")
    if real == CANONICAL:
        snapshot_canonical(real)
    os.environ["IDADIR"] = IDA_DIR
    if not os.path.isdir(os.path.join(os.environ.get("HOME", ""), ".idapro")):
        os.environ["HOME"] = IDA_HOME
    sys.path.insert(0, os.path.join(IDA_DIR, "idalib", "python"))
    import idapro
    if idapro.open_database(real, run_auto_analysis=False):
        sys.exit(f"idalib could not open {path}")
    import ida_hexrays
    ida_hexrays.init_hexrays_plugin()
    return idapro


if __name__ == "__main__":
    snapshot_canonical(CANONICAL)
