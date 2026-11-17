#include "Pdb.hpp"

#include <llvm/DebugInfo/CodeView/CVTypeVisitor.h>
#include <llvm/DebugInfo/CodeView/LazyRandomTypeCollection.h>
#include <llvm/DebugInfo/CodeView/SymbolDeserializer.h>
#include <llvm/DebugInfo/CodeView/SymbolRecord.h>
#include <llvm/DebugInfo/CodeView/TypeDeserializer.h>
#include <llvm/DebugInfo/CodeView/TypeRecord.h>
#include <llvm/DebugInfo/CodeView/TypeVisitorCallbacks.h>
#include <llvm/DebugInfo/MSF/MappedBlockStream.h>
#include <llvm/DebugInfo/PDB/IPDBSession.h>
#include <llvm/DebugInfo/PDB/Native/DbiModuleDescriptor.h>
#include <llvm/DebugInfo/PDB/Native/DbiModuleList.h>
#include <llvm/DebugInfo/PDB/Native/DbiStream.h>
#include <llvm/DebugInfo/PDB/Native/ModuleDebugStream.h>
#include <llvm/DebugInfo/PDB/Native/NativeSession.h>
#include <llvm/DebugInfo/PDB/Native/PDBFile.h>
#include <llvm/DebugInfo/PDB/Native/SymbolStream.h>
#include <llvm/DebugInfo/PDB/Native/TpiStream.h>
#include <llvm/DebugInfo/PDB/PDB.h>

#include <cctype>
#include <map>
#include <optional>
#include <set>
#include <unordered_map>
#include <vector>

namespace cv = llvm::codeview;
namespace pdb = llvm::pdb;

namespace srcdb
{

namespace
{

// a stream the PDB must have
template <class Stream>
Stream& take(llvm::Expected<Stream&> stream, const char* what)
{
	if (!stream)
		throw Error(std::string("srcdb: the PDB's ") + what + ": " + llvm::toString(stream.takeError()));
	return *stream;
}

// a type record LLVM cannot read is skipped
template <class Record>
std::optional<Record> deserialize(const cv::CVType& type)
{
	Record record(static_cast<cv::TypeRecordKind>(type.kind()));
	if (llvm::Error error = cv::TypeDeserializer::deserializeAs<Record>(const_cast<cv::CVType&>(type), record))
	{
		llvm::consumeError(std::move(error));
		return std::nullopt;
	}
	return record;
}

// a symbol record LLVM cannot read is skipped
template <class Symbol>
std::optional<Symbol> readSymbol(const cv::CVSymbol& symbol)
{
	llvm::Expected<Symbol> record = cv::SymbolDeserializer::deserializeAs<Symbol>(symbol);
	if (!record)
	{
		llvm::consumeError(record.takeError());
		return std::nullopt;
	}
	return std::move(*record);
}

// A type row (Model.hpp's type table); unset columns are NULL.
struct TypeRow
{
	const char* kind;
	std::optional<std::string> name, of, cc, ret, cls;
	std::optional<int64_t> count, size, isSigned, isFloat, variadic;
	std::vector<std::string> params;
};

struct Builtin
{
	const char* name;
	int64_t size;
	bool isSigned, isFloat;
};

// CodeView's simple types as clang spells the PC's builtins
std::optional<Builtin> builtin(cv::SimpleTypeKind kind)
{
	using K = cv::SimpleTypeKind;
	switch (kind)
	{
	case K::Void:
		return Builtin{"void", 0, false, false};
	case K::SignedCharacter:
	case K::NarrowCharacter:
		return Builtin{"char", 1, true, false};
	case K::SByte:
		return Builtin{"signed char", 1, true, false};
	case K::UnsignedCharacter:
	case K::Byte:
		return Builtin{"unsigned char", 1, false, false};
	case K::WideCharacter:
		return Builtin{"wchar_t", 2, false, false};
	case K::Int16Short:
	case K::Int16:
		return Builtin{"short", 2, true, false};
	case K::UInt16Short:
	case K::UInt16:
		return Builtin{"unsigned short", 2, false, false};
	case K::Int32Long:
	case K::HResult:
		return Builtin{"long", 4, true, false};
	case K::UInt32Long:
		return Builtin{"unsigned long", 4, false, false};
	case K::Int32:
		return Builtin{"int", 4, true, false};
	case K::UInt32:
		return Builtin{"unsigned int", 4, false, false};
	case K::Int64Quad:
	case K::Int64:
		return Builtin{"long long", 8, true, false};
	case K::UInt64Quad:
	case K::UInt64:
		return Builtin{"unsigned long long", 8, false, false};
	case K::Float32:
		return Builtin{"float", 4, true, true};
	case K::Float64:
		return Builtin{"double", 8, true, true};
	case K::Float80:
		return Builtin{"long double", 8, true, true};
	case K::Boolean8:
		return Builtin{"bool", 1, false, false};
	default:
		return std::nullopt;
	}
}

// The PDB's types under the model's keys, each written once (INSERT OR IGNORE: src's rows of the same key stay).
class Types
{
public:
	Types(Store& store, pdb::TpiStream& tpi)
		: m_tpi(tpi), m_types(tpi.typeCollection()),
		  m_insert(store.handle(), "INSERT OR IGNORE INTO type (key, kind, name, of, count, size, signed, float, cc, "
								   "ret, variadic, const, class) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)"),
		  m_insertParam(store.handle(), "INSERT OR IGNORE INTO type_param (type, idx, param) VALUES (?, ?, ?)")
	{
	}

