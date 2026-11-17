#!/usr/bin/env python3
"""De-inlining (docs/PLAN.md Part 2, step 7): where src writes a helper's body out, call the helper.

Usage: env/venv/bin/python harness/tools/deinline.py measure [--root DIR] [--addresses TSV] [--out DIR]
                                                             [--compile-commands JSON]
       env/venv/bin/python harness/tools/deinline.py rules [--root DIR]
       env/venv/bin/python harness/tools/deinline.py apply [--root DIR] [--addresses TSV] [--out DIR] [--outline]
                                                           [--compile-commands JSON] [--no-markers] [RULE...]
       env/venv/bin/python harness/tools/deinline.py gate --compare REPORT [--out DIR]

measure  one row per marker comment in src/ (`EXE-INLINED 0xADDR`, `EXE-INLINED none: HELPER`, `SWEEP-INLINE
         0xADDR`): the helper it names, the statement it covers (first and last line) and the class of that
         statement against the helper (below). Writes sites.tsv, helpers.tsv (per helper and class) and classes.txt
         into --out. An address is named through --addresses (harness/addresses.tsv, from `harness/tools/addresses.py
         export`), else the IDA function list env/cache/ida/functions.tsv. The assignments are clang's
         (clang-query over the files with an assignment under an `operator=` marker, with the flags of
         --compile-commands, by default the Linux build's of --root: configure it first).
rules    writes harness/tools/deinline/rules/ID.yml (ast-grep) from ruleset.py and checks each rule's helper
         definition against src/ of --root: a rule whose helper reads otherwise there is refused by apply.
apply    runs the rules of group `inline` (with --outline also `outline`; or only the RULEs named) over src/ of
         --root, each to a fixed point, taking with each statement a rule replaces the marker comment above it and
         any comment between that marker and the statement (it says how the statement wrote the helper out). A
         replacement of several lines takes the statement's indent, in the file's unit. Then it deletes the
         declaration of a local a rewrite left unused (one declarator on lines of its own, no initialiser with a
         call, increment or assignment; the others: locals.tsv), and the one-line markers whose class is call,
         trivial or implicit (not with --no-markers). Writes rewrites.tsv (file, original line, rule, helper,
         enclosing function, before, after, any other comment the rewrite took), comments.tsv (the comments left
         right above a rewritten statement, and those of the function's head that may describe the written-out form:
         a person reads them), apply.txt (counts, refused rules) and the measure of what is left (sites.tsv,
         helpers.tsv) into --out. A second run changes nothing.
gate     reads an `objsame.py compare --all` report of the rewritten tree's build against the build before, and
         writes gate.tsv into --out: per rewritten function, `same` (no symbol of its name differs: the compiler
         inlined the helper back) or `differs` (for the sampled review).

The classes of a marker's statement (sites.tsv `cls`):
  call       the statement already calls the helper (by name, also in an if/while/for condition; an assignment
             operator where clang calls it on an object of the helper's class, or by its name): only the marker is
             left
  trivial    the helper's out-of-line copy is a trivial accessor or an empty function (harness/tools/exefacts.py),
             which may be called or written out freely: only the marker is left
  implicit   the helper is a constructor, destructor or their Construct/Destruct statics and the statement declares
             an object of its class (not a pointer or reference), or the marker stands at the end of its scope or
             before a jump out of it: the C++ object runs it, only the marker is left
"""

from __future__ import annotations

import argparse
import collections
import csv
import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
RULES = os.path.join(ROOT, "harness", "tools", "deinline")
EXTENSIONS = (".h", ".hpp", ".inl", ".cpp", ".c", ".inc")
MARKER = re.compile(r"(EXE-INLINED|SWEEP-INLINE)\s*(0x[0-9A-Fa-f]+|none:\s*(.*))?")
AST_GREP = "/usr/bin/ast-grep"

# --------------------------------------------------------------------------------------------------------------------
# Tokens

TOKEN = re.compile(r"""
	(?P<comment>//[^\n]*|/\*.*?\*/)
	|(?P<pp>^[ \t]*\#(?:[^\n\\]|\\.|\\\n)*)
	|(?P<str>(?:u8|[uUL])?R"(?P<delim>[^(\s]*)\(.*?\)(?P=delim)"|(?:u8|[uUL])?"(?:[^"\\\n]|\\.)*"|(?:u8|[uUL])?'(?:[^'\\\n]|\\.)*')
	|(?P<num>\.?\d(?:[\w.]|[eEpP][+-])*)
	|(?P<id>[A-Za-z_]\w*)
	|(?P<punct>::|->\*?|\+\+|--|<<=|>>=|<=>|<<|>>|<=|>=|==|!=|&&|\|\||[-+*/%&|^]=|\.\.\.|\.\*|[^\s])
	""", re.S | re.M | re.X)


class Tok:
	__slots__ = ("kind", "text", "start", "end", "line")

	def __init__(self, kind, text, start, end, line):
		self.kind, self.text, self.start, self.end, self.line = kind, text, start, end, line

	def __repr__(self):
		return f"{self.text}@{self.line}"


def tokenize(text):
	out = []
	line, last = 1, 0
	for m in TOKEN.finditer(text):
		kind = m.lastgroup if m.lastgroup != "delim" else "str"
		if kind == "delim":
			kind = "str"
		line += text.count("\n", last, m.start())
		last = m.start()
		out.append(Tok(kind, m.group(0), m.start(), m.end(), line))
	return out


OPEN = {"(": ")", "[": "]", "{": "}"}


def close_of(code, i):
	"""The index of the bracket closing code[i] (code: tokens without comments and directives)."""
	depth = 0
	for j in range(i, len(code)):
		t = code[j].text
		if t in OPEN:
			depth += 1
		elif t in (")", "]", "}"):
			depth -= 1
			if depth == 0:
				return j
	return len(code) - 1


