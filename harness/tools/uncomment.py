#!/usr/bin/env python3
"""Delete the comments that exist only for the recovery from the exe (docs/PLAN.md Part 2, step 2).

Usage: env/venv/bin/python harness/tools/uncomment.py [--dry-run] [--after-deinline] [--no-strip]
                                                       [--addresses TSV] [--out DIR] [--root DIR] [PATH...]
  PATH              files or folders to rewrite, under --root (default src); the tables cover what was given
  --root DIR        the tree (default the current directory)
  --dry-run         change no file; write the tables only (the measure)
  --after-deinline  also delete the SWEEP-INLINE and EXE-INLINED markers (PLAN step 7 reads them: run this only
                    after it)
  --no-strip        never shorten a comment: one that would lose only its leading marker sentences or an address
                    parenthetical goes to mixed.tsv instead
  --addresses TSV   the exe's address table (default harness/addresses.tsv): its lowest and highest address bound
                    what counts as an address in a comment (`0x` and 6 to 8 hex digits)
  --out DIR         the tables (default env/scratch/Clean/uncomment)

How it reads the source. A lexer finds the comments as the compiler does: string and character literals (with
their prefixes), raw strings, pp-numbers with digit separators, `#include <...>` names and backslash-newline
splices (a `//` comment ending in a backslash goes on to the next line). A comment's unit is:
  - a run of `//` lines with nothing else on them (a paragraph), cut before every line that starts with a marker
    and after a marker's sentence when it ends a line (so a marker line and the description under it are two
    units; a helper's name after `EXE-INLINED none:` goes on over a line that starts in lower case or with `>`);
  - a `//` comment after code on its line, a block comment, a `//` comment continued by a backslash: each alone.

Classes (the first that applies), and what happens to the unit:
  licence, pragma (NOLINT, clang-format on/off, IWYU...)   kept
  review marker                  the emitter's `/* review: ... */` notes: deleted
  SWEEP-INLINE, EXE-INLINED      kept (gated) without --after-deinline; with it as the markers below
  EXE-CALLED, EXE-CAST, Same function as, Not reconstructed yet, IDA name(s), Not named in the PDB
                                 the marker sentences at the unit's start (exact forms, MARKERS; several in a row
                                 all) come off, with the address parentheticals of what follows: nothing left (or
                                 only addresses) deletes the unit; a sentence left that names nothing of the
                                 recovery stays alone (stripped); else the whole unit is mixed. A marker that is not
                                 of its exact form: mixed
  PDB:, Guessed type:            a declaration's evidence note: the unit is deleted
  KEEP:                          a body's reason to differ, read by PLAN step 8: mixed
  address only                   nothing but addresses, calling conventions and `ret N`: deleted
  seam record                    a commented-out FNVD_ macro statement: deleted
  exe, IDA, PDB, address, hybrid, checker, checkbody, emitter, reconstructed, at parity, debug build, decomp, seam
  (FNVD_, fnvd::), section (`(.rdata)`), registers (eax, [esp, ret N, thiscall...), marker inside
                                 when the only recovery in it is in parentheticals of addresses, calling conventions
                                 and sections (`Clears the list (thiscall, ret 4, 0x8A1234).`), those go (stripped);
                                 else mixed: left as it is, listed for PLAN step 9
  code                           commented-out code naming nothing of the recovery: kept
  rest                           kept
A unit's whole lines go when it is deleted; a comment after code goes with the spaces before it; a block comment
inside a line leaves one space where it separated two tokens. A stripped line keeps its comment opening and its
other words; punctuation a removal leaves at a line's start joins the line before. A blank line the deletion
leaves double, or right after an opening brace or the file's start, or right before a closing brace, goes too. A
comment in a preprocessor directive line that continues (a backslash) is never edited: listed as `directive`.

The check. Before a file is written, its code with every comment as one space and the whitespace that cannot
separate tokens dropped must be the same line for line (blank lines aside) before and after; a file failing it is
left as it is, named, and the run exits 1. The dry run makes the same check.

Tables (in --out, rewritten by each run):
  measure.txt   per class and action: the count and lines, the units naming each recovery phrase, then ten
                samples per class and action (file:line and the unit's words)
  sites.tsv     every unit: file, line, last line, class, action (deleted, stripped, kept, mixed, gated), words
  mixed.tsv     the mixed units: file, line, class, words (the input of PLAN step 9)
  edits.tsv     every change: file, line, action, old words, new words

Idempotent: a second run finds nothing to delete or strip (a stripped unit's sentence classifies as rest).
"""

