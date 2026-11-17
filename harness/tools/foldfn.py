#!/usr/bin/env python3
"""Fold the constants that only name a function (docs/PLAN.md Part 2): each use becomes the function itself.

Usage: env/venv/bin/python harness/tools/foldfn.py [--root DIR] [--compile-commands FILE]... [--jobs N] [--srcdb BIN]
                                                   [--refs FILE] [--save-refs FILE] [--dry-run] [--out DIR]

The seam wrote a function's address as a constant (`inline const T_original_t T_original = &T::F;`,
`inline void* const X_CreateObject = reinterpret_cast<void*>(&X::CreateObject);`, `static const` ones file-local).
The compiler finds them: `srcdb fold-refs` parses every src TU of the compile databases (default build/linux under
the root; build it first) and lists each variable of a const type whose initialiser is one function's address
(`&f`, `f`, either inside one explicit cast), every declaration of it, every use, and the typedef or alias its type is
written as. What each becomes:

  a call through it        `K(args)`, `(*K)(args)`: a direct call of the function, `F(args)`, where its initialiser
                           is `&F` or `F` of the constant's own type (a call through a cast is left)
  a member call            `(o->*K)(args)`: `(o->*&C::F)(args)`
  any other use            `&F` (`&C::F`); where the initialiser has a cast (`reinterpret_cast<void*>(&C::F)`): that
                           expression, so a use compared with or stored as `void*` keeps it
  the constant             deleted (each declaration, through its `;`) once every use is rewritten; with it the
                           comment block right above it when every line of code from there to the next blank line is
                           deleted too (it introduced them)
  its typedef or alias     deleted when nothing else spells it

The function is spelled as the initialiser spells it where the parse finds that the use's scopes give that name to
the function (`visible`); else qualified: from its class or namespace, from the global namespace where the use's
scopes give the name to another declaration first (`shadowed`).

Left, with the reason in the tables: a constant spelled by a macro's body; one whose name is spelled where no parse
saw it nor another declaration of that name (a preprocessor branch the builds do not take, a file no TU includes);
the parse names another declaration's spellings only for the names --names lists, which the script takes from the
tree's text (every declarator after a `const` that has an `=`); an automatic local; one with a use left: its address
taken (`&K`), an unevaluated use (sizeof, decltype), a call through a cast or a converted pointer, a value its
initialiser converts implicitly; one whose declaration shares its line or its declarators.

Tables (in --out, default env/scratch/Clean/foldfn; each rewritten by the run; lines are the input tree's):
  constants.tsv  every constant: file, line, name, scope, form, flags, function, uses, action (folded, left), reason
  uses.tsv       every use: constant's file and line, name, file, line, context, the text before and after, action
  aliases.tsv    the aliases the constants' types are written as: file, line, name, action (deleted, kept), reason
  comments.tsv   the comment blocks deleted with a constant, and the comments left that name a deleted constant
  report.txt     the counts, the TUs whose parse failed or met errors
A second run changes nothing (what it finds then is what the first left).
"""
import argparse
import bisect
import collections
import concurrent.futures
import json
import os
import re
import shlex
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import unseam  # noqa: E402  (the tokenizer)

EXTENSIONS = (".h", ".hpp", ".inl", ".cpp", ".c", ".inc")
# a value use under one of these needs the replacement in parentheses (`&F` is a unary expression)
TIGHT = {"UnaryOperator", "ArraySubscriptExpr", "MemberExpr", "CXXDependentScopeMemberExpr"}


# --- the parse ------------------------------------------------------------------------------------------------------

def tu_commands(root, compile_commands):
	tus = []
	src = os.path.join(root, "src") + os.sep
	for path in compile_commands:
		with open(path, encoding="utf-8") as source:
			entries = json.load(source)
		for entry in entries:
			file = os.path.realpath(os.path.join(entry["directory"], entry["file"]))
			if not file.startswith(src):
				continue
			words = entry["arguments"] if "arguments" in entry else shlex.split(entry["command"])
			tus.append((file, entry["directory"], words))
	return tus


