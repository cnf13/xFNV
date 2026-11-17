#!/usr/bin/env python3
"""Take the seam's record and definition macros out of src (docs/PLAN.md Part 2, steps 1a and 1b).

Usage: env/venv/bin/python harness/tools/unseam.py GROUP [--exported] [--dry-run] [--out DIR] [--build DIR]
                                                    [--probe TSV] [--jobs N] [PATH...]
  GROUP       records    FNVD_REPLACES*, FNVD_ADDRESS, FNVD_LAYOUT statements deleted (step 1a)
              data       FNVD_GLOBAL, FNVD_STATIC_MEMBER, FNVD_DEFINE_*, FNVD_FUNCTION_STATIC, FNVD_TU_STATIC become
                         the declarations and definitions they expand to; FNVD_TU_COPY lines deleted (step 1b)
              functions  FNVD_VFTABLE and FNVD_ORIGINAL_FUNCTION become their constants; FNVD_IMPORT* listed
              casts      FNVD_DYNAMIC_CAST becomes dynamic_cast, FNVD_THROW a throw; FNVD_DYNAMIC_CAST_VOID listed
              objects    FNVD_CONSTRUCT*, FNVD_DESTRUCT*, FNVD_RECONSTRUCT*, FNVD_DELETE* become placement new,
                         destructor calls and delete; FNVD_DESTRUCT_IN_PLACE* statements deleted. Runs the probe
                         (below) first unless --probe names its table.
              all        the five in that order
              probe      only the probe: writes probe.tsv into --out
              report     no change: the FNVD_ macros left in src/ and gen/ and the fnvd:: helpers used there,
                         counted (macros.tsv, helpers.tsv in --out)
  PATH        files or folders to rewrite (default src/ and gen/). The tables always cover what was given.
  --exported  the address export (harness/tools/addresses.py) has run on this tree: records, data and functions drop
              exe addresses and refuse to run without it
  --dry-run   change no file; write the tables only
  --out DIR   the tables (default env/scratch/Clean/unseam)
  --build DIR the Linux build folder whose compile_commands.json the probe compiles (default build/linux)
  --probe TSV an existing probe table (objects only)
  --jobs N    probe compiles at once (default 8)

Tables (in --out, each rewritten by the run; every line number is the input tree's, before any group ran):
  sites.tsv    group, macro, file, line, action (deleted, rewritten, listed), reason (listed), the use's first line
  listed.tsv   the listed rows only: the uses left for a hand step, with why
  sample.tsv   up to 3 deleted and 3 rewritten rows per macro (a fixed seed), for a reader to check by hand
  notes.tsv    comment blocks on deleted lines that say more than the line itself (why a function is not
               registered, a check's verdict, a layout deviation, a guessed type, a naming note), deleted with it;
               and blocks the rule would delete but a marker keeps: file, line, the entity it sat on, action, text
  unused.tsv   declarations the rewrite leaves with no use in src/ and gen/ (a vtable constant, an _original pointer,
               an RTTI descriptor only the dropped cast arguments named): listed, never deleted

How it reads the source. The text is tokenized (comments, string and character literals with their prefixes, raw
strings, pp-numbers, preprocessor directives). A macro's arguments are split as the preprocessor splits them: at
the commas outside parentheses (template brackets and braces do not protect a comma, as there), so an argument
holding commas, templates or nested parentheses is cut exactly where the compiler cuts it. A use inside a directive
(a #define body) is listed, never rewritten. Where the plain form depends on a type the text does not show, the
compiler decides (the probe); where it depends on the use's context (precedence, statement or value), the
neighbouring tokens do; where neither settles it, the use is listed.

What each macro becomes (the native expansion in harness/seam/Reconstruction.hpp, written plainly):
  FNVD_REPLACES*(...), FNVD_ADDRESS(...), FNVD_LAYOUT(...)   deleted: static_asserts that bind nothing
  FNVD_TU_COPY(...)                                         deleted: static_assert(true)
  FNVD_GLOBAL(T, n, a)                                      extern T n
  FNVD_STATIC_MEMBER(T, n, a)                               static T n
  FNVD_DEFINE_DATA(T, n, a, init...)                        T n{init...}; T n = init for one init of a scalar or
                                                            pointer T; T n for none at namespace scope (zeroed
                                                            anyway), T n{} elsewhere and for a const T
  FNVD_DEFINE_GLOBAL(T, n, a, args...)                      T n(args...); T n = arg for one arg of a scalar or
                                                            pointer T; T n for none
  FNVD_DEFINE_LOCAL_DATA / _LOCAL_GLOBAL                    the same with static
  FNVD_FUNCTION_STATIC(T, n, a, g, m, x, args...)           static T n; = arg for one; (args...) for 2 to 8
  FNVD_TU_STATIC(T, n, init...)                             static T n{init...}; = init as for FNVD_DEFINE_DATA
  FNVD_VFTABLE(n, a)                                        inline constexpr fnvd::Vtable n{} (the address is 0
                                                            natively; fnvd::setVtable stores it: step 1d)
  FNVD_ORIGINAL_FUNCTION(T, n, a, f)                        inline const T n = &f (nullptr without f); for T
                                                            void*: reinterpret_cast<void*>(&f), which is what
                                                            fnvd::detail::OriginalFunction<void*> does
  FNVD_DYNAMIC_CAST(T)(o, from, to)                         dynamic_cast<T*>(o): fnvd::dynamicCast is that cast
                                                            plus a static_assert that it tests something
  FNVD_THROW(o, info)                                       throw o
  FNVD_CONSTRUCT(T)(m, args...) and _AS                     ::new (m) T(args...), in parentheses before a postfix
                                                            operator (fnvd::construct is that placement new; `::`
                                                            because a class operator new would hide the global
                                                            placement form)
  FNVD_DESTRUCT(T)(p) and _AS                               p->~T() (x.~T() for p = &x), when the probe finds p
                                                            to be a T* (fnvd::destruct converts p to T* first: a
                                                            derived or smart pointer is listed)
  FNVD_DELETE(T)(p, f) and _AS                              a statement with f 0, 1 or 3 and p a T*: p->~T(),
                                                            delete p, delete[] p (fnvd::deletingDestruct by flags);
                                                            flags 2, a variable flag or a used result: listed
  FNVD_RECONSTRUCT(T)(p, args...) and _AS                   a statement with p a T* that has no side effect and
                                                            arguments that neither call nor name p's object:
                                                            p->~T(); and ::new (p) T(args) on the next line
                                                            (fnvd::reconstruct evaluates the arguments first)
  FNVD_DESTRUCT_IN_PLACE(T)(p) and _AS                      deleted where it is a statement of its own and p is
                                                            &x for a named object x (a local, a parameter, a
                                                            member of this, or a member of one of these):
                                                            fnvd::destructInPlace does nothing, and C++ destroys
                                                            x anyway. Any other operand (a pointer, an element)
                                                            is listed: nothing else may destroy that object
Listed always: FNVD_DYNAMIC_CAST_VOID (the void* result is used as the exe's: a static_cast of it to a base takes
no adjustment that a typed result would), FNVD_IMPORT, FNVD_IMPORT_STUB, FNVD_IMPORT_AS (dllimport on Windows only,
fnvd::ImportAs; their plain form is the import's prototype, which needs the pointer typedef written out).
A use whose argument ends in a // comment (the closing parenthesis on the next line) is listed.

Comments. An argument's comments go with it, up to the comma or parenthesis around it. A deleted statement takes
with it a trailing // comment on its line and the comment block right above it (no blank line between):
  - above a FNVD_LAYOUT, FNVD_TU_COPY or in-class FNVD_ADDRESS line: always;
  - above a FNVD_REPLACES* or namespace-scope FNVD_ADDRESS line: unless a declaration or definition of the function
    it names follows the deleted lines directly (the comment then describes that function);
  - above any other deleted statement: when every line from there to the next blank line or `}` is deleted too.
A block holding a marker a later step reads (KEEP, SWEEP-INLINE, EXE-INLINED, EXE-CALLED: PLAN step 2 deletes those
after steps 7 and 8) stays, and is written to notes.tsv when the rule would have deleted it. A blank line the
deletion leaves double, or right after an opening brace or access label, or right before a closing brace, goes.

Layout. A data or function declaration the rewrite leaves on one line longer than 120 columns (tab width 4) is
wrapped: after its `=` (and before the declared name when the declarator alone is that long), else between the
arguments of its initialiser, with the use's own continuation indent (else one more level of the line's). An
initialiser the use wrote over lines keeps its lines. At namespace scope, `static FNVD_ORIGINAL_FUNCTION` and
`static FNVD_VFTABLE` are written `static const` and `static constexpr`, without `inline`: the constant stays this
file's (several files hold same-named ones that point at different functions).

The probe. The objects group's plain forms hold only where p's static type is exactly T*. The probe compiles every
src/gen file of the build's compile_commands.json with -fsyntax-only and a forced include that redefines those
macros as calls of templates marked [[deprecated("unseam:exact")]] or ("unseam:other"); the warnings' locations are
the uses (a template's use once per instantiation: exact only when every instantiation is). A use the probe never
reaches (a template never instantiated, a file the Linux build does not compile, a branch of #if it skips) is
listed.

Idempotent: a rewritten tree has nothing left of the group's macros but the listed uses, which a second run lists
again and leaves as they are.
"""

