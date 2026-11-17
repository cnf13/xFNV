// Self: the src declaration at the function's address (the ref idakit sync types it from), checked against the exe:
// a member (virtual or not), a static member or a free function; a constructor or destructor written as a static
// member (RULES: Construct, Destruct, DeletingDestruct); a member of a class template instance or a function
// template's instance where src has its explicit specialization (its `template <>` heads), whether its name is
// overloaded in its class's scope and whether it is a protected member (the registration's form). Src's convention
// puts the arguments where the database's does and pops what the exe pops (exefacts' pops: its `ret N`, else what its
// proven tail-jump targets pop; where the exe proves none, any; cdecl pops nothing), with Hex-Rays' parameters and
// return value of the declared types; a record src returns by value is the exe's hidden result pointer. Its
// parameters take the declared types and names. A __usercall/__userpurge src declares through its usercall forwarder
// is registered FNVD_REPLACES_USERCALL/_USERPURGE: its arguments where the model's locations put them (usercall-
// Registration). An address src registers a body for
// is not emitted, unless that body sits in the emitter's guarded block (SrcEdit's emittedBlock): such a body is
// emitted again.

#include "EmitPasses.hpp"

#include "IdaSupport.hpp"

#include "idakit/SrcEdit.hpp"
#include "idakit/SrcIda.hpp"

#include <Facts.hpp>

#include <fstream>
#include <regex>

namespace idakit
{

namespace
{

// The canonical key with each reference as a pointer to the same type: IDA has no reference type, so sync writes
// src's references as pointers, a function pointer's parameters and return included.
std::string referencesAsPointers(SrcTypes& types, const std::string& key)
{
	if (std::optional<SrcTypes::Function> f = SrcTypes::function(key))
	{
		f->ret = referencesAsPointers(types, f->ret);
		for (std::string& param : f->params)
			param = referencesAsPointers(types, param);
		return SrcTypes::functionKey(*f);
	}
	SrcTypes::Parts p = types.parts(key);
	if (p.kind == "lref" || p.kind == "ptr")
		return "ptr(" + referencesAsPointers(types, p.of) + ")";
	if (p.kind == "array")
		return "array(" + referencesAsPointers(types, p.of) + "," + std::to_string(p.count) + ")";
	return key;
}

// src's key and Hex-Rays' are one type, or the same but for src's references where Hex-Rays has pointers to the same
// types (at the top, or in a function pointer's parameters and return)
bool sameOrReference(SrcTypes& types, const std::string& src, const std::string& ida)
{
	std::string a = types.canonical(src), b = types.canonical(ida);
	return a == b || referencesAsPointers(types, a) == b;
}

// the lvar is written or has its address taken somewhere (a reference parameter can be neither)
bool rebound(const Function& function, int index)
{
	bool found = false;
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
		if ((written(e) || e->op == cot_ref) && lvarIndex(e->x) == index)
			found = true;
	});
	return found;
}

// `R result;` runs no code: no vfptr in it or its bases, its default constructor (if it declares constructors) has no
// code, and so for every base and field of record type
bool constructsNothing(const SrcModel& model, const std::string& name)
{
	const SrcModel::Record* record = model.record(name);
	if (!record || record->vfptr || !record->vtables.empty())
		return false;
	bool declared = false, empty = false;
	for (const SrcModel::Function* f : model.structorsOf(name))
		if (f->structor == "ctor")
		{
			declared = true;
			const SrcModel::Type* type = model.type(f->type);
			empty = empty || (type && type->params.empty() && f->empty);
		}
	if (declared && !empty)
		return false;
	for (const SrcModel::Base& base : record->bases)
		if (base.isVirtual || !constructsNothing(model, base.record))
			return false;
	for (const SrcModel::Field& field : record->fields)
	{
		const SrcModel::Type* t = model.type(model.storage(field.type));
		if (t && t->kind == "record" && !constructsNothing(model, t->name))
			return false;
	}
	return true;
}

