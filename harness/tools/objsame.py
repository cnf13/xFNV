#!/usr/bin/env python3
"""The object gate: a source rewrite that must not change code is accepted when every built object's code and data
are the same before and after it.

Usage: env/venv/bin/python harness/tools/objsame.py snapshot [--target linux|x86|x64] [--build DIR] OUT
       env/venv/bin/python harness/tools/objsame.py compare [--build DIR] [--renames TSV] [--all] SNAPSHOT

  snapshot   records every object of the build folder's compile_commands.json (default build/linux; --target x86 is
             build/noexe, x64 build/noexe64): per symbol a digest of what the linker uses. Build first.
  compare    after the rewrite and a rebuild of the same folder: every symbol whose code or data differs, each with
             its object, then a count. Exit 0 when nothing differs, 1 otherwise. --all lists every line of a long
             report (by default 200 lines per kind).
  --renames  old<TAB>new rows: a rename step's mapping. A row is an identifier (`sub_8A1234`) or a qualified name
             (`Actor::sub_8A1234`, `NiNode`), replaced as a source-name in every mangled name and RTTI name string
             of the rebuilt objects, or a whole symbol (`_Z...`/`?...` on both sides, or any name written `=old`),
             replaced exactly. The new names are mapped back to the old ones before comparing. An identifier row
             maps every symbol spelling that identifier, also other classes' same-named members (`sub_C422D0` to
             `Shutdown` maps every `Shutdown` back): where the new name is common, give whole-symbol rows for every
             symbol holding the old name (from the snapshot). An identifier that is the tail of a longer one written
             with a letter before it (`a3Foo` in a mangled name) can be matched too: give a qualified row then.

What is compared. Per defined symbol with a size in an allocated section (functions; data, read-only data, .bss and
thread-local data): its bytes (the size alone for .bss) and its relocations, each as (offset, type, target, offset
into the target). A target is a symbol's name; a reference through a section (what the assembler writes for a
file-local symbol) is resolved to the symbol at that place in the section, a string literal to its text, a merged
constant to its bytes, other unnamed data (a jump table) and compiler-numbered helpers (`__cxx_global_var_init.N`,
`.L` labels) to their own content, so their numbering does not count. Per object the static initialiser lists
(.init_array, .fini_array) in order. Symbols are matched by name: global ones across the whole build (an inline or
template copy in several objects counts once per distinct content), file-local ones in their object, else across the
build where one name is unique (a static function moved to another file matches).

Not compared: debug info, the symbol table's order, section and function order within an object, the exception
and unwind tables (.eh_frame, .gcc_except_table: they follow from the code), .comment and notes, the
address-significance table. So line numbers and file paths in debug info never show. The tree's commit (`git
describe` in the build folder's crashlog/BuildInfo.h, `-dirty` or not) is read as a placeholder wherever a string
holds it (the crash log's build stamp, SDL's revision): it differs between any two trees.

What it cannot see through, and how that shows:
  - `__LINE__`, `__FILE__`, `__func__` in code: a different constant or string, reported on the function using it.
    src uses none of them in code (Reconstruction.hpp's `__LINE__` names a static_assert holder only).
  - A definition moved into another file: its symbol matches by name and is equal. When it has a dynamic
    initialiser, the initialiser order changes: the objects' `_GLOBAL__sub_I_<file>` functions and init lists are
    reported (`init:`). The order of initialisers matters: such a report is a real difference.
  - A rename: the old symbol is reported `gone`, the new one `new`, and a gone/new pair with one content is reported
    `renamed` (all three are differences). With --renames covering it, it is equal.
  - Anonymous-namespace and lambda names: matched by name like any other. A lambda renumbered (another lambda added
    or removed before it in the same scope) shows as a `renamed` pair, and its callers differ in that target.
  - Inline functions the compiler kept in one object and not in another (a change of which objects use them) show
    as `gone` or `new` without being a code change of a function that is kept in both.

Targets: linux (ELF, the reference) and x86/x64 (COFF from clang-cl, compared the same way).
"""

from __future__ import annotations

