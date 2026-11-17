// Fold owners: the linker folded identical functions into one exe address that several src functions own (folds()).
// Each call of such an address names one of them, its owner, which the call site's user-defined call records: the
// owner's src prototype, named with its IDA name. Hex-Rays then reads the call (its arguments and result) as the
// owner's, and the next round's decompile sees it, so that a receiver only an earlier fold call types resolves in a
// later round.
//
// A Construct's call (static, `void* apMemory` first) names the Construct of the object its result goes to: that
// class's among the members, else the one src's class template forwards to the fold for that instance (generic).
//
// A member call names the owner on the receiver's class or its nearest object at offset 0 (a base, or a by-value record
// field: the exe passes `this` unadjusted, and a call of the first field's member is the call on that field) that the
// caller's TU can call (of several on that class, the one src registers at the address); else a class template's
// member a header forwarder names, on the receiver's instance; else the template's member on the receiver's instance
// standing in for that template's instances among the members (other classes' members are no call on this
// receiver), under their one name; else the receiver's instance's member made from a class template's header
// forwarder to the address (srcdb's generic ref; of several on that class, the name a body or fold comment names);
// no template member where a src call of it reaches other exe addresses only
// (its body registered there, or the exe calls its definition makes through forwarders). A fold of free functions
// names the member
// whose parameters are the call's argument types (integers of one size alike, an enum as its integer; a C-variadic
// member by its fixed parameters) and whose return type is what the call's result is used as (of several, the one
// src registers at the address). A folded trivial accessor (exefacts' trivialAccessor) returning a pointer gets its
// owner's too: its result is then of the owner's type (the fold's own returns a neutral `void *`, which leaves every
// vtable call on it untyped), and emit still prints the call as the access itself (FoldedAccessors); so does a folded
// setter, whose owner's parameter types the value it stores (the fold's neutral one leaves it `void *`); one loading
// a field of `this` where no owner resolves returns the receiver's src field there (through by-value members), a
// setter takes that field's type, and one returning `this` plus an offset that field's address (the identity, the
// receiver itself).
// One returning an integer or float keeps the fold's type. An owner whose prototype the exe's code contradicts gets
// none, nor does a C-variadic
// one (Hex-Rays reads no variadic argument at a user-defined call of a C-variadic type).
//
// An address src registers one implicit class template instance's member at (made from the template's definition)
// is no fold in src, but the exe folded its other instances' copies there: a call of it on a receiver of another
// instance of the template (or a class deriving from one at offset 0) names that instance's member made from the
// same definition, which the caller's TU can call, whose src call reaches no other exe address and whose prototype
// passes the same bits; the call is then the receiver's own, uncast.
//
// The fold's own body is its owner's too (ownerPrototype): src registers it once, as the member its declaration
// names (`Same function as Owner::f` on every other copy). For the rebuild of an emit or decompile session the fold
// gets that owner's prototype in place of the class-neutral one sync writes, so that Hex-Rays reads `this` and the
// arguments as the owner's; what the other classes' objects make of the code stays as Hex-Rays then reads it, where
// the passes mark what the owner's types do not hold. Not where the exe's code contradicts the owner's prototype.

#include "idakit/Unfold.hpp"

#include "IdaSupport.hpp"

#include "idakit/ExePrototype.hpp"

#include <Facts.hpp>

#include <algorithm>
#include <climits>
#include <functional>
#include <set>

namespace idakit
{

namespace
{

// The two prototypes pass and return the same bits: one convention, argument count and hidden result pointer, and
// each argument and the return of one type, both pointers (two instances' element pointers, references) or both
// integers (an enum as its integer) of one size.
bool sameBits(const tinfo_t& a, const tinfo_t& b)
{
	func_type_data_t x, y;
	if (!a.get_func_details(&x) || !b.get_func_details(&y) || x.size() != y.size() ||
		(x.get_cc() & CM_CC_MASK) != (y.get_cc() & CM_CC_MASK))
		return false;
	auto alike = [](const tinfo_t& s, const tinfo_t& t) {
		auto integer = [](const tinfo_t& u) { return u.is_integral() || u.is_enum(); };
		return sameType(s, t) || (s.is_ptr() && t.is_ptr()) ||
			   (integer(s) && integer(t) && s.get_size() == t.get_size());
	};
	for (size_t k = 0; k < x.size(); ++k)
		if ((x[k].flags & FAI_RETPTR) != (y[k].flags & FAI_RETPTR) || !alike(x[k].type, y[k].type))
			return false;
	return alike(x.rettype, y.rettype);
}

// The owner rules over src's model, for the calls of one caller (its TU: the src .cpp of its declared function's
// class, "" for a caller src does not declare, which calls header declarations only); definedAt: SrcPush's, the exe
// addresses a src call of a function reaches.
class Owners
{
public:
	using DefinedAt = std::function<std::vector<ea_t>(const SrcModel::Function&)>;

	Owners(const SrcModel& model, SrcTypes& types, std::string tu, DefinedAt definedAt)
		: m_model(model), m_types(types), m_tu(std::move(tu)), m_definedAt(std::move(definedAt))
	{
	}

	// a member called on its object: not static, or a static one whose first parameter is its class's pointer or
	// Construct's `void* apMemory`
	bool onObject(const SrcModel::Function& f) const
	{
		if (f.cls.empty())
			return false;
		if (!f.isStatic)
			return true;
		const SrcModel::Type* type = m_model.type(f.type);
		if (!type || type->params.empty())
			return false;
		std::string first = m_types.canonical(type->params[0]);
		return first == "ptr(record:" + f.cls + ")" || first == "ptr(builtin:void)";
	}

	// a Construct: a static member taking the memory as `void* apMemory` first and returning its class's pointer
	bool constructs(const SrcModel::Function& f) const
	{
		const SrcModel::Type* type = m_model.type(f.type);
		return f.isStatic && !f.cls.empty() && type && !type->params.empty() &&
			   m_types.canonical(type->params[0]) == "ptr(builtin:void)" &&
			   m_types.canonical(type->ret) == "ptr(record:" + f.cls + ")";
	}

