#!/usr/bin/env python3
"""Rewrites src's uses of `namespace msvc9` from the table harness/tools/msvc9drop.tsv.

msvc9drop.py [--log TSV] [FILE...]
  No FILE: every tracked .h/.hpp/.inl/.cpp/.c under src/, gen/, harness/seam/ and external/ of the git checkout
  holding the current directory (run it from the worktree's root), except src/SDK/MSVC/ (written by hand). Idempotent:
  a second run changes nothing.
  --log TSV  one row per file: file, then name=count for each edit kind.

The edits:
  - The definition outside src/SDK/MSVC of a class 1 function (`float msvc9::sqrtf(float afX) { ... }`: the exe's
    out-of-line copies of VC9's inline <math.h> functions, now the standard ones) goes, with the comment lines
    directly above it and its FNVD_REPLACES/FNVD_ADDRESS line.
  - `msvc9::NAME` becomes the table's `new` spelling (code and comments alike: a comment that names a moved name gets
    the new one). `{0}`, `{1}` in a spelling are the call's arguments: `msvc9::_alldiv(a, b)` is `(a / b)`, each
    argument parenthesised unless it is a name or a literal. `*msvc9::_errno()` is `errno`, any other
    `msvc9::_errno()` is `(&errno)`.
  - A `namespace msvc9 { ... }` block (game declarations in it) is unwrapped: its opening and closing lines go, and
    the bare standard names in it (vector, allocator, pair, basic_string, char_traits, string) get `std::`.
  - An identifier holding `msvc9` (the emitter's spelled-out type names, msvc9_vector_..., msvc9_time_original) gets
    `std` in its place.
  - `#include "SDK/MSVC/H"` of a header the hand-written side deleted becomes the standard header it modelled
    (HEADERS below); a line that would repeat an include already in the file goes.
A `msvc9::NAME` the table does not know is left as it is and reported, as is every other `msvc9` left in a file.
"""
import os
import re
import subprocess
import sys

TABLE = os.path.join(os.path.dirname(os.path.abspath(__file__)), "msvc9drop.tsv")
EXTS = (".h", ".hpp", ".inl", ".cpp", ".c", ".cc")
DIRS = ("src/", "gen/", "harness/seam/", "external/")
SKIP = ("src/SDK/MSVC/",)

# The SDK/MSVC headers the hand-written side deleted, and the standard headers that replace their includes (empty: the
# line goes).
HEADERS = {
    "ctype.h": ["<cctype>"],
    "float.h": ["<cmath>"],
    "fstream.h": ["<fstream>"],
    "ios.h": ["<ios>"],
    "istream.h": ["<istream>"],
    "map.h": ["<map>"],
    "math.h": ["<cmath>"],
    "new.h": ["<new>"],
    "ostream.h": ["<ostream>"],
    "setlocal.h": [],
    "sstream.h": ["<sstream>"],
    "utility.h": ["<utility>"],
    "vector.h": ["<vector>"],
    "xfunctional.h": ["<functional>"],
    "xlocale.h": ["<locale>"],
    "xlocinfo.h": ["<string>"],
    "xlock.h": [],
    "xlocnum.h": ["<locale>"],
    "xmemory.h": ["<memory>"],
    "xstring.h": ["<string>"],
    "xtree.h": ["<map>"],
    "xutility.h": ["<algorithm>"],
}

NAME_RE = re.compile(r"\bmsvc9\s*::\s*([A-Za-z_]\w*)")
IDENT_RE = re.compile(r"\b(\w*msvc9\w*)\b(?!\s*::)")
INCLUDE_RE = re.compile(r'^([ \t]*#[ \t]*include[ \t]*)"SDK/MSVC/([^"]+)"[^\n]*\n', re.M)
ANY_INCLUDE_RE = re.compile(r'^[ \t]*#[ \t]*include[ \t]*([<"][^>"]+[>"])', re.M)
BLOCK_OPEN_RE = re.compile(r"^[ \t]*namespace[ \t]+msvc9[ \t]*(\n[ \t]*)?\{[^\n]*\n", re.M)
BARE_STD_RE = re.compile(r"(?<![\w:.>])(vector|allocator|pair|basic_string|char_traits)(?=\s*<)|"
                         r"(?<![\w:.>])(string)\b(?!\s*\()")
