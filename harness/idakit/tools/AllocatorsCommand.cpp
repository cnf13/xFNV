// idakit allocators: the table of the exe's allocators and the wrappers that pass a size on to them
// (env/cache/allocators.tsv, `address<TAB>name<TAB>size_arg<TAB>scale`), which emit's --allocators reads.
//
// size_arg is the 0-based index into Hex-Rays' argument list of a call (a thiscall's `this` is argument 0) and scale
// the byte factor the function applies to it (1 for a size, the element size for a count). The seeds are the
// allocators themselves, found by name (an IDA spelling of each, as sync names them; one not found is an error). Then,
// round by round, each new row's direct callers are decompiled: a caller that returns the allocation and passes one
// of its own parameters, never assigned, as the size, unchanged, times a literal or shifted by one, joins the table
// with that parameter's index and the product scale. The rounds end when one adds no row.
//
// An allocator a caller reaches through a vtable, taking the size by reference (kSlotSeeds), is no row: emit's table
// reads a size passed by value. Its callers are the functions that read a global whose type leads to the allocator's
// class (the global, or a field of the record it points to, a pointer to the class); one that returns the allocation
// and passes the address of a local holding one of its own parameters (the parameter copied before anything writes
// it or takes its address; a 0 raised to 1 in between, operator new's rule for a zero size) joins the table with that
// parameter as its size, and its own callers are the next round's.

#include "idakit/Address.hpp"
#include "idakit/Command.hpp"
#include "idakit/Database.hpp"
#include "idakit/Ida.hpp"

#include <algorithm>
#include <cstdio>
#include <fstream>
#include <map>
#include <optional>
#include <set>
#include <vector>