# a declaration that may be one of the constants: its declarator's name before the `=` (a superset the parse narrows;
# a constant this misses is left where its name is another declaration's too)
CONST_DECLARATION = re.compile(r"\bconst\b[^;=(){}]*?\b([A-Za-z_]\w*)\s*=[^=]")


def constant_names(tree):
	names = set()
	for text in tree.texts.values():
		names.update(CONST_DECLARATION.findall(text))
	return names


def run_refs(root, srcdb, tus, names, jobs):
	"""Every output line of srcdb fold-refs over the TUs, and the TUs that failed or met errors. names: the file of
	the names a constant may have."""
	lines, failed, errors = set(), [], []

	def one(tu):
		file, directory, words = tu
		command = [srcdb, "fold-refs", "--root", root, "--file", file, "--directory", directory, "--names", names,
				   "--"] + words
		result = subprocess.run(command, capture_output=True, text=True, errors="surrogateescape")
		return file, result.returncode, result.stdout

	with concurrent.futures.ThreadPoolExecutor(jobs) as pool:
		for file, code, out in pool.map(one, tus):
			if code == 3:
				errors.append(os.path.relpath(file, root))
			elif code != 0:
				failed.append(os.path.relpath(file, root))
			lines.update(out.splitlines())
	return sorted(lines), failed, errors


class Constant:
	def __init__(self, key, name, scope, form, flags, qualified):
		self.key, self.name, self.scope, self.form, self.qualified = key, name, scope, form, qualified
		self.flags = set() if flags == "-" else set(flags.split(","))
		self.decls = set()  # (file, begin, end)
		self.init = None  # (file, written b, e, address b, e, name b, e)
		self.alias = None  # (alias key, name, file, begin, end)
		self.uses = {}  # (file, begin, end) -> [context, parent, flags]
		self.macros = set()  # (file, line)
		self.reasons = []

	@property
	def file(self):
		return self.key.rsplit(":", 1)[0]


def read_refs(lines):
	constants, arefs, others, errors = {}, collections.defaultdict(set), collections.defaultdict(set), set()
	pending = []
	for line in lines:
		f = line.split("\t")
		if f[0] == "const":
			constants[f[1]] = Constant(*f[1:7])
		elif f[0] == "err":
			errors.add((f[1], int(f[2]), f[3]))
		elif f[0] == "aref":
			arefs[f[1]].add((f[2], int(f[3])))
		elif f[0] == "other":
			others[f[1]].add((f[2], int(f[3])))
		else:
			pending.append(f)
	for f in pending:
		c = constants[f[1]]
		if f[0] == "decl":
			c.decls.add((f[2], int(f[3]), int(f[4])))
		elif f[0] == "init":
			c.init = (f[2],) + tuple(int(x) for x in f[3:9])
		elif f[0] == "alias":
			c.alias = (f[2], f[3], f[4], int(f[5]), int(f[6]))
		elif f[0] == "use":
			at = (f[2], int(f[3]), int(f[4]))
			flags = set() if f[7] == "-" else set(f[7].split(","))
			if at in c.uses:
				c.uses[at][2] |= flags
			else:
				c.uses[at] = [f[5], f[6], flags]
		elif f[0] == "macro":
			c.macros.add((f[2], int(f[3])))
	return constants, arefs, others, errors


# --- the tree's text ------------------------------------------------------------------------------------------------

