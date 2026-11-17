#!/usr/bin/env python3
"""What src/ already has, by exe address and by class: the index the Python tools share (draft.py, checkbody.py).

    import srcindex
    index = srcindex.load()
    index.symbol(0xA80BF0)          # 'NiGeometry::LoadBinary' (registered body, forwarder or exe global)
    index.kind(0xA80BF0)            # 'forwarder'
    index.header("NiGeometry")      # 'src/Gamebryo2.2/CoreLibs/NiMain/NiGeometry.h'
    index.fields("NiGeometry")      # {offset: Field(name, type, size, ...)} as clang lays the class out
    index.slots(0xA80BF0)           # [('NiGeometry', 41), ...] primary exe vtable entries holding the address
    index.pdb_func("NiGeometry::GetShader")   # PDB names, parameter names, types, source files, inline
    index.source_path("NiGeometry::GetShader") # src path of the PDB source file of the function

CLI (for a quick look): srcindex.py ADDRESS|CLASS|QUALIFIED [...]

Caches (env/cache): the src scan per file (by mtime), the PDB signatures (by the dump's mtime) and the exe vtables (by
the exe's mtime) in srcindex-*.pickle; the clang record/vtable layouts per header (by the mtimes, else the contents,
of every file the probe included) in srcindex-layouts.sqlite (LayoutStore), probed by build/srcdb/srcdb
(harness/srcdb). Nothing here writes to src/.

A registration naming its class by a file-scope type alias (`using NiTMapInt64Int = NiTMap<int64_t, int32_t>;`) is
also filed in by_symbol under the aliased type (`NiTMap<int64_t,int32_t>::NewItem`), so callers find it by name.
"""

from __future__ import annotations

import array
import bisect
import collections
import concurrent.futures
import dataclasses
import hashlib
import importlib.util
import json
import os
import pickle
import re
import shlex
import sqlite3
import struct
import subprocess
import sys
import tempfile
import time
from dataclasses import dataclass, field
from typing import NamedTuple

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, "src")
CACHE = os.path.join(ROOT, "env", "cache")
LLVM = "/home/cnf/.local/opt/LLVM-23.1.3-Linux-X64/bin"
HYBRID = os.path.join(ROOT, "build", "hybrid", "compile_commands.json")
TOOLS = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, TOOLS)

# 12: FNVD_REPLACES_CONSTRUCT(_OVERLOAD) / DESTRUCT / DELETING_DESTRUCT registrations are refs
# 13: FNVD_ADDRESS(address, symbol) of an unregistered (KEEP) src function is an `address` ref
# 14: type alias targets keep one space between two words (`const msvc9::string`), for Index.spelling_key
# 15: aliases on the native-only side of an FNVD_HYBRID test are left out
SCAN_VERSION = 15
# 6: namespaced classes are probed for vtables too (by their qualified names)
LAYOUT_VERSION = 6
PDB_VERSION = 4
# 2: vtable-check ends a table at the next slot address code references (ImageSpaceEffect's ran into the next one)
# 3: an adjustor thunk is the compiler's `sub ecx, N` or stdcall `sub dword ptr [esp + 4], N`, then a jmp; an
# `add ecx, N; jmp` is a body
EXE_VERSION = 3


def _vtable_check():
    """vtable-check.py as a module (its file name has a dash)."""
    if "vtable_check" not in sys.modules:
        spec = importlib.util.spec_from_file_location("vtable_check", os.path.join(TOOLS, "vtable-check.py"))
        module = importlib.util.module_from_spec(spec)
        sys.modules["vtable_check"] = module
        spec.loader.exec_module(module)
    return sys.modules["vtable_check"]


def _load_cache(name: str):
    path = os.path.join(CACHE, f"srcindex-{name}.pickle")
    try:
        with open(path, "rb") as handle:
            return pickle.load(handle)
    except (OSError, EOFError, pickle.UnpicklingError, AttributeError, ModuleNotFoundError):
        return None


def _save_cache(name: str, value) -> None:
    os.makedirs(CACHE, exist_ok=True)
    path = os.path.join(CACHE, f"srcindex-{name}.pickle")
    with tempfile.NamedTemporaryFile("wb", dir=CACHE, delete=False) as handle:
        pickle.dump(value, handle, protocol=pickle.HIGHEST_PROTOCOL)
    os.replace(handle.name, path)


def rel(path: str) -> str:
    return os.path.relpath(path, ROOT)


# ---------------------------------------------------------------------------------------------------------------
# src scan: registrations, forwarders, globals, classes, declarations and definitions, with a brace-aware scanner
# ---------------------------------------------------------------------------------------------------------------

@dataclass
class Ref:
    """One exe address src names: a registration, a forwarder call, or an exe global/static/function pointer."""
    address: int
    kind: str  # body | constructor | destructor | forwarder | global | static_member | original_function | address
    symbol: str  # qualified src name ('Class::f', 'gName', 'Class::sName')
    path: str  # repo-relative
    line: int
    form: str = ""  # REPLACES form (VIRTUAL, OVERLOAD, MEMBER, ...) or forwarder convention (thiscall, cdecl_, ...)
    type: str = ""  # declared type of a global / static member / function pointer
    local: bool = False  # file-local (`static FNVD_GLOBAL` at file scope)


@dataclass
class Decl:
    """A member function declared (or defined inline) in a class body."""
    cls: str
    name: str
    path: str
    line: int  # first line of the declaration
    end_line: int  # line of its `;` or closing brace
    text: str  # the declaration as written, comments removed, whitespace collapsed
    access: str
    virtual: bool
    static: bool
    const: bool
    inline_body: bool
    pure: bool
    ret: str
    params: str
    free: bool = False  # a namespace-scope function (cls: its namespace)


@dataclass
class Definition:
    """A function definition with a body (out of line, or inline in a class)."""
    qualified: str
    path: str
    line: int  # line of the first declarator token
    body_start: int  # offset of `{` in the file
    body_end: int  # offset of the matching `}`
    end_line: int
    header_start: int  # offset where the definition's text starts (after the previous statement)
    in_class: bool


@dataclass
class ClassDef:
    name: str
    path: str
    line: int
    open: int  # offset of `{`
    close: int  # offset of the matching `}`
    kind: str  # class | struct | union
    bases: str
    template: bool = False
    namespace: str = ""


@dataclass
class FileScan:
    mtime: float
    refs: list[Ref] = field(default_factory=list)
    decls: list[Decl] = field(default_factory=list)
    definitions: list[Definition] = field(default_factory=list)
    classes: list[ClassDef] = field(default_factory=list)
    enums: dict[str, str] = field(default_factory=dict)  # name (qualified as nested) -> underlying type or ''
    # enum (qualified) -> (scoped, {value: enumerator}); the first enumerator of a value wins
    enumerators: dict[str, tuple[bool, dict[int, str]]] = field(default_factory=dict)
    free_decls: list[Decl] = field(default_factory=list)  # namespace-scope functions (Decl.cls: the namespace)
    # file-scope `using A = T;` / `typedef T A;` (column 0): alias -> T with spaces dropped
    aliases: dict[str, str] = field(default_factory=dict)


def blank(text: str) -> str:
    """The text with comments, string/char literals and preprocessor lines replaced by spaces (newlines kept), so
    offsets and lines stay those of the file."""
    out = list(text)
    i, n = 0, len(text)
    line_start = True
    while i < n:
        c = text[i]
        if c == "\n":
            line_start = True
            i += 1
            continue
        if line_start and c in " \t":
            i += 1
            continue
        if line_start and c == "#":
            # a directive, with its continuation lines
            while i < n and text[i] != "\n":
                if text[i] == "\\" and i + 1 < n and text[i + 1] == "\n":
                    out[i] = " "
                    i += 2
                    continue
                if text.startswith("//", i):
                    break
                if text.startswith("/*", i):
                    break
                out[i] = " "
                i += 1
            line_start = False
            continue
        line_start = False
        if text.startswith("//", i):
            while i < n and text[i] != "\n":
                out[i] = " "
                i += 1
            continue
        if text.startswith("/*", i):
            end = text.find("*/", i + 2)
            end = n if end < 0 else end + 2
            for k in range(i, end):
                if text[k] != "\n":
                    out[k] = " "
            i = end
            continue
        if c == '"' or (c == "'" and not (i and (text[i - 1].isalnum() or text[i - 1] == "_"))):
            raw = c == '"' and i and text[i - 1] == "R"
            if raw:
                delim_end = text.find("(", i)
                delimiter = ")" + text[i + 1:delim_end] + '"'
                end = text.find(delimiter, delim_end)
                end = n if end < 0 else end + len(delimiter)
            else:
                end = i + 1
                while end < n and text[end] != c and text[end] != "\n":
                    end += 2 if text[end] == "\\" else 1
                end += 1
            for k in range(i + 1, min(end - 1, n)):
                if text[k] != "\n":
                    out[k] = " "
            i = end
            continue
        i += 1
    return "".join(out)


ACCESS_LABEL = re.compile(r"^\s*(public|protected|private)\s*:(?!:)")
CLASS_HEAD = re.compile(r"(?:^|[\s>])(class|struct|union)\s+(?:(?:__declspec\s*\([^)]*\)|alignas\s*\([^)]*\)|"
                        r"\[\[[^\]]*\]\])\s*)*([A-Za-z_][\w:]*(?:<[^{]*>)?)\s*(?:final\s*)?(?::(?!:)([^{]*))?$", re.S)
ANON_HEAD = re.compile(r"(?:^|\s)(class|struct|union)\s*$")
ENUM_HEAD = re.compile(r"(?:^|\s)enum(?:\s+(?:class|struct))?\s*([A-Za-z_]\w*)?\s*(?::\s*([\w\s:]+?))?\s*$")
NAMESPACE_HEAD = re.compile(r"(?:^|\s)namespace(?:\s+([\w:]+))?\s*$")
QUALIFIERS_TAIL = re.compile(r"\)\s*((?:const|volatile|override|final|noexcept(?:\s*\([^)]*\))?|&&|&|\s|"
                             r"->\s*[\w:<>*&\s,]+?|=\s*0|=\s*default|=\s*delete)*)$", re.S)
DECLARATOR = re.compile(r"((?:[A-Za-z_]\w*(?:<[^()]*?>)?\s*::\s*)*(?:~\s*[A-Za-z_]\w*|operator\s*(?:\(\)|[^\w\s(]+|"
                        r"\w[\w\s*&:<>]*?)|[A-Za-z_]\w*))\s*$")


def _line_starts(text: str) -> list[int]:
    starts = [0]
    for m in re.finditer("\n", text):
        starts.append(m.end())
    return starts


def _line_of(starts: list[int], offset: int) -> int:
    return bisect.bisect_right(starts, offset)


def _native_only_lines(text: str) -> set[int]:
    """The (1-based) lines on the native-only side of an FNVD_HYBRID test: inside `#if !FNVD_HYBRID` before its
    `#else`/`#elif`, inside the `#else`/`#elif` of `#if FNVD_HYBRID`. Other conditions count as neither side."""
    lines: set[int] = set()
    # per open #if: whether its current branch is native-only, and its test (+1 FNVD_HYBRID, -1 !FNVD_HYBRID, 0)
    stack: list[list] = []
    for number, line in enumerate(text.split("\n"), 1):
        m = re.match(r"\s*#\s*(if|ifdef|ifndef|elif|else|endif)\b(.*)", line)
        if m:
            directive, condition = m.group(1), re.sub(r"//.*|/\*.*?\*/", "", m.group(2)).strip()
            if directive == "if":
                test = 1 if condition == "FNVD_HYBRID" else -1 if condition == "!FNVD_HYBRID" else 0
                stack.append([test == -1, test])
            elif directive in ("ifdef", "ifndef"):
                stack.append([False, 0])
            elif directive == "endif":
                if stack:
                    stack.pop()
            elif stack:
                stack[-1][0] = stack[-1][1] == 1
        elif any(native for native, _ in stack):
            lines.add(number)
    return lines


def _enumerators(body: str) -> dict[int, str]:
    """{value: name} of an enum body; stops at the first value it cannot work out."""
    values: dict[int, str] = {}
    known: dict[str, int] = {}
    following = 0
    for item in _split_top(body):
        m = re.match(r"^\s*([A-Za-z_]\w*)\s*(?:=\s*(.+?))?\s*$", item, re.S)
        if not m:
            break
        if m.group(2) is not None:
            expression = re.sub(r"\b(\d+|0[xX][0-9A-Fa-f]+)[uUlL]+\b", r"\1", m.group(2))
            expression = re.sub(r"\b[A-Za-z_]\w*\b", lambda w: str(known[w.group(0)]) if w.group(0) in known else "?",
                                expression)
            if not re.fullmatch(r"[0-9a-fA-FxX\s|&^<>+\-~()*]+", expression):
                break
            try:
                following = int(eval(expression, {"__builtins__": {}}))  # digits and operators only
            except Exception:
                break
        known[m.group(1)] = following
        values.setdefault(following & 0xFFFFFFFF, m.group(1))
        following += 1
    return values


def _split_top(text: str, sep: str = ",") -> list[str]:
    parts, depth, current = [], 0, ""
    for char in text:
        if char == sep and depth == 0:
            parts.append(current.strip())
            current = ""
            continue
        depth += char in "<([{"
        depth -= char in ">)]}"
        current += char
    if current.strip():
        parts.append(current.strip())
    return parts