	std::string key(cv::TypeIndex index)
	{
		auto found = m_keys.find(index.getIndex());
		if (found != m_keys.end())
			return found->second;
		std::string key = build(index);
		m_keys.emplace(index.getIndex(), key);
		return key;
	}

	// The function type's key, `this` left out, and the number of its parameters.
	std::pair<std::string, size_t> function(cv::TypeIndex index)
	{
		std::string key = this->key(index);
		auto found = m_parameterCounts.find(key);
		return {key, found == m_parameterCounts.end() ? 0 : found->second};
	}

	// The class a member function type belongs to; "" for a free function or another type.
	std::string owner(cv::TypeIndex index)
	{
		if (index.isSimple())
			return "";
		if (auto member = deserialize<cv::MemberFunctionRecord>(m_types.getType(index)))
			return name(member->getClassType());
		return "";
	}

	bool isStatic(cv::TypeIndex index)
	{
		if (index.isSimple())
			return false;
		auto member = deserialize<cv::MemberFunctionRecord>(m_types.getType(index));
		return member && member->getThisType().isNoneType();
	}

private:
	std::string node(const std::string& key, TypeRow row)
	{
		auto text = [](Statement& s, int i, const std::optional<std::string>& v) -> Statement& {
			return v ? s.bind(i, *v) : s.bindNull(i);
		};
		auto number = [](Statement& s, int i, const std::optional<int64_t>& v) -> Statement& {
			return v ? s.bind(i, *v) : s.bindNull(i);
		};
		m_insert.bind(1, key).bind(2, std::string(row.kind));
		text(m_insert, 3, row.name);
		text(m_insert, 4, row.of);
		number(m_insert, 5, row.count);
		number(m_insert, 6, row.size);
		number(m_insert, 7, row.isSigned);
		number(m_insert, 8, row.isFloat);
		text(m_insert, 9, row.cc);
		text(m_insert, 10, row.ret);
		number(m_insert, 11, row.variadic);
		number(m_insert, 12, row.kind == std::string_view("fn") ? std::optional<int64_t>(0) : std::nullopt);
		text(m_insert, 13, row.cls);
		m_insert.run();
		for (size_t k = 0; k < row.params.size(); ++k)
			m_insertParam.bind(1, key).bind(2, static_cast<int64_t>(k)).bind(3, row.params[k]).run();
		return key;
	}

	std::string wrap(const char* kind, const std::string& of)
	{
		return node(std::string(kind) + "(" + of + ")", {.kind = kind, .of = of});
	}

	std::string name(cv::TypeIndex index)
	{
		std::string key = this->key(index);
		size_t colon = key.find(':');
		return colon == std::string::npos ? key : key.substr(colon + 1);
	}

