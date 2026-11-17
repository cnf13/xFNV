#!/usr/bin/env python3
"""Statics go home: the file-level globals a table assigns to one function or one class become that function's
static or that class's static data member (docs/PLAN.md Part 2, between the names and step 5).

Usage: env/venv/bin/python harness/tools/statics.py measure [--root DIR] [--build DIR] [--table TSV] [--out DIR]
                                                          [--jobs N]
       env/venv/bin/python harness/tools/statics.py apply   (the same options) [--dry-run]

TABLE (default env/scratch/Clean/3d/data.tsv) holds a row per global: `new` its name in the tree, `home_name` its
name at home, `home` where it belongs: `function:<qualified function>`, `class:<qualified class>` or `file` (not
moved). The tree is parsed (libclang, the build's compile_commands.json: --build, default ROOT/build/linux; build
first) in every translation unit that spells a row's name; every spelling outside comments and strings must be one
the parse saw, so a use in another preprocessor branch keeps its row where it is.

measure  writes OUT/sites.tsv (default OUT env/scratch/Clean/5b): per global, what apply does or why it does not.
apply    measures, then edits the tree; OUT/applied.tsv lists every edit (file, line, what, old text, new text),
         harness/addresses.tsv gets the moved names. A second run finds nothing left to do.

A function static. Its home function is the one function that uses the global (the home's name, or the home's
name with a numeric suffix); the global's every use is in it. Three shapes:
  guarded   the function tests a bit of a guard variable, sets it and assigns the global, once:
                if ((guard & 4) == 0) { guard |= 4u; [locals...] value = init; }
            The block becomes `static T name = init;` where it stood; when it computes init in several statements,
            `static T name = [&] { ...; return init; }();` (a temporary declared outside the block and used only in
            it moves into it). A block that only placement-constructs the global (`value.~T(); ::new (&value)
            T(args);`) becomes `static T name(args);`. An empty block (its static is already a function static) is
            deleted. Every use of the global must come after the block, inside the block's enclosing statement.
            The guard bit is tested in that block only; the guard variable is deleted with its last bit.
  constant  no guard: the global's file-level definition has no dynamic initialiser and nothing to destroy (the
            object file's static initialiser does not refer to it); `static T name = <its initialiser>;` goes to
            the top of the function.
  A guarded global the table does not name (its guard does) moves with its guard under its present name.
The function-static form initialises thread-safely where the flag did not: accepted. Not moved, with the reason in
sites.tsv: a global used by several functions (an inlined copy of the home), outside a function, in a template,
by a macro or in another preprocessor branch; a guard bit tested twice or a block of another shape; a global with
a dynamic initialiser or a destructor (moving it would change when it is constructed); a home name the function
already spells (then the interim name is kept).

A class static. The global becomes a static data member under its home name: declared in the class body (after the
class's last static data member, else at its end, under `public:` when the end is not public), defined where the
global was defined (`T Class::name = init;`, so its place in the file's initialisation order is kept); its extern
declarations go; every use is renamed (the bare name in the class's own member functions, `Class::name` elsewhere).
Not moved: a class the parse does not find or that is a template, a name the class or its bases declare, a class
in src/SDK, a definition whose file does not see the class's definition.
"""

from __future__ import annotations

import argparse
import collections
import concurrent.futures
import csv
import json
import os
import re
import shlex
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
CLANG_PACKAGE = "/usr/lib/python3.14/site-packages"
LIBCLANG = "/usr/lib/libclang.so"
EXTENSIONS = (".h", ".hpp", ".inl", ".cpp", ".inc", ".c")
IDENT = re.compile(rb"[A-Za-z_]\w*")
# `(guard & 4) == 0`, `!(guard & 4)`, with or without the outer parentheses and an unsigned suffix
GUARD_TEST = re.compile(rb"^\(*\s*(?:!\s*\(\s*(\w+)\s*&\s*(\d+)u?\s*\)|\(\s*(\w+)\s*&\s*(\d+)u?\s*\)\s*==\s*0)\s*\)*$")
GUARD_SET = re.compile(rb"^(\w+)\s*\|=\s*(\d+)u?$")


def fail(message: str) -> None:
	sys.exit(f"statics: {message}")


# ---------------------------------------------------------------------------------------------------------------
# text helpers
# ---------------------------------------------------------------------------------------------------------------

def blank_code(text: bytes) -> bytes:
	"""The text with comments, string and character literals replaced by spaces (newlines kept)."""
	out = bytearray(text)
	i, n = 0, len(text)
	while i < n:
		c = text[i]
		if c == 0x2F and i + 1 < n and text[i + 1] == 0x2F:
			j = text.find(b"\n", i)
			j = n if j < 0 else j
			out[i:j] = b" " * (j - i)
			i = j
		elif c == 0x2F and i + 1 < n and text[i + 1] == 0x2A:
			j = text.find(b"*/", i + 2)
			j = n if j < 0 else j + 2
			out[i:j] = bytes(ch if ch == 0x0A else 0x20 for ch in text[i:j])
			i = j
		elif c in (0x22, 0x27):
			if c == 0x27 and i > 0 and chr(text[i - 1]).isalnum() and i + 1 < n and chr(text[i + 1]).isalnum():
				i += 1
				continue
			if c == 0x22 and i > 0 and text[i - 1] == 0x52:  # R"( raw string )"
				close = text.find(b"(", i)
				delim = b")" + text[i + 1:close] + b'"'
				j = text.find(delim, close)
				j = n if j < 0 else j + len(delim)
			else:
				j = i + 1
				while j < n and text[j] != c and text[j] != 0x0A:
					j += 2 if text[j] == 0x5C else 1
				j = min(j + 1, n)
			out[i:j] = bytes(ch if ch == 0x0A else 0x20 for ch in text[i:j])
			i = j
		else:
			i += 1
	return bytes(out)


def line_of(text: bytes, offset: int) -> int:
	return text.count(b"\n", 0, offset) + 1


def line_start(text: bytes, offset: int) -> int:
	return text.rfind(b"\n", 0, offset) + 1


def line_end(text: bytes, offset: int) -> int:
	"""The offset just past the newline of the line holding offset (or the end)."""
	j = text.find(b"\n", offset)
	return len(text) if j < 0 else j + 1


def indent_at(text: bytes, offset: int) -> bytes:
	start = line_start(text, offset)
	m = re.match(rb"[ \t]*", text[start:])
	return m.group(0)


def tree_files(root: str):
	for top in ("src", "gen"):
		for base, _, names in os.walk(os.path.join(root, top)):
			for name in names:
				if name.endswith(EXTENSIONS):
					yield os.path.relpath(os.path.join(base, name), root)


def strip_templates(name: str) -> str:
	out, depth = [], 0
	for c in name:
		if c == "<":
			depth += 1
		elif c == ">" and depth:
			depth -= 1
		elif not depth:
			out.append(c)
	return "".join(out).replace(" ", "")


# ---------------------------------------------------------------------------------------------------------------
# the parse (worker processes)
# ---------------------------------------------------------------------------------------------------------------

_ci = None
_state: dict = {}


def _clang():
	global _ci
	if _ci is None:
		sys.path.append(CLANG_PACKAGE)
		import clang.cindex as ci  # noqa: E402
		ci.Config.set_library_file(LIBCLANG)
		_ci = ci
	return _ci


def compile_arguments(entry: dict, resource: str) -> list[str]:
	arguments = entry["arguments"] if "arguments" in entry else shlex.split(entry["command"])
	out, skip = [], False
	for argument in arguments[1:]:
		if skip:
			skip = False
			continue
		if argument in ("-o", "-c", "-MF", "-MT", "-MQ"):
			skip = True
			continue
		if argument in ("-MD", "-MMD") or argument == entry["file"]:
			continue
		out.append(argument)
	return out + ["-resource-dir", resource, "-Wno-everything"]


def _init_worker(root: str, names: list[str], classes: list[str], resource: str) -> None:
	_clang()
	_state.update(root=root, names=set(names), classes=set(classes), resource=resource, index=_ci.Index.create())


def _rel(location) -> str | None:
	if location is None or location.file is None:
		return None
	path = os.path.realpath(location.file.name)
	root = _state["root"]
	if not path.startswith(root + os.sep):
		return None
	return os.path.relpath(path, root)


def _qualified(cursor) -> str:
	ci = _ci
	parts = [cursor.spelling]
	parent = cursor.semantic_parent
	while parent is not None and parent.kind != ci.CursorKind.TRANSLATION_UNIT:
		if parent.kind == ci.CursorKind.LINKAGE_SPEC:
			parent = parent.semantic_parent
			continue
		parts.append(parent.spelling or "(anonymous)")
		parent = parent.semantic_parent
	return "::".join(reversed(parts))


_spelling_function = None


def _spelling(location) -> tuple[str | None, int]:
	"""(file, offset) where the text of a location is spelled: a macro argument's place in the call, not the
	expansion's."""
	global _spelling_function
	import ctypes
	ci = _ci
	if _spelling_function is None:
		_spelling_function = ci.conf.lib.clang_getSpellingLocation
		_spelling_function.argtypes = [ci.SourceLocation, ctypes.POINTER(ci.c_object_p),
									   ctypes.POINTER(ctypes.c_uint), ctypes.POINTER(ctypes.c_uint),
									   ctypes.POINTER(ctypes.c_uint)]
		_spelling_function.restype = None
	f, line, column, offset = ci.c_object_p(), ctypes.c_uint(), ctypes.c_uint(), ctypes.c_uint()
	_spelling_function(location, ctypes.byref(f), ctypes.byref(line), ctypes.byref(column), ctypes.byref(offset))
	if not f:
		return None, 0
	path = os.path.realpath(ci.File(f).name)
	root = _state["root"]
	if not path.startswith(root + os.sep):
		return None, int(offset.value)
	return os.path.relpath(path, root), int(offset.value)


