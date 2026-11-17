// srcdb globals-tu: one TU's variables with static storage, as harness/tools/globals.py needs them to move their
// definitions (the globals step of the clean-up).
//
//   srcdb globals-tu --root DIR --file TU [--directory DIR] [--headers] -- COMPILE COMMAND...
//
// Output: one JSON object per line, its kind in "t". FILE is repo-relative (files under --root only), offsets are
// byte offsets into it.
//   var      a variable with static storage that is no function's local: every redeclaration under --root and what
//            the variable's definition needs. Emitted when a redeclaration is in the TU's main file, or with
//            --headers in any file under src/ (the all-headers TU).
//   include  FILE OFFSET INCLUDED: an #include directive (also one that a guard makes a no-op), system headers' too.
//   cond     FILE BEGIN END: an #if/#ifdef/#ifndef ... #endif region of a file a var line names.
//   top      a declaration directly in a namespace scope (the TU, a namespace, a linkage specification) of the main
//            file, or with --headers of any src/ header: file, begin, end, kind (fn: a function definition or a
//            namespace scope holding one; fndecl; class: a class, union or enum definition; var; other), ctx (the
//            enclosing scope's begin offset, -1 for the TU).
//   ref      FILE OFFSET NAME: a reference to the variable NAME (qualified) from a src/ header (--headers only).
//   errors   N: the parse's error count.
// A location list ("locs") is one [FILE, BEGIN, END] per redeclaration of what a definition names; END is the end of
// a definition (a type's: its closing brace), else BEGIN. In includes, locations and macro definitions, a file outside
// --root (a system header) is named by its absolute path; a builtin or command-line one has no location.

#include "Clang.hpp"
#include "Store.hpp"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/DeclTemplate.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/RecursiveASTVisitor.h>
#include <clang/AST/TypeLoc.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Lex/Lexer.h>
#include <clang/Lex/PPCallbacks.h>
#include <clang/Lex/Preprocessor.h>
#include <llvm/ADT/SetVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/JSON.h>
#include <llvm/Support/raw_ostream.h>

#include <map>
#include <set>
#include <string>
#include <unistd.h>
#include <vector>

using namespace clang;

namespace srcdb
{

namespace
{

struct Shared
{
	std::string root;  // with a trailing slash
	bool headers = false;
	SourceManager* sm = nullptr;
	std::map<FileID, std::string> paths;  // FileID -> repo-relative path, empty outside the root
	// macro expansions in files under the root, per file in offset order: the macro definition's file and offset
	struct Expansion
	{
		unsigned offset;
		std::string defFile;
		unsigned defOffset;
		std::string name;
	};
	std::map<FileID, std::vector<Expansion>> expansions;
	std::set<std::string> includes;
	std::map<FileID, std::vector<std::pair<unsigned, unsigned>>> conds;

	const std::string& pathOf(FileID id)
	{
		auto found = paths.find(id);
		if (found != paths.end())
			return found->second;
		std::string& path = paths[id];
		OptionalFileEntryRef entry = sm->getFileEntryRefForID(id);
		llvm::SmallString<256> real;
		if (entry && !llvm::sys::fs::real_path(entry->getName(), real) && real.str().starts_with(root))
			path = real.str().drop_front(root.size()).str();
		return path;
	}

	// the file location's path and offset; false outside the root
	bool place(SourceLocation at, std::string& path, unsigned& offset)
	{
		if (at.isInvalid())
			return false;
		at = sm->getFileLoc(at);
		FileID id = sm->getFileID(at);
		path = pathOf(id);
		offset = sm->getFileOffset(at);
		return !path.empty();
	}

	// a file's path under the root (repo-relative), else its absolute real path (a system header); empty for none
	const std::string& anyPathOf(FileID id)
	{
		auto found = anyPaths.find(id);
		if (found != anyPaths.end())
			return found->second;
		std::string& path = anyPaths[id];
		path = pathOf(id);
		OptionalFileEntryRef entry = sm->getFileEntryRefForID(id);
		llvm::SmallString<256> real;
		if (path.empty() && entry && !llvm::sys::fs::real_path(entry->getName(), real))
			path = real.str().str();
		return path;
	}

