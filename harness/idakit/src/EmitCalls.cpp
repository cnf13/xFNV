// Calls: each call's callee as a declared src function. A direct call takes the src function its user-defined call
// names (a fold owner: Function::namedCallees), else the src ref at the callee's address (the same choice as the
// function's own declaration); a call through an import's slot takes the src header global of
// function-pointer type there; a call through `obj->__vftable` takes the slot of the src class whose IDA vtable
// struct it reads: its primary vtable (`X_vtbl`), or the vtable of its base subobject at offset N (`X_%04X_vtbl`),
// whose slot is the base's (the class at N whose own vtable that is), called on that base subobject; so is a call
// through a local the exe loaded the slot into before it (hoistedSlot), the load then left out. The arguments
// must be of the declared parameter types (a pointer to the
// object for a member's `this`; a pointer where src takes a reference; integer literals pass as any integer), or of
// a type whose bits the exe passes alike (converts: the Conversions pass converts them), and, where the result is
// used, the result of the declared return type or one converted where it is used. A variadic callee's further
// arguments pass as C promotes them, without Hex-Rays' casts to the stack slot's 4-byte integer (peelVariadic). A
// Hex-Rays memset, qmemcpy or strlen that is exactly the CRT function's effect is src's msvc9 call of it, marked
// EXE-INLINED (inlinedCrt).

#include "EmitPasses.hpp"

#include "EmitScopes.hpp"
#include "IdaSupport.hpp"

#include "idakit/AsmFacts.hpp"
#include "idakit/SrcIda.hpp"

#include <Facts.hpp>

#include <algorithm>
#include <map>
#include <regex>
#include <set>
#include <unordered_set>

namespace idakit
{

namespace
{

const cexpr_t* addressOf(const cexpr_t* lvalue);

// the pointer an object expression is once casts and the address of a base subobject at offset 0 are peeled
const cexpr_t* objectPointer(const cexpr_t* e)
{
	auto baseAddress = [](const cexpr_t* e) {
		return e->op == cot_ref && (e->x->op == cot_memptr || e->x->op == cot_memref) && e->x->m == 0;
	};
	while (e->op == cot_cast || baseAddress(e))
	{
		if (e->op == cot_cast)
			e = e->x;
		else if (e->x->op == cot_memptr)
			e = e->x->x;
		else if (const cexpr_t* inner = addressOf(e->x->x))
			e = inner;
		else
			break;
	}
	return e;
}

// the pointer to an object lvalue (`*p`, `p->Base`, `x.Base` at offset 0), nullptr where there is none to peel to
const cexpr_t* addressOf(const cexpr_t* lvalue)
{
	if (lvalue->op == cot_ptr)
		return objectPointer(lvalue->x);
	if (lvalue->op == cot_memptr && lvalue->m == 0)
		return objectPointer(lvalue->x);
	if (lvalue->op == cot_memref && lvalue->m == 0)
		return addressOf(lvalue->x);
	return nullptr;
}

// the msvc9 function (src/SDK/MSVC/math.h) a Hex-Rays math helper call stands for, from the exe's instruction at the
// site: a __CIxxx call is that entry's _CIxxx (it makes the same call), the inline x87 fabs is _fabsIntrinsic (the
// bare instruction); a direct call of a __CIxxx entry Hex-Rays did not read as its helper (its one argument in ST0,
// which no fnvd::original form passes) is that _CIxxx too; "" for any other call or site
std::string mathFunction(const cexpr_t* call)
{
	// helper: Hex-Rays' name; callee: the __CI entry the site calls (BADADDR: an inline instruction); instruction: the
	// site's instruction
	struct Site
	{
		const char* helper;
		ea_t callee;
		uint16_t instruction;
		const char* function;
	};
	static const Site sites[] = {
		{"sqrt", 0xEC6040, NN_call, "msvc9::_CIsqrtCall"}, {"atan", 0xEC9050, NN_call, "msvc9::_CIatan"},
		{"fmod", 0xEC913A, NN_call, "msvc9::_CIfmod"},	  {"asin", 0xEC9650, NN_call, "msvc9::_CIasin"},
		{"pow", 0xEC9B00, NN_call, "msvc9::_CIpow"},		  {"cos", 0xEC9F70, NN_call, "msvc9::_CIcos"},
		{"sin", 0xECA0A0, NN_call, "msvc9::_CIsin"},		  {"acos", 0xECA1D0, NN_call, "msvc9::_CIacos"},
		{"exp", 0xECAE6C, NN_call, "msvc9::_CIexp"},		  {"fabs", BADADDR, NN_fabs, "msvc9::_fabsIntrinsic"},
	};
	insn_t insn;
	if (decode_insn(&insn, call->ea) <= 0)
		return "";
	ea_t callee = insn.itype == NN_call && insn.ops[0].type == o_near ? insn.ops[0].addr : BADADDR;
	bool direct = call->x->op == cot_obj && call->x->obj_ea == callee && call->a->size() == 1 &&
				  (*call->a)[0].type.is_floating();
	for (const Site& site : sites)
	{
		bool named = call->x->op == cot_helper && call->x->helper == std::string_view(site.helper);
		if ((named || (direct && site.callee != BADADDR)) && insn.itype == site.instruction && callee == site.callee)
			return site.function;
	}
	return "";
}

// a number's value as its type has it (a signed one sign-extended); nullopt for any other expression
std::optional<int64_t> numberValue(const cexpr_t* e)
{
	if (e->op != cot_num)
		return std::nullopt;
	uint64_t value = e->numval();
	size_t size = e->type.get_size();
	if (e->type.is_signed() && size > 0 && size < 8)
		return static_cast<int64_t>(value << (64 - size * 8)) >> (64 - size * 8);
	return static_cast<int64_t>(value);
}

// The object an address reaches and the address's byte offset into it (nullopt: not a constant), through casts,
// `&`, `*`, members (`p->m` and `x.m`: p's pointee, x), constant indexes and constant pointer steps. The base is a
// local (cot_var), a global (cot_obj) or any other expression (a call's result, arithmetic), itself.
std::pair<const cexpr_t*, std::optional<int64_t>> regionOf(const cexpr_t* e)
{
	std::optional<int64_t> offset = 0;
	auto add = [&](std::optional<int64_t> n) { offset = offset && n ? std::optional(*offset + *n) : std::nullopt; };
	for (;;)
	{
		switch (e->op)
		{
		case cot_cast: case cot_ref: case cot_ptr:
			e = e->x;
			continue;
		case cot_memref: case cot_memptr:
			add(static_cast<int64_t>(e->m));
			e = e->x;
			continue;
		case cot_idx:
		{
			std::optional<int64_t> index = numberValue(e->y);
			add(index ? std::optional(*index * static_cast<int64_t>(e->type.get_size())) : std::nullopt);
			e = e->x;
			continue;
		}
		case cot_add: case cot_sub:
			if (e->x->type.is_ptr() && !e->y->type.is_ptr())
			{
				std::optional<int64_t> step = numberValue(e->y);
				int64_t size = static_cast<int64_t>(e->x->type.get_pointed_object().get_size());
				add(step ? std::optional((e->op == cot_sub ? -*step : *step) * size) : std::nullopt);
				e = e->x;
				continue;
			}
			return {e, std::nullopt};
		default:
			return {e, offset};
		}
	}
}

// The CRT function (memset, memcpy, strlen) a Hex-Rays helper call of the exe's inlined sequence is exactly, for the
// arguments it prints (user, 2026-11-12: src writes the msvc9 call under `// EXE-INLINED none: <function>`); ""
// for any other call. Hex-Rays' memset(p, c, n) is the n bytes at p stored as the byte c (`rep stos` or a run of
// stores of one value), qmemcpy(d, s, n) the n bytes at s copied forward to d (`rep movs` or a run of moves), its
// strlen(s) the length `repne scasb` scans; memcpy's effect is the forward copy's only where the two regions do not
// overlap partially: not where both lie in one object (one local, global or pointer's pointee) unless their
// constant offsets set them n bytes apart at least. memset's value is a number of one byte; memset and qmemcpy
// are a statement each (Hex-Rays' result is no pointer the exe keeps). memset32 (dwords of a value) is no memset.
std::string inlinedCrt(const cexpr_t* call, bool statement)
{
	if (call->x->op != cot_helper)
		return "";
	insn_t insn;
	if (decode_insn(&insn, call->ea) <= 0 || insn.itype == NN_call)
		return "";
	std::string_view helper = call->x->helper;
	const carglist_t& args = *call->a;
	if (helper == "strlen")
		return args.size() == 1 ? "strlen" : "";
	if (!statement || args.size() != 3)
		return "";
	if (helper == "memset")
	{
		const cexpr_t* value = &args[1];
		while (value->op == cot_cast)
			value = value->x;
		std::optional<int64_t> byte = numberValue(value);
		return byte && *byte >= 0 && *byte <= 0xFF ? "memset" : "";
	}
	if (helper != "qmemcpy")
		return "";
	auto [destination, to] = regionOf(&args[0]);
	auto [source, from] = regionOf(&args[1]);
	bool same = destination->op == cot_var && source->op == cot_var ? destination->v.idx == source->v.idx
				: destination->op == cot_obj && source->op == cot_obj ? destination->obj_ea == source->obj_ea
																	  : destination->equal_effect(*source);
	if (!same)
		return "memcpy";
	std::optional<int64_t> count = numberValue(&args[2]);
	return to && from && count && (*to - *from >= *count || *from - *to >= *count) ? "memcpy" : "";
}

// The type holds no pointer and nothing pointer-sized at any depth (fixed-width integers, floats, enums, arrays and
// records of those, no vtable): its byte size is the same on x64, so a byte count of it is right there too
bool pointerFree(const Context& context, const std::string& key, int depth = 0)
{
	if (key.empty() || depth > 32 || hostWidth(context, key))
		return false;
	SrcTypes::Parts parts = context.types.parts(context.types.canonical(key));
	if (parts.kind == "builtin" || parts.kind == "enum")
		return parts.name != "void" && parts.name != "nullptr_t";
	if (parts.kind == "array")
		return pointerFree(context, parts.of, depth + 1);
	if (parts.kind != "record")
		return false;
	const SrcModel::Record* record = context.model.record(parts.name);
	if (!record || !record->vtables.empty())
		return false;
	for (const SrcModel::Field& field : record->fields)
		if (!pointerFree(context, field.type, depth + 1))
			return false;
	for (const SrcModel::Base& base : record->bases)
		if (!pointerFree(context, "record:" + base.record, depth + 1))
			return false;
	return true;
}

// The byte count of an inlined memset or memcpy (user, for x64): right as printed where it is the destination's
// sizeof (`sizeof(x)`, or `sizeof(T) * n` of its elements; checkRules judges the sizeof), or any count over a
// destination of pointer-free type, whose bytes are the same on x64. Any other count (a number over bytes that may
// hold pointers, or a destination src has no type for) is marked, naming the destination's type.
void markByteCount(Function& function, Context& context, const cexpr_t* call, const std::string& crt)
{
	const cexpr_t* count = stripCasts(&(*call->a)[2]);
	auto sized = [](const cexpr_t* x) { return stripCasts(x)->op == cot_sizeof; };
	if (sized(count) || (count->op == cot_mul && (sized(count->x) || sized(count->y))))
		return;
	std::string destination = pointedType(function, context, &(*call->a)[0]);
	if (pointerFree(context, destination))
		return;
	function.meaning[count] = Missing{"byte count of " + crt + " over " +
									  (destination.empty() ? std::string("a type src has no record of") : destination) +
									  ": its bytes may hold pointers, which are 8 bytes on x64"};
}

class Calls
{
public:
	// the body's TU: the src .cpp whose forwarder it replaces (a free function's, or a member's outside its class's
	// .cpp), else its class's; it sees that file's own declarations
	Calls(const Function& function, Context& context)
		: m_function(function), m_context(context), m_types(context.types),
		  m_tu(function.replaces ? function.replaces->file
			   : function.self	 ? context.model.classSource(function.self->cls)
								 : ""),
		  m_parents(parentsOf(function))
	{
	}

