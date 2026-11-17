#include "Model.hpp"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Attr.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/Mangle.h>
#include <clang/AST/PrettyPrinter.h>
#include <clang/AST/RecordLayout.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/VTableBuilder.h>
#include <clang/Basic/SourceManager.h>
#include <clang/CodeGen/CGFunctionInfo.h>
#include <clang/CodeGen/CodeGenABITypes.h>
#include <clang/CodeGen/ModuleBuilder.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/MacroArgs.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>
#include <clang/Sema/Scope.h>
#include <clang/Sema/Sema.h>
#include <llvm/IR/CallingConv.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/Support/FileSystem.h>

#include <algorithm>
#include <cstring>
#include <map>
#include <optional>
#include <regex>
#include <set>
#include <unordered_map>
#include <variant>

using namespace clang;

namespace srcdb
{

namespace
{

// --- rows ---------------------------------------------------------------------------------------------------------

// One SQLite value: NULL, an integer (booleans 0/1) or text.
using Cell = std::variant<std::monostate, int64_t, std::string>;

Cell flag(bool value)
{
	return static_cast<int64_t>(value);
}

Cell number(int64_t value)
{
	return value;
}

Cell textOrNull(const std::string& text)
{
	return text.empty() ? Cell() : Cell(text);
}

// A function row's provenance until `srcdb index` decides it from the markers and the PDB (Index.cpp).
const Cell kProvenancePending = std::string("src");

struct Row
{
	const char* table;
	std::vector<Cell> cells;
};

// FNV-1a over the cells, tagged by kind: the digest an item's main row carries in its last column.
uint64_t digest(const std::vector<Row>& rows)
{
	uint64_t hash = 1469598103934665603ull;
	auto mix = [&](const void* data, size_t size) {
		for (size_t k = 0; k < size; ++k)
			hash = (hash ^ static_cast<const unsigned char*>(data)[k]) * 1099511628211ull;
	};
	for (const Row& row : rows)
	{
		mix(row.table, std::strlen(row.table) + 1);
		for (const Cell& cell : row.cells)
		{
			unsigned char kind = static_cast<unsigned char>(cell.index());
			mix(&kind, 1);
			if (auto* value = std::get_if<int64_t>(&cell))
				mix(value, sizeof *value);
			else if (auto* text = std::get_if<std::string>(&cell))
				mix(text->c_str(), text->size() + 1);
		}
	}
	return hash;
}

// --- macro invocations --------------------------------------------------------------------------------------------

// An FNVD_ macro invocation in a file: its name and its arguments as written.
struct Expansion
{
	std::string name;
	std::vector<std::string> args;
};

using Expansions = std::unordered_map<SourceLocation::UIntTy, Expansion>;

class MacroLog : public PPCallbacks
{
public:
	MacroLog(Preprocessor& preprocessor, Expansions& expansions)
		: m_preprocessor(preprocessor), m_expansions(expansions)
	{
	}

	void MacroExpands(const Token& nameToken, const MacroDefinition&, SourceRange range,
					  const MacroArgs* args) override
	{
		const IdentifierInfo* name = nameToken.getIdentifierInfo();
		if (!range.getBegin().isFileID() || !name || !name->getName().starts_with("FNVD_"))
			return;
		Expansion expansion{name->getName().str(), {}};
		for (unsigned k = 0; args && k < args->getNumMacroArguments(); ++k)
		{
			std::string text;
			for (const Token* token = args->getUnexpArgument(k); token->isNot(tok::eof); ++token)
			{
				if (!text.empty() && token->hasLeadingSpace())
					text += ' ';
				text += m_preprocessor.getSpelling(*token);
			}
			expansion.args.push_back(std::move(text));
		}
		m_expansions[range.getBegin().getRawEncoding()] = std::move(expansion);
	}

private:
	Preprocessor& m_preprocessor;
	Expansions& m_expansions;
};

// anonymous records print as `(anonymous struct at FILE:LINE:COL)`: FILE repo-relative, so every checkout and
// worktree names them alike
class RelativePaths : public PrintingCallbacks
{
public:
	explicit RelativePaths(std::string root) : m_root(std::move(root)) {}

	std::string remapPath(StringRef path) const override
	{
		llvm::SmallString<256> real;
		if (!llvm::sys::fs::real_path(path, real) && real.str().starts_with(m_root))
			return real.str().substr(m_root.size()).str();
		return path.str();
	}

private:
	std::string m_root;
};

struct FileIdHash
{
	size_t operator()(FileID id) const { return id.getHashValue(); }
};

struct FileInfo
{
	std::string path;  // repo-relative, or the absolute path outside the root; empty: no file (builtins)
	bool collected = false;
	bool main = false;
};

// Who writes an item: Own (this TU's share), Shared (a template instance any TU may complete), External (declared
// outside src: whichever TU sees it first), Skip (another TU's share).
enum class Owner
{
	Skip,
	Own,
	Shared,
	External,
};

// --- clang spellings ----------------------------------------------------------------------------------------------

const char* tagName(TagTypeKind kind)
{
	switch (kind)
	{
	case TagTypeKind::Class:
		return "class";
	case TagTypeKind::Union:
		return "union";
	case TagTypeKind::Enum:
		return "enum";
	case TagTypeKind::Interface:
		return "__interface";
	default:
		return "struct";
	}
}

const char* callingConvention(CallingConv cc)
{
	switch (cc)
	{
	case CC_C:
		return "cdecl";
	case CC_X86StdCall:
		return "stdcall";
	case CC_X86FastCall:
		return "fastcall";
	case CC_X86ThisCall:
		return "thiscall";
	case CC_X86VectorCall:
		return "vectorcall";
	case CC_X86RegCall:
		return "regcall";
	default:
		return "other";
	}
}

const char* effectiveConvention(unsigned cc)
{
	switch (cc)
	{
	case llvm::CallingConv::C:
		return "cdecl";
	case llvm::CallingConv::X86_StdCall:
		return "stdcall";
	case llvm::CallingConv::X86_FastCall:
		return "fastcall";
	case llvm::CallingConv::X86_ThisCall:
		return "thiscall";
	case llvm::CallingConv::X86_VectorCall:
		return "vectorcall";
	default:
		return "other";
	}
}

const char* abiKind(const CodeGen::ABIArgInfo& info)
{
	switch (info.getKind())
	{
	case CodeGen::ABIArgInfo::Direct:
		return "direct";
	case CodeGen::ABIArgInfo::Extend:
		return "extend";
	case CodeGen::ABIArgInfo::Indirect:
		return "indirect";
	case CodeGen::ABIArgInfo::IndirectAliased:
		return "indirect_aliased";
	case CodeGen::ABIArgInfo::Ignore:
		return "ignore";
	case CodeGen::ABIArgInfo::Expand:
		return "expand";
	case CodeGen::ABIArgInfo::CoerceAndExpand:
		return "coerce_and_expand";
	case CodeGen::ABIArgInfo::InAlloca:
		return "inalloca";
	case CodeGen::ABIArgInfo::TargetSpecific:
		return "target_specific";
	}
	return "other";
}

bool hasInReg(const CodeGen::ABIArgInfo& info)
{
	switch (info.getKind())
	{
	case CodeGen::ABIArgInfo::Direct:
	case CodeGen::ABIArgInfo::Extend:
	case CodeGen::ABIArgInfo::Indirect:
		return info.getInReg();
	default:
		return false;
	}
}

// --- AST helpers --------------------------------------------------------------------------------------------------

// A call, construction or brace initialisation's arguments.
std::vector<const Expr*> initArguments(const Expr* init)
{
	std::vector<const Expr*> args;
	if (!init)
		return args;
	init = init->IgnoreImplicit();
	if (auto* cleanups = dyn_cast<ExprWithCleanups>(init))
		init = cleanups->getSubExpr()->IgnoreImplicit();
	if (auto* cast = dyn_cast<CXXFunctionalCastExpr>(init))
		init = cast->getSubExpr()->IgnoreImplicit();
	if (auto* construct = dyn_cast<CXXConstructExpr>(init))
		args.assign(construct->arg_begin(), construct->arg_end());
	else if (auto* list = dyn_cast<InitListExpr>(init))
		args.assign(list->inits().begin(), list->inits().end());
	else if (auto* parens = dyn_cast<ParenListExpr>(init))
		args.assign(parens->exprs().begin(), parens->exprs().end());
	return args;
}

// Empty constructors and destructors: no code (`NiPoint3() {}`), so a local of the class is exact where the exe
// inlines them. A structor is empty when its class has no vptr to store and no virtual base, its body is visible to
// every TU (defined in the class, inline, or defaulted; a template member's pattern) with no statements and no
// written initialisers, and every base and member is constructed (destroyed) trivially or emptily, a member with no
// default initialiser.
bool emptyConstruction(const CXXRecordDecl* record);
bool emptyDestruction(const CXXRecordDecl* record);

// every base and member constructed (destroyed) trivially or emptily
bool emptySubobjects(const CXXRecordDecl* record, bool construction)
{
	auto empty = [&](const CXXRecordDecl* r) {
		return !r || (construction ? emptyConstruction(r) : emptyDestruction(r));
	};
	for (const CXXBaseSpecifier& base : record->bases())
		if (!empty(base.getType()->getAsCXXRecordDecl()))
			return false;
	for (const FieldDecl* field : record->fields())
		if ((construction && field->hasInClassInitializer()) ||
			!empty(field->getType()->getBaseElementTypeUnsafe()->getAsCXXRecordDecl()))
			return false;
	return true;
}

// a user-provided structor's body, as every TU sees it: no statements, no written initialisers
bool emptyBody(const CXXMethodDecl* method)
{
	const FunctionDecl* pattern = method->getTemplateInstantiationPattern();
	const FunctionDecl* definition = (pattern ? pattern : method)->getDefinition();
	if (!definition || !definition->isInlined())
		return false;
	if (auto* constructor = dyn_cast<CXXConstructorDecl>(definition))
		for (const CXXCtorInitializer* init : constructor->inits())
			if (init->isWritten())
				return false;
	auto* body = dyn_cast_or_null<CompoundStmt>(definition->getBody());
	return body && body->body_empty();
}

// the structor itself is empty (its class's subobjects included)
bool emptyStructor(const CXXMethodDecl* method)
{
	const CXXRecordDecl* record = method->getParent()->getDefinition();
	if (!record || record->isDynamicClass() || record->getNumVBases() != 0)
		return false;
	if (!method->isDefaulted() && !emptyBody(method))
		return false;
	return emptySubobjects(record, isa<CXXConstructorDecl>(method));
}

bool emptyConstruction(const CXXRecordDecl* record)
{
	record = record->getDefinition();
	if (!record)
		return false;
	if (record->hasTrivialDefaultConstructor())
		return true;
	for (const CXXConstructorDecl* constructor : record->ctors())
		if (constructor->isDefaultConstructor())
			return emptyStructor(constructor);
	// the implicit default constructor clang has not declared yet
	return !record->isDynamicClass() && record->getNumVBases() == 0 && emptySubobjects(record, true);
}

bool emptyDestruction(const CXXRecordDecl* record)
{
	record = record->getDefinition();
	if (!record)
		return false;
	if (record->hasTrivialDestructor())
		return true;
	if (const CXXDestructorDecl* destructor = record->getDestructor())
		return emptyStructor(destructor);
	return !record->isDynamicClass() && emptySubobjects(record, false);
}

bool inNamespaceFnvd(const Decl* decl)
{
	for (const DeclContext* context = decl->getDeclContext(); context; context = context->getParent())
	{
		auto* space = dyn_cast<NamespaceDecl>(context);
		if (space && space->getParent()->isTranslationUnit())
			return space->getName() == "fnvd";
	}
	return false;
}

// The first function a registration's expression names (`&function`, `static_cast<T>(&function)`), past the
// harness's own helpers.
const FunctionDecl* namedFunction(const Stmt* statement)
{
	if (!statement)
		return nullptr;
	if (auto* ref = dyn_cast<DeclRefExpr>(statement))
	{
		auto* function = dyn_cast<FunctionDecl>(ref->getDecl());
		if (function && !inNamespaceFnvd(function))
			return function;
	}
	for (const Stmt* child : statement->children())
		if (const FunctionDecl* found = namedFunction(child))
			return found;
	return nullptr;
}

const CXXReinterpretCastExpr* reinterpretCast(const Stmt* statement)
{
	if (!statement)
		return nullptr;
	if (auto* cast = dyn_cast<CXXReinterpretCastExpr>(statement))
		return cast;
	for (const Stmt* child : statement->children())
		if (const CXXReinterpretCastExpr* found = reinterpretCast(child))
			return found;
	return nullptr;
}

const CXXConstructExpr* constructExpr(const Stmt* statement)
{
	if (!statement)
		return nullptr;
	if (auto* construct = dyn_cast<CXXConstructExpr>(statement))
		return construct;
	for (const Stmt* child : statement->children())
		if (const CXXConstructExpr* found = constructExpr(child))
			return found;
	return nullptr;
}

bool instantiated(const Decl* decl)
{
	for (const Decl* d = decl; d && !isa<TranslationUnitDecl>(d);
		 d = d->getDeclContext() ? Decl::castFromDeclContext(d->getDeclContext()) : nullptr)
	{
		if (auto* record = dyn_cast<CXXRecordDecl>(d))
		{
			if (isTemplateInstantiation(record->getTemplateSpecializationKind()))
				return true;
		}
		else if (auto* function = dyn_cast<FunctionDecl>(d))
		{
			if (isTemplateInstantiation(function->getTemplateSpecializationKind()))
				return true;
		}
		else if (auto* enumeration = dyn_cast<EnumDecl>(d))
		{
			if (isTemplateInstantiation(enumeration->getTemplateSpecializationKind()))
				return true;
		}
	}
	return false;
}

class Collector;

class Visitor : public RecursiveASTVisitor<Visitor>
{
public:
	explicit Visitor(Collector& collector) : m_collector(collector) {}

	bool shouldVisitTemplateInstantiations() const { return false; }
	bool shouldVisitImplicitCode() const { return false; }

	bool TraverseDecl(Decl* decl);
	bool VisitVarDecl(VarDecl* var);
	bool VisitStaticAssertDecl(StaticAssertDecl* assertion);
	bool VisitCallExpr(CallExpr* call);
	bool VisitRecordDecl(RecordDecl* record);
	bool VisitEnumDecl(EnumDecl* enumeration);
	bool VisitTypedefNameDecl(TypedefNameDecl* alias);
	bool VisitFunctionDecl(FunctionDecl* function);
	bool VisitClassTemplateDecl(ClassTemplateDecl* pattern);
	bool VisitDeclRefExpr(DeclRefExpr* reference);
	bool VisitMemberExpr(MemberExpr* member);

private:
	// The call whose callee (parentheses and implicit casts dropped) is this expression: a use's call (use()).
	const CallExpr* callOf(const Expr* callee) const
	{
		auto found = m_callees.find(callee);
		return found == m_callees.end() ? nullptr : found->second;
	}

	Collector& m_collector;
	std::vector<const FunctionDecl*> m_functions;  // the functions around the current node, innermost last
	// each call's callee, recorded before the traversal reaches it (pre-order)
	std::unordered_map<const Expr*, const CallExpr*> m_callees;
};

// The fields of a type node (the type table's columns after key and kind).
struct TypeNode
{
	const char* kind;
	Cell name, of, count, size, isSigned, isFloat, cc, ret, variadic, isConst, cls;
	std::vector<std::string> params;
};

class Collector : public ASTConsumer
{
public:
	Collector(CompilerInstance& ci, Store& store, const IndexScope& scope, const Expansions& expansions)
		: m_ci(ci), m_store(store), m_scope(scope), m_expansions(expansions), m_paths(scope.root),
		  m_policy(ci.getLangOpts())
	{
		m_codegen = CreateLLVMCodeGen(ci, "srcdb", m_llvm);
	}

	void Initialize(ASTContext& context) override
	{
		m_context = &context;
		m_codegen->Initialize(context);
		m_mangle.reset(context.createMangleContext());
		m_policy = PrintingPolicy(context.getLangOpts());
		m_policy.SuppressTagKeyword = true;
		m_policy.PrintAsCanonical = true;
		m_policy.FullyQualifiedName = true;
		m_policy.Callbacks = &m_paths;
	}

