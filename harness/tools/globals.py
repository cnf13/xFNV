#!/usr/bin/env python3
"""Globals together, and defined in headers where they can be (docs/PLAN.md Part 2, step 5).

Usage: env/venv/bin/python harness/tools/globals.py measure [--root DIR] [--compile-commands FILE] [--out DIR]
                                                            [--jobs N] [--srcdb BIN]
       env/venv/bin/python harness/tools/globals.py apply [--root DIR] [--out DIR] [--dry-run]

  measure  parses every src/ TU of the build (--compile-commands, default ROOT/build/linux/compile_commands.json:
           build it first) and one TU of every src/ header with `srcdb globals-tu` (--srcdb, default this checkout's
           build/srcdb/srcdb: harness/srcdb), and writes OUT/model.json.gz (default OUT env/scratch/Clean/globals),
           then decides: OUT/sites.tsv, OUT/blocks.tsv, OUT/listed.tsv, OUT/report.txt (the counts).
  apply    decides again from the model and edits the tree. The model must still describe it (every file it would
           edit unchanged since measure); a second run after a measure changes nothing. --dry-run writes the tables
           only.

What it does, for every variable with static storage that src/ (not src/SDK/) defines outside a function:
  1. A constant-initialised one (the compiler's verdict: its initialiser is a constant expression, or it has none
     and is zero-initialised, and its type needs no destruction) that one header declares: the definition replaces
     that declaration, `inline` (`inline constexpr` for a `const` of a literal, non-pointer type), and leaves its
     .cpp. A static data member's in-class declaration gets the initialiser (`static inline`, `static constexpr`).
  2. The definitions a .cpp keeps (dynamic initialisers, destructors, the variables no header declares, file-local
     ones) form one block in each namespace scope, before its first function definition, in their present order.
  3. The variable declarations of a header's namespace scope (extern, inline, static, constexpr) form one block
     after its last class definition (else where the first of them is), in their present order.
A declaration moves with the comment lines right above it and a comment after it on its line.

What is not moved, and listed in OUT/listed.tsv with why: a variable of a class template or a template
specialisation; one declared or defined in a conditional (`#if`) region, by a macro, with an attribute, in a
declaration with other declarators, thread-local; one more than one header declares, or that another .cpp or gen/
declares; a `const` pointer whose initialiser holds a string literal (a use may fold to that file's own copy of the
literal: its address would differ between files); one whose initialiser or type names something the header does not
see (its includes up to that point, the header above it), or that the block's place would not see; one whose move
into a block would put it below a use, or reorder it among the dynamically initialised (or destroyed) definitions of
its file; the function-pointer constants named `*_original`. A static data member's type that is incomplete in its
class is listed too. The block of a file holds only what can move; the rest stays where it is.

No record layout changes: only static data members get initialisers, nothing else of a class is touched.
"""
from __future__ import annotations

import argparse
import collections
import concurrent.futures
import gzip
import hashlib
import json
import os
import re
import shlex
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
HEADER = (".h", ".hpp", ".inl")
SCOPES = ("tu", "namespace", "anonymous", "linkage")


def fail(message: str) -> None:
	sys.exit(f"globals: {message}")


def digest(path: str) -> str:
	with open(path, "rb") as source:
		return hashlib.sha1(source.read()).hexdigest()


def is_header(path: str) -> bool:
	return path.endswith(HEADER)


def in_scope(path: str) -> bool:
	return path.startswith("src/") and not path.startswith("src/SDK/")


# ---------------------------------------------------------------------------------------------------------------
# measure
# ---------------------------------------------------------------------------------------------------------------

def tu_commands(root: str, compile_commands: str, scratch: str) -> list[tuple[str, str, list[str], bool]]:
	"""(file, directory, command words, headers mode) of every src/ TU outside src/SDK/, and the all-headers TU."""
	with open(compile_commands, encoding="utf-8") as source:
		entries = json.load(source)
	tus, seen, first = [], set(), None
	for entry in entries:
		file = os.path.realpath(os.path.join(entry["directory"], entry["file"]))
		relative = os.path.relpath(file, root)
		if not in_scope(relative) or file in seen:
			continue
		seen.add(file)
		words = entry["arguments"] if "arguments" in entry else shlex.split(entry["command"])
		tus.append((file, entry["directory"], words, False))
		if first is None and relative.endswith(".cpp"):
			first = (file, entry["directory"], words)
	if first is None:
		fail(f"no src/ TU in {compile_commands}")
	headers = []
	for base, _, names in os.walk(os.path.join(root, "src")):
		for name in names:
			if name.endswith((".h", ".hpp")):
				headers.append(os.path.relpath(os.path.join(base, name), os.path.join(root, "src")))
	allheaders = os.path.join(scratch, "allheaders.cpp")
	with open(allheaders, "w", encoding="utf-8") as out:
		out.writelines(f'#include "{header}"\n' for header in sorted(headers))
	words = [allheaders if w == first[0] or os.path.realpath(os.path.join(first[1], w)) == first[0] else w
			 for w in first[2]]
	tus.append((allheaders, first[1], words, True))
	return tus


def run_tu(root: str, srcdb: str, tu) -> tuple[str, int, list]:
	file, directory, words, headers = tu
	command = [srcdb, "globals-tu", "--root", root, "--file", file, "--directory", directory]
	if headers:
		command.append("--headers")
	result = subprocess.run(command + ["--"] + words, capture_output=True, text=True, errors="surrogateescape")
	lines = [json.loads(line) for line in result.stdout.splitlines() if line.startswith(("{", "["))]
	return file, result.returncode, lines


