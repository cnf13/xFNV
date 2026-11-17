// Scopes: a function's statements as a tree (each one's parent), the statements holding each local's uses (an
// alias's counting as its original's; none in the statements rewrites drop, src leaves out or an alias's copy is),
// the locals whose address escapes, and the gotos. Built once by a pass that asks where statements and uses are
// relative to each other.
#pragma once

#include "EmitPasses.hpp"

#include <set>
#include <unordered_map>
#include <vector>

namespace idakit
{

class Scopes
{
public:
	explicit Scopes(const Function& function);

	const cinsn_t* body() const { return m_body; }
	// the statements holding the lvar's uses (an alias's are its original's), in order
	const std::vector<const cinsn_t*>& uses(int lvar) const;
	// the lvar's address (`&v`) goes somewhere other than a resolved call's object, hidden result pointer or
	// reference argument (those are plain uses of v, as src writes them: `v.F()`, `v = F()`, `F(v)`)
	bool addressTaken(int lvar) const { return m_taken.count(lvar) != 0; }
	// the lvar's uses are all in (or inside) the statements
	bool usedOnlyIn(int lvar, const std::vector<const cinsn_t*>& statements) const;

	const cinsn_t* parent(const cinsn_t* s) const;
	// the innermost block holding s (s itself when it is one)
	const cinsn_t* blockOf(const cinsn_t* s) const;
	// s is inside (or is) outer
	bool inside(const cinsn_t* s, const cinsn_t* outer) const;
	// the innermost block holding both
	const cinsn_t* common(const cinsn_t* a, const cinsn_t* b) const;
	// the gotos to a label (by label number)
	std::vector<const cinsn_t*> gotosTo(int label) const;
	// a goto from before item `at` of block (or from outside the block) to a label after it in the block: it would
	// jump past a declaration at that item
	bool jumpedPast(const cinsn_t* block, size_t at) const;
	// `return;`, `return n;` or `return v;` (v a local whose address is never taken): C++ reads the value before a
	// local's destructor runs, the exe after its destructor steps; the same value, since nothing the destructor calls
	// can reach v
	bool plainReturn(const cinsn_t* s) const;
	// a statement leaves the statements: a return, a break or continue not of a loop (or switch) inside them, a goto
	// to a label outside them
	static bool leaves(const std::vector<const cinsn_t*>& statements);
	// the block holding the lvar's uses (it has some), and the indexes of its first and last statements using it
	struct Scope
	{
		const cinsn_t* block = nullptr;
		std::vector<const cinsn_t*> items;
		size_t first = 0, last = 0;
	};
	Scope scopeOf(int lvar) const;

private:
	// printed: no statement from the body down to s is one a rewrite drops or src leaves out (else s holds no uses and
	// no gotos)
	void walk(const cinsn_t* s, const cinsn_t* parent, bool printed);
	// the uses in e as s prints it
	void count(const cexpr_t* e, const cinsn_t* s);
	// the value of e, a local's address, leaves the statement's expression (see addressTaken)
	bool escapes(const cexpr_t* e) const;

	const Function& m_function;
	const cinsn_t* m_body;
	std::unordered_map<const cinsn_t*, const cinsn_t*> m_parents;
	std::unordered_map<const cexpr_t*, const citem_t*> m_expressionParents;
	std::unordered_map<int, std::vector<const cinsn_t*>> m_uses;
	std::set<int> m_taken;
	std::vector<const cinsn_t*> m_gotos;
	std::set<const cinsn_t*> m_dropped;  // not printed
};

// a frame of a block (Function's frames: a block of its own), its items list, holds item first but not item last: a
// declaration at first would end before a use at last
inline bool splitByFrame(const Function& function, const std::vector<const cinsn_t*>& list, size_t first, size_t last)
{
	for (size_t n = 0; n <= first && n < list.size(); ++n)
		if (auto frame = function.frames.find(list[n]);
			frame != function.frames.end() && first < n + frame->second && last >= n + frame->second)
			return true;
	return false;
}

}  // namespace idakit
