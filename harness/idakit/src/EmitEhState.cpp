// EH frame: a cleanup-only C++ EH frame is left out of a body (RULES.md), and with it what Hex-Rays makes of it. The
// frame (exefacts' ehFrame) gives the addresses of its state stores, its registration and its unregistrations. Where
// Hex-Rays recognises the frame it prints only the state, as a local (`v17 = 1; ... v17 = -1;`); where it does not
// (a realigned frame, `and esp, -16` before `push -1`: 0x964260) it prints the registration too (`v113 = -1;
// v112 = 0xF186AB; ExceptionList = NtCurrentTeb()->ExceptionList; NtCurrentTeb()->ExceptionList = &v111;`) and the
// unregistration (`NtCurrentTeb()->ExceptionList = ExceptionList;`).
//
// The frame's statements are the constants stored in a local at a state store (MSVC writes a small state as a byte,
// `mov byte ptr [ebp-4], 1`, which Hex-Rays prints `LOBYTE(v17) = 1`: a byte or word of the local is the local) and
// the assignments at a registration or unregistration instruction. The frame's locals are those that occur in no
// other statement: the state (its `-1` seed included), the saved exception list, the record. A frame statement goes
// when it writes one of them or no local (the TIB's list). A frame with a try/catch keeps everything.

#include "EmitPasses.hpp"

#include <Facts.hpp>

#include <algorithm>
#include <map>
#include <set>
#include <string_view>

namespace idakit
{

namespace
{

// the local an assignment target writes: the local itself, or its low byte or word (`LOBYTE(v17)`); -1 for another
int storedLocal(const cexpr_t* target)
{
	if (target->op == cot_call && target->x->op == cot_helper && target->a->size() == 1 &&
		(std::string_view(target->x->helper) == "LOBYTE" || std::string_view(target->x->helper) == "LOWORD"))
		return lvarIndex(&target->a->at(0));
	return lvarIndex(target);
}

// the assignment statement s is at one of the (ascending) addresses
bool at(const std::vector<uint32_t>& addresses, const cinsn_t* s)
{
	auto holds = [&](ea_t ea) { return std::binary_search(addresses.begin(), addresses.end(), uint32_t(ea)); };
	return holds(s->ea) || holds(s->cexpr->ea);
}

}  // namespace

std::set<const cinsn_t*> ehFrameStatements(Function& function, Context& context)
{
	const std::optional<exefacts::EhFrame>* frame = nullptr;
	try
	{
		frame = &context.exe.ehFrame(uint32_t(function.ea));
	}
	catch (const exefacts::Raised&)
	{
		return {};
	}
	if (!*frame || (*frame)->kind != exefacts::EhFrame::kCxx || (*frame)->tryCount > 0)
		return {};
	const exefacts::EhFrame& eh = **frame;
	std::set<const cinsn_t*> statements;
	forEachStatement(&function.cfunc->body, [&](const cinsn_t* s) {
		if (s->op != cit_expr || s->cexpr->op != cot_asg)
			return;
		const cexpr_t* assignment = s->cexpr;
		if ((storedLocal(assignment->x) >= 0 && assignment->y->op == cot_num && at(eh.stores, s)) ||
			at(eh.registration, s) || at(eh.unregistration, s))
			statements.insert(s);
	});
	// per local that occurs: every occurrence is in a frame statement
	std::map<int, bool> onlyFrame;
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t* s) {
		int v = lvarIndex(e);
		if (v < 0)
			return;
		auto [found, inserted] = onlyFrame.emplace(v, true);
		found->second = found->second && statements.count(s);
	});
	for (const auto& [v, frameLocal] : onlyFrame)
		if (frameLocal)
			function.locals[v].ehFrame = true;
	// the statements writing one of them or no local
	std::set<const cinsn_t*> dropped;
	for (const cinsn_t* s : statements)
	{
		int v = storedLocal(s->cexpr->x);
		if (v < 0 || function.locals[v].ehFrame)
			dropped.insert(s);
	}
	return dropped;
}

void dropEhFrame(Function& function, Context& context)
{
	// the statements are not printed: what the layers found in them (a LOBYTE helper, NtCurrentTeb) goes
	std::set<const cinsn_t*> statements = ehFrameStatements(function, context);
	function.ehFrameStatements.insert(statements.begin(), statements.end());
	eraseReplaced(function, std::vector<const cinsn_t*>(statements.begin(), statements.end()), {});
}

}  // namespace idakit
