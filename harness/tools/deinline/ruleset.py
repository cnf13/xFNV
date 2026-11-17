"""The de-inlining rules: one per helper whose body src writes out, each tied to the helper's definition in src.

Read by harness/tools/deinline.py (`rules` writes one ast-grep rule file per entry into this folder, `apply` runs
them). Every rule names its helper's definition (`Authority`: the file, the line the definition starts with, and its
text): the rule is refused when the definition in the tree no longer reads so, because the pattern spells that body.
Where the body is one expression, the pattern is derived from it (`derive`: the members of the object become `$A.m`,
the parameters `$B`); a body of several statements is spelled statement by statement after it, in the written-out
forms the tree holds (the reference-count steps through the atomic primitives the helper's callees use).

A rule's `group`:
  inline    the helper is inline in its header: the call compiles to the code written out before (applied)
  outline   the helper is defined in a .cpp: the call adds a call where the code was inline (applied only with
            --outline; the helper is better made inline in its header first)
  locate    no fix: the sites are counted and classed, a person rewrites them (why: `note`)
"""

import re

NC = {"not": {"kind": "comment"}}
ANY = {"regex": "[\\s\\S]"}  # matches any node: the positive matcher ast-grep wants beside a relational rule
# the comments a rewrite takes away with the statements they mark
MARKER = {"kind": "comment", "regex": "EXE-INLINED|EXE-CALLED|SWEEP-INLINE"}


class Authority:
	"""The helper's definition: `start` is a regex matching its first line in `file` (under src/); `text` the
	definition from there to its closing brace (or a macro's last line), compared with whitespace collapsed."""

	def __init__(self, helper, file, start, text):
		self.helper, self.file, self.start, self.text = helper, file, start, text

	def check(self, root):
		"""None when the tree's definition reads as quoted, else why not."""
		import os
		path = os.path.join(root, "src", self.file)
		if not os.path.exists(path):
			return f"{self.file} is missing"
		src = open(path, encoding="utf-8", errors="surrogateescape").read()
		m = re.search(self.start, src, re.M)
		if not m:
			return f"{self.helper}: no definition in {self.file}"
		got = definition_text(src, m.start())
		if squash(got) != squash(self.text):
			return f"{self.helper}: the definition in {self.file} differs from the rule's"
		return None

	def inline(self):
		return self.file.endswith((".h", ".inl", ".hpp"))


def squash(s):
	return re.sub(r"\s+", " ", re.sub(r"//[^\n]*", "", s)).strip()


def definition_text(src, i):
	"""From i to the end of the definition: a #define with its continuation lines, else the balanced braces."""
	if src[i:].lstrip().startswith("#define"):
		j = i
		while True:
			e = src.find("\n", j)
			if e < 0 or src[e - 1] != "\\":
				return src[i:e if e >= 0 else len(src)]
			j = e + 1
	j = src.index("{", i)
	depth = 0
	for k in range(j, len(src)):
		if src[k] == "{":
			depth += 1
		elif src[k] == "}":
			depth -= 1
			if depth == 0:
				return src[i:k + 1]
	return src[i:]


def derive(text, members, params):
	"""The ast-grep pattern of an expression-bodied helper (`{ return EXPR; }`): each member of the object becomes
	`$A.member`, each parameter `$<NAME>` (its name upper-cased), each parameter's member access kept."""
	body = re.search(r"\{\s*return\s+(.*?);\s*\}\s*$", text, re.S).group(1)
	out = []
	for m in re.finditer(r"[A-Za-z_]\w*|\.|\s+|[^\w\s.]+", body):
		t = m.group(0)
		prev = "".join(out).rstrip()
		if t in params:
			out.append("$" + params[t])
		elif t in members and not prev.endswith("."):
			out.append("$A." + t)
		else:
			out.append(t)
	return re.sub(r"\s+", " ", "".join(out)).strip()


# --------------------------------------------------------------------------------------------------------------------
# Pattern pieces

def nth(n, **rule):
	"""The n-th child that is not a comment (marker comments never break a match)."""
	return {"nthChild": {"position": n, "ofRule": NC}, **(rule or ANY)}


def only(*rules):
	"""A compound statement whose non-comment children are exactly `rules`, in order."""
	return {"kind": "compound_statement", "all": [{"has": nth(i + 1, **r)} for i, r in enumerate(rules)] +
			[{"not": {"has": nth(len(rules) + 1)}}]}


def body(rule):
	"""An if's or loop's body: the statement itself, or a block holding only it."""
	return {"any": [rule, only(rule)]}


def cond(rule):
	return {**ANY, "has": {"field": "condition", "has": {"field": "value", **rule}}}


NO_ELSE = {"not": {"has": {"field": "alternative", **ANY}}}


def if_(test, then):
	return {"kind": "if_statement", "all": [cond(test), {"has": {"field": "consequence", **body(then)}}, NO_ELSE]}