def _extent(cursor) -> tuple[int, int]:
	return cursor.extent.start.offset, cursor.extent.end.offset


def _scope_kind(cursor) -> str:
	k = _ci.CursorKind
	parent = cursor.semantic_parent
	if parent is None or parent.kind in (k.TRANSLATION_UNIT, k.NAMESPACE, k.LINKAGE_SPEC):
		return "file"
	if parent.kind in (k.CLASS_DECL, k.STRUCT_DECL, k.UNION_DECL, k.CLASS_TEMPLATE,
					   k.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION):
		return "class"
	return "function"


def _in_template(cursor) -> bool:
	k = _ci.CursorKind
	while cursor is not None and cursor.kind != k.TRANSLATION_UNIT:
		if cursor.kind in (k.FUNCTION_TEMPLATE, k.CLASS_TEMPLATE, k.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION):
			return True
		cursor = cursor.semantic_parent
	return False


def _class_members(cursor, seen: set) -> set[str]:
	"""Every member name of a class and of its bases."""
	k = _ci.CursorKind
	names = set()
	if cursor is None:
		return names
	usr = cursor.get_usr()
	if usr in seen:
		return names
	seen.add(usr)
	definition = cursor.get_definition() or cursor
	for child in definition.get_children():
		if child.kind == k.CXX_BASE_SPECIFIER:
			base = child.type.get_declaration()
			names |= _class_members(base, seen)
		elif child.spelling:
			names.add(child.spelling)
	return names


def _class_record(cursor, file: str) -> dict:
	k = _ci.CursorKind
	members = []
	for child in cursor.get_children():
		if child.kind == k.CXX_BASE_SPECIFIER:
			continue
		start, end = _extent(child)
		members.append({"kind": child.kind.name, "name": child.spelling, "start": start, "end": end,
						"access": child.access_specifier.name if child.access_specifier else "",
						"file": _rel(child.extent.start)})
	start, end = _extent(cursor)
	return {"name": _qualified(cursor), "usr": cursor.get_usr(), "file": file, "start": start, "end": end,
			"kind": cursor.kind.name, "template": _in_template(cursor), "members": members,
			"all_members": sorted(_class_members(cursor, set()))}


def _constant(expression) -> bool:
	"""Whether an initialiser is built of literals and operators only (a constant initialisation)."""
	k = _ci.CursorKind
	allowed = {k.INTEGER_LITERAL, k.FLOATING_LITERAL, k.CHARACTER_LITERAL, k.STRING_LITERAL, k.CXX_BOOL_LITERAL_EXPR,
			   k.CXX_NULL_PTR_LITERAL_EXPR, k.INIT_LIST_EXPR, k.UNEXPOSED_EXPR, k.PAREN_EXPR, k.CSTYLE_CAST_EXPR,
			   k.CXX_STATIC_CAST_EXPR, k.CXX_FUNCTIONAL_CAST_EXPR, k.UNARY_OPERATOR, k.BINARY_OPERATOR,
			   k.CONDITIONAL_OPERATOR, k.CXX_UNARY_EXPR, k.TYPE_REF}
	for cursor in expression.walk_preorder():
		if cursor.kind in allowed:
			continue
		if cursor.kind == k.DECL_REF_EXPR and cursor.referenced is not None and cursor.referenced.kind in (
				k.ENUM_CONSTANT_DECL, k.FUNCTION_DECL, k.CXX_METHOD):
			continue
		return False
	return True


def _statement(cursor) -> dict:
	"""A statement of a guard block: kind, extent, and for an assignment its sides."""
	k = _ci.CursorKind
	start, end = _extent(cursor)
	record = {"kind": cursor.kind.name, "start": start, "end": end}
	if cursor.kind in (k.BINARY_OPERATOR, k.COMPOUND_ASSIGNMENT_OPERATOR):
		children = list(cursor.get_children())
		try:
			record["op"] = cursor.binary_operator.name
		except Exception:
			record["op"] = ""
		if len(children) == 2:
			record["lhs"] = list(_extent(children[0]))
			record["rhs"] = list(_extent(children[1]))
			lhs = children[0]
			while lhs.kind == k.UNEXPOSED_EXPR and len(list(lhs.get_children())) == 1:
				lhs = next(lhs.get_children())
			if lhs.kind == k.DECL_REF_EXPR and lhs.referenced is not None:
				record["lhs_usr"] = lhs.referenced.get_usr()
	return record


def parse_unit(job: tuple) -> dict:
	ci = _clang()
	k = ci.CursorKind
	entry, interesting = job
	interesting = set(interesting)
	root, names, classes = _state["root"], _state["names"], _state["classes"]
	os.chdir(entry["directory"])
	main = os.path.relpath(os.path.realpath(entry["file"]), root)
	options = ci.TranslationUnit.PARSE_INCOMPLETE
	try:
		tu = _state["index"].parse(entry["file"], compile_arguments(entry, _state["resource"]), options=options)
	except ci.TranslationUnitLoadError as error:
		return {"unit": main, "error": str(error)}
	errors = [f"{_rel(d.location)}:{d.location.line}: {d.spelling}" for d in tu.diagnostics
			  if d.severity >= ci.Diagnostic.Error and _rel(d.location)]
	decls, refs, functions, class_records, class_seen, others = [], [], {}, [], [], set()
	function_kinds = {k.FUNCTION_DECL, k.CXX_METHOD, k.CONSTRUCTOR, k.DESTRUCTOR, k.CONVERSION_FUNCTION,
					  k.FUNCTION_TEMPLATE}
	class_kinds = {k.CLASS_DECL, k.STRUCT_DECL, k.UNION_DECL, k.CLASS_TEMPLATE}
	scope_kinds = {k.NAMESPACE, k.LINKAGE_SPEC, k.UNEXPOSED_DECL}

	def function_record(cursor, file):
		body = None
		for child in cursor.get_children():
			if child.kind == k.COMPOUND_STMT:
				body = child
		start, end = _extent(cursor)
		parent = cursor.semantic_parent
		return {"usr": cursor.get_usr(), "name": _qualified(cursor), "file": file, "start": start, "end": end,
				"body": list(_extent(body)) if body is not None else None,
				"class": _qualified(parent) if parent is not None and parent.kind in class_kinds else "",
				"template": _in_template(cursor), "locals": [], "local_refs": [], "ifs": [], "refs": 0}

	def visit_function(fcursor, file):
		record = function_record(fcursor, file)
		local_usrs = {}
		stack = [(fcursor, None)]
		while stack:
			cursor, parent = stack.pop()
			kind = cursor.kind
			if kind not in (k.VAR_DECL, k.DECL_REF_EXPR) and cursor.spelling in names:
				others.add(_spelling(cursor.location))
			if kind == k.VAR_DECL or kind == k.PARM_DECL:
				usr = cursor.get_usr()
				scope = _scope_kind(cursor) if kind == k.VAR_DECL else "function"
				if scope == "function":
					start, end = _extent(cursor)
					local_usrs[usr] = cursor.spelling
					record["locals"].append({"usr": usr, "name": cursor.spelling, "start": start, "end": end,
											 "param": kind == k.PARM_DECL,
											 "static": cursor.storage_class == ci.StorageClass.STATIC,
											 "parent": parent.kind.name if parent is not None else ""})
				if cursor.spelling in names and kind == k.VAR_DECL:
					decls.append(decl_record(cursor, file, record["usr"]))
			elif kind == k.DECL_REF_EXPR:
				target = cursor.referenced
				if target is not None and target.kind == k.VAR_DECL:
					if target.spelling in names:
						refs.append(ref_record(cursor, target, record["usr"]))
						record["refs"] += 1
					elif _scope_kind(target) == "function":
						file_, offset = _spelling(cursor.location)
						record["local_refs"].append({"usr": target.get_usr(), "offset": offset, "file": file_})
				elif cursor.spelling in names:
					others.add(_spelling(cursor.location))
			elif kind == k.IF_STMT:
				children = list(cursor.get_children())
				if len(children) >= 2:
					cond, then = children[0], children[1]
					cs, ce = _extent(cond)
					if ce - cs < 120 and _rel(cond.extent.start) == file:
						start, end = _extent(cursor)
						statements = []
						if then.kind == k.COMPOUND_STMT:
							statements = [_statement(child) for child in then.get_children()]
						record["ifs"].append({"start": start, "end": end, "cond": [cs, ce],
											  "then": list(_extent(then)), "then_kind": then.kind.name,
											  "else": len(children) > 2, "statements": statements,
											  "parent": list(_extent(parent)) if parent is not None else None,
											  "parent_kind": parent.kind.name if parent is not None else ""})
			for child in reversed(list(cursor.get_children())):
				stack.append((child, cursor))
		if record["refs"] or any(local["name"] in names for local in record["locals"]):
			functions[record["usr"] + "@" + file] = record

	def decl_record(cursor, file, function):
		start, end = _extent(cursor)
		canonical = cursor.type.get_canonical()
		init, constant = None, True
		children = [child for child in cursor.get_children()
					if child.kind not in (k.TYPE_REF, k.NAMESPACE_REF, k.TEMPLATE_REF)
					and not child.kind.is_attribute()]
		expressions = [child for child in children if child.kind.is_expression() or child.kind == k.INIT_LIST_EXPR]
		if expressions:
			# from the first expression to the declaration's end: a constructor's arguments are children of the
			# declaration themselves
			init = [expressions[0].extent.start.offset, end]
			constant = all(_constant(child) for child in expressions)
		alias = None
		declaration = cursor.type.get_declaration()
		if declaration is not None and declaration.kind in (k.TYPE_ALIAS_DECL, k.TYPEDEF_DECL):
			alias_start, alias_end = _extent(declaration)
			alias = {"name": declaration.spelling, "file": _rel(declaration.location), "start": alias_start,
					 "end": alias_end, "offset": declaration.location.offset}
		return {"name": cursor.spelling, "usr": cursor.get_usr(), "file": file, "start": start, "end": end,
				"offset": cursor.location.offset, "definition": cursor.is_definition(),
				"scope": _scope_kind(cursor), "storage": cursor.storage_class.name if cursor.storage_class else "",
				"type": cursor.type.spelling, "pod": canonical.is_pod(), "init": init, "constant": constant,
				"function": function, "alias": alias,
				"class": _qualified(cursor.semantic_parent) if _scope_kind(cursor) == "class" else "",
				"mangled": cursor.mangled_name or "", "unit": main}

	def ref_record(cursor, target, function):
		file_, offset = _spelling(cursor.location)
		return {"name": target.spelling, "usr": target.get_usr(), "file": file_, "offset": offset,
				"function": function, "scope": _scope_kind(target), "unit": main,
				"expansion": [_rel(cursor.location), cursor.location.offset]}

	def visit(cursor, file):
		kind = cursor.kind
		if kind in scope_kinds:
			for child in cursor.get_children():
				child_file = _rel(child.location)
				if child_file:
					visit(child, child_file)
			return
		if kind in class_kinds and cursor.is_definition():
			name = _qualified(cursor)
			if name in classes:
				class_seen.append(name)
				class_records.append(_class_record(cursor, file))
			nested = any(wanted.startswith(name + "::") for wanted in classes)
			if file not in interesting and not nested:
				return
			for child in cursor.get_children():
				child_file = _rel(child.location)
				if child_file and (child_file in interesting or child.kind in class_kinds):
					visit(child, child_file)
			return
		if file not in interesting:
			return
		if kind in function_kinds and cursor.is_definition():
			visit_function(cursor, file)
			return
		if kind == k.VAR_DECL:
			if cursor.spelling in names:
				decls.append(decl_record(cursor, file, ""))
			# a file-level initialiser using a global
			for child in cursor.walk_preorder():
				if child.kind == k.DECL_REF_EXPR:
					target = child.referenced
					if target is not None and target.kind == k.VAR_DECL and target.spelling in names:
						refs.append(ref_record(child, target, ""))
			return
		if kind == k.FIELD_DECL or kind == k.ENUM_DECL or kind == k.TYPEDEF_DECL or kind == k.TYPE_ALIAS_DECL:
			if cursor.spelling in names:
				others.add(_spelling(cursor.location))
			return
		for child in cursor.walk_preorder():
			if child.kind == k.DECL_REF_EXPR:
				target = child.referenced
				if target is not None and target.kind == k.VAR_DECL:
					if target.spelling in names:
						refs.append(ref_record(child, target, ""))
					continue
			if child.kind != k.VAR_DECL and child.spelling in names:
				others.add(_spelling(child.location))

	for top in tu.cursor.get_children():
		file = _rel(top.location)
		if file:
			visit(top, file)
	return {"unit": main, "errors": errors[:20], "decls": decls, "refs": refs, "functions": list(functions.values()),
			"classes": class_records, "class_seen": sorted(set(class_seen)),
			"others": sorted(other for other in others if other[0])}


