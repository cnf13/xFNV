// Thread data: the exe's inlined read of its thread-local block, the TEB's TLS array indexed by the CRT's TLS index,
// as the src accessor it is (BSCore/TLSData.h's GetThreadTLSData, whose hybrid body is that read and whose x64 body
// is a thread_local):
//
//     NtCurrentTeb()->ThreadLocalStoragePointer[_tls_index]  ->  GetThreadTLSData()
//
// The index must be the src global `_tls_index` (the Globals pass's), the TEB member the one at fs:[0x2C], and the
// element IDA's struct of the record the accessor returns a pointer to. Otherwise the read stays as it is (the TEB is
// no src record: marked).

#include "EmitPasses.hpp"

namespace idakit
{

namespace
{

// the TEB's ThreadLocalStoragePointer (fs:[0x2C] on x86)
constexpr uint64_t kTlsArray = 0x2C;

class ThreadData
{
public:
	ThreadData(Function& function, Context& context) : m_function(function), m_context(context)
	{
		for (const SrcModel::Function* f : context.model.functionsNamed("GetThreadTLSData"))
		{
			const SrcModel::Type* type = context.model.type(f->type);
			SrcTypes::Parts ret = type ? context.types.parts(context.types.canonical(type->ret)) : SrcTypes::Parts{};
			if (f->cls.empty() && type && type->params.empty() && ret.kind == "ptr" &&
				context.types.parts(ret.of).kind == "record")
			{
				m_accessor = f;
				m_record = context.types.parts(ret.of).name;
			}
		}
	}

	void run()
	{
		if (!m_accessor)
			return;
		std::vector<const cexpr_t*> found;
		forEachExpression(m_function, [&](const cexpr_t* e, const cinsn_t*) {
			if (threadBlock(e))
				found.push_back(e);
		});
		for (const cexpr_t* e : found)
		{
			// what the layers found inside it (the TEB's member, the helper, the index global) is the accessor's
			forEachSubexpression(e, [&](const cexpr_t* x) { m_function.meaning.erase(x); });
			m_function.meaning[e] = Rewrite{1, "GetThreadTLSData()", {}, {}, "ptr(record:" + m_record + ")",
											m_accessor->file.substr(m_accessor->file.rfind("src/", 0) == 0 ? 4 : 0)};
		}
	}

private:
	// e is `NtCurrentTeb()->ThreadLocalStoragePointer[_tls_index]`, of the accessor's pointer type
	bool threadBlock(const cexpr_t* e) const
	{
		if (e->op != cot_idx || !e->type.is_ptr())
			return false;
		const cexpr_t* array = e->x;
		if (array->op != cot_memptr || array->m != kTlsArray || array->x->op != cot_call ||
			array->x->x->op != cot_helper || std::string(array->x->x->helper) != "NtCurrentTeb" ||
			!array->x->a->empty())
			return false;
		const cexpr_t* index = stripCasts(e->y);
		auto meaning = m_function.meaning.find(index);
		const SrcModel::Ref* const* global =
			index->op == cot_obj && meaning != m_function.meaning.end()
				? std::get_if<const SrcModel::Ref*>(&meaning->second)
				: nullptr;
		if (!global || !*global || (*global)->target != "_tls_index")
			return false;
		const SrcModel::Record* element = m_context.types.recordOf(e->type.get_pointed_object());
		return element && element->name == m_record;
	}

	Function& m_function;
	Context& m_context;
	const SrcModel::Function* m_accessor = nullptr;
	std::string m_record;
};

}  // namespace

void rewriteThreadData(Function& function, Context& context)
{
	ThreadData(function, context).run();
}

}  // namespace idakit
