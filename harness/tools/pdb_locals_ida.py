"""Names and types the PC database's Hex-Rays locals from the Xbox 360 debug build. Runs inside IDA (GUI or idalib).

pdb_locals_xbox.py wrote, per debug function, its variables and the events its code has for them (calls with their
arguments, stores, returns, comparisons). This reads the same events off the PC function's ctree, describes every
variable on both sides by the events it takes part in ("argument `this` of the 2nd TESObjectCELL::GetInterior call",
"assigned TESObjectREFR::GetParentCell(apRef)", "returned"), and pairs a PC local with the debug local that shares
enough of them. Three kinds of pairs, in this order:

  feat    shared events. Parameters pair by position; each round's pairs name the context of the next round's
          events (`x = pc1->GetWorldSpace()` only tells x apart once pc1 is known).
  layout  the frame. Both compilers lay a scope's locals out in declaration order, so the PC stack locals between
          two paired neighbours are the debug locals between their two debug locals, when the counts and sizes
          agree; a run of paired neighbours extends over its unpaired ends the same way.
  copy    a PC local whose only pairing is that it is copied into a paired one the debug build never copies
          (the value before it is pushed as an argument).

A pair is written as a Hex-Rays user lvar setting: the debug local's name, and its type where the PC local can hold
it. One debug local the optimizer split over several PC locals gives name, name_2, ...; one PC local that holds
several debug locals (a reused slot) gets their names joined with `__`. Parameters keep the prototype's names and
types; one Hex-Rays named itself (a1) gets the debug name.

In IDA:   import pdb_locals_ida as P
          P.preview(ea)                       -> the pairs with their evidence, nothing written
          P.run([ea, ...], apply=True)        -> writes the settings; rows go to env/scratch/pdblocals/report.tsv
"""
import bisect
import collections
import json
import os
import re
import sqlite3
import struct
import sys
import time

import ida_bytes
import ida_frame
import ida_funcs
import ida_gdl
import ida_hexrays as hx
import ida_lines
import ida_name
import ida_typeinf
import ida_ua
import idaapi
import idautils
import idc

TOOLS = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, TOOLS)
import names as pdbnames  # noqa: E402
import srcindex  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(TOOLS))
FACTS = os.path.join(ROOT, "env", "cache", "pdb-locals.sqlite")
REPORT = os.path.join(ROOT, "env", "scratch", "pdblocals", "report.tsv")

# a pair needs this much shared evidence (one call argument or call result nothing else shares is 1.0)
THRESHOLD = 0.6
ROUNDS = 5
# the score written for a parameter, a layout pair and a copy pair
PARAMETER, LAYOUT, COPY = 9.0, 0.5, 0.4
RELATIONAL = {hx.cot_eq, hx.cot_ne, hx.cot_sge, hx.cot_uge, hx.cot_sle, hx.cot_ule, hx.cot_sgt, hx.cot_ugt,
              hx.cot_slt, hx.cot_ult}
LOGICAL = {hx.cot_lnot, hx.cot_land, hx.cot_lor}
COMPOUND = {hx.cot_asgbor, hx.cot_asgxor, hx.cot_asgband, hx.cot_asgadd, hx.cot_asgsub, hx.cot_asgmul,
            hx.cot_asgsshr, hx.cot_asgushr, hx.cot_asgshl, hx.cot_asgsdiv, hx.cot_asgudiv, hx.cot_asgsmod,
            hx.cot_asgumod}
STEPS = {hx.cot_preinc: 1, hx.cot_postinc: 1, hx.cot_predec: -1, hx.cot_postdec: -1}
# Hex-Rays helpers that only reinterpret their operand
TRANSPARENT = re.compile(r"^(LO(BYTE|WORD|DWORD)|SLO(BYTE|WORD|DWORD)|COERCE_\w+|BYTE\d?|WORD\d?|DWORD\d?)$")
# a name Hex-Rays gave a parameter itself
AUTOMATIC = re.compile(r"^(a\d+|arg_\w+|result)$")
# a name IDA gave a global itself
AUTO_GLOBAL = re.compile(r"^(dword|byte|word|qword|flt|dbl|tbyte|off|unk|stru|asc|xmmword|a[A-Z]\w*)_[0-9A-Fa-f]+$|^$")
# the debug build's types that are no named type of the database
BUILTIN = {"bool": ida_typeinf.BTF_BOOL, "char": ida_typeinf.BTF_CHAR, "float": ida_typeinf.BTF_FLOAT,
           "double": ida_typeinf.BTF_DOUBLE}

_db = None
_layouts = None
_function_keys = {}
_global_keys = {}
_named_types = None
_pc_shapes = {}
_debug_shapes = {}
_sizes = {}
_instances = None
_global_addresses = {}


# ---------------------------------------------------------------------------------------------------------------
# the debug build's facts
# ---------------------------------------------------------------------------------------------------------------

def database():
    global _db, _layouts
    if _db is None:
        _db = sqlite3.connect(FACTS)
        _layouts = {key: json.loads(layouts) for key, layouts in _db.execute("select key, layouts from callee")}
    return _db


def candidates(key: str) -> list:
    return [(name, address, json.loads(info)) for name, address, info in
            database().execute("select name, addr, info from func where key = ? order by addr", (key,))]


def function_key(ea: int) -> str:
    """The name key (srcindex.name_key) of the PC function at ea, '?' when its name is none the PDB could have.
    IDA's `_N` suffix of a repeated name is dropped where the debug build has no function of the suffixed name."""
    known = _function_keys.get(ea)
    if known is None:
        database()
        name = ida_funcs.get_func_name(ea) or ida_name.get_name(ea) or ""
        if name.startswith("?") and not name.startswith("??"):
            name = ida_name.demangle_name(name, 0) or name
        qualified = srcindex.qualified_of_ida(name) if name else None
        known = srcindex.name_key(qualified) if qualified else "?"
        if known not in _layouts and re.search(r"_\d+$", known):
            known = re.sub(r"_\d+$", "", known)
        _function_keys[ea] = known
    return known


def load_names(path: str) -> int:
    """Takes a table's debug names (`address<TAB>debug function key` per line) for the PC functions at those
    addresses, over the names the database has: names an earlier run's votes gave (pdb_names.py's
    names-accepted.tsv), not in the database yet. Returns the number taken."""
    database()
    taken = 0
    with open(path, encoding="utf-8") as source:
        for line in source:
            if line.strip():
                address, key = line.rstrip("\n").split("\t")[:2]
                _function_keys[int(address, 16)] = key
                taken += 1
    return taken


def global_key(ea: int) -> str:
    known = _global_keys.get(ea)
    if known is None:
        name = ida_name.get_name(ea) or ""
        if name.startswith("?"):
            text = ida_name.demangle_name(name, ida_name.MNG_SHORT_FORM) or name
            name = text.rsplit(" ", 1)[-1]
        known = _global_keys[ea] = name or "?"
        _global_addresses[known] = ea
    return known


def float_text(number: float) -> str:
    try:
        number = struct.unpack("<f", struct.pack("<f", number))[0]
    except (OverflowError, struct.error):
        pass
    return "%.9g" % number


# ---------------------------------------------------------------------------------------------------------------
# the PC function's events, in pdb_locals_xbox.py's form
# ---------------------------------------------------------------------------------------------------------------

def plus(x, k: int):
    if x is None:
        return None
    if x[0] == "a":
        return ["a", x[1], x[2] + k]
    if x[0] == "add":
        return ["add", x[1], x[2] + k]
    if x[0] == "ga":
        return ["ga", f"{x[1]}+{k}"] if k else x
    return ["add", x, k] if k else x


def loaded(x):
    """The value at address x."""
    if x is None:
        return None
    if x[0] == "a":
        return ["v", x[1], x[2]]
    if x[0] == "ga":
        return ["g", x[1]]
    if x[0] == "add":
        return ["ld", x[1], x[2]]
    return ["ld", x, 0]