	void HandleTranslationUnit(ASTContext& context) override
	{
		Visitor visitor(*this);
		visitor.TraverseDecl(context.getTranslationUnitDecl());
		drain();
		write();
	}

	// --- what the visitor finds -------------------------------------------------------------------------------

	void registration(const VarDecl* var);
	void addressRef(const StaticAssertDecl* assertion);
	void pin(const StaticAssertDecl* assertion);
	void specialization(const FunctionDecl* function);
	void forwarder(const CallExpr* call, const FunctionDecl* around);
	void foldRef(const FunctionDecl* function);

	// A src function an expression names by its identifier, at the name's spelling (Model.hpp's function_use): a
	// call, its address, a registration's argument. A name spelled in a macro's body is the macro's, not src's text.
	// When the expression is a call's callee, the row also gives the call's text: the file range clang maps the whole
	// call to (a call inside one macro argument maps to its spelling there), NULL where it maps to none.
	void use(const ValueDecl* named, SourceLocation at, const CallExpr* call)
	{
		const auto* function = dyn_cast_or_null<FunctionDecl>(named);
		if (!function || isa<CXXConstructorDecl>(function) || isa<CXXDestructorDecl>(function) ||
			!function->getDeclName().isIdentifier() || !usable(function))
			return;
		const SourceManager& sm = m_ci.getSourceManager();
		if (at.isMacroID())
		{
			if (!sm.isMacroArgExpansion(at))
				return;
			at = sm.getSpellingLoc(at);
		}
		if (at.isInvalid() || at.isMacroID() || !ownsLocation(at))
			return;
		std::vector<Cell> cells{functionKey(globalOf(function)), fileAt(at).path,
								number(sm.getSpellingLineNumber(at)), number(sm.getSpellingColumnNumber(at))};
		CharSourceRange range;
		if (call)
			range = Lexer::makeFileCharRange(CharSourceRange::getTokenRange(call->getSourceRange()), sm,
											 m_ci.getLangOpts());
		SourceLocation begin = range.getBegin(), end = range.getEnd();
		if (range.isValid() && sm.getFileID(begin) == sm.getFileID(at) && sm.getFileID(end) == sm.getFileID(at))
			for (SourceLocation edge : {begin, end})
			{
				cells.push_back(number(sm.getSpellingLineNumber(edge)));
				cells.push_back(number(sm.getSpellingColumnNumber(edge)));
			}
		else
			cells.resize(cells.size() + 4);
		m_refs.push_back({"function_use", std::move(cells)});
	}

	void noteRecord(const RecordDecl* record)
	{
		if (record->isCompleteDefinition() && !record->isDependentType() && !record->isInvalidDecl() &&
			!record->isLambda() && !record->getParentFunctionOrMethod() && m_recordSeen.insert(record).second)
			m_records.push_back(record);
	}

	void noteEnum(const EnumDecl* enumeration)
	{
		if (enumeration->isComplete() && !enumeration->isDependentType() && !enumeration->getParentFunctionOrMethod() &&
			m_enumSeen.insert(enumeration).second)
			m_enums.push_back(enumeration);
	}

	// A class template's member typedef whose type depends on no template parameter (`BSSimpleArray<T>::
	// CompareFunction`) is one type in every instance: clang leaves it as it is when it instantiates a member
	// declaration naming it, so the instances' signatures name the template's typedef, which needs its row.
	void noteTypedef(const TypedefNameDecl* alias)
	{
		if (!alias->getUnderlyingType()->isDependentType() && !alias->getParentFunctionOrMethod() &&
			!alias->isInvalidDecl() && m_typedefSeen.insert(alias).second)
			m_typedefs.push_back(alias);
	}

	// A typedef of a compiler vector type in the compiler's headers (`__m128i`, `__m64`), with its type node, src
	// naming it or not: emit types IDA's union of that name (an SSE2 or MMX value) as the compiler's typedef.
	void noteVectorTypedef(const TypedefNameDecl* alias)
	{
		noteTypedef(alias);
		std::string name = localName(alias);
		node("typedef:" + name, {.kind = "typedef", .name = name});
	}

	void noteFunction(const FunctionDecl* function)
	{
		function = function->getFirstDecl();
		if (usable(function) && m_functionSeen.insert(function).second)
			m_functions.push_back(function);
	}

	void noteDefinition(const FunctionDecl* function)
	{
		if (function->isThisDeclarationADefinition() && usable(function) && m_definitionSeen.insert(function).second)
			m_definitions.push_back(function);
	}

	// functions the model can name: no template pattern, no lambda, a type mangling can spell
	bool usable(const FunctionDecl* function) const
	{
		if (function->isInvalidDecl() || function->isDependentContext() || function->getType()->isDependentType() ||
			function->getReturnType()->isUndeducedType())
			return false;
		if (auto* method = dyn_cast<CXXMethodDecl>(function); method && method->getParent()->isLambda())
			return false;
		return !function->getParentFunctionOrMethod();
	}

	const FileInfo& fileAt(SourceLocation location)
	{
		static const FileInfo none;
		const SourceManager& sm = m_ci.getSourceManager();
		location = sm.getExpansionLoc(location);
		if (location.isInvalid())
			return none;
		FileID id = sm.getFileID(location);
		auto found = m_files.find(id);
		if (found != m_files.end())
			return found->second;
		FileInfo info;
		if (OptionalFileEntryRef entry = sm.getFileEntryRefForID(id))
		{
			llvm::SmallString<256> real;
			std::string path = llvm::sys::fs::real_path(entry->getName(), real) ? entry->getName().str()
																				: real.str().str();
			if (StringRef(path).starts_with(m_scope.root))
			{
				path = path.substr(m_scope.root.size());
				for (const std::string& prefix : m_scope.collect)
					info.collected |= StringRef(path).starts_with(prefix);
			}
			info.path = path;
			info.main = id == sm.getMainFileID();
		}
		return m_files.emplace(id, info).first->second;
	}

	Owner owner(const Decl* decl)
	{
		// the helper classes and adapters an FNVD_ macro declares are the harness's, not src's
		if (decl->getLocation().isMacroID() && expansionAt(decl->getLocation()))
			return Owner::Skip;
		const FileInfo& file = fileAt(decl->getLocation());
		if (file.path.empty() || !file.collected)
			return Owner::External;
		if (instantiated(decl))
			return Owner::Shared;
		return (m_scope.headers ? !file.main : file.main) ? Owner::Own : Owner::Skip;
	}

	bool ownsLocation(SourceLocation location)
	{
		const FileInfo& file = fileAt(location);
		return file.collected && (m_scope.headers ? !file.main : file.main);
	}

private:
	// --- names and keys ---------------------------------------------------------------------------------------

	unsigned line(SourceLocation location) const
	{
		return m_ci.getSourceManager().getExpansionLineNumber(location);
	}

	std::string qualified(const NamedDecl* decl) const
	{
		std::string text;
		llvm::raw_string_ostream out(text);
		decl->getNameForDiagnostic(out, m_policy, /*Qualified=*/true);
		return text;
	}

	// A function's key: its MSVC mangled name, and for internal linkage (a `static` helper, a member of an
	// anonymous-namespace class) the file declaring it, since such names repeat across TUs.
	std::string functionKey(GlobalDecl global)
	{
		const auto* function = cast<FunctionDecl>(global.getDecl());
		std::string key;
		if (!m_mangle->shouldMangleDeclName(function))
			key = function->getNameAsString();
		else
		{
			llvm::raw_string_ostream out(key);
			m_mangle->mangleName(global, out);
		}
		if (!function->isExternallyVisible())
			key += " @" + fileAt(declaredAt(function)).path;
		return key;
	}

	// The declaration a function's file, line and parameter names come from: an instantiated member's or
	// specialization's pattern declaration (clang moves an instance to its pattern's definition when the TU
	// instantiates the body, so TUs would disagree), else its own first declaration.
	static const FunctionDecl* declared(const FunctionDecl* function)
	{
		if (const FunctionDecl* member = function->getInstantiatedFromMemberFunction())
			return member->getFirstDecl();
		if (const FunctionTemplateDecl* pattern = function->getPrimaryTemplate())
			return pattern->getTemplatedDecl()->getFirstDecl();
		return function->getFirstDecl();
	}

	// Where a function is declared (declared()); an implicit member (a destructor clang declares where a TU first
	// needs it): its class.
	static SourceLocation declaredAt(const FunctionDecl* function)
	{
		if (auto* method = dyn_cast<CXXMethodDecl>(function); method && method->isImplicit())
			return method->getParent()->getLocation();
		return declared(function)->getLocation();
	}

	// A type's name: qualified, and for one declared in a .cpp file (visible in that TU only, so its name repeats
	// across TUs) that file. A record or enum is where its definition is (clang's type names the redeclaration the
	// lookup found: `class BaseProcess;` in a .cpp that includes BaseProcess.h); one the TU only declares at namespace
	// scope with external linkage is the type of its name defined elsewhere: no file.
	std::string localName(const NamedDecl* decl)
	{
		std::string name = qualified(decl);
		if (auto* tag = dyn_cast<TagDecl>(decl))
		{
			if (const TagDecl* definition = tag->getDefinition())
				decl = definition;
			else if (tag->isExternallyVisible() && tag->getDeclContext()->isFileContext())
				return name;
		}
		StringRef path = fileAt(decl->getLocation()).path;
		if (path.ends_with(".cpp") || path.ends_with(".c"))
			name += " @" + path.str();
		return name;
	}

	static GlobalDecl globalOf(const FunctionDecl* function)
	{
		if (auto* constructor = dyn_cast<CXXConstructorDecl>(function))
			return GlobalDecl(constructor, Ctor_Complete);
		if (auto* destructor = dyn_cast<CXXDestructorDecl>(function))
			return GlobalDecl(destructor, Dtor_Base);
		return GlobalDecl(function);
	}

	std::string node(const std::string& key, TypeNode node)
	{
		if (m_types.count(key))
			return key;
		Row row{"type", {key, node.kind, node.name, node.of, node.count, node.size, node.isSigned, node.isFloat,
						 node.cc, node.ret, node.variadic, node.isConst, node.cls}};
		std::vector<Row> params;
		for (size_t k = 0; k < node.params.size(); ++k)
			params.push_back({"type_param", {key, number(k), node.params[k]}});
		m_types.emplace(key, std::make_pair(std::move(row), std::move(params)));
		return key;
	}

	// fn(cc;ret;params;[const]): a function type's key, and its node
	std::string functionTypeKey(const char* cc, QualType result, llvm::ArrayRef<QualType> params, bool variadic,
								bool isConst)
	{
		std::string ret = typeKey(result);
		TypeNode data{.kind = "fn", .cc = cc, .ret = ret, .variadic = flag(variadic), .isConst = flag(isConst)};
		std::string key = std::string("fn(") + cc + ";" + ret + ";";
		for (size_t k = 0; k < params.size(); ++k)
		{
			std::string param = typeKey(params[k]);
			key += (k ? "," : "") + param;
			data.params.push_back(param);
		}
		if (variadic)
			key += params.empty() ? "..." : ",...";
		key += isConst ? ";const)" : ";)";
		return node(key, std::move(data));
	}

	std::string typeKey(QualType type)
	{
		if (type.isNull())
			return "null";
		Qualifiers qualifiers = type.getLocalQualifiers();
		std::string key = unqualifiedKey(type.getTypePtr());
		if (qualifiers.hasVolatile())
			key = node("volatile(" + key + ")", {.kind = "volatile", .of = key});
		if (qualifiers.hasConst())
			key = node("const(" + key + ")", {.kind = "const", .of = key});
		return key;
	}

	std::string unqualifiedKey(const Type* type)
	{
		ASTContext& context = *m_context;
		if (auto* builtin = dyn_cast<BuiltinType>(type))
		{
			std::string name = builtin->getName(m_policy).str();
			TypeNode data{.kind = "builtin", .name = name};
			if (!builtin->isVoidType() && !builtin->isDependentType())
			{
				data.size = number(context.getTypeSize(builtin) / 8);
				data.isSigned = flag(builtin->isSignedIntegerType());
				data.isFloat = flag(builtin->isFloatingPoint());
			}
			return node("builtin:" + name, std::move(data));
		}
		if (auto* pointer = dyn_cast<PointerType>(type))
		{
			std::string of = typeKey(pointer->getPointeeType());
			return node("ptr(" + of + ")", {.kind = "ptr", .of = of});
		}
		if (auto* reference = dyn_cast<LValueReferenceType>(type))
		{
			std::string of = typeKey(reference->getPointeeType());
			return node("lref(" + of + ")", {.kind = "lref", .of = of});
		}
		if (auto* reference = dyn_cast<RValueReferenceType>(type))
		{
			std::string of = typeKey(reference->getPointeeType());
			return node("rref(" + of + ")", {.kind = "rref", .of = of});
		}
		if (auto* array = dyn_cast<ConstantArrayType>(type))
		{
			std::string of = typeKey(array->getElementType());
			int64_t count = static_cast<int64_t>(array->getZExtSize());
			return node("array(" + of + "," + std::to_string(count) + ")",
						{.kind = "array", .of = of, .count = count});
		}
		if (auto* array = dyn_cast<IncompleteArrayType>(type))
		{
			std::string of = typeKey(array->getElementType());
			return node("array(" + of + ",-1)", {.kind = "array", .of = of, .count = number(-1)});
		}
		if (auto* function = dyn_cast<FunctionProtoType>(type))
			return functionTypeKey(callingConvention(function->getCallConv()), function->getReturnType(),
								   function->getParamTypes(), function->isVariadic(),
								   function->getMethodQuals().hasConst());
		if (auto* member = dyn_cast<MemberPointerType>(type))
		{
			const CXXRecordDecl* owner = member->getMostRecentCXXRecordDecl();
			std::string cls = owner ? recordKey(owner) : "null";
			std::string of = typeKey(member->getPointeeType());
			TypeNode data{.kind = "memptr", .of = of, .cls = cls};
			if (!member->isDependentType() && owner && owner->hasDefinition())
				data.size = number(context.getTypeSize(member) / 8);
			return node("memptr(" + cls + "," + of + ")", std::move(data));
		}
		if (auto* vector = dyn_cast<VectorType>(type))
		{
			std::string of = typeKey(vector->getElementType());
			int64_t count = vector->getNumElements();
			return node("vector(" + of + "," + std::to_string(count) + ")",
						{.kind = "vector", .of = of, .count = count});
		}
		if (auto* tag = dyn_cast<RecordType>(type))
			return recordKey(tag->getDecl());
		if (auto* tag = dyn_cast<EnumType>(type))
		{
			const EnumDecl* enumeration = tag->getDecl();
			if (const EnumDecl* definition = enumeration->getDefinition())
				noteEnum(definition);
			std::string name = localName(enumeration);
			return node("enum:" + name, {.kind = "enum", .name = name});
		}
		if (auto* alias = dyn_cast<TypedefType>(type))
		{
			const TypedefNameDecl* decl = alias->getDecl();
			// a class template's member typedef of a non-dependent type, which an instance's member keeps as the
			// template's own (`off_type` in `basic_streambuf<char>::seekoff`): no name spells it outside the template
			// (`basic_streambuf::off_type`), so it is its target
			auto* owner = dyn_cast<CXXRecordDecl>(decl->getDeclContext());
			if (owner && owner->getDescribedClassTemplate() && !decl->getUnderlyingType()->isDependentType())
				return typeKey(decl->getUnderlyingType());
			noteTypedef(decl);
			std::string name = localName(decl);
			return node("typedef:" + name, {.kind = "typedef", .name = name});
		}
		QualType desugared = type->getLocallyUnqualifiedSingleStepDesugaredType();
		if (desugared.getTypePtr() != type)
			return typeKey(desugared);
		std::string name = type->getTypeClassName();
		return node("unknown:" + name, {.kind = "unknown", .name = name});
	}

	std::string recordName(const RecordDecl* record) { return localName(record); }

