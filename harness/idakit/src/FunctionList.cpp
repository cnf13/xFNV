#include "idakit/FunctionList.hpp"

#include "idakit/Address.hpp"
#include "idakit/Database.hpp"
#include "idakit/Ida.hpp"

#include <filesystem>
#include <fstream>

#include <unistd.h>

namespace idakit
{

std::string functionName(uint64_t start)
{
	qstring name;
	get_func_name(&name, start);
	qstring demangled;
	if (demangle_name(&demangled, name.c_str(), 0) > 0)
		return demangled.c_str();
	return name.c_str();
}

namespace
{

// rows(out) writes the rows and returns their count; path is replaced only once all are written, from a file of this
// process's own, so that a concurrent writer of the same list never interleaves with it
template <typename Rows>
size_t writeList(const std::string& path, Rows rows)
{
	std::string partial = path + ".partial." + std::to_string(getpid());
	size_t count = 0;
	{
		std::ofstream out(partial, std::ios::binary | std::ios::trunc);
		count = rows(out);
		if (!out)
			throw Error("idakit: cannot write " + partial);
	}
	std::filesystem::rename(partial, path);
	return count;
}

}  // namespace

size_t writeFunctionList(const std::string& path)
{
	return writeList(path, [](std::ofstream& out) {
		size_t rows = 0;
		// idautils.Functions()'s walk: getn_func gives some functions under several indices
		for (func_t* function = get_next_func(0); function != nullptr; function = get_next_func(function->start_ea))
		{
			out << "0x" << hex8(function->start_ea) << "\t0x" << hex8(function->end_ea) << '\t'
				<< functionName(function->start_ea) << '\n';
			++rows;
		}
		return rows;
	});
}

size_t writeNameList(const std::string& path)
{
	return writeList(path, [](std::ofstream& out) {
		size_t rows = get_nlist_size();
		for (size_t k = 0; k < rows; ++k)
			out << "0x" << hex8(get_nlist_ea(k)) << '\t' << get_nlist_name(k) << '\n';
		return rows;
	});
}

}  // namespace idakit