namespace idakit
{

namespace
{

struct Row
{
	std::string name;
	int sizeArg = 0;
	uint64_t scale = 1;
};

struct Seed
{
	// the spellings the database may give it, the first the seed's own name
	std::vector<const char*> names;
	int sizeArg;
	// every function named `Class::<suffix>` (the overrides of a virtual) rather than one by name
	bool overrides = false;
};

// The allocators: operator new, the game's heaps, Gamebryo's and Havok's, the CRT's. calloc's size argument is the
// element size (its count multiplies it at run time). The exe's global operator new is folded into ourMalloc
// (0x401000, MemoryManager::Allocate on the instance), the one function the database has for it.
const std::vector<Seed> kSeeds = {
	{{"??2@YAPAXI@Z", "operator new", "ourMalloc"}, 0},
	{{"MemoryManager::Allocate"}, 1},
	{{"ScrapHeap::Allocate"}, 1},
	{{"NiMalloc", "_NiMalloc"}, 0},
	{{"ourMalloc"}, 0},
	{{"hkMemoryAllocator::blockAlloc"}, 1, true},
	{{"malloc", "_malloc"}, 0},
	{{"_malloc_crt", "::malloc_crt", "__malloc_crt"}, 0},
	{{"calloc", "_calloc"}, 1},
};

// The allocators reached through a vtable that take the size by reference: the class, its virtual (its IDA vtable
// struct's member, `<class>_vtbl`) and the size argument's index (`this` first). Gamebryo's NiAllocator::Allocate
// (size_t& stSizeInBytes, size_t& stAlignment, NiMemEventType, bool, const char*, int, const char*): src's
// Gamebryo2.2/CoreLibs/NiSystem/NiAllocator.h.
struct SlotSeed
{
	const char* cls;
	const char* member;
	int sizeArg;
};

const std::vector<SlotSeed> kSlotSeeds = {
	{"NiAllocator", "Allocate", 1},
};

std::string nameAt(ea_t ea)
{
	qstring name;
	get_func_name(&name, ea);
	return name.c_str();
}

// The functions named `X::member` for the member of a qualified name (`hkMemoryAllocator::blockAlloc`: every
// `X::blockAlloc`)
std::vector<ea_t> overridesOf(const std::string& qualified)
{
	std::string member = qualified.substr(qualified.rfind("::"));
	std::vector<ea_t> out;
	for (size_t n = 0; n < get_func_qty(); ++n)
	{
		func_t* f = getn_func(n);
		std::string name = f ? nameAt(f->start_ea) : "";
		if (name.size() > member.size() && name.compare(name.size() - member.size(), member.size(), member) == 0)
			out.push_back(f->start_ea);
	}
	return out;
}

const cexpr_t* uncast(const cexpr_t* e)
{
	while (e->op == cot_cast)
		e = e->x;
	return e;
}

// The literal of a numeric expression, nullopt for any other
std::optional<uint64_t> literal(const cexpr_t* e)
{
	e = uncast(e);
	if (e->op != cot_num)
		return std::nullopt;
	return e->numval();
}

// What a caller does with one call of a table function: the size argument as one of its parameters times a factor,
// and whether it returns the call's result
struct Use
{
	int param = -1;     // the caller's parameter index, -1 when the size is no parameter (or not unchanged)
	uint64_t factor = 0;
	bool returned = false;
};

// the slot seed a callee reads from an object's vtable (`p->vftable->member`, `p->vftable->Base.member`: the vtable
// pointer of IDA's `<class>_vtbl` struct, the member at the slot's offset in the struct it is read from), nullptr for
// any other callee
const SlotSeed* slotSeed(const cexpr_t* callee)
{
	if (callee->op != cot_memptr && callee->op != cot_memref)
		return nullptr;
	tinfo_t holder = callee->op == cot_memptr ? callee->x->type.get_pointed_object() : callee->x->type;
	udm_t slot;
	slot.offset = uint64_t(callee->m) * 8;
	if (holder.find_udm(&slot, STRMEM_OFFSET) < 0)
		return nullptr;
	const cexpr_t* read = callee;
	while (read->op == cot_memref)
		read = read->x;
	if (read->op != cot_memptr)
		return nullptr;
	// the vtable pointer: the object's member at 0
	const cexpr_t* table = read->x;
	qstring tableName;
	if ((table->op != cot_memptr && table->op != cot_memref) || table->m != 0 || !table->type.is_ptr() ||
		!table->type.get_pointed_object().get_type_name(&tableName))
		return nullptr;
	for (const SlotSeed& seed : kSlotSeeds)
		if (tableName == (std::string(seed.cls) + "_vtbl").c_str() && slot.name == seed.member)
			return &seed;
	return nullptr;
}

struct Finder : ctree_visitor_t
{
	Finder(cfunc_t* cfunc, const std::map<ea_t, Row>& table) : ctree_visitor_t(CV_PARENTS), m_cfunc(cfunc), m_table(table)
	{
		for (size_t k = 0; k < cfunc->argidx.size(); ++k)
			m_params[cfunc->argidx[k]] = int(k);
		size_t index = 0;
		for (const cinsn_t& statement : *cfunc->body.cblock)
			m_statements[&statement] = index++;
	}

	int idaapi visit_expr(cexpr_t* e) override
	{
		// a parameter written anywhere, or whose address is taken, is not passed on unchanged
		bool writes = (e->op == cot_asg || (e->op >= cot_asgbor && e->op <= cot_asgumod) || e->op == cot_preinc ||
					   e->op == cot_predec || e->op == cot_postinc || e->op == cot_postdec || e->op == cot_ref) &&
					  e->x->op == cot_var;
		if (writes)
		{
			m_written.insert(e->x->v.idx);
			m_touched[e->x->v.idx].push_back(statement());
		}
		if (e->op == cot_asg && e->x->op == cot_var)
			m_assignments[e->x->v.idx].push_back({e->y, statement(), guard()});
		if (e->op == cot_asg && e->x->op == cot_var && uncast(e->y)->op == cot_call)
			m_assigned[uncast(e->y)] = e->x->v.idx;
		if (e->op != cot_call)
			return 0;
		if (const SlotSeed* seed = slotSeed(e->x))
		{
			const cexpr_t* size = seed->sizeArg < int(e->a->size()) ? uncast(&e->a->at(seed->sizeArg)) : nullptr;
			if (size && size->op == cot_ref && size->x->op == cot_var)
				m_pointed.push_back({e, size->x->v.idx});
			return 0;
		}
		if (e->x->op != cot_obj)
			return 0;
		auto row = m_table.find(e->x->obj_ea);
		if (row == m_table.end())
			return 0;
		Use use;
		if (row->second.sizeArg < int(e->a->size()))
			use = sizeUse(&e->a->at(row->second.sizeArg), row->second.scale);
		m_calls.push_back({e, use});
		return 0;
	}

