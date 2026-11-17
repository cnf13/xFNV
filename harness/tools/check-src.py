#!/usr/bin/env python3
"""Compile reconstruction sources in the no-exe build (the native Windows program) without linking.

Usage: env/venv/bin/python harness/tools/check-src.py [--noexe | --noexe64] [--jobs N] FILE.cpp [...]
  --noexe    the 32-bit no-exe program (build/noexe; the default): a real compile, into no object
  --noexe64  the 64-bit no-exe program (build/noexe64, `noexe.py build --arch x64` configures it), the same way
  --jobs N   files compiled at once (default 1)

Each file is compiled with the flags of an existing src/ entry of that build's compile_commands.json
(build/{noexe,noexe64}), so new files work before CMake has been re-configured. A header can be checked by passing a
.cpp that includes it. Exit status 1 if any file fails.
"""

import concurrent.futures
import json
import os
import shlex
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
BUILDS = ("noexe",)


def template(build: str) -> tuple[str, list[str]]:
    with open(os.path.join(ROOT, "build", build, "compile_commands.json")) as handle:
        entries = json.load(handle)
    # A file under ROOT/src/ (the checkout's own path holds "/src/" too, and so external/ C files' paths do).
    src = os.path.join(os.path.realpath(ROOT), "src") + os.sep
    entry = next(e for e in entries if os.path.realpath(e["file"]).startswith(src))
    words = shlex.split(entry["command"])
    kept = []
    skip = False
    for word in words:
        if skip:
            skip = False
            continue
        if word == entry["file"] or word == "--":
            continue
        # outputs: clang-cl spells them glued (/Fo..., /Fd...)
        if word.startswith(("/Fo", "/Fd")):
            continue
        if word in ("-o", "-c"):
            skip = word == "-o"
            continue
        kept.append(word)
    # Compiled for real (main() names a scratch object): backend errors (a missing target feature on an SSE body) do
    # not show in a syntax-only pass.
    check = ["/c"]
    return entry["directory"], kept + check


def main() -> int:
    global ROOT
    args = sys.argv[1:]
    # --root DIR: check against another tree's src/ (a trial copy with its own build/*/compile_commands.json)
    if len(args) >= 2 and args[0] == "--root":
        ROOT = os.path.abspath(args[1])
        args = args[2:]
    builds = BUILDS
    jobs = 1
    while args and args[0] in ("--noexe", "--noexe64", "--jobs"):
        if args[0] == "--jobs":
            jobs = int(args[1])
            args = args[2:]
            continue
        builds = (args[0][2:],)
        args = args[1:]
    if not args:
        print(__doc__, file=sys.stderr)
        return 2
    os.environ["PATH"] = "/home/cnf/.local/opt/LLVM-23.1.3-Linux-X64/bin:" + os.environ.get("PATH", "")
    commands = {build: template(build) for build in builds}

    def check(path: str) -> tuple[bool, str]:
        source = os.path.abspath(path)
        ok, text = True, ""
        for build, (directory, command) in commands.items():
            # The object goes to the project's scratch (never /tmp) and is deleted with its directory.
            parent = os.path.join(ROOT, "env", "scratch", "check-src")
            os.makedirs(parent, exist_ok=True)
            with tempfile.TemporaryDirectory(dir=parent) as holder:
                result = subprocess.run(command + ["/Fo" + os.path.join(holder, "check.obj"), "--", source],
                    cwd=directory, capture_output=True, text=True)
            status = "ok" if result.returncode == 0 else "FAIL"
            text += f"{status:4} {build:10} {os.path.relpath(source, ROOT)}\n"
            if result.returncode != 0:
                ok = False
                text += result.stdout + result.stderr
        return ok, text

    failed = False
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, jobs)) as pool:
        for ok, text in pool.map(check, args):
            failed = failed or not ok
            sys.stdout.write(text)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
