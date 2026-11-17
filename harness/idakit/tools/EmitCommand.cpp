// idakit emit: functions from their Hex-Rays ctree to the member definitions of their src declarations (Emit.hpp):
// form src where src knows everything they need (`/* review: ... */` notes at most), else marked (each unresolved spot
// under a `/* unresolved: ... */` marker); none where nothing is printed (no ctree, a body src registers, or no
// matching src declaration). Writes only into --out: DIR/emit.tsv (address, form, IDA name, blocker count, the
// blockers joined by " | "), and for each printed function its check file DIR/<ADDR8>.cpp with a DIR/check.list row
// (`ADDR FILE`, for checkbody.py --batch), DIR/src/<ADDR8>.cpp, the definition as printed (what idakit merge puts
// into src/), and a DIR/spots.tsv row per marker in it, in print order (address, root 1 or else 0: nested or a review
// note, reason, origin, target; Emitted::Spot), and DIR/unfold-log.tsv, the log of the functions unfolded (Unfold).
// Each function is unfolded right before its decompile; the database is never saved.
// Run from the repository root (exefacts reads its env/exe).

#include "idakit/Address.hpp"
#include "idakit/Command.hpp"
#include "idakit/Database.hpp"
#include "idakit/Emit.hpp"
#include "idakit/ExeSlotTargets.hpp"
#include "idakit/ExeVtables.hpp"
#include "idakit/LocalTrials.hpp"
#include "idakit/SrcEdit.hpp"
#include "idakit/SrcModel.hpp"
#include "idakit/Unfold.hpp"

#include <Facts.hpp>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <regex>
#include <set>
#include <sqlite3.h>
#include <sstream>