import argparse
import collections
import csv
import os
import random
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
EXTENSIONS = (".h", ".hpp", ".inl", ".cpp", ".c", ".inc")

# ---------------------------------------------------------------------------------------------------------------
# The lexer: the comments of a text, as (start, end, kind, directive) with kind "line" or "block" and directive
# 0 outside a directive, 1 in a directive line that ends there, 2 in one continued by a backslash.

IDENT = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")
PPNUMBER = re.compile(r"\.?[0-9](?:[eEpP][+-]|'[0-9A-Za-z_]|[0-9A-Za-z_.])*")
INCLUDE = re.compile(r"[ \t]*(?:include|include_next|import)[ \t]*<[^>\n]*>")
STRING_PREFIXES = {"L", "u", "U", "u8"}
RAW_PREFIXES = {"R", "LR", "uR", "UR", "u8R"}


def _splice_len(text, i):
	"""The length of a backslash-newline at i (0 when there is none)."""
	if text.startswith("\\\n", i):
		return 2
	if text.startswith("\\\r\n", i):
		return 3
	return 0


def _skip_quoted(text, i, quote):
	n = len(text)
	i += 1
	while i < n:
		c = text[i]
		if c == "\\":
			i += 2
			continue
		if c == quote:
			return i + 1
		if c == "\n":
			return i
		i += 1
	return n


def _skip_raw(text, i):
	"""i is at the quote of R"delim( ... )delim"."""
	paren = text.find("(", i + 1)
	if paren < 0 or paren - i > 17:
		return i + 1
	close = text.find(")" + text[i + 1:paren] + '"', paren)
	return len(text) if close < 0 else close + paren - i + 1


def lex_comments(text):
	out = []
	i, n = 0, len(text)
	line_start, directive, directive_comments = True, False, []

	def end_directive(continued):
		for c in directive_comments:
			c[3] = 2 if continued else 1
		directive_comments.clear()

	while i < n:
		c = text[i]
		splice = _splice_len(text, i)
		if splice:
			if directive:
				# The directive goes on: comments already in it sit in a continued directive.
				for d in directive_comments:
					d[3] = 2
			i += splice
			continue
		if c == "\n":
			if directive:
				end_directive(any(d[3] == 2 for d in directive_comments))
			directive, line_start = False, True
			i += 1
			continue
		if c in " \t\r\f\v":
			i += 1
			continue
		if c == "#" and line_start:
			directive, line_start = True, False
			m = INCLUDE.match(text, i + 1)
			i = m.end() if m else i + 1
			continue
		line_start = False
		if c == "/" and text.startswith("//", i):
			j = i + 2
			while True:
				nl = text.find("\n", j)
				if nl < 0:
					j = n
					break
				before = nl - 1 if nl > 0 and text[nl - 1] == "\r" else nl
				if before > 0 and text[before - 1] == "\\":
					j = nl + 1
					continue
				j = before
				break
			item = [i, j, "line", 0]
			out.append(item)
			if directive:
				directive_comments.append(item)
			i = j
			continue
		if c == "/" and text.startswith("/*", i):
			j = text.find("*/", i + 2)
			j = n if j < 0 else j + 2
			item = [i, j, "block", 0]
			out.append(item)
			if directive:
				directive_comments.append(item)
			i = j
			continue
		if c == '"' or c == "'":
			i = _skip_quoted(text, i, c)
			continue
		if c.isalpha() or c == "_":
			m = IDENT.match(text, i)
			word, i = m.group(0), m.end()
			if i < n and text[i] == '"' and word in RAW_PREFIXES:
				i = _skip_raw(text, i)
			elif i < n and text[i] in "\"'" and word in STRING_PREFIXES:
				i = _skip_quoted(text, i, text[i])
			continue
		if c.isdigit() or (c == "." and i + 1 < n and text[i + 1].isdigit()):
			i = PPNUMBER.match(text, i).end()
			continue
		i += 1
	if directive:
		end_directive(any(d[3] == 2 for d in directive_comments))
	return out


# ---------------------------------------------------------------------------------------------------------------
# Classes

HEX = re.compile(r"\b0[xX]([0-9A-Fa-f]{6,8})\b")
LOW, HIGH = 0, 0


def is_address(value):
	return LOW <= value <= HIGH