class Tree:
	"""The files of src/ and gen/, read once. Each text is its bytes as Latin-1, so that an offset in it is the byte
	offset the parse reports, and writing it back gives the same bytes."""

	def __init__(self, root):
		self.root = root
		self.texts = {}
		for top in ("src", "gen"):
			for base, dirs, names in os.walk(os.path.join(root, top)):
				dirs.sort()
				for name in sorted(names):
					if name.endswith(EXTENSIONS):
						path = os.path.relpath(os.path.join(base, name), root)
						with open(os.path.join(root, path), "rb") as source:
							self.texts[path] = source.read().decode("latin-1")

	def index(self, words):
		"""Where the tree spells each of words: name -> [(file, offset)] in code; and the comments naming them."""
		spelled, mentioned = collections.defaultdict(list), collections.defaultdict(list)
		for path, text in self.texts.items():
			if not any(word in text for word in words):
				continue
			for tok in unseam.tokenize(text):
				if tok.kind == "ident" and tok.text in words:
					spelled[tok.text].append((path, tok.start))
				elif tok.kind == "comment":
					for word in set(unseam.IDENT.findall(tok.text)) & words:
						mentioned[word].append((path, tok.start, tok.end))
		return spelled, mentioned

	def line(self, path, offset):
		return self.texts[path].count("\n", 0, offset) + 1


def inside(at, ranges):
	path, offset = at
	return any(p == path and b <= offset < e for p, b, e in ranges)


def line_bounds(text, begin, end):
	"""[start of begin's line, end of end's line including its newline)."""
	start = text.rfind("\n", 0, begin) + 1
	stop = text.find("\n", end)
	return start, len(text) if stop < 0 else stop + 1


def statement_end(text, end):
	"""The offset after the `;` that ends a declaration at end (white space between), or None."""
	k = end
	while k < len(text) and text[k] in " \t\r\n":
		k += 1
	return k + 1 if k < len(text) and text[k] == ";" else None


def removal(text, begin, end):
	"""The range a declaration [begin, end) and its `;` take: whole lines where nothing else is on them (a comment
	after it goes too), else None."""
	stop = statement_end(text, end)
	if stop is None:
		return None
	start, finish = line_bounds(text, begin, stop - 1)
	if text[start:begin].strip():
		return None
	rest = text[stop:finish].strip()
	if rest and not rest.startswith("//"):
		return None
	return start, finish


# --- what each use becomes ------------------------------------------------------------------------------------------

def components(name):
	"""A qualified name's components, split at the `::` outside template arguments (a leading `::` gives '')."""
	parts, depth, start, k = [], 0, 0, 0
	while k < len(name):
		if name[k] == "<":
			depth += 1
		elif name[k] == ">":
			depth -= 1
		elif depth == 0 and name.startswith("::", k):
			parts.append(name[start:k])
			start = k + 2
			k += 1
		k += 1
	parts.append(name[start:])
	return parts


def bare(component):
	"""A component without its template arguments and white space."""
	return "".join(component.split("<", 1)[0].split())


def spelling(c, tree, use_flags):
	"""The function's name as the use writes it: as the initialiser does where that finds it from the use, else
	qualified (from the global namespace only where the use's scopes give the name to another declaration)."""
	path, wb, we, ab, ae, nb, ne = c.init
	name = tree.texts[path][nb:ne]
	written = components(name)
	qualified = components(c.qualified)[1:]
	if len(written) == 1:
		if "visible" in use_flags:
			return name
		scope = "::".join(qualified[:-1])
		if "shadowed" in use_flags:
			return "::" + scope + ("::" if scope else "") + name
		return scope + "::" + name if scope else name
	if written[0] == "" or len(written) >= len(qualified):
		return name
	# qualified relative to the initialiser's scope (a namespace's, a class's): the scopes it leaves out before it
	tail = qualified[len(qualified) - len(written):]
	if [bare(x) for x in tail] != [bare(x) for x in written]:
		return name
	return "::".join(qualified[:len(qualified) - len(written)]) + "::" + name


def value_text(c, tree, use_flags):
	path, wb, we, ab, ae, nb, ne = c.init
	text = tree.texts[path]
	address = "&" + spelling(c, tree, use_flags)
	return text[wb:ab] + address + text[ae:we]


