// The emitter's passes (one file each, run in Emit.cpp's order), its printer, and the ctree walks they share
// (internal to the idakit library).
#pragma once

#include "IdaSupport.hpp"

#include "idakit/Emit.hpp"
#include "idakit/SrcIda.hpp"

#include <unordered_map>
#include <vector>

namespace idakit
{

// The overlay's default, written before the passes run on every item that prints raw without a meaning (an obj that
// is no call's callee, a call, a member access): unresolved until a pass gives it its meaning, so that with a pass
// off its items still print marked. A test for whether a layer has judged an item tells it from a pass's Missing.
inline const std::string kNotResolved = "not resolved: no pass gave it a meaning";
inline bool unlooked(const Meaning& meaning)
{
	const Missing* missing = std::get_if<Missing>(&meaning);
	return missing && missing->what == kNotResolved;
}

// --- passes: each fills its part of the overlay (Emit.hpp) or adds blockers

// every local's src type from IDA's, the return type, and the type of each cast, number and dereference
void resolveTypes(Function& function, Context& context);
// The class C++ would run user code for, for an object of the record ("" for none), and what it runs ("constructor",
// "destructor", "constructor and destructor"): its own non-empty default constructor (a missing one where others are
// user-declared) and destructor, else its bases', else its by-value members'.
std::pair<std::string, std::string> userStructors(const SrcModel& model, const std::string& record);
// C++ would run user code destroying an object of the record: its destructor's, unless empty (srcdb: no code), or a
// base's or a by-value member's
bool destroys(const SrcModel& model, const std::string& record);
// the statements of the function's cleanup-only C++ EH frame that dropEhFrame leaves out (its state stores, its
// registration and unregistration); marks the frame's locals (Local::ehFrame). Empty for no such frame.
std::set<const cinsn_t*> ehFrameStatements(Function& function, Context& context);
// resolveTypes leaves a local of such a record raw storage holding it (type empty, marked); a locals pass that
// declares it the exe's way takes it back first: its src type, its uses' meanings cleared
void restoreStructorLocal(Function& function, size_t k);
// the src declaration at the function's address, checked against the exe; the parameters' src types and names
void resolveSelf(Function& function, Context& context);
// The lvars of a declaration's `this`, hidden result pointer and parameters (by index; -1: src's apEdx, which no
// argument fills) among Hex-Rays' arguments, as Self maps them, whatever their types; nullopt where the counts differ
struct ArgumentLvars
{
	int thisLvar = -1;
	int result = -1;
	std::vector<int> parameters;
};
std::optional<ArgumentLvars> argumentLvars(const Function& function, const SrcModel& model,
										   const SrcModel::Function& declaration);
// the calls of trivial accessors Calls cannot make on their receivers (folds, another class's members) printed as the
// access on the receiver's src class
void resolveFoldedAccessors(Function& function, Context& context);
// each memptr/memref as a src field
void resolveFields(Function& function, Context& context);
// each call's callee as a declared src function (direct calls and calls through a src class's primary vtable)
void resolveCalls(Function& function, Context& context);
// the Calls pass's check of a call as a call of target (its arguments and result against target's type)
Meaning checkCall(const Function& function, Context& context, const cexpr_t* call, const SrcModel::Function& target,
				  bool resultUsed);
// Calls' choice of the src function a direct call of a function's address calls: the one its user-defined call names
// (restoreCallees: a fold owner), else chooseRef's body or forwarder, else a kept body's FNVD_ADDRESS assertion.
// function null where it calls none, with why; several where src names several there (a user-defined call naming
// several src functions, a fold without a registered body). Which callee Calls then can call (its TU, its arguments)
// is Calls' own judgement.
struct ChosenCallee
{
	const SrcModel::Function* function = nullptr;
	std::string why;
	bool several = false;
};
ChosenCallee chosenCallee(const Function& function, const SrcModel& model, const cexpr_t* call);
// Calls' variadic peel of every call it resolved: the casts of each argument past a C-variadic callee's declared
// parameters that pass the operand's dword as it is, judged on the operand's src type as it is now
std::vector<const cexpr_t*> variadicCasts(const Function& function, Context& context);
// the literal sizes of allocator calls (the allocator table): sizeof of the one record the allocation is used as where
// src pins it, else marked
void markAllocations(Function& function, Context& context);
// each indirect call through a src field, global or local of function-pointer type as a call through it
void resolvePointerCalls(Function& function, Context& context);
// each data obj as a src global or static member
void resolveGlobals(Function& function, Context& context);
// a pointer field's storage read or written as a form ID during load: the cast an EXE-CAST (user, 2026-11-12)
void markFormIds(Function& function, Context& context);
// the locals of a class with inline structors doing exactly the exe's inlined steps (NiPointer) as plain C++ locals
// (before the inlined-locals pass, which leaves them alone)
void resolveStructorLocals(Function& function, Context& context);
// the locals of a class whose user-declared constructor or destructor C++ would run: FNVD_INLINED_LOCAL where the
// exe's inlined vptr stores frame their uses, else raw storage (marked); and the bytes holding an object
// X::Construct builds, as raw storage used through that object
void resolveInlinedLocals(Function& function, Context& context);
// a call's argument Hex-Rays reads from a local's bytes that are its own outgoing stack slot, stored right before the
// call, as the stored value; one an out-of-line constructor builds there marked (src has no form for it)
void foldOutgoingArguments(Function& function, Context& context);
// each number whose type, or whose place (a comparison, an assignment, an argument, a return, a switch case), is a
// src enum as its enumerator
void resolveEnums(Function& function, Context& context);
// each floating constant the exe reads from a 4-byte memory operand (x87 or SSE) whose value is that float, as a float
void resolveFloatConstants(Function& function, Context& context);
// each float-to-integer cast the exe makes with a bare fistp (the FPU's rounding mode, not truncation): the rounding
// helper src declares for it, else marked
void markRoundingConversions(Function& function, Context& context);
// Calls' variadic peel (variadicCasts) once the operands' src types are final: each such cast printed as its operand
void peelVariadicArguments(Function& function, Context& context);
// the const of a pointee src returns, carried onto the local Hex-Rays assigns it to; every use that drops it
void carryConst(Function& function, Context& context);
// the casts and base-subobject addresses C++ converts implicitly (printed as their operand)
void markImplicitConversions(Function& function, Context& context);
// the locals that are only a copy of another of the identical type (printed as that one)
void resolveAliases(Function& function, Context& context);
// the records calls return by value into a local only C++'s temporary would be (`G(F())` for `p = &(w = F()); G(*p)`)
void foldHiddenResults(Function& function, Context& context);
// a left-out cleanup-only EH frame's statements and locals (its state stores, registration and unregistration)
void dropEhFrame(Function& function, Context& context);
// the calls that do nothing src would write (a trivial accessor's whose result is unused)
void dropNoOpCalls(Function& function, Context& context);
// where each local is declared (its first assignment, or the top of the block holding its uses)
void placeDeclarations(Function& function, Context& context);
// the members of `this` a local or parameter shadows
void markShadowing(Function& function, Context& context);
// the Hex-Rays helper calls that are compiler intrinsics of both builds (SSE `_mm_*`, `__rdtsc`, byte swaps) whose
// arguments are of the intrinsic's own types, as those intrinsics
void resolveIntrinsics(Function& function, Context& context);
// the SSE vector locals written lane by lane whose reads need only lanes written before them: no uninitialised read,
// declared `= _mm_undefined_ps()`
void resolveVectorLanes(Function& function, Context& context);
// what the milestone's src form does not take: Hex-Rays helpers and constructs, exception frames
void checkRules(Function& function, Context& context);
// the exe's calls of __RTDynamicCast as src's `static_cast<Dst*>(msvc9::__RTDynamicCast(...))`
void resolveDynamicCasts(Function& function, Context& context);
// the exe's inlined read of its thread-local block (`NtCurrentTeb()->ThreadLocalStoragePointer[_tls_index]`) as src's
// GetThreadTLSData()
void rewriteThreadData(Function& function, Context& context);
// the exe's inlined NiPointer<T>::operator= as `sp = p;`, `sp = other;`, `sp = nullptr;`
void rewriteNiPointerAssignments(Function& function, Context& context);
// the exe's loops over a BSSimpleList as src's range-for
void rewriteSimpleListLoops(Function& function, Context& context);
// the exe's inlined copies of a whole record (Hex-Rays' qmemcpy) as src's assignment
void rewriteStructCopies(Function& function, Context& context);
// Hex-Rays' CONTAINING_RECORD of a base subobject as src's downcast (static_cast)
void rewriteContainingRecords(Function& function, Context& context);
// Hex-Rays' `p == (T*)-N` null test of an adjusted pointer as the test of the member's or base subobject's address
void rewriteAdjustedNullTests(Function& function, Context& context);
// Hex-Rays' part, pair and coerce helpers on scalars whose bits C++ reaches exactly, as shifts, masks and bit_cast
void rewritePartHelpers(Function& function, Context& context);
// a flag in a pointer's low bits masked off (`(int)p & ~1`) through uintptr_t, tested or read through as the pointer
void rewritePointerMasks(Function& function, Context& context);
// a constructor's or destructor's vtable stores as RULES' fnvd::setVtable
void rewriteVtableStores(Function& function, Context& context);
// the field reads an inline src accessor of the field's class returns exactly, as its call (x.GetSize())
void spellAccessors(Function& function, Context& context);
// a bool or 1-byte parameter the exe re-extends before passing it on, at that argument, read through a volatile byte
// pointer (`*reinterpret_cast<volatile const bool*>(&abFlag)`)
void rereadByteParameters(Function& function, Context& context);
// the explicit conversions where a value's printed type disagrees with where it goes (assignments, arguments, the
// object of a member call, returns, comparisons, enum steps, casts, accesses, writes through const)
void convertValues(Function& function, Context& context);
// each resolved call of a function src overloads printed so that overload resolution picks it: an argument cast to
// its parameter's type while another overload threatens, else marked
void pinOverloads(Function& function, Context& context);
// the arbitrary integer literals outside marked items, noted for a person to review (no blocker)
void flagLiterals(Function& function, Context& context);
// `alignas(N)` on each local whose address the body takes where the exe's realigned frame holds it N-aligned
void alignFrameLocals(Function& function, Context& context);

// The check file of the form (Emit.hpp), its markers appended to spots in print order.
std::string print(const Function& function, Context& context, Emitted::Form form, std::vector<Emitted::Spot>& spots);
// the C++ literal of a C string's bytes: `"`, `\`, newline, carriage return and tab escaped, any other byte below 0x20
// or from 0x7F up as a three-digit octal escape, `$` before a digit too (a rewrite's form reads `$N` as its item)
std::string cLiteral(const std::string& bytes);
// the shortest literal that reads back as the value (`0.6f` for the float nearest 0.6; a double without the `f`)
std::string floatLiteral(double value, bool single);
// src's type (a canonical key) and Hex-Rays' (canonical) are one function pointer type, or an array of those, but for
// src's references among a function's parameters and its return where Hex-Rays has pointers to the same types (IDA
// has no references; sync writes them as pointers): the same ABI, src's type the one C++ calls and stores through.
// False where they are equal, or for any other type (a reference held itself is no pointer to C++).
bool functionReferencesAsPointers(const std::string& src, const std::string& ida);

// --- ctree walks

// the expressions a statement holds itself (not those of the statements inside it)
std::vector<const cexpr_t*> ownExpressions(const cinsn_t* statement);
// the statements directly inside one (a block's items, an if's branches, a loop's body, a switch's cases)
std::vector<const cinsn_t*> childStatements(const cinsn_t* statement);
// the type of the IDA member a memptr/memref names; an array member decays to a pointer in Hex-Rays' expression type
tinfo_t memberType(const cexpr_t* e);
// CONTAINING_RECORD(p, T, f)'s offset of f in IDA's T: f's name at any depth through T's bases and unnamed members;
// where the name is at several offsets (each polymorphic base's `__vftable`), the one where a subobject of p's pointee
// type starts (Hex-Rays names the innermost member at the offset it subtracts); none where that leaves no one offset
std::optional<uint64_t> containingOffset(const cexpr_t* call);
// the record has base at that offset (directly or through its bases)
bool baseAt(const SrcModel& model, const SrcModel::Record& record, const std::string& base, int64_t offset);
// the non-virtual paths from record up to base (1: C++'s conversion to it, or a member access qualified by it, is
// unambiguous)
int basePaths(const SrcModel& model, const std::string& record, const std::string& base);
// every expression's parent item (an expression, or the statement holding it)
std::unordered_map<const cexpr_t*, const citem_t*> parentsOf(const Function& function);
// the src type key of an expression from its meaning (a local's, a field's, a call's result, a cast's, a number's, a
// global's, an accessor rewrite's), else from IDA's type; "" where neither has one
std::string srcType(const Function& function, Context& context, const cexpr_t* e);
// the src type of the object a pointer expression points at (Hex-Rays' casts peeled): `&x`'s x, an array's or
// pointer's element; "" unknown
std::string pointedType(const Function& function, Context& context, const cexpr_t* e);
// the src type of an object expression, through elements and dereferences srcType has no meaning for
std::string objectType(const Function& function, Context& context, const cexpr_t* e);
// A number cast to a pointer that is only compared (== or !=) with a pointer of the cast's src type, its cv apart, and
// is -1 or 1 .. 0xFFFF: the exe's compare of the pointer with a sentinel (INVALID_HANDLE_VALUE's -1, a small code
// below the null page), which C++'s conversion through uintptr_t gives the same bits on x64 (-1 extends to every
// bit, 1 .. 0xFFFF are no address). Never a number stored or dereferenced, nor any other value.
bool comparedSentinel(SrcTypes& types, const cexpr_t* cast,
					  const std::unordered_map<const cexpr_t*, const citem_t*>& parents);
// the object the lvalue names is const as src types it: reached through a pointer to const or a reference to const,
// a member or element of a const object, or a const reference parameter
bool srcConstObject(const Function& function, Context& context, const cexpr_t* lvalue);
// an integer key whose width is the pointer's or long's (long, size_t, ptrdiff_t, intptr_t, uintptr_t, ssize_t,
// through typedefs): 8 bytes on x64, where C++ would convert other bits than the exe's 4
bool hostWidth(const Context& context, const std::string& key);
// an integer operator C++ promotes its operands for: + - * / % & | ^ << >>, ~ and unary -
bool integerOperator(ctype_t op);
// C++'s type of an integer operator's result (op: cot_add ... cot_bnot, cot_neg) from its operands' src keys (y unused
// for a unary operator or a shift): the integral promotions, then the usual arithmetic conversions, which Hex-Rays'
// type need not follow (`GetMonth() << 9` is the enum to it, `u + 500` an int): builtin:int or builtin:unsigned int;
// "" where an operand is no integer or unscoped enum of 4 bytes at most (whose width is the same on x64)
std::string promotedResult(const Context& context, ctype_t op, const std::string& x, const std::string& y);
// the object a member access starts from, past base subobjects and implicit conversions: `this` for e = `this->Base`,
// `&this->Base` or an upcast of `this`
const cexpr_t* objectOf(const Function& function, const cexpr_t* e);
// e is C++'s `this`: the function's `this` parameter, or in a _VIA virtual (whose Hex-Rays this is the base
// subobject, printed as a cast of it) Hex-Rays' ADJ(this), which Self made `this`
bool isThis(const Function& function, const cexpr_t* e);
// e is Hex-Rays' ADJ(this): the object the function's shifted `this` (sync's `B *__shifted(D, N)` of a _VIA virtual)
// is a base subobject of
bool adjustedThis(const Function& function, const cexpr_t* e);
// e is a _VIA virtual's Hex-Rays this (the base subobject C++'s this converts to, printed `static_cast<B*>(this)`;
// never rebound), a local that is an alias of it, or a local assigned only copies of it (`v1 = this;` on several
// paths), never possibly undefined nor addressed
bool viaThis(const Function& function, const cexpr_t* e);
// the lvar is a structor's object (Construct's pThis, Destruct's and DeletingDestruct's apThis: Hex-Rays' first
// argument), never rebound: C++'s `this` of the constructor or destructor it stands for, never null
bool structorObject(const Function& function, int index);
// the src type key of a receiver (the object a member is read or called through) as the pointer Hex-Rays has: a src
// reference (a reference parameter) as the pointer to its referent, const kept
std::string receiverType(const Function& function, Context& context, const cexpr_t* receiver);
// e names a variable's storage (a local that is no reference, a global): its address is never null
bool variableObject(const Function& function, const cexpr_t* e);
// C++'s name lookup of a member name from the src record: the class declaring a field (its anonymous members' included)
// or member function of the name, else each base's lookup; one class is the member found alone
std::vector<std::string> nameLookup(Context& context, const std::string& record, const std::string& name);
// The pointer cannot be null where the item at is evaluated, so that C++'s conversion of it to a base or derived class
// at a nonzero offset adds no null test the exe lacks: `this` (isThis; C++ takes it as never null; also a _VIA
// virtual's base subobject of it, viaThis), a structor's object (structorObject), a reference local or parameter, a
// variable's
// address (`&v`, `&g`), or a local the code tests nonzero on every path to at and does not write in between (nor
// takes the address of anywhere): the left operand of `p && ...` / `!p || ...` whose right operand holds at, or the
// condition of an enclosing `if` whose then-branch holds at (`if (p)`, `if (p && ...)`; the else-branch for `if (!p)`,
// `if (!p || ...)`), the branch holding no label (a goto into it skips the test).
bool neverNull(const Function& function, const std::unordered_map<const cexpr_t*, const citem_t*>& parents,
			   const cexpr_t* pointer, const citem_t* at);
// the classes C++'s name lookup of a member name from the class finds it declared in (a function or a field): the
// class itself, else each base's (a class reached through two bases twice)
void memberLookup(const SrcModel& model, const std::string& cls, const std::string& name,
				  std::vector<std::string>& found);
// The type of the value an expression has: Hex-Rays' type, but for a call it types as an integer of a pointer's size
// whose callee's type returns a pointer, that pointer (Hex-Rays types an indirect call it could not type in its
// microcode `int` and casts the value to the vtable slot's return type: `(NiPoint3*)this->_Allocate(this, n)`, a
// `__struct_ptr` hidden result's `*(_BYTE*)p->isCollisionEnabled(...)`), which the exe returns in eax
tinfo_t valueType(const cexpr_t* e);
// Where a value goes, as src declares it: a resolved call's parameter (byReference: one src takes by reference,
// Hex-Rays passing the pointer to it), the return type (not a reference), the local (not a reference or raw storage),
// src field or global an assignment's left side is. type "" for any other place. (Conversions converts an assignment
// to what its left side prints as, whatever lvalue it is.)
struct Destination
{
	std::string type;
	bool byReference = false;
};
Destination destinationOf(const Function& function, Context& context,
						  const std::unordered_map<const cexpr_t*, const citem_t*>& parents, const cexpr_t* value);
// A pointer's (or reference's) pointee as src types it: its type with typedefs resolved at every level (cv kept below
// its own), and its own const and volatile apart
struct PointeeCv
{
	std::string type;
	bool isConst = false, isVolatile = false;
};
// nullopt for a key that is no pointer or reference
std::optional<PointeeCv> pointeeOf(const SrcTypes& types, const std::string& key);
// two pointer keys to one src type, its own const or volatile apart, the volatile not dropped (`const CHAR*` and
// `char*`): no reinterpretation
bool cvApart(const SrcTypes& types, const std::string& from, const std::string& to);
// a Hex-Rays cast from a pointer to const to the same src pointer without the const (`(char*)"text"`, `(char*)(const
// CHAR*)p`), as cvApart
bool dropsConst(SrcTypes& types, const cexpr_t* cast);
// e writes its operand (x): an assignment (`x = `, `x += `) or an increment or decrement
inline bool written(const cexpr_t* e)
{
	return is_assignment(e->op) || is_prepost(e->op);
}

// A call of a trivial accessor (exefacts' trivialAccessor), decoded: the argument holding the object (`this`: the ecx
// argument of a thiscall; else a stack argument), the offset into it, and whether the field there is loaded (else
// its address is the result); a load's width in bytes and its extension (a narrower load than 4 bytes: 1 movzx, -1
// movsx, 0 `mov al/ax`, which leaves eax's upper bytes to the callee's code), and whether it is an x87 load (the
// result in ST0; eax is then not the result). A setter's: the argument whose low width bytes it stores into the field
// (value; load false, no result).
struct AccessorCall
{
	const cexpr_t* receiver = nullptr;
	int32_t offset = 0;
	bool load = false;
	uint8_t width = 0;
	int8_t extension = 0;
	bool x87 = false;
	const cexpr_t* value = nullptr;
};
// nullopt for any other call (an indirect one, a callee no trivial accessor, an object or value argument the call
// lacks)
std::optional<AccessorCall> accessorCall(Context& context, const cexpr_t* call);
// e does something (EmitNoOps.cpp): an assignment, an increment or decrement, or a call other than a Hex-Rays helper
// that only computes a value from its arguments or a trivial accessor's load or address (NiPointer's operator->)
bool sideEffects(Context& context, const cexpr_t* e);

// --- values (EmitValue.cpp): what an expression reads, as an address expression, for the idioms' shape matching

// A value: base plus a byte offset. base is a local (`v3`, an alias's original), a local's storage (`&v3`) or a load
// (`[v0+12]`).
struct Value
{
	std::string base;
	int64_t offset = 0;

