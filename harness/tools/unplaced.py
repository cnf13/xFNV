#!/usr/bin/env python3
"""Empty src's Unplaced files: each declaration and definition goes to a home file (docs/PLAN.md Part 2, step 4).

Usage: env/venv/bin/python harness/tools/unplaced.py measure [--root DIR] [--build DIR] [--out DIR] [--jobs N]
       env/venv/bin/python harness/tools/unplaced.py decide [--root DIR] [--out DIR]
       env/venv/bin/python harness/tools/unplaced.py apply [--root DIR] [--out DIR] [--table TSV] [--dry-run]
                                                          [--addresses TSV] [--homes TSV]

  measure  parses translation units of the build (libclang, --build's compile_commands.json, default
           ROOT/build/linux: build it first) in two passes: the Unplaced .cpp files and the files including an
           Unplaced file give the entities; then one unit for each other file that spells an entity's name gives
           their other uses. Writes OUT/model.json.gz (default OUT env/scratch/Clean/unplaced), then decides.
  decide   from the model: OUT/entities.tsv, one row per top-level entity of the Unplaced files with its home
           (below; a `chosen` filled in a previous entities.tsv is kept), OUT/becomes-member.tsv (the rule d rows),
           OUT/open.tsv (the rule e rows, with the files using them), OUT/report.txt (counts per kind and rule, open
           rows per folder).
  apply    moves the rows of the table (OUT/entities.tsv, or --table) to their homes. The model must still describe
           the tree (the Unplaced files unchanged), else measure again; a second run after a measure changes
           nothing. Writes OUT/moved.tsv, left.tsv (rows that could not move, with why), internal.tsv (what became
           file-local), includes.tsv (each include replaced or added), dropped-comments.tsv (comment lines of a
           deleted file that belonged to no entity), deleted.list. --dry-run writes the tables only. The address
           table (--addresses, default ROOT/harness/addresses.tsv when it exists) gets each moved entity's new file
           and line; its rows still naming a deleted file go to addresses-stale.tsv. The homes table (--homes,
           default harness/tools/unplaced-homes.tsv beside this script) fills `chosen` first: its rows that match no
           entity of the table go to OUT/homes-unused.tsv.

The home of an entity, in this order (the table's `rule`):
  a        the function's source file in the debug symbols (harness/tools/names.py `sources`), the file a global
           or a function is already defined in (rule `defined`): the definition goes to that .cpp, the declaration
           to the header that .cpp's class or siblings use (its own stem's header; for `XClass.cpp`, the header of
           X; else the first header of its folder it includes)
  b        a member of a class (a method or a static data member defined out of line): the class's .cpp (the one
           defining most of its other members, else the one named after its header)
  c        a free function, global or type whose every use is in one file: that file; a function or global there
           becomes file-local (`static`) when nothing outside that file names it
  d        a free function taking a class (pointer or reference) as its first parameter: the class's files; it is
           listed in becomes-member.tsv (making it a member changes its signature: not done here)
  users    a type or alias that only the moved declarations name, all going to one header: that header
  e        open: no rule decides; `chosen` (a .cpp or header path, or `home_cpp;home_h`) is filled by a person and
           apply takes it. A src/Havok entity never gets a home outside src/Havok (its include isolation), unless
           a person chose one.
  chosen   a row of the homes table (columns name, kind, home in `chosen`'s form, evidence, and an optional usr),
           matched by the entity's name and the row's kind, and by its clang USR where the row gives one (overloads
           of one name): its home replaces any rule's, and the entity is not made file-local. A row without a usr
           that matches several entities stops apply.
           The decisions no rule can make (the open rows, the rows the rules could not place) are kept there.
  support  includes and forward declarations: not moved; a file that loses its include of the Unplaced header gets
           in its place the headers of the homes it uses, the Unplaced header's includes that declare something it
           uses, and the forward declarations it names.

What apply does. Each entity's text (with the comment lines right above it) leaves its Unplaced file:
  - a definition goes to the end of home_cpp (inside its namespace, written around it), a declaration after the last
    top-level declaration of home_h (before its first function definition or the end); a file-local definition
    gets `static` and, when its home uses it above that end, a `static` declaration after the home's includes;
    texts going to one place there are ordered types and aliases, declarations, definitions;
  - the home gets the includes the moved text needs where it goes (an include lower in the home moves up): those
    of its Unplaced file whose headers declare what the text names, the homes' headers of the other Unplaced
    entities it names, the Unplaced headers of those that stay, and the forward declarations it names;
  - an Unplaced file left with includes, forward declarations and the braces of emptied namespaces only is deleted
    (CMake globs the sources: configure again), and each `#include` of a deleted Unplaced header is replaced as
    `support` says.
Nothing is guessed: a row whose move cannot be written (a home that would include itself in a cycle, a text the
model does not match) goes to left.tsv with why, and its Unplaced file stays (the moves are planned again with it
known, until no new such row turns up).
"""

from __future__ import annotations

import argparse
import collections
import concurrent.futures
import csv
import gzip
import hashlib
import json
import os
import re
import shlex
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
CLANG_PACKAGE = "/usr/lib/python3.14/site-packages"
LIBCLANG = "/usr/lib/libclang.so"
INCLUDE_ROOTS = ("src", "harness/seam", "gen")
INCLUDE = re.compile(r'^\s*#\s*include\s*"([^"]+)"', re.M)
UNPLACED = re.compile(r"(^|/)Unplaced\.(h|cpp)$")
EXTENSIONS = (".h", ".hpp", ".inl", ".cpp", ".inc")


def fail(message: str) -> None:
	sys.exit(f"unplaced: {message}")


def digest(path: str) -> str:
	with open(path, "rb") as source:
		return hashlib.sha1(source.read()).hexdigest()


def tree_files(root: str):
	for top in ("src", "gen"):
		for base, _, names in os.walk(os.path.join(root, top)):
			for name in names:
				if name.endswith(EXTENSIONS):
					yield os.path.relpath(os.path.join(base, name), root)


def resolve_include(root: str, including: str, spelled: str) -> str | None:
	"""The tree file a quoted include names (relative to root), as the compiler finds it: beside the including
	file first, then the include roots."""
	for base in (os.path.dirname(including),) + INCLUDE_ROOTS:
		path = os.path.normpath(os.path.join(base, spelled))
		if os.path.isfile(os.path.join(root, path)):
			return path
	return None


def include_graph(root: str) -> dict[str, list[str]]:
	graph = {}
	for path in tree_files(root):
		with open(os.path.join(root, path), encoding="utf-8", errors="replace") as source:
			text = source.read()
		graph[path] = [found for spelled in INCLUDE.findall(text)
					   if (found := resolve_include(root, path, spelled))]
	return graph


# ---------------------------------------------------------------------------------------------------------------
# The parse (one worker per translation unit)
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


def compile_arguments(entry: dict) -> list[str]:
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
	resource = subprocess_output(["clang", "-print-resource-dir"])
	return out + ["-resource-dir", resource, "-Wno-everything"]


_resource: list[str] = []


def subprocess_output(command: list[str]) -> str:
	if not _resource:
		import subprocess
		_resource.append(subprocess.run(command, capture_output=True, text=True, check=True).stdout.strip())
	return _resource[0]


def _init_worker(root: str, unplaced: list[str]) -> None:
	_clang()
	_state["root"] = root
	_state["unplaced"] = set(unplaced)
	_state["seen"] = set()
	_state["index"] = _ci.Index.create()


def _rel(cursor_or_location) -> str | None:
	location = cursor_or_location
	if hasattr(location, "location"):
		location = location.location
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


def _referenced(cursor) -> list:
	ci = _ci
	if cursor.kind == ci.CursorKind.OVERLOADED_DECL_REF:
		count = ci.conf.lib.clang_getNumOverloadedDecls(cursor)
		return [ci.conf.lib.clang_getOverloadedDecl(cursor, i) for i in range(count)]
	try:
		found = cursor.referenced
	except Exception:
		return []
	return [found] if found is not None else []


REF_KINDS = None
CLASS_KINDS = None
FUNCTION_KINDS = None


def _kinds():
	global REF_KINDS, CLASS_KINDS, FUNCTION_KINDS
	if REF_KINDS is None:
		k = _ci.CursorKind
		REF_KINDS = {k.DECL_REF_EXPR, k.MEMBER_REF_EXPR, k.TYPE_REF, k.TEMPLATE_REF, k.OVERLOADED_DECL_REF,
					 k.MACRO_INSTANTIATION, k.NAMESPACE_REF, k.MEMBER_REF, k.VARIABLE_REF, k.CALL_EXPR}
		CLASS_KINDS = {k.CLASS_DECL, k.STRUCT_DECL, k.UNION_DECL, k.CLASS_TEMPLATE,
					   k.CLASS_TEMPLATE_PARTIAL_SPECIALIZATION, k.ENUM_DECL}
		FUNCTION_KINDS = {k.FUNCTION_DECL, k.CXX_METHOD, k.CONSTRUCTOR, k.DESTRUCTOR, k.CONVERSION_FUNCTION,
						  k.FUNCTION_TEMPLATE}


def _top_end(cursor) -> int:
	"""The last line of the top-level declaration holding the cursor (the namespace or class around it)."""
	ci = _ci
	while True:
		parent = cursor.lexical_parent
		if parent is None or parent.kind == ci.CursorKind.TRANSLATION_UNIT:
			return cursor.extent.end.line
		cursor = parent