// `R result;` compiles: R declares a constructor callable without arguments (no parameters, or default arguments for
// all: NiColorA's), or none (the implicit one), and so do its bases and record fields
bool defaultConstructible(const SrcModel& model, const std::string& name)
{
	const SrcModel::Record* record = model.record(name);
	if (!record)
		return false;
	bool declared = false, plain = false;
	for (const SrcModel::Function* f : model.structorsOf(name))
		if (f->structor == "ctor")
		{
			declared = true;
			const SrcModel::Type* type = model.type(f->type);
			plain = plain || (type && (type->params.empty() || f->required == 0));
		}
	if (declared)
		return plain;
	for (const SrcModel::Base& base : record->bases)
		if (!defaultConstructible(model, base.record))
			return false;
	for (const SrcModel::Field& field : record->fields)
	{
		const SrcModel::Type* t = model.type(model.storage(field.type));
		if (t && t->kind == "record" && !defaultConstructible(model, t->name))
			return false;
	}
	return true;
}

// the ref sits in the emitter's guarded block of its file (a body with unresolved spots, emitted again each run)
bool emitted(const SrcModel::Ref& ref)
{
	std::ifstream in(ref.file);
	std::vector<std::string> lines;
	for (std::string line; std::getline(in, line);)
		lines.push_back(line);
	return emittedBlock(lines, ref.line).has_value();
}

// src's and Hex-Rays' keys are arithmetic types C++ converts between implicitly and the exe passes alike: integers
// (or enums) of one size, or (result) two floating types, both returned in ST0
bool sameScalar(SrcTypes& types, const std::string& src, const std::string& ida, bool result)
{
	auto kind = [&](const std::string& key) {
		std::string k = types.canonical(key);
		SrcTypes::Parts p = types.parts(k);
		if (k == "builtin:float" || k == "builtin:double" || k == "builtin:long double")
			return 2;
		return p.kind == "enum" || (p.kind == "builtin" && k != "builtin:void") ? 1 : 0;
	};
	int a = kind(src), b = kind(ida);
	if (a == 0 || a != b)
		return false;
	return a == 2 ? result : types.model().sizeOf(src) == types.model().sizeOf(ida);
}

// the name of the local pointer a rebound parameter is copied to at entry: Hungarian `arX`/`aX` as `pX`, `akX` as
// `pkX`, another name `p` + it capitalised; one no other local or parameter has
std::string entryCopyName(const Function& function, const std::string& parameter)
{
	std::string rest = parameter;
	if (rest.size() > 1 && rest[0] == 'a' && std::isupper(static_cast<unsigned char>(rest[1])))
		rest = rest.substr(1);
	else if (rest.size() > 2 && rest[0] == 'a' && std::islower(static_cast<unsigned char>(rest[1])) &&
			 std::isupper(static_cast<unsigned char>(rest[2])))
		rest = rest[1] == 'r' ? rest.substr(2) : rest.substr(1);
	else if (!rest.empty())
		rest[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(rest[0])));
	std::string name = "p" + rest;
	auto taken = [&](const std::string& candidate) {
		for (const Local& local : function.locals)
			if (local.name == candidate)
				return true;
		return function.self && std::find(function.self->params.begin(), function.self->params.end(), candidate) !=
									function.self->params.end();
	};
	std::string unique = name;
	for (int k = 2; taken(unique); ++k)
		unique = name + std::to_string(k);
	return unique;
}

// The parameter types and constness of two function types are the same (one overrides or hides the other).
bool sameParameters(const SrcModel& model, const std::string& a, const std::string& b)
{
	const SrcModel::Type* x = model.type(a);
	const SrcModel::Type* y = model.type(b);
	return x && y && x->params == y->params && x->isConst == y->isConst;
}

// the name with every template argument list dropped (`hkArrayUtil::construct<T>` as `hkArrayUtil::construct`)
std::string withoutTemplateArguments(const std::string& name)
{
	std::string out;
	int depth = 0;
	for (char c : name)
		if (c == '<')
			++depth;
		else if (c == '>')
			--depth;
		else if (depth == 0)
			out += c;
	return out;
}