def statement_end(code, i):
	"""The index after the statement starting at code[i]."""
	if i >= len(code):
		return i
	t = code[i].text
	if t == "{":
		return close_of(code, i) + 1
	if t == "if":
		j = i + 1
		if j < len(code) and code[j].text == "constexpr":
			j += 1
		j = statement_end(code, close_of(code, j) + 1)
		if j < len(code) and code[j].text == "else":
			j = statement_end(code, j + 1)
		return j
	if t in ("for", "while", "switch"):
		return statement_end(code, close_of(code, i + 1) + 1)
	if t == "do":
		j = statement_end(code, i + 1)
		j = close_of(code, j + 1) + 1
		return j + 1
	if t in ("case", "default"):
		j = i
		while j < len(code) and code[j].text != ":":
			j += 1
		return j + 1
	if t == "else":
		return statement_end(code, i + 1)
	depth, assigned = 0, False
	j = i
	while j < len(code):
		x = code[j].text
		if x in OPEN:
			if x == "{" and depth == 0 and not assigned and j > i and (
					code[j - 1].text in (")", "const", "override", "noexcept", "final")):
				return close_of(code, j) + 1
			depth += 1
		elif x in (")", "]", "}"):
			depth -= 1
			if depth < 0:
				return j
		elif depth == 0 and x == ";":
			return j + 1
		elif depth == 0 and x == "=":
			assigned = True
		j += 1
	return j


# --------------------------------------------------------------------------------------------------------------------
# Names

def load_names(addresses):
	"""address -> (name, source): the src name of a registration, else IDA's."""
	prefer = ("function", "virtual", "member", "construct", "destruct", "constructor", "destructor",
			  "deleting destruct", "usercall", "userpurge", "kept")
	names = {}
	if addresses and os.path.exists(addresses):
		rank = {}
		for row in csv.DictReader(open(addresses, encoding="utf-8"), delimiter="\t"):
			kind = row["kind"]
			r = prefer.index(kind) if kind in prefer else (len(prefer) if kind.startswith("virtual") else None)
			if r is None:
				continue
			a = int(row["address"], 16)
			if a not in rank or r < rank[a]:
				rank[a] = r
				names[a] = (row["name"], "src")
	ida = os.path.join(ROOT, "env", "cache", "ida", "functions.tsv")
	if os.path.exists(ida):
		for line in open(ida, encoding="utf-8"):
			f = line.rstrip("\n").split("\t")
			a = int(f[0], 16)
			if a not in names and len(f) > 2:
				names[a] = (f[2], "ida")
	return names


QUALIFIED = re.compile(r"[~A-Za-z_][\w~]*(?:<(?:[^<>]|<[^<>]*>)*>)?(?:::(?:operator\s*(?:\(\)|\[\]|[^\s\w(]+)|[~A-Za-z_][\w~]*)"
					   r"(?:<(?:[^<>]|<[^<>]*>)*>)?)*")


def helper_of(text):
	"""The qualified name a `none:` marker's text starts with ('' for a description)."""
	text = text.strip()
	m = QUALIFIED.match(text)
	if not m:
		return ""
	name = m.group(0)
	rest = text[m.end():].lstrip()
	# a description starting with a common word ("the node's ...") is not a name
	if "::" not in name and not rest.startswith("(") and rest and not re.match(r"^[,.;]|$", rest) and \
			name[:1].islower() and name not in ("memset", "memcpy", "strlen", "strcmp", "memmove"):
		return ""
	return name


def short_name(name):
	"""The last component of a qualified name, without template arguments or an overload's parameters."""
	name = re.sub(r"\(.*$", "", name) if "operator()" not in name else name.split("operator()")[0] + "operator()"
	depth, cut = 0, []
	for ch in name:
		if ch == "<" and not cut[-8:] == list("operator"):
			depth += 1
		elif ch == ">" and depth:
			depth -= 1
		elif depth == 0:
			cut.append(ch)
	name = "".join(cut)
	last = name.split("::")[-1]
	return re.sub(r"_\d+$", "", last)


# --------------------------------------------------------------------------------------------------------------------
# Markers and their statements

def src_files(root):
	for dp, dn, fns in os.walk(os.path.join(root, "src")):
		dn.sort()
		for f in sorted(fns):
			if f.endswith(EXTENSIONS):
				yield os.path.join(dp, f)


def marker_sites(path, text, toks):
	"""(marker token, kind, key, note, statement tokens) per marker comment of the file."""
	code = [t for t in toks if t.kind not in ("comment", "pp")]
	starts = [t.start for t in code]
	import bisect
	out = []
	for t in toks:
		if t.kind != "comment":
			continue
		for m in MARKER.finditer(t.text):
			kind = m.group(1)
			key = m.group(2) or ""
			note = ""
			if key.startswith("none:"):
				note = (m.group(3) or "").strip().rstrip("*/").strip()
				key = "none"
			k = bisect.bisect_left(starts, t.start)
			trailing = k > 0 and code[k - 1].line == t.line and t.text.startswith("//")
			if trailing:
				# a comment after code on the same line covers the statement ending there
				b = k - 1
				while b > 0 and code[b - 1].text not in (";", "{", "}") :
					b -= 1
				span = code[b:k]
			else:
				e = statement_end(code, k)
				span = code[k:e]
			out.append((t, kind, key, note, span, trailing))
	return out


CALL_FREE = {"memset", "memcpy", "memmove", "strlen", "strcmp", "strcpy", "strcat"}


def classify(helper, span_text, span, assigned=None):
	"""call, body or other (rules are applied later and override). assigned: the file's class-type assignments
	(line -> class names, from class_assignments), which decide an assignment operator's call."""
	short = short_name(helper) if helper else ""
	if short and not short.startswith(("operator", "~")) and span and span[0].text in ("if", "while", "for",
																					   "switch") and len(span) > 1:
		# a compound statement: the call is looked for in its condition
		depth = 0
		for k, t in enumerate(span[1:], 1):
			depth += t.text == "("
			depth -= t.text == ")"
			if depth == 0:
				break
		head = span[2:k]
		for i, t in enumerate(head[:-1]):
			if t.text == short and head[i + 1].text in ("(", "<"):
				return "call"
	simple = span and span[0].text not in ("if", "for", "while", "do", "switch", "{", "using", "typedef") and \
		not span[0].text.startswith("FNVD_")
	idents = [t.text for t in span]
	if short and simple:
		if short.startswith("operator"):
			op = short[len("operator"):].strip()
			if op == "=":
				# an object of the helper's class assigned as a whole (clang's operator call on the statement's
				# lines), or the operator called by name: not a pointer or reference declared, not one member copied
				want = bare_class(helper_class(helper))
				for i, t in enumerate(span[:-2]):
					if t.text == "operator" and span[i + 1].text == "=" and span[i + 2].text == "(" and i > 1 and \
							span[i - 1].text == "::" and span[i - 2].text == want:
						return "call"
				lines = range(span[0].line, span[-1].line + 1)
				if want and assigned and any(want in assigned.get(n, ()) for n in lines):
					return "call"
			elif op in ("+", "-", "*", "/", "+=", "-=", "*=", "/=", "[]", "()", "==", "!="):
				if re.search(r"\.\s*[xyz]\b|\[\s*[0-3]\s*\]", span_text) is None and (op in idents or op in ("[]", "()")):
					return "call"
		elif short.startswith("~"):
			if re.search(r"~\s*" + re.escape(short[1:]) + r"\s*\(", span_text):
				return "call"
		else:
			for i, t in enumerate(span[:-1]):
				if t.text == short and span[i + 1].text in ("(", "<", "{"):
					return "call"
	if re.search(r"\bm_\w+|Interlocked\w*|NiAtomic\w+|DeleteThis|RefCount|DONT_DEALLOCATE|CAPACITY_MASK|_mm_\w+",
				 span_text):
		return "body"
	return "other"


