#!/usr/bin/env python3
"""The exe's addresses and 32-bit layouts, as tables exported from src's seam records.

Usage: env/venv/bin/python harness/tools/addresses.py export [--out TSV]     (default harness/addresses.tsv)
       env/venv/bin/python harness/tools/addresses.py layout [--out TSV]     (default harness/layout-x86.tsv)
       env/venv/bin/python harness/tools/addresses.py find NAME|0xADDR...

export   one row per exe address a seam record in src/ and gen/ names, and per row of harness/cut-registrations.tsv
         (the registrations already cut from src): every macro of harness/seam/Reconstruction.hpp that takes an exe
         address (GenSupport.hpp has none). Columns:
           address  0x%X
           kind     function, virtual, virtual via base, member, constructor, destructor, construct, destruct,
                    deleting destruct, usercall, userpurge, kept (FNVD_ADDRESS), vtable, global, static member,
                    import, import as, import stub, original function, data definition, global definition, local
                    data definition, local global definition, function static, function static guard, function
                    static atexit, tu copy
           name     the qualified name: a registration's function (`Class::f`; `Class::f(int, float)` where the
                    record picks an overload by its pointer type), a declaration's or definition's object (with its
                    enclosing namespaces, class or function), a function static `Function::name`
           type     a declaration's type; an overload's pointer type; a function static guard's bit mask
           file, line   where the record is (for a cut row, where it was)
           macro    the record's macro
           origin   src, or cut (harness/cut-registrations.tsv)
         Then, per macro: the lines a plain grep finds (`\\bFNVD_X\\s*\\(` in src/ and gen/), those in comments or a
         #define, the rows, and every use it could not parse (file:line).
layout   one row per FNVD_LAYOUT line: type, member (empty for sizeof; `alignof` for alignof), value (a number, else
         the expression), file, line. A condition of another form is kept whole in `type`, member `(condition)`,
         and listed.
find     the rows of harness/addresses.tsv (exported first when it is missing) for an address, or for a name: exact,
         else the rows whose name ends in `::NAME` or `NAME(`, else those containing it.
"""

from __future__ import annotations

import argparse
import csv
import os
import re
import sys
from collections import Counter, defaultdict

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
TABLE = os.path.join(ROOT, "harness", "addresses.tsv")
LAYOUT_TABLE = os.path.join(ROOT, "harness", "layout-x86.tsv")
CUT = os.path.join(ROOT, "harness", "cut-registrations.tsv")
COLUMNS = ["address", "kind", "name", "type", "file", "line", "macro", "origin"]