// Several src functions answer to the name in the class's scope: its overloads, or (a non-static member's) a function
// of the name with other parameters in a base, which a `using` declaration may bring into the class (counted whether
// or not one does: a registration naming the pointer type is right either way), or for `operator=` the copy assignment
// C++ declares implicitly beside a user-declared one of other parameters (srcdb does not record it). A static
// member's bases are not looked at: src's static members (Construct, Destruct) are declared in each class and hide
// the base's. A template's member or a function template's instance is overloaded where the template declares
// another function of the name (every instance has it; srcdb records only the instances src completes, each at its
// template's declaration).
bool overloaded(const SrcModel& model, const SrcModel::Function& self)
{
	if (model.functionsNamed(self.name).size() > 1)
		return true;
	std::string last = lastComponent(self.name);
	// an operator whose symbol has an angle bracket (`operator<`, `operator->`) has no name template arguments can be
	// told from
	bool angled = last.starts_with("operator") && last.size() > 8 &&
				  (last[8] == '<' || last[8] == '>' || last.compare(8, 2, "->") == 0);
	if (model.templateHeads(self.mangled, self.cls) > 0 && !angled)
	{
		std::string name = withoutTemplateArguments(self.name);
		for (const auto& [mangled, f] : model.functions())
			if (f.file == self.file && f.line != self.line && withoutTemplateArguments(f.name) == name)
				return true;
	}
	const SrcModel::Record* record = self.cls.empty() || self.isStatic ? nullptr : model.record(self.cls);
	if (!record)
		return false;
	if (last == "operator=")
		return true;
	std::vector<const SrcModel::Record*> pending{record};
	std::set<std::string> seen;
	while (!pending.empty())
	{
		const SrcModel::Record* r = pending.back();
		pending.pop_back();
		for (const SrcModel::Base& base : r->bases)
		{
			const SrcModel::Record* b = model.record(base.record);
			if (!b || !seen.insert(b->name).second)
				continue;
			for (const SrcModel::Function* f : model.functionsNamed(b->name + "::" + last))
				if (!sameParameters(model, f->type, self.type))
					return true;
			pending.push_back(b);
		}
	}
	return false;
}

// C++'s keywords and alternative tokens, which name no variable
bool keyword(const std::string& name)
{
	static const std::set<std::string> words = {
		"alignas", "alignof", "and", "and_eq", "asm", "auto", "bitand", "bitor", "bool", "break", "case", "catch",
		"char", "char8_t", "char16_t", "char32_t", "class", "compl", "concept", "const", "consteval", "constexpr",
		"constinit", "const_cast", "continue", "co_await", "co_return", "co_yield", "decltype", "default", "delete",
		"do", "double", "dynamic_cast", "else", "enum", "explicit", "export", "extern", "false", "float", "for",
		"friend", "goto", "if", "inline", "int", "long", "mutable", "namespace", "new", "noexcept", "not", "not_eq",
		"nullptr", "operator", "or", "or_eq", "private", "protected", "public", "register", "reinterpret_cast",
		"requires", "return", "short", "signed", "sizeof", "static", "static_assert", "static_cast", "struct",
		"switch", "template", "this", "thread_local", "throw", "true", "try", "typedef", "typeid", "typename",
		"union", "unsigned", "using", "virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq"};
	return words.count(name) > 0;
}

// Hex-Rays names a local after the callee parameter it is passed to, unique among its own names only (and repeats a
// register's name, `_AX`, in an asm statement's locals): a local that is no parameter, named as a name of the head
// (src's parameters, `pThis`, `this`), as a local before it or as a C++ keyword, takes that name with the first
// free number
void uniqueLocalNames(Function& function)
{
	const lvars_t& lvars = *function.cfunc->get_lvars();
	std::set<std::string> taken(function.self->params.begin(), function.self->params.end());
	for (size_t k = 0; k < lvars.size(); ++k)
		if (lvars[k].is_arg_var())
			taken.insert(function.locals[k].name);
	for (size_t k = 0; k < lvars.size(); ++k)
	{
		if (lvars[k].is_arg_var())
			continue;
		Local& local = function.locals[k];
		std::string name = local.name;
		for (int n = 2; taken.count(name) || keyword(name); ++n)
			name = local.name + std::to_string(n);
		local.name = name;
		taken.insert(name);
	}
}

