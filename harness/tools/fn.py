#!/usr/bin/env python3
"""Queries over the exe's functions, for choosing and writing reconstructions.

Usage: harness/tools/fn.py COMMAND ...   (run with env/venv/bin/python; needs capstone and pefile)
  show FN              size, reconstructed or not, probe hits, callers, callees, Windows imports
  decomp FN            the Hex-Rays text, from the canonical IDA database (decomp_text)
  asm FN               the instructions, branch and call targets named
  type NAME            the struct and its vtable from IDA's header (resources/FalloutNV.exe.h)
  vtable CLASS|ADDR    the exe's vtables of a class (by RTTI; every subobject's table), or the vtable at or around
                       an address: each slot's index, byte offset, function. `show` lists the vtables holding FN
  candidates           functions to reconstruct next, cheapest first (see --help of the command)
  classes              per class: functions, exercised, reconstructed, sizes
  probe-list           every function entry not reconstructed, for harness/run.sh -p
  where ARG...         what src/ and gen/ of the checkout you run it in say about exe addresses (0x...) and src
                       names (Class::member, a function, global, class or template instance), one block per ARG:
                       registrations, forwarders, folds, declarations, bodies, users (`where --help`)

FN is an address (any address inside the function) or a name (exact, else a unique substring).
Inputs: env/cache/ida/functions.tsv (the canonical IDA database's function list, `idakit functions`, with its names in
env/cache/ida/ida-names.tsv; both rewritten here when the database is newer), env/exe/FalloutNV.exe, src/
(FNVD_REPLACES), and the newest env/out/*/probe.tsv (or --probe).
The call graph is cached in env/cache/fn-graph.pickle, the Hex-Rays text in env/cache/decomp/ (per database state).
"""

from __future__ import annotations

import argparse
import bisect
import fcntl
import glob
import hashlib
import itertools
import os
import pickle
import re
import struct
import sys
from collections import defaultdict
from dataclasses import dataclass, field

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
FUNCTIONS = os.path.join(ROOT, "env", "cache", "ida", "functions.tsv")
NAMES = os.path.join(ROOT, "env", "cache", "ida", "ida-names.tsv")
HEADER = os.path.join(ROOT, "resources", "FalloutNV.exe.h")
EXE = os.path.join(ROOT, "env", "exe", "FalloutNV.exe")
CACHE = os.path.join(ROOT, "env", "cache", "fn-graph.pickle")
VTABLES = os.path.join(ROOT, "env", "cache", "fn-vtables.pickle")
DECOMP_CACHE = os.path.join(ROOT, "env", "cache", "decomp")
SRC = os.path.join(ROOT, "src")
OUT = os.path.join(ROOT, "env", "out")

CACHE_VERSION = 3
HEX = re.compile(r"0x[0-9a-fA-F]+")
REPLACES = re.compile(r"FNVD_REPLACES(?:_CONSTRUCTOR|_OVERLOAD|_VIRTUAL|_VIRTUAL_OVERLOAD|_VIRTUAL_VIA|"
                      r"_VIRTUAL_VIA_OVERLOAD|_MEMBER)?\(\s*(0x[0-9a-fA-F]+)")


@dataclass
class Function:
    address: int
    name: str
    size: int = 0
    callees: list[int] = field(default_factory=list)
    imports: list[str] = field(default_factory=list)
    # pointers to the entry in .rdata: vtable slots (virtual members); the shim replaces those only through a free
    # adapter (docs/VERIFY.md)
    slots: int = 0


class Graph:
    def __init__(self, functions: list[Function]):
        self.functions = functions
        self.starts = [f.address for f in functions]
        self.by_address = {f.address: f for f in functions}
        self.callers: dict[int, list[int]] = defaultdict(list)
        for function in functions:
            for callee in function.callees:
                self.callers[callee].append(function.address)

    def containing(self, address: int) -> Function | None:
        index = bisect.bisect_right(self.starts, address) - 1
        if index < 0:
            return None
        function = self.functions[index]
        return function if address < function.address + max(function.size, 1) else None

    def resolve(self, token: str) -> Function:
        if re.fullmatch(r"(0x)?[0-9a-fA-F]{6,8}", token):
            function = self.containing(int(token, 16))
            if function:
                return function
            sys.exit(f"fn: no function at {token}")
        exact = [f for f in self.functions if f.name == token]
        if len(exact) == 1:
            return exact[0]
        matches = exact or [f for f in self.functions if token in f.name]
        if len(matches) == 1:
            return matches[0]
        if not matches:
            sys.exit(f"fn: no function named like '{token}'")
        listing = "\n".join(f"  {f.address:#010x} {f.name}" for f in matches[:30])
        more = f"\n  ... {len(matches) - 30} more" if len(matches) > 30 else ""
        sys.exit(f"fn: '{token}' is ambiguous:\n{listing}{more}")


def refresh_functions() -> None:
    """Rewrites FUNCTIONS and NAMES from the canonical IDA database (idakit functions) when FUNCTIONS is missing or
    older than the database (a save that changed no function leaves its content as it was: load_graph keys on the
    content)."""
    import idakit
    os.makedirs(os.path.dirname(FUNCTIONS), exist_ok=True)
    with open(FUNCTIONS + ".lock", "w") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        if os.path.exists(FUNCTIONS) and os.path.getmtime(FUNCTIONS) >= os.path.getmtime(idakit.CANONICAL):
            return
        idakit.functions(FUNCTIONS + ".new", NAMES + ".new")
        if not (os.path.exists(FUNCTIONS + ".new") and os.path.exists(NAMES + ".new")):
            sys.exit("fn: idakit functions wrote no list")
        # FUNCTIONS last: its mtime says both are current
        os.replace(NAMES + ".new", NAMES)
        os.replace(FUNCTIONS + ".new", FUNCTIONS)


def ida_names() -> dict[int, str]:
    """The canonical IDA database's names (NAMES: `0xADDR<TAB>NAME`), address -> name."""
    refresh_functions()
    names = {}
    with open(NAMES, encoding="utf-8", errors="replace") as source:
        for line in source:
            parts = line.rstrip("\n").split("\t")
            if len(parts) == 2:
                names.setdefault(int(parts[0], 16), parts[1])
    return names


def vtable_names() -> dict[int, str]:
    """The canonical IDA database's vtables (`??_7X@@6B@`, `??_7X@@6BBase@@@`) demangled, address -> ``const
    X::`vftable'`` or ``const X::`vftable'{for `Base'}``, template arguments without `class `/`struct ` and without
    a space after a comma (`NiTMap<char const *,Setting *>`). A name llvm-undname cannot read is left out."""
    import srcindex
    tables = {address: name for address, name in ida_names().items() if name.startswith("??_7")}
    out = {}
    for address, text in zip(tables, srcindex.undname(list(tables.values()))):
        if text:
            out[address] = re.sub(r"\b(?:class|struct|union|enum) ", "", text).replace(", ", ",")
    return out


def vtables() -> dict[int, tuple[str, int, list[int]]]:
    """Every exe vtable, address -> (class, base count, slot functions): the runs of code pointers in .rdata that
    start after an RTTI locator (the class is its type descriptor's, ``X`` or ``X {for the subobject at +N}`` for a
    base subobject's table; the base count is its hierarchy's, the class included: the fewest is the base-most) or
    at a vtable IDA names (IDA's demangled name; base count -1: unknown). A run ends before the next vtable, before an
    address the code stores into memory (`mov [mem], imm32`: another table, without RTTI or a name, that a
    constructor stores) and at the first dword that is no function's entry (a string or constant that reads as a code
    address). Cached in VTABLES per exe and IDA name list; processes take turns building it."""
    refresh_functions()
    stamp = (3, os.path.getmtime(EXE), os.path.getmtime(NAMES))

    def cached():
        try:
            with open(VTABLES, "rb") as cache:
                saved = pickle.load(cache)
        except (OSError, EOFError, pickle.UnpicklingError):
            return None
        return saved[1] if saved[0] == stamp else None

    if (saved := cached()) is not None:
        return saved
    with open(VTABLES + ".lock", "w") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        if (saved := cached()) is not None:
            return saved
        out = build_vtables()
        temporary = f"{VTABLES}.{os.getpid()}.new"
        with open(temporary, "wb") as cache:
            pickle.dump((stamp, out), cache)
        os.replace(temporary, VTABLES)
        return out


def build_vtables() -> dict[int, tuple[str, int, list[int]]]:
    """vtables(), from the exe."""
    import pefile
    import srcindex
    pe = pefile.PE(EXE, fast_load=True)
    base = pe.OPTIONAL_HEADER.ImageBase
    sections = [(base + s.VirtualAddress, s.get_data()) for s in pe.sections]
    text_start, text = next((base + s.VirtualAddress, s.get_data()) for s in pe.sections
                            if s.Name.rstrip(b"\0") == b".text")
    rdata_start, rdata = next((base + s.VirtualAddress, s.get_data()) for s in pe.sections
                              if s.Name.rstrip(b"\0") == b".rdata")
    text_end = text_start + len(text)

    def dword(va: int) -> int | None:
        for start, data in sections:
            if start <= va <= start + len(data) - 4:
                return struct.unpack_from("<I", data, va - start)[0]
        return None

    def text_at(va: int) -> bytes:
        for start, data in sections:
            if start <= va < start + len(data):
                return data[va - start: va - start + 1024].split(b"\0", 1)[0]
        return b""

    def locator(va: int):
        """MSVC's RTTICompleteObjectLocator at va (signature 0, the subobject's offset, cdOffset, type descriptor,
        class hierarchy): (mangled class, subobject offset, base count), else None."""
        if not rdata_start <= va < rdata_start + len(rdata) or dword(va) != 0:
            return None
        descriptor, hierarchy = dword(va + 12), dword(va + 16)
        raw = text_at(descriptor + 8) if descriptor else b""
        if not raw.startswith((b".?AV", b".?AU")):
            return None
        count = dword(hierarchy + 8) if hierarchy else None
        return raw[4:].decode("ascii", "replace"), dword(va + 4) or 0, count if count is not None else -1

    named = vtable_names()
    entries = {f.address for f in read_functions()}
    # the .rdata addresses the code stores into memory: `mov [mem], imm32` (C7 /0) in its register, disp8, disp32 and
    # esp-based forms
    rdata_end = rdata_start + len(rdata)
    referenced = set()
    for form in (rb"\xC7[\x00-\x03\x06\x07]", rb"\xC7[\x40-\x43\x45-\x47].", rb"\xC7[\x80-\x83\x85-\x87].{4}",
                 rb"\xC7\x04\x24", rb"\xC7\x44\x24.", rb"\xC7\x84\x24.{4}"):
        for match in re.finditer(form + rb"(?=(.{4}))", text, re.DOTALL):
            value = struct.unpack("<I", match.group(1))[0]
            if rdata_start <= value < rdata_end:
                referenced.add(value)
    words = struct.unpack_from(f"<{len(rdata) // 4}I", rdata)
    found, current = {}, None
    for index, value in enumerate(words):
        address = rdata_start + index * 4
        if value not in entries:
            current = None
            continue
        rtti = locator(words[index - 1]) if index and words[index - 1] not in entries else None
        if rtti or address in named:
            current = found[address] = [rtti, []]
        elif address in referenced:
            current = None
        if current:
            current[1].append(value)
    mangled = {address: "??_7" + entry[0][0] + "6B@" for address, entry in found.items() if entry[0]}
    classes = dict(zip(mangled, srcindex.undname(list(mangled.values()))))

    def tidy(text: str) -> str:
        text = re.sub(r"\b(?:class|struct|union|enum) ", "", text).replace(", ", ",")
        return re.sub(r"^const (.*)::`vftable'", r"\1", text)

    out = {}
    for address, (rtti, slots) in found.items():
        if rtti:
            name = tidy(classes.get(address) or rtti[0])
            name += f" {{for the subobject at +{rtti[1]:#x}}}" if rtti[1] else ""
            out[address] = (name, rtti[2], slots)
        else:
            out[address] = (tidy(named[address]), -1, slots)
    return out