# macro -> (kind, argument roles). Roles: a address, f function expression, t type, m member, n declared name,
# b base class, p overload pointer type (optional, variadic), c constructor argument types (variadic), g guard,
# k guard mask, x atexit thunk, _ anything (variadic tail).
MACROS = {
    "FNVD_REPLACES": ("function", "af"),
    "FNVD_REPLACES_OVERLOAD": ("function", "afp"),
    "FNVD_REPLACES_VIRTUAL": ("virtual", "atm"),
    "FNVD_REPLACES_VIRTUAL_OVERLOAD": ("virtual", "atmp"),
    "FNVD_REPLACES_VIRTUAL_VIA": ("virtual via base", "atbm"),
    "FNVD_REPLACES_VIRTUAL_VIA_OVERLOAD": ("virtual via base", "atbmp"),
    "FNVD_REPLACES_MEMBER": ("member", "atmp"),
    "FNVD_REPLACES_CONSTRUCTOR": ("constructor", "atc"),
    "FNVD_REPLACES_DESTRUCTOR": ("destructor", "at"),
    "FNVD_REPLACES_CONSTRUCT": ("construct", "af"),
    "FNVD_REPLACES_CONSTRUCT_OVERLOAD": ("construct", "afp"),
    "FNVD_REPLACES_DESTRUCT": ("destruct", "af"),
    "FNVD_REPLACES_DELETING_DESTRUCT": ("deleting destruct", "af"),
    "FNVD_REPLACES_USERCALL": ("usercall", "af_"),
    "FNVD_REPLACES_USERPURGE": ("userpurge", "af_"),
    "FNVD_ADDRESS": ("kept", "afp"),
    "FNVD_VFTABLE": ("vtable", "na"),
    "FNVD_GLOBAL": ("global", "tna"),
    "FNVD_STATIC_MEMBER": ("static member", "tna"),
    "FNVD_IMPORT": ("import", "tna"),
    "FNVD_IMPORT_AS": ("import as", "tna_"),
    "FNVD_IMPORT_STUB": ("import stub", "tna"),
    "FNVD_ORIGINAL_FUNCTION": ("original function", "tna_"),
    "FNVD_DEFINE_DATA": ("data definition", "tna_"),
    "FNVD_DEFINE_GLOBAL": ("global definition", "tna_"),
    "FNVD_DEFINE_LOCAL_DATA": ("local data definition", "tna_"),
    "FNVD_DEFINE_LOCAL_GLOBAL": ("local global definition", "tna_"),
    "FNVD_FUNCTION_STATIC": ("function static", "tnagkx_"),
    "FNVD_TU_COPY": ("tu copy", "tna"),
}
# Seam macros that take no exe address (not exported; counted for the report).
NO_ADDRESS = ("FNVD_TU_STATIC",)
CALL = re.compile(r"\bFNVD_[A-Z0-9_]+(?=\s*\()")
HEX = re.compile(r"0[xX][0-9a-fA-F]+")
KEYWORDS = {"if", "for", "while", "switch", "catch", "return", "sizeof", "alignof", "decltype", "do", "else",
            "static_assert", "noexcept", "requires", "alignas", "__attribute__", "defined"}


def sources():
    for top in ("src", "gen"):
        for dirpath, dirnames, filenames in os.walk(os.path.join(ROOT, top)):
            dirnames.sort()
            for name in sorted(filenames):
                if name.endswith((".h", ".hpp", ".inl", ".cpp", ".c")):
                    yield os.path.join(dirpath, name)


def blank_noncode(text: str) -> str:
    """The text with comments, string and character literals and preprocessor directives blanked (newlines kept)."""
    out = list(text)
    i, n = 0, len(text)
    line_start = True
    while i < n:
        c = text[i]
        if line_start and c in " \t":
            i += 1
            continue
        if line_start and c == "#":
            # a directive, with its continuation lines
            j = i
            while j < n and text[j] != "\n":
                if text[j] == "\\" and j + 1 < n and text[j + 1] == "\n":
                    j += 2
                    continue
                if text.startswith("/*", j):
                    end = text.find("*/", j + 2)
                    j = n if end < 0 else end + 2
                    continue
                j += 1
            for k in range(i, j):
                if out[k] != "\n":
                    out[k] = " "
            i = j
            continue
        line_start = c == "\n"
        if text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j < 0 else j
            for k in range(i, j):
                out[k] = " "
            i = j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            for k in range(i, j):
                if out[k] != "\n":
                    out[k] = " "
            i = j
        elif c == 'R' and text.startswith('R"', i) and (i == 0 or not (text[i - 1].isalnum() or text[i - 1] == "_")):
            m = re.match(r'R"([^(\s]*)\(', text[i:])
            if not m:
                i += 1
                continue
            end = text.find(")" + m.group(1) + '"', i)
            j = n if end < 0 else end + len(m.group(1)) + 2
            for k in range(i + 2, j - 1):
                if out[k] != "\n":
                    out[k] = " "
            i = j
        elif c in "\"'":
            if c == "'" and i > 0 and text[i - 1].isalnum() and i + 1 < n and text[i + 1].isalnum():
                i += 1  # a digit separator (0x10'0000)
                continue
            j = i + 1
            while j < n and text[j] != c and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            for k in range(i + 1, min(j, n)):
                out[k] = " "
            i = j + 1
        else:
            i += 1
    return "".join(out)