	// The Construct of the class template instance the key points to, where src's template defines it for every
	// instance as a forwarder to ea (a generic forwarder ref: BSSimpleList<T>::Construct to 0x96A2D0), which chooseRef
	// leaves out of the fold where src registers one instance's body there (BSSimpleList<void *>'s): a src call of it
	// reaches ea alone, as the exe's does. With the call's argument count, callable from the caller's TU; nullptr for
	// none or several.
	const SrcModel::Function* instanceConstruct(ea_t ea, const std::string& key, size_t arguments) const
	{
		std::string record = pointee(key);
		if (record.empty())
			return nullptr;
		const SrcModel::Function* owner = nullptr;
		for (const SrcModel::Ref& r : m_model.refsAt(ea))
		{
			const SrcModel::Function* f = r.generic && r.kind == "forwarder" ? m_model.function(r.function) : nullptr;
			const SrcModel::Type* type = f ? m_model.type(f->type) : nullptr;
			if (!type || f->cls != record || f == owner || !constructs(*f) || type->params.size() != arguments ||
				!callable(f, false) || reachesElsewhere(ea, *f))
				continue;
			if (owner)
				return nullptr;
			owner = f;
		}
		return owner;
	}

	// A fold member a call on an object can name: one called on its object that src registers, forwards or names in
	// a fold comment at ea, a forwarder only where its function's body is at ea alone (one in a body registered at
	// another address is a call from that body); a static one only from its body, a fold comment or a header
	// forwarder (a header holds no body: its call of the address is the function itself, never a raw call inside a
	// longer one).
	bool candidate(ea_t ea, const SrcModel::Function& f) const
	{
		if (!onObject(f))
			return false;
		const std::vector<SrcModel::Ref>& refs = m_model.refsAt(ea);
		return std::any_of(refs.begin(), refs.end(), [&](const SrcModel::Ref& r) {
			if (r.function != f.mangled || (r.kind != "body" && r.kind != "fold" && r.kind != "forwarder"))
				return false;
			if (r.kind != "forwarder")
				return true;
			const std::vector<uint64_t>& registered = m_model.bodyAddresses(r);
			return std::all_of(registered.begin(), registered.end(), [&](uint64_t a) { return a == ea; }) &&
				   (!f.isStatic || isHeader(r.file));
		});
	}

	// The owner of a call of the fold at ea on a receiver of the src key (empty: untyped) with the arguments, by the
	// receiver's class or an object at its offset 0 (objectsAtZero); nullptr for none.
	const SrcModel::Function* onReceiver(ea_t ea, const SrcPush::Fold& fold, const std::string& receiver,
										 size_t arguments) const
	{
		std::string record = pointee(receiver);
		if (record.empty())
			return nullptr;
		bool constReceiver = m_types.constPointee(receiver);
		std::map<std::string, int> bases = objectsAtZero(record);
		std::vector<const SrcModel::Function*> candidates;
		for (const SrcModel::Function* f : fold.members)
			if (candidate(ea, *f))
				candidates.push_back(f);
		// the owner on the nearest class; of several there, the one src registers at ea, else none
		std::vector<const SrcModel::Function*> nearestOwners;
		int nearest = INT_MAX;
		for (const SrcModel::Function* f : candidates)
			if (auto found = bases.find(f->cls); found != bases.end() && callable(f, constReceiver))
			{
				if (found->second < nearest)
					nearestOwners.clear(), nearest = found->second;
				if (found->second == nearest)
					nearestOwners.push_back(f);
			}
		if (const SrcModel::Function* f = chosen(ea, nearestOwners))
			return f;
		// a class template's member a header forwarder at ea is: the member of the receiver's class (or nearest object
		// at its offset 0) when that is an instance of the template; first the template's member of the forwarder's
		// name where no instance src completes spells the ref, then the template's member some candidates are
		// (templateMember), then srcdb's ref of the receiver's instance's member made from the forwarder (a generic
		// ref); of several generic ones on that class (two forwarders of one template at ea: BSSimpleList's Destruct
		// and DestructPointerList at 0x46FFB0), the one of the name src's body or a fold comment names ea by
		std::vector<const SrcModel::Function*> forwarded;
		nearest = INT_MAX;
		auto consider = [&](const SrcModel::Function* f, int depth) {
			const SrcModel::Type* type = m_model.type(f->type);
			size_t count = type ? type->params.size() + (f->isStatic ? 0 : 1) : 0;
			if (depth > nearest || !onObject(*f) || count != arguments || !callable(f, constReceiver) ||
				reachesElsewhere(ea, *f) || std::find(forwarded.begin(), forwarded.end(), f) != forwarded.end())
				return;
			if (depth < nearest)
				forwarded.clear(), nearest = depth;
			forwarded.push_back(f);
		};
		for (const SrcModel::Ref& ref : m_model.refsAt(ea))
		{
			size_t scope = ref.target.rfind("::");
			if (ref.kind != "forwarder" || !ref.function.empty() || !isHeader(ref.file) ||
				scope == std::string::npos || ref.target.find('<') != std::string::npos)
				continue;
			std::string name = ref.target.substr(0, scope), member = ref.target.substr(scope + 2);
			for (const auto& [cls, depth] : bases)
				if (templateOf(cls) == name)
					for (const SrcModel::Function* f : m_model.functionsNamed(cls + "::" + member))
						consider(f, depth);
		}
		if (forwarded.size() == 1)
			return forwarded.front();
		if (const SrcModel::Function* f = templateMember(ea, candidates, bases, constReceiver, arguments))
			return f;
		forwarded.clear();
		nearest = INT_MAX;
		for (const SrcModel::Ref& ref : m_model.refsAt(ea))
			if (const SrcModel::Function* f = ref.generic && ref.kind == "forwarder" && isHeader(ref.file)
												  ? m_model.function(ref.function)
												  : nullptr)
				if (auto found = bases.find(f->cls); found != bases.end())
					consider(f, found->second);
		if (forwarded.size() > 1)
		{
			std::set<std::string> named;
			for (const SrcModel::Ref& ref : m_model.refsAt(ea))
				if (ref.kind == "body" || ref.kind == "fold")
					if (const SrcModel::Function* f = m_model.function(ref.function))
						named.insert(memberName(*f));
			std::erase_if(forwarded, [&](const SrcModel::Function* f) { return !named.contains(memberName(*f)); });
		}
		return forwarded.size() == 1 ? forwarded.front() : nullptr;
	}

