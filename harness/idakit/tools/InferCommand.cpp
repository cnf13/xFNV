// idakit infer: the slot types of functions src declares with IDA's guesses, from the exe's evidence (Infer.hpp;
// docs/IDAKIT.md "idakit infer"). Each target and each function calling one is analysed as emit analyses it (unfolded,
// decompiled, the passes run; never saved), its facts read, and the rounds solved. Writes DIR/infer.tsv (a row per
// slot: address, function, mangled, slot, src's type, the proposal, verdict proven/guessed/callers-only/open/inferred
// (src's `// Inferred type` clause has it)/note, round, evidence; with --hide also the slot's kind and truth: the
// proposal against the declaration's own type, equal/chain/wrong) and DIR/retype.tsv, a row per proven slot and per
// guessed slot whose type changes, in the table `idakit declare --retype` reads (id `<mangled>#<slot>`, verdict apply
// where the type changes, keep where the guess is confirmed, open where the change fails SlotRowChecks (a virtual
// family not changed whole, an overload made another's), the key, the function's one `// Inferred type: ...` comment
// naming each proven slot written or its `// Guessed type: ...` comment naming each guessed one, and the tier).
// Run from the repository root (exefacts reads its env/exe).

#include "idakit/Address.hpp"
#include "idakit/Command.hpp"
#include "idakit/Database.hpp"
#include "idakit/ExeSlotTargets.hpp"
#include "idakit/ExeVtables.hpp"
#include "idakit/Infer.hpp"
#include "idakit/SrcIda.hpp"
#include "idakit/Unfold.hpp"

#include "DeclarePdb.hpp"

#include <Facts.hpp>

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>