def toplevel(span):
	out, depth = [], 0
	for t in span:
		if t.text in OPEN:
			depth += 1
		elif t.text in (")", "]", "}"):
			depth -= 1
		elif depth == 0:
			out.append(t.text)
	return out


def helper_class(helper):
	"""The class of an operator helper's qualified name (`hkArray<T>::operator=(...)` -> `hkArray<T>`)."""
	i = helper.rfind("::operator")
	return helper[:i] if i > 0 else ""


def bare_class(name):
	"""A type's class name: its last component, without template arguments, qualifiers or elaboration."""
	out, depth = [], 0
	for ch in name:
		if ch == "<":
			depth += 1
		elif ch == ">":
			depth -= 1
		elif depth == 0:
			out.append(ch)
	s = re.sub(r"\b(const|volatile|struct|class|union)\b", "", "".join(out)).strip()
	return s.split("::")[-1].strip()


CLANG_QUERY = "/usr/bin/clang-query"
ASSIGN_QUERY = 'match cxxOperatorCallExpr(hasOverloadedOperatorName("="), isExpansionInMainFile())'
ASSIGN_AT = re.compile(r'^(.*):(\d+):\d+: note: "root" binds here$')
NODE_TYPE = re.compile(r"^[|`\- ]*\w+ 0x[0-9a-f]+ (?:<.*?> )?(?:\w+ )*'([^']*)'(?::'([^']*)')?")


def class_assignments(root, rels, compile_commands, jobs=8):
	"""rel path -> {line: class names}: the assignments clang resolves to a class's operator= in each file, by the
	line the assignment starts on; the class is the object's type after any conversion to the operator's base. A
	file without a compile command (a header) is left out."""
	import concurrent.futures
	db = json.load(open(compile_commands, encoding="utf-8"))
	known = {os.path.realpath(os.path.join(e["directory"], e["file"])) for e in db}
	build = os.path.dirname(os.path.abspath(compile_commands))

	def one(rel):
		path = os.path.realpath(os.path.join(root, rel))
		if path not in known:
			return rel, None
		p = subprocess.run([CLANG_QUERY, "-p", build, "-c", "enable output diag", "-c", "enable output dump",
							"-c", ASSIGN_QUERY, path], capture_output=True, text=True, errors="surrogateescape")
		if p.returncode != 0 or re.search(r"\berror:", p.stderr):
			raise SystemExit(f"clang-query failed on {rel}: {p.stderr[-2000:]}")
		found = collections.defaultdict(set)
		lines = p.stdout.split("\n")
		at = None
		for i, l in enumerate(lines):
			m = ASSIGN_AT.match(l)
			if m:
				at = int(m.group(2))
				continue
			if not l.startswith("CXXOperatorCallExpr ") or at is None:
				continue
			# the call's direct children: the operator, the object, the argument
			kids = []
			for k in lines[i + 1:]:
				if not k.startswith(("|", "`", " ")):
					break
				if k.startswith(("|-", "`-")):
					kids.append(k)
			m = NODE_TYPE.match(kids[1]) if len(kids) > 1 else None
			if m:
				found[at].update(bare_class(t) for t in m.groups() if t)
			at = None
		return rel, dict(found)

	with concurrent.futures.ThreadPoolExecutor(jobs) as ex:
		return {rel: got for rel, got in ex.map(one, sorted(rels)) if got is not None}


def one_line(s, n=300):
	s = re.sub(r"\s+", " ", s).strip()
	return s if len(s) <= n else s[:n] + "..."


def measure(root, addresses, out, compile_commands, quiet=False):
	names = load_names(addresses)
	rows = []
	sites = []
	for path in src_files(root):
		text = open(path, encoding="utf-8", errors="surrogateescape").read()
		if "EXE-INLINED" not in text and "SWEEP-INLINE" not in text:
			continue
		toks = tokenize(text)
		rel = os.path.relpath(path, root)
		for t, kind, key, note, span, trailing in marker_sites(path, text, toks):
			if key.startswith("0x"):
				name, source = names.get(int(key, 16), ("", ""))
			elif key == "none":
				name, source = helper_of(note), "marker"
			else:
				name, source = "", ""
			span_text = text[span[0].start:span[-1].end] if span else ""
			sites.append((rel, t, kind, key, note, span, trailing, name, source, span_text))
	# an assignment operator's call is the compiler's to tell: the files with an assignment under such a marker
	asks = {s[0] for s in sites if short_name(s[7]) == "operator=" and "=" in toplevel(s[5])} if sites else set()
	assigned = class_assignments(root, asks, compile_commands) if asks else {}
	for rel, t, kind, key, note, span, trailing, name, source, span_text in sites:
		cls = classify(name, span_text, span, assigned.get(rel))
		if cls != "call" and key.startswith("0x"):
			cls = refine(int(key, 16), name, span_text, span, cls)
		rows.append(dict(file=rel, line=t.line, marker=kind, key=key, helper=name, source=source,
						 note=one_line(note, 120), first=span[0].line if span else 0,
						 last=span[-1].line if span else 0, trailing=int(trailing), cls=cls,
						 statement=one_line(span_text)))
	rule_classes(root, rows)
	os.makedirs(out, exist_ok=True)
	cols = ["file", "line", "marker", "key", "helper", "source", "note", "first", "last", "trailing", "cls",
			"statement"]
	write_tsv(os.path.join(out, "sites.tsv"), cols, rows)
	per = collections.defaultdict(collections.Counter)
	files = collections.defaultdict(set)
	for r in rows:
		k = (r["marker"], r["helper"] or (r["key"] if r["key"] != "none" else "none: " + r["note"][:60]))
		per[k][r["cls"]] += 1
		files[k].add(r["file"])
	classes = ["call", "trivial", "implicit", "body", "other"]
	with open(os.path.join(out, "helpers.tsv"), "w", encoding="utf-8") as f:
		f.write("marker\thelper\tsites\tfiles\t" + "\t".join(classes) + "\trules\n")
		for k, c in sorted(per.items(), key=lambda kv: -sum(kv[1].values())):
			ruled = ",".join(f"{x[5:]}:{n}" for x, n in sorted(c.items()) if x.startswith("rule:"))
			f.write(f"{k[0]}\t{k[1]}\t{sum(c.values())}\t{len(files[k])}\t" +
					"\t".join(str(c[x]) for x in classes) + f"\t{ruled}\n")
	tot = collections.Counter((r["marker"], r["cls"] if not r["cls"].startswith("rule:") else "rule") for r in rows)
	with open(os.path.join(out, "classes.txt"), "w") as f:
		for k, v in sorted(tot.items()):
			f.write(f"{k[0]}\t{k[1]}\t{v}\n")
		f.write(f"total\t{len(rows)}\n")
	if not quiet:
		print(open(os.path.join(out, "classes.txt")).read(), end="")
	return rows