class Events(hx.ctree_visitor_t):
    def __init__(self, cfunc, shapes: bool = True):
        super().__init__(hx.CV_FAST)
        self.shapes = shapes
        self.cfunc = cfunc
        self.events = []
        self.calls = {}  # obj_id -> provisional index
        self.call_order = []  # (ea, provisional index)
        self.targets = {}  # provisional index -> the address a direct call calls

    # -- expressions ----------------------------------------------------------------------------------------
    def address(self, e, depth=0):
        """The address of an lvalue."""
        while e.op == hx.cot_cast:
            e = e.x
        op = e.op
        if depth > 8:
            return None
        if op == hx.cot_var:
            return ["a", e.v.idx, 0]
        if op == hx.cot_obj:
            ea = e.obj_ea
            f = ida_funcs.get_func(ea)
            return ["ga", function_key(ea) if f and f.start_ea == ea else global_key(ea)]
        if op == hx.cot_memref:
            return plus(self.address(e.x, depth + 1), e.m)
        if op == hx.cot_memptr:
            return plus(self.value(e.x, depth + 1), e.m)
        if op == hx.cot_idx:
            if e.y.op != hx.cot_num:
                base = self.address(e.x, depth + 1) if e.x.type.is_array() else self.value(e.x, depth + 1)
                inner = [c for c in (base, self.value(e.y, depth + 1)) if c]
                return ["op", inner] if inner else None
            size = e.type.get_size()
            size = size if 0 < size < 0x10000 else 1
            k = self.number(e.y) * size
            if e.x.type.is_array():
                return plus(self.address(e.x, depth + 1), k)
            return plus(self.value(e.x, depth + 1), k)
        if op == hx.cot_ptr:
            return self.value(e.x, depth + 1)
        return None

    @staticmethod
    def number(e) -> int:
        value = e.numval() & 0xFFFFFFFF
        return value - (1 << 32) if value & 0x80000000 else value

    def call_index(self, e) -> int:
        index = self.calls.get(e.obj_id)
        if index is None:
            index = self.calls[e.obj_id] = len(self.calls)
            self.call_order.append((e.ea, index))
        return index

    def value(self, e, depth=0):
        while e.op == hx.cot_cast:
            e = e.x
        op = e.op
        if depth > 8:
            return None
        if op == hx.cot_var:
            return ["v", e.v.idx, 0]
        if op == hx.cot_num:
            return ["c", self.number(e)]
        if op == hx.cot_fnum:
            try:
                return ["fc", float_text(float(ida_lines.tag_remove(e.print1(None)).rstrip("fF")))]
            except ValueError:
                return None
        if op == hx.cot_str:
            return ["s", e.string[:48]]
        if op == hx.cot_obj:
            ea = e.obj_ea
            if ida_bytes.is_strlit(ida_bytes.get_flags(ea)):
                raw = idc.get_strlit_contents(ea) or b""
                return ["s", raw.decode("latin-1")[:48]]
            f = ida_funcs.get_func(ea)
            if f and f.start_ea == ea:
                return ["ga", function_key(ea)]
            if e.type.is_array():
                return ["ga", global_key(ea)]
            return ["g", global_key(ea)]
        if op == hx.cot_ref:
            return self.address(e.x, depth + 1)
        if op in (hx.cot_memref, hx.cot_memptr, hx.cot_idx, hx.cot_ptr):
            return loaded(self.address(e, depth))
        if op in (hx.cot_add, hx.cot_sub) and e.y.op == hx.cot_num:
            scale = 1
            if e.x.type.is_ptr():
                scale = e.x.type.get_ptrarr_objsize()
                scale = scale if 0 < scale < 0x10000 else 1
            k = self.number(e.y) * scale
            return plus(self.value(e.x, depth + 1), k if op == hx.cot_add else -k)
        if op == hx.cot_call:
            if e.x.op == hx.cot_helper:
                if TRANSPARENT.match(e.x.helper) and len(e.a) == 1:
                    return self.value(e.a[0], depth + 1)
                inner = [c for c in (self.value(a, depth + 1) for a in e.a) if c]
                return ["op", inner] if inner else None
            return ["r", self.call_index(e)]
        inner = []
        if hx.op_uses_x(op):
            inner.append(self.value(e.x, depth + 1))
        if hx.op_uses_y(op):
            inner.append(self.value(e.y, depth + 1))
        if hx.op_uses_z(op):
            inner.append(self.value(e.z, depth + 1))
        inner = [c for c in inner if c]
        return ["op", inner] if inner else None

    # -- events ---------------------------------------------------------------------------------------------
    def condition(self, e) -> None:
        """A condition that is a plain value compares it with zero."""
        while e.op == hx.cot_cast:
            e = e.x
        if e.op in RELATIONAL or e.op in LOGICAL:
            return
        value = self.value(e)
        if value:
            self.events.append(["cmp", value, ["c", 0]])

    def visit_insn(self, i):
        op = i.op
        if op == hx.cit_if:
            self.condition(i.cif.expr)
        elif op == hx.cit_while:
            self.condition(i.cwhile.expr)
        elif op == hx.cit_do:
            self.condition(i.cdo.expr)
        elif op == hx.cit_for:
            self.condition(i.cfor.expr)
        elif op == hx.cit_return:
            value = self.value(i.creturn.expr)
            if value:
                self.events.append(["ret", value])
        return 0

    def visit_expr(self, e):
        op = e.op
        if op == hx.cot_call:
            self.call(e)
        elif op == hx.cot_asg:
            target, value = self.address(e.x), self.value(e.y)
            if target and value:
                if target[0] == "a":
                    self.events.append(["asg", target[1], target[2], value])
                elif target[0] == "add":
                    self.events.append(["mst", target[1], target[2], value])
                else:
                    self.events.append(["mst", target, 0, value])
        elif op in COMPOUND or op in STEPS:
            target = self.address(e.x)
            if target and target[0] == "a":
                scale = 1
                if e.x.type.is_ptr():
                    scale = e.x.type.get_ptrarr_objsize()
                    scale = scale if 0 < scale < 0x10000 else 1
                old = ["v", target[1], target[2]]
                if op in STEPS:
                    value = ["add", old, scale * STEPS[op]]
                elif op in (hx.cot_asgadd, hx.cot_asgsub) and e.y.op == hx.cot_num:
                    value = ["add", old, scale * self.number(e.y) * (1 if op == hx.cot_asgadd else -1)]
                else:
                    other = self.value(e.y)
                    value = ["op", [old, other] if other else [old]]
                self.events.append(["asg", target[1], target[2], value])
        elif op in RELATIONAL:
            a, b = self.value(e.x), self.value(e.y)
            if a and b:
                self.events.append(["cmp", a, b])
        elif op == hx.cot_lnot:
            self.condition(e.x)
        elif op in (hx.cot_land, hx.cot_lor):
            self.condition(e.x)
            self.condition(e.y)
        return 0

    def call(self, e) -> None:
        f = e.x
        while f.op == hx.cot_cast:
            f = f.x
        if f.op == hx.cot_helper:
            return
        key, layout, this_first = "?", None, False
        arguments = [self.value(a) for a in e.a]
        if f.op == hx.cot_obj:
            key = function_key(f.obj_ea)
            # the overload with this many parameters
            known = _layouts.get(key) or []
            fitting = [one for one in known if one["this"] + one["hid"] + len(one["params"]) == len(arguments)]
            layout = fitting[0] if len(fitting) == 1 else known[0] if len(known) == 1 else None
            this_first = takes_this(f.type)
            self.targets[self.call_index(e)] = f.obj_ea
            shape = pc_shape(f.obj_ea) if self.shapes else None
            # a trivial callee goes by its shape, and by its name for where the name is its own
            alias = [canonical(key)] if shape else []
            key = "t:" + shape if shape else canonical(key)
        else:
            through = self.value(f)
            # a virtual call loads the function from the object's table: (*(*object + offset))
            if through and through[0] == "ld" and through[1] and through[1][0] == "ld" and through[1][2] == 0:
                key = f"v:{through[2]}"
                this_first = True
            elif through and through[0] == "g":
                key = "i:" + through[1]
        if layout:
            labels = (["this"] if layout["this"] else []) + (["ret"] if layout["hid"] else [])
        else:
            labels = ["this"] if this_first else []
        labelled = []
        for n, x in enumerate(arguments):
            labelled.append([labels[n] if n < len(labels) else str(n - len(labels)), x])
        self.events.append(["call", self.call_index(e), key, labelled] + (alias if f.op == hx.cot_obj else []))

    def finish(self) -> list:
        """The events with the calls numbered in address order."""
        order = {index: n for n, (_, index) in enumerate(sorted(self.call_order))}
        self.targets = {order[index]: ea for index, ea in self.targets.items() if index in order}
        return [renumber(e, order) for e in self.events]


def takes_this(t) -> bool:
    """Whether a callee's type is a __thiscall function's."""
    if t.is_ptr():
        t = t.get_pointed_object()
    return t.is_func() and (t.get_cc() & ida_typeinf.CM_CC_MASK) == ida_typeinf.CM_CC_THISCALL


def canonical(key: str) -> str:
    """A callee's name without template arguments: the linker folds a template's identical instances into one
    function, which carries one instance's name."""
    if key == "?" or key[:2] in ("v:", "i:", "t:"):
        return key
    return pdbnames.strip_templates(key)


def trivial_shape(events: list, first, second) -> str | None:
    """What a function without calls does, where that is one of: return a field of its first parameter (get:N),
    return its address (adr:N), return a constant (ret:N), store its second parameter into a field of the first
    (set:N). The linker folds every function of one shape into one, which carries one of their names, so a call to
    one is told by its shape."""
    if any(e[0] in ("call", "cmp", "asg") for e in events):
        return None
    returns = [e[1] for e in events if e[0] == "ret"]
    stores = [e for e in events if e[0] == "mst"]
    if len(returns) == 1 and not stores:
        x = returns[0]
        if x[0] == "c":
            return f"ret:{x[1]}"
        if first is None:
            return None
        if x == ["v", first, 0]:
            return "adr:0"
        if x[0] in ("ld", "add") and x[1] == ["v", first, 0]:
            return f"{'get' if x[0] == 'ld' else 'adr'}:{x[2]}"
        return None
    if not returns and len(stores) == 1 and first is not None and second is not None:
        _, base, disp, x = stores[0]
        if base == ["v", first, 0] and x == ["v", second, 0]:
            return f"set:{disp}"
    return None


def pc_shape(ea: int) -> str | None:
    """trivial_shape of the PC function at ea."""
    if ea in _pc_shapes:
        return _pc_shapes[ea]
    _pc_shapes[ea] = None
    f = ida_funcs.get_func(ea)
    if not f or f.start_ea != ea or f.end_ea - ea > 48:
        return None
    try:
        cfunc = hx.decompile(ea)
    except hx.DecompilationFailure:
        return None
    if not cfunc:
        return None
    visitor = Events(cfunc, shapes=False)
    visitor.apply_to(cfunc.body, None)
    arguments = list(cfunc.argidx)
    _pc_shapes[ea] = trivial_shape(visitor.finish(), arguments[0] if arguments else None,
                                   arguments[1] if len(arguments) > 1 else None)
    return _pc_shapes[ea]


def debug_shape(key: str) -> str | None:
    """trivial_shape of the debug functions of a name, where they all have one."""
    if key in _debug_shapes:
        return _debug_shapes[key]
    shapes = set()
    for (text,) in database().execute("select info from func where key = ? limit 9", (key,)):
        if len(text) > 2000:
            shapes.add(None)
            break
        info = json.loads(text)
        labels = {v["p"]: i for i, v in enumerate(info["vars"]) if "p" in v}
        first, second = ("this", "0") if "this" in labels else ("0", "1")
        shapes.add(trivial_shape(info["ev"], labels.get(first), labels.get(second)))
    _debug_shapes[key] = shapes.pop() if len(shapes) == 1 else None
    return _debug_shapes[key]


def canonical_events(info: dict) -> list:
    """A debug function's events with its callees named as the PC side names them (trivial_shape, canonical)."""
    known = info.get("canonical")
    if known is None:
        known = info["canonical"] = []
        for e in info["ev"]:
            if e[0] == "call":
                shape = debug_shape(e[2]) if e[2] != "?" and e[2][:2] not in ("v:", "i:") else None
                e = ["call", e[1], "t:" + shape, e[3], canonical(e[2])] if shape else \
                    ["call", e[1], canonical(e[2]), e[3]]
            known.append(e)
    return known


