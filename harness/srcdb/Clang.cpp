#include "Clang.hpp"

#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/SourceManager.h>
#include <clang/Tooling/Tooling.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

#include <algorithm>
#include <unordered_set>

namespace srcdb
{

void initializeTargets()
{
	llvm::InitializeAllTargetInfos();
	llvm::InitializeAllTargetMCs();
	llvm::InitializeAllAsmParsers();
}

std::vector<std::string> syntaxOnlyCommand(const std::vector<std::string>& command, const std::string& file)
{
	std::vector<std::string> args;
	for (size_t k = 0; k < command.size(); ++k)
	{
		llvm::StringRef word = command[k];
		if (k > 0 && (word == file || word == "--" || word.starts_with("/Fo") || word.starts_with("/Fd") ||
					  word == "-c" || word == "/c" || word == "-Zi" || word == "/Zi"))
			continue;
		args.push_back(word.str());
	}
	if (llvm::sys::path::filename(args[0]).starts_with("clang-cl"))
		args.insert(args.begin() + 1, "--driver-mode=cl");
	llvm::SmallString<256> real;
	if (!llvm::sys::fs::real_path(args[0], real))
		args[0] = std::string(real.str());
	args.push_back("/Zs");
	args.push_back("--");
	args.push_back(file);
	return args;
}

// a GNU-style compile command as a parse only (clang-cl's goes through syntaxOnlyCommand)
std::vector<std::string> gnuSyntaxOnly(const std::vector<std::string>& command, const std::string& file)
{
	std::vector<std::string> args;
	for (size_t k = 0; k < command.size(); ++k)
	{
		llvm::StringRef word = command[k];
		if (k > 0 && (word == "-o" || word == "-MF" || word == "-MT" || word == "-MQ"))
		{
			++k;
			continue;
		}
		if (k > 0 && (word == file || word == "-c" || word == "-MD" || word == "-MMD" || word == "--" ||
					  word.starts_with("-g") || word.starts_with("-fdebug-compilation-dir")))
			continue;
		args.push_back(word.str());
	}
	llvm::SmallString<256> real;
	if (!llvm::sys::fs::real_path(args[0], real))
		args[0] = std::string(real.str());
	args.push_back("-fsyntax-only");
	args.push_back("-ferror-limit=0");
	args.push_back("-Wno-everything");
	args.push_back(file);
	return args;
}

bool runParse(const std::vector<std::string>& args, std::unique_ptr<clang::FrontendAction> action,
			  clang::DiagnosticConsumer& diagnostics, clang::DiagnosticOptions& options)
{
	auto files = llvm::makeIntrusiveRefCnt<clang::FileManager>(clang::FileSystemOptions());
	clang::tooling::ToolInvocation invocation(args, std::move(action), files.get());
	invocation.setDiagnosticConsumer(&diagnostics);
	invocation.setDiagnosticOptions(&options);
	return invocation.run();
}

namespace
{

// counts the diagnostics; an error located in a file under the root's src/ or gen/ is listed (`err FILE LINE TEXT`),
// so that a caller comparing two parses sees the errors an edit brings (a new name already taken in the scope)
class Quiet : public clang::DiagnosticConsumer
{
public:
	explicit Quiet(const std::string& root) : m_root(root) {}

	void HandleDiagnostic(clang::DiagnosticsEngine::Level level, const clang::Diagnostic& info) override
	{
		DiagnosticConsumer::HandleDiagnostic(level, info);
		if (level < clang::DiagnosticsEngine::Error || !info.hasSourceManager() || info.getLocation().isInvalid())
			return;
		clang::SourceManager& sm = info.getSourceManager();
		clang::SourceLocation at = sm.getFileLoc(info.getLocation());
		clang::OptionalFileEntryRef entry = sm.getFileEntryRefForID(sm.getFileID(at));
		llvm::SmallString<256> real;
		if (!entry || llvm::sys::fs::real_path(entry->getName(), real))
			return;
		llvm::StringRef path = real.str();
		if (!path.starts_with(m_root))
			return;
		path = path.drop_front(m_root.size());
		if (!path.starts_with("src/") && !path.starts_with("gen/"))
			return;
		llvm::SmallString<256> text;
		info.FormatDiagnostic(text);
		std::string line = "err\t" + path.str() + "\t" + std::to_string(sm.getSpellingLineNumber(at)) + "\t" +
						   std::string(text.str());
		if (m_seen.insert(line).second)
			llvm::outs() << line << "\n";
	}

private:
	std::string m_root;
	std::unordered_set<std::string> m_seen;
};

}  // namespace

int parseTu(const std::string& root, const std::string& file, const std::vector<std::string>& command,
			std::unique_ptr<clang::FrontendAction> action)
{
	initializeTargets();
	bool cl = llvm::StringRef(command[0]).contains("clang-cl") ||
			  std::find(command.begin(), command.end(), "--driver-mode=cl") != command.end();
	std::vector<std::string> args = cl ? syntaxOnlyCommand(command, file) : gnuSyntaxOnly(command, file);
	if (cl)
	{
		args.insert(args.end() - 2, "/clang:-ferror-limit=0");
		args.insert(args.end() - 2, "/clang:-Wno-everything");
	}
	Quiet diagnostics(root);
	clang::DiagnosticOptions options;
	bool ran = runParse(args, std::move(action), diagnostics, options);
	llvm::outs().flush();
	if (!ran)
		return 1;
	return diagnostics.getNumErrors() ? 3 : 0;
}

}  // namespace srcdb