_FACTS = {}


def facts(addr):
	"""(trivial accessor, empty function) of the helper's out-of-line copy, from harness/tools/exefacts.py."""
	if addr not in _FACTS:
		try:
			sys.path.insert(0, os.path.join(ROOT, "harness", "tools"))
			import exefacts
			_FACTS[addr] = (exefacts.trivial_accessor(addr) is not None, exefacts.empty_function(addr) is not None)
		except Exception:
			_FACTS[addr] = (False, False)
	return _FACTS[addr]


# a declaration of one object (not a pointer or reference to one): its type, then its name, then any initialiser
OBJECT_DECL = re.compile(r"^(?:const\s+)?[\w:]+(?:<[^;=(){}]*>\s*|\s+)\w+\s*(?:\(.*\)|\{.*\}|=.*)?;$", re.S)


def refine(addr, helper, span_text, span, cls):
	"""trivial: the helper's copy is a trivial accessor or an empty function, which src may call or write out
	freely; implicit: the helper is a constructor or destructor the C++ object runs by itself (the statement
	declares the object, or the marker stands at the end of its scope)."""
	acc, empty = facts(addr)
	if acc or empty:
		return "trivial"
	short = short_name(helper)
	cls_name = helper.split("::")[-2] if helper.count("::") else ""
	cls_name = re.sub(r"<.*", "", cls_name)
	if short in ("Construct", "Destruct") or short.startswith("~") or (cls_name and short == cls_name):
		# the object's own declaration (its type names the class), the end of its scope, or a jump out of it
		if not span or span[0].text in ("return", "break", "continue", "goto") and short in ("Destruct",) or (
				OBJECT_DECL.match(span_text.strip()) and cls_name and
				re.match(r"^(?:const\s+)?(?:\w+::)*" + re.escape(cls_name) + r"\b", span_text.strip())):
			return "implicit"
	return cls


def rule_classes(root, rows):
	"""The rules' matches over the marker sites: a marker whose statement lies in a rule's match (or right above
	it) gets class rule:ID. Returns nothing: rows are updated in place."""
	rs = load_ruleset()
	rules_dir = write_rules(rs)
	files = {}
	for r in rows:
		files.setdefault(r["file"], []).append(r)
	hits = collections.defaultdict(list)  # file -> (first line, last line, rule)
	cfg = os.path.join(RULES, "sgconfig.yml")
	p = subprocess.run([AST_GREP, "scan", "-c", cfg, "--json=stream", "src"], cwd=root, capture_output=True,
					   text=True)
	for line in p.stdout.splitlines():
		m = json.loads(line)
		rid = m["ruleId"].split(".")[0]
		if rid not in rs.RULES:
			continue
		hits[m["file"]].append((m["range"]["start"]["line"] + 1, m["range"]["end"]["line"] + 1, rid))
	for f, rs_ in files.items():
		for r in rs_:
			a, b = (r["first"], r["last"]) if r["first"] else (r["line"], r["line"])
			for s, e, rid in hits.get(f, ()):
				if s <= a and b <= e or s <= r["line"] <= e:
					r["cls"] = "rule:" + rid
					break


# --------------------------------------------------------------------------------------------------------------------
# Rules

def load_ruleset():
	import importlib.util
	spec = importlib.util.spec_from_file_location("deinline_ruleset", os.path.join(RULES, "ruleset.py"))
	mod = importlib.util.module_from_spec(spec)
	spec.loader.exec_module(mod)
	return mod


def emit_yaml(v, ind):
	pad = "  " * ind
	if isinstance(v, dict):
		out = ""
		for k, x in v.items():
			out += f"\n{pad}{k}:" + emit_yaml(x, ind + 1)
		return out or " {}"
	if isinstance(v, list):
		return "".join(f"\n{pad}-" + emit_yaml(x, ind + 1) for x in v) or " []"
	return " " + json.dumps(v)


def write_rules(rs):
	"""One ast-grep rule file per rule (harness/tools/deinline/rules/ID.yml), from ruleset.py."""
	out = os.path.join(RULES, "rules")
	os.makedirs(out, exist_ok=True)
	for name in os.listdir(out):
		if name.endswith(".yml") and name[:-4] not in rs.RULES:
			os.remove(os.path.join(out, name))
	for id, r in rs.RULES.items():
		parts = []
		for d in r["docs"]:
			doc = {"id": d["id"], "language": "cpp", "message": r["note"], "severity": "hint"}
			if r["files"]:
				doc["files"] = r["files"]
			doc["rule"] = d["rule"]
			if r["constraints"]:
				doc["constraints"] = r["constraints"]
			if r["transform"] and d["id"] == id:
				doc["transform"] = r["transform"]
			if d["fix"] is not None:
				doc["fix"] = d["fix"]
			parts.append(emit_yaml(doc, 0).lstrip("\n"))
		text = "# Generated by harness/tools/deinline.py rules from ruleset.py; edit there.\n" + \
			"\n---\n".join(parts) + "\n"
		path = os.path.join(out, id + ".yml")
		if not os.path.exists(path) or open(path).read() != text:
			open(path, "w").write(text)
	return out