def plan(c, tree, spelled_known):
	"""The reasons c is left, else the edits that fold it: [(file, begin, end, new, kind)]."""
	if c.macros:
		path, line = sorted(c.macros)[0]
		c.reasons.append(f"spelled by a macro's body: {path}:{line}")
	if c.scope == "local":
		c.reasons.append("an automatic local")
	if c.init is None:
		c.reasons.append("no initialiser seen")
	for (path, b, e), (context, parent, flags) in sorted(c.uses.items()):
		where = f"{path}:{tree.line(path, b)}"
		if context == "address":
			c.reasons.append(f"its address is taken: {where}")
		elif "unevaluated" in flags:
			c.reasons.append(f"an unevaluated use: {where}")
		elif context == "call" and (c.form == "cast" or "same" not in c.flags):
			c.reasons.append(f"a call through a converted pointer: {where}")
		elif context == "value" and c.form != "cast" and "same" not in c.flags:
			c.reasons.append(f"its initialiser converts the address implicitly: {where}")
	if c.reasons:
		return []
	edits = []
	for path, b, e in sorted(c.decls):
		span = removal(tree.texts[path], b, e)
		if span is None:
			c.reasons.append(f"a declaration shares its lines or declarators: {path}:{tree.line(path, b)}")
			continue
		edits.append((path, span[0], span[1], "", "decl"))
	for (path, b, e), (context, parent, flags) in sorted(c.uses.items()):
		before = tree.texts[path][b:e]
		if c.name not in before:
			c.reasons.append(f"a use's text does not spell the name: {path}:{tree.line(path, b)}")
			continue
		if context == "call":
			new = spelling(c, tree, flags)
		else:
			new = value_text(c, tree, flags)
			if parent in TIGHT:
				new = "(" + new + ")"
		edits.append((path, b, e, new, context))
	if c.reasons:
		return []
	return edits


def line_spans(text):
	"""(start, end) of every line, its newline included."""
	spans, k = [], 0
	while k < len(text):
		nl = text.find("\n", k)
		stop = len(text) if nl < 0 else nl + 1
		spans.append((k, stop))
		k = stop
	return spans


def comment_lines(text, spans):
	"""Per line: whether it holds a comment and nothing else."""
	starts = [b for b, e in spans]
	comment, code = [False] * len(spans), [False] * len(spans)
	for tok in unseam.tokenize(text):
		first = bisect.bisect_right(starts, tok.start) - 1
		last = bisect.bisect_right(starts, max(tok.start, tok.end - 1)) - 1
		for k in range(first, last + 1):
			(comment if tok.kind == "comment" else code)[k] = True
	return [c and not d for c, d in zip(comment, code)]


def removed_lines(text, removals):
	"""Line starts of the lines the removals take."""
	starts = set()
	for b, e in removals:
		k = b
		while k < e:
			starts.add(k)
			nl = text.find("\n", k)
			k = len(text) if nl < 0 else nl + 1
	return starts


def with_comments(text, removals):
	"""The comment blocks that introduce only removed code: each block of comment lines whose scope (the lines after
	it up to a blank line, the next comment block or a closing brace) holds code and only removed code."""
	gone = removed_lines(text, removals)
	spans = line_spans(text)
	stripped = [text[b:e].strip() for b, e in spans]
	is_comment = comment_lines(text, spans)
	taken = []
	k = 0
	while k < len(spans):
		if spans[k][0] in gone or not is_comment[k]:
			k += 1
			continue
		first = k
		while k < len(spans) and spans[k][0] not in gone and is_comment[k]:
			k += 1
		code = 0
		only = True
		j = k
		while j < len(spans):
			if spans[j][0] in gone:
				code += 1
			elif not stripped[j] or is_comment[j] or stripped[j].startswith("}"):
				break
			else:
				only = False
				break
			j += 1
		if only and code:
			taken.append((spans[first][0], spans[k - 1][1]))
	return taken


