#!/usr/bin/env python3
"""The Xbox 360 debug build's locals and how its code uses them, for pdb_locals_ida.py.

The debug build (resources/xbox/Fallout_Debug.exe, PowerPC, /Od) keeps every local in its own stack slot, and the PDB
names each slot (S_REGREL32: name, type, frame offset). This walks every function's code and writes, per function,
its variables (parameters, locals) and the events that mention them:

  ["call", index, callee, [[label, x], ...]]   a call; label "this", "ret" (hidden result), "0".. (explicit parameter).
                                                A callee whose layout the PDB does not give gets them by position:
                                                a virtual call's first is "this", any other call's first is "0"
  ["asg", var, delta, x]                        a store into a variable
  ["mst", x_base, disp, x]                      a store through a pointer or into a global
  ["ret", x]                                    the value returned
  ["cmp", x, x]                                 a comparison

x is ["v", var, delta] (the variable's value), ["a", var, delta] (its address), ["c", int], ["fc", text] (a float
constant), ["s", text] (a string literal), ["g", name] (a global's value), ["ga", name] (a global's or function's
address), ["ld", x, disp] (a load through x), ["add", x, k], ["r", call index] (a call's result), ["op", [x, ...]]
(any other computation over them) or null. Compiler temporaries (slots the PDB does not name) are replaced by what
the code stores in them.

pdb_locals_ida.py reads the same events off the PC function's Hex-Rays decompile and matches the two.

Output: env/cache/pdb-locals.sqlite (table func: key, name, addr, info JSON; table callee: key, layout JSON).
Usage: pdb_locals_xbox.py [--jobs N] [--only NAME-REGEX] [--dump NAME]
"""
import argparse
import bisect
import collections
import json
import multiprocessing
import os
import re
import sqlite3
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import names as pdbnames  # noqa: E402
import srcindex  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
EXE = os.path.join(ROOT, "resources", "xbox", "Fallout_Debug.exe")
OUT = os.path.join(ROOT, "env", "cache", "pdb-locals.sqlite")

# ---------------------------------------------------------------------------------------------------------------
# the exe
# ---------------------------------------------------------------------------------------------------------------


class Image:
    def __init__(self, path: str):
        with open(path, "rb") as handle:
            self.data = handle.read()
        pe = struct.unpack_from("<I", self.data, 0x3C)[0]
        count = struct.unpack_from("<H", self.data, pe + 6)[0]
        optional = struct.unpack_from("<H", self.data, pe + 20)[0]
        self.base = struct.unpack_from("<I", self.data, pe + 24 + 28)[0]
        self.sections = []
        at = pe + 24 + optional
        for _ in range(count):
            name = self.data[at:at + 8].rstrip(b"\0").decode()
            vsize, va, rsize, rptr = struct.unpack_from("<IIII", self.data, at + 8)
            self.sections.append((name, self.base + va, vsize, rptr, rsize))
            at += 40
        self.low = self.base
        self.high = max(va + vsize for _, va, vsize, _, _ in self.sections)

    def address(self, segment: int, offset: int) -> int:
        return self.sections[segment - 1][1] + offset

    def offset(self, va: int) -> int | None:
        for _, start, vsize, rptr, rsize in self.sections:
            if start <= va < start + min(vsize, rsize):
                return rptr + va - start
        return None

    def contains(self, va: int) -> bool:
        return self.low <= va < self.high

    def cstring(self, va: int, limit: int = 48) -> str | None:
        at = self.offset(va)
        if at is None:
            return None
        end = self.data.find(b"\0", at, at + 512)
        raw = self.data[at:end if end >= 0 else at + limit]
        try:
            return raw.decode("ascii")[:limit]
        except UnicodeDecodeError:
            return raw.decode("latin-1")[:limit]


# ---------------------------------------------------------------------------------------------------------------
# PDB types: size and a description pdb_locals_ida.py can build an IDA type from
# ---------------------------------------------------------------------------------------------------------------

# CodeView simple types: base (low byte) -> (name, size, kind)
SIMPLE = {
    0x03: ("void", 0, "void"), 0x08: ("int32_t", 4, "int"),
    0x10: ("int8_t", 1, "int"), 0x20: ("uint8_t", 1, "int"), 0x70: ("char", 1, "int"),
    0x68: ("int8_t", 1, "int"), 0x69: ("uint8_t", 1, "int"),
    0x11: ("int16_t", 2, "int"), 0x21: ("uint16_t", 2, "int"), 0x72: ("int16_t", 2, "int"),
    0x73: ("uint16_t", 2, "int"), 0x71: ("wchar_t", 2, "int"),
    0x12: ("int32_t", 4, "int"), 0x22: ("uint32_t", 4, "int"), 0x74: ("int32_t", 4, "int"),
    0x75: ("uint32_t", 4, "int"),
    0x13: ("int64_t", 8, "int"), 0x23: ("uint64_t", 8, "int"), 0x76: ("int64_t", 8, "int"),
    0x77: ("uint64_t", 8, "int"),
    0x40: ("float", 4, "flt"), 0x41: ("double", 8, "flt"), 0x30: ("bool", 1, "int"),
}


