#!/usr/bin/env python3
"""Spell src/'s built-in integer types by their fixed-width names (.omp/RULES.md "Types from the PDB").

Usage: env/venv/bin/python harness/tools/fixedwidth.py [--dry-run] [--report DIR] [PATH...]
  PATH        files or folders (default: src/ without src/SDK/, and gen/); .h .hpp .inl .cpp .c files under a folder.
              src/SDK/ is skipped even when named.
  --dry-run   change no file; only write the reports
  --report D  write kept.tsv, long.tsv and overlong.tsv into D (default: no reports)

The text is tokenized (comments, string and character literals with their prefixes, raw strings, pp-numbers,
preprocessor directives), so a word in a comment or a literal is never touched. Only code is changed: ordinary lines
and the bodies of #define; the other directives (#include, #if, #pragma, #error...) are left as they are.

A run of type keywords (signed, unsigned, short, int, long, char, __int8..__int64, double) separated by white space
only is one spelling, mapped by the table below. Idempotent: a converted file has nothing left to convert.

Left as they are, with a row in kept.tsv:
  char, wchar_t, bool, float, double, long double     not integers of a fixed width (counted, one row per reason)
  __int8                                              MSVC's synonym of char, not of signed char (int8_t)
  int main / WinMain / wWinMain                       the C++ and Windows entry points fix `int`
  operator++(int), operator--(int)                    the postfix form the standard fixes
  int f(const void*, const void*)                     the qsort/bsearch comparator form the CRT fixes
  a run with const/volatile inside it                 converted by hand
Left as they are, with a row in long.tsv (never scripted): long, long int, signed long, unsigned long: on Windows a
type distinct from int (overloads, template arguments, mangled names), on Linux 64 bits.
overlong.tsv lists the lines the change takes past 120 columns (tab width 4).
"""

import argparse
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
EXTENSIONS = (".h", ".hpp", ".inl", ".cpp", ".c")

KEYWORDS = {"signed", "unsigned", "short", "int", "long", "char", "double", "__int8", "__int16", "__int32",
            "__int64"}
CV = {"const", "volatile"}

# The rule's table: a sorted keyword multiset -> the fixed-width name.
TABLE = {}


def _add(spellings, name):
    for s in spellings:
        TABLE[tuple(sorted(s.split()))] = name


_add(["int", "signed", "signed int", "__int32", "signed __int32"], "int32_t")
_add(["unsigned", "unsigned int", "unsigned __int32"], "uint32_t")
_add(["short", "short int", "signed short", "signed short int", "__int16", "signed __int16"], "int16_t")
_add(["unsigned short", "unsigned short int", "unsigned __int16"], "uint16_t")
_add(["unsigned char", "unsigned __int8"], "uint8_t")
_add(["signed char", "signed __int8"], "int8_t")
_add(["long long", "long long int", "signed long long", "signed long long int", "__int64", "signed __int64"],
     "int64_t")
_add(["unsigned long long", "unsigned long long int", "unsigned __int64"], "uint64_t")

LONGS = {tuple(sorted(s.split())) for s in
         ["long", "long int", "signed long", "signed long int", "unsigned long", "unsigned long int"]}
# Kept by type, counted per reason.
KEPT_TYPES = {("char",): "char (text)", ("double",): "double", ("double", "long"): "long double",
              ("__int8",): "__int8 (MSVC's char, not signed char)"}
# Counted only: types the rule keeps as they are.
OTHER_TYPES = {"bool", "wchar_t", "float"}

STRING_PREFIXES = {"L", "u", "U", "u8", "R", "LR", "uR", "UR", "u8R"}
ENTRY_POINTS = {"main", "WinMain", "wWinMain"}
CALLING = re.compile(r"^(__cdecl|__stdcall|__fastcall|__thiscall|WINAPI|APIENTRY|CALLBACK|FNVD_\w*CALL\w*)$")

