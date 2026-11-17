#include "EmitPasses.hpp"

#include "IdaSupport.hpp"

#include <Facts.hpp>

#include <algorithm>
#include <set>

namespace idakit
{

std::vector<const cexpr_t*> ownExpressions(const cinsn_t* s)
{
	std::vector<const cexpr_t*> out;
	auto add = [&](const cexpr_t& e) {
		if (e.op != cot_empty)
			out.push_back(&e);
	};
	switch (s->op)
	{
	case cit_expr:
		add(*s->cexpr);
		break;
	case cit_if:
		add(s->cif->expr);
		break;
	case cit_for:
		add(s->cfor->init);
		add(s->cfor->expr);
		add(s->cfor->step);
		break;
	case cit_while:
		add(s->cwhile->expr);
		break;
	case cit_do:
		add(s->cdo->expr);
		break;
	case cit_switch:
		add(s->cswitch->expr);
		break;
	case cit_return:
		add(s->creturn->expr);
		break;
	default:
		break;
	}
	return out;
}

std::vector<const cinsn_t*> childStatements(const cinsn_t* s)
{
	std::vector<const cinsn_t*> out;
	switch (s->op)
	{
	case cit_block:
		for (const cinsn_t& item : *s->cblock)
			out.push_back(&item);
		break;
	case cit_if:
		out.push_back(s->cif->ithen);
		if (s->cif->ielse)
			out.push_back(s->cif->ielse);
		break;
	case cit_for:
		out.push_back(s->cfor->body);
		break;
	case cit_while:
		out.push_back(s->cwhile->body);
		break;
	case cit_do:
		out.push_back(s->cdo->body);
		break;
	case cit_switch:
		for (const ccase_t& c : s->cswitch->cases)
			out.push_back(&c);
		break;
	default:
		break;
	}
	return out;
}

tinfo_t memberType(const cexpr_t* e)
{
	tinfo_t base = e->op == cot_memptr ? e->x->type.get_pointed_object() : e->x->type;
	udm_t member;
	// a union's m is the member's index, a struct's its offset
	bool found = base.is_union() ? base.get_udm(&member, size_t(e->m)) >= 0
								 : base.get_udm_by_offset(&member, uint64(e->m) * 8) >= 0 &&
									   member.offset == uint64(e->m) * 8;
	return found ? member.type : e->type;
}

namespace
{

// the name's IDA struct type, cv dropped; "" for any other type
std::string recordName(const tinfo_t& type)
{
	qstring name;
	if (!type.is_udt() || !type.get_type_name(&name))
		return "";
	return name.c_str();
}

// every offset of a member of the name in the record, at any depth through its bases and unnamed members, each with
// whether a member of the record type `at` (the one CONTAINING_RECORD's pointer points to) starts there on its path
void namedMembers(const tinfo_t& record, const char* name, const std::string& at, uint64_t offset, bool inAt,
				  std::vector<std::pair<uint64_t, bool>>& found)
{
	udt_type_data_t members;
	if (!record.get_udt_details(&members))
		return;
	for (const udm_t& member : members)
	{
		uint64_t start = offset + member.offset / 8;
		// a member starting at the subobject's own start lies at the pointer where the subobject is at's
		bool here = (inAt && member.offset == 0) || (!at.empty() && recordName(member.type) == at);
		if (member.name == name)
			found.emplace_back(start, here);
		else if (member.is_baseclass() || member.name.empty())
			namedMembers(member.type, name, at, start, here, found);
	}
}

}  // namespace

std::optional<uint64_t> containingOffset(const cexpr_t* call)
{
	const carglist_t& args = *call->a;
	if (args.size() != 3 || args[2].op != cot_helper || !call->type.is_ptr())
		return std::nullopt;
	// the record the pointer points to: the first pointer to a struct past Hex-Rays' casts (to `char*`)
	std::string at;
	for (const cexpr_t* p = &args[0]; at.empty() && p; p = p->op == cot_cast ? p->x : nullptr)
		if (p->type.is_ptr())
			at = recordName(p->type.get_pointed_object());
	std::vector<std::pair<uint64_t, bool>> found;
	namedMembers(call->type.get_pointed_object(), args[2].helper, at, 0, false, found);
	std::set<uint64_t> offsets, atPointer;
	for (const auto& [offset, here] : found)
	{
		offsets.insert(offset);
		if (here)
			atPointer.insert(offset);
	}
	if (offsets.size() == 1)
		return *offsets.begin();
	if (atPointer.size() == 1)
		return *atPointer.begin();
	return std::nullopt;
}

std::unordered_map<const cexpr_t*, const citem_t*> parentsOf(const Function& function)
{
	std::unordered_map<const cexpr_t*, const citem_t*> parents;
	auto children = [&](const cexpr_t* e, auto&& self) -> void {
		auto child = [&](const cexpr_t* c) {
			parents[c] = e;
			self(c, self);
		};
		if (e->op == cot_call)
		{
			child(e->x);
			for (const carg_t& argument : *e->a)
				child(&argument);
			return;
		}
		if (op_uses_x(e->op) && e->x)
			child(e->x);
		if (op_uses_y(e->op) && e->y)
			child(e->y);
		if (op_uses_z(e->op) && e->z)
			child(e->z);
	};
	forEachStatement(&function.cfunc->body, [&](const cinsn_t* s) {
		for (const cexpr_t* own : ownExpressions(s))
		{
			parents[own] = s;
			children(own, children);
		}
	});
	return parents;
}

std::string srcType(const Function& function, Context& context, const cexpr_t* e)
{
	auto found = function.meaning.find(e);
	const Meaning* meaning = found == function.meaning.end() ? nullptr : &found->second;
	// a use of raw storage as the object it holds (or its address; InlinedLocals) has the type of its meaning; a use of
	// FNVD_CONSTRUCTED_LOCAL's local Hex-Rays types as bytes its meaning's, else (a use of its bytes) IDA's
	if (int v = lvarIndex(e); v >= 0 && !function.locals[v].type.empty())
	{
		const Local& local = function.locals[v];
		bool constructed = local.inlined && local.inlined->construct && !e->type.is_udt();
		if (!constructed && !(local.storage && meaning && std::holds_alternative<std::string>(*meaning)))
			return local.type;
	}
	if (const Member* m = meaning ? std::get_if<Member>(meaning) : nullptr)
		return m->field->type;
	if (const Call* c = meaning ? std::get_if<Call>(meaning) : nullptr)
		if (const SrcModel::Type* type = context.model.type(c->function->type))
			return type->ret;
	if (const std::string* key = meaning ? std::get_if<std::string>(meaning) : nullptr)
		return *key;
	// an implicit conversion's value: its operand's (a folded call's receiver without Hex-Rays' cast)
	if (meaning && std::holds_alternative<Implicit>(*meaning) && e->op == cot_cast)
		return srcType(function, context, e->x);
	// a trivial accessor's access: the field, its address (a pointer to const where the object is const, as C++'s
	// `&p->m`), the receiver (a src reference parameter as the pointer Hex-Rays has), or nothing (a store)
	if (const Access* access = meaning ? std::get_if<Access>(meaning) : nullptr)
	{
		const cexpr_t* receiver = &(*e->a)[0];
		while (receiver->op == cot_cast)
			receiver = receiver->x;
		if (!access->field)
			return receiverType(function, context, receiver);
		if (access->value)
			return "builtin:void";
		if (access->load)
			return access->field->type;
		// the object's pointer: through the members a receiver `&p->Base.m` names, p
		const cexpr_t* object = receiver;
		if (object->op == cot_ref)
		{
			const cexpr_t* member = object->x;
			while (member->op == cot_memref)
				member = member->x;
			object = member->op == cot_memptr ? member->x : object;
		}
		bool constant = context.types.constPointee(receiverType(function, context, object)) &&
						!context.types.bare(access->field->type).starts_with("const(");
		return constant ? "ptr(const(" + access->field->type + "))" : "ptr(" + access->field->type + ")";
	}
	if (const SrcModel::Ref* const* ref = meaning ? std::get_if<const SrcModel::Ref*>(meaning) : nullptr)
		return (*ref)->type;
	// an idiom's form of a type its pass knows (an accessor's call)
	if (const Rewrite* rewrite = meaning ? std::get_if<Rewrite>(meaning) : nullptr; rewrite && !rewrite->type.empty())
		return rewrite->type;
	std::string why;
	return context.types.keyOf(e->type, why).value_or("");
}

std::string pointedType(const Function& function, Context& context, const cexpr_t* e)
{
	e = stripCasts(e);
	if (e->op == cot_ref)
		return objectType(function, context, e->x);
	SrcTypes::Parts parts = context.types.parts(context.types.canonical(objectType(function, context, e)));
	return parts.kind == "ptr" || parts.kind == "array" ? parts.of : "";
}

std::string objectType(const Function& function, Context& context, const cexpr_t* e)
{
	if (std::string key = srcType(function, context, e); !key.empty())
		return key;
	if (e->op == cot_idx || e->op == cot_ptr)
		return pointedType(function, context, e->x);
	return "";
}

bool comparedSentinel(SrcTypes& types, const cexpr_t* cast,
					  const std::unordered_map<const cexpr_t*, const citem_t*>& parents)
{
	if (cast->op != cot_cast || !cast->type.is_ptr() || cast->x->op != cot_num || cast->x->type.is_ptr())
		return false;
	size_t size = cast->x->type.get_size();
	if (size == 0 || size > 8)
		return false;
	uint64_t value = cast->x->n->_value;
	int64_t extended = static_cast<int64_t>(value << (64 - size * 8)) >> (64 - size * 8);
	if (extended != -1 && (extended < 1 || extended > 0xFFFF))
		return false;
	auto parent = parents.find(cast);
	if (parent == parents.end() || (parent->second->op != cot_eq && parent->second->op != cot_ne))
		return false;
	const auto* compare = static_cast<const cexpr_t*>(parent->second);
	const cexpr_t* other = compare->x == cast ? compare->y : compare->x;
	std::string why;
	std::optional<std::string> a = types.keyOf(other->type, why), b = types.keyOf(cast->type, why);
	return other->type.is_ptr() && a && b &&
		   (types.canonical(*a) == types.canonical(*b) || cvApart(types, *a, *b));
}

bool srcConstObject(const Function& function, Context& context, const cexpr_t* lvalue)
{
	// a Hex-Rays cast that only drops the const prints as its operand (Const's castAway)
	auto constPointer = [&](const cexpr_t* e) {
		while (e->op == cot_cast && dropsConst(context.types, e))
			e = e->x;
		std::string key = srcType(function, context, e);
		std::string kind = key.empty() ? "" : context.types.parts(context.types.canonical(key)).kind;
		return (kind == "ptr" || kind == "lref") && context.types.constPointee(key);
	};
	switch (lvalue->op)
	{
	case cot_memptr:
	case cot_ptr:
		return constPointer(lvalue->x);
	case cot_memref:
		return srcConstObject(function, context, lvalue->x);
	case cot_idx:
		return lvalue->x->type.is_ptr() ? constPointer(lvalue->x) : srcConstObject(function, context, lvalue->x);
	case cot_var:
	{
		const Local& local = function.locals[lvalue->v.idx];
		return local.reference && context.types.constPointee(local.type);
	}
	default:
		return false;
	}
}

const cexpr_t* objectOf(const Function& function, const cexpr_t* e)
{
	for (;;)
	{
		auto found = function.meaning.find(e);
		if (found == function.meaning.end())
			return e;
		bool base = (e->op == cot_memptr || e->op == cot_memref) && std::holds_alternative<std::string>(found->second);
		if (!base && !std::holds_alternative<Implicit>(found->second))
			return e;
		e = e->x;
	}
}

bool isThis(const Function& function, const cexpr_t* e)
{
	if (!function.virtualBase.empty())
	{
		auto found = adjustedThis(function, e) ? function.meaning.find(e) : function.meaning.end();
		const Rewrite* rewrite = found == function.meaning.end() ? nullptr : std::get_if<Rewrite>(&found->second);
		return rewrite && rewrite->form == "this";
	}
	int v = lvarIndex(e);
	return v >= 0 && function.locals[v].name == "this" && function.cfunc->get_lvars()->at(v).is_arg_var();
}

bool adjustedThis(const Function& function, const cexpr_t* e)
{
	return e->op == cot_call && e->x->op == cot_helper && std::string_view(e->x->helper) == "ADJ" &&
		   e->a->size() == 1 && !function.cfunc->argidx.empty() && lvarIndex(&(*e->a)[0]) == function.cfunc->argidx[0];
}

bool structorObject(const Function& function, int index)
{
	if (function.structor.empty() || function.cfunc->argidx.empty() || index != function.cfunc->argidx[0])
		return false;
	bool rebound = false;
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
		rebound |= (written(e) || e->op == cot_ref) && lvarIndex(e->x) == index;
	});
	return !rebound;
}

