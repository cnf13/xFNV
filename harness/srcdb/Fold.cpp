// srcdb fold-refs: the constants of one TU that only name a function, and every use of them (harness/tools/foldfn.py
// runs it per TU and folds them away).
//
//   srcdb fold-refs --root DIR --file TU [--directory DIR] [--names FILE] -- COMPILE COMMAND...
//
// FILE: one name per line, the names of constants other TUs may declare (a constant's name may be another
// declaration's too: `other` lines say where the TU spells it so); this TU's own constants' names are added.
//
// A constant here: a variable that is not a parameter, of a const-qualified type, whose initialiser is one function's
// address: `&f` or `f`, possibly inside one explicit cast (`reinterpret_cast<void*>(&f)`), parentheses allowed.
// Templated declarations (a member of a class template) are not taken.
//
// Output, one TAB-separated line each (FILE repo-relative; B, E a byte range [B, E) in it; KEY a constant's first
// declaration's name as FILE:OFFSET; only declarations and uses in files under --root's src/ or gen/):
//   const KEY NAME SCOPE FORM FLAGS QUALIFIED
//       SCOPE  namespace (external linkage), internal (static or an anonymous namespace), member (static data
//              member), static-local, local (an automatic variable of a body)
//       FORM   address (`&f`), decay (`f`), cast (an explicit cast around either)
//       FLAGS  comma list, `-` for none: method (a non-static member function: the constant is a member pointer),
//              virtual, overloaded (another function of that name in the function's scope), same (the constant's
//              type without const is the type of `&f`), template (the function is a template's specialization)
//       QUALIFIED the function's name qualified from the global namespace (`::f`, `::A::f`), no template arguments
//   decl KEY FILE B E           a declaration of the constant: from its first specifier to the end of its declarator
//                               or initialiser (the `;` not included)
//   init KEY FILE B E B2 E2 B3 E3  the initialiser as written; the address expression in it (`&f` or `f`); the
//                               function's name as written there (`A::f`, `f<T>`)
//   alias KEY AKEY ANAME FILE B E  the constant's type is written as this typedef or alias (its declaration, the `;`
//                               not included), declared in a file under the root
//   aref AKEY FILE B            a spelling of a listed alias's name (also its own declaration's)
//   use KEY FILE B E CONTEXT PARENT FLAGS
//       CONTEXT call        a call through the constant; B E: the callee as written (`K`, `(K)`, `(*K)`)
//               member-call `(o->*K)(...)` or `(o.*K)(...)`; B E: the use itself
//               address     `&K`; B E: the use itself
//               value       anything else; B E: the use itself
//       PARENT  the class of the first expression around the use that is not an implicit cast (`-` for none)
//       FLAGS   comma list, `-` for none: visible (the function's unqualified name, looked up from the use's scopes,
//               finds it), shadowed (finds another declaration first; neither: the use is outside the function's
//               scope), unevaluated (inside sizeof, alignof, decltype or noexcept), member-of=QUALIFIED
//               (member-call: the object's class)
//   macro KEY FILE LINE         a use or declaration spelled by a macro's body (no text to rewrite)
//   other NAME FILE OFFSET      a spelling of a constant's name that names another declaration (a function of that
//                               name): declarations, references, members, dependent names
//   err FILE LINE TEXT          an error of the parse in a file under the root
// Template instantiations are visited, so a use in a template's body is seen where some TU instantiates it.

#include "Clang.hpp"
#include "Store.hpp"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/Decl.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/ExprCXX.h>
#include <clang/AST/ParentMapContext.h>
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
#include <map>
#include <string>
#include <unistd.h>
#include <unordered_set>
#include <vector>

using namespace clang;

namespace srcdb
{

namespace
{

const Expr* skipParensAndImplicit(const Expr* e)
{
	for (;;)
	{
		const Expr* next = e->IgnoreImplicit();
		if (auto* paren = dyn_cast<ParenExpr>(next))
			next = paren->getSubExpr();
		if (next == e)
			return e;
		e = next;
	}
}

// the function an address expression names: `&f`, `f`; null for anything else
const FunctionDecl* functionOf(const Expr* e, const DeclRefExpr*& name, bool& address)
{
	address = false;
	if (auto* op = dyn_cast<UnaryOperator>(e); op && op->getOpcode() == UO_AddrOf)
	{
		address = true;
		e = skipParensAndImplicit(op->getSubExpr());
	}
	name = dyn_cast<DeclRefExpr>(e);
	return name ? dyn_cast<FunctionDecl>(name->getDecl()) : nullptr;
}

class Finder : public RecursiveASTVisitor<Finder>
{
public:
	Finder(ASTContext& context, const std::string& root, const std::unordered_set<std::string>& names)
		: m_context(context), m_sm(context.getSourceManager()), m_root(root), m_names(names)
	{
	}