def addresses_in(text):
	return [m for m in HEX.finditer(text) if is_address(int(m.group(1), 16))]


CONVENTION = r"(?:thiscall|cdecl|stdcall|fastcall|usercall|userpurge|__thiscall|__cdecl|__stdcall|__fastcall)"
SECTION = r"\.(?:rdata|data|bss|tls|text)\b"
ABI_ITEM = re.compile(r"(?:" + CONVENTION + r"|ret (?:0x[0-9A-Fa-f]+|\d+)|0[xX][0-9A-Fa-f]{6,8}|" + SECTION + ")")
# A parenthetical of addresses, calling conventions, `ret N` and section names only, with the space before it.
ABI_PAREN = re.compile(r"[ \t]*\((?:" + ABI_ITEM.pattern + r")(?:(?:,\s*|\s+and\s+|\s+)" + ABI_ITEM.pattern + r")*\)")

ADDR = r"0[xX][0-9A-Fa-f]{6,8}"
# The fixed markers: name, and the regex of the marker sentence at a unit's start (None: the unit is a whole note).
# EXE-INLINED's `none: NAME` takes a C++ name with its template arguments (form_end).
MARKERS = [
	("SWEEP-INLINE", re.compile(r"SWEEP-INLINE " + ADDR + r"\.?(?=\s|$)")),
	("EXE-INLINED", re.compile(r"EXE-INLINED (?:" + ADDR + r"\.?(?=\s|$)|none: )")),
	("EXE-CALLED", re.compile(r"EXE-CALLED " + ADDR + r"\.?(?=\s|$)")),
	("EXE-CAST", re.compile(r"EXE-CAST: [^()\n]*?(?= \(|\.(?:\s|$)|$)\.?")),
	("Same function as", re.compile(r"Same function as [^()]+?(?: \([^()]*\))? \(" + ADDR + r"\),? registered there\.")),
	("Not reconstructed yet", re.compile(r"Not reconstructed yet(?:" + ABI_PAREN.pattern + r")?\.")),
	("IDA names", re.compile(r"IDA names (?:it|the entry|this(?: \w+)?) \S+?\.(?=\s|$)")),
	("IDA name", re.compile(r"IDA name; not in the PDB\.")),
	("Not named in the PDB", re.compile(r"Not named in the PDB or IDA(?: for this class)?(?: \([^()]*\))?; names? chosen "
										r"here(?:[^().]|\([^()]*\))*\.")),
	("PDB:", None),
	("Guessed type", None),
	("KEEP:", None),
]
MARKER_START = re.compile(r"(?:SWEEP-INLINE|EXE-INLINED|EXE-CALLED|EXE-CAST|Same function as|Not reconstructed yet|"
						  r"IDA name|Not named in the PDB|PDB:|Guessed type|KEEP:)")
MARKER_ANYWHERE = re.compile(r"\b(?:SWEEP-INLINE|EXE-INLINED|EXE-CALLED|EXE-CAST|Same function as|"
							 r"Not reconstructed yet|registered there|IDA name|Not named in the PDB|Guessed type|KEEP:)")
GATED = {"SWEEP-INLINE", "EXE-INLINED"}
WHOLE_NOTE = {"PDB:", "Guessed type"}

LICENCE = re.compile(r"\bCopyright\s+(?:\([Cc]\)|©|\d{4})|\bSPDX-License-Identifier\b|\bLicensed under\b|"
					 r"\bAll rights reserved\b|\bPermission is hereby granted\b", re.I)
PRAGMA = re.compile(r"^\s*(?:NOLINT|clang-format (?:on|off)|IWYU pragma|NOSONAR|cppcheck-suppress|fallthrough|"
					r"FALLTHROUGH|\$Id|@formatter|-\*-)")
REVIEW = re.compile(r"^review: [^\n]*$")
SEAM_RECORD = re.compile(r"(?:FNVD_\w+\([^;]*\);\s*)+")
CODE = re.compile(r"(?:;\s*$|^\s*[{}]\s*$|^\s*#\s*(?:if|ifdef|ifndef|include|define|endif|else)\b|"
				  r"^\s*(?:if|for|while|switch|return|else|case|break|continue)\b[^.]*[;{(]\s*$|"
				  r"^\s*[A-Za-z_][\w:<>,*& \[\]]*\([^()]*\)\s*(?:const\s*)?[;{]\s*$)")