def unused_after(v):
	"""No later statement of the enclosing block names the local."""
	return {"not": {"precedes": {"stopBy": "end", "any": [{"pattern": v},
														  {**ANY, "has": {"stopBy": "end", "pattern": v}}]}}}


def prev(rule):
	"""The statement right before, a marker comment between allowed."""
	return {**ANY, "any": [{**ANY, "follows": {"stopBy": "neighbor", **rule}},
						   {**ANY, "follows": {"stopBy": "neighbor", **MARKER,
											   "follows": {"stopBy": "neighbor", **rule}}}]}


def nxt(rule):
	return {**ANY, "any": [{**ANY, "precedes": {"stopBy": "neighbor", **rule}},
						   {**ANY, "precedes": {"stopBy": "neighbor", **MARKER,
												"precedes": {"stopBy": "neighbor", **rule}}}]}


def with_all(rule, extra):
	"""rule and extra, in that order: ast-grep checks an `all` in order and binds metavariables as it goes, so a
	guard naming $V must come after the rule that binds $V."""
	if not extra:
		return rule
	base = {k: v for k, v in rule.items() if k != "all"}
	return {"all": ([base] if base else []) + list(rule.get("all", [])) + extra}


def release(v):
	"""The statements that drop one NiRefObject reference of v (v known non-null)."""
	dec_tests = [{"pattern": f"!InterlockedDecrementUInt32(&{v}->m_uiRefCount)"},
				 {"pattern": f"InterlockedDecrementUInt32(&{v}->m_uiRefCount) == 0"}]
	return {"any": [{"pattern": f"NiDecRefCount({v});"}, {"pattern": f"{v}->DecRefCount();"},
					if_({"any": dec_tests}, {"pattern": f"{v}->DeleteThis();"})]}


def release_if(v):
	"""`if (v) release(v)`, or the one-test spelling `if (v && !InterlockedDecrement(..)) v->DeleteThis();`."""
	tests = [{"pattern": f"{v} && !InterlockedDecrementUInt32(&{v}->m_uiRefCount)"},
			 {"pattern": f"{v} && InterlockedDecrementUInt32(&{v}->m_uiRefCount) == 0"}]
	return {"any": [if_({"pattern": v}, release(v)), if_({"any": tests}, {"pattern": f"{v}->DeleteThis();"})]}


def hold_if(v):
	inc = {"any": [{"pattern": f"NiIncRefCount({v});"}, {"pattern": f"{v}->IncRefCount();"},
				   {"pattern": f"InterlockedIncrementUInt32(&{v}->m_uiRefCount);"}]}
	return if_({"pattern": v}, inc)


# --------------------------------------------------------------------------------------------------------------------
# The rules

RULES = {}


def rule(id, group, authority, note, rule, fix=None, constraints=None, transform=None, files=None):
	"""A pattern of one statement or expression. With a fix, a companion takes the marker comment right above."""
	docs = [dict(id=id, rule=rule, fix=fix)]
	if fix is not None:
		docs.append(dict(id=f"{id}.marker", rule={**MARKER, "precedes": {"stopBy": "neighbor", **rule}}, fix=""))
	RULES[id] = dict(group=group, authority=authority, note=note, docs=docs, constraints=constraints,
					 transform=transform, files=files)


def seq(id, group, authority, note, stmts, anchor, fix=None, constraints=None, transform=None):
	"""A run of statements (marker comments between allowed), replaced by `fix` at stmts[anchor]; the others and
	the markers above each are deleted (companion documents `<id>.del<k>`, `<id>.marker<k>` with an empty fix)."""
	n = len(stmts)

	def back(k):
		return [] if k == 0 else [prev(with_all(stmts[k - 1], back(k - 1)))]

	def fwd(k):
		return [] if k == n - 1 else [nxt(with_all(stmts[k + 1], fwd(k + 1)))]

	def full(k):
		return with_all(stmts[k], back(k) + fwd(k))

	docs = [dict(id=id, rule=full(anchor), fix=fix)]
	if fix is not None:
		docs += [dict(id=f"{id}.del{k}", rule=full(k), fix="") for k in range(n) if k != anchor]
		docs += [dict(id=f"{id}.marker{k}", rule={**MARKER, "precedes": {"stopBy": "neighbor", **full(k)}}, fix="")
				 for k in range(n)]
	RULES[id] = dict(group=group, authority=authority, note=note, docs=docs, constraints=constraints,
					 transform=transform, files=None)