	int idaapi visit_insn(cinsn_t* i) override
	{
		if (i->op == cit_return)
		{
			const cexpr_t* value = uncast(&i->creturn->expr);
			if (value->op == cot_call)
				m_returnedCalls.insert(value);
			else if (value->op == cot_var)
				m_returnedVars.insert(value->v.idx);
		}
		return 0;
	}

	// the index in the body of the statement the visited item is in
	size_t statement() const
	{
		for (size_t k = 0; k + 1 < parents.size(); ++k)
			if (parents[k] == &m_cfunc->body)
				if (auto found = m_statements.find(static_cast<const cinsn_t*>(parents[k + 1]));
					found != m_statements.end())
					return found->second;
		return 0;
	}

	// the condition of the `if` whose then-branch holds the visited item (the nearest one), nullptr for none (the
	// root's parent, apply_to's, is null)
	const cexpr_t* guard() const
	{
		for (size_t k = parents.size(); k-- > 0;)
			if (const citem_t* p = parents[k]; p && p->op == cit_if)
			{
				const cif_t* branch = static_cast<const cinsn_t*>(p)->cif;
				return k + 1 < parents.size() && parents[k + 1] == branch->ithen ? &branch->expr : nullptr;
			}
		return nullptr;
	}

	Use sizeUse(const cexpr_t* arg, uint64_t scale) const
	{
		Use use;
		const cexpr_t* e = uncast(arg);
		uint64_t factor = 1;
		if (e->op == cot_mul)
		{
			if (std::optional<uint64_t> k = literal(e->y))
				factor = *k, e = uncast(e->x);
			else if (std::optional<uint64_t> k = literal(e->x))
				factor = *k, e = uncast(e->y);
			else
				return use;
		}
		else if (e->op == cot_shl)
		{
			std::optional<uint64_t> k = literal(e->y);
			if (!k || *k >= 32)
				return use;
			factor = uint64_t(1) << *k;
			e = uncast(e->x);
		}
		if (e->op != cot_var)
			return use;
		auto param = m_params.find(e->v.idx);
		if (param == m_params.end())
			return use;
		use.param = param->second;
		use.factor = factor * scale;
		return use;
	}

	// `x == 0` or `!x`, x a local: its index, else -1
	static int zeroTest(const cexpr_t* condition)
	{
		condition = uncast(condition);
		const cexpr_t* tested = condition->op == cot_lnot ? condition->x
							  : condition->op == cot_eq && literal(condition->y) == 0 ? condition->x
																					   : nullptr;
		tested = tested ? uncast(tested) : nullptr;
		return tested && tested->op == cot_var ? tested->v.idx : -1;
	}

