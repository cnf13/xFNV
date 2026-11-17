// The local trials (LocalTrials.hpp): try and measure, the emitter the judge.
//
// The candidates for a local with root spots (one emit each, on top of the settings kept so far):
// - no setting: Hex-Rays' own type, where the database's settings give the local a type;
// - the debug build's: the type a database setting gives another local of the same stack slot (the pairing's,
//   pdb_locals.py, where Hex-Rays has the slot under another definition);
// - each type its occurrences ask (Unfold::askedTypes), of its size, or smaller where no use passes its address on
//   (a callee given its address may write every byte of it);
// - a split at an offset inside a stack local that holds two stack objects merged: where a call takes the address of
//   its bytes at that offset (a constructor or destructor of the second object, Hex-Rays' `&kPick[1].m_pkNext`), or
//   where a database setting names another local there (the debug build's): the local typed as one of its asked
//   types, or the debug build's type of its slot, that ends at or before the offset, else as that many bytes, so that
//   Hex-Rays makes the bytes from the offset another local (typed by the next round's local steps).
// Kept: the candidate with the fewest root spots, where they are fewer than before, every spot it prints (but reviews)
// was printed before, Hex-Rays' numbered names (v12, a3) aside, and its text compiles.
#include "idakit/LocalTrials.hpp"

#include "IdaSupport.hpp"

#include "idakit/Address.hpp"
#include "idakit/Unfold.hpp"

#include <algorithm>
#include <map>
#include <optional>
#include <set>
#include <regex>