def measure(root: str, compile_commands: str, jobs: int, srcdb: str) -> dict:
	if not os.path.exists(srcdb):
		fail(f"build {srcdb} first (harness/srcdb)")
	os.makedirs(os.path.join(root, "build"), exist_ok=True)
	scratch = tempfile.mkdtemp(prefix="globals-", dir=os.path.join(root, "build"))
	tus = tu_commands(root, compile_commands, scratch)
	print(f"globals: {len(tus)} translation units", file=sys.stderr)
	variables: dict[str, dict] = {}
	includes, conds, tops, refs = set(), set(), {}, set()
	failed, errors = [], {}
	with concurrent.futures.ThreadPoolExecutor(jobs) as pool:
		for done, (file, code, lines) in enumerate(pool.map(lambda tu: run_tu(root, srcdb, tu), tus), 1):
			if done % 200 == 0:
				print(f"globals: {done}/{len(tus)}", file=sys.stderr)
			if code != 0:
				failed.append(os.path.relpath(file, root))
			for line in lines:
				if isinstance(line, dict):
					kind = line["t"]
					if kind == "var":
						merge_var(variables, line)
					elif kind == "top":
						tops[(line["file"], line["begin"], line["ctx"])] = line
					continue
				kind = line[0]
				if kind == "include":
					includes.add(tuple(line[1:]))
				elif kind == "cond":
					conds.add(tuple(line[1:]))
				elif kind == "ref":
					refs.add(tuple(line[1:]))
				elif kind == "errors" and line[1]:
					errors[os.path.relpath(file, root)] = line[1]
	files = sorted({d["file"] for v in variables.values() for d in v["decls"].values()} | {t[0] for t in tops})
	return {
		"root": root,
		"digests": {path: digest(os.path.join(root, path)) for path in files if os.path.exists(os.path.join(root, path))},
		"failed": failed, "errors": errors,
		"variables": variables, "includes": sorted(includes), "conds": sorted(conds),
		"tops": sorted(tops.values(), key=lambda t: (t["file"], t["begin"])), "refs": sorted(refs),
	}


def merge_var(variables: dict, record: dict) -> None:
	"""One variable's records of several TUs: every declaration once; the verdicts of the TUs that define it."""
	decls = [d for d in record["decls"]]
	defining = next((d for d in decls if d["def"]), None)
	key = record["name"]
	if not record["external"]:
		where = defining or decls[0]
		key += "@" + where["file"]
	known = variables.get(key)
	if known is None:
		known = variables[key] = {k: v for k, v in record.items() if k not in ("decls", "t")}
		known["decls"] = {}
		known["verdicts"] = []
	for decl in decls:
		known["decls"].setdefault(f"{decl['file']}:{decl['begin']}", decl)
	if record["defined"] or defining:
		verdict = (record["const_init"], record["dtor"])
		if verdict not in [tuple(v) for v in known["verdicts"]]:
			known["verdicts"].append(list(verdict))
		known["const_init"], known["dtor"], known["strptr"] = record["const_init"], record["dtor"], record["strptr"]
		known["defined"] = True


def save_model(model: dict, out: str) -> None:
	os.makedirs(out, exist_ok=True)
	with gzip.open(os.path.join(out, "model.json.gz"), "wt", encoding="utf-8") as sink:
		json.dump(model, sink)


def load_model(out: str) -> dict:
	path = os.path.join(out, "model.json.gz")
	if not os.path.exists(path):
		fail(f"no model in {out}: run measure first")
	with gzip.open(path, "rt", encoding="utf-8") as source:
		return json.load(source)


# ---------------------------------------------------------------------------------------------------------------
# text
# ---------------------------------------------------------------------------------------------------------------

TRIVIA = re.compile(rb"\s+|//[^\n]*|/\*.*?\*/", re.S)


def only_trivia(text: bytes) -> bool:
	return not TRIVIA.sub(b"", text)


def line_start(text: bytes, at: int) -> int:
	return text.rfind(b"\n", 0, at) + 1


def line_end(text: bytes, at: int) -> int:
	"""The offset just past the newline of the line holding `at`."""
	end = text.find(b"\n", at)
	return len(text) if end < 0 else end + 1


def closing_paren(text: str) -> int:
	"""The index of the parenthesis closing the one `text` opens with (strings and characters skipped); -1."""
	depth, k, quote = 0, 0, None
	while k < len(text):
		c = text[k]
		if quote:
			if c == "\\":
				k += 1
			elif c == quote:
				quote = None
		elif c in "\"'":
			quote = c
		elif c == "(":
			depth += 1
		elif c == ")":
			depth -= 1
			if depth == 0:
				return k
		k += 1
	return -1


def line_of(text: bytes, at: int) -> int:
	return text.count(b"\n", 0, at) + 1


def statement_end(text: bytes, end: int) -> int | None:
	"""The offset just past the `;` closing a declaration whose last token ends at `end` (a block's `}` needs
	none)."""
	if end > 0 and text[end - 1:end] == b"}":
		at = end
		while at < len(text) and text[at:at + 1] in b" \t":
			at += 1
		return at + 1 if text[at:at + 1] == b";" else end
	at = end
	while at < len(text) and text[at:at + 1] in b" \t\r\n":
		at += 1
	return at + 1 if text[at:at + 1] == b";" else None