	// the meaning of the call
	Meaning resolve(const cexpr_t* call, bool resultUsed)
	{
		const cexpr_t* callee = call->x;
		Callee found = Missing{};
		if (callee->op == cot_obj)
		{
			// in data: an import's slot src may declare as a global of function-pointer type
			if (!get_func(callee->obj_ea))
				if (std::optional<Meaning> through = throughGlobal(call, resultUsed))
					return *through;
			ChosenCallee chosen = chosenCallee(m_function, m_context.model, call);
			if (!chosen.function)
			{
				// a fold without a registered body: what src lacks for the receiver's class to own the call
				bool fold = chosen.several && !m_function.namedCallees.count(call);
				return Missing{chosen.why +
							   (fold ? unowned(call, chooseRef(m_context.model, callee->obj_ea).names) : "")};
			}
			found = direct(constFold(call, *chosen.function));
		}
		else if (const cexpr_t* table = tableRead(slotOf(call)))
		{
			const SrcModel::Record* deleting = nullptr;
			found = virtualSlot(table, slotOffset(slotOf(call)), &deleting);
			// the object whose vtable is read must be the call's `this`: the same pointer, or the same object at the
			// vtable pointer's offset (`a[0].__vftable` of `a`, `x.__vftable_30` of `&x.Base`), or the exe's code
			// passes as `this` the very value it reads the vtable through, where Hex-Rays prints the two apart (a
			// copy of the pointer in a local and the field it was loaded from)
			const cexpr_t* object = table->op == cot_memptr ? objectPointer(table->x) : addressOf(table->x);
			bool own = !call->a->empty() && ((object && objectPointer(&(*call->a)[0])->equal_effect(*object)) ||
											 sameObject(tablePlace(table), pointerPlace(&(*call->a)[0])) ||
											 passesVtableObject(call));
			// a deletion through a hoisted slot reads the slot itself (resolvedLoads leaves the load out)
			if (deleting && own && (slotOf(call) == callee || hoistedSlot(call)))
				if (std::optional<Meaning> deletion = deleteCall(call, *deleting, resultUsed))
				{
					m_deletions.insert(call);
					return *deletion;
				}
			if (std::holds_alternative<const SrcModel::Function*>(found) && !own)
				return Missing{"vtable call whose this is not the vtable's object"};
			if (const SrcModel::Function* const* slot = std::get_if<const SrcModel::Function*>(&found))
				if (std::string why = castConvention(call, **slot); !why.empty())
					return Missing{why};
		}
		else if (std::optional<Meaning> cleanup = havokCleanup(call, resultUsed))
			return *cleanup;
		else
			return Missing{"indirect call through " + text(callee->type)};
		if (const Missing* missing = std::get_if<Missing>(&found))
			return *missing;
		const SrcModel::Function* target = std::get<const SrcModel::Function*>(found);
		// a direct call of a src virtual names its class: only a call through the vtable is a virtual call
		Meaning meaning = checked(call, target, resultUsed, callee->op == cot_obj && target->isVirtual);
		if (Call* c = std::get_if<Call>(&meaning); c && callee->op == cot_obj)
			c->exeCalled = visibleBody(callee->obj_ea, *target);
		return meaning;
	}

	// the call checked as a call of target, a function no src ref names (a function-pointer type's): checkCall
	Meaning check(const cexpr_t* call, const SrcModel::Function& target, bool resultUsed)
	{
		return checked(call, &target, resultUsed, false);
	}

	// The member C++ calls on the call's receiver: where the chosen one is a non-const member and the receiver, as src
	// types it, points to const (or is the address of a const object: a member of one reached through a pointer to
	// const), src's const overload of the same parameters that src names at the call's target too (a fold of one
	// address: `_Myptr` and `_Myptr const`); the chosen one for any other
	const SrcModel::Function& constFold(const cexpr_t* call, const SrcModel::Function& chosen)
	{
		const SrcModel& model = m_context.model;
		const SrcModel::Type* own = model.type(chosen.type);
		if (!own || own->isConst || chosen.isStatic || chosen.cls.empty() || call->a->empty())
			return chosen;
		const cexpr_t* receiver = &(*call->a)[0];
		while (receiver->op == cot_cast)
			receiver = receiver->x;
		std::string key = receiver->op == cot_ref ? "" : srcType(m_function, m_context, receiver);
		std::string kind = key.empty() ? "" : m_types.parts(m_types.canonical(key)).kind;
		// a reference parameter's receiver is Hex-Rays' pointer of the reference
		bool constReceiver = receiver->op == cot_ref ? srcConstObject(m_function, m_context, receiver->x)
												  : (kind == "ptr" || kind == "lref") && m_types.constPointee(key);
		if (!constReceiver)
			return chosen;
		const std::vector<SrcModel::Ref>& refs = model.refsAt(call->x->obj_ea);
		const SrcModel::Function* overload = nullptr;
		for (const SrcModel::Function* f : model.functionsNamed(chosen.name))
		{
			const SrcModel::Type* type = model.type(f->type);
			bool named = std::any_of(refs.begin(), refs.end(), [&](const SrcModel::Ref& r) {
				return r.function == f->mangled && (r.kind == "fold" || r.kind == "body" || r.kind == "forwarder");
			});
			if (f == &chosen || f->cls != chosen.cls || f->isStatic || !type || !type->isConst ||
				type->params != own->params || type->variadic != own->variadic || !named)
				continue;
			if (overload)
				return chosen;
			overload = f;
		}
		return overload ? *overload : chosen;
	}

	// A resolved call's arguments past a C-variadic callee's declared parameters: Unfold types each stack slot there
	// as a 4-byte integer, so Hex-Rays casts what the exe pushes to it. Casts that only reinterpret a 4-byte integer
	// or pointer as another, from the outside in, print as their operand (Implicit): it passes through `...` as the
	// same dword the exe pushes (a pointer as itself, which the integer cast would truncate on x64; an array decays
	// to its address). The operand, as src prints it, must be a pointer, an array, or a 4-byte integer or enum not of
	// a type whose width differs on x64; anything else keeps its casts. Judged once the operands' src types are final
	// (variadicCasts, from peelVariadicArguments): a nested call, a global, a local a later layer retypes.
	void peelVariadic(const cexpr_t* call, const Call& resolved)
	{
		const SrcModel::Type* type = m_context.model.type(resolved.function->type);
		if (!type || !type->variadic)
			return;
		auto dword = [](const tinfo_t& t) {
			return (t.is_ptr() || t.is_enum() || (t.is_integral() && !t.is_bool())) && t.get_size() == 4;
		};
		for (size_t k = 0; k < call->a->size(); ++k)
		{
			int parameter = parameterIndex(resolved, k);
			if (parameter < 0 || static_cast<size_t>(parameter) < type->params.size())
				continue;
			std::vector<const cexpr_t*> casts;
			const cexpr_t* operand = &(*call->a)[k];
			while (operand->op == cot_cast && dword(operand->type) &&
				   (dword(operand->x->type) || operand->x->type.is_array()))
			{
				casts.push_back(operand);
				operand = operand->x;
				if (operand->type.is_array())
					break;
			}
			if (casts.empty())
				continue;
			std::string why, key = srcType(m_function, m_context, operand);
			if (key.empty())
				key = m_types.keyOf(operand->type, why).value_or("");
			if (key.empty())
				continue;
			std::string kind = m_types.parts(m_types.canonical(key)).kind;
			std::optional<int64_t> size = m_context.model.sizeOf(key);
			if (kind == "ptr" || kind == "array" ||
				(scalar(key) == "integer" && size && *size == 4 && !hostWidth(m_context, key)))
				peeled.insert(peeled.end(), casts.begin(), casts.end());
		}
	}

	// the casts peelVariadic() found, for peelVariadicArguments to print as their operand
	std::vector<const cexpr_t*> peeled;

private:
	// a body clang could inline where the exe calls the address: src registers a body for it and defines it where the
	// caller's TU sees it (its definition in the TU or a header, or, a specialization the model has no definition row
	// for, its registration in the TU); or src defines the called function in a header with a body other than a
	// forwarder to the address (a template's member a fold owner's call names); or src's forwarder to the address is
	// defined in the caller's TU, whose body a later merge puts there (a run merges the caller and the callee together:
	// 0xC92860 and its 0xC927F0, both hkpWorld.cpp's), unless the exe's function is a trivial accessor (porting rules:
	// called or inlined freely)
	bool visibleBody(ea_t ea, const SrcModel::Function& target) const
	{
		const SrcModel::Definition* definition = m_context.model.definition(target.mangled);
		if (!m_context.model.registers(ea))
		{
			const std::vector<SrcModel::Ref>& refs = m_context.model.refsAt(ea);
			bool own = std::any_of(refs.begin(), refs.end(),
								   [&](const SrcModel::Ref& ref) { return ref.function == target.mangled; });
			return definition && ((isHeader(definition->file) && !own) || definition->file == m_tu) &&
				   !m_context.exe.trivialAccessor(static_cast<uint32_t>(ea));
		}
		if (definition)
			return definition->file == m_tu || isHeader(definition->file);
		for (const SrcModel::Ref& ref : m_context.model.refsAt(ea))
			if (ref.kind == "body" && ref.file == m_tu)
				return true;
		return false;
	}

	using Callee = std::variant<const SrcModel::Function*, Missing>;