def _definition(cursor) -> tuple[str, int]:
	try:
		found = cursor.get_definition()
	except Exception:
		found = None
	if found is None or not _rel(found):
		return "", 0
	return _rel(found), _top_end(found)


def _first_parameter_class(cursor) -> dict | None:
	ci = _ci
	arguments = list(cursor.get_arguments() or [])
	if not arguments or arguments[0] is None:
		return None
	t = arguments[0].type
	if t.kind not in (ci.TypeKind.POINTER, ci.TypeKind.LVALUEREFERENCE, ci.TypeKind.RVALUEREFERENCE):
		return None
	declaration = t.get_pointee().get_canonical().get_declaration()
	if declaration is None or declaration.kind not in CLASS_KINDS or declaration.kind == ci.CursorKind.ENUM_DECL:
		return None
	template = declaration.specialized_template
	owner = template if template is not None and template.kind == ci.CursorKind.CLASS_TEMPLATE else declaration
	definition = owner.get_definition()
	return {"usr": owner.get_usr(), "name": _qualified(owner), "file": (_rel(definition) or "") if definition else "",
			"pointer": t.kind == ci.TypeKind.POINTER, "const": t.get_pointee().is_const_qualified()}


def _walk_uses(top, file: str, entity_key, uses: list, visible: set, needs: set | None) -> None:
	"""Uses of the Unplaced files' entities under top; with needs, also every file declaring what top names."""
	for node in top.walk_preorder():
		if node.kind not in REF_KINDS:
			continue
		for target in _referenced(node):
			usr = target.get_usr()
			if usr in visible:
				uses.append((usr, _rel(node) or file, node.location.line, entity_key))
			if needs is not None:
				found = _rel(target)
				if found:
					needs.add(found)


def _entity(cursor, file: str, namespaces: list[str]) -> dict:
	ci = _ci
	kind = cursor.kind
	extent = cursor.extent
	record = {
		"file": file, "line": cursor.location.line, "start": extent.start.line, "start_col": extent.start.column,
		"end": extent.end.line, "end_col": extent.end.column, "kind": kind.name, "name": cursor.spelling,
		"qname": _qualified(cursor), "usr": cursor.get_usr(), "definition": cursor.is_definition(),
		"namespaces": namespaces, "linkage": "", "storage": "", "parent": None, "first": None, "refs": [],
		"display": cursor.displayname,
	}
	try:
		record["linkage"] = cursor.linkage.name
	except Exception:
		pass
	try:
		record["storage"] = cursor.storage_class.name
	except Exception:
		pass
	if kind == ci.CursorKind.INCLUSION_DIRECTIVE:
		included = cursor.get_included_file()
		record["included"] = (os.path.relpath(os.path.realpath(included.name), _state["root"])
							  if included is not None else "")
		return record
	parent = cursor.semantic_parent
	if parent is not None and parent.kind in CLASS_KINDS:
		definition = parent.get_definition() or parent
		record["parent"] = {"usr": parent.get_usr(), "name": _qualified(parent), "file": _rel(definition) or ""}
	if kind in (ci.CursorKind.FUNCTION_DECL, ci.CursorKind.FUNCTION_TEMPLATE) and record["parent"] is None:
		record["first"] = _first_parameter_class(cursor)
	refs = {}
	for node in cursor.walk_preorder():
		if node.kind not in REF_KINDS:
			continue
		for target in _referenced(node):
			usr = target.get_usr()
			if not usr or usr in refs or usr == record["usr"]:
				continue
			definition, definition_end = _definition(target)
			# (usr, kind, name, declaring file, the last line of the top-level declaration holding it, defining
			# file, the same of the definition)
			refs[usr] = (usr, target.kind.name, _qualified(target), _rel(target) or "", _top_end(target),
						 definition, definition_end)
	record["refs"] = list(refs.values())
	return record


def _top_level(cursor, namespaces: list[str]):
	"""Top-level declarations, namespaces and linkage blocks opened."""
	ci = _ci
	for child in cursor.get_children():
		if child.kind == ci.CursorKind.NAMESPACE:
			yield from _top_level(child, namespaces + [child.spelling])
		elif child.kind == ci.CursorKind.LINKAGE_SPEC:
			yield from _top_level(child, namespaces)
		else:
			yield child, namespaces


def _is_forward(cursor) -> bool:
	return cursor.kind in CLASS_KINDS and not cursor.is_definition()


def parse_unit(entry: dict) -> dict:
	ci = _clang()
	_kinds()
	root = _state["root"]
	unplaced = _state["unplaced"]
	seen = _state["seen"]
	os.chdir(entry["directory"])
	main = os.path.relpath(os.path.realpath(entry["file"]), root)
	options = ci.TranslationUnit.PARSE_DETAILED_PROCESSING_RECORD | ci.TranslationUnit.PARSE_INCOMPLETE
	try:
		tu = _state["index"].parse(entry["file"], compile_arguments(entry), options=options)
	except ci.TranslationUnitLoadError as error:
		return {"unit": main, "error": str(error)}
	errors = [str(d) for d in tu.diagnostics if d.severity >= ci.Diagnostic.Error]
	includes = set()
	for inclusion in tu.get_includes():
		source, included = _rel(inclusion.location), None
		if inclusion.include is not None:
			path = os.path.realpath(inclusion.include.name)
			if path.startswith(root + os.sep):
				included = os.path.relpath(path, root)
		if source and included:
			includes.add((source, included))
	tops = []
	for cursor, namespaces in _top_level(tu.cursor, []):
		file = _rel(cursor)
		if file:
			tops.append((cursor, namespaces, file))
	# what the Unplaced files declare: a forward declaration of a class is no entity of theirs (support); a
	# member's uses decide nothing
	visible = {cursor.get_usr(): cursor.spelling for cursor, _, file in tops
			   if file in unplaced and not _is_forward(cursor)
			   and not (cursor.semantic_parent is not None and cursor.semantic_parent.kind in CLASS_KINDS)}
	visible.pop("", None)
	names = set(visible.values())
	entities, uses, redeclarations, methods = [], [], [], []
	# a file including an Unplaced header directly: what it names, by declaring file (its include's replacement)
	direct = {source for source, included in includes if included in unplaced and source not in unplaced}
	needs: dict[str, set] = {}
	walked = set()
	for cursor, namespaces, file in tops:
		if file != main and file in seen:
			continue
		walked.add(file)
		key = None
		if file in unplaced and cursor.kind != ci.CursorKind.MACRO_INSTANTIATION:
			record = _entity(cursor, file, namespaces)
			record["forward"] = _is_forward(cursor)
			key = (file, record["start"], record["start_col"])
			entities.append(record)
		elif cursor.get_usr() in visible and cursor.kind != ci.CursorKind.MACRO_DEFINITION:
			redeclarations.append((cursor.get_usr(), file, cursor.location.line, cursor.kind.name,
								   cursor.is_definition()))
		if cursor.kind in FUNCTION_KINDS and cursor.is_definition():
			parent = cursor.semantic_parent
			if parent is not None and parent.kind in CLASS_KINDS:
				methods.append((parent.get_usr(), file))
		# a file that spells no entity's name uses none: its cursors are not walked (unless its needs count)
		if file in direct or file in unplaced or _identifiers(file) & names:
			_walk_uses(cursor, file, key, uses, visible, needs.setdefault(file, set()) if file in direct else None)
	seen.update(walked)
	files = sorted({file for _, _, file in tops} | {included for _, included in includes})
	return {"unit": main, "errors": errors[:20], "error_count": len(errors), "includes": sorted(includes),
			"entities": entities, "uses": uses, "redeclarations": redeclarations, "methods": methods,
			"needs": {file: sorted(found) for file, found in needs.items()}, "files": files}


_identifier_cache: dict[str, frozenset] = {}


def _identifiers(file: str) -> frozenset:
	if file not in _identifier_cache:
		with open(os.path.join(_state["root"], file), encoding="utf-8", errors="replace") as source:
			_identifier_cache[file] = frozenset(re.findall(r"[A-Za-z_]\w*", source.read()))
	return _identifier_cache[file]


# ---------------------------------------------------------------------------------------------------------------
# measure: the model
# ---------------------------------------------------------------------------------------------------------------

def unplaced_files(root: str) -> list[str]:
	return sorted(path for path in tree_files(root) if UNPLACED.search(path))


def compile_commands(root: str, build: str) -> dict[str, dict]:
	with open(os.path.join(build, "compile_commands.json"), encoding="utf-8") as source:
		commands = json.load(source)
	units = {}
	for entry in commands:
		path = os.path.relpath(os.path.realpath(os.path.join(entry["directory"], entry["file"])), root)
		units.setdefault(path, entry)
	return units


def parse_units(root: str, unplaced: list[str], units: list[dict], jobs: int, label: str) -> list[dict]:
	print(f"unplaced: {label}: {len(units)} translation units to parse", file=sys.stderr)
	results = []
	if not units:
		return results
	with concurrent.futures.ProcessPoolExecutor(jobs, initializer=_init_worker, initargs=(root, unplaced)) as pool:
		for done, result in enumerate(pool.map(parse_unit, units, chunksize=2), 1):
			results.append(result)
			if done % 100 == 0:
				print(f"unplaced: {label}: {done}/{len(units)} parsed", file=sys.stderr)
	return results