	bool shouldVisitTemplateInstantiations() const { return true; }
	bool shouldVisitImplicitCode() const { return false; }

	bool VisitVarDecl(VarDecl* var)
	{
		if (m_second || isa<ParmVarDecl>(var) || var->isTemplated() || var->isImplicit())
			return true;
		VarDecl* first = var->getCanonicalDecl();
		if (!candidate(first))
			return true;
		std::string key;
		if (!keyOf(first, key))
			return true;
		declaration(var, key);
		return true;
	}

	bool VisitTypedefTypeLoc(TypedefTypeLoc type)
	{
		if (m_second)
			return true;
		const TypedefNameDecl* alias = type.getTypePtr()->getDecl()->getCanonicalDecl();
		m_aliasSpellings[alias].push_back(type.getNameLoc());
		return true;
	}

	bool VisitDeclRefExpr(DeclRefExpr* reference)
	{
		auto* var = dyn_cast<VarDecl>(reference->getDecl());
		bool constant = var && candidate(var->getCanonicalDecl());
		if (m_second)
		{
			if (!constant)
				other(reference->getDecl()->getDeclName(), reference->getLocation());
			return true;
		}
		if (!constant)
			return true;
		VarDecl* first = var->getCanonicalDecl();
		std::string key;
		if (!keyOf(first, key))
			return true;
		use(reference, first, key);
		return true;
	}

	// --- the second pass: the spellings of the constants' names that name another declaration ---------------------

	bool VisitNamedDecl(NamedDecl* decl)
	{
		if (!m_second || decl->isImplicit())
			return true;
		if (auto* var = dyn_cast<VarDecl>(decl); var && candidate(var->getCanonicalDecl()))
			return true;
		other(decl->getDeclName(), decl->getLocation());
		return true;
	}

	bool VisitMemberExpr(MemberExpr* member)
	{
		if (m_second)
			other(member->getMemberDecl()->getDeclName(), member->getMemberLoc());
		return true;
	}

	bool VisitOverloadExpr(OverloadExpr* overload)
	{
		if (m_second)
			other(overload->getName(), overload->getNameLoc());
		return true;
	}

	bool VisitCXXDependentScopeMemberExpr(CXXDependentScopeMemberExpr* member)
	{
		if (m_second)
			other(member->getMember(), member->getMemberLoc());
		return true;
	}

	bool VisitDependentScopeDeclRefExpr(DependentScopeDeclRefExpr* reference)
	{
		if (m_second)
			other(reference->getDeclName(), reference->getLocation());
		return true;
	}

	void others()
	{
		for (const auto& [var, is] : m_candidates)
			if (is)
				m_names.insert(var->getNameAsString());
		if (m_names.empty())
			return;
		m_second = true;
		TraverseDecl(m_context.getTranslationUnitDecl());
	}

	// the listed aliases' spellings, once the whole TU is seen
	void finish()
	{
		for (const auto& [alias, key] : m_aliases)
		{
			std::string path;
			unsigned offset;
			if (!fileOffset(alias->getLocation(), path, offset))
				continue;
			emit("aref\t" + key + "\t" + path + "\t" + std::to_string(offset));
			auto found = m_aliasSpellings.find(alias);
			if (found == m_aliasSpellings.end())
				continue;
			for (SourceLocation at : found->second)
				if (fileOffset(at, path, offset))
					emit("aref\t" + key + "\t" + path + "\t" + std::to_string(offset));
		}
	}

private:
	// the constant's address expression and its function, null when var is no constant of this kind
	const FunctionDecl* target(const VarDecl* var, const Expr*& written, const Expr*& address,
							   const ExplicitCastExpr*& cast, const DeclRefExpr*& name, bool& addressOf) const
	{
		const VarDecl* definition = var->getInitializingDeclaration();
		if (!definition || !definition->getType().isConstQualified())
			return nullptr;
		const Expr* init = definition->getInit();
		if (!init)
			return nullptr;
		written = skipParensAndImplicit(init);
		cast = dyn_cast<ExplicitCastExpr>(written);
		address = cast ? skipParensAndImplicit(cast->getSubExpr()) : written;
		return functionOf(address, name, addressOf);
	}