# --- NiPointer<T> --------------------------------------------------------------------------------------------------
NIPOINTER_ASSIGN = Authority(
	"NiPointer<T>::operator=(T*)", "Gamebryo2.2/CoreLibs/NiMain/NiPointer.inl",
	r"^inline NiPointer<T>& NiPointer<T>::operator=\(T\* apObject\)", """
inline NiPointer<T>& NiPointer<T>::operator=(T* apObject)
{
	if (m_pObject != apObject) {
		if (m_pObject) {
			NiDecRefCount(m_pObject);
		}
		m_pObject = apObject;
		if (m_pObject) {
			NiIncRefCount(m_pObject);
		}
	}
	return *this;
}""")
ASSIGN_BLOCK = only(release_if("$OLD"), {"pattern": "$SP.m_pObject = $NEW;"}, hold_if("$NEW"))
IN_ASSIGN = {"inside": {"stopBy": "end", "kind": "if_statement", "has": {"field": "consequence", **ASSIGN_BLOCK}}}
ASSIGN_IF = {"kind": "if_statement", "all": [NO_ELSE, {"has": {"field": "consequence", **ASSIGN_BLOCK}}]}
seq("nipointer-assign", "inline", NIPOINTER_ASSIGN,
	"NiPointer<T>::operator=(T*) written out: the old object read into a local, released, the new one stored and held",
	[{"pattern": "$T* $OLD = $SP.m_pObject;"},
	 with_all(ASSIGN_IF, [unused_after("$OLD"), {**ANY, "any": [
		 cond({"pattern": "$OLD != $NEW"}),
		 # compared with $X, stored $NEW: $NEW was read from $X earlier in the block
		 {**ANY, "all": [cond({"pattern": "$OLD != $X"}),
						 {**ANY, "follows": {"stopBy": "end", "pattern": "$TN* $NEW = $X;"}}]}]}])],
	1, fix="$SP = $NEW;")
rule("nipointer-assign-field", "inline", NIPOINTER_ASSIGN,
	 "NiPointer<T>::operator=(T*) written out, the test reading the pointer's field itself",
	 with_all(ASSIGN_IF, [cond({"pattern": "$SP.m_pObject != $NEW"}), cond({"pattern": "$OLD != $NEW"})]),
	 fix="$SP = $NEW;")
rule("nipointer-assign-inner", "inline", NIPOINTER_ASSIGN,
	 "NiPointer<T>::operator=(T*) written out with the old object read inside the test of the pointer itself",
	 {"kind": "if_statement", "all": [NO_ELSE, cond({"pattern": "$SP != $NEW"}), {"has": {
		 "field": "consequence", **only({"any": [{"pattern": "$T* $OLD = $SP;"}, {"pattern": "$T* $OLD = $SP.m_pObject;"}]},
										release_if("$OLD"), {"pattern": "$SP.m_pObject = $NEW;"}, hold_if("$NEW"))}}]},
	 fix="$SP = $NEW;")

CLEAR = {"pattern": "$SP.m_pObject = $Z;"}
Z_NULL = {"Z": {"regex": r"^(0|nullptr|NULL)$"}}
rule("nipointer-release-clear", "inline", NIPOINTER_ASSIGN,
	 "NiPointer<T>::operator=(nullptr) written out: `if (sp) { release(sp); sp.m_pObject = 0; }`",
	 {"kind": "if_statement", "all": [NO_ELSE, {**ANY, "any": [
		 {**ANY, "all": [cond({"pattern": "$SP.m_pObject"}),
						 {"has": {"field": "consequence", **only(release("$SP.m_pObject"), CLEAR)}}]},
		 {**ANY, "all": [cond({"pattern": "$SP"}), {"has": {"field": "consequence", **only(release("$SP"), CLEAR)}}]}]}]},
	 fix="$SP = nullptr;", constraints=Z_NULL)
seq("nipointer-release-clear-local", "inline", NIPOINTER_ASSIGN,
	"the same through a local read from the field right before",
	[{"pattern": "$T* $V = $SP.m_pObject;"},
	 {"kind": "if_statement", "all": [NO_ELSE, cond({"pattern": "$V"}), unused_after("$V"),
									  {"has": {"field": "consequence", **only(release("$V"), CLEAR)}}]}],
	1, fix="$SP = nullptr;", constraints=Z_NULL)
seq("nipointer-release-other", "locate", NIPOINTER_ASSIGN,
	"a reference dropped through a local read from a smart pointer's field, the field left set: the holder owns "
	"the reference by hand; making it a NiPointer moves the release into its destructor (read its whole life)",
	[{"pattern": "$T* $V = $SP.m_pObject;"}, with_all(release_if("$V"), [{"not": IN_ASSIGN}])], 1)

# --- NiRefObject / NiD3DPass reference counts ----------------------------------------------------------------------
NIREFOBJECT_DEC = Authority(
	"NiRefObject::DecRefCount", "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h", r"^\s*void DecRefCount\(\)$", """
	void DecRefCount()
	{
		if (NiAtomicDecrement(m_uiRefCount) == 0) {
			DeleteThis();
		}
	}""")
NIREFOBJECT_INC = Authority(
	"NiRefObject::IncRefCount", "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h", r"^\s*void IncRefCount\(\) \{",
	"void IncRefCount() { NiAtomicIncrement(m_uiRefCount); }")
DEC_TESTS = [{"pattern": "!InterlockedDecrementUInt32(&$P->m_uiRefCount)"},
			 {"pattern": "InterlockedDecrementUInt32(&$P->m_uiRefCount) == 0"}]