def extent(text: bytes, begin: int, end: int) -> tuple[int, int, int] | None:
	"""The lines a declaration owns: (first line of the comments right above it, the declaration's first line, the
	end of its last line). None when another declaration or statement shares one of its lines."""
	stop = statement_end(text, end)
	if stop is None:
		return None
	rest = text[stop:line_end(text, stop)]
	if not only_trivia(rest) or b"/*" in rest and b"*/" not in rest:
		return None
	first = line_start(text, begin)
	if text[first:begin].strip():
		return None
	top = first
	while top > 0:
		previous = line_start(text, top - 1)
		line = text[previous:top].strip()
		if line.startswith(b"//"):
			top = previous
			continue
		if line.endswith(b"*/") and not line.startswith(b"*/"):
			opening = text.rfind(b"/*", 0, top)
			if opening < 0 or text[line_start(text, opening):opening].strip():
				break
			top = line_start(text, opening)
			continue
		break
	return top, first, line_end(text, stop)


# ---------------------------------------------------------------------------------------------------------------
# decide
# ---------------------------------------------------------------------------------------------------------------

class Tree:
	"""The model's facts about files: text, includes, conditional regions, top-level declarations, references."""

	def __init__(self, root: str, model: dict):
		self.root = root
		self.texts: dict[str, bytes] = {}
		self.edges = collections.defaultdict(list)
		for file, offset, included in model["includes"]:
			self.edges[file].append((offset, included))
		self.conds = collections.defaultdict(list)
		for file, begin, end in model["conds"]:
			self.conds[file].append((begin, end))
		self.tops = collections.defaultdict(list)
		for top in model["tops"]:
			self.tops[top["file"]].append(top)
		self.refs = collections.defaultdict(list)
		for file, offset, name in model["refs"]:
			self.refs[file].append((offset, name))
		self._closure: dict[str, set] = {}

	def text(self, path: str) -> bytes:
		if path not in self.texts:
			with open(os.path.join(self.root, path), "rb") as source:
				self.texts[path] = source.read()
		return self.texts[path]

	def reach(self, path: str, at: int) -> set:
		"""The files an #include of `path` above offset `at` brings in. A file in an include cycle with `path` does
		not count, nor what only it brings in: a TU that includes it first sees `path` before the rest of it."""
		key = (path, at)
		if key not in self._closure:
			cycle = self.includers(path)
			stack = [included for offset, included in self.edges.get(path, [])
					 if offset < at and included not in cycle]
			seen = set(stack)
			while stack:
				for _, found in self.edges.get(stack.pop(), []):
					if found not in seen and found not in cycle:
						seen.add(found)
						stack.append(found)
			self._closure[key] = seen
		return self._closure[key]

	def includers(self, path: str) -> set:
		"""Every file whose includes reach `path`, and `path`."""
		if not hasattr(self, "_reverse"):
			self._reverse = collections.defaultdict(set)
			for file, edges in self.edges.items():
				for _, included in edges:
					self._reverse[included].add(file)
			self._includers = {}
		if path not in self._includers:
			seen, stack = {path}, [path]
			while stack:
				for found in self._reverse.get(stack.pop(), ()):
					if found not in seen:
						seen.add(found)
						stack.append(found)
			self._includers[path] = seen
		return self._includers[path]

	def guard(self, path: str, begin: int, end: int) -> bool:
		text = self.text(path)
		return only_trivia(text[:begin]) and only_trivia(text[line_end(text, end):])

	def conditional(self, path: str, at: int) -> tuple[int, int] | None:
		"""The outermost #if region (not the include guard) holding offset `at`."""
		best = None
		for begin, end in self.conds.get(path, []):
			if begin <= at <= end and not self.guard(path, begin, end):
				if best is None or begin < best[0]:
					best = (begin, end)
		return best


def visible(tree: Tree, path: str, at: int, locs: list, reach: set, inside=None) -> bool:
	"""A declaration (any of its locations) is seen at offset `at` of `path`: above it in that file, or in a file
	that file includes above it. `inside(loc)` says a location in `path` moves along (it stays above)."""
	if not locs:
		return True
	for file, begin, end in locs:
		if file == path and end < at:
			return True
		if file == path and inside and inside(begin):
			return True
		if file in reach:
			return True
	return False


class Item:
	"""A unit a block holds: one declaration, or a namespace scope of declarations only, with its lines."""

	def __init__(self, path, top, first, end, decls, variables, ctx):
		self.path, self.top, self.first, self.end = path, top, first, end
		self.decls, self.variables, self.ctx = decls, variables, ctx
		self.moving = True
		self.why = ""
		self.text = None  # a replacement text (a definition moved here), else the file's own lines


