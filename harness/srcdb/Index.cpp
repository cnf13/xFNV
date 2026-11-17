// srcdb index: the src model (Model.hpp) of the hybrid build, rebuilt whole into a new database.
//
//   srcdb index --root DIR --compile-commands FILE --out DB [--jobs N] [--scratch DIR] [--tus PREFIX]
//               [--collect PREFIX]... [--limit N]
//
// One worker process per TU (`srcdb index-tu`, at most --jobs at once): the all-headers TU (every .h/.hpp under the
// collected prefixes, written to --scratch, by default DB.tus beside the database: `env` is one directory for the
// main checkout and every worktree, so each index keeps its generated TUs with its own output) and the compile
// database's TUs under --tus (default src/). They write one database (DB.partial) that replaces DB when all are
// done. A line per TU on stdout: status, seconds, peak RSS.

#include "Clang.hpp"
#include "Model.hpp"
#include "Pdb.hpp"
#include "Store.hpp"

#include <clang/Tooling/JSONCompilationDatabase.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/raw_ostream.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <deque>
#include <functional>
#include <map>
#include <regex>
#include <set>
#include <unordered_map>
#include <spawn.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>

extern char** environ;

namespace srcdb
{

namespace
{

struct Options
{
	std::map<std::string, std::vector<std::string>> values;
	std::vector<std::string> flags;
	std::vector<std::string> rest;  // after --

	std::string get(const std::string& name, const std::string& fallback = "") const
	{
		auto found = values.find(name);
		return found == values.end() || found->second.empty() ? fallback : found->second.back();
	}