def measure(root: str, build: str, jobs: int) -> dict:
	"""Two passes: the units of the Unplaced .cpp files and of the files including an Unplaced file give the
	entities; then one unit for each other file that spells an entity's name gives the rest of their uses."""
	unplaced = unplaced_files(root)
	graph = include_graph(root)
	commands = compile_commands(root, build)
	unplaced_set = set(unplaced)
	first = [entry for path, entry in sorted(commands.items())
			 if path in unplaced_set or unplaced_set & set(graph.get(path, []))]
	results = parse_units(root, unplaced, first, jobs, "entities")
	names = {record["name"] for result in results for record in result.get("entities", [])
			 if record["kind"] not in ("INCLUSION_DIRECTIVE", "MACRO_INSTANTIATION") and not record.get("forward")
			 and not record["parent"]}
	covered = set()
	for result in results:
		covered.update(result.get("files", []))
	closures: dict[str, set] = {}

	def closure(path: str) -> set:
		if path not in closures:
			seen, stack = set(), [path]
			while stack:
				for found in graph.get(stack.pop(), []):
					if found not in seen:
						seen.add(found)
						stack.append(found)
			closures[path] = seen
		return closures[path]

	spelling = re.compile(r"[A-Za-z_]\w*")
	second = []
	for path in sorted(graph):
		if path in covered or path in unplaced_set:
			continue
		with open(os.path.join(root, path), encoding="utf-8", errors="replace") as source:
			if not names & set(spelling.findall(source.read())):
				continue
		unit = path if path in commands else next((cpp for cpp in sorted(commands) if path in closure(cpp)), None)
		if unit is None:
			continue
		second.append(commands[unit])
		covered.add(unit)
		covered.update(closure(unit))
	results += parse_units(root, unplaced, second, jobs, "uses")
	entities, seen = [], set()
	uses, redeclarations, includes = set(), set(), set()
	methods: dict[str, collections.Counter] = collections.defaultdict(collections.Counter)
	needs: dict[str, set] = collections.defaultdict(set)
	failed = []
	for result in results:
		if "error" in result or result["error_count"]:
			failed.append({"unit": result["unit"], "error": result.get("error") or result["errors"][:3]})
		if "entities" not in result:
			continue
		for record in result["entities"]:
			key = (record["file"], record["start"], record["start_col"], record["kind"], record["usr"])
			if key not in seen:
				seen.add(key)
				entities.append(record)
		uses.update((usr, file, line, tuple(key) if key else None) for usr, file, line, key in result["uses"])
		redeclarations.update(tuple(row) for row in result["redeclarations"])
		includes.update(tuple(edge) for edge in result["includes"])
		for usr, file in result["methods"]:
			methods[usr][file] += 1
		for file, found in result["needs"].items():
			needs[file].update(found)
	entities.sort(key=lambda record: (record["file"], record["start"], record["start_col"]))
	return {
		"root": root, "unplaced": unplaced,
		"digests": {path: digest(os.path.join(root, path)) for path in unplaced},
		"units": [result["unit"] for result in results], "failed": failed,
		"entities": entities, "uses": sorted(uses, key=str), "redeclarations": sorted(redeclarations),
		"includes": sorted(includes), "methods": {usr: dict(counter) for usr, counter in methods.items()},
		"needs": {file: sorted(found) for file, found in sorted(needs.items())},
	}


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
# decide: a home for every entity
# ---------------------------------------------------------------------------------------------------------------

METHOD_KINDS = {"CXX_METHOD", "CONSTRUCTOR", "DESTRUCTOR", "CONVERSION_FUNCTION"}
TYPE_KINDS = {"CLASS_DECL", "STRUCT_DECL", "UNION_DECL", "ENUM_DECL", "CLASS_TEMPLATE",
			  "CLASS_TEMPLATE_PARTIAL_SPECIALIZATION"}
TYPEDEF_KINDS = {"TYPEDEF_DECL", "TYPE_ALIAS_DECL", "TYPE_ALIAS_TEMPLATE_DECL"}
COLUMNS = ("id", "file", "start", "end", "kind", "name", "rule", "home_cpp", "home_h", "internal", "evidence",
		   "users", "chosen", "usr")


def category(record: dict) -> str:
	kind = record["kind"]
	if kind == "INCLUSION_DIRECTIVE":
		return "include"
	if record.get("forward"):
		return "forward"
	if kind == "MACRO_DEFINITION":
		return "macro"
	if record["parent"] and (kind in METHOD_KINDS or kind in ("FUNCTION_TEMPLATE", "VAR_DECL")):
		return "member"
	if kind in ("FUNCTION_DECL", "FUNCTION_TEMPLATE"):
		return "function"
	if kind == "VAR_DECL":
		return "global"
	if kind == "CLASS_TEMPLATE":
		return "template"
	if kind in TYPE_KINDS:
		return "type"
	if kind in TYPEDEF_KINDS:
		return "typedef"
	return "other"


def row_kind(record: dict) -> str:
	what = category(record)
	if what in ("function", "member", "global"):
		return f"{what}-{'definition' if record['definition'] else 'declaration'}"
	return what


def strip_templates(name: str) -> str:
	out, depth = [], 0
	for character in name:
		if character == "<":
			depth += 1
		elif character == ">":
			depth = max(0, depth - 1)
		elif depth == 0:
			out.append(character)
	return "".join(out)


def pdb_sources(root: str, out: str) -> dict[str, str]:
	"""names.py's table of each function's source file (relative to src/), made once per --out."""
	path = os.path.join(out, "pdb-sources.tsv")
	if not os.path.exists(path):
		import subprocess
		subprocess.run([sys.executable, os.path.join(root, "harness", "tools", "names.py"), "sources", path],
					   cwd=root, check=True)
	table = {}
	with open(path, encoding="utf-8") as source:
		for line in source:
			name, _, file = line.rstrip("\n").partition("\t")
			table[name] = "src/" + file
	return table


class Homes:
	"""The files of the tree a home is chosen among, and the debug symbols' source files."""

	def __init__(self, root: str, model: dict, pdb: dict[str, str]):
		self.root = root
		self.unplaced = set(model["unplaced"])
		self.graph = include_graph(root)
		self.pdb_exact = pdb
		self.pdb_stripped: dict[str, collections.Counter] = collections.defaultdict(collections.Counter)
		self.pdb_class: dict[str, collections.Counter] = collections.defaultdict(collections.Counter)
		for name, file in pdb.items():
			stripped = strip_templates(name)
			self.pdb_stripped[stripped][file] += 1
			at = stripped.find("::")
			while at >= 0:
				self.pdb_class[stripped[:at]][file] += 1
				at = stripped.find("::", at + 1)
		self.methods = model["methods"]

	def exists(self, path: str) -> bool:
		return os.path.isfile(os.path.join(self.root, path))

	def pdb(self, name: str) -> str | None:
		found = self.pdb_exact.get(name)
		if found:
			return found
		counts = self.pdb_stripped.get(strip_templates(name))
		if counts and len(counts) == 1:
			return next(iter(counts))
		return None

	def header_of(self, cpp: str) -> tuple[str, str]:
		"""The header a .cpp's declarations go to, and how it was found."""
		stem, _ = os.path.splitext(cpp)
		if self.exists(stem + ".h"):
			return stem + ".h", "its own header"
		if stem.endswith("Class") and self.exists(stem[:-len("Class")] + ".h"):
			return stem[:-len("Class")] + ".h", "the header of the class it describes"
		folder = os.path.dirname(cpp)
		for included in self.graph.get(cpp, []):
			if os.path.dirname(included) == folder and included not in self.unplaced and included.endswith(".h"):
				return included, "the first header of its folder it includes"
		return stem + ".h", "a new header named after it"

	def cpp_of(self, header: str) -> str | None:
		stem, _ = os.path.splitext(header)
		return stem + ".cpp" if self.exists(stem + ".cpp") else None

	def class_cpp(self, usr: str, name: str, header: str) -> tuple[str | None, str]:
		"""A class's .cpp: by the debug symbols (its own .cpp among its members' sources, else the one holding most
		of them), else the file defining most of its other members, else the one named after its header."""
		stripped = strip_templates(name)
		counts = self.pdb_class.get(stripped)
		if counts:
			own = [file for file in counts if os.path.splitext(os.path.basename(file))[0].lower() == stripped.split(
				"::")[-1].lower() and file.endswith(".cpp")]
			cpps = {file: count for file, count in counts.items() if file.endswith(".cpp")}
			if own:
				return own[0], "its members' source file in the debug symbols"
			if cpps:
				return max(sorted(cpps), key=cpps.get), "the source file of most of its members in the debug symbols"
		files = {file: count for file, count in self.methods.get(usr, {}).items()
				 if file not in self.unplaced and file.endswith(".cpp")}
		if files:
			return max(sorted(files), key=files.get), "the file defining most of its members"
		if header and header.endswith((".h", ".hpp")):
			found = self.cpp_of(header)
			if found:
				return found, "the .cpp of its header"
		return None, ""


def isolated(entity_file: str, home: str | None) -> bool:
	"""src/Havok includes no game header: a src/Havok entity keeps a src/Havok home."""
	return not home or not entity_file.startswith("src/Havok/") or home.startswith("src/Havok/")


def is_header(path: str) -> bool:
	return path.endswith((".h", ".hpp", ".inl"))


