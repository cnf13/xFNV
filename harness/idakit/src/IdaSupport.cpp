#include "IdaSupport.hpp"

#include "idakit/Asm.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>

namespace idakit
{

namespace
{

bool identifierChar(char c)
{
	return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

bool plainInteger(const tinfo_t& t)
{
	return t.is_integral() && !t.is_enum() && !t.is_bool();
}

}  // namespace

std::vector<ea_t> gameFunctions()
{
	std::vector<ea_t> out;
	for (size_t k = 0, n = get_func_qty(); k < n; ++k)
		if (func_t* f = getn_func(k); f && f->start_ea < kGameEnd)
			out.push_back(f->start_ea);
	std::sort(out.begin(), out.end());
	return out;
}

bool idaNoreturn(ea_t address)
{
	tinfo_t stored;
	func_type_data_t fd;
	if (get_tinfo(&stored, address) && stored.get_func_details(&fd, GTD_NO_ARGLOCS) && fd.is_noret())
		return true;
	func_t* f = get_func(address);
	return f && f->start_ea == address && (f->flags & FUNC_NORET);
}

bool functionStart(ea_t address)
{
	func_t* f = get_func(address);
	return f && f->start_ea == address;
}

std::vector<ea_t> itemHeads(func_t* function)
{
	std::vector<ea_t> out;
	func_item_iterator_t it;
	for (bool ok = it.set(function); ok; ok = it.next_head())
		out.push_back(it.current());
	return out;
}

std::vector<ea_t> codeItems(func_t* function)
{
	std::vector<ea_t> out;
	func_item_iterator_t it;
	for (bool ok = it.set(function); ok; ok = it.next_code())
		out.push_back(it.current());
	return out;
}

std::string hex(uint64_t value)
{
	char buffer[24];
	std::snprintf(buffer, sizeof buffer, "0x%llx", static_cast<unsigned long long>(value));
	return buffer;
}

std::string lastComponent(const std::string& name)
{
	size_t operatorAt = name.find("::operator");
	if (operatorAt != std::string::npos)
		return name.substr(operatorAt + 2);
	int depth = 0;
	size_t start = 0;
	for (size_t k = 0; k + 1 < name.size(); ++k)
	{
		char c = name[k];
		if (c == '<' || c == '(')
			++depth;
		else if (c == '>' || c == ')')
			--depth;
		else if (depth == 0 && c == ':' && name[k + 1] == ':')
			start = k + 2;
	}
	return name.substr(start);
}

std::string scopeOf(const std::string& name)
{
	std::string last = lastComponent(name);
	return name.size() > last.size() + 2 ? name.substr(0, name.size() - last.size() - 2) : "";
}

std::string pdbSpelling(ea_t address)
{
	std::string name = get_name(address).c_str();
	if (!name.starts_with('?'))
		return name;
	// IDA's short demangling: `X::f(int)`, or `X::`scalar deleting destructor'` without arguments
	name = get_short_name(address).c_str();
	// the argument list: from the `(` matching the last `)`
	size_t close = name.rfind(')');
	if (close == std::string::npos)
		return name;
	int depth = 0;
	for (size_t k = close + 1; k-- > 0;)
		if (name[k] == ')')
			++depth;
		else if (name[k] == '(' && --depth == 0)
			return name.substr(0, k);
	return name;
}

bool constructorName(const std::string& name)
{
	auto bare = [](const std::string& s) { return s.substr(0, s.find('<')); };
	std::string owner = bare(lastComponent(scopeOf(name)));
	return !owner.empty() && bare(lastComponent(name)) == owner;
}

std::optional<std::string> withoutSuffix(const std::string& name)
{
	size_t underscore = name.rfind('_');
	if (underscore == std::string::npos || underscore == 0 || underscore + 1 == name.size() ||
		!std::all_of(name.begin() + underscore + 1, name.end(), [](char c) { return std::isdigit(c); }))
		return std::nullopt;
	return name.substr(0, underscore);
}

std::string withoutBlanks(std::string text)
{
	std::erase(text, ' ');
	return text;
}

const cexpr_t* stripCasts(const cexpr_t* e)
{
	while (e->op == cot_cast)
		e = e->x;
	return e;
}

std::set<int> addressTakenLocals(cfunc_t& cfunc)
{
	struct Visitor : ctree_visitor_t
	{
		lvars_t& lvars;
		std::set<int> taken;
		std::set<const cexpr_t*> subscripted;

		explicit Visitor(lvars_t& l) : ctree_visitor_t(CV_FAST), lvars(l) {}

		// Hex-Rays types an array local's uses as the pointer it decays to
		bool arrayVar(const cexpr_t* e) const { return e->op == cot_var && lvars[e->v.idx].type().is_array(); }

		// pre-order: a subscript is seen before its array
		int idaapi visit_expr(cexpr_t* e) override
		{
			if (e->op == cot_idx && arrayVar(e->x))
				subscripted.insert(e->x);
			if (e->op == cot_ref)
			{
				const cexpr_t* part = e->x;
				while (part->op == cot_memref || (part->op == cot_idx && arrayVar(part->x)))
					part = part->x;
				if (part->op == cot_var)
					taken.insert(part->v.idx);
			}
			else if (arrayVar(e) && !subscripted.contains(e))
				taken.insert(e->v.idx);
			return 0;
		}
	} visitor(*cfunc.get_lvars());
	visitor.apply_to(&cfunc.body, nullptr);
	return visitor.taken;
}

bool isPartHelper(std::string_view helper)
{
	if (helper.starts_with('S'))
		helper.remove_prefix(1);
	if (helper.starts_with("LO") || helper.starts_with("HI"))
		return helper.substr(2) == "BYTE" || helper.substr(2) == "WORD" || helper.substr(2) == "DWORD";
	for (std::string_view unit : {"BYTE", "WORD"})
		if (helper.starts_with(unit) && helper.size() > unit.size() &&
			std::all_of(helper.begin() + unit.size(), helper.end(), [](char c) { return c >= '0' && c <= '9'; }))
			return true;
	return false;
}

std::string text(const tinfo_t& type)
{
	return type.dstr();
}

bool sameType(const tinfo_t& a, const tinfo_t& b)
{
	auto strip = [](std::string s) {
		for (const char* tag : {"struct ", "enum ", "union ", "class "})
		{
			size_t length = std::char_traits<char>::length(tag);
			for (size_t at = s.find(tag); at != std::string::npos; at = s.find(tag, at))
				if (at == 0 || !identifierChar(s[at - 1]))
					s.erase(at, length);
				else
					at += length;
		}
		return s;
	};
	return strip(text(a)) == strip(text(b));
}

tinfo_t pointerTo(const tinfo_t& target)
{
	tinfo_t t;
	t.create_ptr(target);
	return t;
}

tinfo_t voidType()
{
	return tinfo_t(BT_VOID);
}

tinfo_t byteArray(int64_t size)
{
	tinfo_t t;
	t.create_array(tinfo_t(BT_INT8 | BTMT_USIGNED), static_cast<uint32>(size));
	return t;
}

int64_t stackSize(const tinfo_t& type)
{
	size_t size = type.get_size();
	int64_t bytes = size == BADSIZE ? 4 : static_cast<int64_t>(size);
	return (bytes + 3) / 4 * 4;
}

std::optional<int64_t> stackBytes(const tinfo_t& member)
{
	tinfo_t type = member.is_ptr() ? member.get_pointed_object() : member;
	func_type_data_t fd;
	if (!type.get_func_details(&fd) || fd.is_vararg_cc())
		return std::nullopt;
	callcnv_t cc = fd.get_cc() & CM_CC_MASK;
	if (cc != CM_CC_THISCALL && cc != CM_CC_STDCALL)
		return std::nullopt;
	return static_cast<int64_t>(fd.stkargs);
}

std::set<int64_t> reachableRets(func_t* function)
{
	FlowChart chart(function);
	const std::vector<FlowChart::Block>& blocks = chart.blocks();
	std::set<int64_t> rets;
	std::vector<bool> seen(blocks.size());
	std::vector<int> pending{0};
	while (!pending.empty())
	{
		int k = pending.back();
		pending.pop_back();
		if (k < 0 || static_cast<size_t>(k) >= blocks.size() || seen[k])
			continue;
		seen[k] = true;
		for (ea_t ea : heads(blocks[k].start, blocks[k].end))
		{
			insn_t insn;
			if (decode_insn(&insn, ea) > 0 && (insn.itype == NN_retn || insn.itype == NN_retf))
				rets.insert(insn.ops[0].type == o_imm ? static_cast<int64_t>(insn.ops[0].value) : 0);
		}
		pending.insert(pending.end(), blocks[k].succs.begin(), blocks[k].succs.end());
	}
	return rets;
}

bool untyped(const tinfo_t& t, bool self)
{
	if (!t.is_ptr())
		return plainInteger(t);
	tinfo_t target = t.get_pointed_object();
	if (target.is_void())
		return true;
	if (target.is_char() || target.is_decl_char())
		return self;
	return plainInteger(target);
}

std::string bucket(const std::string& why, const char* stops)
{
	std::string out = why.substr(0, why.find_first_of(stops));
	while (!out.empty() && out.back() == ' ')
		out.pop_back();
	return out;
}

const char* const kUserRegisters[8] = {"eax", "ecx", "edx", "ebx", nullptr, nullptr, "esi", "edi"};

int userFamily(const argloc_t& where)
{
	if (!where.is_reg1() || where.regoff() != 0)
		return -1;
	int reg = where.reg1() >= 16 && where.reg1() < 20 ? where.reg1() - 16 : where.reg1();
	return reg >= 0 && reg < 8 && kUserRegisters[reg] ? reg : -1;
}

std::string userRegister(const argloc_t& where)
{
	int family = userFamily(where);
	return family < 0 ? "" : kUserRegisters[family];
}

std::string locationText(const argloc_t& where, size_t size)
{
	qstring name;
	if (where.is_reg1() && get_reg_name(&name, where.reg1(), size) > 0)
		return name.c_str();
	if (where.is_reg2())
		return "a register pair";
	return "a location IDA cannot name";
}

std::optional<std::string> userResult(const func_type_data_t& fd)
{
	const tinfo_t& ret = fd.rettype;
	if (ret.is_void())
		return "";
	size_t size = ret.get_size();
	if (ret.is_udt() || size == BADSIZE)
		return std::nullopt;
	const argloc_t& loc = fd.retloc;
	if (ret.is_floating())
	{
		std::string name = locationText(loc, size);
		if ((name == "st0" || name == "st") && (size == 4 || size == 8))
			return "st0";
		return std::nullopt;
	}
	if (size <= 4 && userRegister(loc) == "eax")
		return "eax";
	if (size == 8 && loc.is_reg2() && loc.reg1() == 0 && loc.reg2() == 2)
		return "edx:eax";
	return std::nullopt;
}

}  // namespace idakit
