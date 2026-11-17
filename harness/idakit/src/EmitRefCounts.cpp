#include "EmitRefCounts.hpp"

namespace idakit
{

bool RefCounts::importCall(const cexpr_t* e, const char* name, const Value& at) const
{
	if (e->op != cot_call || e->x->op != cot_obj || e->a->size() != 1)
		return false;
	qstring callee;
	get_name(&callee, e->x->obj_ea);
	return callee == name && is(&e->a->front(), at);
}

bool RefCounts::deleteThis(const cinsn_t* s, const Value& old) const
{
	if (!s || s->op != cit_expr || s->cexpr->op != cot_call || s->cexpr->a->size() != 1)
		return false;
	const cexpr_t* call = s->cexpr;
	return is(call->x, old.load().plus(4).load()) && is(&call->a->front(), old);
}

bool RefCounts::decrement(const cexpr_t* test, const cinsn_t* then, const Value& old, const Value& receiver) const
{
	return test->op == cot_lnot && importCall(test->x, "InterlockedDecrement", old.plus(4)) &&
		   deleteThis(soleStatement(then), receiver);
}

int RefCounts::copyOf(const cinsn_t* s, const Value& old) const
{
	if (!s || s->op != cit_expr || s->cexpr->op != cot_asg)
		return -1;
	int t = lvarIndex(s->cexpr->x);
	return t >= 0 && is(s->cexpr->y, old) ? t : -1;
}

bool RefCounts::release(const cinsn_t* s, const Value& old, int* temporary, int copy) const
{
	if (!s || s->op != cit_if || s->cif->ielse)
		return false;
	Value receiver = copy >= 0 ? Value::local(copy) : old;
	const cexpr_t* test = &s->cif->expr;
	if (test->op == cot_land)
		return is(test->x, old) && decrement(test->y, s->cif->ithen, old, receiver);
	if (!is(test, old))
		return false;
	const cinsn_t* inner = soleStatement(s->cif->ithen);
	// `t = old;` first: Hex-Rays' copy DeleteThis is called on
	const cinsn_t* then = s->cif->ithen;
	if (!inner && temporary && copy < 0 && then->op == cit_block && then->cblock->size() == 2)
	{
		int t = copyOf(&then->cblock->front(), old);
		if (t < 0)
			return false;
		*temporary = t;
		receiver = Value::local(t);
		inner = &then->cblock->back();
	}
	return inner && inner->op == cit_if && !inner->cif->ielse &&
		   decrement(&inner->cif->expr, inner->cif->ithen, old, receiver);
}

bool RefCounts::hold(const cinsn_t* s, const Value& p, const std::optional<Value>& same) const
{
	auto either = [&](const cexpr_t* e, int64_t plus) {
		std::optional<Value> v = value(e);
		return v && (*v == p.plus(plus) || (same && *v == same->plus(plus)));
	};
	if (!s || s->op != cit_if || s->cif->ielse || !either(&s->cif->expr, 0))
		return false;
	const cinsn_t* call = soleStatement(s->cif->ithen);
	return call && call->op == cit_expr &&
		   (importCall(call->cexpr, "InterlockedIncrement", p.plus(4)) ||
			(same && importCall(call->cexpr, "InterlockedIncrement", same->plus(4))));
}

// --- steps ------------------------------------------------------------------------------------------------------

namespace
{

Step atom(std::string text)
{
	return Step{"", std::move(text), {}};
}

Step node(std::string op, std::vector<Step> kids, std::string text = "")
{
	return Step{std::move(op), std::move(text), std::move(kids)};
}

Step number(int64_t n)
{
	return atom("#" + std::to_string(n));
}

bool isNumber(const Step& s, int64_t n)
{
	return s.op.empty() && s.atom == "#" + std::to_string(n);
}

bool boolean(const Step& s)
{
	return s.op == "eq" || s.op == "ne" || s.op == "and" || s.op == "or" || s.op == "not";
}

bool counter(const std::string& op)
{
	return op == "pre+" || op == "pre-" || op == "post+" || op == "post-";
}

// a step that writes memory or calls (a load after it reads memory as it is then)
bool effect(const Step& s)
{
	return s.op == "set" || s.op == "call" || s.op == "vcall" || s.op == "icall" || s.op == "atomic" ||
		   counter(s.op);
}

// s or a step inside it writes or calls
bool writes(const Step& s)
{
	return effect(s) || std::any_of(s.kids.begin(), s.kids.end(), writes);
}

class Parser
{
public:
	explicit Parser(const std::string& text) : m_text(text) {}

