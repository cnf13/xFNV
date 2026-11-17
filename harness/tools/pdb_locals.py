#!/usr/bin/env python3
"""Runs pdb_locals_ida.py over the PC database in parallel, headless.

Each worker opens its own scratch copy of the database (--idb, default the canonical) with idalib, pairs its share of
the functions with the debug build's locals (pdb_locals_xbox.py's facts), writes the settings on its copy to see them
hold, and exports them. The copies are never saved and are deleted at the end. The parts are joined, ordered by
function, into env/scratch/pdblocals/settings.jsonl (one function's user lvar settings per line) and report.tsv (one
local per line: function, old name, new name, old type, new type, score, kind, state, old name was the user's).

Writing the settings into a database is pdb_locals_ida.apply_exported, in the session that has the database open (no
decompile, so it takes seconds): --apply does it here and saves --idb, which no other process may hold open.

Usage: pdb_locals.py [--idb I64] [--jobs N] [--only FILE-OF-ADDRESSES] [--fresh] [--apply]
"""
import argparse
import collections
import json
import os
import shutil
import subprocess
import sys
import time

TOOLS = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(TOOLS))
CANONICAL = os.path.join(ROOT, "env", "ida", "FalloutNV.exe.i64")
OUT = os.path.join(ROOT, "env", "scratch", "pdblocals")
WORK = os.path.join(OUT, "work")
# CRT functions with user lvar settings older than this pass: --apply never clears them (the pass has settings of
# its own for 0xEC660E only)
KEPT = (0xEC623A, 0xEC660E, 0xEC680F, 0xEC9983, 0xED8238, 0xEE311D, 0xEE383C, 0xEE3908, 0xEE8AC0)


def write(arguments) -> None:
    """The exported settings written into the database and saved."""
    sys.path.insert(0, TOOLS)
    import idalib_open
    idapro = idalib_open.open_idb(arguments.database)
    import ida_hexrays
    import pdb_locals_ida as matcher
    if not ida_hexrays.init_hexrays_plugin():
        sys.exit("pdb_locals: no Hex-Rays")
    path = os.path.join(OUT, "settings.jsonl")
    with open(path, encoding="utf-8") as source:
        lines = [line for line in source
                 if (item := json.loads(line))["lvars"] or item["ea"] not in KEPT]
    with open(os.path.join(WORK, "apply.jsonl"), "w", encoding="utf-8") as out:
        out.writelines(lines)
    result = matcher.apply_exported(os.path.join(WORK, "apply.jsonl"), budget=float("inf"))
    if result["dropped"]:
        idapro.close_database(False)
        sys.exit(f"pdb_locals: {result['dropped']} settings' types did not load; not saved")
    idapro.close_database(True)
    print(f"pdb_locals: settings of {result['written']} functions written, saved {arguments.database}")


def worker(arguments) -> None:
    sys.path.insert(0, TOOLS)
    import idalib_open
    idapro = idalib_open.open_idb(arguments.database)
    import idautils
    import pdb_locals_ida as matcher
    functions = sorted(idautils.Functions())
    if arguments.only:
        with open(arguments.only, encoding="utf-8") as source:
            wanted = {int(line.split()[0], 16) for line in source if line.strip()}
        functions = [ea for ea in functions if ea in wanted]
    part = os.path.join(WORK, f"part{arguments.worker}")
    matcher.REPORT = part + ".tsv"
    if arguments.names:
        matcher.load_names(arguments.names)
    result = matcher.run(functions[arguments.worker::arguments.of], apply=True, budget=float("inf"),
                         settings=part + ".jsonl", fresh=arguments.fresh, votes=part if arguments.votes else None)
    with open(part + ".counts", "w", encoding="utf-8") as out:
        json.dump(result["counts"], out)
    idapro.close_database(False)