def vtable_slots(address: int) -> list[tuple[int, int, str, int]]:
    """The exe vtables that hold the function: (vtable address, slot index, class, base count), base-most first
    (vtables without RTTI last)."""
    out = [(start, slots.index(address), name, bases)
           for start, (name, bases, slots) in vtables().items() if address in slots]
    return sorted(out, key=lambda row: (row[3] < 0, row[3], row[0]))



def read_functions() -> list[Function]:
    """The canonical IDA database's functions (FUNCTIONS: `0xADDR<TAB>0xEND<TAB>NAME`), by address; sizes come from
    build_graph."""
    refresh_functions()
    functions = []
    with open(FUNCTIONS, encoding="utf-8") as listing:
        for line in listing:
            address, _, name = line.rstrip("\n").split("\t", 2)
            functions.append(Function(int(address, 16), name))
    functions.sort(key=lambda f: f.address)
    return functions


def build_graph() -> Graph:
    import capstone
    import pefile

    functions = read_functions()
    pe = pefile.PE(EXE, fast_load=True)
    pe.parse_data_directories(directories=[pefile.DIRECTORY_ENTRY["IMAGE_DIRECTORY_ENTRY_IMPORT"]])
    base = pe.OPTIONAL_HEADER.ImageBase
    text = next(s for s in pe.sections if s.Name.rstrip(b"\0") == b".text")
    text_start = base + text.VirtualAddress
    code = text.get_data()
    text_end = text_start + len(code)

    imports = {}
    for library in pe.DIRECTORY_ENTRY_IMPORT:
        dll = library.dll.decode("ascii", "replace")
        for entry in library.imports:
            name = entry.name.decode("ascii", "replace") if entry.name else f"#{entry.ordinal}"
            imports[entry.address] = f"{dll}!{name}"

    starts = {f.address for f in functions}
    disassembler = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    for index, function in enumerate(functions):
        end = functions[index + 1].address if index + 1 < len(functions) else text_end
        end = min(end, text_end)
        if not text_start <= function.address < end:
            continue
        # alignment padding after the last instruction is not part of the function
        while end > function.address and code[end - 1 - text_start] == 0xCC:
            end -= 1
        function.size = end - function.address

        callees, used = set(), set()
        offset = function.address
        while offset < end:
            chunk = code[offset - text_start : end - text_start]
            for address, size, mnemonic, operands in disassembler.disasm_lite(chunk, offset):
                offset = address + size
                for match in HEX.findall(operands):
                    value = int(match, 16)
                    if value in imports:
                        used.add(imports[value])
                # direct calls and tail jumps into another function (a function's range holds no other start)
                if mnemonic in ("call", "jmp") and operands.startswith("0x"):
                    target = int(operands, 16)
                    if target in starts and target != function.address:
                        callees.add(target)
            if offset < end:
                # disasm_lite stopped at an undecodable byte (a jump table, data): skip it
                offset += 1
        function.callees = sorted(callees)
        function.imports = sorted(used)

    rdata = next(s for s in pe.sections if s.Name.rstrip(b"\0") == b".rdata").get_data()
    by_address = {f.address: f for f in functions}
    for (value,) in struct.iter_unpack("<I", rdata[: len(rdata) & ~3]):
        if value in by_address:
            by_address[value].slots += 1
    return Graph(functions)


def load_graph() -> Graph:
    refresh_functions()
    with open(FUNCTIONS, "rb") as listing:
        digest = hashlib.sha1(listing.read()).hexdigest()
    stamp = (CACHE_VERSION, digest, os.path.getmtime(EXE))
    if os.path.exists(CACHE):
        with open(CACHE, "rb") as cache:
            cached_stamp, functions = pickle.load(cache)
        if cached_stamp == stamp:
            return Graph(functions)
    print("fn: building the call graph (once; cached in env/cache)", file=sys.stderr)
    graph = build_graph()
    os.makedirs(os.path.dirname(CACHE), exist_ok=True)
    with open(CACHE, "wb") as cache:
        pickle.dump((stamp, graph.functions), cache)
    return graph


def address_table(root: str = ROOT) -> list[dict]:
    """The rows of the checkout's harness/addresses.tsv (harness/tools/addresses.py export: every exe address src's
    seam records named, with kind, name and file); [] when it does not exist (the records are then still in src)."""
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    import addresses
    return addresses.load(os.path.join(root, "harness", "addresses.tsv"))


def reconstructed() -> dict[int, str]:
    """Addresses registered by FNVD_REPLACES (the forms REPLACES matches) in src/, with the file that replaces each:
    from harness/addresses.tsv when it exists, else from the registration lines."""
    table = address_table()
    if table:
        return {r["address"]: r["file"] for r in table if r["origin"] == "src" and REPLACES.match(r["macro"] + "(0x0")
                and r["file"].startswith("src/") and r["file"].endswith(".cpp")}
    found = {}
    for path in glob.glob(os.path.join(SRC, "**", "*.cpp"), recursive=True):
        with open(path, encoding="utf-8") as source:
            for match in REPLACES.finditer(source.read()):
                found[int(match.group(1), 16)] = os.path.relpath(path, ROOT)
    return found


def newest_probe() -> str | None:
    reports = glob.glob(os.path.join(OUT, "*", "probe.tsv"))
    return max(reports, key=os.path.getmtime) if reports else None


def load_probe(path: str | None) -> tuple[dict[int, tuple[int, int]], str | None]:
    path = path or newest_probe()
    hits = {}
    if path:
        with open(path, encoding="utf-8") as report:
            for line in report:
                address, main, other = line.split("\t")
                hits[int(address, 16)] = (int(main), int(other))
    return hits, path


def class_of(name: str) -> str:
    # "A::B::Method" -> "A::B"; templates may contain "::" inside <>, so cut at the last top-level "::"
    depth, cut = 0, -1
    for i, character in enumerate(name):
        if character in "<(":
            depth += 1
        elif character in ">)":
            depth -= 1
        elif depth == 0 and name.startswith("::", i):
            cut = i
    return name[:cut] if cut > 0 else ""


def mark(address: int, done: dict[int, str]) -> str:
    return "✓" if address in done else " "


def command_show(graph: Graph, arguments) -> None:
    done = reconstructed()
    hits, probe = load_probe(arguments.probe)
    function = graph.resolve(arguments.function)
    main, other = hits.get(function.address, (0, 0))
    print(f"{function.address:#010x}  {function.name}")
    print(f"  size {function.size} bytes")
    print(f"  reconstructed: {done.get(function.address, 'no')}")
    if function.slots:
        tables = vtable_slots(function.address)
        print(f"  {function.slots} pointer(s) to it in .rdata, {len(tables)} in vtables (virtual: replace through a"
              " free adapter); the rest are callback tables. Base-most class first (bases: the class's RTTI base"
              " count, itself included); a derived class's slot holds the base's function until it overrides:")
        for start, slot, name, bases in tables[: arguments.limit]:
            print(f"    {start:#010x} slot {slot:3}  {name}" + (f"  (bases {bases})" if bases >= 0 else ""))
        if len(tables) > arguments.limit:
            print(f"    ... {len(tables) - arguments.limit} more")
    if probe:
        print(f"  probe hits: main {main}, other threads {other}   ({os.path.relpath(probe, ROOT)})")
    if function.imports:
        listing = ", ".join(function.imports)
        print(f"  windows: {listing}")
    for title, addresses in (("callees", function.callees), ("callers", graph.callers.get(function.address, []))):
        print(f"  {title} ({len(addresses)}):")
        for address in addresses[: arguments.limit]:
            other_function = graph.by_address[address]
            calls = sum(hits.get(address, (0, 0)))
            print(f"    {mark(address, done)} {address:#010x} {other_function.size:6} {calls:4}  {other_function.name}")
        if len(addresses) > arguments.limit:
            print(f"    ... {len(addresses) - arguments.limit} more")


def decomp_texts(addresses) -> dict[int, str | None]:
    """The canonical IDA database's Hex-Rays text of functions (by start address; None where Hex-Rays fails), from
    DECOMP_CACHE: decompiled on first use through idakit (one run for all missing), kept until the database is saved
    again or the srcdb model is rebuilt (idakit.decompile_stamp). Processes take turns on the cache."""
    import idakit
    addresses = sorted(set(addresses))
    os.makedirs(DECOMP_CACHE, exist_ok=True)
    stamp_path = os.path.join(DECOMP_CACHE, "STAMP")
    with open(os.path.join(DECOMP_CACHE, ".lock"), "w") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        stamp = idakit.decompile_stamp()
        if not os.path.exists(stamp_path) or open(stamp_path).read() != stamp:
            for name in os.listdir(DECOMP_CACHE):
                if name.endswith((".c", ".err")):
                    os.unlink(os.path.join(DECOMP_CACHE, name))
            with open(stamp_path, "w") as handle:
                handle.write(stamp)
        missing = [a for a in addresses if not any(os.path.exists(os.path.join(DECOMP_CACHE, f"{a:08X}{suffix}"))
                                                   for suffix in (".c", ".err"))]
        if missing:
            idakit.decompile(missing, DECOMP_CACHE)
    texts = {}
    for address in addresses:
        path = os.path.join(DECOMP_CACHE, f"{address:08X}.c")
        texts[address] = open(path, encoding="utf-8", errors="replace").read() if os.path.exists(path) else None
    return texts


def decomp_text(address: int) -> str | None:
    """decomp_texts for one function."""
    return decomp_texts([address])[address]


def command_decomp(graph: Graph, arguments) -> None:
    function = graph.resolve(arguments.function)
    text = decomp_text(function.address)
    if text is None:
        error = os.path.join(DECOMP_CACHE, f"{function.address:08X}.err")
        reason = open(error).read().strip() if os.path.exists(error) else "no text"
        sys.exit(f"no Hex-Rays text for {function.address:#x}: {reason}")
    sys.stdout.write(text)