def renumber(x, order: dict):
    if not isinstance(x, list) or not x:
        return x
    if x[0] == "call":
        return ["call", order[x[1]], x[2], [[label, renumber(a, order)] for label, a in x[3]]] + x[4:]
    if x[0] == "r":
        return ["r", order.get(x[1])]
    return [renumber(c, order) for c in x]


def pc_events(cfunc, targets: dict | None = None) -> list:
    """The function's events; targets gets call index -> the address a direct call calls."""
    database()
    visitor = Events(cfunc)
    visitor.apply_to(cfunc.body, None)
    events = visitor.finish()
    if targets is not None:
        targets.update(visitor.targets)
    return events


def result_definitions(events: list, excluded: set) -> dict:
    """variable -> call index, for the variables whose one store is a call's result: Hex-Rays' own temporaries,
    which the debug build keeps in a register."""
    stores = collections.Counter(e[1] for e in events if e[0] == "asg")
    return {e[1]: e[3][1] for e in events
            if e[0] == "asg" and e[2] == 0 and e[3][0] == "r" and e[3][1] is not None and stores[e[1]] == 1 and
            e[1] not in excluded}


# ---------------------------------------------------------------------------------------------------------------
# features: what an event says about one variable
# ---------------------------------------------------------------------------------------------------------------

def variables_in(x, into: set) -> None:
    if not isinstance(x, list) or not x:
        return
    if x[0] in ("v", "a"):
        into.add(x[1])
    elif x[0] in ("ld", "add"):
        variables_in(x[1], into)
    elif x[0] == "op":
        for c in x[1]:
            variables_in(c, into)


class Describer:
    """Event operands as text, from one variable's point of view: itself is `@`, a variable with a known debug name
    is that name, any other is `*` (or, with definitions, the call whose result it holds)."""

    def __init__(self, names: dict, callees: dict, definitions: dict):
        self.names, self.callees, self.definitions = names, callees, definitions

    def text(self, x, me) -> str:
        if x is None:
            return "?"
        kind = x[0]
        if kind in ("v", "a"):
            prefix = "" if kind == "v" else "&"
            if x[1] == me:
                return prefix + "@"
            name = self.names.get(x[1])
            if name is None and kind == "v" and x[1] in self.definitions:
                return "r:" + self.callees.get(self.definitions[x[1]], "?")
            return prefix + ("v:" if kind == "v" else "") + (name or "*")
        if kind == "c":
            return f"c:{x[1]}"
        if kind in ("fc", "s"):
            return f"{kind}:{x[1]}"
        if kind == "g":
            return "g:" + srcindex.name_key(x[1])
        if kind == "ga":
            return "&g:" + srcindex.name_key(x[1])
        if kind == "ld":
            return f"ld({self.text(x[1], me)},{x[2]})"
        if kind == "add":
            return f"add({self.text(x[1], me)},{x[2]})"
        if kind == "r":
            return "r:" + self.callees.get(x[1], "?")
        if kind == "op":
            # one level, each operand once: the two compilers nest and repeat a computation's operands differently
            return "op(" + ",".join(sorted({self.text(c, me) for c in leaves(x)})) + ")"
        return "?"


def weight(feature: str) -> float:
    """How much a feature nothing else shares counts for."""
    head, _, rest = feature.partition("|")
    if head == "A":
        base = 1.0
    elif head == "=":
        if rest.startswith(("r:", "s:")):
            base = 1.0
        elif rest.startswith(("g:", "&g:")):
            base = 0.9
        elif rest.startswith("fc:"):
            base = 0.25 if rest in ("fc:0", "fc:1") else 0.5
        elif rest.startswith("c:"):
            base = 0.12 if rest in ("c:0", "c:1", "c:-1") else 0.3
        elif rest.startswith("ld("):
            base = 0.7
        else:
            base = 0.5
    elif head == "RET":
        base = 0.8
    elif head in ("M", "MV"):
        base = 0.6
    elif head == "U":
        base = 0.4
    elif head == "CMP":
        if "c:" in rest and "fc:" not in rest:
            base = 0.15
        else:
            base = 0.5 if "v:" in rest and "*" not in rest else 0.3
    else:
        base = 0.3
    # each unnamed other variable, and each unknown callee, says less
    base *= 0.6 ** min(feature.count("*"), 3)
    if "r:?" in feature or "|?|" in feature:
        base *= 0.5
    if feature.startswith(("A|t:", "=|r:t:")):
        # a trivial callee's shape is shared by unrelated functions: several such calls count, one does not
        base *= 0.25
    return base


def features(events: list, names: dict, ordinals: set, definitions: dict) -> dict:
    """variable -> Counter of features. names: the variables whose debug name is known (they name the context);
    ordinals: the callees both sides call equally often, whose calls are told apart by their order; definitions:
    result_definitions (PC side)."""
    callees, identities, ordinal, seen = {}, {}, {}, collections.Counter()
    for e in events:
        if e[0] == "call":
            callees[e[1]] = e[2]
            identities[e[1]] = e[2:3] + e[4:5]
    for index in sorted(identities):
        for name in identities[index]:
            seen[name] += 1
            ordinal[(index, name)] = seen[name]
    by_index = {e[1]: e for e in events if e[0] == "call"}
    plain = Describer({}, callees, {})
    named = Describer(names, callees, {})
    defined = Describer(names, callees, definitions) if definitions else None
    out = collections.defaultdict(collections.Counter)
    # the calls whose result something keeps: their hidden result buffer is a temporary, not the variable
    kept = set()
    for e in events:
        if e[0] == "asg":
            x = e[3]
            x = x[1] if x[0] == "ld" and x[1] and x[1][0] in ("r", "v") and x[2] == 0 else x
            if x[0] == "r":
                kept.add(x[1])
            elif x[0] == "v" and x[1] in definitions:
                kept.add(definitions[x[1]])

    def numbered(index, name):
        return f"{name}#{ordinal[(index, name)]}" if name in ordinals and name != "?" else None

    def result(me, index):
        """me holds the result of call index."""
        into = out[me]
        call = by_index.get(index)
        signatures = set()
        if call and call[3]:
            signatures = {";".join(f"{label}={d.text(x, me)}" for label, x in call[3] if label != "ret")
                          for d in (named, defined) if d} - {""}
        for name in identities.get(index, ["?"]):
            into[f"=|r:{name}"] += 1
            if numbered(index, name):
                into[f"=|r:{numbered(index, name)}"] += 1
            for signature in signatures:
                into[f"=|r:{name}({signature})"] += 1

    for e in events:
        kind = e[0]
        if kind == "call":
            index, arguments = e[1], e[3]
            for label, x in arguments:
                mentioned = set()
                variables_in(x, mentioned)
                for me in mentioned:
                    if label == "ret" and x[0] == "a":
                        # the hidden result's object is the call's result
                        if index not in kept:
                            result(me, index)
                        continue
                    local = plain.text(x, me)
                    whole = set()
                    if len(arguments) > 1:
                        whole = {";".join(f"{l}={d.text(a, me)}" for l, a in arguments if l != "ret")
                                 for d in (named, defined) if d}
                    for name in identities[index]:
                        out[me][f"A|{name}|{label}|{local}"] += 1
                        if numbered(index, name):
                            out[me][f"A|{numbered(index, name)}|{label}|{local}"] += 1
                        for text in whole:
                            out[me][f"A|{name}|{text}"] += 1
        elif kind == "asg":
            _, target, _, x = e
            if x[0] == "ld" and x[1] and x[1][0] in ("r", "v") and x[2] == 0:
                # a record copied out of a returned pointer
                inner = x[1]
                if inner[0] == "r" or inner[1] in definitions:
                    x = inner
            if x[0] == "v" and x[1] in definitions and x[1] != target:
                result(target, definitions[x[1]])
            if x[0] == "r":
                result(target, x[1])
            else:
                first = "=|" + plain.text(x, target)
                out[target][first] += 1
                second = "=|" + named.text(x, target)
                if second != first:
                    out[target][second] += 1
            mentioned = set()
            variables_in(x, mentioned)
            for me in mentioned - {target}:
                if target in names:
                    out[me][f"U|={names[target]}|{named.text(x, me)}"] += 1
        elif kind == "mst":
            _, base, disp, x = e
            in_base, in_value = set(), set()
            variables_in(base, in_base)
            variables_in(x, in_value)
            for me in in_base:
                first = f"M|{plain.text(base, me)}|{disp}|{plain.text(x, me)}"
                out[me][first] += 1
                second = f"M|{named.text(base, me)}|{disp}|{named.text(x, me)}"
                if second != first:
                    out[me][second] += 1
            for me in in_value - in_base:
                out[me][f"MV|{named.text(base, me)}|{disp}|{named.text(x, me)}"] += 1
        elif kind == "ret":
            mentioned = set()
            variables_in(e[1], mentioned)
            for me in mentioned:
                out[me]["RET|" + named.text(e[1], me)] += 1
        elif kind == "cmp":
            mentioned = set()
            variables_in(e[1], mentioned)
            variables_in(e[2], mentioned)
            for me in mentioned:
                out[me]["CMP|" + "|".join(sorted((named.text(e[1], me), named.text(e[2], me))))] += 1
    return out


def call_counts(events: list) -> collections.Counter:
    return collections.Counter(name for e in events if e[0] == "call" for name in e[2:3] + e[4:5])


def leaves(x):
    """The operands of nested computations."""
    for c in x[1]:
        if c[0] == "op":
            yield from leaves(c)
        else:
            yield c


def strong_callees(found) -> set:
    """The callees of the features that name a call."""
    out = set()
    for f in found:
        if f.startswith("A|"):
            out.add(f.split("|")[1].split("#")[0])
        elif f.startswith("=|r:"):
            out.add(re.split(r"[#(]", f[4:])[0])
    return out


# ---------------------------------------------------------------------------------------------------------------
# matching
# ---------------------------------------------------------------------------------------------------------------