	// the src function chosen at a direct call (chosenCallee), where the body's TU can call it
	Callee direct(const SrcModel::Function& f)
	{
		if (!isHeader(f.file) && f.file != m_tu)
			return Missing{"callee " + f.name + " is declared only in " + f.file +
						   " (another src .cpp than the body's)"};
		// a template instance's member the body's TU cannot call: one src specializes only in another .cpp (its
		// specialization declared in no header, `declare --place-specializations`), or one made from its template's
		// definition in another .cpp
		if (std::string why = unseenDefinition(f); !why.empty())
			return Missing{"callee " + f.name + why};
		return &f;
	}

	// What src lacks for a member call of a fold without a registered body to have an owner on its receiver (Unfold's
	// fold owners): ": src declares no R::m" where neither the receiver's src class R nor a base at offset 0 declares
	// the members' one name m, else why the nearest declaration is none; "" where the members have several names or
	// are no members, or the receiver has no src class.
	std::string unowned(const cexpr_t* call, const std::set<std::string>& names)
	{
		std::string member;
		for (const std::string& n : names)
		{
			size_t scope = n.rfind("::");
			if (scope == std::string::npos || (!member.empty() && n.substr(scope + 2) != member))
				return "";
			member = n.substr(scope + 2);
		}
		if (member.empty() || call->a->empty())
			return "";
		const cexpr_t* receiver = objectPointer(&(*call->a)[0]);
		std::string why, key = srcType(m_function, m_context, receiver);
		if (key.empty())
			key = m_types.keyOf(receiver->type, why).value_or("");
		SrcTypes::Parts pointer = m_types.parts(m_types.canonical(key));
		SrcTypes::Parts pointee = pointer.kind == "ptr" ? m_types.parts(unconst(pointer.of)) : SrcTypes::Parts{};
		const SrcModel::Record* record = pointee.kind == "record" ? m_context.model.record(pointee.name) : nullptr;
		if (!record)
			return "";
		// the receiver's class and its bases at offset 0, nearest first
		std::vector<const SrcModel::Record*> level{record};
		std::set<std::string> seen;
		while (!level.empty())
		{
			std::vector<const SrcModel::Record*> next;
			for (const SrcModel::Record* r : level)
			{
				if (!seen.insert(r->name).second)
					continue;
				const std::vector<const SrcModel::Function*>& declared =
					m_context.model.functionsNamed(r->name + "::" + member);
				if (!declared.empty())
					return ": " + notOwner(*declared.front(), call);
				for (const SrcModel::Base& b : r->bases)
					if (const SrcModel::Record* base = m_context.model.record(b.record); base && b.offset == 0 &&
																						  !b.isVirtual)
						next.push_back(base);
			}
			level = std::move(next);
		}
		return ": src declares no " + record->name + "::" + member;
	}

	// why the receiver's declared member is no fold owner of the call, as far as emit sees it: declared only in
	// another .cpp, a template instance's member with no definition the body's TU sees (none at all: src lacks its
	// specialization), not named at the fold (src lacks its fold comment), another argument count; else Unfold's log
	// row says why
	std::string notOwner(const SrcModel::Function& f, const cexpr_t* call) const
	{
		const ea_t ea = call->x->obj_ea;
		if (!isHeader(f.file) && f.file != m_tu)
			return f.name + " is declared only in " + f.file;
		if (std::string why = unseenDefinition(f); !why.empty())
			return f.name + why;
		auto specializations = m_context.model.specializations().find(f.mangled);
		bool specialized =
			specializations != m_context.model.specializations().end() && !specializations->second.empty();
		if (templateInstance(f.name) && !m_context.model.definition(f.mangled) && !specialized)
			return f.name + " is declared without a definition (src lacks its specialization)";
		const std::vector<SrcModel::Ref>& refs = m_context.model.refsAt(ea);
		if (!templateInstance(f.name) &&
			std::none_of(refs.begin(), refs.end(), [&](const SrcModel::Ref& r) { return r.function == f.mangled; }))
			return f.name + " is not named at " + hex(ea) + " (src lacks its `Same function as` comment)";
		const SrcModel::Type* type = m_context.model.type(f.type);
		if (type && !f.isStatic && type->params.size() + 1 != call->a->size())
			return f.name + " takes " + std::to_string(type->params.size() + 1) + " arguments, the call " +
				   std::to_string(call->a->size());
		return f.name + " is no owner here (unfold-log fold-unresolved)";
	}

	// " is specialized only in F" / " is made from its template's definition only in F" where the callee is a
	// template instance's member the body's TU cannot call (an explicit specialization with no declaration in a
	// header or the TU; an implicit instance whose template's definition is in another .cpp), "" where it can (a
	// member of a class template's explicit specialization is any class's member)
	std::string unseenDefinition(const SrcModel::Function& f) const
	{
		if (!templateInstance(f.name))
			return "";
		auto specializations = m_context.model.specializations().find(f.mangled);
		if (specializations != m_context.model.specializations().end() && !specializations->second.empty())
		{
			for (const SrcModel::Specialization& s : specializations->second)
				if ((isHeader(s.file) && s.file.starts_with("src/")) || s.file == m_tu)
					return "";
			return " is specialized only in " + specializations->second[0].file;
		}
		const SrcModel::Definition* definition = m_context.model.definition(f.mangled);
		if (definition && definition->instance && !isHeader(definition->file) && definition->file != m_tu)
			return " is made from its template's definition only in " + definition->file;
		return "";
	}

	// the qualified name names a template's instance or specialization: template arguments in its scope or its own
	// name, an operator's own `<` (`operator<<`, `operator<`) aside
	static bool templateInstance(const std::string& name)
	{
		size_t scope = name.rfind("::");
		size_t own = scope == std::string::npos ? 0 : scope + 2;
		std::string checked = name.compare(own, 8, "operator") == 0 ? name.substr(0, own) : name;
		return checked.find('<') != std::string::npos;
	}

	// A call through a src global of function-pointer type declared in a header at the callee's address (an import's
	// slot: `InterlockedDecrementUInt32(p)`), checked as a call of a function of the global's name and its pointee's
	// type, whose convention must be the one Hex-Rays calls with. Of several such globals at the address, the first
	// whose parameters take every argument as it is, else the first that resolves; nullopt where src declares none.
	std::optional<Meaning> throughGlobal(const cexpr_t* call, bool resultUsed)
	{
		const cexpr_t* callee = call->x;
		tinfo_t called = callee->type.is_ptr() ? callee->type.get_pointed_object() : callee->type;
		std::string cc = conventionName(called.get_cc());
		std::optional<Meaning> chosen;
		for (const SrcModel::Ref& ref : m_context.model.refsAt(callee->obj_ea))
		{
			if (placeholder(ref) || ref.kind != "global" || !isHeader(ref.file))
				continue;
			SrcTypes::Parts pointer = m_types.parts(m_types.bare(ref.type));
			const SrcModel::Type* fn = pointer.kind == "ptr" ? m_context.model.type(pointer.of) : nullptr;
			if (!fn || fn->kind != "fn")
				continue;
			m_converted = 0;
			Meaning meaning = fn->cc != cc ? Meaning{Missing{"call through " + ref.target + ", " + fn->cc +
															 ", in Hex-Rays' " + cc}}
										   : checked(call, &calledGlobal(ref, pointer.of), resultUsed, false);
			bool resolved = std::holds_alternative<Call>(meaning);
			if (resolved && m_converted == 0)
				return meaning;
			if (!chosen || (resolved && !std::holds_alternative<Call>(*chosen)))
				chosen = std::move(meaning);
		}
		return chosen;
	}

	// the function a call through the global names (Context::calledGlobals)
	const SrcModel::Function& calledGlobal(const SrcModel::Ref& global, const std::string& fn)
	{
		auto [found, added] = m_context.calledGlobals.try_emplace(&global);
		if (added)
		{
			found->second.name = global.target;
			found->second.type = fn;
			found->second.file = global.file;
			found->second.line = global.line;
		}
		return found->second;
	}

	// The slot read under Hex-Rays' casts of the callee from one function pointer type to another (the vtable
	// struct's slot type is not the prototype the call is made with): the slot is the callee all the same, and src's
	// declaration of it types the call; the callee itself where it is no such cast.
	static const cexpr_t* slotRead(const cexpr_t* callee)
	{
		while (callee->op == cot_cast && callee->type.is_funcptr() && callee->x->type.is_funcptr())
			callee = callee->x;
		return callee;
	}

	// a callee cast to another function pointer type, or a local holding the slot (hoistedSlot), is called with that
	// type's convention: "" where it is src's slot's (a C-variadic member's is cdecl), else why not
	std::string castConvention(const cexpr_t* call, const SrcModel::Function& slot)
	{
		const cexpr_t* callee = call->x;
		if (slotOf(call) == callee)
			return "";
		const SrcModel::Type* type = m_context.model.type(slot.type);
		std::string called = conventionName(callee->type.get_pointed_object().get_cc());
		if (called == "vararg")
			called = "cdecl";
		if (type && type->cc == called)
			return "";
		return "vtable slot " + slot.name + " called as " + called + ", src " + (type ? type->cc : "unknown");
	}

	// the vtable slot read a call goes through: its callee's (slotRead), or the load's where the callee is a local
	// the exe loaded the slot into (hoistedSlot)
	const cexpr_t* slotOf(const cexpr_t* call)
	{
		const cexpr_t* read = slotRead(call->x);
		const Hoisted* hoisted = hoistedSlot(call);
		return hoisted ? slotRead(hoisted->load->y) : read;
	}

public:
	// the statements loading a vtable slot into a local (hoistedSlot) whose every call resolved through it to a
	// call of the slot's function, or to a deletion (deleteCall): src writes nothing for them (each call reads the
	// slot itself)
	std::vector<const cinsn_t*> resolvedLoads() const
	{
		std::vector<const cinsn_t*> out;
		if (!m_hoisted)
			return out;
		for (const auto& [v, local] : *m_hoisted)
			if (local.every && std::all_of(local.calls.begin(), local.calls.end(), [&](const cexpr_t* call) {
					auto found = m_function.meaning.find(call);
					const Call* c = found != m_function.meaning.end() ? std::get_if<Call>(&found->second) : nullptr;
					return (c && !c->through) || m_deletions.count(call);
				}))
				out.push_back(local.hoisted.statement);
		return out;
	}

private:
	// A local the exe loaded a vtable slot into before calling through it (a register holding `p->__vftable->F`
	// across the argument set-up: `f = p->__vftable->F; ...; f(p, ...)`), which is the call `p->F(...)` that reads
	// the same slot: the local is written once, by that load (a statement of its own, the object p a local or
	// parameter), its address is never taken, every other use of it is a call's callee, each call is in the load's
	// block after it (or in a statement of that block after it), and nothing from the load's next statement through
	// the call's writes p. The load, nullptr for any other callee.
	struct Hoisted
	{
		const cexpr_t* load = nullptr;	// the assignment
		const cinsn_t* statement = nullptr;
	};
	const Hoisted* hoistedSlot(const cexpr_t* call)
	{
		int v = lvarIndex(slotRead(call->x));
		if (v < 0)
			return nullptr;
		if (!m_hoisted)
			findHoisted();
		auto found = m_hoisted->find(v);
		if (found == m_hoisted->end() || !found->second.calls.count(call))
			return nullptr;
		return &found->second.hoisted;
	}