import argparse
import bisect
import collections
import json
import os
import random
import re
import shlex
import subprocess
import sys
from multiprocessing import Pool

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
EXTENSIONS = (".h", ".hpp", ".inl", ".cpp", ".c")

RECORDS = re.compile(r"^FNVD_(REPLACES\w*|ADDRESS|LAYOUT)$")
DATA = {"FNVD_GLOBAL", "FNVD_STATIC_MEMBER", "FNVD_DEFINE_DATA", "FNVD_DEFINE_GLOBAL", "FNVD_DEFINE_LOCAL_DATA",
        "FNVD_DEFINE_LOCAL_GLOBAL", "FNVD_FUNCTION_STATIC", "FNVD_TU_STATIC", "FNVD_TU_COPY"}
FUNCTIONS = {"FNVD_VFTABLE", "FNVD_ORIGINAL_FUNCTION", "FNVD_IMPORT", "FNVD_IMPORT_STUB", "FNVD_IMPORT_AS"}
CASTS = {"FNVD_DYNAMIC_CAST", "FNVD_DYNAMIC_CAST_VOID", "FNVD_THROW"}
OBJECTS = {"FNVD_CONSTRUCT", "FNVD_CONSTRUCT_AS", "FNVD_DESTRUCT", "FNVD_DESTRUCT_AS", "FNVD_RECONSTRUCT",
           "FNVD_RECONSTRUCT_AS", "FNVD_DELETE", "FNVD_DELETE_AS", "FNVD_DESTRUCT_IN_PLACE",
           "FNVD_DESTRUCT_IN_PLACE_AS"}
# The macros followed by a second argument list (the call's own, or FNVD_DYNAMIC_CAST_OBJECT_'s).
TWO_LISTS = OBJECTS | {"FNVD_DYNAMIC_CAST", "FNVD_DYNAMIC_CAST_VOID"}
GROUPS = ("records", "data", "functions", "casts", "objects")
# The groups whose rewrite drops an exe address.
ADDRESSED = {"records", "data", "functions"}


def group_of(name):
    if RECORDS.match(name):
        return "records"
    for group, names in (("data", DATA), ("functions", FUNCTIONS), ("casts", CASTS), ("objects", OBJECTS)):
        if name in names:
            return group
    return None


# ---------------------------------------------------------------------------------------------------------------
# Tokens

STRING_PREFIXES = {"L", "u", "U", "u8", "R", "LR", "uR", "UR", "u8R"}
IDENT = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")
PPNUMBER = re.compile(r"\.?[0-9](?:[eEpP][+-]|'[0-9A-Za-z_]|[0-9A-Za-z_.])*")
WS = re.compile(r"[ \t\f\v\r]+|\\\r?\n")
PUNCT = ("...", "->*", "<<=", ">>=", "::", "->", "++", "--", "<<", ">>", "<=", ">=", "==", "!=", "&&", "||", "+=",
         "-=", "*=", "/=", "%=", "&=", "|=", "^=", ".*", "##")


class Tok:
    __slots__ = ("kind", "start", "end", "text", "pp")

    def __init__(self, kind, start, end, text, pp):
        # kind: ident, number, literal, punct, comment; pp: 0 code, 1 a directive, 2 a #define or #undef
        self.kind, self.start, self.end, self.text, self.pp = kind, start, end, text, pp


def _quoted(text, i, quote):
    j, n = i + 1, len(text)
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
    """Every token but white space and newlines."""
    toks = []
    i, n = 0, len(text)
    line_start = True
    pp = 0
    expect_name = False
    while i < n:
        c = text[i]
        if c == "\n":
            i += 1
            line_start = True
            pp = 0
            continue
        m = WS.match(text, i)
        if m:
            i = m.end()
            continue
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
            toks.append(Tok("comment", i, k, text[i:k], pp))
            i = k
            continue
        if text.startswith("/*", i):
            k = text.find("*/", i + 2)
            k = n if k < 0 else k + 2
            toks.append(Tok("comment", i, k, text[i:k], pp))
            i = k
            continue
        if c == "#" and line_start and not pp:
            pp = 1
            toks.append(Tok("punct", i, i + 1, c, pp))
            i += 1
            line_start = False
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
                toks.append(Tok("literal", i, e, text[i:e], pp))
                i = e
                continue
            if expect_name:
                if word in ("define", "undef"):
                    pp = 2
                expect_name = False
            toks.append(Tok("ident", i, j, word, pp))
            i = j
            continue
        expect_name = False
        m = PPNUMBER.match(text, i)
        if m:
            toks.append(Tok("number", i, m.end(), m.group(), pp))
            i = m.end()
            continue
        if c in "\"'":
            e = _quoted(text, i, c)
            toks.append(Tok("literal", i, e, text[i:e], pp))
            i = e
            continue
        for p in PUNCT:
            if text.startswith(p, i):
                toks.append(Tok("punct", i, i + len(p), p, pp))
                i += len(p)
                break
        else:
            toks.append(Tok("punct", i, i + 1, c, pp))
            i += 1
    return toks


def is_namespace_brace(code, k):
    """Whether the `{` at code[k] opens a namespace or an extern "C" block."""
    j = k - 1
    while j >= 0 and (code[j].kind == "ident" and code[j].text not in ("namespace", "extern")
                      or code[j].text in ("::",)):
        j -= 1
    if j >= 0 and code[j].text == "namespace":
        return True
    return j >= 1 and code[j].kind == "literal" and code[j - 1].text == "extern"


def scopes(code):
    """For each code token, whether it is at namespace scope (every enclosing brace a namespace's)."""
    out = []
    stack = []
    for k, t in enumerate(code):
        out.append(all(stack))
        if t.text == "{":
            stack.append(is_namespace_brace(code, k))
        elif t.text == "}" and stack:
            stack.pop()
    return out


def template_close(code, j):
    """The index of the `>` (or `>>`) that closes the template argument list a `<` at code[j] opens; None when none
    closes it before the enclosing list or the statement ends (then it is a less-than)."""
    depth = 0
    nest = 0
    i = j
    while i < len(code):
        x = code[i].text
        if x in ("(", "[", "{"):
            nest += 1
        elif x in (")", "]", "}"):
            nest -= 1
            if nest < 0:
                return None
        elif x in (";", "&&", "||") and nest == 0:
            return None
        elif nest == 0 and x == "<":
            depth += 1
        elif nest == 0 and x in (">", ">>"):
            depth -= 1 if x == ">" else 2
            if depth <= 0:
                return i
        i += 1
    return None


def parens(code, k, cpp=False):
    """The argument spans of the parenthesised list opening at code[k] and the index of its closing `)`. A macro's
    list splits as the preprocessor splits it, at commas outside parentheses; a C++ call's (cpp) also keeps a
    template argument list whole (a `<` after a name that a `>` closes). An argument span is (first token, last
    token) indexes, None for an empty argument."""
    depth = 0
    args = []
    first = None
    j = k
    while j < len(code):
        t = code[j]
        if t.text == "(":
            depth += 1
            if depth == 1:
                first = None
                j += 1
                continue
        elif t.text == ")":
            depth -= 1
            if depth == 0:
                args.append(None if first is None else (first, j - 1))
                return args, j
        elif t.text == "," and depth == 1:
            args.append(None if first is None else (first, j - 1))
            first = None
            j += 1
            continue
        elif cpp and t.text == "<" and depth == 1 and j > k and code[j - 1].kind == "ident":
            end = template_close(code, j)
            if end is not None:
                if first is None:
                    first = j
                j = end + 1
                continue
        if first is None:
            first = j
        j += 1
    return None, None


# ---------------------------------------------------------------------------------------------------------------
# Text helpers

SCALAR_WORDS = {"bool", "char", "short", "int", "long", "float", "double", "unsigned", "signed", "wchar_t",
                "char8_t", "char16_t", "char32_t", "int8_t", "int16_t", "int32_t", "int64_t", "uint8_t", "uint16_t",
                "uint32_t", "uint64_t", "intptr_t", "uintptr_t", "size_t", "ptrdiff_t", "const", "volatile"}
CV = {"const", "volatile"}
SIMPLE_TYPE = re.compile(r"^(::)?[A-Za-z_]\w*(::[A-Za-z_]\w*)*(<.*>)?$")
CASTS_KW = {"static_cast", "reinterpret_cast", "const_cast", "dynamic_cast"}
MARKERS = re.compile(r"\b(KEEP|SWEEP-INLINE|EXE-INLINED|EXE-CALLED)\b")
# A deleted comment that says more than the line it sat on: why a function is not registered, a check's verdict, a
# layout deviation, a guessed type, a naming note, a reason given with `because`.
NOTE = re.compile(r"\b(?:KEEP|CHK-[A-Z]+|UNCOVERED|DIFF|[Nn]ot (?:replaced|registered)|[Uu]nregistered|PDB names|"
                  r"Guessed type|DEVIATION|IDA(?:'s)? names?|[Nn]ot named in|because|unwritten|nothing wrote)\b|"
                  r"\bPDB:")