SIMPLE_ARG_RE = re.compile(r"^(::)?[A-Za-z_]\w*(::[A-Za-z_]\w*)*$|^[\d.]+\w*$")
OPEN, CLOSE = "([{", ")]}"
DEFINITION_RE = re.compile(r"^[A-Za-z_][\w:<>*& ]*?\bmsvc9::([A-Za-z_]\w*)\s*\(", re.M)


def load_table():
    rows = {}
    with open(TABLE, encoding="utf-8") as f:
        head = f.readline().rstrip("\n").split("\t")
        for line in f:
            if not line.strip():
                continue
            row = dict(zip(head, line.rstrip("\n").split("\t")))
            rows[row["name"]] = row
    return rows


def code_mask(s):
    """1 for each character of s that is code, 2 for comment characters, 0 for literal characters."""
    mask = bytearray(len(s))
    i, n = 0, len(s)
    while i < n:
        c = s[i]
        if c == "/" and s.startswith("//", i):
            j = i
            while True:
                j = s.find("\n", j)
                if j < 0:
                    j = n
                elif s[j - 1] == "\\":
                    j += 1
                    continue
                break
            mask[i:j] = b"\2" * (j - i)
            i = j
            continue
        if c == "/" and s.startswith("/*", i):
            j = s.find("*/", i + 2)
            j = n if j < 0 else j + 2
            mask[i:j] = b"\2" * (j - i)
            i = j
            continue
        word_start = i == 0 or not (s[i - 1].isalnum() or s[i - 1] == "_")
        raw = re.match(r'(u8|u|U|L)?R"', s[i:i + 4]) if word_start else None
        if raw:
            q = i + raw.end() - 1
            d = s.index("(", q)
            delim = ")" + s[q + 1:d] + '"'
            j = s.find(delim, d)
            i = n if j < 0 else j + len(delim)
            continue
        if c == '"' or (c == "'" and word_start):
            j = i + 1
            while j < n and s[j] != c and s[j] != "\n":
                j += 2 if s[j] == "\\" else 1
            i = j + 1
            continue
        mask[i] = 1
        i += 1
    return mask


def match_paren(s, mask, i):
    depth = 0
    for j in range(i, len(s)):
        if mask[j] != 1:
            continue
        if s[j] in OPEN:
            depth += 1
        elif s[j] in CLOSE:
            depth -= 1
            if depth == 0:
                return j
    raise ValueError("unbalanced bracket at offset %d" % i)


def split_args(s, mask, a, b):
    """The top-level comma-separated arguments of s[a:b] (inside a call's parentheses), stripped."""
    args, depth, start = [], 0, a
    for j in range(a, b):
        if mask[j] != 1:
            continue
        if s[j] in OPEN:
            depth += 1
        elif s[j] in CLOSE:
            depth -= 1
        elif s[j] == "," and depth == 0:
            args.append(s[start:j].strip())
            start = j + 1
    args.append(s[start:b].strip())
    return args