def check_rules(rs, root):
	"""rule id -> None when its helper's definition in root reads as the rule quotes it, else why not."""
	return {id: (r["authority"].check(root) if r["authority"] else None) for id, r in rs.RULES.items()}


def scan(root, rule_file, paths):
	"""ast-grep's matches of one rule file over paths (relative to root)."""
	cfg = os.path.join(RULES, "sgconfig.yml")
	p = subprocess.run([AST_GREP, "scan", "-c", cfg, "-r", rule_file, "--json=stream", *paths], cwd=root,
					   capture_output=True, text=True)
	if p.returncode not in (0, 1) and not p.stdout:
		raise SystemExit(f"ast-grep failed on {rule_file}: {p.stderr[-2000:]}")
	return [json.loads(line) for line in p.stdout.splitlines() if line.strip()]


# --------------------------------------------------------------------------------------------------------------------
# Functions of a file (for the gate: the function each rewrite lands in)

def functions(text):
	"""(start, end, qualified name) of each function body in the text (offsets of its braces)."""
	toks = [t for t in tokenize(text) if t.kind not in ("comment", "pp")]
	out = []
	scope = []  # (kind, name, close index)
	i = 0
	n = len(toks)
	while i < n:
		t = toks[i]
		while scope and i > scope[-1][2]:
			scope.pop()
		if t.text in ("class", "struct", "union", "namespace") and i + 1 < n:
			# a definition: name ... {  (not a forward declaration or an elaborated type)
			j = i + 1
			name = ""
			while j < n and toks[j].text not in ("{", ";", "(", ")", "=", ","):
				if toks[j].kind == "id" and not name and toks[j].text not in ("final", "alignas"):
					name = toks[j].text
				if toks[j].text == ":" and t.text != "namespace":
					# base list
					while j < n and toks[j].text not in ("{", ";"):
						j += 1
					break
				j += 1
			if j < n and toks[j].text == "{":
				e = close_of(toks, j)
				scope.append((t.text, name, e))
				i = j + 1
				continue
		if t.text == "{" and i > 0 and toks[i - 1].text in (")", "const", "override", "noexcept", "final") and (
				not scope or scope[-1][0] != "function"):
			# the name: the identifiers before the matching '(' of the parameter list
			k = i - 1
			while k > 0 and toks[k].text != ")":
				k -= 1
			depth = 0
			while k > 0:
				if toks[k].text == ")":
					depth += 1
				elif toks[k].text == "(":
					depth -= 1
					if depth == 0:
						break
				k -= 1
			parts = []
			m = k - 1
			while m >= 0 and (toks[m].kind == "id" or toks[m].text in ("::", "~", "operator") or
							  (parts and parts[0] == "operator")):
				if toks[m].kind not in ("id",) and toks[m].text not in ("::", "~"):
					if parts and parts[0] == "operator":
						break
				parts.insert(0, toks[m].text)
				m -= 1
				if m >= 0 and toks[m].kind == "id" and parts[0] not in ("::", "~") and toks[m + 1].kind == "id":
					break
			while parts and parts[0] == "::":
				parts.pop(0)
			name = "".join(parts)
			if name and name not in ("if", "for", "while", "switch", "catch"):
				outer = [s[1] for s in scope if s[0] in ("class", "struct", "union") and s[1]]
				qual = "::".join(outer + [name])
				e = close_of(toks, i)
				out.append((toks[i].start, toks[e].end, qual))
				scope.append(("function", qual, e))
				i += 1
				continue
		i += 1
	return out


def function_at(funcs, off):
	best = ""
	for s, e, q in funcs:
		if s <= off < e:
			best = q
	return best


# --------------------------------------------------------------------------------------------------------------------
# Apply

class Text:
	"""A file's text being edited, with each current line's line in the original."""

	def __init__(self, text):
		self.text = text
		self.orig = list(range(1, text.count("\n") + 2))

	def line_of(self, off):
		return self.orig[self.text.count("\n", 0, off)]

	def edit(self, start, end, new):
		"""Replace [start, end); an empty replacement that leaves its line blank takes the line away."""
		t = self.text
		if new == "":
			ls = t.rfind("\n", 0, start) + 1
			le = t.find("\n", end)
			le = len(t) if le < 0 else le
			if t[ls:start].strip() == "" and t[end:le].strip() == "":
				start, end = ls, min(le + 1, len(t))
		a = t.count("\n", 0, start)
		b = t.count("\n", 0, end)
		k = new.count("\n")
		self.orig[a:b + 1] = [self.orig[a]] * (k + 1) if a < len(self.orig) else []
		self.text = t[:start] + new + t[end:]


def indent_unit(text, indent):
	"""The indentation step for a statement at indent: a tab after a tab, the file's space step after spaces (the
	commonest step between the leading spaces of successive lines), and at no indent the file's usual one."""
	tabs = spaces = prev = 0
	steps = collections.Counter()
	for line in text.split("\n"):
		if not line.strip():
			continue
		lead = line[:len(line) - len(line.lstrip(" \t"))]
		if lead.startswith("\t"):
			tabs += 1
			continue
		spaces += bool(lead)
		if len(lead) > prev:
			steps[len(lead) - prev] += 1
		prev = len(lead)
	space = " " * (steps.most_common(1)[0][0] if steps else 4)
	if "\t" in indent or not indent and tabs >= spaces:
		return "\t"
	return space


def reindent(new, template, indent, unit):
	"""A replacement of several lines laid out as its fix template is (one tab per level there), from the indent of
	the statement it replaces, in the file's own unit."""
	lines, tl = new.split("\n"), (template or "").split("\n")
	if len(lines) < 2 or len(lines) != len(tl):
		return new
	out = [lines[0]]
	for line, t in zip(lines[1:], tl[1:]):
		out.append(indent + unit * (len(t) - len(t.lstrip("\t"))) + line.lstrip(" \t") if line.strip() else "")
	return "\n".join(out)


def line_start(text, off):
	return text.rfind("\n", 0, off) + 1


OTHER_MARKER = re.compile(r"EXE-CALLED|EXE-CAST")