def command_asm(graph: Graph, arguments) -> None:
    import capstone
    import pefile

    function = graph.resolve(arguments.function)
    pe = pefile.PE(EXE, fast_load=True)
    start = function.address - pe.OPTIONAL_HEADER.ImageBase
    code = pe.get_data(start, function.size)
    disassembler = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    print(f"; {function.address:#010x} {function.name}, {function.size} bytes")
    tables = vtables()
    for address, size, mnemonic, operands in disassembler.disasm_lite(code, function.address):
        note = ""
        if operands.startswith("0x") and mnemonic in ("call", "jmp") or mnemonic.startswith("j"):
            target = graph.containing(int(operands, 16)) if operands.startswith("0x") else None
            if target and target is not function:
                note = f"  ; {target.name}"
        else:
            # a vtable's address as an operand: the store that makes the object one of that class
            for match in HEX.findall(operands):
                if int(match, 16) in tables:
                    note = f"  ; vtable of {tables[int(match, 16)][0]}"
        print(f"{address:08x}  {mnemonic:7} {operands}{note}")


def command_vtable(graph: Graph, arguments) -> None:
    tables = vtables()
    wanted = arguments.name
    if re.fullmatch(r"(0x)?[0-9a-fA-F]{6,8}", wanted):
        address = int(wanted, 16)
        chosen = [start for start, (_, _, slots) in tables.items() if start <= address < start + 4 * len(slots)]
    else:
        def owner(name: str) -> str:
            return name.split(" {for", 1)[0].split("{for", 1)[0].strip()
        chosen = [start for start, (name, _, _) in tables.items() if owner(name) == wanted]
        if not chosen:
            owners = sorted({owner(name) for name, _, _ in tables.values() if wanted in name})
            if len(owners) != 1:
                sys.exit(f"fn: no vtable of a class '{wanted}'" + (": " + ", ".join(owners[:30]) if owners else ""))
            chosen = [start for start, (name, _, _) in tables.items() if owner(name) == owners[0]]
    if not chosen:
        sys.exit(f"fn: no vtable at {wanted}")
    for start in sorted(chosen, key=lambda s: (tables[s][0], s)):
        name, bases, slots = tables[start]
        print(f"{start:#010x}  vtable of {name}, {len(slots)} slots"
              + (f", {bases} classes in its hierarchy (itself included)" if bases >= 0 else ", no RTTI"))
        for index, target in enumerate(slots):
            function = graph.by_address.get(target)
            print(f"  slot {index:3} (+{index * 4:#05x})  {target:#010x}  {function.name if function else '?'}")


def command_type(graph: Graph, arguments) -> None:
    names = [arguments.name, arguments.name + "_vtbl"]
    opener = re.compile(
        r"^(?:struct|union|enum)(?: __cppobj| __declspec\([^)]*\)| :\s*\w+)* (" + "|".join(map(re.escape, names)) + r")(?:\s*:.*)?$"
    )
    printed = False
    with open(HEADER, encoding="utf-8", errors="replace") as header:
        lines = iter(header)
        for line in lines:
            if not opener.match(line.rstrip("\n")):
                continue
            block = [line]
            for body in lines:
                block.append(body)
                if body.startswith("};"):
                    break
            sys.stdout.write("".join(block) + "\n")
            printed = True
    if not printed:
        sys.exit(f"fn: no struct '{arguments.name}' in {os.path.relpath(HEADER, ROOT)}")


def command_candidates(graph: Graph, arguments) -> None:
    done = reconstructed()
    hits, probe = load_probe(arguments.probe)
    if not probe and not arguments.unexercised:
        sys.exit("fn: no probe.tsv yet (harness/run.sh -p); pass --unexercised to rank without one")
    rows = []
    for function in graph.functions:
        if function.address in done or not arguments.min_size <= function.size <= arguments.max_size:
            continue
        # IDA's labels for chunks inside CRT functions ("strrchr:::returndi") are not functions of their own
        if ":::" in function.name:
            continue
        if arguments.class_name and class_of(function.name) != arguments.class_name:
            continue
        if arguments.match and not re.search(arguments.match, function.name):
            continue
        calls = sum(hits.get(function.address, (0, 0)))
        if calls < arguments.min_hits and not arguments.unexercised:
            continue
        windows = function.imports
        open_callees = [c for c in function.callees if c not in done]
        neighbours = function.callees + graph.callers.get(function.address, [])
        adjacent = sum(1 for n in neighbours if n in done)
        # cost: code to write, plus each unreconstructed callee to declare; closeness to finished code pays
        score = function.size + 25 * len(open_callees) - 40 * min(adjacent, 3)
        rows.append((score, function, calls, len(open_callees), adjacent, windows))
    rows.sort(key=lambda row: (row[0], row[1].address))
    if probe:
        print(f"probe: {os.path.relpath(probe, ROOT)}")
    print(f"{'address':10} {'size':>5} {'hits':>5} {'open':>4} {'adj':>3} {'v':>2}  name")
    for score, function, calls, open_count, adjacent, windows in rows[: arguments.limit]:
        flag = f"  [windows: {', '.join(windows)}]" if windows else ""
        virtual = function.slots or ""
        print(
            f"{function.address:#010x} {function.size:5} {calls:5} {open_count:4} {adjacent:3} {virtual:>2}  "
            f"{function.name}{flag}"
        )
    print(f"({len(rows)} candidates)")


def command_classes(graph: Graph, arguments) -> None:
    done = reconstructed()
    hits, probe = load_probe(arguments.probe)
    stats: dict[str, list[int]] = defaultdict(lambda: [0, 0, 0, 0])
    for function in graph.functions:
        name = class_of(function.name)
        if not name:
            continue
        row = stats[name]
        row[0] += 1
        if sum(hits.get(function.address, (0, 0))):
            row[1] += 1
            row[3] += function.size
        if function.address in done:
            row[2] += 1
    ordered = sorted(stats.items(), key=lambda item: (-item[1][2], -item[1][1], item[0]))
    if arguments.exercised:
        ordered = [item for item in ordered if item[1][1]]
    if probe:
        print(f"probe: {os.path.relpath(probe, ROOT)}")
    print(f"{'functions':>9} {'hit':>5} {'done':>5} {'hit bytes':>10}  class")
    for name, (count, exercised, finished, size) in ordered[: arguments.limit]:
        print(f"{count:9} {exercised:5} {finished:5} {size:10}  {name}")


def command_probe_list(graph: Graph, arguments) -> None:
    done = reconstructed()
    lines = [f"{f.address:#010x}\n" for f in graph.functions if f.address not in done and f.size]
    if arguments.output:
        os.makedirs(os.path.dirname(os.path.abspath(arguments.output)), exist_ok=True)
        with open(arguments.output, "w", encoding="ascii") as output:
            output.writelines(lines)
        print(f"fn: {len(lines)} addresses in {arguments.output}", file=sys.stderr)
    else:
        sys.stdout.writelines(lines)


# ---------------------------------------------------------------------------------------------------------------
# where: what the checkout's src/ and gen/ say about exe addresses and src names, read from the files as they are now
# (no index, no cache: a porter's worktree changes every minute)
# ---------------------------------------------------------------------------------------------------------------

WHERE_GLOBS = ("*.h", "*.cpp", "*.inl", "*.hpp")
# comments, string and character literals and preprocessor lines, blanked before code is read
BLANKABLE = re.compile(r"//[^\n]*|/\*.*?\*/|\"(?:\\.|[^\"\\\n])*\"|'(?:\\.|[^'\\\n])*'|^[ \t]*#(?:[^\n\\]|\\.)*",
                       re.S | re.M)
NOT_NEWLINE = re.compile(r"[^\n]")
# MSVC's and the PDB's integer names as src spells them (`.omp/RULES.md` "Types from the PDB"), for comparing names
INTEGER_NAMES = re.compile(r"\b(?:unsigned\s+__int64|__int64|unsigned\s+(?:int|long|short|char)|signed\s+char|short|"
                           r"long|int|unsigned)\b")
FIXED_WIDTH = {"unsigned __int64": "uint64_t", "__int64": "int64_t", "unsigned int": "uint32_t",
               "unsigned long": "uint32_t", "unsigned short": "uint16_t", "unsigned char": "uint8_t",
               "signed char": "int8_t", "short": "int16_t", "long": "int32_t", "int": "int32_t",
               "unsigned": "uint32_t"}
TAG_KEYWORDS = re.compile(r"\b(?:class|struct|union|enum)\s+(?=[A-Za-z_])")
PUNCTUATION_SPACE = re.compile(r" (?=\W)|(?<=\W) ")
ACCESS_LABELS = re.compile(r"^(?:(?:public|protected|private)\s*:(?!:)\s*)+")
MARKER = re.compile(r"\b(EXE-INLINED|EXE-CALLED|SWEEP-INLINE)\s+$")
FOLD = re.compile(r"Same function as (.+?) \((0x[0-9A-Fa-f]+)\),?\s+registered\s+there")
DECLARING = ("FNVD_GLOBAL", "FNVD_STATIC_MEMBER", "FNVD_VFTABLE", "FNVD_ORIGINAL_FUNCTION")
# default caps of a block's sections (--more lifts them)
WHERE_CAPS = {"places": 12, "mentions": 3, "markers": 4, "symbols": 2, "functions": 1, "decls": 4, "defs": 4,
              "users": 8, "tables": 4, "lines": 40, "text": 150}
WHERE_HELP = """\
One block per ARG, read from the src/ and gen/ of the checkout holding the working directory (run it from your
worktree; the files as they are now, no index or cache). A section with nothing found says `none`.

ARG 0x...: an exe address (any case, any zero padding).
  exe     IDA's name; PDB: the PDB function among IDA's and src's names (names.py's index) and its source file;
          size; `ret N` (exefacts.tail_pops); exefacts.trivial_accessor and empty_function; the exe vtable slots
          holding it (base-most first, as `show`).
  src     every place naming it, `file:line  kind  text`: registration (FNVD_REPLACES*, FNVD_ADDRESS), forwarder
          (a fnvd::original:: call: the enclosing function's signature line), fold (a `Same function as ...
          registered there.` comment: the declaration below it), declaration (FNVD_GLOBAL, FNVD_STATIC_MEMBER,
          FNVD_VFTABLE, FNVD_ORIGINAL_FUNCTION), marker (EXE-INLINED/EXE-CALLED/SWEEP-INLINE: a count per file),
          mention (any other text, capped).
  then per src function those places name: as for a Class::member ARG, without users.
ARG Class::member (or Namespace::function; Instance<Args>::member):
  declared    the class body's declaration and namespace-scope ones (explicit specializations), up to 4 comment
              lines above each
  defined     each definition's lines and its body: a marked emitted body (the block's lines), the #else
              forwarder of one, a forwarder, or a clean body
  registered  the registrations naming it (through a class alias too) and their address
  users       code lines naming it qualified, a count per file; and the `.member(`/`->member(` calls on any type
ARG Class or Instance<Args> (as src spells it; `unsigned int` and `uint32_t` compare equal):
  the class or template head and its lines, the FNVD_LAYOUT(sizeof(...)) pin, the FNVD_VFTABLE constants (named
  Class_vftable*, or holding an exe vtable whose RTTI or IDA name is the instance), the explicit specializations
  declared for an instance with the address of their registration or fold comment, its aliases, its users.
ARG name (a global, static member, free function or member of any class): its FNVD_GLOBAL-style declarations,
  each function or member of that name as above, and the code lines inside function bodies naming it.

Names are matched as written in code (comments and strings are not code); calls through an object are not resolved
by type. Example: env/venv/bin/python harness/tools/fn.py where 0x559A40 NiRTTI::Construct 'NiPointer<NiNode>'"""