def compatible(variable: dict, lvar, exact: bool = False) -> float:
    """How well the PC local can be the debug variable: 0 not at all, 1 fully. A record or array needs a PC local
    of its size (the PC type's, where the database has the type): the events do not say at which offset of a
    record a smaller PC local lies. exact: a float needs its size too."""
    t = variable["t"]
    size, kind, width = t["sz"], t["k"], lvar.width
    floating = lvar.type().is_floating()
    if kind in ("rec", "arr"):
        key = (t["b"], t["p"], tuple(t.get("dims", ())))
        known = _sizes.get(key)
        if known is None:
            tinfo = make_type(t)
            known = _sizes[key] = tinfo.get_size() if tinfo is not None else size
        return 1.0 if width == known and not floating else 0.0
    if kind == "flt":
        if lvar.is_stk_var():
            return 1.0 if width == size else (0.5 if width in (4, 8) and not exact else 0.0)
        return 1.0 if floating else 0.0
    if floating and not lvar.is_stk_var():
        return 0.0
    if size >= 4:
        if width != size:
            return 0.0
    elif width not in (size, 4) and not (size == 1 and width == 2):
        return 0.0
    return 0.5 if floating else 1.0


class Match:
    """The pairing of one PC function with one debug function."""

    def __init__(self, cfunc, events, info):
        self.cfunc, self.info = cfunc, info
        self.lvars = list(cfunc.get_lvars())
        self.arguments = list(cfunc.argidx)
        self.pc_events, self.debug_events = events, canonical_events(info)
        self.variables = info["vars"]
        self.definitions = result_definitions(events, set(self.arguments))
        self.pairs = {}  # PC lvar index -> [(debug variable index, score)]
        self.kinds = {}  # PC lvar index -> feat | layout | copy
        self.parameters = {}  # PC lvar index -> debug variable index
        self.evidence = {}
        self._parameters()
        pc_counts, debug_counts = call_counts(self.pc_events), call_counts(self.debug_events)
        self.ordinals = {name for name, count in pc_counts.items() if debug_counts.get(name) == count}
        self.common = set(pc_counts) & set(debug_counts)
        # (target, source) of the debug build's plain copies: `derived = (Derived*)base`
        self.debug_copies = {(e[1], e[3][1]) for e in self.debug_events if e[0] == "asg" and e[3][0] == "v"}
        self.gates = {}
        # stack locals in the outgoing argument area: arguments on their way to a call, no locals
        area = argument_area(cfunc.entry_ea)
        self.transit = {i for i, v in enumerate(self.lvars) if v.is_stk_var() and v.location.stkoff() < area}
        fixed = {}
        for _ in range(3):
            pairs = self._features(fixed)
            if not self._layout(pairs, fixed) + self._copies(pairs, fixed):
                break
        self.pairs = pairs
        self.kinds = {p: "feat" for p in pairs}
        for p, (d, kind) in fixed.items():
            if p not in pairs:
                self.pairs[p] = [(d, LAYOUT if kind == "layout" else COPY)]
                self.kinds[p] = kind
        self.total = sum(score for ds in self.pairs.values() for _, score in ds) + 0.01 * len(self.parameters)

    def identity(self, d: int):
        v = self.variables[d]
        return v["n"], json.dumps(v["t"], sort_keys=True)

    def gate(self, d: int, p: int) -> float:
        known = self.gates.get((d, p))
        if known is None:
            known = self.gates[(d, p)] = compatible(self.variables[d], self.lvars[p])
        return known

    def _parameters(self) -> None:
        by_label = {v["p"]: i for i, v in enumerate(self.variables) if "p" in v}
        explicit = sorted((int(label), i) for label, i in by_label.items() if label.isdigit())
        expected = ([by_label["this"]] if "this" in by_label else []) + \
            ([by_label["ret"]] if "ret" in by_label else []) + [i for _, i in explicit]
        self.parameters_agree = len(expected) == len(self.arguments)
        if self.parameters_agree:
            self.parameters = dict(zip(self.arguments, expected))
        elif "this" in by_label and self.arguments and self.lvars[self.arguments[0]].name == "this":
            self.parameters = {self.arguments[0]: by_label["this"]}

    # -- shared events --------------------------------------------------------------------------------------
    def _features(self, fixed: dict) -> dict:
        arguments = set(self.arguments) | self.transit
        debug_locals = [i for i, v in enumerate(self.variables) if "p" not in v]
        pairs = {}
        for _ in range(ROUNDS):
            debug_names = {d: self.variables[d]["n"] for d in self.parameters.values()}
            pc_names = {p: self.variables[d]["n"] for p, d in self.parameters.items()}
            for p, (d, _) in fixed.items():
                pc_names[p] = debug_names[d] = self.variables[d]["n"]
            for p, ds in pairs.items():
                # a reused PC local names its context only where it holds one debug local
                if len(ds) == 1:
                    pc_names[p] = self.variables[ds[0][0]]["n"]
                for d, _ in ds:
                    debug_names[d] = self.variables[d]["n"]
            debug_features = features(self.debug_events, debug_names, self.ordinals, {})
            pc_features = features(self.pc_events, pc_names, self.ordinals, self.definitions)
            holders_debug, holders_pc = collections.Counter(), collections.Counter()
            for found in debug_features.values():
                holders_debug.update(found.keys())
            for found in pc_features.values():
                holders_pc.update(found.keys())
            by_feature = collections.defaultdict(list)
            for d in debug_locals:
                for f in debug_features.get(d, ()):
                    by_feature[f].append(d)
            scores = collections.defaultdict(lambda: collections.defaultdict(float))
            exclusive = collections.defaultdict(float)
            evidence = collections.defaultdict(list)
            for p, found in pc_features.items():
                if p in arguments or not self.lvars[p].name:
                    continue
                for f in found:
                    for d in by_feature.get(f, ()):
                        gate = self.gate(d, p)
                        if not gate:
                            continue
                        share = weight(f) * gate / (holders_debug[f] * holders_pc[f])
                        scores[p][d] += share
                        if holders_debug[f] == 1:
                            exclusive[(p, d)] += share
                        evidence[(p, d)].append((share, f))
            found = {}
            for p, by_debug in scores.items():
                ranked = sorted(by_debug.items(), key=lambda pair: (-pair[1], pair[0]))
                top, top_score = ranked[0]
                if top_score < THRESHOLD:
                    continue
                rivals = [score for d, score in ranked[1:] if self.identity(d) != self.identity(top)]
                if rivals and top_score < 1.2 * rivals[0] and exclusive[(p, top)] < 0.3:
                    # two debug locals fit alike
                    continue
                chosen = [(top, top_score)]
                for d, score in ranked[1:]:
                    # another debug local with evidence of its own: the slot is reused
                    if self.identity(d) != self.identity(top) and exclusive[(p, d)] >= THRESHOLD and \
                            self.variables[d]["n"] != self.variables[top]["n"]:
                        chosen.append((d, score))
                # a debug local that is only copied into another one the PC local holds is that one (a cast); two
                # that are copied into each other both stay
                held = {d for d, _ in chosen}
                found[p] = [(d, score) for d, score in chosen
                            if not any((other, d) in self.debug_copies for other in held if other != d)][:3] or \
                    chosen[:1]
            self.evidence, self.debug_features, self.pc_features = evidence, debug_features, pc_features
            same = {p: [d for d, _ in ds] for p, ds in found.items()} == \
                {p: [d for d, _ in ds] for p, ds in pairs.items()}
            pairs = found
            if same:
                break
        return pairs

    # -- the frame ------------------------------------------------------------------------------------------
    def _layout(self, pairs: dict, fixed: dict) -> int:
        arguments = set(self.arguments) | self.transit
        stack = sorted((v.location.stkoff(), i) for i, v in enumerate(self.lvars)
                       if v.is_stk_var() and i not in arguments and v.name)
        offsets = collections.Counter(offset for offset, _ in stack)
        pc = [i for _, i in stack]
        debug = sorted((i for i, v in enumerate(self.variables) if "p" not in v),
                       key=lambda i: (self.variables[i]["o"], i))
        position = {d: j for j, d in enumerate(debug)}
        barrier = object()

        def states():
            state, taken = [], set()
            for offset, p in stack:
                ds = [d for d, _ in pairs.get(p, ())] or ([fixed[p][0]] if p in fixed else [])
                taken.update(ds)
                if offsets[offset] > 1 or len(ds) > 1:
                    state.append(barrier)
                else:
                    state.append(position[ds[0]] if ds else None)
            return state, taken

        def fits(p, d) -> bool:
            if compatible(self.variables[d], self.lvars[p], exact=True) < 1.0:
                return False
            # Hex-Rays saw a record or a record pointer where the debug local is a number: a compiler temporary
            mine = self.lvars[p].type()
            pointed = mine.get_pointed_object() if mine.is_ptr() else mine
            if pointed.is_udt() and self.variables[d]["t"]["k"] in ("int", "flt"):
                return False
            # both take part in calls to functions both builds call, and share none: not the same variable
            mine = strong_callees(self.pc_features.get(p, ())) & self.common
            theirs = strong_callees(self.debug_features.get(d, ())) & self.common
            return not (mine and theirs and not mine & theirs)

        def packed(low, high) -> bool:
            """Whether the PC locals low..high lie back to back: no compiler temporary between them."""
            for k in range(low, high):
                gap = stack[k + 1][0] - stack[k][0] - self.lvars[stack[k][1]].width
                if not 0 <= gap <= 3:
                    return False
            return True

        added = 0
        while True:
            state, taken = states()
            new = {}
            anchors = [i for i, s in enumerate(state) if s is not None]
            # between two paired neighbours
            for a, b in zip(anchors, anchors[1:]):
                count = b - a - 1
                if count < 1 or state[a] is barrier or state[b] is barrier:
                    continue
                step = 1 if state[b] > state[a] else -1
                if abs(state[b] - state[a]) != count + 1:
                    continue
                run = [(pc[a + n], debug[state[a] + step * n]) for n in range(1, count + 1)]
                if packed(a, b) and all(d not in taken and fits(p, d) for p, d in run):
                    new.update(run)
            # past the end of a run of paired neighbours
            for a, b in zip(anchors, anchors[1:]):
                if b != a + 1 or state[a] is barrier or state[b] is barrier or abs(state[b] - state[a]) != 1:
                    continue
                step = state[b] - state[a]
                for start, at, direction in ((a, state[a], -1), (b, state[b], 1)):
                    for n in range(1, 4):
                        i, j = start + direction * n, at + direction * step * n
                        if not (0 <= i < len(pc) and 0 <= j < len(debug)) or state[i] is not None:
                            break
                        if debug[j] in taken or pc[i] in new or not fits(pc[i], debug[j]) or \
                                not packed(min(i, start), max(i, start)):
                            break
                        new[pc[i]] = debug[j]
            new = {p: d for p, d in new.items() if p not in fixed and p not in pairs}
            if not new:
                return added
            for p, d in new.items():
                fixed[p] = (d, "layout")
            added += len(new)

    # -- copies ---------------------------------------------------------------------------------------------
    def _copies(self, pairs: dict, fixed: dict) -> int:
        arguments = set(self.arguments) | self.transit
        copied = {e[1] for e in self.debug_events if e[0] == "asg" and e[3][0] == "v" and e[3][1] != e[1]}
        added = 0
        for e in self.pc_events:
            if e[0] != "asg" or e[2] != 0 or e[3][0] != "v" or e[3][2] != 0:
                continue
            target, source = e[1], e[3][1]
            if source == target or source in arguments or source in pairs or source in fixed or \
                    not self.lvars[source].name:
                continue
            ds = pairs.get(target, ())
            if len(ds) != 1 or ds[0][0] in copied or self.gate(ds[0][0], source) < 0.5:
                continue
            fixed[source] = (ds[0][0], "copy")
            added += 1
        return added

    # -- the settings ---------------------------------------------------------------------------------------
    def settings(self) -> list:
        """[(lvar index, name, tinfo or None, score, debug names, kind)] for every PC local that gets a name or
        type."""
        wanted = {}
        for p, d in self.parameters.items():
            if AUTOMATIC.match(self.lvars[p].name or "") and not self.variables[d].get("hid"):
                wanted[p] = ([d], PARAMETER, "param")
        for p, ds in self.pairs.items():
            wanted[p] = ([d for d, _ in ds], ds[0][1], self.kinds[p])
        # names: one debug local over several PC locals is numbered in the order of their definitions
        taken = {v.name for i, v in enumerate(self.lvars) if i not in wanted and v.name}
        users = collections.defaultdict(list)
        for p, (ds, _, _) in wanted.items():
            users["__".join(self.variables[d]["n"] for d in ds)].append(p)
        out = []
        for base in sorted(users, key=lambda b: min(users[b])):
            ranked = sorted(users[base], key=lambda p: (wanted[p][2] != "param", wanted[p][2] != "copy",
                                                        self.lvars[p].defea, p))
            n = 0
            for p in ranked:
                n += 1
                name = base if n == 1 else f"{base}_{n}"
                while name in taken:
                    n += 1
                    name = f"{base}_{n}"
                taken.add(name)
                ds, score, kind = wanted[p]
                tinfo = self._type(p, ds, kind) if kind != "param" else None
                out.append((p, name, tinfo, score, [self.variables[d]["n"] for d in ds], kind))
        return sorted(out, key=lambda row: row[0])

    def _type(self, p: int, ds: list, kind: str):
        """The debug type for the PC local, where every debug local it holds has that type and it is the PC local's
        size. A PC local of another size keeps its type: a larger record would swallow its neighbours, and the
        events do not say at which offset of the record the PC local lies. A `va_list` stays one (the debug build
        has its `char *`). A pair that only the frame's layout or a copy gives is too weak to change what kind of
        value the local is: it keeps a pointer a pointer and a float a float, and a pointer to a record or a
        record that record."""
        if len({self.identity(d)[1] for d in ds}) != 1:
            return None
        tinfo = make_type(self.variables[ds[0]]["t"])
        old = self.lvars[p].type()
        if tinfo is None or tinfo.get_size() != self.lvars[p].width or str(old) == "va_list":
            return None
        if kind != "feat":
            if tinfo.is_ptr() != old.is_ptr() or tinfo.is_floating() != old.is_floating():
                return None
            inner = old.get_pointed_object() if old.is_ptr() else old
            now = tinfo.get_pointed_object() if tinfo.is_ptr() else tinfo
            if inner.is_udt() and str(inner) != str(now):
                return None
        return tinfo

    # -- what the pairing says about other things ---------------------------------------------------------------
    def votes(self) -> list:
        """[(kind, target, value, strength)]: what this function's paired locals say about its callees (kind f:
        target the callee's address, value the debug callee's name), its globals (g: the PC global's address, the
        debug global's name) and its parameters (p: the parameter's label, `PC name : PC type => debug name :
        debug type`, for a parameter Hex-Rays named itself or whose types differ; strength 1 where the types are
        equal). A PC call and a debug call are the same call when they pass as many arguments, no argument
        contradicts and the arguments that are the same named local, string, global or number weigh 1 or more (a
        named local, a string or a float 1, a number 0.25); a PC call votes for a callee name when every debug
        call it can be has that name and no PC call to another function can be the same debug call. A global is
        voted for where the two builds pass it at the same place of one call, or assign it to the same local."""
        pc_names = {p: self.variables[d]["n"] for p, d in self.parameters.items()}
        pc_names.update({p: self.variables[ds[0][0]]["n"] for p, ds in self.pairs.items() if len(ds) == 1})
        raw = {e[1]: e[2] for e in self.info["ev"] if e[0] == "call"}
        pc_calls = [e for e in self.pc_events if e[0] == "call"]
        debug_calls = [e for e in self.debug_events if e[0] == "call"]
        pc_callee = {e[1]: e[2] for e in pc_calls}
        debug_callee = {e[1]: e[2] for e in debug_calls}
        hard = ("c", "fc", "s")

        def unify(x, y, found: list):
            """The weight of what PC value x and debug value y share, None where they cannot be one value."""
            if x is None or y is None:
                return 0.0
            kx, ky = x[0], y[0]
            if kx in ("v", "a"):
                name = pc_names.get(x[1])
                if name is None:
                    return 0.0
                return 1.0 if kx == ky and name == self.variables[y[1]]["n"] and x[2] == y[2] else None
            if kx != ky:
                return None if kx in hard and ky in hard else 0.0
            if kx == "c":
                return None if x[1] != y[1] else 0.1 if abs(x[1]) <= 1 else 0.25
            if kx in ("fc", "s"):
                return 1.0 if x[1] == y[1] else None
            if kx in ("g", "ga"):
                if srcindex.name_key(x[1]) == srcindex.name_key(y[1]):
                    return 1.0
                found.append((x[1], y[1]))
                return 0.0
            if kx in ("ld", "add"):
                return unify(x[1], y[1], found) if x[2] == y[2] else None
            if kx == "r":
                name = pc_callee.get(x[1])
                return 0.5 if name and name != "?" and name == debug_callee.get(y[1]) else 0.0
            return 0.0

        def results(events, name_of) -> dict:
            out = collections.defaultdict(set)
            for e in events:
                if e[0] == "asg" and e[2] == 0 and e[3][0] == "r" and e[3][1] is not None:
                    out[e[3][1]].add(name_of(e[1]))
            return out

        pc_results = results(self.pc_events, pc_names.get)
        debug_results = results(self.debug_events, lambda d: self.variables[d]["n"])
        rank = {"this": -2, "ret": -1}

        def ordered(arguments):
            return [x for _, x in sorted(arguments, key=lambda a: rank.get(a[0]) if a[0] in rank else int(a[0]))]

        def same_call(c, d):
            if len(c[3]) != len(d[3]):
                return None
            found, total = [], 0.0
            for x, y in zip(ordered(c[3]), ordered(d[3])):
                weight = unify(x, y, found)
                if weight is None:
                    return None
                total += weight
            named = pc_results.get(c[1], set()) - {None}
            if named and debug_results.get(d[1]):
                if not named & debug_results[d[1]]:
                    return None
                total += 1.0
            return total, found

        out, can_be, claimed = [], {}, collections.defaultdict(set)
        for c in pc_calls:
            fits = []
            for d in debug_calls:
                same = same_call(c, d)
                if same and same[0] >= 1.0:
                    fits.append((d, same[0], same[1]))
            can_be[c[1]] = fits
            for d, _, _ in fits:
                claimed[d[1]].add(self.targets.get(c[1], -1 - c[1]))
        for c in pc_calls:
            fits = can_be[c[1]]
            target = self.targets.get(c[1])
            if not fits:
                continue
            if len(fits) == 1:
                for pc_global, debug_global in fits[0][2]:
                    if pc_global in _global_addresses and debug_global != "?":
                        out.append(("g", f"{_global_addresses[pc_global]:#x}", debug_global, fits[0][1]))
            names = {raw.get(d[1], "?") for d, _, _ in fits}
            if target is None or c[2][:2] == "t:" or any(d[2][:2] in ("t:", "v:", "i:") or d[2] == "?" for d, _, _ in fits):
                continue
            if len({canonical(name) for name in names}) == 1 and all(claimed[d[1]] == {target} for d, _, _ in fits):
                out.append(("f", f"{target:#x}", sorted(names)[0], max(weight for _, weight, _ in fits)))
        # a global assigned to the same local, once on each side
        loads = [collections.defaultdict(list), collections.defaultdict(list)]
        for side, (events, name_of) in enumerate(((self.pc_events, pc_names.get),
                                                  (self.debug_events, lambda d: self.variables[d]["n"]))):
            for e in events:
                if e[0] == "asg" and e[2] == 0 and e[3][0] in ("g", "ga") and name_of(e[1]):
                    loads[side][(name_of(e[1]), e[3][0])].append(e[3][1])
        for where, pc_globals in loads[0].items():
            debug_globals = loads[1].get(where, ())
            if len(pc_globals) == 1 and len(debug_globals) == 1 and pc_globals[0] in _global_addresses and \
                    srcindex.name_key(pc_globals[0]) != srcindex.name_key(debug_globals[0]):
                out.append(("g", f"{_global_addresses[pc_globals[0]]:#x}", debug_globals[0], 1.0))
        for p, d in self.parameters.items():
            lvar, variable = self.lvars[p], self.variables[d]
            tinfo = make_type(variable["t"])
            debug_type = str(tinfo) if tinfo is not None else variable["t"]["b"] + "*" * variable["t"]["p"]
            equal = tinfo is not None and (tinfo == lvar.type() or str(tinfo) == str(lvar.type()).replace("struct ", ""))
            if not equal or AUTOMATIC.match(lvar.name or ""):
                out.append(("p", variable.get("p", "?"), f"{lvar.name} : {lvar.type()} => {variable['n']} : {debug_type}",
                            1.0 if equal else 0.0))
        return out


