// Adjusted null tests: Hex-Rays folds the exe's null test of an adjusted pointer (`lea eax, [ecx+N]; test eax, eax`:
// the address of a member or base subobject, which an inlined callee or loop tests, as MSVC tests a pointer it does
// not know is non-null) into a compare of the object's pointer with the negated offset, `p == (T*)-N`. That is the
// test of the address src names: `&p->m == nullptr` for the member at N (the outermost one starting there, through
// bases and members that hold it; named alone where p is C++'s this and no local hides it; `&r.m` of a reference r),
// `static_cast<B*>(p) == nullptr` for the base subobject at N, directly or through bases, where p's class has one
// such base (`!=` alike). The base's static_cast tests p itself, which agrees with the exe's test only where p is
// never null (neverNull: then neither is null), so only there; the member's address is computed whatever p is, as the
// exe's. Not a -1 (all bits set: a sentinel such as INVALID_HANDLE_VALUE, no address), nor an offset no member or
// base of p's src record starts at, nor a name lookup from the record that finds another field. A _VIA virtual's
// Hex-Rays this (sync's `B *__shifted(D, N)`) compared with N itself is the exe's test of the object: `this ==
// nullptr`.

#include "EmitPasses.hpp"

namespace idakit
{

namespace
{

class AdjustedNullTests
{
public:
	AdjustedNullTests(Function& function, Context& context)
		: m_function(function), m_context(context), m_parents(parentsOf(function))
	{
	}

	void run()
	{
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			if (e->op == cot_eq || e->op == cot_ne)
				rewrite(e);
		});
	}

private:
	// what is at the offset: a base subobject (base set), or a member path (`a.b`)
	struct At
	{
		std::string base;
		std::string path;
	};

	void rewrite(const cexpr_t* test)
	{
		// the number: Hex-Rays' cast of one to the pointer's type, or one of that type itself
		auto numberOf = [](const cexpr_t* e) -> const cexpr_t* {
			if (!e->type.is_ptr())
				return nullptr;
			if (e->op == cot_cast && e->x->op == cot_num)
				return e->x;
			return e->op == cot_num ? e : nullptr;
		};
		const cexpr_t* number = test->y;
		const cexpr_t* pointer = test->x;
		if (!numberOf(number))
			std::swap(number, pointer);
		if (!numberOf(number) || !pointer->type.is_ptr())
			return;
		int32_t value = static_cast<int32_t>(static_cast<uint32_t>(numberOf(number)->numval()));
		std::string compare = test->op == cot_eq ? " == nullptr" : " != nullptr";
		// a _VIA virtual's this, the base subobject at N, compared with N: the test of the object, C++'s this
		ptr_type_data_t shifted;
		if (value > 0 && viaThis(m_function, pointer) && pointer->type.get_ptr_details(&shifted) &&
			(shifted.taptr_bits & TAPTR_SHIFTED) && shifted.delta == value)
		{
			forEachSubexpression(test, [&](const cexpr_t* e) {
				if (e != test)
					m_function.meaning.erase(e);
			});
			m_function.meaning[test] = Rewrite{1, "this" + compare, {}, {}, "builtin:bool"};
			return;
		}
		int64_t offset = -static_cast<int64_t>(value);
		if (offset <= 1)
			return;
		while (pointer->op == cot_cast && pointer->x->type.is_ptr() && !m_function.meaning.count(pointer))
			pointer = pointer->x;
		if (unresolvedIn(m_function, pointer))
			return;
		SrcTypes& types = m_context.types;
		SrcTypes::Parts p = types.parts(types.canonical(srcType(m_function, m_context, pointer)));
		// (a src reference Hex-Rays has as a pointer: the object it refers to)
		bool reference = p.kind == "lref";
		if (p.kind != "ptr" && !reference)
			return;
		SrcTypes::Parts pointee = types.parts(types.canonical(p.of));
		bool isConst = pointee.kind == "const";
		if (isConst)
			pointee = types.parts(types.canonical(pointee.of));
		if (pointee.kind != "record")
			return;
		std::optional<At> at = find(pointee.name, offset, 0);
		if (!at)
			return;
		std::string form;
		if (!at->base.empty())
		{
			std::optional<std::string> spelled = types.spell("record:" + at->base);
			if (!spelled || bases(pointee.name, at->base, 0) != 1 ||
				(!reference && !neverNull(m_function, m_parents, pointer, test)))
				return;
			std::string to = isConst ? "const " + *spelled : *spelled;
			form = "static_cast<" + to + "*>(" + (reference || pointer->op == cot_ref ? "&*$0" : "$0") + ")";
		}
		else
		{
			std::string outer = at->path.substr(0, at->path.find('.'));
			if (fieldsNamed(pointee.name, outer, 0) != 1)
				return;
			bool alone = isThis(m_function, pointer) &&
						 std::none_of(m_function.locals.begin(), m_function.locals.end(),
									  [&](const Local& local) { return local.name == outer; });
			std::string object = reference || pointer->op == cot_ref ? "*$0." : "$0->";
			form = "&" + (alone ? std::string() : object) + at->path;
		}
		forEachSubexpression(test, [&](const cexpr_t* e) {
			if (e != test && !insideOf(e, pointer))
				m_function.meaning.erase(e);
		});
		m_function.meaning[test] = Rewrite{1, form + compare, {pointer}, {}, "builtin:bool"};
	}