	// a hoisted local, the calls through it hoistedSlot() holds for, and whether those are all of them
	struct HoistedLocal
	{
		Hoisted hoisted;
		std::set<const cexpr_t*> calls;
		bool every = false;
	};
	// the locals hoistedSlot() takes, with the calls through each that it holds for
	void findHoisted()
	{
		m_hoisted.emplace();
		const lvars_t& lvars = *m_function.cfunc->get_lvars();
		std::map<int, std::vector<std::pair<const cexpr_t*, const cinsn_t*>>> writes, calls;
		std::set<int> other;  // used otherwise: as a value, its address taken
		std::set<const cexpr_t*> callees;
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t* s) {
			if ((written(e) || e->op == cot_ref) && lvarIndex(e->x) >= 0)
				writes[lvarIndex(e->x)].emplace_back(e, s);
			if (e->op == cot_call && lvarIndex(slotRead(e->x)) >= 0)
			{
				calls[lvarIndex(slotRead(e->x))].emplace_back(e, s);
				callees.insert(slotRead(e->x));
			}
		});
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			if (int v = lvarIndex(e); v >= 0 && !callees.count(e))
			{
				auto up = m_parents.find(e);
				bool assigned = up != m_parents.end() && up->second->is_expr() &&
								static_cast<const cexpr_t*>(up->second)->op == cot_asg &&
								static_cast<const cexpr_t*>(up->second)->x == e;
				if (!assigned)
					other.insert(v);
			}
		});
		Scopes scopes(m_function);
		for (const auto& [v, through] : calls)
		{
			auto write = writes.find(v);
			if (lvars[v].is_arg_var() || other.count(v) || write == writes.end() || write->second.size() != 1)
				continue;
			auto [load, statement] = write->second[0];
			const cexpr_t* table = load->op == cot_asg ? tableRead(slotRead(load->y)) : nullptr;
			int object = table && table->op == cot_memptr ? lvarIndex(objectPointer(table->x)) : -1;
			const cinsn_t* block = scopes.parent(statement);
			if (object < 0 || statement->op != cit_expr || statement->cexpr != load || !block ||
				block->op != cit_block)
				continue;
			std::vector<const cinsn_t*> items;
			for (const cinsn_t& item : *block->cblock)
				items.push_back(&item);
			size_t at = std::find(items.begin(), items.end(), statement) - items.begin();
			auto writesObject = [&](const cinsn_t* s) {
				bool writes = false;
				forEachStatement(s, [&](const cinsn_t* t) {
					for (const cexpr_t* own : ownExpressions(t))
						forEachSubexpression(own, [&](const cexpr_t* e) {
							writes |= (written(e) || e->op == cot_ref) && lvarIndex(e->x) == object;
						});
				});
				return writes;
			};
			HoistedLocal local{{load, statement}, {}};
			for (const auto& [call, holder] : through)
			{
				// the statement of the load's block holding the call
				const cinsn_t* top = holder;
				while (top && scopes.parent(top) != block)
					top = scopes.parent(top);
				size_t to = std::find(items.begin(), items.end(), top) - items.begin();
				if (!top || to <= at || to >= items.size())
					continue;
				bool kept = true;
				for (size_t k = at + 1; k <= to && kept; ++k)
					kept = !writesObject(items[k]);
				if (kept)
					local.calls.insert(call);
			}
			local.every = local.calls.size() == through.size();
			if (!local.calls.empty())
				m_hoisted->emplace(v, std::move(local));
		}
	}

	std::optional<std::unordered_map<int, HoistedLocal>> m_hoisted;
	// the calls deleteCall printed as a deletion (resolvedLoads)
	std::unordered_set<const cexpr_t*> m_deletions;

	// The read of the object's vtable pointer when the callee is a slot of it: `table->slot`, or `table->Base.slot`
	// (IDA's vtable structs hold their bases' as members); nullptr for any other callee. The pointer is the object's
	// member at its offset: 0 for its primary vtable, a base subobject's for that one's (`X_%04X_vtbl`).
	static const cexpr_t* tableRead(const cexpr_t* callee)
	{
		while (callee->op == cot_memref)
			callee = callee->x;
		if (callee->op != cot_memptr)
			return nullptr;
		const cexpr_t* table = callee->x;
		bool member = table->op == cot_memptr || table->op == cot_memref;
		return member && table->type.is_ptr() ? table : nullptr;
	}

	// Where an object is: a root expression (a pointer, or an lvalue where `lvalue`) and a byte offset into what it
	// designates, through base subobjects (Fields'), `*p` and `a[0]` (an array's first element, a pointer's pointee)
	struct Place
	{
		const cexpr_t* root = nullptr;
		bool lvalue = false;
		int64_t offset = 0;
	};

	Place pointerPlace(const cexpr_t* e) const
	{
		while (e->op == cot_cast)
			e = e->x;
		return e->op == cot_ref ? lvaluePlace(e->x) : Place{e, false, 0};
	}

	Place lvaluePlace(const cexpr_t* e) const
	{
		if (e->op == cot_ptr)
			return pointerPlace(e->x);
		if (e->op == cot_idx && e->y->op == cot_num && e->y->numval() == 0)
			return pointerPlace(e->x);
		if ((e->op == cot_memptr || e->op == cot_memref) && baseSubobject(e))
		{
			Place place = e->op == cot_memptr ? pointerPlace(e->x) : lvaluePlace(e->x);
			place.offset += e->m;
			return place;
		}
		return Place{e, true, 0};
	}

	// the object whose vtable pointer the table read reads, at that pointer's offset
	Place tablePlace(const cexpr_t* table) const
	{
		Place place = table->op == cot_memptr ? pointerPlace(table->x) : lvaluePlace(table->x);
		place.offset += table->m;
		return place;
	}

	static bool sameObject(const Place& a, const Place& b)
	{
		return a.lvalue == b.lvalue && a.offset == b.offset && a.root->equal_effect(*b.root);
	}

	// the slot's offset in the vtable
	static uint64_t slotOffset(const cexpr_t* callee)
	{
		uint64_t offset = 0;
		for (; callee->op == cot_memref; callee = callee->x)
			offset += callee->m;
		return offset + callee->m;
	}

	// the src class's slot at the offset in the vtable `table` reads: its IDA `X_vtbl` struct's primary vtable, or
	// `X_%04X_vtbl`'s, the vtable of X's base subobject at that offset, whose slot is that base's (the class there
	// whose own vtable it is: the call is made on the base subobject); deleting: set to that class where the slot is
	// its deleting destructor (the primary vtable only)
	Callee virtualSlot(const cexpr_t* table, uint64_t offset, const SrcModel::Record** deleting)
	{
		qstring tableName;
		table->type.get_pointed_object().get_final_type_name(&tableName);
		std::string name = tableName.c_str();
		if (name.size() <= 5 || name.compare(name.size() - 5, 5, "_vtbl") != 0)
			return Missing{"call through " + text(table->type)};
		std::string owner = name.substr(0, name.size() - 5);
		int64_t at = 0;
		// `X_%04X_vtbl`: X's vtable at that offset, unless IDA has a type of the whole name (`X_0030` itself)
		static const std::regex secondary(R"(^(.+)_([0-9A-F]{4})$)");
		tinfo_t ownerType;
		if (std::smatch m;
			std::regex_match(owner, m, secondary) && !ownerType.get_named_type(get_idati(), owner.c_str()))
		{
			owner = m[1];
			at = std::stoll(m[2], nullptr, 16);
		}
		const SrcModel::Record* record = nullptr;
		if (ownerType.get_named_type(get_idati(), owner.c_str()))
			record = m_types.recordOf(ownerType);
		if (!record)
			return Missing{"no src class for vtable " + name};
		const SrcModel::Vtable* vtable = nullptr;
		for (const SrcModel::Vtable& v : record->vtables)
			if (v.offset == at)
				vtable = &v;
		// a base subobject's vtable: the slot of the class there whose primary vtable it is
		const SrcModel::Record* holder = at == 0 ? record : vtableBase(*record, at);
		const SrcModel::Vtable* primary = nullptr;
		if (holder)
			for (const SrcModel::Vtable& v : holder->vtables)
				if (v.offset == 0)
					primary = &v;
		size_t index = offset / 4;
		if (!vtable || !primary || offset % 4 != 0 || index >= vtable->slots.size() ||
			index >= primary->slots.size())
			return Missing{"no slot " + std::to_string(index) + " in src's vtable of " + record->name +
						   (at ? " at " + hex(static_cast<uint64_t>(at)) : "")};
		const SrcModel::Slot& slot = primary->slots[index];
		if (slot.kind == "deleting" && at == 0)
			*deleting = record;
		if (slot.kind != "method")
			return Missing{"vtable slot " + std::to_string(index) + " of " + holder->name + " is a " + slot.kind};
		const SrcModel::Function* f = m_context.model.function(slot.function);
		if (!f)
			return Missing{"src slot " + slot.name + " not in the model"};
		return f;
	}

	// the base subobject of the record at the byte offset whose own vtable is at its start (the most derived one on
	// the way there), nullptr for none
	const SrcModel::Record* vtableBase(const SrcModel::Record& record, int64_t offset) const
	{
		for (const SrcModel::Base& b : record.bases)
		{
			const SrcModel::Record* base = m_context.model.record(b.record);
			if (!base || b.isVirtual || b.offset > offset || b.offset + base->size <= offset)
				continue;
			if (b.offset == offset)
				for (const SrcModel::Vtable& v : base->vtables)
					if (v.offset == 0)
						return base;
			if (const SrcModel::Record* inner = vtableBase(*base, offset - b.offset))
				return inner;
		}
		return nullptr;
	}

	// A call of the deleting destructor's slot of the record's vtable whose result nothing reads, on a pointer to a src
	// class deriving at offset 0 from the record (whose virtual destructor the slot is) whose destructor src declares
	// (destructorWhy), with a literal flag: 1 is C++'s `delete p`, 0 its explicit destructor call `p->~X()`, each of
	// which MSVC's ABI (clang's MS ABI too) makes as this very call of the slot, `delete p` behind a null test the
	// exe's call lacks (user, 2026-11-12: printed `delete p` all the same; the two differ only for a null p, where the
	// exe faults on the vtable read); marked where the class's implicit destructor is one C++ cannot define there;
	// nullopt for any other (another flag: an array's or a value; no destructor src declares)
	std::optional<Meaning> deleteCall(const cexpr_t* call, const SrcModel::Record& record, bool resultUsed)
	{
		const carglist_t& args = *call->a;
		// the flag under Hex-Rays' casts of the literal to the slot's parameter type (`(uint32_t)1`)
		const cexpr_t* flag = args.size() == 2 ? stripCasts(&args[1]) : nullptr;
		if (resultUsed || !flag || flag->op != cot_num || flag->numval() > 1)
			return std::nullopt;
		std::string object = srcType(m_function, m_context, &args[0]), why;
		if (object.empty())
			if (std::optional<std::string> key = keyOf(&args[0], why))
				object = *key;
		SrcTypes::Parts pointer = m_types.parts(m_types.canonical(object));
		SrcTypes::Parts pointee = pointer.kind == "ptr" ? m_types.parts(unconst(pointer.of)) : SrcTypes::Parts{};
		const SrcModel::Record* cls = pointee.kind == "record" ? m_context.model.record(pointee.name) : nullptr;
		if (!cls || !m_types.derivesAtZero(cls->name, record.name))
			return std::nullopt;
		std::optional<std::string> undefinable = destructorWhy(*cls);
		if (!undefinable)
			return std::nullopt;
		if (!undefinable->empty())
			return Missing{"deleting destructor of " + cls->name + ": " + *undefinable};
		std::string header = isHeader(cls->file) && cls->file.starts_with("src/") ? cls->file.substr(4) : "";
		std::string form = flag->numval() == 1 ? "delete $0" : "$0->~" + destructorName(*cls) + "()";
		return Rewrite{1, form, {&args[0]}, {}, "builtin:void", header};
	}

	// Whether C++ may call the record's virtual destructor in the body's TU: "" where the record declares its own
	// destructor, or where its destructor is implicit, a base (at any depth) declares its own virtual one, and the
	// implicit one's definition, which C++ makes in the TU calling it, instantiates no class template's destructor
	// (`NiPointer<T>`'s needs T complete, which the TU may not have: templateDestructor); why not where that last
	// fails; nullopt where src declares no virtual destructor of it. srcdb records an implicit destructor at its
	// class's own line.
	std::optional<std::string> destructorWhy(const SrcModel::Record& record) const
	{
		if (declaresDestructor(record, false))
			return "";
		if (!inheritsVirtualDestructor(record))
			return std::nullopt;
		if (std::string instance = templateDestructor(record); !instance.empty())
			return record.name + "'s destructor is implicit, and C++ would define it here, instantiating " + instance +
				   "'s destructor (src declares no ~" + destructorName(record) + ")";
		return "";
	}

	// the record declares its own destructor (a virtual one where isVirtual)
	bool declaresDestructor(const SrcModel::Record& record, bool isVirtual) const
	{
		const std::vector<const SrcModel::Function*>& destructors =
			m_context.model.functionsNamed(record.name + "::~" + destructorName(record));
		return std::any_of(destructors.begin(), destructors.end(), [&](const SrcModel::Function* f) {
			return f->structor == "dtor" && !(f->file == record.file && f->line == record.line) &&
				   (!isVirtual || f->isVirtual);
		});
	}

	// a base of the record (at any depth) declares its own virtual destructor
	bool inheritsVirtualDestructor(const SrcModel::Record& record) const
	{
		for (const SrcModel::Base& b : record.bases)
			if (const SrcModel::Record* base = m_context.model.record(b.record);
				base && (declaresDestructor(*base, true) || inheritsVirtualDestructor(*base)))
				return true;
		return false;
	}

	// The class template instance whose declared destructor the record's implicit destructor calls, through its bases
	// and by-value members (arrays of them too) and theirs with implicit destructors; "" for none
	std::string templateDestructor(const SrcModel::Record& record) const
	{
		auto instance = [&](const SrcModel::Record& part) -> std::string {
			if (declaresDestructor(part, false))
				return part.pattern.empty() ? "" : part.name;
			return templateDestructor(part);
		};
		for (const SrcModel::Base& b : record.bases)
			if (const SrcModel::Record* base = m_context.model.record(b.record))
				if (std::string found = instance(*base); !found.empty())
					return found;
		for (const SrcModel::Field& f : record.fields)
		{
			SrcTypes::Parts type = m_types.parts(m_types.canonical(f.type));
			while (type.kind == "array")
				type = m_types.parts(m_types.canonical(type.of));
			if (const SrcModel::Record* member = type.kind == "record" ? m_context.model.record(type.name) : nullptr)
				if (std::string found = instance(*member); !found.empty())
					return found;
		}
		return "";
	}

	// the destructor's name after `~`: the record's own name without its scope and template arguments
	static std::string destructorName(const SrcModel::Record& record)
	{
		std::string own = lastComponent(record.name);
		return own.substr(0, own.find('<'));
	}

	// Havok's cleanupLoadedObject<TEXT> callback, which the SDK defines with HK_REFLECTION_DEFINE_INTERNAL_CLEANUP
	// (hkTypeInfo.h) as `static_cast<C*>(p)->~C()` on its `void* p`, C the class TEXT names (its qualified name
	// without the `::`): the exe's whole body is that call, of slot 0 of p's vtable with flag 0. Printed so where the
	// body is the call alone, on the function's first parameter, its result unused, and src's C has a virtual
	// destructor C++ may call there (destructorWhy) in its primary vtable's slot 0; marked with the class named where
	// the function is such a callback and anything of that fails; nullopt for any other function.
	std::optional<Meaning> havokCleanup(const cexpr_t* call, bool resultUsed)
	{
		static const std::string prefix = "cleanupLoadedObject";
		const SrcModel::Function* self = m_function.self;
		if (!self || !self->cls.empty() || !self->name.starts_with(prefix) || self->name.size() == prefix.size())
			return std::nullopt;
		std::string named = self->name.substr(prefix.size());
		const SrcModel::Record* cls = nullptr;
		for (const auto& [name, record] : m_context.model.records())
		{
			std::string flat = name;
			for (size_t at; (at = flat.find("::")) != std::string::npos;)
				flat.erase(at, 2);
			if (flat != named)
				continue;
			if (cls)
				return Missing{"Havok cleanup of " + named + ": several src classes of that name"};
			cls = &record;
		}
		std::string what = "Havok cleanup of " + (cls ? cls->name : named);
		if (!cls)
			return Missing{what + ": no src class of that name"};
		const SrcModel::Vtable* primary = nullptr;
		for (const SrcModel::Vtable& v : cls->vtables)
			if (v.offset == 0)
				primary = &v;
		std::optional<std::string> undefinable = destructorWhy(*cls);
		if (!primary || primary->slots.empty() || primary->slots[0].kind != "deleting" || !undefinable)
			return Missing{what + ": src's class has no virtual destructor in its slot 0"};
		if (!undefinable->empty())
			return Missing{what + ": " + *undefinable};
		// the body: this call as its one statement
		const cinsn_t& body = m_function.cfunc->body;
		bool alone = body.op == cit_block && body.cblock->size() == 1 && body.cblock->front().op == cit_expr &&
					 body.cblock->front().cexpr == call;
		// the callee: `**(fn**)p`, slot 0 of p's vtable, p the first parameter, which the call passes as this
		auto parameter = [&](const cexpr_t* e) {
			int v = lvarIndex(stripCasts(e));
			return v >= 0 && m_function.locals[v].parameter == 0 ? v : -1;
		};
		const cexpr_t* slot = stripCasts(call->x);
		const cexpr_t* table = slot->op == cot_ptr ? stripCasts(slot->x) : nullptr;
		int object = table && table->op == cot_ptr ? parameter(table->x) : -1;
		const carglist_t& args = *call->a;
		if (!alone || resultUsed || object < 0 || args.size() != 2 || parameter(&args[0]) != object ||
			stripCasts(&args[1])->op != cot_num || stripCasts(&args[1])->numval() != 0)
			return Missing{what + ": the body is not the destructor's call of slot 0 on its parameter"};
		std::optional<std::string> spelled = m_types.spell("record:" + cls->name);
		if (!spelled)
			return Missing{what + ": src cannot spell the class"};
		std::string header = isHeader(cls->file) && cls->file.starts_with("src/") ? cls->file.substr(4) : "";
		return Rewrite{1, "static_cast<" + *spelled + "*>($0)->~" + destructorName(*cls) + "()",
					   {stripCasts(&args[0])}, {}, "builtin:void", header};
	}

	// the argument's key (a floating constant's own width, the Types pass's); nullopt with why for an unresolved one
	std::optional<std::string> keyOf(const cexpr_t* e, std::string& why)
	{
		if (std::string key = e->op == cot_fnum ? srcType(m_function, m_context, e) : ""; !key.empty())
			return key;
		return m_types.keyOf(e->type, why);
	}

	// the object a record returned by value goes into, through the hidden result pointer args[index]: "" where C++
	// writes the call as an assignment to it, else why not. A plain local of the returned type (`local = F(...)`), or
	// raw storage holding exactly that record (resolveTypes'), which the locals passes may take back (else it stays
	// marked as storage); or any other object the argument points to as an object of the returned type, not const:
	// the body's own hidden result (Self's `result = F(...)`, which every return returns), the object a pointer or
	// reference parameter or local points to (`*apResult = F(...)`), a member or element (`x.m = F(...)`, `a[i] =
	// F(...)`), an array of the record decaying to its first element (`*a = F(...)`). Declarations decides whether
	// C++'s assignment does what the exe does there (a byte copy). Not an argument with effects (C++ evaluates an
	// assignment's object after its value) or one another argument passes too (the exe builds the result in an
	// object the callee reads, where C++ reads it before the assignment).
	std::string resultObject(const carglist_t& args, int index, const std::string& returned)
	{
		const carg_t& result = args[index];
		int v = result.op == cot_ref ? lvarIndex(result.x) : -1;
		if (const Local* local = v >= 0 ? &m_function.locals[v] : nullptr; local && !local->reference)
		{
			const std::string& held = local->storage ? local->holds : local->type;
			return !held.empty() && m_types.canonical(held) == m_types.canonical(returned)
					   ? ""
					   : "a local of another type";
		}
		std::string why;
		std::string printed = srcType(m_function, m_context, &result);
		if (printed.empty())
			printed = keyOf(&result, why).value_or("");
		SrcTypes::Parts pointer = m_types.parts(m_types.bare(printed));
		if ((pointer.kind != "ptr" && pointer.kind != "lref" && pointer.kind != "array") ||
			m_types.canonical(pointer.of) != m_types.canonical(returned))
			return "no object of its type (" + (printed.empty() ? why : printed) + ")";
		if (m_types.parts(m_types.bare(pointer.of)).kind == "const")
			return "a const object (" + printed + ")";
		if (result.has_side_effects())
			return "an object an expression with effects names";
		for (size_t k = 0; k < args.size(); ++k)
			if (static_cast<int>(k) != index && stripCasts(&args[k])->equal_effect(*stripCasts(&result)))
				return "an object argument " + std::to_string(k + 1) + " passes too";
		return "";
	}

	Meaning checked(const cexpr_t* call, const SrcModel::Function* target, bool resultUsed, bool qualified)
	{
		const SrcModel::Type* type = m_context.model.type(target->type);
		const SrcModel::Abi* abi = m_context.model.abi(target->mangled);
		if (!type)
			return Missing{"type of " + target->name + " not in the model"};
		// a C++ constructor or destructor has no call syntax (only Construct/Destruct do, RULES)
		if (!target->structor.empty())
			return Missing{"callee " + target->name + " is a C++ " +
						   (target->structor == "ctor" ? "constructor" : "destructor")};
		const carglist_t& args = *call->a;
		bool member = !target->cls.empty() && !target->isStatic;
		// a record returned by value: MSVC passes the result's address after `this` (clang's MS ABI does the same) and
		// returns it in eax
		bool byValue = abi && abi->ret == "indirect";
		size_t first = (member ? 1 : 0) + (byValue ? 1 : 0);
		// src's apEdx for the exe's thiscall: no argument of Hex-Rays' fills it (not with a hidden result pointer,
		// which the two conventions need not pass alike)
		tinfo_t called = call->x->type.is_ptr() ? call->x->type.get_pointed_object() : call->x->type;
		int edx = byValue ? -1 : edxParameter(*target, *type, conventionName(called.get_cc()));
		size_t expected = type->params.size() + first - (edx >= 0 ? 1 : 0);
		if (type->variadic ? args.size() < expected : args.size() != expected)
			return Missing{"call of " + target->name + " with " + std::to_string(args.size()) + " arguments, src " +
						   std::to_string(expected) + (type->variadic ? " and more" : "")};
		// a variadic callee's further arguments print as they are: C's default promotions push what the exe pushes
		// (a dword for a narrower integer, a double), except for a float the exe pushes as 4 bytes (8 C++ would) and
		// a record by value. A float in a slot Unfold types as the 8-byte double the exe stores there is C++'s
		// promotion where the exe's double is the float's exact value (promotedExactly)
		for (size_t index = expected; index < args.size(); ++index)
			if (const tinfo_t& t = args[index].type;
				(t.is_floating() && t.get_size() != 8 && !promotedExactly(call, index)) || !t.is_scalar())
				return Missing{"variadic argument " + std::to_string(index - first + 1) + " of " + target->name +
							   ": Hex-Rays pushes " + text(t)};
		std::string why;
		// a parameter src declares `void*` (the PDB's untyped object: a condition function's `apParam1`) passed as
		// the object of a member of class C is C's object as the exe uses it: `static_cast<C*>(p)->F()`; only for a
		// direct call of an address no other class's member shares (no fold: the class is the code's own). An
		// expression Hex-Rays types void* (a cast, a call's result) stays unresolved: its type is the fix
		bool untypedParameter = false;
		if (member)
		{
			std::optional<std::string> object = keyOf(&args[0], why);
			SrcTypes::Parts p = object ? m_types.parts(m_types.canonical(*object)) : SrcTypes::Parts{};
			std::string record = p.kind == "ptr" ? m_types.parts(p.of).name : "";
			int v = lvarIndex(&args[0]);
			untypedParameter = p.kind == "ptr" && p.of == "builtin:void" && v >= 0 &&
							   m_function.locals[v].parameter >= 0 &&
							   m_types.canonical(m_function.locals[v].type) == "ptr(builtin:void)" &&
							   call->x->op == cot_obj &&
							   std::ranges::none_of(m_context.model.refsAt(call->x->obj_ea),
													[](const SrcModel::Ref& r) { return r.kind == "fold"; });
			if (!untypedParameter && (record.empty() || !m_types.derivesAtZero(record, target->cls)))
				return Missing{"call of " + target->name + " on " + (object ? *object : why)};
		}
		Call resolved{target, qualified, isReference(type->ret), std::vector<bool>(args.size(), false),
					  std::vector<std::string>(args.size()), std::vector<std::string>(args.size())};
		resolved.edxParameter = edx;
		// a member that name lookup from the receiver's class does not find alone (another class declares the name
		// on the way, or two bases do): a direct call names its class, a call through the vtable casts the receiver
		// to it (a qualified call is never virtual)
		if (untypedParameter)
			resolved.castTo[0] = "ptr(record:" + target->cls + ")";
		else if (member && hidden(&args[0], *target))
		{
			if (call->x->op == cot_obj)
				resolved.qualified = true;
			else
				resolved.castTo[0] = "ptr(record:" + target->cls + ")";
		}
		// a free function of the global namespace that a member of the body's class (or a base's) of its name hides
		// from unqualified lookup (`ToggleFullHelp()` inside Script::ToggleFullHelp): `::F(...)`
		if (target->cls.empty() && target->name.find("::") == std::string::npos && m_function.self &&
			!m_function.self->cls.empty())
		{
			std::vector<std::string> found;
			memberLookup(m_context.model, m_function.self->cls, target->name, found);
			resolved.qualified = !found.empty();
		}
		if (byValue)
		{
			int index = member ? 1 : 0;
			if (std::string into = resultObject(args, index, type->ret); !into.empty())
				return Missing{"callee " + target->name + " returns a record by value into " + into};
			resolved.resultArgument = index;
		}
		for (size_t k = 0; k < type->params.size(); ++k)
		{
			if (static_cast<int>(k) == edx)
				continue;
			size_t index = first + k - (edx >= 0 && static_cast<int>(k) > edx ? 1 : 0);
			const carg_t& arg = args[index];
			const std::string& declared = type->params[k];
			std::optional<std::string> key = keyOf(&arg, why);
			if (!key)
				return Missing{"argument " + std::to_string(k + 1) + " of " + target->name + ": " + why};
			std::string mismatch = "argument " + std::to_string(k + 1) + " of " + target->name + ": src " + declared +
								   ", Hex-Rays " + *key;
			// C++ converts no pointer to int to a pointer to long, which keys alike on the exe's target (the argument
			// as printed: its src type)
			std::string printed = srcType(m_function, m_context, &arg);
			if (printed.empty())
				printed = *key;
			if (m_types.canonical(declared) == m_types.canonical(printed) &&
				longPointer(declared) != longPointer(printed))
				return Missing{mismatch};
			// a reference to a pointer binds only a pointer of the same pointee constness (`const char*&` no `char*`),
			// which keys alike once cv is dropped
			if (std::string referred = m_types.bare(m_types.parts(m_types.bare(declared)).of);
				isReference(declared) && m_types.parts(referred).kind == "ptr")
				if (SrcTypes::Parts object = m_types.parts(m_types.bare(printed)); object.kind == "ptr" &&
					m_types.constPointee(referred) != m_types.constPointee(m_types.bare(object.of)))
					return Missing{mismatch};
			if (!passes(declared, *key, arg, false))
			{
				++m_converted;
				std::optional<bool> marked = converts(declared, *key, arg, call, index);
				if (!marked)
					return Missing{mismatch};
				if (*marked)
					resolved.mismatch[index] = mismatch;
			}
			resolved.byReference[index] = isReference(declared);
		}
		// a record returned by value gives Hex-Rays' pointer to the result, or nothing; a result Hex-Rays types void
		// is discarded (a comma's left operand)
		if (resultUsed && !call->type.is_void())
		{
			std::string returned = byValue ? "ptr(" + type->ret + ")" : type->ret;
			std::optional<std::string> result = keyOf(call, why);
			if (!result)
				return Missing{"result of " + target->name + ": " + why};
			if (!passes(returned, *result, *call, true))
			{
				if (!resultConverts(returned, *result, call))
					return Missing{"result of " + target->name + ": src " + returned + ", Hex-Rays " + *result};
				++m_converted;
			}
		}
		return resolved;
	}

	// name lookup of the target's name from the class of the object the receiver points to (as printed: casts and
	// base subobjects peeled) finds anything but the target's class, once
	bool hidden(const cexpr_t* receiver, const SrcModel::Function& target)
	{
		const cexpr_t* object = receiver;
		std::string cls;
		while (cls.empty())
		{
			if (object->op == cot_cast)
				object = object->x;
			else if (object->op == cot_ref && (object->x->op == cot_memptr || object->x->op == cot_memref) &&
					 baseSubobject(object->x))
			{
				const cexpr_t* base = object->x;
				while ((base->x->op == cot_memptr || base->x->op == cot_memref) && baseSubobject(base->x))
					base = base->x;
				if (base->op == cot_memptr)
					object = base->x;
				else if ((cls = recordName(base->x->type)).empty())
					return false;
			}
			else if (object->type.is_ptr())
			{
				if ((cls = recordName(object->type.get_pointed_object())).empty())
					return false;
			}
			else
				return false;
		}
		std::vector<std::string> found;
		memberLookup(m_context.model, cls, lastComponent(target.name), found);
		return found.size() != 1 || found[0] != target.cls;
	}

	// the Fields pass resolved the member as a base subobject
	bool baseSubobject(const cexpr_t* e) const
	{
		auto found = m_function.meaning.find(e);
		return found != m_function.meaning.end() && std::holds_alternative<std::string>(found->second);
	}

	// the src record of IDA's struct, "" for none
	std::string recordName(const tinfo_t& type)
	{
		const SrcModel::Record* record = type.is_udt() ? m_types.recordOf(type) : nullptr;
		return record ? record->name : "";
	}

	bool isReference(const std::string& key) const { return m_types.parts(m_types.canonical(key)).kind == "lref"; }

	// what a pointer or reference key points to, cv kept
	std::string pointee(const std::string& key) const { return m_types.bare(m_types.parts(m_types.bare(key)).of); }

	// the key without its top-level const
	std::string unconst(const std::string& key) const
	{
		SrcTypes::Parts p = m_types.parts(key);
		return p.kind == "const" ? m_types.bare(p.of) : key;
	}

	// A value of Hex-Rays' type (key) is what src's declared type takes (an argument) or gives (a result). A pointer
	// to a class converts to a pointer to its base at offset 0: Hex-Rays' to src's for an argument, src's to
	// Hex-Rays' for a result. An argument's object pointer converts to a void* or const void* parameter. An argument
	// keeps the const of its src type (a field's, a local's), which Hex-Rays' type may lack; a const src's result has
	// and Hex-Rays' type lacks is the Const pass's. Where src takes a reference, Hex-Rays passes the object's address:
	// an object of a class deriving from the referent's at offset 0 binds to its base subobject (the same address),
	// and a pointer binds a reference to a const pointer it converts to by itself (to void*, to its base at offset 0:
	// C++ binds a temporary of the same value, which the callee reads as the exe's reads the object).
	bool passes(const std::string& declared, const std::string& key, const cexpr_t& value, bool result)
	{
		std::string a = m_types.canonical(declared), b = m_types.canonical(key);
		if (a == b)
			return true;
		SrcTypes::Parts p = m_types.parts(a), q = m_types.parts(b);
		if (p.kind == "lref" && q.kind == "ptr" && p.of == q.of)
			return true;
		if (p.kind == "lref" && q.kind == "ptr" && !result)
		{
			SrcTypes::Parts src = m_types.parts(p.of), ida = m_types.parts(q.of);
			std::string argument = srcType(m_function, m_context, &value);
			if (argument.empty())
				argument = key;
			if (src.kind == "record" && ida.kind == "record" && m_types.derivesAtZero(ida.name, src.name))
				return !m_types.constPointee(argument) || m_types.constPointee(declared);
			if (src.kind == "ptr" && ida.kind == "ptr" && m_types.constPointee(declared))
			{
				SrcTypes::Parts to = m_types.parts(src.of), from = m_types.parts(ida.of);
				bool constKept = !m_types.constPointee(unconst(pointee(argument))) ||
								 m_types.constPointee(unconst(pointee(declared)));
				return constKept && (src.of == "builtin:void" || (to.kind == "record" && from.kind == "record" &&
																	 m_types.derivesAtZero(from.name, to.name)));
			}
		}
		if (p.kind == "ptr" && q.kind == "ptr")
		{
			SrcTypes::Parts src = m_types.parts(p.of), ida = m_types.parts(q.of);
			if (result)
				return src.kind == "record" && ida.kind == "record" && m_types.derivesAtZero(src.name, ida.name);
			std::string argument = srcType(m_function, m_context, &value);
			bool constKept = !m_types.constPointee(argument) || m_types.constPointee(declared);
			if (constKept && (p.of == "builtin:void" || (src.kind == "record" && ida.kind == "record" &&
														 m_types.derivesAtZero(ida.name, src.name))))
				return true;
		}
		// an integer literal passes as any integer, and as an enum (the Enums pass prints its enumerator)
		return value.op == cot_num && q.kind == "builtin" && (p.kind == "builtin" || p.kind == "enum");
	}

	// the key is a pointer (or reference) to long, through any typedefs and cv
	bool longPointer(const std::string& key) const
	{
		std::string k = m_types.bare(key);
		bool pointer = false;
		for (;;)
		{
			SrcTypes::Parts p = m_types.parts(k);
			if (p.kind == "ptr" || p.kind == "lref")
				pointer = true;
			else if (p.kind != "const" && p.kind != "volatile")
				return pointer && (k == "builtin:long" || k == "builtin:unsigned long");
			k = m_types.bare(p.of);
		}
	}

	// what a scalar key is to a conversion: "pointer", "integer" (an enum too), "bool", "floating"; "" for another
	std::string scalar(const std::string& key) const
	{
		std::string k = m_types.canonical(key);
		SrcTypes::Parts p = m_types.parts(k);
		if (p.kind == "ptr")
			return "pointer";
		if (p.kind == "enum")
			return "integer";
		const SrcModel::Type* type = p.kind == "builtin" && k != "builtin:void" ? m_context.model.type(k) : nullptr;
		if (!type)
			return "";
		return type->isFloat ? "floating" : k == "builtin:bool" ? "bool" : "integer";
	}

	// An argument of Hex-Rays' type (key) that passes() refuses, which still goes where src's parameter takes it as
	// the bits the exe pushes: the Conversions pass gives it its explicit conversion where C++ makes none (marked;
	// from a void* the static_cast src writes). Two pointers; a pointer and a 4-byte integer or enum (a literal
	// for a pointer: 0 is C++'s null pointer, another number converts marked); two floating types where the exe
	// pushes the parameter's (the formal's size is src's: C++ converts the value as the exe's x87 store does);
	// integers of two sizes C++ converts as the exe passes them: a wider one for a narrower parameter (C++ keeps
	// its low bytes, all the parameter holds of the exe's slot), a narrower one the exe extends in its 4-byte slot as
	// C++ converts it (extendedAsConverted), an unsigned 4-byte one for an 8-byte parameter whose high dword the exe
	// pushes as 0 (zeroHighDword), neither of a type whose width differs on x64 (hostWidth); a bool for a 4-byte
	// integer parameter that the exe zero-extends before the push (extendedAsConverted). true for an
	// integer or enum of the parameter's size and another type, which C++ converts by itself: the user keeps it
	// marked (Call::mismatch). nullopt for any other (a reference, a record, a bool pushed as a dword whose upper bytes
	// the exe did not clear, other integers of two sizes, an integer for a floating type: the bits would not be the
	// exe's).
	std::optional<bool> converts(const std::string& declared, const std::string& key, const carg_t& arg,
								 const cexpr_t* call, size_t index) const
	{
		std::string to = scalar(declared), from = scalar(key);
		std::optional<int64_t> declaredSize = m_context.model.sizeOf(declared);
		int64_t exeSize = static_cast<int64_t>(arg.type.get_size());
		if (to == "pointer" && from == "pointer")
			return false;
		if (to == "pointer" && from == "integer" && (exeSize == 4 || arg.op == cot_num))
			return false;
		if (to == "integer" && from == "pointer" && declaredSize == 4)
			return false;
		if (to == "integer" && from == "integer" && arg.op != cot_num && declaredSize &&
			*declaredSize == exeSize)
			return true;
		if (to == "integer" && from == "integer" && arg.op != cot_num && declaredSize && exeSize > 0 &&
			!hostWidth(m_context, declared) && !hostWidth(m_context, key))
		{
			if (*declaredSize < exeSize)
				return false;
			if (*declaredSize == 4 && !arg.type.is_enum() && extendedAsConverted(call, index, arg.type))
				return false;
		}
		// a bool for a 4-byte integer parameter (no enum, none whose width differs on x64), which the exe extends to
		// the whole dword before the push (AsmFacts' pushesExtendedBool: `movzx eax, al`, `xor eax, eax; ...; setz
		// al`): C++'s promotion gives the dword the exe pushes. Not where the exe pushes a dword whose upper bytes it
		// did not clear (declarations.md "Parameter widths")
		if (to == "integer" && from == "bool" && arg.op != cot_num && declaredSize == 4 &&
			m_types.parts(m_types.canonical(declared)).kind == "builtin" && !hostWidth(m_context, declared) &&
			extendedBool(call, index))
			return false;
		// a 4-byte unsigned integer for an 8-byte integer parameter whose high dword the exe pushes as 0: C++'s
		// conversion zero-extends it, as the exe passes it
		if (to == "integer" && from == "integer" && arg.op != cot_num && declaredSize == 8 && exeSize == 4 &&
			!arg.type.is_enum() && !arg.type.is_signed() && !hostWidth(m_context, declared) &&
			!hostWidth(m_context, key) && zeroHighDword(call, index))
			return false;
		if (to == "floating" && from == "floating" && !arg.formal_type.empty() && declaredSize &&
			static_cast<int64_t>(arg.formal_type.get_size()) == *declaredSize)
			return false;
		return std::nullopt;
	}

	// A float variadic argument in a stack slot of the 8-byte double the exe stores there (the call's type there:
	// Unfold's site type, from its `fstp qword`), whose value is the float's exact value (exactFloat): the exe loads
	// it and stores it as a double, which is C++'s promotion. Not a computed value, which the exe stores at the
	// x87's precision.
	bool promotedExactly(const cexpr_t* call, size_t index) const
	{
		const carg_t& arg = (*call->a)[index];
		tinfo_t called = call->x->type.is_ptr() ? call->x->type.get_pointed_object() : call->x->type;
		func_type_data_t fd;
		tinfo_t slot = called.get_func_details(&fd) && index < fd.size() ? fd[index].type : arg.formal_type;
		return arg.type.get_size() == 4 && slot.is_floating() && slot.get_size() == 8 && exactFloat(&arg);
	}

	// A float value the x87 holds exactly as the float: a float object read (a local, member, element or global, a
	// trivial accessor's x87 load of a 4-byte field), a float constant, or such a value negated or made absolute
	// (`fchs`, `fabs`, exact)
	bool exactFloat(const cexpr_t* e) const
	{
		if (!e->type.is_floating() || e->type.get_size() != 4)
			return false;
		switch (e->op)
		{
		case cot_fneg:
			return exactFloat(e->x);
		case cot_var:
		case cot_obj:
		case cot_memptr:
		case cot_memref:
		case cot_idx:
		case cot_ptr:
		case cot_fnum:
			return true;
		case cot_call:
		{
			if (e->x->op == cot_helper && std::string_view(e->x->helper) == "fabs" && e->a->size() == 1)
				return exactFloat(&(*e->a)[0]);
			std::optional<AccessorCall> access = accessorCall(m_context, e);
			return access && access->load && access->x87 && access->width == 4;
		}
		default:
			return false;
		}
	}

	// The exe's call passes in its first argument's place (ecx, or the stack slot Hex-Rays' type puts it in) the very
	// pointer whose vtable it reads the slot through (callsOnVtableObject), which Hex-Rays may print as two
	// expressions of one value: the local it copied the pointer to, and the field it loaded it from.
	static bool passesVtableObject(const cexpr_t* call)
	{
		tinfo_t called = call->x->type.is_ptr() ? call->x->type.get_pointed_object() : call->x->type;
		func_type_data_t fd;
		if (call->a->empty() || !called.get_func_details(&fd) || fd.empty())
			return false;
		const argloc_t& place = fd[0].argloc;
		if (place.is_reg1() && place.reg1() == R_cx)
			return callsOnVtableObject(call->ea, std::nullopt);
		return place.is_stkoff() && callsOnVtableObject(call->ea, place.stkoff());
	}

	// The exe passes the call's argument (index) of Hex-Rays' integer type, narrower than 4 bytes, in its 4-byte stack
	// slot extended as C++ converts it: the call's block pushes that slot from a register its last write before set
	// by movzx (an unsigned type) or movsx (a signed one) from the type's size, with no join or call between them and
	// the call (pushedArgument).
	static bool extendedAsConverted(const cexpr_t* call, size_t index, const tinfo_t& type)
	{
		std::optional<int64_t> slot = stackSlot(call, index);
		std::optional<PushedArgument> pushed = slot ? pushedArgument(call->ea, *slot) : std::nullopt;
		if (!pushed || pushed->afterCall)
			return false;
		const insn_t& insn = pushed->source;
		bool zero = insn.itype == NN_movzx, sign = insn.itype == NN_movsx;
		return (zero || sign) && sign == type.is_signed() && insn.ops[0].type == o_reg &&
			   get_dtype_size(insn.ops[1].dtype) == type.get_size();
	}

	// The exe passes the call's bool argument (index) extended to its whole 4-byte stack slot (pushesExtendedBool)
	static bool extendedBool(const cexpr_t* call, size_t index)
	{
		std::optional<int64_t> slot = stackSlot(call, index);
		return slot && pushesExtendedBool(call->ea, *slot);
	}

	// the stack offset of the call's 4-byte-aligned stack argument (index), as the call's type places it
	static std::optional<int64_t> stackSlot(const cexpr_t* call, size_t index)
	{
		tinfo_t called = call->x->type.is_ptr() ? call->x->type.get_pointed_object() : call->x->type;
		func_type_data_t fd;
		if (!called.get_func_details(&fd) || index >= fd.size() || !fd[index].argloc.is_stkoff() ||
			fd[index].argloc.stkoff() % 4 != 0)
			return std::nullopt;
		return fd[index].argloc.stkoff();
	}

	// The exe passes the call's 8-byte stack argument (index) with its high dword a pushed 0, in the call's block
	// (pushedArgument): the value Hex-Rays reads as 4 bytes zero-extended.
	static bool zeroHighDword(const cexpr_t* call, size_t index)
	{
		tinfo_t called = call->x->type.is_ptr() ? call->x->type.get_pointed_object() : call->x->type;
		func_type_data_t fd;
		if (!called.get_func_details(&fd) || index >= fd.size() || !fd[index].argloc.is_stkoff() ||
			fd[index].argloc.stkoff() % 4 != 0 || fd[index].type.get_size() != 8)
			return false;
		std::optional<PushedArgument> pushed = pushedArgument(call->ea, fd[index].argloc.stkoff() + 4);
		return pushed && pushed->source.itype == NN_push && pushed->source.ops[0].type == o_imm &&
			   pushed->source.ops[0].value == 0;
	}

	// A result of src's type (returned) that passes() refuses as Hex-Rays' (key), used where the Conversions pass
	// converts it to what the use takes: two pointers, a pointer and a 4-byte integer or enum, or two integers or
	// enums of one size (marked there, user), assigned, passed, returned, compared, cast, read through (the access
	// converts the pointer to Hex-Rays' type) or tested; two floating types anywhere (both returned in ST0). Not in
	// arithmetic: a pointer would step by src's pointee, an integer would take src's signedness.
	bool resultConverts(const std::string& returned, const std::string& key, const cexpr_t* call) const
	{
		std::string to = scalar(key), from = scalar(returned);
		if (to == "floating" && from == "floating")
			return true;
		std::optional<int64_t> size = m_context.model.sizeOf(returned);
		bool integers = to == "integer" && from == "integer" && size &&
						*size == static_cast<int64_t>(call->type.get_size()) && !hostWidth(m_context, returned) &&
						!hostWidth(m_context, key);
		bool pointers = (to == "pointer" && from == "pointer") || (to == "pointer" && from == "integer" && size == 4) ||
						(to == "integer" && from == "pointer" && call->type.get_size() == 4);
		auto parent = m_parents.find(call);
		if (!(pointers || integers) || parent == m_parents.end())
			return false;
		const citem_t* p = parent->second;
		if (!p->is_expr())
			return p->op == cit_return || p->op == cit_if || p->op == cit_while || p->op == cit_do;
		const cexpr_t* use = static_cast<const cexpr_t*>(p);
		switch (use->op)
		{
		case cot_asg:
			return use->y == call;
		case cot_call:
		case cot_eq:
		case cot_ne:
		case cot_lnot:
		case cot_land:
		case cot_lor:
			return true;
		case cot_cast:
			return use->x == call;
		case cot_memptr:
		case cot_ptr:
		case cot_idx:
		case cot_tern:
			return pointers && use->x == call;
		default:
			// an integer's order compares by its signedness, which src's type would change
			return pointers && use->op >= cot_sge && use->op <= cot_ult;
		}
	}

	const Function& m_function;
	Context& m_context;
	SrcTypes& m_types;
	std::string m_tu;
	std::unordered_map<const cexpr_t*, const citem_t*> m_parents;
	int m_converted = 0;  // the arguments and results checked() took converted (passes() refused them)
};

}  // namespace