# ---------------------------------------------------------------------------------------------------------------
# measure
# ---------------------------------------------------------------------------------------------------------------

def load_rows(table: str) -> list[dict]:
	with open(table, encoding="utf-8", newline="") as source:
		rows = list(csv.DictReader(source, delimiter="\t"))
	out = []
	for row in rows:
		home = row.get("home", "")
		if row.get("action") != "rename" or not home.startswith(("function:", "class:")):
			continue
		kind, _, owner = home.partition(":")
		out.append({"name": row["new"], "home_name": row["home_name"], "kind": kind, "owner": owner,
					"address": row.get("address", ""), "declared": row.get("declared", ""),
					"flag": row.get("flag", "")})
	return out


class Tree:
	def __init__(self, root: str):
		self.root = root
		self.texts: dict[str, bytes] = {}
		self.blanks: dict[str, bytes] = {}

	def text(self, path: str) -> bytes:
		if path not in self.texts:
			with open(os.path.join(self.root, path), "rb") as source:
				self.texts[path] = source.read()
		return self.texts[path]

	def blank(self, path: str) -> bytes:
		if path not in self.blanks:
			self.blanks[path] = blank_code(self.text(path))
		return self.blanks[path]


def spellings(tree: Tree, names: set[str]) -> dict[str, list[tuple[str, int, bool]]]:
	"""name -> [(file, offset, in code)] for every spelling of a name in src and gen."""
	found = collections.defaultdict(list)
	pattern = re.compile(rb"\b(" + b"|".join(re.escape(n.encode()) for n in sorted(names, key=len, reverse=True))
						 + rb")\b")
	for path in tree_files(tree.root):
		text = tree.text(path)
		if not pattern.search(text):
			continue
		blank = tree.blank(path)
		for m in pattern.finditer(text):
			found[m.group(1).decode()].append((path, m.start(), blank[m.start():m.end()] == m.group(1)))
	return found


def compile_commands(root: str, build: str) -> dict[str, dict]:
	with open(os.path.join(build, "compile_commands.json"), encoding="utf-8") as source:
		commands = json.load(source)
	units = {}
	for entry in commands:
		path = os.path.relpath(os.path.realpath(os.path.join(entry["directory"], entry["file"])), root)
		units.setdefault(path, entry)
	return units


def includers(tree: Tree, units: dict[str, dict], header: str) -> list[str]:
	"""Translation units including the header directly (by its file name)."""
	base = os.path.basename(header).encode()
	pattern = re.compile(rb'^\s*#\s*include\s*"([^"]*' + re.escape(base) + rb')"', re.M)
	found = []
	for unit in sorted(units):
		if not unit.startswith("src/"):
			continue
		for m in pattern.finditer(tree.text(unit)):
			spelled = m.group(1).decode()
			for base_dir in (os.path.dirname(unit), "src", "harness/seam", "gen"):
				if os.path.normpath(os.path.join(base_dir, spelled)) == header:
					found.append(unit)
					break
	return found


def parse(tree: Tree, units: dict[str, dict], names: set[str], classes: set[str], wanted: dict[str, set[str]],
		  jobs: int) -> list[dict]:
	resource = subprocess.run(["clang", "-print-resource-dir"], capture_output=True, text=True,
							  check=True).stdout.strip()
	work = [(units[unit], sorted(files)) for unit, files in sorted(wanted.items())]
	print(f"statics: parsing {len(work)} translation units", file=sys.stderr)
	results = []
	with concurrent.futures.ProcessPoolExecutor(jobs, initializer=_init_worker,
												initargs=(tree.root, sorted(names), sorted(classes),
														  resource)) as pool:
		for done, result in enumerate(pool.map(parse_unit, work), 1):
			if "error" in result:
				fail(f"{result['unit']}: {result['error']}")
			results.append(result)
			if done % 50 == 0:
				print(f"statics: {done}/{len(work)} parsed", file=sys.stderr)
	return results


class Model:
	"""The parse's facts, merged over the translation units (a header's facts once)."""

	def __init__(self, results: list[dict]):
		self.decls: dict[tuple, dict] = {}
		self.refs: dict[tuple, dict] = {}
		self.functions: dict[tuple, dict] = {}
		self.classes: dict[str, dict] = {}
		self.class_seen: dict[str, set] = collections.defaultdict(set)
		self.errors = []
		self.others: set[tuple] = set()
		for result in results:
			self.errors.extend(result["errors"])
			for decl in result["decls"]:
				self.decls.setdefault((decl["file"], decl["offset"]), decl)
			for ref in result["refs"]:
				if ref["file"]:
					self.refs.setdefault((ref["file"], ref["offset"]), ref)
			for function in result["functions"]:
				key = (function["file"], function["start"])
				known = self.functions.get(key)
				if known is None or len(function["ifs"]) > len(known["ifs"]):
					self.functions[key] = function
			for record in result["classes"]:
				self.classes.setdefault(record["name"], record)
			for name in result["class_seen"]:
				self.class_seen[name].add(result["unit"])
			self.others.update(tuple(other) for other in result["others"])
		self.function_by_usr = {}
		for function in self.functions.values():
			self.function_by_usr.setdefault(function["usr"], []).append(function)

	def function_of(self, usr: str, file: str | None = None) -> dict | None:
		found = self.function_by_usr.get(usr, [])
		if file:
			found = [f for f in found if f["file"] == file] or found
		return found[0] if found else None


def run_parse(tree: Tree, units: dict[str, dict], names: set[str], classes: set[str], jobs: int):
	found = spellings(tree, names)
	files_by_unit: dict[str, set[str]] = collections.defaultdict(set)
	headers = set()
	for name, places in found.items():
		for path, _offset, code in places:
			if not code:
				continue
			if path in units:
				files_by_unit[path].add(path)
			else:
				headers.add(path)
	# a header is walked in one unit that includes it (the first that spells a name, else the first includer)
	for header in sorted(headers):
		users = [unit for unit in sorted(files_by_unit) if header in includes_of(tree, unit)]
		chosen = users[0] if users else next(iter(includers(tree, units, header)), None)
		if chosen:
			files_by_unit[chosen].add(header)
	# every unit also records whether it sees a wanted class (the definition files' visibility)
	results = parse(tree, units, names, classes, files_by_unit, jobs)
	return found, Model(results)