	bool flag(const std::string& name) const { return std::count(flags.begin(), flags.end(), name) > 0; }
};

Options parseOptions(int argc, char** argv, int first, const std::vector<std::string>& flagNames)
{
	Options options;
	for (int k = first; k < argc; ++k)
	{
		std::string word = argv[k];
		if (word == "--")
		{
			options.rest.assign(argv + k + 1, argv + argc);
			break;
		}
		if (word.rfind("--", 0) != 0)
			throw Error("srcdb: unexpected argument " + word);
		std::string name = word.substr(2);
		if (std::count(flagNames.begin(), flagNames.end(), name))
			options.flags.push_back(name);
		else if (k + 1 < argc)
			options.values[name].push_back(argv[++k]);
		else
			throw Error("srcdb: --" + name + " needs a value");
	}
	return options;
}

std::string realDirectory(const std::string& path)
{
	llvm::SmallString<256> real;
	if (llvm::sys::fs::real_path(path, real))
		throw Error("srcdb: no directory " + path);
	std::string text = real.str().str();
	return text.back() == '/' ? text : text + "/";
}

// every header under the collected prefixes, one #include each, in path order
std::string writeHeadersTu(const std::string& root, const std::vector<std::string>& collect, const std::string& scratch)
{
	std::vector<std::string> headers;
	for (const std::string& prefix : collect)
	{
		std::error_code error;
		for (llvm::sys::fs::recursive_directory_iterator it(root + prefix, error), end; it != end && !error;
			 it.increment(error))
		{
			llvm::StringRef path = it->path();
			if (path.ends_with(".h") || path.ends_with(".hpp"))
				headers.push_back(path.str());
		}
	}
	std::sort(headers.begin(), headers.end());
	llvm::sys::fs::create_directories(scratch);
	std::string file = scratch + "/allheaders.cpp";
	std::error_code error;
	llvm::raw_fd_ostream out(file, error);
	if (error)
		throw Error("srcdb: cannot write " + file);
	out << "// srcdb index: every src header in one TU (generated)\n";
	for (const std::string& header : headers)
		out << "#include \"" << header << "\"\n";
	return file;
}

// a type name without its blanks (clang's `A<B *, C<D>>` and the PDB's `A<B *,C<D> >` alike)
std::string compact(std::string name)
{
	name.erase(std::remove(name.begin(), name.end(), ' '), name.end());
	return name;
}

// The PDB's std:: is the exe's VC9 standard library, src's msvc9::: `std::vector<T,std::allocator<T> >` as
// `msvc9::vector<T,msvc9::allocator<T> >`.
std::string msvc9Named(const std::string& name)
{
	std::string out;
	for (size_t k = 0; k < name.size(); ++k)
	{
		char before = k ? name[k - 1] : ' ';
		bool word = !(std::isalnum(static_cast<unsigned char>(before)) || before == '_' || before == ':');
		if (word && name.compare(k, 5, "std::") == 0)
		{
			out += "msvc9::";
			k += 4;
		}
		else
			out += name[k];
	}
	return out;
}

// The PDB's template instantiations src does not have yet, under src's names (msvc9Named): those with no src record
// of their name, and none of the PDB's own characters no C++ name has (`` ` ``, `$`, `'`). Clang rejects those src
// cannot name (their errors are expected). Read from the model the src TUs wrote.
std::vector<std::string> newInstances(Store& store, const std::vector<std::string>& instances)
{
	std::set<std::string> records;
	for (Statement names(store.handle(), "SELECT name FROM record WHERE name LIKE '%<%'"); names.step();)
		records.insert(compact(names.text(0)));
	std::vector<std::string> out;
	for (const std::string& pdbName : instances)
		if (std::string instance = msvc9Named(pdbName);
			instance.find_first_of("`$'") == std::string::npos && !records.count(compact(instance)))
			out.push_back(instance);
	return out;
}

// the all-headers TU and a sizeof of each instantiation, which instantiates the class (not its members' bodies)
std::string writeInstancesTu(const std::string& headersTu, const std::vector<std::string>& instances,
							 const std::string& scratch)
{
	std::string file = scratch + "/pdbinstances.cpp";
	std::error_code error;
	llvm::raw_fd_ostream out(file, error);
	if (error)
		throw Error("srcdb: cannot write " + file);
	out << "// srcdb index: the PDB's template instantiations src lacks, each instantiated by its sizeof "
		   "(generated)\n#include \""
		<< headersTu << "\"\n\nnamespace\n{\n\nvoid pdbInstances()\n{\n";
	for (const std::string& instance : instances)
		out << "\t(void)sizeof(" << instance << ");\n";
	out << "}\n\n}  // namespace\n";
	return file;
}

struct Job
{
	std::string file;
	std::string directory;
	std::vector<std::string> command;
	bool headers = false;	 // the all-headers TU: its errors are reported, not fatal
	bool instances = false;	 // the PDB instances TU (indexed as the all-headers TU is): its errors are expected
};

std::string selfPath()
{
	llvm::SmallString<256> real;
	if (llvm::sys::fs::real_path("/proc/self/exe", real))
		throw Error("srcdb: cannot find its own executable");
	return real.str().str();
}

pid_t spawn(const std::vector<std::string>& args)
{
	std::vector<char*> argv;
	for (const std::string& arg : args)
		argv.push_back(const_cast<char*>(arg.c_str()));
	argv.push_back(nullptr);
	pid_t pid = 0;
	if (posix_spawn(&pid, argv[0], nullptr, nullptr, argv.data(), environ) != 0)
		throw Error("srcdb: cannot start " + args[0]);
	return pid;
}

std::string relative(const std::string& root, const std::string& path)
{
	return llvm::StringRef(path).starts_with(root) ? path.substr(root.size()) : path;
}

std::string absolute(const std::string& path)
{
	llvm::SmallString<256> text(path);
	llvm::sys::fs::make_absolute(text);
	return text.str().str();
}

struct RunResult
{
	size_t total = 0, crashed = 0, failed = 0;  // failed: src TUs with errors
	bool headersClean = true;
	double seconds = 0;
};

// One `srcdb index-tu` process per job, at most `jobs` at once; a line per TU as each ends.
RunResult runWorkers(std::deque<Job>& queue, const std::string& db, const std::string& root,
					 const std::vector<std::string>& collect, unsigned jobs)
{
	RunResult result;
	result.total = queue.size();
	Store store(db, false);
	Statement measured(store.handle(), "UPDATE tu SET seconds = ?, peak_kb = ? WHERE path = ?");
	Statement crashed(store.handle(), "INSERT OR IGNORE INTO tu VALUES (?, 0, ?, NULL, NULL)");
	Statement ok(store.handle(), "SELECT ok FROM tu WHERE path = ?");
	std::string self = selfPath();
	std::map<pid_t, std::pair<Job, std::chrono::steady_clock::time_point>> running;
	auto started = std::chrono::steady_clock::now();
	size_t done = 0;
	while (!queue.empty() || !running.empty())
	{
		while (!queue.empty() && running.size() < std::max(1u, jobs))
		{
			Job job = std::move(queue.front());
			queue.pop_front();
			std::vector<std::string> args{self, "index-tu", "--db", db, "--root", root};
			for (const std::string& prefix : collect)
				args.insert(args.end(), {"--collect", prefix});
			if (job.headers || job.instances)
				args.push_back("--headers");
			args.insert(args.end(), {"--directory", job.directory, "--file", job.file, "--"});
			args.insert(args.end(), job.command.begin(), job.command.end());
			pid_t pid = spawn(args);
			running.emplace(pid, std::make_pair(std::move(job), std::chrono::steady_clock::now()));
		}
		int status = 0;
		rusage usage{};
		pid_t pid = wait4(-1, &status, 0, &usage);
		if (pid < 0)
			throw Error("srcdb: wait4 failed");
		auto found = running.find(pid);
		if (found == running.end())
			continue;
		Job job = std::move(found->second.first);
		double seconds =
			std::chrono::duration<double>(std::chrono::steady_clock::now() - found->second.second).count();
		running.erase(found);
		std::string path = relative(root, job.file);
		bool exited = WIFEXITED(status) && WEXITSTATUS(status) == 0;
		if (!exited)
		{
			std::string why = WIFSIGNALED(status) ? "killed by signal " + std::to_string(WTERMSIG(status))
												  : "exit " + std::to_string(WEXITSTATUS(status));
			crashed.bind(1, path).bind(2, "srcdb index-tu: " + why).run();
			++result.crashed;
		}
		measured.bind(1, seconds).bind(2, static_cast<int64_t>(usage.ru_maxrss)).bind(3, path).run();
		ok.bind(1, path);
		bool clean = exited && ok.step() && ok.integer(0);
		ok.reset();
		if (job.headers)
			result.headersClean = clean;
		else if (exited && !clean && !job.instances)
			++result.failed;
		std::printf("[%zu/%zu] %s %s %.1fs %ldMB\n", ++done, result.total, clean ? "ok " : "ERR", path.c_str(),
					seconds, usage.ru_maxrss / 1024);
		std::fflush(stdout);
	}
	result.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
	return result;
}

// function.provenance from the TUs' function_marker rows and the PDB, first match winning: a declaration or the
// definition with an `// Inferred type` comment whose clauses, over all of them, name every slot (`inferred`); one
// with a Havok library symbol's signature line (`library`); one marked `guess`; a qualified name the PDB has (`pdb`);
// else `src` (Model.hpp). function_slot keeps each slot and whether a clause names it. The marker rows are dropped
// after PdbTyped has read the `pdb` ones; a line reports the declarations a macro's body spells (their comment lines
// unseen, so only their parameter names or the PDB can tell).
void decideProvenance(Store& store)
{
	store.exec("INSERT OR IGNORE INTO function_slot (function, slot, inferred) SELECT s.function, substr(s.marker, 6), "
			   "EXISTS (SELECT 1 FROM function_marker i WHERE i.function = s.function AND "
			   "i.marker = 'inferred ' || substr(s.marker, 6)) FROM function_marker s WHERE s.marker LIKE 'slot %'");
	store.exec("UPDATE function SET provenance = CASE "
			   "WHEN mangled IN (SELECT function FROM function_marker WHERE marker = 'inferred') AND NOT EXISTS "
			   "(SELECT 1 FROM function_slot WHERE function = mangled AND inferred = 0) THEN 'inferred' "
			   "WHEN mangled IN (SELECT function FROM function_marker WHERE marker = 'library') THEN 'library' "
			   "WHEN mangled IN (SELECT function FROM function_marker WHERE marker = 'guess') THEN 'guess' "
			   "WHEN name IN (SELECT name FROM pdb_function) THEN 'pdb' ELSE 'src' END");
	{
		Statement macros(store.handle(), "SELECT provenance, count(*) FROM function WHERE mangled IN "
										 "(SELECT function FROM function_marker WHERE marker = 'macro') GROUP BY 1");
		std::string counts;
		while (macros.step())
			counts += " " + macros.text(0) + " " + std::to_string(macros.integer(1));
		std::printf("srcdb index: declarations a macro spells (comment lines unseen):%s\n",
					counts.empty() ? " none" : counts.c_str());
	}
	Statement provenance(store.handle(), "SELECT provenance, count(*) FROM function GROUP BY 1 ORDER BY 1");
	std::string line;
	while (provenance.step())
		line += " " + provenance.text(0) + " " + std::to_string(provenance.integer(1));
	std::printf("srcdb index: function provenance:%s\n", line.c_str());
}

// pdb_typed (Model.hpp): each src declaration whose type is the one the PDB declares for it, with the PDB's type: a
// src function's parameters and return, a record's fields, a global or static member; and the PDB's locals of a src
// function. A src name is the PDB's spelled alike once spaces are dropped, the PDB's std:: is src's msvc9::
// (newInstances' match) and a `T const` in it is `const T`. A src function (provenance pdb or src: not a guess or
// inferred) is paired with one PDB function of its name (`pair`): of those with as many parameters, or with one fewer
// that return a record src's first parameter points to (src's explicit hidden result pointer: `hidden`, src's
// parameter k + 1 the PDB's k), the one whose slots agree with the most of src's, where that is one type and no other
// src function of the name pairs with it on as many; its agreeing slots are its items. A src function left unpaired
// has an item where every PDB function of its name with as many parameters declares it alike. A field is every PDB
// record of its record's name (all of them declaring a member of the field's name), a global every PDB global of its
// name. Types compare as declared: typedefs resolved, a builtin integer by its size and signedness (the PDB's `long`
// is src's `int32_t`), char, wchar_t and bool as themselves, a function type by its convention, return and
// parameters, cv kept but a declaration's own. A `// PDB:` comment above a function's declaration (function_marker
// `pdb <slot> <line>`) is `recorded` for its slots; one above a field (field_marker) is `field_recorded` for it.
class PdbTyped
{
public:
	explicit PdbTyped(Store& store) : m_store(store)
	{
		for (Statement q(store.handle(), "SELECT key, kind, name, of, count, size, signed, float, ret, cc, variadic "
										 "FROM type");
			 q.step();)
			m_nodes[q.text(0)] = Node{q.text(1), q.text(2), q.text(3), q.text(8), q.text(9), q.integer(4),
									  q.integer(5), q.integer(6), q.integer(7), q.integer(10)};
		for (Statement q(store.handle(), "SELECT type, param FROM type_param ORDER BY type, idx"); q.step();)
			m_nodes[q.text(0)].params.push_back(q.text(1));
		for (Statement q(store.handle(), "SELECT name, target FROM typedef"); q.step();)
			m_typedefs[q.text(0)] = q.text(1);
	}

