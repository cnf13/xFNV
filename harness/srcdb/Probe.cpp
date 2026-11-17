// srcdb probe: srcindex's layout probe (harness/tools/srcindex.py _run_probe) as one in-process clang parse.
//
// Usage: srcdb probe < REQUEST.json > RESULT.json
//
// REQUEST: {"directory": the compile's working directory, "command": [clang-cl, flags...] (the hybrid build's,
// without the source, /Fo, /c), "file": the probe TU (a file: its #includes, an instance's static_assert, then one
// `using fnvdSrcdbClassN = <class>;` per entry of "classes" from line "alias_line" on), "classes": [names],
// "alias_line": N, "dep_roots": [path prefixes whose files count as dependencies]}.
//
// RESULT: {"ok": no error outside the alias lines, "error": the diagnostics, "deps": [included files under
// dep_roots], "records": clang's -fdump-record-layouts-complete text (every record the TU completes, in completion
// order: ASTContext::DumpRecordLayout), "canonical": the same records with canonical field types
// (-fdump-record-layouts-canonical), "vtables": [{"path": ["Base", ..., "Class"], "entries": [{"text", "adjust"}]}]
// (the MS ABI vftables of each class an alias names and of its dynamic bases, spelled as -fdump-vtable-layouts
// prints them), "missing": [classes whose alias did not name a complete class]}.
//
// The text forms are clang's own printers, so srcindex parses them exactly as it parsed the clang-cl dumps the
// probe used to compile three times.

#include "Clang.hpp"

#include <clang/AST/ASTConsumer.h>
#include <clang/AST/ASTMutationListener.h>
#include <clang/AST/ASTContext.h>
#include <clang/AST/DeclCXX.h>
#include <clang/AST/Expr.h>
#include <clang/AST/RecordLayout.h>
#include <clang/AST/VTableBuilder.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Sema/Sema.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/JSON.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_ostream.h>

#include <map>
#include <set>
#include <string>
#include <vector>

using namespace clang;

namespace srcdb
{

namespace
{

struct Request
{
	std::string directory;
	std::vector<std::string> command;
	std::string file;
	std::vector<std::string> classes;
	unsigned aliasLine = 0;
	std::vector<std::string> depRoots;
};

struct Result
{
	bool fatal = false;  // an error outside the alias lines
	std::string diagnostics;
	std::vector<std::string> deps;
	std::string records;
	std::string canonical;
	llvm::json::Array vtables;
	llvm::json::Array missing;
};

// The diagnostics as clang-cl prints them; an error on an alias line only drops that class.
class Diagnostics : public DiagnosticConsumer
{
public:
	Diagnostics(Result& result, DiagnosticOptions& options, unsigned aliasLine)
		: m_result(result), m_stream(result.diagnostics), m_printer(m_stream, options), m_aliasLine(aliasLine)
	{
	}

	void BeginSourceFile(const LangOptions& lang, const Preprocessor* pp) override
	{
		m_printer.BeginSourceFile(lang, pp);
	}

	void EndSourceFile() override { m_printer.EndSourceFile(); }

	void HandleDiagnostic(DiagnosticsEngine::Level level, const Diagnostic& info) override
	{
		DiagnosticConsumer::HandleDiagnostic(level, info);
		m_printer.HandleDiagnostic(level, info);
		if (level < DiagnosticsEngine::Error)
			return;
		bool alias = false;
		if (info.hasSourceManager() && info.getLocation().isValid())
		{
			const SourceManager& sm = info.getSourceManager();
			SourceLocation at = sm.getExpansionLoc(info.getLocation());
			alias = m_aliasLine && sm.isInMainFile(at) && sm.getExpansionLineNumber(at) >= m_aliasLine;
		}
		if (!alias)
			m_result.fatal = true;
	}

private:
	Result& m_result;
	llvm::raw_string_ostream m_stream;
	TextDiagnosticPrinter m_printer;
	unsigned m_aliasLine;
};

// -fdump-vtable-layouts' spelling of a thunk's this adjustment (dumpMicrosoftThunkAdjustment): the integer when it
// is only non-virtual, else the text inside `[this adjustment: ...]`.
llvm::json::Value thisAdjustment(const ThunkInfo& thunk)
{
	const ThisAdjustment& t = thunk.This;
	if (t.Virtual.isEmpty())
		return t.NonVirtual;
	std::string text;
	llvm::raw_string_ostream out(text);
	out << "vtordisp at " << t.Virtual.Microsoft.VtordispOffset << ", ";
	if (t.Virtual.Microsoft.VBPtrOffset)
	{
		out << "vbptr at " << t.Virtual.Microsoft.VBPtrOffset << " to the left,";
		out << "\n        vboffset at " << t.Virtual.Microsoft.VBOffsetOffset << " in the vbtable, ";
	}
	out << t.NonVirtual << " non-virtual";
	return text;
}

class Collector : public ASTConsumer, public ASTMutationListener
{
public:
	Collector(CompilerInstance& ci, const Request& request, Result& result)
		: m_ci(ci), m_request(request), m_result(result)
	{
	}

