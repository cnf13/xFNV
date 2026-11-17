#include "idakit/Database.hpp"

#include "idakit/Ida.hpp"

#include <algorithm>
#include <ctime>
#include <filesystem>
#include <vector>

namespace idakit
{

namespace
{

bool g_libraryReady = false;
bool g_databaseOpen = false;

// The canonical is env/ida/FalloutNV.exe.i64 of any checkout; its snapshots go to env/ida/snapshots/, the newest 5
// kept (the stamp sorts by name in time order).
void snapshotCanonical(const std::string& path)
{
	namespace fs = std::filesystem;
	std::error_code error;
	const fs::path real = fs::weakly_canonical(path, error);
	if (error || real.filename() != "FalloutNV.exe.i64" || real.parent_path().filename() != "ida" ||
		real.parent_path().parent_path().filename() != "env")
		return;
	const fs::path folder = real.parent_path() / "snapshots";
	char stamp[32];
	const std::time_t now = std::time(nullptr);
	std::strftime(stamp, sizeof(stamp), "%Y%m%d-%H%M%S", std::localtime(&now));
	fs::create_directories(folder, error);
	if (error || !fs::copy_file(real, folder / ("FalloutNV.exe." + std::string(stamp) + ".i64"),
			fs::copy_options::overwrite_existing, error) || error)
		throw Error("idakit: cannot snapshot " + path);
	std::vector<fs::path> snapshots;
	for (const fs::directory_entry& entry : fs::directory_iterator(folder, error))
		if (entry.is_regular_file() && entry.path().extension() == ".i64")
			snapshots.push_back(entry.path());
	std::sort(snapshots.begin(), snapshots.end());
	for (size_t i = 0; i + 5 < snapshots.size(); ++i)
		fs::remove(snapshots[i], error);
}

}  // namespace

Database::Database(const std::string& path, const Options& options) : m_path(path), m_options(options)
{
	if (g_databaseOpen)
		throw Error("idakit: a database is open already (idalib holds one per process)");
	if (!g_libraryReady)
	{
		if (init_library() != 0)
			throw Error("idakit: idalib did not start (the IDA installation or its license under $HOME/.idapro)");
		g_libraryReady = true;
	}
	enable_console_messages(options.messages);
	if (open_database(path.c_str(), options.autoAnalysis) != 0)
		throw Error("idakit: cannot open " + path + " (missing, or open in another IDA)");
	g_databaseOpen = true;
	if (options.hexrays && !init_hexrays_plugin())
	{
		close_database(false);
		g_databaseOpen = false;
		throw Error("idakit: the Hex-Rays decompiler is not available");
	}
}

Database::~Database()
{
	if (m_options.hexrays)
		term_hexrays_plugin();
	close_database(false);
	g_databaseOpen = false;
}

void Database::save()
{
	snapshotCanonical(m_path);
	if (!save_database(nullptr, 0))
		throw Error("idakit: cannot save " + m_path);
}

}  // namespace idakit