	std::optional<int64_t> sizeOf(cv::TypeIndex index)
	{
		if (index.isSimple())
		{
			if (index.getSimpleMode() != cv::SimpleTypeMode::Direct)
				return 4;
			std::optional<Builtin> b = builtin(index.getSimpleKind());
			return b ? std::optional<int64_t>(b->size) : std::nullopt;
		}
		cv::CVType type = m_types.getType(index);
		switch (type.kind())
		{
		case cv::LF_POINTER:
			return 4;
		case cv::LF_MODIFIER:
			if (auto modifier = deserialize<cv::ModifierRecord>(type))
				return sizeOf(modifier->getModifiedType());
			return std::nullopt;
		case cv::LF_ARRAY:
			if (auto array = deserialize<cv::ArrayRecord>(type))
				return static_cast<int64_t>(array->getSize());
			return std::nullopt;
		case cv::LF_CLASS:
		case cv::LF_STRUCTURE:
		case cv::LF_INTERFACE:
		case cv::LF_UNION:
		{
			// a forward reference's size is its full declaration's
			llvm::Expected<cv::TypeIndex> full = m_tpi.findFullDeclForForwardRef(index);
			cv::TypeIndex defined = full ? *full : index;
			if (!full)
				llvm::consumeError(full.takeError());
			cv::CVType definition = m_types.getType(defined);
			if (definition.kind() == cv::LF_UNION)
			{
				auto u = deserialize<cv::UnionRecord>(definition);
				return u ? std::optional<int64_t>(static_cast<int64_t>(u->getSize())) : std::nullopt;
			}
			auto c = deserialize<cv::ClassRecord>(definition);
			return c ? std::optional<int64_t>(static_cast<int64_t>(c->getSize())) : std::nullopt;
		}
		case cv::LF_ENUM:
			if (auto e = deserialize<cv::EnumRecord>(type))
				return sizeOf(e->getUnderlyingType());
			return std::nullopt;
		default:
			return std::nullopt;
		}
	}

	// A procedure's convention as the PC's: the Xbox's PPC convention is each kind's default (cdecl, thiscall).
	static const char* convention(cv::CallingConvention cc, bool member)
	{
		switch (cc)
		{
		case cv::CallingConvention::NearStdCall:
			return "stdcall";
		case cv::CallingConvention::NearFast:
			return "fastcall";
		case cv::CallingConvention::ThisCall:
			return "thiscall";
		default:
			return member ? "thiscall" : "cdecl";
		}
	}

	std::string functionKey(const char* cc, cv::TypeIndex ret, cv::TypeIndex arguments)
	{
		TypeRow row{.kind = "fn", .cc = std::string(cc), .ret = key(ret)};
		std::vector<cv::TypeIndex> indices;
		if (!arguments.isSimple())
			if (auto list = deserialize<cv::ArgListRecord>(m_types.getType(arguments)))
				indices = list->getIndices();
		// a trailing `no type` is CodeView's `...`
		bool variadic = !indices.empty() && indices.back().isNoneType();
		if (variadic)
			indices.pop_back();
		std::string text = std::string("fn(") + cc + ";" + *row.ret + ";";
		for (size_t k = 0; k < indices.size(); ++k)
		{
			row.params.push_back(key(indices[k]));
			text += (k ? "," : "") + row.params.back();
		}
		if (variadic)
			text += indices.empty() ? "..." : ",...";
		text += ";)";
		row.variadic = variadic;
		size_t count = indices.size();
		std::string result = node(text, std::move(row));
		m_parameterCounts[result] = count;
		return result;
	}