def checkout_root() -> str:
    """The checkout holding the working directory (a worktree's own src/ and gen/), else this script's."""
    here = os.getcwd()
    while True:
        if os.path.exists(os.path.join(here, ".git")) and os.path.isdir(os.path.join(here, "src")):
            return here
        parent = os.path.dirname(here)
        if parent == here:
            return ROOT
        here = parent


def spelling(text: str) -> str:
    """The text in one spelling for comparing names: `class `/`struct ` dropped, integer names fixed-width, no
    whitespace next to punctuation (`NiTArray<unsigned int, X>` and `NiTArray<uint32_t,X>` compare equal)."""
    text = INTEGER_NAMES.sub(lambda m: FIXED_WIDTH[" ".join(m.group().split())], TAG_KEYWORDS.sub("", text))
    return PUNCTUATION_SPACE.sub("", " ".join(text.split()))


def exe_spelling(text: str) -> str:
    """spelling() of a name as llvm-undname writes it (`char const *`)."""
    return spelling(re.sub(r"\b([A-Za-z_]\w*) const\b", r"const \1", text))


def occurs(needle: str, hay: str) -> bool:
    """needle in hay as a whole name (not part of a longer identifier or a longer qualified name)."""
    start = hay.find(needle)
    while start >= 0:
        end = start + len(needle)
        if (start == 0 or not (hay[start - 1].isalnum() or hay[start - 1] in "_:")) and \
                (end == len(hay) or not (hay[end].isalnum() or hay[end] == "_")):
            return True
        start = hay.find(needle, start + 1)
    return False


def hex_pattern(address: int) -> str:
    """An address as src writes it, any case, any zero padding (ERE: rg and grep -E read it as Python does)."""
    digits = "".join(f"[{c}{c.upper()}]" if c.isalpha() else c for c in f"{address:x}")
    return rf"\b0[xX]0*{digits}\b"


def identifier_of(name: str) -> str:
    """The last identifier of a name (`operator` for an operator), for the grep that finds candidate lines."""
    words = re.findall(r"[A-Za-z_]\w*", strip_template_arguments(name).split("::")[-1])
    return "operator" if "operator" in words else (words[-1] if words else "")


def strip_template_arguments(name: str) -> str:
    while True:
        stripped = re.sub(r"<[^<>]*>", "", name)
        if stripped == name:
            return name
        name = stripped


def declarator_column(raw: str, name: str) -> int:
    """Where a function's name stands in its head's text: `operator`, else the first `name (` not inside a longer
    identifier; the head's first non-blank character when neither is found."""
    start = raw.find(name)
    while start >= 0:
        before = raw[start - 1] if start else " "
        after = raw[start + len(name):].lstrip()
        if not (before.isalnum() or before == "_") and (name == "operator" or after.startswith("(")):
            return start
        start = raw.find(name, start + 1)
    return len(raw) - len(raw.lstrip())


def trailing_declarator(before: str) -> str:
    """The qualified name that ends a declaration's text before its parameter list (`T* FNVD_THISCALL A<B, C>::f`
    -> `A<B, C>::f`; `A::operator T*`)."""
    match = None
    for match in re.finditer(r"\boperator\b", before):
        pass
    end = match.start() if match else len(before)
    depth, j = 0, end
    while j > 0:
        c = before[j - 1]
        if c == ">":
            depth += 1
        elif c == "<":
            if depth == 0:
                break
            depth -= 1
        elif depth == 0 and not (c.isalnum() or c in "_:~"):
            break
        j -= 1
    return before[j:].strip()


@dataclass
class Scope:
    """A brace pair of a file: a namespace, a class, a function body, or another block (inside functions, every
    brace pair is a block)."""
    open: int
    close: int
    kind: str  # ns | class | func | block
    _name: str = ""  # a namespace's or class's name (without template arguments), a function's declarator
    head: int = 0  # offset where its head starts (after the previous statement)
    _line: int = 0  # a class's head line, a function's declarator line
    # a function's file until its name is read (most functions' names are never needed)
    source: SrcFile | None = None

    @property
    def name(self) -> str:
        if self.source:
            self.source.name_function(self)
        return self._name

    @property
    def line(self) -> int:
        if self.source:
            self.source.name_function(self)
        return self._line


class SrcFile:
    """A file of the checkout as it is now: its lines, its code with comments, literals and preprocessor lines
    blanked (offsets kept), its scopes and its emitted blocks."""

    def __init__(self, root: str, rel: str):
        self.rel = rel
        with open(os.path.join(root, rel), encoding="utf-8", errors="replace") as handle:
            self.text = handle.read()
        self.lines = self.text.split("\n")
        self.starts = [0, *itertools.accumulate(len(line) + 1 for line in self.lines[:-1])]
        self._code: str | None = None
        self._scopes: list[Scope] | None = None
        self._blocks: list[tuple[int, int, int]] | None = None
        self._aliases: dict[str, str] | None = None

    def offset(self, line: int) -> int:
        return self.starts[line - 1]

    def line_of(self, offset: int) -> int:
        return bisect.bisect_right(self.starts, offset)

    def code_line(self, line: int) -> str:
        """The line with comments, literals and preprocessor text blanked: from the line alone when no block comment,
        continued line or raw string reaches into it (blanking whole files is the slow part of a run), else from the
        file's code."""
        if self._code is None:
            start = self.starts[line - 1]
            if self.text.rfind("/*", 0, start) <= self.text.rfind("*/", 0, start) and 'R"' not in self.text and \
                    not (line > 1 and self.lines[line - 2].endswith("\\")):
                return BLANKABLE.sub(lambda m: " " * len(m.group()), self.lines[line - 1])
        return self.code[self.starts[line - 1]: self.starts[line] - 1 if line < len(self.starts) else None]

    @property
    def code(self) -> str:
        if self._code is None:
            self._code = BLANKABLE.sub(lambda m: NOT_NEWLINE.sub(" ", m.group()) if "\n" in m.group()
                                       else " " * len(m.group()), self.text)
        return self._code

    @property
    def scopes(self) -> list[Scope]:
        """Every brace pair, by opening offset."""
        if self._scopes is None:
            code, out, stack, after = self.code, [], [], 0
            for match in re.finditer(r"[{}]", code):
                i = match.start()
                if code[i] == "{":
                    if not stack or stack[-1].kind in ("ns", "class"):
                        # the head starts after the previous brace or `;` of the namespace or class
                        stack.append(self._scope(max(after, code.rfind(";", after, i) + 1), i))
                    else:
                        stack.append(Scope(i, len(code), "block", head=i))
                elif stack:
                    scope = stack.pop()
                    scope.close = i
                    out.append(scope)
                after = i + 1
            out.extend(stack)
            self._scopes = sorted(out, key=lambda s: s.open)
        return self._scopes

    def _scope(self, start: int, open_: int) -> Scope:
        import srcindex
        raw = self.code[start:open_]
        head = ACCESS_LABELS.sub("", " ".join(raw.split()))
        plain = re.sub(r"^template\s*<.*?>\s*(?=(?:class|struct|union|[A-Za-z_]))", "", head)
        end = len(self.code)
        if re.search(r"(?:^|\s)namespace(?:\s+[\w:]+)?$", plain) or plain == "extern":
            name = re.search(r"namespace\s+([\w:]+)$", plain)
            return Scope(open_, end, "ns", name.group(1) if name else "", start)
        if re.search(r"(?:^|\s)enum\b", plain) and "(" not in plain:
            return Scope(open_, end, "block", "", start)
        if "(" not in re.sub(r"(?:alignas|__declspec)\s*\([^)]*\)", "", plain).split(":")[0] and "=" not in plain \
                and (match := srcindex.CLASS_HEAD.search(plain)):
            return Scope(open_, end, "class", re.sub(r"<.*", "", match.group(2)), start,
                         self.line_of(start + len(raw) - len(raw.lstrip())))
        if "(" in plain and not re.match(r"^(?:\w+\s+)*\w+(?:\s*\[[^\]]*\])*\s*=", plain) and not plain.endswith("="):
            return Scope(open_, end, "func", "", start, 0, self)
        return Scope(open_, end, "block", "", start)

    def name_function(self, scope: Scope) -> None:
        """A function's declarator and the line it stands on."""
        import srcindex
        scope.source = None
        raw = self.code[scope.head:scope.open]
        plain = re.sub(r"^template\s*<.*?>\s*(?=[A-Za-z_])", "", ACCESS_LABELS.sub("", " ".join(raw.split())))
        span = srcindex._param_list(plain)
        scope._name = trailing_declarator(plain[:span[0]].rstrip()) if span else ""
        last = strip_template_arguments(scope._name).split("::")[-1].lstrip("~")
        column = declarator_column(raw, "operator" if last.startswith("operator") else last) if last else \
            len(raw) - len(raw.lstrip())
        scope._line = self.line_of(scope.head + column)

    def enclosing(self, offset: int) -> list[Scope]:
        """The scopes holding offset (inside their braces), outermost first."""
        out = []
        for scope in self.scopes:
            if scope.open >= offset:
                break
            if offset < scope.close:
                out.append(scope)
        return out

    def qualified(self, scope: Scope) -> str:
        """A function's or class's name with the namespaces and classes around it."""
        outer = "::".join(s.name for s in self.enclosing(scope.open) if s.kind in ("ns", "class") and s.name)
        return f"{outer}::{scope.name}" if outer and not scope.name.startswith(outer + "::") else scope.name

    def function_at(self, offset: int) -> Scope | None:
        """The function whose head (offset in its declaration, before the body) holds offset."""
        for scope in self.scopes:
            if scope.head > offset:
                break
            if scope.kind == "func" and scope.head <= offset < scope.open:
                return scope
        return None

    @property
    def blocks(self) -> list[tuple[int, int, int]]:
        """The emitted blocks: (#if line, #else line or 0, #endif line)."""
        if self._blocks is None:
            import srcindex
            self._blocks, start, middle = [], 0, 0
            for number, line in enumerate(self.lines, 1):
                stripped = line.strip()
                if stripped == srcindex.EMITTED_IF:
                    start, middle = number, 0
                elif start and stripped == "#else" and not middle:
                    middle = number
                elif start and stripped == srcindex.EMITTED_END:
                    self._blocks.append((start, middle, number))
                    start = 0
        return self._blocks

    @property
    def aliases(self) -> dict[str, str]:
        """`using A = T;` and `typedef T A;` of the file: A -> T."""
        if self._aliases is None:
            self._aliases = {m.group(1): " ".join(m.group(2).split())
                             for m in re.finditer(r"\busing\s+(\w+)\s*=\s*([^;{}]+);", self.code)}
            self._aliases.update({m.group(2): " ".join(m.group(1).split())
                                  for m in re.finditer(r"\btypedef\s+([^;{}()]+?)\s+(\w+)\s*;", self.code)})
        return self._aliases

    def comments_above(self, line: int, limit: int) -> list[str]:
        """Up to limit comment lines directly above a line (above its `template <...>` line)."""
        k = line - 2
        if k >= 0 and self.lines[k].strip().startswith("template"):
            k -= 1
        out: list[str] = []
        while k >= 0 and len(out) < limit and self.lines[k].strip().startswith("//"):
            out.insert(0, self.lines[k].strip())
            k -= 1
        return out

    def comment_block(self, line: int) -> tuple[int, int]:
        """The first and last line of the run of `//` lines holding line."""
        first = last = line
        while first > 1 and self.lines[first - 2].strip().startswith("//"):
            first -= 1
        while last < len(self.lines) and self.lines[last].strip().startswith("//"):
            last += 1
        return first, last

    def call_at(self, offset: int) -> tuple[str, int] | None:
        """The function or macro whose call's parentheses hold offset (`fnvd::original::thiscall`, `FNVD_REPLACES`),
        with the offset of its `(`."""
        code, depth, i = self.code, 0, offset - 1
        while i >= max(0, offset - 4000):
            c = code[i]
            if c == ")":
                depth += 1
            elif c == "(":
                if depth == 0:
                    break
                depth -= 1
            elif c in ";{}" and depth == 0:
                return None
            i -= 1
        else:
            return None
        j = i - 1
        while j >= 0 and code[j].isspace():
            j -= 1
        if j >= 0 and code[j] == ">":
            depth = 0
            while j >= 0:
                depth += code[j] == ">"
                depth -= code[j] == "<"
                j -= 1
                if depth == 0:
                    break
            while j >= 0 and code[j].isspace():
                j -= 1
        k = j
        while k >= 0 and (code[k].isalnum() or code[k] in "_:"):
            k -= 1
        return code[k + 1:j + 1], i

    def arguments(self, open_: int) -> list[str]:
        """The top-level arguments of the call whose `(` is at open_, whitespace collapsed."""
        code, depth = self.code, 0
        for j in range(open_, len(code)):
            depth += code[j] == "("
            depth -= code[j] == ")"
            if depth == 0:
                import srcindex
                return [" ".join(a.split()) for a in srcindex._split_top(code[open_ + 1:j])]
        return []

    def statement_head(self, offset: int) -> tuple[str, str]:
        """The code from offset to the first `;` or `{` outside parentheses, and that character."""
        code, depth = self.code, 0
        for j in range(offset, min(len(code), offset + 4000)):
            depth += code[j] == "("
            depth -= code[j] == ")"
            if depth == 0 and code[j] in ";{":
                return code[offset:j], code[j]
        return code[offset:], ""

    def body_kind(self, scope: Scope) -> str:
        """A function body: inside a marked emitted block, the #else forwarder of one, a forwarder, or clean."""
        for start, middle, end in self.blocks:
            if start <= scope.line <= end:
                if middle and scope.line > middle:
                    return f"forwarder (the #else of the emitted block {start}-{end})"
                return f"marked emitted body (block {start}-{end})"
        body = " ".join(self.code[scope.open + 1:scope.close].split())
        if body.count(";") == 1 and "fnvd::original::" in body:
            target = HEX.search(body)
            return "forwarder" + (f" to {target.group()}" if target else "")
        return "clean body"