PHRASES = [
	("exe", re.compile(r"\b[Ee]xe(?:'s)?\b|\.exe\b|\bFalloutNV\b")),
	("IDA", re.compile(r"\bIDA(?:'s)?\b|\bHex-Rays\b")),
	("PDB", re.compile(r"\bPDB(?:'s)?\b")),
	("address", None),
	("hybrid", re.compile(r"\b[Hh]ybrid\b")),
	("checker", re.compile(r"\b[Cc]hecker\b")),
	("checkbody", re.compile(r"\bcheckbody\b")),
	("emitter", re.compile(r"\b[Ee]mit(?:ter|ted)\b")),
	("reconstructed", re.compile(r"\b[Rr]econstruct(?:ed|ion)\b")),
	("at parity", re.compile(r"\b(?:at|until|before|after) parity\b")),
	("debug build", re.compile(r"\b[Dd]ebug build\b")),
	("decomp", re.compile(r"\b[Dd]ecomp\w*")),
	("seam", re.compile(r"\bFNVD_\w+|\bfnvd::")),
	("section", re.compile(r"\(" + SECTION + r"\)")),
	("registers", re.compile(r"\b(?:e[abcd]x|e[sd]i|e[sb]p|[abcd][lh]|ST\(?\d\)?|xmm\d)\b|\[(?:esp|ebp)\b|"
							 r"\bret (?:0x[0-9A-Fa-f]+|\d+)\b|\b(?:__)?(?:thiscall|cdecl|stdcall|fastcall|usercall|"
							 r"userpurge)\b|\bvftable\b")),
	("marker inside", MARKER_ANYWHERE),
]


def marker_at(words, pos):
	return next(n for n, _ in MARKERS if words.startswith(n, pos))


def form_end(name, words, pos):
	"""Where the marker sentence of marker name starting at pos ends, or None when it is not of the fixed form."""
	regex = dict(MARKERS)[name]
	m = regex.match(words, pos) if regex else None
	if m is None or not m.group(0).endswith("none: "):
		return m and m.end()
	# The helper's name and what it says of it ("X<A, B>::f", "NiPointer<T> destructor") up to the end of the
	# sentence: a period outside parentheses before a space or the end.
	depth, k = 0, m.end()
	while k < len(words):
		c = words[k]
		if c in "([":
			depth += 1
		elif c in ")]" and depth:
			depth -= 1
		elif c == "." and depth == 0 and (k + 1 == len(words) or words[k + 1].isspace()):
			k += 1
			break
		k += 1
	return k if k > m.end() else None


def recovery_classes(text):
	hits = []
	for name, regex in PHRASES:
		if name == "address":
			if addresses_in(text):
				hits.append(name)
		elif regex.search(text):
			hits.append(name)
	return hits


def only_abi(text):
	"""Nothing but addresses, calling conventions, `ret N` and punctuation, with at least one address."""
	if not addresses_in(text):
		return False
	rest = ABI_ITEM.sub("", text)
	return not re.search(r"[A-Za-z0-9]", rest)


def abi_paren_spans(text, base=0):
	"""The spans (from base) of text's parentheticals of addresses, calling conventions and sections, with the
	spaces before them."""
	return [(base + m.start(), base + m.end()) for m in ABI_PAREN.finditer(text)
			if addresses_in(m.group(0)) or re.search(CONVENTION + "|" + SECTION, m.group(0))]


def without(text, spans):
	out, pos = [], 0
	for s, e in spans:
		out.append(text[pos:s])
		pos = e
	out.append(text[pos:])
	return "".join(out)


# ---------------------------------------------------------------------------------------------------------------
# Units

class Segment:
	"""One physical line's share of a unit: its text lies at text[start:end] of the file (the comment's words, without
	the comment's opening and closing marks)."""
	__slots__ = ("line", "start", "end")

	def __init__(self, line, start, end):
		self.line, self.start, self.end = line, start, end


class Unit:
	__slots__ = ("kind", "segments", "start", "end", "whole_lines", "directive", "cls", "action", "removals",
				 "words", "pieces", "first_line", "last_line")

	def __init__(self, kind, segments, start, end, whole_lines, directive):
		self.kind, self.segments, self.start, self.end = kind, segments, start, end
		self.whole_lines, self.directive = whole_lines, directive
		self.cls, self.action, self.removals = "rest", "kept", []
		self.first_line, self.last_line = segments[0].line, segments[-1].line

	def layout(self, file_text):
		"""The unit's words: each line's words stripped, joined by one space; pieces maps them back to the file as
		(start in words, start in the file, length)."""
		parts, self.pieces, pos = [], [], 0
		for s in self.segments:
			raw = file_text[s.start:s.end]
			stripped = raw.strip()
			if not stripped:
				continue
			if parts:
				pos += 1
			self.pieces.append((pos, s.start + len(raw) - len(raw.lstrip()), len(stripped)))
			parts.append(stripped)
			pos += len(stripped)
		self.words = " ".join(parts)
		return self.words