std::string receiverType(const Function& function, Context& context, const cexpr_t* receiver)
{
	std::string key = srcType(function, context, receiver);
	SrcTypes::Parts p = context.types.parts(context.types.bare(key));
	return p.kind == "lref" ? "ptr(" + p.of + ")" : key;
}

bool variableObject(const Function& function, const cexpr_t* e)
{
	int v = lvarIndex(e);
	return e->op == cot_obj || (v >= 0 && !function.locals[v].reference);
}

// r declares a field of the name, its anonymous members' included
static bool declaresField(Context& context, const SrcModel::Record& r, const std::string& name)
{
	for (const SrcModel::Field& f : r.fields)
	{
		if (f.name == name)
			return true;
		const SrcModel::Type* type = f.name.empty() ? context.model.type(context.types.canonical(f.type)) : nullptr;
		const SrcModel::Record* inner = type && type->kind == "record" ? context.model.record(type->name) : nullptr;
		if (inner && unnamedRecord(inner->name) && declaresField(context, *inner, name))
			return true;
	}
	return false;
}

static void nameLookup(Context& context, const std::string& record, const std::string& name,
					   std::vector<std::string>& classes)
{
	const SrcModel::Record* r = context.model.record(record);
	if (!r)
		return;
	if (declaresField(context, *r, name) || !context.model.functionsNamed(record + "::" + name).empty())
	{
		classes.push_back(record);
		return;
	}
	for (const SrcModel::Base& b : r->bases)
		nameLookup(context, b.record, name, classes);
}