	// The parameter (its argument index) a local holds, nullopt for none: each of its assignments is one parameter's
	// value or another local's that holds it, aside from a read of a local nothing assigns (the slot's initial
	// value: a register pushed to reserve it) and the literal 1 under `if (!x)`, x the value or the local (operator
	// new's zero size); the parameter is written and its address taken only in statements after the copy.
	std::optional<int> held(int local, int depth = 0) const
	{
		auto found = m_assignments.find(local);
		if (found == m_assignments.end() || depth > 3)
			return std::nullopt;
		const Assignment* copy = nullptr;
		std::vector<const Assignment*> ones;
		for (const Assignment& a : found->second)
		{
			const cexpr_t* value = uncast(a.value);
			if (value->op == cot_var && !m_params.contains(value->v.idx) && !m_assignments.contains(value->v.idx))
				continue;
			if (literal(value) == 1 && a.guard)
			{
				ones.push_back(&a);
				continue;
			}
			if (value->op != cot_var || (copy && uncast(copy->value)->v.idx != value->v.idx))
				return std::nullopt;
			copy = &a;
		}
		if (!copy)
			return std::nullopt;
		int source = uncast(copy->value)->v.idx;
		for (const Assignment* one : ones)
			if (int tested = zeroTest(one->guard); tested != source && tested != local)
				return std::nullopt;
		auto param = m_params.find(source);
		if (param == m_params.end())
			return held(source, depth + 1);
		if (auto touched = m_touched.find(source); touched != m_touched.end())
			for (size_t at : touched->second)
				if (at <= copy->statement)
					return std::nullopt;
		return param->second;
	}

	// Each call's use, its parameter dropped where the caller writes it, `returned` where a return yields it; a slot
	// seed's call: the parameter the local whose address it passes holds
	std::vector<Use> uses() const
	{
		std::vector<Use> out;
		auto returned = [&](const cexpr_t* call) {
			auto var = m_assigned.find(call);
			return m_returnedCalls.contains(call) || (var != m_assigned.end() && m_returnedVars.contains(var->second));
		};
		for (auto [call, use] : m_calls)
		{
			if (use.param >= 0 && m_written.contains(m_cfunc->argidx[use.param]))
				use.param = -1;
			use.returned = returned(call);
			out.push_back(use);
		}
		for (auto [call, local] : m_pointed)
		{
			Use use;
			if (std::optional<int> param = m_params.contains(local) ? std::nullopt : held(local))
				use.param = *param, use.factor = 1;
			use.returned = returned(call);
			out.push_back(use);
		}
		return out;
	}

	struct Assignment
	{
		const cexpr_t* value;
		size_t statement;
		const cexpr_t* guard;  // the condition of the if whose then-branch holds it
	};