import argparse
import bisect
import gzip
import hashlib
import json
import os
import re
import shlex
import shutil
import struct
import subprocess
import sys
from collections import defaultdict
from multiprocessing import Pool

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
BUILDS = {"linux": "build/linux", "x86": "build/noexe", "x64": "build/noexe64"}

# Compiler-numbered helper functions and labels: named by their content, not their name.
NUMBERED = re.compile(r"^(\.L|__cxx_global_var_init|__cxx_global_array_dtor|__dtor_|\$|\.)")

RENAMES = None  # (regex, {new mangled chunk: old mangled chunk}, {new symbol: old symbol}) while comparing
COMMIT = None  # the build's commit stamp, compared as a placeholder


def h(data) -> str:
    return hashlib.sha1(data if isinstance(data, bytes) else repr(data).encode()).hexdigest()[:20]


def canon(name: str) -> str:
    if RENAMES is None:
        return name
    regex, chunks, whole = RENAMES
    if name in whole:
        return whole[name]
    if regex is not None and (name.startswith("_Z") or name.startswith("?")):
        return regex.sub(lambda m: chunks[m.group(0)], name)
    return name


def load_renames(path):
    chunks, whole = {}, {}
    with open(path) as f:
        for line in f:
            line = line.rstrip("\n")
            if not line or line.startswith("#"):
                continue
            old, new = line.split("\t")[:2]
            if old.startswith("=") or old.startswith("_Z") or old.startswith("?"):
                whole[new] = old.removeprefix("=")
                continue
            o = "".join(f"{len(p)}{p}" for p in old.split("::"))
            n = "".join(f"{len(p)}{p}" for p in new.split("::"))
            chunks[n] = o
    regex = None
    if chunks:
        alt = "|".join(re.escape(c) for c in sorted(chunks, key=len, reverse=True))
        regex = re.compile(r"(?<![0-9])(?:" + alt + r")")
    return regex, chunks, whole


# --- ELF ------------------------------------------------------------------------------------------------------

SHT_SYMTAB, SHT_RELA, SHT_NOBITS, SHT_REL = 2, 4, 8, 9
SHF_ALLOC, SHF_MERGE, SHF_STRINGS, SHF_EXECINSTR = 0x2, 0x10, 0x20, 0x4
STT_SECTION, STT_FILE = 3, 4
# x86-64 relocations relative to the place: the target is the addend plus the distance to the instruction's end.
ELF_PCREL = {2, 4, 9, 19, 20, 24, 26, 41, 42}
SKIP_SECTIONS = (".eh_frame", ".gcc_except_table", ".note", ".comment", ".llvm_addrsig", ".group")


class Section:
    __slots__ = ("index", "name", "data", "size", "code", "nobits", "strings", "entsize", "starts", "syms",
                 "relocs", "targets", "kind")

    def __init__(self, index, name, data, size, code, nobits, strings, entsize):
        self.index, self.name, self.data, self.size = index, name, data, size
        self.code, self.nobits, self.strings, self.entsize = code, nobits, strings, entsize
        self.syms = []  # (start, end, name or None for numbered, symbol index)
        self.starts = []
        self.relocs = []  # (offset, type, symbol index, addend), sorted
        self.targets = []  # every offset some relocation points at, sorted (boundaries of unnamed data)
        self.kind = "code" if code else ("bss" if nobits else "data")


class Symbol:
    __slots__ = ("name", "value", "size", "type", "bind", "shndx")

    def __init__(self, name, value, size, type_, bind, shndx):
        self.name, self.value, self.size, self.type, self.bind, self.shndx = name, value, size, type_, bind, shndx