	void run()
	{
		Statement insert(m_store.handle(), "INSERT OR IGNORE INTO pdb_typed (kind, key, item, type) VALUES (?, ?, ?, ?)");
		std::map<std::string, int64_t> counts;
		auto add = [&](const char* kind, const std::string& key, const std::string& item, const std::string& type) {
			insert.bind(1, std::string(kind)).bind(2, key).bind(3, item).bind(4, type).run();
			++counts[kind];
		};
		m_store.exec("BEGIN");
		functions(add);
		fields(add);
		globals(add);
		m_store.exec("COMMIT");
		std::string line;
		for (const auto& [kind, count] : counts)
			line += " " + kind + " " + std::to_string(count);
		std::printf("srcdb index: declarations the PDB types alike:%s\n", line.empty() ? " none" : line.c_str());
	}

private:
	struct Node
	{
		std::string kind, name, of, ret, cc;
		int64_t count = 0, size = 0, isSigned = 0, isFloat = 0, variadic = 0;
		std::vector<std::string> params;
	};
	using Add = std::function<void(const char*, const std::string&, const std::string&, const std::string&)>;

	// the name as compared: msvc9Named, `T const` as `const T` (the PDB's `AStarNode<NavMeshInfo const *,...>`,
	// clang's `AStarNode<const NavMeshInfo *, ...>`), no elaborated `enum`/`struct`/`class`/`union` (the PDB's
	// `hkEnum<enum X::Type,unsigned char>`), `long` as the `int` it is on the exe's target and in src's fixed widths
	// (RULES: long -> int32_t; the PDB's `hkArray<unsigned long,...>`, clang's `hkArray<unsigned int, ...>`), and
	// `__int64` as `long long`; blanks dropped
	static std::string named(const std::string& name)
	{
		static const std::regex eastConst(R"(([A-Za-z_][A-Za-z0-9_:]*) const\b)");
		static const std::regex elaborated(R"(\b(enum|struct|class|union) +)");
		static const std::regex longLong(R"(\blong +long\b|\b__int64\b)"), longDouble(R"(\blong +double\b)");
		static const std::regex plainLong(R"(\blong\b)"), longLongMark("\x01"), longDoubleMark("\x02");
		std::string out = msvc9Named(name), before;
		while (before != out)
			before = out, out = std::regex_replace(out, eastConst, "const $1");
		out = std::regex_replace(out, elaborated, "");
		if (out.find("long") != std::string::npos || out.find("__int64") != std::string::npos)
		{
			out = std::regex_replace(out, longLong, "\x01");
			out = std::regex_replace(out, longDouble, "\x02");
			out = std::regex_replace(out, plainLong, "int");
			out = std::regex_replace(out, longLongMark, "long long");
			out = std::regex_replace(out, longDoubleMark, "long double");
		}
		return compact(out);
	}