LINE_BODY = re.compile(r"//[/!]?[ \t]?")


def line_of(offsets, pos):
	lo, hi = 0, len(offsets) - 1
	while lo < hi:
		mid = (lo + hi + 1) // 2
		if offsets[mid] <= pos:
			lo = mid
		else:
			hi = mid - 1
	return lo


def build_units(text, comments, offsets):
	"""The units of a file, in order."""
	units = []
	paragraph = []  # (comment, line) of own-line single-line `//` comments, consecutive lines

	def own_line(start, end):
		ls = text.rfind("\n", 0, start) + 1
		le = text.find("\n", end)
		le = len(text) if le < 0 else le
		return not text[ls:start].strip() and not text[end:le].strip()

	def flush():
		if paragraph:
			units.extend(split_paragraph(text, paragraph))
			paragraph.clear()

	for start, end, kind, directive in comments:
		first, last = line_of(offsets, start), line_of(offsets, max(start, end - 1))
		whole = own_line(start, end)
		if kind == "line" and first == last and whole and not directive:
			if paragraph and paragraph[-1][1] != first - 1:
				flush()
			body = LINE_BODY.match(text, start).end()
			paragraph.append((Segment(first, body, end), first))
			continue
		flush()
		if kind == "line":
			body = LINE_BODY.match(text, start).end()
			segments = [Segment(first, body, end)]
		else:
			segments = []
			inner_start, inner_end = start + 2, max(start + 2, end - 2)
			pos = inner_start
			for ln in range(first, last + 1):
				seg_end = min(inner_end, offsets[ln + 1] - 1 if ln + 1 < len(offsets) else inner_end)
				seg_start = pos
				m = re.match(r"[ \t]*\*?(?!/)[ \t]?", text[seg_start:seg_end]) if ln != first else None
				if m:
					seg_start += m.end()
				segments.append(Segment(ln, seg_start, seg_end))
				pos = seg_end + 1
		units.append(Unit(kind, segments, start, end, whole, directive))
	flush()
	return units


def split_paragraph(text, lines):
	"""Cut a paragraph before every marker line and after a marker unit whose sentence ends a line."""
	units, current, current_marker = [], [], None

	def close():
		nonlocal current, current_marker
		if current:
			start = current[0].start
			units.append(Unit("line", current, text.rfind("//", 0, start), current[-1].end, True, 0))
		current, current_marker = [], None

	for k, (seg, _) in enumerate(lines):
		words = text[seg.start:seg.end].strip()
		if MARKER_START.match(words):
			close()
			current_marker = True
		current.append(seg)
		if current_marker:
			joined = " ".join(text[s.start:s.end].strip() for s in current)
			complete = any(r is not None and form_end(name, joined, 0) == len(joined) for name, r in MARKERS)
			# A helper's name after `none:` may go on to the next line ("X<A,\n B>::f", "X\n default constructor").
			following = text[lines[k + 1][0].start:lines[k + 1][0].end].strip() if k + 1 < len(lines) else ""
			goes_on = ("none: " in joined and not MARKER_START.match(following)
					   and re.match(r"[a-z>:,(<*&]", following) is not None)
			if joined.endswith(".") or (complete and not goes_on):
				close()
	close()
	return units


# ---------------------------------------------------------------------------------------------------------------
# Decisions


