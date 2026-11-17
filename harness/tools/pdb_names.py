#!/usr/bin/env python3
"""Turns the votes of a `pdb_locals.py --votes` run into proposals: function names, global names and parameter
differences, for review and for the src rename path. Writes nothing into the database or src.

A vote is one PC call (or global use) that the paired locals of its function line up with one debug call (or global
use): pdb_locals_ida.Match.votes. Here the votes of every caller are counted per target:

  names.tsv    address, name now, proposed name, class, tier, callers, votes, body, xbox_proposed's name
  globals.tsv  address, name now, proposed name, class, functions voting
  params.tsv   function, parameter, `PC name : PC type => debug name : debug type`, how the types compare
  names-accepted.tsv  address, debug name: the input of `pdb_locals.py --names`

The name now is the database's, which for a function src declares is src's name (sync writes it).
class: `confirms` (the name now is the proposed one), `unnamed` (the function has no real name), `mangled` (its name
is a mangled spelling IDA kept), `not-pdb` (its name is none the debug build has), `other-pdb` (its name is another
debug function's: a fold of identical code, or a wrong name), `split` (the callers disagree; proposed lists every
name). tier: A two or more calling functions, all for one name; B one calling function with vote weight 2 or
more; C the rest. body (functions with no pairing of their own): `agrees` when the PC body and a debug body of the
proposed name share two or more tokens (strings, floats, larger numbers, named globals, named callees) and at
least half of the smaller set, `small` when either has under two, `differs` otherwise; inlining moves tokens
between functions, so `differs` is weak evidence and `agrees` strong. A name voted through an inlined wrapper is
the wrapper's, not the callee's: `agrees` rules that out, callers alone do not.

Usage: pdb_names.py [--dir env/scratch/pdblocals]
"""
import argparse
import collections
import csv
import json
import os
import re
import sqlite3
import sys

TOOLS = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(TOOLS))
sys.path.insert(0, TOOLS)
import names as pdbnames  # noqa: E402
import srcindex  # noqa: E402

UNNAMED = re.compile(r"(^|::|_)sub_[0-9A-Fa-f]{5,}|^\.text_|^nullsub_|^j_|^unknown_libname|^\?|^$")
AUTO_GLOBAL = re.compile(r"^(dword|byte|word|qword|flt|dbl|tbyte|off|unk|stru|asc|xmmword|a[A-Z]\w*)_[0-9A-Fa-f]+$|^$")


def canonical(name: str) -> str:
    return pdbnames.strip_templates(name).replace(" ", "")


def key_of(name: str, known: set) -> str | None:
    """The name key of an IDA function name, None for one that is no name; IDA's `_N` suffix dropped where the
    debug build has no function of the suffixed name."""
    if UNNAMED.search(name):
        return None
    qualified = srcindex.qualified_of_ida(name)
    if not qualified:
        return None
    key = canonical(srcindex.name_key(qualified))
    if key not in known and re.search(r"_\d+$", key):
        key = re.sub(r"_\d+$", "", key)
    return key


def read_bodies(here: str) -> tuple[dict, dict]:
    """(PC bodies' token sets by address, debug bodies' token sets by debug name) from a votes run's directory."""
    bodies = {}
    with open(os.path.join(here, "tokens.jsonl"), encoding="utf-8") as source:
        for line in source:
            item = json.loads(line)
            bodies[item["ea"]] = set(item["t"])
    debug_bodies = collections.defaultdict(list)
    with open(os.path.join(here, "debug-tokens.jsonl"), encoding="utf-8") as source:
        for line in source:
            item = json.loads(line)
            debug_bodies[item["key"]] += [set(body) for body in item["bodies"]]
    return bodies, debug_bodies