void resolveCalls(Function& function, Context& context)
{
	// the calls whose result nothing reads: a statement of their own
	std::unordered_set<const cexpr_t*> statements;
	forEachStatement(&function.cfunc->body, [&](const cinsn_t* s) {
		if (s->op == cit_expr)
			statements.insert(s->cexpr);
	});
	Calls calls(function, context);
	std::vector<const cexpr_t*> virtualCallees;
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
		if (e->op != cot_call)
			return;
		// a Hex-Rays math helper the exe's site makes as an msvc9 call is that call, so is a direct call of a __CI
		// entry; whether any other helper is taken is checkRules' verdict
		if (std::string math = mathFunction(e); !math.empty())
		{
			function.meaning[e] = Intrinsic{math, "SDK/MSVC/math.h"};
			return;
		}
		// a Hex-Rays memset, qmemcpy or strlen that is the CRT function's effect is src's msvc9 call of it, checked as
		// any call, under EXE-INLINED (inlinedCrt; user, 2026-11-12)
		if (e->x->op == cot_helper)
		{
			if (std::string crt = inlinedCrt(e, statements.count(e) != 0); !crt.empty())
				for (const SrcModel::Function* f : context.model.functionsNamed("msvc9::" + crt))
					if (Meaning checked = calls.check(e, *f, statements.count(e) == 0);
						std::holds_alternative<Call>(checked))
					{
						std::get<Call>(checked).exeInlined = "none: " + crt;
						function.meaning[e] = checked;
						if (crt != "strlen")
							markByteCount(function, context, e, crt);
						break;
					}
			return;
		}
		// a folded call the FoldedAccessors pass printed as its access
		Meaning& meaning = function.meaning[e];
		if (std::holds_alternative<Access>(meaning))
			return;
		meaning = calls.resolve(e, statements.count(e) == 0);
		const Meaning& resolved = function.meaning[e];
		if ((std::holds_alternative<Call>(resolved) || std::holds_alternative<Rewrite>(resolved)) &&
			e->x->op != cot_obj)
			virtualCallees.push_back(e->x);
	});
	// a resolved call through the vtable prints as `obj->F(...)` (a deleting destructor's as `delete obj`): the
	// vtable read is not printed
	for (const cexpr_t* callee : virtualCallees)
		forEachSubexpression(callee, [&](const cexpr_t* e) { function.meaning.erase(e); });
	// a vtable slot the exe loaded into a local before its calls is read by each call C++ makes: the load is left
	// out, with what the layers found in it
	std::vector<const cinsn_t*> loads = calls.resolvedLoads();
	function.noOps.insert(loads.begin(), loads.end());
	eraseReplaced(function, loads, {});
}