def _param_list(header: str) -> tuple[int, int] | None:
    """(open, close) of the function's parameter list in a declaration/definition header: the first top-level `(`
    that follows a declarator name (not inside template arguments)."""
    depth = 0
    i = 0
    while i < len(header):
        c = header[i]
        if c == "<":
            # template brackets: only after an identifier (not `operator<`)
            before = header[:i].rstrip()
            if before and (before[-1].isalnum() or before[-1] == "_") and not before.endswith("operator"):
                depth += 1
        elif c == ">" and depth:
            depth -= 1
        elif c == "(" and depth == 0:
            before = header[:i].rstrip()
            if re.search(r"\boperator$", before) and re.match(r"\(\s*\)", header[i:]):
                # `operator()`: its own parentheses are the name, the parameter list is the next pair
                i = header.index(")", i) + 1
                continue
            if DECLARATOR.search(before) and not re.search(r"(?:__declspec|alignas|decltype|__attribute__)$", before):
                # match the parentheses
                level = 0
                for j in range(i, len(header)):
                    if header[j] == "(":
                        level += 1
                    elif header[j] == ")":
                        level -= 1
                        if level == 0:
                            return i, j
                return None
            return None
        i += 1
    return None


def _function_parts(header: str):
    """(qualified declarator, return text, params text, tail qualifiers, prefix) of a function header, or None."""
    span = _param_list(header)
    if not span:
        return None
    open_, close = span
    before = header[:open_].rstrip()
    m = DECLARATOR.search(before)
    if not m:
        return None
    raw = m.group(1)
    start = m.start(1)
    if not raw.lstrip().startswith("operator"):
        # DECLARATOR's template-argument group may swallow a template return type (`NiPointer<T> List<T>::Remove`
        # would be named `NiPointer<T>List<T>::Remove`): the declarator starts after the last whitespace outside
        # template arguments that is not next to a `::` (and before an `operator` name, which has its own spaces)
        op = re.search(r"\boperator\b", raw)
        limit = op.start() if op else len(raw)
        depth, cut = 0, None
        for k, ch in enumerate(raw[:limit]):
            if ch == "<":
                depth += 1
            elif ch == ">":
                depth -= 1
            elif ch.isspace() and depth == 0 and not raw[:k].rstrip().endswith("::") and \
                    not raw[k:].lstrip().startswith("::") and raw[k + 1:].strip():
                cut = k
        if cut is not None:
            raw, start = raw[cut + 1:], start + cut + 1
    name = re.sub(r"\s+", "", raw) if not raw.lstrip().startswith("operator") else \
        re.sub(r"\s+", " ", raw).strip()
    if "operator" in name:
        name = re.sub(r"\s*::\s*", "::", re.sub(r"operator\s+", "operator ", name)).replace("operator ", "operator", 1) \
            if not re.search(r"operator\s+\w", name) else name
    prefix = before[:start]
    tail = header[close + 1:]
    return name, prefix, header[open_ + 1:close], tail


def scan_file(path: str) -> FileScan:
    with open(path, encoding="utf-8", errors="replace") as handle:
        text = handle.read()
    code = blank(text)
    starts = _line_starts(text)
    relpath = rel(path)
    result = FileScan(os.path.getmtime(path))
    # frames: (kind, name, access, open offset, extra)
    stack: list[list] = [["file", "", "public", -1, None]]
    stmt = 0
    i, n = 0, len(code)
    func_depth = 0  # >0 while inside a function body (nested braces counted)
    func_frame = None

    def scope_names() -> list[str]:
        return [f[1] for f in stack if f[0] == "class" and f[1]]

    def class_qual() -> str:
        names = scope_names()
        return "::".join(names)

    def current_namespace() -> str:
        return "::".join(f[1] for f in stack if f[0] == "ns" and f[1])

    while i < n:
        c = code[i]
        if func_depth:
            if c == "{":
                func_depth += 1
            elif c == "}":
                func_depth -= 1
                if func_depth == 0:
                    qualified, line, open_, header_start, in_class, decl = func_frame
                    result.definitions.append(Definition(qualified, relpath, line, open_, i, _line_of(starts, i),
                                                         header_start, in_class))
                    if decl is not None:
                        decl.end_line = _line_of(starts, i)
                        result.decls.append(decl)
                    func_frame = None
                    stmt = i + 1
            i += 1
            continue
        if c == "{":
            header = code[stmt:i]
            frame = stack[-1]
            # strip access labels at class level
            while frame[0] == "class" and (m := ACCESS_LABEL.match(header)):
                frame[2] = m.group(1)
                header = header[m.end():]
                stmt += 0
            head = " ".join(header.split())
            head_nt = re.sub(r"^template\s*<.*?>\s*(?=(?:class|struct|union|[A-Za-z_]))", "", head)
            if frame[0] in ("init", "enum"):
                stack.append(["init", "", "public", i, None])
            elif (m := NAMESPACE_HEAD.search(head)) and "(" not in head:
                stack.append(["ns", m.group(1) or "", "public", i, None])
            elif (m := ENUM_HEAD.search(head_nt)) and "(" not in head_nt:
                if m.group(1):
                    qual = class_qual()
                    enum = (qual + "::" if qual else "") + m.group(1)
                    result.enums[enum] = (m.group(2) or "").strip()
                    close = code.find("}", i)
                    if close > 0:
                        result.enumerators[enum] = (bool(re.search(r"\benum\s+(?:class|struct)\b", head_nt)),
                                                    _enumerators(code[i + 1:close]))
                stack.append(["enum", m.group(1) or "", "public", i, None])
            elif ("(" not in re.sub(r"(?:alignas|__declspec)\s*\([^)]*\)", "", head_nt).split(":")[0]
                  and (m := CLASS_HEAD.search(head_nt)) and "=" not in head_nt):
                # the frame keeps the name as written (nested: local); class_qual() joins the enclosing ones
                name = re.sub(r"<.*", "", m.group(2))
                kind = m.group(1)
                stack.append(["class", name, "private" if kind == "class" else "public", i,
                              (kind, (m.group(3) or "").strip(), head.startswith("template") or "<" in m.group(2))])
            elif ANON_HEAD.search(head_nt):
                stack.append(["anon", "", frame[2], i, None])
            elif frame[0] in ("file", "ns", "class", "anon") and "(" in head_nt and not re.match(
                    r"^(?:\w+\s+)*\w+(?:\s*\[[^\]]*\])*\s*=", head_nt) and not head_nt.rstrip().endswith("="):
                parts = _function_parts(head_nt)
                # a constructor's member initializer list: cut at the `)` closing the parameters
                if parts:
                    name, prefix, params, tail = parts
                    if frame[0] in ("class", "anon"):
                        qualified = class_qual() + "::" + name if class_qual() else name
                    else:
                        qualified = name
                        outer = class_qual()
                        if outer:
                            qualified = outer + "::" + name
                    # the declarator token's line
                    k = header.rfind(name.split("::")[-1].split("<")[0])
                    line = _line_of(starts, stmt + max(k, 0))
                    decl = None
                    if frame[0] in ("class", "anon") and "friend" not in prefix.split():
                        decl = _make_decl(class_qual(), name, prefix, params, tail, frame[2], relpath,
                                          _line_of(starts, stmt + len(header) - len(header.lstrip())), head, True)
                    elif frame[0] in ("file", "ns") and "::" not in name:
                        free = _make_free_decl(current_namespace(), name, prefix, params, tail, relpath,
                                               _line_of(starts, stmt + len(header) - len(header.lstrip())), head, True)
                        if free:
                            free.end_line = _line_of(starts, i)
                            result.free_decls.append(free)
                    func_frame = (qualified, line, i, stmt, frame[0] in ("class", "anon"), decl)
                    func_depth = 1
                    i += 1
                    continue
                stack.append(["init", "", "public", i, None])
            else:
                stack.append(["init", "", "public", i, None])
            stmt = i + 1
        elif c == "}":
            frame = stack.pop() if len(stack) > 1 else stack[0]
            if frame[0] == "class":
                kind, bases, template = frame[4]
                outer = "::".join(f[1] for f in stack if f[0] == "class" and f[1])
                name = frame[1] if "::" in frame[1] or not outer else outer + "::" + frame[1]
                namespace = "::".join(f[1] for f in stack if f[0] == "ns")
                result.classes.append(ClassDef(name, relpath, _line_of(starts, frame[3]), frame[3], i, kind, bases,
                                               template or any(f[0] == "class" and f[4][2] for f in stack),
                                               namespace))
            stmt = i + 1
        elif c == ";":
            frame = stack[-1]
            if frame[0] in ("class", "anon"):
                header = code[stmt:i]
                while (m := ACCESS_LABEL.match(header)):
                    frame[2] = m.group(1)
                    header = header[m.end():]
                head = " ".join(header.split())
                if "(" in head and not re.match(r"^(?:FNVD_|static_assert|using|typedef|friend|template\s*<[^>]*>\s*"
                                                 r"friend)", head):
                    parts = _function_parts(re.sub(r"^template\s*<.*?>\s*", "", head))
                    if parts:
                        name, prefix, params, tail = parts
                        if "=" not in prefix and not re.search(r"[\w\]]\s*=\s*[^=]", prefix):
                            decl = _make_decl(class_qual(), name, prefix, params, tail, frame[2], relpath,
                                              _line_of(starts, stmt + len(header) - len(header.lstrip())), head, False)
                            if decl:
                                decl.end_line = _line_of(starts, i)
                                result.decls.append(decl)
            elif frame[0] in ("file", "ns"):
                header = code[stmt:i]
                head = " ".join(header.split())
                if "(" in head and not re.match(r"^(?:FNVD_|static_assert|using|typedef|template\s*<[^>]*>\s*friend)",
                                                head):
                    parts = _function_parts(re.sub(r"^template\s*<.*?>\s*", "", head))
                    if parts:
                        name, prefix, params, tail = parts
                        if "::" not in name and "=" not in prefix and not re.search(r"[\w\]]\s*=\s*[^=]", prefix):
                            free = _make_free_decl(current_namespace(), name, prefix, params, tail, relpath,
                                                   _line_of(starts, stmt + len(header) - len(header.lstrip())), head,
                                                   False)
                            if free:
                                free.end_line = _line_of(starts, i)
                                result.free_decls.append(free)
            stmt = i + 1
        elif c == ":" and stack[-1][0] in ("class", "anon"):
            # access label (not `::`)
            if code[i - 1] != ":" and (i + 1 >= n or code[i + 1] != ":"):
                m = re.search(r"(public|protected|private)\s*$", code[stmt:i])
                if m and not code[stmt:stmt + m.start()].strip():
                    stack[-1][2] = m.group(1)
                    stmt = i + 1
        i += 1

    _scan_refs(text, code, starts, relpath, result)
    return result


def _make_decl(cls: str, name: str, prefix: str, params: str, tail: str, access: str, path: str, line: int,
               text: str, inline_body: bool) -> Decl | None:
    if not cls:
        return None
    words = prefix.split()
    if any(w in ("friend", "typedef", "using") for w in words) or name.startswith("FNVD_") or name.isupper():
        return None
    short = name.split("::")[-1]
    flags = re.sub(r"\b(?:virtual|static|inline|explicit|constexpr|__forceinline|FNVD_\w+|\[\[[^\]]*\]\])\s*", "",
                   prefix).strip()
    return Decl(cls, short, path, line, line, text, access, "virtual" in words or "override" in tail,
                "static" in words, bool(re.search(r"\bconst\b", tail)), inline_body, bool(re.search(r"=\s*0", tail)),
                flags, params.strip())


def _make_free_decl(namespace: str, name: str, prefix: str, params: str, tail: str, path: str, line: int, text: str,
                    inline_body: bool) -> Decl | None:
    """A namespace-scope function: Decl.cls is the namespace ('' for the global one), static marks a file-local one."""
    words = prefix.split()
    if not prefix.strip() or any(w in ("friend", "typedef", "using", "return", "extern", "operator") for w in words):
        return None
    if name.startswith(("FNVD_", "operator")) or name.isupper() or not re.fullmatch(r"[A-Za-z_]\w*", name):
        return None
    flags = re.sub(r"\b(?:static|inline|constexpr|__forceinline|FNVD_\w+|\[\[[^\]]*\]\])\s*", "", prefix).strip()
    if not flags:
        return None
    return Decl(namespace, name, path, line, line, text, "public", False, "static" in words, False, inline_body, False,
                flags, params.strip(), True)


REPLACES_REF = re.compile(r"\bFNVD_REPLACES(?:_(CONSTRUCTOR|DESTRUCTOR|CONSTRUCT_OVERLOAD|CONSTRUCT|DELETING_DESTRUCT|"
                          r"DESTRUCT|OVERLOAD|VIRTUAL_VIA_OVERLOAD|VIRTUAL_VIA|VIRTUAL_OVERLOAD|VIRTUAL|MEMBER))?\s*\(")
FORWARDER_REF = re.compile(r"\bfnvd::original::(thiscall|cdecl_|stdcall|fastcall|promoted)\b")
GLOBAL_REF = re.compile(r"\bFNVD_(GLOBAL|STATIC_MEMBER|ORIGINAL_FUNCTION)\s*\(")
# a src function left unregistered on purpose (KEEP) carrying its exe address: bound, never registered
ADDRESS_REF = re.compile(r"\bFNVD_ADDRESS\s*\(")


def _macro_args(code: str, open_: int) -> tuple[list[str], int] | None:
    depth = 0
    for j in range(open_, len(code)):
        if code[j] == "(":
            depth += 1
        elif code[j] == ")":
            depth -= 1
            if depth == 0:
                return _split_top(code[open_ + 1:j]), j
    return None