_includes_cache: dict[str, set] = {}


def includes_of(tree: Tree, unit: str) -> set[str]:
	"""The tree files a unit includes, transitively (quoted includes resolved as the compiler does)."""
	if unit in _includes_cache:
		return _includes_cache[unit]
	seen, todo = set(), [unit]
	pattern = re.compile(rb'^\s*#\s*include\s*"([^"]+)"', re.M)
	while todo:
		path = todo.pop()
		try:
			text = tree.text(path)
		except OSError:
			continue
		for m in pattern.finditer(text):
			spelled = m.group(1).decode()
			for base in (os.path.dirname(path), "src", "harness/seam", "gen"):
				candidate = os.path.normpath(os.path.join(base, spelled))
				if os.path.isfile(os.path.join(tree.root, candidate)):
					if candidate not in seen:
						seen.add(candidate)
						todo.append(candidate)
					break
	_includes_cache[unit] = seen
	return seen

# ---------------------------------------------------------------------------------------------------------------
# the objects: what a translation unit initialises at startup
# ---------------------------------------------------------------------------------------------------------------

STARTUP = ("__cxx_global_var_init", "_GLOBAL__sub_I", "__cxx_global_array_dtor", "__dtor_")


class Objects:
	def __init__(self, units: dict[str, dict]):
		self.units = units
		self.cache: dict[str, set | None] = {}

	def startup_targets(self, unit: str) -> set | None:
		"""The symbols the unit's static initialisation code refers to (a dynamic initialiser, a destructor's
		registration); None when the unit has no object."""
		if unit in self.cache:
			return self.cache[unit]
		import bisect
		import objsame
		entry = self.units.get(unit)
		path = entry and os.path.join(entry["directory"], entry.get("output", ""))
		if not entry or not entry.get("output") or not os.path.exists(path):
			self.cache[unit] = None
			return None
		obj = objsame.read_elf(path)
		functions = collections.defaultdict(list)
		for symbol in obj.symbols:
			if symbol.shndx in obj.sections and symbol.size and symbol.type == 2:
				functions[symbol.shndx].append((symbol.value, symbol.value + symbol.size, symbol.name))
		for spans in functions.values():
			spans.sort()
		found = set()
		for sec in obj.sections.values():
			if not sec.code:
				continue
			spans = functions.get(sec.index, [])
			starts = [span[0] for span in spans]
			startup_section = sec.name.startswith(".text.startup")
			for off, rtype, symi, addend, width in sec.relocs:
				i = bisect.bisect_right(starts, off) - 1
				holder = spans[i][2] if i >= 0 and spans[i][0] <= off < spans[i][1] else ""
				if startup_section or holder.startswith(STARTUP):
					found.add(obj.target(sec, off, rtype, symi, addend, 0)[0])
		self.cache[unit] = found
		return found


# ---------------------------------------------------------------------------------------------------------------
# the plan
# ---------------------------------------------------------------------------------------------------------------

STORAGE = re.compile(rb"^(?:\s*\b(?:static|extern|inline)\b\s*)+")
FLOW = re.compile(rb"\b(?:return|break|continue|goto|case|default|atexit|_onexit|__cxa_atexit)\b")
PLACEMENT_NEW = re.compile(rb"^(?:::\s*)?new\s*\(\s*&\s*(\w+)\s*\)\s*([\w:<>, *]+?)\s*(\(.*\)|\{.*\})?$", re.S)
DESTRUCTOR = re.compile(rb"^(\w+)\s*\.\s*~\s*[\w:<>, *]+\(\s*\)$")
SITE_COLUMNS = ("name", "kind", "home", "home_name", "action", "spelled", "function", "file", "line", "guard",
				"bit", "detail")


def collapse(text: bytes) -> bytes:
	return b" ".join(text.split())


def same_function(name: str, home: str) -> bool:
	a, b = strip_templates(name), strip_templates(home)
	if a == b or re.sub(r"_\d+$", "", a) == b:
		return True
	return a.endswith("::" + b)


class Var:
	"""A file-level global: its declarations and the uses the parse saw."""

	def __init__(self, name: str, decls: list[dict], refs: list[dict]):
		self.name = name
		self.decls = decls
		self.defs = [d for d in decls if d["definition"]]
		self.externs = [d for d in decls if not d["definition"]]
		self.refs = refs
		self.usr = decls[0]["usr"]

	@property
	def definition(self) -> dict | None:
		return self.defs[0] if len(self.defs) == 1 else None