def decide(root: str, model: dict) -> dict:
	tree = Tree(root, model)
	variables = model["variables"]
	listed, sites = [], []
	header_moves: dict[str, dict] = {}  # variable key -> its move into a header

	def where(decl):
		return f"{decl['file']}:{line_of(tree.text(decl['file']), decl['begin'])}"

	def note(key, decl, why):
		listed.append({"variable": variables[key]["name"], "where": where(decl), "why": why})

	# rule 1: candidates for a definition in the header
	for key, var in sorted(variables.items()):
		decls = list(var["decls"].values())
		defs = [d for d in decls if d["def"]]
		if not defs or not all(in_scope(d["file"]) for d in defs):
			continue
		definition = defs[0]
		if var["name"].endswith("_original"):
			continue
		if var.get("template"):
			note(key, definition, "template")
			continue
		if is_header(definition["file"]):
			continue
		if len(var.get("verdicts", [])) > 1:
			note(key, definition, "the TUs disagree whether it is constant-initialised")
			continue
		if not var["const_init"] or var["dtor"]:
			continue
		if var["member"]:
			own = [d for d in decls if d["ctx"][0] == "class"]
		else:
			own = [d for d in decls if not d["def"] and is_header(d["file"])]
		others = [d for d in decls if d is not definition and d not in own]
		if not own:
			continue
		why = None
		if len(own) > 1:
			why = "declared in more than one header"
		elif others:
			why = "declared in another file too: " + ", ".join(sorted({d["file"] for d in others}))
		elif not in_scope(own[0]["file"]):
			why = "declared outside src/ or in src/SDK/"
		elif var["tls"]:
			why = "thread-local"
		elif definition["macro"] or own[0]["macro"]:
			why = "declared by a macro"
		elif definition["attrs"] or own[0]["attrs"]:
			why = "attribute: " + ", ".join(definition["attrs"] + own[0]["attrs"])
		elif definition["group"] or own[0]["group"]:
			why = "a declaration with other declarators"
		elif tree.conditional(definition["file"], definition["begin"]):
			why = "defined in a conditional region"
		elif tree.conditional(own[0]["file"], own[0]["begin"]):
			why = "declared in a conditional region"
		elif var["const"] and var["strptr"]:
			why = "a const pointer to a string literal (its address would differ between files)"
		if why:
			note(key, definition, why)
			continue
		header_moves[key] = {"key": key, "def": definition, "decl": own[0]}

	# the definition's new text, its needs (where it is seen) and its form
	for key, move in list(header_moves.items()):
		var = variables[key]
		text = tree.text(move["def"]["file"])
		decl_text = tree.text(move["decl"]["file"])
		definition, decl = move["def"], move["decl"]
		stop = statement_end(text, definition["end"])
		lines = extent(text, definition["begin"], definition["end"])
		home = extent(decl_text, decl["begin"], decl["end"])
		if stop is None or lines is None or home is None:
			note(key, definition, "shares a line with another declaration")
			del header_moves[key]
			continue
		prefix = text[definition["begin"]:definition["qual"]].decode("utf-8", "surrogateescape")
		name = decl_text[decl["name"]:decl["name"] + len(var["name"].rsplit("::", 1)[-1])].decode()
		suffix = text[definition["name"] + len(name):stop - 1].decode("utf-8", "surrogateescape")
		prefix = re.sub(r"^\s*extern\s+", "", prefix)
		# the header's spelling of the type where the declarators agree (no array bound, no function declarator):
		# it is written for the header's scope (a member's type without its class's qualification)
		decl_stop = statement_end(decl_text, decl["end"])
		own_suffix = decl_text[decl["name"] + len(name):decl_stop - 1].decode("utf-8", "surrogateescape").strip()
		if not own_suffix and (not suffix.strip() or suffix.lstrip()[0] in "={("):
			own_prefix = decl_text[decl["begin"]:decl["name"]].decode("utf-8", "surrogateescape")
			own_prefix = re.sub(r"^\s*(?:(?:extern|static|inline)\s+)+", "", own_prefix)
			if own_prefix.strip() and "\n" not in own_prefix:
				prefix = own_prefix
		literal = var["const"] and var["literal"] and not var["pointer"]
		if re.match(r"\s*constexpr\b", prefix):
			form, body = "constexpr", prefix
		elif literal and re.match(r"\s*const\s", prefix):
			form, body = "constexpr", re.sub(r"^\s*const\s+", "constexpr ", prefix)
		else:
			form, body = "inline", prefix
		if var["member"]:
			lead = "static " + ("" if form == "constexpr" else "inline ")
			form = "static " + form
			# a class member's initialiser cannot be parenthesised: the same constructor call in braces (a constant
			# initialiser narrows nothing that does not fit)
			bare = suffix.strip()
			if bare.startswith("(") and closing_paren(bare) == len(bare) - 1:
				suffix = suffix[:len(suffix) - len(suffix.lstrip())] + "{" + bare[1:-1] + "}"
		else:
			lead = "inline "
		new = lead + body + name + suffix
		if definition["init"] == [] and not decl["init"]:
			form += " (zero-initialised)"
		elif decl["init"] and not definition["init"]:
			# the initialiser is the in-class one: the declaration keeps it
			written = decl_text[decl["begin"]:statement_end(decl_text, decl["end"]) - 1].decode(
				"utf-8", "surrogateescape")
			written = re.sub(r"^\s*static\s+", "", written)
			if form.endswith("constexpr") and not written.startswith("constexpr"):
				written = "constexpr " + re.sub(r"^const\s+", "", written)
			new = lead + written
		indent = decl_text[line_start(decl_text, decl["begin"]):decl["begin"]].decode()
		body_lines = new.split("\n")
		first_indent = text[line_start(text, definition["begin"]):definition["begin"]].decode()
		for k in range(1, len(body_lines)):
			line = body_lines[k]
			if line.startswith(first_indent):
				line = line[len(first_indent):]
			body_lines[k] = indent + line if line.strip() else line
		comments = text[lines[0]:lines[1]].decode("utf-8", "surrogateescape")
		own_comments = decl_text[home[0]:home[1]].decode("utf-8", "surrogateescape")
		if comments.strip() and comments.strip() != own_comments.strip():
			comments = "".join(indent + c.strip() + "\n" for c in comments.splitlines())
		else:
			comments = ""
		trailing = text[stop:line_end(text, stop)].decode("utf-8", "surrogateescape").rstrip("\n")
		own_trailing = decl_text[statement_end(decl_text, decl["end"]):line_end(
			decl_text, statement_end(decl_text, decl["end"]))].decode("utf-8", "surrogateescape").rstrip("\n")
		added_trailing = ""
		if not own_trailing.strip() and trailing.strip():
			own_trailing = added_trailing = trailing
		move.update(form=form, new="\n".join(body_lines), comments=comments, own_comments=own_comments,
					trailing=own_trailing, added_trailing=added_trailing, indent=indent, cut=(lines[0], lines[2]),
					needs=definition["needs"] + [["macro", m] for m in definition["macros"]] + decl["needs"])

	def needs_seen(path, at, needs, reach, inside=None):
		for need in needs:
			if isinstance(need, list):
				_, (macro, file, offset) = need
				if file and not visible(tree, path, at, [[file, offset, offset]], reach, inside):
					return need
				continue
			if not visible(tree, path, at, need["locs"], reach, inside):
				return need
		return None

	def need_name(need):
		return "macro " + need[1][0] if isinstance(need, list) else need["name"] or need["why"]

	# a member's initialiser goes into the class where the class sees what it needs; else the definition goes after
	# the class, `inline` (the class's own type, say, is incomplete in it)
	for key, move in list(header_moves.items()):
		if not variables[key]["member"]:
			continue
		decl = move["decl"]
		reach = tree.reach(decl["file"], decl["begin"])
		missing = needs_seen(decl["file"], decl["begin"], move["needs"], reach)
		if not missing:
			continue
		owner = next((t for t in tree.tops.get(decl["file"], []) if t["begin"] == decl["ctx"][1]
					  and t["kind"] == "class"), None)
		if owner is None or not is_header(decl["file"]) or decl["init"]:
			note(key, move["def"], f"the class does not see {need_name(missing)}")
			del header_moves[key]
			continue
		definition = move["def"]
		text = tree.text(definition["file"])
		decl_text = tree.text(decl["file"])
		stop = statement_end(text, definition["end"])
		written = text[definition["begin"]:stop - 1].decode("utf-8", "surrogateescape")
		trailing = text[stop:line_end(text, stop)].decode("utf-8", "surrogateescape").rstrip("\n")
		lines = written.split("\n")
		first_indent = text[line_start(text, definition["begin"]):definition["begin"]].decode()
		lines = [lines[0]] + [l[len(first_indent):] if l.startswith(first_indent) else l for l in lines[1:]]
		comments = text[move["cut"][0]:line_start(text, definition["begin"])].decode("utf-8", "surrogateescape")
		after = line_end(decl_text, statement_end(decl_text, owner["end"]) or owner["end"])
		move.update(outside={"at": after, "ctx": owner["ctx"]}, form="inline (after the class)",
					text=("".join(c.strip() + "\n" for c in comments.splitlines() if c.strip())
						  + "inline " + "\n".join(lines) + ";" + trailing + "\n").encode("utf-8", "surrogateescape"))

	# the blocks, until no move into a header is given up
	while True:
		plans, given_up = plan_blocks(tree, variables, header_moves, needs_seen, need_name)
		if not given_up:
			break
		for key, why in given_up:
			note(key, header_moves[key]["def"], why)
			del header_moves[key]
	for plan in plans.values():
		for item in plan["items"]:
			if not item.moving and item.why:
				for decl, key in zip(item.decls, item.variables):
					if key not in header_moves or decl is not header_moves[key]["decl"]:
						listed.append({"variable": variables[key]["name"], "where": where(decl),
									   "why": item.why})
	for key, move in sorted(header_moves.items()):
		sites.append({"variable": variables[key]["name"], "from": where(move["def"]), "to": where(move["decl"]),
					  "form": move["form"]})
	return {"tree": tree, "moves": header_moves, "plans": plans, "listed": listed, "sites": sites}