def decide(model: dict, homes: Homes) -> list[dict]:
	unplaced = homes.unplaced
	records = [record for record in model["entities"] if record["kind"] != "MACRO_INSTANTIATION"]
	by_key = {(record["file"], record["start"], record["start_col"]): record for record in records}
	groups: dict[str, list[dict]] = collections.defaultdict(list)
	for record in records:
		record["category"] = category(record)
		if record["category"] not in ("include", "forward"):
			groups[record["usr"] or f"{record['file']}:{record['start']}"].append(record)
	uses: dict[str, list] = collections.defaultdict(list)
	for usr, file, line, key in model["uses"]:
		uses[usr].append((file, line, by_key.get(tuple(key)) if key else None))
	outside: dict[str, list] = collections.defaultdict(list)
	for usr, file, line, kind, definition in model["redeclarations"]:
		if file not in unplaced:
			outside[usr].append((file, line, kind, definition))

	decision: dict[str, dict] = {}

	def decided(usr: str, rule: str, home_cpp: str | None, home_h: str | None, evidence: str) -> None:
		decision[usr] = {"rule": rule, "home_cpp": home_cpp or "", "home_h": home_h or "", "evidence": evidence,
						 "internal": False}

	def dest(record: dict) -> str | None:
		"""The file a record's text ends up in, None while its group has no home."""
		usr = record["usr"] or f"{record['file']}:{record['start']}"
		found = decision.get(usr)
		if found is None:
			return None
		if found["rule"] == "e":
			return record["file"]
		if not is_header(record["file"]) or found["internal"]:
			return found["home_cpp"] or found["home_h"]
		return found["home_h"] or found["home_cpp"]

	def users(usr: str, group: list[dict]) -> set[str] | None:
		"""The files naming the entity once every record moved, None while one of them has no home. A type's own
		members do not count."""
		files = set()
		for file, _, inside in uses.get(usr, []):
			if inside is None:
				if file not in unplaced:
					files.add(file)
				continue
			if inside["usr"] == usr or (inside["parent"] and inside["parent"]["usr"] == usr):
				continue
			where = dest(inside)
			if where is None:
				return None
			files.add(where)
		for file, _, _, _ in outside.get(usr, []):
			files.add(file)
		return files

	def home_files_for(group: list[dict], cpp: str | None, header: str | None) -> tuple[str | None, str | None, str]:
		"""Complete a home: a definition in a .cpp needs a .cpp, a declaration in a header a header."""
		note = ""
		needs_cpp = any(not is_header(record["file"]) for record in group)
		needs_h = any(is_header(record["file"]) for record in group)
		if needs_cpp and not cpp and header:
			cpp = homes.cpp_of(header)
			if not cpp:
				return None, None, ""
		if needs_h and not header and cpp:
			header, note = homes.header_of(cpp)
		return cpp, header, note

	def static_rules(usr: str, group: list[dict]) -> bool:
		first = group[0]
		what = first["category"]
		file = first["file"]
		if what in ("function", "global", "member"):
			defined = sorted({f for f, _, _, definition in outside.get(usr, []) if definition and not is_header(f)})
			if defined and not any(record["definition"] for record in group):
				if len(defined) == 1 and isolated(file, defined[0]):
					cpp, header, note = home_files_for(group, defined[0], None)
					if cpp and isolated(file, header):
						decided(usr, "defined", cpp, header, f"defined in {defined[0]}" + (f"; {note}" if note else ""))
						return True
				return False
			pdb = homes.pdb(first["qname"])
			if pdb and isolated(file, pdb):
				if is_header(pdb):
					cpp, header, note = home_files_for(group, None, pdb)
				else:
					cpp, header, note = home_files_for(group, pdb, None)
				if (cpp or header) and isolated(file, header):
					decided(usr, "a", cpp, header, f"debug symbols: {first['qname']} in {pdb}"
							+ (f"; header: {note}" if note else ""))
					return True
		if what == "member":
			parent = first["parent"]
			if parent["file"] in unplaced:
				return False
			cpp, how = homes.class_cpp(parent["usr"], parent["name"], parent["file"])
			if cpp and isolated(file, cpp):
				decided(usr, "b", cpp, None, f"member of {parent['name']}: {how}")
				return True
		return False

	def single_user(usr: str, group: list[dict]) -> bool:
		first = group[0]
		if first["category"] not in ("function", "global", "type", "template", "typedef"):
			return False
		files = users(usr, group)
		if files is None or len(files) != 1:
			return False
		(home,) = files
		if not isolated(first["file"], home):
			return False
		if is_header(home):
			cpp, header, note = home_files_for(group, None, home)
			if first["category"] in ("type", "template", "typedef"):
				cpp, header = None, home
			elif header is None:
				return False
			decided(usr, "c", cpp, header, f"used only in {home}")
		else:
			if first["category"] in ("type", "template", "typedef"):
				decided(usr, "c", home, None, f"used only in {home}")
			else:
				cpp, header, note = home_files_for(group, home, None)
				decided(usr, "c", home, header, f"used only in {home}")
		return True

	def first_parameter(usr: str, group: list[dict]) -> bool:
		first = group[0]
		if first["category"] != "function":
			return False
		owner = next((record["first"] for record in group if record.get("first")), None)
		if not owner or (owner["const"] and not owner["pointer"]) or not owner["file"]:
			return False
		if owner["file"] in unplaced:
			return False
		header = owner["file"] if is_header(owner["file"]) else None
		cpp, how = homes.class_cpp(owner["usr"], owner["name"], header or "")
		if not cpp and not is_header(owner["file"]):
			cpp = owner["file"]
		needs_cpp = any(not is_header(record["file"]) for record in group)
		if (needs_cpp and not cpp) or not isolated(first["file"], cpp) or not isolated(first["file"], header):
			return False
		decided(usr, "d", cpp, header, f"first parameter {owner['name']}: {owner['file']}"
				+ (f", {cpp} ({how})" if cpp else ""))
		return True

	def with_users(usr: str, group: list[dict]) -> bool:
		"""A type or alias named only by moved declarations that all go to one header (and by the files that will
		include that header for them)."""
		if group[0]["category"] not in ("type", "template", "typedef"):
			return False
		files, headers = set(), set()
		for file, _, inside in uses.get(usr, []):
			if inside is None or inside["usr"] == usr:
				continue
			where = dest(inside)
			if where is None:
				return False
			if is_header(inside["file"]):
				headers.add(where)
		if len(headers) != 1:
			return False
		(home,) = headers
		if not is_header(home) or not isolated(group[0]["file"], home):
			return False
		decided(usr, "users", None, home, f"named by declarations going to {home}")
		return True

	for usr, group in groups.items():
		static_rules(usr, group)
	pending = [usr for usr in groups if usr not in decision]
	while pending:
		progress = False
		for usr in pending:
			if single_user(usr, groups[usr]):
				progress = True
		pending = [usr for usr in pending if usr not in decision]
		if progress:
			continue
		for rule in (first_parameter, with_users):
			for usr in pending:
				if rule(usr, groups[usr]):
					progress = True
			pending = [usr for usr in pending if usr not in decision]
			if progress:
				break
		if not progress:
			break
	for usr in pending:
		decision[usr] = {"rule": "e", "home_cpp": "", "home_h": "", "evidence": "", "internal": False}

	# file-local: a function or global defined here whose every user is its home .cpp
	for usr, found in decision.items():
		group = groups[usr]
		if found["rule"] == "e" or group[0]["category"] not in ("function", "global") or any(
				record["kind"] not in ("FUNCTION_DECL", "VAR_DECL") for record in group):
			continue
		if not any(record["definition"] for record in group) or outside.get(usr):
			continue
		files = users(usr, group)
		if files and files == {found["home_cpp"]}:
			found["internal"] = True

	rows = []
	for number, record in enumerate(records, 1):
		what = record["category"]
		usr = record["usr"] or f"{record['file']}:{record['start']}"
		row = {"id": number, "file": record["file"], "start": record["start"], "end": record["end"],
			   "kind": row_kind(record), "name": record["qname"] if what != "include" else record.get("included", ""),
			   "usr": record["usr"], "chosen": "", "internal": ""}
		if what in ("include", "forward"):
			row.update(rule="support", home_cpp="", home_h="", evidence="", users="")
		else:
			found = decision[usr]
			files = users(usr, groups[usr]) or set()
			row.update(rule=found["rule"], home_cpp=found["home_cpp"], home_h=found["home_h"],
					   evidence=found["evidence"], users=";".join(sorted(files)),
					   internal="yes" if found["internal"] else "")
		rows.append(row)
	return rows


# ---------------------------------------------------------------------------------------------------------------
# apply: the moves
# ---------------------------------------------------------------------------------------------------------------

FORWARD_LINE = re.compile(r"^\s*(?:template\s*<[^;{}]*>\s*)?(?:class|struct|union|enum(?:\s+class)?)\s+(\w+)\s*"
						  r"(?::[^;{}]*)?;\s*(?://.*)?$")
SYSTEM_INCLUDE = re.compile(r'^\s*#\s*include\s*<([^>]+)>')
QUOTED_INCLUDE = re.compile(r'^\s*#\s*include\s*"([^"]+)"')
IDENT = re.compile(r"[A-Za-z_]\w*")
# attribute macros (all capitals) and attributes, which may stand on an entity's first line before it
ATTRIBUTES = re.compile(r"^\s*(?:(?:[A-Z_][A-Z0-9_]*(?:\([^()]*\))?|\[\[[^\]]*\]\])\s*)+$")


