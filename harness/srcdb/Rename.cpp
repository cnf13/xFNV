// srcdb rename-refs: where one TU spells the names a rename table changes (harness/tools/rename.py runs it per TU
// and applies the edits).
//
//   srcdb rename-refs --root DIR --names FILE --file TU [--directory DIR] -- COMPILE COMMAND...
//
// FILE: one `KIND<TAB>KEY` per line, the declarations to rename. KIND is function, field, var, type, enum,
// enumerator or param; KEY the declaration's qualified name with every template argument list and `(anonymous
// namespace)::` dropped (`BSSimpleList::RemoveAll`, `NiNode::m_kChildren`), an enumerator's its enum's key and its
// name (`ENUM_FORM_ID::kFormType_NPC`), a parameter's its function's key, `#` and its index (`Actor::Kill#0`).
// All overloads of a function key are one declaration here.
//
// Output, one line each (FILE repo-relative, OFFSET a byte offset, LINE 1-based):
//   occ   KIND KEY FILE OFFSET   a spelling of the declaration's name (declaration, definition or reference) at its
//                                spelling in a file under --root: a macro argument's spelling counts; the text there
//                                is the key's last name
//   macro KIND KEY FILE LINE     a reference a macro's body spells (no text of src to rewrite there)
//   fam   KEY KEY                a method (first) overriding another (second) where either has a table name's leaf
//   dep   LEAF FILE OFFSET       a dependent name in a template (not resolvable to a declaration) spelling a table
//                                name's leaf
//   err   FILE LINE TEXT         an error the parse met in a file under --root (a caller compares two parses)
// Template instantiations are visited too, so a member a template's body names through `this` or a dependent object
// resolves where some TU instantiates it.

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
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>

#include <fstream>
#include <set>
#include <string>
#include <unistd.h>
#include <unordered_set>

using namespace clang;

namespace srcdb
{

namespace
{

struct Table
{
	std::unordered_set<std::string> entries;  // KIND \t KEY
	std::unordered_set<std::string> leaves;
};

// a qualified name without template argument lists and anonymous namespaces
std::string plainKey(const std::string& qualified)
{
	std::string out;
	int depth = 0;
	for (size_t k = 0; k < qualified.size(); ++k)
	{
		char c = qualified[k];
		if (c == '<')
			++depth;
		else if (c == '>' && depth > 0)
			--depth;
		else if (depth == 0)
			out += c;
	}
	const std::string anonymous = "(anonymous namespace)::";
	for (size_t at; (at = out.find(anonymous)) != std::string::npos;)
		out.erase(at, anonymous.size());
	return out;
}

std::string leafOf(const std::string& key)
{
	size_t hash = key.rfind('#');
	std::string base = hash == std::string::npos ? key : key.substr(0, hash);
	size_t colon = base.rfind("::");
	return colon == std::string::npos ? base : base.substr(colon + 2);
}

class Finder : public RecursiveASTVisitor<Finder>
{
public:
	Finder(CompilerInstance& ci, const Table& table, const std::string& root)
		: m_ci(ci), m_sm(ci.getSourceManager()), m_table(table), m_root(root)
	{
	}

	bool shouldVisitTemplateInstantiations() const { return true; }
	bool shouldVisitImplicitCode() const { return false; }

	// --- declarations ---------------------------------------------------------------------------------------------

	bool VisitNamedDecl(NamedDecl* decl)
	{
		if (decl->isImplicit())
			return true;
		if (auto* structor = dyn_cast<CXXConstructorDecl>(decl))
			return spell(structor->getParent(), decl->getLocation()), true;
		if (auto* structor = dyn_cast<CXXDestructorDecl>(decl))
			return spell(structor->getParent(), decl->getLocation()), true;
		if (auto* method = dyn_cast<CXXMethodDecl>(decl))
			family(method);
		if (isa<ClassTemplateSpecializationDecl>(decl) &&
			!cast<ClassTemplateSpecializationDecl>(decl)->isExplicitSpecialization())
			return true;
		spell(decl, decl->getLocation());
		return true;
	}

	// --- references -----------------------------------------------------------------------------------------------

	bool VisitDeclRefExpr(DeclRefExpr* reference)
	{
		spell(reference->getDecl(), reference->getLocation());
		return true;
	}

	bool VisitMemberExpr(MemberExpr* member)
	{
		spell(member->getMemberDecl(), member->getMemberLoc());
		return true;
	}

	bool VisitTagTypeLoc(TagTypeLoc type)
	{
		spell(type.getDecl(), type.getNameLoc());
		return true;
	}