	bool anyPlace(SourceLocation at, std::string& path, unsigned& offset)
	{
		if (at.isInvalid())
			return false;
		at = sm->getFileLoc(at);
		path = anyPathOf(sm->getFileID(at));
		offset = sm->getFileOffset(at);
		return !path.empty();
	}

	std::map<FileID, std::string> anyPaths;
};

class Callbacks : public PPCallbacks
{
public:
	explicit Callbacks(Shared& shared) : m_shared(shared) {}

	// every include, system headers' too (a macro or declaration of one is seen through them)
	void InclusionDirective(SourceLocation hash, const Token&, StringRef, bool, CharSourceRange,
							OptionalFileEntryRef file, StringRef, StringRef, const Module*, bool,
							SrcMgr::CharacteristicKind) override
	{
		std::string path;
		unsigned offset;
		if (!file || !m_shared.anyPlace(hash, path, offset))
			return;
		llvm::SmallString<256> real;
		if (llvm::sys::fs::real_path(file->getName(), real))
			return;
		std::string included = real.str().starts_with(m_shared.root)
								   ? real.str().drop_front(m_shared.root.size()).str()
								   : real.str().str();
		llvm::json::Array line{"include", path, offset, included};
		std::string text;
		llvm::raw_string_ostream(text) << llvm::json::Value(std::move(line));
		m_shared.includes.insert(text);
	}

	void MacroExpands(const Token& name, const MacroDefinition& definition, SourceRange range,
					  const MacroArgs*) override
	{
		SourceLocation at = name.getLocation();
		if (at.isInvalid() || at.isMacroID())
			return;
		FileID id = m_shared.sm->getFileID(at);
		if (m_shared.pathOf(id).empty())
			return;
		std::string defFile;
		unsigned defOffset = 0;
		if (const MacroInfo* info = definition.getMacroInfo())
			m_shared.anyPlace(info->getDefinitionLoc(), defFile, defOffset);
		m_shared.expansions[id].push_back({m_shared.sm->getFileOffset(at), defFile, defOffset,
										   name.getIdentifierInfo()->getName().str()});
	}

	void Endif(SourceLocation at, SourceLocation ifAt) override
	{
		if (at.isInvalid() || ifAt.isInvalid() || at.isMacroID())
			return;
		FileID id = m_shared.sm->getFileID(at);
		if (m_shared.pathOf(id).empty() || m_shared.sm->getFileID(ifAt) != id)
			return;
		m_shared.conds[id].push_back({m_shared.sm->getFileOffset(ifAt), m_shared.sm->getFileOffset(at)});
	}

private:
	Shared& m_shared;
};

// what a declaration's type and initialiser name: each declaration, and the definitions of the types it needs whole
class Needs : public RecursiveASTVisitor<Needs>
{
public:
	Needs(Shared& shared, const Decl* self) : m_shared(shared), m_self(self) {}

	llvm::json::Array take() { return std::move(m_out); }

	bool shouldVisitImplicitCode() const { return true; }

	void declaration(const Decl* decl, const char* why)
	{
		if (!decl || decl == m_self || (isa<VarDecl>(decl) && cast<VarDecl>(decl)->isLocalVarDeclOrParm()))
			return;
		std::string key = std::string(why) + "\t" + std::to_string(reinterpret_cast<uintptr_t>(decl->getCanonicalDecl()));
		if (!m_seen.insert(key).second)
			return;
		llvm::json::Array locs;
		for (const Decl* redecl : decl->redecls())
		{
			std::string path;
			unsigned offset;
			if (m_shared.anyPlace(redecl->getLocation(), path, offset))
				locs.push_back(llvm::json::Array{path, offset, offset});
		}
		m_out.push_back(llvm::json::Object{{"req", "decl"}, {"why", why}, {"name", nameOf(decl)}, {"locs", std::move(locs)}});
	}