def classify(unit, words, args):
	"""Set unit.cls and unit.action, and for a stripped unit the spans of its words that go (unit.removals)."""
	if LICENCE.search(words):
		unit.cls = "licence"
		return
	if PRAGMA.search(words):
		unit.cls = "pragma"
		return
	if unit.kind == "block" and REVIEW.match(words):
		unit.cls, unit.action = "review marker", "deleted"
		return
	if MARKER_START.match(words):
		name = marker_at(words, 0)
		unit.cls = name
		if name in GATED and not args.after_deinline:
			unit.action = "gated"
			return
		# Marker sentences in a row ("Not reconstructed yet. IDA name; not in the PDB. ...") all come off.
		rest = 0
		while True:
			if name in WHOLE_NOTE:
				unit.action = "deleted"
				return
			end = form_end(name, words, rest)
			if end is None:
				break
			rest = end
			while rest < len(words) and words[rest] in " \t":
				rest += 1
			name = marker_at(words, rest) if MARKER_START.match(words, rest) else None
			if name is None or (name in GATED and not args.after_deinline):
				break
		if rest == 0:
			unit.action = "mixed"
			return
		strip(unit, words, [(0, rest)] + abi_paren_spans(words[rest:], rest), args)
		return
	if only_abi(words):
		unit.cls, unit.action = "address only", "deleted"
		return
	if SEAM_RECORD.fullmatch(words):
		unit.cls, unit.action = "seam record", "deleted"
		return
	hits = recovery_classes(words)
	if not hits:
		if CODE.search(words):
			unit.cls = "code"
		return
	unit.cls = hits[0]
	spans = abi_paren_spans(words)
	if spans:
		strip(unit, words, spans, args)
	else:
		unit.action = "mixed"


def strip(unit, words, spans, args):
	"""The unit loses spans of its words: deleted when nothing but addresses is left, stripped when what is left
	names nothing of the recovery and is a sentence (not a parenthetical left alone), else mixed."""
	left = without(words, spans).strip()
	if not left or only_abi(left):
		unit.action = "deleted"
	elif recovery_classes(left) or left.startswith("(") or args.no_strip or unit.directive == 2:
		unit.action = "mixed"
	else:
		unit.action, unit.removals = "stripped", spans


# ---------------------------------------------------------------------------------------------------------------
# Edits


def unit_edits(text, unit, offsets):
	"""Character edits (start, end, replacement, kind) for a deleted or stripped unit; kind "lines" for whole lines
	removed, else "text". None when the unit cannot be edited safely."""
	if unit.directive == 2:
		return None
	if unit.action == "deleted":
		if unit.whole_lines:
			first, last = unit.segments[0].line, unit.segments[-1].line
			start = offsets[first]
			end = offsets[last + 1] if last + 1 < len(offsets) else len(text)
			return [(start, end, "", "lines")]
		if len(unit.segments) > 1 and unit.kind == "block":
			return None
		# After code: the spaces before the comment go with it; inside a line, keep one space where needed.
		start, end = unit.start, unit.end
		ls = text.rfind("\n", 0, start) + 1
		le = text.find("\n", end)
		le = len(text) if le < 0 else le
		if not text[end:le].strip():
			while start > ls and text[start - 1] in " \t":
				start -= 1
			while end < le and text[end] in " \t":
				end += 1
			return [(start, end, "", "text")]
		# Inside a line: the spacing the code had on one side stays; none before a closer or after an opener.
		before_ws = start > ls and text[start - 1] in " \t"
		after_ws = text[end] in " \t"
		if after_ws:
			while end < le and text[end] in " \t":
				end += 1
		if before_ws and (after_ws or text[end] not in ",;)]}"):
			return [(start, end, "", "text")]
		if before_ws:
			while start > ls and text[start - 1] in " \t":
				start -= 1
			return [(start, end, "", "text")]
		if text[end] in ",;)]}" or (start > ls and text[start - 1] in "([{"):
			return [(start, end, "", "text")]
		return [(start, end, " ", "text")]
	if unit.action == "stripped":
		return strip_edits(text, unit, offsets)
	return None


def strip_edits(text, unit, offsets):
	"""The edits that take unit.removals (spans of its words) out of the file: each line keeps its comment opening
	and the words left on it; a line left with punctuation only (the period after a removed parenthetical) gives it
	to the line before; a `//` line left with nothing goes."""
	lines = []  # [file start, length, old, new]
	for jstart, fstart, length in unit.pieces:
		local = [(max(s, jstart) - jstart, min(e, jstart + length) - jstart) for s, e in unit.removals
				 if s < jstart + length and e > jstart]
		old = text[fstart:fstart + length]
		lines.append([fstart, length, old, without(old, local).strip() if local else old,
					  any(s == 0 for s, _ in local)])
	for k in range(1, len(lines)):
		lead = re.match(r"[.,;:]+\s*", lines[k][3])
		if lines[k][4] and lead and lines[k - 1][3]:
			lines[k - 1][3] += lead.group(0).strip()
			lines[k][3] = lines[k][3][lead.end():]
	edits = []
	for fstart, length, old, new, _ in lines:
		if new == old:
			continue
		if new:
			edits.append((fstart, fstart + length, new, "text"))
			continue
		line = line_of(offsets, fstart)
		if not (unit.whole_lines and unit.kind == "line"):
			return None
		end = offsets[line + 1] if line + 1 < len(offsets) else len(text)
		edits.append((offsets[line], end, "", "lines"))
	return edits


