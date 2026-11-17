// The functions section: src's prototypes, the folds' class-neutral prototypes, src's globals and src's function
// names, pushed into the database.

#include "idakit/Sync.hpp"

#include "IdaSupport.hpp"

#include "idakit/AsmFacts.hpp"
#include "idakit/ExePrototype.hpp"

#include <Facts.hpp>

#include <algorithm>
#include <cstdio>
#include <set>
#include <tuple>

namespace idakit
{

namespace
{

// The type applied at the address. IDA keeps a stored type it deems equal to the new one in its old spelling (a
// reference to `struct X` by name, which prints with the tag until the next open): the old type goes first, so that
// the stored type is the new one as it is.
bool applyType(ea_t address, const tinfo_t& type)
{
	del_tinfo(address);
	return apply_tinfo(address, type, TINFO_DEFINITE);
}

// The stored type is the new one: the two and their field names and comments serialize alike (IDA stores the one as
// the other), or they differ only in how a reference to the same named type is spelled, by its tag (`enum X`, as IDA
// stored it) or as a typedef (`X`, as sync converts src's): IDA deems the two equal, they print alike but for the
// tag, and their names and comments serialize alike. Writing such a type changes no type, but it queues the analysis
// of the callers, which gives their stack slots the arguments' types and names (names' auto_wait), and Hex-Rays
// drops the callers' lvar settings at those slots.
bool sameStoredType(const tinfo_t& a, const tinfo_t& b)
{
	qtype typeA, fieldsA, commentsA, typeB, fieldsB, commentsB;
	return a.serialize(&typeA, &fieldsA, &commentsA) && b.serialize(&typeB, &fieldsB, &commentsB) &&
		   fieldsA == fieldsB && commentsA == commentsB &&
		   (typeA == typeB || (a.equals_to(b) && sameType(a, b)));
}

// the type with `__noreturn`; nullopt where IDA cannot make it
std::optional<tinfo_t> withNoreturn(const tinfo_t& type)
{
	func_type_data_t fd;
	if (!type.get_func_details(&fd))
		return std::nullopt;
	if (fd.is_noret())
		return type;
	fd.flags |= FTI_NORET;
	tinfo_t out;
	return out.create_func(fd) ? std::optional(out) : std::nullopt;
}

// Where the exe's code contradicts the prototype, why; "" where it does not.
std::string contradiction(exefacts::Facts& facts, AsmFacts& asmFacts, ea_t address, const tinfo_t& type)
{
	if (std::string registers = unpassedRegisters(facts, address, type); !registers.empty())
		return "the code reads " + registers + " before writing it; the prototype does not pass it";
	if (contradicted(facts, address, type))
		return "the code reads other registers or stack slots, or pops other bytes";
	std::string why;
	if (std::optional<tinfo_t> returning = exeReturn(facts, asmFacts, address, type, why))
		return "returns void, but the code sets eax and the callers read it: " + text(*returning);
	if (std::optional<tinfo_t> wider = widerReturn(facts, asmFacts, address, type))
		return "returns fewer bytes than every return path writes and a caller reads: " + text(*wider);
	return "";
}

// The characters of the names outside IDA's name set ('<', '~', ' ', ...), which set_name would replace by '_',
// allowed for this session: the database spells templates, operators and destructors verbatim.
void allowNameCharacters(const std::map<ea_t, std::string>& names)
{
	std::set<unsigned char> characters;
	for (const auto& [address, name] : names)
		characters.insert(name.begin(), name.end());
	for (unsigned char c : characters)
		if (!is_ident_cp(c))
			set_cp_validity(UCDR_NAME, c, c + 1, true);
}

// a global's IDA name: the exe's names have no anonymous namespace, IDA cannot spell clang's; src's msvc9:: is no
// namespace of the exe's data names either
std::string globalName(std::string name)
{
	for (const std::string scope : {"(anonymous namespace)::", "msvc9::"})
		for (size_t at = name.find(scope); at != std::string::npos; at = name.find(scope))
			name.erase(at, scope.size());
	return name;
}

// IDA's item at an item head, for a log: a string, a struct (its type) or other data with its bytes, or unexplored
// bytes
std::string itemKind(ea_t head)
{
	flags64_t flags = get_flags(head);
	std::string bytes = std::to_string(get_item_size(head)) + " bytes";
	if (is_unknown(flags))
		return "unexplored bytes";
	if (is_strlit(flags))
		return "string of " + bytes;
	if (is_struct(flags))
	{
		tinfo_t type;
		return (get_tinfo(&type, head) ? text(type) : std::string("struct")) + " of " + bytes;
	}
	return "data of " + bytes;
}

}  // namespace

// Hex-Rays drops a function's user lvar settings at its arguments (definition address the function's start)
// whenever a type is applied to it, the type it holds included: a prototype is applied only where it changes. A
// stored type equal to src's that IDA or Hex-Rays guessed is marked the user's, so that no decompile re-derives it.
// A function IDA has as never returning keeps that (a prototype without it would give its callers' code after the
// call back to their sp traces and decompiles): the written type is `__noreturn`, src's `[[noreturn]]` or not.
bool Sync::writePrototype(ea_t address, const tinfo_t& written)
{
	tinfo_t stored;
	bool typed = get_tinfo(&stored, address);
	tinfo_t type = written;
	if (idaNoreturn(address))
	{
		std::optional<tinfo_t> kept = withNoreturn(written);
		if (!kept)
		{
			count("noreturn unkept");
			log("noreturn-unkept", hex(address), text(written));
			return false;
		}
		if (!sameType(*kept, written))
		{
			count("noreturn kept");
			log("noreturn-kept", hex(address), text(written));
		}
		type = *kept;
	}
	if (typed && sameStoredType(stored, type))
	{
		if (!is_userti(address))
			set_userti(address);
		count("prototype-unchanged");
		return true;
	}
	if (!applyType(address, type))
	{
		if (typed)
			apply_tinfo(address, stored, TINFO_DEFINITE);
		return false;
	}
	m_prototypeChanged.insert(address);
	count("prototype-changed");
	log("prototype-changed", hex(address), (typed ? text(stored) : std::string("untyped")) + " -> " + text(type));
	return true;
}

// IDA's name at each address set to the wanted one, with IDA's `_N` suffix where another address holds it. A name
// that already is the wanted one, or it with a suffix (an earlier run's), stays. A name of IDA's dummy form (src's
// `sub_<ADDR>`) is no user name: the address's user name goes, and IDA's own dummy name is its name.
void Sync::setNames(const std::map<ea_t, std::string>& wanted, const std::string& kind)
{
	allowNameCharacters(wanted);
	for (const auto& [address, name] : wanted)
	{
		std::string current = get_name(address).c_str();
		bool dummy = is_ident(name.c_str()) && !is_uname(name.c_str());
		if (dummy ? !has_user_name(get_flags(address)) : current == name || withoutSuffix(current) == name)
		{
			count(kind + " names unchanged");
			continue;
		}
		bool set = dummy ? set_name(address, "", SN_NOWARN)
						 : set_name(address, name.c_str(), SN_NOWARN | SN_NOCHECK | SN_FORCE);
		std::string given = get_name(address).c_str();
		if (!set || (dummy ? has_user_name(get_flags(address)) : given == current))
		{
			count(kind + " names failed");
			log(kind + "-name-failed", hex(address), name + " (IDA: " + given + ")");
			continue;
		}
		count(kind + " names set");
		if (!dummy && given != name)
			log(kind + "-name-suffixed", hex(address), name + " -> " + given + " (another address holds the name)");
		log(kind + "-renamed", hex(address), current + " -> " + given);
	}
}

// The folds are found first: a typed fold's address gets the fold's prototype alone (foldPrototypes), since each
// write of another type drops the function's argument lvar settings (writePrototype).
void Sync::prototypes()
{
	findFolds();
	for (const auto& [address, refs] : m_model.refs())
	{
		if (address >= kGameEnd)
			break;
		const SrcModel::Function* function = declared(address);
		if (!function)
			continue;
		if (!functionStart(address))
		{
			log("prototype-not-a-function", hex(address), function->name);
			continue;
		}
		std::optional<tinfo_t> type;
		if (usercallDeclared(address))
		{
			std::string why;
			type = usercallPrototype(address, why);
			if (!type)
			{
				tinfo_t kept;
				count("usercalls refused");
				log("usercall-refused", hex(address),
					function->name + ": " + why + " (kept: " +
						(get_tinfo(&kept, address) ? text(kept) : std::string("untyped")) + ")");
				continue;
			}
			count("usercalls");
			if (!why.empty())
			{
				tinfo_t replaced;
				count("usercalls exe-decided");
				log("usercall-exe-decided", hex(address),
					function->name + ": " + why + " (was: " +
						(get_tinfo(&replaced, address) ? text(replaced) : std::string("untyped")) + ")");
			}
		}
		else
			type = prototype(*function, address);
		if (!type)
		{
			count("prototypes unmapped");
			continue;
		}
		auto fold = folds().find(address);
		bool folded = fold != folds().end() && fold->second.type;
		if (!folded && !writePrototype(address, *type))
		{
			count("prototypes failed");
			log("prototype-failed", hex(address), function->name + ": " + text(*type));
			continue;
		}
		count("prototypes");
		if (std::string why = contradiction(m_facts, m_asm, address, *type); !why.empty())
		{
			count("src-contradicted");
			log("src-contradicted", hex(address), function->name + ": " + text(*type) + ": " + why);
		}
		m_facts.trim();
	}
}

void Sync::foldPrototypes()
{
	for (const auto& [address, fold] : folds())
	{
		count("folds");
		if (!fold.type)
		{
			count("folds untyped");
			log("fold-untyped", hex(address), std::to_string(fold.members.size()) + " members, none typed");
			continue;
		}
		if (!writePrototype(address, *fold.type))
		{
			count("folds failed");
			log("fold-failed", hex(address), std::to_string(fold.members.size()) + " members: " + text(*fold.type));
			continue;
		}
		count("folds typed");
	}
}

void Sync::globals()
{
	std::map<ea_t, std::string> wanted;
	// the last global placed and its end: one that starts inside it overlaps it (src's two globals disagree)
	std::string placedName;
	ea_t placedEnd = 0;
	for (const auto& [address, all] : m_model.refs())
	{
		// an emitted body's stand-in for a global src does not declare (`static FNVD_GLOBAL(T, g_XXXXXXXX, addr)`) is
		// neither src's name nor its type
		char standIn[16];
		std::snprintf(standIn, sizeof standIn, "g_%08llX", static_cast<unsigned long long>(address));
		std::vector<const SrcModel::Ref*> refs;
		for (const SrcModel::Ref& r : all)
			if ((r.kind == "global" || r.kind == "static_member") && !placeholder(r) && r.target != standIn)
				refs.push_back(&r);
		if (refs.empty())
			continue;
		const SrcModel::Ref& ref = **std::min_element(refs.begin(), refs.end(), [](const auto* a, const auto* b) {
			return std::make_tuple(a->local, a->kind != "static_member", a->file) <
				   std::make_tuple(b->local, b->kind != "static_member", b->file);
		});
		if (is_code(get_flags(address)))
		{
			// an overlay of a function's address (a function pointer src takes): the function keeps its type and name
			count("globals in code");
			log("global-in-code", hex(address), ref.target + ": " + ref.type);
			continue;
		}
		if (address < placedEnd)
		{
			count("globals overlapping");
			log("global-overlap", hex(address), ref.target + " starts inside " + placedName);
			continue;
		}
		count("globals");
		std::optional<tinfo_t> t = typeOf(ref.type);
		size_t size = t ? t->get_size() : BADSIZE;
		placedName = ref.target;
		placedEnd = address + (size == BADSIZE || size == 0 ? 1 : size);
		// src's type at the address, as one item of its size: IDA's items in the way (a string around it, an array
		// of another size) go, and so do the names inside it (IDA names only an item's head; one of src's own globals
		// there overlaps this one, above). The bytes are made one item first: IDA reads a char applied to unexplored
		// bytes at a C string as the string.
		auto placed = [&] {
			return applyType(address, *t) && get_item_head(address) == address &&
				   (size == BADSIZE || get_item_size(address) == size);
		};
		// IDA's item at the address before the global's type goes there
		ea_t head = get_item_head(address);
		std::string before = itemKind(head) + (head == address ? "" : " from " + hex(head));
		bool string = is_strlit(get_flags(head));
		if (!t)
		{
			count("globals unmapped");
			log("global-unmapped", hex(address), ref.target + ": " + ref.type);
		}
		else if (!placed())
		{
			asize_t bytes = static_cast<asize_t>(placedEnd - address);
			for (ea_t inside = address + 1; inside < placedEnd; ++inside)
				if (has_any_name(get_flags(inside)))
					set_name(inside, "", SN_NOWARN);
			del_items(address, DELIT_SIMPLE, bytes);
			create_byte(address, bytes);
			if (placed())
			{
				// IDA's items in the way are gone: a src fix where src's type is not the data's (a C string's
				// `const char x` for `const char x[N]`)
				count("globals typed over IDA's items");
				if (string)
					count("globals typed over IDA's strings");
				log("global-retyped", hex(address), ref.target + ": " + text(*t) + " over IDA's " + before);
			}
			else
			{
				count("globals type failed");
				log("global-type-failed", hex(address), ref.target + ": " + text(*t));
			}
		}
		wanted.emplace(address, globalName(ref.target));
	}
	setNames(wanted, "global");
}

void Sync::names()
{
	std::map<ea_t, std::string> wanted;
	for (const auto& [address, refs] : m_model.refs())
	{
		if (address >= kGameEnd)
			break;
		if (folds().count(address) && !m_model.registers(address))
		{
			count("function names left to IDA (a fold without a registered body)");
			continue;
		}
		const SrcModel::Function* function = declared(address);
		if (!function)
			continue;
		std::string name = idaFunctionName(function->name);
		if (name.empty())
			continue;
		if (!functionStart(address))
		{
			log("function-name-not-a-function", hex(address), name);
			continue;
		}
		wanted.emplace(address, name);
	}
	setNames(wanted, "function");
	// the analysis IDA queues for the section's types (each caller's argument types, and the names it gives the
	// dummy-named data and functions passed as typed arguments) runs now, not on the next open
	auto_wait();
}

}  // namespace idakit