rule("nirefobject-decref", "inline", NIREFOBJECT_DEC,
	 "NiRefObject::DecRefCount written out through the interlocked decrement",
	 with_all(if_({"any": DEC_TESTS}, {"pattern": "$P->DeleteThis();"}), [{"not": IN_ASSIGN}]),
	 fix="$P->DecRefCount();")
rule("nirefobject-decref-nulltest", "inline", NIREFOBJECT_DEC,
	 "`if (p && !InterlockedDecrement(&p->m_uiRefCount)) p->DeleteThis();`: a null-tested DecRefCount",
	 with_all(if_({"any": [{"pattern": "$P && " + t["pattern"]} for t in DEC_TESTS]},
				  {"pattern": "$P->DeleteThis();"}), [{"not": IN_ASSIGN}]),
	 fix="if ($P) {\n\t$P->DecRefCount();\n}")
rule("nirefobject-incref", "inline", NIREFOBJECT_INC,
	 "NiRefObject::IncRefCount written out through the interlocked increment",
	 {"pattern": "InterlockedIncrementUInt32(&$P->m_uiRefCount);", "not": IN_ASSIGN},
	 fix="$P->IncRefCount();",
	 # a string table entry's header has a count of the same name and is no NiRefObject
	 constraints={"P": {"not": {"regex": r"GetStringHeader"}}})

NID3DPASS_DEC = Authority(
	"NiD3DPass::DecRefCount", "Gamebryo2.2/CoreLibs/NiXenonRenderer/NiD3DPass.h", r"^\s*void DecRefCount\(\)$", """
	void DecRefCount()
	{
		if (--m_uiRefCount == 0) {
			ReturnPassToPool();
		}
	}""")
seq("nid3dpass-decref", "inline", NID3DPASS_DEC,
	"NiD3DPass::DecRefCount written out: post-decrement, then ReturnPassToPool on the last",
	[{"any": [{"pattern": "bool $B = $P->m_uiRefCount-- == 1;"}, {"pattern": "$B = $P->m_uiRefCount-- == 1;"}]},
	 with_all(if_({"pattern": "$B"}, {"pattern": "$P->ReturnPassToPool();"}), [unused_after("$B")])],
	1, fix="$P->DecRefCount();")

# --- NiFixedString / NiGlobalStringTable ---------------------------------------------------------------------------
NIFIXEDSTRING_ASSIGN = Authority(
	"NiFixedString::operator=(const NiFixedString&)", "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h",
	r"^\s*NiFixedString& operator=\(const NiFixedString& kString\)$", """
	NiFixedString& operator=(const NiFixedString& kString)
	{
		if (m_kHandle != kString.m_kHandle) {
			NiGlobalStringTable::GlobalStringHandle kHandle = kString.m_kHandle;
			NiGlobalStringTable::IncRefCount(kHandle);
			NiGlobalStringTable::DecRefCount(m_kHandle);
			m_kHandle = kHandle;
		}
		return *this;
	}""")
seq("nifixedstring-assign", "inline", NIFIXEDSTRING_ASSIGN,
	"NiFixedString::operator=(const NiFixedString&) written out: acquire the new handle, release the old, store",
	[{"any": [{"pattern": "$HT $H = $S.m_kHandle;"}, {"pattern": "$HT $H = $S.GetHandle();"}]},
	 {"kind": "if_statement", "all": [
		 cond({"pattern": "$D.m_kHandle != $H"}), NO_ELSE, unused_after("$H"),
		 {"has": {"field": "consequence", **only({"pattern": "NiGlobalStringTable::AcquireHandle($H);"},
												 {"pattern": "NiGlobalStringTable::ReleaseHandle($D.m_kHandle);"},
												 {"pattern": "$D.m_kHandle = $H;"})}}]}],
	1, fix="$D = $S;")
rule("nifixedstring-assign-direct", "inline", NIFIXEDSTRING_ASSIGN,
	 "the same reading the source string's handle in place (no local)",
	 {"kind": "if_statement", "all": [
		 cond({"pattern": "$D.m_kHandle != $S.m_kHandle"}), NO_ELSE,
		 {"has": {"field": "consequence", **only({"pattern": "NiGlobalStringTable::AcquireHandle($S.m_kHandle);"},
												 {"pattern": "NiGlobalStringTable::ReleaseHandle($D.m_kHandle);"},
												 {"pattern": "$D.m_kHandle = $S.m_kHandle;"})}}]},
	 fix="$D = $S;")
rule("nifixedstring-release", "locate", None,
	 "NiFixedString's end of life written out (the handle released by hand): the class has no destructor yet, so "
	 "there is no call to write; with one, a release at the end of a local's life goes and one before a "
	 "re-construction becomes an assignment",
	 {"any": [{"pattern": f"NiGlobalStringTable::ReleaseHandle($K{a}{m});"}
			  for a in (".", "->") for m in ("GetHandle()", "m_kHandle")],
	  "not": {"inside": {"stopBy": "end", "kind": "if_statement", "has": {"field": "condition", "has": {
		  "field": "value", "pattern": "$D.m_kHandle != $H"}}}}})
