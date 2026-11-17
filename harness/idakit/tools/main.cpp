// idakit: the harness's IDA tools on the IDA SDK (idalib). `idakit COMMAND ARGS...`; `idakit` alone lists them.

#include "idakit/Command.hpp"
#include "idakit/Database.hpp"

#include <cstdio>
#include <cstring>
#include <exception>

int main(int argc, char** argv)
{
	using namespace idakit;
	const std::vector<Command>& table = commands();
	if (argc < 2)
	{
		std::fprintf(stderr, "usage: idakit COMMAND ...\n");
		for (const Command& command : table)
			std::fprintf(stderr, "  %s %s\n      %s\n", command.name, command.usage, command.summary);
		return 2;
	}
	for (const Command& command : table)
	{
		if (std::strcmp(argv[1], command.name) != 0)
			continue;
		try
		{
			Args args(std::vector<std::string>(argv + 2, argv + argc), command.flags);
			return command.run(args);
		}
		catch (const std::exception& error)
		{
			std::fprintf(stderr, "%s\n", error.what());
			return 2;
		}
	}
	std::fprintf(stderr, "idakit: no command %s (`idakit` lists them)\n", argv[1]);
	return 2;
}