	// the key as compared (above), cv kept
	std::string comparable(const std::string& key, int depth = 0)
	{
		if (auto found = m_comparable.find(key); found != m_comparable.end())
			return found->second;
		auto node = m_nodes.find(key);
		std::string out = key;
		if (node != m_nodes.end() && depth < 64)
		{
			static const std::set<std::string> integers = {
				"signed char", "unsigned char", "short", "unsigned short", "int", "unsigned int", "long",
				"unsigned long", "long long", "unsigned long long"};
			const Node& n = node->second;
			if (n.kind == "typedef")
			{
				auto target = m_typedefs.find(n.name);
				if (target != m_typedefs.end())
					out = comparable(target->second, depth + 1);
			}
			else if (n.kind == "builtin" && integers.count(n.name) && n.size > 0)
				out = "int" + std::to_string(n.size) + (n.isSigned ? "s" : "u");
			else if (n.kind == "ptr" || n.kind == "lref" || n.kind == "rref" || n.kind == "const" ||
					 n.kind == "volatile")
				out = n.kind + "(" + comparable(n.of, depth + 1) + ")";
			else if (n.kind == "array")
				out = "array(" + comparable(n.of, depth + 1) + "," + std::to_string(n.count) + ")";
			else if (n.kind == "record" || n.kind == "enum")
				out = n.kind + ":" + named(n.name);
			else if (n.kind == "fn")
			{
				out = "fn(" + n.cc + ";" + comparable(n.ret, depth + 1) + ";";
				for (const std::string& param : n.params)
					out += comparable(param, depth + 1) + ",";
				out += std::string(n.variadic ? "...)" : ")");
			}
		}
		m_comparable.emplace(key, out);
		return out;
	}