def plan_blocks(tree: Tree, variables: dict, header_moves: dict, needs_seen, need_name):
	"""Per file and namespace scope: the items, where their block goes, which of them move."""
	moved_defs = {(m["def"]["file"], m["def"]["begin"]) for m in header_moves.values()}
	moved_decls = {(m["decl"]["file"], m["decl"]["begin"]): m for m in header_moves.values() if "outside" not in m}
	per_file: dict[str, list] = collections.defaultdict(list)
	for key, var in variables.items():
		if var.get("template") or var["name"].endswith("_original"):
			continue
		for decl in var["decls"].values():
			if decl["ctx"][0] not in SCOPES or not in_scope(decl["file"]):
				continue
			if (decl["file"], decl["begin"]) in moved_defs:
				continue
			if is_header(decl["file"]) or decl["def"]:
				per_file[decl["file"]].append((decl, key))
	for key, move in header_moves.items():
		if "outside" in move:
			at = move["outside"]["at"]
			path = move["decl"]["file"]
			per_file[path].append(({"file": path, "begin": at, "end": at, "def": True, "needs": move["needs"],
									"macros": [], "macro": False, "group": False, "synthetic": move,
									"ctx": ["namespace", move["outside"]["ctx"], path],
									"rank": move["def"]["begin"]}, key))
	plans, given_up = {}, []
	for path, entries in sorted(per_file.items()):
		text = tree.text(path)
		header = is_header(path)
		tops = tree.tops.get(path, [])
		by_begin = {}
		for decl, key in entries:
			by_begin.setdefault(decl["begin"], []).append((decl, key))
		items, scopes = [], collections.defaultdict(list)
		# scopes holding declarations only: one item in their parent scope
		children = collections.defaultdict(list)
		for top in tops:
			children[top["ctx"]].append(top)
		units = {}
		for top in tops:
			if top["kind"] != "other" or top["begin"] not in children:
				continue
			inner = children[top["begin"]]
			if inner and all(t["kind"] == "var" and t["begin"] in by_begin for t in inner) and not header:
				units[top["begin"]] = top
		for decl, key in sorted(entries, key=lambda e: e[0]["begin"]):
			ctx = decl["ctx"][1]
			if ctx in units:
				continue
			item = Item(path, None, None, None, [decl], [key], ctx)
			scopes[ctx].append(item)
			if decl.get("synthetic"):
				item.top = item.first = item.end = decl["begin"]
				item.text = decl["synthetic"]["text"]
				item.synthetic = decl["synthetic"]
				continue
			span = extent(text, decl["begin"], decl["end"])
			if span is None:
				item.moving, item.why = False, "shares a line with another declaration"
			else:
				item.top, item.first, item.end = span
				# a type alias right above it goes with it when it or a later declaration of the scope names it (the
				# alias stays right above it in the block, so a second run decides the same)
				above = [t for t in children.get(ctx, []) if t["end"] <= decl["begin"]]
				if above and above[-1]["kind"] == "type" and above[-1]["name"] in {
						n["name"] for d, _ in entries if d["ctx"][1] == ctx and d["begin"] >= decl["begin"]
						for n in d["needs"] if isinstance(n, dict)}:
					alias = extent(text, above[-1]["begin"], above[-1]["end"])
					if alias and only_trivia(text[alias[2]:item.top]):
						item.top = alias[0]
						item.types = [above[-1]["name"]]
						item.extra_needs = above[-1].get("needs", [])
			if decl["macro"]:
				item.moving, item.why = False, "declared by a macro"
			elif decl["group"]:
				item.moving, item.why = False, "a declaration with other declarators"
			elif tree.conditional(path, decl["begin"]):
				item.moving, item.why = False, "in a conditional region"
			move = moved_decls.get((path, decl["begin"]))
			if move:
				item.text = (text[item.top:item.first].decode("utf-8", "surrogateescape") + move["comments"]
							 + move["indent"] + move["new"] + ";" + move["trailing"] + "\n").encode(
					"utf-8", "surrogateescape")
		for begin, top in units.items():
			span = extent(text, top["begin"], top["end"])
			inner = [by_begin[t["begin"]][0] for t in children[begin]]
			item = Item(path, None, None, None, [d for d, _ in inner], [k for _, k in inner], top["ctx"])
			if span is None:
				item.moving, item.why = False, "shares a line with another declaration"
			else:
				item.top, item.first, item.end = span
			if tree.conditional(path, top["begin"]):
				item.moving, item.why = False, "in a conditional region"
			scopes[top["ctx"]].append(item)
		ordered_all = [(d["begin"], d["ctx"][1]) for d, k in entries if defines_dynamic(variables[k], d)]
		for ctx, scope_items in scopes.items():
			scope_items.sort(key=lambda i: (i.decls[0]["begin"], i.decls[0].get("rank", 0)))
			foreign = [o for o, c in ordered_all if c != ctx and not any(
				i.first is not None and i.top <= o < i.end for i in scope_items)]
			plan = place_block(tree, path, ctx, scope_items, children.get(ctx, []), header, variables,
							   moved_decls, needs_seen, need_name, given_up, foreign)
			if plan:
				plans[(path, ctx)] = plan
	return plans, given_up