def tokens(events: list) -> list:
    """What a function's body holds that inlining leaves in place: its strings, floats, larger numbers, named
    globals and named callees. Two builds' copies of one function share most of them."""
    out = set()

    def walk(x) -> None:
        if not isinstance(x, list) or not x:
            return
        if x[0] == "s":
            out.add(f"s:{x[1]}")
        elif x[0] == "fc":
            out.add(f"fc:{x[1]}")
        elif x[0] == "c":
            if isinstance(x[1], int) and abs(x[1]) >= 0x100:
                out.add(f"c:{x[1]}")
        elif x[0] in ("g", "ga"):
            if not AUTO_GLOBAL.match(x[1]) and x[1] != "?":
                out.add("g:" + srcindex.name_key(x[1]))
        else:
            for c in x[1:]:
                if isinstance(c, list):
                    walk(c)

    for e in events:
        if e[0] == "call":
            for name in e[2:3] + e[4:5]:
                if name != "?" and name[:2] not in ("v:", "i:", "t:"):
                    out.add("f:" + name)
            for _, x in e[3]:
                walk(x)
        else:
            for x in e[1:]:
                walk(x)
    return sorted(out)


def make_type(described: dict):
    """The IDA type of a debug type, or None when the database has no type of the name."""
    base = described["b"]
    tinfo = ida_typeinf.tinfo_t()
    if described["bk"] == "fn":
        return None
    if described["bk"] == "void" or base == "void":
        if not described["p"]:
            return None
        tinfo = ida_typeinf.tinfo_t(ida_typeinf.BT_VOID)
    elif base in BUILTIN:
        tinfo = ida_typeinf.tinfo_t(BUILTIN[base])
    elif not tinfo.get_named_type(None, base):
        other = named_types().get(srcindex.name_key(base))
        if not other or not tinfo.get_named_type(None, other):
            return None
    if described.get("c"):
        tinfo.set_const()
    for _ in range(described["p"]):
        pointer = ida_typeinf.tinfo_t()
        pointer.create_ptr(tinfo)
        tinfo = pointer
    for count in reversed(described.get("dims", [])):
        if count <= 0:
            return None
        array = ida_typeinf.tinfo_t()
        array.create_array(tinfo, count)
        tinfo = array
    return tinfo