	std::optional<Step> all()
	{
		std::optional<Step> s = step();
		skip();
		return s && m_at == m_text.size() ? s : std::nullopt;
	}

private:
	void skip()
	{
		while (m_at < m_text.size() && m_text[m_at] == ' ')
			++m_at;
	}

	std::optional<Step> step()
	{
		skip();
		if (m_at >= m_text.size())
			return std::nullopt;
		if (m_text[m_at] == '"')
		{
			std::string text;
			for (++m_at; m_at < m_text.size() && m_text[m_at] != '"'; ++m_at)
			{
				if (m_text[m_at] == '\\' && m_at + 1 < m_text.size())
					++m_at;
				text += m_text[m_at];
			}
			if (m_at >= m_text.size())
				return std::nullopt;
			++m_at;
			return atom("\"" + text);
		}
		if (m_text[m_at] != '(')
		{
			size_t end = m_text.find_first_of(" ()", m_at);
			end = end == std::string::npos ? m_text.size() : end;
			std::string text = m_text.substr(m_at, end - m_at);
			m_at = end;
			return text.empty() ? std::nullopt : std::optional(atom(text));
		}
		++m_at;
		skip();
		size_t end = m_text.find_first_of(" ()", m_at);
		if (end == std::string::npos)
			return std::nullopt;
		Step s = node(m_text.substr(m_at, end - m_at), {});
		m_at = end;
		for (;;)
		{
			skip();
			if (m_at >= m_text.size())
				return std::nullopt;
			if (m_text[m_at] == ')')
			{
				++m_at;
				break;
			}
			std::optional<Step> kid = step();
			if (!kid)
				return std::nullopt;
			s.kids.push_back(std::move(*kid));
		}
		// a call's key: its first operand, a quoted atom
		if (s.op == "call")
		{
			if (s.kids.empty() || s.kids.front().atom.empty() || s.kids.front().atom[0] != '"')
				return std::nullopt;
			s.atom = s.kids.front().atom.substr(1);
			s.kids.erase(s.kids.begin());
		}
		return s;
	}

	const std::string& m_text;
	size_t m_at = 0;
};

// The one form both sides are compared in (the class comment). Every load carries the version of memory it reads:
// a count of the writes and calls before it on its path (an if whose branches write starts a new one after it), so
// two loads of one address compare equal only where nothing can have written in between. A let's value is evaluated
// where the let stands (its loads at that version) and stands for itself wherever its name is read; a stored value
// is read back as that value only by a load of its address at the version the store made, so across no other write,
// counter step or call, and into a branch only up to the branch's first one.
class Normal
{
public:
	Step statement(const Step& s)
	{
		std::vector<Step> items;
		flatten(expression(s, true), items);
		return run(items);
	}

	// an expression's rewritten form alone, without versions (the shape of a vtable slot's read)
	Step value(const Step& s) { return expression(s, false); }

private:
	struct Stored
	{
		std::string size;
		std::string address;
		Step value;
		int version = 0;
	};

	struct State
	{
		int version = 0;
		std::optional<Stored> stored;
		std::unordered_map<std::string, Step> lets;
	};

	static Step sequence(std::vector<Step> items)
	{
		return items.size() == 1 ? std::move(items.front()) : node("seq", std::move(items));
	}

	// nested sequences as one list of items; an if's branches each one sequence of items
	void flatten(const Step& s, std::vector<Step>& items)
	{
		if (s.op == "seq")
		{
			for (const Step& kid : s.kids)
				flatten(kid, items);
			return;
		}
		if (s.op == "if" && s.kids.size() == 2)
		{
			// `if (p && q) T` is `if (p) if (q) T`
			if (s.kids[0].op == "and")
			{
				flatten(node("if", {s.kids[0].kids[0], node("if", {s.kids[0].kids[1], s.kids[1]})}), items);
				return;
			}
			// a counter changed in the test: its own step first, the test reading it back
			const Step& test = s.kids[0];
			if ((test.op == "eq" || test.op == "ne") && counter(test.kids[0].op) && test.kids[0].op[1] == 'r' &&
				test.kids[1].op.empty())
			{
				const Step& change = test.kids[0];
				flatten(change, items);
				flatten(node("if", {node(test.op, {node("ld", change.kids), test.kids[1]}), s.kids[1]}), items);
				return;
			}
		}
		if (s.op == "if")
		{
			Step branch = node("if", {s.kids[0]});
			for (size_t k = 1; k < s.kids.size(); ++k)
			{
				std::vector<Step> inner;
				flatten(s.kids[k], inner);
				branch.kids.push_back(node("seq", std::move(inner)));
			}
			items.push_back(std::move(branch));
			return;
		}
		items.push_back(s);
	}