namespace idakit
{

namespace
{

// A definition's check file with its include of the class's src .cpp replaced by that .cpp's text, as the .cpp will
// be once the body is in it: without the forwarder the definition replaces (its lines), or without the whole
// emitter's guarded block (the body emitted before and its #else forwarder) that holds it, and with the check file's
// own `static FNVD_GLOBAL` lines as idakit merge leaves them: one the .cpp has already dropped, one it declares as
// another type dropped with its uses read as their own type (readAs). The lines cut call the address: lines that do
// not (a model older than the .cpp, or a definition that is not this address's) give nothing, and the function is
// not emitted.
std::optional<std::string> inlineSource(const std::string& text, const std::string& path,
										const SrcModel::Definition* replaces, uint64_t ea)
{
	std::string include = "#include \"" + path.substr(path.rfind("src/", 0) == 0 ? 4 : 0) + "\"\n";
	size_t at = text.find(include);
	if (at == std::string::npos)
		return text;
	std::ifstream in(path);
	if (!in)
		throw Error("idakit: cannot read " + path);
	bool cut = replaces && replaces->file == path;
	std::vector<std::string> lines;
	for (std::string line; std::getline(in, line);)
		lines.push_back(line);
	std::optional<LineRange> forwarder;
	if (cut)
	{
		// the guarded block names the address in its registration, past the definition's own lines
		if (std::optional<EmittedBlock> block = emittedBlock(lines, replaces->line))
			forwarder = block->whole;
		if (forwarder)
		{
			std::string block;
			for (int64_t number = forwarder->first; number <= forwarder->last; ++number)
				block += lines[number - 1] + "\n";
			std::regex literal("0x0*" + hexText(ea).substr(2) + "(?![0-9a-f])", std::regex::icase);
			if (!std::regex_search(block, literal))
				return std::nullopt;
		}
		else if (!(forwarder = definitionLines(lines, *replaces, ea)))
			return std::nullopt;
	}
	std::string source;
	std::vector<std::string> kept;
	for (int64_t number = 1; number <= static_cast<int64_t>(lines.size()); ++number)
		if (!forwarder || number < forwarder->first || number > forwarder->last)
		{
			source += lines[number - 1] + "\n";
			kept.push_back(lines[number - 1]);
		}
	std::vector<std::string> after;
	std::istringstream rest(text.substr(at + include.size()));
	for (std::string line; std::getline(rest, line);)
		after.push_back(line);
	// the check file's globals: each its FNVD_GLOBAL line, after its alias line if any
	std::vector<std::vector<std::string>> globals;
	for (size_t k = 0; k < after.size(); ++k)
		if (after[k].starts_with("static FNVD_GLOBAL("))
			globals.push_back(k > 0 && after[k - 1].starts_with("using ") ? std::vector{after[k - 1], after[k]}
																		   : std::vector{after[k]});
	std::vector<EmittedGlobal> redeclared = takeRedeclaredGlobals(globals, kept);
	std::set<std::string> dropped(kept.begin(), kept.end());
	for (const EmittedGlobal& global : redeclared)
		for (const std::string& line : after)
			if (std::optional<EmittedGlobal> g = emittedGlobal(line); g && g->name == global.name)
				dropped.insert(line);
	std::string body;
	for (size_t k = 0; k < after.size(); ++k)
	{
		bool alias = k + 1 < after.size() && after[k].starts_with("using ") && dropped.count(after[k + 1]) &&
					 emittedGlobal(after[k + 1]);
		if ((after[k].starts_with("static FNVD_GLOBAL(") && dropped.count(after[k])) || alias)
			continue;
		body += after[k] + "\n";
	}
	for (const EmittedGlobal& global : redeclared)
		body = readAs(body, global);
	return text.substr(0, at) + source + body;
}

// the allocator table (`address<TAB>name<TAB>size_arg<TAB>scale`, a header row first), by address
std::map<uint64_t, Allocator> readAllocators(const std::string& path)
{
	std::ifstream in(path);
	if (!in)
		throw Error("idakit: --allocators " + path + ": cannot read it");
	std::map<uint64_t, Allocator> out;
	std::string line;
	std::getline(in, line);
	while (std::getline(in, line))
	{
		std::istringstream fields(line);
		std::string address, name, sizeArgument, scale;
		if (!std::getline(fields, address, '\t') || !std::getline(fields, name, '\t') ||
			!std::getline(fields, sizeArgument, '\t') || !std::getline(fields, scale, '\t'))
			throw Error("idakit: --allocators " + path + ": not four columns: " + line);
		out[std::stoull(address, nullptr, 16)] = {name, std::stoul(sizeArgument), std::stoll(scale)};
	}
	return out;
}

// The debug build's locals bound to a parameter (--debug-locals: env/cache/pdb-locals.sqlite, pdb_locals_xbox.py's
// facts), by the PDB function's name: each reference or pointer local its code assigns exactly once, from a
// parameter's value unchanged (`["asg", local, 0, ["v", parameter, 0]]`), with that parameter's name. A name whose
// functions (overloads) bind a local to different parameters keeps none of them.
std::map<std::string, std::map<std::string, std::string>> readBoundLocals(const std::string& path)
{
	sqlite3* db = nullptr;
	std::string uri = "file:" + path + "?immutable=1";
	if (sqlite3_open_v2(uri.c_str(), &db, SQLITE_OPEN_READONLY | SQLITE_OPEN_URI, nullptr) != SQLITE_OK)
	{
		std::string message = db ? sqlite3_errmsg(db) : "out of memory";
		sqlite3_close_v2(db);
		throw Error("idakit: --debug-locals " + path + ": " + message);
	}
	// per function: every assignment to a local of pointer kind (a reference is one), with its source parameter where
	// the whole value assigned is a parameter's
	const char* sql =
		"SELECT f.id, f.name, json_extract(l.value, '$.n'), json_extract(p.value, '$.n') "
		"FROM func f, json_each(f.info, '$.ev') e, json_each(f.info, '$.vars') l "
		"LEFT JOIN json_each(f.info, '$.vars') p ON json_extract(e.value, '$[2]') = 0 "
		"AND json_extract(e.value, '$[3][0]') = 'v' AND json_extract(e.value, '$[3][2]') = 0 "
		"AND p.key = json_extract(e.value, '$[3][1]') AND json_extract(p.value, '$.p') IS NOT NULL "
		"WHERE json_extract(e.value, '$[0]') = 'asg' AND l.key = json_extract(e.value, '$[1]') "
		"AND json_extract(l.value, '$.p') IS NULL AND json_extract(l.value, '$.t.k') = 'ptr' "
		"ORDER BY f.id";
	sqlite3_stmt* statement = nullptr;
	if (sqlite3_prepare_v2(db, sql, -1, &statement, nullptr) != SQLITE_OK)
	{
		std::string message = sqlite3_errmsg(db);
		sqlite3_close_v2(db);
		throw Error("idakit: --debug-locals " + path + ": " + message);
	}
	auto text = [&](int column) {
		const unsigned char* value = sqlite3_column_text(statement, column);
		return std::string(value ? reinterpret_cast<const char*>(value) : "");
	};
	// by function id: each local's assignments' sources ("" for a value that is no parameter's)
	std::map<int64_t, std::pair<std::string, std::map<std::string, std::vector<std::string>>>> functions;
	while (sqlite3_step(statement) == SQLITE_ROW)
	{
		auto& function = functions[sqlite3_column_int64(statement, 0)];
		function.first = text(1);
		function.second[text(2)].push_back(text(3));
	}
	sqlite3_finalize(statement);
	sqlite3_close_v2(db);
	std::map<std::string, std::map<std::string, std::string>> out;
	std::set<std::pair<std::string, std::string>> disagree;
	for (const auto& [id, function] : functions)
		for (const auto& [local, sources] : function.second)
		{
			if (sources.size() != 1 || sources[0].empty())
				continue;
			auto [it, added] = out[function.first].emplace(local, sources[0]);
			if (!added && it->second != sources[0])
				disagree.emplace(function.first, local);
		}
	for (const auto& [name, local] : disagree)
		out[name].erase(local);
	return out;
}

}  // namespace

int runEmit(Args& args)
{
	// Disabled (the user, 2026-11-14): nothing emits bodies any more; a function with no clean body is ported by
	// hand. Only the user takes this guard out.
	std::fprintf(stderr, "idakit emit: disabled (the user, 2026-11-14): bodies are ported by hand\n");
	return 2;

	std::string idb = args.required("idb");
	std::string modelPath = args.required("model");
	std::string aliases = args.required("aliases");
	std::filesystem::path out = args.required("out");
	std::vector<uint64_t> addresses = parseAddresses(args.positional());
	// the passes left out (--skip-pass NAME, repeatable; all: every one), checked before the database opens; a
	// pre-step is never left out
	std::vector<std::string> names = passNames(), preSteps = preStepNames();
	std::set<std::string> skipped;
	for (const std::string& name : args.values("skip-pass"))
	{
		if (name == "all")
			skipped.insert(names.begin(), names.end());
		else if (std::find(names.begin(), names.end(), name) != names.end())
			skipped.insert(name);
		else if (std::find(preSteps.begin(), preSteps.end(), name) != preSteps.end())
			throw Error(name + " is a pre-step and cannot be skipped");
		else
		{
			std::string valid = "all";
			for (const std::string& known : names)
				valid += ", " + known;
			throw Error("idakit: --skip-pass " + name + ": no such pass (" + valid + ")");
		}
	}
	std::filesystem::create_directories(out);

	SrcModel model(modelPath);
	exefacts::Program program(".");
	exefacts::Facts facts(program);
	Database::Options options;
	options.hexrays = true;
	Database database(idb, options);
	Context context(model, aliases, facts);
	// the allocator table (--allocators, C1: env/cache/allocators.tsv), which the Allocations pass reads
	if (std::optional<std::string> allocators = args.option("allocators"))
		context.allocators = readAllocators(*allocators);
	// the debug build's locals bound to a parameter (--debug-locals: env/cache/pdb-locals.sqlite), which the Fields
	// pass's downcast locals read
	if (std::optional<std::string> debugLocals = args.option("debug-locals"))
		context.boundLocals = readBoundLocals(*debugLocals);
	// each function is unfolded in memory right before its decompile; the exe's pops follow vtable tail jumps
	ExeVtables vtables;
	ExeSlotTargets slotTargets(vtables, facts);
	Unfold unfold(model, vtables, facts, context.types);
	// --try-locals: each marked function's spotted locals tried first (LocalTrials), the body printed the one under
	// the settings kept. A candidate's text compiles where its check file (as below) passes
	// `harness/tools/check-src.py --hybrid` (DIR/trials/<ADDR8>.cpp and its .log, the last candidate checked)
	std::ofstream trialRows, trialSettings;
	std::optional<LocalTrials> trials;
	if (args.flag("try-locals"))
	{
		trialRows.open(out / "local-trials.tsv", std::ios::trunc);
		trialSettings.open(out / "local-settings.jsonl", std::ios::trunc);
		trialRows << "address\tlocal\ttype\tcandidate\troot before\troot after\tverdict\n";
		std::filesystem::create_directories(out / "trials");
		auto compiles = [&out](ea_t ea, const Emitted& emitted) {
			std::optional<std::string> source;
			try
			{
				source = inlineSource(emitted.text, emitted.source, emitted.replaces, ea);
			}
			catch (const Error&)
			{
			}
			if (!source)
				return false;
			std::filesystem::path file = std::filesystem::absolute(out / "trials" / (hex8(ea) + ".cpp"));
			std::ofstream(file, std::ios::trunc) << *source;
			std::string command = "env/venv/bin/python harness/tools/check-src.py --hybrid '" + file.string() +
								  "' > '" + file.string() + ".log' 2>&1";
			return std::system(command.c_str()) == 0;
		};
		trials.emplace(context, unfold, skipped, trialRows, trialSettings, compiles);
	}

	std::ofstream table(out / "emit.tsv", std::ios::trunc);
	std::ofstream list(out / "check.list", std::ios::trunc);
	std::ofstream spots(out / "spots.tsv", std::ios::trunc);
	table << "address\tform\tname\tblockers\twhy\n";
	spots << "address\troot\treason\torigin\ttarget\n";
	int counts[3] = {};
	// each function on stderr as it starts (unbuffered), so a run that hangs names the function it hangs in; emit.tsv
	// is flushed row by row, so a killed run keeps the rows before it
	for (size_t k = 0; k < addresses.size(); ++k)
	{
		uint64_t ea = addresses[k];
		std::fprintf(stderr, "emit: %zu/%zu %s\n", k + 1, addresses.size(), hexText(ea).c_str());
		Emitted emitted =
			trials ? trials->run(static_cast<ea_t>(ea)) : emit(static_cast<ea_t>(ea), context, unfold, skipped);
		trialRows.flush();
		std::optional<std::string> source;
		if (emitted.form != Emitted::Form::none)
		{
			try
			{
				source = inlineSource(emitted.text, emitted.source, emitted.replaces, ea);
				if (!source)
					emitted.blockers.push_back("the src definition at " + emitted.replaces->file + ":" +
											   std::to_string(emitted.replaces->line) +
											   " is not this address's forwarder (or the model is older than it)");
			}
			catch (const Error& error)
			{
				emitted.blockers.push_back(error.what());
			}
			if (!source)
				emitted.form = Emitted::Form::none;
		}
		const char* form = emitted.form == Emitted::Form::src		? "src"
						   : emitted.form == Emitted::Form::marked ? "marked"
																   : "none";
		++counts[static_cast<int>(emitted.form)];
		std::string why;
		for (const std::string& blocker : emitted.blockers)
			why += (why.empty() ? "" : " | ") + blocker;
		table << hexText(ea) << '\t' << form << '\t' << emitted.name << '\t' << emitted.blockers.size() << '\t' << why
			  << std::endl;
		if (emitted.form == Emitted::Form::none)
			continue;
		std::filesystem::path file = out / (hex8(ea) + ".cpp");
		std::ofstream(file, std::ios::trunc) << *source;
		list << hexText(ea) << ' ' << std::filesystem::absolute(file).string() << std::endl;
		std::filesystem::create_directories(out / "src");
		std::ofstream(out / "src" / (hex8(ea) + ".cpp"), std::ios::trunc) << emitted.text;
		for (const Emitted::Spot& spot : emitted.spots)
		{
			auto cell = [](std::string text) {
				std::replace_if(text.begin(), text.end(), [](char c) { return c == '\t' || c == '\n'; }, ' ');
				return text;
			};
			spots << hexText(ea) << '\t' << (spot.root ? 1 : 0) << '\t' << cell(spot.reason) << '\t'
				  << cell(spot.origin) << '\t' << cell(spot.target) << '\n';
		}
		spots.flush();
	}
	std::fprintf(stderr, "idakit emit: %zu functions: %d src, %d marked, %d none (no ctree, already in src, or no "
						 "matching src declaration)\n",
				 addresses.size(), counts[0], counts[1], counts[2]);
	unfold.writeLogs(out);
	return 0;
}

}  // namespace idakit