def named_types() -> dict:
    """name key -> the database's spelling of every named type."""
    global _named_types
    if _named_types is None:
        _named_types = {}
        til = ida_typeinf.get_idati()
        for ordinal in range(1, ida_typeinf.get_ordinal_limit(til)):
            name = ida_typeinf.get_numbered_type_name(til, ordinal)
            if name:
                _named_types.setdefault(srcindex.name_key(name), name)
    return _named_types


# ---------------------------------------------------------------------------------------------------------------
# one function
# ---------------------------------------------------------------------------------------------------------------

def unique_instance(key: str) -> str | None:
    """The one debug function key that is `key` once template arguments are left out of both: the PC name of a
    template's member often lacks them."""
    global _instances
    if _instances is None:
        _instances = collections.defaultdict(set)
        for (known,) in database().execute("select distinct key from func"):
            _instances[canonical(known)].add(known)
    found = _instances.get(canonical(key), ())
    return next(iter(found)) if len(found) == 1 else None


def match(ea: int):
    """The best Match of the function at ea, or a reason string."""
    key = function_key(ea)
    found = candidates(key)
    if not found and re.search(r"_\d+$", key):
        found = candidates(re.sub(r"_\d+$", "", key))
    if not found and key != "?" and unique_instance(key):
        found = candidates(unique_instance(key))
    if not found:
        return "no debug function"
    try:
        cfunc = hx.decompile(ea)
    except hx.DecompilationFailure as error:
        return f"decompile: {error}"
    if not cfunc:
        return "decompile failed"
    targets = {}
    events = pc_events(cfunc, targets)
    best, alike = None, False
    for _, _, info in found:
        if not same_callees(events, canonical_events(info)):
            continue
        alike = True
        m = Match(cfunc, events, info)
        m.targets = targets
        if len(found) > 1 and not m.parameters_agree:
            continue
        if best is None or m.total > best.total:
            best = m
    return best or ("no debug function with these parameters" if alike else "callees differ")


def same_callees(pc: list, debug: list) -> bool:
    """Whether the two functions can be the same one: where both call three or more named functions, a third of the
    shorter list is shared. A PC function that carries another function's name fails this."""
    named = [{e[2] for e in events if e[0] == "call" and e[2] != "?" and e[2][:2] not in ("v:", "i:")}
             for events in (pc, debug)]
    least = min(len(named[0]), len(named[1]))
    return least < 3 or len(named[0] & named[1]) * 3 >= least


def argument_area(ea: int) -> int:
    """The bytes at the bottom of the function's frame that only hold outgoing arguments: how far its deepest push
    goes below the sp its body runs at. Hex-Rays counts stack offsets from that deepest sp, so a stack local below
    this offset is an argument on its way to a call, not a local."""
    depths = collections.Counter(idc.get_spd(head) for head in idautils.FuncItems(ea))
    return depths.most_common(1)[0][0] - min(depths) if depths else 0


class Assignments(hx.ctree_visitor_t):
    """lvar index -> the addresses of the plain assignments to it; the lvars whose address goes anywhere but
    straight into a call's argument (a pointer to them may outlive the call); the number of calls; lvar index ->
    the address of each place the body names it: the expression's own, else the nearest enclosing expression's
    (an assignment's target has the assignment's)."""

    def __init__(self):
        super().__init__(hx.CV_PARENTS)
        self.sites = collections.defaultdict(set)
        self.uses = collections.defaultdict(list)
        self.escaped = set()
        self.calls = 0

    def visit_expr(self, e):
        if e.op == hx.cot_var:
            at = e.ea
            for k in range(len(self.parents) - 1, -1, -1):
                if at != idaapi.BADADDR or self.parents[k] is None:
                    break
                at = self.parents[k].ea
            self.uses[e.v.idx].append(at)
        elif e.op == hx.cot_call:
            self.calls += 1
        elif e.op == hx.cot_asg and e.x.op == hx.cot_var:
            self.sites[e.x.v.idx].add(e.ea)
        elif e.op == hx.cot_ref and e.x.op == hx.cot_var:
            parent = None
            for k in range(len(self.parents) - 1, -1, -1):
                parent = self.parents[k]
                if parent is None or parent.op != hx.cot_cast:
                    break
            if parent is None or parent.op != hx.cot_call:
                self.escaped.add(e.x.v.idx)
        return 0


CHANGED = (idaapi.CF_CHG1, idaapi.CF_CHG2, idaapi.CF_CHG3, idaapi.CF_CHG4)
USED = (idaapi.CF_USE1, idaapi.CF_USE2, idaapi.CF_USE3, idaapi.CF_USE4)


def stack_accesses(function):
    """(ea, frame offset, size, whole write) of every stack operand of the function's instructions. A whole write
    stores the operand without reading it (mov, fstp, pop); lea and everything else is not one. None when an
    operand based on esp, or on ebp in a function with a frame pointer, is not marked as a stack variable: its
    slot is unknown."""
    out = []
    insn = ida_ua.insn_t()
    bases = (4, 5) if function.flags & ida_funcs.FUNC_FRAME else (4,)
    for ea in idautils.FuncItems(function.start_ea):
        if not ida_ua.decode_insn(insn, ea):
            continue
        feature, flags = insn.get_canon_feature(), ida_bytes.get_flags(ea)
        for n in range(len(CHANGED) + 2):
            op = insn.ops[n]
            if op.type == ida_ua.o_void:
                break
            if op.type not in (ida_ua.o_displ, ida_ua.o_phrase):
                continue
            if not ida_bytes.is_stkvar(flags, n):
                # the base register: the SIB byte's when there is one
                if (op.specflag2 & 7 if op.specflag1 else op.phrase) in bases:
                    return None
                continue
            whole = n < len(CHANGED) and bool(feature & CHANGED[n]) and not feature & USED[n] and \
                insn.itype != idaapi.NN_lea
            out.append((ea, ida_frame.calc_stkvar_struc_offset(function, insn, n),
                        ida_ua.get_dtype_size(op.dtype), whole))
    return out


