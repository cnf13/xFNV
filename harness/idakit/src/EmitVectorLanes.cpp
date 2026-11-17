// VectorLanes: an SSE vector local the code writes only lane by lane (`v.m128_f32[0] = x`, Hex-Rays' form of a
// `movss xmm1, xmm2` that keeps xmm1's other lanes), which Hex-Rays reports possibly undefined for the lanes nothing
// writes, is no uninitialised read where no read of it needs those lanes: each read of a lane, and each lane of a
// read of the whole vector that what takes it needs, follows a write of that lane. A lane is needed through the SSE
// intrinsics Hex-Rays names by their lane rules (`_mm_cmpeq_ss(a, v)` needs v's lane 0, `_mm_unpacklo_ps(v, w)` v's
// lanes 0 and 1 where its result's lanes 0 and 2 are needed, `_mm_shuffle_ps` the lanes its selectors pick, a lane-wise
// operation the lanes needed of its result), a vector local assigned the value passes on the lanes its own reads need
// (all of the function's reads together), and anything else (a store to memory or a field, a call's argument, a
// return) needs every lane. A write precedes a read where the write is a statement of a block and the read is in a
// later statement of that block, and no goto jumps past the write into that block. The local is declared
// `__m128 v = _mm_undefined_ps();`: C++'s value whose lanes are unspecified, what the exe's register holds there, and
// the lane writes then write a defined vector.

#include "EmitScopes.hpp"

#include "IdaSupport.hpp"

#include <map>
#include <string_view>

namespace idakit
{

namespace
{

constexpr unsigned kAll = 0xF;

// the lanes of an SSE intrinsic's operand k that the lanes `result` of its result need; nullopt where the table has
// no rule for the helper (every lane)
std::optional<unsigned> operandLanes(const cexpr_t* call, size_t k, unsigned result)
{
	std::string_view name = call->x->helper;
	auto lanes = [](std::initializer_list<int> from, unsigned result) {
		// from[i]: the operand lane result lane i reads, -1 for none
		unsigned out = 0;
		int i = 0;
		for (int lane : from)
		{
			if ((result >> i & 1) && lane >= 0)
				out |= 1u << lane;
			++i;
		}
		return out;
	};
	static constexpr std::string_view laneWise[] = {
		"_mm_add_ps", "_mm_sub_ps", "_mm_mul_ps", "_mm_div_ps", "_mm_min_ps", "_mm_max_ps", "_mm_and_ps",
		"_mm_andnot_ps", "_mm_or_ps", "_mm_xor_ps", "_mm_cmpeq_ps", "_mm_cmplt_ps", "_mm_cmple_ps", "_mm_cmpgt_ps",
		"_mm_cmpge_ps", "_mm_cmpneq_ps", "_mm_rcp_ps", "_mm_rsqrt_ps", "_mm_sqrt_ps", "_mm_rcp_ss", "_mm_rsqrt_ss",
		"_mm_sqrt_ss"};
	for (std::string_view w : laneWise)
		if (name == w)
			return result;
	// lane 0 of both operands, the other lanes the first operand's
	static constexpr std::string_view scalar[] = {"_mm_add_ss", "_mm_sub_ss", "_mm_mul_ss", "_mm_div_ss",
												  "_mm_min_ss", "_mm_max_ss", "_mm_cmpeq_ss", "_mm_cmplt_ss",
												  "_mm_cmple_ss"};
	for (std::string_view s : scalar)
		if (name == s)
			return k == 0 ? result : result & 1;
	if (name == "_mm_move_ss")
		return k == 0 ? result & ~1u : result & 1;
	if (name == "_mm_unpacklo_ps")
		return k == 0 ? lanes({0, -1, 1, -1}, result) : lanes({-1, 0, -1, 1}, result);
	if (name == "_mm_unpackhi_ps")
		return k == 0 ? lanes({2, -1, 3, -1}, result) : lanes({-1, 2, -1, 3}, result);
	if (name == "_mm_movelh_ps")
		return k == 0 ? lanes({0, 1, -1, -1}, result) : lanes({-1, -1, 0, 1}, result);
	if (name == "_mm_movehl_ps")
		return k == 0 ? lanes({-1, -1, 2, 3}, result) : lanes({2, 3, -1, -1}, result);
	if (name == "_mm_shuffle_ps" && call->a->size() == 3 && (*call->a)[2].op == cot_num && k < 2)
	{
		auto imm = static_cast<int>((*call->a)[2].numval());
		auto pick = [&](int i) { return imm >> (2 * i) & 3; };
		return k == 0 ? lanes({pick(0), pick(1), -1, -1}, result) : lanes({-1, -1, pick(2), pick(3)}, result);
	}
	return std::nullopt;
}

bool isVector(const tinfo_t& type)
{
	return text(type) == "__m128";
}

class VectorLanes
{
public:
	explicit VectorLanes(Function& function)
		: m_function(function), m_lvars(*function.cfunc->get_lvars()), m_parents(parentsOf(function)),
		  m_scopes(function)
	{
	}

