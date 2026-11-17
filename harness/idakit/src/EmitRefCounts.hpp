// The exe's inlined NiRefObject reference counting (NiRefCount.h's NiIncRefCount / NiDecRefCount: the atomic count
// at +4, DeleteThis in vtable slot 1 at zero), matched by value (EmitValue.cpp) for the NiPointer idioms; and the
// general match of Hex-Rays' statements against a src inline function's steps (srcdb's steps rows).
#pragma once

#include "EmitPasses.hpp"

namespace idakit
{

class RefCounts : public Values
{
public:
	using Values::Values;

	// `if (!InterlockedDecrement(&old->m_uiRefCount)) receiver->DeleteThis();` as its test and branch (receiver: old,
	// or Hex-Rays' copy of it)
	bool decrement(const cexpr_t* test, const cinsn_t* then, const Value& old, const Value& receiver) const;
	// `t = old;`: t's lvar, else -1
	int copyOf(const cinsn_t* s, const Value& old) const;
	// NiDecRefCount under `if (old)`: nested ifs, or one `&&`; DeleteThis called on old, or on copy (an lvar holding
	// old, -1: none); where temporary is given and there is no copy, also with Hex-Rays' copy `t = old;` first in the
	// branch, DeleteThis called on t (*temporary: t's lvar)
	bool release(const cinsn_t* s, const Value& old, int* temporary = nullptr, int copy = -1) const;
	// NiIncRefCount under `if (p)`: `if (p) InterlockedIncrement(&p->m_uiRefCount);`, either p being same where given (a
	// copy holding the same pointer: the NiPointer's field it was just stored to)
	bool hold(const cinsn_t* s, const Value& p, const std::optional<Value>& same = std::nullopt) const;

private:
	// a direct call of the import name with one argument, the address `at`
	bool importCall(const cexpr_t* e, const char* name, const Value& at) const;
	// `old->DeleteThis()`: the call through slot 1 of old's vtable, old as this
	bool deleteThis(const cinsn_t* s, const Value& old) const;
};

// A src function's inline steps (srcdb's steps row; its language: srcdb's Model.hpp) as a tree: an atom (op empty:
// `this`, `pN`, `#N`, a let's name `@...`, a local's address `&vN` or value `vN`), or an op with its operands (`call`:
// the callee's key in atom).
struct Step
{
	std::string op;
	std::string atom;
	std::vector<Step> kids;

	// the steps text parsed (nullopt: not the language)
	static std::optional<Step> parse(const std::string& text);
	// the steps of a call with these values for the atoms they name (`this`, `pN`): each a let before the steps, so
	// that it is evaluated once, before them, as C++ evaluates a call's arguments
	Step bind(const std::unordered_map<std::string, Step>& bound) const;
	std::string text() const;
};

// Hex-Rays' statements matched against a src function's inline steps: both written in the step language, then
// brought to one form (Hex-Rays' copies of locals as lets, nested blocks and ifs flattened, `p && q` as nested ifs, a
// counter changed in an if's test as its own step first, each load tagged with the version of memory it reads so
// that a read after a write never equals one before it, a stored value read back as that value until the next write,
// a step whose value is unused in its one form, a compare's operands in one order, the Win32 Interlocked imports as
// the atomic builtins src writes for them), and compared. A cast Hex-Rays prints is in the language only where it
// keeps the value's bits (pointers and integers of one size).
class Steps : public Values
{
public:
	using Values::Values;

	// e in the step language (nullopt: outside it); copies: the let names of the locals Hex-Rays copied values into
	std::optional<Step> lower(const cexpr_t* e, const std::unordered_map<int, Step>& copies = {}) const;
	// the statements do exactly the steps; *copies: the locals a statement there only copies a value into (Hex-Rays'
	// temporaries, which the caller checks are used nowhere else), not `except`; known: locals the caller knows hold
	// a value at the first statement
	bool match(const std::vector<const cinsn_t*>& statements, const Step& steps, int except,
			   std::vector<int>* copies, const std::unordered_map<int, Step>& known = {}) const;
	// the address the lvalue e names in the step language (nullopt: outside it)
	std::optional<Step> address(const cexpr_t* e, const std::unordered_map<int, Step>& copies = {}) const;

private:
	std::optional<Step> statement(const cinsn_t* s, std::unordered_map<int, Step>& copies, int except) const;
};

}  // namespace idakit