def split_args(text: str, start: int) -> tuple[list[str], int] | None:
    """The arguments of the call whose '(' is at start, split as the preprocessor does (commas outside parentheses),
    and the offset after its ')'."""
    depth, args, cur = 0, [], start + 1
    i = start
    while i < len(text):
        c = text[i]
        if c in "([{":
            depth += 1
        elif c in ")]}":
            depth -= 1
            if depth == 0:
                args.append(text[cur:i])
                return [" ".join(a.split()) for a in args], i + 1
        elif c == "," and depth == 1:
            args.append(text[cur:i])
            cur = i + 1
        i += 1
    return None


class Scopes:
    """The namespaces, classes and functions enclosing each offset of a file's code (brace-matched)."""

    def __init__(self, code: str):
        self.events = []  # (open, close, kind, name)
        stack = []
        last = 0  # where the current statement head starts
        for i, c in enumerate(code):
            if c == "{":
                head = " ".join(code[last:i].split())
                stack.append((i, *self.classify(head)))
                last = i + 1
            elif c == "}":
                if stack:
                    o, kind, name = stack.pop()
                    self.events.append((o, i, kind, name))
                last = i + 1
            elif c == ";":
                last = i + 1
            elif c == ":" and code.startswith(("public:", "private:", "protected:"), max(0, i - 9)):
                last = i + 1
        for o, kind, name in stack:
            self.events.append((o, len(code), kind, name))
        self.events.sort()

    @staticmethod
    def classify(head: str) -> tuple[str, str]:
        head = re.sub(r"^(?:\s*\b(?:public|private|protected)\s*:)+", "", head).strip()
        m = re.match(r"^(?:inline\s+)?namespace\s*([\w:]*)\s*$", head)
        if m:
            return "ns", m.group(1)
        if re.match(r'^extern\s+"', head) or head.startswith("extern"):
            return "other", ""
        h = re.sub(r"^template\s*<.*?>\s*(?=(class|struct|union)\b)", "", head)
        m = re.search(r"\b(class|struct|union)\s+(?:\[\[[^\]]*\]\]\s*)?(?:alignas\s*\([^)]*\)\s*)?"
                      r"(?:FNVD_\w+\s+)?([A-Za-z_][\w:]*)\s*(<.*>)?\s*(?:final\s*)?(?::(?!:)[^()]*)?$", h)
        if m and not re.search(r"\benum\b", h[:m.start()]) and "(" not in h[:m.start()].replace("alignas(", ""):
            return "class", m.group(2)
        if re.search(r"\benum\b", head):
            return "other", ""
        # a function: the name before the first top-level '(' (a constructor's initialiser list after it)
        h = re.sub(r"^template\s*<[^()]*?>\s*", "", head)
        depth = 0
        for k, c in enumerate(h):
            if c == "<":
                depth += 1
            elif c == ">":
                depth = max(0, depth - 1)
            elif c == "(" and depth == 0:
                m = re.search(r"((?:[A-Za-z_][\w]*(?:<[^()]*>)?::)*(?:~?[A-Za-z_]\w*|operator\s*\S+?))\s*$", h[:k])
                if m and m.group(1).split("::")[-1] not in KEYWORDS and not h[:k].rstrip().endswith("="):
                    return "func", m.group(1).replace(" ", "")
                return "other", ""
        return "other", ""

    def enclosing(self, at: int) -> list[tuple[str, str]]:
        return [(kind, name) for o, c, kind, name in self.events if o < at < c]


def qualify(outer: list[tuple[str, str]], name: str) -> str:
    """name with the namespaces, classes and function enclosing it (a qualified name keeps the part it gives)."""
    parts = []
    for kind, n in outer:
        if kind in ("ns", "class", "func") and n:
            parts.append(n)
    prefix = "::".join(parts)
    if not prefix:
        return name
    # the enclosing scopes the name repeats (`Foo::bar` defined inside namespace X: X::Foo::bar)
    for k in range(len(parts)):
        tail = "::".join(parts[k:])
        if name.startswith(tail + "::"):
            return "::".join(parts[:k] + [name]) if k else name
    return prefix + "::" + name