IDENT = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")
PPNUMBER = re.compile(r"\.?[0-9](?:[eEpP][+-]|'[0-9A-Za-z_]|[0-9A-Za-z_.])*")
WS = re.compile(r"[ \t\f\v]+|\\\n")


class Tok:
    __slots__ = ("kind", "start", "end", "text", "code")

    def __init__(self, kind, start, end, text, code):
        self.kind, self.start, self.end, self.text, self.code = kind, start, end, text, code


def _quoted(text, i, quote):
    """End of a string or character literal whose opening quote is at i."""
    j = i + 1
    n = len(text)
    while j < n:
        c = text[j]
        if c == "\\":
            j += 2
            continue
        if c == quote or c == "\n":
            return j + 1
        j += 1
    return n


def tokenize(text):
    """Every token of the text; `code` is False inside directives other than #define/#undef."""
    toks = []
    i, n = 0, len(text)
    line_start = True       # only white space since the last newline (outside a directive)
    directive = None        # None, or True/False: the current directive's tokens are code
    expect_name = False     # the token after `#` names the directive
    while i < n:
        c = text[i]
        if c == "\n":
            toks.append(Tok("nl", i, i + 1, c, True))
            i += 1
            line_start = True
            directive = None
            continue
        m = WS.match(text, i)
        if m:
            toks.append(Tok("ws", i, m.end(), m.group(), True))
            i = m.end()
            continue
        code = directive is not False
        if text.startswith("//", i):
            j = i
            while True:
                k = text.find("\n", j)
                if k < 0:
                    k = n
                    break
                if text[k - 1] == "\\":
                    j = k + 1
                    continue
                break
            toks.append(Tok("comment", i, k, text[i:k], False))
            i = k
            continue
        if text.startswith("/*", i):
            k = text.find("*/", i + 2)
            k = n if k < 0 else k + 2
            toks.append(Tok("comment", i, k, text[i:k], False))
            i = k
            continue
        if c == "#" and line_start and directive is None:
            toks.append(Tok("punct", i, i + 1, c, False))
            i += 1
            line_start = False
            directive = False
            expect_name = True
            continue
        line_start = False
        m = IDENT.match(text, i)
        if m:
            word = m.group()
            j = m.end()
            if word in STRING_PREFIXES and j < n and text[j] in "\"'":
                if word.endswith("R") and text[j] == '"':
                    k = text.find("(", j)
                    delim = text[j + 1:k]
                    e = text.find(")" + delim + '"', k)
                    e = n if e < 0 else e + len(delim) + 2
                else:
                    e = _quoted(text, j, text[j])
                toks.append(Tok("literal", i, e, text[i:e], False))
                i = e
                continue
            if expect_name:
                directive = word in ("define", "undef")
                expect_name = False
                toks.append(Tok("ident", i, j, word, False))
            else:
                toks.append(Tok("ident", i, j, word, code))
            i = j
            continue
        expect_name = False
        m = PPNUMBER.match(text, i)
        if m:
            toks.append(Tok("number", i, m.end(), m.group(), code))
            i = m.end()
            continue
        if c in "\"'":
            e = _quoted(text, i, c)
            toks.append(Tok("literal", i, e, text[i:e], False))
            i = e
            continue
        for p in ("::", "->", "++", "--"):
            if text.startswith(p, i):
                toks.append(Tok("punct", i, i + 2, p, code))
                i += 2
                break
        else:
            toks.append(Tok("punct", i, i + 1, c, code))
            i += 1
    return toks


def significant(toks, i, step):
    """Index of the next token from i (exclusive) in direction step that is not white space or a comment."""
    i += step
    while 0 <= i < len(toks) and toks[i].kind in ("ws", "nl", "comment"):
        i += step
    return i


def _text(toks, i):
    return toks[i].text if 0 <= i < len(toks) else ""