std::vector<std::string> nameLookup(Context& context, const std::string& record, const std::string& name)
{
	std::vector<std::string> classes;
	nameLookup(context, record, name, classes);
	return classes;
}

// the condition tests the local nonzero (nonzero) or zero wherever it is true: `p`, `p != 0`, `!p`, `p == 0`, a
// conjunction one of whose operands tests it nonzero, a disjunction one of whose operands tests it zero
static bool testsLocal(const cexpr_t* c, int v, bool nonzero)
{
	if (lvarIndex(c) == v)
		return nonzero;
	if ((c->op == cot_ne || c->op == cot_eq) && c->y->op == cot_num && c->y->numval() == 0 && lvarIndex(c->x) == v)
		return (c->op == cot_ne) == nonzero;
	if (c->op == cot_lnot)
		return testsLocal(c->x, v, !nonzero);
	if (c->op == (nonzero ? cot_land : cot_lor))
		return testsLocal(c->x, v, nonzero) || testsLocal(c->y, v, nonzero);
	return false;
}

// the expression writes the local or takes its address
static bool writesLocal(const cexpr_t* e, int v)
{
	bool writes = false;
	forEachSubexpression(e, [&](const cexpr_t* x) {
		writes |= (written(x) || x->op == cot_ref) && lvarIndex(x->x) == v;
	});
	return writes;
}