def blank_code(text: str) -> str:
	"""The text with comments, string and character literals replaced by spaces (newlines kept)."""
	out = list(text)
	i, n = 0, len(text)
	while i < n:
		c = text[i]
		if c == "/" and i + 1 < n and text[i + 1] == "/":
			j = text.find("\n", i)
			j = n if j < 0 else j
			out[i:j] = " " * (j - i)
			i = j
		elif c == "/" and i + 1 < n and text[i + 1] == "*":
			j = text.find("*/", i + 2)
			j = n if j < 0 else j + 2
			out[i:j] = [ch if ch == "\n" else " " for ch in text[i:j]]
			i = j
		elif c in "\"'":
			if c == "'" and i > 0 and text[i - 1].isalnum() and i + 1 < n and text[i + 1].isalnum():
				i += 1
				continue
			j = i + 1
			while j < n and text[j] != c and text[j] != "\n":
				j += 2 if text[j] == "\\" else 1
			j = min(j + 1, n)
			out[i:j] = " " * (j - i)
			i = j
		else:
			i += 1
	return "".join(out)


def normalized(lines: list[str]) -> str:
	return " ".join(blank_code("".join(lines)).split())


def spelled(path: str) -> str:
	"""How the tree writes an include of a tree file: from its include root."""
	for base in INCLUDE_ROOTS:
		if path.startswith(base + "/"):
			return path[len(base) + 1:]
	return path


TEMPLATE_HEAD = re.compile(r"^\s*template\s*<[^;{}]*>\s*$")
NAMESPACE_OPEN = re.compile(r"^\s*namespace\s+(\w+)\s*\{\s*$")
NAMESPACE_CLOSE = re.compile(r"^\s*\}\s*(?://.*)?$")


def forward_spans(lines: list[str]) -> list[tuple[int, int, str, str]]:
	"""The forward declarations of a file's lines: (first line, last line, the name, the declaration as it is
	written elsewhere). A declaration is one line, a template head line and its declaration, or a namespace block
	holding nothing but declarations (one entry per declaration, each written in its namespace)."""
	out, i, n = [], 0, len(lines)
	while i < n:
		line = lines[i]
		match = FORWARD_LINE.match(line)
		if match:
			out.append((i, i, match.group(1), line.strip() + "\n"))
			i += 1
			continue
		if TEMPLATE_HEAD.match(line) and i + 1 < n and (match := FORWARD_LINE.match(lines[i + 1])):
			out.append((i, i + 1, match.group(1), line.strip() + "\n" + lines[i + 1].strip() + "\n"))
			i += 2
			continue
		opened = NAMESPACE_OPEN.match(line)
		if opened:
			j, inner = i + 1, []
			while j < n and (FORWARD_LINE.match(lines[j]) or not lines[j].strip()):
				if lines[j].strip():
					inner.append(lines[j].strip())
				j += 1
			if inner and j < n and NAMESPACE_CLOSE.match(lines[j]):
				for declaration in inner:
					out.append((i, j, FORWARD_LINE.match(declaration).group(1),
								f"namespace {opened.group(1)} {{\n{declaration}\n}}\n"))
				i = j + 1
				continue
		i += 1
	return out


def contains(lines: list[str], text: str) -> bool:
	"""Whether a file's lines hold a declaration as forward_spans writes it (one line or several)."""
	return text in lines if text.count("\n") == 1 else text in "".join(lines)


def preamble_end(lines: list[str]) -> tuple[int, int]:
	"""(the last line of the leading includes, pragmas and forward declarations; the last #include among them),
	-1 for none."""
	last = last_include = -1
	in_comment = False
	spans = {first: end for first, end, _, _ in forward_spans(lines)}
	skip_to = -1
	for i, line in enumerate(lines):
		if i <= skip_to:
			continue
		s = line.strip()
		if in_comment:
			in_comment = "*/" not in s
			continue
		if s.startswith("/*"):
			in_comment = "*/" not in s
			continue
		if not s or s.startswith("//"):
			continue
		if re.match(r"#\s*(pragma|include)\b", s):
			last = i
			if re.match(r"#\s*include", s):
				last_include = i
			continue
		if i in spans:
			last = skip_to = spans[i]
			continue
		break
	return last, last_include


class Edits:
	"""Line edits of one file, on its lines as read; applied at the end."""

	def __init__(self, lines: list[str], new: bool):
		self.lines = lines
		self.new = new
		self.drop: set[int] = set()
		self.replace: dict[int, list[str]] = {}
		# moved blocks after a line, each with its rank: types and aliases, then declarations, then definitions
		self.after: dict[int, list[tuple[int, list[str]]]] = collections.defaultdict(list)
		self.end: list[str] = []
		self.includes: list[str] = []  # include lines to add after the last include
		self.forwards: list[str] = []  # forward declarations to add after the includes and forward declarations

	def changed(self) -> bool:
		return bool(self.drop or self.replace or any(self.after.values()) or self.end or self.includes
					or self.forwards)

	def text(self) -> str:
		lines = list(self.lines)
		last, last_include = preamble_end(lines)
		anchor = last_include if last_include >= 0 else last
		after = {index: [line for _, block in sorted(blocks, key=lambda placed: placed[0]) for line in block]
				 for index, blocks in self.after.items()}
		if self.forwards:
			after[last] = ["\n"] + self.forwards + ["\n"] + after.get(last, [])
		if self.includes:
			after[anchor] = self.includes + after.get(anchor, [])
		out = []
		for i in range(-1, len(lines)):
			if i >= 0:
				if i in self.replace:
					out.extend(self.replace[i])
				elif i not in self.drop:
					out.append(lines[i])
			out.extend(after.get(i, []))
		if self.end:
			out.append("\n")
			out.extend(self.end)
		# at most one blank line in a row, none at the start or the end
		result = []
		for line in out:
			if not line.strip() and (not result or not result[-1].strip()):
				continue
			result.append(line)
		while result and not result[-1].strip():
			result.pop()
		return "".join(result)