def comments_above(text, off):
	"""The whole-line `//` comments right above the line of off: [(offset of the line, its text)], top first."""
	out = []
	ls = line_start(text, off)
	while ls > 0:
		ps = line_start(text, ls - 1)
		line = text[ps:ls - 1]
		if not line.lstrip().startswith("//"):
			break
		out.insert(0, (ps, line.strip()))
		ls = ps
	return out


def between_marker(block):
	"""The index in block (comments_above) of the topmost marker with plain comments below it, between it and the
	statement, else None: those comments say how the statement wrote the helper out, and go with it. Never when
	another kind of marker is among them."""
	for i, (_, line) in enumerate(block):
		if MARKER.search(line):
			rest = block[i + 1:]
			if any(not MARKER.search(c) for _, c in rest) and not any(OTHER_MARKER.search(c) for _, c in rest):
				return i
			return None
	return None


# the words of a comment that may describe a written-out helper (comments.tsv lists them for a person)
CUE = re.compile(r"inlin|written out|by hand|\braw\b|\bcasts?\b|untyped|hence|assignment|HK_TIMER|HK_MONITOR|"
				 r"monitor|timer|rdtsc|m_end|1[26][- ]byte|reference|RefCount|Interlocked|NiPointer|smart pointer|"
				 r"SetAt|SetSize|NiFixedString|handle", re.I)


def head_comment(text, off):
	"""The comment block above the definition whose body opens at off: the lines above its signature."""
	ls = line_start(text, off)
	while ls > 0:
		ps = line_start(text, ls - 1)
		line = text[ps:ls - 1].strip()
		if not line or line.startswith(("//", "#")) or line.endswith((";", "}")):
			break
		ls = ps
	return comments_above(text, ls)


def enclosing_comments(text, body, off):
	"""The comment blocks above the statements that enclose off inside the function body [start, end): a loop's,
	a branch's, a lambda's declaration."""
	start, end = body
	code = [t for t in tokenize(text[start:end]) if t.kind not in ("comment", "pp")]
	out, opened = [], []
	for i, t in enumerate(code):
		if t.text == "{":
			opened.append(i)
		elif t.text == "}" and opened:
			o = opened.pop()
			if o > 0 and start + code[o].start < off < start + t.start:
				b = o
				while b > 0 and code[b - 1].text not in (";", "{", "}"):
					b -= 1
				out = comments_above(text, start + code[b].start) + out
	return out


def apply_rule(root, files, rid, rule_file, rows, rs, notes, vanished, paths, touched):
	"""Apply one rule's matches in paths to the Text objects in files (path -> Text), until it matches no more.
	notes gets the comments left beside a rewrite (comments.tsv), vanished the names a rewrite took out of a
	function, touched the files it changed."""
	fixes = {d["id"]: d["fix"] for d in rs.RULES[rid]["docs"]}
	for _ in range(5):
		ms = scan(root, rule_file, paths)
		if not ms:
			return
		by = collections.defaultdict(list)
		for m in ms:
			by[m["file"]].append(m)
		changed = False
		for f, lst in by.items():
			if f not in files:
				files[f] = Text(open(os.path.join(root, f), encoding="utf-8", errors="surrogateescape").read())
			tx = files[f]
			old = tx.text
			units = {}
			# the file on disk is what ast-grep read: offsets are byte offsets into it
			raw = open(os.path.join(root, f), "rb").read()
			edits = []
			for m in lst:
				s, e = m["range"]["byteOffset"]["start"], m["range"]["byteOffset"]["end"]
				s, e = len(raw[:s].decode("utf-8", "surrogateescape")), len(raw[:e].decode("utf-8", "surrogateescape"))
				new = m.get("replacement", "")
				ls = line_start(tx.text, s)
				lead = tx.text[ls:s]
				indent = lead if not lead.strip() else lead[:len(lead) - len(lead.lstrip(" \t"))]
				if indent not in units:
					units[indent] = indent_unit(tx.text, indent)
				new = reindent(new, fixes.get(m["ruleId"]), indent, units[indent])
				start = s
				if m["ruleId"] == rid or m["ruleId"].startswith(rid + ".del"):
					block = comments_above(tx.text, s) if not lead.strip() else []
					top = between_marker(block)
					if top is not None:
						# the marker and the comments under it go with the statement
						start = block[top][0]
						new = new and lead + new
						block = block[:top]
					if m["ruleId"] == rid:
						m["notes"] = [(o, c, "above") for o, c in block if not MARKER.search(c)]
				edits.append((start, e, s, new, m))
			edits.sort(key=lambda x: (x[0], -x[1]))
			kept, last = [], -1
			for x in edits:
				if x[0] < last:
					continue  # inside an edit already taken: the next round sees it again
				kept.append(x)
				last = x[1]
			funcs = functions(tx.text)
			for start, e, s, new, m in reversed(kept):
				inner = [x for x in funcs if x[0] <= s < x[1]]
				fn = inner[-1][2] if inner else ""
				if m["ruleId"] == rid:
					cues = m.get("notes", []) + [(o, c, "head") for o, c in (head_comment(tx.text, inner[-1][0])
																		   if inner else [])]
					cues += [(o, c, "enclosing") for o, c in (enclosing_comments(tx.text, inner[-1][:2], s)
																if inner else [])]
					cues = [x for x in cues if x[2] == "above" or CUE.search(x[1]) and not MARKER.search(x[1])]
					for o, c, where in cues:
						notes[(f, tx.line_of(o))] = dict(file=f, line=tx.line_of(o), where=where, rule=rid,
														  function=fn, rewrite=tx.line_of(s), comment=c)
					rows.append(dict(file=f, line=tx.line_of(s), rule=rid,
									 helper=rs.RULES[rid]["authority"].helper, function=fn,
									 before=one_line(tx.text[s:e], 400), after=one_line(new, 200),
									 comments="; ".join(one_line(c.text, 80) for c in tokenize(tx.text[start:e])
														if c.kind == "comment" and not MARKER.search(c.text))))
				if inner:
					# the function by its first line in the tree before (the edits leave it where it was)
					gone = {t.text for t in tokenize(tx.text[start:e]) if t.kind == "id"} - \
						{t.text for t in tokenize(new) if t.kind == "id"}
					vanished[(f, tx.line_of(inner[-1][0]))] |= gone
				tx.edit(start, e, new)
			if tx.text != old:
				changed = True
				touched.add(f)
			open(os.path.join(root, f), "w", encoding="utf-8", errors="surrogateescape").write(tx.text)
		if not changed:
			return


