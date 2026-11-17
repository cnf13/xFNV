#include "idakit/Command.hpp"

namespace idakit
{

const std::vector<Command>& commands()
{
	static const std::vector<Command> table = {
		{"allocators", "--idb I64 [--out TSV]",
		 "the allocators and the wrappers that pass a size on to them, `address<TAB>name<TAB>size_arg<TAB>scale`, "
		 "into --out (default env/cache/allocators.tsv; emit's --allocators): the seeds by name, then each round's "
		 "callers that return the allocation and pass a parameter on as its size; the row count per round on stdout",
		 runAllocators, {}},
		{"decompile", "--idb I64 [--model DB] [--aliases TSV] [--out DIR] ADDR... | @FILE",
		 "the Hex-Rays text of functions, decompiled afresh after they are unfolded as emit unfolds "
		 "them (--model, --aliases: as sync's), on stdout (--out: DIR/<ADDR8>.c each); never saves the database",
		 runDecompile, {}},
		{"declare",
		 "--idb I64 --model DB --aliases TSV --sources TSV --out DIR [--pdb-types TXT] [--apply] [ADDR... | @FILE] | "
		 "--place-specializations --model DB --aliases TSV --sources TSV --out DIR [--apply] | "
		 "--globals --emitted DIR --idb I64 --model DB --aliases TSV --sources TSV --out DIR [--pdb-globals TXT] "
		 "[--pdb-types TXT] [--apply] [ADDR... | @FILE] | --vftables --idb I64 --model DB --aliases TSV --sources TSV "
		 "--out DIR [--apply] [ADDR... | @FILE] | --layouts --emitted DIR --model DB --out DIR [--apply] | "
		 "--callees --emitted DIR --idb I64 --model DB --aliases TSV --sources TSV --out DIR [--apply] "
		 "[ADDR... | @FILE] | --folds LIST --idb I64 --model DB --aliases TSV --sources TSV --out DIR [--apply] | "
		 "--fold-calls --emitted DIR --idb I64 --model DB --aliases TSV --sources TSV --out DIR [--apply] | "
		 "--pdb-signatures --model DB --aliases TSV --out DIR | --pdb-fields --model DB --aliases TSV --out DIR | "
		 "--retype TABLE --model DB --aliases TSV --out DIR [--apply] | --revert --out DIR ROW... | @FILE | "
		 "--unregister --out DIR [--apply] ADDR... | @FILE | "
		 "--hk-symbols TSV --model DB --aliases TSV --sources TSV --out DIR [--apply] [ADDR... | @FILE]",
		 "src declarations, with `// Not reconstructed yet.` forwarders, for exe functions src has none for "
		 "(DIR/declare.tsv, needs-class.tsv; --sources: env/cache/pdb-function-sources.tsv; --pdb-types: default "
		 "env/cache/pdb-types.txt, the PDB's vtables; no addresses: every game function); never saves the database. "
		 "--place-specializations (no database): every explicit "
		 "specialization declared once at its home and defined in a .cpp, every header forwarder or emitted block "
		 "defined in a .cpp (DIR/place.tsv). --globals: the globals, static members, vtables and function addresses "
		 "(FNVD_ORIGINAL_FUNCTION) an emit directory's bodies read without a src declaration (its spots.tsv; ADDR: "
		 "only those), each declared once "
		 "(DIR/declare-globals.tsv; --pdb-globals: default env/cache/pdb-globals.txt, --pdb-types: default "
		 "env/cache/pdb-types.txt). --vftables: an FNVD_VFTABLE constant after its class for each exe vtable (ADDR: "
		 "only those) src has none at, by --globals' rules (DIR/declare-vftables.tsv). --layouts (no database): a "
		 "sizeof pin for each class the emit directory's "
		 "allocations build without one (`no size check for A`), where every site's size is srcdb's "
		 "(DIR/declare-layouts.tsv). --callees: as without it, for the functions an emit directory's bodies call "
		 "without a src declaration (its spots.tsv; ADDR: only those; DIR/declare-callees.tsv). --folds: for each "
		 "LIST line `ADDR<TAB>Y::g` (harness/tools/fold_members.py's folds.list), Y::g as a member of Y forwarding "
		 "to ADDR, another source function folded there; Y a class template instance: its explicit specialization "
		 "of the member, a copy of the instance's at ADDR (DIR/declare-folds.tsv). --fold-calls: the same for each "
		 "call of a class template instance's member on another instance of its template an emit directory's "
		 "bodies make (its spots.tsv), on the receiver's instance. --pdb-signatures (no database): "
		 "the slots of src's PDB-named functions whose type differs from the PDB's, each with the PDB's type and a "
		 "verdict apply/keep/spelling/open and its reason (DIR/pdb-signatures.tsv). --pdb-fields (no database): the "
		 "same for src's fields whose type differs from the PDB member's of their name (DIR/pdb-fields.tsv). "
		 "--retype TABLE: such a table's apply rows' types and rows' comments written into the declarations, "
		 "specializations and definitions, or a field table's into the members' declarations (an emitted one "
		 "listed in DIR/reemit.list; a field table's users in DIR/users.tsv), each change in DIR/applied.tsv, the "
		 "outcomes in DIR/retype.tsv. --revert ROW... | @FILE: those rows' changes in DIR/applied.tsv put back. "
		 "--unregister: the registrations an earlier run's DIR/declare.tsv `registered` rows (a class template "
		 "instance's member registered from the template's body; declare now specializes it) wrote for those "
		 "addresses taken out again (DIR/unregister.tsv). --hk-symbols TSV (no database): for each address "
		 "env/scratch/HkSyms/apply/symbols.tsv (harness/tools/havok-syms-apply.py tables) ties to one Havok library "
		 "symbol (ADDR: only those), src's declaration against the symbol's (name, owner, types with the SSE "
		 "headers' math types; the exe's ABI): DIR/hk-declare.tsv with verdict same/apply/disagree/pdb/open "
		 "and its reason; --apply writes the apply rows (declaration, forwarder, uses; DIR/hk-applied.tsv, "
		 "reemit.list, renames.tsv for harness/tools/ida_rename.py)",
		 runDeclare, {"apply", "place-specializations", "globals", "vftables", "layouts", "callees", "fold-calls",
					  "pdb-signatures", "pdb-fields", "revert", "unregister"}},
		{"emit",
		 "--idb I64 --model DB --aliases TSV --out DIR [--allocators TSV] [--debug-locals DB] [--skip-pass NAME]... "
		 "[--try-locals] ADDR... | @FILE",
		 "functions as the member definitions of their src declarations, from their ctree: clean where src knows "
		 "all they need, else with what is missing marked (DIR/<ADDR8>.cpp, emit.tsv, spots.tsv, check.list, "
		 "unfold-log.tsv; --model: srcdb's model, --aliases: harness/tools/ida_type_aliases.tsv of the same tree; "
		 "--allocators: the allocator table, env/cache/allocators.tsv (idakit allocators); --debug-locals: the debug "
		 "build's locals, env/cache/pdb-locals.sqlite (pdb_locals_xbox.py); --skip-pass: leave that "
		 "pass out, all: every pass; the pre-steps resolveTypes and resolveSelf always run); each function is "
		 "unfolded right before its decompile; --try-locals: a marked function's locals with root spots retyped by "
		 "trial first, a candidate kept where the root spots go down and no spot is new, the body printed under the "
		 "settings kept (DIR/local-trials.tsv, a row per trial; DIR/local-settings.jsonl, the changed functions' "
		 "whole lvar settings as pdb_locals.py writes them); never saves the database",
		 runEmit, {"try-locals"}},
		{"frames", "--idb I64 [--model DB] [--aliases TSV] [--out DIR]",
		 "every game function's sp trace made right, the same whatever ran before (the locals pass's first step): "
		 "purged bytes made the exe's pops, a function with a user stack point, an `and esp` frame or a call whose "
		 "recorded sp change is not its callee's purged bytes traced again, the realigned frames' points; a function "
		 "whose sp moved loses its user lvar settings (--model, --aliases: as sync's); DIR (default env/cache/frames) "
		 "gets frames-log.tsv, frames-changed.txt (pdb_locals.py --only's input), state.tsv and functions.tsv; saves "
		 "the database once, after the last step",
		 runFrames, {}},
		{"functions", "--idb I64 [--out TSV] [--names TSV]",
		 "every function, `0xADDR<TAB>0xEND<TAB>NAME`, into --out (default env/cache/ida/functions.tsv) and every "
		 "name, `0xADDR<TAB>NAME`, into --names (default env/cache/ida/ida-names.tsv); sync writes both under its "
		 "--out",
		 runFunctions, {}},
		{"infer", "--idb I64 --model DB --aliases TSV --out DIR [--hide] [ADDR... | @FILE]",
		 "the slot types of the functions src declares with IDA's guesses (ADDR: only those; no addresses: every "
		 "one), from the exe's evidence: the callers' arguments from evidence declarations bound each slot from "
		 "below, the body's uses from above, solved in rounds (a slot proven in one is evidence in the next): "
		 "DIR/infer.tsv (a row per slot: proven, guessed (a class pointer only the body's uses bound, no caller "
		 "contradicting), callers-only, open, inferred (src's `// Inferred type` has it), `this` as a note) and "
		 "DIR/retype.tsv (each proven slot and each guessed slot's change, as `declare --retype` reads it, with its "
		 "function's one `// Inferred type` or `// Guessed type` comment, tier proven or guessed; open where a "
		 "virtual's family would not change whole or an overload would become another's); --hide: the addresses' own "
		 "declarations hidden, as guesses, and each proposal compared with their types (infer.tsv's truth: "
		 "equal, chain, wrong); never saves the database",
		 runInfer, {"hide"}},
		{"locals", "--idb I64 [--model DB] [--aliases TSV] [--out DIR] [ADDR... | @FILE]",
		 "Unfold's local steps (merged arrays, frame locals, passed objects, constructed locals, value locals) on "
		 "functions (no addresses: every game function), on top of the lvar settings the database holds: each "
		 "function gains the user lvar settings they give and nothing else (--model, --aliases: as sync's); DIR "
		 "(default env/cache/ida/locals) gets locals-log.tsv and state.tsv; saves the database once, at the end",
		 runLocals, {}},
		{"merge",
		 "--model DB --emitted DIR --sources TSV [--aliases TSV] [--exclude ADDR | @FILE] [--apply] [ADDR... | @FILE]",
		 "emitted bodies into src/ (form src or marked; ADDR: only those; --exclude: not those): each replaces its "
		 "forwarder, a marked body for the hybrid build only; a row not merged (none, excluded, skipped) and an "
		 "excluded address outside the run whose src text is an emitted block gets its forwarder back, with src's "
		 "declaration's signature; so does such an address's plain src body, its registration removed (else "
		 "refused, merge.tsv saying why) (DIR/merged.txt, merge.tsv, merge-files.txt, with --apply merge-spans.tsv; "
		 "--sources: env/cache/pdb-function-sources.tsv; --aliases: default harness/tools/ida_type_aliases.tsv); "
		 "src/ written only with --apply",
		 runMerge, {"apply"}},
		{"sync", "--idb I64 [--model DB] [--aliases TSV] [--out DIR] [--hand-locals-to-pass]",
		 "src pushed into the database, deterministically and idempotently (--model: srcdb's model, default "
		 "env/cache/srcmodel.sqlite; --aliases: default harness/tools/ida_type_aliases.tsv); nothing per function "
		 "(stack points, Hex-Rays stores) is written: saves the database once, after the last phase, only where every "
		 "game function's lvar settings and stack points are as before the first (else exit 1, the first differing "
		 "functions on stdout); --hand-locals-to-pass: but the lvar settings of a function whose prototype it changed "
		 "or which calls one directly (never a stack point); DIR (default env/cache/ida) gets sync-log.tsv, "
		 "locals-lost.txt (those retyped functions and direct callers whose locals alone lost settings, with or "
		 "without the flag: clearlvars.py's and pdb_locals.py --fresh --only's input), state.tsv (the database's "
		 "state after the run) and the database's functions.tsv and ida-names.tsv (idakit functions')",
		 runSync, {"hand-locals-to-pass"}},
	};
	return table;
}

}  // namespace idakit
