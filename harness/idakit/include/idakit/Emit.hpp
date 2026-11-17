// idakit emit: one exe function from its Hex-Rays ctree to C++, the member definition of its src declaration: clean
// when src knows everything it needs, else with each unresolved spot marked.
//
// The IR is the ctree itself (the cfunc, kept alive for the emit) plus a sparse overlay keyed by ctree item: what an
// item means in src, or what a layer looked for and did not find. Types, operands, numbers and addresses are always
// read from the ctree. The layers are passes run in one fixed order (Emit.cpp); the printer prints each item from
// its meaning and decides nothing.
#pragma once

#include "idakit/Ida.hpp"
#include "idakit/SrcModel.hpp"
#include "idakit/SrcTypes.hpp"

#include <map>
#include <optional>
#include <set>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace exefacts
{
class Facts;
}

namespace idakit
{

class Unfold;

// what a layer looked for and did not find (`no src field at hkpEntity+0x1C (4 bytes)`)
struct Missing
{
	std::string what;
};

// A src field a memptr/memref reads, and the named record declaring it (owner). Through a base subobject it is still
// the object's member (`p->field`); qualifier names the class to read it through (`p->Base::field`) where name lookup
// from the object's class finds another member.
struct Member
{
	const SrcModel::Field* field = nullptr;
	std::string owner;
	std::string qualifier;
};

// A src global (declared in a header) that Hex-Rays reads as another type than src declares (what says how): the
// global read at its address as Hex-Rays' type, marked, rather than a second, file-local name for the address.
struct GlobalAs
{
	const SrcModel::Ref* ref = nullptr;
	std::string what;
};

// A call of a declared src function, as src writes it. Hex-Rays has a pointer wherever src has a reference.
struct Call
{
	const SrcModel::Function* function = nullptr;
	bool qualified = false;			// `obj->Class::F()`: a direct call of a src virtual (only a call through the
									// vtable is a virtual call); `::F()`: a free function a member hides
	bool returnsReference = false;  // Hex-Rays' pointer result is src's reference
	std::vector<bool> byReference;  // by argument: Hex-Rays passes a pointer where src takes a reference
	// by argument: the src type it is static_cast to ("": none): a void* for another pointer, an integer with no
	// enumerator for an enum
	std::vector<std::string> castTo;
	// by argument: a same-size integer or enum of another type than src's parameter, which C++ converts by itself
	// (user: stays marked unless proved the original's), the reason convertValues marks its static_cast with ("": none)
	std::vector<std::string> mismatch;
	// the exe calls a body src defines where the caller's TU sees it (clang could inline it): printed inside
	// FNVD_NOINLINE, which is its marker
	bool exeCalled = false;
	// the exe inlines the callee here (a Hex-Rays memset, qmemcpy or strlen Calls resolved to src's msvc9 function;
	// user, 2026-11-12): printed under `// EXE-INLINED <exeInlined>` above its statement ("": none)
	std::string exeInlined;
	// a record returned by value through MSVC's hidden result pointer, Hex-Rays' argument `&local` (after `this` for a
	// member): its index (-1: none). Printed as `local = F(...)`, `&(local = F(...))` where the pointer is read.
	int resultArgument = -1;
	// src's free `R FNVD_FASTCALL f(T* apThis, void* apEdx, ...)` for the exe's thiscall, which Hex-Rays calls
	// without the edx argument (edxParameter): the src parameter no argument fills (-1: none), printed `nullptr`
	int edxParameter = -1;
	// a call through a src field, global or local of function-pointer type (the PointerCalls pass): the callee
	// expression, printed as the call's callee (`m_pfnRead(...)`); nullptr for a call of a named function
	const cexpr_t* through = nullptr;
	// why no cast pins the overload C++ would pick (pinOverloads; "": none) on a call printed from its Call alone,
	// which has no raw form (an inlined local's Construct call, a by-value call with its hidden result folded back):
	// printed as the call, marked with it. Any other call pinOverloads cannot pin becomes a Missing, printed raw.
	std::string unpinned = "";
};

// the enumerators an item's values print as: a number's (or a cast of one), each value of a switch case
struct Enumerators
{
	std::vector<std::string> names;
	std::string type;  // the enum's src type key (its header is included)
	// a number that is no one enumerator of a flag enum but an OR of its flags: `static_cast<E>(A | B)` in the enum's
	// place (C++'s OR of unscoped enumerators is an integer), `A | B` as a bitwise operator's operand, or, as a
	// mask's operand, `~A` / `~(A | B)`; the last two are integers
	enum class Form
	{
		one,
		cast,
		combined,
		complement,
	} form = Form::one;
};

// a conversion C++ makes implicitly: a cast to a base class pointer, the address of a base subobject (`&p->Base`);
// or a cast of a variadic argument to its 4-byte stack slot's integer that passes the operand's dword as it is
// (resolveCalls); printed as its operand (`p`)
struct Implicit
{
};

// a cast that drops a pointee's const where C++ needs it said: printed `const_cast<type>(operand)` (src type key)
struct ConstCast
{
	std::string type;
};

// a pointer cast between arithmetic types of one size (`*(_DWORD *)&f`): the exe reads or writes the same bytes as
// another type; printed `reinterpret_cast<type>(operand)` (src type key) under `// EXE-CAST: from as to` (RULES.md)
struct ExeCast
{
	std::string type;
};

// An explicit conversion where the printed C++ types disagree and C++ converts no further by itself (the value's type
// against an assignment's left side, a parameter, the object a member is called on, the return type, the other side
// of a comparison, an access's pointer): a marked spot, printed around the value with `/* unresolved: what */`.
// constCast `const_cast<type>(v)`; pointerToInteger `static_cast<type>(reinterpret_cast<uintptr_t>(v))`;
// integerToPointer `reinterpret_cast<type>(static_cast<uintptr_t>(v))`; reinterpret `reinterpret_cast<type>(v)`;
// staticCast `static_cast<type>(v)` (both through `const_cast<unconst>(v)` where the value's pointee is const);
// bits `gen::bitcast<type>(v)` (the value's bytes as a type C++ does not convert it to: a record, an SSE vector, a
// gen::Blob); sameBits `std::bit_cast<type>(v)` (the bytes of a fixed-width scalar or compiler vector as another such
// type of its size: the same bits on any layout); lowLane, Hex-Rays' zero-extending cast of a 4-byte value to a
// 16-byte compiler vector (movss, movd from memory): the intrinsic form that builds it, `form` with `$0` for the
// value and `headers` its includes; memberAddress, a record's pointer used as the pointer to its member at offset 0:
// `&v->form` (form the member's path); enumStep, on an increment or decrement of an enum: `v = static_cast<type>(v
// + 1)` (- 1); enumPostStep, on one whose old value is used: `std::exchange(v, static_cast<type>(v + 1))`; byteStep,
// on an increment, decrement, `+=` or `-=` of a pointer to void (C++ steps none; the exe by bytes): `v =
// static_cast<type>(v) + n` (type the pointer to char, its const kept), bytePostStep, on one whose old value is used:
// `std::exchange(v, static_cast<type>(v) + 1)`;
// valueInit `type{}` in place of the value (the exe's zero byte of an empty class's object: C++
// value-initialises it); storeBits, on an assignment to an object of an abstract class (which C++ has no value of):
// `gen::store(x, v)`, the value's bytes stored into it (replacesCast: v is the operand of Hex-Rays' cast of it to
// that class); implicit, on Hex-Rays' cast between integers of one size (replacesCast) whose operand only IDA or
// Hex-Rays types and whose value goes to a src declaration's integer of that size: the operand alone, `v`, C++'s
// implicit conversion (user, 2026-11-12; type the operand's).
struct Conversion
{
	enum class Kind
	{
		constCast,
		pointerToInteger,
		integerToPointer,
		reinterpret,
		staticCast,
		bits,
		sameBits,
		lowLane,
		memberAddress,
		enumStep,
		enumPostStep,
		byteStep,
		bytePostStep,
		valueInit,
		storeBits,
		implicit
	};
	Kind kind = Kind::staticCast;
	std::string type;	  // src type key ("": IDA's type, ida, raw)
	tinfo_t ida;		  // the target's IDA type, spelled where type is empty
	std::string unconst;  // the value's type without its pointee's const ("": none needed)
	bool replacesCast = false;  // on a Hex-Rays cast: converts its operand, in its place
	// reinterpret or staticCast: the type a const_cast after it adds the consts below the first pointee to that no
	// other cast adds (`const_cast<const char**>(reinterpret_cast<char**>(v))`; "": none)
	std::string addConst;
	std::string form;			// lowLane: the intrinsic call, `$0` the value; memberAddress: the member's path
	std::vector<std::string> headers;  // lowLane's includes
	std::string what;
};

// a call of a trivial accessor (exefacts' trivialAccessor) src may not call on its receiver, printed as the access
// itself: `receiver->field` (a load), `&receiver->field` (an address), the receiver (`this`; field null),
// `receiver->field = value` (a setter's store of its argument, value without Hex-Rays' casts). through: the named
// by-value members the field lies in, outermost first (`receiver->data.iRange`); owner: the named record declaring
// the outermost of them (the field itself where there is none); qualifier: the class the outermost name is read
// through where the receiver's class hides it by another member (`receiver->BSTempEffect::fAge`), else empty
struct Access
{
	bool load = false;
	const SrcModel::Field* field = nullptr;
	const cexpr_t* value = nullptr;
	std::vector<const SrcModel::Field*> through;
	std::string owner;
	std::string qualifier;
};

// something a person should look at (an arbitrary literal): printed as it would be, with a `/* review: ... */`
// comment; a note, which never keeps the function marked (user, 2026-11-09)
struct Review
{
	std::string what;
};

// A statement next to FNVD_INLINED_LOCAL's: the vptr store of the constructor or destructor the exe inlines, or
// FNVD_CONSTRUCTED_LOCAL's Construct or Destruct call (macro: the macro does it; not printed apart from the
// declaration it becomes), or a write to the local right after or before such a store, which may be more of their
// inlined work: printed under a `// MAYBE-INLINED 0xADDR` marker (what) for a later sweep.
struct InlinedStatement
{
	bool macro = false;
	std::string what;
};

// An idiom: consecutive statements of one block printed as one piece of src, on the first of them (the next count - 1
// are not printed, and must carry no label). form is the src text, `$0`, `$1`, ... standing for items; a form may
// span lines. An expression item prints as the expression it is (its own meaning applies), in parentheses unless it
// binds as tightly as a postfix expression; `*$N` is the object a pointer item points at (`&x` prints `x`); a
// statement item stands alone on its form line and prints as the
// statements it holds (a block's items, at that line's depth). dropped: statements inside the statement items (or
// elsewhere in the block) the form makes moot, not printed; none carries a label. On an expression, a rewrite is how
// that expression prints: a one-line form (count unused; dropped: statements before it its form makes moot, an
// outgoing argument's constructor or store), of src type type where the pass knows it (an accessor's result; "":
// IDA's); a form that is one expression item alone (`$0`) prints as that item, without parentheses of its own.
// Meanings inside what it replaces still count (an idiom pass erases what its form makes moot).
struct Rewrite
{
	size_t count = 1;
	std::string form;
	std::vector<const citem_t*> items;
	std::vector<const cinsn_t*> dropped;
	std::string type;
	std::string header;  // the src header (relative to src/) declaring what the form names ("": none needed)
};

// A Hex-Rays helper call that is a compiler intrinsic both builds have (an SSE `_mm_*`, `__rdtsc`, a byte swap), or
// the msvc9 math function the exe's site calls (`msvc9::_CIsqrtCall`): the call as Hex-Rays has it, its callee
// spelled name, its arguments of the function's own types, with the header that declares it ("" for a builtin).
struct Intrinsic
{
	std::string name;
	std::string header;
};

// What one ctree item means in src: not looked at by any layer (printed as the ctree has it); a src field (a
// memptr/memref); a call of a declared src function; a src global or static member (an obj); a src type key (a
// cast's, a number's, a base subobject's, the type an assignment or return converts a value to: a void*, a number
// with no enumerator); enumerators; an implicit conversion; a const_cast; a trivial accessor's call printed as its
// access; something to review (printed as it is, with a comment); a statement of an inlined local's; an idiom's
// rewrite (statements); a compiler intrinsic's call; or what a layer looked for and did not find (printed raw and
// marked, never in src form).
using Meaning = std::variant<std::monostate, Member, Call, const SrcModel::Ref*, std::string, Enumerators, Implicit,
							 ConstCast, ExeCast, Access, Review, InlinedStatement, Rewrite, GlobalAs, Intrinsic, Missing>;

// FNVD_INLINED_LOCAL's vtables: the exe vtables (vftable refs) its class's inlined constructor and destructor store;
// nullptr for none. Or FNVD_CONSTRUCTED_LOCAL's calls (construct set): the X::Construct call the declaration makes,
// and whether X::Destruct ends its scope. A local Hex-Rays types as X needs nothing more; one it types as bytes has its
// uses carry the object's key, or its pointer key where the use is its address (Hex-Rays' array decaying to it:
// printed `&v`).
struct InlinedLocal
{
	const SrcModel::Ref* atStart = nullptr;
	const SrcModel::Ref* atEnd = nullptr;
	const cexpr_t* construct = nullptr;
	bool destruct = false;
};

// one Hex-Rays local (an lvar, by its index)
struct Local
{
	std::string type;		 // src type key; empty: unresolved
	std::string why;		 // what is missing when type is empty
	std::string name;		 // the src parameter's name, else Hex-Rays'
	bool reference = false;  // a src reference parameter Hex-Rays has as a pointer
	int parameter = -1;		 // the src parameter it is (its index in self's type, `this` aside); -1: none
	// a parameter the body reassigns or takes the address of (C++'s this is no variable, a reference cannot be
	// rebound): the lvar is a local pointer copied from it at entry (`X* pThis = this;`, `T* pX = &arX;`); the
	// parameter's src type key and name, printed in the head ("this" for this: none)
	struct EntryCopy
	{
		std::string type, name;
	};
	std::optional<EntryCopy> entryCopy;
	// raw storage, always marked (why): IDA's object as raw bytes (a local of src's class would run code the exe
	// inlines; type empty), or bytes Hex-Rays types the local as (type kept) holding an object X::Construct builds
	bool storage = false;
	std::string holds;  // raw storage's object (src type key); its uses as the object carry its key as their meaning
	// declared FNVD_INLINED_LOCAL (at its constructor's vptr store) or FNVD_CONSTRUCTED_LOCAL (at its Construct call)
	std::optional<InlinedLocal> inlined;
	// a copy of another local of the identical type (by lvar index; -1: none): printed as that local, its copy
	// statement (copiedAt) and its declaration dropped
	int aliasOf = -1;
	const cinsn_t* copiedAt = nullptr;
	// a local of a cleanup-only C++ EH frame (left out): its state, the saved exception list, the registration record;
	// every statement it occurs in is the frame's (Function::ehFrameStatements), its declaration is dropped too
	bool ehFrame = false;
	// where it is declared: the statement `v = expr;` that becomes `T v = expr;` (initialised), else the block it is
	// declared at the top of; nullptr for a parameter or an unused local. Every used local starts at the top of the
	// body (emit's default, before the passes), which the passes refine
	const cinsn_t* declaredAt = nullptr;
	bool initialised = false;
	// Hex-Rays reports it possibly undefined (WARN_UNDEF_LVAR), the body reads it and nothing writes it, or a return
	// reads it that some path leaves it unwritten: a read on some path before any write (KEEP by RULES)
	bool undefined = false;
	// declared at the top of its block with this initialiser (`= _mm_undefined_ps()`: VectorLanes), from this header;
	// empty: none
	std::string initialiser, initialiserHeader;
	// declared `alignas(N)`: the exe's frame, realigned by `and esp, -N`, holds it at an N-aligned address its callees
	// see, which src's type alone does not give it (0: none)
	int64_t align = 0;
};

struct Function
{
	cfuncptr_t cfunc{nullptr};
	ea_t ea = BADADDR;
	const SrcModel::Function* self = nullptr;  // its src declaration
	bool defines = false;					   // Self matched self: the body is its definition
	std::string registered;  // where src registers a body at its address already (outside the emitter's guard)
	// self's forwarder in a src .cpp (or the emitter's guarded block holding it), which the definition replaces
	const SrcModel::Definition* replaces = nullptr;
	std::string returnType;					   // src type key of what it returns; empty: unresolved
	bool returnsReference = false;			   // src returns a reference where Hex-Rays returns a pointer
	std::optional<int64_t> pops;			   // the stack bytes it pops (exefacts' pops); nullopt: the exe proves none
	// a constructor or destructor written as a static member (porting rules): "Construct", "Destruct", "DeletingDestruct";
	// empty for any other function. Construct's object (Hex-Rays' first argument) is its local pThis, its first src
	// parameter (`void* apMemory`) is printed in the head and cast to pThis first thing.
	std::string structor;
	// src returns a record by value where the exe fills a hidden result pointer: that pointer's lvar (-1: none), a
	// local of the record printed as the object (`R result;`), every return returning it (clang's NRVO builds it in
	// the caller's slot, as the exe does)
	int resultArgument = -1;
	// a virtual the exe reaches only through the class's secondary vtable for this base subobject (Hex-Rays' `this`
	// is that subobject): registered FNVD_REPLACES_VIRTUAL_VIA; empty for none
	std::string virtualBase;
	// src's free `R FNVD_FASTCALL f(T* apThis, void* apEdx, ...)` for the exe's thiscall (edxParameter): the src
	// parameter Hex-Rays has no argument for (-1: none), printed in the head as src declares it
	int edxParameter = -1;
	// the `template <>` heads its definition takes: one per class template instance its class is or is nested in
	// (none for an explicit specialization of a class template, whose members are defined as any class's), and one
	// for a function template's specialization
	int templateHeads = 0;
	// several src functions answer to its name in its class's scope: its registration names it by its pointer type
	bool overloaded = false;
	// a non-virtual member declared under `protected:`, which FNVD_REPLACES cannot name: registered
	// FNVD_REPLACES_MEMBER (_OVERLOAD for an overload), through a class that makes it public
	bool protectedMember = false;
	// an exe function with register arguments outside thiscall and fastcall (a __usercall/__userpurge src declares
	// through its usercall forwarder): its registration's location list, a member's `this` first, through its last
	// register parameter (`stack, stack, eax`), registered FNVD_REPLACES_USERPURGE where userPurge (it pops its stack
	// arguments), else FNVD_REPLACES_USERCALL; nullopt for any other function
	std::optional<std::string> userLocations;
	bool userPurge = false;
	std::unordered_map<const citem_t*, Meaning> meaning;
	std::vector<Local> locals;			// by lvar index
	// the member names a local or parameter shadows (Shadowing: src writes `this->` there, and nowhere else); nullopt
	// where the pass did not run: every member of `this` keeps `this->`
	std::optional<std::set<std::string>> shadowed;
	std::vector<std::string> blockers;  // function-level reasons it is not src (declaration, rules)
	std::vector<std::string> notes;		// comments above the body (what src leaves out by the rules)
	// a left-out cleanup-only C++ EH frame's statements (its state stores, registration and unregistration)
	std::set<const cinsn_t*> ehFrameStatements;
	// statements that do nothing src would write: calls (a trivial accessor's whose result is unused), and a vtable
	// slot's load into a local that the calls through it read again (resolveCalls)
	std::set<const cinsn_t*> noOps;
	// the explicit conversions of values whose printed types disagree with where they go (marked spots)
	std::unordered_map<const cexpr_t*, Conversion> conversions;
	// calls returning a record by value whose result is C++'s temporary where it is used (foldHiddenResults): printed
	// `F(...)`, without the local Hex-Rays has it stored in
	std::set<const cexpr_t*> temporaries;
	// calls returning a byte-copied record by value into a whole object built before (a pointer's pointee, a local, a
	// reference's referent), each a statement of its own: printed `::new (p) T(F(...))` (placeDeclarations), which
	// passes the object itself as the hidden result pointer, as the exe does, where `*p = F(...)` passes a temporary
	std::set<const cexpr_t*> placements;
	// statements printed in a block of their own (`{ ... }`), by the first: how many of its block's statements from it
	// on (a local's lifetime whose destructor steps the exe does before its block ends; resolveStructorLocals)
	std::map<const cinsn_t*, size_t> frames;
	// floating constants whose value is the exe's double operand where Hex-Rays has it rounded to a float (a qword
	// 0.2 compared with a float shows as 0.2000000029802322; resolveFloatConstants): printed as the exe's value
	std::unordered_map<const cexpr_t*, double> floatValues;
	// the calls whose user-defined call (Hex-Rays' helper call, its callee made the call instruction's target again
	// before the passes) names src functions of its type (SrcPush::calledAs: a fold owner's), with those functions:
	// Calls resolves each to its one function, never to another, and leaves one naming several unresolved
	std::unordered_map<const cexpr_t*, std::vector<const SrcModel::Function*>> namedCallees;
	// the globals no src global resolves that are read by their address (resolveGlobals): an array, a C string, or an
	// array IDA's type at the address declares that Hex-Rays reads as the pointer to its first element, where it
	// decays, each with its element type: printed as that element's object's address, `(&g_X)`
	std::unordered_map<const cexpr_t*, tinfo_t> addressedGlobals;
	// The debug build's reference or pointer locals that hold a parameter cast to a class derived from the parameter's
	// (the Fields pass's downcast locals: `const NiSphereBV& kSphABV`, which the PC exe keeps in no slot of its own):
	// declared first thing in the body, `type name = static_cast<type>(parameter);` (`*parameter` for a reference
	// local of a pointer parameter, `&parameter` for a pointer local of a reference one), parameter the lvar index
	struct Downcast
	{
		std::string name, type;
		int parameter = -1;
	};
	std::vector<Downcast> downcasts;