def slot_webs(function, accesses: list, offset: int, width: int):
    """The whole writes of the stack slot [offset, offset + width) in groups: two writes are in one group when a
    read of the slot can get either's value (reaching definitions over the flow chart). Each group comes as
    (writes, open): open when a read of the group can also see the slot as it was at entry, or when none of its
    writes is read at all. A write that covers the slot only in part, a read and an lea of it all count as reads,
    which keep earlier writes alive. A block nothing leads to (a handler) is taken to see every write. Returns
    (groups, {address of each instruction touching the slot: its group's writes}); the group that only sees the
    slot as it was at entry has no writes."""
    events = collections.defaultdict(list)
    for ea, at, size, whole in accesses:
        if at < offset + width and at + max(size, 1) > offset:
            events[ea].append(whole and at == offset and size == width)
    # an instruction that also reads the slot is a read
    writes = {ea for ea, kinds in events.items() if min(kinds)}
    if len(writes) < 2:
        return [], {}
    entry = -1
    blocks = list(ida_gdl.FlowChart(function))
    inside = {b.id: sorted((ea, min(kinds)) for ea, kinds in events.items() if b.start_ea <= ea < b.end_ea)
              for b in blocks}
    if sum(len(found) for found in inside.values()) != len(events):
        return [], {}
    preds = {b.id: [p.id for p in b.preds()] for b in blocks}
    every = writes | {entry}

    def start(b):
        if b.start_ea == function.start_ea:
            return {entry}.union(*(leaving[p] for p in preds[b.id]))
        return set().union(*(leaving[p] for p in preds[b.id])) if preds[b.id] else set(every)

    leaving = {b.id: set() for b in blocks}
    changed = True
    while changed:
        changed = False
        for b in blocks:
            reach = start(b)
            for ea, write in inside[b.id]:
                if write:
                    reach = {ea}
            if reach != leaving[b.id]:
                leaving[b.id] = reach
                changed = True
    # a read joins the writes that reach it
    group = {w: w for w in every}

    def find(w):
        while group[w] != w:
            group[w] = group[group[w]]
            w = group[w]
        return w

    read = set()
    seen_by = {}
    for b in blocks:
        reach = start(b)
        for ea, write in inside[b.id]:
            if write:
                reach = {ea}
                continue
            read |= reach
            first = find(next(iter(reach))) if reach else None
            for other in reach:
                group[find(other)] = first
            if reach:
                seen_by[ea] = next(iter(reach))
    groups = collections.defaultdict(set)
    for w in writes:
        groups[find(w)].add(w)
    where = {w: frozenset(groups[find(w)]) for w in writes}
    where.update({ea: frozenset(groups.get(find(w), ())) for ea, w in seen_by.items()})
    return [(frozenset(group), find(entry) == key or not group & read) for key, group in groups.items()], where


def remove_splits(ea: int, splits: set):
    """Takes the (stack offset, address) split settings out of the function's user lvar settings."""
    vector = hx.lvar_uservec_t()
    hx.restore_user_lvar_settings(vector, ea)
    kept = hx.lvar_uservec_t()
    for saved in vector.lvvec:
        if not (saved.is_split_lvar() and saved.ll.location.is_stkoff() and
                (saved.ll.location.stkoff(), saved.ll.defea) in splits):
            kept.lvvec.push_back(saved)
    hx.save_user_lvar_settings(ea, kept)
    hx.mark_cfunc_dirty(ea)


def split_reused(ea: int) -> list:
    """Splits a stack local where the compiler reused its slot for another variable (Hex-Rays' split-variable
    setting), so that the other variable is its own local with the type its own uses give it. Needs nothing from
    the debug build. Hex-Rays leaves many such slots as one local, and it trusts a split it is given, so one is
    asked only at the first assignment of a group of writes that slot_webs proves separate from the local's other
    writes, for a local whose address only ever goes straight into a call. Checked on the new decompile
    (misplaced), until what is left holds: a slot Hex-Rays did not divide along the groups loses its splits, as
    does a split that gave no local of its own; a decompile with another number of calls loses them all. Returns
    [(stack offset, address, name, type)] of the splits that hold. Locals in the outgoing argument area are left
    alone."""
    function = ida_funcs.get_func(ea)
    try:
        cfunc = hx.decompile(ea)
    except hx.DecompilationFailure:
        return []
    if not cfunc or not function:
        return []
    visitor = Assignments()
    visitor.apply_to(cfunc.body, None)
    lvars, arguments, area = cfunc.get_lvars(), set(cfunc.argidx), argument_area(ea)
    accesses = None
    slots = {}
    asked = []
    for index, sites in sorted(visitor.sites.items()):
        lvar = lvars[index]
        offset = lvar.location.stkoff() if lvar.is_stk_var() else -1
        if index in arguments or index in visitor.escaped or len(sites) < 2 or offset < area:
            continue
        if accesses is None:
            accesses = stack_accesses(function)
            if accesses is None:
                return []
        if offset not in slots:
            slots[offset] = slot_webs(function, accesses, cfunc.mba.stkoff_vd2ida(offset), lvar.width)
        own = min(sites, key=lambda site: abs(site - lvar.defea))
        for web, is_open in slots[offset][0]:
            if is_open or own in web or not web & sites:
                continue
            info = hx.lvar_saved_info_t()
            info.ll.location.set_stkoff(offset)
            info.ll.defea = min(web & sites)
            info.set_split_lvar()
            if hx.modify_user_lvar_info(ea, hx.MLI_SET_FLAGS, info):
                asked.append((offset, info.ll.defea, lvar.name, str(lvar.type())))
    blocks = sorted(b.start_ea for b in ida_gdl.FlowChart(function)) if asked else []
    while asked:
        hx.mark_cfunc_dirty(ea)
        try:
            cfunc = hx.decompile(ea)
        except hx.DecompilationFailure:
            cfunc = None
        after = Assignments()
        if cfunc:
            after.apply_to(cfunc.body, None)
        if not cfunc or after.calls != visitor.calls:
            remove_splits(ea, {split[:2] for split in asked})
            return []
        lvars = cfunc.get_lvars()
        torn = misplaced(lvars, after, slots, blocks, asked)
        have = {(v.location.stkoff(), v.defea): v for v in lvars if v.is_stk_var()}
        gone = {split[:2] for split in asked if split[0] in torn or split[:2] not in have}
        if not gone:
            break
        remove_splits(ea, gone)
        asked = [split for split in asked if split[:2] not in gone]
    out = []
    for split in asked:
        mine = type_kind(have[split[:2]].type())
        others = [type_kind(v.type()) for where, v in have.items() if where[0] == split[0] and where != split[:2]]
        out.append(split + (all(apart(mine, other) for other in others),))
    return out


def misplaced(lvars, found: Assignments, slots: dict, blocks: list, splits: list) -> set:
    """The stack offsets of the split slots that Hex-Rays did not divide along slot_webs' groups. Each place the
    body names a local of the slot stands for one instruction touching the slot: the one at its address, else
    the last one before it in its basic block (a compare's jump, the push of an address an lea took). That
    instruction is in one group: a group's places must all name one local, and a split's group a local that no
    other group names. A place with no such instruction cannot be checked, so its slot counts too. Without this a
    split can leave a read in one local and the write it reads, or the call that fills it through a pointer, in
    another."""
    torn = set()
    held = collections.defaultdict(lambda: collections.defaultdict(set))
    offsets = {split[0] for split in splits}
    touching = {offset: sorted(slots[offset][1]) for offset in offsets}
    for index, uses in found.uses.items():
        offset = lvars[index].location.stkoff() if lvars[index].is_stk_var() else -1
        if offset not in offsets:
            continue
        where = slots[offset][1]
        for at in uses:
            if at not in where:
                before = bisect.bisect_right(touching[offset], at)
                last = touching[offset][before - 1] if before else None
                if last is None or bisect.bisect_right(blocks, last) != bisect.bisect_right(blocks, at):
                    torn.add(offset)
                    continue
                at = last
            held[offset][where[at]].add(index)
    for offset, defea in (split[:2] for split in splits):
        groups = held[offset]
        web = slots[offset][1].get(defea)
        mine = groups.get(web, set())
        if len(mine) != 1 or any(len(names) > 1 or (key != web and names & mine) for key, names in groups.items()):
            torn.add(offset)
    return torn


def type_kind(tinfo):
    """What a local's type says about the variable: a float of its size, a pointer to its named pointee (None
    for an untyped one), or nothing (an integer is what Hex-Rays gives a value it knows nothing about)."""
    if tinfo.is_floating():
        return "float", tinfo.get_size()
    if tinfo.is_ptr():
        pointee = tinfo.get_pointed_object()
        return "pointer", str(pointee) if pointee.is_udt() or pointee.is_enum() or pointee.is_ptr() else None
    return None


def apart(a, b) -> bool:
    """Whether two type_kinds cannot be one variable's: a float and anything else, or pointers to two named
    types."""
    if a == b or a is None and b is None:
        return False
    if (a and a[0] == "float") or (b and b[0] == "float"):
        return True
    return bool(a and b and a[1] is not None and b[1] is not None)


def unwarranted(ea: int, splits: list) -> set:
    """The splits (stack offset, address) with no sign of a second variable in the slot; they only cut one variable
    that is assigned again into two. A split's local that pairs with debug variables shows one when it holds a
    variable (name and type) no other local of the slot holds, or when the slot's locals of its first variable are
    no more than the debug build has variables of it; an unpaired one shows one when its type cannot be the slot's
    other locals' (apart)."""
    m = match(ea)
    slots, exist = collections.defaultdict(dict), collections.Counter()
    if not isinstance(m, str):
        exist.update(m.identity(d) for d in range(len(m.variables)))
        for p, ds in m.pairs.items():
            if m.lvars[p].is_stk_var():
                slots[m.lvars[p].location.stkoff()][m.lvars[p].defea] = [m.identity(d) for d, _ in ds]
    weak = set()
    for offset, site, _, _, typed_apart in splits:
        mine = slots[offset].get(site)
        if mine is None:
            if not typed_apart:
                weak.add((offset, site))
            continue
        others = [held for defea, held in slots[offset].items() if defea != site]
        alone = any(all(identity not in held for held in others) for identity in mine)
        if not alone and 1 + sum(held[0] == mine[0] for held in others) > exist[mine[0]]:
            weak.add((offset, site))
    return weak