def _scan_refs(text: str, code: str, starts: list[int], path: str, result: FileScan) -> None:
    spans = sorted((d.body_start, d.body_end, d.qualified) for d in result.definitions)
    span_starts = [s[0] for s in spans]
    classes = sorted(result.classes, key=lambda c: c.open)

    def enclosing_function(offset: int) -> str:
        k = bisect.bisect_right(span_starts, offset) - 1
        while k >= 0:
            start, end, name = spans[k]
            if start <= offset <= end:
                return name
            k -= 1
        return ""

    def enclosing_class(offset: int) -> str:
        best = ""
        for c in classes:
            if c.open < offset < c.close:
                best = c.name
        return best

    for m in REPLACES_REF.finditer(code):
        parsed = _macro_args(code, m.end() - 1)
        if not parsed:
            continue
        args, _ = parsed
        try:
            address = int(args[0], 16)
        except (ValueError, IndexError):
            continue
        form = m.group(1) or ""
        if form in ("VIRTUAL", "VIRTUAL_OVERLOAD", "MEMBER"):
            symbol = re.sub(r"\s+", "", args[1]) + "::" + args[2].strip()
            kind = "body"
        elif form in ("VIRTUAL_VIA", "VIRTUAL_VIA_OVERLOAD"):
            # (address, type, base, member): the base is the subobject the exe passes as this
            symbol = re.sub(r"\s+", "", args[1]) + "::" + args[3].strip()
            kind = "body"
        elif form == "CONSTRUCTOR":
            cls = re.sub(r"\s+", "", args[1])
            symbol, kind = cls + "::" + cls.split("::")[-1], "constructor"
        elif form == "DESTRUCTOR":
            cls = re.sub(r"\s+", "", args[1])
            symbol, kind = cls + "::~" + cls.split("::")[-1], "destructor"
        else:
            symbol, kind = re.sub(r"\s+", "", args[1]), "body"
        result.refs.append(Ref(address, kind, symbol, path, _line_of(starts, m.start()), form or "PLAIN"))
    for m in ADDRESS_REF.finditer(code):
        parsed = _macro_args(code, m.end() - 1)
        if not parsed or len(parsed[0]) < 2:
            continue
        args, _ = parsed
        try:
            address = int(args[0], 16)
        except ValueError:
            continue
        result.refs.append(Ref(address, "address", re.sub(r"\s+", "", args[1]), path, _line_of(starts, m.start()),
                               "ADDRESS"))
    for m in FORWARDER_REF.finditer(code):
        # the address is the first call argument after the template arguments
        call = re.compile(r"\s*<").match(code, m.end())
        at = m.end()
        if call:
            depth = 0
            for j in range(m.end(), len(code)):
                if code[j] == "<":
                    depth += 1
                elif code[j] == ">":
                    depth -= 1
                    if depth == 0:
                        at = j + 1
                        break
        a = re.compile(r"\s*\(\s*(0x[0-9A-Fa-f]+)").match(code, at)
        if not a:
            continue
        result.refs.append(Ref(int(a.group(1), 16), "forwarder", enclosing_function(m.start()), path,
                               _line_of(starts, m.start()), m.group(1)))
    for m in GLOBAL_REF.finditer(code):
        parsed = _macro_args(code, m.end() - 1)
        if not parsed:
            continue
        args, _ = parsed
        if len(args) < 3:
            continue
        try:
            address = int(args[-1], 16)
        except ValueError:
            continue
        kind = {"GLOBAL": "global", "STATIC_MEMBER": "static_member", "ORIGINAL_FUNCTION": "original_function"}[
            m.group(1)]
        name = args[-2].strip()
        cls = enclosing_class(m.start())
        function = enclosing_function(m.start())
        symbol = (cls + "::" + name) if cls and kind == "static_member" else name
        line_text = code[starts[_line_of(starts, m.start()) - 1]:m.start()]
        local = bool(re.search(r"\bstatic\s*$", line_text)) and kind == "global" or bool(function)
        result.refs.append(Ref(address, kind, symbol, path, _line_of(starts, m.start()), m.group(1),
                               ", ".join(args[:-2]), local))
    # file-scope type aliases, which registrations name instead of a template instance whose arguments hold a comma
    # (`using NiTMapInt64Int = NiTMap<int64_t, int32_t>; FNVD_REPLACES_VIRTUAL(..., NiTMapInt64Int, NewItem)`); spaces
    # dropped, except one between two words (`const msvc9::string`, `unsigned int`). The checker checks the hybrid:
    # an alias on the native-only side (`#if FNVD_HYBRID` VC9 class `#else` std spelling `#endif`) is left out.
    native = _native_only_lines(text)
    for m in re.finditer(r"^using\s+(\w+)\s*=\s*([^;{}()]+);", code, re.M):
        if _line_of(starts, m.start()) not in native:
            result.aliases[m.group(1)] = _alias_spacing(m.group(2))
    for m in re.finditer(r"^typedef\s+([^;{}()]+?)\s+(\w+)\s*;", code, re.M):
        if _line_of(starts, m.start()) not in native:
            result.aliases[m.group(2)] = _alias_spacing(m.group(1))


def _alias_spacing(text: str) -> str:
    return re.sub(r"(?<!\w) | (?!\w)", "", re.sub(r"\s+", " ", text.strip()))


def _src_files() -> list[str]:
    found = []
    for directory, _, files in os.walk(SRC):
        for name in files:
            if name.endswith((".h", ".cpp", ".inl", ".hpp")):
                found.append(os.path.join(directory, name))
    return sorted(found)


def scan_all() -> dict[str, FileScan]:
    """Every src file's scan, from the cache where the file's mtime is the one scanned, else where a file of the same
    repo-relative path and content (sha1) was scanned: a scan depends only on these two, so a scan made in another
    checkout (the worktrees share env/cache) or before a copy (a tree, a merge) holds."""
    cached = _load_cache("scan")
    if not cached or cached.get("version") != SCAN_VERSION:
        cached = {"version": SCAN_VERSION, "files": {}}
    files = cached["files"]
    # (repo-relative path, sha1) -> scan, the current files' last (least recently used first)
    known: dict[tuple[str, str], FileScan] = cached.get("content", {})
    current = _src_files()
    changed = False
    used: dict[tuple[str, str], FileScan] = {}
    for path in current:
        mtime = os.path.getmtime(path)
        scan = files.get(path)
        key = (rel(path), _file_sha(path))
        if scan is None or scan.mtime != mtime:
            found = known.get(key)
            scan = files[path] = dataclasses.replace(found, mtime=mtime) if found is not None else scan_file(path)
            changed = True
        changed |= key not in known
        used[key] = scan
    for path in set(files) - set(current):
        del files[path]
        changed = True
    # the other content scans stay (another checkout's files) up to twice as many as the current ones
    others = [(k, v) for k, v in known.items() if k not in used]
    cached["content"] = dict(others[max(0, len(others) - 2 * len(used)):]) | used
    if changed:
        _save_cache("scan", cached)
    return files


# every src registration spells its original as a hex literal (vtable-check.py and verify-scope.py rely on it too)
SRC_REGISTRATION = re.compile(r"\bFNVD_REPLACES\w*\(\s*(0x[0-9A-Fa-f]+)\s*,\s*([^,)\n]+)")
LINE_COMMENT = re.compile(r"//[^\n]*")
BLOCK_COMMENT = re.compile(r"/\*.*?\*/", re.S)
# the guard idakit merge writes around a marked (hybrid-only) emitted body; the `#else` holds the forwarder
EMITTED_IF = "#if FNVD_HYBRID  // emitted: unresolved spots marked"
EMITTED_END = "#endif  // emitted"


class Registration(NamedTuple):
    name: str
    path: str  # checkout-relative
    line: int
    emitted: bool  # inside an emitted guard: a marked body, hybrid-only


def src_registrations() -> dict[int, Registration]:
    """Original -> its first uncommented registration under src/ (a plain text scan, without the cached index)."""
    found: dict[int, Registration] = {}
    for path in _src_files():
        with open(path, encoding="utf-8", errors="replace") as handle:
            text = handle.read()
        emitted, inside = set(), False
        for number, line in enumerate(text.split("\n"), 1):
            stripped = line.strip()
            if stripped == EMITTED_IF:
                inside = True
            elif stripped == EMITTED_END:
                inside = False
            elif inside:
                emitted.add(number)
        # block comments keep their newlines so the line numbers stay the file's
        text = LINE_COMMENT.sub("", BLOCK_COMMENT.sub(lambda m: "\n" * m.group().count("\n"), text))
        for match in SRC_REGISTRATION.finditer(text):
            number = text.count("\n", 0, match.start()) + 1
            found.setdefault(int(match.group(1), 16),
                             Registration(match.group(2).strip(), rel(path), number, number in emitted))
    return found


# ---------------------------------------------------------------------------------------------------------------
# clang layouts per header: record layouts (fields) and vtable layouts (slot -> member)
# ---------------------------------------------------------------------------------------------------------------

@dataclass
class Field:
    name: str  # access path from the class: 'm_kWorld.m_Translate.x', 'm_aspProps[3]'
    type: str  # as written in src ('uint32_t', 'NiNode*')
    size: int | None
    offset: int = 0
    canonical: str = ""  # typedefs resolved ('unsigned int', 'class NiNode *')
    kind: str = ""  # int | uint | bool | float | ptr | enum | record | array | other
    bits: tuple[int, int] | None = None  # bitfield (first bit, last bit)
    owner: str = ""  # the class (or base) that declares it

    @property
    def spelled(self) -> str:
        """The type fully qualified, typedefs resolved, in src spelling ('StartMenu::UserOption*')."""
        return _tidy_type(re.sub(r"\s*__attribute__\(\(\w+\)\)", "", self.canonical))

    @property
    def target(self) -> str:
        """The record the field is or points to, as fields()/record() take it ('StartMenu::UserOption'), else ''."""
        t = re.sub(r"(\[\d+\])+$", "", self.spelled)
        t = re.sub(r"^(?:const|volatile)\s+|\s*\b(?:const|volatile)\b", "", t).strip()
        t = t.rstrip("*& ").strip()
        t = re.sub(r"^(?:const|volatile)\s+", "", t)
        return "" if t in PRIMITIVES or "(" in t else t


PRIMITIVES = {
    "bool": (1, "bool"), "_Bool": (1, "bool"), "char": (1, "int"), "signed char": (1, "int"),
    "unsigned char": (1, "uint"),
    "char8_t": (1, "uint"), "short": (2, "int"), "unsigned short": (2, "uint"), "wchar_t": (2, "uint"),
    "char16_t": (2, "uint"), "int": (4, "int"), "unsigned int": (4, "uint"), "long": (4, "int"),
    "unsigned long": (4, "uint"), "char32_t": (4, "uint"), "long long": (8, "int"), "unsigned long long": (8, "uint"),
    "__int64": (8, "int"), "unsigned __int64": (8, "uint"), "float": (4, "float"), "double": (8, "float"),
    "long double": (8, "float"), "std::nullptr_t": (4, "ptr"), "nullptr_t": (4, "ptr"),
}
RECORD_LINE = re.compile(r"^\s*(\d+)(?::(\d+)-(\d+))? \| ( *)(.*)$")


def plain_type(name: str) -> str:
    return re.sub(r"\b(?:class|struct|union|enum) ", "", name).strip()


def _tidy_type(name: str) -> str:
    """'class NiNode *' -> 'NiNode*' (src spelling)."""
    name = plain_type(name)
    name = re.sub(r"\s+([*&])", r"\1", name)
    name = re.sub(r"\s*>\s*>", ">>", name)
    return name


# The exe's MSVC 2008 standard library: IDA and the PDB spell its classes std::X, src models them as msvc9::X
# (src/SDK/MSVC). The C++ language's own std:: names (std::nullptr_t, std::byte, the <cstdint> types) stay std::.
MSVC9_KEEP_STD = re.compile(r"(?:nullptr_t|byte|size_t|ptrdiff_t|u?int(?:8|16|32|64)_t)\b")


def msvc9_spelling(text: str) -> str:
    """An IDA or PDB type or class name with the VC9 library's std:: as src's msvc9:: ('std::basic_string<char,
    std::char_traits<char>,std::allocator<char> >' -> 'msvc9::basic_string<char,msvc9::char_traits<char>,...>')."""
    return re.sub(r"(?<![\w:])std::(?=[A-Za-z_])",
                  lambda m: m.group(0) if MSVC9_KEEP_STD.match(text, m.end()) else "msvc9::", text)


def _parse_records(dump: str) -> list[tuple[str, int, list[tuple]]]:
    """[(record name, sizeof, entries)], entries (offset, bits|None, depth, text) in dump order."""
    records = []
    lines = dump.splitlines()
    at = 0
    while at < len(lines):
        if lines[at] != "*** Dumping AST Record Layout":
            at += 1
            continue
        at += 1
        entries = []
        name, size = None, None
        while at < len(lines):
            line = lines[at]
            if "[sizeof=" in line:
                size = int(re.search(r"sizeof=(\d+)", line).group(1))
                at += 1
                break
            m = RECORD_LINE.match(line)
            if m:
                depth = len(m.group(4)) // 2
                bits = (int(m.group(2)), int(m.group(3))) if m.group(2) is not None else None
                if name is None and depth == 0:
                    name = _tidy_type(m.group(5))
                else:
                    entries.append((int(m.group(1)), bits, depth, m.group(5)))
            at += 1
        if name is not None:
            records.append((name, size or 0, entries))
    return records


_PROBE_COMMAND: dict[float, tuple[str, list[str]]] = {}  # compile_commands.json's mtime -> _probe_command


def _probe_command() -> tuple[str, list[str]]:
    stamp = os.path.getmtime(HYBRID)
    known = _PROBE_COMMAND.get(stamp)
    if known is not None:
        return known[0], list(known[1])
    with open(HYBRID) as handle:
        entry = next(e for e in json.load(handle) if "/src/" in e["file"])
    words = [w for w in shlex.split(entry["command"]) if w != entry["file"] and w != "--"
             and not w.startswith(("/Fo", "/Fd")) and w not in ("-c", "/c", "-Zi", "/Zi")]
    # the machine is shared: compiles run at the lowest priority
    _PROBE_COMMAND[stamp] = (entry["directory"], ["nice", "-n", "19"] + words)
    return entry["directory"], ["nice", "-n", "19"] + words


# srcdb (harness/srcdb): the layout probe as one in-process clang parse. The checkout's build: a shadow tree links
# harness/tools to the checkout's files, so this file's real path names the checkout; an agent worktree (env/wt/NAME)
# links env to the main checkout's and has no build of its own, so its env's real parent is the main checkout
_HERE = os.path.dirname(os.path.dirname(os.path.dirname(os.path.realpath(__file__))))
SRCDB = next((p for p in (os.path.join(_HERE, "build", "srcdb", "srcdb"),
                          os.path.join(os.path.dirname(os.path.realpath(os.path.join(_HERE, "env"))), "build", "srcdb",
                                       "srcdb"))
              if os.path.exists(p)), os.path.join(_HERE, "build", "srcdb", "srcdb"))
