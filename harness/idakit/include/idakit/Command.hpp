// idakit's CLI commands (`idakit COMMAND ...`) and the argument parser they share.
//
// A new command: a function `int runX(Args& args)` in tools/XCommand.cpp, declared below, and one row in commands()
// (tools/Commands.cpp). It reads its options from Args, opens a Database if it needs one, and returns the exit
// status (0 done, 1 done with failures, 2 bad arguments or setup).
#pragma once

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace idakit
{

// `--name value`, `--flag` and positional words, in any order; `--` ends the options.
class Args
{
public:
	// flags: the options that take no value
	Args(const std::vector<std::string>& words, const std::vector<std::string>& flags);

	// the option's value (its last, where it is given more than once)
	std::optional<std::string> option(const std::string& name) const;
	// every value of an option given more than once, in order
	std::vector<std::string> values(const std::string& name) const;
	// the option's value; throws Error naming it when missing
	std::string required(const std::string& name) const;
	bool flag(const std::string& name) const;
	const std::vector<std::string>& positional() const { return m_positional; }

private:
	std::map<std::string, std::vector<std::string>> m_options;
	std::vector<std::string> m_flags;
	std::vector<std::string> m_positional;
};

struct Command
{
	const char* name;
	const char* usage;    // the arguments after the command name
	const char* summary;  // one line
	int (*run)(Args& args);
	std::vector<std::string> flags;  // its options without a value
};

const std::vector<Command>& commands();

int runAllocators(Args& args);
int runDeclare(Args& args);
int runDecompile(Args& args);
int runEmit(Args& args);
int runFrames(Args& args);
int runFunctions(Args& args);
int runInfer(Args& args);
int runLocals(Args& args);
int runMerge(Args& args);
int runSync(Args& args);

}  // namespace idakit