	// the definition of a class or enum (of a class template's instance: its pattern's)
	void definition(QualType type, const char* why)
	{
		if (type.isNull())
			return;
		type = m_self->getASTContext().getBaseElementType(type.getCanonicalType());
		const TagDecl* tag = type->getAsTagDecl();
		if (!tag)
			return;
		if (auto* record = dyn_cast<CXXRecordDecl>(tag))
			if (const CXXRecordDecl* pattern = record->getTemplateInstantiationPattern())
				tag = pattern;
		const TagDecl* def = tag->getDefinition();
		if (!def)
			return;
		std::string key = std::string("def\t") + std::to_string(reinterpret_cast<uintptr_t>(def));
		if (!m_seen.insert(key).second)
			return;
		llvm::json::Array locs;
		std::string path, endPath;
		unsigned begin, end;
		if (m_shared.anyPlace(def->getBeginLoc(), path, begin) && m_shared.anyPlace(def->getEndLoc(), endPath, end) &&
			path == endPath)
			locs.push_back(llvm::json::Array{path, begin, end});
		m_out.push_back(llvm::json::Object{{"req", "def"}, {"why", why}, {"name", nameOf(def)}, {"locs", std::move(locs)}});
	}

	bool VisitDeclRefExpr(DeclRefExpr* reference)
	{
		declaration(reference->getDecl(), "ref");
		return true;
	}

	bool VisitMemberExpr(MemberExpr* member)
	{
		declaration(member->getMemberDecl(), "member");
		return true;
	}

	bool VisitCXXConstructExpr(CXXConstructExpr* construct)
	{
		declaration(construct->getConstructor(), "ctor");
		definition(construct->getType(), "construct");
		return true;
	}

	bool VisitTypedefTypeLoc(TypedefTypeLoc type)
	{
		declaration(type.getDecl(), "type");
		return true;
	}

	bool VisitTagTypeLoc(TagTypeLoc type)
	{
		declaration(type.getDecl(), "type");
		return true;
	}

	bool VisitTemplateSpecializationTypeLoc(TemplateSpecializationTypeLoc type)
	{
		if (TemplateDecl* pattern = type.getTypePtr()->getTemplateName().getAsTemplateDecl())
			declaration(pattern, "type");
		return true;
	}

private:
	static std::string nameOf(const Decl* decl)
	{
		if (auto* named = dyn_cast<NamedDecl>(decl))
			return named->getQualifiedNameAsString();
		return "";
	}

	Shared& m_shared;
	const Decl* m_self;
	llvm::json::Array m_out;
	std::set<std::string> m_seen;
};

class Refs : public RecursiveASTVisitor<Refs>
{
public:
	explicit Refs(Shared& shared) : m_shared(shared) {}

	bool VisitDeclRefExpr(DeclRefExpr* reference)
	{
		auto* var = dyn_cast<VarDecl>(reference->getDecl());
		if (var && var->hasGlobalStorage() && !var->isStaticLocal())
			emit(reference->getLocation(), var->getQualifiedNameAsString());
		return true;
	}

	bool VisitTypedefTypeLoc(TypedefTypeLoc type)
	{
		emit(type.getNameLoc(), type.getDecl()->getQualifiedNameAsString());
		return true;
	}

	void emit(SourceLocation at, const std::string& name)
	{
		std::string path;
		unsigned offset;
		bool main = m_shared.sm->isInMainFile(m_shared.sm->getFileLoc(at));
		if (!m_shared.place(at, path, offset) || !llvm::StringRef(path).starts_with("src/") ||
			main == m_shared.headers)
			return;
		std::string text;
		llvm::raw_string_ostream(text) << llvm::json::Value(llvm::json::Array{"ref", path, offset, name});
		m_lines.insert(text);
	}

