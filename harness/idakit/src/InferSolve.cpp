// idakit infer's solver (Infer.hpp): each slot's bounds evaluated in a round (the sources' and destinations' types
// from evidence declarations and the slots earlier rounds proved), then solved by the evidence rules
// (env/scratch/plans/src-true.md "Evidence rules"; docs/IDAKIT.md "idakit infer").

#include "idakit/Infer.hpp"

#include "idakit/Address.hpp"
#include "idakit/SrcIda.hpp"

#include <algorithm>
#include <functional>

namespace idakit
{

namespace
{

std::string slotName(int slot)
{
	return slot == Infer::kReturn ? "ret" : slot == Infer::kThis ? "this" : std::to_string(slot);
}

// an IDA placeholder name of a field (no name src or the PDB gave it)
bool placeholderField(const std::string& name)
{
	return name.starts_with("unk") || name.starts_with("field_") || name.starts_with("pad") || name.starts_with("_");
}

}  // namespace

// a slot's type from one place: from below (a value passed or returned) or above (where the value goes, plus adjust
// bytes), where it was read and what it rests on
struct Infer::Bound
{
	std::string key;
	int64_t adjust = 0;
	uint64_t at = 0;
	std::string rests;
};

struct Infer::Solution
{
	std::string key, verdict, evidence, comment, truth;
};

bool Infer::evidence(uint64_t address, const SrcModel::Function* function, int slot) const
{
	if (!function || m_hidden.count(address))
		return false;
	// a fold's code is several functions', its declaration one of theirs
	if (folded(address))
		return false;
	// a target's slot only as its `// Inferred type` clause has it, the others being what this run infers
	if (m_targets.count(address))
		return slot != kThis && function->inferredSlots.count(slotName(slot)) != 0;
	return function->slotEvidence(slotName(slot));
}

bool Infer::folded(uint64_t address) const
{
	auto [found, added] = m_folded.try_emplace(address, false);
	if (added)
	{
		const SrcModel& model = m_context.model;
		const std::vector<SrcModel::Ref>& refs = model.refsAt(address);
		found->second = chooseRef(model, address).names.size() > 1 ||
						std::any_of(refs.begin(), refs.end(), [](const SrcModel::Ref& r) { return r.kind == "fold"; });
	}
	return found->second;
}

std::string Infer::slotType(uint64_t address, const SrcModel::Function* function, int slot) const
{
	if (m_targets.count(address) && !evidence(address, function, slot))
	{
		auto found = m_proven.find({address, slot});
		return found != m_proven.end() && found->second.round < m_round ? found->second.key : "";
	}
	if (!evidence(address, function, slot))
		return "";
	const SrcModel::Type* type = m_context.model.type(function->type);
	if (!type)
		return "";
	// a reference is the pointer Hex-Rays passes
	auto value = [&](const std::string& key) {
		SrcTypes::Parts p = m_context.types.parts(m_context.types.bare(key));
		return p.kind == "lref" ? "ptr(" + p.of + ")" : key;
	};
	if (slot == kThis)
	{
		if (function->cls.empty() || function->isStatic)
			return "";
		std::string cls = entryClass(m_context.model, *function);
		return cls.empty() ? "" : "ptr(record:" + cls + ")";
	}
	if (slot == kReturn)
	{
		const SrcModel::Abi* abi = m_context.model.abi(function->mangled);
		if (abi && abi->ret == "indirect")
			return "";
		return value(type->ret);
	}
	if (slot < 0 || static_cast<size_t>(slot) >= type->params.size())
		return "";
	return value(type->params[slot]);
}

namespace
{

// the src function an address's calls and parameters are typed by: a target's declaration, else chooseRef's
const SrcModel::Function* declarationAt(const SrcModel& model, uint64_t address)
{
	RefChoice choice = chooseRef(model, address);
	if (!choice.ref || (choice.names.size() > 1 && choice.ref->kind != "body"))
		return nullptr;
	return model.function(choice.ref->function);
}

}  // namespace

std::string Infer::typeOf(const Source& source, std::string& rests, int depth) const
{
	const SrcModel& model = m_context.model;
	SrcTypes& types = m_context.types;
	if (depth > 8)
		return "";
	auto declaration = [&](uint64_t address) -> const SrcModel::Function* {
		auto found = m_targets.find(address);
		return found != m_targets.end() ? found->second.declaration : declarationAt(model, address);
	};
	auto named = [&](uint64_t address) {
		const SrcModel::Function* f = declaration(address);
		return f ? f->name : hexText(address);
	};
	// the record a pointer key points to ("" for none)
	auto pointee = [&](const std::string& key) -> std::string {
		SrcTypes::Parts p = types.parts(types.canonical(key));
		if (p.kind != "ptr")
			return "";
		SrcTypes::Parts of = types.parts(p.of);
		return of.kind == "record" ? of.name : "";
	};
	std::string key;
	switch (source.kind)
	{
	case Source::Kind::none:
	case Source::Kind::number:
		return "";
	case Source::Kind::parameter:
		key = slotType(source.function, declaration(source.function), source.slot);
		rests = "parameter " + std::to_string(source.slot) + " of " + named(source.function);
		break;
	case Source::Kind::self:
		key = slotType(source.function, declaration(source.function), kThis);
		rests = "this of " + named(source.function);
		break;
	case Source::Kind::result:
	{
		const SrcModel::Function* callee = declaration(source.function);
		// a constructor's object is its class's only where the code stores the class's vtable: one without may be
		// the linker's fold of another class's constructor of the same code (NiPoint3's and NiColor's)
		if (callee && structorName(callee->name) == "Construct")
			if (const SrcModel::Record* record = model.record(callee->cls); !record || record->vtables.empty())
				return "";
		key = slotType(source.function, callee, kReturn);
		rests = "result of " + named(source.function);
		break;
	}
	case Source::Kind::field:
	case Source::Kind::address:
	{
		std::string inner;
		std::string receiver = source.receiver ? pointee(typeOf(*source.receiver, inner, depth + 1)) : "";
		if (receiver.empty())
			return "";
		if (source.kind == Source::Kind::address)
		{
			const SrcModel::Record* record = model.record(receiver);
			if (!record)
				return "";
			if (source.offset == 0 && (source.width == 0 || source.width == record->size))
				key = "ptr(record:" + receiver + ")";
			else if (std::string base = baseAt(model, *record, source.offset);
					 !base.empty() && (source.width == 0 || (model.record(base) && model.record(base)->size ==
																						 source.width)))
				key = "ptr(record:" + base + ")";
			else if (std::optional<FieldAt> field = fieldAt(receiver, source.offset, source.width, Find::outermost);
					 field && field->evidence)
				key = "ptr(" + field->type + ")";
			rests = "&" + receiver + "+" + hexText(source.offset) + " (" + inner + ")";
			break;
		}
		std::optional<FieldAt> field = fieldAt(receiver, source.offset, source.width, Find::innermost);
		if (!field || !field->evidence)
			return "";
		key = field->type;
		rests = field->owner + "::" + field->field->name + " (" + inner + ")";
		break;
	}
	case Source::Kind::global:
	case Source::Kind::globalAddress:
		if (!source.global || !model.pdbTyped("global", source.global->target, ""))
			return "";
		key = source.kind == Source::Kind::global ? source.global->type : "ptr(" + source.global->type + ")";
		rests = (source.kind == Source::Kind::global ? "" : "&") + source.global->target;
		break;
	case Source::Kind::object:
		if (folded(source.function))
			return "";
		key = "ptr(" + source.key + ")";
		rests = "a " + source.key + " " + named(source.function) + " builds";
		break;
	}
	if (key.empty())
		return "";
	if (source.adjust != 0)
	{
		std::string record = pointee(key);
		const SrcModel::Record* r = record.empty() ? nullptr : model.record(record);
		std::string base = r ? baseAt(model, *r, source.adjust) : "";
		if (base.empty())
			return "";
		rests += " + " + hexText(source.adjust);
		key = "ptr(record:" + base + ")";
	}
	return key;
}

std::optional<Infer::FieldAt> Infer::fieldAt(const std::string& recordName, int64_t offset, int64_t width,
											 Find find) const
{
	const SrcModel& model = m_context.model;
	const SrcModel::Record* record = model.record(recordName);
	if (!record || offset < 0 || width <= 0 || offset + width > record->size)
		return std::nullopt;
	for (const SrcModel::Base& base : record->bases)
		if (const SrcModel::Record* inner = model.record(base.record);
			inner && !base.isVirtual && base.offset <= offset && offset + width <= base.offset + inner->size)
			if (std::optional<FieldAt> found = fieldAt(base.record, offset - base.offset, width, find))
				return found;
	for (const SrcModel::Field& field : record->fields)
	{
		int64_t at = field.offsetBits / 8;
		std::optional<int64_t> size = model.sizeOf(field.type);
		if (!size || field.bits)
			continue;
		if (at > offset || offset + width > at + *size)
			continue;
		bool evidence = fieldEvidence(recordName, field);
		// the member itself, where it is what is named
		if (find == Find::outermost && at == offset && *size == width)
			return FieldAt{&field, recordName, field.type, evidence};
		// a member record or array: its own member there, where it has one; evidence only where every member on
		// the way is
		auto within = [&](std::optional<FieldAt> inner) {
			if (inner)
				inner->evidence = inner->evidence && evidence;
			return inner;
		};
		std::string storage = m_context.types.canonical(model.storage(field.type));
		SrcTypes::Parts p = m_context.types.parts(storage);
		if (p.kind == "record")
			if (std::optional<FieldAt> inner = within(fieldAt(p.name, offset - at, width, find)))
				return inner;
		SrcTypes::Parts whole = m_context.types.parts(m_context.types.canonical(field.type));
		if (whole.kind == "array")
		{
			std::string element = whole.of;
			std::optional<int64_t> step = model.sizeOf(element);
			SrcTypes::Parts e = m_context.types.parts(m_context.types.canonical(model.storage(element)));
			if (step && *step > 0)
			{
				int64_t inside = (offset - at) % *step;
				if (e.kind == "record")
					if (std::optional<FieldAt> inner = within(fieldAt(e.name, inside, width, find)))
						return inner;
				if (find == Find::covering || (inside == 0 && width == *step))
					return FieldAt{&field, recordName, element, evidence};
			}
		}
		if (find == Find::covering || (at == offset && *size == width))
			return FieldAt{&field, recordName, field.type, evidence};
	}
	return std::nullopt;
}

bool Infer::fieldEvidence(const std::string& owner, const SrcModel::Field& field) const
{
	const SrcModel& model = m_context.model;
	if (model.pdbTyped("field", owner, field.name))
		return true;
	// a field of a record the PDB types none of: src's own record; else a field src types otherwise than the PDB
	auto known = m_pdbRecords.find(owner);
	if (known == m_pdbRecords.end())
	{
		bool any = false;
		if (const SrcModel::Record* record = model.record(owner))
			for (const SrcModel::Field& f : record->fields)
				any = any || model.pdbTyped("field", owner, f.name);
		known = m_pdbRecords.emplace(owner, any).first;
	}
	return !known->second && !placeholderField(field.name);
}

bool Infer::derivesAt(const std::string& record, const std::string& base, int64_t offset) const
{
	if (record == base && offset == 0)
		return true;
	const SrcModel::Record* r = m_context.model.record(record);
	if (!r)
		return false;
	for (const SrcModel::Base& b : r->bases)
		if (!b.isVirtual && b.offset <= offset && derivesAt(b.record, base, offset - b.offset))
			return true;
	return false;
}

// what class C has of the body's uses: "" where it has every one, else the first it lacks
std::string Infer::lacks(const std::string& cls, const std::vector<Bound>& uppers, const std::vector<Use>& uses) const
{
	const SrcModel::Record* record = m_context.model.record(cls);
	if (!record)
		return "no src record " + cls;
	for (const Bound& upper : uppers)
	{
		std::string base = m_context.types.parts(m_context.types.parts(m_context.types.canonical(upper.key)).of).name;
		if (!derivesAt(cls, base, upper.adjust))
			return "no " + base + " at +" + hexText(upper.adjust) + " (" + hexText(upper.at) + ")";
	}
	for (const Use& use : uses)
	{
		if (use.kind == Use::Kind::access && use.width > 0)
		{
			bool vptr = use.width == 4 && std::any_of(record->vtables.begin(), record->vtables.end(),
														[&](const SrcModel::Vtable& v) { return v.offset == use.offset; });
			if (!vptr && !fieldAt(cls, use.offset, use.width, Find::covering))
				return "no member at +" + hexText(use.offset) + " (" + std::to_string(use.width) + " bytes, " +
					   hexText(use.at) + ")";
		}
		else if (use.kind == Use::Kind::access && use.offset >= record->size)
			return "no member at +" + hexText(use.offset) + " (" + hexText(use.at) + ")";
		else if (use.kind == Use::Kind::vcall)
		{
			auto vtable = std::find_if(record->vtables.begin(), record->vtables.end(),
									   [&](const SrcModel::Vtable& v) { return v.offset == use.offset; });
			if (vtable == record->vtables.end() || static_cast<int64_t>(vtable->slots.size()) * 4 <= use.slotOffset)
				return "no virtual at +" + hexText(use.slotOffset) + " of the vtable at +" + hexText(use.offset) +
					   " (" + hexText(use.at) + ")";
		}
	}
	return "";
}

Infer::Solution Infer::solveSlot(uint64_t address, const Target& target, int slot, const Slot& facts,
								 const std::string& truth) const
{
	const SrcModel& model = m_context.model;
	SrcTypes& types = m_context.types;
	Solution out;
	out.verdict = "open";
	auto kindOf = [&](const std::string& key) -> std::string {
		SrcTypes::Parts p = types.parts(types.canonical(key));
		if (p.kind == "ptr")
		{
			SrcTypes::Parts of = types.parts(p.of);
			if (of.kind == "record")
				return "class";
			if (of.kind == "builtin" && of.name == "void")
				return "void";
			return "pointer";
		}
		if (p.kind == "enum")
			return "enum";
		if (p.kind == "builtin")
		{
			const SrcModel::Type* t = model.type(types.canonical(key));
			return t && t->isFloat ? "float" : "integer";
		}
		return "other";
	};
	auto recordOf = [&](const std::string& key) {
		return types.parts(types.parts(types.canonical(key)).of).name;
	};
	auto spelled = [&](const std::string& key) {
		std::optional<std::string> s = types.spell(key);
		return s ? *s : key;
	};

	// the bounds this round
	std::vector<Bound> lowers, uppers;
	int constants = 0;
	for (const Source& source : facts.below)
	{
		std::string rests;
		if (source.kind == Source::Kind::number)
		{
			constants += source.offset != 0;
			continue;
		}
		std::string key = typeOf(source, rests);
		if (!key.empty())
			lowers.push_back({key, 0, source.at, rests});
	}
	std::vector<Use> structural;
	int signedUses = 0, unsignedUses = 0, neutralUses = 0, floatUses = 0, typeTests = 0;
	for (const Use& use : facts.uses)
	{
		switch (use.kind)
		{
		case Use::Kind::access:
		case Use::Kind::vcall:
			structural.push_back(use);
			break;
		case Use::Kind::integer:
			(use.signedness > 0 ? signedUses : use.signedness < 0 ? unsignedUses : neutralUses)++;
			break;
		case Use::Kind::floating:
			++floatUses;
			break;
		case Use::Kind::typeTest:
			++typeTests;
			break;
		case Use::Kind::pass:
		{
			if (use.callee == address && use.slot == slot)
				break;
			const SrcModel::Function* f = use.function;
			if (!f)
			{
				auto found = m_targets.find(use.callee);
				f = found == m_targets.end() ? nullptr : found->second.declaration;
			}
			std::string key = slotType(use.callee, f, use.slot);
			if (!key.empty())
				uppers.push_back({key, use.adjust, use.at,
								  "passed to " + (f ? f->name : hexText(use.callee)) + " " + slotName(use.slot)});
			break;
		}
		case Use::Kind::store:
		case Use::Kind::compare:
		{
			std::string rests;
			std::string key = use.destination ? typeOf(*use.destination, rests) : "";
			// pointers compared convert either way (an Actor* with the PlayerCharacter* global): no direction
			if (use.kind == Use::Kind::compare && !key.empty() &&
				m_context.types.parts(m_context.types.bare(key)).kind == "ptr")
				break;
			if (!key.empty())
				uppers.push_back({key, use.adjust, use.at,
								  (use.kind == Use::Kind::store ? "stored into " : "compared with ") + rests});
			break;
		}
		case Use::Kind::returned:
		{
			if (use.callee == address && slot == kReturn)
				break;
			auto found = m_targets.find(use.callee);
			const SrcModel::Function* f =
				found != m_targets.end() ? found->second.declaration : declarationAt(model, use.callee);
			std::string key = slotType(use.callee, f, kReturn);
			if (!key.empty())
				uppers.push_back({key, use.adjust, use.at, "returned from " + (f ? f->name : hexText(use.callee))});
			break;
		}
		}
	}

	// the evidence, as text: each bound with its address, the body's members
	auto describe = [&](const Bound& b, const char* side) {
		return std::string(side) + " " + hexText(b.at) + " " + spelled(b.key) + " (" + b.rests +
			   (b.adjust ? ", +" + hexText(b.adjust) : std::string()) + ")";
	};
	std::vector<std::string> evidence;
	for (const Bound& b : lowers)
		evidence.push_back(describe(b, slot == kReturn ? "returns" : "caller"));
	for (const Bound& b : uppers)
		evidence.push_back(describe(b, "uses"));
	std::string members;
	for (const Use& use : structural)
	{
		if (members.size() > 160)
		{
			members += " ...";
			break;
		}
		members += (members.empty() ? "" : ", ") +
				   (use.kind == Use::Kind::vcall
						? "vcall +" + hexText(use.slotOffset) + (use.offset ? "@+" + hexText(use.offset) : "")
						: (use.width ? "+" + hexText(use.offset) + "/" + std::to_string(use.width) : "[i]")) +
				   " " + hexText(use.at);
	}
	if (!members.empty())
		evidence.push_back("members " + members);
	if (signedUses + unsignedUses + neutralUses)
		evidence.push_back("integer uses: " + std::to_string(signedUses) + " signed, " +
						   std::to_string(unsignedUses) + " unsigned, " + std::to_string(neutralUses) + " neither");
	if (floatUses)
		evidence.push_back("float uses: " + std::to_string(floatUses));
	if (constants)
		evidence.push_back("callers pass " + std::to_string(constants) + " nonzero constants");
	if (facts.rebound)
		evidence.push_back("the body writes the parameter");
	for (size_t k = 0; k < evidence.size() && k < 12; ++k)
		out.evidence += (k ? "; " : "") + evidence[k];

	// the slot's clause of the comment above the declaration (`<slot>: <evidence>`, srcdb's form: no `; ` inside,
	// which separates the clauses): the slot, the bound that decided it and the members
	auto comment = [&](std::string why) {
		std::string name = slot == kReturn ? "return"
						   : static_cast<size_t>(slot) < target.declaration->params.size() &&
								   !target.declaration->params[slot].empty()
							   ? target.declaration->params[slot]
							   : "parameter " + std::to_string(slot);
		for (size_t at = why.find("; "); at != std::string::npos; at = why.find("; ", at))
			why.replace(at, 2, ", ");
		return name + ": " + why;
	};

	auto open = [&](const std::string& why) {
		out.verdict = "open";
		out.key.clear();
		out.evidence = why + (out.evidence.empty() ? "" : " | " + out.evidence);
		return out;
	};

	// the kinds the bounds have
	std::vector<Bound> classLowers, classUppers, otherPointers, integers, enums, floats, voids, others;
	for (const Bound& b : lowers)
	{
		std::string kind = kindOf(b.key);
		(kind == "class"	   ? classLowers
		 : kind == "pointer" ? otherPointers
		 : kind == "enum"	   ? enums
		 : kind == "float"   ? floats
		 : kind == "void"	   ? voids
		 : kind == "integer" ? integers
							   : others)
			.push_back(b);
	}
	std::vector<Bound> upperIntegers, upperEnums, upperFloats, upperPointers;
	for (const Bound& b : uppers)
	{
		std::string kind = kindOf(b.key);
		if (kind == "class")
			classUppers.push_back(b);
		else if (kind == "void")
			continue;
		else if (kind == "pointer")
			upperPointers.push_back(b);
		else if (kind == "enum")
			upperEnums.push_back(b);
		else if (kind == "float")
			upperFloats.push_back(b);
		else if (kind == "integer")
			upperIntegers.push_back(b);
		else
			others.push_back(b);
	}
	// a record by value, an array, a member pointer: no slot type this solver proposes
	if (!others.empty())
		return open("a bound of another kind: " + describe(others[0], "bound"));
	bool holdsAddress = !structural.empty() || !classLowers.empty() || !classUppers.empty() || !otherPointers.empty() ||
				   !upperPointers.empty() || !voids.empty();
	bool scalar = !integers.empty() || !enums.empty() || !floats.empty() || !upperIntegers.empty() ||
				  !upperEnums.empty() || !upperFloats.empty();
	if (holdsAddress && scalar)
		return open("holds an address and a scalar: " + (integers.size() + enums.size() + floats.size()
															  ? describe(!integers.empty() ? integers[0]
																		 : !enums.empty()  ? enums[0]
																						   : floats[0],
																		 "caller")
															  : describe(!upperIntegers.empty() ? upperIntegers[0]
																		 : !upperEnums.empty()	? upperEnums[0]
																								: upperFloats[0],
																		 "uses")));

	// a class pointer: the callers bound it from below, the uses from above
	if (!classLowers.empty() || !classUppers.empty())
	{
		if (!otherPointers.empty() || !upperPointers.empty())
			return open("a class pointer and another pointer: " +
						describe(!otherPointers.empty() ? otherPointers[0] : upperPointers[0], "bound"));
		// the declaration's own class against the proposal: equal, a base or derived class inside the bounds (it is
		// or is a base of every caller's class, and has every use), or wrong
		auto judge = [&](const std::string& mine) {
			if (truth.empty())
				return;
			SrcTypes::Parts was = types.parts(types.canonical(truth));
			std::string pdb = was.kind == "ptr" || was.kind == "lref" ? types.parts(was.of).name : "";
			bool inside = !pdb.empty() && lacks(pdb, classUppers, structural).empty() &&
						  std::all_of(classLowers.begin(), classLowers.end(), [&](const Bound& b) {
							  return types.derivesAtZero(recordOf(b.key), pdb);
						  });
			out.truth = pdb == mine ? "equal"
						: inside && (types.derivesAtZero(pdb, mine) || types.derivesAtZero(mine, pdb)) ? "chain"
																										  : "wrong";
		};
		// a class test in the body (NiIsKindOf, a type tag, a test of a virtual's result): what follows it may use a
		// derived class's members on the value, cast down, so the body bounds nothing; the callers alone do
		if (typeTests)
		{
			if (classLowers.empty())
				return open("the body tests its class (a downcast may follow) and no caller bounds it");
			classUppers.clear();
			structural.clear();
		}
		std::vector<std::string> candidates;
		bool fromCallers = !classLowers.empty();
		std::string castable;
		if (fromCallers)
		{
			// the classes every lower bound is or derives from at offset 0
			std::function<void(const std::string&, std::vector<std::string>&)> ancestors =
				[&](const std::string& cls, std::vector<std::string>& into) {
					if (std::find(into.begin(), into.end(), cls) != into.end())
						return;
					into.push_back(cls);
					if (const SrcModel::Record* r = model.record(cls))
						for (const SrcModel::Base& b : r->bases)
							if (b.offset == 0 && !b.isVirtual)
								ancestors(b.record, into);
				};
			// callers passing unequal classes on one chain (NiAVObject and NiNode): one passing the base may cast
			// it down, as src's calls of a NiNode* parameter with Get3D()'s NiAVObject* do; the range is up to the
			// most derived of them, decided only where the body needs that one
			std::string mostDerived = recordOf(classLowers[0].key);
			bool chain = true, alike = true;
			for (const Bound& b : classLowers)
			{
				std::string c = recordOf(b.key);
				alike = alike && c == mostDerived;
				if (types.derivesAtZero(c, mostDerived))
					mostDerived = c;
				else if (!types.derivesAtZero(mostDerived, c))
					chain = false;
			}
			castable = chain && !alike ? mostDerived : "";
			if (!castable.empty())
				ancestors(castable, candidates);
			else
			{
				ancestors(recordOf(classLowers[0].key), candidates);
				for (size_t k = 1; k < classLowers.size(); ++k)
				{
					std::vector<std::string> other;
					ancestors(recordOf(classLowers[k].key), other);
					std::erase_if(candidates, [&](const std::string& c) {
						return std::find(other.begin(), other.end(), c) == other.end();
					});
				}
			}
			if (candidates.empty())
				return open("the callers pass classes with no common base at offset 0: " +
							describe(classLowers[0], "caller") + ", " + describe(classLowers.back(), "caller"));
		}
		else
		{
			// no caller bounds it: the classes that have the uppers' bases; the least derived of them below
			for (const auto& [name, record] : model.records())
				if (record.line > 0 && lacks(name, classUppers, {}).empty())
					candidates.push_back(name);
		}
		// with no use in the body, the callers' most derived common base
		if (classUppers.empty() && structural.empty())
		{
			if (!castable.empty())
				return open("the callers pass classes on one chain up to " + castable + " (one may cast down)");
			auto derived = std::find_if(candidates.begin(), candidates.end(), [&](const std::string& c) {
				return std::all_of(candidates.begin(), candidates.end(),
								   [&](const std::string& d) { return types.derivesAtZero(c, d); });
			});
			if (derived == candidates.end())
				return open("the callers' classes have no one most derived common base");
			out.key = "ptr(record:" + *derived + ")";
			out.verdict = "callers-only";
			judge(*derived);
			return out;
		}
		std::vector<std::string> fit;
		std::string firstLack;
		for (const std::string& c : candidates)
		{
			std::string lack = lacks(c, classUppers, structural);
			if (lack.empty())
				fit.push_back(c);
			else if (firstLack.empty())
				firstLack = c + ": " + lack;
		}
		if (fit.empty())
			return open("no class in the bounds has the body's uses (" + firstLack + ")");
		// the least derived: the one every other derives from
		auto least = std::find_if(fit.begin(), fit.end(), [&](const std::string& c) {
			return std::all_of(fit.begin(), fit.end(), [&](const std::string& d) { return types.derivesAtZero(d, c); });
		});
		if (least == fit.end())
		{
			std::string names;
			for (size_t k = 0; k < fit.size() && k < 4; ++k)
				names += (k ? ", " : "") + fit[k];
			return open("several unrelated classes have the uses (" + std::to_string(fit.size()) + ": " + names +
						")");
		}
		if (!castable.empty() && *least != castable)
			return open("the callers pass classes on one chain up to " + castable + " (one may cast down); the "
						"body needs only " + *least);
		// src's class where it is in the bounds too (every caller's class is or derives from it at offset 0, and it
		// has every use): the evidence confirms it as well as the least derived, and a fitting type is not weakened
		// to a base. Not with --hide, whose declaration is the answer compared with.
		std::string chosen = *least;
		if (truth.empty())
			if (const SrcModel::Type* type = model.type(target.declaration->type))
			{
				SrcTypes::Parts mine = types.parts(types.canonical(slot == kReturn ? type->ret : type->params[slot]));
				SrcTypes::Parts pointee = mine.kind == "ptr" || mine.kind == "lref" ? types.parts(mine.of)
																					  : SrcTypes::Parts{};
				if (pointee.kind == "const")
					pointee = types.parts(pointee.of);
				if (pointee.kind == "record" && std::find(fit.begin(), fit.end(), pointee.name) != fit.end())
					chosen = pointee.name;
			}
		out.key = "ptr(record:" + chosen + ")";
		// the body's uses alone may follow a downcast the exe shows as no test (a form-type read, a switch): no
		// proof, but the best-supported type where no caller contradicts it (a caller's scalar or other pointer
		// opened the slot above): `guessed`, written under `// Guessed type:` (RULES "Evidence for declarations and
		// types") and never evidence of a later round's. Only a parameter (a return's bound from above is its
		// callers' uses, not the body's) of a function called directly and no virtual (one reached only through a
		// vtable or a function pointer has its interface's type, which the body casts down: CopyMembers'
		// NiCloningProcess& used as BSCloningProcess; PDB-named samples: 8 of 169 such slots wrong, against 3 of
		// 116 called ones), no caller passing a void* (a generic slot: placement new, a script command's
		// parameters), and src's type naming no pointee yet (an integer or void*: a pointer src types is not
		// moved to another class on the body's uses alone; not with --hide, whose declaration is the answer)
		if (!fromCallers)
		{
			std::string need = "no caller bounds it; the body's uses need " + *least + " (" +
								std::to_string(target.callers.size()) + " calling functions, " +
								std::to_string(classUppers.size()) + " class uses)";
			if (slot == kReturn)
				return open(need + "; a return's bound from above is its callers' uses");
			if (target.declaration->isVirtual)
				return open(need + "; a virtual's type is its family's, which the body may cast down");
			if (target.callers.empty())
				return open(need + "; no direct call: a callback's type is its interface's, which the body may "
								   "cast down");
			if (!voids.empty())
				return open(need + "; " + describe(voids[0], "caller") + ": a generic slot");
			if (truth.empty())
				if (const SrcModel::Type* type = model.type(target.declaration->type))
				{
					std::string mine = kindOf(type->params[slot]);
					if (types.parts(types.canonical(type->params[slot])).kind == "lref" || mine == "class" ||
						mine == "pointer")
						return open(need + "; src types it " + spelled(type->params[slot]) +
									" already, a pointee the body's uses alone do not move");
				}
			out.verdict = "guessed";
			out.evidence = need + (out.evidence.empty() ? "" : " | " + out.evidence);
			out.comment = comment(spelled(out.key) + ": the body's uses, " + classUppers[0].rests + " at " +
								  hexText(classUppers[0].at) +
								  (structural.empty() ? "" : ", its members at " + hexText(structural[0].at)) +
								  ", no caller typed by evidence");
			judge(chosen);
			return out;
		}
		out.verdict = "proven";
		const Bound& decisive = classLowers[0];
		out.comment = comment(spelled(out.key) + " from " + hexText(decisive.at) + " (" + decisive.rests + ")" +
							  (structural.empty() ? "" : ", its members used at " + hexText(structural[0].at)));
		judge(chosen);
		return out;
	}

	// a pointer to a non-class type: every bound points to it
	if (!otherPointers.empty() || !upperPointers.empty())
	{
		// callers alone may pass another pointer type reinterpreted (a padded storage, an array of bases)
		if (upperPointers.empty())
			return open("only callers bound it: " + describe(otherPointers[0], "caller") + " (one may reinterpret)");
		std::vector<Bound> all = otherPointers;
		all.insert(all.end(), upperPointers.begin(), upperPointers.end());
		std::string pointee = types.parts(types.canonical(all[0].key)).of;
		for (const Bound& b : all)
			if (types.parts(types.canonical(b.key)).of != pointee || b.adjust != 0)
				return open("pointers to different types: " + describe(all[0], "bound") + ", " + describe(b, "bound"));
		out.key = "ptr(" + pointee + ")";
		out.verdict = "proven";
		out.comment = comment(spelled(out.key) + " from " + hexText(all[0].at) + " (" + all[0].rests + ")");
		return out;
	}
	if (holdsAddress)
		return open(structural.empty() ? "holds an address of no shown pointee (void* bounds only)"
									   : "holds an address: no bound names its pointee");

	// A scalar. C++ converts an integer or an enum to any integer type implicitly where a value is passed, stored or
	// compared, so the callers' and the destinations' integer types decide nothing: an integer is the guess confirmed
	// (its width and signedness as the body's own operations use it). An enum the body passes on, stores or compares
	// as itself is the enum (an integer needs a cast to become one); a float bound is a float of its width (a
	// conversion between a float and an integer is code the exe would show).
	const SrcModel::Type* type = model.type(target.declaration->type);
	std::string current = !type ? "" : slot == kReturn ? type->ret : type->params[slot];
	std::optional<int64_t> width = model.sizeOf(current);
	std::string currentKind = kindOf(current);
	int integerUses = signedUses + unsignedUses + neutralUses;
	if (!upperEnums.empty())
	{
		std::string key = types.canonical(upperEnums[0].key);
		for (const Bound& b : upperEnums)
			if (types.canonical(b.key) != key)
				return open("enums of different types: " + describe(upperEnums[0], "uses") + ", " +
							describe(b, "uses"));
		std::optional<int64_t> size = model.sizeOf(key);
		if (!width || !size || *size != *width)
			return open("the body's " + spelled(key) + " is not the slot's width");
		if (currentKind == "enum" && types.canonical(current) != key)
			return open("src's enum " + spelled(current) + ", the body's " + spelled(key));
		if (floatUses || !floats.empty() || !upperFloats.empty())
			return open(spelled(key) + " against float uses");
		out.key = upperEnums[0].key;
		out.verdict = "proven";
		out.comment = comment(spelled(out.key) + ": " + upperEnums[0].rests + " at " + hexText(upperEnums[0].at));
		return out;
	}
	if (!floats.empty() || !upperFloats.empty())
	{
		std::vector<Bound> all = floats;
		all.insert(all.end(), upperFloats.begin(), upperFloats.end());
		std::string key = types.canonical(all[0].key);
		for (const Bound& b : all)
			if (types.canonical(b.key) != key)
				return open("floats of different types: " + describe(all[0], "bound") + ", " + describe(b, "bound"));
		std::optional<int64_t> size = model.sizeOf(key);
		if (!width || !size || *size != *width)
			return open("the bounds' " + spelled(key) + " is not the slot's width");
		if (integerUses || !integers.empty() || !upperIntegers.empty() || !enums.empty())
			return open(spelled(key) + " against integer uses or bounds");
		out.key = all[0].key;
		out.verdict = "proven";
		out.comment = comment(spelled(out.key) + " from " + hexText(all[0].at) + " (" + all[0].rests + ")");
		return out;
	}
	bool integerBounds = !integers.empty() || !upperIntegers.empty() || !enums.empty();
	if (floatUses && !integerUses && !integerBounds)
	{
		if (!width || (*width != 4 && *width != 8))
			return open("float uses in a slot of " + (width ? std::to_string(*width) : std::string("unknown")) +
						" bytes");
		out.key = *width == 4 ? "builtin:float" : "builtin:double";
		out.verdict = "proven";
		uint64_t at = 0;
		for (const Use& use : facts.uses)
			if (use.kind == Use::Kind::floating && !at)
				at = use.at;
		out.comment = comment(spelled(out.key) + ": used as one at " + hexText(at));
		return out;
	}
	if (floatUses)
		return open("used both as a float and as an integer");
	if (!integerUses && !integerBounds)
		return open("no evidence");
	if (signedUses && unsignedUses)
		return open("used both as a signed and as an unsigned integer");
	if (!signedUses && !unsignedUses)
		return open("an integer whose signedness no use of the body shows");
	// the guess confirmed: src's integer (or enum) of the slot's width, of the signedness the body uses
	std::string integer = currentKind == "enum" ? model.enumeration(types.parts(types.canonical(current)).name)
													  ? model.enumeration(types.parts(types.canonical(current)).name)
															->underlying
													  : ""
						  : currentKind == "integer" ? current
													 : "";
	const SrcModel::Type* t = integer.empty() ? nullptr : model.type(types.canonical(integer));
	if (!t)
		return open("an integer use of a slot src types " + spelled(current));
	if (t->isSigned != (signedUses > 0))
		return open("src's " + spelled(current) + ", the body's " + (signedUses ? "signed" : "unsigned") + " uses");
	out.key = current;
	out.verdict = "proven";
	uint64_t at = 0;
	for (const Use& use : facts.uses)
		if (use.kind == Use::Kind::integer && use.signedness != 0 && !at)
			at = use.at;
	out.comment = comment(spelled(out.key) + ": " + (signedUses ? "signed" : "unsigned") + " uses from " +
						  hexText(at));
	return out;
}

std::vector<Infer::Row> Infer::solve(bool truth)
{
	SrcTypes& types = m_context.types;
	const SrcModel& model = m_context.model;
	// the declaration's type of a slot
	auto current = [&](const Target& target, int slot) -> std::string {
		const SrcModel::Type* type = model.type(target.declaration->type);
		return slot == kThis ? "ptr(record:" + target.declaration->cls + ")"
			   : !type		 ? ""
			   : slot == kReturn ? type->ret
								 : type->params[slot];
	};
	std::map<std::pair<uint64_t, int>, Solution> proven;
	// rounds: each proves from the declarations and the slots the rounds before proved
	for (m_round = 1;; ++m_round)
	{
		size_t found = 0;
		for (const auto& [address, target] : m_targets)
			for (const auto& [slot, facts] : target.slots)
			{
				if (slot == kThis || !target.why.empty() || m_proven.count({address, slot}) ||
					evidence(address, target.declaration, slot))
					continue;
				Solution s = solveSlot(address, target, slot, facts, truth ? current(target, slot) : "");
				if (s.verdict != "proven")
					continue;
				m_proven[{address, slot}] = Proven{s.key, m_round};
				proven[{address, slot}] = s;
				++found;
			}
		if (found == 0)
			break;
	}
	// the rows: a proven slot as its round proved it, one src's `// Inferred type` clause has as that, any other as
	// the last round leaves it
	std::vector<Row> rows;
	for (const auto& [address, target] : m_targets)
	{
		for (const auto& [slot, facts] : target.slots)
		{
			Row row;
			row.address = address;
			row.function = target.declaration->name;
			row.mangled = target.declaration->mangled;
			row.slot = slotName(slot);
			row.current = current(target, slot);
			Solution s;
			if (!target.why.empty())
			{
				s.verdict = "open";
				s.evidence = target.why;
			}
			else if (slot != kThis && evidence(address, target.declaration, slot))
			{
				s.verdict = "inferred";
				s.key = row.current;
				s.evidence = "src's `// Inferred type` clause";
			}
			else if (auto p = proven.find({address, slot}); p != proven.end())
			{
				s = p->second;
				row.round = m_proven.at({address, slot}).round;
			}
			else if (slot == kThis)
			{
				// an owner question, the owners waves': the callers' classes as a note
				s.verdict = "note";
				for (const Source& source : facts.below)
				{
					std::string rests, key = typeOf(source, rests);
					if (!key.empty() && s.evidence.size() < 300)
						s.evidence += (s.evidence.empty() ? "" : "; ") + std::string("caller ") + hexText(source.at) +
									  " " + key + " (" + rests + ")";
				}
			}
			else
				s = solveSlot(address, target, slot, facts, truth ? row.current : "");
			row.proposed = s.key;
			row.verdict = s.verdict;
			row.evidence = s.evidence;
			row.clause = s.comment;
			row.kind = [&] {
				SrcTypes::Parts p = types.parts(types.canonical(row.current));
				if (p.kind == "ptr" || p.kind == "lref")
					return types.parts(p.of).kind == "record" ? "class" : "pointer";
				if (p.kind == "enum")
					return "enum";
				if (p.kind == "builtin")
				{
					const SrcModel::Type* t = model.type(types.canonical(row.current));
					return t && t->isFloat ? "float" : "integer";
				}
				return "other";
			}();
			row.truth = s.truth;
			if (truth && !s.key.empty() && row.truth.empty())
			{
				// the declaration's own type (a hidden function's PDB type), cv aside: equal or wrong
				SrcTypes::Parts was = types.parts(types.canonical(row.current));
				std::string wasKey = was.kind == "lref" ? "ptr(" + was.of + ")" : types.canonical(row.current);
				row.truth = wasKey == types.canonical(s.key) ? "equal" : "wrong";
			}
			rows.push_back(std::move(row));
		}
	}
	return rows;
}

}  // namespace idakit
