// The disassembly as the analyses read it: instructions with their mnemonic and operands (the meanings of IDC's
// print_insn_mnem / get_operand_type / get_operand_value / print_operand, which the Python tools used), heads, the
// stack pointer delta and a function's flow chart.
#pragma once

#include "idakit/Ida.hpp"

#include <optional>
#include <string>
#include <vector>

namespace idakit
{

// One decoded instruction (an address that decodes to none has no operands and an empty mnemonic).
class Insn
{
public:
	explicit Insn(ea_t ea);

	ea_t ea() const { return m_ea; }
	bool decoded() const { return m_decoded; }
	const std::string& mnem() const { return m_mnem; }

	// IDC's get_operand_type: o_void.. (o_reg, o_mem, o_phrase, o_displ, o_imm, o_near, ...); -1 when nothing
	// decodes there.
	int opType(int n) const;
	// IDC's get_operand_value: addr for o_mem/o_far/o_near/o_displ, the register for o_reg, the value for o_imm,
	// the phrase for o_phrase; -1 otherwise or when nothing decodes.
	int64_t opValue(int n) const;
	// IDC's print_operand: the operand's text without color tags ("" when there is none).
	std::string opText(int n) const;

private:
	ea_t m_ea;
	bool m_decoded = false;
	insn_t m_insn;
	std::string m_mnem;
};

// The heads (instruction or data starts) in [start, end).
std::vector<ea_t> heads(ea_t start, ea_t end);
// The heads of a function's main chunk.
std::vector<ea_t> heads(const func_t* function);
// The next / previous head (BADADDR at the bounds).
ea_t nextHead(ea_t ea, ea_t limit = BADADDR);
ea_t prevHead(ea_t ea, ea_t limit);
// The stack pointer delta at ea in its function (IDC's get_spd); nullopt outside a function.
std::optional<int64_t> spDelta(ea_t ea);
// The 32-bit two's complement value of the low dword (`v - 2**32 if v & 0x80000000`).
inline int64_t signed32(uint64_t value)
{
	value &= 0xFFFFFFFFull;
	return value & 0x80000000ull ? static_cast<int64_t>(value) - (int64_t(1) << 32) : static_cast<int64_t>(value);
}

// A function's basic blocks (the flow chart IDA draws), block 0 the entry.
class FlowChart
{
public:
	struct Block
	{
		ea_t start;
		ea_t end;
		std::vector<int> succs;  // block indexes
	};

	explicit FlowChart(func_t* function);
	const std::vector<Block>& blocks() const { return m_blocks; }

private:
	std::vector<Block> m_blocks;
};

}  // namespace idakit