	// a member's own name, without its class
	static std::string memberName(const SrcModel::Function& f)
	{
		size_t scope = f.name.rfind("::");
		return scope == std::string::npos ? f.name : f.name.substr(scope + 2);
	}

	// an argument of a call after `this`: its src key past Hex-Rays' casts (nullopt: none), whether it is a number
	// literal (any integer or pointer parameter takes one), and the key of the wider integer Hex-Rays' casts extend an
	// integer to (the exe's movzx or movsx, C++'s implicit integral conversion to a parameter of that type)
	struct Argument
	{
		std::optional<std::string> key;
		bool literal = false;
		std::optional<std::string> widened;
	};

	// a parameter takes the argument as it is, or the integer the exe extends it to
	bool fitsArgument(const std::string& parameter, const Argument& a) const
	{
		return (a.key && takes(parameter, *a.key)) || (a.widened && takes(parameter, *a.widened));
	}

	// The owner of a call of the fold at ea of free functions: the member whose parameters take the arguments as they
	// are (takes: a reference parameter the pointer Hex-Rays passes; a number literal any integer, enum or pointer
	// parameter, as ofInstance's; the fixed ones of a C-variadic member) and whose return the result's use takes
	// (nullopt: any); of several, the one src registers at ea; nullptr for none.
	const SrcModel::Function* byTypes(ea_t ea, const SrcPush::Fold& fold, const std::vector<Argument>& arguments,
									  const std::optional<std::string>& use) const
	{
		std::vector<const SrcModel::Function*> fitting;
		for (const SrcModel::Function* f : fold.members)
		{
			const SrcModel::Type* type = m_model.type(f->type);
			if (!type)
				continue;
			// the hidden result pointer is the first argument, a pointer to the return type
			const SrcModel::Abi* abi = m_model.abi(f->mangled);
			bool indirect = abi && abi->ret == "indirect";
			std::vector<std::string> params;
			if (indirect)
				params.push_back("ptr(" + type->ret + ")");
			for (const std::string& p : type->params)
				if (p != "...")
					params.push_back(p);
			bool fits = (type->variadic ? arguments.size() >= params.size() : arguments.size() == params.size()) &&
						(!use || takes(*use, indirect ? "ptr(" + type->ret + ")" : type->ret));
			for (size_t k = 0; k < params.size() && fits; ++k)
			{
				const Argument& a = arguments[k];
				std::string kind = m_types.parts(m_types.canonical(params[k])).kind;
				fits = (a.literal && (kind == "ptr" || kind == "builtin" || kind == "enum")) ||
					   fitsArgument(params[k], a);
			}
			if (fits)
				fitting.push_back(f);
		}
		return chosen(ea, fitting);
	}

	// The member src registers at ea when it is an implicit class template instance's member (made from its
	// template's definition, no explicit specialization) called on its object, and src registers nothing else
	// there; nullptr for any other. The exe's other instances folded to ea (folds() does not list them).
	const SrcModel::Function* registeredInstance(ea_t ea, const SrcModel::Function* registered) const
	{
		if (!registered || registered->isStatic || !onObject(*registered) || templateOf(registered->cls).empty() ||
			!registeredAt(ea, *registered) || m_model.specializations().contains(registered->mangled))
			return nullptr;
		const SrcModel::Definition* definition = m_model.definition(registered->mangled);
		return definition && definition->instance ? registered : nullptr;
	}