# Per FNVD_REPLACES* and FNVD_ADDRESS macro: the argument naming the function (the constructor's and destructor's
# the class).
NAME_ARG = {"FNVD_REPLACES_VIRTUAL": 2, "FNVD_REPLACES_MEMBER": 2, "FNVD_REPLACES_VIRTUAL_OVERLOAD": 2,
            "FNVD_REPLACES_VIRTUAL_VIA": 3, "FNVD_REPLACES_VIRTUAL_VIA_OVERLOAD": 3}
WIDTH = 120


def width(s):
    """Columns of a line, tabs at 4."""
    return len(s.expandtabs(4))


def last_name(s):
    """The last `::` component of a qualified name outside template brackets, without its template arguments and
    white space (`operator delete` is `operatordelete`)."""
    depth = 0
    cut = 0
    i = 0
    while i < len(s):
        if s[i] == "<":
            depth += 1
        elif s[i] == ">":
            depth -= 1
        elif depth == 0 and s.startswith("::", i):
            cut = i + 2
            i += 2
            continue
        i += 1
    return re.sub(r"\s+", "", s[cut:].split("<")[0])


def names_own_object(toks):
    """`&x`, `&x.m...`, `&N::x` or `&this->m...` (apThis, pThis too): an object C++ itself destroys (a local, a
    parameter, a member of this or of one of these, a static)."""
    if len(toks) < 2 or toks[0].text != "&" or toks[1].kind != "ident":
        return False
    i = 2
    while i + 1 < len(toks) and toks[i].text == "::" and toks[i + 1].kind == "ident":
        i += 2
    if toks[1].text in ("this", "apThis", "pThis") and i == 2:
        if i + 1 >= len(toks) or toks[i].text != "->" or toks[i + 1].kind != "ident":
            return False
        i += 2
    while i + 1 < len(toks) and toks[i].text == "." and toks[i + 1].kind == "ident":
        i += 2
    return i == len(toks)


def wrap(prefix, new, suffix, cont):
    """new, written after prefix and before suffix on one line, broken into lines of at most WIDTH columns: after
    its first top-level `=` (the value on the next line, at cont; a declarator still too long also before its name),
    and between the arguments of the list it ends with where that is still too long."""
    toks = [t for t in tokenize(new) if t.kind != "comment"]
    depth = 0
    for i, t in enumerate(toks):
        if t.text in ("(", "[", "{"):
            depth += 1
        elif t.text in (")", "]", "}"):
            depth -= 1
        elif t.text == "=" and depth == 0:
            head = new[:t.end].rstrip()
            value = new[t.end:].strip()
            if width(prefix + head) > WIDTH and i >= 2 and toks[i - 1].kind == "ident":
                head = new[:toks[i - 1].start].rstrip() + "\n" + cont + new[toks[i - 1].start:t.end]
            return head + "\n" + cont + wrap_arguments(cont, value, suffix, cont)
    op = last_open(toks)
    if op is not None and op >= 1 and toks[op - 1].kind == "ident" and (
            op == len(toks) - 2 or width(prefix + new[:toks[op].end]) > WIDTH):
        name = toks[op - 1].start
        return new[:name].rstrip() + "\n" + cont + wrap_arguments(cont, new[name:], suffix, cont)
    return wrap_arguments(prefix, new, suffix, cont)


def last_open(toks):
    """The index of the opener of the parenthesised or braced list toks end with; None when they end in none."""
    if not toks or toks[-1].text not in (")", "}"):
        return None
    depth = 0
    for i in range(len(toks) - 1, -1, -1):
        x = toks[i].text
        if x in (")", "]", "}"):
            depth += 1
        elif x in ("(", "[", "{"):
            depth -= 1
            if depth == 0:
                return i
    return None


def wrap_first(prefix, new, cont):
    """new, written over lines after prefix, with its first line broken where it is longer than WIDTH: after the
    opener of its initialiser (`{`, `(` or `=`) when items follow it there, and before the declared name when the
    declarator alone is that long; the new lines at cont."""
    first, rest = new.split("\n", 1)
    if width(prefix + first) <= WIDTH:
        return new
    toks = [t for t in tokenize(first) if t.kind != "comment"]
    i = next((i for i, t in enumerate(toks) if t.text in ("{", "(", "=")), None)
    if i is None:
        return new
    opener = toks[i]
    out = first[:opener.end]
    if width(prefix + out) > WIDTH and i >= 1 and toks[i - 1].kind == "ident":
        out = first[:toks[i - 1].start].rstrip() + "\n" + cont + first[toks[i - 1].start:opener.end]
    tail = first[opener.end:].strip()
    if tail:
        out += "\n" + cont + tail
    return out + "\n" + rest


def wrap_arguments(prefix, s, suffix, cont):
    """s after prefix, its last parenthesised or braced list's arguments filled into lines of at most WIDTH columns
    (each further line at cont); s as it is where it fits or ends in no list."""
    if width(prefix + s + suffix) <= WIDTH:
        return s
    toks = [t for t in tokenize(s) if t.kind != "comment"]
    op = last_open(toks)
    if op is None or op == len(toks) - 2:
        return s
    pieces = []
    depth = 0
    start = toks[op].end
    for t in toks[op + 1:-1]:
        if t.text in ("(", "[", "{"):
            depth += 1
        elif t.text in (")", "]", "}"):
            depth -= 1
        elif t.text == "," and depth == 0:
            pieces.append(s[start:t.end].strip())
            start = t.end
    pieces.append(s[start:toks[-1].start].strip())
    closer = s[toks[-1].start:]
    done = []
    line = prefix + s[:toks[op].end]
    for n, p in enumerate(pieces):
        end = closer + suffix if n == len(pieces) - 1 else ""
        sep = "" if n == 0 else " "
        if width(line + sep + p + end) > WIDTH:
            done.append(line)
            line = cont + p
        else:
            line += sep + p
    done.append(line + closer)
    return "\n".join(done)[len(prefix):]


def squash(s):
    """An argument's text on one line, unless a comment holds it apart."""
    if "//" in s or "/*" in s:
        return s.strip()
    return re.sub(r"\s+", " ", s).strip()


def type_words(t):
    return re.findall(r"[A-Za-z_]\w*|::|\S", t)


def is_scalar(t):
    """A built-in arithmetic type or a pointer, by its spelling."""
    w = type_words(t)
    while w and w[-1] in CV:
        w.pop()
    if not w:
        return False
    if w[-1] == "*" and "(" not in w and "[" not in w:
        return True
    return all(x in SCALAR_WORDS for x in w) and any(x not in CV for x in w)


def is_const(t):
    w = type_words(t)
    return "const" in w or "constexpr" in w


def last_component(t):
    """The class name a destructor call names: the last `::` component of t outside template brackets (`typename`,
    `struct`, `class` dropped), and whether t is qualified. None where a component is not a name with optional
    template arguments (a cv-qualified, pointer or built-in type)."""
    t = t.strip()
    for kw in ("typename ", "struct ", "class "):
        if t.startswith(kw):
            t = t[len(kw):].strip()
    parts = []
    depth = 0
    cut = 0
    i = 0
    while i < len(t):
        c = t[i]
        if c == "<":
            depth += 1
        elif c == ">":
            depth -= 1
        elif depth == 0 and t.startswith("::", i):
            parts.append(t[cut:i].strip())
            cut = i + 2
            i += 2
            continue
        i += 1
    parts.append(t[cut:].strip())
    if parts and parts[0] == "":
        parts.pop(0)
    component = re.compile(r"^[A-Za-z_]\w*\s*(<.*>)?$", re.S)
    if not parts or depth != 0 or not all(component.match(p) for p in parts):
        return None, False
    name = parts[-1]
    if name.split("<")[0].strip() in SCALAR_WORDS:
        return None, False
    return name, len(parts) > 1


def expr_shape(toks):
    """How an expression's tokens stand: 'postfix' (a name, member access, call or subscript chain, a named cast),
    'address' (`&` before such a chain), or 'other'."""
    if not toks:
        return "other"

    def chain(ts):
        i = 0
        n = len(ts)
        if n == 0:
            return False
        expect_operand = True
        while i < n:
            t = ts[i]
            if expect_operand:
                if t.kind in ("ident", "number", "literal") or t.text == "::":
                    if t.text in CASTS_KW and i + 1 < n and ts[i + 1].text == "<":
                        depth = 0
                        j = i + 1
                        while j < n:
                            if ts[j].text == "<":
                                depth += 1
                            elif ts[j].text == ">":
                                depth -= 1
                                if depth == 0:
                                    break
                            elif ts[j].text == ">>":
                                depth -= 2
                                if depth <= 0:
                                    break
                            j += 1
                        if j >= n or j + 1 >= n or ts[j + 1].text != "(":
                            return False
                        i = j + 1
                        expect_operand = False
                        continue
                    if t.text == "::":
                        i += 1
                        continue
                    i += 1
                    expect_operand = False
                    continue
                if t.text == "(":
                    i = skip_group(ts, i)
                    if i is None:
                        return False
                    expect_operand = False
                    continue
                return False
            if t.text in (".", "->", "::"):
                i += 1
                expect_operand = True
                continue
            if t.text in ("(", "["):
                i = skip_group(ts, i)
                if i is None:
                    return False
                continue
            return False
        return not expect_operand

    if toks[0].text == "&" and chain(toks[1:]):
        return "address"
    return "postfix" if chain(toks) else "other"