	// a class template instance's name with the arguments that equal their defaults left out (`hkArray<int>` for
	// hkArray<int, hkContainerHeapAllocator>): the spelling source, layout dumps and IDA's types use; "" when it is the
	// name itself
	std::string spelledName(const RecordDecl* record)
	{
		auto* specialization = dyn_cast<ClassTemplateSpecializationDecl>(record);
		if (!specialization || isa<ClassTemplatePartialSpecializationDecl>(specialization))
			return "";
		// clang leaves defaults in under canonical printing; an instance's arguments are canonical types already
		PrintingPolicy policy = m_policy;
		policy.SuppressDefaultTemplateArgs = true;
		policy.PrintAsCanonical = false;
		std::string text;
		llvm::raw_string_ostream out(text);
		ClassTemplateDecl* pattern = specialization->getSpecializedTemplate();
		pattern->printQualifiedName(out, policy);
		printTemplateArgumentList(out, specialization->getTemplateArgs().asArray(), policy,
								  pattern->getTemplateParameters());
		out.flush();
		return text == recordName(record) ? "" : text;
	}

	std::string recordKey(const RecordDecl* record)
	{
		if (const RecordDecl* definition = record->getDefinition())
			noteRecord(definition);
		std::string name = recordName(record);
		return node("record:" + name, {.kind = "record", .name = name});
	}

	// --- items ------------------------------------------------------------------------------------------------

	// An item: its main row (key first, digest last) and its child rows.
	struct Item
	{
		const char* keyColumn;
		Row main;
		std::vector<Row> children;
	};

	// Shared and external items another TU already wrote are not built again, except in the all-headers TU: what it
	// reaches (and so the rows it adds, the steps rows of instances only it completes among them) must not depend on
	// which items the TUs running beside it wrote first, since an item it skips is one whose types and members it
	// does not visit. Its copy of an item already written is not written again (a differing one is a conflict).
	bool written(const char* table, const char* keyColumn, const std::string& key)
	{
		std::string sql = std::string("SELECT 1 FROM ") + table + " WHERE " + keyColumn + " = ?";
		Statement query(m_store.handle(), sql.c_str());
		query.bind(1, key);
		return query.step();
	}

	bool wanted(Owner owner, const char* table, const char* keyColumn, const std::string& key)
	{
		if (owner == Owner::Skip)
			return false;
		if (!m_itemSeen.insert(std::string(table) + ":" + key).second)
			return false;
		return owner == Owner::Own || m_scope.headers || !written(table, keyColumn, key);
	}

	void addItem(const char* keyColumn, Row main, std::vector<Row> children)
	{
		std::vector<Row> all{main};
		all.insert(all.end(), children.begin(), children.end());
		main.cells.push_back(static_cast<int64_t>(digest(all)));
		m_items.push_back({keyColumn, std::move(main), std::move(children)});
	}

	void drain()
	{
		// records note their fields' and methods' types, functions their parameters': until nothing new
		size_t records = 0, enums = 0, typedefs = 0, functions = 0, definitions = 0;
		while (records < m_records.size() || enums < m_enums.size() || typedefs < m_typedefs.size() ||
			   functions < m_functions.size() || definitions < m_definitions.size())
		{
			while (records < m_records.size())
				record(m_records[records++]);
			while (enums < m_enums.size())
				enumeration(m_enums[enums++]);
			while (typedefs < m_typedefs.size())
				typedefItem(m_typedefs[typedefs++]);
			while (functions < m_functions.size())
				function(m_functions[functions++]);
			while (definitions < m_definitions.size())
				definition(m_definitions[definitions++]);
		}
	}

	void record(const RecordDecl* record)
	{
		Owner who = owner(record);
		std::string name = recordName(record);
		if (!wanted(who, "record", "name", name))
			return;
		const ASTRecordLayout& layout = m_context->getASTRecordLayout(record);
		std::vector<Row> children;
		int64_t index = 0;
		for (const FieldDecl* field : record->fields())
		{
			Cell bits = field->isBitField() ? number(field->getBitWidthValue()) : Cell();
			children.push_back({"field",
								{name, number(index++), field->getName().str(), typeKey(field->getType()),
								 number(layout.getFieldOffset(field->getFieldIndex())), bits}});
			// a `// PDB:` line in the comment lines right above the field records why its type is not the PDB's
			// (Model.hpp's field_recorded): RULES' `// PDB: <Enum>` and `// PDB: <form>; the exe <fact>.` alike
			if (who != Owner::External && !field->getName().empty())
			{
				SourceLocation start = m_ci.getSourceManager().getExpansionLoc(field->getBeginLoc());
				for (StringRef comment : commentLines(start))
					if (comment.drop_front(2).trim().starts_with("PDB:"))
					{
						m_refs.push_back({"field_marker", {name, field->getName().str(), comment.str()}});
						break;
					}
			}
		}
		// a C struct's copy assignment is a byte copy
		Cell dynamic, abstract, trivialCopy = flag(true), vfptr, vbptr, primaryBase, pattern;
		if (auto* cxx = dyn_cast<CXXRecordDecl>(record))
		{
			dynamic = flag(cxx->isDynamicClass());
			abstract = flag(cxx->isAbstract());
			trivialCopy = flag(cxx->hasTrivialCopyAssignment());
			vfptr = flag(layout.hasOwnVFPtr());
			if (layout.hasOwnVBPtr())
				vbptr = number(layout.getVBPtrOffset().getQuantity());
			if (layout.getPrimaryBase())
				primaryBase = recordName(layout.getPrimaryBase());
			if (auto* specialization = dyn_cast<ClassTemplateSpecializationDecl>(cxx))
				pattern = qualified(specialization->getSpecializedTemplate());
			cxxChildren(name, cxx, layout, children);
		}
		// a class template instance's member enums: no visit reaches an instance's declarations, and their
		// enumerators are the instance's own (`hkArrayBase<int>::CAPACITY_MASK`)
		if (instantiated(record))
			for (const Decl* member : record->decls())
				if (auto* enumeration = dyn_cast<EnumDecl>(member))
					noteEnum(enumeration);
		const FileInfo& file = fileAt(record->getLocation());
		addItem("name",
				{"record",
				 {name, textOrNull(file.path), number(line(record->getLocation())),
				  number(line(record->getBraceRange().getEnd())), tagName(record->getTagKind()),
				  number(layout.getSize().getQuantity()), number(layout.getAlignment().getQuantity()),
				  number(layout.getDataSize().getQuantity()), dynamic, abstract, trivialCopy, vfptr, vbptr, primaryBase,
				  pattern, flag(who == Owner::External), textOrNull(spelledName(record))}},
				std::move(children));
	}

	void cxxChildren(const std::string& name, const CXXRecordDecl* record, const ASTRecordLayout& layout,
					 std::vector<Row>& children)
	{
		int64_t index = 0;
		for (const CXXBaseSpecifier& base : record->bases())
		{
			const CXXRecordDecl* decl = base.getType()->getAsCXXRecordDecl();
			if (!decl)
				continue;
			int64_t offset = base.isVirtual() ? layout.getVBaseClassOffset(decl).getQuantity()
											  : layout.getBaseClassOffset(decl).getQuantity();
			children.push_back({"base", {name, number(index++), typeKey(base.getType()), number(offset),
										 flag(base.isVirtual())}});
		}
		index = 0;
		for (const CXXMethodDecl* method : record->methods())
		{
			if ((method->isImplicit() && !method->isVirtual()) || !usable(method))
				continue;
			noteFunction(method);
			children.push_back({"method", {name, number(index++), functionKey(globalOf(method))}});
		}
		// the base members a using-declaration brings into the class's scope (clang makes no shadow of one the class
		// hides with its own member of the same parameters); an inheriting constructor's are no member functions
		index = 0;
		for (const Decl* decl : record->decls())
		{
			auto* shadow = dyn_cast<UsingShadowDecl>(decl);
			auto* method = shadow && !isa<ConstructorUsingShadowDecl>(shadow)
							   ? dyn_cast<CXXMethodDecl>(shadow->getTargetDecl())
							   : nullptr;
			if (!method || !usable(method))
				continue;
			noteFunction(method);
			children.push_back({"using_member", {name, number(index++), functionKey(globalOf(method))}});
		}
		vtables(name, record, children);
	}

	void vtables(const std::string& name, const CXXRecordDecl* record, std::vector<Row>& children)
	{
		ASTContext& context = *m_context;
		if (!record->isDynamicClass() || !context.getTargetInfo().getCXXABI().isMicrosoft())
			return;
		m_ci.getSema().ForceDeclarationOfImplicitMembers(const_cast<CXXRecordDecl*>(record));
		auto* builder = cast<MicrosoftVTableContext>(context.getVTableContext());
		bool vectorDeleting = context.getTargetInfo().emitVectorDeletingDtors(context.getLangOpts());
		int64_t table = 0;
		for (const auto& vptr : builder->getVFPtrOffsets(record))
		{
			children.push_back({"vtable", {name, number(table), number(vptr->FullOffsetInMDC.getQuantity())}});
			int64_t step = 0;
			for (auto it = vptr->PathToIntroducingObject.rbegin(); it != vptr->PathToIntroducingObject.rend(); ++it)
				children.push_back({"vtable_path", {name, number(table), number(step++), recordName(*it)}});
			children.push_back({"vtable_path", {name, number(table), number(step++), name}});
			const VTableLayout& layout = builder->getVFTableLayout(record, vptr->FullOffsetInMDC);
			std::map<uint64_t, const ThunkInfo*> thunks;
			for (const auto& thunk : layout.vtable_thunks())
				thunks[thunk.first] = &thunk.second;
			auto components = layout.vtable_components();
			int64_t slot = 0;
			for (uint64_t k = 0; k < components.size(); ++k)
			{
				const VTableComponent& component = components[k];
				const CXXMethodDecl* method = nullptr;
				GlobalDecl global;
				const char* kind = "method";
				if (component.getKind() == VTableComponent::CK_FunctionPointer)
				{
					method = component.getFunctionDecl();
					global = GlobalDecl(method);
				}
				else if (component.getKind() == VTableComponent::CK_DeletingDtorPointer)
				{
					const CXXDestructorDecl* destructor = component.getDestructorDecl();
					method = destructor;
					global = GlobalDecl(destructor, vectorDeleting ? Dtor_VectorDeleting : Dtor_Deleting);
					kind = "deleting";
				}
				else
				{
					continue;  // RTTI: no slot
				}
				noteFunction(method);
				int64_t adjust = 0;
				bool virtualAdjust = false;
				auto thunk = thunks.find(k);
				if (thunk != thunks.end())
				{
					adjust = thunk->second->This.NonVirtual;
					virtualAdjust = !thunk->second->This.Virtual.isEmpty();
				}
				children.push_back({"slot",
									{name, number(table), number(slot++), kind, functionKey(global), qualified(method),
									 flag(method->isPureVirtual()), number(adjust), flag(virtualAdjust)}});
			}
			++table;
		}
	}

	void enumeration(const EnumDecl* declaration)
	{
		// the defining declaration (an opaque one, `enum CHUNK_ID : uint32_t;`, may be seen first)
		const EnumDecl* enumeration = declaration->getDefinition() ? declaration->getDefinition() : declaration;
		Owner who = owner(enumeration);
		std::string name = localName(enumeration);
		if (!wanted(who, "enum", "name", name))
			return;
		std::vector<Row> children;
		int64_t index = 0;
		for (const EnumConstantDecl* constant : enumeration->enumerators())
		{
			const llvm::APSInt& value = constant->getInitVal();
			int64_t number = value.isSigned() ? value.getSExtValue() : static_cast<int64_t>(value.getZExtValue());
			children.push_back({"enumerator", {name, index++, constant->getName().str(), number}});
		}
		const FileInfo& file = fileAt(enumeration->getLocation());
		addItem("name",
				{"enum",
				 {name, textOrNull(file.path), number(line(enumeration->getLocation())), flag(enumeration->isScoped()),
				  typeKey(enumeration->getIntegerType()), flag(who == Owner::External)}},
				std::move(children));
	}

	void typedefItem(const TypedefNameDecl* alias)
	{
		Owner who = owner(alias);
		std::string name = localName(alias);
		if (!wanted(who, "typedef", "name", name))
			return;
		const FileInfo& file = fileAt(alias->getLocation());
		addItem("name",
				{"typedef",
				 {name, textOrNull(file.path), number(line(alias->getLocation())),
				  typeKey(alias->getUnderlyingType()), flag(who == Owner::External)}},
				{});
	}

	// clang's i686-MSVC lowering of the function (CodeGen's CGFunctionInfo), once per mangled name, by whichever
	// TU sees its parameter types complete
	void abi(const FunctionDecl* function, const std::string& key)
	{
		if (isa<CXXConstructorDecl>(function) || isa<CXXDestructorDecl>(function))
			return;  // CodeGen arranges structors through the C++ ABI (MS: a constructor returns this)
		if (!wanted(Owner::Shared, "abi", "function", key))
			return;
		Sema& sema = m_ci.getSema();
		auto complete = [&](QualType type) {
			return type->isVoidType() || sema.isCompleteType(function->getLocation(), type);
		};
		if (!complete(function->getReturnType()))
			return forgetAbi(key);
		for (const ParmVarDecl* param : function->parameters())
			if (!complete(param->getType()))
				return forgetAbi(key);
		const auto* prototype = function->getType()->getAs<FunctionProtoType>();
		if (!prototype)
			return;
		CodeGen::CodeGenModule& module = m_codegen->CGM();
		const CodeGen::CGFunctionInfo* info;
		auto* method = dyn_cast<CXXMethodDecl>(function);
		if (method && method->isInstance())
			info = &CodeGen::arrangeCXXMethodType(module, method->getParent(), prototype, method);
		else
			info = &CodeGen::arrangeFreeFunctionType(
				module, CanQual<FunctionProtoType>::CreateUnsafe(m_context->getCanonicalType(QualType(prototype, 0))));
		const CodeGen::ABIArgInfo& ret = info->getReturnInfo();
		int64_t stack = 0;
		if (ret.isIndirect() && !ret.getInReg())
			stack += 4;  // the hidden result pointer
		std::vector<Row> args;
		// x86_thiscallcc puts the first integer argument in ECX by itself (CC_X86_32_ThisCall): CodeGen marks no
		// inreg there, as it does for fastcall's
		bool ecxFree = info->getEffectiveCallingConvention() == llvm::CallingConv::X86_ThisCall;
		int64_t index = 0;
		for (const CodeGen::CGFunctionInfoArgInfo& arg : info->arguments())
		{
			int64_t bytes = static_cast<int64_t>((m_context->getTypeSize(arg.type) + 31) / 32 * 4);
			int64_t pushed = 0;
			bool inreg = hasInReg(arg.info);
			switch (arg.info.getKind())
			{
			case CodeGen::ABIArgInfo::Direct:
			case CodeGen::ABIArgInfo::Extend:
				if (ecxFree && bytes == 4 && !arg.type->isFloatingType())
					inreg = true, ecxFree = false;
				pushed = inreg ? 0 : bytes;
				break;
			case CodeGen::ABIArgInfo::Indirect:
				pushed = inreg ? 0 : arg.info.getIndirectByVal() ? bytes : 4;
				break;
			case CodeGen::ABIArgInfo::Ignore:
				break;
			default:
				pushed = bytes;
				break;
			}
			stack += pushed;
			args.push_back({"abi_arg", {key, number(index++), abiKind(arg.info), flag(inreg), number(pushed)}});
		}
		addItem("function",
				{"abi",
				 {key, effectiveConvention(info->getEffectiveCallingConvention()), abiKind(ret),
				  flag(ret.isIndirect() && ret.isSRetAfterThis()), flag(info->usesInAlloca()), number(stack)}},
				std::move(args));
	}

	// a TU that cannot lay the parameters out leaves the ABI to one that can
	void forgetAbi(const std::string& key) { m_itemSeen.erase("abi:" + key); }

