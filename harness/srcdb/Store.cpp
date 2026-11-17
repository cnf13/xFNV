#include "Store.hpp"

#include <cstdio>

namespace srcdb
{

namespace
{
// The model (Model.hpp describes it). Booleans are 0/1; type columns hold type keys; NULL: not applicable or not
// known. Items carry a digest of their row and child rows: a key two TUs write differently becomes a conflict row.
constexpr const char* kSchema = R"sql(
CREATE TABLE meta (name TEXT PRIMARY KEY, value TEXT);
CREATE TABLE tu (path TEXT PRIMARY KEY, ok INTEGER, errors TEXT, seconds REAL, peak_kb INTEGER);
CREATE TABLE type (key TEXT PRIMARY KEY, kind TEXT, name TEXT, of TEXT, count INTEGER, size INTEGER,
                   signed INTEGER, float INTEGER, cc TEXT, ret TEXT, variadic INTEGER, const INTEGER, class TEXT);
CREATE TABLE type_param (type TEXT, idx INTEGER, param TEXT, PRIMARY KEY (type, idx));
CREATE TABLE record (name TEXT PRIMARY KEY, file TEXT, line INTEGER, end_line INTEGER, tag TEXT, size INTEGER,
                     align INTEGER, data_size INTEGER, dynamic INTEGER, abstract INTEGER, trivial_copy INTEGER,
                     vfptr INTEGER, vbptr INTEGER, primary_base TEXT, template TEXT, external INTEGER, spelled TEXT,
                     digest INTEGER);
CREATE TABLE field (record TEXT, idx INTEGER, name TEXT, type TEXT, offset INTEGER, bits INTEGER,
                    PRIMARY KEY (record, idx));
CREATE TABLE base (record TEXT, idx INTEGER, type TEXT, offset INTEGER, virtual INTEGER, PRIMARY KEY (record, idx));
CREATE TABLE method (record TEXT, idx INTEGER, function TEXT, PRIMARY KEY (record, idx));
CREATE TABLE using_member (record TEXT, idx INTEGER, function TEXT, PRIMARY KEY (record, idx));
CREATE TABLE vtable (record TEXT, idx INTEGER, offset INTEGER, PRIMARY KEY (record, idx));
CREATE TABLE vtable_path (record TEXT, vtable INTEGER, idx INTEGER, class TEXT, PRIMARY KEY (record, vtable, idx));
CREATE TABLE slot (record TEXT, vtable INTEGER, idx INTEGER, kind TEXT, function TEXT, name TEXT, pure INTEGER,
                   this_adjust INTEGER, virtual_adjust INTEGER, PRIMARY KEY (record, vtable, idx));
CREATE TABLE pin (record TEXT, file TEXT, line INTEGER, size INTEGER, PRIMARY KEY (record, file, line));
CREATE TABLE enum (name TEXT PRIMARY KEY, file TEXT, line INTEGER, scoped INTEGER, underlying TEXT,
                   external INTEGER, digest INTEGER);
CREATE TABLE enumerator (enum TEXT, idx INTEGER, name TEXT, value INTEGER, PRIMARY KEY (enum, idx));
CREATE TABLE typedef (name TEXT PRIMARY KEY, file TEXT, line INTEGER, target TEXT, external INTEGER, digest INTEGER);
CREATE TABLE function (mangled TEXT PRIMARY KEY, name TEXT, file TEXT, line INTEGER, type TEXT, class TEXT,
                       this TEXT, static INTEGER, virtual INTEGER, pure INTEGER, const INTEGER, structor TEXT,
                       variadic INTEGER, empty INTEGER, access TEXT, noreturn INTEGER, provenance TEXT NOT NULL,
                       digest INTEGER);
CREATE INDEX function_name ON function (name);
CREATE TABLE param (function TEXT, idx INTEGER, name TEXT, PRIMARY KEY (function, idx));
CREATE TABLE default_args (function TEXT PRIMARY KEY, required INTEGER);
CREATE TABLE function_pattern (function TEXT PRIMARY KEY, pattern TEXT);
CREATE INDEX function_pattern_pattern ON function_pattern (pattern);
CREATE TABLE abi (function TEXT PRIMARY KEY, cc TEXT, ret TEXT, sret_after_this INTEGER, inalloca INTEGER,
                  stack_bytes INTEGER, digest INTEGER);
CREATE TABLE abi_arg (function TEXT, idx INTEGER, kind TEXT, inreg INTEGER, bytes INTEGER,
                      PRIMARY KEY (function, idx));
CREATE TABLE definition (mangled TEXT PRIMARY KEY, name TEXT, file TEXT, line INTEGER, end_line INTEGER,
                         instance INTEGER, emitted INTEGER NOT NULL, digest INTEGER);
CREATE TABLE definition_param (definition TEXT, idx INTEGER, name TEXT, PRIMARY KEY (definition, idx));
CREATE TABLE function_marker (function TEXT, marker TEXT, PRIMARY KEY (function, marker));
CREATE TABLE function_slot (function TEXT, slot TEXT, inferred INTEGER, PRIMARY KEY (function, slot));
CREATE TABLE function_use (function TEXT, file TEXT, line INTEGER, col INTEGER, call_line INTEGER, call_col INTEGER,
                           call_end_line INTEGER, call_end_col INTEGER, PRIMARY KEY (function, file, line, col));
CREATE TABLE field_marker (record TEXT, field TEXT, marker TEXT, PRIMARY KEY (record, field));
CREATE TABLE specialization (mangled TEXT, file TEXT, line INTEGER, end_line INTEGER, definition INTEGER,
                             inline INTEGER, PRIMARY KEY (mangled, file, line));
CREATE TABLE accessor (function TEXT PRIMARY KEY, kind TEXT, field TEXT, param INTEGER, digest INTEGER);
CREATE TABLE steps (function TEXT PRIMARY KEY, steps TEXT, digest INTEGER);
CREATE TABLE ref (file TEXT, line INTEGER, col INTEGER, kind TEXT, form TEXT, address INTEGER, target TEXT,
                  function TEXT, entry TEXT, type TEXT, local INTEGER, context TEXT,
                  PRIMARY KEY (file, line, col, kind, address));
CREATE INDEX ref_address ON ref (address);
CREATE TABLE ref_param (file TEXT, line INTEGER, col INTEGER, address INTEGER, idx INTEGER, name TEXT,
                        literal INTEGER, value INTEGER, PRIMARY KEY (file, line, col, address, idx));
CREATE TABLE ref_location (file TEXT, line INTEGER, col INTEGER, address INTEGER, idx INTEGER, location TEXT,
                           PRIMARY KEY (file, line, col, address, idx));
CREATE TABLE conflict (kind TEXT, key TEXT, file TEXT);
CREATE TABLE pdb_function (id INTEGER PRIMARY KEY, name TEXT, class TEXT, type TEXT, static INTEGER,
                           code_size INTEGER);
CREATE INDEX pdb_function_name ON pdb_function (name);
CREATE TABLE pdb_param (function INTEGER, idx INTEGER, name TEXT, PRIMARY KEY (function, idx));
CREATE TABLE pdb_local (function INTEGER, idx INTEGER, name TEXT, type TEXT, PRIMARY KEY (function, idx));
CREATE TABLE pdb_field (record TEXT, name TEXT, type TEXT, PRIMARY KEY (record, name, type));
CREATE TABLE pdb_global (name TEXT, type TEXT, PRIMARY KEY (name, type));
CREATE TABLE pdb_typed (kind TEXT, key TEXT, item TEXT, type TEXT, PRIMARY KEY (kind, key, item));
)sql";

}  // namespace