def params_of(pointer_type: str) -> str:
    """`void (X::*)(int, float) const` -> `(int, float) const`: the last top-level parenthesised group and after."""
    depth, start = 0, -1
    for i, c in enumerate(pointer_type):
        if c == "(":
            if depth == 0:
                start = i
            depth += 1
        elif c == ")":
            depth -= 1
    return pointer_type[start:].strip() if start >= 0 else ""


def scope_name(name: str) -> str:
    return re.sub(r"<.*>$", "", name.split("::")[-1]) if "::" in name else re.sub(r"<.*>$", "", name)


def address(text: str) -> int | None:
    text = text.strip()
    return int(text, 16) if HEX.fullmatch(text) else (0 if text == "0" else None)


def unalias(name: str, aliases: dict[str, str]) -> str:
    """name with a leading class alias of the file (`using NiTMapInt64Int = NiTMap<int64_t, int32_t>;`) resolved."""
    head, sep, rest = name.partition("::")
    return aliases[head] + sep + rest if sep and head in aliases else aliases.get(name, name)


def rows_of(macro: str, args: list[str], outer, rel: str, line: int, aliases: dict[str, str]) -> list[dict] | None:
    kind, roles = MACROS[macro]
    named = {}
    for k, role in enumerate(roles):
        if role == "_" or role == "p" or role == "c":
            named[role] = ", ".join(args[k:])
            break
        if k >= len(args):
            return None
        named[role] = args[k]
    row = dict(kind=kind, type="", file=rel, line=line, macro=macro, origin="src")
    params = params_of(named["p"]) if named.get("p") else ""
    owner = unalias(named["t"], aliases) if "t" in named else ""
    if "f" in named:
        name = unalias(named["f"].lstrip("&").strip(), aliases)
        row["name"] = qualify(outer, name) + params
        row["type"] = named.get("p", "")
    elif "m" in named:
        row["name"] = f"{owner}::{named['m']}{params}"
        row["type"] = named.get("p", "")
        if "b" in named:
            row["kind"] = f"virtual via {named['b']}"
    elif kind == "constructor":
        row["name"] = f"{owner}::{scope_name(owner)}({named.get('c', '')})"
    elif kind == "destructor":
        row["name"] = f"{owner}::~{scope_name(owner)}"
    else:
        row["name"] = qualify(outer, named["n"])
        row["type"] = named.get("t", "")
    a = address(named["a"])
    if a is None:
        return None
    out = [dict(row, address=a)]
    if macro == "FNVD_FUNCTION_STATIC":
        guard, atexit = address(named["g"]), address(named["x"])
        if guard is None or atexit is None:
            return None
        out.append(dict(row, address=guard, kind="function static guard", type=named["k"]))
        if atexit:
            out.append(dict(row, address=atexit, kind="function static atexit", type=""))
    return out


