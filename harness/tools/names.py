"""Names from the Xbox 360 debug PDB (resources/Fallout_Debug.pdb), for naming the reconstruction.

The PDB is a different build for a different platform: take only names from it (classes, members, methods,
parameters, enumerators, source files and folders). Layouts, offsets, signatures and behaviour come from the PC exe.
Member offsets are printed only to match a name to the PC field it corresponds to.

  class NAME      bases, members in order, methods (virtual marked), source file
  func NAME       parameter names of a function (Class::Method or a free function), source file
  enum NAME       enumerator names
  file NAME       source path(s) of a class or file name (e.g. TESForm, bssimplelist.h)
  search REGEX    class, enum, function and global variable names matching
  sources OUT     NAME<TAB>SOURCE: each function's source file (source_of) as the src file it is, relative to src/

Setup: harness/tools/pdb_repack.py resources/Fallout_Debug.pdb env/cache/Fallout_Debug.pdb (llvm-pdbutil refuses the
original's 1 KB pages); the dumps and the index are made on first use in env/cache/.
"""

import argparse
import os
import pickle
import re
import subprocess
import sys
from dataclasses import dataclass, field

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
CACHE = os.path.join(ROOT, "env", "cache")
PDB = os.path.join(CACHE, "Fallout_Debug.pdb")
INDEX = os.path.join(CACHE, "pdb-names.pickle")
INDEX_VERSION = 3
LLVM_BIN = "/home/cnf/.local/opt/LLVM-23.1.3-Linux-X64/bin"
CODE_ROOT = re.compile(r"^.*?\\_Fallout3\\Platforms\\Common\\Code\\", re.IGNORECASE)

HEADER = re.compile(r"^\s*(0x[0-9A-F]+) \| (LF_\w+) \[size = \d+\](?: `(.*)`)?$")
MEMBER = re.compile(r"- LF_MEMBER \[name = `(.*?)`, Type = (0x[0-9A-F]+)(?: \((.*?)\))?, offset = (\d+)")
STATIC = re.compile(r"- LF_STMEMBER \[name = `(.*?)`")
ONEMETHOD = re.compile(r"- LF_ONEMETHOD \[name = `(.*?)`\]")
METHOD = re.compile(r"- LF_METHOD \[name = `(.*?)`, # overloads = \d+, overload list = (0x[0-9A-F]+)\]")
METHOD_ATTRS = re.compile(r"type = (0x[0-9A-F]+), vftable offset = (-?\d+), attrs = (.*)$")
LIST_ENTRY = re.compile(r"- Method \[type = (0x[0-9A-F]+), vftable offset = (-?\d+), attrs = (.*)\]$")
BASE = re.compile(r"type = (0x[0-9A-F]+), offset = \d+, attrs")
ENUMERATE = re.compile(r"- LF_ENUMERATE \[(.*?) = (-?\d+)\]")
FIELD_LIST = re.compile(r"field list: (0x[0-9A-F]+)")
ARGS = re.compile(r"# args = (\d+)")

MODULE = re.compile(r"^Mod (\d+) \| `(.*)`:")
SOURCE = re.compile(r"src file ni: \d+ `(.*)`")
PROC = re.compile(r"^\s*\d+ \| S_[GL]PROC32 \[size = \d+\] `(.*)`$")
PROC_TYPE = re.compile(r"type = `(0x[0-9A-F]+) ")
REGREL = re.compile(r"^\s*\d+ \| S_REGREL32 \[size = \d+\] `(.*)`$")
SYMBOL = re.compile(r"^\s*\d+ \| (S_\w+)")
GLOBAL = re.compile(r"^\s*\d+ \| S_GDATA32 \[size = \d+\] `(.*)`$")


@dataclass
class Method:
    name: str
    virtual: bool
    static: bool
    access: str
    # byte offset in the vtable for a virtual that introduces a slot; -1 for overrides and non-virtuals
    vtoff: int = -1


@dataclass
class Class:
    name: str
    kind: str
    bases: list[str] = field(default_factory=list)
    members: list[tuple[int, str]] = field(default_factory=list)
    statics: list[str] = field(default_factory=list)
    methods: list[Method] = field(default_factory=list)


@dataclass
class Index:
    version: int
    classes: dict[str, Class]
    enums: dict[str, list[str]]
    # function name -> list of (parameter names, source file); overloads give several
    functions: dict[str, list[tuple[list[str], str]]]
    sources: list[str]
    globals: list[str]