class Planner:
	def __init__(self, tree: Tree, model: Model, found: dict, rows: list[dict], units: dict[str, dict]):
		self.tree, self.model, self.found, self.rows, self.units = tree, model, found, rows, units
		self.objects = Objects(units)
		self.sites: list[dict] = []
		self.edits: dict[str, list] = collections.defaultdict(list)
		self.addresses: dict[str, tuple] = {}  # old name -> (kind, new name, type, file)
		self.comments: list[dict] = []
		self.renames: list[tuple[str, str]] = []  # (old symbol, new symbol) of the class statics
		self.begin()
		by_name = collections.defaultdict(list)
		for decl in model.decls.values():
			by_name[decl["name"]].append(decl)
		self.decls_by_name = by_name
		refs = collections.defaultdict(list)
		for ref in model.refs.values():
			refs[ref["usr"]].append(ref)
		self.refs_by_usr = refs
		self.refs_by_name = collections.defaultdict(list)
		for ref in model.refs.values():
			self.refs_by_name[ref["name"]].append(ref)

	# --- facts ---------------------------------------------------------------------------------------------

	def var(self, name: str) -> Var | str:
		decls = [d for d in self.decls_by_name.get(name, []) if d["scope"] == "file"]
		if not decls:
			return "missing"
		usrs = {d["usr"] for d in decls}
		if len(usrs) > 1:
			return "several globals of this name: " + ", ".join(sorted({d["file"] for d in decls}))
		var = Var(name, decls, sorted(self.refs_by_usr.get(decls[0]["usr"], []),
									  key=lambda r: (r["file"] or "", r["offset"])))
		if not var.definition:
			return f"{len(var.defs)} definitions"
		seen = {(d["file"], d["offset"]) for d in self.decls_by_name.get(name, [])}
		seen |= {(r["file"], r["offset"]) for r in self.refs_by_name.get(name, [])}
		seen |= self.model.others
		homes = {d["file"] for d in decls}
		visible = {path for path, _, _ in self.found.get(name, [])
				   if path in homes or includes_of(self.tree, path) & homes}
		unseen = [(path, offset) for path, offset, code in self.found.get(name, []) if code
				  and path in visible and (path, offset) not in seen]
		if unseen:
			path, offset = unseen[0]
			return (f"spelled where the parse did not see it ({len(unseen)}: {path}:"
					f"{line_of(self.tree.text(path), offset)}; another preprocessor branch or a macro)")
		for decl in decls:
			if decl["file"].startswith("src/SDK/"):
				return f"declared in src/SDK ({decl['file']})"
		macro = [r for r in var.refs if r["expansion"][0] != r["file"] or r["expansion"][1] != r["offset"]]
		for r in macro:
			text = self.tree.text(r["file"])
			if text[r["offset"]:r["offset"] + len(name)] != name.encode():
				return f"used through a macro ({r['file']}:{line_of(text, r['offset'])})"
		return var

	def function(self, ref: dict) -> dict | None:
		return self.model.function_of(ref["function"], ref["file"]) if ref["function"] else None

	def statement_end(self, path: str, end: int) -> int | None:
		"""The offset past the `;` closing a declaration whose extent ends at end."""
		blank = self.tree.blank(path)
		m = re.compile(rb"\s*;").match(blank, end)
		return m.end() if m else None

	def whole_lines(self, path: str, start: int, end: int) -> tuple[int, int] | None:
		"""[start, end) widened to whole lines when nothing else but blanks (and a trailing comment) is on them."""
		text, blank = self.tree.text(path), self.tree.blank(path)
		first = line_start(text, start)
		last = line_end(text, end)
		if blank[first:start].strip() or blank[end:last].strip():
			return None
		return first, last

	def trailing_comment(self, path: str, end: int) -> bytes:
		text = self.tree.text(path)
		return text[end:line_end(text, end)].strip()

	def attached_comment(self, path: str, start: int) -> tuple[int, int] | None:
		"""The `//` lines right above the line at start (no blank line between)."""
		text = self.tree.text(path)
		first = line_start(text, start)
		top = first
		while top > 0:
			above = line_start(text, top - 1)
			if not text[above:top].strip().startswith(b"//"):
				break
			top = above
		return (top, first) if top < first else None

	def declaration_parts(self, decl: dict) -> tuple[bytes, bytes, bytes] | str:
		"""(type prefix without storage keywords, declarator suffix, initialiser with its `=` or braces)."""
		text = self.tree.text(decl["file"])
		whole = text[decl["start"]:decl["end"]]
		at = decl["offset"] - decl["start"]
		if whole[at:at + len(decl["name"])] != decl["name"].encode():
			return "the declaration's text does not spell its name"
		prefix = STORAGE.sub(b"", whole[:at])
		rest = whole[at + len(decl["name"]):]
		# the declarator's array bounds, then the initialiser (`= ...`, `(...)` or `{...}`), if any
		blank = blank_code(rest)
		i = 0
		while True:
			while i < len(rest) and rest[i:i + 1].isspace():
				i += 1
			if blank[i:i + 1] != b"[":
				break
			close = blank.find(b"]", i)
			if close < 0:
				return "an unbalanced array bound"
			i = close + 1
		declarator, init = rest[:i].rstrip(), rest[i:].strip()
		if init.startswith(b"="):
			init = b" = " + init[1:].lstrip()
		elif init and not init.startswith((b"(", b"{")):
			return "an initialiser of another form"
		return prefix, declarator, init

	def startup_initialised(self, var: Var) -> bool | None:
		"""Whether the definition is initialised or destroyed by code at startup and exit: not for a trivial type
		with a constant initialiser; otherwise when the object's static initialisation code refers to it."""
		decl = var.definition
		if decl["pod"] and decl["constant"]:
			return False
		targets = self.objects.startup_targets(decl["unit"])
		if targets is None:
			return None
		return (decl["mangled"] or var.name) in targets

	def body_identifiers(self, function: dict, skip: set[int]) -> set[bytes]:
		blank = self.tree.blank(function["file"])
		start, end = function["body"]
		return {m.group(0) for m in IDENT.finditer(blank, start, end) if m.start() not in skip}

	# --- edits ---------------------------------------------------------------------------------------------

	def edit(self, path: str, start: int, end: int, new: bytes, what: str) -> None:
		"""An edit of the plan being made: kept by commit(), dropped by begin()."""
		self.pending.append((path, (start, end, new, what)))

	def own_comment(self, path: str, start: int, end: int) -> tuple[int, int] | None:
		"""The `//` block right above a declaration when it is about that declaration alone: it stands inside a
		run of declarations (the line above it is code), or the line after the declaration is blank, a comment,
		a closing brace, a directive or the end."""
		comment = self.attached_comment(path, start)
		if comment is None:
			return None
		text = self.tree.text(path)
		above = text[line_start(text, max(comment[0] - 1, 0)):comment[0]].strip() if comment[0] else b""
		if above and not above.startswith((b"//", b"#")) and not above.endswith((b"{", b":")):
			return comment
		after = line_end(text, end)
		following = text[after:line_end(text, after)].strip()
		if following and not following.startswith((b"//", b"}", b"#")):
			return None
		return comment

	def delete_declaration(self, decl: dict, what: str, moved: list | None = None) -> str | None:
		"""Deletes a declaration's lines; its own comment goes to moved (to stand above its new place), or is
		deleted with it when moved is None."""
		path = decl["file"]
		text = self.tree.text(path)
		end = self.statement_end(path, decl["end"])
		if end is None:
			return f"no `;` after the declaration ({path}:{line_of(text, decl['start'])})"
		span = self.whole_lines(path, decl["start"], end)
		if span is None:
			return f"the declaration shares its line ({path}:{line_of(text, decl['start'])})"
		trailing = self.trailing_comment(path, end)
		if trailing:
			self.note(path, decl["start"], "dropped with " + what, trailing)
		comment = self.own_comment(path, decl["start"], end)
		if comment:
			lines = [line.strip() for line in text[comment[0]:comment[1]].splitlines()]
			if moved is None:
				self.note(path, comment[0], "deleted above " + what, b"\n".join(lines))
			elif lines not in moved:
				moved.append(lines)
			span = (comment[0], span[1])
		self.edit(path, span[0], span[1], b"", what)
		return None

	def note(self, path: str, offset: int, what: str, text: bytes) -> None:
		self.pending_notes.append({"file": path, "line": line_of(self.tree.text(path), offset), "what": what,
								   "text": text.decode(errors="replace")})

	def begin(self) -> None:
		self.pending, self.pending_notes = [], []

	def commit(self) -> None:
		for path, edit in self.pending:
			self.edits[path].append(edit)
		self.comments.extend(self.pending_notes)
		self.begin()

	# --- rows ----------------------------------------------------------------------------------------------

	def site(self, row: dict, action: str, detail: str = "", **fields) -> None:
		record = {"name": row["name"], "kind": row["kind"], "home": row["owner"], "home_name": row["home_name"],
				  "action": action, "detail": detail}
		record.update(fields)
		self.sites.append(record)

	def plan(self) -> None:
		rows = []
		for row in self.rows:
			if row["flag"].startswith("comment only"):
				# a comment's name for a variable src declares under another row or name: nothing to move
				self.site(row, "left", "a comment-only row (" + row["flag"] + ")")
			else:
				rows.append(row)
		self.plan_functions([row for row in rows if row["kind"] == "function"])
		for row in rows:
			if row["kind"] == "class":
				self.plan_class(row)

	# --- function statics ----------------------------------------------------------------------------------

	def guard_ifs(self, function: dict) -> list[dict]:
		"""The function's guard tests: {if, guard, bit}."""
		text = self.tree.text(function["file"])
		out = []
		for record in function["ifs"]:
			m = GUARD_TEST.match(collapse(text[record["cond"][0]:record["cond"][1]]))
			if not m:
				continue
			guard = (m.group(1) or m.group(3)).decode()
			bit = int(m.group(2) or m.group(4))
			if bit & (bit - 1):
				continue
			out.append({"if": record, "guard": guard, "bit": bit})
		return out

	def block(self, function: dict, test: dict) -> dict:
		"""What a guard block does: {shape, static (the global it initialises), ...} or {reason}."""
		record, guard, bit = test["if"], test["guard"], test["bit"]
		path = function["file"]
		text = self.tree.text(path)
		if record["else"]:
			return {"reason": "the guard test has an else branch"}
		statements = record["statements"]
		if record["then_kind"] != "COMPOUND_STMT":
			statements = [{"kind": record["then_kind"], "start": record["then"][0], "end": record["then"][1]}]
		sets, rest = [], []
		for statement in statements:
			m = GUARD_SET.match(collapse(text[statement["start"]:statement["end"]]))
			if m and m.group(1).decode() == guard:
				sets.append((statement, int(m.group(2))))
			else:
				rest.append(statement)
		if len(sets) != 1 or sets[0][1] != bit:
			return {"reason": "the block does not set its guard bit once"}
		shape = {"set": sets[0][0], "rest": rest}
		if FLOW.search(self.tree.blank(path)[record["then"][0]:record["then"][1]]):
			return dict(shape, reason="the block holds a jump or registers a destructor (atexit)")
		if not rest:
			return dict(shape, shape="empty", static=None)
		texts = [collapse(text[s["start"]:s["end"]]) for s in rest]
		m = PLACEMENT_NEW.match(texts[-1])
		if m and (len(rest) == 1 or (len(rest) == 2 and DESTRUCTOR.match(texts[0])
									 and DESTRUCTOR.match(texts[0]).group(1) == m.group(1))):
			arguments = m.group(3) or b""
			if re.fullmatch(rb"\(\s*\)|\{\s*\}", arguments):
				arguments = b""
			return dict(shape, shape="placement", static=m.group(1).decode(), arguments=arguments,
						type=m.group(2))
		last = rest[-1]
		if last["kind"] == "BINARY_OPERATOR" and last.get("op") == "Assign" and last.get("lhs_usr"):
			lhs = collapse(text[last["lhs"][0]:last["lhs"][1]])
			if not re.fullmatch(rb"\w+", lhs):
				return dict(shape, reason="the block assigns a qualified name (a class's static member)")
			return dict(shape, shape="assign" if len(rest) == 1 else "lambda", static=lhs.decode(),
						static_usr=last["lhs_usr"], init=last["rhs"], last=last)
		return dict(shape, reason="the block writes its static piecewise (member or element assignments, "
					"a constructor call)")

	def plan_functions(self, rows: list[dict]) -> None:
		by_name = {row["name"]: row for row in self.rows}
		candidates: dict[tuple, list] = collections.defaultdict(list)  # function key -> [(row, var)]
		functions: dict[tuple, dict] = {}
		guard_vars: dict[str, tuple] = {}
		for row in rows:
			var = self.var(row["name"])
			if isinstance(var, str):
				if var == "missing":
					self.site(row, *self.done_function(row))
				else:
					self.site(row, "left", var)
				continue
			users = {r["function"] for r in var.refs}
			if not var.refs:
				if row["name"].endswith(("_guard", "_guards")):
					# a guard no test reads any more: its statics are function statics already
					self.begin()
					error = self.delete_var(var, "guard " + var.name)
					if error:
						self.begin()
						self.site(row, "left", error, file=var.definition["file"])
					else:
						self.commit()
						self.addresses[var.name] = ("function static guard", f"{row['owner']}::", "",
													var.definition["file"])
						self.site(row, "guard deleted", "no bit in use", file=var.definition["file"],
								  line=line_of(self.tree.text(var.definition["file"]), var.definition["start"]))
					continue
				detail = "no use in src"
				if "ListBox<" in row["owner"]:
					detail += ("; the template ListBox<T>'s own function static serves every instance, so this "
							   "per-instance copy has nothing to move into (delete it when the table's unused "
							   "globals go)")
				self.site(row, "left", detail, file=var.definition["file"])
				continue
			if "" in users:
				ref = next(r for r in var.refs if not r["function"])
				self.site(row, "left", f"used outside a function ({ref['file']}:"
						  f"{line_of(self.tree.text(ref['file']), ref['offset'])})")
				continue
			homes = {}
			for ref in var.refs:
				function = self.function(ref)
				if function is not None:
					homes[(function["file"], function["start"])] = function
			if len(homes) != 1:
				names = sorted({f["name"] for f in homes.values()})
				inlined = [n for n in names if not same_function(n, row["owner"])]
				self.site(row, "left", f"used by {len(names)} functions: " + ", ".join(names)
						  + (f" (an inlined copy of {row['owner']} in " + ", ".join(inlined) + ")"
							 if inlined and len(inlined) < len(names) else ""))
				continue
			key, function = next(iter(homes.items()))
			if row["name"].endswith(("_guard", "_guards")) or row["name"] in guard_vars:
				guard_vars[row["name"]] = (key, var)
			if function["template"]:
				self.site(row, "left", f"its home {function['name']} is a template")
				continue
			if not same_function(function["name"], row["owner"]):
				self.site(row, "left", f"used only by {function['name']}, not by its home")
				continue
			functions[key] = function
			candidates[key].append((row, var))
		for key, members in candidates.items():
			self.plan_function(functions[key], members, by_name)

	def done_function(self, row: dict) -> tuple[str, str]:
		for decl in self.decls_by_name.get(row["name"], []):
			if decl["scope"] == "function":
				return "done", "already a static of " + decl["function"]
		static = re.compile(rb"^[ \t]+static\b[^;\n]*\b" + re.escape(row["home_name"].encode()) + rb"\b", re.M)
		owner = strip_templates(row["owner"]).split("::")[-1].encode()
		for path in tree_files(self.tree.root):
			text = self.tree.text(path)
			if owner in text and static.search(text):
				return "done", f"a static {row['home_name']} is declared in {path}"
		return "absent", "not in src (an earlier run moved or deleted it, or nothing declares it)"

	def plan_function(self, function: dict, members: list[tuple], by_name: dict) -> None:
		path = function["file"]
		text = self.tree.text(path)
		tests = self.guard_ifs(function)
		vars_ = {var.name: (row, var) for row, var in members}
		guards = collections.defaultdict(list)
		for test in tests:
			guards[test["guard"]].append(test)
		taken: set[bytes] = set()
		statics = []  # (row, var, test or None, block or None)
		handled = set()
		guard_state = {}  # guard name -> {"var", "tests", "converted", "reasons"}
		for guard, guard_tests in guards.items():
			entry = vars_.get(guard)
			if entry is None:
				outside = self.var(guard)
				mine = (function["file"], function["start"])
				if isinstance(outside, Var) and outside.refs and all(
						(self.function(r) or {}).get("start") == mine[1] and r["file"] == mine[0]
						for r in outside.refs):
					entry = ({"name": guard, "home_name": guard, "kind": "function", "owner": function["name"],
							  "extra": True}, outside)
			guard_var = entry[1] if entry else None
			state = {"var": guard_var, "row": entry[0] if entry else None, "tests": guard_tests, "converted": [],
					 "reasons": []}
			guard_state[guard] = state
			if guard_var is None:
				outside = self.var(guard)
				state["reasons"].append("the guard is used by other functions too"
										+ (f" ({outside})" if isinstance(outside, str) else ""))
				continue
			# every use of the guard is in its tests and sets
			spans = []
			for test in guard_tests:
				spans.append(tuple(test["if"]["cond"]))
			bits = collections.Counter(test["bit"] for test in guard_tests)
			for test in guard_tests:
				block = self.block(function, test)
				test["block"] = block
				if "set" in block:
					spans.append((block["set"]["start"], block["set"]["end"]))
				if bits[test["bit"]] > 1:
					test["block"] = {"set": block.get("set"),
									 "reason": f"bit {test['bit']} is tested {bits[test['bit']]} times"}
			stray = [r for r in guard_var.refs if not any(a <= r["offset"] < b for a, b in spans)]
			if stray:
				state["reasons"].append(f"the guard is used outside its tests (line "
										f"{line_of(text, stray[0]['offset'])})")
		# each static: its guard block, else a constant
		for row, var in members:
			if var.name in guard_state:
				continue
			tests_for = [t for state in guard_state.values() for t in state["tests"]
						 if t.get("block", {}).get("static") == var.name and "reason" not in t["block"]]
			inside = [t for state in guard_state.values() for t in state["tests"]
					  if any(t["if"]["then"][0] <= r["offset"] < t["if"]["then"][1] for r in var.refs)]
			if len(tests_for) > 1:
				self.site(row, "left", "initialised by several guard blocks", function=function["name"])
				continue
			if tests_for:
				test = tests_for[0]
				state = guard_state[test["guard"]]
				if state["reasons"]:
					self.site(row, "left", f"its guard {test['guard']} stays: " + "; ".join(state["reasons"]),
							  function=function["name"], guard=test["guard"], bit=test["bit"])
					continue
				statics.append((row, var, test))
				handled.add(var.name)
				continue
			if inside:
				test = inside[0]
				reason = test.get("block", {}).get("reason") or "; ".join(guard_state[test["guard"]]["reasons"])
				self.site(row, "left", f"set in the block of guard {test['guard']} bit {test['bit']}, which "
						  f"stays: {reason}", function=function["name"], guard=test["guard"], bit=test["bit"])
				continue
			statics.append((row, var, None))
			handled.add(var.name)
		# a guard block whose static the table does not name moves that static under its present name
		for guard, state in guard_state.items():
			if state["reasons"]:
				continue
			for test in state["tests"]:
				name = test["block"].get("static")
				if name and "reason" not in test["block"] and name not in handled and name not in vars_:
					var = self.var(name)
					pseudo = {"name": name, "home_name": name, "kind": "function", "owner": function["name"],
							  "extra": True}
					if isinstance(var, str):
						test["block"] = {"reason": f"its static {name}: {var}"}
						continue
					users = {(f["file"], f["start"]) for f in (self.function(r) for r in var.refs) if f}
					if users != {(function["file"], function["start"])} or any(not r["function"] for r in var.refs):
						test["block"] = {"reason": f"its static {name} is used outside {function['name']}"}
						continue
					statics.append((pseudo, var, test))
					handled.add(name)
		# names: the home name unless the function spells it otherwise
		statics.sort(key=lambda entry: (entry[2] is not None, entry[1].definition["file"],
										entry[1].definition["start"]))
		for row, var, test in statics:
			own = {r["offset"] for r in var.refs}
			spelled = self.body_identifiers(function, own)
			name = row["home_name"].encode()
			if name in spelled or name in taken:
				row = dict(row, spelled=var.name, conflict=True)
			else:
				row = dict(row, spelled=row["home_name"])
			taken.add(row["spelled"].encode())
			self.plan_static(function, row, var, test)
		# a block with nothing but the guard's set: its static is already a function static
		for guard, state in guard_state.items():
			if state["reasons"]:
				continue
			for test in state["tests"]:
				if test["block"].get("shape") != "empty" or "reason" in test["block"]:
					continue
				record = test["if"]
				span = self.whole_lines(path, record["start"], record["end"])
				if span is None:
					test["failed"] = "the guard test shares its line"
					continue
				self.edit(path, span[0], span[1], b"", f"empty guard block of {guard} bit {test['bit']}")
				self.commit()
				test["converted"] = True
		# the guards: deleted with their last bit
		for guard, state in guard_state.items():
			row = state["row"]
			if row is None:
				continue
			left = [t for t in state["tests"] if not t.get("converted")]
			if state["reasons"]:
				self.site(row, "left", "; ".join(state["reasons"]), function=function["name"])
				continue
			if left:
				reasons = sorted({f"bit {t['bit']}: " + (t.get("failed") or t["block"].get("reason")
														 or "its static stays") for t in left})
				self.site(row, "left", "bits stay: " + "; ".join(reasons), function=function["name"])
				continue
			error = self.delete_var(state["var"], "guard " + guard)
			if error:
				self.begin()
				self.site(row, "left", error, function=function["name"])
				continue
			self.commit()
			bits = ",".join(str(t["bit"]) for t in state["tests"])
			statics_ = ",".join(t["block"].get("static") or "" for t in state["tests"])
			self.addresses[guard] = ("function static guard", f"{function['name']}::{statics_}", bits, path)
			self.site(row, "guard deleted", f"bits {bits}" + (" (not in the table)" if row.get("extra") else ""),
					  function=function["name"], file=path, line=line_of(text, state["tests"][0]["if"]["start"]))
		for row, var in members:
			if var.name not in handled and var.name not in guard_state and not any(
					s["name"] == row["name"] for s in self.sites):
				self.site(row, "left", "no decision", function=function["name"])

	def delete_var(self, var: Var, what: str) -> str | None:
		for decl in var.decls:
			error = self.delete_declaration(decl, what)
			if error:
				return error
		return None

	def plan_static(self, function: dict, row: dict, var: Var, test: dict | None) -> None:
		path = function["file"]
		text = self.tree.text(path)
		definition = var.definition
		fields = {"function": function["name"], "spelled": row["spelled"], "file": path}
		if test:
			fields.update(guard=test["guard"], bit=test["bit"])

		def leave(reason: str) -> None:
			self.begin()
			if test:
				test["failed"] = f"{var.name}: {reason}"
			self.site(row, "left", reason, **fields)

		if definition["file"].endswith((".h", ".hpp", ".inl")):
			return leave(f"defined in a header ({definition['file']})")
		startup = self.startup_initialised(var)
		if startup is None:
			return leave(f"no object for {definition['unit']}")
		if startup:
			return leave("its file-level definition is constructed or destroyed at startup and exit; as a function "
						 "static it would be constructed at the first call instead")
		parts = self.declaration_parts(definition)
		if isinstance(parts, str):
			return leave(parts)
		prefix, declarator, file_init = parts
		if test and test["block"].get("shape") in ("assign", "lambda"):
			last = test["block"]["last"]
			inner = [r for r in var.refs if test["if"]["start"] <= r["offset"] < test["if"]["end"]
					 and not (last["lhs"][0] <= r["offset"] < last["lhs"][1])]
			if inner:
				return leave("the guard block reads the static before assigning it")
		spelled = row["spelled"].encode()
		head = b"static " + prefix + spelled + declarator
		block = test["block"] if test else None
		inner_moves = []
		if block is None:
			# a constant: at the top of the function
			body_start = function["body"][0]
			if self.tree.blank(path)[body_start + 1:line_end(text, body_start)].strip():
				return leave("the function's opening brace shares its line")
			at = line_end(text, body_start)
			first = re.compile(rb"[ \t]*(?=\S)").match(text, at)
			indent = first.group(0) if first else indent_at(text, function["start"]) + b"\t"
			new = indent + head + file_init + b";\n"
			replace = (at, at)
			shape = "constant"
		else:
			record = test["if"]
			if record["parent_kind"] != "COMPOUND_STMT":
				return leave("the guard test is not a statement of a block")
			own = [r for r in var.refs if not (record["start"] <= r["offset"] < record["end"])]
			parent = record["parent"]
			outside = [r for r in own if not (record["end"] <= r["offset"] < parent[1])]
			if outside:
				return leave(f"used before or outside the guard block's scope (line "
							 f"{line_of(text, outside[0]['offset'])})")
			span = self.whole_lines(path, record["start"], record["end"])
			if span is None:
				return leave("the guard test shares its line")
			indent = indent_at(text, record["start"])
			shape = block["shape"]
			if shape == "assign":
				init = text[block["init"][0]:block["init"][1]]
				new = indent + head + b" = " + init + b";\n"
			elif shape == "placement":
				new = indent + head + block["arguments"] + b";\n"
			elif shape == "lambda":
				lines = self.lambda_body(function, test, var, inner_moves)
				if isinstance(lines, str):
					return leave(lines)
				new = indent + head + b" = [&] {\n" + lines + indent + b"}();\n"
			else:
				return leave(f"block shape {shape}")
			replace = span
			comment = self.attached_comment(path, record["start"])
			if comment and test["guard"].encode() in text[comment[0]:comment[1]]:
				replace = (comment[0], span[1])
				self.note(path, comment[0], "deleted above the guard block", text[comment[0]:comment[1]].strip())
		moved: list = []
		error = self.delete_declaration(definition, "definition of " + var.name, moved)
		if error:
			return leave(error)
		for decl in var.externs:
			error = self.delete_declaration(decl, "declaration of " + var.name, moved)
			if error:
				return leave(error)
		if moved:
			new = b"".join(indent + line + b"\n" for lines in moved for line in lines) + new
		for move in inner_moves:
			self.edit(path, move[0], move[1], b"", f"temporary moved into the initialiser of {row['spelled']}")
		self.edit(path, replace[0], replace[1], new, f"static {row['spelled']} ({shape})")
		if spelled != var.name.encode():
			for ref in var.refs:
				if test and ref.get("usr") == var.usr and test["if"]["start"] <= ref["offset"] < test["if"]["end"]:
					continue
				self.edit(ref["file"], ref["offset"], ref["offset"] + len(var.name), spelled, "use")
		self.commit()
		if test:
			test["converted"] = True
		self.addresses[var.name] = ("function static", f"{function['name']}::{row['spelled']}",
									definition["type"], path)
		action = "moved" + (" (interim name: the home name is spelled in the function)" if row.get("conflict")
							else "")
		self.site(row, action, shape + (" (not in the table: its guard is)" if row.get("extra") else ""),
				  line=line_of(text, replace[0]), **{k: v for k, v in fields.items() if k != "file"},
				  file=path)

	def lambda_body(self, function: dict, test: dict, var: Var, moves: list) -> bytes | str:
		"""The statements of a guard block as a lambda's body: the guard's set dropped, the assignment of the
		static returned; a temporary declared outside the block and used only in it moves in."""
		path = function["file"]
		text, blank = self.tree.text(path), self.tree.blank(path)
		record, block = test["if"], test["block"]
		then_start, then_end = record["then"]
		inner_start = line_end(text, then_start)
		inner_end = line_start(text, then_end - 1)
		if blank[then_start + 1:inner_start].strip() or blank[inner_end:then_end - 1].strip():
			return "the guard block's braces share a line with a statement"
		drop = self.whole_lines(path, block["set"]["start"], self.statement_end(path, block["set"]["end"]) or 0)
		last = block["last"]
		last_end = self.statement_end(path, last["end"])
		if drop is None or last_end is None:
			return "the guard block's statements share lines"
		indent = indent_at(text, last["start"])
		pieces = [text[inner_start:drop[0]], text[drop[1]:last["start"]],
				  b"return " + text[last["rhs"][0]:last["rhs"][1]] + b";", text[last_end:inner_end]]
		body = b"".join(pieces)
		# temporaries declared outside the block and used only inside it
		locals_ = {local["usr"]: local for local in function["locals"]}
		used = collections.defaultdict(list)
		for ref in function["local_refs"]:
			used[ref["usr"]].append(ref["offset"])
		head = b""
		for usr, offsets in used.items():
			local = locals_.get(usr)
			if local is None or local["param"] or local["static"]:
				continue
			if record["start"] <= local["start"] < record["end"]:
				continue
			if all(then_start <= o < then_end for o in offsets):
				end = self.statement_end(path, local["end"])
				span = end and self.whole_lines(path, local["start"], end)
				declaration = text[local["start"]:local["end"]]
				if span and b"=" not in declaration and b"," not in declaration and local["parent"] == "DECL_STMT":
					moves.append(span)
					head += indent + collapse(declaration) + b";\n"
		return head + body

	# --- class statics -------------------------------------------------------------------------------------

	def plan_class(self, row: dict) -> None:
		owner = row["owner"]
		var = self.var(row["name"])
		if isinstance(var, str):
			if var == "missing":
				record = self.model.classes.get(owner)
				if record and row["home_name"] in {m["name"] for m in record["members"]}:
					return self.site(row, "done", f"already a member of {owner}")
				return self.site(row, "left", "not in src (" + (row["flag"] or "nothing declares it") + ")")
			return self.site(row, "left", var)
		definition = var.definition
		record = self.model.classes.get(owner)
		if record is None:
			return self.site(row, "left", f"the parse found no definition of {owner}")
		if record["template"]:
			return self.site(row, "left", f"{owner} is a template")
		if record["file"].startswith("src/SDK/"):
			return self.site(row, "left", f"{owner} is declared in src/SDK")
		home = row["home_name"]
		if home in record["all_members"]:
			return self.site(row, "left", f"{owner} or a base already declares {home}")
		if definition["file"].endswith((".h", ".hpp", ".inl")):
			return self.site(row, "left", f"defined in a header ({definition['file']})")
		if definition["unit"] not in self.model.class_seen.get(owner, ()):
			return self.site(row, "left", f"{definition['file']} does not see the definition of {owner}")
		blind = sorted({r["file"] for r in var.refs if r["unit"] not in self.model.class_seen.get(owner, ())})
		if blind:
			return self.site(row, "left", f"used where {owner} is not defined: " + ", ".join(blind))
		parts = self.declaration_parts(definition)
		if isinstance(parts, str):
			return self.site(row, "left", parts)
		prefix, declarator, init = parts
		if b"constexpr" in prefix:
			return self.site(row, "left", "a constexpr global (an in-class initialiser)")
		self.begin()
		dtext = self.tree.text(definition["file"])
		qualified = (owner + "::" + home).encode()
		# a type alias declared where the class's body cannot see it, used by this global alone: written out
		declare = None
		alias = definition.get("alias")
		if alias and alias["file"] and not self.alias_visible(alias, record):
			declare = self.inline_alias(alias, var)
			if isinstance(declare, str):
				self.begin()
				return self.site(row, "left", declare)
		if declare:
			declaration = b"static " + declare(home.encode()) + declarator + b";"
			self.edit(definition["file"], definition["start"], definition["end"],
					  declare(qualified) + declarator + init, "definition of " + owner + "::" + home)
		else:
			declaration = b"static " + prefix + home.encode() + declarator + b";"
			head = dtext[definition["start"]:definition["offset"]]
			stripped = re.sub(rb"^\s*static\s+", b"", head)
			self.edit(definition["file"], definition["start"], definition["offset"] + len(var.name),
					  stripped + qualified, "definition of " + owner + "::" + home)
		# the declaration in the class, after its last public static data member, else at its end
		ctext = self.tree.text(record["file"])
		members = [m for m in record["members"] if m["file"] == record["file"]]
		statics = [m for m in members if m["kind"] == "VAR_DECL" and m["access"] == "PUBLIC"]
		if statics:
			anchor = statics[-1]
			end = self.statement_end(record["file"], anchor["end"])
			if end is None:
				self.begin()
				return self.site(row, "left", "no `;` after the class's last static member")
			at = line_end(ctext, end)
			indent = indent_at(ctext, anchor["start"])
			opening = b""
		else:
			close = ctext.rfind(b"}", record["start"], record["end"])
			if close < 0 or ctext[line_start(ctext, close):close].strip():
				self.begin()
				return self.site(row, "left", f"the closing brace of {owner} shares its line")
			at = line_start(ctext, close)
			outer = indent_at(ctext, record["start"])
			access = members[-1]["access"] if members else ("PUBLIC" if record["kind"] == "STRUCT_DECL" else
															"PRIVATE")
			indent = indent_at(ctext, members[-1]["start"]) if members else outer + b"\t"
			opening = b"" if access == "PUBLIC" else outer + b"public:\n"
		moved: list = []
		for decl in var.externs:
			error = self.delete_declaration(decl, "declaration of " + var.name, moved)
			if error:
				self.begin()
				return self.site(row, "left", error)
		comment = b"".join(indent + line + b"\n" for lines in moved for line in lines)
		self.edit(record["file"], at, at, opening + comment + indent + declaration + b"\n", f"member {owner}::{home}")
		for ref in var.refs:
			function = self.function(ref)
			bare = function is not None and (function["class"] == owner or function["class"].startswith(owner + "::"))
			if bare:
				spelled = self.body_identifiers(function, {r["offset"] for r in var.refs})
				bare = home.encode() not in spelled
			rtext = self.tree.text(ref["file"])
			start = ref["offset"]
			if rtext[start - 2:start] == b"::" and not re.match(rb"\w", rtext[start - 3:start - 2]):
				start -= 2
			self.edit(ref["file"], start, ref["offset"] + len(var.name), home.encode() if bare else qualified,
					  "use")
		self.commit()
		self.addresses[var.name] = ("static member", f"{owner}::{home}", definition["type"], definition["file"])
		member = "_ZN" + "".join(f"{len(part)}{part}" for part in owner.split("::") + [home]) + "E"
		self.renames.append(("=" + (definition["mangled"] or var.name), member))
		self.site(row, "member", "alias written out" if declare else "", spelled=f"{owner}::{home}",
				  file=definition["file"], line=line_of(dtext, definition["start"]))

	def alias_visible(self, alias: dict, record: dict) -> bool:
		"""Whether the class's body sees the alias: declared above the class in its file, or in a file it
		includes."""
		if alias["file"] == record["file"]:
			return alias["start"] < record["start"]
		return alias["file"] in includes_of(self.tree, record["file"])

	def inline_alias(self, alias: dict, var: Var):
		"""For an alias of an array or a function pointer that only this global's declarations spell: deletes it
		and returns a function writing the declarator around a name; else a reason."""
		text = self.tree.text(alias["file"])
		m = re.fullmatch(rb"using\s+(\w+)\s*=\s*(.+)", collapse(text[alias["start"]:alias["end"]]), re.S)
		if not m or m.group(1).decode() != alias["name"]:
			return f"its type alias {alias['name']} is declared where the class cannot see it"
		aliased = m.group(2).strip()
		pattern = re.compile(rb"\b" + re.escape(alias["name"].encode()) + rb"\b")
		spelled = sum(len(pattern.findall(self.tree.blank(path))) for path in tree_files(self.tree.root)
					  if alias["name"].encode() in self.tree.text(path))
		if spelled != len(var.decls) + 1:
			return (f"its type alias {alias['name']} is declared after the class and spelled elsewhere too "
					f"({spelled - len(var.decls) - 1} more)")
		array = re.fullmatch(rb"(.+?)\s*(\[.*\])", aliased, re.S)
		if array:
			def declare(name: bytes) -> bytes:
				return array.group(1) + b" " + name + array.group(2)
		elif aliased.count(b"(*)") == 1:
			def declare(name: bytes) -> bytes:
				return aliased.replace(b"(*)", b"(*" + name + b")")
		else:
			return f"its type alias {alias['name']} is declared after the class and is not an array or pointer"
		error = self.delete_declaration(alias, "alias " + alias["name"])
		if error:
			return error
		return declare