def where_grep(root: str, patterns: list[str]) -> dict[str, dict[int, str]]:
    """The lines of the checkout's src/ and gen/ that match any pattern: file -> line -> text. rg when there is one,
    else grep -E (the patterns are written for both)."""
    import shutil
    import subprocess
    folders = [f for f in ("src", "gen") if os.path.isdir(os.path.join(root, f))]
    if not patterns or not folders:
        return {}
    expressions = [part for pattern in patterns for part in ("-e", pattern)]
    rg = shutil.which("rg") or next((p for p in ("/usr/bin/rg", "/usr/local/bin/rg") if os.access(p, os.X_OK)), None)
    if rg:
        command = [rg, "--no-config", "--no-ignore", "--null", "-nH", "--no-heading", "--color=never"]
        command += [part for glob_ in WHERE_GLOBS for part in ("-g", glob_)]
    else:
        command = [shutil.which("grep") or "/usr/bin/grep", "-rnHZE"] + [f"--include={g}" for g in WHERE_GLOBS]
    done = subprocess.run(command + expressions + ["--"] + folders, cwd=root, capture_output=True)
    if done.returncode > 1:
        sys.exit(f"fn: {os.path.basename(command[0])} failed: {done.stderr.decode(errors='replace').strip()}")
    found: dict[str, dict[int, str]] = defaultdict(dict)
    for raw in done.stdout.split(b"\n"):
        path, separator, rest = raw.partition(b"\0")
        number, _, text = rest.partition(b":")
        if separator and number.isdigit():
            found[path.decode(errors="replace")][int(number)] = text.decode("utf-8", errors="replace")
    return found


class Where:
    """One `where` run: the checkout, the files read so far and the lines the greps found."""

    def __init__(self, root: str, graph: Graph, arguments):
        self.root, self.graph = root, graph
        self.caps = defaultdict(lambda: 1 << 30) if arguments.more else WHERE_CAPS
        self.list_users = arguments.users
        self.hits: dict[str, dict[int, str]] = defaultdict(dict)
        self.searched: set[str] = set()
        self._files: dict[str, SrcFile] = {}
        self._flat: list[tuple[str, int, str]] | None = None
        self._pdb = None
        self._pdb_prefixed: dict[str, list[str]] = {}
        self._table: list[dict] | None = None

    def table(self) -> list[dict]:
        """The checkout's harness/addresses.tsv rows ([] while it does not exist)."""
        if self._table is None:
            self._table = address_table(self.root)
        return self._table

    def search(self, patterns: list[str]) -> None:
        fresh = [p for p in dict.fromkeys(patterns) if p not in self.searched]
        self.searched.update(fresh)
        for rel, lines in where_grep(self.root, fresh).items():
            self.hits[rel].update(lines)
            self._flat = None

    def file(self, rel: str) -> SrcFile:
        if rel not in self._files:
            self._files[rel] = SrcFile(self.root, rel)
        return self._files[rel]

    def matching(self, pattern: str, needle: str = "") -> list[tuple[str, int, str]]:
        """The found lines that match pattern (and hold needle, a quick test first), by file and line."""
        if self._flat is None:
            self._flat = [(rel, number, text) for rel in sorted(self.hits)
                          for number, text in sorted(self.hits[rel].items())]
        regex = re.compile(pattern)
        return [row for row in self._flat if needle in row[2] and regex.search(row[2])]

    def clip(self, text: str) -> str:
        text = " ".join(text.split())
        cap = self.caps["text"]
        return text if len(text) <= cap else text[:cap - 3] + "..."

    def pdb_name(self, names: list[str]) -> str:
        """The first of names that is a PDB function (names.py's index; template arguments compared in one
        spelling), with its PDB source file."""
        if self._pdb is None:
            import pickle as pickle_
            import names as names_

            class Unpickler(pickle_.Unpickler):
                # names.py pickles its index as a script: its classes are __main__'s
                def find_class(self, module, name):
                    if module == "__main__" and hasattr(names_, name):
                        return getattr(names_, name)
                    return super().find_class(module, name)

            index = None
            if os.path.exists(names_.INDEX):
                with open(names_.INDEX, "rb") as handle:
                    index = Unpickler(handle).load()
            if index is None or index.version != names_.INDEX_VERSION:
                index = names_.load()
            self._pdb = (names_, index)
        names_, index = self._pdb
        for name in names:
            found = name if name in index.functions else None
            if not found and "<" in name:
                key, prefix = exe_spelling(name), name.split("<", 1)[0] + "<"
                if prefix not in self._pdb_prefixed:
                    self._pdb_prefixed[prefix] = [f for f in index.functions if f.startswith(prefix)]
                found = next((f for f in self._pdb_prefixed[prefix] if exe_spelling(f) == key), None)
            if found:
                return f"{found} ({names_.source_of(index, found) or 'no source'})"
        return "none"


def class_def_pattern(base: str) -> str:
    return rf"\b(class|struct|union)\b[^;(]*\b{base}\b"


def registration_symbol(call: str, arguments: list[str], aliases: dict[str, str]) -> str:
    """The src function a registration (`FNVD_REPLACES*`, `FNVD_ADDRESS`) names, its class alias resolved through
    aliases (the file's)."""
    form = call[len("FNVD_REPLACES"):].lstrip("_") if call.startswith("FNVD_REPLACES") else ""
    if len(arguments) < 2:
        return ""
    if form in ("VIRTUAL", "VIRTUAL_OVERLOAD", "MEMBER") and len(arguments) > 2:
        symbol = f"{arguments[1]}::{arguments[2]}"
    elif form.startswith("VIRTUAL_VIA") and len(arguments) > 3:
        symbol = f"{arguments[1]}::{arguments[3]}"
    elif form == "CONSTRUCTOR":
        symbol = f"{arguments[1]}::{strip_template_arguments(arguments[1]).split('::')[-1]}"
    elif form == "DESTRUCTOR":
        symbol = f"{arguments[1]}::~{strip_template_arguments(arguments[1]).split('::')[-1]}"
    else:
        symbol = arguments[1]
    owner = class_of(symbol)
    if owner in aliases:
        symbol = aliases[owner] + symbol[len(owner):]
    return symbol


REGISTRATION_LINE = re.compile(r"^\s*(FNVD_REPLACES\w*|FNVD_ADDRESS)\s*\(")


def line_registration(text: str) -> tuple[str, list[str]] | None:
    """A registration written on one line (its macro and arguments), from the line's text alone."""
    match = REGISTRATION_LINE.match(text)
    if not match:
        return None
    depth = 0
    for j in range(match.end() - 1, len(text)):
        depth += text[j] == "("
        depth -= text[j] == ")"
        if depth == 0:
            import srcindex
            return match.group(1), [" ".join(a.split()) for a in srcindex._split_top(text[match.end():j])]
    return None


def class_scopes(where: Where, owner: str) -> list[tuple[SrcFile, Scope]]:
    """The class bodies of owner (a class or a template instance): an explicit specialization spelled as owner,
    else the template's (or class's) own."""
    base = strip_template_arguments(owner)
    last = base.split("::")[-1]
    found = []
    for rel, number, _ in where.matching(class_def_pattern(re.escape(last)), last):
        source = where.file(rel)
        for scope in source.scopes:
            if scope.kind == "class" and scope.line <= number <= source.line_of(scope.open) and \
                    (scope.name == last or scope.name.endswith("::" + last)) and \
                    spelling(source.qualified(scope)).endswith(spelling(base)):
                found.append((source, scope))
    exact, primary = [], []
    for source, scope in found:
        head = " ".join(source.code[scope.head:scope.open].split())
        spelled = re.search(rf"\b{re.escape(last)}\s*<", head)
        if spelled and "<" in owner and occurs(spelling(owner.split("::")[-1]), spelling(head)):
            exact.append((source, scope))
        elif not spelled:
            primary.append((source, scope))
    return exact or primary


