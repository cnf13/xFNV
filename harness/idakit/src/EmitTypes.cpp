// Types: IDA's types of the locals, the return value, casts, numbers and dereferences as src type keys. A local of a
// record C++ would run src's user-declared structors for (the exe inlines them, or runs none) is raw storage holding
// the object, unresolved, until a locals pass declares it the exe's way (StructorLocals, InlinedLocals).

#include "EmitPasses.hpp"

#include "IdaSupport.hpp"

namespace idakit
{

// The class C++ would run user code for, for an object of the record ("" for none), and what it runs: its default
// constructor (a missing one too, where other constructors are user-declared: the local could not be declared) and
// its destructor, unless empty (srcdb: no code), then those of its bases and members.
std::pair<std::string, std::string> userStructors(const SrcModel& model, const std::string& record)
{
	bool constructors = false, defaultConstructor = false, ctor = false, dtor = false;
	for (const SrcModel::Function* f : model.structorsOf(record))
	{
		if (f->structor == "dtor")
		{
			dtor |= !f->empty;
			continue;
		}
		const SrcModel::Type* type = model.type(f->type);
		bool isDefault = type && type->params.empty();
		constructors = true;
		defaultConstructor |= isDefault;
		ctor |= isDefault && !f->empty;
	}
	ctor |= constructors && !defaultConstructor;
	if (ctor || dtor)
		return {record, ctor && dtor ? "constructor and destructor" : ctor ? "constructor" : "destructor"};
	const SrcModel::Record* r = model.record(record);
	if (!r)
		return {};
	for (const SrcModel::Base& base : r->bases)
		if (auto found = userStructors(model, base.record); !found.first.empty())
			return found;
	for (const SrcModel::Field& field : r->fields)
	{
		const SrcModel::Type* held = model.type(model.storage(field.type));
		if (held && held->kind == "record" && held->name != record)
			if (auto found = userStructors(model, held->name); !found.first.empty())
				return found;
	}
	return {};
}

void restoreStructorLocal(Function& function, size_t k)
{
	Local& local = function.locals[k];
	if (!local.storage || !local.type.empty() || local.holds.empty())
		return;
	local.type = local.holds;
	local.storage = false;
	local.holds.clear();
	local.why.clear();
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
		if (lvarIndex(e) == static_cast<int>(k))
			function.meaning.erase(e);
	});
}

namespace
{

// the src record a pointer points at, "" when it points at no record
std::string pointeeRecord(SrcTypes& types, const tinfo_t& pointer)
{
	std::string why;
	std::optional<std::string> key = types.keyOf(pointer, why);
	if (!key)
		return "";
	SrcTypes::Parts p = types.parts(types.canonical(*key));
	if (p.kind != "ptr")
		return "";
	SrcTypes::Parts pointee = types.parts(p.of);
	return pointee.kind == "record" ? pointee.name : "";
}

// a pointer cast between arithmetic types of one size (`*(_DWORD *)&f`, `(float *)&i`): a read or write of the same
// bytes as another type (ExeCast; resolveTypes keeps the pointee's const)
bool reinterpretsBytes(const cexpr_t* cast)
{
	if (!cast->type.is_ptr() || !cast->x->type.is_ptr())
		return false;
	tinfo_t to = cast->type.get_pointed_object(), from = cast->x->type.get_pointed_object();
	auto arithmetic = [](const tinfo_t& t) {
		return t.is_floating() || (t.is_integral() && !t.is_bool() && !t.is_enum());
	};
	return arithmetic(to) && arithmetic(from) && to.get_size() == from.get_size();
}

// A call argument cast to a pointer to bytes (char, signed or unsigned) from a pointer to a scalar that holds no
// pointer (arithmetic, bool, an enum, an array of them) (`SaveData((char*)&fValue, 4)`): the callee reads or writes the
// object's bytes, the same ones on any layout (ExeCast; resolveTypes keeps the pointee's const). The cast is the
// argument itself, so no pointer arithmetic is made on it.
bool viewsBytes(const cexpr_t* cast, const std::unordered_map<const cexpr_t*, const citem_t*>& parents)
{
	if (!cast->type.is_ptr() || !cast->x->type.is_ptr())
		return false;
	auto parent = parents.find(cast);
	if (parent == parents.end() || parent->second->op != cot_call ||
		static_cast<const cexpr_t*>(parent->second)->x == cast)
		return false;
	tinfo_t to = cast->type.get_pointed_object(), from = cast->x->type.get_pointed_object();
	if (from.is_array())
		from = from.get_array_element();
	bool scalar = from.is_floating() || from.is_enum() || from.is_bool() || from.is_integral();
	return to.get_size() == 1 && to.is_integral() && !to.is_bool() && scalar;
}

// A pointer to an object pointer cast to a pointer to `void*` (`(void**)&pDistortion`, `*(void**)ppArrays[i]`): the
// slot read or written as the untyped pointer it holds, an object pointer of the same size and representation as
// any other on every layout (ExeCast; resolveTypes keeps the pointee's const). A function pointer's slot is none.
bool viewsPointerSlot(const cexpr_t* cast)
{
	if (!cast->type.is_ptr() || !cast->x->type.is_ptr())
		return false;
	tinfo_t to = cast->type.get_pointed_object(), from = cast->x->type.get_pointed_object();
	return to.is_ptr() && to.get_pointed_object().is_void() && from.is_ptr() &&
		   !from.get_pointed_object().is_void() && !from.get_pointed_object().is_func();
}

// the ExeCast's key with the pointee's const IDA's operand has (`(int*)(const float*)p` reads the bytes as `const
// int`); a write through it stays the conversion's to decide
std::string keptConst(SrcTypes& types, const cexpr_t* cast, const std::string& key)
{
	if (!cast->x->type.get_pointed_object().is_const() || types.constPointee(key))
		return key;
	return "ptr(const(" + types.parts(types.bare(key)).of + "))";
}

// a pointer cast between pointers to one src type, its cv apart (cvApart: IDA's `const CHAR*` to `char*`, `LONG*` to
// `int*`): no reinterpretation. The operand's type is its value's (valueType: an indirect call Hex-Rays types `int`
// holds its slot's pointer)
bool samePointee(SrcTypes& types, const cexpr_t* cast)
{
	tinfo_t operand = valueType(cast->x);
	if (!cast->type.is_ptr() || !operand.is_ptr())
		return false;
	std::string why;
	std::optional<std::string> from = types.keyOf(operand, why), to = types.keyOf(cast->type, why);
	return from && to && cvApart(types, *from, *to);
}

// a cast src writes as a static_cast: arithmetic, or between pointers to void or to related records (one derives
// from the other), or between pointers to one src type that differ in the pointee's cv (cvApart: a const dropped is
// the Const pass's); nullopt, else what it is
std::optional<std::string> reinterpreting(SrcTypes& types, const cexpr_t* cast)
{
	const tinfo_t& to = cast->type;
	const tinfo_t from = valueType(cast->x);
	if (to.is_ptr() != from.is_ptr())
		return "cast between an integer and a pointer (" + text(from) + " to " + text(to) + ")";
	if (!to.is_ptr())
		return std::nullopt;
	if (to.get_pointed_object().is_void() || from.get_pointed_object().is_void() || samePointee(types, cast))
		return std::nullopt;
	std::string a = pointeeRecord(types, from), b = pointeeRecord(types, to);
	if (!a.empty() && !b.empty() && (types.derivesAtZero(a, b) || types.derivesAtZero(b, a)))
		return std::nullopt;
	return "cast between unrelated pointers (" + text(from) + " to " + text(to) + ")";
}

}  // namespace