// the statements from s down to the one holding target, s first (empty where s does not hold it)
static bool pathTo(const cinsn_t* s, const cinsn_t* target, std::vector<const cinsn_t*>& path)
{
	path.push_back(s);
	if (s == target)
		return true;
	for (const cinsn_t* child : childStatements(s))
		if (pathTo(child, target, path))
			return true;
	path.pop_back();
	return false;
}

// the local is tested nonzero on every path to the expression at, and not written between the test and at (see
// neverNull)
static bool testedBefore(const Function& function, const std::unordered_map<const cexpr_t*, const citem_t*>& parents,
						 const cexpr_t* at, int v)
{
	bool addressed = false;
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
		addressed |= e->op == cot_ref && lvarIndex(e->x) == v;
	});
	if (addressed)
		return false;
	// `p && ...`, `!p || ...` around it
	const cexpr_t* c = at;
	const citem_t* statement = nullptr;
	for (;;)
	{
		auto up = parents.find(c);
		if (up == parents.end())
			return false;
		if (!up->second->is_expr())
		{
			statement = up->second;
			break;
		}
		const cexpr_t* q = static_cast<const cexpr_t*>(up->second);
		if (q->y == c && ((q->op == cot_land && testsLocal(q->x, v, true)) ||
						  (q->op == cot_lor && testsLocal(q->x, v, false))))
			return !writesLocal(q->x, v) && !writesLocal(q->y, v);
		c = q;
	}
	// an enclosing if's branch: the innermost such if whose condition tests it, its condition and branch not writing it
	std::vector<const cinsn_t*> path;
	if (!pathTo(&function.cfunc->body, static_cast<const cinsn_t*>(statement), path))
		return false;
	for (size_t k = path.size() - 1; k-- > 0;)
	{
		const cinsn_t* s = path[k];
		const cinsn_t* branch = path[k + 1];
		if (s->op != cit_if || !testsLocal(&s->cif->expr, v, branch == s->cif->ithen))
			continue;
		if (writesLocal(&s->cif->expr, v))
			return false;
		bool writes = false, labelled = false;
		forEachStatement(branch, [&](const cinsn_t* inner) {
			labelled |= inner->label_num >= 0;
			for (const cexpr_t* own : ownExpressions(inner))
				writes |= writesLocal(own, v);
		});
		return !writes && !labelled;
	}
	return false;
}