def main() -> None:
    global OUT, WORK
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--idb", default=CANONICAL, help="the database (default: the canonical)")
    parser.add_argument("--out", default=OUT, help="where the result goes (default: env/scratch/pdblocals)")
    parser.add_argument("--apply", action="store_true",
                        help="write the settings into --idb and save it (nothing else may hold it open)")
    parser.add_argument("--jobs", type=int, default=min(16, os.cpu_count() or 1))
    parser.add_argument("--only", help="a file of function addresses (hex, one per line): only these")
    parser.add_argument("--fresh", action="store_true",
                        help="clear each function's user lvar settings first; a function left with none is "
                             "exported empty, so applying the result clears it")
    parser.add_argument("--votes", action="store_true",
                        help="also write what the pairings say about callees, globals and parameters "
                             "(votes.tsv, tokens.jsonl, debug-tokens.jsonl: pdb_names.py's input)")
    parser.add_argument("--names", help="pdb_names.py's names-accepted.tsv: debug names for PC functions, taken "
                                        "over the database's for this run")
    parser.add_argument("--worker", type=int, help=argparse.SUPPRESS)
    parser.add_argument("--of", type=int, help=argparse.SUPPRESS)
    parser.add_argument("--database", help=argparse.SUPPRESS)
    arguments = parser.parse_args()
    OUT = os.path.abspath(arguments.out)
    WORK = os.path.join(OUT, "work")
    if arguments.worker == -1:
        write(arguments)
        return
    if arguments.worker is not None:
        worker(arguments)
        return
    started = time.time()
    shutil.rmtree(WORK, ignore_errors=True)
    os.makedirs(WORK)
    jobs = []
    try:
        for n in range(arguments.jobs):
            copy = os.path.join(WORK, f"w{n}.i64")
            shutil.copyfile(arguments.idb, copy)
            command = [sys.executable, os.path.abspath(__file__), "--worker", str(n), "--of", str(arguments.jobs),
                       "--out", OUT,
                       "--database", copy] + (["--only", os.path.abspath(arguments.only)] if arguments.only else []) + \
                (["--fresh"] if arguments.fresh else []) + (["--votes"] if arguments.votes else []) + \
                (["--names", os.path.abspath(arguments.names)] if arguments.names else [])
            log = open(os.path.join(WORK, f"part{n}.log"), "w", encoding="utf-8")
            jobs.append((subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT, cwd=ROOT), log))
        failed = []
        for n, (process, log) in enumerate(jobs):
            if process.wait():
                failed.append(n)
            log.close()
        if failed:
            for n in failed:
                with open(os.path.join(WORK, f"part{n}.log"), encoding="utf-8", errors="replace") as log:
                    sys.stderr.write(f"--- worker {n} ---\n{log.read()[-3000:]}\n")
            sys.exit(f"pdb_locals: workers {failed} failed (PDB_LOCALS_KEEP_WORK=1 keeps their parts in {WORK})")
        counts = collections.Counter()
        settings, rows = [], []
        for n in range(arguments.jobs):
            part = os.path.join(WORK, f"part{n}")
            with open(part + ".counts", encoding="utf-8") as source:
                counts.update(json.load(source))
            for suffix, into in ((".jsonl", settings), (".tsv", rows)):
                if os.path.exists(part + suffix):
                    with open(part + suffix, encoding="utf-8") as source:
                        into += source.readlines()
        # ordered by function; a function's rows keep their order
        settings.sort(key=lambda line: json.loads(line)["ea"])
        rows = [row for _, _, row in sorted((int(row.split("\t", 1)[0], 16), n, row) for n, row in enumerate(rows))]
        with open(os.path.join(OUT, "settings.jsonl"), "w", encoding="utf-8") as out:
            out.writelines(settings)
        with open(os.path.join(OUT, "report.tsv"), "w", encoding="utf-8") as out:
            out.writelines(rows)
        for suffix, name in ((".votes.tsv", "votes.tsv"), (".tokens.jsonl", "tokens.jsonl"),
                             (".debug.jsonl", "debug-tokens.jsonl")) if arguments.votes else ():
            lines = set()
            for n in range(arguments.jobs):
                if os.path.exists(os.path.join(WORK, f"part{n}{suffix}")):
                    with open(os.path.join(WORK, f"part{n}{suffix}"), encoding="utf-8") as source:
                        lines.update(source)
            with open(os.path.join(OUT, name), "w", encoding="utf-8") as out:
                out.writelines(sorted(lines))
        if arguments.apply:
            subprocess.run([sys.executable, os.path.abspath(__file__), "--worker", "-1", "--out", OUT, "--database",
                            os.path.abspath(arguments.idb)], cwd=ROOT, check=True)
    finally:
        for process, _ in jobs:
            if process.poll() is None:
                process.kill()
        if not os.environ.get("PDB_LOCALS_KEEP_WORK"):
            shutil.rmtree(WORK, ignore_errors=True)
    print(f"pdb_locals: {len(settings)} functions with settings, {len(rows)} locals, "
          f"{time.time() - started:.0f} s, {arguments.jobs} workers")
    for name, count in sorted(counts.items()):
        print(f"  {name}: {count}")


if __name__ == "__main__":
    main()