	std::set<std::string> m_lines;

private:
	Shared& m_shared;
};

class Collector : public RecursiveASTVisitor<Collector>
{
public:
	explicit Collector(Shared& shared) : m_shared(shared) {}

	bool VisitVarDecl(VarDecl* var)
	{
		if (isa<ParmVarDecl>(var) || !var->hasGlobalStorage() || var->isStaticLocal() || var->isLocalExternDecl())
			return true;
		if (var->getDeclContext()->isDependentContext() || var->isTemplated())
			return true;
		m_vars.insert(var->getCanonicalDecl());
		return true;
	}

	llvm::SetVector<VarDecl*> m_vars;

private:
	Shared& m_shared;
};

class Consumer : public ASTConsumer
{
public:
	Consumer(CompilerInstance& ci, Shared& shared) : m_ci(ci), m_shared(shared) {}

	void HandleTranslationUnit(ASTContext& context) override
	{
		m_context = &context;
		SourceManager& sm = context.getSourceManager();
		Collector collector(m_shared);
		collector.TraverseDecl(context.getTranslationUnitDecl());
		std::set<FileID> named;
		for (VarDecl* var : collector.m_vars)
			emitVar(var, named);
		tops(context.getTranslationUnitDecl(), -1);
		Refs refs(m_shared);
		refs.TraverseDecl(context.getTranslationUnitDecl());
		for (const std::string& line : refs.m_lines)
			llvm::outs() << line << "\n";
		for (const std::string& line : m_shared.includes)
			llvm::outs() << line << "\n";
		for (auto& [id, regions] : m_shared.conds)
		{
			if (!named.count(id) && !(m_shared.headers ? isHeader(id) : id == sm.getMainFileID()))
				continue;
			for (auto [begin, end] : regions)
				print(llvm::json::Array{"cond", m_shared.pathOf(id), begin, end});
		}
	}

private:
	bool isHeader(FileID id)
	{
		llvm::StringRef path = m_shared.pathOf(id);
		return path.starts_with("src/") && id != m_shared.sm->getMainFileID();
	}

	bool wanted(FileID id) { return m_shared.headers ? isHeader(id) : id == m_shared.sm->getMainFileID(); }

	void print(llvm::json::Value value) { llvm::outs() << value << "\n"; }

	// the offset just past a token-ending location
	unsigned tokenEnd(SourceLocation at)
	{
		SourceManager& sm = *m_shared.sm;
		SourceLocation end = Lexer::getLocForEndOfToken(sm.getFileLoc(at), 0, sm, m_ci.getLangOpts());
		return sm.getFileOffset(end);
	}