# ---------------------------------------------------------------------------------------------------------------
# measure and apply
# ---------------------------------------------------------------------------------------------------------------

def discover(tree: Tree, model: Model, names: set[str]) -> set[str]:
	"""The guards and guarded globals the table does not name, in the functions that use its globals."""
	extra = set()
	for function in model.functions.values():
		if not function["refs"]:
			continue
		text = tree.text(function["file"])
		locals_ = {local["name"] for local in function["locals"]}
		found = set()
		for record in function["ifs"]:
			m = GUARD_TEST.match(collapse(text[record["cond"][0]:record["cond"][1]]))
			if not m:
				continue
			found.add((m.group(1) or m.group(3)).decode())
			for statement in record["statements"]:
				if statement["kind"] == "BINARY_OPERATOR" and statement.get("lhs"):
					lhs = collapse(text[statement["lhs"][0]:statement["lhs"][1]])
					if re.fullmatch(rb"\w+", lhs):
						found.add(lhs.decode())
				placement = PLACEMENT_NEW.match(collapse(text[statement["start"]:statement["end"]]))
				if placement:
					found.add(placement.group(1).decode())
		extra |= found - locals_
	return extra - names


def write_tsv(path: str, columns, rows) -> None:
	with open(path, "w", encoding="utf-8", newline="") as out:
		writer = csv.DictWriter(out, columns, delimiter="\t", lineterminator="\n", extrasaction="ignore")
		writer.writeheader()
		for row in rows:
			writer.writerow({k: ("" if row.get(k) is None else row.get(k)) for k in columns})