	bool candidate(const VarDecl* first)
	{
		auto known = m_candidates.find(first);
		if (known != m_candidates.end())
			return known->second;
		const Expr *written, *address;
		const ExplicitCastExpr* cast;
		const DeclRefExpr* name;
		bool addressOf;
		bool is = !isa<ParmVarDecl>(first) && !first->isTemplated() && !isa<VarTemplateSpecializationDecl>(first) &&
				  !first->getInstantiatedFromStaticDataMember() &&
				  target(first, written, address, cast, name, addressOf) != nullptr;
		m_candidates[first] = is;
		return is;
	}

	// --- the declaration ------------------------------------------------------------------------------------------

	void declaration(const VarDecl* var, const std::string& key)
	{
		SourceRange range = var->getSourceRange();
		if (range.getBegin().isMacroID() || range.getEnd().isMacroID())
		{
			macro(key, range.getBegin());
			return;
		}
		std::string path;
		unsigned begin, end;
		if (!fileRange(range, path, begin, end))
			return;
		emit("decl\t" + key + "\t" + path + "\t" + std::to_string(begin) + "\t" + std::to_string(end));
		if (var != var->getInitializingDeclaration())
			return;
		const VarDecl* first = var->getCanonicalDecl();
		const Expr *written, *address;
		const ExplicitCastExpr* cast;
		const DeclRefExpr* name;
		bool addressOf;
		const FunctionDecl* function = target(var, written, address, cast, name, addressOf);
		std::string writtenPath, addressPath, namePath;
		unsigned b1, e1, b2, e2, b3, e3;
		if (!fileRange(written->getSourceRange(), writtenPath, b1, e1) ||
			!fileRange(address->getSourceRange(), addressPath, b2, e2) ||
			!fileRange(name->getSourceRange(), namePath, b3, e3))
			return;
		emit("init\t" + key + "\t" + writtenPath + "\t" + std::to_string(b1) + "\t" + std::to_string(e1) + "\t" +
			 std::to_string(b2) + "\t" + std::to_string(e2) + "\t" + std::to_string(b3) + "\t" + std::to_string(e3));

		std::string scope;
		if (first->isStaticDataMember())
			scope = "member";
		else if (first->isStaticLocal())
			scope = "static-local";
		else if (first->isLocalVarDecl())
			scope = "local";
		else if (!first->isExternallyVisible())
			scope = "internal";
		else
			scope = "namespace";
		std::string form = cast ? "cast" : addressOf ? "address" : "decay";
		std::vector<std::string> flags;
		auto* method = dyn_cast<CXXMethodDecl>(function);
		if (method && !method->isStatic())
			flags.push_back("method");
		if (method && method->isVirtual())
			flags.push_back("virtual");
		if (overloaded(function))
			flags.push_back("overloaded");
		QualType own = var->getType().getUnqualifiedType().getCanonicalType();
		QualType of = (addressOf ? address->getType() : m_context.getPointerType(function->getType()))
						  .getCanonicalType();
		if (own == of)
			flags.push_back("same");
		if (function->getTemplateSpecializationInfo() || function->isTemplateInstantiation())
			flags.push_back("template");
		emit("const\t" + key + "\t" + first->getNameAsString() + "\t" + scope + "\t" + form + "\t" + join(flags) +
			 "\t" + qualified(function));

		if (TypeSourceInfo* info = var->getTypeSourceInfo())
		{
			TypeLoc type = info->getTypeLoc().getUnqualifiedLoc();
			if (auto alias = type.getAs<TypedefTypeLoc>())
			{
				const TypedefNameDecl* decl = alias.getTypePtr()->getDecl()->getCanonicalDecl();
				std::string aliasPath, aliasKey;
				unsigned b, e, offset;
				if (!decl->getSourceRange().getBegin().isMacroID() &&
					fileRange(decl->getSourceRange(), aliasPath, b, e) &&
					fileOffset(decl->getLocation(), aliasKey, offset))
				{
					aliasKey += ":" + std::to_string(offset);
					m_aliases[decl] = aliasKey;
					emit("alias\t" + key + "\t" + aliasKey + "\t" + decl->getNameAsString() + "\t" + aliasPath + "\t" +
						 std::to_string(b) + "\t" + std::to_string(e));
				}
			}
		}
	}