	// The owner of a call of ea, where src registers the instance's member (registeredInstance) there, on a receiver
	// of the src key that is (or derives at offset 0 from, or holds at its offset 0: objectsAtZero) another instance
	// of that class template: that instance's member made from the same definition of the template (same file and
	// line, no explicit specialization), with the call's argument count, which the caller's TU can call, whose src
	// call reaches no other exe address, and whose parameters and return of other types than the registered member's
	// (those of the template's parameters) take the call's arguments and its result's use (nullopt: any) as they are;
	// the registered member itself where the receiver holds its class at offset 0 (the call is on that member object).
	// nullptr for none: why empty where the receiver is no other instance (the registered member's own class or one
	// deriving from it, or no instance of the template), else why the receiver's member is no owner.
	const SrcModel::Function* ofInstance(ea_t ea, const SrcModel::Function& registered, const std::string& receiver,
										 const std::vector<Argument>& arguments, const std::optional<std::string>& use,
										 std::string& why) const
	{
		why.clear();
		std::string record = pointee(receiver);
		if (record.empty())
			return nullptr;
		std::string templateName = templateOf(registered.cls);
		std::string instance;
		int nearest = INT_MAX;
		for (const auto& [cls, depth] : objectsAtZero(record))
			if (depth < nearest && templateOf(cls) == templateName)
				instance = cls, nearest = depth;
		bool constReceiver = m_types.constPointee(receiver);
		if (instance == registered.cls && !basesAtZero(record).contains(instance))
		{
			if (callable(&registered, constReceiver))
				return &registered;
			why = registered.name + " is not callable from the caller's TU";
			return nullptr;
		}
		if (instance.empty() || instance == registered.cls)
			return nullptr;
		std::string member = instance + "::" + registered.name.substr(registered.name.rfind("::") + 2);
		const SrcModel::Definition* own = m_model.definition(registered.mangled);
		const SrcModel::Type* theirs = m_model.type(registered.type);
		const SrcModel::Function* owner = nullptr;
		why = "src declares no " + member;
		for (const SrcModel::Function* f : m_model.functionsNamed(member))
		{
			const SrcModel::Type* type = m_model.type(f->type);
			const SrcModel::Definition* definition = m_model.definition(f->mangled);
			if (f->isStatic || !type || !theirs || type->params.size() != arguments.size() ||
				theirs->params.size() != arguments.size())
			{
				if (!owner && why.starts_with("src declares no "))
					why = member + " takes another argument count";
				continue;
			}
			if (!definition || !definition->instance || definition->file != own->file ||
				definition->line != own->line || m_model.specializations().contains(f->mangled))
				why = member + " is not made from " + registered.name + "'s definition";
			else if (!callable(f, constReceiver))
				why = member + " is not callable from the caller's TU";
			else if (reachesElsewhere(ea, *f))
				why = member + " reaches another exe address";
			else if (std::string misfit = unfitting(*type, *theirs, arguments, use); !misfit.empty())
				why = member + misfit;
			else if (owner)
				return why = member + " is overloaded", nullptr;
			else
				owner = f;
		}
		if (owner)
			why.clear();
		return owner;
	}

private:
	// Where a parameter of the owner's prototype own is of another type than the registered member's (theirs: one of
	// the template's parameters) and does not take the call's argument as it is: " takes argument N as P, ..."; the
	// same for the return against the result's use; "" where all fit
	std::string unfitting(const SrcModel::Type& own, const SrcModel::Type& theirs,
						  const std::vector<Argument>& arguments, const std::optional<std::string>& use) const
	{
		for (size_t k = 0; k < arguments.size(); ++k)
		{
			const std::string& p = own.params[k];
			if (m_types.canonical(p) == m_types.canonical(theirs.params[k]))
				continue;
			const Argument& a = arguments[k];
			std::string kind = m_types.parts(m_types.canonical(p)).kind;
			if (a.literal && (kind == "ptr" || kind == "builtin" || kind == "enum"))
				continue;
			if (!fitsArgument(p, a))
				return " takes argument " + std::to_string(k + 1) + " as " + p + ", the call passes " +
					   a.key.value_or("an untyped value");
		}
		if (use && m_types.canonical(own.ret) != m_types.canonical(theirs.ret) && !takes(*use, own.ret))
			return " returns " + own.ret + ", the call's result is used as " + *use;
		return "";
	}

	// a parameter of the key takes a value of the other as it is: the same value (sameValue), a reference parameter
	// the pointer to its referent, a pointer to const the pointer to its non-const pointee
	bool takes(const std::string& parameter, const std::string& value) const
	{
		std::string p = m_types.canonical(parameter), v = m_types.canonical(value);
		SrcTypes::Parts pp = m_types.parts(p), vp = m_types.parts(v);
		if (pp.kind == "lref" || pp.kind == "rref")
			p = m_types.canonical("ptr(" + pp.of + ")"), pp = m_types.parts(p);
		if (vp.kind == "lref" || vp.kind == "rref")
			v = m_types.canonical("ptr(" + vp.of + ")"), vp = m_types.parts(v);
		if (sameValue(p, v))
			return true;
		SrcTypes::Parts referent = pp.kind == "ptr" ? m_types.parts(pp.of) : SrcTypes::Parts{};
		return vp.kind == "ptr" && referent.kind == "const" &&
			   m_types.canonical(referent.of) == m_types.canonical(vp.of);
	}

	// The one function, else the one of them src registers at ea; nullptr for none, or for several registered there.
	const SrcModel::Function* chosen(ea_t ea, const std::vector<const SrcModel::Function*>& functions) const
	{
		if (functions.size() == 1)
			return functions.front();
		const SrcModel::Function* registered = nullptr;
		for (const SrcModel::Function* f : functions)
			if (registeredAt(ea, *f))
			{
				if (registered)
					return nullptr;
				registered = f;
			}
		return registered;
	}

	// src registers the function's body at ea
	bool registeredAt(ea_t ea, const SrcModel::Function& f) const
	{
		const std::vector<SrcModel::Ref>& refs = m_model.refsAt(ea);
		return std::any_of(refs.begin(), refs.end(),
						   [&](const SrcModel::Ref& r) { return r.function == f.mangled && r.kind == "body"; });
	}

	// src registers the function's body at ea or names it there in a fold comment
	bool namedByBodyOrFold(ea_t ea, const SrcModel::Function& f) const
	{
		const std::vector<SrcModel::Ref>& refs = m_model.refsAt(ea);
		return std::any_of(refs.begin(), refs.end(), [&](const SrcModel::Ref& r) {
			return r.function == f.mangled && (r.kind == "body" || r.kind == "fold");
		});
	}

	// a src call of the function reaches other addresses only (its body registered there, or the exe addresses its
	// definition calls through forwarders), never ea
	bool reachesElsewhere(ea_t ea, const SrcModel::Function& f) const
	{
		std::vector<ea_t> at = m_definedAt(f);
		return !at.empty() && std::find(at.begin(), at.end(), ea) == at.end();
	}

	// The two keys pass the same value: the same type, or integers (an enum as its integer) of one size: Hex-Rays'
	// signedness of a value, and its int for a number src passes as an enumerator, are its guesses.
	bool sameValue(const std::string& a, const std::string& b) const
	{
		std::string x = m_types.canonical(a), y = m_types.canonical(b);
		auto integer = [&](const std::string& key) {
			std::string kind = m_types.parts(key).kind;
			return kind == "enum" || (kind == "builtin" && key != "builtin:bool" && key != "builtin:float" &&
									  key != "builtin:double" && key != "builtin:void");
		};
		return x == y || (integer(x) && integer(y) && m_model.sizeOf(x) && m_model.sizeOf(x) == m_model.sizeOf(y));
	}

	// the record a pointer key points to ("" for any other key)
	std::string pointee(const std::string& key) const
	{
		if (key.empty())
			return "";
		SrcTypes::Parts p = m_types.parts(m_types.canonical(key));
		return p.kind == "ptr" && m_types.parts(p.of).kind == "record" ? m_types.parts(p.of).name : "";
	}