	static Value local(int lvar) { return {"v" + std::to_string(lvar), 0}; }
	std::string key() const { return base + "+" + std::to_string(offset); }
	Value load() const { return {"[" + key() + "]", 0}; }
	Value plus(int64_t n) const { return {base, offset + n}; }
	bool operator==(const Value&) const = default;
};

// The values of one function's expressions.
class Values
{
public:
	Values(const Function& function, Context& context) : m_function(function), m_context(context) {}

	// the value of a local, a load, a cast of one, an address, one plus a constant (scaled by the pointee's size; an
	// integer holding an address, as `*v + 4` for a vtable slot, by bytes), or a call of a trivial accessor whose
	// result is a whole dword in eax; nullopt for anything else
	std::optional<Value> value(const cexpr_t* e) const;
	// the address an lvalue (a local, `*p`, `p->f`, `x.f`) names
	std::optional<Value> location(const cexpr_t* e) const;
	// e's value is expected
	bool is(const cexpr_t* e, const Value& expected) const
	{
		std::optional<Value> v = value(e);
		return v && *v == expected;
	}

protected:
	const Function& m_function;
	Context& m_context;
};

// --- idioms (EmitIdioms.cpp): what the idiom passes share

// e past its casts is the number 0
bool isZero(const cexpr_t* e);
// the one statement of a block (nullptr for another size), or s itself when it is no block
const cinsn_t* soleStatement(const cinsn_t* s);
// e's value, through Hex-Rays' casts, is only tested for zero: a condition (if, while, do, for, the test of `?:`), an
// operand of `!`, `&&` or `||`, or compared with 0 by `==` or `!=`
bool truthTested(const cexpr_t* e, const std::unordered_map<const cexpr_t*, const citem_t*>& parents);
// a statement in (or inside) the statements carries a label, except's own aside
bool labelledInside(const std::vector<const cinsn_t*>& statements, const cinsn_t* except = nullptr);
// a layer left something in e unresolved
bool unresolvedIn(const Function& function, const cexpr_t* e);
// The meanings inside the replaced statements, which an idiom's form does not print: all but those inside the kept
// items (an expression item's subexpressions, a statement item's statements).
void eraseReplaced(Function& function, const std::vector<const cinsn_t*>& replaced,
				   const std::vector<const citem_t*>& kept);

// e and every expression inside it, parents first
template <class F>
void forEachSubexpression(const cexpr_t* e, F&& f)
{
	f(e);
	if (e->op == cot_call)
	{
		forEachSubexpression(e->x, f);
		for (const carg_t& argument : *e->a)
			forEachSubexpression(&argument, f);
		return;
	}
	if (op_uses_x(e->op) && e->x)
		forEachSubexpression(e->x, f);
	if (op_uses_y(e->op) && e->y)
		forEachSubexpression(e->y, f);
	if (op_uses_z(e->op) && e->z)
		forEachSubexpression(e->z, f);
}

// every statement from s down, parents first
template <class F>
void forEachStatement(const cinsn_t* s, F&& f)
{
	f(s);
	for (const cinsn_t* child : childStatements(s))
		forEachStatement(child, f);
}

// every expression of the function, f(expression, the statement holding it)
template <class F>
void forEachExpression(const Function& function, F f)
{
	auto statement = [&](const cinsn_t* s) {
		for (const cexpr_t* own : ownExpressions(s))
		{
			auto each = [&](const cexpr_t* e) { f(e, s); };
			forEachSubexpression(own, each);
		}
	};
	forEachStatement(&function.cfunc->body, statement);
}

// an obj naming the start of a function (Hex-Rays types some as a function, some as a pointer to one)
inline bool functionStart(const cexpr_t* e)
{
	func_t* function = e->op == cot_obj ? get_func(e->obj_ea) : nullptr;
	return function && function->start_ea == e->obj_ea;
}

// the lvar index of a cot_var, -1 for any other expression
inline int lvarIndex(const cexpr_t* e)
{
	return e->op == cot_var ? e->v.idx : -1;
}

// the src parameter a resolved call's argument k is; -1 for the object of a member or the hidden result pointer
// (src's apEdx, which no argument fills, skipped)
inline int parameterIndex(const Call& call, size_t k)
{
	int member = !call.function->cls.empty() && !call.function->isStatic ? 1 : 0;
	if (static_cast<int>(k) < member || static_cast<int>(k) == call.resultArgument)
		return -1;
	int parameter = static_cast<int>(k) - member - (call.resultArgument >= 0 ? 1 : 0);
	return call.edxParameter >= 0 && parameter >= call.edxParameter ? parameter + 1 : parameter;
}

}  // namespace idakit