class Types:
    """Type index -> {"b": base name, "bk": prim|rec|enum|fn|void, "p": pointer depth, "c": const base,
    "dims": [counts], "sz": size, "k": int|flt|ptr|rec|arr}."""

    def __init__(self):
        self.records = {}
        for index, kind, name, body in pdbnames.records(pdbnames.dump("types")):
            if kind in ("LF_FIELDLIST", "LF_METHODLIST", "LF_ARGLIST", "LF_VTSHAPE"):
                if kind == "LF_ARGLIST":
                    self.records[index] = (kind, "", [int(m, 16) for line in body
                                                      for m in re.findall(r"^(0x[0-9A-F]+)", line)])
                continue
            self.records[index] = (kind, name or "", " ".join(body))
        self.cache = {}

    def describe(self, index: int) -> dict:
        known = self.cache.get(index)
        if known is None:
            known = self.cache[index] = self._describe(index)
        return known

    def _describe(self, index: int) -> dict:
        if index < 0x1000:
            mode, base = (index >> 8) & 0xF, index & 0xFF
            name, size, kind = SIMPLE.get(base, ("void", 0, "void"))
            if mode:
                return {"b": name, "bk": "void" if kind == "void" else "prim", "p": 1, "sz": 4, "k": "ptr"}
            return {"b": name, "bk": "void" if kind == "void" else "prim", "p": 0, "sz": size, "k": kind}
        record = self.records.get(index)
        if not record:
            return {"b": "void", "bk": "void", "p": 0, "sz": 0, "k": "void"}
        kind, name, text = record
        if kind == "LF_MODIFIER":
            inner = dict(self.describe(int(re.search(r"referent = (0x[0-9A-F]+)", text).group(1), 16)))
            if "const" in text.split("modifiers =")[1] and not inner["p"]:
                inner["c"] = 1
            return inner
        if kind == "LF_POINTER":
            if "mode = pointer to" in text:
                return {"b": "void", "bk": "fn", "p": 1, "sz": 4, "k": "ptr"}
            inner = dict(self.describe(int(re.search(r"referent = (0x[0-9A-F]+)", text).group(1), 16)))
            if inner.get("dims"):
                # a pointer to an array: the array's element
                inner.pop("dims")
            inner["p"] = inner["p"] + 1
            inner["sz"], inner["k"] = 4, "ptr"
            if "mode = ref" in text:
                inner["ref"] = 1
            return inner
        if kind == "LF_ARRAY":
            m = re.search(r"size: (\d+), index type: \S+(?: \([^)]*\))?, element type: (0x[0-9A-F]+)", text)
            inner = dict(self.describe(int(m.group(2), 16)))
            total, element = int(m.group(1)), inner["sz"]
            inner["dims"] = [total // element if element else 0] + inner.get("dims", [])
            inner["sz"], inner["k"] = total, "arr"
            return inner
        if kind in ("LF_CLASS", "LF_STRUCTURE", "LF_UNION"):
            forward = re.search(r"forward ref \(-> (0x[0-9A-F]+)\)", text)
            if forward:
                return self.describe(int(forward.group(1), 16))
            size = re.search(r"sizeof (\d+)", text)
            return {"b": name, "bk": "rec", "p": 0, "sz": int(size.group(1)) if size else 0, "k": "rec"}
        if kind == "LF_ENUM":
            under = re.search(r"underlying type: (0x[0-9A-F]+)", text)
            size = self.describe(int(under.group(1), 16))["sz"] if under else 4
            return {"b": name, "bk": "enum", "p": 0, "sz": size, "k": "int"}
        if kind == "LF_BITFIELD":
            return self.describe(int(re.search(r"type = (0x[0-9A-F]+)", text).group(1), 16))
        if kind in ("LF_PROCEDURE", "LF_MFUNCTION"):
            return {"b": "void", "bk": "fn", "p": 0, "sz": 0, "k": "void"}
        return {"b": "void", "bk": "void", "p": 0, "sz": 0, "k": "void"}

    def returns(self, index: int) -> dict | None:
        record = self.records.get(index)
        if not record or record[0] not in ("LF_PROCEDURE", "LF_MFUNCTION"):
            return None
        m = re.search(r"return type = (0x[0-9A-F]+)", record[2])
        return self.describe(int(m.group(1), 16)) if m else None


# ---------------------------------------------------------------------------------------------------------------
# PDB symbols
# ---------------------------------------------------------------------------------------------------------------

SYMBOL = re.compile(r"^\s*\d+ \| (S_[A-Z0-9_]+) \[size = \d+\](?: `(.*)`)?\s*$")
# llvm-pdbutil prints CodeView register numbers with x86's names; the PowerPC ones are GPR0 = 1 .. GPR31 = 32
STACK_REGISTERS = {"CL": 1, "FLAGS": 31}


class Function:
    __slots__ = ("name", "address", "size", "type", "frame", "variables", "registers", "key")

    def __init__(self, name):
        self.name, self.address, self.size, self.type, self.frame = name, 0, 0, 0, 0
        self.variables = []  # (name, type index, register, offset)
        self.registers = 0


def read_symbols(image: Image):
    """(functions, data address -> name) from the module symbol streams."""
    functions, data = [], {}
    stack, current, pending = [], None, None
    with open(pdbnames.dump("symbols"), encoding="utf-8", errors="replace") as source:
        for line in source:
            m = SYMBOL.match(line)
            if m:
                kind, name = m.group(1), m.group(2)
                pending = (kind, name)
                if kind in ("S_GPROC32", "S_LPROC32"):
                    current = Function(name)
                    functions.append(current)
                    stack = ["proc"]
                elif kind in ("S_BLOCK32", "S_THUNK32"):
                    stack.append("block" if kind == "S_BLOCK32" else "thunk")
                elif kind == "S_END":
                    if stack and stack.pop() == "proc":
                        current = None
                elif kind == "S_REGISTER" and current:
                    current.registers += 1
                continue
            if not pending:
                continue
            kind, name = pending
            text = line.strip()
            if kind in ("S_GPROC32", "S_LPROC32") and current:
                if text.startswith("parent ="):
                    a = re.search(r"addr = (\d+):(\d+), code size = (\d+)", text)
                    current.address, current.size = image.address(int(a.group(1)), int(a.group(2))), int(a.group(3))
                elif text.startswith("type ="):
                    t = re.search(r"type = `(0x[0-9A-F]+)", text)
                    current.type = int(t.group(1), 16) if t else 0
            elif kind == "S_FRAMEPROC" and current and text.startswith("size ="):
                current.frame = int(re.match(r"size = (\d+)", text).group(1))
            elif kind == "S_REGREL32" and current and text.startswith("type ="):
                r = re.match(r"type = (0x[0-9A-F]+).*, register = (\w+), offset = (-?\d+)", text)
                if r:
                    current.variables.append((name, int(r.group(1), 16), r.group(2), int(r.group(3))))
            elif kind in ("S_LDATA32", "S_GDATA32") and text.startswith("type ="):
                a = re.search(r"addr = (\d+):(\d+)", text)
                if a and int(a.group(1)):
                    address = image.address(int(a.group(1)), int(a.group(2)))
                    # a function's static local is named inside its function
                    data.setdefault(address, f"{current.name}::{name}" if current else name)
    return functions, data


def read_named(path: str, kinds: tuple, image: Image) -> dict:
    out = {}
    pending = None
    with open(path, encoding="utf-8", errors="replace") as source:
        for line in source:
            m = SYMBOL.match(line)
            if m:
                pending = m.group(2) if m.group(1) in kinds else None
            elif pending:
                a = re.search(r"addr = (\d+):(\d+)", line)
                if a and int(a.group(1)):
                    out.setdefault(image.address(int(a.group(1)), int(a.group(2))), pending)
                pending = None
    return out


# ---------------------------------------------------------------------------------------------------------------
# the code walk
# ---------------------------------------------------------------------------------------------------------------

LOADS = {32: (4, 0), 33: (4, 0), 34: (1, 0), 35: (1, 0), 40: (2, 0), 41: (2, 0), 42: (2, 0), 43: (2, 0),
         48: (4, 1), 49: (4, 1), 50: (8, 1), 51: (8, 1)}
STORES = {36: (4, 0), 37: (4, 0), 38: (1, 0), 39: (1, 0), 44: (2, 0), 45: (2, 0),
          52: (4, 1), 53: (4, 1), 54: (8, 1), 55: (8, 1)}
UPDATES = {33, 35, 41, 43, 49, 51, 37, 39, 45, 53, 55}
INDEXED_LOADS = {23: (4, 0), 87: (1, 0), 279: (2, 0), 343: (2, 0), 535: (4, 1), 599: (8, 1), 21: (8, 0)}
INDEXED_STORES = {151: (4, 0), 215: (1, 0), 407: (2, 0), 663: (4, 1), 727: (8, 1), 149: (8, 0)}
ARITHMETIC = {266, 40, 235, 491, 459, 104, 8, 10, 138, 136, 202, 200, 75, 11}  # write rD
LOGICAL = {28, 444, 316, 124, 60, 412, 476, 284, 24, 536, 792, 824, 26, 27, 539, 794, 826}  # write rA
HELPERS = re.compile(r"^__(save|rest)(gpr|fpr|vmx)")
VOLATILE_G = (0, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12)


def sign16(v: int) -> int:
    return v - 0x10000 if v & 0x8000 else v


def depth(x) -> int:
    if not isinstance(x, tuple):
        return 0
    return 1 + max((depth(c) for c in x[1:] if isinstance(c, tuple)), default=0)


def op(*children):
    kept = tuple(c for c in children if c is not None)
    if not kept:
        return None
    node = ("op",) + kept
    return node if depth(node) <= 5 else None


def add(x, k: int):
    if x is None:
        return None
    if x[0] == "sp":
        return ("sp", x[1] + k)
    if x[0] == "c":
        return ("c", (x[1] + k) & 0xFFFFFFFF)
    if x[0] == "add":
        return ("add", x[1], x[2] + k)
    return ("add", x, k)


def walk(image: Image, function: Function, helper):
    """The raw events of one function: (kind, ...) with register expressions. Expressions: ("sp", offset) a stack
    address, ("lv", offset, size) a stack slot's value, ("c", value), ("ld", x, disp, size, float), ("add", x, k),
    ("r", call), ("op", x...), ("in", n) an incoming register. helper(address) tells the register save/restore
    helpers, which are no calls."""
    start, size = function.address, function.size
    at = image.offset(start)
    if at is None or size < 8 or size % 4:
        return None
    words = struct.unpack_from(f">{size // 4}I", image.data, at)
    end = start + size
    # block leaders: branch targets and the instructions after branches
    leaders = set()
    last_return = None
    for i, w in enumerate(words):
        pc, opcode = start + i * 4, w >> 26
        if opcode == 18:
            target = ((w & 0x03FFFFFC) ^ 0x02000000) - 0x02000000
            target = target if w & 2 else pc + target
            if not w & 1:
                leaders.add(pc + 4)
                if start <= target < end:
                    leaders.add(target)
                else:
                    last_return = pc
        elif opcode == 16:
            target = sign16(w & 0xFFFC)
            leaders.add(pc + 4)
            leaders.add(target if w & 2 else pc + target)
        elif opcode == 19 and (w >> 1) & 0x3FF in (16, 528) and not w & 1:
            leaders.add(pc + 4)
            if (w >> 1) & 0x3FF == 16:
                last_return = pc
    # the block that returns: every `return x` branches to it or falls into it
    epilogue = None
    if last_return is not None:
        epilogue = max((a for a in leaders if a <= last_return), default=start)

    events, calls = [], []
    G, F = [None] * 32, [None] * 32
    pinned = {1}
    G[1] = ("sp", 0)
    for n in range(8):
        G[3 + n] = ("in", n)
    for n in range(1, 14):
        F[n] = ("inf", n)
    fresh = set()
    ctr = None
    framed = False

    def setg(r, x):
        if r not in pinned:
            G[r] = x
            fresh.add(r)

    def reset():
        nonlocal ctr
        for r in range(32):
            if r not in pinned:
                G[r] = None
            F[r] = None
        fresh.clear()
        ctr = None

    def returned(where):
        events.append(("ret", G[3], F[1], where))

    def call(pc, target, x):
        nonlocal ctr
        index = len(calls)
        calls.append((pc, target, x, {r: G[r] for r in range(3, 11) if G[r] is not None},
                      {r: F[r] for r in range(1, 14) if F[r] is not None}, set(fresh)))
        for r in VOLATILE_G:
            G[r] = None
        for r in range(14):
            F[r] = None
        fresh.clear()
        ctr = None
        G[3] = ("r", index)
        F[1] = ("r", index)

    def load(base, disp, width, isfloat):
        if base is None:
            return None
        if base[0] == "sp":
            return ("lv", base[1] + disp, width) if base[1] + disp >= 0 else None
        if base[0] == "c":
            return ("ld", ("c", (base[1] + disp) & 0xFFFFFFFF), 0, width, isfloat)
        if base[0] == "add":
            return ("ld", base[1], base[2] + disp, width, isfloat)
        node = ("ld", base, disp, width, isfloat)
        return node if depth(node) <= 5 else None

    def store(pc, base, disp, width, value):
        if base is None:
            return
        if base[0] == "sp":
            # the frame is the function's own once its prologue has moved r1; saves made before that are skipped
            if framed and base[1] + disp >= 0 and not (value is not None and value[0] in ("in", "inf")):
                events.append(("asg", base[1] + disp, width, value, pc))
        elif base[0] == "c":
            events.append(("mst", ("c", (base[1] + disp) & 0xFFFFFFFF), 0, value))
        elif base[0] == "add":
            events.append(("mst", base[1], base[2] + disp, value))
        else:
            events.append(("mst", base, disp, value))

    for i, w in enumerate(words):
        pc = start + i * 4
        if pc in leaders:
            if pc == epilogue and i and not _unconditional(words[i - 1]):
                returned("in")
            reset()
            framed = True
        opcode, rd, ra, rb = w >> 26, (w >> 21) & 31, (w >> 16) & 31, (w >> 11) & 31
        simm = sign16(w & 0xFFFF)
        if opcode == 14:
            setg(rd, ("c", simm & 0xFFFFFFFF) if ra == 0 else add(G[ra], simm))
        elif opcode == 15:
            setg(rd, ("c", (simm << 16) & 0xFFFFFFFF) if ra == 0 else add(G[ra], simm << 16))
        elif opcode in (24, 25):
            value, imm = G[rd], (w & 0xFFFF) << (16 if opcode == 25 else 0)
            if imm == 0:
                if ra != rd:
                    setg(ra, value)
            elif value is not None and value[0] == "c":
                setg(ra, ("c", value[1] | imm))
            else:
                setg(ra, op(value))
        elif opcode in LOADS:
            width, isfloat = LOADS[opcode]
            value = load(G[ra] if ra else ("c", 0), simm, width, isfloat)
            if opcode in UPDATES:
                setg(ra, add(G[ra], simm))
            if isfloat:
                F[rd] = value
            else:
                setg(rd, value)
        elif opcode in STORES:
            width, isfloat = STORES[opcode]
            if ra == 1 and opcode == 37 and rd == 1:
                framed = True
            else:
                store(pc, G[ra] if ra else ("c", 0), simm, width, F[rd] if isfloat else G[rd])
                if opcode in UPDATES:
                    setg(ra, add(G[ra], simm))
        elif opcode == 58:
            form, disp = w & 3, sign16(w & 0xFFFC)
            setg(rd, load(G[ra] if ra else ("c", 0), disp, 4 if form == 2 else 8, 0))
            if form == 1:
                setg(ra, add(G[ra], disp))
        elif opcode == 62:
            form, disp = w & 3, sign16(w & 0xFFFC)
            if ra == 1 and rd == 1 and form == 1:
                framed = True
            else:
                store(pc, G[ra] if ra else ("c", 0), disp, 8, G[rd])
                if form == 1:
                    setg(ra, add(G[ra], disp))
        elif opcode == 18:
            target = ((w & 0x03FFFFFC) ^ 0x02000000) - 0x02000000
            target = target if w & 2 else pc + target
            if w & 1:
                if not helper(target):
                    call(pc, target, None)
                    framed = True
            elif target == epilogue:
                returned("in")
            elif not start <= target < end:
                # a tail jump: the register-restore helper returns for the function, anything else is a call
                if helper(target):
                    returned("end")
                else:
                    call(pc, target, None)
                    returned("end")
        elif opcode == 16:
            pass
        elif opcode == 19:
            xo = (w >> 1) & 0x3FF
            if xo == 528:
                if w & 1:
                    call(pc, None, ctr)
            elif xo == 16 and not w & 1:
                returned("end")
        elif opcode in (10, 11):
            events.append(("cmp", G[ra], ("c", (w & 0xFFFF) if opcode == 10 else simm & 0xFFFFFFFF)))
        elif opcode == 31:
            xo = (w >> 1) & 0x3FF
            if xo == 444 and rd == rb:
                if ra != rd:
                    if G[rd] is not None and G[rd][0] == "sp" and ra >= 13:
                        # a frame pointer: it keeps the stack address for the whole function
                        G[ra] = G[rd]
                        pinned.add(ra)
                    else:
                        setg(ra, G[rd])
            elif xo == 183 and rd == 1 and ra == 1:
                framed = True
            elif xo == 467:
                if (ra | rb << 5) == 9:
                    ctr = G[rd]
            elif xo == 339:
                setg(rd, None)
            elif xo in (954, 922, 986):
                setg(ra, G[rd])
            elif xo in (0, 32):
                events.append(("cmp", G[ra], G[rb]))
            elif xo in INDEXED_LOADS:
                width, isfloat = INDEXED_LOADS[xo]
                base = G[rb] if ra == 0 else op(G[ra], G[rb])
                value = ("ld", base, 0, width, isfloat) if base is not None and depth(base) <= 4 else None
                if isfloat:
                    F[rd] = value
                else:
                    setg(rd, value)
            elif xo in INDEXED_STORES:
                width, isfloat = INDEXED_STORES[xo]
                base = G[rb] if ra == 0 else op(G[ra], G[rb])
                if base is not None:
                    events.append(("mst", base, 0, F[rd] if isfloat else G[rd]))
            elif xo & 0x1FF in ARITHMETIC:
                short = xo & 0x1FF
                if short == 266 and G[ra] is not None and G[rb] is not None and "c" in (G[ra][0], G[rb][0]):
                    const, other = (G[ra], G[rb]) if G[ra][0] == "c" else (G[rb], G[ra])
                    setg(rd, add(other, const[1] - (1 << 32) if const[1] & 0x80000000 else const[1]))
                else:
                    setg(rd, op(G[ra], G[rb] if short not in (104, 202, 200) else None))
            elif xo in LOGICAL:
                setg(ra, op(G[rd], G[rb] if xo not in (824, 26, 826) else None))
            else:
                setg(rd, None)
                setg(ra, None)
        elif opcode == 21:
            # rlwinm: a mask with no rotation only narrows the value
            setg(ra, G[rd] if rb == 0 else op(G[rd]))
        elif opcode == 30:
            shift = rb | ((w >> 1) & 1) << 5
            setg(ra, G[rd] if shift == 0 else op(G[rd]))
        elif opcode in (7, 8):
            setg(rd, op(G[ra]))
        elif opcode in (12, 13):
            setg(rd, add(G[ra], simm))
        elif opcode in (26, 27, 28, 29):
            setg(ra, op(G[rd]))
        elif opcode == 63:
            xo = (w >> 1) & 0x3FF
            if xo in (72, 12, 14, 15, 846, 814, 815, 264, 40):
                F[rd] = F[rb]
            elif xo in (0, 32):
                events.append(("cmp", F[ra], F[rb]))
            elif xo & 0x1F in (18, 20, 21, 25, 28, 29, 30, 31, 22, 26, 23):
                F[rd] = op(F[ra], F[rb], F[(w >> 6) & 31] if xo & 0x1F in (25, 28, 29, 30, 31, 23) else None)
            else:
                F[rd] = None
        elif opcode == 59:
            xo = (w >> 1) & 0x1F
            F[rd] = op(F[ra], F[rb], F[(w >> 6) & 31] if xo in (25, 28, 29, 30, 31) else None)
        elif opcode in (4, 5, 6, 46, 47, 56, 57, 60, 61):
            # vector and multi-word forms: nothing tracked
            setg(rd, None)
            setg(ra, None)
        else:
            setg(rd, None)
            setg(ra, None)
            F[rd] = None
    return events, calls


def _unconditional(w: int) -> bool:
    opcode = w >> 26
    if opcode == 18:
        return not w & 1
    if opcode == 19 and (w >> 1) & 0x3FF in (16, 528):
        return (w >> 21) & 0x14 == 0x14 and not w & 1
    return False


# ---------------------------------------------------------------------------------------------------------------
# events over variables
# ---------------------------------------------------------------------------------------------------------------

class Context:
    def __init__(self):
        self.image = Image(EXE)
        self.types = Types()
        functions, self.data = read_symbols(self.image)
        for address, name in read_named(pdbnames.dump("globals"), ("S_GDATA32", "S_LDATA32"), self.image).items():
            self.data.setdefault(address, name)
        publics = read_named(pdbnames.dump("publics"), ("S_PUB32",), self.image)
        # one function per address: the first record (the linker folds no code in this build)
        self.functions = {}
        for f in functions:
            if f.address and f.address not in self.functions:
                f.key = srcindex.name_key(f.name)
                self.functions[f.address] = f
        self.strings, self.reals, self.code_names = {}, {}, {}
        mangled = []
        for address, name in publics.items():
            if name.startswith("??_C@"):
                self.strings[address] = True
            elif name.startswith("__real@"):
                self.reals[address] = len(name) - 7
            elif address not in self.functions and address not in self.data:
                mangled.append((address, name))
        self.publics = publics
        text = srcindex.undname([n for _, n in mangled if n.startswith("?")])
        demangled = dict(zip([n for _, n in mangled if n.startswith("?")], text))
        section = {name: (start, start + size) for name, start, size, _, _ in self.image.sections}
        lo, hi = section[".text"]
        for address, name in mangled:
            plain = name
            if name.startswith("?"):
                signature = demangled.get(name)
                if not signature:
                    continue
                plain = srcindex.qualified_of_signature(signature) if "(" in signature else \
                    signature.rsplit(" ", 1)[-1]
            if lo <= address < hi:
                self.code_names[address] = srcindex.name_key(plain)
            else:
                self.data[address] = plain
        self.data_addresses = sorted(self.data)
        self.layouts = {}

    def callee(self, address: int) -> str:
        f = self.functions.get(address)
        if f:
            return f.key
        return self.code_names.get(address) or "?"

    def helper(self, address: int) -> bool:
        return bool(HELPERS.match(self.code_names.get(address, "")) or HELPERS.match(self.publics.get(address, "")))

    def layout(self, f: Function):
        """(variables, slots, hidden result): the function's variables as dicts and its named stack slots."""
        known = self.layouts.get(f.address)
        if known:
            return known
        variables, positions = [], []
        seen = set()
        for name, index, register, offset in f.variables:
            if register not in STACK_REGISTERS or (name, offset) in seen:
                continue
            seen.add((name, offset))
            t = self.types.describe(index)
            v = {"n": name, "t": t, "o": offset}
            if offset >= f.frame:
                v["pos"] = (offset - f.frame - 16) // 8
                positions.append(v["pos"])
            variables.append(v)
        returns = self.types.returns(f.type)
        hidden = bool(positions) and min(positions) == 1 or \
            (not positions and bool(returns) and returns["k"] == "rec" and not returns["p"])
        if hidden:
            variables.insert(0, {"n": "result", "t": dict(returns or {"b": "void", "bk": "void", "sz": 0}, p=1, sz=4,
                                                          k="ptr"), "o": f.frame + 20, "pos": 0, "hid": 1})
        explicit = 0
        for v in sorted((v for v in variables if "pos" in v), key=lambda v: v["pos"]):
            if v.get("hid"):
                v["p"] = "ret"
            elif v["n"] == "this":
                v["p"] = "this"
            else:
                v["p"] = str(explicit)
                explicit += 1
        slots = sorted((v["o"], max(v["t"]["sz"], 1), i) for i, v in enumerate(variables))
        result = (variables, slots, hidden, returns)
        self.layouts[f.address] = result
        return result

    def argument_labels(self, callee: Function):
        """position -> (label, float register or 0) of the callee's parameters."""
        variables, _, _, _ = self.layout(callee)
        out, floats = {}, 0
        for v in sorted((v for v in variables if "pos" in v), key=lambda v: v["pos"]):
            isfloat = v["t"]["k"] == "flt"
            floats += isfloat
            out[v["pos"]] = (v["p"], floats if isfloat else 0)
        return out

    def constant(self, value: int, loaded: bool, width: int = 4, isfloat: int = 0):
        """A constant, or what the image holds at it."""
        if not self.image.contains(value):
            if loaded:
                return None
            return ["c", value - (1 << 32) if value & 0x80000000 else value]
        if loaded:
            if value in self.reals or isfloat:
                at = self.image.offset(value)
                if at is None:
                    return None
                number = struct.unpack_from(">f" if width == 4 else ">d", self.image.data, at)[0]
                return ["fc", float_text(number)]
            name = self.symbol(value)
            return ["g", name] if name else None
        if value in self.strings:
            return ["s", self.image.cstring(value)]
        f = self.functions.get(value)
        if f:
            return ["ga", f.key]
        name = self.symbol(value)
        return ["ga", name] if name else None

    def symbol(self, value: int) -> str | None:
        name = self.data.get(value)
        if name:
            return name
        at = bisect.bisect_right(self.data_addresses, value) - 1
        if at >= 0 and value - self.data_addresses[at] < 0x1000:
            base = self.data_addresses[at]
            return f"{self.data[base]}+{value - base}"
        return None

    def facts(self, f: Function) -> dict | None:
        variables, slots, hidden, returns = self.layout(f)
        if not variables or f.registers:
            return None
        walked = walk(self.image, f, self.helper)
        if not walked:
            return None
        raw_events, raw_calls = walked
        starts = [s[0] for s in slots]

        def slot(offset):
            at = bisect.bisect_right(starts, offset) - 1
            if at >= 0 and offset < slots[at][0] + slots[at][1]:
                return slots[at][2], offset - slots[at][0]
            return None

        def cx(x):
            if x is None:
                return None
            kind = x[0]
            if kind == "sp":
                hit = slot(x[1])
                return ["a", hit[0], hit[1]] if hit else ["ta", x[1]]
            if kind == "lv":
                hit = slot(x[1])
                return ["v", hit[0], hit[1]] if hit else ["t", x[1]]
            if kind == "c":
                return self.constant(x[1], False)
            if kind == "ld":
                if x[1][0] == "c" and x[2] == 0:
                    return self.constant(x[1][1], True, x[3], x[4])
                inner = cx(x[1])
                return ["ld", inner, x[2]] if inner else None
            if kind == "add":
                inner = cx(x[1])
                return ["add", inner, x[2]] if inner else None
            if kind == "r":
                return ["r", x[1]]
            if kind == "op":
                inner = [c for c in map(cx, x[1:]) if c]
                return ["op", inner] if inner else None
            return None

        events = []
        for index, (pc, target, through, gprs, fprs, fresh) in enumerate(raw_calls):
            callee, labels = "?", None
            if target is not None:
                callee = self.callee(target)
                known = self.functions.get(target)
                if known and not known.registers:
                    labels = self.argument_labels(known)
            elif through is not None:
                # a virtual call loads the function from the object's table: (*(*object + offset))
                if through[0] == "ld" and through[1] is not None and through[1][0] == "ld" and through[1][2] == 0:
                    callee = f"v:{through[2]}"
                else:
                    inner = cx(through)
                    callee = "i:" + inner[1] if inner and inner[0] == "g" else "?"
            arguments = []
            if labels is not None:
                for position, (label, float_register) in sorted(labels.items()):
                    x = fprs.get(float_register) if float_register else gprs.get(3 + position)
                    arguments.append([label, cx(x)])
            else:
                virtual = callee.startswith("v:")
                for r in range(3, 11):
                    if r not in fresh and not (r in gprs and gprs[r][0] == "r"):
                        break
                    position = r - 3 - virtual
                    arguments.append(["this" if position < 0 else str(position), cx(gprs.get(r))])
            events.append(["call", index, callee, arguments])
        for e in raw_events:
            if e[0] == "asg":
                hit, value = slot(e[1]), cx(e[3])
                if hit:
                    if value is not None:
                        events.append(["asg", hit[0], hit[1], value])
                else:
                    events.append(["tasg", e[1], e[2], value])
            elif e[0] == "mst":
                if e[1][0] == "c":
                    base = self.constant(e[1][1], False)
                    base = ["ga", base[1]] if base and base[0] in ("ga",) else None
                else:
                    base = cx(e[1])
                value = cx(e[3])
                if base and value:
                    events.append(["mst", base, e[2], value])
            elif e[0] == "ret":
                if returns and not hidden and returns["k"] != "void":
                    value = cx(e[2] if returns["k"] == "flt" and not returns["p"] else e[1])
                    if value:
                        events.append(["ret", value, e[3]])
            elif e[0] == "cmp":
                a, b = cx(e[1]), cx(e[2])
                if a and b:
                    events.append(["cmp", a, b])
        # the returning block's own value where it sets one, else the values the branches into it carry
        own = any(e[0] == "ret" and e[2] == "end" for e in events)
        events = [e[:2] if e[0] == "ret" else e for e in events if e[0] != "ret" or not own or e[2] == "end"]
        events = resolve_temporaries(events)
        # a call's index is its place among the calls kept
        order = {}
        for e in events:
            if e[0] == "call":
                order.setdefault(e[1], len(order))
        events = [e for e in (renumber(e, order) for e in events) if complete(e)]
        unique = []
        seen = set()
        for e in events:
            text = json.dumps(e)
            if e[0] == "call" or text not in seen:
                seen.add(text)
                unique.append(e)
        kept = [{k: v[k] for k in ("n", "t", "p", "hid", "o") if k in v} for v in variables]
        return {"vars": kept, "ev": unique, "ret": returns["k"] if returns else None, "hid": int(hidden)}


def float_text(number: float) -> str:
    try:
        number = struct.unpack("<f", struct.pack("<f", number))[0]
    except (OverflowError, struct.error):
        pass
    return "%.9g" % number


def mentions_temporary(x) -> bool:
    if not isinstance(x, list):
        return False
    if x and x[0] in ("t", "ta"):
        return True
    return any(mentions_temporary(c) for c in x)


def substitute(x, sources):
    """x with each temporary replaced by what was stored in it (the first source; None when there is none)."""
    if not isinstance(x, list) or not x:
        return x
    if x[0] in ("t", "ta"):
        found = sources.get(x[1])
        if not found:
            return None
        value = found[0]
        if x[0] == "ta":
            # the address of a temporary object: the object, when it is a call's result
            return value if value and value[0] == "r" else None
        return value
    if x[0] in ("ld", "add"):
        inner = substitute(x[1], sources)
        return [x[0], inner, x[2]] if inner else None
    if x[0] == "op":
        inner = [c for c in (substitute(c, sources) for c in x[1]) if c]
        return ["op", inner] if inner else None
    return x


def resolve_temporaries(events: list) -> list:
    sources = collections.defaultdict(list)
    spans = {}
    # a temporary passed by address as a call's hidden result holds that result
    for e in events:
        if e[0] == "call":
            for label, x in e[3]:
                if label == "ret" and x and x[0] == "ta":
                    sources[x[1]].append(["r", e[1]])
    for _ in range(3):
        changed = False
        for e in events:
            if e[0] != "tasg" or e[3] is None:
                continue
            value = substitute(e[3], sources) if mentions_temporary(e[3]) else e[3]
            if value is None or mentions_temporary(value):
                continue
            # a wide store read back in parts (a double's low word): each part reads the whole
            for offset in range(e[1], e[1] + max(e[2], 1), 4):
                if value not in sources[offset]:
                    sources[offset].append(value)
                    changed = True
            spans[e[1]] = e[2]
        if not changed:
            break
    out = []
    for e in events:
        if e[0] == "tasg":
            continue
        if e[0] == "call":
            arguments = [[label, substitute(x, sources) if mentions_temporary(x) else x] for label, x in e[3]]
            out.append(["call", e[1], e[2], arguments])
            continue
        alternatives = [e]
        if mentions_temporary(e):
            # one event per source of a temporary with several (a conditional expression's two values)
            offsets = set()
            collect_temporaries(e, offsets)
            alternatives = []
            for pick in range(max((len(sources.get(o, [])) for o in offsets), default=0)):
                chosen = {o: [sources[o][min(pick, len(sources[o]) - 1)]] for o in offsets if sources.get(o)}
                alternatives.append([e[0]] + [substitute(part, chosen) for part in e[1:]])
        out += [a for a in alternatives if complete(a)]
    return out


def complete(e) -> bool:
    """Whether an event still has every operand it needs."""
    if e[0] == "asg":
        return holds(e[3])
    if e[0] == "mst":
        return holds(e[1]) and holds(e[3])
    if e[0] == "ret":
        return holds(e[1])
    if e[0] == "cmp":
        return holds(e[1]) and holds(e[2])
    return True


def holds(x) -> bool:
    if x is None:
        return False
    if x[0] in ("ld", "add"):
        return holds(x[1])
    if x[0] == "r":
        return x[1] is not None
    return True


def collect_temporaries(x, into: set) -> None:
    if isinstance(x, list):
        if x and x[0] in ("t", "ta"):
            into.add(x[1])
        else:
            for c in x:
                collect_temporaries(c, into)


def renumber(x, order: dict):
    if not isinstance(x, list) or not x:
        return x
    if x[0] == "call":
        return ["call", order[x[1]], x[2], [[label, renumber(a, order)] for label, a in x[3]]]
    if x[0] == "r":
        return ["r", order.get(x[1])]
    return [renumber(c, order) for c in x]


# ---------------------------------------------------------------------------------------------------------------

CONTEXT = None


def work(addresses: list) -> list:
    out = []
    for address in addresses:
        f = CONTEXT.functions[address]
        try:
            info = CONTEXT.facts(f)
        except Exception as error:  # one function's failure is reported, not fatal
            print(f"pdb_locals_xbox: {f.name} at {address:#x}: {error!r}", file=sys.stderr)
            info = None
        if info:
            out.append((f.key, f.name, address, json.dumps(info, separators=(",", ":"))))
    return out


def main() -> None:
    global CONTEXT
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--jobs", type=int, default=os.cpu_count())
    parser.add_argument("--only", help="only functions whose name matches this regex")
    parser.add_argument("--dump", help="print one function's facts (exact name) and write nothing")
    arguments = parser.parse_args()
    CONTEXT = Context()
    functions = CONTEXT.functions
    if arguments.dump:
        for address, f in functions.items():
            if f.name == arguments.dump:
                info = CONTEXT.facts(f)
                print(f"{f.name} at {address:#x}")
                if info:
                    for i, v in enumerate(info["vars"]):
                        print(f"  var {i}: {json.dumps(v)}")
                    for e in info["ev"]:
                        print(f"  {json.dumps(e)}")
        return
    wanted = sorted(a for a, f in functions.items()
                    if not arguments.only or re.search(arguments.only, f.name))
    chunks = [wanted[i:i + 500] for i in range(0, len(wanted), 500)]
    rows = []
    with multiprocessing.get_context("fork").Pool(arguments.jobs) as pool:
        for part in pool.imap(work, chunks):
            rows += part
    # each function name's parameter layouts (hidden result, `this`, explicit parameter names): one per overload;
    # none for a name whose functions keep their parameters in registers
    layouts = collections.defaultdict(set)
    for f in functions.values():
        found = layouts[f.key]
        if f.registers:
            continue
        variables, _, hidden, _ = CONTEXT.layout(f)
        params = [v["n"] for v in sorted((v for v in variables if "pos" in v), key=lambda v: v["pos"])
                  if v.get("p", "").isdigit()]
        found.add(json.dumps({"hid": int(hidden), "this": int(any(v.get("p") == "this" for v in variables)),
                              "params": params}))
    if os.path.exists(OUT):
        os.remove(OUT)
    db = sqlite3.connect(OUT)
    db.execute("create table func (id integer primary key, key text, name text, addr integer, info text)")
    db.execute("create table callee (key text primary key, layouts text)")
    db.executemany("insert into func (key, name, addr, info) values (?, ?, ?, ?)", rows)
    db.executemany("insert into callee values (?, ?)",
                   [(key, "[" + ",".join(sorted(found)) + "]") for key, found in sorted(layouts.items())])
    db.execute("create index func_key on func (key)")
    db.commit()
    db.close()
    print(f"pdb_locals_xbox: {len(rows)} of {len(functions)} functions with stack variables -> {OUT}")


if __name__ == "__main__":
    main()