rule("nifixedstring-construct", "locate", None,
	 "NiFixedString(const char*) written out on raw string storage: the storage's type and the release before "
	 "decide between a constructor and an assignment",
	 {"any": [{"pattern": "$K.m_kHandle = NiGlobalStringTable::AddString($S);"},
			  {"pattern": "$K->m_kHandle = NiGlobalStringTable::AddString($S);"}]})

# --- NiTArray ------------------------------------------------------------------------------------------------------
NITARRAY_ADD = Authority(
	"NiTArray<T, Allocator>::SetAtGrow (Add's body)", "Gamebryo2.2/CoreLibs/NiMain/NiTArray.inl",
	r"^uint32_t NiTArray<T, Allocator>::SetAtGrow\(", """
uint32_t NiTArray<T, Allocator>::SetAtGrow(uint32_t uiIndex, const T& element)
{
    if (uiIndex >= m_usMaxSize)
        SetSize(uiIndex + m_usGrowBy);

    SetAt(uiIndex, element);
    return uiIndex;
}""")
for acc, suffix in (("->", ""), (".", "-member")):
	SETAT = {"any": [{"pattern": f"$A{acc}SetAt($I, $V);"}, {"pattern": f"FNVD_NOINLINE($A{acc}SetAt($I, $V));"}]}
	GROW = {"any": [{"pattern": f"$A{acc}SetSize($I + $A{acc}m_usGrowBy);"},
					{"pattern": f"FNVD_NOINLINE($A{acc}SetSize($I + $A{acc}m_usGrowBy));"}]}
	GROW_IF = {"kind": "if_statement", "all": [cond({"pattern": f"$I >= $A{acc}m_usMaxSize"}), NO_ELSE,
											   {"has": {"field": "consequence", **body(GROW)}}]}
	seq(f"nitarray-add{suffix}", "inline", NITARRAY_ADD,
		f"NiTArray<T>::Add (SetAtGrow at m_usSize) written out on an array {'pointer' if acc == '->' else 'object'}:"
		" grow by m_usGrowBy when full, then SetAt (the index in a local assigned again before each use)",
		[{"pattern": f"$I = $A{acc}m_usSize;"}, GROW_IF, SETAT], 1, fix=f"$A{acc}Add($V);")
	seq(f"nitarray-add{suffix}-decl", "inline", NITARRAY_ADD,
		"the same with the index declared for this Add alone",
		[{"pattern": f"$TI $I = $A{acc}m_usSize;"}, GROW_IF, with_all(SETAT, [unused_after("$I")])], 1,
		fix=f"$A{acc}Add($V);")

# --- Havok ---------------------------------------------------------------------------------------------------------
HK_TIMER = Authority(
	"HK_TIMER_INTERN (HK_TIMER_BEGIN, HK_TIMER_END)", "Havok/Source/Common/Base/Monitor/hkMonitorStream.h",
	r"^#define HK_TIMER_INTERN\(", r"""
#define HK_TIMER_INTERN(NAME, COMMAND, OBJECT) \
	{ \
		hkMonitorStream& mStream = hkMonitorStream::getInstance(); \
		if (mStream.memoryAvailable()) { \
			hkMonitorStream::TimerCommand* h = reinterpret_cast<hkMonitorStream::TimerCommand*>(mStream.getEnd()); \
			h->m_commandAndMonitor = COMMAND HK_MONITOR_TYPE_TIMER NAME; \
			h->setTime(); \
			mStream.setEnd(reinterpret_cast<char*>(h + 1)); \
		} \
	}""")
HK_TIMER_LIST = Authority(
	"HK_TIMER_BEGIN_LIST", "Havok/Source/Common/Base/Monitor/hkMonitorStream.h",
	r"^#define HK_TIMER_BEGIN_LIST\(", r"""
#define HK_TIMER_BEGIN_LIST(NAME, FIRST_SPLIT) \
	{ \
		hkMonitorStream& mStream = hkMonitorStream::getInstance(); \
		if (mStream.memoryAvailable()) { \
			hkMonitorStream::TimerBeginListCommand* h = \
				reinterpret_cast<hkMonitorStream::TimerBeginListCommand*>(mStream.getEnd()); \
			h->m_commandAndMonitor = HK_MONITOR_COMMAND_BEGIN_LIST HK_MONITOR_TYPE_TIMER NAME; \
			h->m_nameOfFirstSplit = HK_MONITOR_COMMAND_SPLIT_LIST HK_MONITOR_TYPE_TIMER FIRST_SPLIT; \
			h->setTime(); \
			mStream.setEnd(reinterpret_cast<char*>(h + 1)); \
		} \
	}""")