	// An inline trivial accessor (an accessor row): a named, non-virtual, inline instance method (in-class, or
	// declared inline: the build inlines nothing else, /Ob1) whose header body (a template member's: its pattern's) is
	// `return m_x;` (load), `return m_x[param];` (index) or `return &m_x;` (address), m_x a field of `this`. A TU that
	// sees no body leaves the row to one that does.
	void accessor(const FunctionDecl* function, const std::string& key)
	{
		// a named non-virtual method (no operator or conversion function; a virtual call is no inlined read)
		auto* method = dyn_cast<CXXMethodDecl>(function);
		if (!method || !method->isInstance() || method->isVirtual() || !method->getDeclName().isIdentifier() ||
			!wanted(Owner::Shared, "accessor", "function", key))
			return;
		const FunctionDecl* pattern = function->getTemplateInstantiationPattern();
		const FunctionDecl* body = (pattern ? pattern : function)->getDefinition();
		if (!body)
			return forgetAccessor(key);
		StringRef file = fileAt(body->getLocation()).path;
		const auto* block = dyn_cast_or_null<CompoundStmt>(body->getBody());
		const auto* ret = block && block->size() == 1 ? dyn_cast<ReturnStmt>(block->body_front()) : nullptr;
		const Expr* value = ret && ret->getRetValue() ? ret->getRetValue()->IgnoreParenImpCasts() : nullptr;
		// inline (in-class, or the inline keyword): the build inlines nothing else (/Ob1)
		if (!value || !body->isInlined() || !(file.ends_with(".h") || file.ends_with(".hpp") || file.ends_with(".inl")))
			return;
		// m_x on the implicit this
		auto field = [](const Expr* e) -> const FieldDecl* {
			const auto* member = dyn_cast<MemberExpr>(e->IgnoreParenImpCasts());
			const auto* self = member ? dyn_cast<CXXThisExpr>(member->getBase()->IgnoreParenImpCasts()) : nullptr;
			return self && self->isImplicit() ? dyn_cast<FieldDecl>(member->getMemberDecl()) : nullptr;
		};
		const auto* address = dyn_cast<UnaryOperator>(value);
		const auto* subscript = dyn_cast<ArraySubscriptExpr>(value);
		const auto* index = subscript ? dyn_cast<DeclRefExpr>(subscript->getIdx()->IgnoreParenImpCasts()) : nullptr;
		const auto* parameter = index ? dyn_cast<ParmVarDecl>(index->getDecl()) : nullptr;
		if (const FieldDecl* target = field(value))
			addItem("function", {"accessor", {key, "load", target->getName().str(), Cell{}}}, {});
		else if (address && address->getOpcode() == UO_AddrOf && (target = field(address->getSubExpr())))
			addItem("function", {"accessor", {key, "address", target->getName().str(), Cell{}}}, {});
		else if (parameter && (target = field(subscript->getBase())))
			addItem("function",
					{"accessor",
					 {key, "index", target->getName().str(), number(parameter->getFunctionScopeIndex())}},
					{});
	}

	void forgetAccessor(const std::string& key) { m_itemSeen.erase("accessor:" + key); }

	// The inline steps of an inline function (a steps row): a class template instance's member, or any other inline
	// member or free function that is no template (in-class, or declared inline: the build inlines nothing else,
	// /Ob1), as the exe inlines it, the inline functions it calls expanded, in Model.hpp's step language. An
	// instance's body is its own (the overloads it calls resolved for its arguments), so a TU that has not
	// instantiated it leaves the row to one that has; a body outside the language has no row.
	void steps(const FunctionDecl* function, const std::string& key)
	{
		auto* method = dyn_cast<CXXMethodDecl>(function);
		bool instance = method && isa<ClassTemplateSpecializationDecl>(method->getParent());
		if ((instance && !isTemplateInstantiation(method->getTemplateSpecializationKind())) ||
			(!instance && (function->isTemplated() || function->isTemplateInstantiation())) ||
			!wanted(Owner::Shared, "steps", "function", key))
			return;
		const FunctionDecl* body = nullptr;
		if (!bodyOf(function, body))
			return forgetSteps(key);
		if (!body->isInlined())
			return;
		StepScope scope{method && method->isInstance() ? "this" : "", {}};
		for (unsigned k = 0; k < body->getNumParams(); ++k)
			scope.params[body->getParamDecl(k)] = "p" + std::to_string(k);
		m_stepLets = 0;
		// none here: another TU may see the bodies this one lacks
		if (std::optional<std::string> text = stepBody(body, scope, 0))
			addItem("function", {"steps", {key, *text}}, {});
		else
			forgetSteps(key);
	}

	void forgetSteps(const std::string& key) { m_itemSeen.erase("steps:" + key); }

	// a function's body; an instance's that the TU did not instantiate is instantiated in the all-headers TU (its
	// errors are reported, not fatal; a src TU instantiates only what it uses)
	bool bodyOf(const FunctionDecl* function, const FunctionDecl*& body)
	{
		if (function->hasBody(body) && body->getBody())
			return true;
		if (!m_scope.headers || !function->getTemplateInstantiationPattern() || function->isInvalidDecl())
			return false;
		// the parser's translation-unit scope is gone at the TU's end; Sema declares builtins and implicit members it
		// needs while instantiating into one
		Sema& sema = m_ci.getSema();
		clang::Scope scope(nullptr, clang::Scope::DeclScope, sema.getDiagnostics());
		scope.setEntity(m_context->getTranslationUnitDecl());
		clang::Scope* saved = sema.TUScope;
		if (!saved)
			sema.TUScope = &scope;
		sema.InstantiateFunctionDefinition(function->getLocation(), const_cast<FunctionDecl*>(function),
										   /*Recursive=*/true);
		sema.TUScope = saved;
		return function->hasBody(body) && body->getBody();
	}

	// what `this` and the parameters stand for in a body being written (an inline callee: its arguments)
	struct StepScope
	{
		std::string self;
		std::map<const ParmVarDecl*, std::string> params;
	};

	static constexpr int kStepDepth = 6;
	// the lets written in the steps row being built (their names `@aN`)
	int m_stepLets = 0;

	// a function's body as one statement: a constructor's member initialisers and delegation first; a destructor
	// only where no member or base of its class has a destructor of its own to run after it
	std::optional<std::string> stepBody(const FunctionDecl* body, const StepScope& scope, int depth)
	{
		std::string text = "(seq";
		if (const auto* ctor = dyn_cast<CXXConstructorDecl>(body))
			for (const CXXCtorInitializer* init : ctor->inits())
			{
				std::optional<std::string> step;
				if (init->isDelegatingInitializer())
					step = stepValue(init->getInit(), scope, depth, true);
				else if (const FieldDecl* field = init->getMember(); field && !field->isBitField())
				{
					std::optional<std::string> value = stepValue(init->getInit(), scope, depth, false);
					if (value)
						step = "(set " + stepSize(field->getType()) + " " +
							   stepPlus(scope.self, fieldOffset(field)) + " " + *value + ")";
				}
				if (!step)
					return std::nullopt;
				text += " " + *step;
			}
		if (const auto* dtor = dyn_cast<CXXDestructorDecl>(body))
		{
			const CXXRecordDecl* record = dtor->getParent();
			for (const CXXBaseSpecifier& base : record->bases())
				if (const CXXRecordDecl* b = base.getType()->getAsCXXRecordDecl(); !b || !b->hasTrivialDestructor())
					return std::nullopt;
			for (const FieldDecl* field : record->fields())
				if (const CXXRecordDecl* f = field->getType()->getBaseElementTypeUnsafe()->getAsCXXRecordDecl();
					f && !f->hasTrivialDestructor())
					return std::nullopt;
		}
		std::optional<std::string> statement = stepStatement(body->getBody(), scope, depth, true);
		if (!statement)
			return std::nullopt;
		return text + " " + *statement + ")";
	}

	std::string stepSize(QualType type) const
	{
		return std::to_string(m_context->getTypeSizeInChars(type).getQuantity());
	}

	static std::string stepPlus(const std::string& address, int64_t offset)
	{
		return offset ? "(+ " + address + " " + std::to_string(offset) + ")" : address;
	}

	int64_t fieldOffset(const FieldDecl* field) const
	{
		return static_cast<int64_t>(m_context->getFieldOffset(field) / 8);
	}

	// a value with no effect of its own (it can be read again or not at all where an inline callee's parameter is)
	static bool pureStep(const std::string& text)
	{
		for (const char* effect : {"(call", "(vcall", "(icall", "(set", "(atomic", "(pre", "(post"})
			if (text.find(effect) != std::string::npos)
				return false;
		return true;
	}

	// a statement; last: the last of its function's body (a plain return there ends nothing)
	std::optional<std::string> stepStatement(const Stmt* s, const StepScope& scope, int depth, bool last)
	{
		if (const auto* block = dyn_cast<CompoundStmt>(s))
		{
			std::string text = "(seq";
			size_t n = 0, size = block->size();
			for (const Stmt* item : block->body())
			{
				++n;
				if (isa<NullStmt>(item))
					continue;
				std::optional<std::string> step = stepStatement(item, scope, depth, last && n == size);
				if (!step)
					return std::nullopt;
				text += " " + *step;
			}
			return text + ")";
		}
		if (const auto* branch = dyn_cast<IfStmt>(s))
		{
			if (branch->getInit() || branch->getConditionVariable() || branch->isConsteval())
				return std::nullopt;
			std::optional<std::string> test = stepValue(branch->getCond(), scope, depth, false);
			std::optional<std::string> then = test ? stepStatement(branch->getThen(), scope, depth, false) : test;
			if (!then)
				return std::nullopt;
			std::string text = "(if " + *test + " " + *then;
			if (const Stmt* otherwise = branch->getElse())
			{
				std::optional<std::string> step = stepStatement(otherwise, scope, depth, false);
				if (!step)
					return std::nullopt;
				text += " " + *step;
			}
			return text + ")";
		}
		if (const auto* ret = dyn_cast<ReturnStmt>(s))
		{
			if (!last)
				return std::nullopt;
			const Expr* value = ret->getRetValue();
			if (!value)
				return std::string("(seq)");
			std::optional<std::string> result = value->isGLValue() ? stepAddress(value, scope, depth)
																	: stepValue(value, scope, depth, false);
			return result ? std::optional("(ret " + *result + ")") : std::nullopt;
		}
		if (const auto* e = dyn_cast<Expr>(s))
			return stepValue(e, scope, depth, true);
		return std::nullopt;
	}

	// an expression's value; statement: its value unused
	std::optional<std::string> stepValue(const Expr* e, const StepScope& scope, int depth, bool statement)
	{
		e = e->IgnoreParens();
		if (const auto* full = dyn_cast<FullExpr>(e))
			return stepValue(full->getSubExpr(), scope, depth, statement);
		// a constant (no effect): its value
		if (!e->isValueDependent() && (e->getType()->isIntegralOrEnumerationType() || e->getType()->isPointerType()))
		{
			if (e->isNullPointerConstant(*m_context, Expr::NPC_ValueDependentIsNotNull))
				return std::string("#0");
			Expr::EvalResult result;
			if (e->getType()->isIntegralOrEnumerationType() && e->EvaluateAsInt(result, *m_context))
				return "#" + std::to_string(result.Val.getInt().getExtValue());
		}
		if (const auto* cast = dyn_cast<CastExpr>(e))
		{
			const Expr* sub = cast->getSubExpr();
			switch (cast->getCastKind())
			{
			case CK_LValueToRValue:
			{
				// a pre-increment or -decrement (an lvalue in C++): the new value
				if (const auto* unary = dyn_cast<UnaryOperator>(sub->IgnoreParens());
					unary && (unary->getOpcode() == UO_PreInc || unary->getOpcode() == UO_PreDec))
					return stepValue(unary, scope, depth, statement);
				// a by-value parameter is its value
				if (const auto* ref = dyn_cast<DeclRefExpr>(sub->IgnoreParens()))
					if (const auto* param = dyn_cast<ParmVarDecl>(ref->getDecl());
						param && !param->getType()->isReferenceType())
					{
						auto found = scope.params.find(param);
						return found != scope.params.end() ? std::optional(found->second) : std::nullopt;
					}
				std::optional<std::string> address = stepAddress(sub, scope, depth);
				return address ? std::optional("(ld " + stepSize(e->getType()) + " " + *address + ")") : std::nullopt;
			}
			case CK_NoOp:
			case CK_BitCast:
			case CK_NullToPointer:
				return stepValue(sub, scope, depth, statement);
			case CK_IntegralCast:
				if (m_context->getTypeSize(e->getType()) != m_context->getTypeSize(sub->getType()))
					return std::nullopt;
				return stepValue(sub, scope, depth, statement);
			case CK_DerivedToBase:
			case CK_UncheckedDerivedToBase:
				return zeroBasePath(cast) ? stepValue(sub, scope, depth, statement) : std::nullopt;
			case CK_PointerToBoolean:
			case CK_IntegralToBoolean:
			{
				std::optional<std::string> value = stepValue(sub, scope, depth, false);
				return value ? std::optional("(ne " + *value + " #0)") : std::nullopt;
			}
			default:
				return std::nullopt;
			}
		}
		if (isa<CXXThisExpr>(e))
			return scope.self;
		if (const auto* unary = dyn_cast<UnaryOperator>(e))
		{
			const char* op = nullptr;
			switch (unary->getOpcode())
			{
			case UO_PreInc: op = "pre+"; break;
			case UO_PreDec: op = "pre-"; break;
			case UO_PostInc: op = "post+"; break;
			case UO_PostDec: op = "post-"; break;
			case UO_AddrOf: return stepAddress(unary->getSubExpr(), scope, depth);
			case UO_LNot:
			{
				std::optional<std::string> value = stepValue(unary->getSubExpr(), scope, depth, false);
				return value ? std::optional("(not " + *value + ")") : std::nullopt;
			}
			default: return std::nullopt;
			}
			if (!unary->getType()->isIntegerType())
				return std::nullopt;
			std::optional<std::string> address = stepAddress(unary->getSubExpr(), scope, depth);
			return address ? std::optional("(" + std::string(op) + " " + stepSize(unary->getType()) + " " + *address + ")")
						   : std::nullopt;
		}
		if (const auto* binary = dyn_cast<BinaryOperator>(e))
		{
			const char* op = nullptr;
			switch (binary->getOpcode())
			{
			case BO_EQ: op = "eq"; break;
			case BO_NE: op = "ne"; break;
			case BO_LAnd: op = "and"; break;
			case BO_LOr: op = "or"; break;
			case BO_Assign:
			{
				std::optional<std::string> address = stepAddress(binary->getLHS(), scope, depth);
				std::optional<std::string> value =
					address ? stepValue(binary->getRHS(), scope, depth, false) : std::nullopt;
				return value ? std::optional("(set " + stepSize(binary->getLHS()->getType()) + " " + *address + " " +
											 *value + ")")
							 : std::nullopt;
			}
			case BO_Add:
			case BO_Sub:
				return stepPlusConstant(binary, scope, depth);
			default: return std::nullopt;
			}
			std::optional<std::string> x = stepValue(binary->getLHS(), scope, depth, false);
			std::optional<std::string> y = x ? stepValue(binary->getRHS(), scope, depth, false) : x;
			return y ? std::optional("(" + std::string(op) + " " + *x + " " + *y + ")") : std::nullopt;
		}
		if (const auto* atomic = dyn_cast<AtomicExpr>(e))
		{
			const char* op = nullptr;
			switch (atomic->getOp())
			{
			case AtomicExpr::AO__atomic_fetch_add: op = "fetch_add"; break;
			case AtomicExpr::AO__atomic_add_fetch: op = "add_fetch"; break;
			case AtomicExpr::AO__atomic_fetch_sub: op = "fetch_sub"; break;
			case AtomicExpr::AO__atomic_sub_fetch: op = "sub_fetch"; break;
			default: return std::nullopt;
			}
			std::optional<std::string> pointer = stepValue(atomic->getPtr(), scope, depth, false);
			std::optional<std::string> value = pointer ? stepValue(atomic->getVal1(), scope, depth, false) : pointer;
			return value ? std::optional("(atomic " + std::string(op) + " " + *pointer + " " + *value + ")")
						 : std::nullopt;
		}
		if (const auto* construct = dyn_cast<CXXConstructExpr>(e))
		{
			// a delegating constructor's target, on the same object
			const CXXConstructorDecl* ctor = construct->getConstructor();
			return stepCall(ctor, scope.self, construct->arguments(), scope, depth, statement, false);
		}
		if (const auto* call = dyn_cast<CallExpr>(e))
		{
			const FunctionDecl* callee = call->getDirectCallee();
			if (!callee && !isa<CXXOperatorCallExpr>(call))
				return stepIndirectCall(call, scope, depth);
			if (!callee || isa<CXXOperatorCallExpr>(call))
				return std::nullopt;
			if (const auto* member = dyn_cast<CXXMemberCallExpr>(call))
			{
				const auto* access = dyn_cast<MemberExpr>(member->getCallee()->IgnoreParens());
				if (!access)
					return std::nullopt;
				const Expr* object = member->getImplicitObjectArgument();
				std::optional<std::string> self =
					access->isArrow() ? stepValue(object, scope, depth, false) : stepAddress(object, scope, depth);
				if (!self)
					return std::nullopt;
				bool dispatched = member->getMethodDecl()->isVirtual() && !access->hasQualifier();
				return stepCall(callee, *self, call->arguments(), scope, depth, statement, dispatched);
			}
			return stepCall(callee, std::nullopt, call->arguments(), scope, depth, statement, false);
		}
		return std::nullopt;
	}

