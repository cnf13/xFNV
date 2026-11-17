// The converter: src's type keys as IDA types, and a src function's prototype from its declaration. Every phase
// that writes a src type into the database reads it from here.

#include "idakit/SrcPush.hpp"

#include "IdaSupport.hpp"

#include <Facts.hpp>

namespace idakit
{

namespace
{

// src's builtin types as IDA's C types (long is 4 bytes on the exe's target; src's 2-byte wide character is char16_t,
// the exe's and IDA's wchar_t, by the user's decision)
const std::map<std::string, std::string> kPrimitive = {
	{"void", "void"},
	{"bool", "bool"},
	{"char", "char"},
	{"signed char", "signed char"},
	{"unsigned char", "unsigned __int8"},
	{"short", "__int16"},
	{"unsigned short", "unsigned __int16"},
	{"wchar_t", "wchar_t"},
	{"int", "int"},
	{"unsigned int", "unsigned int"},
	{"long", "int"},
	{"unsigned long", "unsigned int"},
	{"long long", "__int64"},
	{"unsigned long long", "unsigned __int64"},
	{"float", "float"},
	{"double", "double"},
	{"long double", "double"},
	{"char8_t", "unsigned __int8"},
	{"char16_t", "wchar_t"},
	{"char32_t", "unsigned int"},
	{"std::nullptr_t", "void *"},
};

std::optional<tinfo_t> named(const std::string& name)
{
	tinfo_t t;
	if (!t.get_named_type(get_idati(), name.c_str()))
		return std::nullopt;
	return t;
}

// a typedef src declares itself (src/, gen/), not a system or SDK header's
bool srcTypedef(const SrcModel::Typedef& alias)
{
	return alias.file.starts_with("src/") || alias.file.starts_with("gen/");
}

funcarg_t argument(const std::string& name, const tinfo_t& type, uint32 flags = 0)
{
	funcarg_t a;
	a.name = name.c_str();
	a.type = type;
	a.flags = flags;
	return a;
}

}  // namespace

std::optional<tinfo_t> SrcPush::typeOf(const std::string& key)
{
	if (auto found = m_typeCache.find(key); found != m_typeCache.end())
		return found->second;
	const SrcModel::Type* type = m_model.type(key);
	std::optional<tinfo_t> result = type ? buildType(*type) : std::nullopt;
	if (!result)
		log("unmapped-type", key);
	m_typeCache.emplace(key, result);
	return result;
}

std::optional<tinfo_t> SrcPush::primitive(const std::string& cName)
{
	if (cName == "void")
		return voidType();
	// the parser reads `signed char` (and `signed __int8`) as plain char: the signed byte is BTMT_SIGNED
	if (cName == "signed char")
		return tinfo_t(BT_INT8 | BTMT_SIGNED);
	tinfo_t t;
	std::string decl = cName + " fnvd_x;";
	if (!parse_decl(&t, nullptr, get_idati(), decl.c_str(), PT_SIL))
		return std::nullopt;
	return t;
}

std::optional<tinfo_t> SrcPush::buildType(const SrcModel::Type& type)
{
	const std::string& kind = type.kind;
	if (kind == "builtin")
	{
		auto c = kPrimitive.find(type.name);
		return c == kPrimitive.end() ? std::nullopt : primitive(c->second);
	}
	if (kind == "const" || kind == "volatile")
		return typeOf(type.of);
	if (kind == "ptr" || kind == "lref" || kind == "rref")
	{
		std::optional<tinfo_t> target = typeOf(type.of);
		return target ? std::optional(pointerTo(*target)) : std::nullopt;
	}
	if (kind == "array")
	{
		std::optional<tinfo_t> element = typeOf(type.of);
		if (!element || type.count < 0)
			return std::nullopt;
		tinfo_t array;
		array.create_array(*element, static_cast<uint32>(type.count));
		return array;
	}
	if (kind == "fn")
	{
		std::optional<callcnv_t> cc = conventionCode(type.cc);
		std::optional<tinfo_t> ret = typeOf(type.ret);
		if ((!cc && !type.variadic) || !ret)
			return std::nullopt;
		func_type_data_t fd;
		fd.set_cc(type.variadic ? CM_CC_ELLIPSIS : *cc);
		fd.rettype = *ret;
		for (const std::string& p : type.params)
		{
			std::optional<tinfo_t> pt = typeOf(p);
			if (!pt)
				return std::nullopt;
			fd.push_back(argument("", *pt));
		}
		tinfo_t f;
		return f.create_func(fd) ? std::optional(f) : std::nullopt;
	}
	if (kind == "record")
		return recordType(type.name);
	if (kind == "enum")
		return enumType(type.name);
	if (kind == "typedef")
	{
		const SrcModel::Typedef* alias = m_model.typedefNamed(type.name);
		if (!alias)
			return std::nullopt;
		if (srcTypedef(*alias))
			return typeOf(alias->target);
		// a system or SDK typedef is IDA's of its name where that is src's type (IDA's int8_t is plain char, src's
		// signed char), or where src's target has no IDA type (__builtin_va_list) to tell them apart. equals_to holds
		// plain char and signed char alike; their full basic types (the realtype's modifier bits) tell them apart.
		std::optional<tinfo_t> ida = named(type.name);
		const SrcModel::Type* target = m_model.type(alias->target);
		std::optional<tinfo_t> own = target ? buildType(*target) : std::nullopt;
		if (ida && (!own || (ida->equals_to(*own) && ida->get_realtype() == own->get_realtype())))
			return ida;
		return own;
	}
	if (kind == "vector" && type.of == "builtin:float" && type.count == 4)
		return named("__m128");
	// a pointer to member: an integer of its size (4 for a single-inheritance class's member function)
	if (kind == "memptr")
		return tinfo_t(type.size == 8 ? BT_INT64 : BT_INT32);
	return std::nullopt;
}

std::optional<tinfo_t> SrcPush::recordType(const std::string& name)
{
	const std::string& ida = m_structNames.pushName(name);
	std::optional<tinfo_t> existing = named(ida);
	const SrcModel::Record* record = m_model.record(name);
	if (!record)
	{
		// a record src declares without a definition (an incomplete type, an instance srcdb did not complete): IDA's
		// struct of its spelling, else a forward declaration of it, so pointers to it convert; no size is guessed
		if (existing && (existing->is_udt() || existing->is_forward_struct() || existing->is_forward_union()))
			return existing;
		tinfo_t declared;
		if (declared.create_forward_decl(get_idati(), BTF_STRUCT, ida.c_str(), NTF_REPLACE) != TERR_OK)
		{
			log("struct-create-failed", name, ida);
			return std::nullopt;
		}
		count("forward declarations created");
		return named(ida);
	}
	// a typedef under the name (IDA's `typedef _X X`) is no struct of it: the shell replaces it
	if (existing && existing->is_udt() && !existing->is_typedef())
		return existing;
	udt_type_data_t udt;
	udt.set_fixed(true);
	udt.total_size = udt.unpadded_size = static_cast<size_t>(record->size);
	tinfo_t shell;
	if (!shell.create_udt(udt, record->tag == "union" ? BTF_UNION : BTF_STRUCT) ||
		shell.set_named_type(get_idati(), ida.c_str(), NTF_REPLACE) != TERR_OK)
	{
		log("struct-create-failed", name, ida);
		return std::nullopt;
	}
	count("structs created");
	return named(ida);
}

std::optional<tinfo_t> SrcPush::enumType(const std::string& name)
{
	const SrcModel::Enum* e = m_model.enumeration(name);
	if (!e)
		return std::nullopt;
	if (std::optional<tinfo_t> t = named(name))
	{
		if (t->is_enum())
			return t;
		log("enum-name-taken", name, text(*t));
		return std::nullopt;
	}
	if (!writeEnum(*e))
		return std::nullopt;
	count("enums created");
	return named(name);
}

bool SrcPush::writeEnum(const SrcModel::Enum& e)
{
	std::optional<tinfo_t> under = e.underlying.empty() ? primitive("int") : typeOf(e.underlying);
	if (!under)
	{
		log("enum-create-failed", e.name, "underlying type " + e.underlying);
		return false;
	}
	// IDA's enumerator names are global: each takes the enum's scope
	enum_type_data_t etd;
	etd.set_nbytes(static_cast<int>(under->get_size()));
	for (const SrcModel::Enumerator& v : e.enumerators)
		etd.push_back(edm_t((e.name + "::" + v.name).c_str(), static_cast<uint64>(v.value)));
	tinfo_t created;
	if (!created.create_enum(etd) || created.set_named_type(get_idati(), e.name.c_str(), NTF_REPLACE) != TERR_OK)
	{
		log("enum-create-failed", e.name, std::to_string(e.enumerators.size()) + " enumerators");
		return false;
	}
	return true;
}

std::optional<tinfo_t> SrcPush::prototype(const SrcModel::Function& function, ea_t address)
{
	auto unmapped = [&](const std::string& why) -> std::optional<tinfo_t> {
		log("prototype-unmapped", function.name, why);
		return std::nullopt;
	};
	const SrcModel::Type* type = m_model.type(function.type);
	if (!type || type->kind != "fn")
		return unmapped("no function type " + function.type);
	std::string cc = type->cc;
	std::optional<tinfo_t> ret = typeOf(type->ret);
	if (!ret)
		return unmapped("return type " + type->ret);
	std::vector<funcarg_t> params;
	for (size_t k = 0; k < type->params.size(); ++k)
	{
		std::optional<tinfo_t> pt = typeOf(type->params[k]);
		if (!pt)
			return unmapped("parameter type " + type->params[k]);
		if (pt->is_array())
			pt = pointerTo(pt->get_array_element());
		params.push_back(argument(k < function.params.size() ? function.params[k] : "", *pt));
	}
	const SrcModel::Abi* abi = m_model.abi(function.mangled);
	bool indirect = abi && abi->ret == "indirect";
	std::optional<tinfo_t> self;
	if (!function.cls.empty() && !function.isStatic)
	{
		std::string entry = entryClass(m_model, function);
		if (entry.empty())
			return unmapped("no base of " + function.cls + " at the secondary vtable holding it");
		std::optional<tinfo_t> object = recordType(entry);
		if (!object)
			return unmapped("class " + entry);
		self = pointerTo(*object);
		// a virtual the exe enters through a secondary vtable: this points to that base subobject of the class, IDA's
		// shifted pointer (`B *__shifted(D, N)`), through which Hex-Rays reads the class's own members (`ADJ(this)`)
		if (int64_t offset = entryOffset(m_model, function).value_or(0); offset != 0)
		{
			std::optional<tinfo_t> cls = recordType(function.cls);
			if (!cls)
				return unmapped("class " + function.cls);
			ptr_type_data_t shifted;
			shifted.obj_type = *object;
			shifted.parent = *cls;
			shifted.delta = static_cast<int32>(offset);
			shifted.taptr_bits = TAPTR_SHIFTED;
			if (!self->create_ptr(shifted))
				return unmapped("shifted pointer to " + entry + " in " + function.cls);
		}
	}
	// src's spellings of exe conventions C++ cannot declare, translated where the exe's code says so
	if (address != BADADDR && !type->variadic)
		try
		{
			if (!indirect && edxParameter(function, *type, "thiscall") == 1 && !m_facts.abi(uint32_t(address)).edx)
			{
				cc = "thiscall";
				params.erase(params.begin() + 1);
			}
			else if (function.isStatic && !function.cls.empty() && structorName(function.name) == "Construct" &&
					 !type->params.empty() && type->params[0] == "ptr(builtin:void)" &&
					 m_facts.returnsThis(uint32_t(address)))
			{
				std::optional<tinfo_t> cls = recordType(function.cls);
				if (!cls)
					return unmapped("class " + function.cls);
				cc = "thiscall";
				params[0] = argument("this", pointerTo(*cls));
			}
		}
		catch (const exefacts::Raised&)
		{
			// no exe facts for the function's code: src's spelling stands
		}
	std::optional<callcnv_t> code = conventionCode(cc);
	if (!code && !type->variadic)
		return unmapped("convention " + cc);
	func_type_data_t fd;
	fd.set_cc(type->variadic ? CM_CC_ELLIPSIS : *code);
	fd.rettype = indirect ? pointerTo(*ret) : *ret;
	if (self)
		fd.push_back(argument("this", *self));
	if (indirect)
		fd.push_back(argument("result", pointerTo(*ret), FAI_RETPTR | FAI_STRUCT));
	for (funcarg_t& p : params)
		fd.push_back(std::move(p));
	// src's `[[noreturn]]` is IDA's `__noreturn`
	if (function.noreturn)
		fd.flags |= FTI_NORET;
	tinfo_t t;
	if (!t.create_func(fd))
		return unmapped("create_func failed");
	return t;
}

}  // namespace idakit