class Rewriter:
    def __init__(self, table):
        self.table = table
        self.counts = {}
        self.unknown = {}

    def count(self, key, n=1):
        self.counts[key] = self.counts.get(key, 0) + n

    def unwrap_blocks(self, s):
        """Unwraps every `namespace msvc9 { ... }` block, qualifying the bare standard names inside."""
        while True:
            mask = code_mask(s)
            m = None
            for cand in BLOCK_OPEN_RE.finditer(s):
                if mask[cand.start() + len(cand.group(0)) - len(cand.group(0).lstrip())] == 1:
                    m = cand
                    break
            if not m:
                return s
            brace = s.index("{", m.start())
            close = match_paren(s, mask, brace)
            body = s[m.end():close]
            body_mask = mask[m.end():close]
            out, last = [], 0
            for b in BARE_STD_RE.finditer(body):
                if body_mask[b.start()] != 1:
                    continue
                out.append(body[last:b.start()])
                out.append("std::" + b.group(0))
                last = b.end()
                self.count("block:std::" + (b.group(1) or b.group(2)))
            out.append(body[last:])
            body = "".join(out)
            # The closing line: the brace and the comment after it.
            line_start = s.rfind("\n", 0, close) + 1
            line_end = s.find("\n", close)
            line_end = len(s) if line_end < 0 else line_end + 1
            before_close = s[line_start:close]
            tail = s[close + 1:line_end]
            if before_close.strip() == "" and (tail.strip() == "" or tail.strip().startswith("//")):
                body = body[:len(body) - (close - line_start)]
                after = s[line_end:]
            else:
                after = s[close + 1:]
            head = s[:m.start()]
            # A blank line on both sides of a removed line: one of them goes.
            if head.endswith("\n\n") and body.startswith("\n"):
                body = body[1:]
            if body.endswith("\n\n") and after.startswith("\n"):
                after = after[1:]
            if head.endswith("\n\n") and body.strip() == "" and after.startswith("\n"):
                body, after = "", after[1:]
            s = head + body + after
            self.count("block:unwrapped")

    def replace_names(self, s):
        mask = code_mask(s)
        out, last = [], 0
        i = 0
        while True:
            m = NAME_RE.search(s, i)
            if not m:
                break
            i = m.end()
            if mask[m.start()] == 0:
                continue
            name = m.group(1)
            row = self.table.get(name)
            if row is None or row["new"] == "-":
                self.unknown[name] = self.unknown.get(name, 0) + 1
                continue
            new = row["new"]
            start, end = m.start(), m.end()
            in_code = mask[start] == 1
            if name == "_errno" and in_code:
                call = re.match(r"\s*\(\s*\)", s[end:])
                if call:
                    star = re.search(r"\*\s*$", s[last:start])
                    end += call.end()
                    if star and mask[last + star.start()] == 1:
                        start = last + star.start()
                        new = "errno"
                    else:
                        new = "(&errno)"
            elif "{0}" in new and in_code:
                call = re.match(r"\s*\(", s[end:])
                if not call:
                    self.unknown[name] = self.unknown.get(name, 0) + 1
                    continue
                paren = end + call.end() - 1
                close = match_paren(s, mask, paren)
                args = split_args(s, mask, paren + 1, close)
                args = [a if SIMPLE_ARG_RE.match(a) else "(" + a + ")" for a in args]
                new = new.format(*args)
                end = close + 1
                # No outer parentheses where the call is a whole operand already (`= x;`, `(x)`, `, x,`).
                before = s[:start].rstrip()
                after = s[end:].lstrip()
                loose_before = before.endswith(("(", "=", ",", "return")) and not before.endswith(("==", "!=", "<=",
                                                                                                   ">="))
                if loose_before and after[:1] in (")", ";", ",") and new.startswith("(") and new.endswith(")"):
                    new = new[1:-1]
            elif "{0}" in new:
                new = new.split("(", 1)[0] or "msvc9::" + name
            out.append(s[last:start])
            out.append(new)
            last = end
            i = end
            self.count(name)
        out.append(s[last:])
        return "".join(out)

    def replace_identifiers(self, s):
        def sub(m):
            ident = m.group(1)
            if ident == "msvc9" or ident == "msvc9_detail":
                return ident
            self.count("identifier")
            return ident.replace("msvc9", "std")
        return IDENT_RE.sub(sub, s)

    def replace_includes(self, s):
        present = set()
        for m in ANY_INCLUDE_RE.finditer(s):
            inc = m.group(1)
            if not (inc.startswith('"SDK/MSVC/') and inc[len('"SDK/MSVC/'):-1] in HEADERS):
                present.add(inc)

        def sub(m):
            header = m.group(2)
            if header not in HEADERS:
                return m.group(0)
            lines = []
            for std in HEADERS[header]:
                if std in present:
                    continue
                present.add(std)
                lines.append(m.group(1) + std + "\n")
            self.count("include:" + header)
            return "".join(lines)
        return INCLUDE_RE.sub(sub, s)

    def delete_definitions(self, s):
        """Deletes the definitions of class 1 functions, their comment lines and their registrations."""
        while True:
            mask = code_mask(s)
            found = None
            for m in DEFINITION_RE.finditer(s):
                row = self.table.get(m.group(1))
                if not row or row["class"] != "1" or mask[m.start()] != 1:
                    continue
                paren = m.end() - 1
                close = match_paren(s, mask, paren)
                rest = re.match(r"\s*(const\s*)?(noexcept\s*)?\{", s[close + 1:])
                if not rest:
                    continue
                brace = close + rest.end()
                found = (m.start(), match_paren(s, mask, brace), m.group(1))
                break
            if not found:
                return s
            start, end, name = found
            # The comment lines directly above.
            while True:
                prev = s.rfind("\n", 0, max(start - 1, 0)) + 1
                if start == 0 or not s[prev:start].lstrip().startswith("//"):
                    break
                start = prev
            end = s.find("\n", end)
            end = len(s) if end < 0 else end + 1
            # The registrations after it, across blank lines.
            reg = re.compile(r"(?:[ \t]*\n)*[ \t]*FNVD_(?:REPLACES|ADDRESS)\w*\(\s*0x[0-9A-Fa-f]+\s*,\s*msvc9::"
                             + re.escape(name) + r"\b[^\n]*\n")
            while True:
                r = reg.match(s, end)
                if not r:
                    break
                end = r.end()
            # One of the blank lines around it.
            if s[end:end + 1] == "\n" and (start == 0 or s[start - 2:start] == "\n\n"):
                end += 1
            s = s[:start] + s[end:]
            self.count("definition:" + name)

    def process(self, text):
        s = self.unwrap_blocks(text)
        s = self.delete_definitions(s)
        s = self.replace_names(s)
        s = self.replace_identifiers(s)
        s = self.replace_includes(s)
        return s