// The member is declared under `protected:` (FNVD_REPLACES cannot name it): the last access label at its class
// body's own brace depth above its declaration, else the class-key's default.
bool protectedMember(const SrcModel& model, const SrcModel::Function& self)
{
	const SrcModel::Record* record = model.record(self.cls);
	if (!record || record->file != self.file || record->line <= 0 || self.line <= record->line)
		return false;
	static const std::regex label(R"(^\s*(public|protected|private)\s*:([^:]|$))");
	std::ifstream in(self.file);
	std::string access = record->tag == "class" ? "private" : "public";
	int depth = 0;
	int64_t number = 0;
	std::smatch match;
	for (std::string line; std::getline(in, line) && ++number < self.line;)
	{
		if (number < record->line)
			continue;
		if (depth == 1 && std::regex_search(line, match, label))
			access = match[1];
		for (char c : line)
			depth += c == '{' ? 1 : c == '}' ? -1 : 0;
	}
	return access == "protected";
}

// A __usercall/__userpurge src declares through its usercall forwarder (or registers FNVD_REPLACES_USERCALL/_USERPURGE
// already): the registration's locations (Function::userLocations, userPurge). Each of Hex-Rays' arguments, in the
// database's usercall type, is src's parameter of the same place (a member's `this` first), at the location the model
// records for it (srcdb's ref_location: the forwarder's UserRegs registers, the rest on the stack in order); the
// result is where the seam's adapter leaves it (eax, edx:eax, ST0); the code takes no x87 input, leaves ebx, esi and
// edi as it found them (the C++ body keeps them) and pops its stack arguments (a userpurge) or nothing (a usercall).
// Returns why it cannot be registered so ("" where it can).
std::string usercallRegistration(Function& function, const SrcModel::Ref& ref, const SrcModel::Function& self,
								 const SrcModel::Type& type, const tinfo_t& ftype, exefacts::Facts& exe)
{
	func_type_data_t fd;
	if (!ftype.get_func_details(&fd))
		return "the database's usercall type has no details";
	if (ref.locations.empty())
		return "no register mapping in the model for the usercall " + self.name;
	bool member = !self.cls.empty() && !self.isStatic;
	size_t first = member ? 1 : 0;
	if (fd.size() != type.params.size() + first)
		return "the database's usercall type has " + std::to_string(fd.size()) + " arguments, src " +
			   std::to_string(type.params.size() + first);
	std::vector<std::string> locations;
	int64_t offset = 0;
	for (size_t k = 0; k < fd.size(); ++k)
	{
		int64_t index = static_cast<int64_t>(k) - static_cast<int64_t>(first);
		std::string which = index < 0 ? std::string("this") : "parameter " + std::to_string(index + 1);
		auto found = ref.locations.find(index);
		if (found == ref.locations.end())
			return "no register mapping in the model for " + which + " of the usercall " + self.name;
		const funcarg_t& a = fd[k];
		size_t size = a.type.get_size();
		if (a.argloc.is_stkoff())
		{
			if (found->second != "stack")
				return which + ": the database's on the stack, src's in " + found->second;
			if (a.argloc.stkoff() != offset)
				return which + ": the database's at stack offset " + std::to_string(a.argloc.stkoff()) +
					   ", src's stack arguments put it at " + std::to_string(offset);
			offset += stackSize(a.type);
		}
		else if (std::string reg = userRegister(a.argloc); !reg.empty())
		{
			if (reg != found->second)
				return which + ": the database's in " + reg + ", src's in " + found->second;
		}
		else
		{
			std::string where = locationText(a.argloc, size);
			bool fpu = a.argloc.is_reg1() && (where.starts_with("st") || where.starts_with("xmm"));
			return std::string(fpu ? "an x87 or SSE register input: " : "an argument in ") + where;
		}
		locations.push_back(found->second);
	}
	uint32_t at = static_cast<uint32_t>(function.ea);
	if (exe.x87Input(at))
		return "an x87 register input: the code uses ST0 before pushing it";
	if (!userResult(fd))
		return "a result in " +
			   (fd.rettype.is_udt() ? std::string("a record") : locationText(fd.retloc, fd.rettype.get_size())) +
			   ", not eax, edx:eax or ST0";
	if (exefacts::Families changed = exe.unrestored(at) & (exefacts::kEbx | exefacts::kEsi | exefacts::kEdi))
	{
		std::string names;
		for (auto [family, name] : {std::pair(exefacts::kEbx, "ebx"), std::pair(exefacts::kEsi, "esi"),
									std::pair(exefacts::kEdi, "edi")})
			if (changed & family)
				names += (names.empty() ? "" : ", ") + std::string(name);
		return "changes " + names + " and does not restore it (a C++ body keeps it)";
	}
	// the exe's pops, else the database's convention
	bool purge = function.pops ? *function.pops != 0 : (fd.get_cc() & CM_CC_MASK) == CM_CC_SPECIALP;
	if (function.pops && *function.pops != (purge ? offset : 0))
		return "pops " + std::to_string(*function.pops) + ", its stack arguments are " + std::to_string(offset) +
			   " bytes";
	size_t listed = locations.size();
	while (listed > 0 && locations[listed - 1] == "stack")
		--listed;
	std::string list;
	for (size_t k = 0; k < listed; ++k)
		list += (list.empty() ? "" : ", ") + locations[k];
	function.userLocations = list;
	function.userPurge = purge && offset != 0;
	return "";
}

}  // namespace