	// as a declaration's type: without its own cv
	std::string declared(const std::string& key)
	{
		std::string out = comparable(key);
		while ((out.starts_with("const(") || out.starts_with("volatile(")) && out.ends_with(")"))
			out = out.substr(out.find('(') + 1, out.size() - out.find('(') - 2);
		return out;
	}

	const Node* fn(const std::string& key) const
	{
		auto found = m_nodes.find(key);
		return found != m_nodes.end() && found->second.kind == "fn" ? &found->second : nullptr;
	}

	struct Pdb
	{
		std::string key;  // its fn type
		const Node* type;
		std::string cls;  // a member's class
		bool member = false;  // not static
		std::vector<std::pair<std::string, std::string>> locals;
	};
	struct Src
	{
		std::string mangled, cls;
		const Node* type;
		bool member = false;  // has a this
	};
	// a src function's pairing (functions(), above): the PDB function, src's parameters it skips (1: src's hidden
	// result pointer) and the slots that agree
	struct Choice
	{
		const Pdb* pdb = nullptr;
		size_t shift = 0;
		int score = -1;
	};

	// src's first parameter points to the record the PDB function returns
	bool hiddenResult(const Node& src, const Node& pdb)
	{
		if (src.params.empty() || !comparable(pdb.ret).starts_with("record:"))
			return false;
		auto first = m_nodes.find(src.params[0]);
		return first != m_nodes.end() && first->second.kind == "ptr" &&
			   declared(first->second.of) == declared(pdb.ret);
	}

	Choice choose(const Src& src, const std::vector<const Pdb*>& pdbs)
	{
		std::vector<Choice> scored;
		for (const Pdb* pdb : pdbs)
		{
			Choice c{pdb, 0, 0};
			if (pdb->type->params.size() + 1 == src.type->params.size() && hiddenResult(*src.type, *pdb->type))
				c.shift = 1;
			else if (pdb->type->params.size() != src.type->params.size())
				continue;
			for (size_t k = c.shift; k < src.type->params.size(); ++k)
				c.score += declared(src.type->params[k]) == declared(pdb->type->params[k - c.shift]);
			if (!c.shift)
				c.score += declared(src.type->ret) == declared(pdb->type->ret);
			scored.push_back(c);
		}
		Choice best;
		for (const Choice& c : scored)
			if (c.score > best.score)
				best = c;
		// the best is one type: every other candidate scoring as well is the same type, spelled otherwise
		for (const Choice& c : scored)
			if (c.score == best.score && (c.shift != best.shift || comparable(c.pdb->key) != comparable(best.pdb->key)))
				return Choice{};
		return best;
	}

	// a local every candidate declares, each declaration of its name alike
	void locals(const Add& add, const std::string& mangled, const std::vector<const Pdb*>& candidates)
	{
		std::set<std::string> seen;
		for (const auto& [name, local] : candidates[0]->locals)
		{
			if (!seen.insert(name).second)
				continue;
			bool same = true;
			for (const Pdb* pdb : candidates)
			{
				bool declares = false;
				for (const auto& [other, otherType] : pdb->locals)
					if (other == name)
					{
						declares = true;
						same = same && declared(otherType) == declared(local);
					}
				same = same && declares;
			}
			if (same)
				add("local", mangled, name, local);
		}
	}