Statement::Statement(sqlite3* db, const char* sql) : m_db(db)
{
	if (sqlite3_prepare_v2(db, sql, -1, &m_statement, nullptr) != SQLITE_OK)
		throw Error(std::string("srcdb: sqlite: ") + sqlite3_errmsg(db) + " in " + sql);
}

Statement::~Statement()
{
	sqlite3_finalize(m_statement);
}

Statement& Statement::bind(int index, const std::string& text)
{
	sqlite3_bind_text(m_statement, index, text.data(), static_cast<int>(text.size()), SQLITE_TRANSIENT);
	return *this;
}

Statement& Statement::bind(int index, int64_t value)
{
	sqlite3_bind_int64(m_statement, index, value);
	return *this;
}

Statement& Statement::bind(int index, double value)
{
	sqlite3_bind_double(m_statement, index, value);
	return *this;
}

Statement& Statement::bindNull(int index)
{
	sqlite3_bind_null(m_statement, index);
	return *this;
}

bool Statement::step()
{
	int status = sqlite3_step(m_statement);
	if (status == SQLITE_ROW)
		return true;
	if (status != SQLITE_DONE)
		throw Error(std::string("srcdb: sqlite: ") + sqlite3_errmsg(m_db));
	return false;
}

void Statement::run()
{
	while (step())
	{
	}
	reset();
}

void Statement::reset()
{
	sqlite3_reset(m_statement);
	sqlite3_clear_bindings(m_statement);
}

int64_t Statement::integer(int column) const
{
	return sqlite3_column_int64(m_statement, column);
}

std::string Statement::text(int column) const
{
	const unsigned char* text = sqlite3_column_text(m_statement, column);
	return text ? std::string(reinterpret_cast<const char*>(text), sqlite3_column_bytes(m_statement, column))
				: std::string();
}

Store::Store(const std::string& path, bool create)
{
	if (create)
	{
		for (const char* suffix : {"", "-wal", "-shm"})
			std::remove((path + suffix).c_str());
	}
	int flags = create ? SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE : SQLITE_OPEN_READWRITE;
	if (sqlite3_open_v2(path.c_str(), &m_db, flags, nullptr) != SQLITE_OK)
		throw Error("srcdb: cannot open " + path + ": " + (m_db ? sqlite3_errmsg(m_db) : "out of memory"));
	// the index's workers write one database at once, each TU in one transaction
	sqlite3_busy_timeout(m_db, 600000);
	exec("PRAGMA journal_mode=WAL");
	exec("PRAGMA synchronous=NORMAL");
	if (create)
	{
		exec(kSchema);
		exec("INSERT INTO meta VALUES ('version', '" + std::to_string(kSchemaVersion) + "')");
		return;
	}
	Statement version(m_db, "SELECT value FROM meta WHERE name = 'version'");
	if (!version.step() || version.text(0) != std::to_string(kSchemaVersion))
		throw Error("srcdb: " + path + " is not a model of format " + std::to_string(kSchemaVersion) +
					" (rebuild it: srcdb index)");
}

Store::~Store()
{
	sqlite3_close_v2(m_db);
}

void Store::exec(const std::string& sql)
{
	char* message = nullptr;
	if (sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &message) != SQLITE_OK)
	{
		std::string text = message ? message : "error";
		sqlite3_free(message);
		throw Error("srcdb: sqlite: " + text);
	}
}

int64_t Store::changes() const
{
	return sqlite3_changes64(m_db);
}

}  // namespace srcdb
