// idakit frames, the first step of the locals pass: every game function's sp trace made right, the same whatever ran
// before. The stack points are this pass's, as the user lvar settings built over them are the locals pass's (a
// setting is keyed by a stack offset): a function whose trace it changes loses its settings and is listed, so that
// pdb_locals.py pairs it again. Sync writes neither (Sync.hpp's save check).
//
// The steps run in the order of the members below, between record and changed.
#pragma once

#include "idakit/SrcPush.hpp"

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace idakit
{

class Frames : public SrcPush
{
public:
	// the database must be open, with Hex-Rays (the lvar settings are its store)
	Frames(const SrcModel& model, const ExeVtables& exe, exefacts::Facts& facts, SrcTypes& types)
		: SrcPush("frames", model, exe, facts, types)
	{
	}

	// The sp before each instruction of every game function, kept for changed (Frames.cpp).
	void record();
	// Each game function's purged bytes (func_t.argsize) made the exe's pops (exefacts' pops; a function with no
	// known pops is left), logged `purge-fixed` with both (Frames.cpp). IDA takes the sp change at a call from the
	// callee's purged bytes.
	void purges();
	// Every point deleted and the sp traced again by IDA's analysis, from the code alone, in each game function with
	// a user stack point (IDA's AFL_USERSP mark at a point's address), a frame an `and esp` aligns, or a direct call
	// to a game function whose sp change IDA recorded is not that callee's purged bytes (Frames.cpp): IDA derives its
	// own points around the user ones and records a call's change when it traces the caller, so only a trace from
	// none gives the same points whatever wrote them before.
	void retrace();
	// The stack points of every frame an `and esp` aligns, from a walk of the real esp depths (FramesRealigned.cpp).
	void realignedFrames();
	// The game functions whose sp before an instruction is not record's, each with its user lvar settings cleared and
	// logged `frame-changed` (the instructions moved, the settings cleared), in address order (Frames.cpp).
	std::vector<ea_t> changed();

private:
	// a direct call of a game function in the function whose recorded sp change is not the callee's purged bytes
	bool staleCall(func_t* function, std::string& why);

	// realigned frames (FramesRealigned.cpp)
	void realignedFrame(func_t* function);
	// the real esp depth before each reachable instruction, in IDA's model of the prolog; shaped: the indirect calls
	// given a purge by their shape; why: the reason it cannot
	std::map<ea_t, int64_t> realignedDepths(func_t* function, std::vector<ea_t>& shaped, std::string& why);

	// record's: each game function's instructions with the sp before each
	std::map<ea_t, std::vector<std::pair<ea_t, sval_t>>> m_spd;
};

}  // namespace idakit