	// a pointer or an integer plus or minus a constant: (+ X BYTES), a pointer's constant scaled by its pointee's size
	std::optional<std::string> stepPlusConstant(const BinaryOperator* binary, const StepScope& scope, int depth)
	{
		const Expr* base = binary->getLHS();
		const Expr* offset = binary->getRHS();
		if (binary->getOpcode() == BO_Add && base->getType()->isIntegerType() && offset->getType()->isPointerType())
			std::swap(base, offset);
		QualType type = base->getType();
		Expr::EvalResult result;
		if (!offset->getType()->isIntegerType() || offset->isValueDependent() ||
			!offset->EvaluateAsInt(result, *m_context) ||
			(!type->isPointerType() && (!type->isIntegerType() || type->isBooleanType() ||
										m_context->getTypeSize(type) != m_context->getTypeSize(binary->getType()))))
			return std::nullopt;
		int64_t scale = 1;
		if (type->isPointerType())
		{
			QualType pointee = type->getPointeeType();
			if (pointee->isIncompleteType() || pointee->isFunctionType())
				return std::nullopt;
			scale = m_context->getTypeSizeInChars(pointee).getQuantity();
		}
		std::optional<std::string> value = stepValue(base, scope, depth, false);
		int64_t bytes = result.Val.getInt().getExtValue() * scale;
		if (!value)
			return std::nullopt;
		return "(+ " + *value + " " + std::to_string(binary->getOpcode() == BO_Add ? bytes : -bytes) + ")";
	}

	// a call through a function pointer's value (an import's IAT global): (icall POINTER ARGS...)
	std::optional<std::string> stepIndirectCall(const CallExpr* call, const StepScope& scope, int depth)
	{
		const Expr* callee = call->getCallee()->IgnoreParens();
		const auto* pointer = callee->getType()->getAs<PointerType>();
		if (!pointer || !pointer->getPointeeType()->isFunctionProtoType())
			return std::nullopt;
		std::optional<std::string> target = stepValue(callee, scope, depth, false);
		if (!target)
			return std::nullopt;
		std::string text = "(icall " + *target;
		for (const Expr* argument : call->arguments())
		{
			std::optional<std::string> value = stepValue(argument, scope, depth, false);
			if (!value)
				return std::nullopt;
			text += " " + *value;
		}
		return text + ")";
	}

	// a call: an inline callee's body with its arguments for its parameters (pure arguments only; one that loads is a
	// let before the body, read where the body reads the parameter, so that it holds the value it had at the call),
	// a virtual call by its vtable slot, else the call of the callee's key
	template <class Arguments>
	std::optional<std::string> stepCall(const FunctionDecl* callee, const std::optional<std::string>& self,
										Arguments arguments, const StepScope& scope, int depth, bool statement,
										bool dispatched)
	{
		std::vector<std::string> values;
		size_t n = 0;
		for (const Expr* argument : arguments)
		{
			if (n >= callee->getNumParams())
				return std::nullopt;
			bool reference = callee->getParamDecl(n++)->getType()->isReferenceType();
			std::optional<std::string> value =
				reference ? stepAddress(argument, scope, depth) : stepValue(argument, scope, depth, false);
			if (!value)
				return std::nullopt;
			values.push_back(*value);
		}
		if (n != callee->getNumParams())
			return std::nullopt;
		const FunctionDecl* body = nullptr;
		bool inlined = !dispatched && bodyOf(callee, body) && body->isInlined();
		// an inline instance this TU has no body of
		const FunctionDecl* pattern = callee->getTemplateInstantiationPattern();
		if (!dispatched && !inlined && pattern && pattern->isInlined())
			return std::nullopt;
		if (inlined)
		{
			if (depth >= kStepDepth)
				return std::nullopt;
			// the parameters as the body reads them: each value itself (direct), or a let's name where it loads
			StepScope direct{self ? *self : std::string(), {}}, bound = direct;
			std::string lets;
			auto let = [&](const std::string& value) {
				if (value.find("(ld") == std::string::npos)
					return value;
				std::string name = "@a" + std::to_string(m_stepLets++);
				lets += " (let " + name + " " + value + ")";
				return name;
			};
			bool pure = !self || pureStep(*self);
			if (self)
				bound.self = let(*self);
			for (size_t k = 0; k < values.size(); ++k)
			{
				pure = pure && pureStep(values[k]);
				const ParmVarDecl* param = body->getParamDecl(static_cast<unsigned>(k));
				direct.params[param] = values[k];
				bound.params[param] = let(values[k]);
			}
			if (!pure)
				return std::nullopt;
			// a value: a body that is one return of it; inside an expression, its parameters read where the value
			// does, only where nothing in it writes before they are read (no effect but its outermost step's, which
			// reads its operands first)
			const auto* block = dyn_cast<CompoundStmt>(body->getBody());
			const auto* ret = block && block->size() == 1 ? dyn_cast<ReturnStmt>(block->body_front()) : nullptr;
			std::optional<std::string> text;
			if (ret && ret->getRetValue() && !body->getReturnType()->isReferenceType())
			{
				if (lets.empty() || !statement)
				{
					text = stepValue(ret->getRetValue(), direct, depth + 1, statement);
					return text && (lets.empty() || pureStep(text->substr(std::min(text->find(' '), text->size()))))
							   ? text
							   : std::nullopt;
				}
				text = stepValue(ret->getRetValue(), bound, depth + 1, statement);
			}
			else if (statement)
				text = stepBody(body, bound, depth + 1);
			if (!text || lets.empty())
				return text;
			return "(seq" + lets + " " + *text + ")";
		}
		std::string text;
		if (dispatched)
		{
			auto* builder = dyn_cast<MicrosoftVTableContext>(m_context->getVTableContext());
			if (!builder || isa<CXXDestructorDecl>(callee))
				return std::nullopt;
			MethodVFTableLocation where = builder->getMethodVFTableLocation(GlobalDecl(callee));
			if (where.VBase || !where.VFPtrOffset.isZero())
				return std::nullopt;
			text = "(vcall " + std::to_string(where.Index * 4);
		}
		else
		{
			noteFunction(callee);
			text = "(call " + quoteStep(functionKey(globalOf(callee)));
		}
		if (self)
			text += " " + *self;
		for (const std::string& value : values)
			text += " " + value;
		return text + ")";
	}

	// a function key in the step language: quoted, `"` and `\` escaped
	static std::string quoteStep(const std::string& key)
	{
		std::string text = "\"";
		for (char c : key)
		{
			if (c == '"' || c == '\\')
				text += '\\';
			text += c;
		}
		return text + "\"";
	}

	// a derived-to-base cast whose bases all sit at offset 0
	bool zeroBasePath(const CastExpr* cast) const
	{
		QualType from = cast->getSubExpr()->getType();
		if (from->isPointerType())
			from = from->getPointeeType();
		const CXXRecordDecl* derived = from->getAsCXXRecordDecl();
		for (const CXXBaseSpecifier* base : cast->path())
		{
			const CXXRecordDecl* b = base->getType()->getAsCXXRecordDecl();
			if (!derived || !b || base->isVirtual() ||
				!m_context->getASTRecordLayout(derived).getBaseClassOffset(b).isZero())
				return false;
			derived = b;
		}
		return true;
	}

	// an lvalue's address
	std::optional<std::string> stepAddress(const Expr* e, const StepScope& scope, int depth)
	{
		e = e->IgnoreParens();
		if (const auto* ref = dyn_cast<DeclRefExpr>(e))
		{
			// an exe global (FNVD_GLOBAL, FNVD_STATIC_MEMBER): its address
			if (const auto* var = dyn_cast<VarDecl>(ref->getDecl()); var && !isa<ParmVarDecl>(var))
				return exeGlobal(var);
			// a reference parameter: the address it holds
			const auto* param = dyn_cast<ParmVarDecl>(ref->getDecl());
			auto found = param && param->getType()->isReferenceType() ? scope.params.find(param) : scope.params.end();
			return found != scope.params.end() ? std::optional(found->second) : std::nullopt;
		}
		if (const auto* member = dyn_cast<MemberExpr>(e))
		{
			const auto* field = dyn_cast<FieldDecl>(member->getMemberDecl());
			if (!field || field->isBitField())
				return std::nullopt;
			std::optional<std::string> base = member->isArrow() ? stepValue(member->getBase(), scope, depth, false)
																: stepAddress(member->getBase(), scope, depth);
			return base ? std::optional(stepPlus(*base, fieldOffset(field))) : std::nullopt;
		}
		if (const auto* unary = dyn_cast<UnaryOperator>(e); unary && unary->getOpcode() == UO_Deref)
			return stepValue(unary->getSubExpr(), scope, depth, false);
		if (const auto* cast = dyn_cast<CastExpr>(e))
		{
			if (cast->getCastKind() == CK_NoOp ||
				((cast->getCastKind() == CK_DerivedToBase || cast->getCastKind() == CK_UncheckedDerivedToBase) &&
				 zeroBasePath(cast)))
				return stepAddress(cast->getSubExpr(), scope, depth);
		}
		return std::nullopt;
	}

	// the address of an exe global as the hybrid declares it, a reference bound to `*reinterpret_cast<T*>(ADDRESS)`:
	// `#ADDRESS`
	std::optional<std::string> exeGlobal(const VarDecl* var) const
	{
		const VarDecl* defined = nullptr;
		const Expr* init = var->getType()->isReferenceType() && var->hasGlobalStorage() ? var->getAnyInitializer(defined)
																						: nullptr;
		const auto* deref = init ? dyn_cast<UnaryOperator>(init->IgnoreParenImpCasts()) : nullptr;
		const auto* cast =
			deref && deref->getOpcode() == UO_Deref ? dyn_cast<ExplicitCastExpr>(deref->getSubExpr()->IgnoreParens())
													: nullptr;
		Expr::EvalResult result;
		if (!cast || !cast->getType()->isPointerType() || cast->getSubExpr()->isValueDependent() ||
			!cast->getSubExpr()->EvaluateAsInt(result, *m_context))
			return std::nullopt;
		return "#" + std::to_string(result.Val.getInt().getExtValue());
	}

	std::vector<Row> paramRows(const char* table, const std::string& key, const FunctionDecl* function) const
	{
		std::vector<Row> rows;
		int64_t index = 0;
		for (const ParmVarDecl* param : function->parameters())
			rows.push_back({table, {key, number(index++), param->getName().str()}});
		return rows;
	}

	// Where a declaration's text starts: its template heads and C++11 attributes included (expansion locations).
	SourceLocation declarationStart(const FunctionDecl* decl) const
	{
		const SourceManager& sm = m_ci.getSourceManager();
		SourceLocation start = sm.getExpansionLoc(decl->getOuterLocStart());
		auto earlier = [&](SourceLocation at) {
			at = at.isValid() ? sm.getExpansionLoc(at) : at;
			if (at.isValid() && (start.isInvalid() || sm.isBeforeInTranslationUnit(at, start)))
				start = at;
		};
		if (const FunctionTemplateDecl* pattern = decl->getDescribedFunctionTemplate())
			earlier(pattern->getBeginLoc());
		if (!decl->getTemplateParameterLists().empty())
			earlier(decl->getTemplateParameterLists().front()->getTemplateLoc());
		for (const Attr* attribute : decl->attrs())
			if (!attribute->isInherited() && attribute->getLocation().isFileID())
				earlier(attribute->getLocation());
		return start;
	}

	// The contiguous `//` comment lines right above the line of start (a file location), top first, trimmed.
	std::vector<StringRef> commentLines(SourceLocation start) const
	{
		std::vector<StringRef> lines;
		if (start.isInvalid())
			return lines;
		const SourceManager& sm = m_ci.getSourceManager();
		auto [file, offset] = sm.getDecomposedLoc(start);
		StringRef text = sm.getBufferData(file);
		size_t lineStart = text.substr(0, offset).rfind('\n') + 1;  // npos + 1: the first line
		while (lineStart > 0)
		{
			size_t end = lineStart - 1;
			size_t previous = text.substr(0, end).rfind('\n') + 1;
			StringRef line = text.slice(previous, end).trim();
			if (!line.starts_with("//"))
				break;
			lines.push_back(line);
			lineStart = previous;
		}
		std::reverse(lines.begin(), lines.end());
		return lines;
	}

	// The provenance markers one declaration or definition of a function gives it, as function_marker rows (`srcdb
	// index` turns them into function.provenance and function_slot; Model.hpp): `slot <s>` for each slot a
	// declaration's evidence covers (the return, `ret`, unless void or a record returned by value; each parameter by
	// its index from 0; a Construct's or DeletingDestruct's first parameter and return, the porting forms, are no
	// slots); `inferred` for a line starting `// Inferred type` in the comment lines right above it, and `inferred
	// <s>` for each slot its clauses name (`<slot>: <evidence>`, separated by `; `, the slot a parameter's name,
	// `return` or `parameter <k>`; continued in the comment lines up to the next marker line); `guess` for a name
	// marker of RULES.md there (`// IDA name; not in the PDB.`, `// Not named in the PDB or IDA; name chosen here.`,
	// also after `// Not reconstructed yet.` on its line), for parameters all named as IDA names them, or for no
	// parameters and an IDA placeholder name; `macro` for one a macro's body spells, whose comment lines src does not
	// write. A line `// PDB: <form>; the exe <fact>.` there records a slot whose type differs from the PDB's for the
	// exe's reason (`idakit declare --retype` writes them): `pdb <slot> <line>`, the slot `ret` for a form `returns
	// <type>`, `*` (every slot) for a whole signature (a form with a parameter list), else the parameter whose name is
	// the form's last word (its index from 0); a form that names no slot so gives no marker.
	void markers(const std::string& key, const FunctionDecl* decl)
	{
		auto mark = [&](const std::string& marker) { m_refs.push_back({"function_marker", {key, marker}}); };
		static const std::regex idaParameter(R"((a|arg|x|(e?[abcd]x|e?[sd]i|e?bp|[abcd][lh]|xmm|st))\d+[a-z]?)");
		static const std::regex placeholder(R"(_?sub_[0-9A-Fa-f]+)");
		ArrayRef<ParmVarDecl*> parameters = decl->parameters();
		std::string name = decl->getDeclName().isIdentifier() ? decl->getName().str() : "";
		bool porting = false;
		for (const char* form : {"Construct", "DeletingDestruct"})
			porting = porting || name == form || name.starts_with(std::string(form) + "_");
		QualType ret = decl->getReturnType().getCanonicalType();
		if (!porting && !ret->isVoidType() && !ret->isRecordType())
			mark("slot ret");
		for (size_t k = porting ? 1 : 0; k < parameters.size(); ++k)
			mark("slot " + std::to_string(k));
		if (!parameters.empty() ? std::all_of(parameters.begin(), parameters.end(),
											  [](const ParmVarDecl* p) {
												  return std::regex_match(p->getName().str(), idaParameter);
											  })
								: decl->getDeclName().isIdentifier() &&
									  std::regex_match(decl->getName().str(), placeholder))
			mark("guess");
		SourceLocation at = decl->getLocation();
		if (at.isMacroID() && m_ci.getSourceManager().isMacroBodyExpansion(at))
		{
			mark("macro");
			return;
		}
		std::vector<StringRef> comments = commentLines(declarationStart(decl));
		// a line of another marker ends an `// Inferred type` comment's clauses (a `// Guessed type` line is no
		// evidence: idakit infer's guessed slots, which stay guesses)
		auto markerLine = [](StringRef text) {
			return text.starts_with("Inferred type") || text.starts_with("Guessed type") || text.starts_with("PDB:") ||
				   text.starts_with("Havok SDK ") ||
				   text.contains("IDA name; not in the PDB.") || text.contains("Not named in the PDB or IDA") ||
				   text.contains("Not reconstructed yet.");
		};
		for (size_t l = 0; l < comments.size(); ++l)
		{
			StringRef line = comments[l];
			StringRef text = line.drop_front(2).trim();
			if (text.starts_with("Inferred type"))
			{
				mark("inferred");
				// the clauses: the line and its continuation lines joined, each slot named as `<slot>: ` first or
				// after `; `
				std::string clauses = text.drop_front(13).ltrim(": ").str();
				for (size_t m = l + 1; m < comments.size() && !markerLine(comments[m].drop_front(2).trim()); ++m)
					clauses += " " + comments[m].drop_front(2).trim().str();
				auto named = [&](const std::string& slot) {
					return clauses.starts_with(slot + ": ") || clauses.find("; " + slot + ": ") != std::string::npos;
				};
				if (!porting && named("return"))
					mark("inferred ret");
				// a parameter by its name (an unnamed one by its name in another declaration, as the model's param
				// rows give it) or by its index
				for (size_t k = porting ? 1 : 0; k < parameters.size(); ++k)
				{
					std::string parameter;
					for (const FunctionDecl* r : decl->redecls())
						if (parameter.empty() && k < r->getNumParams())
							parameter = r->getParamDecl(unsigned(k))->getName().str();
					if ((!parameter.empty() && named(parameter)) || named("parameter " + std::to_string(k)))
						mark("inferred " + std::to_string(k));
				}
			}
			else if (text.starts_with("Havok SDK signature ("))
				mark("library");
			else if (text.contains("IDA name; not in the PDB.") ||
					 text.contains("Not named in the PDB or IDA; name chosen here.") ||
					 text.contains("Not named in the PDB or IDA; names chosen here") ||
					 text.contains("Havok SDK name; not in the PDB."))
				mark("guess");
			else if (text.starts_with("PDB:") && text.contains("; the exe "))
			{
				StringRef form = text.drop_front(4).split("; the exe ").first.trim();
				std::string slot;
				if (form.starts_with("returns "))
					slot = "ret";
				else if (form.contains('('))
					slot = "*";
				else
				{
					size_t end = form.size();
					while (end > 0 && (std::isalnum(static_cast<unsigned char>(form[end - 1])) || form[end - 1] == '_'))
						--end;
					StringRef word = form.substr(end);
					for (size_t k = 0; k < parameters.size() && slot.empty(); ++k)
						if (!word.empty() && parameters[k]->getName() == word)
							slot = std::to_string(k);
				}
				if (!slot.empty())
					mark("pdb " + slot + " " + line.str());
			}
		}
	}

