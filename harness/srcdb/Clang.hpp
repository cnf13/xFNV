// What srcdb's commands share: running one clang parse of a TU with the hybrid build's clang-cl flags.
#pragma once

#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Frontend/FrontendAction.h>

#include <memory>
#include <string>
#include <vector>

namespace srcdb
{

// MS-style inline asm (Reconstruction.hpp's naked bodies) is parsed through the target's MC layer.
void initializeTargets();

// The hybrid build's compile command for file as a parse only: clang-cl's driver mode (taken from argv[0]'s name,
// which may be a symlink), the compiler's real path (its resource directory: the builtin headers), no outputs, no
// debug info, /Zs. command: the compile_commands.json command line, with or without its source and outputs.
std::vector<std::string> syntaxOnlyCommand(const std::vector<std::string>& command, const std::string& file);

// A GNU-style compile command (the Linux build's) for file as a parse only: no outputs, no dependency files, no
// debug info, -fsyntax-only, every error reported, no warnings.
std::vector<std::string> gnuSyntaxOnly(const std::vector<std::string>& command, const std::string& file);

// One parse of the TU args name; false when clang could not run it at all (errors in the TU still return true).
bool runParse(const std::vector<std::string>& args, std::unique_ptr<clang::FrontendAction> action,
			  clang::DiagnosticConsumer& diagnostics, clang::DiagnosticOptions& options);

// One parse of file by a compile_commands.json command (clang-cl's or a GNU driver's), with no error limit and no
// warnings; each error located in a file under root's src/ or gen/ is printed once as `err FILE LINE TEXT` (root:
// a real path ending in `/`). 0: parsed without errors, 3: with errors, 1: clang could not run.
int parseTu(const std::string& root, const std::string& file, const std::vector<std::string>& command,
			std::unique_ptr<clang::FrontendAction> action);

}  // namespace srcdb