def drop_unused_locals(root, files, vanished, rows, left):
	"""Delete the declarations of locals a rewrite left unused: a name the rewrite took out of a function that is
	now named only by its own declaration, one declarator on lines of its own, with no initialiser or one without
	a call, an increment or an assignment. Any other such declaration goes to left (locals.tsv)."""
	byfile = collections.defaultdict(dict)
	for (f, first), names in vanished.items():
		byfile[f][first] = names
	n = 0
	for f, fns in sorted(byfile.items()):
		tx = files[f]
		dels = []
		for s, e, q in functions(tx.text):
			names = fns.get(tx.line_of(s))
			if not names:
				continue
			code = [t for t in tokenize(tx.text[s:e]) if t.kind not in ("comment", "pp")]
			for name in sorted(names):
				at = [k for k, t in enumerate(code) if t.text == name]
				if len(at) != 1:
					continue
				k = at[0]
				b = k
				while b > 0 and code[b - 1].text not in (";", "{", "}"):
					b -= 1
				j = k
				while j < len(code) and code[j].text != ";":
					j += 1
				head, init = code[b:k], code[k + 1:j]
				if not head or not all(t.kind == "id" or t.text in ("::", "<", ">", "*", "&", ",") for t in head) or \
						head[0].text in ("return", "delete", "goto", "case", "throw", "else", "do") or \
						head[-1].text in ("::", ",") or init and init[0].text not in ("=", "{"):
					continue  # not a declaration of it
				why = ""
				if head[0].text in ("static", "extern", "thread_local"):
					why = "static or extern"
				elif any(t.text == "," for t in head + init):
					why = "more than one declarator"
				elif any(t.text in ("(", "++", "--", "new", "delete") or t.text.endswith("=") and t.text not in (
						"==", "!=", "<=", ">=") for t in init[1:]):
					why = "initialiser with a call, increment or assignment"
				ds, de = s + code[b].start, s + code[j].end if j < len(code) else e
				ls = line_start(tx.text, ds)
				le = tx.text.find("\n", de)
				le = len(tx.text) if le < 0 else le
				if not why and (tx.text[ls:ds].strip() or tx.text[de:le].strip()):
					why = "not on lines of its own"
				if why:
					left.append(dict(file=f, line=tx.line_of(ds), function=q, name=name, why=why,
									 declaration=one_line(tx.text[ds:de])))
					continue
				dels.append((ls, min(le + 1, len(tx.text)), q, name))
		for ls, le, q, name in sorted(dels, reverse=True):
			rows.append(dict(file=f, line=tx.line_of(ls), rule="unused-local", helper="", function=q,
							 before=one_line(tx.text[ls:le]), after="", comments=""))
			tx.edit(ls, le, "")
			n += 1
		open(os.path.join(root, f), "w", encoding="utf-8", errors="surrogateescape").write(tx.text)
	return n


MARKER_LINE = re.compile(r"^[ \t]*//[ \t]*(EXE-INLINED|SWEEP-INLINE)\b[^\n]*\n")


def delete_markers(root, sites, classes, rows):
	"""Delete the whole-line marker comments of the sites whose class is in classes (a marker comment that only
	names the helper; one that goes on to explain stays for a person)."""
	by = collections.defaultdict(set)
	for r in sites:
		if r["cls"] in classes and r["trailing"] == "0":
			by[r["file"]].add(int(r["line"]))
	n = 0
	for f, lines in by.items():
		path = os.path.join(root, f)
		text = open(path, encoding="utf-8", errors="surrogateescape").read().split("\n")
		cls = {int(r["line"]): r for r in sites if r["file"] == f}
		out = []
		for i, l in enumerate(text, 1):
			nxt_comment = i < len(text) and re.match(r"^\s*//", text[i]) and not MARKER.search(text[i])
			if i in lines and re.match(r"^\s*//\s*(EXE-INLINED|SWEEP-INLINE)\s+(0x[0-9A-Fa-f]+|none:\s*"
									   r"[\w:<>,* ~&=+\-\[\]()]+)\s*\.?\s*$", l) or (
					i in lines and not nxt_comment and
					re.match(r"^\s*//\s*SWEEP-INLINE\s+0x[0-9A-Fa-f]+: the exe calls [^\n]*$", l)):
				r = cls[i]
				rows.append(dict(file=f, line=i, rule="marker:" + r["cls"], helper=r["helper"],
								 function="", before=l.strip(), after="", comments=""))
				n += 1
				continue
			out.append(l)
		open(path, "w", encoding="utf-8", errors="surrogateescape").write("\n".join(out))
	return n


def read_tsv(path):
	return list(csv.DictReader(open(path, encoding="utf-8"), delimiter="\t", quoting=csv.QUOTE_NONE,
							   escapechar="\\"))


def write_tsv(path, cols, rows):
	with open(path, "w", encoding="utf-8") as f:
		w = csv.writer(f, delimiter="\t", lineterminator="\n", quoting=csv.QUOTE_NONE, escapechar="\\")
		w.writerow(cols)
		for r in rows:
			w.writerow([str(r.get(c, "")).replace("\t", " ").replace("\n", " ") for c in cols])