SRCDB_BUILD = ("cmake -S harness/srcdb -B build/srcdb -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_COMPILER=clang "
               "-DCMAKE_CXX_COMPILER=clang++ && cmake --build build/srcdb")


def _file_sha(path: str) -> str:
    with open(path, "rb") as handle:
        return hashlib.sha1(handle.read()).hexdigest()


def _run_probe(header: str, classes: list[str], includes: tuple[str, ...] = (), instance: str = "") -> dict:
    """Lay out a TU including the header (and INCLUDES) with srcdb (one clang parse with the hybrid build's flags):
    record layouts (sugared and canonical, as -fdump-record-layouts-complete prints them), the src and harness files it
    includes, and the vtable layouts of the given (polymorphic) classes and their bases. INSTANCE (a class template
    instance) is instantiated by a `sizeof`, so its record is laid out. A class the TU cannot name has no vtables."""
    if not os.path.exists(SRCDB):
        raise RuntimeError(f"srcindex: {SRCDB} is missing; build it: {SRCDB_BUILD}")
    directory, command = _probe_command()
    words = command[3:] if command[:3] == ["nice", "-n", "19"] else command
    text = "".join(f'#include "{os.path.relpath(h, SRC)}"\n' for h in dict.fromkeys((header, *includes)))
    if instance:
        text += f"static_assert(sizeof({instance}) > 0);\n"
    # the classes are named after the TU proper: an error there drops only that class's vtables
    alias_line = text.count("\n") + 1
    text += "".join(f"using fnvdSrcdbClass{k} = {c};\n" for k, c in enumerate(classes))
    work = os.path.join(ROOT, "env", "scratch", "srcdb")
    os.makedirs(work, exist_ok=True)
    with tempfile.NamedTemporaryFile("w", prefix="probe-", suffix=".cpp", dir=work, delete=False) as handle:
        handle.write(text)
    request = {"directory": directory, "command": words, "file": handle.name, "classes": classes,
               "alias_line": alias_line, "dep_roots": [SRC + os.sep, os.path.join(ROOT, "harness")]}
    try:
        done = subprocess.run(["nice", "-n", "19", SRCDB, "probe"], input=json.dumps(request), capture_output=True,
                              text=True)
    finally:
        os.unlink(handle.name)
    if done.returncode:
        raise RuntimeError(f"srcindex: srcdb probe of {header} failed ({done.returncode}): {done.stderr[-2000:]}")
    result = json.loads(done.stdout)
    if not result["ok"]:
        return {"ok": False, "error": result["error"][-6000:], "deps": {header: os.path.getmtime(header)}}
    found = [p for p in dict.fromkeys((header, *result["deps"])) if os.path.exists(p)]
    vtables = {tuple(t["path"]): [_vtable_entry(e["text"], e["adjust"]) for e in t["entries"]]
               for t in result["vtables"]}
    return {"ok": True, "records": _parse_records(result["records"]), "canonical": _parse_records(result["canonical"]),
            "vtables": vtables, "deps": {p: os.path.getmtime(p) for p in found},
            "sha": {p: _file_sha(p) for p in found}}


def _vtable_entry(text: str, adjust) -> dict:
    """A vftable slot as srcdb spells it (-fdump-vtable-layouts' text; the this adjustment, an int when only
    non-virtual): its owner, name, return and parameter types and constness (vtable-check's signature parse)."""
    vtc = _vtable_check()
    signature = vtc.SIGNATURE.match(text)
    if not signature:
        return {"owner": "", "name": "", "ret": "", "params": [], "text": text, "adjust": adjust, "const": False}
    return {"owner": signature["owner"], "name": signature["name"], "ret": signature["ret"],
            "params": vtc.split_params(signature["params"]), "text": text, "adjust": adjust,
            "const": bool(re.search(r"\)\s*const\b", text))}


@dataclass
class Record:
    name: str
    size: int
    entries: list[tuple]  # (offset, bits, depth, text, canonical text)
    primary_chain: list[str] = field(default_factory=list)


LAYOUT_DB = os.path.join(CACHE, "srcindex-layouts.sqlite")
# the database's own format (meta 'version' is LAYOUT_VERSION/this); another one is recreated empty
_LAYOUT_SCHEMA_VERSION = 3
# body ids AUTOINCREMENT: an id drop/prune freed is never given to another body, so a process's cached body for an id
# (LayoutStore._bodies) cannot turn into another layout
_LAYOUT_SCHEMA = """
CREATE TABLE probe (key TEXT PRIMARY KEY, ok INTEGER, error TEXT, deps TEXT, sha TEXT, records BLOB, canonical BLOB,
                    vtables BLOB);
CREATE TABLE body (id INTEGER PRIMARY KEY AUTOINCREMENT, hash BLOB UNIQUE, data BLOB);
"""
# connections a forked child inherited: kept open, never closed there (closing a parent's SQLite connection in a
# child corrupts the parent's locks); the child opens its own
_INHERITED: list[sqlite3.Connection] = []


def _ids(blob: bytes | None) -> array.array:
    ids = array.array("q")
    if blob:
        ids.frombytes(blob)
    return ids


class LayoutStore:
    """The layout probes by key (a header, or `HEADER#INSTANCE`) in LAYOUT_DB, as Layouts.cache["headers"]: a
    mapping of probe entries ({ok, records, canonical, vtables, deps, sha} or {ok, error, deps}). Each record,
    canonical record and vtable is one body row, stored once by content (the record every header including it laid
    out alike), and a probe holds the ids of its bodies in order. A process reads only the keys it asks for, the bodies
    shared between its entries: the pickle of every probe this replaces had grown to 930 MB, ~4 GB in every process
    that loaded it, for 18,650 distinct bodies (2026-11-06). Processes share the database (WAL)."""

    def __init__(self, path: str | None = None):
        self.path = path
        self.entries: dict[str, dict] = {}  # read or set in this process
        self._bodies: dict[int, object] = {}  # body id -> its value, shared by the entries holding it
        self._db: sqlite3.Connection | None = None
        self._pid = 0

    def db(self) -> sqlite3.Connection:
        """This process's connection (a forked worker opens its own)."""
        if self._db is None or self._pid != os.getpid():
            if self._db is not None:
                _INHERITED.append(self._db)
            # the real path: a shadow tree's env/cache links the checkout's database, whose WAL must be the one
            path = os.path.realpath(self.path or LAYOUT_DB)
            os.makedirs(os.path.dirname(path), exist_ok=True)
            db = sqlite3.connect(path, timeout=600, isolation_level=None)
            db.execute("PRAGMA journal_mode=WAL")
            db.execute("PRAGMA synchronous=NORMAL")
            want = (LAYOUT_VERSION, _LAYOUT_SCHEMA_VERSION)
            db.execute("CREATE TABLE IF NOT EXISTS meta (name TEXT PRIMARY KEY, value TEXT)")

            def stored() -> tuple[int, int] | None:
                row = db.execute("SELECT value FROM meta WHERE name = 'version'").fetchone()
                return tuple(int(part) for part in row[0].split("/")) if row else None

            if stored() != want:
                db.execute("BEGIN IMMEDIATE")
                have = stored()
                if have is not None and have > want:
                    # newer code wrote it (the shared database: a worktree or agent on older code must not wipe it)
                    db.execute("ROLLBACK")
                    raise RuntimeError(f"srcindex: {path} is layout format {have}, this code writes {want}: run "
                                       f"newer code (merge master)")
                if have != want:
                    # an older format (or none): recreated empty
                    for (table,) in db.execute("SELECT name FROM sqlite_master WHERE type = 'table' AND "
                                               "name != 'meta' AND name NOT LIKE 'sqlite_%'").fetchall():
                        db.execute(f"DROP TABLE {table}")
                    for statement in _LAYOUT_SCHEMA.split(";"):
                        if statement.strip():
                            db.execute(statement)
                    db.execute("INSERT OR REPLACE INTO meta VALUES ('version', ?)", ("/".join(map(str, want)),))
                db.execute("COMMIT")
            self._db, self._pid = db, os.getpid()
            self._bodies = {}
        return self._db

    def _read(self, key: str) -> dict | None:
        db = self.db()
        row = db.execute("SELECT ok, error, deps, sha, records, canonical, vtables FROM probe WHERE key = ?",
                         (key,)).fetchone()
        if row is None:
            return None
        ok, error, deps, sha, *blobs = row
        if not ok:
            return {"ok": False, "error": error, "deps": json.loads(deps)}
        lists = [_ids(blob) for blob in blobs]
        # the bodies another entry of this process read already are shared, not read again
        unknown = sorted({body for ids in lists for body in ids} - self._bodies.keys())
        for k in range(0, len(unknown), 500):
            chunk = unknown[k:k + 500]
            for body, data in db.execute(f"SELECT id, data FROM body WHERE id IN ({','.join('?' * len(chunk))})",
                                         chunk):
                self._bodies[body] = pickle.loads(data)
        records, canonical, vtables = ([self._bodies[body] for body in ids] for ids in lists)
        return {"ok": True, "records": records, "canonical": canonical, "vtables": dict(vtables),
                "deps": json.loads(deps), "sha": json.loads(sha)}

    def get(self, key: str, default=None):
        entry = self.entries.get(key)
        if entry is None:
            entry = self._read(key)
            if entry is None:
                return default
            self.entries[key] = entry
        return entry

    def __getitem__(self, key: str) -> dict:
        entry = self.get(key)
        if entry is None:
            raise KeyError(key)
        return entry

    def __setitem__(self, key: str, entry: dict) -> None:
        self.entries[key] = entry

    def __contains__(self, key: str) -> bool:
        return self.get(key) is not None

    def keys(self) -> list[str]:
        return sorted({k for (k,) in self.db().execute("SELECT key FROM probe")} | set(self.entries))

    def items(self):
        """Every probe (read one at a time; only this process's own stay in memory, the bodies shared)."""
        for key in self.keys():
            entry = self.entries.get(key) or self._read(key)
            if entry is not None:
                yield key, entry

    def values(self):
        return (entry for _, entry in self.items())

    def _transaction(self, work) -> None:
        db = self.db()
        db.execute("BEGIN IMMEDIATE")
        try:
            work(db)
            db.execute("COMMIT")
        except BaseException:
            db.execute("ROLLBACK")
            raise

    def save(self, keys) -> None:
        """Writes the entries of keys (one transaction)."""
        def work(db):
            for key in keys:
                entry = self.entries[key]
                lists = [None, None, None]
                if entry.get("ok"):
                    values = (entry["records"], entry["canonical"], list(entry["vtables"].items()))
                    lists = [self._body_ids(db, items).tobytes() for items in values]
                db.execute("INSERT OR REPLACE INTO probe VALUES (?, ?, ?, ?, ?, ?, ?, ?)",
                           (key, int(bool(entry.get("ok"))), entry.get("error", ""), json.dumps(entry["deps"]),
                            json.dumps(entry.get("sha", {})), *lists))
        self._transaction(work)

    @staticmethod
    def _body_ids(db: sqlite3.Connection, values: list) -> array.array:
        ids = array.array("q")
        for value in values:
            # by content: repr is the same text for equal values (pickle's bytes depend on object identity)
            digest = hashlib.sha1(repr(value).encode()).digest()
            found = db.execute("SELECT id FROM body WHERE hash = ?", (digest,)).fetchone()
            if found is None:
                found = (db.execute("INSERT INTO body (hash, data) VALUES (?, ?)",
                                    (digest, pickle.dumps(value, protocol=pickle.HIGHEST_PROTOCOL))).lastrowid,)
            ids.append(found[0])
        return ids

    def rows(self, prefix: str):
        """(key, ok, deps, sha) of the probes whose keys start with prefix, without their bodies."""
        for key, ok, deps, sha in self.db().execute(
                "SELECT key, ok, deps, sha FROM probe WHERE substr(key, 1, length(?)) = ?", (prefix, prefix)):
            yield key, bool(ok), json.loads(deps), json.loads(sha or "{}")

    def copy(self, moves) -> None:
        """(source key, target key, deps, sha) per probe: the target gets the source's bodies with these deps and
        shas (one transaction; fieldgen's export of a tree's probes to the checkout)."""
        def work(db):
            for source, target, deps, sha in moves:
                db.execute("INSERT OR REPLACE INTO probe SELECT ?, ok, error, ?, ?, records, canonical, vtables "
                           "FROM probe WHERE key = ?", (target, json.dumps(deps), json.dumps(sha), source))
        self._transaction(work)

    def copy_tree(self, old: str, new: str) -> int:
        """The probes of old's src/ headers as new's (a shadow tree's warm start): keys and the dependency paths in
        deps and sha moved from old/ to new/, bodies shared. The number copied."""
        source, target = old + os.sep + "src" + os.sep, new + os.sep + "src" + os.sep
        quoted_old, quoted_new = json.dumps(old + os.sep)[:-1], json.dumps(new + os.sep)[:-1]
        count = 0

        def work(db):
            nonlocal count
            db.execute("DELETE FROM probe WHERE substr(key, 1, length(?)) = ?", (target, target))
            count = db.execute(
                "INSERT INTO probe SELECT ? || substr(key, length(?) + 1), ok, error, replace(deps, ?, ?), "
                "replace(sha, ?, ?), records, canonical, vtables FROM probe WHERE substr(key, 1, length(?)) = ?",
                (target, source, quoted_old, quoted_new, quoted_old, quoted_new, source, source)).rowcount
        self._transaction(work)
        return count

    def _forget(self, db: sqlite3.Connection, keys) -> None:
        """Deletes the probes of keys and the bodies no probe holds any more."""
        for key in keys:
            db.execute("DELETE FROM probe WHERE key = ?", (key,))
        held = {body for blobs in db.execute("SELECT records, canonical, vtables FROM probe")
                for blob in blobs for body in _ids(blob)}
        db.execute("CREATE TEMP TABLE IF NOT EXISTS held (id INTEGER PRIMARY KEY)")
        db.execute("DELETE FROM held")
        db.executemany("INSERT INTO held VALUES (?)", ((body,) for body in held))
        db.execute("DELETE FROM body WHERE id NOT IN (SELECT id FROM held)")
        db.execute("DELETE FROM held")
        self.entries = {k: v for k, v in self.entries.items() if k not in keys}
        self._bodies = {}

    def drop(self, prefix: str) -> int:
        """Deletes the probes whose keys start with prefix (a tree's, once exported) and the bodies no probe holds
        any more. The number of probes deleted."""
        keys = {key for key, *_ in self.rows(prefix)}
        self._transaction(lambda db: self._forget(db, keys))
        return len(keys)

    def prune(self) -> int:
        """Deletes the probes of headers that no longer exist (a removed tree or worktree, a deleted header) and the
        bodies no probe holds any more. The number of probes deleted."""
        keys = {key for (key,) in self.db().execute("SELECT key FROM probe")
                if not os.path.exists(key.split("#", 1)[0])}
        self._transaction(lambda db: self._forget(db, keys))
        return len(keys)