class Mover:
	def __init__(self, root: str, model: dict, rows: list[dict], homes: Homes, stays: dict[tuple, str],
				 rehomed: dict[tuple, str]):
		self.root = root
		self.model = model
		self.homes = homes
		self.stays = stays  # rows known not to move ((file, start, usr) -> why), from an earlier run
		self.rehomed = dict(rehomed)  # rows going to another header than their home's, which would include itself
		self.unplaced = set(model["unplaced"])
		self.graph = {path: list(found) for path, found in homes.graph.items()}
		for source, included in model["includes"]:
			if included not in self.graph.setdefault(source, []):
				self.graph[source].append(included)
		self.rows = rows
		self.edits: dict[str, Edits] = {}
		self.moved, self.left, self.internal, self.include_rows = [], [], [], []
		self.deleted: list[str] = []
		self.dropped_comments = []
		self.closures: dict[str, frozenset] = {}

	# -- files -----------------------------------------------------------------------------------------------

	def file(self, path: str) -> Edits:
		if path not in self.edits:
			full = os.path.join(self.root, path)
			if os.path.exists(full):
				with open(full, encoding="utf-8") as source:
					self.edits[path] = Edits(source.readlines(), False)
			else:
				self.edits[path] = Edits(["#pragma once\n"] if is_header(path) else [], True)
		return self.edits[path]

	def closure(self, path: str) -> frozenset:
		"""Every tree file path includes, not through an Unplaced file."""
		if path not in self.closures:
			seen, stack = set(), [path]
			while stack:
				for found in self.graph.get(stack.pop(), []):
					if found not in seen and found not in self.unplaced:
						seen.add(found)
						stack.append(found)
			self.closures[path] = frozenset(seen)
		return self.closures[path]

	def add_edge(self, source: str, included: str) -> None:
		self.graph.setdefault(source, []).append(included)
		self.closures.clear()

	def expanded_includes(self, path: str) -> tuple[list[str], list[str]]:
		"""The tree headers and system headers a file includes, an Unplaced header's own includes in its place."""
		tree, system, seen = [], [], set()

		def visit(file: str) -> None:
			if file in seen:
				return
			seen.add(file)
			for line in self.file(file).lines:
				found = SYSTEM_INCLUDE.match(line)
				if found:
					if found.group(1) not in system:
						system.append(found.group(1))
					continue
				found = QUOTED_INCLUDE.match(line)
				if found:
					resolved = resolve_include(self.root, file, found.group(1))
					if resolved in self.unplaced:
						visit(resolved)
					elif resolved and resolved not in tree:
						tree.append(resolved)

		visit(path)
		return tree, system

	def forwards(self, path: str) -> dict[str, str]:
		"""The forward declarations of an Unplaced file and the Unplaced headers it includes: name -> declaration."""
		found, seen, stack = {}, set(), [path]
		while stack:
			file = stack.pop()
			if file in seen:
				continue
			seen.add(file)
			lines = self.file(file).lines
			for _, _, name, declaration in forward_spans(lines):
				found.setdefault(name, declaration)
			for line in lines:
				include = QUOTED_INCLUDE.match(line)
				if include:
					resolved = resolve_include(self.root, file, include.group(1))
					if resolved in self.unplaced:
						stack.append(resolved)
		return found

	# -- one entity --------------------------------------------------------------------------------------------

	def span(self, record: dict) -> tuple[int, int] | str:
		"""The record's whole lines (0-based, inclusive) with the comment above it, or why it has none."""
		lines = self.file(record["file"]).lines
		start, end = record["start"] - 1, record["end"] - 1
		if end >= len(lines):
			return "the model's lines do not match the file"
		# what may stand before the entity on its first line: attribute macros and attributes
		prefix = blank_code(lines[start][:record["start_col"] - 1])
		if prefix.strip() and not ATTRIBUTES.match(prefix):
			return "shares its first line with other code"
		tail = blank_code(lines[end][record["end_col"] - 1:]).strip()
		if tail not in ("", ";", "};"):
			return "shares its last line with other code"
		first = start
		while first > 0:
			above = lines[first - 1].strip()
			if above.startswith("//"):
				first -= 1
			elif above.endswith("*/"):
				top = first - 1
				while top >= 0 and "/*" not in lines[top]:
					top -= 1
				if top < 0 or not lines[top].strip().startswith("/*"):
					break
				first = top
			else:
				break
		return first, end

	def moved_text(self, record: dict, first: int, end: int, internal: bool) -> list[str] | None:
		"""The record's lines for its home, None when it has no place there (a file-local global's declaration)."""
		lines = self.file(record["file"]).lines[first:end + 1]
		if internal:
			if record["kind"] == "VAR_DECL" and not record["definition"]:
				return None
			at = record["start"] - 1 - first
			column = record["start_col"] - 1
			line = lines[at]
			rest = line[column:]
			if rest.startswith("inline "):
				rest = "static " + rest[len("inline "):]
			elif rest.startswith("extern "):
				rest = "static " + rest[len("extern "):]
			elif not rest.startswith("static "):
				rest = "static " + rest
			lines[at] = line[:column] + rest
		for name in reversed(record["namespaces"]):
			label = f"namespace {name}" if name else "namespace"
			lines = [f"{label} {{\n", "\n"] + lines + ["\n", f"}}  // {label}\n"]
		return lines

	def placement(self, record: dict, target: str, internal: bool, moved: set) -> tuple[str, int] | str:
		"""Where the text goes in target: ("top", line) after the includes and forward declarations, ("end", -1),
		or ("after", line): after the last of target's own top-level declarations it names, before target's first
		use of it. Or why it cannot go there (it names code of target that comes after target's first use of it)."""
		lines = self.file(target).lines
		last, _ = preamble_end(lines)
		after = -1
		for usr, kind, _, file, end, definition, definition_end in record["refs"]:
			if usr in moved:
				continue
			if file == target:
				after = max(after, end - 1)
			if definition == target and kind in ("CLASS_DECL", "STRUCT_DECL", "UNION_DECL", "CLASS_TEMPLATE",
												  "ENUM_DECL"):
				after = max(after, definition_end - 1)
		uses = self.uses_at.get((record["usr"], target))
		before = min(uses) - 1 if uses else None
		if record["category"] in ("type", "template", "typedef", "macro") or (internal and (
				record["kind"] == "VAR_DECL" or is_header(record["file"]))) or (
				not is_header(target) and is_header(record["file"])):
			where = "top"
		elif before is None:
			where = "end"
		else:
			where = "after"
		if where == "end":
			return "end", -1
		at = max(after, last)
		if before is not None and at >= before:
			return (f"names code of {target} (up to line {after + 1}) that comes after its first use there "
					f"(line {before + 1})")
		return ("top", last) if at == last else ("after", at)

	def cover(self, candidates: list[str], wanted: list[set], have: set) -> list[str]:
		"""The fewest candidates (in their order) whose includes reach every wanted set that have does not: first
		the candidates that are themselves wanted, then the one reaching most of the rest, until none is left."""
		wanted = [files for files in wanted if files and not files & have]
		chosen = []
		for include in candidates:
			if any(include in files for files in wanted):
				chosen.append(include)
		for include in chosen:
			reach = self.closure(include) | {include}
			wanted = [files for files in wanted if not files & reach]
		while wanted:
			best, hits = None, 0
			for include in candidates:
				if include in chosen:
					continue
				reach = self.closure(include) | {include}
				count = sum(1 for files in wanted if files & reach)
				if count > hits:
					best, hits = include, count
			if best is None:
				break
			chosen.append(best)
			reach = self.closure(best) | {best}
			wanted = [files for files in wanted if not files & reach]
		return [include for include in candidates if include in chosen]

	def visible(self, path: str, upto: int | None) -> set[str]:
		"""The tree files the code of path sees at its line upto (0-based; None: its end): the file, the headers it
		includes above that line or is given by this run, and theirs."""
		edits = self.file(path)
		seen = {path}
		included = []
		for i, line in enumerate(edits.lines):
			if upto is not None and i > upto:
				break
			match = QUOTED_INCLUDE.match(line)
			if match:
				included.append(resolve_include(self.root, path, match.group(1)))
		included += [resolve_include(self.root, path, QUOTED_INCLUDE.match(line).group(1))
					 for line in edits.includes]
		for include in included:
			if include and include not in self.unplaced and include not in seen:
				seen |= self.closure(include) | {include}
		return seen

	def needed_includes(self, record: dict, target: str, upto: int | None,
						homes_of: dict[str, str]) -> tuple[list[str], list[tuple[str, str]]]:
		"""(includes, forward declarations as (name, declaration)) the record's text needs in target at its line
		upto: the homes of the Unplaced entities it names, the Unplaced headers keeping what it names, the fewest
		includes of its Unplaced file whose headers declare (a class: define) the rest of what it names, the forward
		declarations of its Unplaced file it names (a declaration that named a class through one keeps naming it
		so)."""
		have = self.visible(target, upto)
		known = self.forwards(record["file"])
		wanted, classes = [], {}
		includes = []
		for usr, kind, name, file, _, definition, _ in record["refs"]:
			if usr in homes_of:
				home = homes_of[usr]
				if home and home != target and is_header(home) and home not in have and home not in includes:
					includes.append(home)
				continue
			if self.staying.get(usr):
				for header in sorted(self.staying[usr]):
					if header != target and header not in have and header not in includes:
						includes.append(header)
				continue
			if kind in ("CLASS_DECL", "STRUCT_DECL", "UNION_DECL", "CLASS_TEMPLATE"):
				classes[name] = (file, definition)
				if (record["category"] in ("function", "global") and not record["definition"]
						and name.split("::")[-1] in known):
					continue
				found = definition or file
			else:
				found = file
			if found and found not in self.unplaced:
				wanted.append({found})
		for include in includes:
			have = have | self.closure(include) | {include}
		tree, _ = self.expanded_includes(record["file"])
		includes += [include for include in self.cover(tree, wanted, have) if include not in includes]
		forwards = []
		covered = set(have)
		for include in includes:
			covered |= self.closure(include) | {include}
		for name, (file, definition) in classes.items():
			short = name.split("::")[-1]
			if short in known and (not definition or definition not in covered) and file not in covered:
				forwards.append((short, known[short]))
		return includes, forwards

	def stay(self, row: dict) -> None:
		"""Note a row that stays in its Unplaced file: a header holding it is included by the texts naming it."""
		if row["usr"] and is_header(row["file"]):
			self.staying[row["usr"]].add(row["file"])

	# -- the whole table ---------------------------------------------------------------------------------------

	def run(self) -> None:
		homes = self.homes
		records = {}
		for record in self.model["entities"]:
			if record["kind"] == "MACRO_INSTANTIATION":
				continue
			record["category"] = category(record)
			records[(record["file"], record["start"], record["usr"])] = record

		# the destination of each moving record
		plan = []
		homes_of: dict[str, str] = {}
		# the entities that stay in their Unplaced file (open rows, rows that cannot move): a moved text naming one
		# includes its Unplaced header
		self.staying: dict[str, set[str]] = collections.defaultdict(set)
		for row in self.rows:
			row = dict(row)
			if row["rule"] != "support" and row.get("chosen"):
				parts = [part for part in row["chosen"].split(";") if part]
				cpp = next((part for part in parts if part.endswith(".cpp")), "")
				header = next((part for part in parts if is_header(part)), "")
				if not header and cpp:
					header = homes.header_of(cpp)[0]
				row.update(rule="chosen", home_cpp=cpp, home_h=header, internal="")
			if row["rule"] == "support":
				continue
			if row["rule"] == "e":
				self.stay(row)
				continue
			key = (row["file"], int(row["start"]), row["usr"])
			if key in self.stays:
				self.left.append({**row, "why": self.stays[key]})
				self.stay(row)
				continue
			record = records.get(key)
			if record is None:
				self.left.append({**row, "why": "not in the model (measure again)"})
				self.stay(row)
				continue
			internal = row["internal"] == "yes"
			if not is_header(record["file"]) or internal:
				target = row["home_cpp"] or row["home_h"]
			else:
				target = row["home_h"] or row["home_cpp"]
			if not target:
				self.left.append({**row, "why": "no home file"})
				self.stay(row)
				continue
			if target in self.unplaced:
				self.left.append({**row, "why": f"its home {target} is an Unplaced file"})
				self.stay(row)
				continue
			target = self.rehomed.get(key, target)
			plan.append((row, record, target, internal))
			if record["usr"] and (is_header(record["file"]) and not internal or record["category"] in (
					"type", "template", "typedef", "macro")):
				homes_of.setdefault(record["usr"], target)

		# the texts
		self.uses_at: dict[tuple, list[int]] = collections.defaultdict(list)
		for usr, file, line, inside in self.model["uses"]:
			if inside is None:
				self.uses_at[(usr, file)].append(line)
		moved_usrs = {record["usr"] for _, record, _, _ in plan}
		definition_home = {record["usr"]: target for _, record, target, _ in plan
						   if not is_header(record["file"]) and not is_header(target)}
		inserted: dict[str, set] = collections.defaultdict(set)
		for row, record, target, internal in plan:
			span = self.span(record)
			if isinstance(span, str):
				self.left.append({**row, "why": span})
				self.stay(row)
				if homes_of.get(record["usr"]) == target:
					del homes_of[record["usr"]]
				continue
			first, end = span
			source = self.file(record["file"])
			text = self.moved_text(record, first, end, internal)
			if text is None:
				source.drop.update(range(first, end + 1))
				self.moved.append({**row, "to": "", "placement": "dropped (a file-local global's declaration)"})
				continue
			# a declaration whose home header would include itself goes to the header of its definition's file,
			# else to the header of its one user: where the decision falls once the other entities have moved
			candidates = [target]
			if is_header(record["file"]) and is_header(target) and not internal:
				users = [file for file in row.get("users", "").split(";") if file]
				for file in ([definition_home[record["usr"]]] if record["usr"] in definition_home else []) + (
						users if len(users) == 1 else []):
					header = file if is_header(file) else homes.header_of(file)[0]
					if header not in candidates and header not in self.unplaced and isolated(record["file"], header):
						candidates.append(header)
			why = ""
			for candidate in candidates:
				destination = self.file(candidate)
				key = normalized(text)
				if key in inserted[candidate] or key in normalized(destination.lines):
					where = ("already there", -1)
					break
				where = self.placement(record, candidate, internal, moved_usrs)
				if isinstance(where, str):
					why = why or where
					continue
				includes, forwards = self.needed_includes(
					record, candidate, None if where[0] == "end" else where[1], homes_of)
				cycle = [include for include in includes if is_header(candidate) and candidate in self.closure(include)]
				if cycle:
					why = why or f"{candidate} would include {cycle[0]}, which includes it"
					continue
				break
			else:
				self.left.append({**row, "why": why})
				self.stay(row)
				if homes_of.get(record["usr"]) == target:
					del homes_of[record["usr"]]
				continue
			at_row = (row["file"], int(row["start"]), row["usr"])
			home = row["home_h"] or row["home_cpp"]
			if candidate != target:
				self.rehomed[at_row] = candidate
				if homes_of.get(record["usr"]) == target:
					homes_of[record["usr"]] = candidate
			note = f" (not {home}: it would include itself)" if self.rehomed.get(at_row) == candidate else ""
			target = candidate
			if where[0] == "already there":
				source.drop.update(range(first, end + 1))
				self.moved.append({**row, "to": target, "placement": "already there"})
				continue
			source.drop.update(range(first, end + 1))
			inserted[target].add(key)
			# an include further down the target does not count: the text goes above it, and so does the include
			head = destination.lines[:preamble_end(destination.lines)[0] + 1]
			for include in includes:
				line = f'#include "{spelled(include)}"\n'
				if line not in destination.includes and line not in head:
					destination.includes.append(line)
					destination.drop.update(i for i in range(len(head), len(destination.lines))
											if destination.lines[i] == line)
					self.add_edge(target, include)
			# a forward declaration the text names, which no header the target includes already gives
			named = set(IDENT.findall(blank_code("".join(text))))
			reach = self.closure(target)
			for name, declaration in forwards:
				if name not in named:
					continue
				if contains(destination.lines, declaration) or declaration in destination.forwards or any(
						declaration in edits.forwards or contains(edits.lines, declaration)
						for path, edits in self.edits.items() if path in reach):
					continue
				destination.forwards.append(declaration)
			block = ["\n"] + text + ["\n"]
			where, at = where
			if where == "end":
				destination.end.extend(block)
			else:
				rank = 0 if record["category"] in ("type", "template", "typedef", "macro") else (
					2 if record["definition"] else 1)
				destination.after[at].append((rank, block))
			if internal:
				self.internal.append({"name": row["name"], "kind": row["kind"], "file": target})
			# the entity's first line (each namespace written around it adds two lines above)
			at = record["start"] - 1 - first + 2 * len(record["namespaces"])
			self.moved.append({**row, "to": target, "placement": where + note,
							   "includes": " ".join(spelled(include) for include in includes),
							   "line_text": text[at].strip()})
		# a new .cpp includes its header
		for path, edits in list(self.edits.items()):
			if edits.new and edits.changed() and not is_header(path):
				header = os.path.splitext(path)[0] + ".h"
				if header in self.edits and self.edits[header].changed() or os.path.exists(
						os.path.join(self.root, header)):
					edits.includes.insert(0, f'#include "{spelled(header)}"\n')

		# the Unplaced files that empty out: what is left is includes, forward declarations, comments and the braces
		# of namespaces whose declarations all moved
		for path in sorted(self.unplaced):
			edits = self.file(path)
			support = {i for first, end, _, _ in forward_spans(edits.lines) for i in range(first, end + 1)}
			remaining = []
			for i, line in enumerate(edits.lines):
				s = line.strip()
				if i in edits.drop or i in support or not s or s.startswith("#pragma once") or QUOTED_INCLUDE.match(
						line) or SYSTEM_INCLUDE.match(line):
					continue
				remaining.append((i, line))
			code = [(i, blank_code(line).strip()) for i, line in remaining if blank_code(line).strip()]
			emptied = True
			while emptied:
				emptied = False
				for at in range(len(code) - 1):
					if NAMESPACE_OPEN.match(code[at][1]) and code[at + 1][1] == "}":
						del code[at:at + 2]
						emptied = True
						break
			if not code:
				self.deleted.append(path)
				for i, line in remaining:
					if not blank_code(line).strip():
						self.dropped_comments.append({"file": path, "line": i + 1, "text": line.rstrip("\n")})
		deleted = set(self.deleted)
		# from here on the Unplaced files that stay are files like any other
		self.unplaced = deleted
		self.closures.clear()

		# each include of a deleted Unplaced header: in its place the homes the file uses, the fewest of the
		# Unplaced header's includes that declare what else the file names, and the forward declarations it names
		needs = self.model["needs"]
		uses_by_file: dict[str, set] = collections.defaultdict(set)
		for usr, file, _, _ in self.model["uses"]:
			uses_by_file[file].add(usr)
		defined_in = {}
		for record in self.model["entities"]:
			for _, kind, name, file, _, definition, _ in record["refs"]:
				if kind in ("CLASS_DECL", "STRUCT_DECL", "UNION_DECL", "CLASS_TEMPLATE") and definition:
					defined_in.setdefault(name.split("::")[-1], definition)
		for path in sorted(self.graph):
			if path in deleted or not os.path.exists(os.path.join(self.root, path)):
				continue
			for i, line in enumerate(self.file(path).lines):
				match = QUOTED_INCLUDE.match(line)
				if not match:
					continue
				resolved = resolve_include(self.root, path, match.group(1))
				if resolved not in deleted:
					continue
				edits = self.file(path)
				replacement = []
				have = self.visible(path, i)
				for usr in sorted(uses_by_file.get(path, ())):
					home = homes_of.get(usr)
					if home and home != path and is_header(home) and home not in replacement and home not in have:
						replacement.append(home)
				for include in replacement:
					have |= self.closure(include) | {include}
				wanted = [{file} for file in needs.get(path, []) if file not in deleted]
				tree, _ = self.expanded_includes(resolved)
				replacement += [include for include in self.cover(tree, wanted, have) if include not in replacement]
				for include in replacement:
					have |= self.closure(include) | {include}
				lines = [f'#include "{spelled(include)}"\n' for include in replacement]
				code = set(IDENT.findall(blank_code("".join(edits.lines))))
				lines += [declaration for name, declaration in sorted(self.forwards(resolved).items())
						  if name in code and defined_in.get(name) not in have]
				given = [line for replaced in edits.replace.values() for line in replaced]
				lines = [line for line in lines if not contains(edits.lines, line) and line not in edits.includes
						 and line not in given]
				edits.replace[i] = lines
				self.graph[path] = [found for found in self.graph.get(path, []) if found != resolved] + replacement
				self.closures.clear()
				self.include_rows.append({"file": path, "removed": spelled(resolved),
										  "added": " ".join(line.strip() for line in lines)})

		# every other file naming a moved entity sees its home header
		for path in sorted(uses_by_file):
			if path in deleted or not os.path.exists(os.path.join(self.root, path)):
				continue
			for usr in sorted(uses_by_file[path]):
				home = homes_of.get(usr)
				if not home or home == path or not is_header(home) or home in self.closure(path):
					continue
				if is_header(path) and path in self.closure(home):
					self.include_rows.append({"file": path, "removed": "", "added": f"(cycle, not added) {home}"})
					continue
				edits = self.file(path)
				line = f'#include "{spelled(home)}"\n'
				if line not in edits.includes:
					edits.includes.append(line)
					self.add_edge(path, home)
					self.include_rows.append({"file": path, "removed": "", "added": line.strip()})

	def write(self) -> None:
		for path, edits in sorted(self.edits.items()):
			full = os.path.join(self.root, path)
			if path in self.deleted:
				if os.path.exists(full):
					os.remove(full)
				continue
			if not edits.changed():
				continue
			text = edits.text()
			os.makedirs(os.path.dirname(full), exist_ok=True)
			with open(full, "w", encoding="utf-8") as sink:
				sink.write(text.rstrip("\n") + "\n" if text else "")