	bool overloaded(const FunctionDecl* function) const
	{
		const DeclContext* owner = function->getDeclContext()->getRedeclContext();
		const FunctionDecl* mine = function->getTemplateInstantiationPattern()
									   ? function->getTemplateInstantiationPattern()->getCanonicalDecl()
									   : function->getCanonicalDecl();
		const FunctionTemplateDecl* pattern = function->getPrimaryTemplate();
		for (const NamedDecl* other : owner->lookup(function->getDeclName()))
		{
			if (pattern && other->getCanonicalDecl() == pattern->getCanonicalDecl())
				continue;
			if (other->getCanonicalDecl() != mine)
				return true;
		}
		return false;
	}

	static std::string qualified(const FunctionDecl* function)
	{
		std::string name = function->getQualifiedNameAsString();
		const std::string anonymous = "(anonymous namespace)::";
		for (size_t at; (at = name.find(anonymous)) != std::string::npos;)
			name.erase(at, anonymous.size());
		return "::" + name;
	}

	// --- the uses -------------------------------------------------------------------------------------------------

	void use(const DeclRefExpr* reference, const VarDecl* first, const std::string& key)
	{
		SourceLocation at = reference->getBeginLoc();
		if (inMacroBody(at) || inMacroBody(reference->getEndLoc()))
		{
			macro(key, at);
			return;
		}
		// the expression chain around the use: implicit casts, parentheses, a dereference
		const Expr* node = reference;
		const Expr* callee = reference;
		const Stmt* parent = parentOf(node);
		bool sawDeref = false;
		while (parent)
		{
			if (isa<ImplicitCastExpr>(parent) || isa<ParenExpr>(parent))
				node = cast<Expr>(parent);
			else if (auto* op = dyn_cast<UnaryOperator>(parent); op && op->getOpcode() == UO_Deref && !sawDeref)
			{
				sawDeref = true;
				node = op;
			}
			else
				break;
			if (!isa<ImplicitCastExpr>(node))
				callee = node;
			parent = parentOf(node);
		}
		std::string context = "value";
		std::vector<std::string> flags;
		SourceRange range = reference->getSourceRange();
		if (auto* call = dyn_cast_or_null<CallExpr>(parent); call && call->getCallee() == node)
		{
			context = "call";
			range = callee->getSourceRange();
		}
		else
		{
			// a use that is not a call: the parent is around the reference itself
			parent = parentOf(reference);
			while (parent && isa<ImplicitCastExpr>(parent))
				parent = parentOf(cast<Expr>(parent));
			if (auto* op = dyn_cast_or_null<UnaryOperator>(parent); op && op->getOpcode() == UO_AddrOf)
				context = "address";
			else if (auto* op = dyn_cast_or_null<BinaryOperator>(parent); op && op->isPtrMemOp())
			{
				const Stmt* around = parentOf(op);
				while (around && isa<ParenExpr>(around))
					around = parentOf(cast<Expr>(around));
				if (isa_and_nonnull<CXXMemberCallExpr>(around) || isa_and_nonnull<CallExpr>(around))
				{
					context = "member-call";
					QualType object = op->getLHS()->getType();
					if (op->getOpcode() == BO_PtrMemI && object->isPointerType())
						object = object->getPointeeType();
					if (const CXXRecordDecl* record = object->getAsCXXRecordDecl())
						flags.push_back("member-of=::" + record->getQualifiedNameAsString());
				}
			}
		}
		if (unevaluated(reference))
			flags.push_back("unevaluated");
		if (const char* lookup = unqualifiedLookup(reference, first))
			flags.push_back(lookup);
		std::string path;
		unsigned begin, end;
		if (!fileRange(range, path, begin, end))
			return;
		std::string around = parent ? parent->getStmtClassName() : "-";
		emit("use\t" + key + "\t" + path + "\t" + std::to_string(begin) + "\t" + std::to_string(end) + "\t" + context +
			 "\t" + around + "\t" + join(flags));
	}

	const Stmt* parentOf(const Stmt* node) const
	{
		DynTypedNodeList parents = m_context.getParents(*node);
		return parents.empty() ? nullptr : parents[0].get<Stmt>();
	}

	bool unevaluated(const Expr* reference) const
	{
		DynTypedNodeList parents = m_context.getParents(*reference);
		while (!parents.empty())
		{
			const DynTypedNode& up = parents[0];
			if (auto* trait = up.get<UnaryExprOrTypeTraitExpr>(); trait && !trait->isArgumentType())
				return true;
			if (up.get<CXXNoexceptExpr>() || up.get<TypeLoc>())
				return true;
			if (up.get<Decl>())
				return false;
			parents = m_context.getParents(up);
		}
		return false;
	}