	// The location lies inside an emitter's guarded block: from a line `#if FNVD_HYBRID  // emitted...` to its
	// `#endif  // emitted` (SrcEdit's markers, any blanks around their words; only those two lines count).
	bool emitted(SourceLocation location)
	{
		const SourceManager& sm = m_ci.getSourceManager();
		location = sm.getExpansionLoc(location);
		if (location.isInvalid())
			return false;
		auto [file, offset] = sm.getDecomposedLoc(location);
		auto found = m_emittedBlocks.find(file);
		if (found == m_emittedBlocks.end())
		{
			static const std::regex opener(R"(\s*#\s*if\s+FNVD_HYBRID\s*//\s*emitted\b.*)");
			static const std::regex closer(R"(\s*#\s*endif\s*//\s*emitted\b.*)");
			std::vector<std::pair<size_t, size_t>> blocks;
			StringRef text = sm.getBufferData(file);
			std::optional<size_t> open;
			for (size_t begin = 0; begin < text.size();)
			{
				size_t end = std::min(text.find('\n', begin), text.size());
				StringRef line = text.slice(begin, end).rtrim();
				if (line.contains("emitted"))
				{
					std::string spelled = line.str();
					if (!open && std::regex_match(spelled, opener))
						open = begin;
					else if (open && std::regex_match(spelled, closer))
						blocks.emplace_back(*open, end), open.reset();
				}
				begin = end + 1;
			}
			if (open)
				blocks.emplace_back(*open, text.size());
			found = m_emittedBlocks.emplace(file, std::move(blocks)).first;
		}
		return std::any_of(found->second.begin(), found->second.end(),
						   [&](const std::pair<size_t, size_t>& block) {
							   return offset >= block.first && offset < block.second;
						   });
	}

	void function(const FunctionDecl* function)
	{
		// an explicit specialization's first declaration can be the one clang made at the template (a header) when
		// a call named it; the specialization belongs to the file that spells it: its definition, else its latest
		// declaration
		const FunctionDecl* spelled = function;
		if (function->getTemplateSpecializationKind() == TSK_ExplicitSpecialization)
			spelled = function->getDefinition() ? function->getDefinition() : function->getMostRecentDecl();
		Owner who = owner(spelled);
		std::string key = functionKey(globalOf(function));
		abi(function, key);
		accessor(function, key);
		steps(function, key);
		// a template's instance (a class template instance's member, a function template's instantiation) is defined
		// by its template's definition, which clang instantiates where a TU uses it (instances are not visited, so
		// this is their only definition row); an instance whose template has none has no row
		if (const FunctionDecl* pattern = function->getTemplateInstantiationPattern())
			if (const FunctionDecl* body = pattern->getDefinition())
				definition(function, body);
		if (!wanted(who, "function", "mangled", key))
			return;
		Cell cls, self, structor, empty;
		bool isStatic = function->getStorageClass() == SC_Static, isVirtual = false, isConst = false;
		if (auto* method = dyn_cast<CXXMethodDecl>(function))
		{
			cls = recordName(method->getParent());
			isStatic = method->isStatic();
			isVirtual = method->isVirtual();
			isConst = method->isConst();
			if (method->isInstance())
				self = typeKey(method->getThisType());
			if (isa<CXXConstructorDecl>(method) || isa<CXXDestructorDecl>(method))
			{
				structor = isa<CXXConstructorDecl>(method) ? "ctor" : "dtor";
				empty = flag(emptyStructor(method));
			}
		}
		const FunctionDecl* source = declared(function);
		const FileInfo& file = fileAt(declaredAt(function));
		// an instance's member made from the class template's member: the member's pattern key, which the forwarders
		// inside it name (not an explicit specialization, whose code is its own). A row of its own, outside the
		// function's digest: a TU that sees no specialization src spells for the instance takes the member for an
		// instantiation (SrcModel drops those src specializes).
		if (const FunctionDecl* member = function->getInstantiatedFromMemberFunction();
			member && isTemplateInstantiation(function->getTemplateSpecializationKind()))
			m_refs.push_back({"function_pattern", {key, patternKey(member)}});
		// the arguments a call must pass, where default arguments let it pass fewer than the parameters
		std::vector<Row> children = paramRows("param", key, source);
		if (unsigned required = function->getMostRecentDecl()->getMinRequiredArguments();
			required < function->getNumParams())
			children.push_back({"default_args", {key, number(required)}});
		addItem("mangled",
				{"function",
				 {key, qualified(function), textOrNull(file.path), number(line(declaredAt(function))),
				  typeKey(function->getType()), cls, self, flag(isStatic), flag(isVirtual),
				  flag(function->isPureVirtual()), flag(isConst), structor, flag(function->isVariadic()), empty,
				  memberAccess(function), flag(function->isNoReturn()), kProvenancePending}},
				std::move(children));
		if (!function->isImplicit())
			markers(key, source);
	}

	// A template's function (a class template's member, a function template) by its first declaration, which has no
	// mangled name: `pattern:<qualified name> @<file>:<line>`
	std::string patternKey(const FunctionDecl* pattern)
	{
		pattern = pattern->getFirstDecl();
		SourceLocation at = pattern->getLocation();
		return "pattern:" + qualified(pattern) + " @" + fileAt(at).path + ":" + std::to_string(line(at));
	}

	// A member of a class template (not itself a function template): each instance's member is made from it
	static bool classTemplateMember(const FunctionDecl* function)
	{
		auto* method = dyn_cast<CXXMethodDecl>(function);
		return method && method->getTemplatedKind() == FunctionDecl::TK_NonTemplate &&
			   method->getParent()->isDependentContext();
	}

	// function.access: a member's access from namespace scope, the narrowest of its own and that of each class it is
	// nested in (public, protected, private); NULL for a free function
	static Cell memberAccess(const FunctionDecl* function)
	{
		const auto* method = dyn_cast<CXXMethodDecl>(function->getFirstDecl());
		if (!method)
			return Cell();
		AccessSpecifier narrowest = AS_public;
		for (const Decl* d = method; d;)
		{
			// AS_public < AS_protected < AS_private; a namespace-scope class's is AS_none
			if (AccessSpecifier a = d->getAccess(); a != AS_none && a > narrowest)
				narrowest = a;
			d = dyn_cast<CXXRecordDecl>(d->getDeclContext());
		}
		return std::string(narrowest == AS_public ? "public" : narrowest == AS_protected ? "protected" : "private");
	}

	// A function template, free or a member of a class that is no template, whose one template parameter is its
	// trailing function parameter pack, every other parameter and its result of a type no template parameter names
	// (`template <class... A> int f(char*, A...)`): a call passes the pack as C's `...` would
	static bool packTemplate(const FunctionDecl* function)
	{
		const FunctionTemplateDecl* pattern = function->getDescribedFunctionTemplate();
		auto* method = dyn_cast<CXXMethodDecl>(function);
		if (!pattern || (method && method->getParent()->isDependentContext()) || function->getNumParams() == 0 ||
			function->getReturnType()->isDependentType())
			return false;
		const TemplateParameterList* parameters = pattern->getTemplateParameters();
		if (parameters->size() != 1 || !parameters->getParam(0)->isTemplateParameterPack())
			return false;
		unsigned last = function->getNumParams() - 1;
		if (!function->getParamDecl(last)->isParameterPack())
			return false;
		for (unsigned k = 0; k < last; ++k)
			if (function->getParamDecl(k)->getType()->isDependentType())
				return false;
		return true;
	}

	// packTemplate's function row: by its pattern key, the C-variadic function of the parameters before the pack.
	// Its convention is a C-variadic function's (cdecl, a member's too) unless the template spells one.
	void packFunction(const FunctionDecl* function, const std::string& key)
	{
		const FunctionDecl* first = function->getFirstDecl();
		if (!wanted(owner(first), "function", "mangled", key))
			return;
		auto* method = dyn_cast<CXXMethodDecl>(first);
		bool instance = method && method->isInstance();
		CallingConv cc = function->getType()->getAs<FunctionProtoType>()->getCallConv();
		if (cc == m_context->getDefaultCallingConvention(false, instance))
			cc = m_context->getDefaultCallingConvention(true, instance);
		std::vector<QualType> fixed;
		std::vector<Row> params;
		for (unsigned k = 0; k + 1 < first->getNumParams(); ++k)
		{
			fixed.push_back(first->getParamDecl(k)->getType());
			params.push_back({"param", {key, number(k), first->getParamDecl(k)->getName().str()}});
		}
		bool isConst = method && method->isConst();
		std::string type = functionTypeKey(callingConvention(cc), function->getReturnType(), fixed, true, isConst);
		Cell cls, self;
		if (method)
			cls = recordName(method->getParent());
		if (instance)
			self = typeKey(method->getThisType());
		addItem("mangled",
				{"function",
				 {key, qualified(first), textOrNull(fileAt(first->getLocation()).path),
				  number(line(first->getLocation())), type, cls, self, flag(method && method->isStatic()), flag(false),
				  flag(false), flag(isConst), Cell{}, flag(true), Cell{}, memberAccess(first), flag(first->isNoReturn()),
				  kProvenancePending}},
				std::move(params));
		markers(key, first);
	}

	// the function's definition: its own, or (body) the template definition an instance gets
	void definition(const FunctionDecl* function, const FunctionDecl* body = nullptr)
	{
		Owner who = owner(function);
		std::string key = functionKey(globalOf(function));
		// an explicit specialization's own definition is built also where another TU wrote the template's body for
		// it (write() replaces that row)
		bool strong = !body && function->getTemplateSpecializationKind() == TSK_ExplicitSpecialization;
		if (!wanted(who, "definition", "mangled", key) && !(strong && who != Owner::Skip && instanceStored(key)))
			return;
		noteFunction(function);
		abi(function, key);
		if (!body)
			body = function;
		const FileInfo& file = fileAt(body->getLocation());
		addItem("mangled",
				{"definition",
				 {key, qualified(function), textOrNull(file.path), number(line(body->getLocation())),
				  number(line(body->getEndLoc())), flag(body != function), flag(emitted(body->getLocation()))}},
				paramRows("definition_param", key, body));
		markers(key, body);
	}

	// --- refs -------------------------------------------------------------------------------------------------

	// One argument of a forwarder's fnvd::original call (a ref_param row).
	struct ForwardedArgument
	{
		std::string name;  // the forwarding function's parameter it passes on; empty where none
		bool literal = false;
		std::optional<int64_t> value;  // an integer constant's value, as the forwarder passes it
	};

	// What a ref row carries past its location, kind, form and address.
	struct RefDetail
	{
		std::string target;
		const FunctionDecl* function = nullptr;
		std::string pattern;  // the function's pattern key where it is a template's (no mangled name)
		Cell entry, type, local, context;
		std::vector<ForwardedArgument> arguments;
		// a usercall's parameter locations (ref_location): parameter index (`this` -1), register or `stack`
		std::vector<std::pair<int64_t, std::string>> locations;
	};

	const Expansion* expansionAt(SourceLocation location) const
	{
		location = m_ci.getSourceManager().getExpansionLoc(location);
		auto found = m_expansions.find(location.getRawEncoding());
		return found == m_expansions.end() ? nullptr : &found->second;
	}

	// The exe function's type as a forwarder calls it: the convention its fnvd::original:: template names, the
	// result it asks for and the arguments it pushes (the template's by-value parameters after the address); the
	// names are those of the forwarding function's own parameters it passes on. A usercall's registers are no
	// convention: it gets none.
	void forwardedType(StringRef form, const CallExpr* call, const FunctionDecl* original, RefDetail& detail)
	{
		const char* cc = form == "cdecl_"	 ? "cdecl"
						 : form == "thiscall" ? "thiscall"
						 : form == "stdcall"  ? "stdcall"
						 : form == "fastcall" ? "fastcall"
											  : nullptr;
		if (!cc)
			return;
		std::vector<QualType> params;
		for (unsigned k = 1; k < original->getNumParams() && k < call->getNumArgs(); ++k)
		{
			QualType pushed = original->getParamDecl(k)->getType();
			// as written: implicit copies of a by-value argument and implicit conversions are not the argument
			const Expr* written = call->getArg(k)->IgnoreUnlessSpelledInSource();
			const Expr* argument = written->IgnoreParenCasts();
			ForwardedArgument forwarded;
			forwarded.literal = isUntypedConstant(written->IgnoreParens());
			forwarded.value = constant(written);
			if (const ParmVarDecl* parameter = parameterOf(argument))
			{
				forwarded.name = parameter->getName().str();
				if (QualType declared = parameter->getType().getUnqualifiedType(); sameSlot(declared, pushed))
					pushed = declared;
			}
			else if (auto* unary = dyn_cast<UnaryOperator>(argument);
					 unary && (unary->getOpcode() == UO_AddrOf || unary->getOpcode() == UO_Deref))
			{
				if (const ParmVarDecl* parameter = parameterOf(unary->getSubExpr()->IgnoreParenCasts()))
					forwarded.name = parameter->getName().str();
			}
			else if (k == 1 && std::string_view(cc) == "thiscall" && isa<CXXThisExpr>(argument))
				forwarded.name = "this";
			params.push_back(pushed);
			detail.arguments.push_back(std::move(forwarded));
		}
		detail.type = functionTypeKey(cc, original->getReturnType(), params, false, false);
	}

	// The arguments of a forwarder's call whose type depends on a template's parameters (a class template's member):
	// no exe type, but each argument's name as forwardedType gives it, and the value of an integer constant the
	// template's parameters do not change.
	void forwardedArguments(const CallExpr* call, RefDetail& detail)
	{
		for (unsigned k = 1; k < call->getNumArgs(); ++k)
		{
			const Expr* written = call->getArg(k)->IgnoreUnlessSpelledInSource();
			const Expr* argument = written->IgnoreParenCasts();
			ForwardedArgument forwarded;
			if (!written->isValueDependent() && !written->isTypeDependent())
			{
				forwarded.literal = isUntypedConstant(written->IgnoreParens());
				forwarded.value = constant(written);
			}
			if (const ParmVarDecl* parameter = parameterOf(argument))
				forwarded.name = parameter->getName().str();
			else if (k == 1 && isa<CXXThisExpr>(argument))
				forwarded.name = "this";
			detail.arguments.push_back(std::move(forwarded));
		}
	}