def context_exception(toks, first, last):
    """The reason an `int` run must stay `int`, or None."""
    nxt = significant(toks, last, 1)
    while CALLING.match(_text(toks, nxt)):
        nxt = significant(toks, nxt, 1)
    prev = significant(toks, first, -1)
    if _text(toks, nxt) in ENTRY_POINTS and _text(toks, significant(toks, nxt, 1)) == "(" and \
            _text(toks, prev) not in ("::", ".", "->"):
        return "entry point: `int %s` is fixed by C++/Windows" % _text(toks, nxt)
    if _text(toks, prev) == "(" and _text(toks, nxt) == ")":
        op = significant(toks, prev, -1)
        if _text(toks, op) in ("++", "--") and _text(toks, significant(toks, op, -1)) == "operator":
            return "postfix operator%s(int): the form the standard fixes" % _text(toks, op)
    # The qsort/bsearch comparator form: int [cc] [(][*][name][)] (const void*, const void*).
    words = []
    j = last
    for _ in range(24):
        j = significant(toks, j, 1)
        if j >= len(toks):
            break
        words.append(toks[j].text)
        if toks[j].text in (";", "{"):
            break
    joined = " ".join(words)
    if re.match(r"^(\( )?((__cdecl|__stdcall|FNVD_\w+) )?(\* )?((\w+ :: )*\w+ )?(\) )?"
                r"\( (const void|void const) \* (\w+ )?, (const void|void const) \* (\w+ )?\)", joined):
        return "qsort/bsearch comparator form int(const void*, const void*): the CRT fixes it"
    return None


def columns(line):
    return len(line.expandtabs(4))


def convert(path, text, rel, kept, longs, overlong, counts):
    toks = tokenize(text)
    edits = []      # (start, end, replacement)
    i, n = 0, len(toks)
    while i < n:
        t = toks[i]
        if t.kind == "ident" and t.code and t.text in OTHER_TYPES:
            counts[("kept", t.text)] = counts.get(("kept", t.text), 0) + 1
        if t.kind != "ident" or not t.code or t.text not in KEYWORDS:
            i += 1
            continue
        run = [i]
        j = i
        cv_inside = False
        while True:
            k = significant(toks, j, 1)
            # Only white space (no comment) between the words of one spelling.
            if any(toks[x].kind == "comment" for x in range(j + 1, min(k, n))):
                break
            if k < n and toks[k].kind == "ident" and toks[k].code and toks[k].text in KEYWORDS:
                run.append(k)
                j = k
                continue
            if k < n and toks[k].text in CV:
                k2 = significant(toks, k, 1)
                if k2 < n and toks[k2].kind == "ident" and toks[k2].text in KEYWORDS:
                    cv_inside = True
                    run.append(k)
                    run.append(k2)
                    j = k2
                    continue
            break
        first, last = run[0], run[-1]
        words = [toks[x].text for x in run]
        key = tuple(sorted(w for w in words if w not in CV))
        spelling = " ".join(words)
        line = text.count("\n", 0, toks[first].start) + 1
        i = last + 1
        if "double" in key:
            counts.setdefault(("kept", KEPT_TYPES.get(key, "double")), 0)
            counts[("kept", KEPT_TYPES.get(key, "double"))] += 1
            continue
        if key in KEPT_TYPES:
            reason = KEPT_TYPES[key]
            counts[("kept", reason)] = counts.get(("kept", reason), 0) + 1
            if key == ("__int8",):
                kept.append((rel, line, spelling, reason))
            continue
        if key in LONGS:
            longs.append((rel, line, spelling))
            counts[("long", spelling)] = counts.get(("long", spelling), 0) + 1
            continue
        if key not in TABLE:
            kept.append((rel, line, spelling, "no fixed-width name for this spelling: by hand"))
            continue
        if cv_inside:
            kept.append((rel, line, spelling, "const/volatile inside the spelling: by hand"))
            continue
        if key == ("int",):
            reason = context_exception(toks, first, last)
            if reason:
                kept.append((rel, line, spelling, reason))
                counts[("kept", reason.split(":")[0])] = counts.get(("kept", reason.split(":")[0]), 0) + 1
                continue
        edits.append((toks[first].start, toks[last].end, TABLE[key]))
        norm = " ".join(words)
        counts[("converted", norm)] = counts.get(("converted", norm), 0) + 1
    if not edits:
        return text
    out = []
    pos = 0
    for s, e, r in edits:
        out.append(text[pos:s])
        out.append(r)
        pos = e
    out.append(text[pos:])
    new = "".join(out)
    old_lines = text.split("\n")
    new_lines = new.split("\n")
    for no, (a, b) in enumerate(zip(old_lines, new_lines), 1):
        if a == b:
            continue
        b = new_lines[no - 1] = realign_continuation(a, b)
        if columns(b) > 120 and columns(b) > columns(a):
            overlong.append((rel, no, columns(a), columns(b)))
    return "\n".join(new_lines)