	void functions(const Add& add)
	{
		std::map<int64_t, Pdb> byId;
		std::map<std::string, std::vector<const Pdb*>> byName;
		for (Statement q(m_store.handle(), "SELECT id, name, type, class, static FROM pdb_function"); q.step();)
			if (const Node* type = fn(q.text(2)))
			{
				Pdb& pdb = byId[q.integer(0)];
				pdb.key = q.text(2);
				pdb.type = type;
				pdb.cls = q.text(3);
				pdb.member = !pdb.cls.empty() && !q.integer(4);
				byName[named(q.text(1))].push_back(&pdb);
			}
		for (Statement q(m_store.handle(), "SELECT function, name, type FROM pdb_local ORDER BY function, idx");
			 q.step();)
			if (auto found = byId.find(q.integer(0)); found != byId.end() && q.text(1) != "this")
				found->second.locals.emplace_back(q.text(1), q.text(2));
		// the `// PDB:` comments' slots: marker `pdb <slot> <line>`
		std::map<std::string, std::vector<std::pair<std::string, std::string>>> recorded;
		for (Statement q(m_store.handle(), "SELECT function, marker FROM function_marker WHERE marker LIKE 'pdb %'");
			 q.step();)
		{
			std::string marker = q.text(1);
			size_t space = marker.find(' ', 4);
			if (space != std::string::npos)
				recorded[q.text(0)].emplace_back(marker.substr(4, space - 4), marker.substr(space + 1));
		}
		std::map<std::string, std::vector<Src>> srcByName;
		for (Statement q(m_store.handle(), "SELECT mangled, name, type, class, this FROM function WHERE provenance IN "
										   "('pdb', 'src')");
			 q.step();)
			if (const Node* type = fn(q.text(2)))
				srcByName[named(q.text(1))].push_back(Src{q.text(0), q.text(3), type, !q.text(4).empty()});
		for (const auto& [name, srcs] : srcByName)
		{
			auto found = byName.find(name);
			std::vector<Choice> choices(srcs.size());
			if (found != byName.end())
				for (size_t i = 0; i < srcs.size(); ++i)
					choices[i] = choose(srcs[i], found->second);
			// no other src function of the name pairs with the same PDB function on as many slots
			std::vector<Choice> paired = choices;
			for (size_t i = 0; i < srcs.size(); ++i)
				for (size_t j = 0; j < srcs.size() && paired[i].pdb; ++j)
					if (j != i && choices[j].pdb == choices[i].pdb && choices[j].score >= choices[i].score)
						paired[i] = Choice{};
			for (size_t i = 0; i < srcs.size(); ++i)
			{
				const Src& src = srcs[i];
				const Node* type = src.type;
				if (const Pdb* pdb = paired[i].pdb)
				{
					size_t shift = paired[i].shift;
					add("pair", src.mangled, shift ? "hidden" : "", pdb->key);
					for (size_t k = shift; k < type->params.size(); ++k)
						if (declared(type->params[k]) == declared(pdb->type->params[k - shift]))
							add("param", src.mangled, std::to_string(k), pdb->type->params[k - shift]);
					if (!shift && declared(type->ret) == declared(pdb->type->ret))
						add("param", src.mangled, "ret", pdb->type->ret);
					// a member's this: of the class the PDB's member belongs to (item `this`, the record's key)
					if (src.member && pdb->member && named(pdb->cls) == named(src.cls))
						add("param", src.mangled, "this", "record:" + pdb->cls);
					locals(add, src.mangled, {pdb});
				}
				else if (found != byName.end())
				{
					std::vector<const Pdb*> candidates;
					for (const Pdb* pdb : found->second)
						if (pdb->type->params.size() == type->params.size())
							candidates.push_back(pdb);
					if (candidates.empty())
						continue;
					auto alike = [&](auto pdbType, const std::string& own) {
						for (const Pdb* pdb : candidates)
							if (declared(pdbType(*pdb)) != declared(own))
								return false;
						return true;
					};
					for (size_t k = 0; k < type->params.size(); ++k)
						if (alike([k](const Pdb& pdb) { return pdb.type->params[k]; }, type->params[k]))
							add("param", src.mangled, std::to_string(k), candidates[0]->type->params[k]);
					if (alike([](const Pdb& pdb) { return pdb.type->ret; }, type->ret))
						add("param", src.mangled, "ret", candidates[0]->type->ret);
					bool member = src.member;
					for (const Pdb* pdb : candidates)
						member = member && pdb->member && named(pdb->cls) == named(src.cls);
					if (member)
						add("param", src.mangled, "this", "record:" + candidates[0]->cls);
					locals(add, src.mangled, candidates);
				}
			}
			// the slots a `// PDB:` comment records: `*` every slot, `ret` the return (and src's hidden result
			// pointer where the pairing has one)
			for (size_t i = 0; i < srcs.size(); ++i)
			{
				auto marks = recorded.find(srcs[i].mangled);
				if (marks == recorded.end())
					continue;
				for (const auto& [slot, line] : marks->second)
				{
					std::vector<std::string> slots{slot};
					if (slot == "*")
					{
						slots = {"ret"};
						if (srcs[i].member)
							slots.push_back("this");
						for (size_t k = 0; k < srcs[i].type->params.size(); ++k)
							slots.push_back(std::to_string(k));
					}
					else if (slot == "ret" && paired[i].pdb && paired[i].shift)
						slots.push_back("0");
					for (const std::string& s : slots)
						add("recorded", srcs[i].mangled, s, line);
				}
			}
		}
	}