def apply(text, edits):
	"""edits: (begin, end, new); none overlap."""
	out = []
	last = len(text)
	for b, e, new in sorted(edits, reverse=True):
		out.append(text[e:last])
		out.append(new)
		last = b
	out.append(text[:last])
	return "".join(reversed(out))


def tidy_blank_lines(text, removals):
	"""Where a removal leaves two blank lines, or a blank line after an opening brace or before a closing one, one
	blank line less: the removals widened by it."""
	wider = []
	for b, e in removals:
		prev_start = text.rfind("\n", 0, b - 1) + 1 if b > 0 else 0
		prev = text[prev_start:b].strip() if b > 0 else ""
		nl = text.find("\n", e)
		nxt_end = len(text) if nl < 0 else nl + 1
		nxt = text[e:nxt_end].strip() if e < len(text) else None
		if b > 0 and prev == "" and nxt == "":
			e = nxt_end
		elif nxt == "" and (b == 0 or prev.endswith("{")):
			e = nxt_end
		elif b > 0 and prev == "" and (nxt is None or nxt.startswith("}")) and prev_start < b:
			b = prev_start
		wider.append((b, e))
	return wider


def merge_ranges(ranges):
	out = []
	for b, e in sorted(ranges):
		if out and b <= out[-1][1]:
			out[-1] = (out[-1][0], max(out[-1][1], e))
		else:
			out.append((b, e))
	return out


# --- the run --------------------------------------------------------------------------------------------------------

def tsv(path, header, rows):
	"""rows' texts are the tree's bytes as Latin-1: written as the UTF-8 they are."""
	with open(path, "w", encoding="utf-8") as out:
		out.write("\t".join(header) + "\n")
		for row in rows:
			cells = (str(x).encode("latin-1", "replace").decode("utf-8", "replace") for x in row)
			out.write("\t".join(x.replace("\t", " ").replace("\n", "\\n") for x in cells) + "\n")