Meaning checkCall(const Function& function, Context& context, const cexpr_t* call, const SrcModel::Function& target,
				  bool resultUsed)
{
	return Calls(function, context).check(call, target, resultUsed);
}

// Of a user-defined call's two src functions, a member's const and non-const overloads of one parameter list (IDA's
// prototype holds no constness of `this`: `GetAt` and `GetAt const` read alike), the one C++ calls on the receiver: the
// const one where the receiver, Hex-Rays' casts peeled, points to const, else the other. nullptr for any other set.
static const SrcModel::Function* constOverload(const SrcModel& model, const cexpr_t* call,
											   const std::vector<const SrcModel::Function*>& named)
{
	if (named.size() != 2 || call->a->empty() || named[0]->name != named[1]->name || named[0]->isStatic ||
		named[1]->isStatic || named[0]->cls.empty())
		return nullptr;
	const SrcModel::Type* a = model.type(named[0]->type);
	const SrcModel::Type* b = model.type(named[1]->type);
	if (!a || !b || a->isConst == b->isConst || a->params != b->params || a->variadic != b->variadic)
		return nullptr;
	const cexpr_t* receiver = &(*call->a)[0];
	while (receiver->op == cot_cast)
		receiver = receiver->x;
	bool constReceiver = receiver->type.is_ptr() && receiver->type.get_pointed_object().is_const();
	return a->isConst == constReceiver ? named[0] : named[1];
}