	void fields(const Add& add)
	{
		std::map<std::pair<std::string, std::string>, std::vector<std::string>> pdb;
		for (Statement q(m_store.handle(), "SELECT record, name, type FROM pdb_field"); q.step();)
			pdb[{named(q.text(0)), q.text(1)}].push_back(q.text(2));
		Statement q(m_store.handle(), "SELECT record, name, type FROM field WHERE bits IS NULL AND name != ''");
		while (q.step())
		{
			auto found = pdb.find({named(q.text(0)), q.text(1)});
			if (found == pdb.end())
				continue;
			std::string src = declared(q.text(2));
			bool same = true;
			for (const std::string& type : found->second)
				same = same && declared(type) == src;
			if (same)
				add("field", q.text(0), q.text(1), found->second[0]);
		}
		// the `// PDB:` comments above fields (Model.cpp's field_marker rows): the comment line
		for (Statement marks(m_store.handle(), "SELECT record, field, marker FROM field_marker"); marks.step();)
			add("field_recorded", marks.text(0), marks.text(1), marks.text(2));
	}

	void globals(const Add& add)
	{
		std::map<std::string, std::vector<std::string>> pdb, src;
		for (Statement q(m_store.handle(), "SELECT name, type FROM pdb_global"); q.step();)
			pdb[named(q.text(0))].push_back(q.text(1));
		for (Statement q(m_store.handle(), "SELECT DISTINCT target, type FROM ref WHERE kind IN ('global', "
										   "'static_member') AND target IS NOT NULL AND type IS NOT NULL");
			 q.step();)
			src[q.text(0)].push_back(q.text(1));
		for (const auto& [target, types] : src)
		{
			auto found = pdb.find(named(target));
			if (found == pdb.end())
				continue;
			bool same = true;
			for (const std::string& type : found->second)
				for (const std::string& own : types)
					same = same && declared(type) == declared(own);
			if (same)
				add("global", target, "", found->second[0]);
		}
	}