	// the outermost base subobject or member of the record starting at the offset (> 0): a base that starts there,
	// a field that does, else the base or record field holding it, looked into
	std::optional<At> find(const std::string& record, int64_t offset, int depth) const
	{
		const SrcModel::Record* r = depth < 16 ? m_context.model.record(record) : nullptr;
		if (!r)
			return std::nullopt;
		for (const SrcModel::Base& b : r->bases)
			if (!b.isVirtual && b.offset == offset)
				return At{b.record, ""};
		for (const SrcModel::Field& f : r->fields)
			if (!f.bits && f.offsetBits == 8 * offset && !f.name.empty())
				return At{"", f.name};
		for (const SrcModel::Base& b : r->bases)
		{
			const SrcModel::Record* inner = b.isVirtual ? nullptr : m_context.model.record(b.record);
			if (inner && b.offset < offset && offset < b.offset + inner->size)
				if (std::optional<At> found = find(b.record, offset - b.offset, depth + 1))
					return found;
		}
		for (const SrcModel::Field& f : r->fields)
		{
			SrcTypes::Parts t = m_context.types.parts(m_context.types.canonical(f.type));
			const SrcModel::Record* inner = t.kind == "record" && !f.bits ? m_context.model.record(t.name) : nullptr;
			int64_t start = f.offsetBits / 8;
			if (inner && !f.name.empty() && start < offset && offset < start + inner->size)
				if (std::optional<At> found = find(t.name, offset - start, depth + 1); found && found->base.empty())
					return At{"", f.name + "." + found->path};
		}
		return std::nullopt;
	}

	// the fields of the name in the record and its non-virtual bases (name lookup finds one only where this is 1)
	int fieldsNamed(const std::string& record, const std::string& name, int depth) const
	{
		const SrcModel::Record* r = depth < 16 ? m_context.model.record(record) : nullptr;
		if (!r)
			return 0;
		if (r->field(name))
			return 1;
		int count = 0;
		for (const SrcModel::Base& b : r->bases)
			if (!b.isVirtual)
				count += fieldsNamed(b.record, name, depth + 1);
		return count;
	}

	// the base subobjects of the class in the record (non-virtual, directly or through bases): a static_cast to it is
	// unambiguous only where this is 1
	int bases(const std::string& record, const std::string& base, int depth) const
	{
		const SrcModel::Record* r = depth < 16 ? m_context.model.record(record) : nullptr;
		if (!r)
			return 0;
		int count = 0;
		for (const SrcModel::Base& b : r->bases)
			if (!b.isVirtual)
				count += (b.record == base) + bases(b.record, base, depth + 1);
		return count;
	}

	// e is the pointer item or inside it
	static bool insideOf(const cexpr_t* e, const cexpr_t* pointer)
	{
		bool found = false;
		forEachSubexpression(pointer, [&](const cexpr_t* sub) { found |= sub == e; });
		return found;
	}

	Function& m_function;
	Context& m_context;
	std::unordered_map<const cexpr_t*, const citem_t*> m_parents;
};

}  // namespace

void rewriteAdjustedNullTests(Function& function, Context& context)
{
	AdjustedNullTests(function, context).run();
}

}  // namespace idakit