namespace idakit
{

namespace
{

// the bounds of one function's trials: a body with many spotted locals (Havok's 0xDEAB10, 120 of them) is tried in
// its most spotted ones
constexpr size_t kTrialsPerFunction = 40;
constexpr size_t kCandidatesPerLocal = 12;

struct Candidate
{
	tinfo_t type;
	bool clear = false;  // no setting's type: Hex-Rays' own
	std::string what;
};

// a local's identity across decompiles: its location and definition address
std::string keyOf(const lvar_locator_t& ll)
{
	std::string where = ll.location.is_stkoff()  ? "stk" + std::to_string(ll.location.stkoff())
						: ll.location.is_reg1() ? "reg" + std::to_string(ll.location.reg1())
												: "other";
	return where + "@" + hexText(ll.defea);
}

size_t roots(const Emitted& emitted)
{
	return static_cast<size_t>(std::count_if(emitted.spots.begin(), emitted.spots.end(),
											 [](const Emitted::Spot& spot) { return spot.root; }));
}

// a spot's text with Hex-Rays' numbered names (v12, a3, v12_1) as one: a retype renumbers them
std::string normalized(const std::string& text)
{
	static const std::regex numbered(R"(\b[va][0-9]+(_[0-9]+)?\b)");
	return std::regex_replace(text, numbered, "v#");
}

// each spot but the reviews, normalized
std::multiset<std::string> spotKeys(const Emitted& emitted)
{
	std::multiset<std::string> out;
	for (const Emitted::Spot& spot : emitted.spots)
		if (!spot.reason.starts_with("review: "))
			out.insert(normalized(spot.reason) + '\t' + normalized(spot.origin) + '\t' + normalized(spot.target));
	return out;
}

// the spots of after that before does not hold (as many times)
std::vector<std::string> freshSpots(std::multiset<std::string> before, const Emitted& after)
{
	std::vector<std::string> fresh;
	for (const std::string& key : spotKeys(after))
		if (auto found = before.find(key); found != before.end())
			before.erase(found);
		else
			fresh.push_back(key);
	return fresh;
}

// the settings with the local's set to the candidate's type (clear: none; a setting left with nothing is dropped)
lvar_uservec_t withCandidate(lvar_uservec_t settings, const lvar_locator_t& local, const Candidate& candidate)
{
	lvar_saved_infos_t& saved = settings.lvvec;
	auto found = std::find_if(saved.begin(), saved.end(), [&](const lvar_saved_info_t& s) { return s.ll == local; });
	if (candidate.clear)
	{
		if (found != saved.end())
		{
			found->type.clear();
			if (found->name.empty() && found->cmt.empty() && found->flags == 0)
				saved.erase(found);
		}
		return settings;
	}
	if (found == saved.end())
	{
		lvar_saved_info_t info;
		info.ll = local;
		saved.push_back(info);
		found = saved.end() - 1;
	}
	found->type = candidate.type;
	return settings;
}

std::optional<int64_t> addressIn(const cexpr_t* e, int v);

// the byte offset in lvar v at which the lvalue e lies (constant member and element offsets); nullopt for none
std::optional<int64_t> offsetIn(const cexpr_t* e, int v)
{
	switch (e->op)
	{
	case cot_var:
		return e->v.idx == v ? std::optional<int64_t>(0) : std::nullopt;
	case cot_memref:
	{
		std::optional<int64_t> at = offsetIn(e->x, v);
		if (!at || e->x->type.is_union())
			return std::nullopt;
		return *at + static_cast<int64_t>(e->m);
	}
	case cot_idx:
	{
		const size_t size = e->type.get_size();
		std::optional<int64_t> at = e->y->op == cot_num ? addressIn(e->x, v) : std::nullopt;
		if (!at || size == BADSIZE)
			return std::nullopt;
		return *at + static_cast<int64_t>(e->y->numval() * size);
	}
	case cot_ptr:
		return addressIn(e->x, v);
	default:
		return std::nullopt;
	}
}

// the byte offset in lvar v the address e points at; nullopt for none
std::optional<int64_t> addressIn(const cexpr_t* e, int v)
{
	switch (e->op)
	{
	case cot_cast:
		return addressIn(e->x, v);
	case cot_ref:
		return offsetIn(e->x, v);
	case cot_var:
		return e->v.idx == v && e->type.is_array() ? std::optional<int64_t>(0) : std::nullopt;
	case cot_add: case cot_sub:
	{
		const size_t size = e->x->type.is_ptr() ? e->x->type.get_pointed_object().get_size() : BADSIZE;
		std::optional<int64_t> at = e->y->op == cot_num ? addressIn(e->x, v) : std::nullopt;
		if (!at || size == BADSIZE)
			return std::nullopt;
		const int64_t step = static_cast<int64_t>(e->y->numval() * size);
		return e->op == cot_add ? *at + step : *at - step;
	}
	default:
		return std::nullopt;
	}
}

// the offsets inside lvar v (0 < offset < width) whose address a call takes
std::set<int64_t> passedInside(cfunc_t& cfunc, int v, int64_t width)
{
	struct Calls : ctree_visitor_t
	{
		int v;
		int64_t width;
		std::set<int64_t> out;
		Calls(int local, int64_t w) : ctree_visitor_t(CV_FAST), v(local), width(w) {}
		int idaapi visit_expr(cexpr_t* e) override
		{
			if (e->op != cot_call)
				return 0;
			for (const carg_t& argument : *e->a)
				if (std::optional<int64_t> at = addressIn(&argument, v); at && *at > 0 && *at < width)
					out.insert(*at);
			return 0;
		}
	} calls(v, width);
	calls.apply_to(&cfunc.body, nullptr);
	return calls.out;
}

// the candidates for lvar v (Candidate order: no setting, the debug build's, asked types, splits)
std::vector<Candidate> candidatesOf(cfunc_t& cfunc, int v, const lvar_uservec_t& current, const lvar_uservec_t& base,
									const Unfold::Asked& asked)
{
	lvar_t& local = (*cfunc.get_lvars())[v];
	const tinfo_t own = local.type();
	const int64_t width = local.width;
	std::vector<Candidate> out;
	auto add = [&](Candidate candidate) {
		if (out.size() >= kCandidatesPerLocal)
			return;
		if (!candidate.clear)
		{
			if (sameType(candidate.type, own) || !local.accepts_type(candidate.type))
				return;
			for (const Candidate& c : out)
				if (!c.clear && sameType(c.type, candidate.type))
					return;
		}
		out.push_back(std::move(candidate));
	};
	const lvar_saved_info_t* setting = nullptr;
	for (const lvar_saved_info_t& s : current.lvvec)
		if (s.ll == local)
			setting = &s;
	if (setting && !setting->type.empty())
		add({tinfo_t(), true, "no setting (Hex-Rays' own type)"});
	// the database's settings of other definitions in the local's stack bytes: of its slot (the debug build's type),
	// or inside it (another object there)
	std::set<int64_t> inside;
	std::optional<tinfo_t> slotType;
	if (local.is_stk_var())
		for (const lvar_saved_info_t& s : base.lvvec)
			if (s.ll.location.is_stkoff() && !(s.ll == local))
			{
				const int64_t offset = s.ll.location.stkoff() - local.location.stkoff();
				if (offset == 0 && !s.type.empty())
				{
					slotType = s.type;
					add({s.type, false, "the debug build's " + text(s.type) + " (" + s.name.c_str() + ")"});
				}
				else if (offset > 0 && offset < width)
					inside.insert(offset);
			}
	for (const tinfo_t& t : asked.types)
	{
		const int64_t size = static_cast<int64_t>(t.get_size());
		if (size == width || (size < width && asked.bounded))
			add({t, false, "asked " + text(t)});
	}
	if (local.is_stk_var())
	{
		for (int64_t offset : passedInside(cfunc, v, width))
			inside.insert(offset);
		for (int64_t offset : inside)
		{
			std::vector<tinfo_t> types;
			if (slotType)
				types.push_back(*slotType);
			types.insert(types.end(), asked.types.begin(), asked.types.end());
			// those ending at the offset first, then the shorter ones
			std::stable_sort(types.begin(), types.end(), [&](const tinfo_t& a, const tinfo_t& b) {
				return (static_cast<int64_t>(a.get_size()) == offset) > (static_cast<int64_t>(b.get_size()) == offset);
			});
			for (const tinfo_t& t : types)
				if (static_cast<int64_t>(t.get_size()) <= offset)
					add({t, false, "split at +" + std::to_string(offset) + " as " + text(t)});
			add({byteArray(offset), false, "split at +" + std::to_string(offset) + " as bytes"});
		}
	}
	return out;
}

std::string jsonString(const std::string& text)
{
	std::string out = "\"";
	for (char c : text)
	{
		if (c == '"' || c == '\\')
			out += std::string("\\") + c;
		else if (static_cast<unsigned char>(c) < 0x20)
		{
			char escaped[8];
			std::snprintf(escaped, sizeof escaped, "\\u%04x", static_cast<unsigned char>(c));
			out += escaped;
		}
		else
			out += c;
	}
	return out + "\"";
}

std::string hexBytes(const qtype& bytes)
{
	static const char kDigits[] = "0123456789abcdef";
	std::string out;
	for (size_t k = 0; k < bytes.length(); ++k)
	{
		out += kDigits[bytes[k] >> 4];
		out += kDigits[bytes[k] & 15];
	}
	return out;
}

std::string cell(std::string text)
{
	std::replace_if(text.begin(), text.end(), [](char c) { return c == '\t' || c == '\n'; }, ' ');
	return text;
}

}  // namespace

LocalTrials::LocalTrials(Context& context, Unfold& unfold, const std::set<std::string>& skipped,
						 std::ostream& trials, std::ostream& settings,
						 std::function<bool(ea_t, const Emitted&)> compiles)
	: m_context(context), m_unfold(unfold), m_skipped(skipped), m_trials(trials), m_settings(settings),
	  m_compiles(std::move(compiles))
{
}

Emitted LocalTrials::run(ea_t ea)
{
	lvar_uservec_t base;
	restore_user_lvar_settings(&base, ea);
	cfuncptr_t cfunc(nullptr);
	Emitted best = emit(ea, m_context, m_unfold, m_skipped, &cfunc);
	if (best.form != Emitted::Form::marked || cfunc == nullptr)
		return best;
	auto row = [&](const std::string& local, const std::string& type, const std::string& candidate, size_t before,
				   const std::string& after, const std::string& verdict) {
		m_trials << hexText(ea) << '\t' << cell(local) << '\t' << cell(type) << '\t' << cell(candidate) << '\t'
				 << before << '\t' << after << '\t' << verdict << '\n';
	};
	// the settings the locals pass writes carry no lvar mappings and no stack offset delta
	if (!base.lmaps.empty() || base.stkoff_delta != 0)
	{
		row("-", "-", "-", roots(best), "-", "skipped: the function's settings hold lvar mappings or a delta");
		return best;
	}
	lvar_uservec_t current = base;
	std::set<std::string> tried;
	size_t trials = 0;
	bool changed = false;
	while (trials < kTrialsPerFunction)
	{
		// the next local: of the untried ones with root spots, the one with most
		std::map<int, size_t> counts;
		const lvars_t& lvars = *cfunc->get_lvars();
		for (const Emitted::Spot& spot : best.spots)
			if (spot.root && spot.local >= 0 && static_cast<size_t>(spot.local) < lvars.size() &&
				!tried.contains(keyOf(lvars[spot.local])))
				++counts[spot.local];
		if (counts.empty())
			break;
		const int v = std::max_element(counts.begin(), counts.end(), [](const auto& a, const auto& b) {
						  return a.second < b.second;
					  })->first;
		const lvar_locator_t local = lvars[v];
		const std::string name = lvars[v].name.c_str(), own = text(lvars[v].type());
		tried.insert(keyOf(local));
		std::map<int, Unfold::Asked> asked = m_unfold.askedTypes(*cfunc);
		std::vector<Candidate> candidates = candidatesOf(*cfunc, v, current, base, asked[v]);
		const size_t before = roots(best);
		if (candidates.empty())
			row(name, own, "-", before, "-", "no candidate (" + std::to_string(counts[v]) + " root spots)");
		const std::multiset<std::string> printed = spotKeys(best);
		std::optional<lvar_uservec_t> keptSettings;
		std::optional<Emitted> kept;
		cfuncptr_t keptCfunc(nullptr);
		for (const Candidate& candidate : candidates)
		{
			if (trials >= kTrialsPerFunction)
				break;
			++trials;
			lvar_uservec_t settings = withCandidate(current, local, candidate);
			save_user_lvar_settings(ea, settings);
			cfuncptr_t decompiled(nullptr);
			Emitted emitted = emit(ea, m_context, m_unfold, m_skipped, &decompiled);
			if (emitted.form == Emitted::Form::none || decompiled == nullptr)
			{
				row(name, own, candidate.what, before, "-", "not printed");
				continue;
			}
			const size_t after = roots(emitted);
			const std::vector<std::string> fresh = freshSpots(printed, emitted);
			const bool fewest = fresh.empty() && after < before && (!kept || after < roots(*kept));
			// only a candidate that would be kept is compiled: the one check that costs seconds
			const bool compiles = fewest && m_compiles(ea, emitted);
			std::string verdict = !fresh.empty()				? std::to_string(fresh.size()) + " new spots: " + fresh[0]
								  : after >= before				? "no fewer root spots"
								  : kept && after >= roots(*kept) ? "fewer, not the fewest"
								  : !compiles					? "does not compile"
																  : "fewest so far";
			row(name, own, candidate.what, before, std::to_string(after), verdict);
			if (fewest && compiles)
			{
				keptSettings = std::move(settings);
				kept = std::move(emitted);
				keptCfunc = decompiled;
			}
		}
		if (kept)
		{
			current = std::move(*keptSettings);
			best = std::move(*kept);
			cfunc = keptCfunc;
			changed = true;
			row(name, own, "-", before, std::to_string(roots(best)), "kept");
		}
	}
	if (trials >= kTrialsPerFunction)
		row("-", "-", "-", roots(best), "-", "trial limit reached");
	// the database holds the kept settings (the last emit's, of the kept ones, unfolded again on the next emit)
	save_user_lvar_settings(ea, current);
	if (!changed)
		return best;
	m_settings << "{\"ea\": " << ea << ", \"lvars\": [";
	bool first = true;
	for (const lvar_saved_info_t& s : current.lvvec)
	{
		std::string where;
		if (s.ll.location.is_stkoff())
			where = "\"stk\": " + std::to_string(s.ll.location.stkoff());
		else if (s.ll.location.is_reg1())
			where = "\"reg\": " + std::to_string(s.ll.location.reg1());
		else
			continue;
		m_settings << (first ? "" : ", ") << '{' << where << ", \"defea\": " << s.ll.defea
				   << ", \"name\": " << jsonString(s.name.c_str()) << ", \"size\": " << static_cast<int64_t>(s.size)
				   << ", \"flags\": " << s.flags;
		qtype type, fields;
		if (!s.type.empty() && s.type.serialize(&type, &fields))
			m_settings << ", \"type\": [\"" << hexBytes(type) << "\", \"" << hexBytes(fields) << "\"]";
		m_settings << '}';
		first = false;
	}
	m_settings << "]}" << std::endl;
	return best;
}

}  // namespace idakit