	// Where a usercall forwarder passes its function's parameters (ref_location): the register its UserRegs
	// designator loads (the parameter, its address for a reference, or `this`, under casts), else the stack. None
	// where the call passes anything else or a parameter twice or not at all (a call from another body), or its
	// stack arguments out of the parameters' order.
	void usercallLocations(const CallExpr* call, const FunctionDecl* around, RefDetail& detail)
	{
		static const char* const kRegisters[] = {"eax", "ecx", "edx", "ebx", "esi", "edi"};
		if (!around || call->getNumArgs() < 2)
			return;
		auto passed = [&](const Expr* e) -> std::optional<int64_t> {
			e = e->IgnoreUnlessSpelledInSource()->IgnoreParenCasts();
			if (auto* unary = dyn_cast<UnaryOperator>(e); unary && unary->getOpcode() == UO_AddrOf)
				e = unary->getSubExpr()->IgnoreParenCasts();
			if (isa<CXXThisExpr>(e))
				return -1;
			const ParmVarDecl* parameter = parameterOf(e);
			if (!parameter || parameter->getDeclContext() != around)
				return std::nullopt;
			return static_cast<int64_t>(parameter->getFunctionScopeIndex());
		};
		// the UserRegs braced initialiser, under the temporary's copy and the functional cast
		const Expr* e = call->getArg(1);
		const InitListExpr* list = nullptr;
		for (int depth = 0; e && !list && depth < 8; ++depth)
		{
			e = e->IgnoreImplicit()->IgnoreParens();
			if (auto* init = dyn_cast<InitListExpr>(e))
				list = init->isSemanticForm() ? init : init->getSemanticForm();
			else if (auto* construct = dyn_cast<CXXConstructExpr>(e); construct && construct->getNumArgs() == 1)
				e = construct->getArg(0);
			else if (auto* cast = dyn_cast<CastExpr>(e))
				e = cast->getSubExpr();
			else if (auto* temporary = dyn_cast<MaterializeTemporaryExpr>(e))
				e = temporary->getSubExpr();
			else
				e = nullptr;
		}
		if (!list || list->getNumInits() > std::size(kRegisters))
			return;
		std::map<int64_t, std::string> where;
		for (unsigned k = 0; k < list->getNumInits(); ++k)
		{
			const Expr* init = list->getInit(k);
			if (!init || isa<ImplicitValueInitExpr>(init) || isa<CXXDefaultInitExpr>(init))
				continue;
			std::optional<int64_t> index = passed(init);
			if (!index || !where.emplace(*index, kRegisters[k]).second)
				return;
		}
		int64_t last = -1;
		for (unsigned k = 2; k < call->getNumArgs(); ++k)
		{
			std::optional<int64_t> index = passed(call->getArg(k));
			if (!index || *index <= last || !where.emplace(*index, "stack").second)
				return;
			last = *index;
		}
		auto* method = dyn_cast<CXXMethodDecl>(around);
		bool self = method && method->isInstance();
		if (where.size() != around->getNumParams() + (self ? 1 : 0) || (where.contains(-1) != self))
			return;
		for (const auto& [index, location] : where)
			detail.locations.emplace_back(index, location);
	}

	static const ParmVarDecl* parameterOf(const Expr* expression)
	{
		auto* ref = dyn_cast<DeclRefExpr>(expression);
		return ref ? dyn_cast<ParmVarDecl>(ref->getDecl()) : nullptr;
	}

	// A constant the forwarder passes without a cast (0, -1, ~0u, nullptr, a macro's value): its type is the
	// expression's own (int, nullptr_t, double), not the exe parameter's. An enum constant keeps its enum.
	bool isUntypedConstant(const Expr* written) const
	{
		QualType type = written->getType();
		if (isa<ExplicitCastExpr>(written) || written->isValueDependent() || type->isEnumeralType())
			return false;
		return type->isNullPtrType() || (type->isArithmeticType() && written->isEvaluatable(*m_context));
	}

	// An integer parameter (bool and enums included) the forwarder casts before pushing keeps its declared type in
	// the same stack slot: the cast only widens it for the push. A pointer cast is kept: it can be a base subobject's
	// adjustment, the class the exe really takes.
	bool sameSlot(QualType declared, QualType pushed) const
	{
		if (declared->isDependentType() || pushed->isDependentType() || declared->isIncompleteType() ||
			!declared->isIntegralOrEnumerationType() || !pushed->isIntegralOrEnumerationType())
			return false;
		auto slots = [&](QualType type) { return (m_context->getTypeSize(type) / 8 + 3) / 4; };
		return slots(declared) == slots(pushed);
	}

	std::optional<int64_t> constant(const Expr* expression) const
	{
		if (!expression || expression->isValueDependent())
			return std::nullopt;
		Expr::EvalResult result;
		if (!expression->EvaluateAsInt(result, *m_context))
			return std::nullopt;
		return result.Val.getInt().getExtValue();
	}

	void addRef(SourceLocation at, const char* kind, std::string form, int64_t address, RefDetail detail)
	{
		const SourceManager& sm = m_ci.getSourceManager();
		at = sm.getExpansionLoc(at);
		Cell function;
		if (detail.function && usable(detail.function))
		{
			noteFunction(detail.function);
			function = functionKey(globalOf(detail.function));
		}
		else if (!detail.pattern.empty())
			function = detail.pattern;
		const std::string& path = fileAt(at).path;
		Cell line = number(sm.getExpansionLineNumber(at)), column = number(sm.getExpansionColumnNumber(at));
		for (size_t k = 0; k < detail.arguments.size(); ++k)
		{
			const ForwardedArgument& argument = detail.arguments[k];
			m_refs.push_back({"ref_param", {path, line, column, address, number(k), textOrNull(argument.name),
											flag(argument.literal), argument.value ? Cell(*argument.value) : Cell()}});
		}
		for (const auto& [index, location] : detail.locations)
			m_refs.push_back({"ref_location", {path, line, column, address, index, location}});
		m_refs.push_back({"ref",
						  {path, line, column, kind, std::move(form), address, textOrNull(detail.target), function,
						   detail.entry, detail.type, detail.local, detail.context}});
	}

	// the function the members of a FNVD_REPLACES_VIRTUAL helper struct name, and the class the exe passes as this
	std::pair<const CXXMethodDecl*, std::string> virtualTarget(const CXXRecordDecl* helper)
	{
		const CXXRecordDecl* access = nullptr;
		const TypedefNameDecl* pointer = nullptr;
		const ClassTemplateDecl* adapter = nullptr;
		for (const Decl* decl : helper->decls())
		{
			if (auto* record = dyn_cast<CXXRecordDecl>(decl); record && record->getName() == "FnvdAccess")
				access = record;
			else if (auto* alias = dyn_cast<TypedefNameDecl>(decl); alias && alias->getName() == "Pointer")
				pointer = alias;
			else if (auto* pattern = dyn_cast<ClassTemplateDecl>(decl); pattern && pattern->getName() == "Adapter")
				adapter = pattern;
		}
		std::vector<const CXXMethodDecl*> candidates;
		if (access)
		{
			for (const Decl* decl : access->decls())
				if (auto* use = dyn_cast<UsingDecl>(decl))
					for (const UsingShadowDecl* shadow : use->shadows())
						if (auto* method = dyn_cast<CXXMethodDecl>(shadow->getTargetDecl()))
							candidates.push_back(method);
		}
		const CXXMethodDecl* chosen = candidates.size() == 1 ? candidates[0] : nullptr;
		if (!chosen && pointer)
		{
			if (auto* member = pointer->getUnderlyingType()->getAs<MemberPointerType>())
			{
				QualType wanted = m_context->getCanonicalType(member->getPointeeType());
				for (const CXXMethodDecl* method : candidates)
					if (m_context->getCanonicalType(method->getType()) == wanted)
						chosen = method;
			}
		}
		std::string entry;
		if (adapter)
		{
			for (const ClassTemplateSpecializationDecl* instance : adapter->specializations())
			{
				if (!instance->hasDefinition())
					continue;
				for (const CXXMethodDecl* method : instance->methods())
				{
					if (method->getName() != "direct" || method->getNumParams() == 0)
						continue;
					if (const CXXRecordDecl* self = method->getParamDecl(0)->getType()->getPointeeCXXRecordDecl())
						entry = recordName(self);
				}
			}
		}
		return {chosen, entry};
	}

	// --- output -----------------------------------------------------------------------------------------------

	// INSERT OR IGNORE of a row; true when it was new
	bool insert(const Row& row)
	{
		auto& statement = m_inserts[{row.table, row.cells.size()}];
		if (!statement)
		{
			std::string sql = std::string("INSERT OR IGNORE INTO ") + row.table + " VALUES (?";
			for (size_t k = 1; k < row.cells.size(); ++k)
				sql += ", ?";
			statement = std::make_unique<Statement>(m_store.handle(), (sql + ")").c_str());
		}
		for (size_t k = 0; k < row.cells.size(); ++k)
		{
			const Cell& cell = row.cells[k];
			int index = static_cast<int>(k + 1);
			if (auto* value = std::get_if<int64_t>(&cell))
				statement->bind(index, *value);
			else if (auto* text = std::get_if<std::string>(&cell))
				statement->bind(index, *text);
			else
				statement->bindNull(index);
		}
		statement->run();
		return m_store.changes() > 0;
	}

	// A function's own definition (an explicit specialization's) and its template's body instantiated for it (instance
	// 1) under one mangled name: the explicit specialization is the program's definition (the linker's strong one over
	// the instances' COMDAT copies), whichever TU wrote first. A TU that instantiates the template's body there
	// does not see the specialization (an ODR slip, src's to fix); the model keeps the one the program runs.
	std::optional<bool> storedInstance(const std::string& key)
	{
		Statement read(m_store.handle(), "SELECT instance FROM definition WHERE mangled = ?");
		read.bind(1, key);
		if (!read.step())
			return std::nullopt;
		return read.integer(0) != 0;
	}
	bool instanceStored(const std::string& key) { return storedInstance(key).value_or(false); }
	static bool instanceRow(const Row& definition) { return std::get<int64_t>(definition.cells[5]) != 0; }
	static const std::string& rowKey(const Row& row) { return std::get<std::string>(row.cells.front()); }
	// the row is the specialization's own definition where the store holds the template's body
	bool strongerDefinition(const Row& definition)
	{
		return !instanceRow(definition) && instanceStored(rowKey(definition));
	}
	// the row is the template's body where the store holds the specialization's own definition
	bool weakerDefinition(const Row& definition)
	{
		return instanceRow(definition) && !storedInstance(rowKey(definition)).value_or(true);
	}

	void write()
	{
		m_store.exec("BEGIN IMMEDIATE");
		for (const auto& [key, rows] : m_types)
			if (insert(rows.first))
				for (const Row& param : rows.second)
					insert(param);
		Statement conflict(m_store.handle(), "INSERT INTO conflict VALUES (?, ?, ?)");
		for (const Item& item : m_items)
		{
			if (insert(item.main))
			{
				for (const Row& child : item.children)
					insert(child);
				continue;
			}
			const std::string& key = std::get<std::string>(item.main.cells.front());
			if (item.main.table == std::string("definition") && strongerDefinition(item.main))
			{
				for (const char* sql : {"DELETE FROM definition WHERE mangled = ?",
										"DELETE FROM definition_param WHERE definition = ?"})
				{
					Statement erase(m_store.handle(), sql);
					erase.bind(1, key);
					erase.run();
				}
				insert(item.main);
				for (const Row& child : item.children)
					insert(child);
				continue;
			}
			// the key is there: a conflict when its digest differs
			std::string sql =
				std::string("SELECT digest FROM ") + item.main.table + " WHERE " + item.keyColumn + " = ?";
			Statement read(m_store.handle(), sql.c_str());
			read.bind(1, key);
			if (read.step() && read.integer(0) != std::get<int64_t>(item.main.cells.back()) &&
				!(item.main.table == std::string("definition") && weakerDefinition(item.main)))
			{
				// the file column: after the name for functions and definitions, after the key for the others
				std::string table = item.main.table;
				size_t column = table == "function" || table == "definition" ? 2 : table == "abi" ? 0 : 1;
				conflict.bind(1, table).bind(2, key);
				if (auto* path = column ? std::get_if<std::string>(&item.main.cells[column]) : nullptr)
					conflict.bind(3, *path);
				conflict.run();
			}
		}
		for (const Row& ref : m_refs)
			insert(ref);
		m_inserts.clear();
		m_store.exec("COMMIT");
	}

	CompilerInstance& m_ci;
	Store& m_store;
	const IndexScope& m_scope;
	const Expansions& m_expansions;
	RelativePaths m_paths;
	PrintingPolicy m_policy;
	ASTContext* m_context = nullptr;
	llvm::LLVMContext m_llvm;
	std::unique_ptr<CodeGenerator> m_codegen;
	std::unique_ptr<MangleContext> m_mangle;
	// node-based: fileAt hands out references that later insertions must not move
	std::unordered_map<FileID, FileInfo, FileIdHash> m_files;
	// per file, its emitter's guarded blocks as byte ranges (emitted())
	std::unordered_map<FileID, std::vector<std::pair<size_t, size_t>>, FileIdHash> m_emittedBlocks;

	std::vector<const RecordDecl*> m_records;
	std::vector<const EnumDecl*> m_enums;
	std::vector<const TypedefNameDecl*> m_typedefs;
	std::vector<const FunctionDecl*> m_functions;
	std::vector<const FunctionDecl*> m_definitions;
	std::set<const Decl*> m_recordSeen, m_enumSeen, m_typedefSeen, m_functionSeen, m_definitionSeen;
	std::set<std::string> m_itemSeen;

	std::map<std::string, std::pair<Row, std::vector<Row>>> m_types;  // key -> type row, type_param rows
	std::vector<Item> m_items;
	std::vector<Row> m_refs;
	std::map<std::pair<std::string, size_t>, std::unique_ptr<Statement>> m_inserts;

