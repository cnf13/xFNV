#include "idakit/Asm.hpp"

namespace idakit
{

Insn::Insn(ea_t ea) : m_ea(ea)
{
	m_decoded = decode_insn(&m_insn, ea) > 0;
	qstring mnem;
	if (print_insn_mnem(&mnem, ea))
		m_mnem = mnem.c_str();
}

int Insn::opType(int n) const
{
	if (!m_decoded)
		return -1;
	return m_insn.ops[n].type;
}

int64_t Insn::opValue(int n) const
{
	if (!m_decoded)
		return -1;
	const op_t& op = m_insn.ops[n];
	switch (op.type)
	{
	case o_mem:
	case o_far:
	case o_near:
	case o_displ:
		return static_cast<int64_t>(op.addr);
	case o_reg:
		return op.reg;
	case o_imm:
		return static_cast<int64_t>(op.value);
	case o_phrase:
		return op.phrase;
	default:
		return -1;
	}
}

std::string Insn::opText(int n) const
{
	qstring text;
	if (!print_operand(&text, m_ea, n))
		return "";
	tag_remove(&text);
	return text.c_str();
}

std::vector<ea_t> heads(ea_t start, ea_t end)
{
	std::vector<ea_t> out;
	ea_t ea = start;
	if (!is_head(get_flags(ea)))
		ea = next_head(ea, end);
	while (ea < end && ea != BADADDR)
	{
		out.push_back(ea);
		ea = next_head(ea, end);
	}
	return out;
}

std::vector<ea_t> heads(const func_t* function)
{
	return heads(function->start_ea, function->end_ea);
}

ea_t nextHead(ea_t ea, ea_t limit)
{
	return next_head(ea, limit);
}

ea_t prevHead(ea_t ea, ea_t limit)
{
	return prev_head(ea, limit);
}

std::optional<int64_t> spDelta(ea_t ea)
{
	func_t* function = get_func(ea);
	if (function == nullptr)
		return std::nullopt;
	return get_spd(function, ea);
}

FlowChart::FlowChart(func_t* function)
{
	qflow_chart_t chart("", function, BADADDR, BADADDR, 0);
	m_blocks.resize(chart.size());
	for (int k = 0; k < chart.size(); ++k)
	{
		m_blocks[k].start = chart.blocks[k].start_ea;
		m_blocks[k].end = chart.blocks[k].end_ea;
		for (int s = 0; s < chart.nsucc(k); ++s)
			m_blocks[k].succs.push_back(chart.succ(k, s));
	}
}

}  // namespace idakit