class Layouts:
    defer = False  # True: ensure() leaves saving to save() (Index.save_layouts; one write per process)

    def __init__(self, index: "Index"):
        self.index = index
        self.cache = {"version": LAYOUT_VERSION, "headers": LayoutStore()}
        self.records: dict[str, Record] = {}
        self.vtables: dict[str, dict[tuple, list[dict]]] = {}
        self.loaded: set[str] = set()
        self.record_header: dict[str, str] = {}  # record name -> the probed header that first laid it out
        self.fresh: set[str] = set()  # headers this process probed: save() appends them to the journal
        self._sha: dict[str, tuple[float, str]] = {}  # path -> (mtime, sha1) read in this process
        # probes another process made (parallel prefetch workers): key -> (probe arguments, the entry, when the
        # probe began and ended, time.monotonic()). ensure()/instance() take one instead of probing when it was probed
        # with the arguments they would use and no tree write of this process overlapped it, and it is current: every
        # write ended before it began, or it is a successful probe whose dependencies are unchanged (_valid, the
        # cache's own test). It is then the entry their own probe makes
        self.ahead: dict[str, tuple[tuple, dict, float, float]] = {}
        self.writes: list[tuple[float, float]] = []  # (start, end) of this process's tree writes
        self._writing: float | None = None
        # when a list: (key, arguments, entry, start, end) of each probe this process makes
        self.log: list[tuple[str, tuple, dict, float, float]] | None = None

    def offer(self, key: str, args: tuple, entry: dict, start: float, end: float) -> None:
        """A probe another process ran (its Layouts.log); the latest one per key."""
        known = self.ahead.get(key)
        if known is None or known[2] < start:
            self.ahead[key] = (args, entry, start, end)

    def begin_write(self) -> None:
        """A tree file is about to be written (forget_ahead follows)."""
        self._writing = time.monotonic()

    def forget_ahead(self) -> None:
        """The tree changed (a header written, after begin_write): the probes begun before hold only while their
        dependencies do; one that overlapped the write is void."""
        now = time.monotonic()
        self.writes.append((now if self._writing is None else self._writing, now))
        self._writing = None

    def _take_ahead(self, key: str, args: tuple) -> dict | None:
        """The offered entry of key when it was probed with these arguments and is current, else None."""
        offered = self.ahead.pop(key, None)
        if offered is None or offered[0] != args:
            return None
        _, entry, start, end = offered
        if any(s <= end and e >= start for s, e in self.writes):
            return None
        if all(e < start for _, e in self.writes):
            return entry
        complete = entry.get("ok") and set(entry.get("sha") or ()) == set(entry["deps"])
        return entry if complete and self._valid(entry) else None

    def _probed(self, key: str, args: tuple, entry: dict, start: float) -> None:
        if self.log is not None:
            self.log.append((key, args, entry, start, time.monotonic()))

    def _polymorphic(self, header: str) -> list[str]:
        """The classes of the header whose vtables a probe dumps."""
        scan = self.index.scans.get(header)
        if not scan:
            return []
        text = open(header, encoding="utf-8", errors="replace").read()
        names = []
        for c in scan.classes:
            # a namespaced class is probed by its qualified name (msvc9::runtime_error; HdrV1 2026-11-05); one
            # in a nested anonymous namespace cannot be named from the probe TU
            if c.template or (c.namespace and "" in c.namespace.split("::")):
                continue
            body = text[c.open:c.close]
            if c.bases or re.search(r"\bvirtual\b|\boverride\b", body):
                names.append(f"{c.namespace}::{c.name}" if c.namespace else c.name)
        return names

    def _valid(self, entry: dict) -> bool:
        """Every dependency has the recorded mtime, or (entries with hashes) the recorded content: a file rewritten
        unchanged (a copy, a merge) keeps the entry, whose mtimes this process then brings up to date in memory."""
        shas = entry.get("sha")
        touched = {}
        for path, mtime in entry["deps"].items():
            try:
                now = os.path.getmtime(path)
            except OSError:
                return False
            if now == mtime:
                continue
            if not shas or path not in shas:
                return False
            known = self._sha.get(path)
            if known is None or known[0] != now:
                known = self._sha[path] = (now, _file_sha(path))
            if known[1] != shas[path]:
                return False
            touched[path] = now
        if touched:
            entry["deps"] = dict(entry["deps"], **touched)
        return True

    def _absorb(self, header: str, entry: dict) -> None:
        self.loaded.add(header)
        if not entry.get("ok"):
            return
        canon = entry["canonical"]
        same = len(canon) == len(entry["records"]) and all(len(a[2]) == len(b[2]) for a, b in zip(canon,
                                                                                                    entry["records"]))
        for k, (name, size, entries) in enumerate(entry["records"]):
            canonical = canon[k][2] if same else [(o, b, d, t) for o, b, d, t in entries]
            merged = [(o, b, d, t, canonical[j][3]) for j, (o, b, d, t) in enumerate(entries)]
            if name not in self.records or len(merged) > len(self.records[name].entries):
                record = Record(name, size, merged)
                chain, depth = [name], 0
                for o, b, d, t, ct in merged:
                    m = re.match(r"(?:class|struct|union) (.+) \(primary base\)", t)
                    if m and d == depth + 1 and o == 0:
                        chain.append(plain_type(m.group(1)))
                        depth = d
                record.primary_chain = chain
                self.records[name] = record
                # the probe key: a header, or `HEADER#INSTANCE` for an instance probe (whose vtable it dumped)
                self.record_header.setdefault(name, header)
        if "#" in header:
            # an instance probe's tables name the template without its arguments ('NiTMap'): filed under that name
            # every instance would overwrite the others' (types differ per instance); src_vtable reads them from
            # the probe's entry
            return
        for path, entries in entry.get("vtables", {}).items():
            owner = path[-1]
            self.vtables.setdefault(owner, {})[path] = entries

    def get(self, name: str) -> Record | None:
        """The class's record, its template arguments spaced as src and IDA write them or as clang does
        ('A<B,C>' and 'A<B, C>' are one key)."""
        found = self.records.get(name)
        if found is None and "," in name:
            if getattr(self, "_unspaced_count", -1) != len(self.records):
                self._unspaced = {re.sub(r"\s*,\s*", ",", n): r for n, r in self.records.items() if "," in n}
                self._unspaced_count = len(self.records)
            found = self._unspaced.get(re.sub(r"\s*,\s*", ",", name))
        return found

    def instance(self, header: str, cls: str, includes: tuple[str, ...] = ()) -> dict:
        """Lay out a class template instance and its vtable (ensure's probes skip templates, and a header's probe
        lays an instance out only where the header names it): a TU of the header and INCLUDES (the headers of the
        template arguments) instantiates it. Cached under `HEADER#INSTANCE` like a header's probe."""
        key = f"{header}#{cls}"
        if key in self.loaded:
            return self.cache["headers"][key]
        entry = self.cache["headers"].get(key)
        if not (entry and self._valid(entry)):
            args = (header, [cls], includes, cls)
            entry = self._take_ahead(key, args)
            if entry is None:
                start = time.monotonic()
                entry = _run_probe(*args)
                self._probed(key, args, entry, start)
            self.cache["headers"][key] = entry
            self.fresh.add(key)
            if not self.defer:
                self.save()
        self._absorb(key, entry)
        return entry

    def ensure(self, headers: list[str], jobs: int | None = None) -> None:
        """Probe every header not yet loaded (in parallel), from the cache where its dependencies are unchanged."""
        pending = []
        for header in dict.fromkeys(headers):
            if header in self.loaded or not header:
                continue
            entry = self.cache["headers"].get(header)
            if entry and self._valid(entry):
                self._absorb(header, entry)
            else:
                pending.append(header)
        if not pending:
            return
        # the offered entries (offer) first; the rest probed here, all absorbed in the headers' order
        work = [(h, self._polymorphic(h)) for h in pending]
        taken = {h: entry for h, classes in work if (entry := self._take_ahead(h, (h, classes))) is not None}
        work = [w for w in work if w[0] not in taken]
        probed = {}
        if work:
            start = time.monotonic()
            # SRCINDEX_JOBS: parallel probe compiles (default 2; the machine is shared)
            workers = jobs or int(os.environ.get("SRCINDEX_JOBS", "2"))
            with concurrent.futures.ThreadPoolExecutor(max_workers=workers) as pool:
                probed = dict(zip((h for h, _ in work), pool.map(lambda w: _run_probe(*w), work)))
            for header, classes in work:
                self._probed(header, (header, classes), probed[header], start)
        for header in pending:
            entry = taken[header] if header in taken else probed[header]
            self.cache["headers"][header] = entry
            self._absorb(header, entry)
            self.fresh.add(header)
        if not self.defer:
            self.save()

    def save(self) -> None:
        """Writes this process's probes to the layout database."""
        if not self.fresh:
            return
        self.cache["headers"].save(sorted(self.fresh))
        self.fresh.clear()


# ---------------------------------------------------------------------------------------------------------------
# PDB signatures (S_GPROC32 type strings, inline marks, this constness)
# ---------------------------------------------------------------------------------------------------------------

def _pdb_signatures() -> dict[str, list[dict]]:
    import names
    symbols = names.dump("symbols")
    types = names.dump("types")
    stamp = (PDB_VERSION, os.path.getmtime(symbols), os.path.getmtime(types))
    cached = _load_cache("pdb")
    if cached and cached.get("stamp") == stamp:
        return cached["functions"]
    print("srcindex: indexing PDB signatures (once)", file=sys.stderr)
    # the symbol dump truncates long type strings: each function type is rebuilt from the type records
    mfunction: dict[int, dict] = {}
    named: dict[int, str] = {}
    pointers: dict[int, tuple[int, str]] = {}
    modifiers: dict[int, tuple[int, bool]] = {}
    arglists: dict[int, list[str]] = {}
    simple: dict[int, str] = {}
    record = re.compile(r"^\s*(0x[0-9A-F]+) \| (LF_\w+) \[size = \d+\](?: `(.*)`)?")
    returned = re.compile(r"return type = (0x[0-9A-F]+)(?: \((.*?)\))?, # args = \d+, param list = (0x[0-9A-F]+)")
    with open(types, encoding="utf-8", errors="replace") as handle:
        current, kind = None, None
        for line in handle:
            m = record.match(line)
            if m:
                current, kind = int(m.group(1), 16), m.group(2)
                if kind in ("LF_CLASS", "LF_STRUCTURE", "LF_UNION", "LF_ENUM") and m.group(3):
                    named[current] = m.group(3)
                elif kind == "LF_ARGLIST":
                    arglists[current] = []
                continue
            if kind in ("LF_MFUNCTION", "LF_PROCEDURE"):
                if (r := returned.search(line)):
                    ret = int(r.group(1), 16)
                    if r.group(2):
                        simple[ret] = r.group(2)
                    mfunction[current] = {"ret": ret, "args": int(r.group(3), 16), "this": 0, "conv": "",
                                          "options": "", "member": kind == "LF_MFUNCTION"}
                elif (t := re.search(r"this type = (0x[0-9A-F]+|<no type>)", line)) and current in mfunction:
                    mfunction[current]["this"] = int(t.group(1), 16) if t.group(1).startswith("0x") else 0
                elif (c := re.search(r"calling conv = (\w+), options = (.*)$", line)) and current in mfunction:
                    mfunction[current]["conv"] = c.group(1)
                    mfunction[current]["options"] = c.group(2).strip()
            elif kind == "LF_POINTER":
                if (r := re.search(r"referent = (0x[0-9A-F]+)(?: \((.*?)\))?, mode = (\w+)", line)):
                    pointers[current] = (int(r.group(1), 16), r.group(3))
                    if r.group(2):
                        simple[int(r.group(1), 16)] = r.group(2)
            elif kind == "LF_MODIFIER":
                if (r := re.search(r"referent = (0x[0-9A-F]+)(?: \((.*?)\))?, modifiers = (.*)$", line)):
                    modifiers[current] = (int(r.group(1), 16), "const" in r.group(3))
                    if r.group(2):
                        simple[int(r.group(1), 16)] = r.group(2)
            elif kind == "LF_ARGLIST":
                if (a := re.match(r"^\s*(0x[0-9A-F]+)(?: \((.*?)\))?: `(.*)`\s*$", line)):
                    arglists[current].append(a.group(3))

    def type_name(index: int, depth: int = 0) -> str:
        if depth > 20:
            return "?"
        if index in named:
            return named[index]
        if index in pointers:
            referent, mode = pointers[index]
            return type_name(referent, depth + 1) + ("&" if mode in ("ref", "lvalue") else
                                                     "&&" if mode == "rvalue" else "*")
        if index in modifiers:
            referent, const = modifiers[index]
            return ("const " if const else "") + type_name(referent, depth + 1)
        if index in simple:
            return simple[index]
        return "?"
    functions: dict[str, list[dict]] = {}
    proc = re.compile(r"^\s*\d+ \| S_[GL]PROC32 \[size = \d+\] `(.*)`$")
    with open(symbols, encoding="utf-8", errors="replace") as handle:
        name, entry = None, None
        for line in handle:
            m = proc.match(line)
            if m:
                name = m.group(1)
                entry = {"type": "", "type_index": 0, "inline": False, "code_size": 0}
                functions.setdefault(name, []).append(entry)
                continue
            if entry is None:
                continue
            if (t := re.search(r"type = `(0x[0-9A-F]+) \((.*)\)`", line)):
                entry["type_index"] = int(t.group(1), 16)
                entry["type"] = t.group(2)
            elif (s := re.search(r"code size = (\d+)", line)):
                entry["code_size"] = int(s.group(1))
            elif "S_FRAMEPROC" in line:
                pass
            elif line.lstrip().startswith("flags = ") and "S_" not in line:
                entry["inline"] = "marked inline" in line
                entry = None
    for entries in functions.values():
        for entry in entries:
            info = mfunction.get(entry["type_index"])
            if info:
                this = info["this"]
                entry["member"] = info["member"]
                entry["static"] = info["member"] and this == 0
                referent = pointers.get(this, (-1, ""))[0]
                entry["const"] = modifiers.get(referent, (0, False))[1]
                entry["options"] = info["options"]
                entry["ret"] = type_name(info["ret"])
                entry["params"] = [p for p in arglists.get(info["args"], []) if p != "void"]
            else:
                entry["member"] = False
                entry["static"] = False
                entry["const"] = False
                entry["options"] = ""
                entry["ret"], _, entry["params"] = split_pdb_type(entry["type"])
    _save_cache("pdb", {"stamp": stamp, "functions": functions})
    return functions