def update_addresses(root: str, path: str, moved: list[dict], deleted: list[str]) -> list[dict]:
	"""The address table's file and line of each moved entity (its rows name where the entity's record was): the
	new file, and the line of the entity's first line there. Returns the rows still naming a deleted file."""
	with open(path, encoding="utf-8", newline="") as source:
		reader = csv.DictReader(source, delimiter="\t")
		columns = reader.fieldnames
		rows = list(reader)
	by_place = collections.defaultdict(list)
	for row in moved:
		if row.get("to"):
			by_place[row["file"]].append(row)
	texts: dict[str, list[str]] = {}
	for row in rows:
		for candidate in by_place.get(row["file"], []):
			if row["name"] != candidate["name"] and not row["name"].startswith(candidate["name"] + "("):
				continue
			target = candidate["to"]
			if target not in texts:
				with open(os.path.join(root, target), encoding="utf-8") as source:
					texts[target] = [line.strip() for line in source]
			row["file"] = target
			lines = texts[target]
			row["line"] = str(lines.index(candidate["line_text"]) + 1) if candidate["line_text"] in lines else ""
			break
	with open(path, "w", encoding="utf-8", newline="") as sink:
		writer = csv.DictWriter(sink, columns, delimiter="\t", lineterminator="\n")
		writer.writeheader()
		writer.writerows(rows)
	gone = set(deleted)
	return [row for row in rows if row["file"] in gone]