def skip_group(ts, i):
    """Index after the bracket group opening at ts[i]."""
    pairs = {"(": ")", "[": "]", "{": "}"}
    close = pairs[ts[i].text]
    depth = 0
    opener = ts[i].text
    while i < len(ts):
        if ts[i].text == opener:
            depth += 1
        elif ts[i].text == close:
            depth -= 1
            if depth == 0:
                return i + 1
        i += 1
    return None


def no_side_effect(toks):
    """An operand that only names an object: names, `this`, `&`, `*`, member access, constant subscripts."""
    for t in toks:
        if t.kind in ("ident", "number"):
            continue
        if t.text in ("&", "*", ".", "->", "::", "[", "]"):
            continue
        return False
    return True


# ---------------------------------------------------------------------------------------------------------------
# One file

class Site:
    __slots__ = ("group", "macro", "line", "col", "action", "reason", "first")

    def __init__(self, group, macro, line, col, action, reason, first):
        self.group, self.macro, self.line, self.col = group, macro, line, col
        self.action, self.reason, self.first = action, reason, first


class FileWork:
    def __init__(self, rel, text, groups, probe, aliases, line_map=None, orig_lines=None):
        self.rel = rel
        self.text = text
        self.groups = groups
        self.probe = probe
        self.aliases = aliases
        # The input tree's line (0-based) of each line of text, and the input tree's lines (None: text is the input).
        self.line_map = line_map
        self.orig_lines = orig_lines
        self.toks = tokenize(text)
        self.code = []
        self.code_ti = []        # each code token's index in toks
        for i, t in enumerate(self.toks):
            if t.kind != "comment":
                self.code.append(t)
                self.code_ti.append(i)
        self.code_starts = [t.start for t in self.code]
        self.at_namespace = scopes([t for t in self.code if not t.pp])
        self.ns_index = {}
        k = 0
        for t in self.code:
            if not t.pp:
                self.ns_index[id(t)] = k
                k += 1
        self.line_starts = [0]
        for m in re.finditer("\n", text):
            self.line_starts.append(m.end())
        self.edits = []         # (start, end, replacement)
        self.dead_lines = set()  # 0-based lines deleted whole
        self.dead_info = {}      # first line of a deleted record -> (macro, entity, comment rule, name)
        self.sites = []
        self.notes = []          # (input line, entity, action, text)
        self.dropped_names = []  # identifiers in dropped arguments (the casts' RTTI descriptors)
        self.declared = []       # (kind, name): FNVD_VFTABLE and FNVD_ORIGINAL_FUNCTION constants rewritten

    # positions
    def line_of(self, off):
        lo, hi = 0, len(self.line_starts) - 1
        while lo < hi:
            mid = (lo + hi + 1) // 2
            if self.line_starts[mid] <= off:
                lo = mid
            else:
                hi = mid - 1
        return lo

    def col_of(self, off):
        ls = self.line_starts[self.line_of(off)]
        return len(self.text[ls:off].encode("utf-8", "surrogateescape")) + 1

    def line_text(self, i):
        s = self.line_starts[i]
        e = self.line_starts[i + 1] - 1 if i + 1 < len(self.line_starts) else len(self.text)
        return self.text[s:e]

    def text_of(self, s, e):
        """text[s:e] with the edits already made inside it (a nested use's rewrite)."""
        out = []
        pos = s
        for es, ee, new in sorted(x for x in self.edits if s <= x[0] and x[1] <= e):
            out.append(self.text[pos:es])
            out.append(new)
            pos = ee
        out.append(self.text[pos:e])
        return "".join(out)

    def arg_text(self, a0, a1):
        """The text of code[a0..a1] with the comments between them and the list's delimiters around them (a comment
        before the first token or after the last one goes with the argument)."""
        s, e = self.code[a0].start, self.code[a1].end
        i = self.code_ti[a0] - 1
        while i >= 0 and self.toks[i].kind == "comment":
            s = self.toks[i].start
            i -= 1
        i = self.code_ti[a1] + 1
        while i < len(self.toks) and self.toks[i].kind == "comment" and not self.toks[i].text.startswith("//"):
            e = self.toks[i].end
            i += 1
        return self.text_of(s, e)

    def break_before(self, a0):
        """The line break and indent between code[a0] (with the comments arg_text gives it) and the delimiter
        before it; "" when they stand on one line."""
        i = self.code_ti[a0] - 1
        while i >= 0 and self.toks[i].kind == "comment":
            i -= 1
        gap = self.text[self.toks[i].end:self.toks[i + 1].start] if i >= 0 else ""
        return gap[gap.rfind("\n"):] if "\n" in gap else ""

    def span(self, a):
        """The source text of an argument span."""
        if a is None:
            return ""
        return self.arg_text(a[0], a[1])

    def rest(self, args, k):
        """The text of arguments k.. joined as written (a variadic argument)."""
        real = [a for a in args[k:] if a is not None]
        if len(args) <= k or not real:
            return ""
        return self.arg_text(real[0][0], real[-1][1])

    def ends_in_line_comment(self, k, last):
        """Whether a // comment stands right before a `,` or `)` of the lists from code[k] to code[last]."""
        for i in range(self.code_ti[k], self.code_ti[last]):
            t = self.toks[i]
            if t.kind == "comment" and t.text.startswith("//") and self.toks[i + 1].text in (",", ")"):
                return True
        return False

    def input_line(self, line):
        """The input tree's line (0-based) of a line of text."""
        return self.line_map[line] if self.line_map is not None else line

    def site(self, group, macro, tok, action, reason=""):
        line = self.input_line(self.line_of(tok.start))
        first = self.orig_lines[line] if self.orig_lines is not None else self.line_text(line)
        self.sites.append(Site(group, macro, line + 1, self.col_of(tok.start), action, reason, first.strip()))

    # edits
    def delete_statement(self, s, e, info=None):
        """Delete the statement text[s:e]: whole lines when it stands alone on them (a trailing // comment too).
        info: a record's (macro, entity, comment rule, name), for the comments above it (attached_comments)."""
        l0, l1 = self.line_of(s), self.line_of(max(s, e - 1))
        before = self.text[self.line_starts[l0]:s]
        after_end = self.line_starts[l1 + 1] - 1 if l1 + 1 < len(self.line_starts) else len(self.text)
        after = self.text[e:after_end]
        if not before.strip() and (not after.strip() or after.strip().startswith("//")):
            for ln in range(l0, l1 + 1):
                self.dead_lines.add(ln)
            if info is not None:
                self.dead_info[l0] = info
            if after.strip():
                self.note(l1, info[1] if info else squash(self.text[s:e]), "deleted", [after.strip()])
            return
        # Not alone: cut the statement and the white space after it.
        m = re.match(r"[ \t]*", self.text[e:])
        self.edits.append((s, e + m.end(), ""))

    def note(self, line, entity, action, lines):
        """A row of notes.tsv when the comment lines say more than the deleted line (NOTE), or a marker kept them."""
        if action == "deleted" and not any(NOTE.search(x) for x in lines):
            return
        self.notes.append((self.input_line(line) + 1, entity, action, "\\n".join(x.strip() for x in lines)))

    def replace(self, s, e, new):
        """Replace text[s:e]; the nested edits inside it are part of new already (text_of)."""
        self.edits = [x for x in self.edits if not (s <= x[0] and x[1] <= e)]
        self.edits.append((s, e, new))

    # the walk
    def run(self):
        self.scan(0, len(self.code))
        return self

    def scan(self, k, end):
        """Every use in code[k:end]. A use's arguments are scanned first, so that a use nested in another's is
        rewritten before the outer one copies its text."""
        code = self.code
        while k < end:
            t = code[k]
            if t.kind != "ident" or not t.text.startswith("FNVD_"):
                k += 1
                continue
            group = group_of(t.text)
            if group is None or group not in self.groups:
                k += 1
                continue
            if t.pp:
                self.site(group, t.text, t, "listed", "in a preprocessor directive")
                k += 1
                continue
            if k + 1 >= len(code) or code[k + 1].text != "(":
                self.site(group, t.text, t, "listed", "not followed by its arguments")
                k += 1
                continue
            args, close = parens(code, k + 1)
            if args is None:
                self.site(group, t.text, t, "listed", "unbalanced parentheses")
                k += 1
                continue
            last = close
            if t.text in TWO_LISTS and close + 1 < len(code) and code[close + 1].text == "(":
                _, close2 = parens(code, close + 1)
                if close2 is not None:
                    last = close2
            self.scan(k + 2, last)
            if group != "records" and self.ends_in_line_comment(k, last):
                self.site(group, t.text, t, "listed", "an argument ends in a // comment")
                k = last + 1
                continue
            handler = getattr(self, "do_" + group)
            k = max(handler(k, t, args, close), last + 1)

    def prev_text(self, k):
        j = k - 1
        while j >= 0 and self.code[j].pp:
            j -= 1
        return self.code[j].text if j >= 0 else ";"

    def next_text(self, j):
        j += 1
        while j < len(self.code) and self.code[j].pp:
            j += 1
        return (self.code[j].text if j < len(self.code) else ""), j

    def statement_end(self, close):
        """The end offset of the statement whose macro closes at code[close]: its `;` when one follows."""
        nxt, j = self.next_text(close)
        if nxt == ";":
            return self.code[j].end, j
        return self.code[close].end, close

    # ---- 1a
    def record_info(self, t, args):
        """(macro, entity, comment rule, name) of a record or FNVD_TU_COPY: the comment block above a LAYOUT, a
        TU_COPY or an in-class ADDRESS line describes that line ('always' deleted with it); above a REPLACES* or a
        namespace-scope ADDRESS line it may describe the function defined right after ('definition', name)."""
        m = t.text
        if m == "FNVD_LAYOUT":
            return m, squash(self.rest(args, 0)), "always", None
        if m == "FNVD_TU_COPY":
            return m, squash(self.span(args[1])) if len(args) > 1 else "", "always", None
        k = NAME_ARG.get(m, 1)
        entity = squash(self.span(args[k])) if len(args) > k else ""
        if m == "FNVD_ADDRESS" and not self.at_namespace[self.ns_index[id(t)]]:
            return m, entity, "always", None
        name = last_name(entity)
        if m == "FNVD_REPLACES_DESTRUCTOR":
            name = "~" + name
        return m, entity, "definition", name

    def do_records(self, k, t, args, close):
        end, last = self.statement_end(close)
        self.delete_statement(t.start, end, self.record_info(t, args))
        self.site("records", t.text, t, "deleted")
        return last + 1

    # ---- layout
    def fit(self, s, e, new):
        """new, the rewrite of text[s:e] (a declaration), wrapped when it leaves its line longer than WIDTH. A rewrite
        that keeps the use's own line breaks (an initialiser written over lines) gets only its first line wrapped."""
        l0, l1 = self.line_of(s), self.line_of(max(s, e - 1))
        ls = self.line_starts[l0]
        le = self.line_starts[l1 + 1] - 1 if l1 + 1 < len(self.line_starts) else len(self.text)
        prefix = self.text_of(ls, s)
        suffix = self.text_of(e, le)
        indent = re.match(r"[ \t]*", prefix).group()
        cont = None
        if "\n" in new:
            cont = re.match(r"[ \t]*", new.split("\n", 1)[1]).group()
        elif l1 > l0:
            cont = re.match(r"[ \t]*", self.line_text(l0 + 1)).group()
        if cont is None or width(cont) <= width(indent):
            tabs = "\t" in indent or (not indent and self.text.count("\n\t") >= self.text.count("\n    "))
            cont = indent + ("\t" if tabs else "    ")
        if "\n" in new:
            return wrap_first(prefix, new, cont)
        if width(prefix + new + suffix) <= WIDTH:
            return new
        return wrap(prefix, new, suffix, cont)

    # ---- 1b data
    def do_data(self, k, t, args, close):
        m = t.text
        n = len(args)
        if m == "FNVD_TU_COPY":
            end, last = self.statement_end(close)
            self.delete_statement(t.start, end, self.record_info(t, args))
            self.site("data", m, t, "deleted")
            return last + 1
        need = {"FNVD_GLOBAL": 3, "FNVD_STATIC_MEMBER": 3, "FNVD_TU_STATIC": 2, "FNVD_FUNCTION_STATIC": 6}.get(m, 3)
        if n < need or any(a is None for a in args[:need]):
            self.site("data", m, t, "listed", "too few arguments")
            return close + 1
        if m in ("FNVD_GLOBAL", "FNVD_STATIC_MEMBER") and n != 3:
            self.site("data", m, t, "listed", "too many arguments")
            return close + 1
        typ = squash(self.span(args[0]))
        name = squash(self.span(args[1]))
        init = self.rest(args, need)
        real = [a for a in args[need:] if a is not None]
        ninit = len(real)
        # An initialiser written over lines keeps the use's line break before it (after the dropped address); a
        # one-line one joins the declaration, which fit() wraps where it gets too long.
        brk = self.break_before(real[0][0]) if real and "\n" in init else ""
        at_ns = self.at_namespace[self.ns_index[id(t)]]
        if m == "FNVD_GLOBAL":
            new = "extern %s %s" % (typ, name)
        elif m == "FNVD_STATIC_MEMBER":
            new = "static %s %s" % (typ, name)
        elif m == "FNVD_FUNCTION_STATIC":
            if ninit > 8:
                self.site("data", m, t, "listed", "more than 8 initialiser arguments")
                return close + 1
            new = "static %s %s" % (typ, name)
            if ninit == 1:
                new += " =" + (brk or " ") + init
            elif ninit > 1:
                new += "(%s%s)" % (brk, init)
        else:
            static = "static " if m in ("FNVD_DEFINE_LOCAL_DATA", "FNVD_DEFINE_LOCAL_GLOBAL", "FNVD_TU_STATIC") else ""
            braces = m in ("FNVD_DEFINE_DATA", "FNVD_DEFINE_LOCAL_DATA", "FNVD_TU_STATIC")
            new = "%s%s %s" % (static, typ, name)
            if ninit == 0:
                if braces and (not at_ns or is_const(typ) or not is_scalar(typ)):
                    new += "{}"
            elif ninit == 1 and is_scalar(typ) and not init.lstrip().startswith("{"):
                new += " =" + (brk or " ") + init
            elif braces:
                new += "{%s%s}" % (brk, init)
            else:
                new += "(%s%s)" % (brk, init)
        # `static FNVD_STATIC_MEMBER(...)` expands to a duplicate specifier: written once.
        word = new.split(" ", 1)[0]
        if word in ("static", "extern") and self.prev_text(k) == word:
            new = new.split(" ", 1)[1]
        end = self.code[close].end
        self.replace(t.start, end, self.fit(t.start, end, new))
        self.site("data", m, t, "rewritten")
        return close + 1

    # ---- 1b functions
    def after_static(self, k, t):
        """Whether a `static` at namespace scope stands right before the macro: the constant is then written
        `static const` (internal, as the macro's expansion has it), without the `inline` the macro adds."""
        j = k - 1
        while j >= 0 and self.code[j].pp:
            j -= 1
        return j >= 0 and self.code[j].text == "static" and self.at_namespace[self.ns_index[id(t)]]

    def do_functions(self, k, t, args, close):
        m = t.text
        if m.startswith("FNVD_IMPORT"):
            self.site("functions", m, t, "listed",
                      "an import: dllimport on Windows only, and its plain form is the prototype the pointer typedef "
                      "spells")
            return close + 1
        end = self.code[close].end
        inline = "" if self.after_static(k, t) else "inline "
        if m == "FNVD_VFTABLE":
            if len(args) != 2 or args[0] is None:
                self.site("functions", m, t, "listed", "not two arguments")
                return close + 1
            name = squash(self.span(args[0]))
            self.replace(t.start, end, self.fit(t.start, end, "%sconstexpr fnvd::Vtable %s{}" % (inline, name)))
            self.declared.append(("vtable", name))
            self.site("functions", m, t, "rewritten")
            return close + 1
        # FNVD_ORIGINAL_FUNCTION(type, name, address, function...)
        if len(args) < 3 or any(a is None for a in args[:3]):
            self.site("functions", m, t, "listed", "too few arguments")
            return close + 1
        typ = squash(self.span(args[0]))
        name = squash(self.span(args[1]))
        fn = self.rest(args, 3).strip()
        void = re.sub(r"\s+", "", typ) == "void*"
        if not fn:
            value = "nullptr"
        elif void:
            value = "reinterpret_cast<void*>(&%s)" % fn
        else:
            value = "&" + fn
        if SIMPLE_TYPE.match(typ) and not void:
            decl = "%sconst %s %s" % (inline, typ, name)
        else:
            decl = "%s%s const %s" % (inline, typ, name)
        self.replace(t.start, end, self.fit(t.start, end, "%s = %s" % (decl, value)))
        self.declared.append(("original", name))
        self.site("functions", m, t, "rewritten")
        return close + 1

    # ---- 1b casts
    def second_list(self, close, cpp):
        """The list right after the macro's: (args, close) or (None, None). FNVD_DYNAMIC_CAST's is another macro's
        (FNVD_DYNAMIC_CAST_OBJECT_), the objects macros' a C++ call's (cpp)."""
        if close + 1 < len(self.code) and self.code[close + 1].text == "(":
            return parens(self.code, close + 1, cpp)
        return None, None

    def do_casts(self, k, t, args, close):
        m = t.text
        if m == "FNVD_THROW":
            if len(args) != 2 or args[0] is None:
                self.site("casts", m, t, "listed", "not two arguments")
                return close + 1
            obj = squash(self.span(args[0]))
            nxt, _ = self.next_text(close)
            shape = expr_shape(tokenize(obj))
            new = "throw %s" % obj if nxt == ";" and shape == "postfix" else "throw (%s)" % obj
            self.replace(t.start, self.code[close].end, new)
            self.site("casts", m, t, "rewritten")
            return close + 1
        if m == "FNVD_DYNAMIC_CAST_VOID":
            self.site("casts", m, t, "listed",
                      "the void* result: a static_cast of it takes no base adjustment that dynamic_cast<T*>'s would")
            return close + 1
        args2, close2 = self.second_list(close, False)
        if args2 is None or len(args2) != 3 or args2[0] is None:
            self.site("casts", m, t, "listed", "the object and the two descriptors do not follow")
            return close + 1
        typ = squash(self.rest(args, 0))
        obj = self.span(args2[0])
        for a in args2[1:]:
            self.dropped_names.extend(x.text for x in self.code[a[0]:a[1] + 1] if a and x.kind == "ident")
        self.replace(t.start, self.code[close2].end, "dynamic_cast<%s*>(%s)" % (typ, obj))
        self.site("casts", m, t, "rewritten")
        return close2 + 1

    # ---- 1b objects
    def probe_kind(self, t):
        return self.probe.get((self.rel, self.line_of(t.start) + 1, self.col_of(t.start)))

    def destroy(self, typ, ptoks, ptext, comments=True):
        """`p->~N()` for the pointer p, `x.~N()` for p = &x; None when T has no name a destructor call can take. The
        operand's comments before and after it go before and after the call (comments)."""
        name, qualified = last_component(typ)
        if name is None:
            return None, "the type has no class name a destructor call can name"
        if qualified and name.split("<")[0].strip() in self.aliases:
            return None, "a qualified alias, which a destructor call cannot name"
        if not ptoks:
            return None, "no operand"
        core = ptext[ptoks[0].start:ptoks[-1].end]
        shape = expr_shape(ptoks)
        if shape == "address":
            new = "%s.~%s()" % (ptext[ptoks[1].start:ptoks[-1].end], name)
        elif shape == "postfix":
            new = "%s->~%s()" % (core, name)
        else:
            new = "(%s)->~%s()" % (core, name)
        if comments:
            new = ptext[:ptoks[0].start] + new + ptext[ptoks[-1].end:]
        return new, None

    def new_type(self, typ, args2):
        # With no constructor arguments the seam's helpers default-initialise: `T`, never `T()`, which would
        # zero-fill a type that has no constructor of its own.
        rest = self.rest(args2, 1)
        return "%s(%s)" % (typ, rest) if rest.strip() else typ

    def do_objects(self, k, t, args, close):
        m = t.text
        aliased = m.endswith("_AS")
        targs = args[1:] if aliased else args
        typ = squash(self.rest(targs, 0)) if targs and targs[0] is not None else ""
        args2, close2 = self.second_list(close, True)
        if not typ or args2 is None:
            self.site("objects", m, t, "listed", "not the form MACRO(T)(arguments)")
            return close + 1
        base = m[:-3] if aliased else m
        prev = self.prev_text(k)
        nxt, nxt_j = self.next_text(close2)
        first = args2[0] if args2 else None
        ptext = self.span(first)
        # The operand's tokens as it now reads (a nested use in it rewritten), offsets into ptext.
        ptoks = [x for x in tokenize(ptext) if x.kind != "comment"]
        if base == "FNVD_CONSTRUCT":
            if first is None:
                self.site("objects", m, t, "listed", "no memory argument")
                return close2 + 1
            new = "::new (%s) %s" % (ptext, self.new_type(typ, args2))
            if nxt in ("->", ".", "[", "(", "++", "--"):
                new = "(%s)" % new
            self.replace(t.start, self.code[close2].end, new)
            self.site("objects", m, t, "rewritten")
            return close2 + 1
        if base == "FNVD_DESTRUCT_IN_PLACE":
            if len(args2) != 1 or first is None:
                self.site("objects", m, t, "listed", "not one argument")
            elif prev not in (";", "{", "}") or nxt != ";":
                self.site("objects", m, t, "listed", "not a statement of its own")
            elif not no_side_effect(ptoks):
                self.site("objects", m, t, "listed", "its operand may have a side effect")
            elif not names_own_object(ptoks):
                self.site("objects", m, t, "listed", "the operand is not &x for a local, parameter or member: "
                          "nothing else may destroy that object")
            else:
                self.delete_statement(t.start, self.code[nxt_j].end)
                self.site("objects", m, t, "deleted")
                return nxt_j + 1
            return close2 + 1
        kind = self.probe_kind(t)
        if kind is None:
            self.site("objects", m, t, "listed", "the probe did not reach it")
            return close2 + 1
        if kind != "exact":
            self.site("objects", m, t, "listed", "the operand is not a T* (a derived, smart or void pointer)")
            return close2 + 1
        if base == "FNVD_DESTRUCT":
            if len(args2) != 1:
                self.site("objects", m, t, "listed", "not one argument")
                return close2 + 1
            new, why = self.destroy(typ, ptoks, ptext)
            if new is None:
                self.site("objects", m, t, "listed", why)
                return close2 + 1
            self.replace(t.start, self.code[close2].end, new)
            self.site("objects", m, t, "rewritten")
            return close2 + 1
        if base == "FNVD_DELETE":
            if len(args2) != 2 or args2[1] is None:
                self.site("objects", m, t, "listed", "not two arguments")
                return close2 + 1
            flags = "".join(x.text for x in self.code[args2[1][0]:args2[1][1] + 1]).rstrip("uU")
            if prev not in (";", "{", "}", ")", "else", ":") or nxt != ";":
                self.site("objects", m, t, "listed", "its result is used")
                return close2 + 1
            shape = expr_shape(ptoks)
            operand = ptext if shape in ("postfix", "address") else "(%s)" % ptext
            if flags == "0":
                new, why = self.destroy(typ, ptoks, ptext)
                if new is None:
                    self.site("objects", m, t, "listed", why)
                    return close2 + 1
            elif flags == "1":
                new = "delete " + operand
            elif flags == "3":
                new = "delete[] " + operand
            else:
                self.site("objects", m, t, "listed", "flags %s: not one plain C++ form" % flags)
                return close2 + 1
            self.replace(t.start, self.code[close2].end, new)
            self.site("objects", m, t, "rewritten")
            return close2 + 1
        # FNVD_RECONSTRUCT
        if prev not in (";", "{", "}") or nxt != ";":
            self.site("objects", m, t, "listed", "not a statement of its own")
            return close2 + 1
        if not no_side_effect(ptoks):
            self.site("objects", m, t, "listed", "its operand may have a side effect (the plain form evaluates it "
                      "twice)")
            return close2 + 1
        rest_toks = [x for a in args2[1:] if a for x in self.code[a[0]:a[1] + 1]]
        named = {x.text for x in ptoks if x.kind == "ident"} - {"this", "pThis"}
        if any(x.text == "(" for x in rest_toks) or named & {x.text for x in rest_toks if x.kind == "ident"}:
            self.site("objects", m, t, "listed", "an argument calls or names the object (the seam evaluates the "
                      "arguments before the destructor)")
            return close2 + 1
        destroy, why = self.destroy(typ, ptoks, ptext, comments=False)
        if destroy is None:
            self.site("objects", m, t, "listed", why)
            return close2 + 1
        # Two statements; on two lines where the use starts its line.
        before = self.text_of(self.line_starts[self.line_of(t.start)], t.start)
        sep = ";\n" + before if not before.strip() else "; "
        new = "%s%s::new (%s) %s" % (destroy, sep, ptext, self.new_type(typ, args2))
        self.replace(t.start, self.code[close2].end, new)
        self.site("objects", m, t, "rewritten")
        return close2 + 1

    # ---- applying
    def declared_name(self, ln):
        """The name a function declaration or definition starting at line ln declares (its last `::` component, as
        last_name spells it); None when no such statement starts there."""
        k = bisect.bisect_left(self.code_starts, self.line_starts[ln])
        if k >= len(self.code) or self.line_of(self.code[k].start) != ln or self.code[k].pp:
            return None
        depth = 0
        for i in range(k, len(self.code)):
            x = self.code[i].text
            if x in (";", "{", "}") and depth == 0:
                return None
            if x == "(" and depth == 0:
                break
            if x == "<":
                depth += 1
            elif x in (">", ">>"):
                depth = max(0, depth - len(x))
        else:
            return None
        names = []
        j = i - 1
        while j >= k:
            t = self.code[j]
            if t.kind == "ident" and names and self.code[j + 1].kind == "ident" and t.text != "operator":
                break
            if t.kind == "ident" or t.text in ("::", "~") or (j > k and self.code[j - 1].text == "operator"):
                names.append(t.text)
                j -= 1
                continue
            break
        return last_name(" ".join(reversed(names))) if names else None

    def attached_comments(self):
        """The comment block right above each run of deleted lines: deleted with it by the rules of the docstring
        ("Comments"), written to notes.tsv when it says more than the run or a marker keeps it."""
        lines = len(self.line_starts)
        extra = set()
        for ln in sorted(self.dead_lines):
            if ln - 1 in self.dead_lines:
                continue
            j = ln
            while j < lines and (j in self.dead_lines):
                j += 1
            # j: the first line after the run
            c = ln - 1
            block = []
            while c >= 0 and c not in self.dead_lines:
                s = self.line_text(c).strip()
                if s.startswith("//") or (s.startswith("/*") and s.endswith("*/")):
                    block.append(c)
                    c -= 1
                    continue
                break
            if not block:
                continue
            block.reverse()
            infos = [self.dead_info[x] for x in range(ln, j) if x in self.dead_info]
            if not infos:
                # Not a record: the unit continues while lines are deleted, up to a blank or a `}`.
                nxt = self.line_text(j).strip() if j < lines else ""
                if nxt and not nxt.startswith("}"):
                    continue
                entity = squash(self.line_text(ln))
            else:
                entity = infos[0][1]
                names = {i[3] for i in infos if i[2] == "definition"}
                if names and j < lines and self.declared_name(j) in names:
                    continue
            text = [self.line_text(x) for x in block]
            # A block holding a marker a later step reads (KEEP, SWEEP-INLINE, EXE-INLINED, EXE-CALLED) stays.
            if any(MARKERS.search(x) for x in text):
                self.note(block[0], entity, "kept: a marker", text)
                continue
            self.note(block[0], entity, "deleted", text)
            extra.update(block)
        self.dead_lines |= extra

    def blank_cleanup(self):
        lines = len(self.line_starts)
        changed = True
        while changed:
            changed = False
            ln = 0
            while ln < lines:
                if ln not in self.dead_lines:
                    ln += 1
                    continue
                s = ln
                while ln < lines and ln in self.dead_lines:
                    ln += 1
                a = s - 1
                b = ln
                at = self.line_text(a).strip() if a >= 0 else None
                bt = self.line_text(b).strip() if b < lines else None
                if bt == "" and b == lines - 1 and self.text.endswith("\n"):
                    bt = None  # the empty string after the last newline is no line
                drop = None
                if at == "" and bt == "":
                    drop = b
                elif at == "" and (bt is None or bt.startswith("}")):
                    drop = a
                elif bt == "" and (at is None or at.endswith("{") or re.match(r"^(public|protected|private)\s*:$",
                                                                            at or "")):
                    drop = b
                if drop is not None and drop not in self.dead_lines:
                    self.dead_lines.add(drop)
                    changed = True

    def result(self):
        """The new text, and per line of it the line of text it comes from (None when nothing changed)."""
        if not self.edits and not self.dead_lines:
            return self.text, None
        self.attached_comments()
        self.blank_cleanup()
        edits = list(self.edits)
        lines = len(self.line_starts)
        for ln in self.dead_lines:
            s = self.line_starts[ln]
            e = self.line_starts[ln + 1] if ln + 1 < lines else len(self.text)
            edits.append((s, e, ""))
        edits.sort()
        out = []
        segs = []   # (offset in the new text, offset in text, copied or replaced)
        size = 0
        pos = 0
        for s, e, new in edits:
            if s < pos:
                # inside a deleted line already
                continue
            segs.append((size, pos, True))
            out.append(self.text[pos:s])
            size += s - pos
            segs.append((size, s, False))
            out.append(new)
            size += len(new)
            pos = e
        segs.append((size, pos, True))
        out.append(self.text[pos:])
        new_text = "".join(out)
        keys = [x[0] for x in segs]
        back = []
        for q in [0] + [m.end() for m in re.finditer("\n", new_text)]:
            at, old, copied = segs[bisect.bisect_right(keys, q) - 1]
            back.append(self.line_of(old + (q - at) if copied else old))
        return new_text, back