	// The caller's TU can call it on the receiver: declared in a header or the TU, a template instance's member
	// defined (its specialization or the template's body; else the call links to nothing) in a header or the TU, or
	// its specialization declared in a header (defined in another .cpp); a const member where the receiver points to
	// const.
	bool callable(const SrcModel::Function* f, bool constReceiver) const
	{
		if (!isHeader(f->file) && f->file != m_tu)
			return false;
		const SrcModel::Type* type = m_model.type(f->type);
		if (constReceiver && (!type || !type->isConst))
			return false;
		if (f->name.find('<') == std::string::npos)
			return true;
		const SrcModel::Definition* definition = m_model.definition(f->mangled);
		if (definition && (isHeader(definition->file) || definition->file == m_tu))
			return true;
		return !specializationHeader(m_model, f->mangled).empty();
	}

	// the record and its bases at offset 0, by distance
	std::map<std::string, int> basesAtZero(const std::string& record) const
	{
		return atZero(record, false);
	}

	// the objects at the record's address, by distance: the record, its bases at offset 0 and its by-value record
	// fields at offset 0 (the exe passes one pointer for all of them: a call of a member of the record's first field
	// passes the record's address unadjusted), each of theirs again
	std::map<std::string, int> objectsAtZero(const std::string& record) const
	{
		return atZero(record, true);
	}

	std::map<std::string, int> atZero(const std::string& record, bool fields) const
	{
		std::map<std::string, int> out;
		std::vector<std::string> level{record};
		for (int depth = 0; !level.empty(); ++depth)
		{
			std::vector<std::string> next;
			for (const std::string& name : level)
			{
				if (!out.emplace(name, depth).second)
					continue;
				const SrcModel::Record* r = m_model.record(name);
				if (!r)
					continue;
				for (const SrcModel::Base& b : r->bases)
					if (b.offset == 0 && !b.isVirtual)
						next.push_back(b.record);
				if (fields)
					for (const SrcModel::Field& f : r->fields)
						if (SrcTypes::Parts p = m_types.parts(m_types.canonical(f.type));
							f.offsetBits == 0 && !f.bits && p.kind == "record")
							next.push_back(p.name);
			}
			level = std::move(next);
		}
		return out;
	}

	// the class template a record is an instance of ("" for none)
	std::string templateOf(const std::string& record) const
	{
		const SrcModel::Record* r = m_model.record(record);
		return r ? r->pattern : "";
	}

	// The member of the receiver's class (or nearest object at its offset 0) that is an instance of a class template
	// some candidates are members of, all under one name, with that name and the call's argument count (`this` among
	// them), unless src's body of it is at another address than ea; nullptr for none. Candidates of other classes
	// (another template's, a class's that is no instance) are no call on this receiver: the exe folded their
	// identical code there. Of several overloads there, the one declared where the template's candidates are (the
	// same member of the template), else the last.
	const SrcModel::Function* templateMember(ea_t ea, const std::vector<const SrcModel::Function*>& candidates,
											 const std::map<std::string, int>& bases, bool constReceiver,
											 size_t arguments) const
	{
		// each template's candidates and the names they have; of a template's several names, those a body or fold
		// comment names ea by (a forwarder alone may be a call inside another member's body src registers under
		// another spelling)
		std::map<std::string, std::pair<std::set<std::string>, std::vector<const SrcModel::Function*>>> templates;
		for (const SrcModel::Function* f : candidates)
			if (std::string t = templateOf(f->cls); !t.empty())
			{
				templates[t].first.insert(f->name.substr(f->name.rfind("::") + 2));
				templates[t].second.push_back(f);
			}
		for (auto& [name, of] : templates)
		{
			if (of.first.size() < 2)
				continue;
			std::set<std::string> named;
			for (const SrcModel::Function* f : of.second)
				if (namedByBodyOrFold(ea, *f))
					named.insert(f->name.substr(f->name.rfind("::") + 2));
			if (named.size() == 1)
				of.first = std::move(named);
		}
		std::vector<const SrcModel::Function*> found;
		const std::vector<const SrcModel::Function*>* members = nullptr;
		int nearest = INT_MAX;
		for (const auto& [cls, depth] : bases)
		{
			auto of = templates.find(templateOf(cls));
			if (depth >= nearest || of == templates.end() || of->second.first.size() != 1)
				continue;
			std::vector<const SrcModel::Function*> here;
			for (const SrcModel::Function* f : m_model.functionsNamed(cls + "::" + *of->second.first.begin()))
			{
				const SrcModel::Type* type = m_model.type(f->type);
				if (!f->isStatic && type && type->params.size() + 1 == arguments && callable(f, constReceiver) &&
					!reachesElsewhere(ea, *f))
					here.push_back(f);
			}
			if (!here.empty())
				found = here, nearest = depth, members = &of->second.second;
		}
		if (found.empty())
			return nullptr;
		const SrcModel::Function* same = nullptr;
		for (const SrcModel::Function* f : found)
			if (std::any_of(members->begin(), members->end(), [&](const SrcModel::Function* c) {
					return c->file == f->file && c->line == f->line;
				}))
			{
				if (same)
					return found.back();
				same = f;
			}
		return same ? same : found.back();
	}