class Object:
    """One object file: its sections, symbols and relocations, the format's details resolved by the reader."""

    def __init__(self, path):
        self.path = path
        self.sections = {}  # index -> Section
        self.symbols = []
        self.pcrel = set()
        self.memo = {}
        self.ends = {}  # (section, function start) -> sorted instruction ends, for place-relative targets
        self.mode = None
        self.machine = None

    # A relocation's target as (identity, offset into it).
    def target(self, sec, off, rtype, symi, addend, inplace):
        s = self.symbols[symi]
        if s.type == STT_SECTION or (s.shndx in self.sections and s.bind == 0 and NUMBERED.match(s.name or "")):
            if s.shndx not in self.sections:
                return ("?sec", addend + inplace)
            t = s.value + addend + inplace
            if rtype in self.pcrel:
                t += self.bias(sec, off, rtype)
            return self.resolve(self.sections[s.shndx], t)
        return (canon(s.name), addend + inplace)

    # The distance from a place-relative field to the end of its instruction: ELF's addend counts to the
    # instruction's end (4 unless an immediate follows the field), COFF's to the field's end (x64's REL32_N: N more).
    def bias(self, sec, off, rtype):
        if self.mode == "coff":
            return 4 + (rtype - 4 if self.machine == 0x8664 and 5 <= rtype <= 9 else 0)
        if not sec.code:
            return 4
        i = bisect.bisect_right(sec.starts, off) - 1
        if i < 0 or not (sec.syms[i][0] <= off < sec.syms[i][1]):
            return 4
        start, end = sec.syms[i][0], sec.syms[i][1]
        key = (sec.index, start)
        ends = self.ends.get(key)
        if ends is None:
            ends = [addr + size for addr, size, _m, _o in disasm().disasm_lite(sec.data[start:end], start)]
            self.ends[key] = ends
        j = bisect.bisect_right(ends, off)
        return ends[j] - off if j < len(ends) else 4

    def resolve(self, tsec, t):
        # the symbol covering t (the first name among aliases), else one whose end t is (a one-past-end pointer)
        i = bisect.bisect_right(tsec.starts, t)
        first = bisect.bisect_left(tsec.starts, tsec.starts[i - 1]) if i else 0
        covering = [x for x in tsec.syms[first:i] if x[0] <= t < x[1]]
        if not covering and i:
            covering = [x for x in tsec.syms[first:i] if x[1] == t]
        if covering:
            named = [x for x in covering if x[2] is not None]
            start, end, name, _ = named[0] if named else covering[0]
            if (name is None and tsec.strings) or (name is not None and name.startswith("??_C@")):
                # a string literal (an MSVC one is named after its text): its text counts
                return (literal(tsec.data[start:end].rstrip(b"\0")), t - start)
            ident = canon(name) if name is not None else self.content_id(tsec, start, end)
            return (ident, t - start)
        if tsec.strings:
            end = tsec.data.find(b"\0", t)
            return (literal(tsec.data[t:end if end >= 0 else len(tsec.data)]), 0)
        if tsec.entsize and not tsec.nobits:
            base = t - t % tsec.entsize
            return ("cst:" + tsec.data[base:base + tsec.entsize].hex(), t - base)
        # unnamed data (a jump table): from the nearest boundary at or before t to the next one
        j = bisect.bisect_right(tsec.targets, t) - 1
        start = tsec.targets[j] if j >= 0 else 0
        nxt = [tsec.size]
        if j + 1 < len(tsec.targets):
            nxt.append(tsec.targets[j + 1])
        k = bisect.bisect_right(tsec.starts, start)
        if k < len(tsec.starts):
            nxt.append(tsec.starts[k])
        return (self.content_id(tsec, start, min(nxt)), t - start)

    def content_id(self, sec, start, end):
        key = (sec.index, start, end)
        got = self.memo.get(key)
        if got is not None:
            return got
        self.memo[key] = "anon:cycle"
        got = "anon:" + h(self.content(sec, start, end))
        self.memo[key] = got
        return got

    # A range's bytes and relocations, as compared.
    def content(self, sec, start, end):
        if sec.nobits:
            return ("bss", end - start)
        lo = bisect.bisect_left(sec.relocs, (start,))
        hi = bisect.bisect_left(sec.relocs, (end,))
        data = bytearray(sec.data[start:end])
        rel = []
        for off, rtype, symi, addend, width in sec.relocs[lo:hi]:
            inplace = 0
            if width:
                # COFF: the addend is in the field; the field is zeroed so that only the target counts
                raw = data[off - start:off - start + width]
                inplace = int.from_bytes(raw, "little", signed=True)
                data[off - start:off - start + width] = bytes(width)
            rel.append((off - start, rtype) + self.target(sec, off, rtype, symi, addend, inplace))
        return (h(bytes(data)), end - start, tuple(rel))

    def index_sections(self):
        for sec in self.sections.values():
            sec.syms.sort(key=lambda x: (x[0], x[2] is None, x[2] or ""))
            sec.starts = [x[0] for x in sec.syms]
            sec.relocs.sort()
        targets = defaultdict(set)
        for sec in self.sections.values():
            for off, rtype, symi, addend, width in sec.relocs:
                s = self.symbols[symi]
                if s.type == STT_SECTION and s.shndx in self.sections:
                    targets[s.shndx].add(s.value + addend + (4 if rtype in self.pcrel else 0))
        for i, t in targets.items():
            self.sections[i].targets = sorted(t)

    def items(self, rel):
        """(key, kind, name, local, digest, size) per named symbol, plus the object's initialiser lists."""
        out = []
        for sec in self.sections.values():
            for start, end, name, symi in sec.syms:
                if name is None or name.startswith("??_C@"):
                    continue
                s = self.symbols[symi]
                c = self.content(sec, start, end)
                if RENAMES is not None and name.startswith(("_ZTS", "??_R0")) and not sec.nobits:
                    # an RTTI name string holds the type's mangled name: compared under the renames where they
                    # change it, else as the snapshot (taken without renames) hashed it
                    text = sec.data[start:end].rstrip(b"\0").decode("latin-1")
                    full = "_Z" + text if name.startswith("_ZTS") else text
                    if canon(full) != full:
                        c = ("rtti", canon(full), c[1:])
                out.append((canon(name), sec.kind, s.bind == 0, h(c), end - start, h(c[0]) if len(c) == 3 else ""))
        for lists in (self.init_lists(),):
            for name, entries in lists:
                out.append(("init:" + name + "@" + rel, "init", True, h(entries), len(entries), ""))
        return out

    def init_lists(self):
        out = []
        for sec in sorted(self.sections.values(), key=lambda s: s.index):
            if sec.name.startswith((".init_array", ".fini_array", ".preinit_array", ".CRT$X")):
                entries = tuple(self.target(sec, off, rtype, symi, addend, 0)
                                for off, rtype, symi, addend, width in sec.relocs)
                out.append((sec.name, entries))
        merged = defaultdict(list)
        for name, entries in out:
            merged[name.split(".")[1] if name.startswith(".") else name[:7]].extend(entries)
        return sorted((k, tuple(v)) for k, v in merged.items())