def apply(root, out, groups, only, addresses, marker_classes, compile_commands):
	rs = load_ruleset()
	rules_dir = write_rules(rs)
	status = check_rules(rs, root)
	rows = []
	refused = []
	files = {}
	notes, vanished, left = {}, collections.defaultdict(set), []
	todo = []
	for rid, r in rs.RULES.items():
		if only and rid not in only:
			continue
		if r["group"] not in groups or not any(d["fix"] is not None for d in r["docs"]):
			continue
		if status[rid]:
			refused.append((rid, status[rid]))
			continue
		todo.append(rid)
	# again over the files a pass changed, until one changes nothing: a rule refused while a later statement
	# still used its names may match once a later rule has rewritten that statement
	paths = ["src"]
	while paths:
		touched = set()
		for rid in todo:
			apply_rule(root, files, rid, os.path.join(rules_dir, rid + ".yml"), rows, rs, notes, vanished, paths,
					   touched)
		paths = sorted(touched)
	nlocal = drop_unused_locals(root, files, vanished, rows, left)
	# the markers left above statements that already call their helper
	sites = measure(root, addresses, out, compile_commands, quiet=True)
	srows = [{k: str(v) for k, v in r.items()} for r in sites]
	nmark = delete_markers(root, srows, marker_classes, rows) if marker_classes else 0
	cols = ["file", "line", "rule", "helper", "function", "before", "after", "comments"]
	write_tsv(os.path.join(out, "rewrites.tsv"), cols, rows)
	write_tsv(os.path.join(out, "comments.tsv"), ["file", "line", "where", "rule", "function", "rewrite", "comment"],
			  sorted(notes.values(), key=lambda r: (r["file"], r["line"])))
	write_tsv(os.path.join(out, "locals.tsv"), ["file", "line", "function", "name", "why", "declaration"], left)
	sites = measure(root, addresses, out, compile_commands, quiet=True)
	per = collections.Counter(r["rule"] for r in rows)
	with open(os.path.join(out, "apply.txt"), "w") as f:
		for k, v in sorted(per.items(), key=lambda kv: -kv[1]):
			f.write(f"{k}\t{v}\n")
		for rid, why in refused:
			f.write(f"refused\t{rid}\t{why}\n")
	print(open(os.path.join(out, "apply.txt")).read(), end="")
	print(f"{sum(v for k, v in per.items() if not k.startswith('marker:') and k != 'unused-local')} rewrites, "
		  f"{nlocal} unused locals deleted ({len(left)} left: locals.tsv), {nmark} markers deleted, "
		  f"{len(notes)} comments beside rewrites (comments.tsv), {len(sites)} marker sites left")


# --------------------------------------------------------------------------------------------------------------------
# Gate

def gate(root, out, report):
	"""gate.tsv: per rewritten function, same or differs in the object gate's report."""
	rows = read_tsv(os.path.join(out, "rewrites.tsv"))
	diff = collections.defaultdict(list)  # object source -> demangled names that differ
	for line in open(report, encoding="utf-8"):
		f = line.rstrip("\n").split("\t")
		if len(f) < 3 or f[0] not in ("code", "data", "gone", "new", "renamed"):
			continue
		m = re.search(r"\((.*)\)$", f[2])
		name = m.group(1) if m else f[2]
		for obj in f[1].split(","):
			diff[obj.strip()].append((f[0] + (f" ({f[3]})" if len(f) > 3 else ""), name))
	allnames = [(k, n) for v in diff.values() for k, n in v]
	res = []
	seen, taken = set(), set()
	for r in rows:
		if r["rule"].startswith("marker:"):
			continue
		key = (r["file"], r["function"])
		if key in seen:
			continue
		seen.add(key)
		fn = r["function"]
		cands = [(k, n) for obj, v in diff.items() if r["file"] in obj for k, n in v] \
			if r["file"].endswith((".cpp", ".c")) else allnames
		hit = [f"{k}: {n}" for k, n in cands if fn and re.search(r"(^|[\s:*&])" + re.escape(fn) + r"(\(|$|\[)", n)]
		taken.update(h.split(": ", 1)[1] for h in hit)
		res.append(dict(file=r["file"], function=fn, rules=",".join(sorted({x["rule"] for x in rows if
																		   (x["file"], x["function"]) == key})),
						verdict="differs" if hit else "same", detail="; ".join(hit)[:400]))
	# a symbol that differs though no rewrite is in it: a rewritten function the compiler inlined into it
	for obj, v in sorted(diff.items()):
		for k, n in v:
			if n not in taken:
				taken.add(n)
				res.append(dict(file=obj, function=n, rules="", verdict="caller", detail=f"{k}: a rewritten "
								"function in the same object is inlined into it"))
	write_tsv(os.path.join(out, "gate.tsv"), ["file", "function", "rules", "verdict", "detail"], res)
	c = collections.Counter(r["verdict"] for r in res)
	print(f"{c['same'] + c['differs']} rewritten functions: {c['same']} same, {c['differs']} differ; "
		  f"{c['caller']} other symbols differ (callers the rewritten functions are inlined into)")


def main():
	ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
	sub = ap.add_subparsers(dest="cmd", required=True)
	out_default = os.path.join(ROOT, "env", "scratch", "Clean", "7")
	m = sub.add_parser("measure")
	m.add_argument("--root", default=ROOT)
	m.add_argument("--addresses", default=os.path.join(ROOT, "harness", "addresses.tsv"))
	m.add_argument("--out", default=out_default)
	m.add_argument("--compile-commands", help="the Linux build's (default: ROOT/build/linux/compile_commands.json)")
	r = sub.add_parser("rules")
	r.add_argument("--root", default=ROOT)
	a = sub.add_parser("apply")
	a.add_argument("--root", default=ROOT)
	a.add_argument("--addresses", default=os.path.join(ROOT, "harness", "addresses.tsv"))
	a.add_argument("--out", default=out_default)
	a.add_argument("--compile-commands", help="the Linux build's (default: ROOT/build/linux/compile_commands.json)")
	a.add_argument("--outline", action="store_true", help="also the rules whose helper is defined in a .cpp")
	a.add_argument("--no-markers", action="store_true", help="leave the markers of the `call` class")
	a.add_argument("rule", nargs="*")
	g = sub.add_parser("gate")
	g.add_argument("--root", default=ROOT)
	g.add_argument("--out", default=out_default)
	g.add_argument("--compare", required=True)
	args = ap.parse_args()
	root = os.path.abspath(args.root)
	cc = getattr(args, "compile_commands", None) or os.path.join(root, "build", "linux", "compile_commands.json")
	if args.cmd in ("measure", "apply") and not os.path.exists(cc):
		sys.exit(f"deinline: no {cc}: configure the Linux build first (noexe.py build --target linux)")
	if args.cmd == "measure":
		measure(root, args.addresses, args.out, cc)
	elif args.cmd == "rules":
		rs = load_ruleset()
		print("rules:", write_rules(rs))
		for rid, why in check_rules(rs, root).items():
			print(f"{rid}\t{rs.RULES[rid]['group']}\t{why or 'ok'}")
	elif args.cmd == "apply":
		groups = {"inline"} | ({"outline"} if args.outline else set())
		os.makedirs(args.out, exist_ok=True)
		apply(root, args.out, groups, set(args.rule), args.addresses,
			  set() if args.no_markers else {"call", "trivial", "implicit"}, cc)
	elif args.cmd == "gate":
		gate(root, args.out, args.compare)


if __name__ == "__main__":
	main()