	const SrcModel& m_model;
	SrcTypes& m_types;
	const std::string m_tu;
	const DefinedAt m_definedAt;
};

}  // namespace

bool Unfold::foldOwners(func_t* function, cfunc_t& cfunc)
{
	const ea_t start = function->start_ea;
	const SrcModel::Function* self = declared(start);
	Owners owners(m_model, m_types, self ? m_model.classSource(self->cls) : "",
				  [this](const SrcModel::Function& f) { return definedAt(f); });
	// the member src registers at a callee that is no fold, where it is an implicit template instance's member
	// (Owners::registeredInstance) and no trivial accessor (FoldedAccessors prints those as the access), by callee
	std::map<ea_t, const SrcModel::Function*> instances;
	auto instanceAt = [&](ea_t callee) {
		auto [at, added] = instances.emplace(callee, nullptr);
		if (added && !folds().contains(callee) && !m_facts.trivialAccessor(static_cast<uint32_t>(callee)))
			at->second = owners.registeredInstance(callee, declared(callee));
		return at->second;
	};
	// each call of a fold or of such an instance's member with the type its result is used as: the left side it is
	// assigned to, the parameter it is passed as, the function's return; nullopt for any (unused, or another use)
	struct Site
	{
		const cexpr_t* call;
		std::optional<tinfo_t> use;
	};
	struct Calls : ctree_visitor_t
	{
		const std::map<ea_t, Fold>& folds;
		const std::function<const SrcModel::Function*(ea_t)> instanceAt;
		const cfunc_t& cfunc;
		std::vector<Site> sites;

		Calls(const std::map<ea_t, Fold>& f, std::function<const SrcModel::Function*(ea_t)> i, const cfunc_t& c)
			: ctree_visitor_t(CV_PARENTS), folds(f), instanceAt(std::move(i)), cfunc(c)
		{
		}

		int idaapi visit_expr(cexpr_t* e) override
		{
			// a user call type applies at the call instruction: Hex-Rays can put a call item at a jump of its
			// path (0x62E4C6 jumps to the push of 0x62E4D9's call), where one would make the jump a call
			insn_t insn;
			if (e->op != cot_call || e->x->op != cot_obj || e->ea == BADADDR || decode_insn(&insn, e->ea) <= 0 ||
				!is_call_insn(insn) ||
				(!folds.contains(e->x->obj_ea) && !instanceAt(e->x->obj_ea)))
				return 0;
			const citem_t* child = e;
			size_t k = parents.size();
			while (k > 0 && parents[k - 1]->op == cot_cast)
				child = parents[--k];
			Site site{e, std::nullopt};
			const citem_t* parent = k > 0 ? parents[k - 1] : nullptr;
			if (parent && parent->op == cot_asg && static_cast<const cexpr_t*>(parent)->y == child)
				site.use = static_cast<const cexpr_t*>(parent)->x->type;
			else if (parent && parent->op == cot_call)
			{
				const cexpr_t* outer = static_cast<const cexpr_t*>(parent);
				func_type_data_t fd;
				tinfo_t type = outer->x->type.is_ptr() ? outer->x->type.get_pointed_object() : outer->x->type;
				for (size_t n = 0; n < outer->a->size(); ++n)
					if (&(*outer->a)[n] == child && type.get_func_details(&fd) && n < fd.size())
						site.use = fd[n].type;
			}
			else if (tinfo_t own; parent && parent->op == cit_return && cfunc.get_func_type(&own))
				site.use = own.get_rettype();
			sites.push_back(site);
			return 0;
		}
	} calls(folds(), instanceAt, cfunc);
	calls.apply_to(&cfunc.body, nullptr);
	if (calls.sites.empty())
		return false;

	udcall_map_t udcalls;
	restore_user_defined_calls(&udcalls, start);
	bool changed = false;
	auto keyOf = [&](const tinfo_t& type) -> std::optional<std::string> {
		std::string why;
		return m_types.keyOf(type, why);
	};
	// a call argument as the owners read it: past Hex-Rays' casts, and the wider integer its formal (the fold's
	// prototype at the call) is where the argument is an integer Hex-Rays passes there as it is or through one cast
	// to the formal's width (the exe's movzx or movsx: C++'s implicit integral conversion to such a parameter)
	auto argumentOf = [&](const carg_t* passed) {
		const cexpr_t* argument = stripCasts(passed);
		Owners::Argument out{keyOf(argument->type), argument->op == cot_num, std::nullopt};
		const tinfo_t &from = argument->type, &to = passed->formal_type;
		bool direct = passed == argument ||
					  (passed->op == cot_cast && passed->x == argument && passed->type.get_size() == to.get_size());
		if (direct && (from.is_integral() || from.is_enum()) && to.is_integral() && !to.is_bool() &&
			to.get_size() > from.get_size())
			out.widened = keyOf(to);
		return out;
	};
	// A folded trivial accessor's 4-byte load of its `this`'s field, its 4-byte store of its one stack argument into
	// such a field (a setter), or its address of `this` plus an offset (the identity 0x6815C0 at offset 0), on a
	// receiver no owner resolves for (a template instance src never calls the member of, a class the fold has no
	// member on): the call returns the receiver's src field at the accessor's offset (through bases and by-value
	// members, `data.iCostOverride`; the first field there, as FoldedAccessors prints it), takes the value it stores as
	// that field's type, or returns that field's address (the receiver itself at offset 0), so that its result and its
	// value are not the fold's neutral `void *`: for a load a pointer field, or an integer or enum one where the fold's
	// type returns a pointer (pointerResult: the owner's type the fold has in this session, `TESObjectCELL*` for an
	// `int` the exe loads); for a store a 4-byte scalar field whose kind (pointer, integer or enum, float) the stored
	// argument has (0x71AC88's receiver Hex-Rays types `NiPlane*`, a BSSimpleList in the exe, would make its 0 a
	// float), at a call whose result nothing reads (the fold's type
	// returns void). A load or an address pops nothing, a store 4 bytes: the udcall takes `this` and the stored value.
	auto accessorResult = [&](ea_t callee, const cexpr_t* call, const tinfo_t& receiver) -> std::optional<tinfo_t> {
		std::optional<exefacts::Accessor> fact = m_facts.trivialAccessor(static_cast<uint32_t>(callee));
		std::optional<std::string> key = keyOf(receiver);
		bool pointerResult = call->type.is_ptr();
		if (!fact || fact->x87 || fact->base != -1 || fact->pops != (fact->store ? 4 : 0) || !key ||
			((fact->load || fact->store) && fact->width != 4) ||
			(fact->store && (!call->type.is_void() || call->a->size() != 2)))
			return std::nullopt;
		SrcTypes::Parts pointer = m_types.parts(m_types.canonical(*key));
		SrcTypes::Parts pointee = pointer.kind == "ptr" ? m_types.parts(pointer.of) : SrcTypes::Parts{};
		const SrcModel::Record* record = pointee.kind == "record" ? m_model.record(pointee.name) : nullptr;
		int64_t offset = fact->offset;
		const SrcModel::Field* field = nullptr;
		bool scalar = fact->load || fact->store;
		auto recordOf = [&](const SrcModel::Field& f) {
			SrcTypes::Parts p = m_types.parts(m_types.canonical(f.type));
			return p.kind == "record" ? m_model.record(p.name) : nullptr;
		};
		while (record && (scalar || offset != 0))
		{
			for (const SrcModel::Field& f : record->fields)
				if (f.offsetBits == offset * 8 && !f.bits)
				{
					field = &f;
					break;
				}
			// a load or store reads a scalar: a by-value record at the offset is entered at its start
			if (field && (!scalar || !recordOf(*field)))
				break;
			const SrcModel::Record* inner = field ? recordOf(*field) : nullptr;
			if (field)
				offset = 0;
			field = nullptr;
			for (const SrcModel::Base& b : record->bases)
				if (const SrcModel::Record* r = inner ? nullptr : m_model.record(b.record);
					r && !b.isVirtual && b.offset <= offset && offset < b.offset + r->size)
				{
					inner = r;
					offset -= b.offset;
					break;
				}
			// a scalar inside a by-value member record (`effect->data.iRange`)
			for (const SrcModel::Field& f : record->fields)
				if (const SrcModel::Record* r = inner || !scalar || f.bits ? nullptr : recordOf(f);
					r && f.offsetBits <= offset * 8 && offset * 8 < f.offsetBits + int64_t(r->size) * 8)
				{
					inner = r;
					offset -= f.offsetBits / 8;
					break;
				}
			record = inner;
		}
		std::optional<tinfo_t> result;
		if (fact->load)
		{
			result = field ? typeOf(field->type) : std::nullopt;
			if (!result || !(result->is_ptr() || (pointerResult && (result->is_integral() || result->is_enum()) &&
												  result->get_size() == 4)))
				return std::nullopt;
		}
		else if (fact->store)
		{
			std::optional<tinfo_t> value = field ? typeOf(field->type) : std::nullopt;
			const cexpr_t* argument = stripCasts(&(*call->a)[1]);
			const tinfo_t& given = argument->type;
			bool agrees = value && (value->is_floating() ? given.is_floating()
									: value->is_ptr()	 ? given.is_ptr() || argument->op == cot_num
														 : (value->is_integral() || value->is_enum()) && !given.is_floating());
			if (!value || value->get_size() != 4 || !agrees)
				return std::nullopt;
			result = tinfo_t(BT_VOID);
			func_type_data_t fd;
			fd.rettype = *result;
			fd.set_cc(CM_CC_THISCALL);
			funcarg_t self, stored;
			self.type = receiver;
			self.name = "this";
			stored.type = *value;
			stored.name = "value";
			fd.push_back(self);
			fd.push_back(stored);
			tinfo_t type;
			return type.create_func(fd) ? std::optional<tinfo_t>(type) : std::nullopt;
		}
		else if (fact->offset == 0 && record)
			result = receiver;
		else if (std::optional<tinfo_t> member = field ? typeOf(field->type) : std::nullopt)
		{
			result = tinfo_t();
			if (!result->create_ptr(*member))
				return std::nullopt;
		}
		else
			return std::nullopt;
		func_type_data_t fd;
		fd.rettype = *result;
		fd.set_cc(CM_CC_THISCALL);
		funcarg_t self;
		self.type = receiver;
		self.name = "this";
		fd.push_back(self);
		tinfo_t type;
		return type.create_func(fd) ? std::optional<tinfo_t>(type) : std::nullopt;
	};
	for (const Site& site : calls.sites)
	{
		const ea_t call = site.call->ea, callee = site.call->x->obj_ea;
		// a folded trivial accessor is typed only where it returns a pointer or stores (a setter), or where the fold's
		// type in this session returns a pointer (its owner's elsewhere, TES::GetInterior's `TESObjectCELL*`) for a
		// load: a pointer type is what loses the receivers of later calls and the setter's value, while an integer or
		// float the fold's type returns already has the exe's width, and an owner's return of another signedness or a
		// float would change the conversions Hex-Rays prints for the access
		std::optional<exefacts::Accessor> fact = m_facts.trivialAccessor(static_cast<uint32_t>(callee));
		bool accessor = fact.has_value();
		bool pointerResult = site.call->type.is_ptr();
		if (udcalls.find(call) != udcalls.end())
			continue;
		const carglist_t& args = *site.call->a;
		// a call of an instance's member src registers at the callee, on another instance of its class template: the
		// receiver's own instance's member made from the same definition, where it passes and returns the same bits
		// as the registered member's prototype
		if (const SrcModel::Function* registered = instanceAt(callee))
		{
			if (args.empty())
				continue;
			const tinfo_t& type = stripCasts(&args[0])->type;
			std::string why, detail = hex(call) + " " + hex(callee) + " on " + text(type);
			std::vector<Owners::Argument> arguments;
			for (size_t k = 1; k < args.size(); ++k)
				arguments.push_back(argumentOf(&args[k]));
			std::optional<std::string> use = site.use ? keyOf(*site.use) : std::nullopt;
			const SrcModel::Function* owner =
				owners.ofInstance(callee, *registered, keyOf(type).value_or(""), arguments, use, why);
			std::optional<tinfo_t> own = owner ? prototype(*owner, callee) : std::nullopt;
			std::optional<tinfo_t> theirs = owner ? prototype(*registered, callee) : std::nullopt;
			if (owner && (!own || !theirs))
				why = owner->name + "'s prototype unmapped";
			else if (owner && !sameBits(*own, *theirs))
				why = owner->name + "'s prototype passes other bits than " + registered->name + "'s";
			else if (owner && contradicted(m_facts, callee, *own))
				why = owner->name + "'s prototype contradicted by the exe";
			else if (owner && own->is_vararg_cc())
				why = owner->name + " is C-variadic";
			else if (owner)
			{
				udcall_t udc;
				udc.name = callName(*owner).c_str();
				udc.tif = *own;
				udcalls[call] = udc;
				changed = true;
				log("instance-owner-typed", hex(start), detail + ": " + owner->name);
				continue;
			}
			if (!why.empty())
				m_kept.emplace_back("instance-unresolved", detail + ": " + why);
			continue;
		}
		const Fold& fold = folds().at(callee);
		std::string receiver;
		const SrcModel::Function* owner = nullptr;
		bool ofFreeFunctions = std::none_of(fold.members.begin(), fold.members.end(),
											[&](const SrcModel::Function* f) { return owners.onObject(*f); });
		if (ofFreeFunctions)
		{
			std::vector<Owners::Argument> arguments;
			std::string types;
			for (const carg_t& a : args)
			{
				arguments.push_back(argumentOf(&a));
				types += (types.empty() ? "" : ", ") + text(stripCasts(&a)->type);
			}
			std::optional<std::string> use = site.use ? keyOf(*site.use) : std::nullopt;
			owner = owners.byTypes(callee, fold, arguments, use);
			receiver = "free functions (" + types + "), result as " + (site.use ? text(*site.use) : "any");
		}
		else if (!args.empty())
		{
			const tinfo_t& type = stripCasts(&args[0])->type;
			receiver = keyOf(type).value_or("");
			// a Construct's memory says nothing of the object it builds (the template's instances share one: 0x96A2D0
			// builds every BSSimpleList<T*>): the owner is the Construct of the object its result goes to, a fold
			// member's, else that of the class template instance src's template forwards to the address
			if (std::optional<std::string> built = site.use ? keyOf(*site.use) : std::nullopt)
			{
				const SrcModel::Function* f = owners.onReceiver(callee, fold, *built, args.size());
				if (!f || !owners.constructs(*f))
					f = owners.instanceConstruct(callee, *built, args.size());
				if (f)
					owner = f, receiver = *built;
			}
			if (!owner)
				owner = owners.onReceiver(callee, fold, receiver, args.size());
			receiver = owner && receiver != keyOf(type).value_or("") ? "the result's use " + text(*site.use)
																	  : text(type);
		}
		std::string detail = hex(call) + " " + hex(callee) + " on " + receiver;
		std::optional<tinfo_t> type = owner ? prototype(*owner, callee) : std::nullopt;
		if (type && accessor && !fact->store && !type->get_rettype().is_ptr() && !pointerResult)
			continue;
		if (!type)
		{
			std::optional<tinfo_t> load = !owner && !ofFreeFunctions && !args.empty()
											  ? accessorResult(callee, site.call, stripCasts(&args[0])->type)
											  : std::nullopt;
			if (qstring name; load && get_func_name(&name, callee) > 0)
			{
				udcall_t udc;
				udc.name = name;
				udc.tif = *load;
				udcalls[call] = udc;
				changed = true;
				log("fold-accessor-typed", hex(start), detail + ": " + text(*load));
				continue;
			}
			// a folded accessor no owner resolves for and without a pointer load keeps the fold's type, as intended:
			// no unresolved row
			if (owner || !accessor)
				m_kept.emplace_back("fold-unresolved",
									detail + (owner ? ": " + owner->name + "'s prototype unmapped" : ""));
			continue;
		}
		// an owner whose prototype the exe's code contradicts (a wrapper passing a literal the exe's function takes)
		// would make Hex-Rays misread the call's arguments: the fold's own prototype stays
		if (contradicted(m_facts, callee, *type))
		{
			m_kept.emplace_back("fold-unresolved", detail + ": " + owner->name + "'s prototype contradicted by the exe");
			continue;
		}
		// Hex-Rays reads no variadic argument at a user-defined call of a C-variadic type (it would drop the ones it
		// sees now); a variadic site the exe shows the arguments of has its own user-defined call already
		if (type->is_vararg_cc())
		{
			m_kept.emplace_back("fold-unresolved", detail + ": " + owner->name + " is C-variadic");
			continue;
		}
		udcall_t udc;
		udc.name = callName(*owner).c_str();
		udc.tif = *type;
		udcalls[call] = udc;
		changed = true;
		log("fold-owner-typed", hex(start), detail + ": " + owner->name);
	}
	if (changed)
		save_user_defined_calls(start, udcalls);
	return changed;
}

std::optional<Unfold::StoredType> Unfold::ownerPrototype(func_t* function)
{
	const ea_t start = function->start_ea;
	if (m_localsSession || !folds().contains(start))
		return std::nullopt;
	// the owner: the src declaration src registers or forwards there (chooseRef's), whose body this is
	const SrcModel::Function* owner = declared(start);
	std::optional<tinfo_t> type = owner ? prototype(*owner, start) : std::nullopt;
	if (!type)
	{
		log("fold-self-unresolved", hex(start), owner ? owner->name + "'s prototype unmapped" : "no owner");
		return std::nullopt;
	}
	// one the exe's code contradicts would misread the body: the fold's own (the exe's where sync found every
	// member contradicted) stays
	if (contradicted(m_facts, start, *type))
	{
		log("fold-self-unresolved", hex(start), owner->name + "'s prototype contradicted by the exe");
		return std::nullopt;
	}
	StoredType before = storedType(start);
	if (!apply_tinfo(start, *type, TINFO_DEFINITE))
	{
		log("fold-self-unresolved", hex(start), "IDA refuses " + owner->name + "'s prototype " + text(*type));
		return std::nullopt;
	}
	log("fold-self-typed", hex(start), owner->name);
	return before;
}

}  // namespace idakit