	// the aliases' copy statements (not printed)
	std::set<const cinsn_t*> copies() const;
	// the statements the rewrites drop (Rewrite::dropped; not printed)
	std::set<const cinsn_t*> dropped() const;
	// a statement src leaves out (an EH frame's, a no-op call): not printed, not checked
	bool omitted(const cinsn_t* s) const { return ehFrameStatements.count(s) || noOps.count(s); }
	// local k's declaration placed by a pass (an inlined or structor local's statement, placeDeclarations' block),
	// not the default at the top of the body every used local starts with
	bool placed(size_t k) const { return locals[k].declaredAt && locals[k].declaredAt != &cfunc->body; }
};

// An allocator or a wrapper of one (env/cache/allocators.tsv, emit's --allocators; `idakit allocators` writes it): the
// call argument holding the size (Hex-Rays' index; a thiscall's this is 0), and the bytes per unit of it (1 for a size,
// the element's size for a count). name: IDA's name of the function.
struct Allocator
{
	std::string name;
	size_t sizeArgument = 0;
	int64_t scale = 1;
};

// what every pass reads
struct Context
{
	// aliases: harness/tools/ida_type_aliases.tsv (SrcTypes)
	Context(const SrcModel& model, const std::string& aliases, exefacts::Facts& exe)
		: model(model), types(model, aliases), exe(exe)
	{
	}