	friend class Visitor;
};

void Collector::registration(const VarDecl* var)
{
	const Expansion* macro = expansionAt(var->getLocation());
	if (!macro || !ownsLocation(var->getLocation()))
		return;
	StringRef name = macro->name;
	RefDetail detail;
	if (auto* around = dyn_cast_or_null<FunctionDecl>(var->getParentFunctionOrMethod()))
		detail.context = qualified(around);
	if (name.starts_with("FNVD_REPLACES"))
	{
		if (!var->getName().starts_with("fnvdReplacement"))
			return;
		std::vector<const Expr*> args = initArguments(var->getInit());
		if (args.size() < 2)
			return;
		std::optional<int64_t> address = constant(args[0]);
		if (!address)
			return;
		std::string form = name == "FNVD_REPLACES" ? "PLAIN" : name.substr(strlen("FNVD_REPLACES_")).str();
		const char* kind = "body";
		const FunctionDecl* target = nullptr;
		if (name.starts_with("FNVD_REPLACES_VIRTUAL"))
		{
			auto* call = dyn_cast<CallExpr>(args[1]->IgnoreImplicit());
			const FunctionDecl* signature = call ? call->getDirectCallee() : nullptr;
			if (signature)
			{
				auto [method, entry] = virtualTarget(cast<CXXRecordDecl>(signature->getParent()));
				target = method;
				detail.entry = textOrNull(entry);
			}
		}
		else if (name == "FNVD_REPLACES_CONSTRUCTOR" || name == "FNVD_REPLACES_DESTRUCTOR")
		{
			bool constructor = name == "FNVD_REPLACES_CONSTRUCTOR";
			kind = constructor ? "constructor" : "destructor";
			auto* call = dyn_cast<CallExpr>(args[1]->IgnoreImplicit());
			const FunctionDecl* signature = call ? call->getDirectCallee() : nullptr;
			auto* adapter = signature ? dyn_cast<ClassTemplateSpecializationDecl>(signature->getParent()) : nullptr;
			const TemplateArgumentList* targs = adapter ? &adapter->getTemplateArgs() : nullptr;
			const CXXRecordDecl* cls = targs && targs->size() && targs->get(0).getKind() == TemplateArgument::Type
										   ? targs->get(0).getAsType()->getAsCXXRecordDecl()
										   : nullptr;
			if (cls && !constructor)
				target = cls->getDestructor();
			else if (adapter)
			{
				// the constructor ConstructorAdapter<type, A...>::construct's `new C(arguments...)` calls (its
				// arguments convert: a registration may spell `char` for an int32_t parameter)
				for (const CXXMethodDecl* method : adapter->methods())
					if (method->getName() == "construct" && method->hasBody())
						if (const CXXConstructExpr* construct = constructExpr(method->getBody()))
							target = construct->getConstructor();
			}
		}
		else if (name == "FNVD_REPLACES_USERCALL" || name == "FNVD_REPLACES_USERPURGE")
		{
			// UsercallAdapter<&function, purge, locations...>::signature(): the function and its locations, a
			// member's `this` first, the parameters past the list on the stack
			static const char* const kLocations[] = {"eax", "ecx", "edx", "ebx", "esi", "edi", "stack"};
			auto* call = dyn_cast<CallExpr>(args[1]->IgnoreImplicit());
			const FunctionDecl* signature = call ? call->getDirectCallee() : nullptr;
			auto* adapter = signature ? dyn_cast<ClassTemplateSpecializationDecl>(signature->getParent()) : nullptr;
			const TemplateArgumentList* targs = adapter ? &adapter->getTemplateArgs() : nullptr;
			if (targs && targs->size() == 3 && targs->get(0).getKind() == TemplateArgument::Declaration &&
				targs->get(2).getKind() == TemplateArgument::Pack)
			{
				target = dyn_cast<FunctionDecl>(targs->get(0).getAsDecl());
				auto* method = dyn_cast_or_null<CXXMethodDecl>(target);
				int64_t index = method && method->isInstance() ? -1 : 0;
				for (const TemplateArgument& location : targs->get(2).pack_elements())
				{
					uint64_t value = location.getKind() == TemplateArgument::Integral
										 ? location.getAsIntegral().getZExtValue()
										 : std::size(kLocations);
					if (value < std::size(kLocations))
						detail.locations.emplace_back(index, kLocations[value]);
					++index;
				}
				for (; target && index < static_cast<int64_t>(target->getNumParams()); ++index)
					detail.locations.emplace_back(index, "stack");
			}
		}
		else
		{
			target = namedFunction(args[1]);
		}
		if (target)
			detail.target = qualified(target);
		detail.function = target;
		addRef(var->getLocation(), kind, form, *address, std::move(detail));
		return;
	}
	const char* kind = name == "FNVD_GLOBAL"			   ? "global"
					   : name == "FNVD_STATIC_MEMBER"	   ? "static_member"
					   : name == "FNVD_ORIGINAL_FUNCTION" ? "original_function"
					   : name == "FNVD_VFTABLE"		   ? "vftable"
													   : nullptr;
	if (!kind)
		return;
	std::optional<int64_t> address;
	if (name == "FNVD_VFTABLE")
	{
		std::vector<const Expr*> args = initArguments(var->getInit());
		if (!args.empty())
			address = constant(args[0]);
	}
	else if (const CXXReinterpretCastExpr* cast = reinterpretCast(var->getInit()))
	{
		address = constant(cast->getSubExpr());
	}
	if (!address)
		return;
	QualType type = var->getType();
	if (name != "FNVD_VFTABLE")
		detail.type = typeKey(type->isReferenceType() ? type->getPointeeType() : type.getUnqualifiedType());
	detail.local = flag(var->isLocalVarDecl() || !var->isExternallyVisible());
	detail.target = qualified(var);
	addRef(var->getLocation(), kind, name.substr(strlen("FNVD_")).str(), *address, std::move(detail));
}

void Collector::addressRef(const StaticAssertDecl* assertion)
{
	const Expansion* macro = expansionAt(assertion->getLocation());
	if (!macro || macro->name != "FNVD_ADDRESS" || macro->args.empty() || !ownsLocation(assertion->getLocation()))
		return;
	char* end = nullptr;
	int64_t address = static_cast<int64_t>(std::strtoull(macro->args[0].c_str(), &end, 0));
	if (end == macro->args[0].c_str())
		return;
	RefDetail detail;
	detail.function = namedFunction(assertion->getAssertExpr());
	if (detail.function)
		detail.target = qualified(detail.function);
	addRef(assertion->getLocation(), "address", "ADDRESS", address, std::move(detail));
}

// A static_assert that pins a record's sizeof to a constant (`FNVD_LAYOUT(sizeof(bhkPickData) == 0xB0)`): src's
// size of the record is the exe's.
void Collector::pin(const StaticAssertDecl* assertion)
{
	auto* compare = dyn_cast_or_null<BinaryOperator>(assertion->getAssertExpr()->IgnoreParenImpCasts());
	if (!compare || compare->getOpcode() != BO_EQ || compare->isValueDependent() ||
		!ownsLocation(assertion->getLocation()))
		return;
	for (auto [side, other] : {std::pair(compare->getLHS(), compare->getRHS()),
							   std::pair(compare->getRHS(), compare->getLHS())})
	{
		auto* size = dyn_cast<UnaryExprOrTypeTraitExpr>(side->IgnoreParenImpCasts());
		if (!size || size->getKind() != UETT_SizeOf)
			continue;
		const RecordDecl* record = size->getTypeOfArgument()->getAsRecordDecl();
		std::optional<int64_t> value = constant(other);
		if (!record || !value || !record->getDefinition())
			return;
		SourceLocation at = m_ci.getSourceManager().getExpansionLoc(assertion->getLocation());
		m_refs.push_back({"pin", {recordName(record->getDefinition()), fileAt(at).path, number(line(at)), *value}});
		return;
	}
}

// One declaration src spells of an explicit specialization (a class template instance's member, a function template's
// specialization): where it is, from its `template <>` line to its end, whether it is the definition and whether it
// says `inline`. Each declaration is its own row, written by the TU that owns its file. One clang rejects as
// following the instance's instantiation (a TU used the member before it) is a row of the member it specializes.
void Collector::specialization(const FunctionDecl* function)
{
	if (function->isImplicit())
		return;
	const FunctionDecl* named = function;
	if (function->isInvalidDecl())
	{
		auto* method = dyn_cast<CXXMethodDecl>(function);
		if (!method || function->getTemplateParameterLists().empty() ||
			!isa<ClassTemplateSpecializationDecl>(method->getParent()))
			return;
		named = nullptr;
		// the rejected declaration's type lacks the member's convention: its parameters and constness name it
		auto same = [&](const CXXMethodDecl* m) {
			if (m == method || m->isInvalidDecl() || m->getNumParams() != method->getNumParams() ||
				m->isConst() != method->isConst())
				return false;
			for (unsigned k = 0; k < m->getNumParams(); ++k)
				if (!m_context->hasSameType(m->getParamDecl(k)->getType(), method->getParamDecl(k)->getType()))
					return false;
			return true;
		};
		for (const NamedDecl* other : method->getParent()->lookup(method->getDeclName()))
			if (auto* m = dyn_cast<CXXMethodDecl>(other); m && same(m))
				named = m;
		if (!named)
			return;
	}
	if (!usable(named->getFirstDecl()))
		return;
	const SourceManager& sources = m_ci.getSourceManager();
	SourceLocation start = function->getOuterLocStart();
	if (!function->getTemplateParameterLists().empty())
		start = function->getTemplateParameterLists().front()->getTemplateLoc();
	start = sources.getExpansionLoc(start);
	if (!ownsLocation(start))
		return;
	SourceLocation end = sources.getExpansionLoc(function->getSourceRange().getEnd());
	m_refs.push_back({"specialization",
					  {functionKey(globalOf(named)), fileAt(start).path, number(line(start)), number(line(end)),
					   flag(function->isThisDeclarationADefinition()), flag(function->isInlineSpecified())}});
}

void Collector::forwarder(const CallExpr* call, const FunctionDecl* around)
{
	std::string callee;
	if (const FunctionDecl* function = call->getDirectCallee())
		callee = function->getQualifiedNameAsString();
	else if (auto* lookup = dyn_cast<UnresolvedLookupExpr>(call->getCallee()->IgnoreParenImpCasts()))
	{
		for (const NamedDecl* decl : lookup->decls())
			callee = decl->getQualifiedNameAsString();
	}
	if (!StringRef(callee).starts_with("fnvd::original::") || call->getNumArgs() == 0)
		return;
	std::optional<int64_t> address = constant(call->getArg(0));
	if (!address || !ownsLocation(call->getBeginLoc()))
		return;
	RefDetail detail;
	if (around)
	{
		detail.target = qualified(around);
		detail.context = detail.target;
		noteDefinition(around);
		if (usable(around))
			detail.function = around;
		else if (classTemplateMember(around))
			detail.pattern = patternKey(around);
		else if (packTemplate(around))
		{
			detail.pattern = patternKey(around);
			packFunction(around, detail.pattern);
		}
	}
	std::string form = callee.substr(strlen("fnvd::original::"));
	if (const FunctionDecl* original = call->getDirectCallee(); original && !call->isTypeDependent())
		forwardedType(form, call, original, detail);
	else
		forwardedArguments(call, detail);
	if (form == "usercall" && !call->isTypeDependent())
		usercallLocations(call, around, detail);
	addRef(call->getBeginLoc(), "forwarder", std::move(form), *address, std::move(detail));
}

// A copy the porting rules' "Folds" marks with `// Same function as X::f (0xADDR), registered there.` in the comment lines
// right above the function (the comment may wrap, and may go on after `registered there:` or `;`): the function is
// the same code as the one src registers at the address. Its ref names the copy; the context, the registered
// function as the comment spells it.
void Collector::foldRef(const FunctionDecl* function)
{
	const SourceManager& sm = m_ci.getSourceManager();
	SourceLocation start = sm.getExpansionLoc(function->getOuterLocStart());
	// a C++11 attribute ahead of the declaration (`[[clang::noinline]]`) starts it
	for (const Attr* attribute : function->attrs())
	{
		SourceLocation at = attribute->getLocation();
		if (!attribute->isInherited() && at.isValid() && at.isFileID() && sm.isBeforeInTranslationUnit(at, start))
			start = at;
	}
	if (start.isInvalid() || !ownsLocation(start))
		return;
	std::string comment;
	for (StringRef line : commentLines(start))
		comment += line.drop_front(2).trim().str() + " ";
	static const std::regex kFold(R"(Same function as (.+?) \((0x[0-9A-Fa-f]+)\), registered there[.:;])");
	for (std::sregex_iterator match(comment.begin(), comment.end(), kFold), done; match != done; ++match)
	{
		RefDetail detail;
		detail.target = qualified(function);
		detail.function = function;
		detail.context = textOrNull((*match)[1].str());
		addRef(start, "fold", "SAME_FUNCTION", static_cast<int64_t>(std::stoull((*match)[2].str(), nullptr, 16)),
			   std::move(detail));
	}
}

bool Visitor::TraverseDecl(Decl* decl)
{
	auto* function = dyn_cast_or_null<FunctionDecl>(decl);
	auto* method = dyn_cast_or_null<CXXMethodDecl>(decl);
	bool around = function && !(method && method->getParent()->isLambda());
	if (around)
		m_functions.push_back(function);
	bool result = RecursiveASTVisitor::TraverseDecl(decl);
	if (around)
		m_functions.pop_back();
	return result;
}

bool Visitor::VisitVarDecl(VarDecl* var)
{
	if (var->getLocation().isMacroID())
		m_collector.registration(var);
	return true;
}

bool Visitor::VisitStaticAssertDecl(StaticAssertDecl* assertion)
{
	if (assertion->getLocation().isMacroID())
		m_collector.addressRef(assertion);
	m_collector.pin(assertion);
	return true;
}

bool Visitor::VisitCallExpr(CallExpr* call)
{
	m_collector.forwarder(call, m_functions.empty() ? nullptr : m_functions.back());
	if (const Expr* callee = call->getCallee())
		m_callees[callee->IgnoreParenImpCasts()] = call;
	return true;
}

bool Visitor::VisitRecordDecl(RecordDecl* record)
{
	if (m_collector.owner(record) == Owner::Own)
		m_collector.noteRecord(record);
	return true;
}

bool Visitor::VisitEnumDecl(EnumDecl* enumeration)
{
	if (m_collector.owner(enumeration) == Owner::Own)
		m_collector.noteEnum(enumeration);
	return true;
}

bool Visitor::VisitTypedefNameDecl(TypedefNameDecl* alias)
{
	Owner owner = m_collector.owner(alias);
	if (owner == Owner::Own)
		m_collector.noteTypedef(alias);
	else if (owner == Owner::External && alias->getUnderlyingType().getCanonicalType()->isVectorType())
		m_collector.noteVectorTypedef(alias);
	return true;
}

bool Visitor::VisitFunctionDecl(FunctionDecl* function)
{
	// a fold copy goes by where it is written (foldRef): an explicit specialization of an instance's member too
	if (!function->isImplicit() && !isTemplateInstantiation(function->getTemplateSpecializationKind()))
		m_collector.foldRef(function);
	if (function->getTemplateSpecializationKind() == TSK_ExplicitSpecialization || function->isInvalidDecl())
		m_collector.specialization(function);
	// an explicit specialization of a class template instance's member is shared (its class is an instance), yet its
	// definition is the file's that spells it: written by the TU that owns that file
	if (function->getTemplateSpecializationKind() == TSK_ExplicitSpecialization &&
		function->isThisDeclarationADefinition() && m_collector.ownsLocation(function->getLocation()))
		m_collector.noteDefinition(function);
	if (m_collector.owner(function) != Owner::Own)
		return true;
	if (function->isFirstDecl() && !function->isImplicit())
		m_collector.noteFunction(function);
	if (function->isThisDeclarationADefinition() && !function->isImplicit())
		m_collector.noteDefinition(function);
	return true;
}

bool Visitor::VisitClassTemplateDecl(ClassTemplateDecl* pattern)
{
	if (!m_collector.fileAt(pattern->getLocation()).collected)
		return true;
	for (ClassTemplateSpecializationDecl* instance : pattern->specializations())
		if (isTemplateInstantiation(instance->getTemplateSpecializationKind()))
			m_collector.noteRecord(instance);
	return true;
}

bool Visitor::VisitDeclRefExpr(DeclRefExpr* reference)
{
	m_collector.use(reference->getDecl(), reference->getLocation(), callOf(reference));
	return true;
}

bool Visitor::VisitMemberExpr(MemberExpr* member)
{
	m_collector.use(member->getMemberDecl(), member->getMemberLoc(), callOf(member));
	return true;
}

class Diagnostics : public DiagnosticConsumer
{
public:
	explicit Diagnostics(TuOutcome& outcome) : m_outcome(outcome) {}

	void HandleDiagnostic(DiagnosticsEngine::Level level, const Diagnostic& info) override
	{
		DiagnosticConsumer::HandleDiagnostic(level, info);
		if (level < DiagnosticsEngine::Error)
			return;
		++m_outcome.errors;
		if (m_outcome.errors > 20)
			return;
		llvm::SmallString<256> message;
		info.FormatDiagnostic(message);
		std::string where;
		if (info.hasSourceManager() && info.getLocation().isValid())
		{
			const SourceManager& sm = info.getSourceManager();
			PresumedLoc at = sm.getPresumedLoc(sm.getExpansionLoc(info.getLocation()));
			if (at.isValid())
				where = std::string(at.getFilename()) + ":" + std::to_string(at.getLine()) + ": ";
		}
		m_outcome.errorText += where + message.str().str() + "\n";
	}

private:
	TuOutcome& m_outcome;
};

class IndexAction : public ASTFrontendAction
{
public:
	IndexAction(Store& store, const IndexScope& scope) : m_store(store), m_scope(scope) {}

	bool BeginSourceFileAction(CompilerInstance& ci) override
	{
		ci.getPreprocessor().addPPCallbacks(std::make_unique<MacroLog>(ci.getPreprocessor(), m_expansions));
		return true;
	}

	std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& ci, StringRef) override
	{
		return std::make_unique<Collector>(ci, m_store, m_scope, m_expansions);
	}

private:
	Store& m_store;
	const IndexScope& m_scope;
	Expansions m_expansions;
};

}  // namespace

std::unique_ptr<FrontendAction> makeIndexAction(Store& store, const IndexScope& scope)
{
	return std::make_unique<IndexAction>(store, scope);
}

std::unique_ptr<DiagnosticConsumer> makeIndexDiagnostics(TuOutcome& outcome)
{
	return std::make_unique<Diagnostics>(outcome);
}

}  // namespace srcdb