	cfunc_t* m_cfunc;
	const std::map<ea_t, Row>& m_table;
	std::map<int, int> m_params;  // lvar index -> argument index
	std::map<const cinsn_t*, size_t> m_statements;  // the body's statements, in order
	std::set<int> m_written;
	std::map<int, std::vector<size_t>> m_touched;  // lvar index -> the statements writing it or taking its address
	std::map<int, std::vector<Assignment>> m_assignments;
	std::map<const cexpr_t*, int> m_assigned;
	std::set<const cexpr_t*> m_returnedCalls;
	std::set<int> m_returnedVars;
	std::vector<std::pair<const cexpr_t*, Use>> m_calls;
	std::vector<std::pair<const cexpr_t*, int>> m_pointed;  // a slot seed's call, the local whose address it passes
};

// The functions that call one of the targets directly
std::set<ea_t> callersOf(const std::vector<ea_t>& targets)
{
	std::set<ea_t> out;
	for (ea_t target : targets)
	{
		xrefblk_t xb;
		for (bool ok = xb.first_to(target, XREF_NOFLOW); ok; ok = xb.next_to())
			if (xb.iscode && (xb.type == fl_CN || xb.type == fl_CF || xb.type == fl_JN || xb.type == fl_JF))
				if (func_t* f = get_func(xb.from))
					out.insert(f->start_ea);
	}
	return out;
}

// The functions that read a global whose type leads to the class: a pointer to it, or a pointer to a record one of
// whose fields points to it (the callers that can reach a slot seed's object)
std::set<ea_t> holderReaders(const char* cls)
{
	auto pointsTo = [&](const tinfo_t& type) {
		qstring name;
		return type.is_ptr() && type.get_pointed_object().get_type_name(&name) && name == cls;
	};
	auto leads = [&](const tinfo_t& type) {
		if (pointsTo(type))
			return true;
		udt_type_data_t record;
		if (!type.is_ptr() || !type.get_pointed_object().get_udt_details(&record))
			return false;
		return std::any_of(record.begin(), record.end(), [&](const udm_t& field) { return pointsTo(field.type); });
	};
	std::set<ea_t> out;
	for (size_t n = 0; n < get_nlist_size(); ++n)
	{
		ea_t ea = get_nlist_ea(n);
		tinfo_t type;
		if (get_func(ea) || !get_tinfo(&type, ea) || !leads(type))
			continue;
		xrefblk_t xb;
		for (bool ok = xb.first_to(ea, XREF_DATA); ok; ok = xb.next_to())
			if (func_t* f = get_func(xb.from))
				out.insert(f->start_ea);
	}
	return out;
}

}  // namespace

int runAllocators(Args& args)
{
	std::string idb = args.required("idb");
	std::string out = args.option("out").value_or("env/cache/allocators.tsv");
	Database::Options options;
	options.hexrays = true;
	Database database(idb, options);

	std::map<ea_t, Row> table;
	std::vector<ea_t> added;
	for (const Seed& seed : kSeeds)
	{
		std::vector<ea_t> found;
		if (seed.overrides)
			found = overridesOf(seed.names.front());
		else
			for (const char* name : seed.names)
				if (ea_t ea = get_name_ea(BADADDR, name); ea != BADADDR && get_func(ea))
				{
					found.push_back(get_func(ea)->start_ea);
					break;
				}
		if (found.empty())
			throw Error(std::string("allocators: no function named ") + seed.names.front());
		for (ea_t ea : found)
			if (table.emplace(ea, Row{nameAt(ea), seed.sizeArg, 1}).second)
				added.push_back(ea);
	}
	std::printf("idakit allocators: round 0: %zu rows (seeds)\n", table.size());
	// a slot seed's callers join round 1: the readers of the globals that reach its class
	std::set<ea_t> slotCallers;
	for (const SlotSeed& seed : kSlotSeeds)
		slotCallers.merge(holderReaders(seed.cls));

	for (int round = 1; !added.empty() || !slotCallers.empty(); ++round)
	{
		std::set<ea_t> callers = callersOf(added);
		callers.merge(slotCallers);
		slotCallers.clear();
		added.clear();
		size_t failed = 0;
		for (ea_t caller : callers)
		{
			if (table.contains(caller))
				continue;
			hexrays_failure_t failure;
			cfuncptr_t cfunc(nullptr);
			try
			{
				cfunc = decompile_func(get_func(caller), &failure, DECOMP_NO_WAIT | DECOMP_NO_CACHE);
			}
			catch (const vd_failure_t&)
			{
			}
			if (cfunc == nullptr)
			{
				++failed;
				continue;
			}
			Finder finder(cfunc, table);
			finder.apply_to(&cfunc->body, nullptr);
			// every returned call agrees on one parameter and scale
			std::optional<std::pair<int, uint64_t>> agreed;
			bool conflict = false;
			for (const Use& use : finder.uses())
			{
				if (!use.returned)
					continue;
				if (use.param < 0 || (agreed && *agreed != std::pair{use.param, use.factor}))
				{
					conflict = true;
					break;
				}
				agreed = std::pair{use.param, use.factor};
			}
			if (conflict || !agreed || agreed->second == 0)
				continue;
			table.emplace(caller, Row{nameAt(caller), agreed->first, agreed->second});
			added.push_back(caller);
		}
		std::printf("idakit allocators: round %d: %zu callers decompiled (%zu failed), %zu rows added, %zu rows\n",
					round, callers.size(), failed, added.size(), table.size());
		std::fflush(stdout);
	}

	std::ofstream file(out);
	if (!file)
		throw Error("allocators: cannot write " + out);
	file << "address\tname\tsize_arg\tscale\n";
	for (const auto& [ea, row] : table)
		file << hexText(ea) << '\t' << row.name << '\t' << row.sizeArg << '\t' << row.scale << '\n';
	std::printf("idakit allocators: %zu rows -> %s\n", table.size(), out.c_str());
	return 0;
}

}  // namespace idakit