	void tops(DeclContext* context, int64_t ctx)
	{
		SourceManager& sm = *m_shared.sm;
		for (Decl* decl : context->decls())
		{
			if (decl->isImplicit() || decl->getBeginLoc().isInvalid())
				continue;
			SourceLocation begin = sm.getFileLoc(decl->getBeginLoc());
			FileID id = sm.getFileID(begin);
			if (!wanted(id))
				continue;
			std::string kind = "other";
			bool scope = isa<NamespaceDecl>(decl) || isa<LinkageSpecDecl>(decl) || isa<ExportDecl>(decl);
			if (scope)
				kind = holdsFunction(cast<DeclContext>(decl)) ? "fn" : "other";
			else if (auto* function = dyn_cast<FunctionDecl>(decl))
				kind = function->doesThisDeclarationHaveABody() ? "fn" : "fndecl";
			else if (auto* pattern = dyn_cast<FunctionTemplateDecl>(decl))
				kind = pattern->getTemplatedDecl()->doesThisDeclarationHaveABody() ? "fn" : "fndecl";
			else if (auto* tag = dyn_cast<TagDecl>(decl))
				kind = tag->isThisDeclarationADefinition() ? "class" : "other";
			else if (auto* pattern = dyn_cast<ClassTemplateDecl>(decl))
				kind = pattern->getTemplatedDecl()->isThisDeclarationADefinition() ? "class" : "other";
			else if (isa<VarDecl>(decl))
				kind = "var";
			else if (isa<TypedefNameDecl>(decl) || isa<TypeAliasTemplateDecl>(decl))
				kind = "type";
			std::string name;
			if (auto* named = dyn_cast<NamedDecl>(decl))
				name = named->getQualifiedNameAsString();
			unsigned offset = sm.getFileOffset(begin);
			unsigned end = tokenEnd(decl->getEndLoc());
			llvm::json::Array needs;
			if (auto* alias = dyn_cast<TypedefNameDecl>(decl); alias && alias->getTypeSourceInfo())
			{
				Needs visitor(m_shared, alias);
				visitor.TraverseTypeLoc(alias->getTypeSourceInfo()->getTypeLoc());
				needs = visitor.take();
			}
			print(llvm::json::Object{{"t", "top"}, {"file", m_shared.pathOf(id)}, {"begin", offset}, {"end", end},
									 {"kind", kind}, {"ctx", ctx}, {"macro", decl->getBeginLoc().isMacroID()},
									 {"name", name}, {"needs", std::move(needs)}});
			if (scope)
				tops(cast<DeclContext>(decl), offset);
		}
	}

	static bool holdsFunction(DeclContext* context)
	{
		for (Decl* decl : context->decls())
		{
			if (auto* function = dyn_cast<FunctionDecl>(decl); function && function->doesThisDeclarationHaveABody())
				return true;
			if (auto* pattern = dyn_cast<FunctionTemplateDecl>(decl);
				pattern && pattern->getTemplatedDecl()->doesThisDeclarationHaveABody())
				return true;
			if ((isa<NamespaceDecl>(decl) || isa<LinkageSpecDecl>(decl)) && holdsFunction(cast<DeclContext>(decl)))
				return true;
		}
		return false;
	}

	// the enclosing scope of a declaration as its kind and begin offset (-1: the TU)
	llvm::json::Array contextOf(const VarDecl* var)
	{
		const DeclContext* context = var->getLexicalDeclContext();
		const char* kind = "tu";
		if (isa<TranslationUnitDecl>(context))
			return llvm::json::Array{kind, -1, ""};
		if (isa<NamespaceDecl>(context))
			kind = cast<NamespaceDecl>(context)->isAnonymousNamespace() ? "anonymous" : "namespace";
		else if (isa<LinkageSpecDecl>(context))
			kind = "linkage";
		else if (isa<RecordDecl>(context))
			kind = "class";
		else if (isa<FunctionDecl>(context))
			kind = "function";
		else
			kind = "other";
		std::string path;
		unsigned offset = 0;
		m_shared.place(cast<Decl>(context)->getBeginLoc(), path, offset);
		return llvm::json::Array{kind, offset, path};
	}