	bool VisitTemplateSpecializationTypeLoc(TemplateSpecializationTypeLoc type)
	{
		if (TemplateDecl* pattern = type.getTypePtr()->getTemplateName().getAsTemplateDecl())
			if (NamedDecl* templated = pattern->getTemplatedDecl())
				spell(templated, type.getTemplateNameLoc());
		return true;
	}

	bool TraverseConstructorInitializer(CXXCtorInitializer* init)
	{
		if (init->isWritten() && init->isMemberInitializer())
			spell(init->getMember(), init->getMemberLocation());
		return RecursiveASTVisitor::TraverseConstructorInitializer(init);
	}

	bool VisitDesignatedInitExpr(DesignatedInitExpr* init)
	{
		for (const DesignatedInitExpr::Designator& designator : init->designators())
			if (designator.isFieldDesignator())
				if (FieldDecl* field = designator.getFieldDecl())
					spell(field, designator.getFieldLoc());
		return true;
	}

	bool VisitCXXDependentScopeMemberExpr(CXXDependentScopeMemberExpr* member)
	{
		dependent(member->getMember(), member->getMemberLoc());
		return true;
	}

	bool VisitDependentScopeDeclRefExpr(DependentScopeDeclRefExpr* reference)
	{
		dependent(reference->getDeclName(), reference->getLocation());
		return true;
	}

	bool VisitUnresolvedLookupExpr(UnresolvedLookupExpr* lookup)
	{
		dependent(lookup->getName(), lookup->getNameLoc());
		return true;
	}

	bool VisitUnresolvedMemberExpr(UnresolvedMemberExpr* member)
	{
		dependent(member->getMemberName(), member->getMemberLoc());
		return true;
	}

private:
	// the table entry of a declaration: KIND \t KEY, empty when it is none the table can name
	std::string entryOf(const NamedDecl* decl) const
	{
		if (!decl)
			return {};
		if (auto* parameter = dyn_cast<ParmVarDecl>(decl))
		{
			auto* function = dyn_cast_or_null<FunctionDecl>(parameter->getDeclContext());
			if (!function)
				return {};
			if (FunctionDecl* pattern = function->getTemplateInstantiationPattern())
				function = pattern;
			return "param\t" + plainKey(function->getQualifiedNameAsString()) + "#" +
				   std::to_string(parameter->getFunctionScopeIndex());
		}
		if (auto* constant = dyn_cast<EnumConstantDecl>(decl))
		{
			auto* owner = cast<EnumDecl>(constant->getDeclContext());
			return "enumerator\t" + plainKey(owner->getQualifiedNameAsString()) + "::" + constant->getNameAsString();
		}
		if (!decl->getDeclName().isIdentifier())
			return {};
		std::string kind;
		if (isa<FieldDecl>(decl))
			kind = "field";
		else if (isa<FunctionDecl>(decl))
			kind = "function";
		else if (auto* var = dyn_cast<VarDecl>(decl))
		{
			if (var->isLocalVarDeclOrParm())
				return {};
			kind = "var";
		}
		else if (isa<EnumDecl>(decl))
			kind = "enum";
		else if (isa<RecordDecl>(decl))
			kind = "type";
		else
			return {};
		return kind + "\t" + plainKey(decl->getQualifiedNameAsString());
	}

	// a file location of src for a name at `at`: a macro argument's spelling; invalid in a macro's body
	SourceLocation fileLocation(SourceLocation at, bool& inMacroBody) const
	{
		inMacroBody = false;
		while (at.isMacroID())
		{
			if (!m_sm.isMacroArgExpansion(at))
			{
				inMacroBody = true;
				return {};
			}
			at = m_sm.getImmediateSpellingLoc(at);
		}
		return at;
	}

	bool pathOf(SourceLocation at, std::string& path) const
	{
		OptionalFileEntryRef file = m_sm.getFileEntryRefForID(m_sm.getFileID(at));
		if (!file)
			return false;
		llvm::SmallString<256> real;
		if (llvm::sys::fs::real_path(file->getName(), real))
			return false;
		llvm::StringRef text = real.str();
		if (!text.starts_with(m_root))
			return false;
		path = text.drop_front(m_root.size()).str();
		return llvm::StringRef(path).starts_with("src/") || llvm::StringRef(path).starts_with("gen/");
	}