RDTSC = {"RDTSC": {"regex": r"^(__rdtsc|__builtin_ia32_rdtsc)$"}}
# A timer command's steps: the stream's end taken as the command, its text, its time stamp (the low dword of the
# cycle counter), the end moved past it. Written with the command's type, or with the 32-bit offsets of its fields
# (time at +4, 12 bytes; a list's first split at +12, 16 bytes), which the macro's typed form replaces. A typed
# command may still step the end by its 32-bit size (12, 16) where the macro steps by the type.


def typed_command(t, size):
	"""The typed command's declaration and its step: past the command (`$H + 1`) or by its 32-bit size."""
	decl = {"any": [{"pattern": f"auto* $H = reinterpret_cast<hkMonitorStream::{t}*>($S.m_end);"},
					{"pattern": f"hkMonitorStream::{t}* $H = reinterpret_cast<hkMonitorStream::{t}*>($S.m_end);"}]}
	step = {"any": [{"pattern": "$S.m_end = reinterpret_cast<char*>($H + 1);"},
					{"pattern": f"$S.m_end += {size};"}]}
	return decl, step


TYPED, TYPED_STEP = typed_command("TimerCommand", 12)
TYPED_LIST, TYPED_LIST_STEP = typed_command("TimerBeginListCommand", 16)
TIMER_BODIES = {
	"typed": only(TYPED,
				  {"pattern": "$H->m_commandAndMonitor = $STR;"},
				  {"pattern": "$H->m_time0 = static_cast<uint32_t>($RDTSC());"},
				  TYPED_STEP),
	"typedlist": only(TYPED_LIST,
					  {"pattern": "$H->m_commandAndMonitor = $STR;"},
					  {"pattern": "$H->m_nameOfFirstSplit = $SPLIT;"},
					  {"pattern": "$H->m_time0 = static_cast<uint32_t>($RDTSC());"},
					  TYPED_LIST_STEP),
	"raw": only({"pattern": "*reinterpret_cast<const char**>($S.m_end) = $STR;"},
				{"pattern": "*reinterpret_cast<uint32_t*>($S.m_end + 4) = static_cast<uint32_t>($RDTSC());"},
				{"pattern": "$S.m_end += 12;"}),
	"rawlist": only({"pattern": "*reinterpret_cast<const char**>($S.m_end) = $STR;"},
					{"pattern": "*reinterpret_cast<const char**>($S.m_end + 12) = $SPLIT;"},
					{"pattern": "*reinterpret_cast<uint32_t*>($S.m_end + 4) = static_cast<uint32_t>($RDTSC());"},
					{"pattern": "$S.m_end += 16;"}),
}
# the command's text prefix -> the macro (NAME: the text without its two-letter prefix)
TIMER_MACROS = [
	("begin", r'^"Tt', "HK_TIMER_BEGIN($NAME, nullptr);", HK_TIMER, ("typed", "raw")),
	("end", r'^"Et"$', "HK_TIMER_END();", HK_TIMER, ("typed", "raw")),
	("named-end", r'^"Et[^"]', "HK_TIMER_NAMED_END($NAME);", HK_TIMER, ("typed", "raw")),
	("split", r'^"St', "HK_TIMER_SPLIT_LIST($NAME);", HK_TIMER, ("typed", "raw")),
	("end-list", r'^"lt"$', "HK_TIMER_END_LIST();", HK_TIMER, ("typed", "raw")),
	("begin-list", r'^"Lt', "HK_TIMER_BEGIN_LIST($NAME, $FIRST);", HK_TIMER_LIST, ("typedlist", "rawlist")),
]
TIMER_DECL = {"pattern": "hkMonitorStream& $S = hkMonitorStream::getInstance();"}
# the stream through a pointer and its end through a local, each declared or assigned again
TIMER_PTR = {"any": [{"pattern": "hkMonitorStream* $S = &hkMonitorStream::getInstance();"},
					 {"pattern": "$S = &hkMonitorStream::getInstance();"}]}
TIMER_PTR_END = {"any": [{"pattern": "char* $E = $S->m_end;"}, {"pattern": "$E = $S->m_end;"}]}
TIMER_PTR_BODY = only({"pattern": "*reinterpret_cast<const char**>($E) = $STR;"},
					  {"pattern": "*reinterpret_cast<uint32_t*>($E + 4) = static_cast<uint32_t>($RDTSC());"},
					  {"pattern": "$S->m_end = $E + 12;"})