def scan(want_layout=False):
    rows, layout, unparsed = [], [], defaultdict(list)
    grep, noncode = Counter(), Counter()
    seam = set(MACROS) | set(NO_ADDRESS) | {"FNVD_LAYOUT"}
    for path in sources():
        rel = os.path.relpath(path, ROOT)
        with open(path, encoding="utf-8", errors="replace") as f:
            text = f.read()
        if "FNVD_" not in text:
            continue
        code = blank_noncode(text)
        for m in re.finditer(r"^.*$", text, re.M):
            for name in set(CALL.findall(m.group(0))):
                if name in seam:
                    grep[name] += 1
        scopes = None
        # the file's class aliases a registration may name (`using X = Template<A, B>;`, a class type: no pointer,
        # reference or function type)
        aliases = {m.group(1): " ".join(m.group(2).split()) for m in
                   re.finditer(r"\busing\s+(\w+)\s*=\s*(?:typename\s+)?([\w:<>, ]+?)\s*;", code)}
        line_starts = [0] + [m.end() for m in re.finditer(r"\n", text)]
        lines_seen = defaultdict(set)
        for m in CALL.finditer(code):
            name = m.group(0)
            if name not in seam:
                continue
            line = bisect_line(line_starts, m.start())
            lines_seen[name].add(line)
            if name in NO_ADDRESS:
                continue
            paren = code.index("(", m.end())
            # split on the blanked code: a comma in a string or comment of an initialiser does not count
            got = split_args(code, paren)
            if name == "FNVD_LAYOUT":
                if want_layout:
                    cond = ", ".join(got[0]) if got else ""
                    row = layout_row(cond, rel, line)
                    if not row:
                        # another condition (a constant's value, a sum of offsets): kept whole, so the table holds
                        # every line
                        row = dict(type=cond, member="(condition)", value="", file=rel, line=line)
                        unparsed[name].append(f"{rel}:{line}: {cond}")
                    layout.append(row)
                continue
            if scopes is None:
                scopes = Scopes(code)
            made = rows_of(name, got[0], scopes.enclosing(m.start()), rel, line, aliases) if got else None
            if made is None:
                unparsed[name].append(f"{rel}:{line}")
            else:
                rows.extend(made)
        # grep lines that hold the macro in a comment, a string or a directive only
        for m in re.finditer(r"^.*$", text, re.M):
            line = bisect_line(line_starts, m.start())
            for name in set(CALL.findall(m.group(0))):
                if name in seam and line not in lines_seen[name]:
                    noncode[name] += 1
    return rows, layout, unparsed, grep, noncode


def bisect_line(starts, offset):
    import bisect
    return bisect.bisect_right(starts, offset)


LAYOUT = re.compile(r"^(sizeof|offsetof|__builtin_offsetof|alignof)\s*(?=\()")


def layout_row(cond: str, rel: str, line: int) -> dict | None:
    """`sizeof(T) == V`, `offsetof(T, m) == V` (or __builtin_offsetof), `alignof(T) == V`; V a number, else kept as
    the expression it is (`0x50 + sizeof(BSSpinLock)`)."""
    cond = cond.strip()
    while cond.startswith("(") and (got := split_args(cond, 0)) and got[1] == len(cond):
        cond = cond[1:-1].strip()  # redundant outer parentheses
    m = LAYOUT.match(cond)
    if not m:
        return None
    got = split_args(cond, m.end())
    if not got:
        return None
    parts, end = got
    rest = cond[end:].strip()
    if not rest.startswith("=="):
        return None
    value = rest[2:].strip()
    if re.fullmatch(r"0[xX][0-9a-fA-F]+|\d+", value):
        value = f"0x{int(value, 0):X}"
    if m.group(1).endswith("offsetof"):
        if len(parts) < 2:
            return None
        typ, member = ", ".join(parts[:-1]), parts[-1]
    else:
        typ, member = ", ".join(parts), ("alignof" if m.group(1) == "alignof" else "")
    return dict(type=typ, member=member, value=value, file=rel, line=line)


def cut_rows() -> list[dict]:
    out = []
    if not os.path.exists(CUT):
        return out
    with open(CUT, newline="") as f:
        for r in csv.DictReader(f, delimiter="\t"):
            macro = r["macro"]
            kind = MACROS.get(macro, ("function", ""))[0]
            out.append(dict(address=int(r["address"], 16), kind=kind, name=r["symbol"], type="", file=r["file"],
                            line=int(r["line"]), macro=macro, origin="cut"))
    return out


def write(path, columns, rows):
    with open(path, "w", newline="") as f:
        w = csv.writer(f, delimiter="\t", lineterminator="\n")
        w.writerow(columns)
        for r in rows:
            w.writerow([f"0x{r[c]:X}" if c == "address" else r[c] for c in columns])