def dump(kind: str) -> str:
    path = os.path.join(CACHE, f"pdb-{kind}.txt")
    if not os.path.exists(path):
        if not os.path.exists(PDB):
            sys.exit("names: run harness/tools/pdb_repack.py resources/Fallout_Debug.pdb env/cache/Fallout_Debug.pdb")
        with open(path, "w", encoding="utf-8") as out:
            subprocess.run([os.path.join(LLVM_BIN, "llvm-pdbutil"), "dump", f"--{kind}", PDB], stdout=out, check=True)
    return path


def records(path: str):
    """(index, kind, name, body lines) per type record."""
    current = None
    with open(path, encoding="utf-8", errors="replace") as source:
        for line in source:
            match = HEADER.match(line)
            if match:
                if current:
                    yield current
                current = (int(match.group(1), 16), match.group(2), match.group(3), [])
            elif current:
                current[3].append(line.strip())
    if current:
        yield current


def access(attrs: str) -> str:
    return attrs.split()[0]


def build_types() -> tuple[dict[str, Class], dict[str, list[str]], dict[int, int]]:
    by_index: dict[int, tuple[str, str, list[str]]] = {}
    for index, kind, name, body in records(dump("types")):
        by_index[index] = (kind, name or "", body)

    def type_name(index: int) -> str:
        record = by_index.get(index)
        if not record:
            return ""
        kind, name, body = record
        if kind == "LF_MODIFIER" or kind == "LF_POINTER":
            referent = re.search(r"referent = (0x[0-9A-F]+)", " ".join(body))
            return type_name(int(referent.group(1), 16)) if referent else ""
        return name

    def methods_of(name: str, entries: list[tuple[str, str, str]]) -> list[Method]:
        return [Method(name, "virtual" in attrs, "static" in attrs, access(attrs), int(vtoff))
                for _, vtoff, attrs in entries]

    classes: dict[str, Class] = {}
    enums: dict[str, list[str]] = {}
    for kind, name, body in by_index.values():
        text = " ".join(body)
        if "forward ref" in text:
            continue
        lists = FIELD_LIST.search(text)
        if not lists or lists.group(1) == "<no":
            continue
        fields = by_index.get(int(lists.group(1), 16))
        if not fields:
            continue
        lines = fields[2]
        if kind == "LF_ENUM":
            enums.setdefault(name, [m.group(1) for line in lines if (m := ENUMERATE.match(line))])
            continue
        if kind not in ("LF_CLASS", "LF_STRUCTURE", "LF_UNION") or name in classes:
            continue
        model = Class(name, {"LF_CLASS": "class", "LF_STRUCTURE": "struct", "LF_UNION": "union"}[kind])
        pending = None
        for line in lines:
            if line == "- LF_BCLASS":
                pending = "base"
            elif pending == "base" and (match := BASE.match(line)):
                model.bases.append(type_name(int(match.group(1), 16)))
                pending = None
            elif match := MEMBER.match(line):
                model.members.append((int(match.group(4)), match.group(1)))
            elif match := STATIC.match(line):
                model.statics.append(match.group(1))
            elif match := ONEMETHOD.match(line):
                pending = match.group(1)
            elif isinstance(pending, str) and pending != "base" and (match := METHOD_ATTRS.match(line)):
                model.methods += methods_of(pending, [(match.group(1), match.group(2), match.group(3))])
                pending = None
            elif match := METHOD.match(line):
                overloads = by_index.get(int(match.group(2), 16))
                entries = []
                for entry in overloads[2] if overloads else []:
                    if found := LIST_ENTRY.match(entry):
                        entries.append((found.group(1), found.group(2), found.group(3)))
                model.methods += methods_of(match.group(1), entries)
        classes[name] = model

    arguments = {}
    for index, (kind, _, body) in by_index.items():
        if kind in ("LF_MFUNCTION", "LF_PROCEDURE"):
            match = ARGS.search(" ".join(body))
            arguments[index] = int(match.group(1)) if match else 0
    return classes, enums, arguments


def source_files() -> tuple[dict[int, str], list[str]]:
    modules: dict[int, str] = {}
    current = None
    with open(dump("modules"), encoding="utf-8", errors="replace") as source:
        for line in source:
            line = line.strip()
            if match := MODULE.match(line):
                current = int(match.group(1))
            elif current is not None and (match := SOURCE.search(line)):
                modules[current] = CODE_ROOT.sub("", match.group(1))
    sources = set()
    with open(dump("files"), encoding="utf-8", errors="replace") as source:
        for line in source:
            for path in re.findall(r"[A-Za-z]:\\[^`\n]*", line):
                if CODE_ROOT.match(path):
                    sources.add(CODE_ROOT.sub("", path).strip())
    sources.update(path for path in modules.values() if not os.path.isabs(path.replace("\\", "/")))
    return modules, sorted(sources, key=str.lower)