// the local is assigned only copies of the _VIA virtual's Hex-Rays this (`v1 = this`, through Hex-Rays' casts, or
// of other such locals, `v11 = v2; ... v2 = v11;`), at least once, is not possibly undefined (so a cycle of copies
// holds what enters it) and its address is never taken: wherever it is read, it holds that pointer. visiting: the
// locals whose writes are being judged, each taken as such a copy
static bool copiesThis(const Function& function, int v, std::vector<int>& visiting)
{
	if (std::find(visiting.begin(), visiting.end(), v) != visiting.end())
		return true;
	if (v < 0 || function.cfunc->get_lvars()->at(v).is_arg_var() || function.locals[v].undefined)
		return false;
	visiting.push_back(v);
	int thisIndex = function.cfunc->argidx[0];
	bool copies = true, wrote = false;
	forEachExpression(function, [&](const cexpr_t* e, const cinsn_t*) {
		if (!copies || !(written(e) || e->op == cot_ref) || lvarIndex(e->x) != v)
			return;
		const cexpr_t* value = e->op == cot_asg ? e->y : nullptr;
		while (value && value->op == cot_cast)
			value = value->x;
		int from = value ? lvarIndex(value) : -1;
		copies = from >= 0 && (from == thisIndex || copiesThis(function, from, visiting));
		wrote = true;
	});
	visiting.pop_back();
	return copies && wrote;
}

bool viaThis(const Function& function, const cexpr_t* e)
{
	int v = lvarIndex(e);
	for (int depth = 0; v >= 0 && function.locals[v].aliasOf >= 0 && depth < 16; ++depth)
		v = function.locals[v].aliasOf;
	if (v < 0 || function.virtualBase.empty() || function.cfunc->argidx.empty() ||
		function.locals[function.cfunc->argidx[0]].name != "this")
		return false;
	std::vector<int> visiting;
	return v == function.cfunc->argidx[0] || copiesThis(function, v, visiting);
}

bool neverNull(const Function& function, const std::unordered_map<const cexpr_t*, const citem_t*>& parents,
			   const cexpr_t* pointer, const citem_t* at)
{
	int v = lvarIndex(pointer);
	if (isThis(function, pointer) || viaThis(function, pointer) || structorObject(function, v) ||
		(v >= 0 && function.locals[v].reference))
		return true;
	if (pointer->op == cot_ref)
		return variableObject(function, pointer->x);
	return v >= 0 && at->is_expr() && testedBefore(function, parents, static_cast<const cexpr_t*>(at), v);
}

void memberLookup(const SrcModel& model, const std::string& cls, const std::string& name,
				  std::vector<std::string>& found)
{
	const SrcModel::Record* record = model.record(cls);
	if (!model.functionsNamed(cls + "::" + name).empty() || (record && record->field(name)))
	{
		found.push_back(cls);
		return;
	}
	if (record)
		for (const SrcModel::Base& base : record->bases)
			memberLookup(model, base.record, name, found);
}