	ASTMutationListener* GetASTMutationListener() override { return this; }

	void CompletedTagDefinition(const TagDecl* tag) override
	{
		// what -fdump-record-layouts-complete lays out: RecordDecl::completeDefinition computes (and so dumps) the
		// layout of every record it completes that is neither dependent nor invalid, the implicit builtins too, at
		// completion (an anonymous struct a typedef names later is still `(unnamed at ...)` then); sugared and
		// canonical
		auto* record = dyn_cast<RecordDecl>(tag);
		if (!record || record->isDependentType() || record->isInvalidDecl())
			return;
		ASTContext& context = record->getASTContext();
		auto& lang = const_cast<LangOptions&>(context.getLangOpts());
		llvm::raw_string_ostream sugared(m_result.records), canonical(m_result.canonical);
		sugared << "\n*** Dumping AST Record Layout\n";
		context.DumpRecordLayout(record, sugared, /*Simple=*/false);
		lang.DumpRecordLayoutsCanonical = true;
		canonical << "\n*** Dumping AST Record Layout\n";
		context.DumpRecordLayout(record, canonical, /*Simple=*/false);
		lang.DumpRecordLayoutsCanonical = false;
	}

	void HandleTranslationUnit(ASTContext& context) override
	{
		if (!m_result.fatal)
			vtables(context);
	}

private:
	void vtables(ASTContext& context)
	{
		for (Decl* decl : context.getTranslationUnitDecl()->decls())
		{
			auto* alias = dyn_cast<TypeAliasDecl>(decl);
			if (alias && alias->getName().starts_with("fnvdSrcdbClass") && !alias->isInvalidDecl())
				m_aliases[alias->getName().str()] = alias;
		}
		Sema& sema = m_ci.getSema();
		std::vector<const CXXRecordDecl*> wanted;
		for (size_t k = 0; k < m_request.classes.size(); ++k)
		{
			std::string key = "fnvdSrcdbClass" + std::to_string(k);
			auto found = m_aliases.find(key);
			const CXXRecordDecl* record = nullptr;
			if (found != m_aliases.end())
			{
				QualType type = found->second->getUnderlyingType();
				if (sema.isCompleteType(found->second->getLocation(), type))
					record = type->getAsCXXRecordDecl();
			}
			if (!record || !record->hasDefinition() || record->isInvalidDecl() || record->isDependentType())
			{
				m_result.missing.push_back(m_request.classes[k]);
				continue;
			}
			wanted.push_back(record->getDefinition());
		}
		if (m_result.fatal || !context.getTargetInfo().getCXXABI().isMicrosoft())
			return;
		auto* vtable = cast<MicrosoftVTableContext>(context.getVTableContext());
		std::set<const CXXRecordDecl*> done;
		for (const CXXRecordDecl* record : wanted)
			tables(sema, vtable, record, done);
	}

	// the vftables of record and its dynamic bases (codegen of the old probe's constructor/destructor emitted the
	// bases' tables too)
	void tables(Sema& sema, MicrosoftVTableContext* vtable, const CXXRecordDecl* record,
				std::set<const CXXRecordDecl*>& done)
	{
		if (!done.insert(record).second)
			return;
		for (const CXXBaseSpecifier& base : record->bases())
		{
			const CXXRecordDecl* b = base.getType()->getAsCXXRecordDecl();
			if (b && b->hasDefinition())
				tables(sema, vtable, b->getDefinition(), done);
		}
		if (!record->isDynamicClass())
			return;
		sema.ForceDeclarationOfImplicitMembers(const_cast<CXXRecordDecl*>(record));
		for (const auto& vptr : vtable->getVFPtrOffsets(record))
		{
			llvm::json::Array path;
			for (auto it = vptr->PathToIntroducingObject.rbegin(); it != vptr->PathToIntroducingObject.rend(); ++it)
				path.push_back(qualified(*it));
			path.push_back(qualified(record));
			const VTableLayout& layout = vtable->getVFTableLayout(record, vptr->FullOffsetInMDC);
			std::map<uint64_t, const ThunkInfo*> thunks;
			for (const auto& thunk : layout.vtable_thunks())
				thunks[thunk.first] = &thunk.second;
			llvm::json::Array entries;
			auto components = layout.vtable_components();
			for (uint64_t i = 0; i < components.size(); ++i)
			{
				const VTableComponent& component = components[i];
				std::string text;
				llvm::raw_string_ostream out(text);
				switch (component.getKind())
				{
				case VTableComponent::CK_FunctionPointer:
				{
					const CXXMethodDecl* method = component.getFunctionDecl();
					out << PredefinedExpr::ComputeName(PredefinedIdentKind::PrettyFunctionNoVirtual, method);
					if (method->isPureVirtual())
						out << " [pure]";
					if (method->isDeleted())
						out << " [deleted]";
					break;
				}
				case VTableComponent::CK_DeletingDtorPointer:
				{
					const CXXDestructorDecl* destructor = component.getDestructorDecl();
					destructor->printQualifiedName(out);
					// clang's MS ABI dumper names the slot after the deleting destructor the target emits
					out << (destructor->getASTContext().getTargetInfo().emitVectorDeletingDtors(
								destructor->getASTContext().getLangOpts())
								? "() [vector deleting]"
								: "() [scalar deleting]");
					if (destructor->isPureVirtual())
						out << " [pure]";
					break;
				}
				default:
					continue;  // RTTI and the Itanium-only kinds: no slot the probe keeps
				}
				llvm::json::Value adjust = 0;
				auto thunk = thunks.find(i);
				if (thunk != thunks.end() && !thunk->second->This.isEmpty())
					adjust = thisAdjustment(*thunk->second);
				entries.push_back(llvm::json::Object{{"text", text}, {"adjust", std::move(adjust)}});
			}
			m_result.vtables.push_back(llvm::json::Object{{"path", std::move(path)},
														  {"entries", std::move(entries)}});
		}
	}