	const SrcModel& model;
	SrcTypes types;
	exefacts::Facts& exe;  // what the exe's code says (harness/exefacts)
	// a src global of function-pointer type the exe calls through (an import's slot), as the function a Call names (the
	// global's name, header and pointee's fn type; no mangled name): the Calls pass's, kept for the run
	std::map<const SrcModel::Ref*, SrcModel::Function> calledGlobals;
	// a src field's, global's or local's function-pointer type the exe calls through, as the function a Call names (no
	// name, file or mangled name; the type's fn key): the PointerCalls pass's, kept for the run
	std::map<std::string, SrcModel::Function> calledPointers;
	// the allocators by address (empty: none given): the Allocations pass's
	std::map<uint64_t, Allocator> allocators;
	// the debug build's locals bound to a parameter (emit's --debug-locals; empty: none given), by the PDB function's
	// name: each local's parameter, by name. The Fields pass's downcast locals
	std::map<std::string, std::map<std::string, std::string>> boundLocals;
};

struct Emitted
{
	enum class Form
	{
		src,	 // the member's definition, nothing unresolved (review notes at most): src's once merged
		marked,	 // the member's definition with its unresolved spots marked: merged hybrid-only, emitted again
		none	 // nothing printed: no ctree, src registers a body at the address already, or Self did not match
	};
	// a marker the text holds: root unless it is printed inside the raw text of another item's marker (a local's is a
	// root always, a review's never: a note, no blocker); reason: the marker's text, `review: ...` for a review;
	// origin and target: where the marked value's type comes from and where the value goes (the printer's Provenance;
	// `-` for a review's and a local's); local: the lvar (an index into the decompile's lvars) whose declaration is
	// marked, else the one the marked value comes from (origin's local) or is stored into, -1 for none (an argument
	// is none)
	struct Spot
	{
		bool root;
		std::string reason;
		std::string origin;
		std::string target;
		int local = -1;
	};
	Form form = Form::none;
	std::string name;					// IDA's name of the function
	std::string text;					// the member's definition, registered FNVD_REPLACES: what src gets
	std::vector<Spot> spots;			// the text's markers, in print order
	std::vector<std::string> blockers;  // what is unresolved (none for src), or why nothing is printed
	// the class's src .cpp the definition's check file includes ("" for none), and the forwarder (or the emitter's
	// guarded block) in it the definition replaces: the check file takes that .cpp's text without it
	std::string source;
	const SrcModel::Definition* replaces = nullptr;
};

// The passes' names, in their order (Emit.cpp's kPasses).
std::vector<std::string> passNames();
// The pre-steps' names (Emit.cpp's kPreSteps): every emit runs them, so --skip-pass refuses them.
std::vector<std::string> preStepNames();

// The function starting at ea, emitted (the database open with Hex-Rays), without the passes named in skipped, from
// the decompile unfold's rebuild of it ends with (the function unfolded first, in memory); decompiled, where given,
// gets that decompile (null where there is none).
Emitted emit(ea_t ea, Context& context, Unfold& unfold, const std::set<std::string>& skipped = {},
			 cfuncptr_t* decompiled = nullptr);
// The function starting at ea as emit reads it before printing: unfolded, decompiled, the pre-steps and the passes
// (but those in skipped) run; false with why where there is no function or ctree.
bool analyze(ea_t ea, Context& context, Unfold& unfold, Function& function, std::string& why,
			 const std::set<std::string>& skipped = {});

}  // namespace idakit