# ---------------------------------------------------------------------------------------------------------------
# The probe

PROBE_HEADER = r"""// unseam.py's probe: the seam's object macros as calls whose deprecation warnings say whether the operand is a T*.
#pragma once
#include "Reconstruction.hpp"
namespace fnvd_unseam {
template <class T, class S>
constexpr bool exact = std::is_class_v<T> && std::is_same_v<std::remove_cv_t<S>, T>;
template <class T, class S> requires exact<T, S>
[[deprecated("unseam:exact")]] inline void destruct(S*) {}
template <class T, class S> requires (!exact<T, S>)
[[deprecated("unseam:other")]] inline void destruct(S*) {}
template <class T, class S> requires (!std::is_pointer_v<std::remove_cvref_t<S>>)
[[deprecated("unseam:other")]] inline void destruct(S&&) {}
template <class T, class S> requires exact<T, S>
[[deprecated("unseam:exact")]] inline T* deletingDestruct(S*, uint32_t) { return nullptr; }
template <class T, class S> requires (!exact<T, S>)
[[deprecated("unseam:other")]] inline T* deletingDestruct(S*, uint32_t) { return nullptr; }
template <class T, class S> requires (!std::is_pointer_v<std::remove_cvref_t<S>>)
[[deprecated("unseam:other")]] inline T* deletingDestruct(S&&, uint32_t) { return nullptr; }
template <class T, class S, class... A> requires exact<T, S>
[[deprecated("unseam:exact")]] inline T* reconstruct(S*, A&&...) { return nullptr; }
template <class T, class S, class... A> requires (!exact<T, S>)
[[deprecated("unseam:other")]] inline T* reconstruct(S*, A&&...) { return nullptr; }
template <class T, class S, class... A> requires (!std::is_pointer_v<std::remove_cvref_t<S>>)
[[deprecated("unseam:other")]] inline T* reconstruct(S&&, A&&...) { return nullptr; }
} // namespace fnvd_unseam
#undef FNVD_DESTRUCT
#undef FNVD_DESTRUCT_AS
#undef FNVD_DELETE
#undef FNVD_DELETE_AS
#undef FNVD_RECONSTRUCT
#undef FNVD_RECONSTRUCT_AS
#define FNVD_DESTRUCT(...) ::fnvd_unseam::destruct<__VA_ARGS__>
#define FNVD_DESTRUCT_AS(name, ...) ::fnvd_unseam::destruct<__VA_ARGS__>
#define FNVD_DELETE(...) ::fnvd_unseam::deletingDestruct<__VA_ARGS__>
#define FNVD_DELETE_AS(name, ...) ::fnvd_unseam::deletingDestruct<__VA_ARGS__>
#define FNVD_RECONSTRUCT(...) ::fnvd_unseam::reconstruct<__VA_ARGS__>
#define FNVD_RECONSTRUCT_AS(name, ...) ::fnvd_unseam::reconstruct<__VA_ARGS__>
"""

