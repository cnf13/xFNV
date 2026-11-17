#!/usr/bin/env python3
"""Renames by table over src/ and gen/ (and harness/addresses.tsv when it exists): the names step of the clean-up.

Usage: env/venv/bin/python harness/tools/rename.py TABLE [--root DIR] [--compile-commands FILE]... [--jobs N]
                                                   [--srcdb BIN] [--dry-run] [--report FILE]

TABLE is a TSV with a header holding at least the columns `kind`, `old`, `new` and `action`; only the rows whose
action is `rename` are applied (the others: open, move, merge, special, conflict... are the tables' notes for a
person). Two kinds of rows:

  address names  `old` is an exe-address name, `sub_XXXXXX` (sub.tsv) or `data_XXXXXXX` (a global, data.tsv):
                 unique in the tree, so every identifier and comment word that embeds it is rewritten as text:
                 `sub_8A1234`, `_sub_8A1234`, `nullsub_8A1234` become NEW; `sub_8A1234_original(_t)`
                 NEW_original(_t); `Owner_sub_8A1234` Owner_NEW. A global's `data_11A7640`, `_data_11A7640` become
                 NEW, `data_11A7640_t` NEW_t, `data_11A7640Array` NEWArray, `Owner_data_11A7640` Owner_NEW;
                 `rdata_11A7640` and a longer address (`data_11A76400`) are other names and stay.
  declarations   `old` is a declaration's key (mismatch.tsv): its qualified name without template arguments, a
                 parameter's its function's and `#INDEX`, an enumerator's its enum's and `::NAME`; `kind` is function,
                 field, var, type, enum, enumerator or param; `new` the new last name. These names are not unique
                 (`m_uiFlags` is a member of many classes), so the compiler finds their spellings: `srcdb rename-refs`
                 parses every TU of the given builds' compile commands (and one TU of every src header) and lists each
                 spelling of each declaration, macro arguments included. A virtual method's whole override family is
                 renamed with it (rows are added for the family, and a family whose rows disagree is refused). A name
                 a template spells dependently is renamed where an instantiation resolves it to a renamed declaration;
                 a spelling none resolves is reported and kept, as are names a macro's body spells.

Every edit is checked against the text it replaces. After the declarations' edits, the same parse runs over the
renamed tree by the new names: an edited spelling that no longer names its declaration (a local or member of the new
name shadows it, overload resolution picks another) or a spelling of the new name that names it without having been
edited (the rename captures an existing name) is reported, as is every parse error in src/ or gen/ the first parse
did not meet (a new name already declared in that scope). The report (stdout, or --report) lists the counts per
row, the refused rows, and what is left for a person. --compile-commands defaults to build/hybrid and build/linux
under the root (the two builds' preprocessor branches differ: FNVD_HYBRID, FNVD_64BIT, _WIN32); build them first so
the generated files exist. The srcdb binary (--srcdb) defaults to build/srcdb/srcdb of the checkout this script is in
(harness/srcdb), so a table can be applied to another tree (--root) by this checkout's tools.
"""
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
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
ADDRESS_NAME = re.compile(r"^(sub_[0-9A-Fa-f]{6}|data_[0-9A-Fa-f]{5,8})$")
AST_KINDS = {"function", "field", "var", "type", "enum", "enumerator", "param"}
EXTENSIONS = (".h", ".hpp", ".inl", ".cpp", ".c", ".inc")


def tree_files(root):
	for top in ("src", "gen"):
		for base, _, names in os.walk(os.path.join(root, top)):
			for name in names:
				if name.endswith(EXTENSIONS):
					yield os.path.join(base, name)
	addresses = os.path.join(root, "harness", "addresses.tsv")
	if os.path.exists(addresses):
		yield addresses


def read_table(path):
	with open(path, encoding="utf-8", newline="") as source:
		rows = list(csv.DictReader(source, delimiter="\t"))
	for column in ("kind", "old", "new", "action"):
		if rows and column not in rows[0]:
			sys.exit(f"rename: {path} has no column {column}")
	return [row for row in rows if row["action"] == "rename" and row["new"]]


def leaf(key):
	base = key.split("#", 1)[0]
	return base.rsplit("::", 1)[-1]


def renamed_key(kind, key, new):
	"""A declaration's key after its rename (a parameter's key holds no name, so it stays)."""
	if kind == "param":
		return key
	return key.rsplit("::", 1)[0] + "::" + new if "::" in key else new


# --- address names: text ------------------------------------------------------------------------------------------

