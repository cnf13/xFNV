// An IDA database opened through idalib for the lifetime of the object.
//
// idalib holds one database per process and locks the .i64 to that process: open a private copy, never a database
// an IDA (GUI or another idalib process) has open. The license and settings come from $HOME/.idapro, the IDA
// installation from the libidalib.so the program is linked against (its rpath).
//
// IDA works on the database unpacked beside the .i64 (.id0, .id1, .id2, .nam, .til) and writes the .i64 only when it
// saves. Closing never saves: a tool that edits the database calls save() once, after its last change succeeded, so a
// run that fails (or throws) leaves the .i64 as it was. Closing deletes the unpacked files; those a killed process
// leaves behind are not read: the next open unpacks the .i64 over them.
#pragma once

#include <stdexcept>
#include <string>

namespace idakit
{

struct Error : std::runtime_error
{
	using std::runtime_error::runtime_error;
};

class Database
{
public:
	struct Options
	{
		bool hexrays = true;        // initialise the decompiler (Decompiler, CtreeDumper need it)
		bool autoAnalysis = false;  // let IDA's auto-analysis run on open
		bool messages = false;      // IDA's own console messages on stdout
	};

	// Throws Error when idalib cannot start, the database cannot be opened, or Hex-Rays is wanted and missing.
	explicit Database(const std::string& path, const Options& options);
	explicit Database(const std::string& path) : Database(path, Options()) {}
	~Database();

	Database(const Database&) = delete;
	Database& operator=(const Database&) = delete;

	const std::string& path() const { return m_path; }
	// Writes the database to its .i64, after a rollback snapshot where it is the canonical (RULES "The IDA database").
	// Throws Error where IDA cannot.
	void save();

private:
	std::string m_path;
	Options m_options;
};

}  // namespace idakit