def member_lines(source: SrcFile, scope: Scope, member: str) -> list[int]:
    """The lines of a class body that name member outside nested braces (its declarations)."""
    ident = identifier_of(member)
    if not ident:
        return []
    pattern = re.compile(r"\boperator\s*" + re.escape(member[len("operator"):].strip()) if ident == "operator" else
                         r"(?<![\w])" + re.escape(member) + r"\b")
    inner = [s for s in source.scopes if scope.open < s.open < scope.close]
    out, called = [], []
    for match in pattern.finditer(source.code, scope.open + 1, scope.close):
        at = match.start()
        if not any(s.open < at < s.close for s in inner):
            number = source.line_of(at)
            if number not in out:
                out.append(number)
            if source.code[match.end():match.end() + 200].lstrip().startswith("(") and number not in called:
                called.append(number)
    # a function's declarations are the lines naming it before a `(` (a constructor's name is also its class's)
    return called or out


def describe_function(where: Where, symbol: str, users: bool, indent: str, follow: bool = True) -> list[str]:
    """A src function's or member's declarations, definitions with what their bodies are, registrations and (users)
    its users, as indented lines."""
    owner = class_of(symbol)
    member = symbol[len(owner) + 2:] if owner else symbol
    wanted = spelling(symbol)
    ident = identifier_of(member)
    caps = where.caps
    declarations, definitions, registrations, uses = [], [], [], []
    seen: set[tuple[str, int]] = set()

    def declared(source: SrcFile, number: int, note: str) -> None:
        if (source.rel, number) not in seen:
            seen.add((source.rel, number))
            declarations.append((source, number, note))

    def defined(source: SrcFile, scope: Scope) -> None:
        if (source.rel, -scope.line) not in seen:
            seen.add((source.rel, -scope.line))
            definitions.append((source, scope))

    # its greps (no-ops when its query ran them already)
    where.search(function_patterns(symbol))
    classes = class_scopes(where, owner) if owner else []
    # a namespace's function (or a free one): the lines naming the member, in the files opening that namespace
    in_namespace = not classes and ident
    namespace_files: set[str] = set()
    if in_namespace:
        last = owner.split("::")[-1] if owner else ""
        where.search([rf"\b{ident}\b"] + ([rf"\bnamespace\s+{last}\b"] if last else []))
        namespace_files = {rel for rel, _, _ in where.matching(rf"\bnamespace\s+{last}\b", last)} if last else set()
    # the class's aliases (`using X = Owner;`), which registrations name
    aliases: set[str] = set()
    if owner:
        last = strip_template_arguments(owner).split("::")[-1]
        for _, _, text in where.matching(rf"\b(using|typedef)\b.*\b{last}\b", last):
            using = re.search(r"\busing\s+(\w+)\s*=\s*([^;]+);", text)
            typedef = re.search(r"\btypedef\s+([^;]+?)\s+(\w+)\s*;", text)
            name, target = using.groups() if using else typedef.groups()[::-1] if typedef else ("", "")
            if name and spelling(target) == spelling(owner):
                aliases.add(name)
    # namespace-scope declarations (explicit specializations), out-of-line definitions, registrations, users
    pattern = rf"\b{ident}\b" if in_namespace else candidate_pattern(symbol)
    for rel, number, text in where.matching(pattern, ident) if pattern else []:
        if REGISTRATION_LINE.match(text):
            # a registration of this function names its class or an alias of it (one on a line, checked quickly)
            if owner and text.rstrip().endswith(";") and strip_template_arguments(owner).split("::")[-1] not in text \
                    and not any(alias in text for alias in aliases):
                continue
            found = line_registration(text)
            if not found:
                # its arguments go on over the next lines
                source = where.file(rel)
                start = REGISTRATION_LINE.match(text)
                found = start.group(1), source.arguments(source.offset(number) + start.end() - 1)
            call, arguments = found
            registered = registration_symbol(call, arguments, {})
            if spelling(registered) != wanted and class_of(registered) in aliases:
                registered = registration_symbol(call, arguments, where.file(rel).aliases)
            if arguments and spelling(registered) == wanted:
                registrations.append((rel, number, arguments[0], text))
            continue
        if not occurs(wanted, spelling(text)) and rel not in namespace_files:
            continue
        source = where.file(rel)
        code = source.code_line(number)
        if not code.strip():
            continue
        at = source.offset(number) + max(code.rfind(ident), 0)
        scopes = source.enclosing(at)
        outer = "::".join(s.name for s in scopes if s.kind in ("ns", "class") and s.name)
        if not occurs(wanted, spelling(code)) and not (
                in_namespace and spelling(f"{outer}::{member}" if outer else member) == wanted):
            continue
        function = source.function_at(at)
        if function:
            if spelling(source.qualified(function)) == wanted:
                defined(source, function)
                continue
        spelled = spelling(code)
        named = wanted if occurs(wanted, spelled) else spelling(member)
        # a declaration names it as its declarator (before its parameters, initializer or end), not as an argument
        declares = re.search(re.escape(named) + r"(?!\w)[(=;\[{]", spelled) and \
            not re.match(r"\s*(using|typedef)\b", code)
        if any(s.kind in ("func", "block") for s in scopes) or not declares:
            uses.append((rel, number))
        elif not any(s.kind == "class" for s in scopes):
            declared(source, number, f"in namespace {outer}" if in_namespace and outer else "at namespace scope")
    # the class body's declarations and in-class definitions (a template's, for an instance)
    for source, scope in classes:
        for number in member_lines(source, scope, member):
            function = next((s for s in source.scopes if s.kind == "func" and s.line == number and
                             scope.open < s.open < scope.close), None)
            if function:
                defined(source, function)
            declared(source, number, f"in class {source.qualified(scope)}")
    # a definition in a header with no declaration of its own declares the function
    if not declarations:
        for source, scope in definitions:
            if source.rel.endswith((".h", ".hpp", ".inl")):
                declared(source, scope.line, "by its definition")
    lines = []
    for source, number, note in declarations[:caps["decls"]]:
        lines.append(f"{indent}declared    {source.rel}:{number}  {where.clip(source.lines[number - 1])}  ({note})")
        lines += [f"{indent}              {where.clip(c)}" for c in source.comments_above(number, 4)]
    if len(declarations) > caps["decls"]:
        lines.append(f"{indent}declared    ... {len(declarations) - caps['decls']} more (--more)")
    if not declarations:
        lines.append(f"{indent}declared    none")
    for source, scope in definitions[:caps["defs"]]:
        end = source.line_of(scope.close)
        inside = [s for s in source.enclosing(scope.open) if s.kind == "class"]
        note = f" (in class {source.qualified(inside[-1])})" if inside else ""
        lines.append(f"{indent}defined     {source.rel}:{scope.line}-{end}  {source.body_kind(scope)}{note}")
    if len(definitions) > caps["defs"]:
        lines.append(f"{indent}defined     ... {len(definitions) - caps['defs']} more (--more)")
    if not definitions:
        lines.append(f"{indent}defined     none")
    for rel, number, address, text in registrations:
        lines.append(f"{indent}registered  {address}  {rel}:{number}  {where.clip(text)}")
    # a registration right below a definition that names it otherwise (through a derived class: `FNVD_REPLACES(...,
    # NiTPrimitiveArray<const char*>::AddFirstEmpty)` below `NiTArray<const char*, ...>::AddFirstEmpty`)
    for source, scope in definitions if not registrations else []:
        end = source.line_of(scope.close)
        for number in range(end + 1, min(end + 4, len(source.lines) + 1)):
            found = line_registration(source.lines[number - 1])
            if found and len(found[1]) > 1 and identifier_of(registration_symbol(*found, {})) == ident:
                registrations.append((source.rel, number, found[1][0], source.lines[number - 1]))
                lines.append(f"{indent}registered  {found[1][0]}  {source.rel}:{number}  "
                             f"{where.clip(source.lines[number - 1])}  (right below the definition at "
                             f"{scope.line}, naming it otherwise)")
                break
    # the registrations that left src for harness/addresses.tsv (addresses.py export)
    tabled = []
    if not registrations and where.table():
        import addresses
        tabled = [r for r in where.table() if r["origin"] == "src" and (r["macro"].startswith("FNVD_REPLACES") or
                  r["macro"] == "FNVD_ADDRESS") and spelling(addresses.plain(r["name"])) == wanted]
        for r in tabled:
            lines.append(f"{indent}registered  0x{r['address']:X}  {r['file']}:{r['line']}  {r['macro']} "
                         f"{r['name']}  (harness/addresses.tsv)")
    if not registrations and not tabled:
        lines.append(f"{indent}registered  none")
    # a registration naming the function otherwise than its definition does (a derived class): the definition right
    # above the registration, described under its own name
    for rel, number, _, _ in registrations if follow and not definitions else []:
        source = where.file(rel)
        above = [s for s in source.scopes if s.kind == "func" and number - 4 <= source.line_of(s.close) < number]
        if above and spelling(source.qualified(above[-1])) != wanted:
            named = source.qualified(above[-1])
            lines.append(f"{indent}defined right above the registration as {named}:")
            lines += describe_function(where, named, False, indent + "  ", follow=False)
    if users:
        lines += users_lines(where, uses, "users (code naming it qualified)", indent)
        if owner and ident and ident != "operator":
            calls = [(rel, number) for rel, number, text in where.matching(rf"(\.|->)\s*{ident}\s*\(", ident)
                     if re.search(rf"(\.|->)\s*{ident}\s*\(", where.file(rel).code_line(number))]
            lines += users_lines(where, calls, f"calls `.{ident}(`/`->{ident}(` (any receiver type, not resolved)",
                                 indent)
    return lines


def users_lines(where: Where, uses: list[tuple[str, int]], title: str, indent: str) -> list[str]:
    """A count per file of uses (the most first), or with --users the lines."""
    if not uses:
        return [f"{indent}{title}: none"]
    per_file: dict[str, int] = defaultdict(int)
    for rel, _ in uses:
        per_file[rel] += 1
    ranked = sorted(per_file.items(), key=lambda item: (-item[1], item[0]))
    cap = where.caps["users"]
    lines = [f"{indent}{title}: {len(uses)} lines in {len(per_file)} files"]
    if where.list_users:
        cap = where.caps["lines"] * 5
        lines += [f"{indent}  {rel}:{number}  {where.clip(where.file(rel).lines[number - 1])}"
                  for rel, number in uses[:cap]]
        if len(uses) > cap:
            lines.append(f"{indent}  ... {len(uses) - cap} more (--more)")
        return lines
    lines += [f"{indent}  {count:4}  {rel}" for rel, count in ranked[:cap]]
    if len(ranked) > cap:
        lines.append(f"{indent}  ... {len(ranked) - cap} more files (--more)")
    return lines