std::optional<ArgumentLvars> argumentLvars(const Function& function, const SrcModel& model,
										   const SrcModel::Function& declaration)
{
	const SrcModel::Type* type = model.type(declaration.type);
	if (!type)
		return std::nullopt;
	tinfo_t ftype;
	function.cfunc->get_func_type(&ftype);
	const SrcModel::Abi* abi = model.abi(declaration.mangled);
	bool byValue = abi && abi->ret == "indirect";
	int edxParameter = byValue ? -1 : idakit::edxParameter(declaration, *type, conventionName(ftype.get_cc()));
	bool member = !declaration.cls.empty() && !declaration.isStatic;
	const intvec_t& argidx = function.cfunc->argidx;
	size_t first = member ? 1 : 0, hidden = byValue ? 1 : 0, edx = edxParameter >= 0 ? 1 : 0;
	if (argidx.size() + edx != type->params.size() + first + hidden)
		return std::nullopt;
	ArgumentLvars out;
	if (member)
		out.thisLvar = argidx[0];
	if (byValue)
		out.result = argidx[first];
	out.parameters.assign(type->params.size(), -1);
	for (size_t k = 0; k < type->params.size(); ++k)
		if (static_cast<int>(k) != edxParameter)
			out.parameters[k] = argidx[first + hidden + k - (edx && static_cast<int>(k) > edxParameter ? 1 : 0)];
	return out;
}