def all_files():
    root = subprocess.run(["git", "rev-parse", "--show-toplevel"], capture_output=True, text=True,
                          check=True).stdout.strip()
    if os.path.realpath(root) != os.path.realpath(os.getcwd()):
        sys.exit("msvc9drop: run it from the checkout's root (%s)" % root)
    files = subprocess.run(["git", "ls-files", "-z", "--"] + list(DIRS), capture_output=True, text=True,
                           check=True).stdout.split("\0")
    return [f for f in files if f.endswith(EXTS) and not f.startswith(SKIP) and os.path.isfile(f)]


def main():
    args = sys.argv[1:]
    log_path = None
    if args[:1] == ["--log"]:
        log_path = args[1]
        args = args[2:]
    files = args or all_files()
    table = load_table()
    total = Rewriter(table)
    log_rows = []
    left = {}
    changed = 0
    for path in files:
        with open(path, encoding="utf-8", newline="") as f:
            text = f.read()
        if "msvc9" not in text and '"SDK/MSVC/' not in text:
            continue
        rw = Rewriter(table)
        try:
            new = rw.process(text)
        except ValueError as e:
            print("msvc9drop: %s: %s (file left as it is)" % (path, e), file=sys.stderr)
            continue
        if new != text:
            with open(path, "w", encoding="utf-8", newline="") as f:
                f.write(new)
            changed += 1
            log_rows.append([path] + ["%s=%d" % kv for kv in sorted(rw.counts.items())])
        for k, v in rw.counts.items():
            total.count(k, v)
        for k, v in rw.unknown.items():
            total.unknown[k] = total.unknown.get(k, 0) + v
        n = new.count("msvc9")
        if n:
            left[path] = n
    for key in sorted(total.counts, key=lambda k: (-total.counts[k], k)):
        print("%6d  %s" % (total.counts[key], key))
    print("files changed: %d" % changed)
    for name in sorted(total.unknown):
        print("not in the table (left): msvc9::%s x%d" % (name, total.unknown[name]))
    for path in sorted(left):
        print("msvc9 left: %s (%d)" % (path, left[path]))
    if log_path:
        with open(log_path, "w", encoding="utf-8") as f:
            for row in log_rows:
                f.write("\t".join(row) + "\n")


if __name__ == "__main__":
    main()