	Store& m_store;
	std::unordered_map<std::string, Node> m_nodes;
	std::unordered_map<std::string, std::string> m_typedefs;
	std::unordered_map<std::string, std::string> m_comparable;
};

}  // namespace

int runIndex(int argc, char** argv)
{
	Options options = parseOptions(argc, argv, 2, {});
	std::string root = realDirectory(options.get("root", "."));
	// absolute: the workers run in the compile's directory
	std::string out = absolute(options.get("out", root + "env/cache/srcmodel.sqlite"));
	std::string scratch = absolute(options.get("scratch", out + ".tus"));
	std::string compileCommands = options.get("compile-commands", root + "build/hybrid/compile_commands.json");
	std::string tus = options.get("tus", "src/");
	std::vector<std::string> collect = options.values.count("collect") ? options.values["collect"]
																	   : std::vector<std::string>{"src/", "gen/"};
	unsigned jobs = static_cast<unsigned>(std::stoul(options.get("jobs", "4")));
	size_t limit = static_cast<size_t>(std::stoull(options.get("limit", "0")));
	std::string pdbPath = options.get("pdb", root + "env/cache/Fallout_Debug.pdb");

	std::string error;
	auto database = clang::tooling::JSONCompilationDatabase::loadFromFile(
		compileCommands, error, clang::tooling::JSONCommandLineSyntax::AutoDetect);
	if (!database)
		throw Error("srcdb: " + error);
	std::deque<Job> queue;
	for (const clang::tooling::CompileCommand& command : database->getAllCompileCommands())
	{
		llvm::SmallString<256> real;
		std::string file = llvm::sys::fs::real_path(command.Filename, real) ? command.Filename : real.str().str();
		if (llvm::StringRef(relative(root, file)).starts_with(tus))
			queue.push_back({file, command.Directory, command.CommandLine, false});
	}
	std::sort(queue.begin(), queue.end(), [](const Job& a, const Job& b) { return a.file < b.file; });
	if (queue.empty())
		throw Error("srcdb: no TU under " + tus + " in " + compileCommands);
	if (limit && queue.size() > limit)
		queue.resize(limit);
	// the all-headers TU parses with a src TU's flags: its command, its own source swapped out
	Job headers = queue.front();
	headers.headers = true;
	std::vector<std::string> command;
	for (const std::string& word : headers.command)
		if (word != headers.file && llvm::sys::path::filename(word) != llvm::sys::path::filename(headers.file))
			command.push_back(word);
	headers.command = command;
	headers.file = writeHeadersTu(root, collect, scratch);
	queue.push_front(headers);

	std::string partial = out + ".partial";
	{
		Store create(partial, true);
	}
	RunResult result = runWorkers(queue, partial, root, collect, jobs);
	// the PDB's template instantiations src lacks, once the src TUs' records show what it has: one more TU, read as the
	// all-headers TU (its errors are the instantiations that do not compile in src)
	if (llvm::sys::fs::exists(pdbPath))
	{
		std::vector<std::string> instances;
		{
			Store store(partial, false);
			instances = newInstances(store, pdbTemplateInstances(pdbPath));
		}
		Job job = headers;
		job.headers = false;
		job.instances = true;
		job.file = writeInstancesTu(headers.file, instances, scratch);
		std::deque<Job> one{job};
		RunResult run = runWorkers(one, partial, root, collect, jobs);
		result.crashed += run.crashed;
		result.seconds += run.seconds;
		std::printf("srcdb index: %zu PDB template instantiations src lacks, sized in %s\n", instances.size(),
					relative(root, job.file).c_str());
	}
	{
		Store store(partial, false);
		std::printf("srcdb index: %zu TUs in %.0fs: %zu crashed, %zu with errors (%s)\n", result.total,
					result.seconds, result.crashed, result.failed, result.headersClean ? "headers clean"
																					   : "the headers TU had errors");
		if (llvm::sys::fs::exists(pdbPath))
			std::printf("srcdb index: %zu PDB functions from %s\n", indexPdb(store, pdbPath), pdbPath.c_str());
		else
			std::printf("srcdb index: no PDB at %s (harness/tools/pdb_repack.py makes it): no pdb_function rows\n",
						pdbPath.c_str());
		decideProvenance(store);
		PdbTyped(store).run();
		store.exec("DROP TABLE function_marker");
		store.exec("DROP TABLE field_marker");
		for (const char* table : {"record", "field", "enum", "typedef", "function", "abi", "definition", "ref", "type",
								  "function_use", "pdb_function", "pdb_local", "pdb_field", "pdb_global", "pdb_typed",
								  "conflict"})
		{
			Statement count(store.handle(), (std::string("SELECT count(*) FROM ") + table).c_str());
			count.step();
			std::printf("  %s %lld\n", table, static_cast<long long>(count.integer(0)));
		}
		// one file, no WAL beside it: readers open the model read-only
		store.exec("PRAGMA journal_mode=DELETE");
	}
	// a run where a worker crashed or a src TU failed keeps the old model (the all-headers TU's errors are src's
	// duplicate class definitions: reported, not fatal)
	if (result.crashed || result.failed)
	{
		std::printf("srcdb index: %s not replaced; this run's model is %s\n", out.c_str(), partial.c_str());
		return 1;
	}
	if (std::rename(partial.c_str(), out.c_str()) != 0)
		throw Error("srcdb: cannot move " + partial + " to " + out);
	return 0;
}

int runIndexTu(int argc, char** argv)
{
	Options options = parseOptions(argc, argv, 2, {"headers"});
	IndexScope scope;
	scope.root = realDirectory(options.get("root", "."));
	scope.collect = options.values.count("collect") ? options.values["collect"]
													: std::vector<std::string>{"src/", "gen/"};
	scope.headers = options.flag("headers");
	std::string file = options.get("file");
	std::string directory = options.get("directory");
	if (file.empty() || options.rest.empty())
		throw Error("srcdb index-tu: needs --file and the compile command after --");
	if (!directory.empty() && chdir(directory.c_str()) != 0)
		throw Error("srcdb index-tu: cannot enter " + directory);
	initializeTargets();
	Store store(options.get("db"), false);
	TuOutcome outcome;
	std::unique_ptr<clang::DiagnosticConsumer> diagnostics = makeIndexDiagnostics(outcome);
	clang::DiagnosticOptions diagnosticOptions;
	std::vector<std::string> args = syntaxOnlyCommand(options.rest, file);
	// no carets: CompilerInstance then prints no "N warnings generated" summary either; no error limit: clang stops a
	// TU at its 20th error otherwise (the instances TU has one per instantiation src cannot compile)
	args.insert(args.end() - 2, "/clang:-fno-caret-diagnostics");
	args.insert(args.end() - 2, "/clang:-ferror-limit=0");
	bool ran = runParse(args, makeIndexAction(store, scope), *diagnostics, diagnosticOptions);
	if (!ran && outcome.errors == 0)
		outcome.errors = 1, outcome.errorText = "srcdb: the clang invocation failed\n";
	Statement row(store.handle(), "INSERT OR REPLACE INTO tu VALUES (?, ?, ?, NULL, NULL)");
	row.bind(1, relative(scope.root, file)).bind(2, int64_t(outcome.errors == 0)).bind(3, outcome.errorText).run();
	return 0;
}

}  // namespace srcdb