// A negative number compared (==, !=, <, ...) with an unsigned operand narrower than int (`(unsigned __int16)v !=
// -1`): Hex-Rays compares at the operand's width, which C++ does not (it promotes the operand to int, which is never
// negative), so the number is the operand type's value of its bits (`0xFFFF`), an int; nullopt for any other number
static std::optional<std::string> narrowCompared(const cexpr_t* number,
												 const std::unordered_map<const cexpr_t*, const citem_t*>& parents)
{
	auto parent = parents.find(number);
	if (parent == parents.end() || parent->second->op < cot_eq || parent->second->op > cot_ult)
		return std::nullopt;
	const auto* compare = static_cast<const cexpr_t*>(parent->second);
	const cexpr_t* other = compare->x == number ? compare->y : compare->x;
	size_t width = other->type.get_size();
	size_t size = number->type.get_size();
	if (!other->type.is_integral() || other->type.is_bool() || other->type.is_enum() || other->type.is_signed() ||
		width >= 4 || width == 0 || size < width || size > 8)
		return std::nullopt;
	uint64_t value = number->n->_value;
	int64_t extended = static_cast<int64_t>(value << (64 - size * 8)) >> (64 - size * 8);
	if (!number->type.is_signed() || extended >= 0 || extended < -(int64_t(1) << (width * 8 - 1)))
		return std::nullopt;
	char text[32];
	std::snprintf(text, sizeof text, "0x%llX",
				  static_cast<unsigned long long>(value & ((uint64_t(1) << (width * 8)) - 1)));
	return text;
}