def split_pdb_type(signature: str) -> tuple[str, str, list[str]]:
    """'NiShader* NiGeometry::(int, float)' -> ('NiShader*', 'NiGeometry', ['int', 'float'])."""
    # the parameter list is the last top-level parenthesis group
    if not signature.endswith(")"):
        return "", "", []
    depth, open_ = 0, -1
    for k in range(len(signature) - 1, -1, -1):
        depth += signature[k] == ")"
        depth -= signature[k] == "("
        if depth == 0:
            open_ = k
            break
    if open_ < 0:
        return "", "", []
    params = signature[open_ + 1:-1]
    left = signature[:open_].rstrip()
    owner = ""
    if left.endswith("::"):
        left = left[:-2]
        # the owner: the trailing qualified name, template arguments matched backwards
        k, depth = len(left), 0
        while k > 0:
            c = left[k - 1]
            if c == ">":
                depth += 1
            elif c == "<":
                depth -= 1
            elif depth == 0 and not (c.isalnum() or c in "_:"):
                break
            k -= 1
        owner, left = left[k:], left[:k]
    params_list = [p for p in _split_top(params) if p and p != "void"]
    return left.strip(), owner, params_list


# ---------------------------------------------------------------------------------------------------------------
# PDB name spelling (IDA names vs the PDB's)
# ---------------------------------------------------------------------------------------------------------------

_CONVENTION = re.compile(r"\b__(cdecl|thiscall|stdcall|fastcall|clrcall|saveregs)\b")
_DELETING = ("`vector deleting destructor'", "`vector deleting dtor'", "`scalar deleting dtor'")


def name_key(name: str) -> str:
    """Spelling-insensitive key of a function name: no class-key words ('BSSimpleArray<class X,1024>'), no spaces
    ('NiPointer<A> >'), one spelling of the deleting destructor (the PC's vector, the PDB's scalar)."""
    name = re.sub(r"\b(class|struct|union|enum) ", "", name)
    for spelled in _DELETING:
        name = name.replace(spelled, "`scalar deleting destructor'")
    return name.replace(" ", "")


def qualified_of_signature(signature: str) -> str:
    """'public: virtual void __thiscall A<B>::f(int) const' -> 'A<B>::f'; a plain name comes back unchanged."""
    text = re.sub(r"^(public|private|protected): ", "", signature)
    text = re.sub(r"^(virtual|static) ", "", text)
    body = text[:-6] if text.endswith(" const") else text
    if not body.endswith(")"):
        return text
    depth, open_ = 0, -1
    for k in range(len(body) - 1, -1, -1):
        depth += body[k] == ")"
        depth -= body[k] == "("
        if depth == 0:
            open_ = k
            break
    if open_ < 0:
        return text
    left = body[:open_]
    found = list(_CONVENTION.finditer(left))
    if found:
        return left[found[-1].end():].strip()
    depth = 0
    for k in range(len(left) - 1, -1, -1):
        depth += left[k] == ">"
        depth -= left[k] == "<"
        if depth == 0 and left[k] == " ":
            return left[k + 1:]
    return left


_UNDNAME: dict[str, str | None] = {}


def undname(symbols: list[str]) -> list[str | None]:
    """llvm-undname over MSVC symbols (IDA's `_N` duplicate suffix dropped); None where it fails. Cached."""
    todo = [s for s in dict.fromkeys(symbols) if s not in _UNDNAME]
    if todo:
        text = "\n".join(re.sub(r"_\d+$", "", s) for s in todo) + "\n"
        out = subprocess.run(["llvm-undname"], input=text, capture_output=True, text=True).stdout
        blocks = out.strip("\n").split("\n\n")
        for k, symbol in enumerate(todo):
            lines = blocks[k].split("\n") if k < len(blocks) else []
            _UNDNAME[symbol] = lines[1] if len(lines) > 1 and not lines[1].startswith("error") else None
    return [_UNDNAME[s] for s in symbols]


def qualified_of_ida(name: str) -> str | None:
    """An IDA function name as a qualified name: `??` symbols demangled (constructors, destructors and deleting
    destructors of symbols llvm-undname rejects read from the prefix), signatures cut to the name. None when a `??`
    symbol cannot be read."""
    if name.startswith("??"):
        text = undname([name])[0]
        if text:
            return qualified_of_signature(text)
        match = re.match(r"^\?\?(0|1|_E|_G)([A-Za-z_]\w*)@@", name)
        if not match:
            return None
        c = match.group(2)
        return {"0": f"{c}::{c}", "1": f"{c}::~{c}", "_E": f"{c}::`vector deleting destructor'",
                "_G": f"{c}::`vector deleting destructor'"}[match.group(1)]
    return qualified_of_signature(name) if "(" in name else name


# ---------------------------------------------------------------------------------------------------------------
# exe vtables
# ---------------------------------------------------------------------------------------------------------------

def _exe_vtables() -> dict:
    import fn
    fn.refresh_functions()
    stamp = (EXE_VERSION, os.path.getmtime(fn.EXE), os.path.getmtime(fn.NAMES))
    cached = _load_cache("exe")
    if cached and cached.get("stamp") == stamp:
        return cached
    print("srcindex: reading the exe vtables (once)", file=sys.stderr)
    vtc = _vtable_check()
    exe = vtc.Exe()
    exe.locator("NiRefObject") if exe.has_locator("NiRefObject") else None
    tables: dict[tuple[str, int], tuple[int, list[int]]] = {}
    for descriptor in list(exe.locators):
        try:
            name = exe.type_name(descriptor)
            tables[(name, 0)] = (exe.vtable_address(name), exe.vtable(name))
            for offset in exe.secondary.get(descriptor, {}):
                tables[(name, offset)] = (exe.vtable_address(name, offset), exe.vtable(name, offset))
        except (SystemExit, KeyError, StopIteration, UnicodeDecodeError, ValueError, struct.error):
            continue
    for name, address in exe.named_vtables.items():
        if (name, 0) not in tables:
            try:
                tables[(name, 0)] = (address, exe.vtable(name))
            except (SystemExit, KeyError):
                pass
    parents = {}
    for name, offset in tables:
        if offset == 0:
            try:
                parents[name] = exe.first_base(name)
            except (SystemExit, KeyError):
                parents[name] = None
    thunks = {}
    for (name, offset), (_, entries) in tables.items():
        for entry in entries:
            if entry not in thunks:
                adjust, target = exe.thunk(entry)
                if target != entry:
                    thunks[entry] = (adjust, target)
    result = {"stamp": stamp, "tables": tables, "parents": parents, "thunks": thunks}
    _save_cache("exe", result)
    return result


def _scopes(scope: str) -> list[str]:
    """'A::B' -> ['A::B', 'A']: the class scopes a name spelled inside B is looked up in."""
    parts = scope.split("::") if scope else []
    return ["::".join(parts[:k]) for k in range(len(parts), 0, -1)]


# ---------------------------------------------------------------------------------------------------------------
# the index
# ---------------------------------------------------------------------------------------------------------------