tinfo_t valueType(const cexpr_t* e)
{
	if (e->op != cot_call || !e->type.is_integral() || e->type.is_bool() || e->type.is_enum())
		return e->type;
	tinfo_t called = e->x->type;
	if (called.is_ptr())
		called = called.get_pointed_object();
	func_type_data_t fd;
	if (!called.get_func_details(&fd) || !fd.rettype.is_ptr() || fd.rettype.get_size() != e->type.get_size())
		return e->type;
	return fd.rettype;
}

Destination destinationOf(const Function& function, Context& context,
						  const std::unordered_map<const cexpr_t*, const citem_t*>& parents, const cexpr_t* value)
{
	auto up = parents.find(value);
	if (up == parents.end() || !up->second)
		return {};
	const citem_t* parent = up->second;
	if (parent->op == cit_return)
		return {function.returnsReference ? "" : function.returnType};
	if (parent->op >= cit_empty)
		return {};
	const cexpr_t* p = static_cast<const cexpr_t*>(parent);
	if (p->op == cot_asg && p->y == value)
	{
		if (int v = lvarIndex(p->x); v >= 0)
		{
			const Local& local = function.locals[v];
			return {local.reference || local.storage ? "" : local.type};
		}
		auto found = function.meaning.find(p->x);
		if (found == function.meaning.end())
			return {};
		if (const Member* member = std::get_if<Member>(&found->second))
			return {member->field->type};
		if (const SrcModel::Ref* const* ref = std::get_if<const SrcModel::Ref*>(&found->second))
			return {(*ref)->type};
		return {};
	}
	if (p->op != cot_call)
		return {};
	auto found = function.meaning.find(p);
	const Call* call = found == function.meaning.end() ? nullptr : std::get_if<Call>(&found->second);
	const SrcModel::Type* type = call ? context.model.type(call->function->type) : nullptr;
	for (size_t k = 0; type && k < p->a->size(); ++k)
	{
		if (&(*p->a)[k] != value)
			continue;
		int parameter = parameterIndex(*call, k);
		if (parameter < 0 || static_cast<size_t>(parameter) >= type->params.size())
			return {};
		return {type->params[parameter], k < call->byReference.size() && call->byReference[k]};
	}
	return {};
}

bool hostWidth(const Context& context, const std::string& key)
{
	static const std::set<std::string> names = {"size_t", "ptrdiff_t", "intptr_t", "uintptr_t", "ssize_t"};
	std::string k = key;
	for (int depth = 0; depth < 16; ++depth)
	{
		SrcTypes::Parts p = context.types.parts(k);
		if (p.kind == "const" || p.kind == "volatile")
		{
			k = p.of;
			continue;
		}
		if (k == "builtin:long" || k == "builtin:unsigned long")
			return true;
		if (!k.starts_with("typedef:"))
			return false;
		std::string name = k.substr(8);
		const SrcModel::Typedef* t = context.model.typedefNamed(name);
		if (names.count(name) || !t)
			return names.count(name) > 0;
		k = t->target;
	}
	return true;
}

bool integerOperator(ctype_t op)
{
	switch (op)
	{
	case cot_bor: case cot_xor: case cot_band: case cot_add: case cot_sub: case cot_mul: case cot_sdiv:
	case cot_udiv: case cot_smod: case cot_umod: case cot_shl: case cot_sshr: case cot_ushr: case cot_bnot:
	case cot_neg:
		return true;
	default:
		return false;
	}
}

// an integer or unscoped enum key's integral promotion (4 bytes at most): builtin:int, builtin:unsigned int; "" for
// any other (a scoped enum, a float, a pointer, a wider integer, one whose width differs on x64)
static std::string promoted(const Context& context, const std::string& key)
{
	if (key.empty() || hostWidth(context, key))
		return "";
	std::string k = context.types.canonical(key);
	SrcTypes::Parts p = context.types.parts(k);
	if (p.kind == "const" || p.kind == "volatile")
		return promoted(context, p.of);
	if (p.kind == "enum")
	{
		const SrcModel::Enum* e = context.model.enumeration(p.name);
		return e && !e->scoped ? promoted(context, e->underlying.empty() ? "builtin:int" : e->underlying) : "";
	}
	const SrcModel::Type* type = p.kind == "builtin" ? context.model.type(k) : nullptr;
	std::optional<int64_t> size = context.model.sizeOf(k);
	if (!type || type->isFloat || p.name == "void" || !size || *size > 4)
		return "";
	return *size == 4 && k == "builtin:unsigned int" ? k : "builtin:int";
}

