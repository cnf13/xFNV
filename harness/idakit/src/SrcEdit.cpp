#include "idakit/SrcEdit.hpp"

#include "IdaSupport.hpp"

#include "idakit/Address.hpp"
#include "idakit/Database.hpp"
#include "idakit/SrcTypes.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>
#include <string_view>

namespace idakit
{

namespace
{

// A text's lines, without their newlines (a final newline ends the last line rather than starting an empty one).
std::vector<std::string> splitLines(const std::string& text)
{
	std::vector<std::string> out;
	size_t start = 0;
	while (start < text.size())
	{
		size_t end = text.find('\n', start);
		if (end == std::string::npos)
			end = text.size();
		out.push_back(text.substr(start, end - start));
		start = end + 1;
	}
	return out;
}

// The line without its leading blanks.
std::string_view trimmed(const std::string& line)
{
	size_t start = line.find_first_not_of(" \t");
	return start == std::string::npos ? std::string_view() : std::string_view(line).substr(start);
}

// Moves the `#include` lines outside any preprocessor block that come after the file's leading lines (blank lines,
// comments, `#pragma once` and includes) up to the end of those leading lines, dropping repeats.
void hoistIncludes(std::vector<std::string>& lines)
{
	auto isInclude = [](std::string_view line) { return line.starts_with("#include"); };
	size_t head = 0, lead = 0;
	for (; lead < lines.size(); ++lead)
	{
		std::string_view line = trimmed(lines[lead]);
		if (line.starts_with("#pragma once") || isInclude(line))
			head = lead + 1;
		else if (!line.empty() && !line.starts_with("//"))
			break;
	}
	std::vector<std::string> moved;
	int depth = 0;
	for (size_t k = lead; k < lines.size();)
	{
		std::string_view line = trimmed(lines[k]);
		if (line.starts_with("#if"))
			++depth;
		else if (line.starts_with("#endif"))
			--depth;
		if (depth == 0 && isInclude(line))
		{
			bool known = std::any_of(lines.begin(), lines.begin() + head,
									 [&](const std::string& l) { return trimmed(l) == line; }) ||
						 std::find(moved.begin(), moved.end(), std::string(line)) != moved.end();
			if (!known)
				moved.emplace_back(line);
			lines.erase(lines.begin() + k);
			// the blank line the include stood on its own between goes with it
			if (k < lines.size() && k > 0 && trimmed(lines[k]).empty() && trimmed(lines[k - 1]).empty())
				lines.erase(lines.begin() + k);
			continue;
		}
		++k;
	}
	lines.insert(lines.begin() + head, moved.begin(), moved.end());
	// the includes stay separated from the code that follows them
	size_t after = head + moved.size();
	if (!moved.empty() && after < lines.size() && !trimmed(lines[after]).empty())
		lines.insert(lines.begin() + after, std::string());
}

}  // namespace

LineRange definitionSpan(const std::vector<std::string>& lines, const SrcModel::Definition& d)
{
	// a model indexed from another tree (or before the file changed) may name lines the file does not have
	if (d.line < 1 || d.endLine < d.line || d.endLine > static_cast<int64_t>(lines.size()))
		throw Error("model and file disagree: " + d.file + ":" + std::to_string(d.line) + "-" +
					std::to_string(d.endLine) + " of its " + std::to_string(lines.size()) + " lines");
	int64_t first = d.line;
	auto continues = [](const std::string& line) {
		size_t start = line.find_first_not_of(" \t"), end = line.find_last_not_of(" \t");
		return start != std::string::npos && line.compare(start, 2, "//") != 0 && line[end] != ';' &&
			   line[end] != '}' && line[end] != '{';
	};
	while (first >= 2 && continues(lines[first - 2]))
		--first;
	return LineRange{first, d.endLine};
}

std::optional<LineRange> definitionLines(const std::vector<std::string>& lines, const SrcModel::Definition& d,
										 uint64_t ea)
{
	LineRange range = definitionSpan(lines, d);
	std::string text;
	for (int64_t number = range.first; number <= range.last; ++number)
		text += lines[number - 1];
	std::regex literal("0x0*" + hexText(ea).substr(2) + "(?![0-9a-f])", std::regex::icase);
	if (!std::regex_search(text, literal))
		return std::nullopt;
	return range;
}

std::optional<EmittedBlock> emittedBlock(const std::vector<std::string>& lines, int64_t line)
{
	// the markers with any blanks around their words: `#if FNVD_HYBRID // emitted`, `#endif // emitted`
	static const std::regex opener(R"(^\s*#\s*if\s+FNVD_HYBRID\s*//\s*emitted\b.*)");
	static const std::regex closer(R"(^\s*#\s*endif\s*//\s*emitted\b.*)");
	static const std::regex directive(R"(^\s*#\s*(\w+).*)");
	int64_t count = static_cast<int64_t>(lines.size());
	if (line < 1 || line > count)
		return std::nullopt;
	int64_t first = 0;
	for (int64_t number = line; number >= 1 && !first; --number)
	{
		if (std::regex_match(lines[number - 1], opener))
			first = number;
		else if (number != line && std::regex_match(lines[number - 1], closer))
			return std::nullopt;
	}
	if (!first)
		return std::nullopt;
	int64_t last = 0;
	for (int64_t number = line; number <= count && !last; ++number)
	{
		if (std::regex_match(lines[number - 1], closer))
			last = number;
		else if (number != line && std::regex_match(lines[number - 1], opener))
			return std::nullopt;
	}
	if (!last)
		return std::nullopt;
	EmittedBlock block{{first, last}, std::nullopt};
	// the block's own #else: the one outside every #if the body itself opens
	int depth = 0;
	std::smatch match;
	for (int64_t number = first + 1; number < last; ++number)
	{
		if (!std::regex_match(lines[number - 1], match, directive))
			continue;
		std::string word = match[1];
		if (word == "if" || word == "ifdef" || word == "ifndef")
			++depth;
		else if (word == "endif")
			--depth;
		else if (word == "else" && depth == 0)
		{
			block.forwarder = LineRange{number + 1, last - 1};
			break;
		}
	}
	return block;
}

const std::string kEmittedBodyIf = "#if FNVD_HYBRID  // emitted: unresolved spots marked";
const std::string kEmittedNeedsIf = "#if FNVD_HYBRID  // emitted: what the marked bodies need";
const std::string kEmittedEnd = "#endif  // emitted";

int64_t declarationFirst(const std::vector<std::string>& lines, int64_t line)
{
	auto continues = [](const std::string& text) {
		std::string_view start = trimmed(text);
		size_t end = text.find_last_not_of(" \t");
		return !start.empty() && !start.starts_with("//") && !start.starts_with("#") &&
			   std::string_view(";{}:").find(text[end]) == std::string_view::npos;
	};
	while (line >= 2 && line - 1 <= static_cast<int64_t>(lines.size()) && continues(lines[line - 2]))
		--line;
	return line;
}

int64_t commentTop(const std::vector<std::string>& lines, int64_t first)
{
	while (first >= 2 && first - 1 <= static_cast<int64_t>(lines.size()) && trimmed(lines[first - 2]).starts_with("//"))
		--first;
	return first;
}

std::optional<InlineDefinition> inlineDefinition(const std::string& text, const std::string& name)
{
	auto identifier = [](char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; };
	size_t open = std::string::npos;
	for (size_t at = text.find(name); at != std::string::npos && open == std::string::npos;
		 at = text.find(name, at + 1))
	{
		size_t after = text.find_first_not_of(" \t\n", at + name.size());
		if ((at == 0 || !identifier(text[at - 1])) && after != std::string::npos && text[after] == '(')
			open = after;
	}
	if (open == std::string::npos)
		return std::nullopt;
	auto matching = [&](size_t at, char opening, char closing) -> size_t {
		int depth = 0;
		for (size_t k = at; k < text.size(); ++k)
			if (text[k] == opening)
				++depth;
			else if (text[k] == closing && --depth == 0)
				return k;
		return std::string::npos;
	};
	size_t close = matching(open, '(', ')');
	size_t brace = close == std::string::npos ? close : text.find('{', close);
	size_t end = brace == std::string::npos ? brace : matching(brace, '{', '}');
	if (end == std::string::npos)
		return std::nullopt;
	InlineDefinition out;
	std::string head = text.substr(0, brace);
	head.erase(head.find_last_not_of(" \t\n") + 1);
	head = std::regex_replace(head, std::regex(R"((^|\s)inline\s+)"), "$1");
	// a line comment ending the head (`f(...) // PDB: X` before the body's `{`) stays after the `;`
	std::string comment;
	size_t lineStart = head.rfind('\n');
	if (size_t slashes = head.find("//", lineStart == std::string::npos ? 0 : lineStart + 1);
		slashes != std::string::npos)
	{
		comment = " " + head.substr(slashes);
		head.erase(slashes);
		head.erase(head.find_last_not_of(" \t") + 1);
	}
	out.declaration = head + ";" + comment + text.substr(end + 1);
	std::istringstream inner(text.substr(brace + 1, end - brace - 1));
	for (std::string line; std::getline(inner, line);)
	{
		size_t start = line.find_first_not_of(" \t");
		if (start == std::string::npos)
			continue;
		line.erase(0, start);
		line.erase(line.find_last_not_of(" \t") + 1);
		out.statements.push_back(line);
	}
	return out;
}

std::string withoutInline(const std::string& text)
{
	static const std::regex head(R"(^(\s*(?:template\s*<\s*>\s*)?(?:\[\[[^\]]*\]\]\s*)*)inline\s+)");
	std::string out;
	for (const std::string& line : splitLines(text))
		out += std::regex_replace(line, head, "$1", std::regex_constants::format_first_only) + "\n";
	return out;
}

std::optional<EmittedGlobal> emittedGlobal(const std::string& line, const std::string& alias)
{
	const std::string_view start = "static FNVD_GLOBAL(";
	std::string_view text = trimmed(line);
	if (!text.starts_with(start) || !text.ends_with(");"))
		return std::nullopt;
	// the type may hold commas: the name and the address are the last two arguments
	text = text.substr(start.size(), text.size() - start.size() - 2);
	size_t address = text.rfind(", ");
	size_t name = address == std::string_view::npos || address == 0 ? address : text.rfind(", ", address - 1);
	if (name == std::string_view::npos)
		return std::nullopt;
	EmittedGlobal out{std::string(text.substr(name + 2, address - name - 2)), std::string(text.substr(0, name))};
	std::string_view using_ = trimmed(alias);
	if (size_t equals = using_.find(" = "); using_.starts_with("using ") && equals != std::string_view::npos)
		out.type = std::string(using_.substr(equals + 3, using_.rfind(';') - equals - 3));
	return out;
}

std::vector<EmittedGlobal> takeRedeclaredGlobals(std::vector<std::vector<std::string>>& globals,
												 const std::vector<std::string>& lines)
{
	std::map<std::string, std::string_view> declared;
	for (const std::string& line : lines)
		if (std::optional<EmittedGlobal> global = emittedGlobal(line))
			declared.emplace(global->name, trimmed(line));
	std::vector<EmittedGlobal> out;
	std::erase_if(globals, [&](const std::vector<std::string>& entry) {
		std::optional<EmittedGlobal> own =
			emittedGlobal(entry.back(), entry.size() > 1 ? entry.front() : std::string());
		auto found = own ? declared.find(own->name) : declared.end();
		if (found == declared.end() || found->second == trimmed(entry.back()))
			return false;
		out.push_back(*own);
		return true;
	});
	return out;
}

std::string readAs(const std::string& text, const EmittedGlobal& global)
{
	// an emitted global's name is `g_` and hex digits: no regex syntax in it
	return std::regex_replace(text, std::regex("\\b" + global.name + "\\b"),
							  "gen::as<" + global.type + ">(" + global.name + ")");
}
FileEdits::File& FileEdits::file(const std::string& path)
{
	auto [it, added] = m_files.try_emplace(path);
	File& file = it->second;
	if (!added || !std::filesystem::exists(path))
		return file;
	std::ifstream in(path, std::ios::binary);
	if (!in)
		throw Error("idakit: cannot read " + path);
	std::ostringstream text;
	text << in.rdbuf();
	file.exists = true;
	file.newlineAtEnd = text.str().empty() || text.str().back() == '\n';
	file.lines = splitLines(text.str());
	return file;
}

const std::vector<std::string>& FileEdits::lines(const std::string& path)
{
	return file(path).lines;
}

size_t FileEdits::add(const std::string& path, Edit edit)
{
	File& target = file(path);
	auto where = [&](const Edit& e) {
		return path + ":" + std::to_string(e.first) + (e.last >= e.first ? "-" + std::to_string(e.last) : "");
	};
	// two replaces overlap when they share a line; an insert overlaps a replace when it falls strictly inside it
	// (before its first line or after its last it is a neighbour)
	for (const Edit& other : target.edits)
	{
		bool clash = edit.last >= edit.first && other.last >= other.first
						 ? edit.first <= other.last && other.first <= edit.last
						 : edit.last >= edit.first   ? edit.first < other.first && other.first <= edit.last
						 : other.last >= other.first ? other.first < edit.first && edit.first <= other.last
													 : false;
		if (clash)
			throw Error("idakit: overlapping edits at " + where(other) + " and " + where(edit));
	}
	edit.order = m_order++;
	target.edits.push_back(std::move(edit));
	return target.edits.back().order;
}

size_t FileEdits::insert(const std::string& path, int64_t line, const std::string& text)
{
	int64_t size = static_cast<int64_t>(lines(path).size());
	if (line < 1 || line > size + 1)
		throw Error("idakit: no line " + std::to_string(line) + " to insert before in " + path);
	return add(path, Edit{line, line - 1, splitLines(text), 0});
}

size_t FileEdits::replace(const std::string& path, LineRange range, const std::string& text)
{
	int64_t size = static_cast<int64_t>(lines(path).size());
	if (range.first < 1 || range.last < range.first || range.last > size)
		throw Error("idakit: no lines " + std::to_string(range.first) + "-" + std::to_string(range.last) + " in " +
					path);
	return add(path, Edit{range.first, range.last, splitLines(text), 0});
}

bool FileEdits::has(const std::string& path, const std::string& line, bool unguarded)
{
	const File& target = file(path);
	std::string_view wanted = trimmed(line);
	auto matches = [&](const std::string& candidate) { return trimmed(candidate) == wanted; };
	// tracks whether a line is inside an emitted hybrid-only block (they do not nest)
	auto guardedAfter = [](bool inside, const std::string& candidate) {
		if (candidate == kEmittedBodyIf || candidate == kEmittedNeedsIf)
			return true;
		if (candidate == kEmittedEnd)
			return false;
		return inside;
	};
	bool inside = false;
	for (int64_t number = 1; number <= static_cast<int64_t>(target.lines.size()); ++number)
	{
		const std::string& candidate = target.lines[number - 1];
		bool guarded = inside;
		inside = guardedAfter(inside, candidate);
		bool removed = std::any_of(target.edits.begin(), target.edits.end(), [&](const Edit& e) {
			return e.first <= number && number <= e.last;
		});
		if (!removed && !(unguarded && guarded) && matches(candidate))
			return true;
	}
	for (const Edit& edit : target.edits)
	{
		bool editInside = false;
		for (const std::string& candidate : edit.text)
		{
			bool guarded = editInside;
			editInside = guardedAfter(editInside, candidate);
			if (!(unguarded && guarded) && matches(candidate))
				return true;
		}
	}
	return false;
}

void FileEdits::apply()
{
	for (auto& [path, target] : m_files)
	{
		if (target.edits.empty())
			continue;
		// last to first, so each edit's lines are still those read; at one line a replace goes before the inserts
		// there (they land above its text), and those inserts last-collected first (they keep their order)
		std::sort(target.edits.begin(), target.edits.end(), [](const Edit& a, const Edit& b) {
			if (a.first != b.first)
				return a.first > b.first;
			bool aReplace = a.last >= a.first, bReplace = b.last >= b.first;
			if (aReplace != bReplace)
				return aReplace;
			return a.order > b.order;
		});
		int64_t size = static_cast<int64_t>(target.lines.size());
		// a file without a newline at its end keeps that unless an edit adds or replaces its last line
		bool newlineAtEnd = target.newlineAtEnd;
		std::vector<std::string>& lines = target.lines;
		for (const Edit& edit : target.edits)
		{
			if (edit.last == size || edit.first == size + 1)
				newlineAtEnd = true;
			lines.erase(lines.begin() + (edit.first - 1), lines.begin() + edit.last);
			lines.insert(lines.begin() + (edit.first - 1), edit.text.begin(), edit.text.end());
		}
		// where each edit's text landed: top to bottom (the reverse of the order applied, at one line too), each
		// moved by the lines the edits above it added or took away
		int64_t shift = 0;
		for (auto edit = target.edits.rbegin(); edit != target.edits.rend(); ++edit)
		{
			int64_t count = static_cast<int64_t>(edit->text.size());
			if (count > 0)
				m_placed[edit->order] = LineRange{edit->first + shift, edit->first + shift + count - 1};
			shift += count - (edit->last - edit->first + 1);
		}
		// a file this run creates gets its text from inserts made in the order its users asked, so an include one
		// of them needed late sits below code: every include outside a preprocessor block moves up to the file's
		// head (after `#pragma once` and the includes already there), once each; each edit's text is then found
		// again, top to bottom (an include line anywhere, as hoisting moved it up)
		if (!target.exists)
		{
			hoistIncludes(lines);
			auto at = [&](const std::vector<std::string>& text, size_t from) {
				for (size_t k = from; k + text.size() <= lines.size(); ++k)
					if (std::equal(text.begin(), text.end(), lines.begin() + k))
						return k;
				return lines.size();
			};
			size_t cursor = 0;
			for (auto edit = target.edits.rbegin(); edit != target.edits.rend(); ++edit)
			{
				if (edit->text.empty())
					continue;
				bool include = std::all_of(edit->text.begin(), edit->text.end(),
										   [](const std::string& line) { return line.starts_with("#include"); });
				size_t found = at(edit->text, include ? 0 : cursor);
				if (found == lines.size())
				{
					m_placed.erase(edit->order);
					continue;
				}
				m_placed[edit->order] = LineRange{static_cast<int64_t>(found) + 1,
												  static_cast<int64_t>(found + edit->text.size())};
				if (!include)
					cursor = found + edit->text.size();
			}
		}
		std::string text;
		for (const std::string& line : lines)
			text += line + "\n";
		if (!newlineAtEnd && !text.empty())
			text.pop_back();
		std::filesystem::path file(path);
		if (!target.exists && file.has_parent_path())
			std::filesystem::create_directories(file.parent_path());
		std::ofstream out(path, std::ios::binary | std::ios::trunc);
		if (!(out << text))
			throw Error("idakit: cannot write " + path);
	}
	m_files.clear();
}

std::optional<LineRange> FileEdits::placed(size_t id) const
{
	auto found = m_placed.find(id);
	return found == m_placed.end() ? std::nullopt : std::optional<LineRange>(found->second);
}

std::vector<std::string> FileEdits::paths() const
{
	std::vector<std::string> out;
	for (const auto& [path, target] : m_files)
		if (!target.edits.empty())
			out.push_back(path);
	return out;
}

std::string includeLine(const std::string& header)
{
	return header.starts_with('<') ? "#include " + header : "#include \"" + header + "\"";
}

std::optional<size_t> ensureInclude(FileEdits& edits, const std::string& path, const std::string& header)
{
	std::string include = includeLine(header);
	// an include only a hybrid-only block has does not serve the code that needs it in the standalone build
	if (edits.has(path, include, true))
		return std::nullopt;
	const std::vector<std::string>& lines = edits.lines(path);
	auto isInclude = [](const std::string& line) { return trimmed(line).starts_with("#include"); };
	// the first run of includes outside the emitted hybrid-only blocks, so the new include is not guarded
	auto run = lines.end();
	bool inside = false;
	for (auto it = lines.begin(); it != lines.end(); ++it)
	{
		if (*it == kEmittedBodyIf || *it == kEmittedNeedsIf)
			inside = true;
		else if (*it == kEmittedEnd)
			inside = false;
		else if (!inside && isInclude(*it))
		{
			run = it;
			break;
		}
	}
	int64_t at = 1;
	if (run != lines.end())
		at = std::find_if_not(run, lines.end(), isInclude) - lines.begin() + 1;
	else
	{
		auto once = std::find_if(lines.begin(), lines.end(),
								 [](const std::string& line) { return trimmed(line).starts_with("#pragma once"); });
		if (once != lines.end())
			at = once - lines.begin() + 2;
	}
	return edits.insert(path, at, include);
}

std::string fixedWidth(std::string text)
{
	// env/scratch/Forwarders/drive.py's FIXED, in its order: the two-word names before the words they contain
	static const std::pair<std::regex, const char*> kFixed[] = {
		{std::regex(R"(\bunsigned\s+__int64\b)"), "uint64_t"},
		{std::regex(R"(\b__int64\b)"), "int64_t"},
		{std::regex(R"(\bunsigned\s+(?:int|long)\b)"), "uint32_t"},
		{std::regex(R"(\bunsigned\s+short\b)"), "uint16_t"},
		{std::regex(R"(\bunsigned\s+char\b)"), "uint8_t"},
		{std::regex(R"(\b(?:signed\s+)?(?:int|long)\b)"), "int32_t"},
		{std::regex(R"(\bshort\b)"), "int16_t"},
		{std::regex(R"(\b(?:DWORD|UINT|ULONG)\b)"), "uint32_t"},
		{std::regex(R"(\b(?:BOOL|LONG)\b)"), "int32_t"},
		{std::regex(R"(\bWORD\b)"), "uint16_t"},
		{std::regex(R"(\bBYTE\b)"), "uint8_t"},
	};
	for (const auto& [pattern, replacement] : kFixed)
		text = std::regex_replace(text, pattern, replacement);
	return text;
}

std::string address(uint64_t ea)
{
	char buffer[24];
	std::snprintf(buffer, sizeof buffer, "0x%llX", static_cast<unsigned long long>(ea));
	return buffer;
}

const std::set<std::string>& keywords()
{
	static const std::set<std::string> words = {
		"alignas", "alignof", "and", "asm", "auto", "bool", "break", "case", "catch", "char", "class", "const",
		"constexpr", "continue", "decltype", "default", "delete", "do", "double", "else", "enum", "explicit",
		"export", "extern", "false", "float", "for", "friend", "goto", "if", "inline", "int", "long", "mutable",
		"namespace", "new", "noexcept", "not", "nullptr", "operator", "or", "private", "protected", "public",
		"register", "return", "short", "signed", "sizeof", "static", "struct", "switch", "template", "this",
		"throw", "true", "try", "typedef", "typeid", "typename", "union", "unsigned", "using", "virtual", "void",
		"volatile", "while", "xor"};
	return words;
}

bool identifier(const std::string& s)
{
	if (s.empty() || std::isdigit(static_cast<unsigned char>(s[0])) ||
		(s[0] == '_' && s.size() > 1 && (s[1] == '_' || std::isupper(static_cast<unsigned char>(s[1])))))
		return false;
	for (char c : s)
		if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_')
			return false;
	return keywords().count(s) == 0;
}

std::string fixed64(const std::string& text)
{
	static const std::regex unsigned64(R"(\bunsigned long long\b)"), signed64(R"(\blong long\b)");
	return std::regex_replace(std::regex_replace(text, unsigned64, "uint64_t"), signed64, "int64_t");
}

std::string wrapped(const std::string& text, const std::string& indent, const std::string& unit)
{
	auto width = [](const std::string& s) {
		size_t w = 0;
		for (char c : s)
			w += c == '\t' ? 4 : 1;
		return w;
	};
	std::string out;
	size_t start = 0;
	while (start <= text.size())
	{
		size_t end = text.find('\n', start);
		if (end == std::string::npos)
			end = text.size();
		std::string line = text.substr(start, end - start);
		start = end + 1;
		// the pieces: the text up to and including each breakable ", ", or the first `(` of an argument list
		std::vector<std::string> pieces;
		int parens = 0, angles = 0;
		size_t from = 0;
		for (size_t k = 0; k < line.size(); ++k)
		{
			char c = line[k];
			parens += c == '(' ? 1 : c == ')' ? -1 : 0;
			angles += c == '<' ? 1 : c == '>' ? -1 : 0;
			if (c == ',' && parens > 0 && angles <= 0 && k + 1 < line.size() && line[k + 1] == ' ')
			{
				pieces.push_back(line.substr(from, k + 1 - from));
				from = k + 2;
			}
			else if (c == '(' && parens == 1 && angles <= 0 && k + 1 < line.size() && line[k + 1] != ')')
			{
				pieces.push_back(line.substr(from, k + 1 - from));
				from = k + 1;
			}
		}
		pieces.push_back(line.substr(from));
		// a head still too long: the return type on a line of its own (its last blank outside template arguments)
		if (width(indent + pieces[0]) > 120)
		{
			int depth = 0;
			size_t split = std::string::npos;
			for (size_t k = 0; k < pieces[0].size(); ++k)
			{
				depth += pieces[0][k] == '<' ? 1 : pieces[0][k] == '>' ? -1 : 0;
				if (pieces[0][k] == ' ' && depth == 0 && k > 0 && pieces[0].compare(0, k, "return") != 0)
					split = k;
			}
			if (split != std::string::npos)
			{
				out += indent + pieces[0].substr(0, split) + "\n";
				pieces[0] = pieces[0].substr(split + 1);
			}
		}
		std::string current = indent + pieces[0];
		for (size_t k = 1; k < pieces.size(); ++k)
		{
			std::string glue = pieces[k - 1].ends_with('(') ? "" : " ";
			if (width(current) + glue.size() + width(pieces[k]) > 120)
			{
				out += current + "\n";
				current = indent + unit + pieces[k];
			}
			else
				current += glue + pieces[k];
		}
		out += current + (start <= text.size() ? "\n" : "");
	}
	return out;
}

std::string indentUnit(const std::vector<std::string>& lines, int64_t first, int64_t last)
{
	for (int64_t k = std::max<int64_t>(first, 1); k <= last && k <= static_cast<int64_t>(lines.size()); ++k)
	{
		const std::string& line = lines[k - 1];
		if (!line.empty() && line[0] == '\t')
			return "\t";
		if (line.size() > 1 && line[0] == ' ' && line.find_first_not_of(' ') != std::string::npos)
			return "    ";
	}
	return "\t";
}

void srcDeclaration(Decl& d, const SrcModel& model, const SrcModel::Function& f, const std::string& self)
{
	const SrcModel::Abi* abi = model.abi(f.mangled);
	const SrcModel::Type* type = model.type(f.type);
	if (!abi || !type)
		throw Skip{"no ABI row for " + f.name};
	d.owner = model.record(f.cls);
	d.exact = true;
	d.indirect = abi->ret == "indirect";
	d.mangled = f.mangled;
	d.member = lastComponent(f.name);
	d.scope = d.owner ? "" : scopeOf(f.name);
	d.ret = type->ret;
	d.isConst = type->isConst;
	d.isStatic = f.isStatic;
	d.cc = abi->cc;
	d.noreturn = f.noreturn;
	d.self = f.isStatic || f.cls.empty() ? "" : self;
	// a const member's `this` is const: so is its base's cast
	if (d.isConst && d.self.starts_with("static_cast<"))
		d.self.insert(12, "const ");
	d.params.clear();
	for (size_t k = 0; k < type->params.size(); ++k)
	{
		std::string name = k < f.params.size() && identifier(f.params[k]) ? f.params[k] : "a" + std::to_string(k + 1);
		d.params.push_back({type->params[k], name});
	}
	d.pdbName = f.name;
}

std::string Forwarders::fixedKey(const std::string& key) const
{
	return m_types.fixedWidthKey(key);
}

std::string Forwarders::spelled(const Decl& d, const std::string& key, const std::string& name) const
{
	std::optional<std::string> s = m_types.spell(d.exact ? key : fixedKey(key), name);
	if (!s)
		throw Skip{"type src cannot spell: " + key};
	return fixed64(*s);
}

std::string Forwarders::signature(const Decl& d, bool definition,
								  const std::function<std::string(const std::string&)>& keyAt) const
{
	auto at = [&](const std::string& key) { return keyAt ? keyAt(key) : key; };
	std::string params;
	for (const Param& p : d.params)
		params += (params.empty() ? "" : ", ") + spelled(d, at(p.key), p.name);
	bool member = d.owner != nullptr;
	std::string defaultCc = member && !d.isStatic ? "thiscall" : "cdecl";
	std::string cc;
	// a usercall forwarder is C++'s default convention: no convention spells its registers (its call does)
	bool user = d.cc == "usercall" || d.cc == "userpurge";
	if (!user && (d.cc != defaultCc || (!d.exact && (d.isStatic || !member))))
		cc = d.cc == "thiscall" ? "FNVD_THISCALL "
		   : d.cc == "stdcall"	? "FNVD_STDCALL "
		   : d.cc == "fastcall" ? "FNVD_FASTCALL "
								: "FNVD_CDECL ";
	// a conversion function (`operator T()`) has no result type of its own: the result's spelling is its name
	bool conversion = d.member.starts_with("operator") && d.member.size() > 8 &&
					  (d.member[8] == ' ' || std::isalpha(static_cast<unsigned char>(d.member[8]))) &&
					  !d.member.starts_with("operator new") && !d.member.starts_with("operator delete");
	std::string own = conversion ? "operator " + spelled(d, at(d.ret)) : d.member;
	std::string name = own;
	// the owner and the scope as the TU spells them: a TU-local record's model name (`X @src/f.cpp`) is X, and an
	// anonymous namespace is no qualifier (tuSpelling)
	if (definition || d.outcome == "specialized")
		name = member ? tuSpelling(fixed64(d.owner->name)) + "::" + own
					  : tuSpelling(d.scope.empty() ? "" : d.scope + "::") + own;
	std::string prefix = std::string(d.noreturn ? "[[noreturn]] " : "") +
						 (!definition && d.isStatic && d.outcome != "specialized" ? "static " : "");
	std::string tail = d.isConst ? " const" : "";
	if (conversion)
		return prefix + cc + name + "(" + params + ")" + tail;
	// a result that is a pointer or reference to a function or an array goes after the parameters: `auto f(...) -> R
	// (FNVD_CDECL*)(...)`, `auto f(...) -> char (&)[512]`
	std::string ret = d.ret;
	for (SrcTypes::Parts p = m_types.parts(ret); p.kind == "ptr" || p.kind == "lref" || p.kind == "const";
		 p = m_types.parts(ret))
		ret = p.of;
	if (SrcTypes::function(ret) || m_types.parts(m_types.canonical(ret)).kind == "array")
		return prefix + "auto " + cc + name + "(" + params + ")" + tail + " -> " + spelled(d, at(d.ret));
	return prefix + spelled(d, at(d.ret)) + " " + cc + name + "(" + params + ")" + tail;
}

std::vector<std::string> Forwarders::statements(const Decl& d) const
{
	bool user = d.cc == "usercall" || d.cc == "userpurge";
	std::string call =
		std::string("fnvd::original::") + (d.cc == "cdecl" ? "cdecl_" : user ? "usercall" : d.cc) + "<";
	SrcTypes::Parts ret = m_types.parts(m_types.bare(d.ret));
	// a record returned by value: built by the exe through its hidden pointer in uninitialised storage (src's
	// default constructor must not run first), then moved out
	std::string local = "aucResult";
	while (std::any_of(d.params.begin(), d.params.end(), [&](const Param& p) { return p.name == local; }))
		local += "_";
	std::string result = d.indirect ? spelled(d, d.ret) : "";
	if (ret.kind == "lref")
		call += spelled(d, "ptr(" + ret.of + ")");
	else if (d.indirect)
		call += spelled(d, "ptr(" + d.ret + ")");
	else
		call += spelled(d, d.ret);
	call += ">(" + address(d.ea);
	// a usercall: the register arguments (`this` too) in fnvd::original::UserRegs, in its member order, as dwords;
	// the stack arguments after it, as any convention's
	if (user)
	{
		static const std::vector<std::string> order = {"eax", "ecx", "edx", "ebx", "esi", "edi"};
		std::map<size_t, std::string> loaded;
		auto load = [&](const std::string& reg, const std::string& key, const std::string& name) {
			SrcTypes::Parts pp = m_types.parts(m_types.canonical(key));
			std::string value = pp.kind == "lref"	? "(uint32_t)(uintptr_t)(&" + name + ")"
								: pp.kind == "ptr" ? "(uint32_t)(uintptr_t)(" + name + ")"
												   : "(uint32_t)" + name;
			loaded[std::find(order.begin(), order.end(), reg) - order.begin()] = "." + reg + " = " + value;
		};
		if (!d.selfReg.empty())
			load(d.selfReg, "ptr(record:" + d.owner->name + ")", "this");
		for (const Param& p : d.params)
			if (auto reg = d.registers.find(p.name); reg != d.registers.end())
				load(reg->second, p.key, p.name);
		std::string regs;
		for (const auto& [index, text] : loaded)
			regs += (regs.empty() ? "" : ", ") + text;
		call += ", fnvd::original::UserRegs{" + regs + "}";
	}
	else if (!d.self.empty())
		call += ", " + d.self;
	if (d.indirect)
		call += ", reinterpret_cast<" + result + "*>(" + local + ")";
	for (const Param& p : d.params)
	{
		if (d.registers.count(p.name))
			continue;
		SrcTypes::Parts pp = m_types.parts(m_types.bare(p.key));
		call += ", " + std::string(pp.kind == "lref" ? "&" : "") + p.name;
	}
	call += ")";
	if (d.indirect)
		return {"alignas(" + result + ") uint8_t " + local + "[sizeof(" + result + ")];", call + ";",
				"return static_cast<" + result + "&&>(*reinterpret_cast<" + result + "*>(" + local + "));"};
	// the exe's function never comes back: nothing follows its call
	if (d.noreturn)
		return {call + ";", "__builtin_unreachable();"};
	if (ret.kind == "lref")
		return {"return *" + call + ";"};
	if (ret.kind == "builtin" && ret.name == "void")
		return {call + ";"};
	return {"return " + call + ";"};
}

std::string Forwarders::templateHeads(const Decl& d) const
{
	int heads = std::max(1, m_types.model().templateHeads(d.mangled, d.owner ? d.owner->name : ""));
	std::string out;
	for (int k = 0; k < heads; ++k)
		out += "template <>\n";
	return out;
}

std::string Forwarders::definition(const Decl& d, bool heads, const std::string& unit, bool comment,
								   const std::function<std::string(const std::string&)>& keyAt) const
{
	std::string body =
		(heads ? templateHeads(d) : std::string()) + wrapped(signature(d, true, keyAt), "", unit) + "\n{";
	if (comment)
		body += "\n" + unit + "// Not reconstructed yet.";
	for (const std::string& statement : statements(d))
		body += "\n" + wrapped(statement, unit, unit);
	return body + "\n}";
}

std::string srcSpelling(const std::string& text)
{
	static const std::regex unsigned64(R"(\bunsigned long long\b)"), signed64(R"(\blong long\b)"),
		blank(R"(([A-Za-z0-9_>]) +([*&]))");
	std::string out = std::regex_replace(std::regex_replace(text, unsigned64, "uint64_t"), signed64, "int64_t");
	return std::regex_replace(out, blank, "$1$2");
}

}  // namespace idakit