DIAG = re.compile(r"^(.*?):(\d+):(\d+): warning: .*? is deprecated: unseam:(\w+)")
DROP_ONE = {"-c", "-fsyntax-only", "-w"}
DROP_TWO = {"-o", "-MF", "-MT", "-MQ"}


def probe_command(entry, header):
    args = entry.get("arguments") or shlex.split(entry["command"])
    out = []
    i = 0
    while i < len(args):
        a = args[i]
        if a in DROP_TWO:
            i += 2
            continue
        if a in DROP_ONE or a in ("-MD", "-MMD") or a.startswith("-Werror") or a.startswith("-W"):
            i += 1
            continue
        out.append(a)
        i += 1
    out[1:1] = ["-fsyntax-only", "-Wno-everything", "-Wdeprecated-declarations", "-ferror-limit=0", "-include",
                header]
    return out


def probe_one(job):
    cmd, cwd = job
    r = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, errors="surrogateescape")
    hits = []
    for line in r.stderr.splitlines():
        m = DIAG.match(line)
        if m:
            path = os.path.realpath(os.path.join(cwd, m.group(1)))
            hits.append((path, int(m.group(2)), int(m.group(3)), m.group(4)))
    return hits, r.returncode


def run_probe(build, out_dir, jobs):
    with open(os.path.join(build, "compile_commands.json")) as f:
        entries = json.load(f)
    header = os.path.join(out_dir, "unseam-probe.hpp")
    with open(header, "w") as f:
        f.write(PROBE_HEADER)
    roots = (os.path.join(ROOT, "src") + os.sep, os.path.join(ROOT, "gen") + os.sep)
    work = [(probe_command(e, header), e["directory"]) for e in entries
            if os.path.realpath(e["file"]).startswith(roots)]
    kinds = collections.defaultdict(set)
    failed = 0
    with Pool(jobs) as pool:
        for hits, rc in pool.imap_unordered(probe_one, work, chunksize=4):
            failed += rc != 0
            for path, line, col, kind in hits:
                rel = os.path.relpath(path, ROOT)
                kinds[(rel, line, col)].add(kind)
    path = os.path.join(out_dir, "probe.tsv")
    with open(path, "w") as f:
        f.write("file\tline\tcol\tkind\n")
        for (rel, line, col), ks in sorted(kinds.items()):
            f.write("%s\t%d\t%d\t%s\n" % (rel, line, col, "exact" if ks == {"exact"} else "other"))
    print("probe: %d files compiled (%d with errors), %d uses typed -> %s" % (len(work), failed, len(kinds), path))
    return path