def pair(ea: int, apply: bool, rows: dict, seen: dict):
    """Pairs the function's locals with the debug build's, up to three times while a written type changes its
    locals, and decompiles the result. Fills rows (see process) by local; seen keeps each local's name and type
    from before the first write. Returns None, or a reason string: a function that no longer decompiles with the
    settings gets the ones it had before the pairing back."""
    rows.clear()
    original = None
    for attempt in range(4):
        m = match(ea)
        if isinstance(m, str):
            if original is None:
                return m
            hx.save_user_lvar_settings(ea, original)
            hx.mark_cfunc_dirty(ea)
            return f"reverted: {m}"
        if attempt == 3:
            break
        changed = False
        for p, name, tinfo, score, sources, kind in m.settings():
            lvar = m.lvars[p]
            old_type = str(lvar.type())
            flags = 0
            if lvar.name != name:
                flags |= hx.MLI_NAME
            if tinfo is not None and tinfo != lvar.type() and str(tinfo) != old_type.replace("struct ", ""):
                flags |= hx.MLI_TYPE
            where = (lvar.location.stkoff() if lvar.is_stk_var() else -1 - lvar.location.reg1(), lvar.defea)
            before = seen.setdefault(where, (lvar.name, old_type, int(lvar.has_user_name)))
            new_type = str(tinfo) if flags & hx.MLI_TYPE else old_type
            rows[where] = (ea, before[0], name, before[1], new_type, score, sources, kind,
                           "set" if (before[0], before[1]) != (name, new_type) else "kept", before[2])
            if apply and flags:
                if original is None:
                    original = hx.lvar_uservec_t()
                    hx.restore_user_lvar_settings(original, ea)
                saved = hx.lvar_saved_info_t()
                saved.ll = lvar
                saved.name = name
                if flags & hx.MLI_TYPE:
                    saved.type = tinfo
                if hx.modify_user_lvar_info(ea, flags, saved):
                    changed = True
        if not apply or not changed:
            break
        hx.mark_cfunc_dirty(ea)
    return None


def process(ea: int, apply: bool):
    """Splits the function's reused stack locals (split_reused), pairs its locals with the debug build's (pair),
    takes back the splits that show no second variable (unwarranted) and pairs again. Returns the rows (ea, old
    name, new name, old type, new type, score, debug names, kind, state, old name was the user's) of the settings,
    or a reason string."""
    splits = split_reused(ea) if apply else []
    rows, seen = {}, {}
    reason = pair(ea, apply, rows, seen)
    weak = unwarranted(ea, splits) if splits else set()
    if weak:
        remove_splits(ea, weak)
        splits = [split for split in splits if split[:2] not in weak]
        reason = pair(ea, apply, rows, seen)
    if reason is not None and (reason.startswith("reverted") or not splits):
        return reason
    out = [(ea, name, f"{name}@{site:x}", text, text, 0.0, [], "split", "set", 0)
           for offset, site, name, text, _ in splits]
    return out + ([] if reason is not None else list(rows.values()))


def preview(ea: int, evidence: int = 3) -> str:
    """The pairs of one function with their evidence, nothing written."""
    m = match(ea)
    if isinstance(m, str):
        return m
    lines = [f"{ida_funcs.get_func_name(ea)}: {len(m.pairs)} locals paired, parameters "
             f"{'agree' if m.parameters_agree else 'differ'}"]
    for p, name, tinfo, score, sources, kind in m.settings():
        lvar = m.lvars[p]
        lines.append(f"  {kind:6} {lvar.name or '<unnamed>'} ({lvar.type()}) -> {name}"
                     f"{' : ' + str(tinfo) if tinfo is not None else ''}  [{score:.2f}]")
        for d, _ in m.pairs.get(p, ()) if kind == "feat" else ():
            for share, f in sorted(m.evidence.get((p, d), ()), reverse=True)[:evidence]:
                lines.append(f"      {share:.2f} {f}")
    paired = {d for ds in m.pairs.values() for d, _ in ds}
    left = [v["n"] for i, v in enumerate(m.variables) if "p" not in v and i not in paired]
    if left:
        lines.append("  debug locals not paired: " + ", ".join(left))
    named = set(m.pairs) | set(m.arguments)
    rest = [v.name for i, v in enumerate(m.lvars) if v.name and i not in named]
    if rest:
        lines.append("  PC locals not paired: " + ", ".join(rest))
    return "\n".join(lines)


def survey(ea: int):
    """What the function says beyond its own locals: (votes, None) of its pairing (Match.votes), or, for a function
    with no pairing, ([], its tokens)."""
    m = match(ea)
    if not isinstance(m, str):
        return m.votes(), None
    try:
        cfunc = hx.decompile(ea)
    except hx.DecompilationFailure:
        return [], None
    return [], tokens(pc_events(cfunc)) if cfunc else None


def run(eas, apply: bool = False, budget: float = 12.0, settings: str | None = None, fresh: bool = False,
        votes: str | None = None) -> dict:
    """Processes functions until the time budget is spent. Returns the counts and the functions left. settings: a
    file that gets each changed function's user lvar settings (exported), for apply_exported in another session.
    fresh: each function's user lvar settings are cleared first, so the result does not build on an earlier run's;
    a function that had settings is then exported even with none left. votes: `votes.votes.tsv` gets each paired
    function's votes (function, kind, target, the target's name now, value, strength), `votes.tokens.jsonl` every
    other function's name, key and tokens (survey), and `votes.debug.jsonl` the tokens of the debug functions of
    each callee name voted for."""
    started = time.time()
    counts = collections.Counter()
    os.makedirs(os.path.dirname(REPORT), exist_ok=True)
    eas = list(eas)
    done = 0
    described = set()
    with open(REPORT, "a", encoding="utf-8") as report, \
            open(settings or os.devnull, "a", encoding="utf-8") as exports, \
            open(votes + ".votes.tsv" if votes else os.devnull, "a", encoding="utf-8") as voted, \
            open(votes + ".tokens.jsonl" if votes else os.devnull, "a", encoding="utf-8") as bodies, \
            open(votes + ".debug.jsonl" if votes else os.devnull, "a", encoding="utf-8") as debug_bodies:
        for ea in eas:
            if time.time() - started > budget:
                break
            done += 1
            had = False
            if fresh and apply:
                vector = hx.lvar_uservec_t()
                had = bool(hx.restore_user_lvar_settings(vector, ea)) and len(vector.lvvec) > 0
                if had:
                    hx.save_user_lvar_settings(ea, hx.lvar_uservec_t())
                    hx.mark_cfunc_dirty(ea)
            try:
                result = process(ea, apply)
            except Exception as error:  # one function's failure is counted, not fatal
                result = f"error: {error!r}"
            if isinstance(result, str):
                counts[result.split(":")[0]] += 1
                if result.startswith(("error", "reverted")):
                    report.write(f"{ea:#x}\t\t\t\t\t0.00\t{result}\tfailed\t0\n")
                result = []
            else:
                counts["functions"] += 1
            for _, old, new, old_type, new_type, score, sources, kind, state, user in result:
                counts[kind] += 1
                counts["renamed"] += old != new and kind != "split"
                counts["retyped"] += old_type != new_type
                counts["reused"] += len(sources) > 1
                counts["user names replaced"] += bool(user and old != new)
                report.write(f"{ea:#x}\t{old}\t{new}\t{old_type}\t{new_type}\t{score:.2f}\t{kind}\t{state}\t{user}\n")
            if settings and (had or any(row[8] == "set" for row in result)):
                exports.write(json.dumps({"ea": ea, "lvars": exported(ea)}) + "\n")
            if votes:
                try:
                    said, body = survey(ea)
                except Exception:  # counted, not fatal
                    counts["survey error"] += 1
                    said, body = [], None
                for kind, target, value, strength in said:
                    name = "" if kind == "p" else (ida_funcs.get_func_name(int(target, 16)) if kind == "f" else
                                                   ida_name.get_name(int(target, 16))) or ""
                    voted.write(f"{ea:#x}\t{kind}\t{target}\t{name}\t{value}\t{strength:.2f}\n")
                    if kind == "f" and value not in described:
                        described.add(value)
                        debug_bodies.write(json.dumps({"key": value, "bodies": [
                            tokens(canonical_events(info)) for _, _, info in candidates(value)]}) + "\n")
                if body is not None:
                    bodies.write(json.dumps({"ea": ea, "name": ida_funcs.get_func_name(ea), "key": function_key(ea),
                                             "t": body}) + "\n")
            if done % 500 == 0:
                # a whole-database run would keep every function's decompile in memory
                hx.clear_cached_cfuncs()
    hx.clear_cached_cfuncs()
    return {"counts": dict(counts), "left": eas[done:]}


def exported(ea: int) -> list:
    """A function's user lvar settings as plain data: where the local lives, where it is defined, its name and its
    serialized type. The same database gives the same locals in every session, so another session can write them
    without decompiling."""
    vector = hx.lvar_uservec_t()
    out = []
    if not hx.restore_user_lvar_settings(vector, ea):
        return out
    for saved in vector.lvvec:
        location = saved.ll.location
        if location.is_stkoff():
            entry = {"stk": location.stkoff()}
        elif location.is_reg1():
            entry = {"reg": location.reg1()}
        else:
            continue
        entry.update(defea=saved.ll.defea, name=saved.name, size=saved.size, flags=saved.flags)
        if not saved.type.empty():
            entry["type"] = [part.hex() if part else "" for part in saved.type.serialize()[:2]]
        out.append(entry)
    return out


def apply_exported(path: str, budget: float = 12.0, start: int = 0) -> dict:
    """Writes the user lvar settings of an exported file (one function per line), from line start, until the time
    budget is spent. Returns the functions written, the entries left out because their type did not load, and the
    next line (None at the end)."""
    started = time.time()
    written = dropped = 0
    with open(path, encoding="utf-8") as source:
        for number, line in enumerate(source):
            if number < start:
                continue
            if time.time() - started > budget:
                return {"written": written, "dropped": dropped, "next": number}
            item = json.loads(line)
            vector = hx.lvar_uservec_t()
            for entry in item["lvars"]:
                saved = hx.lvar_saved_info_t()
                if "stk" in entry:
                    saved.ll.location.set_stkoff(entry["stk"])
                else:
                    saved.ll.location.set_reg1(entry["reg"])
                saved.ll.defea = entry["defea"]
                saved.name = entry["name"]
                saved.size = entry["size"]
                saved.flags = entry["flags"]
                if "type" in entry:
                    kind, fields = (bytes.fromhex(part) for part in entry["type"])
                    if not saved.type.deserialize(None, kind, fields or None):
                        dropped += 1
                        continue
                vector.lvvec.push_back(saved)
            hx.save_user_lvar_settings(item["ea"], vector)
            hx.mark_cfunc_dirty(item["ea"])
            written += 1
    return {"written": written, "dropped": dropped, "next": None}