def main():
	parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
	parser.add_argument("--root", default=ROOT)
	parser.add_argument("--compile-commands", action="append")
	parser.add_argument("--jobs", type=int, default=8)
	parser.add_argument("--srcdb", default=os.path.join(ROOT, "build", "srcdb", "srcdb"))
	parser.add_argument("--refs", help="a saved parse (--save-refs) instead of parsing")
	parser.add_argument("--save-refs")
	parser.add_argument("--dry-run", action="store_true")
	parser.add_argument("--out", default=os.path.join(ROOT, "env", "scratch", "Clean", "foldfn"))
	arguments = parser.parse_args()
	root = os.path.realpath(arguments.root)
	os.makedirs(arguments.out, exist_ok=True)
	tree = Tree(root)

	failed, parse_errors = [], []
	if arguments.refs:
		with open(arguments.refs, encoding="utf-8", errors="surrogateescape") as source:
			lines = source.read().splitlines()
	else:
		if not os.path.exists(arguments.srcdb):
			sys.exit(f"foldfn: build {arguments.srcdb} first (harness/srcdb)")
		commands = arguments.compile_commands or [os.path.join(root, "build", "linux", "compile_commands.json")]
		names = os.path.join(os.path.realpath(arguments.out), "names.txt")
		with open(names, "w", encoding="utf-8") as out:
			out.writelines(name + "\n" for name in sorted(constant_names(tree)))
		lines, failed, parse_errors = run_refs(root, arguments.srcdb, tu_commands(root, commands), names,
											   arguments.jobs)
		if arguments.save_refs:
			with open(arguments.save_refs, "w", encoding="utf-8", errors="surrogateescape") as out:
				out.write("".join(line + "\n" for line in lines))
	if failed:
		sys.exit(f"foldfn: the parse of {len(failed)} TUs failed: {failed[:5]}")
	constants, arefs, others, errors = read_refs(lines)

	names = {c.name for c in constants.values()}
	alias_names = {c.alias[1] for c in constants.values() if c.alias}
	spelled, mentioned = tree.index(names | alias_names)

	# a name the tree spells where no parse saw a declaration or use of a constant of that name, nor another
	# declaration of that name (a preprocessor branch the build does not take, a file no TU includes)
	by_name = collections.defaultdict(list)
	for c in constants.values():
		by_name[c.name].append(c)
	for name, group in by_name.items():
		known = [d for c in group for d in c.decls] + [u for c in group for u in c.uses]
		unknown = [at for at in spelled[name] if at not in others[name] and not inside(at, known)]
		if unknown:
			path, offset = unknown[0]
			for c in group:
				c.reasons.append(f"spelled where no parse saw it: {path}:{tree.line(path, offset)}"
								 + (f" (+{len(unknown) - 1})" if len(unknown) > 1 else ""))

	edits = collections.defaultdict(list)  # file -> (b, e, new, kind, constant)
	for c in sorted(constants.values(), key=lambda c: c.key):
		for path, b, e, new, kind in plan(c, tree, spelled):
			edits[path].append((b, e, new, kind, c))

	# the aliases: deleted where every spelling is in a deleted declaration (its own too)
	folded = {c.key for c in constants.values() if not c.reasons}
	alias_rows = {}
	alias_users = collections.defaultdict(list)
	for c in constants.values():
		if c.alias:
			alias_users[c.alias[0]].append(c)
	alias_by_name = collections.defaultdict(list)
	for akey, users in alias_users.items():
		alias_by_name[users[0].alias[1]].append(akey)
	for akey, users in sorted(alias_users.items()):
		_, aname, path, b, e = users[0].alias
		span = removal(tree.texts[path], b, e)
		deleted = [(p, x, y) for p, items in edits.items() for x, y, new, kind, c in items if kind == "decl"]
		own = (path, span[0], span[1]) if span else None
		reason = ""
		if any(c.key not in folded for c in users):
			reason = "a constant written with it is left"
		elif span is None:
			reason = "its declaration shares its lines"
		else:
			outside = [at for at in arefs[akey] if not inside(at, deleted + [own])]
			if outside:
				reason = f"spelled elsewhere: {outside[0][0]}:{tree.line(*sorted(outside)[0])}"
			else:
				known = [(p, o, o + 1) for k in alias_by_name[aname] for p, o in arefs[k]]
				unknown = [at for at in spelled[aname] if not inside(at, known)]
				if unknown:
					reason = f"spelled where no parse saw it: {unknown[0][0]}:{tree.line(*unknown[0])}"
		if not reason:
			edits[path].append((span[0], span[1], "", "alias", None))
		alias_rows[akey] = (path, tree.line(path, b), aname, "kept" if reason else "deleted", reason)

	# the comment blocks that introduce only deleted declarations; the blank lines a deletion doubles
	comment_rows = []
	changed = {}
	for path, items in sorted(edits.items()):
		text = tree.texts[path]
		removals = [(b, e) for b, e, new, kind, c in items if kind in ("decl", "alias")]
		for b, e in with_comments(text, removals):
			comment_rows.append((path, tree.line(path, b), "deleted", text[b:e].rstrip("\n")))
			removals.append((b, e))
		removals = merge_ranges(tidy_blank_lines(text, merge_ranges(removals)))
		replacements = [(b, e, new) for b, e, new, kind, c in items if kind not in ("decl", "alias")]
		replacements = [r for r in replacements if not any(x <= r[0] < y for x, y in removals)]
		spans = sorted([(b, e) for b, e in removals] + [(b, e) for b, e, _ in replacements])
		for (b1, e1), (b2, e2) in zip(spans, spans[1:]):
			if b2 < e1:
				sys.exit(f"foldfn: overlapping edits in {path} at line {tree.line(path, b2)}")
		changed[path] = apply(text, [(b, e, "") for b, e in removals] + replacements)

	# the names that were only constants' (a name another declaration has too is that one's in a comment)
	gone_names = {c.name for c in constants.values() if c.key in folded} - \
		{c.name for c in constants.values() if c.key not in folded} - {name for name, at in others.items() if at}
	deleted_comments = {(r[0], r[1]) for r in comment_rows}
	for name in sorted(gone_names):
		for path, b, e in mentioned[name]:
			line = tree.line(path, b)
			if (path, line) in deleted_comments:
				continue
			comment_rows.append((path, line, "left: names " + name, tree.texts[path][b:e]))

	if not arguments.dry_run:
		for path, text in changed.items():
			with open(os.path.join(root, path), "wb") as target:
				target.write(text.encode("latin-1"))

	# the tables
	out = arguments.out
	tsv(os.path.join(out, "constants.tsv"),
		("file", "line", "name", "scope", "form", "flags", "function", "uses", "action", "reason"),
		[(c.file, tree.line(c.file, int(c.key.rsplit(":", 1)[1])), c.name, c.scope, c.form,
		  ",".join(sorted(c.flags)) or "-", c.qualified, len(c.uses), "left" if c.reasons else "folded",
		  "; ".join(c.reasons)) for c in sorted(constants.values(), key=lambda c: c.key)])
	use_rows = []
	for c in sorted(constants.values(), key=lambda c: c.key):
		news = {(path, b, e): new for path, items in edits.items() for b, e, new, kind, k in items if k is c}
		for (path, b, e), (context, parent, flags) in sorted(c.uses.items()):
			new = news.get((path, b, e))
			use_rows.append((c.file, tree.line(c.file, int(c.key.rsplit(":", 1)[1])), c.name, path,
							 tree.line(path, b), context + ("," + ",".join(sorted(flags)) if flags else ""),
							 tree.texts[path][b:e], new if new is not None else "", "rewritten" if new is not None
							 else "left"))
	tsv(os.path.join(out, "uses.tsv"),
		("constant_file", "constant_line", "name", "file", "line", "context", "before", "after", "action"), use_rows)
	tsv(os.path.join(out, "aliases.tsv"), ("file", "line", "name", "action", "reason"),
		[alias_rows[k] for k in sorted(alias_rows)])
	tsv(os.path.join(out, "comments.tsv"), ("file", "line", "action", "text"), sorted(comment_rows))
	count = collections.Counter("left" if c.reasons else "folded" for c in constants.values())
	reasons = collections.Counter(r.split(":")[0] for c in constants.values() for r in c.reasons[:1])
	with open(os.path.join(out, "report.txt"), "w", encoding="utf-8") as report:
		report.write(f"constants {len(constants)}: folded {count['folded']}, left {count['left']}\n")
		for reason, n in reasons.most_common():
			report.write(f"  left: {reason}: {n}\n")
		kinds = collections.Counter(row[8] for row in use_rows)
		report.write(f"uses {len(use_rows)}: rewritten {kinds['rewritten']}, left {kinds['left']}\n")
		contexts = collections.Counter(row[5].split(",")[0] for row in use_rows if row[8] == "rewritten")
		for context, n in contexts.most_common():
			report.write(f"  rewritten {context}: {n}\n")
		acount = collections.Counter(row[3] for row in alias_rows.values())
		report.write(f"aliases {len(alias_rows)}: deleted {acount['deleted']}, kept {acount['kept']}\n")
		ccount = collections.Counter(row[2].split(":")[0] for row in comment_rows)
		report.write(f"comments: deleted {ccount['deleted']}, left naming a folded constant {ccount['left']}\n")
		report.write(f"files changed {len(changed)}{' (dry run)' if arguments.dry_run else ''}\n")
		report.write(f"TUs with parse errors {len(parse_errors)}: {' '.join(parse_errors[:20])}\n")
		for path, line, text in sorted(errors)[:50]:
			report.write(f"  err {path}:{line}: {text}\n")
	with open(os.path.join(out, "report.txt"), encoding="utf-8") as report:
		sys.stdout.write(report.read())


if __name__ == "__main__":
	main()