def place_block(tree, path, ctx, items, tops, header, variables, moved_decls, needs_seen, need_name, given_up,
				foreign):
	text = tree.text(path)
	item_begins = {i.decls[0]["begin"] for i in items}
	others = [t for t in tops if not any(i.first is not None and i.top <= t["begin"] < i.end for i in items)
			  and t["begin"] not in item_begins]
	at = None
	if header:
		types = [t for t in others if t["kind"] == "class" or t["kind"] == "type" and not t["name"].endswith(
			"_original_t")]
		if types:
			last = types[-1]
			stop = statement_end(text, last["end"]) or last["end"]
			at = line_end(text, stop)
	else:
		functions = [t for t in others if t["kind"] == "fn"]
		if functions:
			first = functions[0]
			span = extent(text, first["begin"], first["end"])
			at = span[0] if span else line_start(text, first["begin"])
		elif others:
			last = others[-1]
			at = line_end(text, statement_end(text, last["end"]) or last["end"])
	movable = [i for i in items if i.moving]
	if at is None:
		at = movable[0].top if movable else None
	if at is None:
		return {"items": items, "at": None, "ctx": ctx}
	region = tree.conditional(path, at)
	if region:
		at = line_start(text, region[0])
	reach = tree.reach(path, at)
	ordered = lambda item: any(defines_dynamic(variables[k], d) for d, k in zip(item.decls, item.variables))
	while True:
		changed = False
		moving = [i for i in items if i.moving]

		def inside(offset, me):
			return me.top <= offset < me.first or any(
				j.top <= offset < j.end and j.decls[0]["begin"] < me.decls[0]["begin"] for j in moving)

		for item in moving:
			names = {variables[k]["name"] for k in item.variables} | set(getattr(item, "types", []))
			synthetic = getattr(item, "synthetic", None)
			moved_here = synthetic or any((path, d["begin"]) in moved_decls for d in item.decls)
			down = item.end <= at
			if down and not synthetic:
				for offset, name in tree.refs.get(path, []):
					if name in names and item.end <= offset < at and not any(j.top <= offset < j.end for j in moving):
						item.moving, item.why = False, "used between it and the block"
						break
			# an item below the place with only trivia and the block's other items between does not move: what it
			# names is seen where it is (a definition moved here is checked there)
			settled = not down and not synthetic and only_trivia(between(text, at, item.top, moving))
			if item.moving and (not down or moved_here) and not (settled and not moved_here):
				# what moves up must see its needs at the block; a definition moved into a header, wherever it goes
				for decl, key in zip(item.decls, item.variables):
					move = synthetic or moved_decls.get((path, decl["begin"]))
					needs = (move["needs"] if move else decl["needs"] + [["macro", m] for m in decl["macros"]]) + getattr(
						item, "extra_needs", [])
					place = decl["begin"] if settled else at
					seen = tree.reach(path, place) if settled else reach
					missing = needs_seen(path, place, needs, seen, lambda offset: inside(offset, item))
					if missing:
						item.moving, item.why = False, f"the block's place does not see {need_name(missing)}"
						break
			if item.moving and ordered(item):
				fixed = [o.decls[0]["begin"] for o in items if not o.moving and ordered(o)] + foreign
				for o in fixed:
					if item.end <= at and item.end <= o < at or item.top >= at and at <= o < item.top:
						item.moving, item.why = False, "would reorder dynamic initialisation"
						break
			if not item.moving:
				changed = True
		if not changed:
			break
	# a definition moved into the header that cannot join the block must be seen where it is
	for item in items:
		if item.moving or item.first is None:
			continue
		for decl, key in zip(item.decls, item.variables):
			move = getattr(item, "synthetic", None) or moved_decls.get((path, decl["begin"]))
			if move:
				missing = needs_seen(path, decl["begin"], move["needs"], tree.reach(path, decl["begin"]))
				if missing:
					given_up.append((key, f"the header does not see {need_name(missing)}"))
	moving = [i for i in items if i.moving]
	if moving:
		if only_trivia(between(text, min(moving[0].top, at), max(moving[-1].end, at), moving)):
			# already one block next to its place: only the texts of definitions moved here change
			for item in moving:
				item.in_place = True
	return {"items": items, "at": at, "ctx": ctx}