def candidate_pattern(symbol: str) -> str | None:
    """The lines that may declare, define, register or use a function: a qualified one's naming its class and the
    member, or a registration naming the member (through a class alias); else the lines naming it."""
    owner, ident = class_of(symbol), identifier_of(symbol)
    if not ident:
        return None
    if owner:
        last = strip_template_arguments(owner).split("::")[-1]
        return rf"\b{last}\b.*\b{ident}\b|FNVD_(REPLACES[A-Z_]*|ADDRESS)\s*\(.*\b{ident}\b"
    return rf"\b{ident}\b"


def function_patterns(symbol: str) -> list[str]:
    """The greps describe_function reads: candidate_pattern's lines, the class heads and the class's aliases."""
    owner, pattern = class_of(symbol), candidate_pattern(symbol)
    patterns = [pattern] if pattern else []
    if owner:
        last = strip_template_arguments(owner).split("::")[-1]
        patterns += [class_def_pattern(last), rf"\b(using|typedef)\b.*\b{last}\b"]
    return patterns


class AddressQuery:
    def __init__(self, text: str):
        self.text, self.address = text, int(text, 16)
        self.places: list[tuple[str, int, str, str]] = []  # file, line, kind, text
        self.markers: dict[str, dict[str, int]] = defaultdict(lambda: defaultdict(int))
        self.mentions: list[tuple[str, int, str]] = []
        self.symbols: list[str] = []
        self.registered: list[str] = []

    def patterns(self) -> list[str]:
        return [hex_pattern(self.address)]

    def collect(self, where: Where) -> list[str]:
        regex = re.compile(hex_pattern(self.address))
        seen = set()
        for rel, number, text in where.matching(hex_pattern(self.address)):
            source = where.file(rel)
            for match in regex.finditer(source.lines[number - 1]):
                place = self.classify(where, source, number, match.start())
                if place and place[:3] not in seen:
                    seen.add(place[:3])
                    self.places.append(place)
        # the records that left src for harness/addresses.tsv (addresses.py export): a place each, its name a symbol
        import addresses
        lines = {(rel, number) for rel, number, _, _ in self.places}
        for r in where.table():
            if r["address"] != self.address or (r["file"], r["line"]) in lines:
                continue
            name = addresses.plain(r["name"])
            if r["macro"].startswith("FNVD_REPLACES") or r["macro"] == "FNVD_ADDRESS":
                self.symbol(name)
                if r["origin"] == "src" and name not in self.registered:
                    self.registered.append(name)
            elif r["kind"] in ("original function", "import", "import as", "import stub"):
                self.symbol(name)
            cut = ", cut" if r["origin"] == "cut" else ""
            self.places.append((r["file"], r["line"], f"{r['kind']} (harness/addresses.tsv{cut})",
                                f"{r['macro']}: {r['name']}"))
        return [p for symbol in self.symbols for p in function_patterns(symbol)]

    def symbol(self, name: str) -> None:
        if name and name not in self.symbols:
            self.symbols.append(name)

    def classify(self, where: Where, source: SrcFile, number: int, column: int):
        at = source.offset(number) + column
        text = source.lines[number - 1]
        if source.code_line(number)[column] == " ":
            # in a comment (or a string)
            if marker := MARKER.search(text[:column]):
                self.markers[source.rel][marker.group(1)] += 1
                return None
            first, last = source.comment_block(number)
            joined = " ".join(source.lines[k - 1].strip().lstrip("/").strip() for k in range(first, last + 1))
            for fold in FOLD.finditer(joined):
                if int(fold.group(2), 16) == self.address:
                    return self.fold(where, source, last + 1)
            self.mentions.append((source.rel, number, text))
            return None
        call = source.call_at(at)
        name = call[0] if call else ""
        if name.startswith("FNVD_REPLACES") or name == "FNVD_ADDRESS":
            symbol = registration_symbol(name, source.arguments(call[1]), source.aliases)
            self.symbol(symbol)
            if symbol and symbol not in self.registered:
                self.registered.append(symbol)
            return source.rel, number, "registration", text
        if name.startswith("fnvd::original::"):
            function = next((s for s in reversed(source.enclosing(at)) if s.kind == "func"), None)
            if function:
                self.symbol(source.qualified(function))
                return source.rel, function.line, "forwarder", source.lines[function.line - 1]
            return source.rel, number, "forwarder", text
        if name in DECLARING:
            return source.rel, number, "declaration", text
        self.mentions.append((source.rel, number, text))
        return None

    def fold(self, where: Where, source: SrcFile, number: int):
        """The declaration a fold comment sits above (its `template <>` line joined to the next)."""
        if number > len(source.lines) or not source.lines[number - 1].strip():
            return source.rel, number - 1, "fold", "(the comment has no declaration directly below)"
        head, _ = source.statement_head(source.offset(number))
        plain = re.sub(r"^template\s*<.*?>\s*", "", " ".join(head.split()))
        import srcindex
        span = srcindex._param_list(plain)
        declarator = trailing_declarator(plain[:span[0]].rstrip()) if span else ""
        line = number + 1 if re.fullmatch(r"\s*template\s*<.*>\s*", source.lines[number - 1]) else number
        if declarator:
            outer = "::".join(s.name for s in source.enclosing(source.offset(number)) if s.kind in ("ns", "class")
                              and s.name)
            self.symbol(f"{outer}::{declarator}" if outer and not declarator.startswith(outer + "::") else declarator)
        return source.rel, line, "fold", source.lines[line - 1]

    def render(self, where: Where) -> list[str]:
        caps = where.caps
        lines = [self.text, "  exe   " + self.exe_facts(where)]
        # one row per line: a fold comment above a forwarder names both
        merged: dict[tuple[str, int], tuple[list[str], str]] = {}
        for rel, number, kind, text in self.places:
            merged.setdefault((rel, number), ([], text))[0].append(kind)
        rows = [f"{rel}:{number}  {', '.join(kinds)}  {where.clip(text)}"
                for (rel, number), (kinds, text) in merged.items()]
        rows += [f"{rel}  marker  " + ", ".join(f"{count} {kind}" for kind, count in sorted(kinds.items()))
                 for rel, kinds in list(self.markers.items())[:caps["markers"]]]
        if len(self.markers) > caps["markers"]:
            rows.append(f"markers in {len(self.markers) - caps['markers']} more files (--more)")
        rows += [f"{rel}:{number}  mention  {where.clip(text)}" for rel, number, text in
                 self.mentions[:caps["mentions"]]]
        if len(self.mentions) > caps["mentions"]:
            rows.append(f"... {len(self.mentions) - caps['mentions']} more mentions (--more)")
        if len(rows) > caps["places"] + caps["markers"] + caps["mentions"] + 1:
            rows = rows[:caps["places"]] + [f"... {len(rows) - caps['places']} more (--more)"]
        lines += [("  src   " if i == 0 else "        ") + row for i, row in enumerate(rows or ["none"])]
        # the registered functions first: the others are its folds' and forwarders' declarations
        symbols = self.registered + [s for s in self.symbols if s not in self.registered]
        for symbol in symbols[:caps["functions"]]:
            lines.append(f"  {symbol}")
            lines += describe_function(where, symbol, users=False, indent="    ")
        if len(symbols) > caps["functions"]:
            lines.append(f"  ... {len(symbols) - caps['functions']} more functions named above (--more): "
                         + ", ".join(symbols[caps["functions"]:]))
        return lines

    def exe_facts(self, where: Where) -> str:
        graph, address = where.graph, self.address
        function = graph.containing(address)
        if not function or function.address != address:
            name = ida_names().get(address)
            inside = f"inside {function.name} {function.address:#x}+{address - function.address:#x}" if function \
                else "not in a function"
            return f"IDA {name or 'none'}; no function starts here ({inside})"
        import exefacts
        facts = [f"IDA {function.name}", "PDB " + where.pdb_name([function.name] + self.registered),
                 f"{function.size} bytes"]
        try:
            pops = exefacts.tail_pops(address)
            facts.append(f"ret {pops}" if pops is not None else "ret unknown")
            accessor = exefacts.trivial_accessor(address)
            if accessor:
                base = "this" if accessor["base"] == -1 else f"[esp+{accessor['base']:#x}]"
                what = "load" if accessor["load"] else "store" if accessor["store"] else "address"
                facts.append(f"trivial accessor: {what} {base}+{accessor['offset']:#x}"
                             + (f", {accessor['width']} bytes" if what != "address" else "")
                             + (" (x87)" if accessor["x87"] else ""))
            else:
                facts.append("not a trivial accessor")
            empty = exefacts.empty_function(address)
            facts.append(f"empty (ret {empty})" if empty is not None else "not empty")
        except exefacts.ExefactsError as error:
            facts.append(f"exefacts: {error}")
        tables = vtable_slots(address)
        if tables:
            cap = where.caps["tables"]
            listed = ", ".join(f"{start:#x}[{slot}] {name}" for start, slot, name, _ in tables[:cap])
            facts.append(f"vtables {listed}" + (f" +{len(tables) - cap} more" if len(tables) > cap else ""))
        else:
            facts.append("vtables none")
        return "; ".join(facts)