for kind, text, fix, auth, forms in TIMER_MACROS:
	cons = {"STR": {"regex": text}, **RDTSC}
	tr = {"NAME": {"replace": {"source": "$STR", "replace": r'^"..', "by": '"'}}}
	if kind == "begin-list":
		cons["SPLIT"] = {"regex": r'^"St'}
		tr["FIRST"] = {"replace": {"source": "$SPLIT", "replace": r'^"..', "by": '"'}}
	for form in forms:
		test = {"kind": "if_statement", "all": [cond({"pattern": "$S.m_end < $S.m_capacityMinus16"}), NO_ELSE,
												{"has": {"field": "consequence", **TIMER_BODIES[form]}}]}
		block = only(TIMER_DECL, test)
		rule(f"hk-timer-{kind}-{form}", "inline", auth,
			 f"the timer macro {fix.split('(')[0]} written out ({form} command) in a block of its own",
			 block, fix=fix, constraints=cons, transform=tr)
		# without braces of their own the stream local's name stays declared after: it must not be used again
		seq(f"hk-timer-{kind}-{form}-bare", "inline", auth,
			f"the timer macro {fix.split('(')[0]} written out ({form} command) without braces of its own",
			[TIMER_DECL, with_all({**test, "not": {"inside": {"stopBy": "neighbor", **block}}}, [unused_after("$S")])],
			1, fix=fix, constraints=cons, transform=tr)
	if auth is HK_TIMER:
		# the raw form through a stream pointer and an end local: neither may be used again (a later block that
		# assigns them again goes first, the next round takes this one)
		test = {"kind": "if_statement", "all": [cond({"pattern": "$E < $S->m_capacityMinus16"}), NO_ELSE,
												{"has": {"field": "consequence", **TIMER_PTR_BODY}}]}
		seq(f"hk-timer-{kind}-rawptr", "inline", auth,
			f"the timer macro {fix.split('(')[0]} written out (raw command) through a stream pointer and an end local",
			[TIMER_PTR, TIMER_PTR_END, with_all(test, [unused_after("$S"), unused_after("$E")])],
			2, fix=fix, constraints=cons, transform=tr)

HK_MONITOR_ADD_VALUE = Authority(
	"HK_MONITOR_ADD_VALUE", "Havok/Source/Common/Base/Monitor/hkMonitorStream.h",
	r"^#define HK_MONITOR_ADD_VALUE\(", r"""
#define HK_MONITOR_ADD_VALUE(NAME, VALUE, MONITOR_TYPE) \
	{ \
		hkMonitorStream& mStream = hkMonitorStream::getInstance(); \
		if (mStream.memoryAvailable()) { \
			hkMonitorStream::AddValueCommand* h = \
				reinterpret_cast<hkMonitorStream::AddValueCommand*>(mStream.getEnd()); \
			h->m_commandAndMonitor = HK_MONITOR_COMMAND_ADD_VALUE MONITOR_TYPE NAME; \
			h->m_value = VALUE; \
			mStream.setEnd(reinterpret_cast<char*>(h + 1)); \
		} \
	}""")
# the value command with the 32-bit offsets: the float at +4, 8 bytes; "Mi" is an int monitor's text prefix
ADD_VALUE_TEST = {"kind": "if_statement", "all": [
	cond({"pattern": "$S.m_end < $S.m_capacityMinus16"}), NO_ELSE,
	{"has": {"field": "consequence", **only({"pattern": "*reinterpret_cast<const char**>($S.m_end) = $STR;"},
											{"pattern": "*reinterpret_cast<float*>($S.m_end + 4) = $V;"},
											{"pattern": "$S.m_end += 8;"})}}]}
ADD_VALUE_BLOCK = only(TIMER_DECL, ADD_VALUE_TEST)
ADD_VALUE = dict(fix="HK_MONITOR_ADD_VALUE($NAME, $V, HK_MONITOR_TYPE_INT);",
				 constraints={"STR": {"regex": r'^"Mi'}},
				 transform={"NAME": {"replace": {"source": "$STR", "replace": r'^"..', "by": '"'}}})
rule("hk-monitor-add-value-raw", "inline", HK_MONITOR_ADD_VALUE,
	 "HK_MONITOR_ADD_VALUE written out (raw command) in a block of its own", ADD_VALUE_BLOCK, **ADD_VALUE)
seq("hk-monitor-add-value-raw-bare", "inline", HK_MONITOR_ADD_VALUE,
	"HK_MONITOR_ADD_VALUE written out (raw command) without braces of its own",
	[TIMER_DECL, with_all({**ADD_VALUE_TEST, "not": {"inside": {"stopBy": "neighbor", **ADD_VALUE_BLOCK}}},
						  [unused_after("$S")])], 1, **ADD_VALUE)

rule("hk-newobject", "locate", None,
	 "hkReferencedObject's class allocation written out (blockAlloc, then m_memSizeAndFlags): the end state is "
	 "`new T(...)`, which also takes the construction after it",
	 {"kind": "expression_statement", "regex": r"m_memSizeAndFlags = sizeof\(",
	  "follows": {"stopBy": "neighbor", "has": {"stopBy": "end", "kind": "call_expression", "regex": r"blockAlloc\(sizeof"}}})
rule("hkarray-init-empty", "locate", None,
	 "hkArray<T>'s default constructor written out on raw storage (null data, size 0): the array's whole life "
	 "(construction and the free) is rewritten together",
	 {"pattern": "$A.m_data = nullptr;", "all": [nxt({"pattern": "$A.m_size = 0;"})]})