	void emitVar(VarDecl* canonical, std::set<FileID>& named)
	{
		SourceManager& sm = *m_shared.sm;
		ASTContext& context = *m_context;
		bool want = false;
		for (VarDecl* redecl : canonical->redecls())
		{
			SourceLocation at = sm.getFileLoc(redecl->getLocation());
			if (wanted(sm.getFileID(at)))
				want = true;
		}
		if (!want)
			return;
		llvm::json::Array decls;
		bool templ = canonical->getTemplateSpecializationKind() != TSK_Undeclared ||
					 isa<ClassTemplateSpecializationDecl>(canonical->getDeclContext()) ||
					 isa<VarTemplateSpecializationDecl>(canonical);
		for (VarDecl* redecl : canonical->redecls())
		{
			std::string path;
			unsigned name;
			if (!m_shared.place(redecl->getLocation(), path, name))
				continue;
			named.insert(sm.getFileID(sm.getFileLoc(redecl->getLocation())));
			unsigned begin = sm.getFileOffset(sm.getFileLoc(redecl->getBeginLoc()));
			unsigned end = tokenEnd(redecl->getEndLoc());
			unsigned qualifier = name;
			if (NestedNameSpecifierLoc q = redecl->getQualifierLoc())
				qualifier = sm.getFileOffset(sm.getFileLoc(q.getSourceRange().getBegin()));
			llvm::json::Array init;
			if (const Expr* expression = redecl->getInit(); expression && redecl == canonical->getInitializingDeclaration())
			{
				// a `= x` or `{...}` or `(...)` as written: from after the declarator to the end
				init = llvm::json::Array{sm.getFileOffset(sm.getFileLoc(expression->getBeginLoc())),
										 tokenEnd(expression->getEndLoc())};
			}
			llvm::json::Array attrs;
			for (const Attr* attr : redecl->attrs())
				if (!attr->isImplicit())
					attrs.push_back(std::string(attr->getSpelling()));
			// another declarator of one declaration (`extern int a, b;`)
			bool group = false;
			for (Decl* other : redecl->getLexicalDeclContext()->decls())
				if (other != redecl && isa<DeclaratorDecl>(other) &&
					sm.getFileLoc(other->getBeginLoc()) == sm.getFileLoc(redecl->getBeginLoc()))
					group = true;
			Needs needs(m_shared, redecl);
			if (TypeSourceInfo* info = redecl->getTypeSourceInfo())
				needs.TraverseTypeLoc(info->getTypeLoc());
			if (redecl->isThisDeclarationADefinition() == VarDecl::Definition || redecl->getInit())
				needs.definition(redecl->getType(), "vartype");
			if (redecl->getInit() && redecl == canonical->getInitializingDeclaration())
				needs.TraverseStmt(const_cast<Expr*>(redecl->getInit()));
			FileID id = sm.getFileID(sm.getFileLoc(redecl->getLocation()));
			llvm::json::Array macros;
			for (const Shared::Expansion& expansion : m_shared.expansions[id])
				if (expansion.offset >= begin && expansion.offset < end)
					macros.push_back(llvm::json::Array{expansion.name, expansion.defFile, expansion.defOffset});
			decls.push_back(llvm::json::Object{
				{"file", path},
				{"begin", begin},
				{"end", end},
				{"name", name},
				{"qual", qualifier},
				{"init", std::move(init)},
				{"def", redecl->isThisDeclarationADefinition() == VarDecl::Definition},
				{"extern", redecl->getStorageClass() == SC_Extern},
				{"static", redecl->getStorageClass() == SC_Static},
				{"inline", redecl->isInlineSpecified()},
				{"constexpr", redecl->isConstexpr()},
				{"constinit", redecl->hasAttr<ConstInitAttr>()},
				{"macro", redecl->getLocation().isMacroID() || redecl->getBeginLoc().isMacroID() ||
							  redecl->getEndLoc().isMacroID()},
				{"attrs", std::move(attrs)},
				{"group", group},
				{"ctx", contextOf(redecl)},
				{"needs", needs.take()},
				{"macros", std::move(macros)},
			});
		}
		QualType type = canonical->getType();
		QualType base = context.getBaseElementType(type.getCanonicalType());
		const VarDecl* initializing = canonical->getInitializingDeclaration();
		const VarDecl* definition = canonical->getDefinition();
		// no initialiser: zero-initialised (a class's default construction is an initialiser in the AST)
		bool constInit = initializing && initializing->getInit() ? initializing->hasConstantInitialization()
																 : definition != nullptr;
		bool strptr = false;
		if (initializing && initializing->getInit())
			strptr = hasStringPointer(initializing->getInit());
		print(llvm::json::Object{
			{"t", "var"},
			{"name", canonical->getQualifiedNameAsString()},
			{"external", canonical->hasExternalFormalLinkage()},
			{"member", canonical->isStaticDataMember()},
			{"tls", canonical->getTLSKind() != VarDecl::TLS_None},
			{"template", templ},
			{"const", type.isConstQualified() || base.isConstQualified()},
			{"pointer", base->isPointerType() || base->isMemberPointerType() || base->isReferenceType()},
			{"literal", !type->isDependentType() && !type->isIncompleteType() && type->isLiteralType(context)},
			{"array", type->isArrayType()},
			{"incomplete_array", type->isIncompleteArrayType()},
			{"defined", definition != nullptr},
			{"const_init", constInit},
			{"dtor", canonical->needsDestruction(context) != QualType::DK_none},
			{"strptr", strptr},
			{"decls", std::move(decls)},
		});
	}