class NameQuery:
    def __init__(self, text: str):
        self.text = text
        self.owner = class_of(text)
        self.member = text[len(self.owner) + 2:] if self.owner else text
        self.ident = identifier_of(text)
        self.classes: list[tuple[SrcFile, Scope]] = []
        self.aliases: list[str] = []
        self.tables: list[int] = []

    def is_type(self) -> bool:
        return not self.owner and "<" in self.text or bool(self.classes)

    def patterns(self) -> list[str]:
        base = strip_template_arguments(self.text).split("::")[-1]
        if self.owner:
            # a member, or a nested class (`HUDMainMenu::SubtitleData`)
            return function_patterns(self.text) + [rf"(\.|->)\s*{self.ident}\s*\(", class_def_pattern(self.ident)]
        if "<" in self.text:
            words = [w for w in re.findall(r"[A-Za-z_]\w*", self.text[self.text.index("<"):])
                     if w not in ("const", "volatile", "class", "struct", "unsigned", "signed", "int", "short",
                                  "long", "char") and not re.fullmatch(r"u?int\d+_t", w)]
            spelled = rf"\b{base}\s*<[^;]*\b{words[0]}\b" if words else rf"\b{base}\s*<"
            return [spelled, class_def_pattern(base)]
        return [rf"\b{base}\b", rf"\b{base}_vftable"]

    def collect(self, where: Where) -> list[str]:
        self.classes = class_scopes(where, self.text)
        if not self.is_type():
            return []
        wanted = spelling(self.text)
        base = strip_template_arguments(self.text).split("::")[-1]
        for rel, number, text in where.matching(rf"\b(using|typedef)\b.*\b{base}\b", base):
            for alias, target in where.file(rel).aliases.items():
                if spelling(target) == wanted and alias not in self.aliases:
                    self.aliases.append(alias)
        owners = {start: name.split(" {for", 1)[0].strip() for start, (name, _, _) in vtables().items()}
        self.tables = sorted(start for start, name in owners.items() if exe_spelling(name) == wanted)
        return [rf"\b{alias}\b" for alias in self.aliases] + [hex_pattern(t) for t in self.tables]

    def render(self, where: Where) -> list[str]:
        lines = [self.text]
        if self.is_type():
            return lines + self.render_type(where)
        if self.owner:
            return lines + describe_function(where, self.text, users=True, indent="  ")
        return lines + self.render_bare(where)

    def render_bare(self, where: Where) -> list[str]:
        """A name without a class: exe globals and function pointers, members of any class, free functions."""
        lines, symbols = [], []
        for rel, number, _ in where.matching(rf"\b{re.escape(self.text)}\b", self.text):
            source = where.file(rel)
            code = source.code_line(number)
            for match in re.finditer(rf"\b{re.escape(self.text)}\b", code):
                at = source.offset(number) + match.start()
                call = source.call_at(at)
                if call and call[0] in DECLARING:
                    arguments = source.arguments(call[1])
                    outer = [s for s in source.enclosing(at) if s.kind == "class"]
                    owner = f" (in class {source.qualified(outer[-1])})" if outer else ""
                    lines.append(f"  declared    {rel}:{number}  {where.clip(source.lines[number - 1])}{owner}")
                    lines += [f"                {where.clip(c)}" for c in source.comments_above(number, 4)]
                    continue
                function = source.function_at(at)
                scopes = source.enclosing(at)
                if function or (scopes and scopes[-1].kind == "class") or \
                        (not any(s.kind in ("func", "block") for s in scopes) and "(" in code[match.end():]):
                    qualified = source.qualified(function) if function else \
                        "::".join(s.name for s in scopes if s.kind in ("ns", "class") and s.name)
                    qualified = qualified if qualified.endswith(self.text) else \
                        (f"{qualified}::{self.text}" if qualified else self.text)
                    if qualified not in symbols:
                        symbols.append(qualified)
        if not lines and not symbols:
            lines.append("  declared    none")
        cap = where.caps["symbols"]
        for symbol in symbols[:cap]:
            lines.append(f"  {symbol}")
            lines += describe_function(where, symbol, users=False, indent="    ")
        if len(symbols) > cap:
            lines.append(f"  ... {len(symbols) - cap} more: " + ", ".join(symbols[cap:cap * 4]))
        uses = []
        for rel, number, _ in where.matching(rf"\b{re.escape(self.text)}\b", self.text):
            source = where.file(rel)
            for match in re.finditer(rf"\b{re.escape(self.text)}\b", source.code_line(number)):
                if any(s.kind in ("func", "block") for s in source.enclosing(source.offset(number) + match.start())):
                    uses.append((rel, number))
                    break
        return lines + users_lines(where, uses, "users (code inside function bodies naming it)", "  ")

    def render_type(self, where: Where) -> list[str]:
        """A class or template instance: its definition, layout pin, vtable constants, explicit specializations and
        users."""
        wanted = spelling(self.text)
        lines = []
        for source, scope in self.classes:
            note = "template" if re.match(r"\s*template\b", source.code[scope.head:scope.open]) else "class"
            # the line of the `class Name` head (under its `template <...>` line)
            head = next((n for n in range(scope.line, source.line_of(scope.open) + 1) if re.search(
                rf"\b(class|struct|union)\b.*\b{re.escape(scope.name.split('::')[-1])}\b", source.code_line(n))),
                scope.line)
            lines.append(f"  {note:9}   {source.rel}:{head}-{source.line_of(scope.close)}  "
                         f"{where.clip(source.lines[head - 1])}")
            lines += [f"                {where.clip(c)}" for c in source.comments_above(head, 4)]
        if not self.classes:
            lines.append("  class       none")
        base = strip_template_arguments(self.text).split("::")[-1]
        names = [re.escape(base)] + [re.escape(alias) for alias in self.aliases]
        spelled_lines = []
        for rel, number, text in where.matching(rf"\b({'|'.join(names)})\b"):
            code = spelling(where.file(rel).code_line(number))
            if occurs(wanted, code) or any(occurs(alias, code) for alias in self.aliases):
                spelled_lines.append((rel, number, code))
        layout = [(rel, number) for rel, number, code in spelled_lines
                  if "FNVD_LAYOUT(sizeof(" + wanted + ")" in code]
        lines += [f"  layout      {rel}:{number}  {where.clip(where.file(rel).lines[number - 1])}"
                  for rel, number in layout] or ["  layout      none"]
        tables = []
        named_tables = "|".join([rf"{re.escape(base)}_vftable"] + [hex_pattern(t) for t in self.tables])
        for rel, number, text in where.matching(rf"FNVD_VFTABLE\s*\(.*({named_tables})", "FNVD_VFTABLE"):
            code = where.file(rel).code_line(number)
            named = re.search(r"FNVD_VFTABLE\(\s*(\w+)\s*,\s*(0x[0-9A-Fa-f]+)", code)
            if named and (int(named.group(2), 16) in self.tables or "<" not in self.text and
                          re.fullmatch(rf"{re.escape(self.text)}_vftable\w*", named.group(1))):
                tables.append(f"  vftable     {rel}:{number}  {where.clip(text)}")
        exe = ", ".join(f"{t:#x}" for t in self.tables) or "none"
        lines += tables or ["  vftable     none in src"]
        lines.append(f"  exe tables  {exe} (by RTTI or IDA name)")
        heads = {(source.rel, number) for source, scope in self.classes
                 for number in range(scope.line, source.line_of(scope.open) + 1)}
        specializations, uses = [], []
        registered = {}
        for rel, number, code in spelled_lines:
            source = where.file(rel)
            raw = source.code_line(number)
            if registration := REGISTRATION_LINE.match(raw):
                found = line_registration(raw) or \
                    (registration.group(1), source.arguments(source.offset(number) + registration.end() - 1))
                if found[1]:
                    registered[spelling(registration_symbol(*found, source.aliases))] = found[1][0]
                continue
            if (rel, number) in heads or code.startswith("FNVD_") or re.match(r"(using|typedef)\b", code):
                continue
            # a member's explicit specialization: a header's line naming Instance::member outside any class or body
            if "<" in self.text and rel.endswith((".h", ".hpp", ".inl")) and \
                    re.search(re.escape(wanted) + r"::~?\w", code) and not any(
                    s.kind in ("class", "func", "block") for s in source.enclosing(
                        source.offset(number) + len(raw) - len(raw.lstrip()))):
                specializations.append((source, number, code))
            else:
                uses.append((rel, number))
        cap = where.caps["decls"] * 3
        for source, number, code in specializations[:cap]:
            member = re.search(re.escape(wanted) + r"::(~?\w+)", code).group(1)
            first, last = source.comment_block(number - 1 if source.lines[number - 2].strip().startswith(
                "template") else number)
            above = " ".join(source.lines[k - 1] for k in range(first, last + 1)
                             if source.lines[k - 1].strip().startswith("//"))
            fold = FOLD.search(" ".join(above.replace("//", " ").split()))
            address = registered.get(spelling(f"{self.text}::{member}")) or \
                (f"fold of {fold.group(2)}" if fold else "no address in a registration or fold comment")
            lines.append(f"  special.    {source.rel}:{number}  {where.clip(source.lines[number - 1])}  [{address}]")
        if len(specializations) > cap:
            lines.append(f"  special.    ... {len(specializations) - cap} more (--more)")
        if "<" in self.text and not specializations:
            lines.append("  special.    none")
        if self.aliases:
            lines.append("  aliases     " + ", ".join(self.aliases))
        return lines + users_lines(where, uses, "users (code spelling it)", "  ")


def command_where(graph: Graph, arguments) -> None:
    root = checkout_root()
    where = Where(root, graph, arguments)
    queries = [AddressQuery(arg) if re.fullmatch(r"0[xX][0-9a-fA-F]+", arg) else NameQuery(arg)
               for arg in arguments.args]
    where.search([p for query in queries for p in query.patterns()])
    where.search([p for query in queries for p in query.collect(where)])
    print(f"(checkout {os.path.relpath(root, os.path.dirname(os.path.realpath(os.path.join(root, 'env'))))})\n")
    print("\n\n".join("\n".join(query.render(where)) for query in queries))



def main() -> int:
    # quiet when piped into head
    import signal
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)

    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--probe", help="probe.tsv to use (default: the newest under env/out)")
    commands = parser.add_subparsers(dest="command", required=True)

    show = commands.add_parser("show")
    show.add_argument("function")
    show.add_argument("--limit", type=int, default=40, help="callers / callees listed (default 40)")

    commands.add_parser("decomp").add_argument("function")
    commands.add_parser("asm").add_argument("function")
    commands.add_parser("type").add_argument("name")
    commands.add_parser("vtable").add_argument("name")

    candidates = commands.add_parser("candidates", help="rank functions to reconstruct next")
    candidates.add_argument("--class", dest="class_name", help="only this class (exact, e.g. TESForm)")
    candidates.add_argument("--match", help="only names matching this regex")
    candidates.add_argument("--max-size", type=int, default=400, help="largest function in bytes (default 400)")
    # a replacement is a 5-byte jmp over the entry; anything shorter would overwrite the next function
    candidates.add_argument("--min-size", type=int, default=5, help="smallest function in bytes (default 5)")
    candidates.add_argument("--min-hits", type=int, default=1, help="probe hits needed, any thread (default 1)")
    candidates.add_argument("--unexercised", action="store_true", help="also functions the probe never saw")
    candidates.add_argument("--limit", type=int, default=50)

    classes = commands.add_parser("classes")
    classes.add_argument("--exercised", action="store_true", help="only classes with probe hits")
    classes.add_argument("--limit", type=int, default=60)

    probe_list = commands.add_parser("probe-list")
    probe_list.add_argument("-o", "--output", help="write here instead of stdout")

    where = commands.add_parser("where", help="what src says about exe addresses and src names",
                                description=WHERE_HELP, formatter_class=argparse.RawDescriptionHelpFormatter)
    where.add_argument("args", nargs="+", metavar="ARG", help="0xADDRESS, Class::member, name, Class, Instance<Args>")
    where.add_argument("--more", action="store_true", help="no caps on the sections")
    where.add_argument("--users", action="store_true", help="print the users' lines, not only a count per file")

    arguments = parser.parse_args()
    graph = load_graph()
    handlers = {
        "show": command_show,
        "decomp": command_decomp,
        "asm": command_asm,
        "type": command_type,
        "vtable": command_vtable,
        "candidates": command_candidates,
        "classes": command_classes,
        "probe-list": command_probe_list,
        "where": command_where,
    }
    handlers[arguments.command](graph, arguments)
    return 0


if __name__ == "__main__":
    sys.exit(main())
