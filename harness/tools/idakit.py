"""The harness's IDA tools in C++ (harness/idakit, built to build/idakit/idakit) from Python.

    import idakit
    idakit.dump(addresses, idb, out_dir, format_version)   # ctree dumps: {address: seconds}
    idakit.functions(out_tsv, names_tsv)                    # the canonical database's function list and names
    idakit.decompile(addresses, out_dir)                    # its Hex-Rays text, out_dir/<ADDR8>.c

The binary runs idalib, which locks the .i64 to its process: never pass a database another IDA (GUI, idalib Python,
another idakit) has open. functions() and decompile() read the canonical database through a scratch copy of it,
deleted after, so they never hold the canonical (the sync, the lead's IDA) and can run while it is open.
"""

from __future__ import annotations

import os
import shutil
import subprocess
import sys
import tempfile

# the checkout's build; an agent worktree (env/wt/NAME) links env to the main checkout's and has no build of its own
_HERE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__))))
BINARY = next((p for p in (os.path.join(_HERE, "build", "idakit", "idakit"),
                           os.path.join(os.path.dirname(os.path.realpath(os.path.join(_HERE, "env"))), "build",
                                        "idakit", "idakit"))
               if os.path.exists(p)), os.path.join(_HERE, "build", "idakit", "idakit"))
BUILD = ("cmake -S harness/idakit -B build/idakit -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang "
         "-DCMAKE_CXX_COMPILER=clang++ && cmake --build build/idakit")
WORK = os.path.join(_HERE, "env", "scratch", "idakit")
CANONICAL = os.path.join(_HERE, "env", "ida", "FalloutNV.exe.i64")
MODEL = os.path.join(_HERE, "env", "cache", "srcmodel.sqlite")


def run(args: list[str], **kwargs) -> subprocess.CompletedProcess:
    """idakit ARGS: idalib reads its license and settings from the user's own HOME (env.sh redirects HOME)."""
    if not os.path.exists(BINARY):
        sys.exit(f"idakit: {BINARY} is missing; build it: {BUILD}")
    env = dict(os.environ, HOME=os.environ.get("FNVD_USER_HOME", os.path.expanduser("~")))
    return subprocess.run(["nice", "-n", "19", BINARY] + args, env=env, text=True, **kwargs)


def canonical_stamp() -> str:
    """The canonical database's saved state: its mtime and size (idalib writes the .i64 only when it saves)."""
    status = os.stat(CANONICAL)
    return f"{status.st_mtime_ns} {status.st_size}"


def decompile_stamp() -> str:
    """What decompile()'s texts depend on: the canonical's stamp and the srcdb model's mtime and size (each
    function is unfolded from the model before its decompile)."""
    status = os.stat(MODEL)
    return f"{canonical_stamp()} {status.st_mtime_ns} {status.st_size}"


def _on_copy(args: list[str], **kwargs) -> str:
    """idakit ARGS with `{idb}` replaced by a scratch copy of the canonical database (deleted after), run with
    kwargs; returns the stamp of the state copied. A copy taken while a save rewrote the .i64 is taken again."""
    os.makedirs(WORK, exist_ok=True)
    folder = tempfile.mkdtemp(prefix="canonical-", dir=WORK)
    try:
        copy = os.path.join(folder, "FalloutNV.exe.i64")
        for _ in range(10):
            stamp = canonical_stamp()
            shutil.copyfile(CANONICAL, copy)
            if canonical_stamp() == stamp:
                break
        else:
            sys.exit(f"idakit: {CANONICAL} kept changing while it was copied")
        done = run([copy if a == "{idb}" else a for a in args], stdout=subprocess.PIPE, **kwargs)
        if done.returncode not in (0, 1):
            sys.exit(f"idakit {args[0]} failed ({done.returncode})")
        return stamp
    finally:
        shutil.rmtree(folder, ignore_errors=True)


def functions(out: str, names: str) -> str:
    """The canonical database's function list into out (`0xADDR<TAB>0xEND<TAB>NAME`) and its names into names
    (`0xADDR<TAB>NAME`); the stamp of its state."""
    return _on_copy(["functions", "--idb", "{idb}", "--out", out, "--names", names])


def decompile(addresses: list[int], out: str) -> str:
    """The canonical database's Hex-Rays text of addresses into out/<ADDR8>.c (out/<ADDR8>.err: why there is
    none), each function's Hex-Rays stores rebuilt first from the checkout's srcdb model, as emit rebuilds them; the
    stamp of the state decompiled."""
    os.makedirs(WORK, exist_ok=True)
    with tempfile.NamedTemporaryFile("w", prefix="decompile-", suffix=".txt", dir=WORK, delete=False) as handle:
        handle.writelines(f"{a:#x}\n" for a in addresses)
    try:
        return _on_copy(["decompile", "--idb", "{idb}", "--out", os.path.abspath(out), "@" + handle.name],
                        cwd=_HERE)
    finally:
        os.unlink(handle.name)