	static std::string qualified(const CXXRecordDecl* record)
	{
		std::string text;
		llvm::raw_string_ostream out(text);
		record->printQualifiedName(out);
		return text;
	}

	CompilerInstance& m_ci;
	const Request& m_request;
	Result& m_result;
	std::map<std::string, const TypeAliasDecl*> m_aliases;
};

class ProbeAction : public ASTFrontendAction
{
public:
	ProbeAction(const Request& request, Result& result) : m_request(request), m_result(result) {}

	std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& ci, StringRef) override
	{
		return std::make_unique<Collector>(ci, m_request, m_result);
	}

	void EndSourceFileAction() override
	{
		// the files the TU read under the dependency roots (the probe's own TU lies outside them)
		const SourceManager& sm = getCompilerInstance().getSourceManager();
		std::set<std::string> seen;
		for (auto it = sm.fileinfo_begin(); it != sm.fileinfo_end(); ++it)
		{
			llvm::SmallString<256> real;
			if (llvm::sys::fs::real_path(it->first.getName(), real))
				continue;
			std::string path(real.str());
			for (const std::string& root : m_request.depRoots)
			{
				if (llvm::StringRef(path).starts_with(root) && seen.insert(path).second)
				{
					m_result.deps.push_back(path);
					break;
				}
			}
		}
	}

private:
	const Request& m_request;
	Result& m_result;
};

bool parseRequest(llvm::StringRef text, Request& request, std::string& error)
{
	llvm::Expected<llvm::json::Value> value = llvm::json::parse(text);
	if (!value)
	{
		error = llvm::toString(value.takeError());
		return false;
	}
	const llvm::json::Object* object = value->getAsObject();
	if (!object)
	{
		error = "request is not a JSON object";
		return false;
	}
	auto strings = [&](llvm::StringRef key, std::vector<std::string>& out) {
		if (const llvm::json::Array* array = object->getArray(key))
			for (const llvm::json::Value& item : *array)
				if (auto s = item.getAsString())
					out.push_back(s->str());
	};
	if (auto s = object->getString("directory"))
		request.directory = s->str();
	if (auto s = object->getString("file"))
		request.file = s->str();
	if (auto n = object->getInteger("alias_line"))
		request.aliasLine = static_cast<unsigned>(*n);
	strings("command", request.command);
	strings("classes", request.classes);
	strings("dep_roots", request.depRoots);
	if (request.command.empty() || request.file.empty())
	{
		error = "request needs command and file";
		return false;
	}
	return true;
}

}  // namespace

int runProbe()
{
	auto input = llvm::MemoryBuffer::getSTDIN();
	if (!input)
	{
		llvm::errs() << "srcdb: cannot read stdin\n";
		return 2;
	}
	Request request;
	std::string error;
	if (!parseRequest((*input)->getBuffer(), request, error))
	{
		llvm::errs() << "srcdb: " << error << "\n";
		return 2;
	}
	if (!request.directory.empty() && llvm::sys::fs::set_current_path(request.directory))
	{
		llvm::errs() << "srcdb: cannot enter " << request.directory << "\n";
		return 2;
	}
	initializeTargets();
	Result result;
	DiagnosticOptions options;
	options.setShowColors(ShowColorsKind::Off);
	Diagnostics diagnostics(result, options, request.aliasLine);
	bool ran = runParse(syntaxOnlyCommand(request.command, request.file),
						std::make_unique<ProbeAction>(request, result), diagnostics, options);
	if (!ran && result.diagnostics.empty())
		result.fatal = true, result.diagnostics = "srcdb: the clang invocation failed";

	llvm::json::Array deps;
	for (const std::string& dep : result.deps)
		deps.push_back(dep);
	llvm::json::Object out{
		{"ok", !result.fatal},
		{"error", result.diagnostics},
		{"deps", std::move(deps)},
		{"records", result.fatal ? std::string() : result.records},
		{"canonical", result.fatal ? std::string() : result.canonical},
		{"vtables", result.fatal ? llvm::json::Array() : std::move(result.vtables)},
		{"missing", std::move(result.missing)},
	};
	llvm::outs() << llvm::json::Value(std::move(out)) << "\n";
	return 0;
}

}  // namespace srcdb