def read_probe(path):
    out = {}
    with open(path) as f:
        next(f)
        for line in f:
            rel, ln, col, kind = line.rstrip("\n").split("\t")
            out[(rel, int(ln), int(col))] = kind
    return out


# ---------------------------------------------------------------------------------------------------------------
# The tree

def files_of(paths):
    for p in paths:
        p = os.path.join(ROOT, p)
        if os.path.isfile(p):
            yield p
            continue
        for d, dirs, names in os.walk(p):
            dirs.sort()
            for n in sorted(names):
                if n.endswith(EXTENSIONS):
                    yield os.path.join(d, n)


def read(path):
    with open(path, "rb") as f:
        return f.read().decode("utf-8", "surrogateescape")


def write(path, text):
    with open(path, "wb") as f:
        f.write(text.encode("utf-8", "surrogateescape"))


ALIAS = re.compile(r"\busing\s+([A-Za-z_]\w*)\s*=|\btypedef\b[^;{}]*?\b([A-Za-z_]\w*)\s*;")
# The standard library's own aliases of class template instances, which src spells qualified (`std::string`) but
# does not declare: a destructor call cannot name them either (`s.~string()` finds no `string`).
STD_ALIASES = {
    "string", "wstring", "u8string", "u16string", "u32string", "string_view", "wstring_view", "u8string_view",
    "u16string_view", "u32string_view", "streambuf", "wstreambuf", "ios", "wios", "istream", "wistream", "ostream",
    "wostream", "iostream", "wiostream", "stringbuf", "wstringbuf", "istringstream", "wistringstream",
    "ostringstream", "wostringstream", "stringstream", "wstringstream", "filebuf", "wfilebuf", "ifstream",
    "wifstream", "ofstream", "wofstream", "fstream", "wfstream", "regex", "wregex",
}
DECL_LINE = re.compile(r"^\s*(?:template\s*<[^>]*>\s*)?(?:extern|static|inline|constexpr|const|FNVD_GLOBAL|"
                       r"FNVD_STATIC_MEMBER|FNVD_DEFINE_\w+|FNVD_VFTABLE|FNVD_ORIGINAL_FUNCTION)\b")