void resolveSelf(Function& function, Context& context)
{
	SrcTypes& types = context.types;
	const SrcModel& model = context.model;
	// Self's reasons lead the function's blockers: why it is no member definition comes first
	size_t own = 0;
	auto block = [&](std::string why) {
		function.blockers.insert(function.blockers.begin() + static_cast<std::ptrdiff_t>(own++), std::move(why));
	};

	// its `ret N`, else what its proven tail-jump targets pop; unknown pops contradict no convention
	try
	{
		function.pops = context.exe.pops(static_cast<uint32_t>(function.ea));
	}
	catch (const std::exception& error)
	{
		block(std::string("the bytes it pops: ") + error.what());
	}

	RefChoice choice = chooseRef(model, function.ea);
	if (choice.names.size() > 1)
	{
		std::string names;
		for (const std::string& n : choice.names)
			names += (names.empty() ? "" : ", ") + n;
		block("fold of " + names);
	}
	// a C++ constructor or destructor src registers there is its body
	if (choice.structor)
	{
		function.registered = choice.structor->file + ":" + std::to_string(choice.structor->line);
		return;
	}
	if (!choice.ref)
	{
		block("no src declaration");
		return;
	}
	// a src body is src's; one in the emitter's guard is emitted again, replacing that whole block
	bool guarded = choice.ref->kind == "body" && emitted(*choice.ref);
	if (choice.ref->kind == "body" && !guarded)
	{
		function.registered = choice.ref->file + ":" + std::to_string(choice.ref->line);
		return;
	}
	const SrcModel::Function* self = model.function(choice.ref->function);
	const SrcModel::Type* type = self ? model.type(self->type) : nullptr;
	if (!self || !type)
	{
		block("src declaration " + choice.ref->target + " not in the model");
		return;
	}
	function.self = self;
	// a forwarder in a .cpp (or the guarded block holding an emitted body and its forwarder): the definition
	// replaces it there
	if (const SrcModel::Definition* d = model.definition(self->mangled);
		d && !d->instance && d->file.size() > 4 && d->file.compare(d->file.size() - 4, 4, ".cpp") == 0)
	{
		function.replaces = d;
		function.notes.push_back(std::string(guarded ? "replaces the emitted body at " : "replaces the forwarder at ") +
								 d->file + ":" + std::to_string(d->line));
	}
	const std::string& cls = self->cls;
	bool member = !cls.empty() && !self->isStatic;
	const SrcModel::Record* owner = cls.empty() ? nullptr : model.record(cls);
	// a member of a class template instance (or of a class nested in one) or a function template's instance: its
	// definition is src's explicit specialization (`template <> R T<A>::f(...)`); an instance of the template's own
	// body is the template's
	function.templateHeads = model.templateHeads(self->mangled, self->cls);
	if (function.templateHeads > 0)
		if (const SrcModel::Definition* d = model.definition(self->mangled); !d || d->instance)
			block("template instance " + self->name + " with no explicit specialization in src");
	function.overloaded = overloaded(model, *self);
	std::string structor = structorName(self->name);
	if (!structor.empty())
	{
		if (cls.empty() || !self->isStatic)
			block("a " + structor + " that is no static member: " + self->name);
		// the destructor macros have no _OVERLOAD form
		else if (structor != "Construct" && model.functionsNamed(self->name).size() > 1)
			block("an overloaded " + structor + ": " + self->name);
		function.structor = structor;
	}
	// a protected non-virtual member (a structor registers by its own macro, a virtual through Access)
	function.protectedMember = !cls.empty() && !self->isVirtual && structor.empty() && protectedMember(model, *self);
	if (type->variadic)
		block("variadic");
	// the exe's convention as the database has it (synced from src's declaration): it must put the arguments where
	// src's does (ecx/edx in thiscall and fastcall only)
	tinfo_t ftype;
	function.cfunc->get_func_type(&ftype);
	std::string exeConvention = conventionName(ftype.get_cc());
	bool byValue = false;
	const SrcModel::Abi* abi = model.abi(self->mangled);
	// src's free fastcall with an unused apEdx is the exe's thiscall that takes no edx argument (not with a hidden
	// result pointer, which the two conventions need not pass alike)
	if (!abi || abi->ret != "indirect")
		function.edxParameter = edxParameter(*self, *type, exeConvention);
	if (!abi)
		block("no ABI row for " + self->name);
	else if (exeConvention == "usercall" || exeConvention == "userpurge")
	{
		// src's own convention (cdecl, a member's thiscall) is the adapter's to call: the exe's registers and pops are
		// the registration's
		if (abi->ret == "indirect")
			block("a usercall returning " + type->ret + " by value: the usercall registration has no form for it");
		else if (function.overloaded)
			block("an overloaded usercall: the usercall registration names it alone");
		// srcdb's access from namespace scope (an enclosing class's included), or a `protected:` label
		else if (function.protectedMember || self->access == "protected" || self->access == "private")
			block("a " + (self->access == "private" ? std::string("private") : std::string("protected")) +
				  " usercall member: the usercall registration names it from namespace scope");
		else if (std::string why = usercallRegistration(function, *choice.ref, *self, *type, ftype, context.exe);
				 !why.empty())
			block("usercall: " + why);
	}
	else
	{
		// the callee pops its stack arguments in thiscall, stdcall and fastcall, the caller in cdecl
		bool calleePops = abi->cc == "thiscall" || abi->cc == "stdcall" || abi->cc == "fastcall";
		int64_t popped = calleePops ? abi->stackBytes : 0;
		if (!calleePops && abi->cc != "cdecl")
			block("src convention " + abi->cc);
		else if (function.pops && popped != *function.pops)
			block("src pops " + std::to_string(popped) + " (" + abi->cc + "), the exe " +
				  std::to_string(*function.pops));
		if (exeConvention.starts_with("user"))
			block("the exe's convention is " + exeConvention);
		else if ((abi->cc == "thiscall" || abi->cc == "fastcall" || exeConvention == "thiscall" ||
				  exeConvention == "fastcall") &&
				 abi->cc != exeConvention && function.edxParameter < 0)
			block("src convention " + abi->cc + ", the exe's " + exeConvention);
		byValue = abi->ret == "indirect";
	}

	// Hex-Rays' arguments: this (a member's), the hidden result pointer (MSVC passes it after this), src's parameters
	// (but apEdx)
	const intvec_t& argidx = function.cfunc->argidx;
	size_t first = member ? 1 : 0;
	size_t hidden = byValue ? 1 : 0;
	size_t edx = function.edxParameter >= 0 ? 1 : 0;
	if (argidx.size() + edx != type->params.size() + first + hidden)
	{
		block("Hex-Rays has " + std::to_string(argidx.size()) + " arguments, src " +
			  std::to_string(type->params.size() + first + hidden - edx));
		return;
	}
	std::optional<ArgumentLvars> lvars = argumentLvars(function, model, *self);
	if (member)
	{
		// a virtual the exe reaches only through a secondary vtable is entered with that base subobject in ecx:
		// Hex-Rays' this is the base's
		std::string entry = cls;
		if (std::optional<int64_t> offset = self->isVirtual && owner ? secondaryVtable(*owner, self->mangled)
																	 : std::nullopt)
		{
			entry = baseAt(model, *owner, *offset);
			if (entry.empty())
				block("virtual in the vtable at " + cls + "+" + std::to_string(*offset) + ", no src base there");
			function.virtualBase = entry;
		}
		Local& thisLocal = function.locals[argidx[0]];
		std::string record = types.parts(types.parts(types.canonical(thisLocal.type)).of).name;
		if (!entry.empty() && (thisLocal.type.empty() || !types.derivesAtZero(entry, record)))
			block("Hex-Rays' this is " + (thisLocal.type.empty() ? thisLocal.why : thisLocal.type) + ", src's " +
				  entry);
		else if (!entry.empty())
			thisLocal.type = "ptr(" + (type->isConst ? "const(record:" + entry + ")" : "record:" + entry) + ")";
		// C++'s this is no variable: a local pointer copied from it at entry
		if (rebound(function, argidx[0]))
		{
			thisLocal.entryCopy = Local::EntryCopy{"", "this"};
			thisLocal.name = entryCopyName(function, "this");
		}
		// Hex-Rays names it from the database's prototype (a1 where that has no `this`, as a stdcall member's)
		else
		{
			thisLocal.name = "this";
			// a _VIA virtual's this is sync's shifted pointer to the base subobject (`B *__shifted(D, N)`): Hex-Rays'
			// ADJ(this), the class's object it is a subobject of, is C++'s this itself
			if (!function.virtualBase.empty())
			{
				std::string object = type->isConst ? "const(record:" + cls + ")" : "record:" + cls;
				forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
					if (adjustedThis(function, e) && types.recordOf(e->type.get_pointed_object()) == owner)
						function.meaning[e] = Rewrite{1, "this", {}, {}, "ptr(" + object + ")"};
				});
			}
		}
	}
	// Construct's object: Hex-Rays' first argument (the database's `X* this`, sync's prototype) is src's pThis
	// (`X* pThis = static_cast<X*>(apMemory);`), src's first parameter (`void* apMemory`) only the head's
	bool construct = structor == "Construct" && !cls.empty() && self->isStatic && !type->params.empty();
	if (construct)
	{
		Local& object = function.locals[argidx[0]];
		std::string key = types.canonical(object.type);
		std::string record = types.parts(types.parts(key).of).name;
		if (types.canonical(type->params[0]) != "ptr(builtin:void)")
			block("Construct's first parameter: src " + type->params[0] + ", not void*");
		else if (object.type.empty() || !types.derivesAtZero(cls, record))
			block("Hex-Rays' object is " + (object.type.empty() ? object.why : object.type) + ", src's " + cls);
		else
		{
			object.type = "ptr(record:" + cls + ")";
			object.name = "pThis";
		}
	}
	if (byValue)
	{
		// src returns the record by value, the exe fills the hidden pointer and returns it: the pointer is a local of
		// the record, `R result;`, that every return returns (clang's NRVO builds it in the caller's slot)
		int index = argidx[first];
		Local& result = function.locals[index];
		std::string pointer = "ptr(" + types.canonical(type->ret) + ")";
		std::string record = types.parts(types.canonical(type->ret)).name;
		bool returnsIt = true;
		forEachStatement(&function.cfunc->body, [&](const cinsn_t* s) {
			if (s->op == cit_return && lvarIndex(&s->creturn->expr) != index)
				returnsIt = false;
		});
		if (result.type.empty() || types.canonical(result.type) != pointer)
			block("the hidden result pointer: src " + pointer + ", Hex-Rays " +
				  (result.type.empty() ? result.why : result.type));
		else if (function.returnType.empty() || types.canonical(function.returnType) != pointer)
			block("return: src " + type->ret + " by value, Hex-Rays " + function.returnType);
		else if (!returnsIt)
			block("returns " + type->ret + " by value: a return other than the hidden result pointer");
		else if (rebound(function, index))
			block("returns " + type->ret + " by value: the hidden result pointer is reassigned or its address taken");
		// `R result;` must compile (a default constructor, implicit or declared) and should run nothing the exe does
		// not: one that runs code (a vfptr store, a declared default constructor with code, a member's) still gives
		// the body, marked, since the exe fills the same hidden result object C++ constructs first
		else if (!defaultConstructible(model, record))
			block("returns " + type->ret + " by value: `" + record + " result;` does not compile (no default constructor)");
		else
		{
			if (!constructsNothing(model, record))
				function.blockers.push_back("returns " + type->ret + " by value: `" + record +
											" result;` runs code the exe does not run");
			// the local is filled: a `const R` result is returned from a plain `R result;`
			SrcTypes::Parts returned = types.parts(types.bare(type->ret));
			result.type = "lref(" + (returned.kind == "const" ? returned.of : type->ret) + ")";
			result.reference = true;
			function.resultArgument = index;
			function.returnType = type->ret;
			function.returnsReference = true;
		}
	}
	for (size_t k = construct ? 1 : 0; k < type->params.size(); ++k)
	{
		if (static_cast<int>(k) == function.edxParameter)
			continue;
		int index = lvars->parameters[k];
		Local& local = function.locals[index];
		const std::string& declared = type->params[k];
		bool convertible = !local.type.empty() && sameScalar(types, declared, local.type, false);
		if (local.type.empty() || (!convertible && !sameOrReference(types, declared, local.type)))
		{
			block("parameter " + std::to_string(k + 1) + ": src " + declared + ", Hex-Rays " +
				  (local.type.empty() ? local.why : local.type));
			continue;
		}
		// a same-size integer for src's enum or integer of another sign stays marked (user, 2026-11-06)
		if (convertible && types.canonical(declared) != types.canonical(local.type))
			function.blockers.push_back("parameter " + std::to_string(k + 1) + ": src " + declared + ", Hex-Rays " +
										local.type);
		bool reference = types.parts(types.canonical(declared)).kind == "lref";
		std::string name = k < self->params.size() && !self->params[k].empty() ? self->params[k] : local.name;
		local.parameter = static_cast<int>(k);
		// a reference cannot be rebound: a local pointer copied from it at entry
		if (reference && rebound(function, index))
		{
			local.type = "ptr(" + types.parts(types.bare(declared)).of + ")";
			local.entryCopy = Local::EntryCopy{declared, name};
			local.name = entryCopyName(function, name);
			continue;
		}
		// a parameter the body writes (`++auiLineOffset` of src's `const uint32_t auiLineOffset`, `++a6` of a
		// `hkVector4* const`) is defined without its top-level const, which only the definition's own parameter
		// has: the declaration keeps it, the function's type is the same
		SrcTypes::Parts top = types.parts(declared);
		bool writes = false;
		if (top.kind == "const")
			forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
				writes |= written(e) && lvarIndex(e->x) == index;
			});
		local.type = writes ? top.of : declared;
		local.reference = reference;
		local.name = name;
	}
	// a record by value is checked above, against the hidden result pointer
	bool convertible = !byValue && !function.returnType.empty() &&
					   sameScalar(types, type->ret, function.returnType, true);
	if (!byValue && !convertible &&
		(function.returnType.empty() || !sameOrReference(types, type->ret, function.returnType)))
		block("return: src " + type->ret + ", Hex-Rays " + function.returnType);
	else if (!byValue)
	{
		if (convertible && types.canonical(type->ret) != types.canonical(function.returnType))
			function.blockers.push_back("return: src " + type->ret + ", Hex-Rays " + function.returnType);
		function.returnsReference = types.parts(types.canonical(type->ret)).kind == "lref";
		function.returnType = type->ret;
	}
	uniqueLocalNames(function);
	function.defines = own == 0;
}

}  // namespace idakit
