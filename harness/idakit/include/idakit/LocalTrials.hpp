// The local trials (idakit emit --try-locals): a marked function's locals with root spots retyped by trial, each
// candidate type written as a user lvar setting in the session, the function emitted again, and the setting kept only
// where emit's root spots go down, no spot appears that was not there and the text compiles. The emitter stays the
// judge of every body: a kept type is no evidence of the exe's types (as Hex-Rays' own are not), only a type under
// which emit proves more. The settings kept are written in the locals pass's form (pdb_locals_ida.exported: one
// function's whole lvar settings per line), for the main session to apply to the canonical database.
#pragma once

#include "idakit/Emit.hpp"

#include <functional>
#include <ostream>
#include <set>
#include <string>

namespace idakit
{

class Unfold;

class LocalTrials
{
public:
	// trials: a row per trial (DIR/local-trials.tsv); settings: a JSON line per function whose settings changed
	// (DIR/local-settings.jsonl); compiles: the emitted text of the function compiles (the command's check)
	LocalTrials(Context& context, Unfold& unfold, const std::set<std::string>& skipped, std::ostream& trials,
				std::ostream& settings, std::function<bool(ea_t, const Emitted&)> compiles);

	// The function emitted (emit), then, where marked, its locals with root spots tried in turn, most spots first:
	// each candidate type set on top of the settings kept so far, the function emitted again, the candidate with the
	// fewest root spots kept where they are fewer than before, every spot it prints was printed before (Hex-Rays'
	// numbered names aside) and its text compiles. The database is left holding the kept settings; returns the emit
	// under them.
	Emitted run(ea_t ea);

private:
	Context& m_context;
	Unfold& m_unfold;
	const std::set<std::string>& m_skipped;
	std::ostream& m_trials;
	std::ostream& m_settings;
	std::function<bool(ea_t, const Emitted&)> m_compiles;
};

}  // namespace idakit