def build_functions(arguments: dict[int, int], modules: dict[int, str]) -> dict[str, list[tuple[list[str], str]]]:
    functions: dict[str, list[tuple[list[str], str]]] = {}
    module_source = ""
    name = None
    wanted = 0
    parameters: list[str] = []
    module_header = re.compile(r"^Mod (\d+) \|")
    with open(dump("symbols"), encoding="utf-8", errors="replace") as source:
        for line in source:
            if match := module_header.match(line):
                module_source = modules.get(int(match.group(1)), "")
                continue
            if match := PROC.match(line):
                name, parameters, wanted = match.group(1), [], -1
                continue
            if name is None:
                continue
            if wanted == -1 and (match := PROC_TYPE.search(line)):
                wanted = arguments.get(int(match.group(1), 16), 0)
                continue
            if match := REGREL.match(line):
                if match.group(1) == "this":
                    continue
                if len(parameters) < wanted:
                    parameters.append(match.group(1))
                continue
            symbol = SYMBOL.match(line)
            if symbol and symbol.group(1) in ("S_END", "S_BLOCK32"):
                entries = functions.setdefault(name, [])
                if (parameters, module_source) not in entries:
                    entries.append((parameters, module_source))
                name = None
    return functions


def load() -> Index:
    if os.path.exists(INDEX):
        with open(INDEX, "rb") as cached:
            index = pickle.load(cached)
        if index.version == INDEX_VERSION:
            return index
    print("names: indexing the PDB dumps (once)", file=sys.stderr)
    classes, enums, arguments = build_types()
    modules, sources = source_files()
    with open(dump("globals"), encoding="utf-8", errors="replace") as source:
        variables = sorted({match.group(1) for line in source if (match := GLOBAL.match(line))})
    index = Index(INDEX_VERSION, classes, enums, build_functions(arguments, modules), sources, variables)
    with open(INDEX, "wb") as cached:
        pickle.dump(index, cached)
    return index


def strip_templates(name: str) -> str:
    """BSSimpleList<TESFile *>::AddHead -> BSSimpleList::AddHead"""
    while True:
        stripped = re.sub(r"<[^<>]*>", "", name)
        if stripped == name:
            return name
        name = stripped


# id(index) -> (index, {class prefix: {source: member count}}, {stem: header sources}): files_of_class's tables, built
# once per index (a scan of every PDB function per call took 0.2-0.6 s)
_CLASS_FILES: dict[int, tuple[Index, dict[str, dict[str, int]], dict[str, list[str]]]] = {}


def _class_files(index: Index) -> tuple[dict[str, dict[str, int]], dict[str, list[str]]]:
    known = _CLASS_FILES.get(id(index))
    if known is not None and known[0] is index:
        return known[1], known[2]
    # every prefix P of a template-free function name followed by `::` (each `startswith(P + "::")` that holds), the
    # sources counted in the functions' order
    counts: dict[str, dict[str, int]] = {}
    for function, entries in index.functions.items():
        sources = [source for _, source in entries if source]
        if not sources:
            continue
        stripped = strip_templates(function)
        at = stripped.find("::")
        while at >= 0:
            table = counts.setdefault(stripped[:at], {})
            for source in sources:
                table[source] = table.get(source, 0) + 1
            at = stripped.find("::", at + 1)
    headers: dict[str, list[str]] = {}
    for path in index.sources:
        if not path.lower().endswith(".cpp"):
            headers.setdefault(os.path.splitext(os.path.basename(path.replace("\\", "/")))[0].lower(), []).append(path)
    _CLASS_FILES[id(index)] = (index, counts, headers)
    return counts, headers


def files_of_class(index: Index, name: str) -> list[str]:
    """The class's own .cpp (the one named after it, else the one defining most of its members) and headers."""
    name = strip_templates(name)
    by_prefix, by_stem = _class_files(index)
    counts = by_prefix.get(name, {})
    stem = name.lower()
    own = [path for path in counts if os.path.splitext(os.path.basename(path.replace("\\", "/")))[0].lower() == stem]
    if not own and counts:
        own = [max(counts, key=counts.get)]
    headers = by_stem.get(stem, [])
    return sorted(set(own + headers), key=str.lower)