	// what the function's unqualified name, looked up from the use's scopes outward, finds: "visible" (the function,
	// in its own scope, a derived class's or an enclosing one of its transparent namespace), "shadowed" (another
	// declaration first) or nothing (the use is outside its scope)
	const char* unqualifiedLookup(const DeclRefExpr* reference, const VarDecl* first) const
	{
		const Expr *written, *address;
		const ExplicitCastExpr* cast;
		const DeclRefExpr* name;
		bool addressOf;
		const FunctionDecl* function = target(first, written, address, cast, name, addressOf);
		if (!function || !function->getDeclName().isIdentifier())
			return nullptr;
		const DeclContext* home = function->getDeclContext()->getRedeclContext();
		auto* homeRecord = dyn_cast<CXXRecordDecl>(home);
		auto* homeSpace = dyn_cast<NamespaceDecl>(home);
		for (const DeclContext* scope = enclosing(reference); scope; scope = scope->getParent())
		{
			const DeclContext* here = scope->getRedeclContext();
			if (here->Equals(home))
				return "visible";
			if (meets(here, function))
				return "shadowed";
			if (auto* record = dyn_cast<CXXRecordDecl>(here); record && homeRecord && record->hasDefinition() &&
				record->isDerivedFrom(homeRecord))
				return "visible";
			if (homeSpace && (homeSpace->isAnonymousNamespace() || homeSpace->isInline()) &&
				homeSpace->getParent()->getRedeclContext()->Equals(here))
				return "visible";
		}
		return nullptr;
	}

	bool meets(const DeclContext* scope, const FunctionDecl* function) const
	{
		DeclarationName name = function->getDeclName();
		auto other = [&](const NamedDecl* decl) {
			return decl->getCanonicalDecl() != function->getCanonicalDecl() &&
				   !(function->getPrimaryTemplate() &&
					 decl->getCanonicalDecl() == function->getPrimaryTemplate()->getCanonicalDecl());
		};
		for (const NamedDecl* decl : scope->lookup(name))
			if (other(decl))
				return true;
		if (auto* record = dyn_cast<CXXRecordDecl>(scope); record && record->hasDefinition())
		{
			bool found = false;
			record->forallBases([&](const CXXRecordDecl* base) {
				for (const NamedDecl* decl : base->lookup(name))
					if (other(decl))
						found = true;
				return true;
			});
			return found;
		}
		return false;
	}

	// the innermost declaration context of the use (its function, else the declaration around it)
	const DeclContext* enclosing(const Expr* reference) const
	{
		DynTypedNodeList parents = m_context.getParents(*reference);
		while (!parents.empty())
		{
			if (auto* decl = parents[0].get<Decl>())
			{
				if (auto* context = dyn_cast<DeclContext>(decl))
					return context;
				return decl->getDeclContext();
			}
			parents = m_context.getParents(parents[0]);
		}
		return nullptr;
	}

	// --- locations ------------------------------------------------------------------------------------------------

	bool inMacroBody(SourceLocation at) const
	{
		while (at.isMacroID())
		{
			if (!m_sm.isMacroArgExpansion(at))
				return true;
			at = m_sm.getImmediateSpellingLoc(at);
		}
		return false;
	}

	SourceLocation fileLocation(SourceLocation at) const
	{
		while (at.isMacroID())
			at = m_sm.getImmediateSpellingLoc(at);
		return at;
	}

	bool pathOf(SourceLocation at, std::string& path) const
	{
		OptionalFileEntryRef file = m_sm.getFileEntryRefForID(m_sm.getFileID(at));
		if (!file)
			return false;
		auto known = m_paths.find(file->getName().str());
		if (known != m_paths.end())
		{
			path = known->second;
			return !path.empty();
		}
		llvm::SmallString<256> real;
		std::string& slot = m_paths[file->getName().str()];
		if (llvm::sys::fs::real_path(file->getName(), real))
			return false;
		llvm::StringRef text = real.str();
		if (!text.starts_with(m_root))
			return false;
		llvm::StringRef relative = text.drop_front(m_root.size());
		if (!relative.starts_with("src/") && !relative.starts_with("gen/"))
			return false;
		slot = relative.str();
		path = slot;
		return true;
	}

	bool fileOffset(SourceLocation at, std::string& path, unsigned& offset) const
	{
		if (at.isInvalid() || inMacroBody(at))
			return false;
		at = fileLocation(at);
		if (!pathOf(at, path))
			return false;
		offset = m_sm.getFileOffset(at);
		return true;
	}