void resolveTypes(Function& function, Context& context)
{
	SrcTypes& types = context.types;
	const lvars_t& lvars = *function.cfunc->get_lvars();
	for (size_t k = 0; k < lvars.size(); ++k)
	{
		Local& local = function.locals[k];
		local.name = lvars[k].name.c_str();
		// a Hex-Rays name that is a src type's (a parameter named TESObjectREFR) or a class template's (a local named
		// AStarNode) would hide the type
		const SrcModel& model = context.model;
		auto instance = model.records().lower_bound(local.name + "<");
		bool templateName = instance != model.records().end() && instance->first.starts_with(local.name + "<");
		if (model.record(local.name) || model.enumeration(local.name) || model.typedefNamed(local.name) ||
			templateName)
			local.name += "_";
		std::optional<std::string> key = types.keyOf(lvars[k].type(), local.why);
		local.type = key.value_or("");
		// a record (or an array of them) C++ would run src's user-declared structors for: raw storage holding it
		tinfo_t held = lvars[k].type();
		while (held.is_array())
			held = held.get_array_element();
		const SrcModel::Record* record =
			key && held.is_udt() && !lvars[k].is_arg_var() ? types.recordOf(held) : nullptr;
		if (auto [cls, runs] = record ? userStructors(model, record->name) : std::pair<std::string, std::string>{};
			!cls.empty())
		{
			local.storage = true;
			local.holds = local.type;
			local.type.clear();
			local.why = cls + "'s " + runs + " is user-declared in src (C++ would run it; the exe inlines it)";
		}
	}
	// a raw local's uses: the object it holds
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
		if (int v = lvarIndex(e); v >= 0 && function.locals[v].storage)
			function.meaning[e] = function.locals[v].holds;
	});

	tinfo_t type;
	function.cfunc->get_func_type(&type);
	std::string why;
	if (std::optional<std::string> key = types.keyOf(type.get_rettype(), why))
		function.returnType = *key;
	else
		function.blockers.push_back("return type: " + why);

	auto missing = [&](const cexpr_t* e, std::string what) { function.meaning[e] = Missing{std::move(what)}; };
	std::unordered_map<const cexpr_t*, const citem_t*> parents = parentsOf(function);
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
		std::string why;
		switch (e->op)
		{
		case cot_fnum:
			// a floating constant is as wide as the exe stores it (fpc.nbytes: 4 a float, 8 a double), whatever
			// Hex-Rays' type (a float argument's constant can be a double)
			if (e->fpc->nbytes == 4 || e->fpc->nbytes == 8)
			{
				function.meaning[e] = std::string(e->fpc->nbytes == 4 ? "builtin:float" : "builtin:double");
				break;
			}
			[[fallthrough]];
		case cot_cast:
		{
			std::optional<std::string> key = types.keyOf(e->type, why);
			if (!key)
				missing(e, (e->op == cot_cast ? "cast to " : "number of ") + why);
			else if (e->op == cot_cast && !samePointee(types, e) &&
					 (reinterpretsBytes(e) || viewsBytes(e, parents) || viewsPointerSlot(e)))
				function.meaning[e] = ExeCast{keptConst(types, e, *key)};
			else if (e->op == cot_cast && comparedSentinel(types, e, parents))
				function.meaning[e] = *key;
			else if (std::optional<std::string> what = e->op == cot_cast ? reinterpreting(types, e) : std::nullopt)
				missing(e, *what);
			else
				function.meaning[e] = *key;
			break;
		}
		case cot_num:
		{
			if (e->type.is_ptr() && e->n->_value != 0)
			{
				char text[64];
				std::snprintf(text, sizeof text, "address constant 0x%llX",
							  static_cast<unsigned long long>(e->n->_value));
				missing(e, text);
				break;
			}
			std::optional<std::string> key = types.keyOf(e->type, why);
			if (std::optional<std::string> narrow = narrowCompared(e, parents))
				function.meaning[e] = Rewrite{1, *narrow, {}, {}, "builtin:int"};
			else if (key)
				function.meaning[e] = *key;
			else
				missing(e, "number of " + why);
			break;
		}
		case cot_ptr:
		{
			// a read of another type than the pointer's (`*(float*)p` where Hex-Rays kept p an int*)
			tinfo_t pointee = e->x->type.is_ptr() ? e->x->type.get_pointed_object() : tinfo_t();
			if (pointee.empty() || text(pointee) != text(e->type))
				missing(e, "reads " + text(e->type) + " through " + text(e->x->type));
			break;
		}
		case cot_asg:
			// C assigns a pointer to an integer and back without a cast
			if (e->x->type.is_ptr() != e->y->type.is_ptr() && e->y->op != cot_num)
				missing(e, "assigns " + text(e->y->type) + " to " + text(e->x->type));
			break;
		case cot_add:
		case cot_sub:
		case cot_idx:
		{
			// a constant offset from an untyped pointer variable or global cast to another (`(int*)pv + 18`,
			// `((char*)pv)[4]`): bytes of an object src gives no type there, whose offsets need not hold on another
			// layout (a call's result is left to the layers that type it)
			const cexpr_t* base = e->x;
			if (!base->type.is_ptr() || base->op != cot_cast || e->y->op != cot_num || e->y->n->_value == 0 ||
				(base->x->op != cot_var && base->x->op != cot_obj))
				break;
			tinfo_t from = base->x->type;
			if (from.is_ptr() && from.get_pointed_object().is_void())
				missing(e, "offset " + std::to_string(e->y->n->_value) + " from the untyped " + text(from));
			break;
		}
		case cot_memptr:
		case cot_memref:
		{
			// IDA's member is no src field until the Fields pass finds one (it gives every access its meaning), so the
			// body stays marked without it
			tinfo_t owner = e->op == cot_memptr ? e->x->type.get_pointed_object() : e->x->type;
			char offset[32];
			std::snprintf(offset, sizeof offset, "+0x%llX", static_cast<unsigned long long>(e->m));
			missing(e, "field " + text(owner) + offset + " not resolved");
			break;
		}
		default:
			break;
		}
	});
}

}  // namespace idakit