def measure(root: str, build: str, table: str, jobs: int) -> Planner:
	tree = Tree(root)
	rows = load_rows(table)
	units = compile_commands(root, build)
	names = {row["name"] for row in rows}
	classes = {row["owner"] for row in rows if row["kind"] == "class"}
	found, model = run_parse(tree, units, names, classes, jobs)
	extra = discover(tree, model, names)
	if extra:
		print(f"statics: {len(extra)} guards and guarded globals the table does not name", file=sys.stderr)
		found, model = run_parse(tree, units, names | extra, classes, jobs)
	planner = Planner(tree, model, found, rows, units)
	planner.plan()
	return planner



def apply_edits(planner: Planner, out: str, dry_run: bool) -> list[dict]:
	tree = planner.tree
	applied = []
	for path, edits in sorted(planner.edits.items()):
		text = tree.text(path)
		unique = {}
		for edit in edits:
			unique.setdefault(edit[:3], edit)
		# stable: insertions at one place keep the order they were planned in
		edits = sorted(unique.values(), key=lambda e: (e[0], e[1]))
		for (a0, a1, _, w0), (b0, b1, _, w1) in zip(edits, edits[1:]):
			if b0 < a1:
				fail(f"{path}: overlapping edits at line {line_of(text, a0)} ({w0}) and line {line_of(text, b0)} "
					 f"({w1})")
		out_text = bytearray()
		at = 0
		for start, end, new, what in edits:
			out_text += text[at:start]
			applied.append({"file": path, "line": line_of(text, start), "what": what,
							"old": text[start:end].decode(errors="replace").strip(),
							"new": new.decode(errors="replace").strip()})
			out_text += new
			at = end
		out_text += text[at:]
		if not dry_run:
			with open(os.path.join(tree.root, path), "wb") as target:
				target.write(bytes(out_text))
	return applied