CONTINUATION = re.compile(r"^(.*\S)( +)\\(\r?)$")


def realign_continuation(old, new):
    """Keeps a macro line's space-aligned `\\` in its column when the spelling changed the text before it."""
    a, b = CONTINUATION.match(old), CONTINUATION.match(new)
    if not a or not b:
        return new
    column = columns(a.group(1) + a.group(2))
    pad = max(1, column - columns(b.group(1)))
    return b.group(1) + " " * pad + "\\" + b.group(3)


def files(paths):
    sdk = os.path.join(ROOT, "src", "SDK") + os.sep
    for p in paths:
        p = os.path.abspath(p)
        if os.path.isdir(p):
            for d, dirs, names in os.walk(p):
                dirs.sort()
                if (d + os.sep).startswith(sdk):
                    dirs[:] = []
                    continue
                for name in sorted(names):
                    if name.endswith(EXTENSIONS):
                        yield os.path.join(d, name)
        elif not p.startswith(sdk):
            yield p


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--report")
    ap.add_argument("paths", nargs="*")
    args = ap.parse_args()
    paths = args.paths or [os.path.join(ROOT, "src"), os.path.join(ROOT, "gen")]
    kept, longs, overlong, counts = [], [], [], {}
    changed = 0
    for path in files(paths):
        rel = os.path.relpath(path, ROOT)
        with open(path, encoding="utf-8", errors="surrogateescape", newline="") as f:
            text = f.read()
        new = convert(path, text, rel, kept, longs, overlong, counts)
        if new != text:
            changed += 1
            if not args.dry_run:
                with open(path, "w", encoding="utf-8", errors="surrogateescape", newline="") as f:
                    f.write(new)
    if args.report:
        os.makedirs(args.report, exist_ok=True)
        with open(os.path.join(args.report, "kept.tsv"), "w") as f:
            f.write("file\tline\tspelling\treason\n")
            for r in kept:
                f.write("%s\t%d\t%s\t%s\n" % r)
            for (kind, what), c in sorted(counts.items()):
                if kind == "kept" and what in set(KEPT_TYPES.values()) | OTHER_TYPES:
                    f.write("(all)\t%d\t%s\tnot an integer of a fixed width: kept by type\n" % (c, what))
        with open(os.path.join(args.report, "long.tsv"), "w") as f:
            f.write("file\tline\tspelling\n")
            for r in longs:
                f.write("%s\t%d\t%s\n" % r)
        with open(os.path.join(args.report, "overlong.tsv"), "w") as f:
            f.write("file\tline\tcolumns before\tcolumns after\n")
            for r in overlong:
                f.write("%s\t%d\t%d\t%d\n" % r)
    for (kind, what), c in sorted(counts.items()):
        print("%s\t%s\t%d" % (kind, what, c))
    print("files changed\t%d" % changed)
    print("kept rows\t%d\nlong rows\t%d\noverlong lines\t%d" % (len(kept), len(longs), len(overlong)))


if __name__ == "__main__":
    main()