def cmd_export(args):
    rows, _, unparsed, grep, noncode = scan()
    src_rows = len(rows)
    cut = cut_rows()
    rows += cut
    rows.sort(key=lambda r: (r["address"], r["origin"], r["file"], r["line"], r["kind"]))
    write(args.out, COLUMNS, rows)
    print(f"addresses: {len(rows)} rows ({src_rows} from src and gen, {len(cut)} from {os.path.relpath(CUT, ROOT)}), "
          f"{len({r['address'] for r in rows})} addresses -> {os.path.relpath(args.out, ROOT)}")
    per = Counter(r["macro"] for r in rows if r["origin"] == "src" and not r["kind"].startswith("function static "))
    print(f"{'macro':36} {'grep':>6} {'noncode':>7} {'rows':>6} {'unparsed':>8}")
    for name in sorted(set(MACROS) | set(NO_ADDRESS)):
        note = "  (takes no address)" if name in NO_ADDRESS else ""
        bad = len(unparsed.get(name, []))
        mismatch = "" if name in NO_ADDRESS or grep[name] - noncode[name] == per[name] + bad else "  MISMATCH"
        print(f"{name:36} {grep[name]:6} {noncode[name]:7} {per[name]:6} {bad:8}{note}{mismatch}")
    for name, places in sorted(unparsed.items()):
        for p in places[:50]:
            print(f"unparsed {name} {p}")
        if len(places) > 50:
            print(f"unparsed {name}: {len(places) - 50} more")
    return 0


def cmd_layout(args):
    _, layout, unparsed, grep, noncode = scan(want_layout=True)
    write(args.out, ["type", "member", "value", "file", "line"], layout)
    bad = unparsed.get("FNVD_LAYOUT", [])
    print(f"layout: {len(layout)} rows -> {os.path.relpath(args.out, ROOT)}; FNVD_LAYOUT grep {grep['FNVD_LAYOUT']}"
          f" lines, {noncode['FNVD_LAYOUT']} in comments or directives, {len(bad)} unparsed")
    for p in bad[:50]:
        print(f"unparsed FNVD_LAYOUT {p}")
    return 0


def load(path=TABLE) -> list[dict]:
    """The rows of harness/addresses.tsv (address as int, line as int); [] when it does not exist."""
    if not os.path.exists(path):
        return []
    with open(path, newline="") as f:
        rows = list(csv.DictReader(f, delimiter="\t"))
    for r in rows:
        r["address"] = int(r["address"], 16)
        r["line"] = int(r["line"])
    return rows


def plain(name: str) -> str:
    """A row's name without its parameter list."""
    depth = 0
    for i, c in enumerate(name):
        if c == "<":
            depth += 1
        elif c == ">":
            depth -= 1
        elif c == "(" and depth == 0 and not name[:i].endswith("operator"):
            return name[:i]
    return name


def find(rows, query: str) -> list[dict]:
    if HEX.fullmatch(query):
        a = int(query, 16)
        return [r for r in rows if r["address"] == a]
    exact = [r for r in rows if r["name"] == query or plain(r["name"]) == query]
    if exact:
        return exact
    tail = [r for r in rows if plain(r["name"]).endswith("::" + query)]
    return tail or [r for r in rows if query in r["name"]]


def cmd_find(args):
    if not os.path.exists(TABLE):
        cmd_export(argparse.Namespace(out=TABLE))
    rows = load()
    status = 0
    for q in args.query:
        got = find(rows, q)
        print(f"{q}: {len(got)} row(s)" if got else f"{q}: none")
        for r in got:
            extra = f"  [{r['type']}]" if r["type"] else ""
            print(f"  0x{r['address']:X}  {r['kind']:22} {r['name']}{extra}  {r['file']}:{r['line']}"
                  f"{'  (cut)' if r['origin'] == 'cut' else ''}")
        status |= 0 if got else 1
    return status


def main():
    p = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = p.add_subparsers(dest="cmd", required=True)
    e = sub.add_parser("export")
    e.add_argument("--out", default=TABLE)
    l_ = sub.add_parser("layout")
    l_.add_argument("--out", default=LAYOUT_TABLE)
    f = sub.add_parser("find")
    f.add_argument("query", nargs="+")
    args = p.parse_args()
    return {"export": cmd_export, "layout": cmd_layout, "find": cmd_find}[args.cmd](args)


if __name__ == "__main__":
    sys.exit(main())