std::string promotedResult(const Context& context, ctype_t op, const std::string& x, const std::string& y)
{
	std::string a = promoted(context, x);
	if (op == cot_bnot || op == cot_neg || op == cot_shl || op == cot_sshr || op == cot_ushr)
		return a;
	std::string b = promoted(context, y);
	if (a.empty() || b.empty())
		return "";
	return a == "builtin:unsigned int" || b == "builtin:unsigned int" ? "builtin:unsigned int" : "builtin:int";
}

// the key with its typedefs resolved at every level and its cv kept (a builtin as it is: `long` is no `int` to C++)
static std::string resolved(const SrcTypes& types, const std::string& key)
{
	std::string bare = types.bare(key);
	SrcTypes::Parts p = types.parts(bare);
	if (p.kind == "const" || p.kind == "volatile" || p.kind == "ptr" || p.kind == "lref" || p.kind == "rref")
		return p.kind + "(" + resolved(types, p.of) + ")";
	if (p.kind == "array")
		return "array(" + resolved(types, p.of) + "," + std::to_string(p.count) + ")";
	return p.kind == "builtin" ? bare : types.canonical(bare);
}

std::optional<PointeeCv> pointeeOf(const SrcTypes& types, const std::string& key)
{
	SrcTypes::Parts p = types.parts(types.bare(key));
	if (key.empty() || (p.kind != "ptr" && p.kind != "lref"))
		return std::nullopt;
	PointeeCv out;
	std::string of = types.bare(p.of);
	for (SrcTypes::Parts q = types.parts(of); q.kind == "const" || q.kind == "volatile"; q = types.parts(of))
	{
		(q.kind == "const" ? out.isConst : out.isVolatile) = true;
		of = types.bare(q.of);
	}
	out.type = resolved(types, of);
	return out;
}

bool cvApart(const SrcTypes& types, const std::string& from, const std::string& to)
{
	std::optional<PointeeCv> a = pointeeOf(types, from), b = pointeeOf(types, to);
	return a && b && a->type == b->type && (b->isVolatile || !a->isVolatile);
}

bool dropsConst(SrcTypes& types, const cexpr_t* cast)
{
	if (cast->op != cot_cast || !cast->type.is_ptr() || !cast->x->type.is_ptr())
		return false;
	std::string why;
	std::optional<std::string> from = types.keyOf(cast->x->type, why), to = types.keyOf(cast->type, why);
	std::optional<PointeeCv> a = from ? pointeeOf(types, *from) : std::nullopt;
	std::optional<PointeeCv> b = to ? pointeeOf(types, *to) : std::nullopt;
	return a && b && cvApart(types, *from, *to) && a->isConst && !b->isConst;
}

std::optional<AccessorCall> accessorCall(Context& context, const cexpr_t* call)
{
	if (call->op != cot_call || call->x->op != cot_obj)
		return std::nullopt;
	std::optional<exefacts::Accessor> fact = context.exe.trivialAccessor(static_cast<uint32_t>(call->x->obj_ea));
	if (!fact)
		return std::nullopt;
	bool thiscall = call->x->type.get_cc() == CM_CC_THISCALL;
	auto argumentOf = [&](int32_t slot) {
		return slot < 0 ? size_t(0) : static_cast<size_t>(slot) + (thiscall ? 1 : 0);
	};
	size_t argument = argumentOf(fact->base), value = argumentOf(fact->value);
	if ((fact->base < 0 && !thiscall) || argument >= call->a->size() || (fact->store && value >= call->a->size()))
		return std::nullopt;
	return AccessorCall{&(*call->a)[argument], fact->offset, fact->load, fact->width, fact->extension, fact->x87,
						fact->store ? &(*call->a)[value] : nullptr};
}

}  // namespace idakit