	std::string build(cv::TypeIndex index)
	{
		if (index.isSimple())
		{
			std::optional<Builtin> b = builtin(index.getSimpleKind());
			std::string of = b ? node(std::string("builtin:") + b->name,
									  {.kind = "builtin", .name = std::string(b->name),
									   .size = b->size ? std::optional<int64_t>(b->size) : std::nullopt,
									   .isSigned = b->size ? std::optional<int64_t>(b->isSigned) : std::nullopt,
									   .isFloat = b->size ? std::optional<int64_t>(b->isFloat) : std::nullopt})
							   : "unknown:" + std::string(cv::TypeIndex::simpleTypeName(index).str());
			return index.getSimpleMode() == cv::SimpleTypeMode::Direct ? of : wrap("ptr", of);
		}
		cv::CVType type = m_types.getType(index);
		switch (type.kind())
		{
		case cv::LF_POINTER:
		{
			auto pointer = deserialize<cv::PointerRecord>(type);
			if (!pointer)
				break;
			std::string of = key(pointer->getReferentType());
			std::string result;
			switch (pointer->getMode())
			{
			case cv::PointerMode::LValueReference:
				result = wrap("lref", of);
				break;
			case cv::PointerMode::RValueReference:
				result = wrap("rref", of);
				break;
			case cv::PointerMode::PointerToDataMember:
			case cv::PointerMode::PointerToMemberFunction:
			{
				std::string cls = key(pointer->getMemberInfo().getContainingType());
				result = node("memptr(" + cls + "," + of + ")", {.kind = "memptr", .of = of, .cls = cls});
				break;
			}
			default:
				result = wrap("ptr", of);
			}
			if (pointer->isVolatile())
				result = wrap("volatile", result);
			if (pointer->isConst())
				result = wrap("const", result);
			return result;
		}
		case cv::LF_MODIFIER:
		{
			auto modifier = deserialize<cv::ModifierRecord>(type);
			if (!modifier)
				break;
			std::string result = key(modifier->getModifiedType());
			auto options = modifier->getModifiers();
			if ((options & cv::ModifierOptions::Volatile) != cv::ModifierOptions::None)
				result = wrap("volatile", result);
			if ((options & cv::ModifierOptions::Const) != cv::ModifierOptions::None)
				result = wrap("const", result);
			return result;
		}
		case cv::LF_ARRAY:
		{
			auto array = deserialize<cv::ArrayRecord>(type);
			if (!array)
				break;
			std::string of = key(array->getElementType());
			std::optional<int64_t> element = sizeOf(array->getElementType());
			int64_t count = element && *element ? static_cast<int64_t>(array->getSize()) / *element : -1;
			return node("array(" + of + "," + std::to_string(count) + ")", {.kind = "array", .of = of, .count = count});
		}
		case cv::LF_CLASS:
		case cv::LF_STRUCTURE:
		case cv::LF_INTERFACE:
			if (auto record = deserialize<cv::ClassRecord>(type))
				return node("record:" + record->getName().str(), {.kind = "record", .name = record->getName().str()});
			break;
		case cv::LF_UNION:
			if (auto record = deserialize<cv::UnionRecord>(type))
				return node("record:" + record->getName().str(), {.kind = "record", .name = record->getName().str()});
			break;
		case cv::LF_ENUM:
			if (auto record = deserialize<cv::EnumRecord>(type))
				return node("enum:" + record->getName().str(), {.kind = "enum", .name = record->getName().str()});
			break;
		case cv::LF_PROCEDURE:
			if (auto procedure = deserialize<cv::ProcedureRecord>(type))
				return functionKey(convention(procedure->getCallConv(), false), procedure->getReturnType(),
								   procedure->getArgumentList());
			break;
		case cv::LF_MFUNCTION:
			if (auto member = deserialize<cv::MemberFunctionRecord>(type))
				return functionKey(convention(member->getCallConv(), true), member->getReturnType(),
								   member->getArgumentList());
			break;
		default:
			break;
		}
		return "unknown:" + std::to_string(index.getIndex());
	}

	pdb::TpiStream& m_tpi;
	cv::LazyRandomTypeCollection& m_types;
	Statement m_insert, m_insertParam;
	std::unordered_map<uint32_t, std::string> m_keys;
	std::unordered_map<std::string, size_t> m_parameterCounts;
};

// One procedure symbol being read: its parameters' names come from the first S_REGISTER / S_REGREL32 / S_BPREL32
// records before the first nested block or its end (the compiler emits the parameters first; the Xbox build keeps
// many in registers, S_REGISTER, and a frame local such as Havok's `assertBuf` follows them). Its variables are every
// such record (and S_LOCAL) up to its S_END, nested blocks included, parameters and `this` too.
struct Procedure
{
	std::string name;
	cv::TypeIndex type;
	int64_t codeSize = 0;
	size_t wanted = 0;  // the function type's parameters, `this` aside
	std::vector<std::string> params;
	std::vector<std::pair<std::string, cv::TypeIndex>> variables;
	int depth = 1;		  // the scopes open: the procedure's own and the blocks inside it
	bool nested = false;  // a nested block began: no more parameters

	void variable(llvm::StringRef local, cv::TypeIndex index)
	{
		if (local != "this" && !nested && params.size() < wanted)
			params.push_back(local.str());
		variables.emplace_back(local.str(), index);
	}
};

// The members of a class's LF_FIELDLIST and the lists it continues into: each data member's name and type, and each
// static data member's.
class Members : public cv::TypeVisitorCallbacks
{
public:
	using cv::TypeVisitorCallbacks::visitKnownMember;

	explicit Members(cv::LazyRandomTypeCollection& types) : m_types(types) {}

	void read(cv::TypeIndex list)
	{
		for (int depth = 0; !list.isSimple() && !list.isNoneType() && depth < 64; ++depth)
		{
			m_continuation = cv::TypeIndex();
			std::optional<cv::FieldListRecord> fields = deserialize<cv::FieldListRecord>(m_types.getType(list));
			if (!fields)
				return;
			if (llvm::Error error = cv::visitMemberRecordStream(fields->Data, *this))
			{
				llvm::consumeError(std::move(error));
				return;
			}
			list = m_continuation;
		}
	}