def body_agreement(mine: set, theirs: list) -> str:
    """The `body` class of a PC body's tokens against the debug bodies of one name (the module docstring)."""
    if not theirs:
        return "none"
    best = max(theirs, key=lambda b: len(b & mine))
    shared, least = len(best & mine), min(len(best), len(mine))
    return "small" if least < 2 else "agrees" if shared >= 2 and 2 * shared >= least else "differs"


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--dir", default=os.path.join(ROOT, "env", "scratch", "pdblocals"))
    arguments = parser.parse_args()
    here = arguments.dir

    facts = sqlite3.connect(os.path.join(ROOT, "env", "cache", "pdb-locals.sqlite"))
    debug_names = {canonical(key) for (key,) in facts.execute("select key from callee")}
    debug_names |= {canonical(key) for (key,) in facts.execute("select distinct key from func")}
    other = {}
    table = os.path.join(ROOT, "env", "cache", "names", "xbox_proposed.tsv")
    if os.path.exists(table):
        with open(table, encoding="utf-8", newline="") as source:
            for row in csv.DictReader(source, delimiter="\t"):
                other[int(row["address"], 16)] = (row["name"], row["confidence"])
    bodies, debug_bodies = read_bodies(here)

    functions = collections.defaultdict(lambda: collections.defaultdict(lambda: [set(), 0.0, 0, set()]))
    globals_ = collections.defaultdict(lambda: collections.defaultdict(set))
    now = {}
    parameters = []
    with open(os.path.join(here, "votes.tsv"), encoding="utf-8") as source:
        for line in source:
            caller, kind, target, name, value, strength = line.rstrip("\n").split("\t")
            if kind == "p":
                parameters.append((caller, target, value, strength))
                continue
            address = int(target, 16)
            now[address] = name
            if kind == "f":
                entry = functions[address][canonical(value)]
                entry[0].add(caller)
                entry[1] = max(entry[1], float(strength))
                entry[2] += 1
                entry[3].add(value)
            else:
                globals_[address][value].add(caller)

    counts = collections.Counter()
    rows = []
    for address, voted in sorted(functions.items()):
        name = now[address]
        key = key_of(name, debug_names)
        ranked = sorted(voted.items(), key=lambda item: (-len(item[1][0]), item[0]))
        proposed, (callers, strength, votes, spellings) = ranked[0]
        if len(ranked) > 1:
            kind = "split"
            shown = " | ".join(f"{sorted(entry[3])[0]} ({len(entry[0])})" for _, entry in ranked)
        else:
            shown = sorted(spellings)[0]
            kind = "confirms" if key == proposed else "mangled" if name.startswith("?") else \
                "unnamed" if key is None else "other-pdb" if key in debug_names else "not-pdb"
        tier = "A" if len(callers) >= 2 else "B" if strength >= 2.0 else "C"
        body = ""
        if address in bodies and kind != "split":
            theirs = [b for spelling in spellings for b in debug_bodies.get(spelling, ())]
            body = body_agreement(bodies[address], theirs)
        proposal = other.get(address)
        agreement = ""
        if proposal:
            agreement = f"{proposal[0]} ({proposal[1]}{', same' if canonical(proposal[0]) == proposed else ''})"
        counts[(kind, tier, body)] += 1
        rows.append((f"{address:#x}", name, shown, kind, tier, len(callers), votes, body, agreement))
    with open(os.path.join(here, "names.tsv"), "w", encoding="utf-8") as out:
        out.write("address\tnow\tproposed\tclass\ttier\tcallers\tvotes\tbody\txbox_proposed\n")
        for row in rows:
            out.write("\t".join(map(str, row)) + "\n")
    # what a next pdb_locals.py run takes (--names): one name, from two callers or a heavy vote, where the function
    # has no PDB name now
    with open(os.path.join(here, "names-accepted.tsv"), "w", encoding="utf-8") as out:
        for row in rows:
            if row[3] in ("unnamed", "mangled", "not-pdb") and row[4] in ("A", "B"):
                out.write(f"{row[0]}\t{row[2]}\n")

    global_counts = collections.Counter()
    with open(os.path.join(here, "globals.tsv"), "w", encoding="utf-8") as out:
        out.write("address\tnow\tproposed\tclass\tfunctions\n")
        for address, voted in sorted(globals_.items()):
            name = now[address]
            ranked = sorted(voted.items(), key=lambda item: (-len(item[1]), item[0]))
            if len(ranked) > 1:
                kind, shown = "split", " | ".join(f"{value} ({len(callers)})" for value, callers in ranked)
            else:
                shown = ranked[0][0]
                kind = "unnamed" if AUTO_GLOBAL.match(name) else "named"
            global_counts[(kind, "2+" if len(ranked[0][1]) >= 2 else "1")] += 1
            out.write(f"{address:#x}\t{name}\t{shown}\t{kind}\t{len(ranked[0][1])}\n")

    def plain(text: str) -> str:
        return re.sub(r"\b(const|struct|class|volatile)\b|\s", "", text)

    compared = collections.Counter()
    with open(os.path.join(here, "params.tsv"), "w", encoding="utf-8") as out:
        out.write("function\tparameter\tPC => debug\ttypes\n")
        for function, label, value, strength in sorted(parameters):
            pc, _, debug = value.partition(" => ")
            how = "equal" if strength == "1.00" else "equal but const" \
                if plain(pc.partition(" : ")[2]) == plain(debug.partition(" : ")[2]) else "differ"
            compared[how] += 1
            out.write(f"{function}\t{label}\t{value}\t{how}\n")

    print(f"functions voted on: {len(rows)}")
    for kind in ("confirms", "unnamed", "mangled", "not-pdb", "other-pdb", "split"):
        for tier in "ABC":
            found = {body: n for (k, t, body), n in counts.items() if k == kind and t == tier}
            if found:
                detail = ", ".join(f"body {body or 'n/a'}: {n}" for body, n in sorted(found.items()))
                print(f"  {kind:9} tier {tier}: {sum(found.values()):6}  ({detail})")
    print(f"globals voted on: {sum(global_counts.values())}")
    for (kind, callers), n in sorted(global_counts.items()):
        print(f"  {kind:8} {callers} functions: {n}")
    print(f"parameters listed: {len(parameters)} " + ", ".join(f"{how}: {n}" for how, n in sorted(compared.items())))


if __name__ == "__main__":
    main()
