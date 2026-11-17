// The src model database (srcdb index's output; docs in Model.hpp): a thin sqlite3 wrapper and the schema.
#pragma once

#include <sqlite3.h>

#include <cstdint>
#include <stdexcept>
#include <string>

namespace srcdb
{

class Error : public std::runtime_error
{
public:
	using std::runtime_error::runtime_error;
};

class Statement
{
public:
	Statement(sqlite3* db, const char* sql);
	~Statement();
	Statement(const Statement&) = delete;
	Statement& operator=(const Statement&) = delete;

	// 1-based, as sqlite3_bind_*
	Statement& bind(int index, const std::string& text);
	Statement& bind(int index, int64_t value);
	Statement& bindNull(int index);
	Statement& bind(int index, double value);
	// true: a row is ready
	bool step();
	// step() until done, then reset() (an INSERT/UPDATE/DELETE)
	void run();
	void reset();
	int64_t integer(int column) const;
	std::string text(int column) const;

private:
	sqlite3* m_db;
	sqlite3_stmt* m_statement = nullptr;
};

class Store
{
public:
	// The model's format: a reader refuses another (srcdb index rebuilds the whole database).
	static constexpr int kSchemaVersion = 22;

	// create: a new, empty database at path (an existing file is replaced); else open one for reading
	Store(const std::string& path, bool create);
	~Store();
	Store(const Store&) = delete;
	Store& operator=(const Store&) = delete;

	void exec(const std::string& sql);
	sqlite3* handle() { return m_db; }
	int64_t changes() const;

private:
	sqlite3* m_db = nullptr;
};

}  // namespace srcdb
