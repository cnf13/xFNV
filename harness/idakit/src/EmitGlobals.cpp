// Globals: each obj that is not a function (a call's callee too: an import's pointer) as the src global or static
// member at its address, declared in a header and of the type Hex-Rays reads, or an array Hex-Rays reads as the
// pointer to its first element (its name decays to it); one Hex-Rays reads as another type is still that global, read
// at its address as Hex-Rays' type and marked. A file-local global of a .cpp (an emitted `static FNVD_GLOBAL`) is no
// declaration another file sees, and is passed over. A C string the database holds in the exe's constant data, used
// other than by its address in a comparison, is the C++ literal of its bytes (docs/VERIFY.md: the checker counts a
// pointer to an equal string in either binary's constant data as equal), also where src declares it as its first char
// (`FNVD_GLOBAL(const char, aX, ...)`); a wide one (wchar_t is 4 bytes on Linux x64) or one in writable data stays
// unresolved.

#include "EmitPasses.hpp"

#include "IdaSupport.hpp"

#include "idakit/SrcIda.hpp"

#include <cctype>
#include <cstdio>
#include <unordered_set>

namespace idakit
{

std::string cLiteral(const std::string& bytes)
{
	std::string out = "\"";
	for (size_t k = 0; k < bytes.size(); ++k)
	{
		unsigned char c = static_cast<unsigned char>(bytes[k]);
		bool dollar = c == '$' && k + 1 < bytes.size() && std::isdigit(static_cast<unsigned char>(bytes[k + 1]));
		if (c == '"' || c == '\\')
			out += std::string("\\") + static_cast<char>(c);
		else if (c == '\n')
			out += "\\n";
		else if (c == '\r')
			out += "\\r";
		else if (c == '\t')
			out += "\\t";
		else if (c < 0x20 || c >= 0x7F || dollar)
		{
			char octal[5];
			std::snprintf(octal, sizeof octal, "\\%03o", c);
			out += octal;
		}
		else
			out += static_cast<char>(c);
	}
	return out + "\"";
}

bool functionReferencesAsPointers(const std::string& src, const std::string& ida)
{
	if (src == ida || !(src.starts_with("ptr(fn(") || src.starts_with("array(ptr(fn(")))
		return false;
	std::string key = src;
	for (const char* reference : {"lref(", "rref("})
		for (size_t at = key.find(reference); at != std::string::npos; at = key.find(reference, at))
			key.replace(at, 4, "ptr");
	return key == ida;
}

namespace
{

// the string literal at a string the database holds: a C string (1-byte characters up to a NUL inside the item) in
// read-only data that is no code, else what keeps it unresolved
std::variant<std::string, Missing> stringLiteral(ea_t ea, const std::string& where)
{
	int32 type = get_str_type(ea);
	if (get_str_type_code(type) != STRTYPE_C)
		return Missing{"string literal at " + where + " of width " + std::to_string(get_strtype_bpu(type)) +
					   " (wchar_t is 4 bytes on Linux x64)"};
	segment_t* segment = getseg(ea);
	if (!segment || (segment->perm & SEGPERM_WRITE) || segment->type == SEG_CODE)
		return Missing{"string literal at " + where + " in writable data or code"};
	asize_t size = get_item_size(ea);
	std::string bytes;
	for (asize_t k = 0; k < size; ++k)
	{
		uchar c = get_byte(ea + k);
		if (c == 0)
			return cLiteral(bytes);
		bytes += static_cast<char>(c);
	}
	return Missing{"string literal at " + where + " without its NUL"};
}

// an obj compared by its address (past Hex-Rays' casts): the exe's own address of the string, which a src literal
// does not have
bool comparedByAddress(const cexpr_t* e, const std::unordered_map<const cexpr_t*, const citem_t*>& parents)
{
	const citem_t* parent = parents.count(e) ? parents.at(e) : nullptr;
	while (parent && parent->op == cot_cast)
	{
		const cexpr_t* cast = static_cast<const cexpr_t*>(parent);
		parent = parents.count(cast) ? parents.at(cast) : nullptr;
	}
	return parent && (parent->op == cot_eq || parent->op == cot_ne || (parent->op >= cot_sge && parent->op <= cot_ult));
}

// an obj the body stores into, itself or an element of it (`table[3] = F`)
bool storedInto(const cexpr_t* e, const std::unordered_map<const cexpr_t*, const citem_t*>& parents)
{
	const cexpr_t* object = e;
	for (auto parent = parents.find(object); parent != parents.end() && parent->second->is_expr();
		 parent = parents.find(object))
	{
		const cexpr_t* p = static_cast<const cexpr_t*>(parent->second);
		if ((p->op == cot_idx || p->op == cot_cast) && p->x == object)
			object = p;
		else
			return (p->op >= cot_asg && p->op <= cot_asgumod) && p->x == object;
	}
	return false;
}

// A function's address that goes where src declares a pointer to a function: a resolved call's parameter, a src field
// or global assigned or compared with, the return type (not a local, whose type is Hex-Rays'). type: that
// destination's src type, the evidence of what the address is passed as; casts: Hex-Rays' casts on the way (its own
// reading of the function as the callback, `(void (__thiscall *)(void *))F`), which the destination's type replaces
struct Callback
{
	std::string type;
	std::vector<const cexpr_t*> casts;
};

std::optional<Callback> callbackDestination(const Function& function, Context& context, const cexpr_t* e,
											const std::unordered_map<const cexpr_t*, const citem_t*>& parents)
{
	auto parentOf = [&](const cexpr_t* item) -> const cexpr_t* {
		auto up = parents.find(item);
		return up != parents.end() && up->second && up->second->is_expr() ? static_cast<const cexpr_t*>(up->second)
																			: nullptr;
	};
	Callback callback;
	const cexpr_t* value = e;
	if (const cexpr_t* ref = parentOf(e); ref && ref->op == cot_ref)
		value = ref;
	for (const cexpr_t* cast = parentOf(value); cast && cast->op == cot_cast; cast = parentOf(value))
	{
		callback.casts.push_back(cast);
		value = cast;
	}
	if (const cexpr_t* asg = parentOf(value); asg && asg->op == cot_asg && asg->y == value && lvarIndex(asg->x) >= 0)
		return std::nullopt;
	Destination to = destinationOf(function, context, parents, value);
	// the other side of `==` / `!=`: a src field or global
	if (const cexpr_t* compare = parentOf(value); compare && (compare->op == cot_eq || compare->op == cot_ne))
		if (auto other = function.meaning.find(compare->x == value ? compare->y : compare->x);
			other != function.meaning.end())
		{
			if (const Member* member = std::get_if<Member>(&other->second))
				to = {member->field->type};
			else if (const SrcModel::Ref* const* ref = std::get_if<const SrcModel::Ref*>(&other->second))
				to = {(*ref)->type};
		}
	if (to.type.empty() || to.byReference)
		return std::nullopt;
	SrcTypes::Parts pointer = context.types.parts(context.types.canonical(to.type));
	if (pointer.kind != "ptr" || context.types.parts(context.types.canonical(pointer.of)).kind != "fn")
		return std::nullopt;
	callback.type = to.type;
	return callback;
}

// the address of a function, not called: the exe's own address (porting.md "Bodies": an exe object holds the exe's
// address, not the DLL's), the src FNVD_ORIGINAL_FUNCTION a header declares at it: of the type of where it goes where
// that is src's pointer to a function (callback; `for TYPE` in the reason without one, which `declare --globals` reads
// to declare one), else of its declared pointer type (one of Hex-Rays' type where several are); else what keeps it
// unresolved (`declare --globals` declares one where src has none)
Meaning functionAddress(Context& context, const cexpr_t* e, const std::string& where,
						const std::optional<Callback>& callback)
{
	std::string why;
	std::optional<std::string> read = context.types.keyOf(e->type, why);
	std::string as = callback ? context.types.canonical(callback->type) : read ? context.types.canonical(*read) : "";
	const SrcModel::Ref* chosen = nullptr;
	const SrcModel::Ref* inSource = nullptr;
	for (const SrcModel::Ref& ref : context.model.refsAt(e->obj_ea))
	{
		if (ref.kind != "original_function" || placeholder(ref))
			continue;
		if (!isHeader(ref.file))
		{
			if (!ref.local && !inSource)
				inSource = &ref;
			continue;
		}
		bool fits = !as.empty() && context.types.canonical(ref.type) == as;
		if (callback ? fits : !chosen || fits)
			chosen = &ref;
	}
	if (!chosen && callback)
		return Missing{"address of function " + where + " for " + callback->type};
	if (!chosen)
		return Missing{"address of function " + where +
					   (inSource ? ": src original function " + inSource->target + " is declared in " + inSource->file
								 : "")};
	Rewrite rewrite;
	rewrite.form = chosen->target;
	rewrite.type = chosen->type;
	rewrite.header = chosen->file.rfind("src/", 0) == 0 ? chosen->file.substr(4) : chosen->file;
	return rewrite;
}

// An exe vtable's address read as a value, `&X::vftable` neither stored nor stored into (a store is an object's vptr:
// VtableStores' setVtable, or a stack object's that the constructed locals step types): Havok's getVtable functions
// return the table of an object they build and drop, as hkLiquidAction's 0xD2C8A0, hand-ported so. It is the src
// FNVD_VFTABLE constant's address as the type Hex-Rays reads, `reinterpret_cast<T>(X_vftable.address)`, the constant
// declared in a header or in the class's own .cpp (source); nullopt where it is stored or no address of the table
std::optional<Meaning> vtableValue(Context& context, const cexpr_t* e,
								   const std::unordered_map<const cexpr_t*, const citem_t*>& parents,
								   const SrcModel::Ref& table, const std::string& source)
{
	auto parentOf = [&](const cexpr_t* item) -> const citem_t* {
		auto up = parents.find(item);
		return up != parents.end() ? up->second : nullptr;
	};
	const citem_t* address = parentOf(e);
	if (!address || address->op != cot_ref || storedInto(e, parents))
		return std::nullopt;
	const cexpr_t* value = static_cast<const cexpr_t*>(address);
	const citem_t* parent = parentOf(value);
	for (const cexpr_t* top = value; parent && parent->op == cot_cast; parent = parentOf(top))
		top = static_cast<const cexpr_t*>(parent);
	if (parent && parent->op >= cot_asg && parent->op <= cot_asgumod)
		return std::nullopt;
	if (!isHeader(table.file) && table.file != source)
		return Missing{"vtable constant " + table.target + " is declared in " + table.file};
	std::string why;
	std::optional<std::string> read = context.types.keyOf(value->type, why);
	std::optional<std::string> spelled = read ? context.types.spell(*read) : std::nullopt;
	if (!spelled || context.types.parts(*read).kind != "ptr")
		return std::nullopt;
	Rewrite rewrite;
	rewrite.form = "reinterpret_cast<" + *spelled + ">(" + table.target + ".address)";
	rewrite.type = *read;
	rewrite.header = table.file.rfind("src/", 0) == 0 ? table.file.substr(4) : table.file;
	return rewrite;
}

Meaning global(Context& context, const cexpr_t* e, const std::unordered_map<const cexpr_t*, const citem_t*>& parents,
			   const std::optional<Callback>& callback)
{
	ea_t ea = e->obj_ea;
	qstring name;
	get_name(&name, ea);
	std::string where = hex(ea) + (name.empty() ? "" : " (" + std::string(name.c_str()) + ")");
	std::optional<Meaning> other;
	const SrcModel::Ref* inSource = nullptr;
	bool string = false;
	// a C string read by its address, not compared by it: its literal (below), whatever src declares there
	bool literal = is_strlit(get_flags(ea)) && !comparedByAddress(e, parents);
	for (const SrcModel::Ref& ref : context.model.refsAt(ea))
	{
		if (placeholder(ref))
			continue;
		if (ref.kind == "vftable")
			return Missing{"vtable address " + where};
		if (ref.kind != "global" && ref.kind != "static_member")
			continue;
		if (!isHeader(ref.file))
		{
			if (!ref.local && !inSource)
				inSource = &ref;
			continue;
		}
		std::string why;
		std::optional<std::string> read = context.types.keyOf(e->type, why);
		std::string declared = context.types.canonical(ref.type), as = read ? context.types.canonical(*read) : "";
		if (read && declared == as)
			return &ref;
		// Hex-Rays reads a pointer where a function type of src's takes or returns a reference: a value stored into
		// it has src's type or is converted (convertValues marks a function of Hex-Rays' type)
		if (read && functionReferencesAsPointers(declared, as))
			return &ref;
		// an array read as the pointer to its first element: its name, which decays to that pointer
		SrcTypes::Parts array = context.types.parts(declared);
		SrcTypes::Parts pointer = read ? context.types.parts(as) : SrcTypes::Parts{};
		if (read && array.kind == "array" && pointer.kind == "ptr" &&
			context.types.canonical(array.of) == context.types.canonical(pointer.of))
			return &ref;
		// a C string src declares as its first char, read by its address (Hex-Rays types a string as the pointer to
		// its first char): its literal, below, unless compared by its address, where it is that char's address
		if (read && declared == "builtin:char" && as == "ptr(builtin:char)" && is_strlit(get_flags(ea)))
		{
			if (literal)
			{
				string = true;
				continue;
			}
			Rewrite address;
			address.form = "&" + ref.target;
			address.type = "ptr(" + ref.type + ")";
			address.header = ref.file.rfind("src/", 0) == 0 ? ref.file.substr(4) : ref.file;
			return address;
		}
		if (!other)
			other = GlobalAs{&ref, read ? "src global " + ref.target + " is " + ref.type + ", Hex-Rays reads " + *read
										: "global " + ref.target + " read as " + why};
	}
	if (other && !string)
		return *other;
	if (inSource && !string && !literal)
		return Missing{"src global " + inSource->target + " is declared in " + inSource->file};
	if (func_t* f = get_func(ea); f && f->start_ea == ea)
		return functionAddress(context, e, where, callback);
	if (is_strlit(get_flags(ea)))
	{
		if (!literal)
			return Missing{"string literal at " + where + " compared by its address"};
		std::variant<std::string, Missing> text = stringLiteral(ea, where);
		if (const Missing* missing = std::get_if<Missing>(&text))
			return *missing;
		Rewrite rewrite;
		rewrite.form = std::get<std::string>(text);
		rewrite.type = "ptr(const(builtin:char))";
		return rewrite;
	}
	return Missing{"no src global at " + where};
}

// The element type of a global read by its address: an array, a C string (Hex-Rays types it as the pointer to its
// first char), or an array IDA's type at the address declares (`T[]` included) that Hex-Rays reads as the pointer to
// its first element, where it decays; nullopt for one read as its value
std::optional<tinfo_t> addressedElement(const cexpr_t* e)
{
	if (e->type.is_array())
		return e->type.get_array_element();
	if (tinfo_t item; e->type.is_ptr() && get_tinfo(&item, e->obj_ea) && item.is_array() &&
					  item.get_array_element().equals_to(e->type.get_pointed_object()))
		return e->type.get_pointed_object();
	if (is_strlit(get_flags(e->obj_ea)))
		return tinfo_t(BTF_CHAR);
	return std::nullopt;
}

}  // namespace

void resolveGlobals(Function& function, Context& context)
{
	std::unordered_map<const cexpr_t*, const citem_t*> parents = parentsOf(function);
	std::unordered_set<const cexpr_t*> callees;
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
		if (e->op == cot_call)
			callees.insert(e->x);
	});
	std::string source =
		function.self && !function.self->cls.empty() ? context.model.classSource(function.self->cls) : "";
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
		if (e->op != cot_obj)
			return;
		// a call's callee in code (a function) is the Calls pass's; one in data (an import's pointer) is a global
		// where src declares one (else the Calls pass's reason is the call's)
		bool callee = callees.count(e) != 0;
		if (callee && get_func(e->obj_ea))
			return;
		// an exe vtable's address as a value: the src constant's, as a whole `&X::vftable`
		for (const SrcModel::Ref& ref : context.model.refsAt(e->obj_ea))
			if (ref.kind == "vftable" && !placeholder(ref))
				if (std::optional<Meaning> value = vtableValue(context, e, parents, ref, source))
				{
					if (std::holds_alternative<Rewrite>(*value))
						function.meaning[parents.at(e)] = *value;
					function.meaning[e] = std::move(*value);
					return;
				}
		std::optional<Callback> callback =
			functionStart(e) ? callbackDestination(function, context, e, parents) : std::nullopt;
		Meaning meaning = global(context, e, parents, callback);
		if (!std::holds_alternative<const SrcModel::Ref*>(meaning))
			if (std::optional<tinfo_t> element = addressedElement(e))
				function.addressedGlobals[e] = *element;
		// Hex-Rays' `&F` of a function is that address as a whole
		if (const Rewrite* address = std::get_if<Rewrite>(&meaning); address && functionStart(e))
			if (auto parent = parents.find(e); parent != parents.end() && parent->second->op == cot_ref)
				function.meaning[parent->second] = *address;
		// Hex-Rays' casts of it to the callback: the src constant is of the destination's type already
		if (callback && std::holds_alternative<Rewrite>(meaning))
			for (const cexpr_t* cast : callback->casts)
				function.meaning[cast] = Implicit{};
		if (!callee || !std::holds_alternative<Missing>(meaning))
			function.meaning[e] = std::move(meaning);
	});
}

}  // namespace idakit