	void run()
	{
		std::set<int> candidates;
		for (size_t k = 0; k < m_lvars.size(); ++k)
			if (m_function.locals[k].undefined && !m_lvars[k].is_arg_var() && isVector(m_lvars[k].type()))
				candidates.insert(static_cast<int>(k));
		if (candidates.empty())
			return;
		std::vector<std::pair<const cexpr_t*, const cinsn_t*>> occurrences;
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t* s) {
			if (e->op == cot_var && isVector(m_lvars[e->v.idx].type()))
				occurrences.emplace_back(e, s);
		});
		// the lanes each vector local's reads need, to a fixpoint (a local assigned another's value passes it on)
		for (bool changed = true; changed;)
		{
			changed = false;
			for (const auto& [e, s] : occurrences)
			{
				Occurrence o = classify(e);
				unsigned& needed = m_needed[e->v.idx];
				if (o.kind == Occurrence::read && (needed | o.lanes) != needed)
				{
					needed |= o.lanes;
					changed = true;
				}
			}
		}
		// each candidate's lane writes (statement, lane) and reads (statement, lanes), or none where another use
		std::map<int, std::vector<std::pair<const cinsn_t*, int>>> writes;
		std::map<int, std::vector<std::pair<const cinsn_t*, unsigned>>> reads;
		std::set<int> rejected;
		for (const auto& [e, s] : occurrences)
		{
			int v = e->v.idx;
			if (!candidates.count(v) || m_function.omitted(s))
				continue;
			Occurrence o = classify(e);
			if (o.kind == Occurrence::laneWrite && s->op == cit_expr && s->cexpr == o.write)
				writes[v].emplace_back(s, o.lane);
			else if (o.kind == Occurrence::read)
				reads[v].emplace_back(s, o.lanes);
			else
				rejected.insert(v);
		}
		for (int v : candidates)
		{
			if (rejected.count(v) || !writes.count(v))
				continue;
			bool covered = true;
			for (const auto& [s, lanes] : reads[v])
				for (int lane = 0; lane < 4 && covered; ++lane)
					if (lanes >> lane & 1)
						covered = writtenBefore(writes[v], lane, s);
			if (!covered)
				continue;
			Local& local = m_function.locals[v];
			local.undefined = false;
			local.initialiser = "_mm_undefined_ps()";
			local.initialiserHeader = "<xmmintrin.h>";
		}
	}

private:
	struct Occurrence
	{
		enum Kind
		{
			read,
			laneWrite,
			other,
		} kind = other;
		unsigned lanes = 0;				// read: the lanes it needs
		int lane = -1;					// laneWrite: the lane
		const cexpr_t* write = nullptr;	// laneWrite: the assignment
	};

	const cexpr_t* parentExpression(const cexpr_t* e) const
	{
		auto found = m_parents.find(e);
		return found != m_parents.end() && found->second && found->second->is_expr()
				   ? static_cast<const cexpr_t*>(found->second)
				   : nullptr;
	}

	// an occurrence of a vector local: a lane read or write (`v.m128_f32[k]`, any member of IDA's union of 4-byte
	// elements: they all start at its first byte), a write of the whole (other), or a read of the whole (the lanes
	// what takes it needs)
	Occurrence classify(const cexpr_t* e) const
	{
		const cexpr_t* parent = parentExpression(e);
		if (parent && parent->op == cot_memref && parent->x == e)
		{
			const cexpr_t* index = parentExpression(parent);
			if (!index || index->op != cot_idx || index->x != parent || index->y->op != cot_num ||
				!parent->type.is_array() || parent->type.get_ptrarr_object().get_size() != 4 ||
				index->y->numval() > 3)
				return {Occurrence::read, kAll};
			int lane = static_cast<int>(index->y->numval());
			const cexpr_t* use = parentExpression(index);
			if (use && use->op == cot_asg && use->x == index)
				return {Occurrence::laneWrite, 0, lane, use};
			// a compound assignment or increment of the lane reads it first and writes it: no plain lane write
			if (use && (is_assignment(use->op) || is_prepost(use->op)) && use->x == index)
				return {Occurrence::other};
			return {Occurrence::read, 1u << lane};
		}
		if (parent && (is_assignment(parent->op) || is_prepost(parent->op) || parent->op == cot_ref) &&
			parent->x == e)
			return {Occurrence::other};
		return {Occurrence::read, needed(e)};
	}

	// the lanes of e's value that what takes it needs
	unsigned needed(const cexpr_t* e) const
	{
		const cexpr_t* parent = parentExpression(e);
		if (!parent)
			return kAll;
		if (parent->op == cot_cast && isVector(parent->type))
			return needed(parent);
		if (parent->op == cot_asg && parent->y == e && parent->x->op == cot_var)
		{
			auto found = m_needed.find(parent->x->v.idx);
			return isVector(m_lvars[parent->x->v.idx].type()) ? (found == m_needed.end() ? 0 : found->second)
															 : kAll;
		}
		if (parent->op == cot_call && parent->x->op == cot_helper)
			for (size_t k = 0; k < parent->a->size(); ++k)
				if (&(*parent->a)[k] == e)
				{
					unsigned result = isVector(parent->type) ? needed(parent) : kAll;
					return operandLanes(parent, k, result).value_or(kAll);
				}
		return kAll;
	}

	// a write of the lane is a statement of a block that a later statement of the block holds s in, and no goto
	// jumps past it into the block
	bool writtenBefore(const std::vector<std::pair<const cinsn_t*, int>>& writes, int lane, const cinsn_t* s) const
	{
		for (const auto& [w, l] : writes)
		{
			if (l != lane)
				continue;
			const cinsn_t* block = m_scopes.parent(w);
			if (!block || block->op != cit_block)
				continue;
			size_t index = 0;
			for (const cinsn_t& item : *block->cblock)
			{
				if (&item == w)
					break;
				++index;
			}
			if (m_scopes.jumpedPast(block, index))
				continue;
			size_t at = 0;
			for (const cinsn_t& item : *block->cblock)
				if (at++ > index && m_scopes.inside(s, &item))
					return true;
		}
		return false;
	}

	Function& m_function;
	const lvars_t& m_lvars;
	std::unordered_map<const cexpr_t*, const citem_t*> m_parents;
	Scopes m_scopes;
	std::map<int, unsigned> m_needed;
};

}  // namespace

void resolveVectorLanes(Function& function, Context&)
{
	VectorLanes(function).run();
}

}  // namespace idakit