ChosenCallee chosenCallee(const Function& function, const SrcModel& model, const cexpr_t* call)
{
	ea_t ea = call->x->obj_ea;
	if (auto named = function.namedCallees.find(call); named != function.namedCallees.end())
	{
		// the src function a fold's user-defined call names is the address's own, unless src registers its body at
		// another address: src's call of it would then be that function's (src declares one function for two)
		auto own = [&](const SrcModel::Function* f) -> ChosenCallee {
			const std::vector<uint64_t>& at = model.bodyAddresses(f->mangled);
			if (!at.empty() && std::find(at.begin(), at.end(), static_cast<uint64_t>(ea)) == at.end())
				return {nullptr, f->name + " names " + hex(ea) + " and src registers its body at " + hex(at.front()),
						false};
			return {f, "", false};
		};
		if (const SrcModel::Function* f = constOverload(model, call, named->second))
			return own(f);
		if (named->second.size() == 1)
			return own(named->second.front());
		// of several functions of one name and type (a second exe copy's forwarder, a TU-local copy of a free
		// function), the one src registers its body at the call's target is the call's: src declares the others as
		// calls of other code
		const SrcModel::Function* registered = nullptr;
		size_t count = 0;
		for (const SrcModel::Function* f : named->second)
		{
			const std::vector<uint64_t>& at = model.bodyAddresses(f->mangled);
			if (std::find(at.begin(), at.end(), static_cast<uint64_t>(ea)) != at.end())
				registered = f, ++count;
		}
		if (count == 1)
			return {registered, "", false};
		return {nullptr,
				"user-defined call " + named->second.front()->name + " names " +
					std::to_string(named->second.size()) + " src functions of its type",
				true};
	}
	qstring name;
	if (get_func_name(&name, ea) <= 0)
		get_name(&name, ea);
	RefChoice choice = chooseRef(model, ea);
	if (choice.names.size() > 1 && !choice.ref)
		return {nullptr, "callee " + std::string(name.c_str()) + " is a fold without a registered body", true};
	const SrcModel::Ref* ref =
		choice.ref && (choice.ref->kind == "body" || choice.ref->kind == "forwarder") ? choice.ref : nullptr;
	// a KEEP body src leaves unregistered: the src function an FNVD_ADDRESS assertion names (its src callers call it)
	for (const SrcModel::Ref& r : model.refsAt(ea))
		if (!ref && r.kind == "address" && !r.function.empty() && !placeholder(r))
			ref = &r;
	if (!ref)
		return {nullptr, "no src declaration of " + std::string(name.c_str()) + " (" + hex(ea) + ")", false};
	const SrcModel::Function* f = model.function(ref->function);
	if (!f)
		return {nullptr, "src declaration of " + ref->target + " not in the model", false};
	return {f, "", false};
}

std::vector<const cexpr_t*> variadicCasts(const Function& function, Context& context)
{
	Calls calls(function, context);
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
		auto found = e->op == cot_call ? function.meaning.find(e) : function.meaning.end();
		if (found != function.meaning.end())
			if (const Call* c = std::get_if<Call>(&found->second))
				calls.peelVariadic(e, *c);
	});
	return calls.peeled;
}

}  // namespace idakit