	llvm::Error visitKnownMember(cv::CVMemberRecord&, cv::DataMemberRecord& record) override
	{
		data.emplace_back(record.getName().str(), record.getType());
		return llvm::Error::success();
	}
	llvm::Error visitKnownMember(cv::CVMemberRecord&, cv::StaticDataMemberRecord& record) override
	{
		statics.emplace_back(record.getName().str(), record.getType());
		return llvm::Error::success();
	}
	llvm::Error visitKnownMember(cv::CVMemberRecord&, cv::ListContinuationRecord& record) override
	{
		m_continuation = record.getContinuationIndex();
		return llvm::Error::success();
	}

	std::vector<std::pair<std::string, cv::TypeIndex>> data, statics;

private:
	cv::LazyRandomTypeCollection& m_types;
	cv::TypeIndex m_continuation;
};

std::unique_ptr<pdb::IPDBSession> openPdb(const std::string& path)
{
	std::unique_ptr<pdb::IPDBSession> session;
	if (llvm::Error error = pdb::loadDataForPDB(pdb::PDB_ReaderType::Native, path, session))
		throw Error("srcdb: cannot read the PDB " + path + ": " + llvm::toString(std::move(error)));
	return session;
}

// a template instantiation's name: an identifier (qualified or not) followed by its argument list
bool instanceName(llvm::StringRef name)
{
	size_t open = name.find('<');
	return open != llvm::StringRef::npos && open > 0 && (std::isalpha(static_cast<unsigned char>(name[0])) ||
														 name[0] == '_');
}

}  // namespace

std::vector<std::string> pdbTemplateInstances(const std::string& path)
{
	std::unique_ptr<pdb::IPDBSession> session = openPdb(path);
	pdb::PDBFile& file = static_cast<pdb::NativeSession&>(*session).getPDBFile();
	pdb::TpiStream& tpi = take(file.getPDBTpiStream(), "type stream");
	std::set<std::string> names;
	for (const cv::CVType& type : tpi.typeArray())
	{
		std::optional<std::string> name;
		if (type.kind() == cv::LF_CLASS || type.kind() == cv::LF_STRUCTURE)
		{
			if (auto record = deserialize<cv::ClassRecord>(type); record && !record->isForwardRef())
				name = record->getName().str();
		}
		else if (type.kind() == cv::LF_UNION)
		{
			if (auto record = deserialize<cv::UnionRecord>(type); record && !record->isForwardRef())
				name = record->getName().str();
		}
		if (name && instanceName(*name))
			names.insert(*name);
	}
	return {names.begin(), names.end()};
}

size_t indexPdb(Store& store, const std::string& path)
{
	std::unique_ptr<pdb::IPDBSession> session = openPdb(path);
	pdb::PDBFile& file = static_cast<pdb::NativeSession&>(*session).getPDBFile();
	pdb::TpiStream& tpi = take(file.getPDBTpiStream(), "type stream");
	pdb::DbiStream& dbi = take(file.getPDBDbiStream(), "DBI stream");
	Types types(store, tpi);
	Statement insertFunction(store.handle(), "INSERT INTO pdb_function (id, name, class, type, static, code_size) "
											 "VALUES (?, ?, ?, ?, ?, ?)");
	Statement insertParam(store.handle(), "INSERT INTO pdb_param (function, idx, name) VALUES (?, ?, ?)");
	Statement insertLocal(store.handle(), "INSERT INTO pdb_local (function, idx, name, type) VALUES (?, ?, ?, ?)");
	Statement insertField(store.handle(), "INSERT OR IGNORE INTO pdb_field (record, name, type) VALUES (?, ?, ?)");
	Statement insertGlobal(store.handle(), "INSERT OR IGNORE INTO pdb_global (name, type) VALUES (?, ?)");
	// one row per name and function type: a function compiled into several modules is the same signature
	std::map<std::pair<std::string, uint32_t>, int64_t> written;
	auto finish = [&](Procedure& procedure) {
		auto [found, added] = written.try_emplace({procedure.name, procedure.type.getIndex()}, written.size() + 1);
		if (!added)
			return;
		int64_t id = found->second;
		std::string owner = types.owner(procedure.type);
		insertFunction.bind(1, id).bind(2, procedure.name);
		owner.empty() ? insertFunction.bindNull(3) : insertFunction.bind(3, owner);
		insertFunction.bind(4, types.key(procedure.type))
			.bind(5, static_cast<int64_t>(types.isStatic(procedure.type)))
			.bind(6, procedure.codeSize)
			.run();
		for (size_t k = 0; k < procedure.params.size(); ++k)
			insertParam.bind(1, id).bind(2, static_cast<int64_t>(k)).bind(3, procedure.params[k]).run();
		for (size_t k = 0; k < procedure.variables.size(); ++k)
			insertLocal.bind(1, id)
				.bind(2, static_cast<int64_t>(k))
				.bind(3, procedure.variables[k].first)
				.bind(4, types.key(procedure.variables[k].second))
				.run();
	};
	store.exec("BEGIN");
	const pdb::DbiModuleList& modules = dbi.modules();
	for (uint32_t m = 0; m < modules.getModuleCount(); ++m)
	{
		pdb::DbiModuleDescriptor descriptor = modules.getModuleDescriptor(m);
		uint16_t index = descriptor.getModuleStreamIndex();
		if (index == pdb::kInvalidStreamIndex)
			continue;
		pdb::ModuleDebugStreamRef module(descriptor, file.createIndexedStream(index));
		if (llvm::Error error = module.reload())
		{
			llvm::consumeError(std::move(error));
			continue;
		}
		std::optional<Procedure> open;
		for (const cv::CVSymbol& symbol : module.symbols(nullptr))
		{
			switch (symbol.kind())
			{
			case cv::S_GPROC32:
			case cv::S_LPROC32:
				if (open)
					finish(*open);
				open.reset();
				if (auto proc = readSymbol<cv::ProcSym>(symbol))
					open = Procedure{proc->Name.str(), proc->FunctionType, proc->CodeSize,
									 types.function(proc->FunctionType).second};
				break;
			case cv::S_REGREL32:
				if (auto local = readSymbol<cv::RegRelativeSym>(symbol); local && open)
					open->variable(local->Name, local->Type);
				break;
			case cv::S_BPREL32:
				if (auto local = readSymbol<cv::BPRelativeSym>(symbol); local && open)
					open->variable(local->Name, local->Type);
				break;
			case cv::S_REGISTER:
				if (auto local = readSymbol<cv::RegisterSym>(symbol); local && open)
					open->variable(local->Name, local->Index);
				break;
			case cv::S_LOCAL:
				if (auto local = readSymbol<cv::LocalSym>(symbol); local && open)
					open->variable(local->Name, local->Type);
				break;
			case cv::S_BLOCK32:
			case cv::S_WITH32:
			case cv::S_SEPCODE:
				if (open)
				{
					++open->depth;
					open->nested = true;
				}
				break;
			case cv::S_END:
				if (open && --open->depth == 0)
				{
					finish(*open);
					open.reset();
				}
				break;
			default:
				break;
			}
		}
		if (open)
			finish(*open);
	}
	// the records' data members (a forward reference has none) and static data members, which are globals
	// (`Class::member`, as the global symbols name a static member's definition)
	for (const cv::CVType& type : tpi.typeArray())
	{
		std::optional<std::string> name;
		cv::TypeIndex list;
		if (type.kind() == cv::LF_CLASS || type.kind() == cv::LF_STRUCTURE || type.kind() == cv::LF_INTERFACE)
		{
			if (auto record = deserialize<cv::ClassRecord>(type); record && !record->isForwardRef())
				name = record->getName().str(), list = record->getFieldList();
		}
		else if (type.kind() == cv::LF_UNION)
		{
			if (auto record = deserialize<cv::UnionRecord>(type); record && !record->isForwardRef())
				name = record->getName().str(), list = record->getFieldList();
		}
		if (!name)
			continue;
		Members members(tpi.typeCollection());
		members.read(list);
		for (const auto& [member, index] : members.data)
			insertField.bind(1, *name).bind(2, member).bind(3, types.key(index)).run();
		for (const auto& [member, index] : members.statics)
			insertGlobal.bind(1, *name + "::" + member).bind(2, types.key(index)).run();
	}
	// the global data symbols: globals, a file's statics, static data members' definitions
	pdb::SymbolStream& globals = take(file.getPDBSymbolStream(), "symbol record stream");
	for (const cv::CVSymbol& symbol : globals.getSymbolArray())
		if (symbol.kind() == cv::S_GDATA32 || symbol.kind() == cv::S_LDATA32)
			if (auto data = readSymbol<cv::DataSym>(symbol))
				insertGlobal.bind(1, data->Name.str()).bind(2, types.key(data->Type)).run();
	store.exec("COMMIT");
	return written.size();
}

}  // namespace srcdb