class Index:
    def __init__(self):
        self.scans = scan_all()
        self.refs: dict[int, list[Ref]] = {}
        self.by_symbol: dict[str, list[Ref]] = {}
        self.class_defs: dict[str, list[ClassDef]] = {}
        self.decls: dict[str, list[Decl]] = {}
        self.definitions: dict[str, list[Definition]] = {}
        self.enums: dict[str, str] = {}
        self.enumerators: dict[str, tuple[bool, dict[int, str]]] = {}
        self.free_decls: dict[str, list[Decl]] = {}  # 'ns::name' / 'name' -> namespace-scope declarations
        for path, scan in self.scans.items():
            for ref in scan.refs:
                self.refs.setdefault(ref.address, []).append(ref)
                self.by_symbol.setdefault(ref.symbol, []).append(ref)
            for c in scan.classes:
                self.class_defs.setdefault(c.name, []).append(c)
            for d in scan.decls:
                self.decls.setdefault(d.cls, []).append(d)
            for d in scan.definitions:
                self.definitions.setdefault(d.qualified, []).append(d)
            self.enums.update(scan.enums)
            self.enumerators.update(scan.enumerators)
            for d in scan.free_decls:
                self.free_decls.setdefault((d.cls + "::" if d.cls else "") + d.name, []).append(d)
        # a registration that names its class by a type alias is filed under the aliased type too, so a caller's
        # demangled name (`DFALL<NiTMapItem<__int64,int>>::Allocate`) finds it
        for scan in self.scans.values():
            for ref in scan.refs:
                canonical = self._alias_resolved(ref.symbol, scan.aliases)
                if canonical != ref.symbol:
                    self.by_symbol.setdefault(canonical, []).append(ref)
        self._layouts: Layouts | None = None
        self._pdb = None
        self._names = None
        self._exe = None
        self._slots = None

    def enumerator(self, spelled: str, value: int, scope: str = "") -> str | None:
        """The enumerator of the enum type (as spelled in `scope`, a class) with the value, qualified so it reads
        anywhere ('Class::NAME', 'Enum::NAME' for a scoped enum, 'NAME' at file scope); None when unknown."""
        name = re.sub(r"^(?:const\s+)?(?:enum\s+)?", "", spelled.strip()).strip()
        candidates = [s + "::" + name for s in _scopes(scope)] + [name]
        enum = next((c for c in candidates if c in self.enumerators), None)
        if enum is None:
            found = [k for k in self.enumerators if k.endswith("::" + name)]
            if len(found) != 1:
                return None
            enum = found[0]
        scoped, values = self.enumerators[enum]
        item = values.get(value & 0xFFFFFFFF)
        if item is None:
            return None
        outer = enum if scoped else enum.rpartition("::")[0]
        return f"{outer}::{item}" if outer else item

    # --- src symbols ---------------------------------------------------------------------------------------
    def _best(self, address: int) -> Ref | None:
        refs = self.refs.get(address)
        if not refs:
            return None
        order = {"body": 0, "constructor": 0, "destructor": 0, "global": 1, "static_member": 1,
                 "original_function": 2, "forwarder": 3, "address": 4}
        return min(refs, key=lambda r: (order.get(r.kind, 9), r.local))

    def symbol(self, address: int) -> str | None:
        """The src name of the exe address: its registered body, else its exe global / static member / function
        pointer, else the src function that forwards to it."""
        best = self._best(address)
        return best.symbol if best else None

    def kind(self, address: int) -> str | None:
        """'body' | 'constructor' | 'destructor' | 'global' | 'static_member' | 'original_function' | 'forwarder'."""
        best = self._best(address)
        return best.kind if best else None

    def refs_of(self, address: int) -> list[Ref]:
        return list(self.refs.get(address, []))

    def _alias_resolved(self, symbol: str, aliases: dict[str, str]) -> str:
        """The symbol with a leading type alias (`Alias::f`) replaced by the type it names: the alias of the ref's own
        file, else the one target every file that defines the alias gives it. Class names are never aliases."""
        for _ in range(4):
            scope, sep, rest = symbol.partition("::")
            if not sep or not re.fullmatch(r"\w+", scope) or scope in self.class_defs:
                return symbol
            target = aliases.get(scope)
            if target is None:
                target = self.alias_target(scope)
                if target is None:
                    return symbol
            symbol = target + "::" + rest
        return symbol

    def alias_target(self, name: str) -> str | None:
        """The type a file-scope alias names, when every file that defines the alias gives it the same one (spelled
        with or without namespace qualifiers: `msvc9::vector<uint16_t, msvc9::allocator<uint16_t>>` in one file,
        `vector<uint16_t, allocator<uint16_t>>` inside namespace msvc9 in another); None for a class name or no
        such alias."""
        if name in self.class_defs:
            return None
        if getattr(self, "_all_aliases", None) is None:
            self._all_aliases: dict[str, set[str]] = {}
            for scan in self.scans.values():
                for alias, aliased in scan.aliases.items():
                    self._all_aliases.setdefault(alias, set()).add(aliased)
        found = self._all_aliases.get(name, set())
        if len(found) > 1:
            unqualified = {re.sub(rf"(?<![\w:])(?:{'|'.join(sorted(self.namespaces()))})::", "", t) for t in found}
            if len(unqualified) == 1:
                return max(found, key=len)
        return next(iter(found)) if len(found) == 1 else None

    def namespaces(self) -> set[str]:
        """msvc9, std and the namespaces src's classes are declared in."""
        if getattr(self, "_namespaces", None) is None:
            self._namespaces = {"msvc9", "std"} | {part for defs in self.class_defs.values() for c in defs
                                                   for part in c.namespace.split("::") if part}
        return self._namespaces

    def spelling_key(self, name: str) -> str:
        """A spelling of a (template instance's) name that src and the demangler give alike: type aliases expanded
        (`msvc9::string`, `CIdvSplineCacheTree::_Node`), namespaces (msvc9, std, src's class namespaces) and
        `class`/`struct`/`union`/`enum`/`const` dropped, `false`/`true` as `0`/`1` (the demangler's template
        arguments), spaces dropped. Fixed-width integer spellings are left as they are."""
        self.namespaces()
        spaces = "|".join(sorted(self._namespaces))

        def expand(text: str) -> str:
            # an alias names a type at the start of a qualified name or right after a namespace (`msvc9::string`),
            # never as a member (`X::string`) or a function (`string(`)
            out, at = [], 0
            for m in re.finditer(r"\b[A-Za-z_]\w*\b(?!\s*\()", text):
                before = text[:m.start()]
                if before.endswith("::"):
                    scope = re.search(r"(?:^|[^\w:])(\w+)::$", before)
                    if not scope or scope.group(1) not in self._namespaces:
                        continue
                target = self.alias_target(m.group(0))
                if target is not None:
                    out.append(text[at:m.start()] + target)
                    at = m.end()
            return "".join(out) + text[at:]
        for _ in range(6):
            expanded = expand(name)
            if expanded == name:
                break
            name = expanded
        name = re.sub(r"\b(?:class|struct|union|enum|const)\b", " ", name)
        name = re.sub(r"\bfalse\b", "0", re.sub(r"\btrue\b", "1", name))
        name = re.sub(rf"(?<![\w:])(?:{spaces})::", "", name)
        return re.sub(r"\s+", "", name)

    def registered(self) -> set[int]:
        return {a for a, refs in self.refs.items() if any(r.kind in ("body", "constructor", "destructor")
                                                          for r in refs)}

    def address_of(self, symbol: str) -> list[int]:
        """Exe addresses a src name stands for (registrations, forwarders, globals). A conversion operator matches
        whatever its spacing and scalar spelling (`Setting::operator unsigned int` = `Setting::operatoruint32_t`)."""
        refs = self.by_symbol.get(symbol, [])
        if not refs and "::operator" in symbol:
            if getattr(self, "_operator_refs", None) is None:
                self._operator_refs: dict[str, list[Ref]] = {}
                for name, found in self.by_symbol.items():
                    key = self._operator_key(name)
                    if key:
                        self._operator_refs.setdefault(key, []).extend(found)
            refs = self._operator_refs.get(self._operator_key(symbol) or "", [])
        return sorted({r.address for r in refs})

    def _operator_key(self, symbol: str) -> str | None:
        cls, operator, rest = symbol.partition("::operator")
        if not operator or not rest.strip() or not re.fullmatch(r"[\w\s]+", rest):
            return None
        return cls + "::operator " + self._canonical_scalar(rest.strip())

    # --- classes -------------------------------------------------------------------------------------------
    def _class_key(self, cls: str, table: dict) -> str:
        """The key a class is filed under: the name itself, else for a template instance ('BSSimpleArray<Actor*>',
        'NiTMap<A,B>::Entry') the template's name with the arguments stripped, as record() falls back to. The VC9
        library model (src/SDK/MSVC, namespace msvc9; IDA and the PDB spell it std::) is filed under its bare names,
        without `msvc9::`; a `std::` name is taken as its msvc9:: model (msvc9_spelling)."""
        for prefix in ("msvc9::", "std::"):
            if cls.startswith(prefix) and cls not in table:
                cls = cls[len(prefix):]
        if cls not in table and "::" in cls:
            # a class declared in a namespace is filed under its name inside it (`bhkWorldNS::ObjectRecData` under
            # `ObjectRecData`, `OEI::IO` under `IO`); MSVC's anonymous namespace (`?A0x1234abcd::FAMThread`) too
            cls = re.sub(r"^`?(?:anonymous namespace'|\?A0x[0-9a-fA-F]+)::", "", cls)
            head = cls.split("<", 1)[0]
            parts = head.split("::")
            for k in range(1, len(parts)):
                namespace, rest = "::".join(parts[:k]), cls[len("::".join(parts[:k])) + 2:]
                if rest in table and any(c.namespace == namespace for c in self.class_defs.get(rest, ())):
                    cls = rest
                    break
        if cls in table or "<" not in cls:
            return cls
        import names as pdbnames
        return pdbnames.strip_templates(cls)

    def header(self, cls: str) -> str | None:
        """Repo-relative path of the src header that defines the class (None if none or several); a template
        instance resolves to its template's header. A name no class carries resolves to the one header declaring
        functions in that namespace (`bhkUtilFunctions`). Memoised: the index is built once and not changed."""
        cache = self.__dict__.setdefault("_header_cache", {})
        if cls in cache:
            return cache[cls]
        key = self._class_key(cls, self.class_defs)
        found = [c for c in self.class_defs.get(key, []) if c.path.endswith((".h", ".hpp", ".inl"))]
        paths = sorted({c.path for c in found})
        if not self.class_defs.get(key):
            paths = sorted({d.path for decls in self.free_decls.values() for d in decls
                            if d.cls == key and d.path.endswith((".h", ".hpp", ".inl"))})
        cache[cls] = paths[0] if len(paths) == 1 else None
        return cache[cls]

    def class_def(self, cls: str) -> ClassDef | None:
        cls = self._class_key(cls, self.class_defs)
        found = [c for c in self.class_defs.get(cls, []) if c.path.endswith((".h", ".hpp", ".inl"))]
        return found[0] if len(found) == 1 else None

    def declarations(self, cls: str, name: str | None = None) -> list[Decl]:
        cls = self._class_key(cls, self.decls)
        return [d for d in self.decls.get(cls, []) if name is None or d.name == name]

    def free_declarations(self, qualified: str) -> list[Decl]:
        """Namespace-scope declarations and inline definitions of the function ('ns::name' or 'name'); one per file
        (a header's declaration and its .cpp's definition both appear)."""
        return list(self.free_decls.get(qualified, []))

    def definitions_of(self, qualified: str) -> list[Definition]:
        return list(self.definitions.get(qualified, []))

    @property
    def layouts(self) -> Layouts:
        if self._layouts is None:
            self._layouts = Layouts(self)
        return self._layouts

    def save_layouts(self) -> None:
        """Writes the layout probes not saved yet (Layouts.defer)."""
        if self._layouts is not None:
            self._layouts.save()

    def prefetch(self, classes: list[str]) -> None:
        """Probe the headers of many classes at once (parallel); fields()/src_slots() then need no compile."""
        headers = [os.path.join(ROOT, h) for c in classes if (h := self.header(c))]
        self.layouts.ensure(headers)

    def includes_of(self, header: str) -> set[str] | None:
        """Repo-relative files a header pulls in (itself included), from its layout probe; None when unprobed."""
        path = os.path.join(ROOT, header)
        self.layouts.ensure([path])
        entry = self.layouts.cache["headers"].get(path)
        if not entry:
            return None
        return {os.path.relpath(p, ROOT) for p in entry["deps"]}

    def record(self, cls: str) -> Record | None:
        """The class's clang record layout. Names are normalised like Field.type ('NiTPointerList<NiNode*>'); a
        template instance is found once a probe that uses it ran: the probe of the header defining the template,
        else of the headers whose text names the instance (a class with such a member). Before that second step a
        caller's answer depended on which other probes the process had run (0x408DF0 found
        BSSimpleList<EffectSetting*> only after another row's probe had laid it out)."""
        layouts = self.layouts
        cls = _tidy_type(cls)
        if layouts.get(cls) is None:
            header = self.header(cls) or self.header(re.sub(r"<.*", "", cls))
            if header:
                layouts.ensure([os.path.join(ROOT, header)])
        if layouts.get(cls) is None and "<" in cls:
            # the headers naming the instance (text only, so the list survives fieldgen's layout refresh), probed
            # one at a time until one lays it out: a common instance is named in hundreds of headers
            users = self.__dict__.setdefault("_instance_users", {})
            if cls not in users:
                spelled = re.compile(r"\s*".join(re.escape(part) for part in re.findall(r"\w+|[^\w\s]", cls)))
                found = []
                for path in self.scans:
                    if not path.endswith((".h", ".hpp", ".inl")):
                        continue
                    with open(path, encoding="utf-8", errors="replace") as handle:
                        if spelled.search(handle.read()):
                            found.append(path)
                users[cls] = found
            for path in users[cls]:
                layouts.ensure([path])
                if layouts.get(cls) is not None:
                    break
        if layouts.get(cls) is None and "<" in cls:
            entry = self.probe_instance(cls)
            if layouts.get(cls) is None and entry and entry.get("ok") and cls.endswith(">"):
                # clang names the instance without its default arguments (`NiTArray<TESObjectCELL*>` for
                # `NiTArray<TESObjectCELL *, NiTMallocInterface<TESObjectCELL *>>`, `BSSimpleArray<T>` for
                # `BSSimpleArray<T, 1024>`): the probe's record of the template whose arguments lead cls's
                base = re.sub(r"<.*", "", cls)
                want = [re.sub(r"\s+", "", a) for a in _split_top(cls[len(base) + 1:-1])]
                for name, _, _ in entry.get("records") or []:
                    if not name.startswith(base + "<") or not name.endswith(">"):
                        continue
                    have = [re.sub(r"\s+", "", a) for a in _split_top(name[len(base) + 1:-1])]
                    if have and want[:len(have)] == have and layouts.get(name) is not None:
                        layouts.records[_tidy_type(cls)] = layouts.get(name)
                        break
        return layouts.get(cls)

    def probe_instance(self, cls: str) -> dict | None:
        """Lay out a class template instance no header names (Layouts.instance): the template's header, plus the
        headers of the classes its arguments name (`NiTMap<WaterZone *, int>`, used only in .cpp files). The probe's
        entry; None unless src defines the template as a class template (src's `SettingCollection` is a plain class
        where the exe has `SettingCollection<Setting>`)."""
        base = re.sub(r"<.*", "", cls)
        header = self.header(base)
        definition = self.class_def(base)
        if not header or definition is None or not definition.template:
            return None
        named = set(re.findall(r"[A-Za-z_]\w*(?:::[A-Za-z_]\w*)*", cls[len(base):])) - {base}
        includes = tuple(sorted({os.path.join(ROOT, h) for n in named if (h := self.header(n)) and h != header}))
        return self.layouts.instance(os.path.join(ROOT, header), cls, includes)

    def _type_size(self, canonical: str, written: str = "") -> tuple[int | None, str]:
        t = canonical.strip()
        t = re.sub(r"^(?:const|volatile)\s+|\s+(?:const|volatile)$", "", t).strip()
        if re.search(r"\[(\d+)\]$", t):
            dims = [int(x) for x in re.findall(r"\[(\d+)\]", t)]
            element, _ = self._type_size(re.sub(r"(\[\d+\])+$", "", t))
            if element is None:
                return None, "array"
            total = element
            for d in dims:
                total *= d
            return total, "array"
        if t.endswith(("*", "&")) or "(*)" in t or re.search(r"\*\s*(?:const|volatile)?$", t):
            return 4, "ptr"
        if t in PRIMITIVES:
            return PRIMITIVES[t]
        m = re.match(r"^enum (.+)$", t)
        if m:
            name = m.group(1)
            underlying = self.enums.get(name)
            if underlying is None:
                underlying = next((v for k, v in self.enums.items() if k.split("::")[-1] == name.split("::")[-1]),
                                  "")
            if underlying:
                size, kind = self._type_size(self._canonical_scalar(underlying))
                return size, "enum" if kind in ("int", "uint") else kind
            return 4, "enum"
        m = re.match(r"^(?:class|struct|union) (.+)$", t)
        if m:
            name = _tidy_type(m.group(1))
            record = self.layouts.get(name)
            return (record.size if record else None), "record"
        return None, "other"

    @staticmethod
    def _canonical_scalar(name: str) -> str:
        table = {"uint8_t": "unsigned char", "int8_t": "signed char", "uint16_t": "unsigned short",
                 "int16_t": "short", "uint32_t": "unsigned int", "int32_t": "int", "uint64_t": "unsigned long long",
                 "int64_t": "long long", "UInt8": "unsigned char", "UInt16": "unsigned short",
                 "UInt32": "unsigned int", "SInt32": "int", "BYTE": "unsigned char", "WORD": "unsigned short",
                 "DWORD": "unsigned long", "char": "char", "short": "short", "int": "int"}
        name = " ".join(name.split())
        return table.get(name, name)

    def all_fields(self, cls: str) -> list[Field]:
        """Every field of the class's layout, nested members and array elements unexpanded, in layout order:
        Field.name is the access path from the class."""
        record = self.record(cls)
        if not record:
            return []
        out = []
        # stack of (depth, kind, name, owner)
        stack: list[tuple[int, str, str, str]] = [(0, "base", "", cls)]
        for offset, bits, depth, text, canon in record.entries:
            while stack and stack[-1][0] >= depth:
                stack.pop()
            parent = stack[-1] if stack else (0, "base", "", cls)
            base = re.match(r"(?:class|struct|union) (.+) \((?:primary |virtual )?base\)(?: \(empty\))?$", text)
            if base:
                stack.append((depth, "base", "", plain_type(base.group(1))))
                continue
            if text.startswith("(") and text.endswith(")"):
                continue  # vfptr / vbptr
            m = re.match(r"^(.*?)\s*([A-Za-z_]\w*)$", text)
            anonymous = "(anonymous" in text or "(unnamed" in text
            name = "" if anonymous or not m else m.group(2)
            written = text if anonymous or not m else m.group(1)
            cm = re.match(r"^(.*?)\s*([A-Za-z_]\w*)$", canon)
            canonical = cm.group(1) if cm and not anonymous else canon
            path_names = [s[2] for s in stack if s[1] == "field" and s[2]]
            path = ".".join(path_names + ([name] if name else []))
            size, kind = self._type_size(canonical, written)
            if kind in ("int", "uint") and canonical.strip().startswith("bool"):
                kind = "bool"
            out.append(Field(path, _tidy_type(written), size, offset, canonical.strip(), kind, bits, parent[3]))
            stack.append((depth, "field", name, parent[3]))
        return out

    def fields(self, cls: str) -> dict[int, Field]:
        """offset -> the outermost field there (the class's and its bases' direct members); nested members and
        array elements: field_at()."""
        found: dict[int, Field] = {}
        for f in self.all_fields(cls):
            if "." in f.name or f.bits:
                continue
            found.setdefault(f.offset, f)
        return found

    def field_at(self, cls: str, offset: int, size: int | None = None, kind: str | None = None) -> list[Field]:
        """Fields covering exactly [offset, offset+size): outermost first, then nested members, then array elements
        (name 'm_a[2]', 'm_a[1][2]'). kind filters ('float', 'ptr', 'int'/'uint'/'bool'/'enum' as 'integer')."""
        out = []
        for f in self.all_fields(cls):
            if f.bits:
                continue
            if f.kind == "array" and f.size and f.offset <= offset < f.offset + f.size:
                if f.offset == offset:
                    out.append(f)  # the whole array, then its element
                element = re.sub(r"(\[\d+\])+$", "", f.canonical)
                dims = [int(x) for x in re.findall(r"\[(\d+)\]", f.canonical)]
                esize, ekind = self._type_size(element)
                if not esize:
                    continue
                index = (offset - f.offset) // esize
                if (offset - f.offset) % esize:
                    continue
                subscript = ""
                rest = index
                strides = []
                for k in range(len(dims)):
                    stride = 1
                    for d in dims[k + 1:]:
                        stride *= d
                    strides.append(stride)
                for stride in strides:
                    subscript += f"[{rest // stride}]"
                    rest %= stride
                written = re.sub(r"(\[\d+\])+$", "", f.type)
                out.append(Field(f"{f.name}{subscript}", _tidy_type(written), esize, offset, element, ekind, None,
                                 f.owner))
                continue
            if f.offset == offset:
                out.append(f)
        if size is not None:
            out = [f for f in out if f.size == size]
        if kind == "integer":
            out = [f for f in out if f.kind in ("int", "uint", "bool", "enum")]
        elif kind:
            out = [f for f in out if f.kind == kind]
        return out

    def src_vtable(self, cls: str, offset: int = 0) -> list[dict] | None:
        """The class's src vtable (clang's layout) for the subobject at the offset: per slot owner, name, ret, params,
        const."""
        record = self.record(cls)
        if not record:
            return None
        tables = self.layouts.vtables.get(cls) or self.layouts.vtables.get(record.name, {})
        if not tables:
            # the record may have come from another header's probe, which dumps only that header's own classes'
            # vtables: probe the class's own header
            header = self.header(cls)
            if header:
                self.layouts.ensure([os.path.join(ROOT, header)])
                tables = self.layouts.vtables.get(cls) or self.layouts.vtables.get(record.name, {})
        if not tables and "<" in record.name:
            # a template instance: a header's probe dumps no template's vtable; the instance probe (the header that
            # laid out its record, else the template's and its arguments' headers) dumps this instance's alone,
            # under the template's bare name ('NiTMapBase', 'NiTMap')
            key = self.layouts.record_header.get(record.name, "")
            if "#" in key:
                entry = self.layouts.cache["headers"].get(key) or {}
            elif key:
                entry = self.layouts.instance(key, record.name)
            else:
                entry = self.probe_instance(record.name) or {}
            import names as pdbnames
            owners = {pdbnames.strip_templates(record.name), re.sub(r"\s*,\s*", ",", record.name)}
            tables = {p: e for p, e in (entry.get("vtables") or {}).items()
                      if p[-1] in owners or re.sub(r"\s*,\s*", ",", p[-1]) in owners}
        # vtable dumps name a template base without its arguments ('BSTask'), the record chain with them
        import names as pdbnames
        chain = set(record.primary_chain) | {pdbnames.strip_templates(c) for c in record.primary_chain}
        for path, entries in tables.items():
            if offset == 0 and set(path) <= chain:
                return entries
        if offset:
            vtc = _vtable_check()
            bases = []
            for o, bits, depth, text, canon in record.entries:
                m = re.match(r"(?:class|struct|union) (.+) \(((?:primary |virtual )?base)\)", text)
                if m:
                    bases.append((o, depth, plain_type(m.group(1)), m.group(2)))
            for path, entries in tables.items():
                if set(path) <= chain:
                    continue
                offsets = vtc.base_offsets(bases, list(path[::-1]))
                if offsets and offsets[-1] == offset:
                    return entries
        return None

    # --- exe vtables ---------------------------------------------------------------------------------------
    @property
    def exe_vtables(self) -> dict:
        if self._exe is None:
            self._exe = _exe_vtables()
        return self._exe

    def _slot_map(self) -> dict[int, list[tuple[str, int, int]]]:
        if self._slots is None:
            slots: dict[int, list[tuple[str, int, int]]] = {}
            for (name, offset), (_, entries) in self.exe_vtables["tables"].items():
                for k, entry in enumerate(entries):
                    slots.setdefault(entry, []).append((name, offset, k))
            self._slots = slots
        return self._slots

    def slots(self, address: int) -> list[tuple[str, int]]:
        """(class, slot) of every primary exe vtable entry holding the address."""
        return sorted((c, s) for c, o, s in self._slot_map().get(address, []) if o == 0)

    def all_slots(self, address: int) -> list[tuple[str, int, int]]:
        """(class, subobject offset, slot) of every exe vtable entry (primary and secondary) holding the address."""
        return sorted(self._slot_map().get(address, []))

    def exe_vtable(self, cls: str, offset: int = 0) -> list[int] | None:
        table = self.exe_vtables["tables"].get((cls, offset))
        return table[1] if table else None

    def exe_parent(self, cls: str) -> str | None:
        return self.exe_vtables["parents"].get(cls)

    def thunk(self, address: int) -> tuple[int, int] | None:
        """(this adjustment, target) when the vtable entry is an adjustor thunk."""
        return self.exe_vtables["thunks"].get(address)

    # --- PDB -----------------------------------------------------------------------------------------------
    @property
    def names(self):
        """names.py's PDB index. names.py pickles it as a script (classes under __main__): unpickle them as names.*."""
        if self._names is None:
            import names

            class Unpickler(pickle.Unpickler):
                def find_class(self, module, name):
                    if module == "__main__" and hasattr(names, name):
                        return getattr(names, name)
                    return super().find_class(module, name)

            index = None
            if os.path.exists(names.INDEX):
                with open(names.INDEX, "rb") as handle:
                    index = Unpickler(handle).load()
            if index is None or index.version != names.INDEX_VERSION:
                index = names.load()
            self._names = index
        return self._names

    @property
    def pdb_signatures(self) -> dict[str, list[dict]]:
        if self._pdb is None:
            self._pdb = _pdb_signatures()
        return self._pdb

    def pdb_func(self, qualified: str) -> dict | None:
        """The PDB's function: {'name', 'params': [[names]...], 'sources': [...], 'signatures': [{'ret', 'owner',
        'params': [types], 'type', 'inline', 'static', 'const', 'code_size'}], 'virtual', 'static', 'access'} or None.
        The exact name first; else, when exactly one PDB function fits (never an ambiguous match), the name as IDA
        spells it (`??` symbols, signatures, 'class ' in template arguments, spaces, the vector deleting destructor;
        name_key), then a template member named without its template arguments ('BSSimpleList::AddHead'). 'name' is
        the PDB's spelling."""
        info = self._pdb_func_named(qualified)
        if info is None:
            name = self.pdb_name_of(qualified)
            info = self._pdb_func_named(name) if name and name != qualified else None
        return info

    def pdb_name_of(self, qualified: str) -> str | None:
        """The one PDB function name the name stands for (exact, IDA-spelled, or template-stripped), else None."""
        import names as pdbnames
        if not hasattr(self, "_name_keys"):
            index = self.names
            known = set(index.functions) | set(self.pdb_signatures)
            for cname, model in index.classes.items():
                known.update(f"{cname}::{m.name}" for m in model.methods)
            self._pdb_known = known
            self._stripped: dict[str, set[str]] = {}
            self._name_keys: dict[str, set[str]] = {}
            for k in known:
                self._name_keys.setdefault(name_key(k), set()).add(k)
                if "<" in k:
                    self._stripped.setdefault(name_key(pdbnames.strip_templates(k)), set()).add(k)
        if qualified in self._pdb_known:
            return qualified
        spelled = qualified_of_ida(qualified)
        if not spelled:
            return None
        # the same name spelled differently, then a template member named without its template arguments (a name
        # with other template arguments is another function: not matched)
        tables = [self._name_keys] + ([self._stripped] if "<" not in spelled else [])
        for table in tables:
            found = table.get(name_key(spelled))
            if found:
                return next(iter(found)) if len(found) == 1 else None
        return None

    def _pdb_func_named(self, name: str) -> dict | None:
        import names as pdbnames
        index = self.names
        found = index.functions.get(name)
        sigs = self.pdb_signatures.get(name, [])
        cls, _, short = name.rpartition("::")
        model = index.classes.get(cls) or index.classes.get(pdbnames.strip_templates(cls)) if cls else None
        methods = [m for m in model.methods if m.name == short] if model else []
        if not found and not sigs and not methods:
            return None
        signatures = []
        seen = set()
        for s in sigs:
            key = (s["type"], s["inline"])
            if key in seen:
                continue
            seen.add(key)
            ret, params = s.get("ret", ""), s.get("params", [])
            signatures.append({"ret": ret, "owner": cls, "params": params, "type": s["type"],
                               "inline": s["inline"], "static": s.get("static", False), "const": s.get("const", False),
                               "code_size": s["code_size"], "member": s.get("member", False)})
        return {
            "name": name,
            "params": [p for p, _ in (found or [])],
            "sources": sorted({src for _, src in (found or []) if src}),
            "signatures": signatures,
            "virtual": any(m.virtual for m in methods),
            "static": any(m.static for m in methods),
            "access": methods[0].access if methods else "",
            "overloads": len(methods),
            "class_known": model is not None,
        }

    def pdb_class(self, cls: str):
        """The PDB class model: the exact name, else the one class the name stands for as IDA spells it
        ('NiTMap<class A *,int>'; name_key); never an ambiguous match."""
        classes = self.names.classes
        found = classes.get(cls)
        if found is None:
            if not hasattr(self, "_class_keys"):
                self._class_keys: dict[str, set[str]] = {}
                for name in classes:
                    self._class_keys.setdefault(name_key(name), set()).add(name)
            names = self._class_keys.get(name_key(cls), set())
            found = classes[next(iter(names))] if len(names) == 1 else None
        return found

    def pdb_source_file(self, qualified: str) -> str | None:
        """The PDB source file of the function (the class's own .cpp when an inline function appears in several)."""
        info = self.pdb_func(qualified)
        if not info or not info["sources"]:
            return None
        import names as pdbnames
        return pdbnames.source_of(self.names, info["name"])

    def map_source(self, pdb_path: str) -> str | None:
        """A PDB source path ('Fallout Shared\\TESForm.cpp') as a repo-relative src path, matched case-insensitively
        against the files src has; None if src has no such file."""
        if not pdb_path:
            return None
        wanted = pdb_path.replace("\\", "/").lower()
        lookup = self._src_lookup()
        if wanted in lookup:
            return lookup[wanted]
        # Gamebryo paths in the PDB may sit under another root: match the longest suffix
        parts = wanted.split("/")
        for k in range(1, len(parts)):
            suffix = "/".join(parts[k:])
            hits = [v for key, v in lookup.items() if key.endswith("/" + suffix) or key == suffix]
            if len(hits) == 1:
                return hits[0]
            if hits:
                return None
        return None

    def _src_lookup(self) -> dict[str, str]:
        if not hasattr(self, "_lookup"):
            self._lookup = {os.path.relpath(p, SRC).lower(): rel(p) for p in self.scans}
        return self._lookup

    def source_path(self, qualified: str) -> str | None:
        """Repo-relative src path of the PDB source file of the function, if src has that file."""
        return self.map_source(self.pdb_source_file(qualified) or "")


_INDEX: Index | None = None


def load() -> Index:
    global _INDEX
    if _INDEX is None:
        _INDEX = Index()
    return _INDEX


def main() -> int:
    index = load()
    for token in sys.argv[1:]:
        if re.fullmatch(r"(0x)?[0-9a-fA-F]{6,8}", token):
            address = int(token, 16)
            print(f"{address:#x}: {index.kind(address)} {index.symbol(address)}")
            for ref in index.refs_of(address):
                print(f"    {ref.kind:17} {ref.form:16} {ref.symbol}  {ref.path}:{ref.line}")
            for cls, offset, slot in index.all_slots(address):
                print(f"    slot {cls}+{offset:#x}[{slot}]")
        elif "::" in token and index.pdb_func(token):
            info = index.pdb_func(token)
            print(json.dumps(info, indent=1))
            print("source_path:", index.source_path(token))
        else:
            print("header:", index.header(token))
            for offset, f in sorted(index.fields(token).items()):
                print(f"    {offset:#06x} {f.size!s:>4} {f.kind:6} {f.type} {f.name}")
            table = index.src_vtable(token)
            for k, entry in enumerate(table or []):
                print(f"    [{k}] {entry['text']}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
