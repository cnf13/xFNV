#!/usr/bin/env python3
"""Names the code addresses in a crash log (harness/crashlog) or fnvd_debugheap.log (harness/debugheap) that the log
left as `MODULE+0xOFFSET`, by llvm-symbolizer.

Usage: harness/tools/crashlog-symbolize.py LOG [PDB | ELF]
  Windows log: PDB default build/noexe/FalloutNV.pdb; the exe is the .exe beside it, its file name the module the
               addresses name. It must be the build the log names ("Build:" line), else the names are wrong.
  POSIX log:   (its Modules lines, or the debug heap's "debugheap: image" line, give each module's path) every module's
               addresses from its ELF's DWARF (or symbol table); ELF, when given, stands in for the program's own file
               (the module of its file name). The files must be those the log's run loaded.
Prints the log with each `MODULE+0xOFFSET` followed by ` = function (file:line)`.
"""

import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
LLVM = os.path.join(os.environ.get("FNVD_USER_HOME", os.path.expanduser("~")), ".local/opt/LLVM-23.1.3-Linux-X64/bin")
# A POSIX log's module paths: "  BEGIN..END NAME PATH" (crash log) and "debugheap: image NAME PATH".
POSIX_MODULE = re.compile(r"^  [0-9A-F]+\.\.[0-9A-F]+ (\S+) (/.*)$|^debugheap: image (\S+) (/.*)$", re.M)


def symbolize(args, offsets):
    """{offset: "function (file:line)"} for the hex offsets, by llvm-symbolizer with args."""
    out = subprocess.run([os.path.join(LLVM, "llvm-symbolizer"), "--output-style=JSON", "--relativenames"] + args +
                         ["0x" + offset for offset in offsets], capture_output=True, text=True, check=True).stdout
    # one JSON array per line, of one or more addresses' entries
    entries = {}
    for line in out.splitlines():
        for entry in json.loads(line):
            entries.setdefault(int(entry["Address"], 16), entry)
    names = {}
    for offset in offsets:
        entry = entries.get(int(offset, 16))
        symbol = entry["Symbol"][0] if entry and entry.get("Symbol") else None
        if symbol and symbol["FunctionName"] and symbol["FunctionName"] != "??":
            where = "%s:%d" % (symbol["FileName"], symbol["Line"]) if symbol["FileName"] else "no line"
            names[offset] = "%s (%s)" % (symbol["FunctionName"], where)
    return names


def annotate(text, module, args):
    pattern = re.compile(r"(?<![\w.+-])" + re.escape(module) + r"\+0x([0-9A-F]+)\b")
    offsets = sorted({match.group(1) for match in pattern.finditer(text)})
    if not offsets:
        return text
    names = symbolize(args, offsets)
    return pattern.sub(lambda m: m.group(0) + (" = " + names[m.group(1)] if m.group(1) in names else ""), text)


def main():
    if len(sys.argv) not in (2, 3):
        sys.exit(__doc__)
    text = open(sys.argv[1], encoding="utf-8", errors="replace").read()
    given = sys.argv[2] if len(sys.argv) == 3 else None
    modules = {}
    for match in POSIX_MODULE.finditer(text):
        name, path = (match.group(1), match.group(2)) if match.group(1) else (match.group(3), match.group(4))
        modules.setdefault(name, path)
    if modules:
        if given:
            modules[os.path.basename(given)] = given
        for name, path in modules.items():
            if os.path.isfile(path):
                text = annotate(text, name, ["--obj=" + path])
    else:
        pdb = given or os.path.join(ROOT, "build", "noexe", "FalloutNV.pdb")
        exe = os.path.splitext(pdb)[0] + ".exe"
        text = annotate(text, os.path.basename(exe), ["--obj=" + exe, "--pdb=" + pdb, "--relative-address"])
    sys.stdout.write(text)


if __name__ == "__main__":
    main()