rule("hkenum-assign", "locate", None,
	 "hkEnum<E, S>::operator=(E) written out: the storage member assigned a cast enumerator; hkFlags has "
	 "m_storage too, so the member's type decides between `x = E` and `x.setAll(E)`",
	 {"pattern": "$X.m_storage = static_cast<$S>($E);"},
	 constraints={"E": {"regex": r"::|^[A-Z][A-Z0-9_]+$"}})
rule("hkpcdbody-root", "locate", None,
	 "hkpCdBody::getRootCollidable written out (the parent walk): the walk leaves a body, the helper returns a "
	 "collidable; the later use decides the cast",
	 {"kind": "while_statement", "all": [
		 {"has": {"field": "condition", "has": {"field": "value", "pattern": "$B->m_parent"}}},
		 {"has": {"field": "body", **body({"pattern": "$B = $B->m_parent;"})}}]})
DOT3_SUM = ["_mm_add_ss(_mm_shuffle_ps($T, $T, 0xAA), _mm_add_ss(_mm_shuffle_ps($T, $T, 0x55), $T))",
			"_mm_add_ss(_mm_shuffle_ps($T, $T, _MM_SHUFFLE(2, 2, 2, 2)), _mm_add_ss(_mm_shuffle_ps($T, $T, "
			"_MM_SHUFFLE(1, 1, 1, 1)), $T))"]
rule("hkvector4-dot3", "locate", None,
	 "hkVector4::dot3 written out in SSE (the product, then lane z + (lane y + lane x)): the result feeds a float "
	 "or a quad and the product local may be reused",
	 {"kind": "declaration",
	  "has": {"stopBy": "end", "any": [{"pattern": f"_mm_cvtss_f32({s})"} for s in DOT3_SUM]},
	  "follows": {"stopBy": "neighbor", "any": [{"pattern": "const __m128 $T = _mm_mul_ps($A.m_quad, $B.m_quad);"},
												{"pattern": "__m128 $T = _mm_mul_ps($A.m_quad, $B.m_quad);"}]}})

# --- engine --------------------------------------------------------------------------------------------------------
rule("mem-context-enter", "locate", None,
	 "the thread's memory context saved and set by hand (AutoMemContext written out): the restores on every return "
	 "path decide whether a guard object replaces them",
	 {"pattern": "$TL->eMemContext = $NEW;",
	  "follows": {"stopBy": "neighbor", "pattern": "$MT $OLD = $TL->eMemContext;"}})

# --- vector maths (helpers defined in .cpp files) ------------------------------------------------------------------
NIPOINT3_DOT = Authority(
	"NiPoint3::Dot", "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.cpp", r"^float NiPoint3::Dot\(", """
float NiPoint3::Dot(const NiPoint3& pt) const
{
    return x * pt.x + y * pt.y + z * pt.z;
}""")
rule("nipoint3-dot", "outline", NIPOINT3_DOT, "NiPoint3::Dot written out: x*x + y*y + z*z of two points",
	 {"pattern": derive(NIPOINT3_DOT.text, ("x", "y", "z"), {"pt": "B"})}, fix="$A.Dot($B)")
for op, name in (("+", "add"), ("-", "sub")):
	auth = Authority(
		f"NiPoint3::operator{op}", "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.cpp",
		rf"^NiPoint3 NiPoint3::operator\{op}\(const NiPoint3& pt\) const", f"""
NiPoint3 NiPoint3::operator{op}(const NiPoint3& pt) const
{{
    return NiPoint3(x {op} pt.x, y {op} pt.y, z {op} pt.z);
}}""")
	seq(f"nipoint3-{name}", "outline", auth,
		f"NiPoint3::operator{op} written out component by component into a point",
		[{"pattern": f"$D.{c} = $A.{c} {op} $B.{c};"} for c in "xyz"], 0, fix=f"$D = $A {op} $B;")
NIPOINT3_SCALE = Authority(
	"NiPoint3::operator*(float)", "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.cpp",
	r"^NiPoint3 NiPoint3::operator\*\(float fScalar\) const", """
NiPoint3 NiPoint3::operator*(float fScalar) const
{
    return NiPoint3(fScalar * x, fScalar * y, fScalar * z);
}""")
# a double literal would be rounded to float first by operator*(float): not the same arithmetic
seq("nipoint3-scale", "outline", NIPOINT3_SCALE, "NiPoint3::operator*(float) written out component by component",
	[{"pattern": f"$D.{c} = $A.{c} * $S;"} for c in "xyz"], 0, fix="$D = $A * $S;",
	constraints={"S": {"not": {"regex": r"^[0-9.]+(e[-+]?[0-9]+)?$"}}})
NIABS = Authority("NiAbs", "Fallout Shared/Magic/EffectSetting.cpp", r"^float NiAbs\(float fValue\)", """
float NiAbs(float fValue)
{
	return std::fabs(fValue);
}""")
rule("niabs", "outline", NIABS, "NiAbs written out as fabsf in Gamebryo code",
	 {"pattern": "__builtin_fabsf($X)"}, fix="NiAbs($X)", files=["**/Gamebryo2.2/**"])