SENTINEL = "\x00"


def apply_edits(text, edits):
	"""Apply edits (non-overlapping); a removal of whole lines leaves a sentinel at the next line's start; then the
	blank lines the removals left double or next to a brace go."""
	parts, pos = [], 0
	for start, end, repl, kind in sorted(edits):
		if start < pos:
			continue
		parts.append(text[pos:start])
		parts.append(repl + (SENTINEL if kind == "lines" else ""))
		pos = end
	parts.append(text[pos:])
	new = "".join(parts)
	if SENTINEL not in new:
		return new
	lines = new.splitlines(keepends=True)
	out = []
	for line in lines:
		gap = line.startswith(SENTINEL)
		line = line.replace(SENTINEL, "")
		if not line:
			continue
		if gap:
			blank = not line.strip()
			prev = out[-1] if out else None
			prev_blank = prev is not None and not prev.strip()
			if blank and (prev is None or prev_blank or prev.rstrip().endswith("{")):
				continue
			if line.lstrip().startswith("}") and prev_blank:
				out.pop()
		out.append(line)
	return "".join(out)


# ---------------------------------------------------------------------------------------------------------------
# Driver


def tree_files(root, paths):
	for path in paths:
		full = os.path.join(root, path)
		if os.path.isfile(full):
			yield full
			continue
		for base, dirs, names in os.walk(full):
			dirs.sort()
			for name in sorted(names):
				if name.endswith(EXTENSIONS):
					yield os.path.join(base, name)


def load_range(path):
	values = []
	with open(path, newline="") as f:
		for row in csv.DictReader(f, delimiter="\t"):
			try:
				values.append(int(row["address"], 16))
			except (KeyError, ValueError):
				pass
	if not values:
		sys.exit(f"uncomment: no addresses in {path}")
	return min(values), max(values)


def one_line(s):
	return re.sub(r"\s+", " ", s).strip()


def code_view(text):
	"""The text's code as the compiler sees it, for the check that an edit touched comments only: each comment becomes
	one space (a block comment over lines joins them, as in translation phase 3), splices join lines, then per line
	whitespace runs become one space, kept only between two word characters or two punctuators (where it can separate
	tokens); blank lines go."""
	parts, pos = [], 0
	for start, end, _, _ in lex_comments(text):
		parts.append(text[pos:start])
		parts.append(" ")
		pos = end
	parts.append(text[pos:])
	out = []
	for line in re.sub(r"\\\r?\n", " ", "".join(parts)).split("\n"):
		line = re.sub(r"\s+", " ", line).strip()
		line = re.sub(r"(?<=\w) (?=[^\w ])|(?<=[^\w ]) (?=\w)", "", line)
		if line:
			out.append(line)
	return out


def process(path, rel, args, sites, edits_out, refused):
	with open(path, encoding="utf-8", errors="surrogateescape", newline="") as f:
		text = f.read()
	offsets = [0] + [m.end() for m in re.finditer(r"\n", text)]
	if offsets[-1] == len(text) and len(offsets) > 1:
		offsets.pop()
	units = build_units(text, lex_comments(text), offsets)
	edits = []
	for unit in units:
		words = unit.layout(text)
		if not words:
			unit.cls = "empty"
			sites.append((rel, unit, words))
			continue
		classify(unit, words, args)
		if unit.action in ("deleted", "stripped"):
			e = unit_edits(text, unit, offsets)
			if e is None:
				unit.action = "mixed"
				if unit.directive == 2:
					unit.cls = "directive"
			else:
				edits.extend(e)
				new_words = "" if unit.action == "deleted" else without(words, unit.removals).strip()
				edits_out.append((rel, unit.first_line + 1, unit.action, one_line(words), one_line(new_words)))
		sites.append((rel, unit, words))
	if not edits:
		return 0
	new = apply_edits(text, edits)
	if code_view(new) != code_view(text):
		refused.append(rel)
		return 0
	if not args.dry_run and new != text:
		with open(path, "w", encoding="utf-8", errors="surrogateescape", newline="") as f:
			f.write(new)
	return len(edits)