	Step run(const std::vector<Step>& items)
	{
		std::vector<Step> out;
		for (const Step& item : items)
			step(item, out);
		return sequence(std::move(out));
	}

	// one item evaluated in order; an if whose test is then a compare of two numbers as the branch it takes
	void step(const Step& item, std::vector<Step>& out)
	{
		if (item.op == "let" && item.kids.size() == 2)
		{
			Step value = eval(item.kids[1]);
			// a value with an effect of its own is no copy: kept where it runs, its name read as itself
			if (writes(value))
			{
				out.push_back(node("let", {item.kids[0], value}));
				m_state.lets[item.kids[0].atom] = item.kids[0];
			}
			else
				m_state.lets[item.kids[0].atom] = std::move(value);
			return;
		}
		if (item.op != "if")
		{
			out.push_back(eval(item));
			return;
		}
		Step test = eval(item.kids[0]);
		if (std::optional<bool> taken = constant(test))
		{
			if (*taken || item.kids.size() > 2)
				for (const Step& s : item.kids[*taken ? 1 : 2].kids)
					step(s, out);
			return;
		}
		Step branch = node("if", {test});
		State before = m_state;
		bool wrote = false;
		for (size_t k = 1; k < item.kids.size(); ++k)
		{
			m_state = before;
			int mark = m_next;
			std::vector<Step> items = item.kids[k].kids;
			// an if first in the branch testing what this one tests (nothing runs between, the tests only read): its
			// branch, as the compiler folds the test
			while (k == 1 && !items.empty() && items.front().op == "if" && items.front().kids.size() == 2 &&
				   !writes(test) && !writes(items.front().kids[0]) && eval(items.front().kids[0]).text() == test.text())
			{
				std::vector<Step> taken = items.front().kids[1].kids;
				items.erase(items.begin());
				items.insert(items.begin(), taken.begin(), taken.end());
			}
			std::vector<Step> inner;
			for (const Step& s : items)
				step(s, inner);
			wrote = wrote || m_next != mark;
			branch.kids.push_back(sequence(std::move(inner)));
		}
		m_state = std::move(before);
		if (wrote)
		{
			m_state.version = ++m_next;
			m_state.stored.reset();
		}
		out.push_back(std::move(branch));
	}

	// a compare of two operands that write nothing in one operand order: a number last, else by their text
	static void order(Step& n)
	{
		if ((n.op != "eq" && n.op != "ne") || writes(n.kids[0]) || writes(n.kids[1]))
			return;
		auto isConstant = [](const Step& x) { return x.op.empty() && x.atom.starts_with("#"); };
		if (isConstant(n.kids[0]) != isConstant(n.kids[1]) ? isConstant(n.kids[0])
															: n.kids[1].text() < n.kids[0].text())
			std::swap(n.kids[0], n.kids[1]);
	}

	// an expression evaluated: operands first, left to right, then its own read or write
	Step eval(const Step& s)
	{
		if (s.op.empty())
		{
			auto found = m_state.lets.find(s.atom);
			return found == m_state.lets.end() ? s : found->second;
		}
		Step n = s;
		for (Step& kid : n.kids)
			kid = eval(kid);
		if (n.op == "ld" && n.kids.size() == 2)
		{
			const std::optional<Stored>& stored = m_state.stored;
			if (stored && stored->version == m_state.version && stored->size == n.kids[0].atom &&
				stored->address == n.kids[1].text())
				return stored->value;
			n.kids.push_back(atom("@" + std::to_string(m_state.version)));
			return n;
		}
		order(n);
		if (effect(n))
		{
			m_state.version = ++m_next;
			m_state.stored.reset();
			if (n.op == "set")
				m_state.stored = Stored{n.kids[0].atom, n.kids[1].text(), n.kids[2], m_state.version};
		}
		return n;
	}

	// a compare of two numbers: its outcome
	static std::optional<bool> constant(const Step& test)
	{
		if ((test.op != "eq" && test.op != "ne") || !test.kids[0].op.empty() || !test.kids[1].op.empty() ||
			!test.kids[0].atom.starts_with("#") || !test.kids[1].atom.starts_with("#"))
			return std::nullopt;
		return (test.kids[0].atom == test.kids[1].atom) == (test.op == "eq");
	}