def rename_addresses(root, rows, dry, report):
	mappings = {"sub": {}, "data": {}}  # family -> address -> new name
	for row in rows:
		family, address = row["old"].split("_", 1)
		mapping = mappings[family]
		address = address.upper()
		if address in mapping and mapping[address] != row["new"]:
			report.append(f"refused {row['old']}: two new names")
			continue
		mapping[address] = row["new"]
	counts = collections.Counter()
	passes = []
	if mappings["sub"]:
		mapping = mappings["sub"]
		pattern = re.compile(r"\b(\w*?)(nullsub_|_?sub_)([0-9A-Fa-f]{6})(?![0-9A-Fa-f])(\w*)")

		def replace_sub(match):
			prefix, marker, address, suffix = match.groups()
			new = mapping.get(address.upper())
			if not new:
				return match.group(0)
			counts["sub_" + address.upper()] += 1
			joiner = "_" if prefix and marker.startswith("_") else ""
			return prefix + joiner + new + suffix

		passes.append(("sub_", pattern, replace_sub))
	if mappings["data"]:
		data = mappings["data"]
		# the mapped addresses only, longest first, so that a suffix starting with a hex letter (`Array`) stays a
		# suffix; an address that runs on in hex digits up to a non-letter is a longer address, not this one
		addresses = "|".join(sorted(data, key=len, reverse=True))
		pattern = re.compile(r"\b(?:(\w+?)_|_)?data_(" + addresses + r")(?![0-9A-Fa-f]+(?![A-Za-z]))(\w*)")

		def replace_data(match):
			owner, address, suffix = match.groups()
			counts["data_" + address.upper()] += 1
			return (owner + "_" if owner else "") + data[address.upper()] + suffix

		passes.append(("data_", pattern, replace_data))
	if not passes:
		return counts

	for path in tree_files(root):
		with open(path, encoding="utf-8", errors="surrogateescape", newline="") as source:
			text = source.read()
		changed = text
		for marker, pattern, replace in passes:
			if marker in changed:
				changed = pattern.sub(replace, changed)
		if changed != text and not dry:
			with open(path, "w", encoding="utf-8", errors="surrogateescape", newline="") as out:
				out.write(changed)
	for family, mapping in mappings.items():
		for address in sorted(set(mapping) - {key.split("_", 1)[1] for key in counts if key.startswith(family)}):
			report.append(f"unused {family}_{address}: no spelling in the tree")
	return counts


# --- declarations: the compiler's spellings ---------------------------------------------------------------------

def tu_commands(root, compile_commands):
	"""(file, directory, command words) of every src TU in the compile databases, and one all-headers TU."""
	tus = []
	src = os.path.join(root, "src") + os.sep
	first = None
	for path in compile_commands:
		with open(path, encoding="utf-8") as source:
			entries = json.load(source)
		for entry in entries:
			file = os.path.realpath(os.path.join(entry["directory"], entry["file"]))
			if not file.startswith(src):
				continue
			words = entry["arguments"] if "arguments" in entry else shlex.split(entry["command"])
			tus.append((file, entry["directory"], words))
			if first is None and "clang-cl" in words[0]:
				first = (file, entry["directory"], words)
	if first:
		headers = sorted(os.path.relpath(p, os.path.join(root, "src")) for p in tree_files(root)
						 if p.startswith(src) and p.endswith((".h", ".hpp")))
		scratch = tempfile.mkdtemp(prefix="rename-", dir=os.path.join(root, "build"))
		allheaders = os.path.join(scratch, "allheaders.cpp")
		with open(allheaders, "w", encoding="utf-8") as out:
			out.writelines(f'#include "{header}"\n' for header in headers)
		words = [allheaders if w == first[0] or os.path.realpath(os.path.join(first[1], w)) == first[0] else w
				 for w in first[2]]
		tus.append((allheaders, first[1], words))
	return tus


def run_refs(root, srcdb, names, tus, jobs):
	"""Every output line of srcdb rename-refs over the TUs, and the TUs that failed."""
	lines, failed = set(), []

	def one(tu):
		file, directory, words = tu
		command = [srcdb, "rename-refs", "--root", root, "--names", names, "--file", file, "--directory",
				   directory, "--"] + words
		result = subprocess.run(command, capture_output=True, text=True, errors="surrogateescape")
		return file, result.returncode, result.stdout

	with concurrent.futures.ThreadPoolExecutor(jobs) as pool:
		for file, code, out in pool.map(one, tus):
			if code not in (0, 3):
				failed.append(file)
			lines.update(out.splitlines())
	return lines, failed