	// a string literal used as a pointer (its address, not an array's contents)
	static bool hasStringPointer(const Stmt* stmt)
	{
		if (!stmt)
			return false;
		if (auto* cast = dyn_cast<ImplicitCastExpr>(stmt))
			if (cast->getCastKind() == CK_ArrayToPointerDecay && isa<StringLiteral>(cast->getSubExpr()->IgnoreParens()))
				return true;
		for (const Stmt* child : stmt->children())
			if (hasStringPointer(child))
				return true;
		return false;
	}

	CompilerInstance& m_ci;
	Shared& m_shared;
	ASTContext* m_context = nullptr;
};

class Action : public ASTFrontendAction
{
public:
	explicit Action(Shared& shared) : m_shared(shared) {}

	std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& ci, llvm::StringRef) override
	{
		m_shared.sm = &ci.getSourceManager();
		ci.getPreprocessor().addPPCallbacks(std::make_unique<Callbacks>(m_shared));
		return std::make_unique<Consumer>(ci, m_shared);
	}

private:
	Shared& m_shared;
};

class Count : public DiagnosticConsumer
{
};

}  // namespace

int runGlobalsTu(int argc, char** argv)
{
	std::string root, file, directory;
	bool headers = false;
	std::vector<std::string> command;
	for (int k = 2; k < argc; ++k)
	{
		std::string word = argv[k];
		if (word == "--")
		{
			command.assign(argv + k + 1, argv + argc);
			break;
		}
		if (word == "--headers")
		{
			headers = true;
			continue;
		}
		if (k + 1 >= argc)
			throw Error("srcdb globals-tu: " + word + " needs a value");
		if (word == "--root")
			root = argv[++k];
		else if (word == "--file")
			file = argv[++k];
		else if (word == "--directory")
			directory = argv[++k];
		else
			throw Error("srcdb globals-tu: unexpected argument " + word);
	}
	if (root.empty() || file.empty() || command.empty())
		throw Error("srcdb globals-tu: needs --root, --file and the compile command after --");
	llvm::SmallString<256> real;
	if (llvm::sys::fs::real_path(root, real))
		throw Error("srcdb globals-tu: no directory " + root);
	Shared shared;
	shared.root = real.str().str() + "/";
	shared.headers = headers;
	if (!directory.empty() && chdir(directory.c_str()) != 0)
		throw Error("srcdb globals-tu: cannot enter " + directory);
	initializeTargets();
	bool cl = llvm::StringRef(command[0]).contains("clang-cl") ||
			  std::find(command.begin(), command.end(), "--driver-mode=cl") != command.end();
	std::vector<std::string> args = cl ? syntaxOnlyCommand(command, file) : gnuSyntaxOnly(command, file);
	if (cl)
	{
		args.insert(args.end() - 2, "/clang:-ferror-limit=0");
		args.insert(args.end() - 2, "/clang:-Wno-everything");
	}
	Count diagnostics;
	DiagnosticOptions options;
	bool ran = runParse(args, std::make_unique<Action>(shared), diagnostics, options);
	llvm::outs() << llvm::json::Value(llvm::json::Array{"errors", diagnostics.getNumErrors()}) << "\n";
	llvm::outs().flush();
	return ran ? 0 : 1;
}

}  // namespace srcdb