	// the Win32 Interlocked imports, called through their IAT slots: the atomic builtins src writes for them (each
	// returns the new count)
	static std::optional<Step> interlocked(const Step& call)
	{
		if (call.kids.size() != 2 || call.kids[0].op != "ld" || call.kids[0].kids.size() != 2 ||
			!call.kids[0].kids[1].op.empty() || !call.kids[0].kids[1].atom.starts_with("#"))
			return std::nullopt;
		qstring name;
		get_name(&name, static_cast<ea_t>(std::stoull(call.kids[0].kids[1].atom.substr(1))));
		if (name != "InterlockedIncrement" && name != "InterlockedDecrement")
			return std::nullopt;
		return node("atomic", {atom(name == "InterlockedIncrement" ? "add_fetch" : "sub_fetch"), call.kids[1],
							   number(1)});
	}

	Step expression(const Step& s, bool unused)
	{
		if (s.op.empty())
			return s;
		Step n = s;
		bool statements = s.op == "seq";
		for (size_t k = 0; k < n.kids.size(); ++k)
		{
			// an if's branches and a sequence's items are statements, every other operand a value
			bool statement = statements || (s.op == "if" && k > 0);
			n.kids[k] = expression(n.kids[k], statement);
		}
		if (n.op == "icall")
			if (std::optional<Step> atomic = interlocked(n))
				n = std::move(*atomic);
		if (n.op == "+")
		{
			if (n.kids[0].op == "+")
			{
				int64_t sum = std::stoll(n.kids[0].kids[1].atom) + std::stoll(n.kids[1].atom);
				n = node("+", {n.kids[0].kids[0], atom(std::to_string(sum))});
			}
			if (n.kids[1].atom == "0")
				return n.kids[0];
			return n;
		}
		// a value tested for truth (an if's test, a logical operand) is its compare with 0
		auto truth = [](const Step& x) { return boolean(x) ? x : node("ne", {x, number(0)}); };
		if (n.op == "if")
			n.kids[0] = truth(n.kids[0]);
		if (n.op == "and" || n.op == "or")
			n.kids = {truth(n.kids[0]), truth(n.kids[1])};
		if (n.op == "not")
		{
			const Step& x = n.kids[0];
			if (x.op == "eq" || x.op == "ne")
				return node(x.op == "eq" ? "ne" : "eq", x.kids);
			if (boolean(x))
				return n;
			return node("eq", {x, number(0)});
		}
		order(n);
		if ((n.op == "eq" || n.op == "ne") && boolean(n.kids[0]) && isNumber(n.kids[1], 0))
			return n.op == "ne" ? n.kids[0] : expression(node("not", {n.kids[0]}), unused);
		if (unused && n.op == "post+")
			n.op = "pre+";
		if (unused && n.op == "post-")
			n.op = "pre-";
		if (unused && n.op == "atomic" && n.atom.empty() && n.kids.size() == 3)
		{
			// the operation's name is its first operand
			if (n.kids[0].atom == "fetch_add")
				n.kids[0].atom = "add_fetch";
			if (n.kids[0].atom == "fetch_sub")
				n.kids[0].atom = "sub_fetch";
		}
		return n;
	}