def defines_dynamic(var: dict, decl: dict) -> bool:
	return decl["def"] and (not var["const_init"] or var["dtor"])


def between(text: bytes, low: int, high: int, items) -> bytes:
	"""The text from `low` to `high` without the lines of `items`."""
	rest, cursor = bytearray(), low
	for item in sorted(items, key=lambda i: i.top):
		if item.end <= cursor or item.top >= high:
			continue
		rest += text[cursor:item.top] if item.top > cursor else b""
		cursor = item.end
	return bytes(rest + text[cursor:high]) if cursor < high else bytes(rest)


# ---------------------------------------------------------------------------------------------------------------
# apply
# ---------------------------------------------------------------------------------------------------------------


def edits_for(decision: dict) -> dict[str, list]:
	"""Per file: (begin, end, replacement) edits, none overlapping."""
	tree = decision["tree"]
	edits = collections.defaultdict(list)
	handled = set()
	for (path, ctx), plan in decision["plans"].items():
		text = tree.text(path)
		moving = [i for i in plan["items"] if i.moving]
		in_place = moving and all(getattr(i, "in_place", False) for i in moving)
		for item in plan["items"]:
			for decl in item.decls:
				handled.add((path, decl["begin"]))
		if in_place or not moving:
			for item in plan["items"]:
				if item.text is not None and item.first is not None:
					edits[path].append((item.top, item.end, item.text))
			continue
		block = b""
		for k, item in enumerate(moving):
			body = item.text if item.text is not None else text[item.top:item.end]
			if k and body.lstrip().startswith((b"//", b"/*")):
				block += b"\n"
			block += body
			edits[path].append((item.top, item.end, b""))
		for item in plan["items"]:
			if not item.moving and item.text is not None and item.first is not None:
				edits[path].append((item.top, item.end, item.text))
		at = plan["at"]
		before = text[:at]
		lead = b"" if before.endswith(b"\n\n") or not before.strip() else b"\n"
		tail = b"" if text[at:].startswith(b"\n") else b"\n"
		edits[path].append((at, at, lead + block + tail))
	# members (their declaration gets the initialiser) and header declarations no block planned
	for move in decision["moves"].values():
		decl = move["decl"]
		path = decl["file"]
		dt = tree.text(path)
		if (path, decl["begin"]) not in handled and "outside" not in move:
			stop = statement_end(dt, decl["end"])
			new = (move["new"] + ";" + move["added_trailing"]).encode("utf-8", "surrogateescape")
			comments = move["comments"].encode("utf-8", "surrogateescape")
			if comments:
				span = extent(dt, decl["begin"], decl["end"])
				edits[path].append((span[1], span[1], comments))
			edits[path].append((decl["begin"], stop, new))
		edits[move["def"]["file"]].append((move["cut"][0], move["cut"][1], b""))
	return edits


def apply_edits(root: str, tree: Tree, edits: dict, model: dict, dry_run: bool) -> list[str]:
	changed = []
	for path, file_edits in sorted(edits.items()):
		if model["digests"].get(path) != digest(os.path.join(root, path)):
			fail(f"{path} changed since measure: measure again")
		text = tree.text(path)
		file_edits = sorted(set(file_edits), key=lambda e: (e[0], e[1]))
		for (b1, e1, _), (b2, e2, _) in zip(file_edits, file_edits[1:]):
			if b2 < e1:
				fail(f"{path}: overlapping edits at {b1}-{e1} and {b2}-{e2}")
		out = bytearray()
		cursor = 0
		for begin, end, replacement in file_edits:
			out += text[cursor:begin]
			if not replacement and begin != end:
				# a removed declaration: no double blank line where it was
				while out.endswith(b"\n\n") and text[end:end + 1] == b"\n":
					end += 1
			out += replacement
			cursor = max(cursor, end)
		out += text[cursor:]
		result = bytes(out)
		if result != text:
			changed.append(path)
			if not dry_run:
				with open(os.path.join(root, path), "wb") as sink:
					sink.write(result)
	return changed


# ---------------------------------------------------------------------------------------------------------------
# tables
# ---------------------------------------------------------------------------------------------------------------