def apply(root: str, out: str, table: str, dry_run: bool, addresses: str | None, homes_table: str) -> None:
	model = load_model(out)
	for path, expected in model["digests"].items():
		full = os.path.join(root, path)
		if not os.path.exists(full) or digest(full) != expected:
			fail(f"{path} changed since the model was made: run measure again")
	rows = read_tsv(table)
	if os.path.exists(homes_table):
		chosen: dict[tuple, list[dict]] = {}
		for row in read_tsv(homes_table):
			chosen.setdefault((row["name"], row["kind"]), []).append(row)
		used: dict[int, list[dict]] = {}
		for row in rows:
			for home in chosen.get((row["name"], row["kind"]), ()):
				if home.get("usr") in (None, "", row["usr"]):
					row["chosen"] = home["home"]
					used.setdefault(id(home), []).append(row)
		ambiguous = [f"{rows_[0]['name']} ({rows_[0]['kind']})" for rows_ in used.values() if len(rows_) > 1]
		if ambiguous:
			fail("homes rows matching several entities (give them a usr column): " + ", ".join(sorted(ambiguous)))
		unused = [home for homes_ in chosen.values() for home in homes_ if id(home) not in used]
		write_tsv(os.path.join(out, "homes-unused.tsv"), ("name", "kind", "home", "evidence", "usr"), unused)
		print(f"unplaced: {len(used)} of {len(used) + len(unused)} rows of {homes_table} matched; the rest in "
			  f"homes-unused.tsv", file=sys.stderr)
	homes = Homes(root, model, pdb_sources(root, out))
	# a row that cannot move keeps its entity in its Unplaced file, and a row whose home header would include itself
	# goes to another header: the texts moved before either was found name them where they were planned. Run
	# again, with every such row known from the start, until no new one turns up
	stays: dict[tuple, str] = {}
	rehomed: dict[tuple, str] = {}
	for _ in range(10):
		mover = Mover(root, model, rows, homes, stays, rehomed)
		mover.run()
		found = {(row["file"], int(row["start"]), row["usr"]): row["why"] for row in mover.left}
		if found.keys() <= stays.keys() and mover.rehomed.items() <= rehomed.items():
			break
		stays.update(found)
		rehomed.update(mover.rehomed)
	else:
		fail("the moves do not settle after 10 plans (rows left or rehomed keep changing)")
	columns = list(COLUMNS) + ["to", "placement", "includes"]
	write_tsv(os.path.join(out, "moved.tsv"), columns, mover.moved)
	write_tsv(os.path.join(out, "left.tsv"), list(COLUMNS) + ["why"], mover.left)
	write_tsv(os.path.join(out, "internal.tsv"), ("name", "kind", "file"), mover.internal)
	write_tsv(os.path.join(out, "includes.tsv"), ("file", "removed", "added"), mover.include_rows)
	write_tsv(os.path.join(out, "dropped-comments.tsv"), ("file", "line", "text"), mover.dropped_comments)
	with open(os.path.join(out, "deleted.list"), "w", encoding="utf-8") as sink:
		sink.writelines(path + "\n" for path in mover.deleted)
	print(f"unplaced: {len(mover.moved)} rows moved, {len(mover.left)} left, {len(mover.internal)} file-local, "
		  f"{len(mover.deleted)} of {len(model['unplaced'])} Unplaced files deleted", file=sys.stderr)
	if dry_run:
		return
	mover.write()
	if addresses and os.path.exists(addresses):
		stale = update_addresses(root, addresses, mover.moved, mover.deleted)
		write_tsv(os.path.join(out, "addresses-stale.tsv"), ("address", "kind", "name", "file", "line"), stale)
		print(f"unplaced: {addresses} updated; {len(stale)} rows still name a deleted file "
			  f"(addresses-stale.tsv)", file=sys.stderr)


def write_tsv(path: str, columns, rows) -> None:
	with open(path, "w", encoding="utf-8", newline="") as sink:
		writer = csv.DictWriter(sink, columns, delimiter="\t", lineterminator="\n", extrasaction="ignore")
		writer.writeheader()
		writer.writerows(rows)


def read_tsv(path: str) -> list[dict]:
	with open(path, encoding="utf-8", newline="") as source:
		return list(csv.DictReader(source, delimiter="\t"))


def row_key(row: dict) -> tuple:
	return (row["file"], row["kind"], row["name"], row["usr"])


def write_tables(rows: list[dict], model: dict, out: str) -> None:
	"""entities.tsv (keeping the `chosen` a person filled in a previous one), becomes-member.tsv, open.tsv and
	report.txt."""
	table = os.path.join(out, "entities.tsv")
	if os.path.exists(table):
		chosen = {row_key(row): row["chosen"] for row in read_tsv(table) if row.get("chosen")}
		for row in rows:
			row["chosen"] = chosen.get(row_key(row), "")
	write_tsv(table, COLUMNS, rows)
	first = {}
	for record in model["entities"]:
		if record.get("first"):
			first[record["usr"]] = record["first"]
	members, seen = [], set()
	for row in rows:
		if row["rule"] == "d" and row["usr"] not in seen:
			seen.add(row["usr"])
			owner = first.get(row["usr"], {})
			members.append({"name": row["name"], "class": owner.get("name", ""), "home_cpp": row["home_cpp"],
							"home_h": row["home_h"], "file": row["file"]})
	write_tsv(os.path.join(out, "becomes-member.tsv"), ("name", "class", "home_cpp", "home_h", "file"), members)
	write_tsv(os.path.join(out, "open.tsv"), COLUMNS, [row for row in rows if row["rule"] == "e"])
	lines = [f"Unplaced files: {len(model['unplaced'])}; translation units parsed: {len(model['units'])}"
			 f" ({len(model['failed'])} with errors)", "", "rows by kind and rule:"]
	counts = collections.Counter((row["kind"], row["rule"]) for row in rows)
	rules = sorted({rule for _, rule in counts})
	kinds = sorted({kind for kind, _ in counts})
	lines.append("  " + "kind".ljust(22) + "".join(rule.rjust(9) for rule in rules) + "    total")
	for kind in kinds:
		lines.append("  " + kind.ljust(22) + "".join(str(counts[(kind, rule)]).rjust(9) for rule in rules)
					 + str(sum(counts[(kind, rule)] for rule in rules)).rjust(9))
	lines.append(f"  internal (file-local after the move): {sum(1 for row in rows if row['internal'])}")
	lines += ["", "open rows by folder:"]
	folders = collections.Counter(os.path.dirname(row["file"]) for row in rows if row["rule"] == "e")
	lines += [f"  {count:5}  {folder}" for folder, count in sorted(folders.items())]
	with open(os.path.join(out, "report.txt"), "w", encoding="utf-8") as sink:
		sink.write("\n".join(lines) + "\n")


def main() -> int:
	parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
	parser.add_argument("command", choices=("measure", "decide", "apply"))
	parser.add_argument("--root", default=ROOT)
	parser.add_argument("--build", help="the build folder whose compile commands are parsed (default ROOT/build/linux)")
	parser.add_argument("--out", default=os.path.join(ROOT, "env", "scratch", "Clean", "unplaced"))
	parser.add_argument("--jobs", type=int, default=8)
	parser.add_argument("--table", help="apply: the table of homes (default OUT/entities.tsv)")
	parser.add_argument("--dry-run", action="store_true")
	parser.add_argument("--addresses", help="apply: the address table whose files follow the moves (default "
						"ROOT/harness/addresses.tsv when it exists)")
	parser.add_argument("--homes", default=os.path.join(os.path.dirname(os.path.abspath(__file__)),
														 "unplaced-homes.tsv"),
						help="apply: the homes table that fills `chosen` (default unplaced-homes.tsv beside this script)")
	arguments = parser.parse_args()
	root = os.path.realpath(arguments.root)
	out = os.path.realpath(arguments.out)
	if arguments.command == "measure":
		model = measure(root, os.path.realpath(arguments.build or os.path.join(root, "build", "linux")), arguments.jobs)
		save_model(model, out)
		print(f"unplaced: {len(model['entities'])} entities, {len(model['failed'])} units with errors", file=sys.stderr)
	if arguments.command in ("measure", "decide"):
		model = load_model(out)
		rows = decide(model, Homes(root, model, pdb_sources(root, out)))
		write_tables(rows, model, out)
		with open(os.path.join(out, "report.txt"), encoding="utf-8") as report:
			sys.stdout.write(report.read())
	if arguments.command == "apply":
		apply(root, out, arguments.table or os.path.join(out, "entities.tsv"), arguments.dry_run,
			  arguments.addresses or os.path.join(root, "harness", "addresses.tsv"), arguments.homes)
	return 0


if __name__ == "__main__":
	sys.exit(main())