def main():
	ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
	ap.add_argument("paths", nargs="*", metavar="PATH")
	ap.add_argument("--dry-run", action="store_true")
	ap.add_argument("--after-deinline", action="store_true")
	ap.add_argument("--no-strip", action="store_true")
	ap.add_argument("--addresses", default=os.path.join(ROOT, "harness", "addresses.tsv"))
	ap.add_argument("--out", default=os.path.join(ROOT, "env", "scratch", "Clean", "uncomment"))
	ap.add_argument("--root", default=os.getcwd(), help="the tree to rewrite (default the current directory)")
	args = ap.parse_args()
	global LOW, HIGH
	LOW, HIGH = load_range(args.addresses)
	root = os.path.abspath(args.root)
	paths = args.paths or ["src"]
	sites, edits_out, refused = [], [], []
	files_changed = 0
	for path in tree_files(root, paths):
		rel = os.path.relpath(path, root)
		if process(path, rel, args, sites, edits_out, refused):
			files_changed += 1
	write_tables(args, sites, edits_out)
	counts = collections.Counter((u.cls, u.action) for _, u, _ in sites)
	print(f"{len(sites)} comment units; {len(edits_out)} edited in {files_changed} files"
		  f"{' (dry run: nothing written)' if args.dry_run else ''}")
	by_action = collections.Counter()
	for (cls, action), n in counts.items():
		by_action[action] += n
	print("  ".join(f"{a} {n}" for a, n in sorted(by_action.items())))
	print(f"tables: {args.out}")
	if refused:
		print(f"refused (an edit would have changed code; file left as it is): {len(refused)}")
		for rel in refused:
			print(f"  {rel}")
		sys.exit(1)


def write_tables(args, sites, edits_out):
	os.makedirs(args.out, exist_ok=True)
	rng = random.Random(0)
	groups = collections.defaultdict(list)
	for rel, unit, words in sites:
		groups[(unit.cls, unit.action)].append((rel, unit, words))
	with open(os.path.join(args.out, "measure.txt"), "w") as f:
		f.write("class\taction\tunits\tlines\n")
		for key in sorted(groups, key=lambda k: -len(groups[k])):
			rows = groups[key]
			lines = sum(u.last_line - u.first_line + 1 for _, u, _ in rows)
			f.write(f"{key[0]}\t{key[1]}\t{len(rows)}\t{lines}\n")
		f.write("\nunits naming each recovery phrase (a unit can name several), by action\n")
		phrases = collections.defaultdict(collections.Counter)
		for _, unit, words in sites:
			for name in recovery_classes(words):
				phrases[name][unit.action] += 1
		for name, _ in PHRASES:
			by = phrases.get(name, {})
			f.write(f"{name}\t{sum(by.values())}\t" + "  ".join(f"{a} {n}" for a, n in sorted(by.items())) + "\n")
		f.write("\n")
		for key in sorted(groups, key=lambda k: -len(groups[k])):
			rows = groups[key]
			f.write(f"== {key[0]} / {key[1]}: {len(rows)}\n")
			for rel, unit, words in rng.sample(rows, min(10, len(rows))):
				f.write(f"  {rel}:{unit.first_line + 1}: {one_line(words)[:300]}\n")
			f.write("\n")
	with open(os.path.join(args.out, "sites.tsv"), "w", newline="") as f:
		w = csv.writer(f, delimiter="\t", lineterminator="\n", quoting=csv.QUOTE_MINIMAL)
		w.writerow(["file", "line", "last", "class", "action", "text"])
		for rel, unit, words in sites:
			w.writerow([rel, unit.first_line + 1, unit.last_line + 1, unit.cls, unit.action, one_line(words)])
	with open(os.path.join(args.out, "mixed.tsv"), "w", newline="") as f:
		w = csv.writer(f, delimiter="\t", lineterminator="\n", quoting=csv.QUOTE_MINIMAL)
		w.writerow(["file", "line", "class", "text"])
		for rel, unit, words in sites:
			if unit.action == "mixed":
				w.writerow([rel, unit.first_line + 1, unit.cls, one_line(words)])
	with open(os.path.join(args.out, "edits.tsv"), "w", newline="") as f:
		w = csv.writer(f, delimiter="\t", lineterminator="\n", quoting=csv.QUOTE_MINIMAL)
		w.writerow(["file", "line", "action", "old", "new"])
		w.writerows(edits_out)


if __name__ == "__main__":
	main()