namespace idakit
{

namespace
{

// the direct calls of a function: the functions they are in
std::set<uint64_t> callers(ea_t function)
{
	std::set<uint64_t> out;
	xrefblk_t xb;
	for (bool ok = xb.first_to(function, XREF_NOFLOW); ok; ok = xb.next_to())
	{
		insn_t insn;
		if (!xb.iscode || decode_insn(&insn, xb.from) <= 0 || insn.itype != NN_call)
			continue;
		if (func_t* caller = get_func(xb.from))
			out.insert(caller->start_ea);
	}
	return out;
}

// The proposal in src's form: the exe shows neither a reference from a pointer nor constness, so a pointer proposed
// for a slot src types as a pointer or reference keeps src's choice of both (`const Script&` proven TESForm* is
// `const TESForm&`); any other proposal as it is
std::string srcForm(const SrcTypes& types, const std::string& current, const std::string& proposed)
{
	SrcTypes::Parts was = types.parts(types.canonical(current)), now = types.parts(types.canonical(proposed));
	if (now.kind != "ptr" || (was.kind != "ptr" && was.kind != "lref"))
		return proposed;
	SrcTypes::Parts pointee = types.parts(now.of);
	std::string of = pointee.kind == "const" ? pointee.of : now.of;
	return was.kind + "(" + (types.parts(was.of).kind == "const" ? "const(" + of + ")" : of) + ")";
}

}  // namespace

int runInfer(Args& args)
{
	std::string idb = args.required("idb");
	std::string modelPath = args.required("model");
	std::string aliases = args.required("aliases");
	std::filesystem::path out = args.required("out");
	bool hide = args.flag("hide");
	std::vector<uint64_t> addresses = parseAddresses(args.positional());
	if (hide && addresses.empty())
		throw Error("idakit infer: --hide needs the addresses to hide");
	std::filesystem::create_directories(out);

	SrcModel model(modelPath);
	exefacts::Program program(".");
	exefacts::Facts facts(program);
	Database::Options options;
	options.hexrays = true;
	Database database(idb, options);
	Context context(model, aliases, facts);
	ExeVtables vtables;
	ExeSlotTargets slotTargets(vtables, facts);
	Unfold unfold(model, vtables, facts, context.types);

	// the targets: the addresses given (each one's declaration, hidden or a guess), else every guess-declared one
	std::set<uint64_t> hidden;
	if (hide)
		hidden.insert(addresses.begin(), addresses.end());
	Infer infer(context, hidden);
	std::vector<uint64_t> wanted = addresses;
	if (wanted.empty())
		for (const auto& [address, refs] : model.refs())
			wanted.push_back(address);
	std::vector<uint64_t> targets;
	size_t skipped = 0;
	for (uint64_t address : wanted)
	{
		RefChoice choice = chooseRef(model, address);
		const SrcModel::Function* f = choice.ref && !choice.structor && (choice.names.size() <= 1)
										  ? model.function(choice.ref->function)
										  : nullptr;
		func_t* pfn = get_func(static_cast<ea_t>(address));
		if (!f || !pfn || pfn->start_ea != address || (!hide && f->provenance != SrcModel::Provenance::Guess) ||
			(choice.ref->kind != "body" && choice.ref->kind != "forwarder" && choice.ref->kind != "address"))
		{
			skipped += !addresses.empty();
			continue;
		}
		infer.target(address, *f);
		targets.push_back(address);
	}
	// the functions to read: the targets and their callers
	std::set<uint64_t> read(targets.begin(), targets.end());
	for (uint64_t address : targets)
	{
		std::set<uint64_t> calling = callers(static_cast<ea_t>(address));
		read.insert(calling.begin(), calling.end());
	}
	std::fprintf(stderr, "idakit infer: %zu targets (%zu addresses skipped), %zu functions to read\n",
				 targets.size(), skipped, read.size());
	size_t k = 0, failures = 0;
	for (uint64_t address : read)
	{
		std::fprintf(stderr, "infer: %zu/%zu %s\n", ++k, read.size(), hexText(address).c_str());
		Function function;
		std::string why;
		if (!analyze(static_cast<ea_t>(address), context, unfold, function, why))
		{
			++failures;
			if (infer.isTarget(address))
				infer.failed(address, why);
			continue;
		}
		infer.read(function);
	}

	std::vector<Infer::Row> rows = infer.solve(hide);
	auto spelled = [&](const std::string& key) {
		if (key.empty())
			return std::string();
		std::optional<std::string> s = context.types.spell(key);
		return s ? *s : key;
	};
	std::ofstream table(out / "infer.tsv", std::ios::trunc);
	table << "address\tfunction\tmangled\tslot\tcurrent\tproposed\tverdict\tround\tevidence"
		  << (hide ? "\tkind\ttruth" : "") << '\n';
	std::map<std::string, int> counts;
	std::vector<SlotRow> slots;
	std::map<std::string, std::string> clauses;  // a slot row's id -> its clause of the comment
	std::set<std::string> guessed;				   // the slot rows of guessed slots
	for (const Infer::Row& row : rows)
	{
		++counts[row.verdict];
		table << hexText(row.address) << '\t' << cell(row.function) << '\t' << row.mangled << '\t' << row.slot << '\t'
			  << cell(spelled(row.current)) << '\t' << cell(spelled(row.proposed)) << '\t' << row.verdict << '\t'
			  << row.round << '\t' << cell(row.evidence);
		if (hide)
			table << '\t' << row.kind << '\t' << row.truth;
		table << '\n';
		// each proven slot (callers-only and open ones stay IDA's guess): the type's change, or the guess confirmed;
		// each guessed slot's change (one confirming src's guess adds nothing)
		if (row.verdict != "proven" && row.verdict != "guessed")
			continue;
		std::string key = srcForm(context.types, row.current, row.proposed);
		bool same = context.types.canonical(row.current) == context.types.canonical(key);
		if (row.verdict == "guessed" && same)
			continue;
		SlotRow& s = slots.emplace_back();
		s.id = row.mangled + "#" + row.slot;
		s.address = hexText(row.address);
		s.function = row.function;
		s.mangled = row.mangled;
		s.slot = row.slot;
		s.src = spelled(row.current);
		s.key = key;
		s.proposed = spelled(s.key);
		s.verdict = same ? "keep" : "apply";
		clauses[s.id] = row.clause;
		if (row.verdict == "guessed")
			guessed.insert(s.id);
	}
	// no change narrows a width the exe keeps (a raw byte returned, a dword passed on whole); a virtual's change goes in
	// for its whole family, each member's slot proven alike (a member's own guess is no evidence of the family's
	// type), and none makes its function another overload of its name: else the row is open, with the reason, and its
	// clause is no part of the comment
	SlotRowChecks checks(model, context.types, facts);
	checks.judgeWidths(slots);
	checks.judgeFamilies(slots, [&](const SrcModel::Function& member, const std::string& slot, const std::string& own) {
		return member.name + "'s slot " + slot + " is " + spelled(own) + ", not proven so";
	});
	checks.judgeOverloads(slots);
	// a function's comments: `// Inferred type: ` and its proven slots' clauses, separated by `; ` (srcdb's form),
	// and `// Guessed type: ` and its guessed slots' clauses (RULES "Evidence for declarations and types": no
	// evidence, which srcdb does not read as clauses)
	std::map<std::string, std::string> comments, guesses;
	for (const SlotRow& s : slots)
		if (s.verdict != "open")
		{
			bool guess = guessed.count(s.id) != 0;
			std::string& c = (guess ? guesses : comments)[s.mangled];
			c += (c.empty() ? guess ? "// Guessed type: " : "// Inferred type: " : "; ") + clauses.at(s.id);
		}
	std::ofstream retype(out / "retype.tsv", std::ios::trunc);
	retype << "id\taddress\tfunction\tslot\tverdict\treason\tcurrent\tproposed\tkey\tfamily\tcomment\ttier\n";
	std::map<std::string, int> written;
	for (const SlotRow& s : slots)
	{
		++written[s.verdict];
		retype << s.id << '\t' << s.address << '\t' << cell(s.function) << '\t' << s.slot << '\t' << s.verdict << '\t'
			   << cell(s.reason) << '\t' << cell(s.src) << '\t' << cell(s.proposed) << '\t' << s.key << '\t'
			   << s.family << '\t'
			   << (s.verdict == "open" ? ""
				   : guessed.count(s.id) ? cell(guesses.at(s.mangled))
										 : cell(comments.at(s.mangled)))
			   << '\t' << (guessed.count(s.id) ? "guessed" : "proven") << '\n';
	}
	std::fprintf(stderr, "idakit infer: %zu rows:", rows.size());
	for (const auto& [verdict, count] : counts)
		std::fprintf(stderr, " %d %s", count, verdict.c_str());
	std::fprintf(stderr, "; %zu functions not read; retype.tsv:", failures);
	for (const auto& [verdict, count] : written)
		std::fprintf(stderr, " %d %s", count, verdict.c_str());
	std::fprintf(stderr, "\n");
	unfold.writeLogs(out);
	return 0;
}

}  // namespace idakit