	void spell(const NamedDecl* decl, SourceLocation at)
	{
		if (!decl || at.isInvalid())
			return;
		std::string entry = entryOf(decl);
		if (entry.empty() || !m_table.entries.count(entry))
			return;
		// the text there: the declaration's own name (a structor's: its class's)
		std::string leaf = decl->getNameAsString();
		// an unnamed parameter has no spelling to rewrite (its location is the declarator's end)
		if (leaf.empty())
			return;
		bool inMacroBody;
		SourceLocation file = fileLocation(at, inMacroBody);
		std::string path;
		if (inMacroBody)
		{
			SourceLocation expansion = m_sm.getExpansionLoc(at);
			if (pathOf(expansion, path))
				emit("macro\t" + entry + "\t" + path + "\t" + std::to_string(m_sm.getExpansionLineNumber(at)));
			return;
		}
		if (file.isInvalid() || !pathOf(file, path))
			return;
		// a destructor's name starts at its `~`
		unsigned offset = m_sm.getFileOffset(file);
		llvm::StringRef buffer = m_sm.getBufferData(m_sm.getFileID(file));
		while (offset < buffer.size() && (buffer[offset] == '~' || buffer[offset] == ' ' || buffer[offset] == '\t'))
			++offset;
		if (!buffer.substr(offset).starts_with(leaf))
			return;
		emit("occ\t" + entry + "\t" + path + "\t" + std::to_string(offset));
	}

	void dependent(DeclarationName name, SourceLocation at)
	{
		if (!name.isIdentifier() || !m_table.leaves.count(name.getAsString()))
			return;
		bool inMacroBody;
		SourceLocation file = fileLocation(at, inMacroBody);
		std::string path;
		if (inMacroBody || file.isInvalid() || !pathOf(file, path))
			return;
		emit("dep\t" + name.getAsString() + "\t" + path + "\t" + std::to_string(m_sm.getFileOffset(file)));
	}

	void family(const CXXMethodDecl* method)
	{
		if (!method->getDeclName().isIdentifier())
			return;
		for (const CXXMethodDecl* base : method->overridden_methods())
		{
			std::string mine = entryOf(method), theirs = entryOf(base);
			if (mine.empty() || theirs.empty())
				continue;
			if (m_table.leaves.count(leafOf(mine.substr(mine.find('\t') + 1))) ||
				m_table.leaves.count(leafOf(theirs.substr(theirs.find('\t') + 1))))
				emit("fam\t" + mine.substr(mine.find('\t') + 1) + "\t" + theirs.substr(theirs.find('\t') + 1));
		}
	}

	void emit(const std::string& line)
	{
		if (m_seen.insert(line).second)
			llvm::outs() << line << "\n";
	}

	CompilerInstance& m_ci;
	SourceManager& m_sm;
	const Table& m_table;
	std::string m_root;
	std::unordered_set<std::string> m_seen;
};

class Consumer : public ASTConsumer
{
public:
	Consumer(CompilerInstance& ci, const Table& table, const std::string& root) : m_ci(ci), m_table(table), m_root(root)
	{
	}

	void HandleTranslationUnit(ASTContext& context) override
	{
		Finder finder(m_ci, m_table, m_root);
		finder.TraverseDecl(context.getTranslationUnitDecl());
	}

private:
	CompilerInstance& m_ci;
	const Table& m_table;
	std::string m_root;
};

class Action : public ASTFrontendAction
{
public:
	Action(const Table& table, const std::string& root) : m_table(table), m_root(root) {}

	std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& ci, llvm::StringRef) override
	{
		return std::make_unique<Consumer>(ci, m_table, m_root);
	}

private:
	const Table& m_table;
	std::string m_root;
};

}  // namespace

int runRenameRefs(int argc, char** argv)
{
	std::string root, names, file, directory;
	std::vector<std::string> command;
	for (int k = 2; k < argc; ++k)
	{
		std::string word = argv[k];
		if (word == "--")
		{
			command.assign(argv + k + 1, argv + argc);
			break;
		}
		if (k + 1 >= argc)
			throw Error("srcdb rename-refs: " + word + " needs a value");
		if (word == "--root")
			root = argv[++k];
		else if (word == "--names")
			names = argv[++k];
		else if (word == "--file")
			file = argv[++k];
		else if (word == "--directory")
			directory = argv[++k];
		else
			throw Error("srcdb rename-refs: unexpected argument " + word);
	}
	if (root.empty() || names.empty() || file.empty() || command.empty())
		throw Error("srcdb rename-refs: needs --root, --names, --file and the compile command after --");
	llvm::SmallString<256> real;
	if (llvm::sys::fs::real_path(root, real))
		throw Error("srcdb rename-refs: no directory " + root);
	root = real.str().str() + "/";
	Table table;
	std::ifstream in(names);
	for (std::string line; std::getline(in, line);)
	{
		size_t tab = line.find('\t');
		if (line.empty() || tab == std::string::npos)
			continue;
		table.entries.insert(line);
		table.leaves.insert(leafOf(line.substr(tab + 1)));
	}
	if (!directory.empty() && chdir(directory.c_str()) != 0)
		throw Error("srcdb rename-refs: cannot enter " + directory);
	return parseTu(root, file, command, std::make_unique<Action>(table, root));
}

}  // namespace srcdb