def unused(names, texts):
    """The names no line of the tree uses outside a declaration or definition line."""
    want = set(names)
    used = set()
    for text in texts:
        for line in text.splitlines():
            if DECL_LINE.match(line):
                continue
            for w in IDENT.findall(line):
                if w in want:
                    used.add(w)
    return sorted(want - used)


def macro_counts(texts):
    """FNVD_ macro uses in code, their mentions in comments, and fnvd:: helper uses in code."""
    counts = collections.Counter()
    mentions = collections.Counter()
    helpers = collections.Counter()
    for text in texts:
        code = []
        for t in tokenize(text):
            if t.kind == "comment":
                mentions.update(re.findall(r"\bFNVD_\w+", t.text))
                continue
            if t.kind == "ident" and t.text.startswith("FNVD_"):
                counts[t.text] += 1
            code.append(t.text)
        for m in re.finditer(r"\bfnvd :: (\w+(?: :: \w+)*)", " ".join(code)):
            helpers[m.group(1).replace(" ", "")] += 1
    return counts, mentions, helpers


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("group", choices=GROUPS + ("all", "probe", "report"))
    ap.add_argument("paths", nargs="*")
    ap.add_argument("--exported", action="store_true")
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--out", default="env/scratch/Clean/unseam")
    ap.add_argument("--build", default="build/linux")
    ap.add_argument("--probe")
    ap.add_argument("--jobs", type=int, default=8)
    o = ap.parse_args()
    out_dir = os.path.join(ROOT, o.out)
    os.makedirs(out_dir, exist_ok=True)
    paths = o.paths or ["src", "gen"]
    groups = GROUPS if o.group == "all" else (o.group,)

    if o.group == "report":
        texts = [read(p) for p in files_of(["src", "gen"])]
        counts, mentions, helpers = macro_counts(texts)
        with open(os.path.join(out_dir, "macros.tsv"), "w") as f:
            f.write("macro\tuses\tcomment mentions\n")
            for k in sorted(set(counts) | set(mentions), key=lambda k: (-counts[k], -mentions[k], k)):
                f.write("%s\t%d\t%d\n" % (k, counts[k], mentions[k]))
        with open(os.path.join(out_dir, "helpers.tsv"), "w") as f:
            f.write("helper\tuses\n")
            for k, v in helpers.most_common():
                f.write("fnvd::%s\t%d\n" % (k, v))
        print("report: %d FNVD_ macro uses (%d macros), %d fnvd:: helper uses (%d helpers) -> %s"
              % (sum(counts.values()), len(counts), sum(helpers.values()), len(helpers), out_dir))
        return 0
    if o.group == "probe":
        run_probe(os.path.join(ROOT, o.build), out_dir, o.jobs)
        return 0
    if set(groups) & ADDRESSED and not o.exported and not o.dry_run:
        print("unseam: %s drops exe addresses; run harness/tools/addresses.py first and pass --exported"
              % ", ".join(sorted(set(groups) & ADDRESSED)), file=sys.stderr)
        return 2

    if o.dry_run and "objects" in groups and len(groups) > 1 and not o.probe:
        # The probe compiles the tree on disk, which a dry run of the earlier groups leaves as it was.
        print("unseam: dry-run the objects group alone", file=sys.stderr)
        return 2

    all_files = list(files_of(paths))
    texts = {p: read(p) for p in all_files}
    aliases = set(STD_ALIASES)
    for p in files_of(["src", "gen"]):
        text = texts.get(p) or read(p)
        for m in ALIAS.finditer(text):
            aliases.add(m.group(1) or m.group(2))

    sites = []
    notes = []
    declared = []
    dropped = set()
    changed = set()
    line_maps = {}    # file -> the input line of each current line, once a group changed it
    inputs = {}       # file -> the input's lines, once a group changed it
    for g in groups:
        probe = {}
        if g == "objects":
            # Run now: the probe's lines and columns are those of the tree the earlier groups left.
            path = o.probe or run_probe(os.path.join(ROOT, o.build), out_dir, o.jobs)
            probe = read_probe(path)
        for p in all_files:
            rel = os.path.relpath(p, ROOT)
            w = FileWork(rel, texts[p], {g}, probe, aliases, line_maps.get(p), inputs.get(p)).run()
            new, back = w.result()
            sites.extend((rel, s) for s in w.sites)
            notes.extend((rel,) + n for n in w.notes)
            declared.extend(w.declared)
            dropped.update(n for n in w.dropped_names if n.startswith("RTTI_"))
            if new != texts[p]:
                if p not in inputs:
                    inputs[p] = texts[p].split("\n")
                line_maps[p] = [w.input_line(b) for b in back]
                texts[p] = new
                changed.add(p)
                if not o.dry_run:
                    write(p, new)

    head = "group\tmacro\tfile\tline\taction\treason\ttext\n"

    def row(rel, s):
        return "%s\t%s\t%s\t%d\t%s\t%s\t%s\n" % (s.group, s.macro, rel, s.line, s.action, s.reason,
                                               s.first.replace("\t", " "))

    with open(os.path.join(out_dir, "sites.tsv"), "w") as f, open(os.path.join(out_dir, "listed.tsv"), "w") as lf:
        f.write(head)
        lf.write(head)
        for rel, s in sites:
            f.write(row(rel, s))
            if s.action == "listed":
                lf.write(row(rel, s))
    by_kind = collections.defaultdict(list)
    for rel, s in sites:
        if s.action != "listed":
            by_kind[(s.macro, s.action)].append((rel, s))
    rng = random.Random(1)
    with open(os.path.join(out_dir, "sample.tsv"), "w") as f:
        f.write(head)
        for key in sorted(by_kind):
            rows = by_kind[key]
            for rel, s in sorted(rng.sample(rows, min(3, len(rows))), key=lambda x: (x[0], x[1].line)):
                f.write(row(rel, s))
    with open(os.path.join(out_dir, "notes.tsv"), "w") as f:
        f.write("file\tline\tentity\taction\ttext\n")
        for rel, line, entity, action, text in notes:
            f.write("%s\t%d\t%s\t%s\t%s\n" % (rel, line, entity.replace("\t", " ").replace("\n", " "), action,
                                             text.replace("\t", " ")))
    tree = [texts.get(p) or read(p) for p in files_of(["src", "gen"])]
    with open(os.path.join(out_dir, "unused.tsv"), "w") as f:
        f.write("kind\tname\n")
        for kind, names in (("vtable", [n for k, n in declared if k == "vtable"]),
                            ("original", [n for k, n in declared if k == "original"]),
                            ("rtti", sorted(dropped))):
            for n in unused(names, tree):
                f.write("%s\t%s\n" % (kind, n))

    tally = collections.Counter((s.macro, s.action) for _, s in sites)
    for macro in sorted({m for m, _ in tally}):
        print("%-36s %s" % (macro, "  ".join("%s %d" % (a, tally[(macro, a)])
                                              for a in ("deleted", "rewritten", "listed") if tally[(macro, a)])))
    print("unseam %s: %d sites, %d files changed%s -> %s" % (o.group, len(sites), len(changed),
                                                           " (dry run)" if o.dry_run else "", out_dir))
    return 0


if __name__ == "__main__":
    sys.exit(main())