	bool fileRange(SourceRange range, std::string& path, unsigned& begin, unsigned& end) const
	{
		if (range.isInvalid() || inMacroBody(range.getBegin()) || inMacroBody(range.getEnd()))
			return false;
		SourceLocation first = fileLocation(range.getBegin()), last = fileLocation(range.getEnd());
		if (m_sm.getFileID(first) != m_sm.getFileID(last) || !pathOf(first, path))
			return false;
		begin = m_sm.getFileOffset(first);
		end = m_sm.getFileOffset(last) + Lexer::MeasureTokenLength(last, m_sm, m_context.getLangOpts());
		return end >= begin;
	}

	bool keyOf(const VarDecl* first, std::string& key) const
	{
		std::string path;
		unsigned offset;
		if (!fileOffset(first->getLocation(), path, offset))
			return false;
		key = path + ":" + std::to_string(offset);
		return true;
	}

	void other(DeclarationName name, SourceLocation at)
	{
		if (!name.isIdentifier() || !m_names.count(name.getAsString()))
			return;
		std::string path;
		unsigned offset;
		if (fileOffset(at, path, offset))
			emit("other\t" + name.getAsString() + "\t" + path + "\t" + std::to_string(offset));
	}

	void macro(const std::string& key, SourceLocation at)
	{
		std::string path;
		SourceLocation expansion = m_sm.getExpansionLoc(at);
		if (pathOf(expansion, path))
			emit("macro\t" + key + "\t" + path + "\t" + std::to_string(m_sm.getExpansionLineNumber(at)));
	}

	static std::string join(const std::vector<std::string>& words)
	{
		std::string out;
		for (const std::string& word : words)
			out += (out.empty() ? "" : ",") + word;
		return out.empty() ? "-" : out;
	}

	void emit(const std::string& line)
	{
		if (m_seen.insert(line).second)
			llvm::outs() << line << "\n";
	}

	ASTContext& m_context;
	SourceManager& m_sm;
	std::string m_root;
	std::map<const VarDecl*, bool> m_candidates;
	std::map<const TypedefNameDecl*, std::string> m_aliases;
	std::map<const TypedefNameDecl*, std::vector<SourceLocation>> m_aliasSpellings;
	mutable std::map<std::string, std::string> m_paths;
	std::unordered_set<std::string> m_seen;
	bool m_second = false;
	std::unordered_set<std::string> m_names;
};

using Names = std::unordered_set<std::string>;

class Consumer : public ASTConsumer
{
public:
	Consumer(const std::string& root, const Names& names) : m_root(root), m_names(names) {}

	void HandleTranslationUnit(ASTContext& context) override
	{
		Finder finder(context, m_root, m_names);
		finder.TraverseDecl(context.getTranslationUnitDecl());
		finder.finish();
		finder.others();
	}

private:
	std::string m_root;
	const Names& m_names;
};

class Action : public ASTFrontendAction
{
public:
	Action(const std::string& root, const Names& names) : m_root(root), m_names(names) {}

	std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance&, llvm::StringRef) override
	{
		return std::make_unique<Consumer>(m_root, m_names);
	}

private:
	std::string m_root;
	const Names& m_names;
};

}  // namespace

int runFoldRefs(int argc, char** argv)
{
	std::string root, file, directory, names;
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
			throw Error("srcdb fold-refs: " + word + " needs a value");
		if (word == "--root")
			root = argv[++k];
		else if (word == "--file")
			file = argv[++k];
		else if (word == "--directory")
			directory = argv[++k];
		else if (word == "--names")
			names = argv[++k];
		else
			throw Error("srcdb fold-refs: unexpected argument " + word);
	}
	if (root.empty() || file.empty() || command.empty())
		throw Error("srcdb fold-refs: needs --root, --file and the compile command after --");
	llvm::SmallString<256> real;
	if (llvm::sys::fs::real_path(root, real))
		throw Error("srcdb fold-refs: no directory " + root);
	root = real.str().str() + "/";
	Names known;
	if (!names.empty())
	{
		std::ifstream in(names);
		for (std::string line; std::getline(in, line);)
			if (!line.empty())
				known.insert(line);
	}
	if (!directory.empty() && chdir(directory.c_str()) != 0)
		throw Error("srcdb fold-refs: cannot enter " + directory);
	return parseTu(root, file, command, std::make_unique<Action>(root, known));
}

}  // namespace srcdb