def rename_declarations(root, rows, compile_commands, jobs, dry, report, srcdb):
	if not os.path.exists(srcdb):
		sys.exit(f"rename: build {srcdb} first (harness/srcdb)")
	new_of = {}
	for row in rows:
		key = (row["kind"], row["old"])
		if new_of.get(key, row["new"]) != row["new"]:
			report.append(f"refused {row['kind']} {row['old']}: two new names")
			new_of[key] = None
			continue
		new_of[key] = row["new"]
	new_of = {k: v for k, v in new_of.items() if v}
	tus = tu_commands(root, compile_commands)
	report.append(f"TUs: {len(tus)}")
	with tempfile.NamedTemporaryFile("w", suffix=".names", delete=False, dir=os.path.join(root, "build")) as names:
		names.writelines(f"{kind}\t{key}\n" for kind, key in new_of)
	lines, failed = run_refs(root, srcdb, names.name, tus, jobs)
	# override families: every method of a family takes the family's one new name
	families = collections.defaultdict(set)
	for line in lines:
		part = line.split("\t")
		if part[0] == "fam":
			families[part[1]].add(part[2])
			families[part[2]].add(part[1])
	added = {}
	for kind, key in list(new_of):
		if kind != "function" or key not in families:
			continue
		seen, todo = {key}, [key]
		while todo:
			for other in families[todo.pop()]:
				if other not in seen:
					seen.add(other)
					todo.append(other)
		news = {new_of.get(("function", member)) for member in seen} - {None}
		if len(news) > 1:
			report.append(f"refused family of {key}: rows disagree ({', '.join(sorted(news))})")
			for member in seen:
				new_of.pop(("function", member), None)
			continue
		for member in seen:
			if ("function", member) not in new_of:
				added[("function", member)] = next(iter(news))
	if added:
		report.append(f"override family rows added: {len(added)}")
		for (kind, key), new in sorted(added.items()):
			report.append(f"  added {key} -> {new}")
		new_of.update(added)
		with open(names.name, "w", encoding="utf-8") as out:
			out.writelines(f"{kind}\t{key}\n" for kind, key in new_of)
		lines, failed2 = run_refs(root, srcdb, names.name, tus, jobs)
		failed += failed2
	os.unlink(names.name)
	for file in sorted(set(failed)):
		report.append(f"TU failed to run: {os.path.relpath(file, root)}")
	edits = collections.defaultdict(dict)  # path -> offset -> (new, (kind, key))
	counts = collections.Counter()
	unresolved = collections.Counter()
	dependent = []
	for line in sorted(lines):
		part = line.split("\t")
		if part[0] == "occ":
			_, kind, key, path, offset = part
			new = new_of.get((kind, key))
			if new is None:
				continue
			edits[path][int(offset)] = (new, (kind, key))
		elif part[0] == "dep":
			dependent.append(part[1:])
	# a name a template spells dependently is renamed only through an instantiation that resolves it to a renamed
	# declaration (an `occ` at the same place); the leaf alone does not say which declaration it names (a template's
	# own `GetAt` is not the one renamed row's `GetAt`)
	for name, path, offset in dependent:
		if int(offset) not in edits.get(path, {}):
			unresolved[f"dependent {name} not renamed (no instantiation resolves it): {path}+{offset}"] += 1
	for line in sorted(lines):
		part = line.split("\t")
		if part[0] == "occ" or part[0] == "dep":
			continue
		elif part[0] == "macro":
			_, kind, key, path, line_number = part
			if (kind, key) in new_of:
				unresolved[f"macro body spells {key} (expanded at {path}:{line_number})"] += 1
	expected = set()  # (new key, path, offset after the edits): every edited spot must name its declaration still
	for path, spots in sorted(edits.items()):
		full = os.path.join(root, path)
		with open(full, "rb") as source:
			data = source.read()
		applied = []
		for offset in sorted(spots, reverse=True):
			new, (kind, key) = spots[offset]
			# a parameter's old name is the text there (its key holds no name)
			match = re.match(rb"[A-Za-z_]\w*", data[offset:offset + 256])
			old = match.group(0).decode() if match else ""
			if kind != "param" and old != leaf(key):
				unresolved[f"text at {path}+{offset} is {old!r}, not {leaf(key)}"] += 1
				continue
			data = data[:offset] + new.encode() + data[offset + len(old):]
			counts[(kind, key)] += 1
			applied.append((offset, len(new) - len(old), kind, renamed_key(kind, key, new)))
		shift = 0
		for offset, delta, kind, key in sorted(applied):
			expected.add((kind, key, path, offset + shift))
			shift += delta
		if not dry:
			with open(full, "wb") as out:
				out.write(data)
	for (kind, key) in sorted(set(new_of) - set(counts)):
		report.append(f"unused {kind} {key}: no spelling found")
	if not dry and expected:
		# the same parse over the renamed tree, by the new keys: an edited spot that no longer names its declaration
		# (a local or a member of the new name shadows it, overload resolution picks another) and a spot that names
		# it now without having been edited (an existing name the rename captures) are both changes of meaning
		with tempfile.NamedTemporaryFile("w", suffix=".names", delete=False, dir=os.path.join(root, "build")) as names:
			names.writelines(f"{kind}\t{key}\n" for kind, key in sorted({(k, key) for k, key, _, _ in expected}))
		after, _ = run_refs(root, srcdb, names.name, tus, jobs)
		os.unlink(names.name)
		found = set()
		for line in after:
			part = line.split("\t")
			if part[0] == "occ":
				found.add((part[1], part[2], part[3], int(part[4])))
		for kind, key, path, offset in sorted(expected - found):
			unresolved[f"after the rename, {path}+{offset} no longer names {kind} {key}"] += 1
		for kind, key, path, offset in sorted(found - expected):
			unresolved[f"after the rename, {path}+{offset} names {kind} {key} but was not renamed (captured)"] += 1
		# errors the renamed tree's parse meets that the first parse did not: a new name already declared where the
		# old one was (a local or another parameter of that name), a name a template's other instance needs
		before = {line for line in lines if line.startswith("err\t")}
		brought = sorted(line for line in after if line.startswith("err\t") and line not in before)
		for line in brought:
			_, path, number, text = line.split("\t", 3)
			unresolved[f"after the rename, error at {path}:{number}: {text}"] += 1
		report.append(f"verified: {len(expected & found)} of {len(expected)} renamed spellings name their "
					  f"declaration after the rename; {len(brought)} errors the rename brings")
	for text, n in sorted(unresolved.items()):
		report.append(f"left for a person: {text}" + (f" (x{n})" if n > 1 else ""))
	addresses = os.path.join(root, "harness", "addresses.tsv")
	if os.path.exists(addresses) and not dry:
		with open(addresses, encoding="utf-8") as source:
			text = source.read()
		for (kind, key), new in new_of.items():
			if kind == "function":
				qualified_new = key.rsplit("::", 1)[0] + "::" + new if "::" in key else new
				text = re.sub(r"(?<![\w:])" + re.escape(key) + r"(?!\w)", qualified_new, text)
		with open(addresses, "w", encoding="utf-8") as out:
			out.write(text)
	return counts