def source_of(index: Index, name: str) -> str | None:
    """The PDB source file of the function: its only one; else (an inline function appears in every file using it)
    the class's own .cpp among them; else the first in sorted order. None when the PDB names no source."""
    sources = sorted({source for _, source in index.functions.get(name, []) if source})
    if not sources:
        return None
    if len(sources) == 1:
        return sources[0]
    cls = name.rpartition("::")[0]
    own = [s for s in files_of_class(index, cls) if s.lower().endswith(".cpp")] if cls else []
    for source in sources:
        if source in own:
            return source
    return sources[0]


def command_class(index: Index, arguments) -> None:
    model = index.classes.get(arguments.name)
    if not model:
        sys.exit(f"names: no class '{arguments.name}' (try: search)")
    bases = f" : {', '.join(model.bases)}" if model.bases else ""
    print(f"{model.kind} {model.name}{bases}")
    for path in files_of_class(index, model.name):
        print(f"  source {path}")
    print("  members (xbox offset only to match names):")
    for offset, member in model.members:
        print(f"    {offset:#06x}  {member}")
    for member in model.statics:
        print(f"    static  {member}")
    print("  methods:")
    seen = set()
    for method in model.methods:
        key = (method.name, method.virtual, method.static)
        count = sum(1 for other in model.methods if (other.name, other.virtual, other.static) == key)
        if key in seen:
            continue
        seen.add(key)
        marks = " ".join(mark for mark, on in (("virtual", method.virtual), ("static", method.static)) if on)
        overloads = f" (x{count})" if count > 1 else ""
        print(f"    {method.access:9} {marks:8} {method.name}{overloads}")


def command_func(index: Index, arguments) -> None:
    entries = index.functions.get(arguments.name)
    if entries:
        matches = [(arguments.name, entries)]
    else:
        matches = [(name, found) for name, found in index.functions.items()
                   if strip_templates(name) == arguments.name]
    if not matches:
        sys.exit(f"names: no function '{arguments.name}' (try: search)")
    printed = set()
    # one line per parameter list; inline functions appear in every file that uses them, so show one and a count
    sites: dict[str, list[str]] = {}
    for name, found in matches:
        for parameters, source in found:
            sites.setdefault(f"{strip_templates(name)}({', '.join(parameters)})", []).append(source)
    for signature, sources in sites.items():
        more = f" (+{len(sources) - 1} more)" if len(sources) > 1 else ""
        print(f"{signature}    {sources[0]}{more}")


def command_enum(index: Index, arguments) -> None:
    names = index.enums.get(arguments.name)
    if names is None:
        sys.exit(f"names: no enum '{arguments.name}' (try: search)")
    for name in names:
        print(name)


def command_file(index: Index, arguments) -> None:
    found = files_of_class(index, arguments.name)
    needle = arguments.name.lower()
    found += [path for path in index.sources if os.path.basename(path.replace("\\", "/")).lower().startswith(needle)]
    for path in sorted(set(found), key=str.lower):
        print(path)


def command_search(index: Index, arguments) -> None:
    pattern = re.compile(arguments.regex)
    for kind, names in (("class", index.classes), ("enum", index.enums), ("func", index.functions),
                        ("var", index.globals)):
        for name in sorted(names):
            if pattern.search(name):
                print(f"{kind:5} {name}")


def command_sources(index: Index, arguments) -> None:
    # the PDB's paths differ from src's in case (Fallout\Ai\ vs src/Fallout/AI/) and include files src never has
    # (obj\xbox\pch_hdr.src): each is written as the src file it matches case-insensitively, relative to src/, and a
    # function whose source matches none is left out
    import srcindex
    src = srcindex.load()
    mapped: dict[str, str | None] = {}
    with open(arguments.out, "w", encoding="utf-8") as out:
        for name in index.functions:
            source = source_of(index, name)
            if source and source not in mapped:
                path = src.map_source(source)
                mapped[source] = path[len("src/"):] if path and path.startswith("src/") else None
            if source and mapped[source]:
                out.write(f"{name}\t{mapped[source]}\n")


def main() -> int:
    # quiet when piped into head
    import signal
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)

    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)
    for command, argument in (("class", "name"), ("func", "name"), ("enum", "name"), ("file", "name"),
                              ("search", "regex"), ("sources", "out")):
        commands.add_parser(command).add_argument(argument)
    arguments = parser.parse_args()
    index = load()
    handlers = {"class": command_class, "func": command_func, "enum": command_enum, "file": command_file,
                "search": command_search, "sources": command_sources}
    handlers[arguments.command](index, arguments)
    return 0


if __name__ == "__main__":
    sys.exit(main())