def update_addresses(root: str, updates: dict[str, tuple], dry_run: bool) -> int:
	path = os.path.join(root, "harness", "addresses.tsv")
	if not os.path.exists(path) or not updates:
		return 0
	with open(path, encoding="utf-8") as source:
		lines = source.read().split("\n")
	changed = 0
	for i, line in enumerate(lines):
		fields = line.split("\t")
		if len(fields) < 8 or fields[2] not in updates:
			continue
		kind, name, type_, file = updates[fields[2]]
		fields[1], fields[2] = kind, name
		if kind == "function static guard":
			fields[3] = type_
		fields[4] = file
		lines[i] = "\t".join(fields)
		changed += 1
	if not dry_run:
		with open(path, "w", encoding="utf-8") as target:
			target.write("\n".join(lines))
	return changed


def run(command: str, root: str, build: str, table: str, out: str, jobs: int, dry_run: bool) -> None:
	os.makedirs(out, exist_ok=True)
	planner = measure(root, build, table, jobs)
	sites = sorted(planner.sites, key=lambda s: (s["kind"], s["action"], s["home"], s["name"]))
	write_tsv(os.path.join(out, "sites.tsv"), SITE_COLUMNS, sites)
	counts = collections.Counter((s["kind"], s["action"].split(" (")[0]) for s in sites)
	for (kind, action), count in sorted(counts.items()):
		print(f"{kind}\t{action}\t{count}")
	if planner.model.errors:
		print(f"statics: {len(planner.model.errors)} parse errors in the tree (first: {planner.model.errors[0]})",
			  file=sys.stderr)
	if command == "measure":
		write_tsv(os.path.join(out, "comments.tsv"), ("file", "line", "what", "text"), planner.comments)
		return
	applied = apply_edits(planner, out, dry_run)
	write_tsv(os.path.join(out, "applied.tsv"), ("file", "line", "what", "old", "new"), applied)
	with open(os.path.join(out, "renames.tsv"), "w", encoding="utf-8") as target:
		target.write("".join(f"{old}\t{new}\n" for old, new in planner.renames))
	write_tsv(os.path.join(out, "comments.tsv"), ("file", "line", "what", "text"), planner.comments)
	changed = update_addresses(root, planner.addresses, dry_run)
	print(f"statics: {len(applied)} edits in {len(planner.edits)} files, {changed} rows of harness/addresses.tsv"
		  + (" (dry run)" if dry_run else ""))



def main() -> None:
	parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
	parser.add_argument("command", choices=("measure", "apply"))
	parser.add_argument("--root", default=ROOT)
	parser.add_argument("--build")
	parser.add_argument("--table", default=os.path.join(ROOT, "env/scratch/Clean/3d/data.tsv"))
	parser.add_argument("--out", default=os.path.join(ROOT, "env/scratch/Clean/5b"))
	parser.add_argument("--jobs", type=int, default=8)
	parser.add_argument("--dry-run", action="store_true")
	arguments = parser.parse_args()
	root = os.path.realpath(arguments.root)
	build = arguments.build or os.path.join(root, "build", "linux")
	run(arguments.command, root, build, arguments.table, arguments.out, arguments.jobs, arguments.dry_run)


if __name__ == "__main__":
	main()