	State m_state;
	int m_next = 0;
};

}  // namespace

std::optional<Step> Step::parse(const std::string& text)
{
	return Parser(text).all();
}

Step Step::bind(const std::unordered_map<std::string, Step>& bound) const
{
	std::map<std::string, Step> sorted(bound.begin(), bound.end());
	Step call = node("seq", {});
	for (const auto& [name, value] : sorted)
		call.kids.push_back(node("let", {Step{"", name, {}}, value}));
	call.kids.push_back(*this);
	return call;
}

std::string Step::text() const
{
	if (op.empty())
		return atom;
	std::string s = "(" + op;
	if (!atom.empty())
		s += " \"" + atom + "\"";
	for (const Step& kid : kids)
		s += " " + kid.text();
	return s + ")";
}

std::optional<Step> Steps::address(const cexpr_t* e, const std::unordered_map<int, Step>& copies) const
{
	switch (e->op)
	{
	case cot_var:
		return atom("&v" + std::to_string(e->v.idx));
	case cot_ptr:
		return lower(e->x, copies);
	case cot_memptr:
	{
		std::optional<Step> object = lower(e->x, copies);
		return object ? std::optional(node("+", {*object, atom(std::to_string(e->m))})) : std::nullopt;
	}
	case cot_memref:
	{
		std::optional<Step> object = address(e->x, copies);
		return object ? std::optional(node("+", {*object, atom(std::to_string(e->m))})) : std::nullopt;
	}
	case cot_obj:
		return number(static_cast<int64_t>(e->obj_ea));
	default:
		return std::nullopt;
	}
}

std::optional<Step> Steps::lower(const cexpr_t* e, const std::unordered_map<int, Step>& copies) const
{
	auto both = [&](const char* op) -> std::optional<Step> {
		std::optional<Step> x = lower(e->x, copies);
		std::optional<Step> y = x ? lower(e->y, copies) : x;
		return y ? std::optional(node(op, {*x, *y})) : std::nullopt;
	};
	auto size = [](const cexpr_t* x) { return atom(std::to_string(x->type.get_size())); };
	switch (e->op)
	{
	case cot_cast:
	{
		// a cast that keeps the value's bits (between pointers and integers of one size), else outside the language
		auto scalar = [](const tinfo_t& t) { return (t.is_ptr() || t.is_integral() || t.is_enum()) && !t.is_bool(); };
		if (!scalar(e->type) || !scalar(e->x->type) || e->type.get_size() != e->x->type.get_size())
			return std::nullopt;
		return lower(e->x, copies);
	}
	case cot_obj:
	{
		// a global's value: its load; an array's: its address
		if (e->type.is_array())
			return number(static_cast<int64_t>(e->obj_ea));
		if (e->type.is_func())
			return std::nullopt;
		return node("ld", {size(e), number(static_cast<int64_t>(e->obj_ea))});
	}
	case cot_var:
	{
		int v = e->v.idx;
		while (m_function.locals[v].aliasOf >= 0)
			v = m_function.locals[v].aliasOf;
		if (auto found = copies.find(v); found != copies.end())
			return found->second;
		return atom("v" + std::to_string(v));
	}
	case cot_num:
	{
		uint64_t n = e->numval();
		int bits = static_cast<int>(e->type.get_size()) * 8;
		int64_t value = bits > 0 && bits < 64 && (n >> (bits - 1)) & 1 && e->type.is_signed()
							? static_cast<int64_t>(n | (~0ULL << bits))
							: static_cast<int64_t>(n);
		return number(value);
	}
	case cot_ref:
		return address(e->x, copies);
	case cot_add:
	case cot_sub:
	{
		std::optional<Step> base = lower(e->x, copies);
		if (!base || e->y->op != cot_num)
			return std::nullopt;
		int64_t scale = e->x->type.is_ptr() ? e->x->type.get_ptrarr_objsize() : e->x->type.is_integral() ? 1 : 0;
		if (scale <= 0)
			return std::nullopt;
		int64_t n = static_cast<int64_t>(e->y->numval()) * scale;
		return node("+", {*base, atom(std::to_string(e->op == cot_add ? n : -n))});
	}
	case cot_ptr:
	case cot_memptr:
	case cot_memref:
	{
		std::optional<Step> where = address(e, copies);
		return where ? std::optional(node("ld", {size(e), *where})) : std::nullopt;
	}
	case cot_lnot:
	{
		std::optional<Step> x = lower(e->x, copies);
		return x ? std::optional(node("not", {*x})) : std::nullopt;
	}
	case cot_eq:
		return both("eq");
	case cot_ne:
		return both("ne");
	case cot_land:
		return both("and");
	case cot_lor:
		return both("or");
	case cot_preinc:
	case cot_predec:
	case cot_postinc:
	case cot_postdec:
	{
		if (!e->x->type.is_integral())
			return std::nullopt;
		std::optional<Step> where = address(e->x, copies);
		const char* op = e->op == cot_preinc ? "pre+" : e->op == cot_predec ? "pre-" : e->op == cot_postinc ? "post+" : "post-";
		return where ? std::optional(node(op, {size(e->x), *where})) : std::nullopt;
	}
	case cot_asg:
	{
		std::optional<Step> where = address(e->x, copies);
		std::optional<Step> value = where ? lower(e->y, copies) : where;
		return value ? std::optional(node("set", {size(e->x), *where, *value})) : std::nullopt;
	}
	case cot_call:
	{
		std::vector<Step> arguments;
		for (const carg_t& argument : *e->a)
		{
			std::optional<Step> value = lower(&argument, copies);
			if (!value)
				return std::nullopt;
			arguments.push_back(std::move(*value));
		}
		// a call through a data item (an import's IAT slot): through the pointer it holds
		if (const cexpr_t* target = stripCasts(e->x); target->op == cot_obj)
			if (func_t* f = get_func(target->obj_ea); !f || f->start_ea != target->obj_ea)
			{
				std::vector<Step> kids{node("ld", {atom("4"), number(static_cast<int64_t>(target->obj_ea))})};
				kids.insert(kids.end(), arguments.begin(), arguments.end());
				return node("icall", std::move(kids));
			}
		auto found = m_function.meaning.find(e);
		const Call* call = found != m_function.meaning.end() ? std::get_if<Call>(&found->second) : nullptr;
		// a direct call of a resolved src function (one through the vtable is its slot, below)
		if (call && call->function && call->resultArgument < 0 && !call->through && stripCasts(e->x)->op == cot_obj)
			return node("call", std::move(arguments), call->function->mangled);
		// through the vtable: (ld 4 (+ (ld 4 OBJ) K)), OBJ the first argument
		std::optional<Step> callee = lower(e->x, copies);
		if (!callee || arguments.empty())
			return std::nullopt;
		Step through = Normal().value(*callee);
		if (through.op != "ld")
			return std::nullopt;
		Step slot = through.kids[1];
		std::string offset = "0";
		if (slot.op == "+" && slot.kids[1].op.empty())
		{
			offset = slot.kids[1].atom;
			slot = Step(slot.kids[0]);
		}
		if (slot.op != "ld" || slot.kids[1].text() != Normal().value(arguments.front()).text())
			return std::nullopt;
		return node("vcall", [&] {
			std::vector<Step> kids{atom(offset)};
			kids.insert(kids.end(), arguments.begin(), arguments.end());
			return kids;
		}());
	}
	default:
		return std::nullopt;
	}
}

std::optional<Step> Steps::statement(const cinsn_t* s, std::unordered_map<int, Step>& copies, int except) const
{
	// a statement src leaves out (an EH frame's state store, a no-op call) does nothing src writes
	if (m_function.omitted(s))
		return node("seq", {});
	switch (s->op)
	{
	case cit_block:
	{
		Step block = node("seq", {});
		for (const cinsn_t& item : *s->cblock)
		{
			std::optional<Step> step = statement(&item, copies, except);
			if (!step)
				return std::nullopt;
			block.kids.push_back(std::move(*step));
		}
		return block;
	}
	case cit_if:
	{
		std::optional<Step> test = lower(&s->cif->expr, copies);
		std::optional<Step> then = test ? statement(s->cif->ithen, copies, except) : test;
		if (!then)
			return std::nullopt;
		Step branch = node("if", {*test, *then});
		if (s->cif->ielse)
		{
			std::optional<Step> otherwise = statement(s->cif->ielse, copies, except);
			if (!otherwise)
				return std::nullopt;
			branch.kids.push_back(std::move(*otherwise));
		}
		return branch;
	}
	case cit_expr:
	{
		// `t = value;`: Hex-Rays' copy, t standing for the value it held there from here on
		const cexpr_t* e = s->cexpr;
		if (e->op == cot_asg && e->x->op == cot_var && e->x->v.idx != except &&
			!copies.count(e->x->v.idx) && m_function.locals[e->x->v.idx].aliasOf < 0)
		{
			std::optional<Step> value = lower(e->y, copies);
			if (!value)
				return std::nullopt;
			Step name = atom("@t" + std::to_string(e->x->v.idx));
			copies.emplace(e->x->v.idx, name);
			return node("let", {name, std::move(*value)});
		}
		return lower(e, copies);
	}
	default:
		return std::nullopt;
	}
}

bool Steps::match(const std::vector<const cinsn_t*>& statements, const Step& steps, int except,
				  std::vector<int>* copies, const std::unordered_map<int, Step>& known) const
{
	// the values the caller knows the locals hold: lets before the statements
	std::unordered_map<int, Step> copied;
	Step exe = node("seq", {});
	for (const auto& [lvar, value] : std::map<int, Step>(known.begin(), known.end()))
	{
		Step name = atom("@t" + std::to_string(lvar));
		copied.emplace(lvar, name);
		exe.kids.push_back(node("let", {name, value}));
	}
	for (const cinsn_t* s : statements)
	{
		std::optional<Step> step = statement(s, copied, except);
		if (!step)
			return false;
		exe.kids.push_back(std::move(*step));
	}
	if (Normal().statement(exe).text() != Normal().statement(steps).text())
		return false;
	if (copies)
		for (const auto& [lvar, value] : copied)
			if (!known.count(lvar))
				copies->push_back(lvar);
	return true;
}

}  // namespace idakit