_MD = None


def disasm():
    global _MD
    if _MD is None:
        import capstone
        _MD = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
    return _MD


def read_elf(path):
    with open(path, "rb") as f:
        raw = f.read()
    if raw[:4] != b"\x7fELF" or raw[4] != 2:
        raise ValueError(f"{path}: not a 64-bit ELF object")
    shoff, = struct.unpack_from("<Q", raw, 0x28)
    shentsize, shnum, shstrndx = struct.unpack_from("<HHH", raw, 0x3A)
    hdrs = [struct.unpack_from("<IIQQQQIIQQ", raw, shoff + i * shentsize) for i in range(shnum)]
    names_off = hdrs[shstrndx][4]

    def cstr(base, off):
        end = raw.index(b"\0", base + off)
        return raw[base + off:end].decode("latin-1")

    obj = Object(path)
    obj.mode = "elf"
    obj.pcrel = ELF_PCREL
    symtab = None
    for i, (name, typ, flags, _a, off, size, link, info, align, entsize) in enumerate(hdrs):
        sname = cstr(names_off, name)
        if typ == SHT_SYMTAB:
            symtab = (off, size, link, entsize)
        if not flags & SHF_ALLOC or sname.startswith(SKIP_SECTIONS) or typ in (SHT_RELA, SHT_REL):
            continue
        nobits = typ == SHT_NOBITS
        obj.sections[i] = Section(i, sname, b"" if nobits else raw[off:off + size], size,
                                  bool(flags & SHF_EXECINSTR), nobits, bool(flags & SHF_STRINGS),
                                  entsize if flags & SHF_MERGE else 0)
    off, size, link, entsize = symtab
    strbase = hdrs[link][4]
    for k in range(size // entsize):
        name, info, _o, shndx, value, ssize = struct.unpack_from("<IBBHQQ", raw, off + k * entsize)
        obj.symbols.append(Symbol(cstr(strbase, name), value, ssize, info & 0xF, info >> 4, shndx))
    for i, (name, typ, flags, _a, off, size, link, info, align, entsize) in enumerate(hdrs):
        if typ != SHT_RELA or info not in obj.sections:
            continue
        sec = obj.sections[info]
        for k in range(size // 24):
            r_off, r_info, addend = struct.unpack_from("<QQq", raw, off + k * 24)
            sec.relocs.append((r_off, r_info & 0xFFFFFFFF, r_info >> 32, addend, 0))
    for idx, s in enumerate(obj.symbols):
        if s.shndx not in obj.sections or s.type in (STT_SECTION, STT_FILE) or not s.name:
            continue
        if s.size == 0:
            continue
        numbered = s.bind == 0 and NUMBERED.match(s.name)
        obj.sections[s.shndx].syms.append((s.value, s.value + s.size, None if numbered else s.name, idx))
    obj.index_sections()
    return obj


# --- COFF -----------------------------------------------------------------------------------------------------

COFF_MACHINES = {0x14C: "x86", 0x8664: "x64"}
# Relocations relative to the place (the end of the 4-byte field) per machine; their field width per type.
COFF_PCREL = {0x14C: {0x14}, 0x8664: {4, 5, 6, 7, 8, 9}}
COFF_WIDTH = {0x14C: {6: 4, 7: 4, 0x14: 4, 0xA: 2, 0xB: 4, 0x0C: 4}, 0x8664: {1: 8, 2: 4, 3: 4, 4: 4, 5: 4,
                                                                           6: 4, 7: 4, 8: 4, 9: 4, 0xA: 2,
                                                                           0xB: 4}}


def read_coff(path):
    with open(path, "rb") as f:
        raw = f.read()
    machine, nsec, _t, symoff, nsym, opt, _c = struct.unpack_from("<HHIIIHH", raw, 0)
    if machine not in COFF_MACHINES:
        raise ValueError(f"{path}: not a COFF object (machine {machine:#x})")
    strtab = symoff + nsym * 18

    def name_of(b8):
        if b8[:4] == b"\0\0\0\0":
            o, = struct.unpack_from("<I", b8, 4)
            end = raw.index(b"\0", strtab + o)
            return raw[strtab + o:end].decode("latin-1")
        return b8.rstrip(b"\0").decode("latin-1")

    obj = Object(path)
    obj.mode = "coff"
    obj.machine = machine
    obj.pcrel = COFF_PCREL[machine]
    widths = COFF_WIDTH[machine]
    relocs = {}
    for i in range(nsec):
        base = 20 + opt + i * 40
        n = raw[base:base + 8]
        _vs, _va, size, dataoff, reloff, _l, nrel, _nl, flags = struct.unpack_from("<IIIIIIHHI", raw, base + 8)
        if n[:1] == b"/":
            o = int(n[1:].rstrip(b"\0"))
            sname = raw[strtab + o:raw.index(b"\0", strtab + o)].decode("latin-1")
        else:
            sname = n.rstrip(b"\0").decode("latin-1")
        if flags & 0x800 or flags & 0x02000000 or sname.startswith((".debug", ".xdata", ".pdata", ".llvm",
                                                                    ".drectve", ".sx", ".gfids", ".gljmp")):
            continue  # LNK_REMOVE, discardable (debug), unwind data, linker directives
        nobits = bool(flags & 0x80) and not flags & 0x40
        code = bool(flags & 0x20)
        idx = i + 1
        obj.sections[idx] = Section(idx, sname, b"" if nobits else raw[dataoff:dataoff + size], size, code,
                                    nobits, False, 0)
        relocs[idx] = (reloff, nrel)
    k = 0
    while k < nsym:
        b = raw[symoff + k * 18:symoff + k * 18 + 18]
        value, shndx, typ, sclass, naux = struct.unpack_from("<IhHBB", b, 8)
        name = name_of(b[:8])
        stype = STT_SECTION if sclass == 3 and value == 0 and naux else (2 if typ >> 4 == 2 else 1)
        obj.symbols.append(Symbol(name, value, 0, stype, 1 if sclass == 2 else 0, shndx if shndx > 0 else 0))
        for _ in range(naux):
            obj.symbols.append(Symbol("", 0, 0, STT_FILE, 0, 0))
        k += 1 + naux
    for idx, (reloff, nrel) in relocs.items():
        sec = obj.sections[idx]
        for r in range(nrel):
            off, symi, rtype = struct.unpack_from("<IIH", raw, reloff + r * 10)
            sec.relocs.append((off, rtype, symi, 0, widths.get(rtype, 4)))
    # COFF symbols carry no size: a symbol runs to the next one in its section, or the section's end.
    starts = defaultdict(list)
    for idx, s in enumerate(obj.symbols):
        if s.shndx in obj.sections and s.type not in (STT_SECTION, STT_FILE) and s.name and \
                not s.name.startswith(("$", ".")):
            starts[s.shndx].append((s.value, idx))
    for shndx, lst in starts.items():
        sec = obj.sections[shndx]
        bounds = sorted({v for v, _ in lst} | {sec.size})
        for value, idx in lst:
            s = obj.symbols[idx]
            end = bounds[bounds.index(value) + 1] if value < sec.size else value
            if end <= value:
                continue
            numbered = s.bind == 0 and NUMBERED.match(s.name)
            sec.syms.append((value, end, None if numbered else s.name, idx))
    obj.index_sections()
    return obj


def read_object(path):
    with open(path, "rb") as f:
        head = f.read(4)
    return read_elf(path) if head == b"\x7fELF" else read_coff(path)


# --- snapshot and compare -------------------------------------------------------------------------------------

def objects(build):
    with open(os.path.join(build, "compile_commands.json")) as f:
        entries = json.load(f)
    out = []
    for e in entries:
        o = e.get("output")
        if o is None:
            args = shlex.split(e["command"])
            for i, a in enumerate(args):
                if a == "-o":
                    o = args[i + 1]
                elif a.startswith(("/Fo", "-Fo")):
                    o = a[3:]
        if o is None or not o.endswith((".o", ".obj")):
            continue  # a precompiled header
        path = o if os.path.isabs(o) else os.path.join(e["directory"], o)
        rel = os.path.relpath(path, build)
        src = os.path.relpath(e["file"], ROOT) if os.path.isabs(e["file"]) else e["file"]
        out.append((rel, src, path))
    return sorted(set(out))


def literal(text: bytes) -> str:
    if COMMIT and COMMIT in text:
        # `git describe` output (`TAG-N-g<commit>`): the count N changes with the commit, so it goes too.
        text = re.sub(rb"(-\d+-g)?" + re.escape(COMMIT) + rb"(-dirty)?", b"<the build's commit>", text)
    return "str:" + (text.decode("latin-1") if len(text) <= 80 else h(text))


def build_commit(build):
    """The commit of the build's tree (crashlog/BuildInfo.h, `-dirty` left out): the crash log's build stamp and
    SDL's revision string hold it, so it differs between any two trees."""
    try:
        with open(os.path.join(build, "crashlog", "BuildInfo.h")) as f:
            m = re.search(r'FNVD_CRASHLOG_COMMIT\s+"([^"]*?)(-dirty)?"', f.read())
        return m.group(1).encode() if m and m.group(1) != "unknown" else None
    except OSError:
        return None


def scan_one(job):
    rel, src, path, renames, commit = job
    global RENAMES, COMMIT
    RENAMES, COMMIT = renames, commit
    if not os.path.exists(path):
        return rel, src, None, "missing"
    try:
        return rel, src, read_object(path).items(rel), None
    except Exception as e:  # a report of the object, not a stop of the whole scan
        return rel, src, None, f"{type(e).__name__}: {e}"


def scan(build, jobs, renames=None):
    commit = build_commit(build)
    work = [(rel, src, path, renames, commit) for rel, src, path in objects(build)]
    with Pool(jobs) as pool:
        return pool.map(scan_one, work, chunksize=8)


def table(results):
    """key -> {digest: [objects]}, kind; locals keyed (name, object) and listed for the cross-object match."""
    glob, loc, kinds, errors = defaultdict(lambda: defaultdict(list)), {}, {}, {}
    for rel, src, items, err in results:
        if err:
            errors[rel] = (src, err)
            continue
        for name, kind, local, digest, size, code in items:
            kinds[name] = kind
            if local:
                loc[(name, rel)] = (digest, size, code, src)
            else:
                glob[name][digest].append((rel, src, size, code))
    return glob, loc, kinds, errors


def cmd_snapshot(args):
    build = os.path.abspath(args.build or BUILDS[args.target])
    results = scan(build, args.jobs)
    snap = {"build": os.path.relpath(build, ROOT), "target": args.target, "objects": results}
    with gzip.open(args.out, "wt") as f:
        json.dump(snap, f)
    n = sum(len(r[2] or ()) for r in results)
    bad = [r for r in results if r[3]]
    print(f"snapshot: {len(results)} objects, {n} symbols, {len(bad)} unreadable -> {args.out}")
    for rel, src, _i, err in bad[:20]:
        print(f"  unreadable: {rel}: {err}")


def demangler(names):
    tool = shutil.which("llvm-cxxfilt") or shutil.which("c++filt")
    names = [n for n in names if n.startswith("_Z")]
    if not tool or not names:
        return {}
    out = subprocess.run([tool], input="\n".join(names) + "\n", capture_output=True, text=True).stdout
    return dict(zip(names, out.splitlines()))


def cmd_compare(args):
    global RENAMES
    with gzip.open(args.snapshot, "rt") as f:
        snap = json.load(f)
    build = os.path.abspath(args.build or os.path.join(ROOT, snap["build"]))
    renames = load_renames(args.renames) if args.renames else None
    before = [tuple(r) for r in snap["objects"]]
    after = scan(build, args.jobs, renames)
    bg, bl, bk, berr = table(before)
    ag, al, ak, aerr = table(after)

    rows = []  # (kind of difference, symbol, detail, objects)

    def where(lst):
        srcs = sorted({x[1] for x in lst})
        return ", ".join(srcs[:3]) + (f" (+{len(srcs) - 3})" if len(srcs) > 3 else "")

    gone, new = {}, {}
    for name in sorted(set(bg) | set(ag)):
        b, a = bg.get(name), ag.get(name)
        if a is None:
            gone[name] = b
        elif b is None:
            new[name] = a
        elif set(b) != set(a):
            bl_ = [x for d in b for x in b[d]]
            al_ = [x for d in a for x in a[d]]
            what = "code" if bk.get(name) == "code" else "data"
            bs, as_ = {x[2] for x in bl_}, {x[2] for x in al_}
            if bs != as_:
                detail = f"size {sorted(bs)} -> {sorted(as_)}"
            elif {x[3] for x in bl_} == {x[3] for x in al_}:
                detail = "relocations (targets) differ"
            else:
                detail = "bytes differ"
            if len(b) > 1 or len(a) > 1:
                detail += f"; {len(b)} -> {len(a)} distinct copies"
            rows.append((what, name, detail, where(al_)))
    # file-local symbols: in their object, else across the build by a unique name
    bl_left, al_left = dict(bl), dict(al)
    for key in sorted(set(bl) & set(al)):
        b, a = bl_left.pop(key), al_left.pop(key)
        if b[0] != a[0]:
            name = key[0]
            if name.startswith("init:"):
                rows.append(("init", name.split("@")[0], "static initialiser list changed (order or entries)",
                             a[3]))
            else:
                detail = f"size {b[1]} -> {a[1]}" if b[1] != a[1] else (
                    "bytes differ" if b[2] != a[2] else "relocations (targets) differ")
                rows.append((bk.get(name, "code") if bk.get(name) != "bss" else "data", name, detail, a[3]))
    bn, an = defaultdict(list), defaultdict(list)
    for (name, rel), v in bl_left.items():
        bn[name].append((rel, v))
    for (name, rel), v in al_left.items():
        an[name].append((rel, v))
    for name in sorted(set(bn) | set(an)):
        b, a = bn.get(name, []), an.get(name, [])
        if len(b) == 1 and len(a) == 1:
            if b[0][1][0] != a[0][1][0]:
                rows.append(("code" if bk.get(name) == "code" else "data", name,
                             f"moved {b[0][1][3]} -> {a[0][1][3]} and differs", a[0][1][3]))
            continue
        for rel, v in b:
            gone[name + "@" + rel] = {v[0]: [(rel, v[3], v[1], v[2])]}
        for rel, v in a:
            new[name + "@" + rel] = {v[0]: [(rel, v[3], v[1], v[2])]}
    # gone/new pairs with one content: renames
    by_digest = defaultdict(list)
    for name, d in new.items():
        by_digest[frozenset(d)].append(name)
    for name in sorted(gone):
        cands = by_digest.get(frozenset(gone[name]))
        if cands:
            other = cands.pop(0)
            lst = [x for xs in new.pop(other).values() for x in xs]
            rows.append(("renamed", name, "-> " + other, where(lst)))
        else:
            lst = [x for xs in gone[name].values() for x in xs]
            rows.append(("gone", name, "", where(lst)))
    for name in sorted(new):
        lst = [x for xs in new[name].values() for x in xs]
        rows.append(("new", name, "", where(lst)))
    for rel in sorted(set(berr) | set(aerr)):
        # an object of a target the build did not build (missing on both sides) is no difference
        if rel in aerr and not (aerr[rel][1] == "missing" and berr.get(rel, ("", ""))[1] == "missing"):
            rows.append(("object", aerr[rel][0], rel + ": " + aerr[rel][1], ""))
    known = {r[0] for r in before} | {r[0] for r in after}
    for rel in sorted(known - {r[0] for r in after}):
        rows.append(("object", rel, "no longer built", ""))
    for rel in sorted(known - {r[0] for r in before}):
        rows.append(("object", rel, "built now, not in the snapshot", ""))

    dem = demangler([r[1].split("@")[0] for r in rows])
    order = ["object", "init", "code", "data", "renamed", "gone", "new"]
    rows.sort(key=lambda r: (order.index(r[0]) if r[0] in order else 9, r[3], r[1]))
    per = defaultdict(int)
    for kind, name, detail, objs in rows:
        per[kind] += 1
        if per[kind] > 200 and not args.all:
            continue
        base = name.split("@")[0]
        shown = f"{name} ({dem[base]})" if base in dem and dem[base] != base else name
        print(f"{kind}\t{objs}\t{shown}" + (f"\t{detail}" if detail else ""))
    for kind in order:
        if per[kind] > 200 and not args.all:
            print(f"{kind}: {per[kind] - 200} more lines (--all)")
    nb = sum(len(r[2] or ()) for r in before)
    na = sum(len(r[2] or ()) for r in after)
    summary = ", ".join(f"{per[k]} {k}" for k in order if per[k])
    if rows:
        print(f"DIFFERENT: {summary} ({len(after)} objects, {nb} -> {na} symbols)")
        return 1
    print(f"SAME: {len(after)} objects, {na} symbols")
    return 0


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)
    s = sub.add_parser("snapshot", help="record the build folder's objects")
    s.add_argument("--target", choices=sorted(BUILDS), default="linux")
    s.add_argument("--build", help="the build folder (default: the target's)")
    s.add_argument("--jobs", type=int, default=8)
    s.add_argument("out")
    c = sub.add_parser("compare", help="compare the rebuilt objects with a snapshot")
    c.add_argument("--build", help="the build folder (default: the snapshot's)")
    c.add_argument("--renames", help="TSV old<TAB>new applied to the rebuilt objects' names")
    c.add_argument("--all", action="store_true", help="every line of a long report")
    c.add_argument("--jobs", type=int, default=8)
    c.add_argument("snapshot")
    args = p.parse_args()
    if args.cmd == "snapshot":
        cmd_snapshot(args)
        return 0
    return cmd_compare(args)


if __name__ == "__main__":
    sys.exit(main())