def write_tsv(path: str, columns, rows) -> None:
	with open(path, "w", encoding="utf-8") as sink:
		sink.write("\t".join(columns) + "\n")
		for row in rows:
			sink.write("\t".join(str(row.get(c, "")) for c in columns) + "\n")


def counts(model: dict, decision: dict) -> list[str]:
	variables = model["variables"]
	moved = set(decision["moves"])
	listed_names = collections.Counter()
	classes = collections.Counter()
	for key, var in variables.items():
		defs = [d for d in var["decls"].values() if d["def"]]
		if not defs or not all(in_scope(d["file"]) for d in defs):
			continue
		header_decl = any(not d["def"] and is_header(d["file"]) or d["ctx"][0] == "class"
						  for d in var["decls"].values())
		kind = "member" if var["member"] else "global"
		if var.get("template"):
			classes[f"{kind}: template (listed)"] += 1
		elif is_header(defs[0]["file"]):
			classes[f"{kind}: defined in a header already"] += 1
		elif key in moved:
			classes[f"{kind}: constant-initialised, moved to its header"] += 1
		elif var["const_init"] and not var["dtor"] and header_decl:
			classes[f"{kind}: constant-initialised, declared in a header, listed (stays in its .cpp)"] += 1
		elif var["const_init"] and not var["dtor"]:
			classes[f"{kind}: constant-initialised, no header declaration (stays in its .cpp)"] += 1
		else:
			classes[f"{kind}: dynamic initialiser or destructor (stays in its .cpp)"] += 1
	for row in decision["listed"]:
		listed_names[re.sub(r":.*", "", row["why"]) if row["why"].startswith(
			("declared in another file", "attribute")) else row["why"]] += 1
	lines = ["variables by class:"]
	lines += [f"  {n:6}  {c}" for c, n in sorted(classes.items())]
	lines.append(f"  {sum(classes.values()):6}  total")
	lines.append("listed, by why:")
	lines += [f"  {n:6}  {c}" for c, n in listed_names.most_common()]
	blocks = collections.Counter()
	for (path, ctx), plan in decision["plans"].items():
		moving = [i for i in plan["items"] if i.moving]
		kind = "header" if is_header(path) else ".cpp"
		if moving and all(getattr(i, "in_place", False) for i in moving):
			blocks[f"{kind} blocks already in place"] += 1
		elif moving:
			blocks[f"{kind} blocks formed"] += 1
			blocks[f"{kind} declarations moved into a block"] += sum(len(i.decls) for i in moving)
		blocks[f"{kind} declarations left where they are"] += sum(len(i.decls) for i in plan["items"]
																	if not i.moving)
	lines.append("blocks:")
	lines += [f"  {n:6}  {c}" for c, n in sorted(blocks.items())]
	return lines


def report(model: dict, decision: dict, out: str, changed: list[str] | None) -> None:
	os.makedirs(out, exist_ok=True)
	write_tsv(os.path.join(out, "sites.tsv"), ("variable", "from", "to", "form"), decision["sites"])
	write_tsv(os.path.join(out, "listed.tsv"), ("variable", "where", "why"),
			  sorted(decision["listed"], key=lambda r: (r["why"], r["where"])))
	rows = []
	tree = decision["tree"]
	for (path, ctx), plan in sorted(decision["plans"].items(), key=lambda p: (p[0][0], p[0][1])):
		moving = [i for i in plan["items"] if i.moving]
		state = "in place" if moving and all(getattr(i, "in_place", False) for i in moving) else (
			"formed" if moving else "none")
		at = plan["at"]
		rows.append({"file": path, "scope": ctx, "line": line_of(tree.text(path), at) if at is not None else "",
					 "state": state, "moved": sum(len(i.decls) for i in moving),
					 "left": sum(len(i.decls) for i in plan["items"] if not i.moving)})
	write_tsv(os.path.join(out, "blocks.tsv"), ("file", "scope", "line", "state", "moved", "left"), rows)
	lines = counts(model, decision)
	if model["failed"] or model["errors"]:
		lines.append(f"parse: {len(model['failed'])} TUs failed, {len(model['errors'])} with errors: "
					 + ", ".join(sorted(model["errors"])[:10]))
	if changed is not None:
		lines.append(f"files changed: {len(changed)}")
	text = "\n".join(lines) + "\n"
	with open(os.path.join(out, "report.txt"), "w", encoding="utf-8") as sink:
		sink.write(text)
	print(text, end="")


def main() -> None:
	parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
	parser.add_argument("command", choices=("measure", "apply"))
	parser.add_argument("--root", default=ROOT)
	parser.add_argument("--compile-commands")
	parser.add_argument("--out", default=os.path.join(ROOT, "env", "scratch", "Clean", "globals"))
	parser.add_argument("--jobs", type=int, default=8)
	parser.add_argument("--srcdb", default=os.path.join(ROOT, "build", "srcdb", "srcdb"))
	parser.add_argument("--dry-run", action="store_true")
	arguments = parser.parse_args()
	root = os.path.realpath(arguments.root)
	out = os.path.abspath(arguments.out)
	if arguments.command == "measure":
		commands = arguments.compile_commands or os.path.join(root, "build", "linux", "compile_commands.json")
		model = measure(root, commands, arguments.jobs, os.path.abspath(arguments.srcdb))
		save_model(model, out)
		report(model, decide(root, model), out, None)
		return
	model = load_model(out)
	if os.path.realpath(model["root"]) != root:
		fail(f"the model in {out} is of {model['root']}, not {root}")
	decision = decide(root, model)
	changed = apply_edits(root, decision["tree"], edits_for(decision), model, arguments.dry_run)
	report(model, decision, out, changed)


if __name__ == "__main__":
	main()