def main():
	parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
	parser.add_argument("table")
	parser.add_argument("--root", default=ROOT)
	parser.add_argument("--compile-commands", action="append")
	parser.add_argument("--jobs", type=int, default=8)
	parser.add_argument("--srcdb", default=os.path.join(ROOT, "build", "srcdb", "srcdb"),
						help="the srcdb binary (default: this checkout's build/srcdb/srcdb)")
	parser.add_argument("--dry-run", action="store_true")
	parser.add_argument("--report")
	arguments = parser.parse_args()
	root = os.path.realpath(arguments.root)
	rows = read_table(arguments.table)
	text_rows = [row for row in rows if ADDRESS_NAME.match(row["old"])]
	ast_rows = [row for row in rows if not ADDRESS_NAME.match(row["old"])]
	bad = [row for row in ast_rows if row["kind"] not in AST_KINDS]
	if bad:
		sys.exit(f"rename: {len(bad)} rows of an unknown kind, e.g. {bad[0]['kind']} {bad[0]['old']}")
	for row in rows:
		if not re.fullmatch(r"[A-Za-z_]\w*", row["new"]):
			sys.exit(f"rename: new name {row['new']!r} of {row['old']} is no identifier")
	report = []
	counts = rename_addresses(root, text_rows, arguments.dry_run, report)
	report.insert(0, f"address names: {len(counts)} of {len(text_rows)} rows, {sum(counts.values())} spellings")
	if ast_rows:
		commands = arguments.compile_commands or [os.path.join(root, "build", b, "compile_commands.json")
												  for b in ("hybrid", "linux")]
		counts = rename_declarations(root, ast_rows, commands, arguments.jobs, arguments.dry_run, report,
									 arguments.srcdb)
		report.insert(1, f"declarations: {len(counts)} of {len(ast_rows)} rows, {sum(counts.values())} spellings")
	text = "\n".join(report) + "\n"
	if arguments.report:
		with open(arguments.report, "w", encoding="utf-8") as out:
			out.write(text)
	print(text, end="")


if __name__ == "__main__":
	main()
