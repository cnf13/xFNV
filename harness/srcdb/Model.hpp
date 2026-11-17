// srcdb's model of src: what clang knows about the reconstruction's types, functions and exe references, for tools
// that must not parse C++ (idakit sync: src's types into an IDA database).
//
// One SQLite database (Store.cpp has the schema), built by `srcdb index` from the hybrid build's TUs: the
// all-headers TU contributes every header's declarations, each src .cpp its own file's (registrations, forwarders,
// definitions, file-local types), and any TU the class template instances it completes. `file` is repo-relative
// (an instance's: its pattern's); booleans are 0/1; a type column holds a type key.
//
//   type        a type node by key; keys are structural and unique: `builtin:unsigned int`, `ptr(K)`, `lref(K)`,
//               `rref(K)`, `const(K)`, `volatile(K)`, `array(K,N)` (N -1: unbounded), `fn(CC;RET;P1,P2[,...];QUALS)`,
//               `memptr(CLASS,K)`, `vector(K,N)`, `record:NAME`, `enum:NAME`, `typedef:NAME`. Columns by kind:
//               size/signed/float (builtin), of (the child), count (array, vector), cc/ret/variadic/const (fn, its
//               parameters in type_param), class (memptr), name (record/enum/typedef: their rows by that name).
//   record      a complete record: line (its name's), end_line (its closing brace's), tag, size, align, data_size,
//               dynamic, abstract, trivial_copy (its copy assignment is a byte copy), vfptr (its own at 0), vbptr
//               (offset), primary_base, template (the pattern of an instance), external (declared outside src),
//               spelled (an instance's name without the arguments that equal their defaults; NULL when the same).
//               field (offset in bits; bits: a bit-field's width), base (offset, virtual), method (mangled),
//               using_member (mangled: a base's member function a using-declaration in the class brings into its
//               scope, one per using shadow, so the class's overload set of a name is its own methods of the name
//               and these), vtable (one per vfptr: offset in the complete object), vtable_path (the classes from the
//               one that introduced the vfptr), slot (kind method|deleting, function mangled, name, pure,
//               this_adjust, virtual_adjust).
//   pin         a static_assert that pins a record's sizeof to a constant (FNVD_LAYOUT): record, file, line, size.
//   enum        scoped, underlying; enumerator (name, value). A class template instance's own member enums are
//               rows of their own, by the instance's qualified name (`hkArrayBase<int>::(unnamed enum at ...)`).
//   typedef     target.
//   function    a declaration by MSVC mangled name: name, type (an fn key), class, this, static, virtual, pure,
//               const, structor (ctor|dtor), variadic, empty (a structor with no code: no vptr, no statements, no
//               written initialisers, its subobjects' trivial or empty; NULL for other functions), access (a
//               member's from namespace scope: the narrowest of its own and its enclosing classes', public|protected|
//               private; NULL for a free function), noreturn (clang's: `[[noreturn]]` or the like on a declaration);
//               param (names); default_args (required: the arguments a call must pass, for a function whose default
//               arguments let it pass fewer than its parameters).
//               provenance, the first that holds of the contiguous `//` comment lines right above its declaration or
//               its definition, and of their parameters: `inferred` (lines starting `// Inferred type` whose clauses
//               name every slot: `<slot>: <evidence>`, separated by `; `, the slot a parameter's name, `return` or
//               `parameter <k>`); `library` (a line starting `// Havok SDK signature (`: the declaration is a Havok
//               library symbol's, its every slot that symbol's types, as idakit declare --hk-symbols writes it);
//               `guess` (`// IDA name; not in the PDB.`, `// Not named in the PDB or IDA; name chosen here.` or
//               `// Havok SDK name; not in the PDB.` without the signature line, also after `// Not
//               reconstructed yet.`; parameters all named as IDA names them, `a1`, `arg4`, `ecx0`...; or no
//               parameters and an IDA placeholder name `sub_ADDR`); `pdb` (its qualified name is a pdb_function's);
//               `src`. A declaration a macro's body spells has no comment lines srcdb reads. The TUs write
//               function_marker rows (function, marker), which `srcdb index` turns into provenance and function_slot
//               and drops. function_slot: each slot a declaration's evidence covers (slot `ret`: a return other than
//               void and a record returned by value; a parameter's index from 0; a Construct's or DeletingDestruct's
//               first parameter and return, the porting forms, are none) and inferred (1: an `// Inferred type`
//               clause names it).
//               function_use: each expression of src that names a src function by its identifier (a call, its
//               address taken, a registration's argument; DeclRefExpr and MemberExpr, a dependent name in a template
//               none), at the name's spelling (a macro argument's; a macro's body spells none): function (mangled),
//               file, line, col (1-based, bytes). A tool renaming a function rewrites exactly these. Where the
//               expression is a call's callee (parentheses and implicit casts dropped): call_line, call_col,
//               call_end_line, call_end_col, the call's text in that file from its first byte to the byte after its
//               `)` (clang's file range of the whole call; inside one macro argument, its spelling there; NULL where
//               it maps to none, and for a use that is no callee). A tool wrapping a call rewrites exactly that.
//               A function template (free, or a member of a class that is no template) whose one template parameter is
//               its trailing function parameter pack (`template <class... A> int f(char*, A...)`) has a row by its
//               pattern key (below) as the C-variadic function of its other parameters, of a C-variadic function's
//               convention unless it spells one: a call passes the pack as `...` would. function_pattern: a class
//               template instance's member instantiated from a member of the template, by mangled name: the
//               member's pattern key (a row of its own, written by the first TU that writes the function). A pattern
//               key is `pattern:<qualified name> @<file>:<line>` of the template's member (its first declaration); a
//               forwarder inside such a member names it as its function.
//   abi         clang's i686-MSVC lowering of a function (CodeGen's CGFunctionInfo; no row where no TU saw its
//               parameter types complete): cc (cdecl|stdcall|fastcall|thiscall|vectorcall), ret (ABI kind),
//               sret_after_this, inalloca, stack_bytes (what the caller pushes, register arguments excluded; the
//               callee pops them under stdcall/thiscall); abi_arg (kind, inreg, bytes pushed; `this` first).
//   definition  a function body by mangled name: name, file, line, end_line, instance (1: a template's instance, whose
//               body is its template's definition, clang's to instantiate), emitted (1: inside the emitter's guard,
//               `#if FNVD_HYBRID  // emitted...` to `#endif  // emitted`); definition_param (names). An explicit
//               specialization's own definition replaces the template's body another TU wrote for its name (the
//               linker's strong definition; that TU did not see the specialization), and is no conflict.
//   specialization  each declaration src spells of an explicit specialization (of a class template instance's
//               member or of a function template), by mangled name: file, line (its `template <>`), end_line,
//               definition (it has the body), inline (the keyword written).
//   accessor    an inline trivial accessor by mangled name (a named, non-virtual instance method whose header body, a
//               template member's pattern's, is one return of a field of this): kind load (`return m_x;`), index
//               (`return m_x[param];`, param: the parameter's index) or address (`return &m_x;`); field.
//   steps       an inline function, by mangled name: a class template instance's member, or any other inline function
//               that is no template (a member, static or not, or a free function); its body as the exe inlines it
//               (the inline functions it calls expanded; a constructor's initialisers first), in a step language:
//               statements `(seq S...)`, `(if C T [E])`, `(ret X)`, `(let @aN X)` (an inline callee's argument that
//               loads, evaluated at the call; `@aN` reads that value), an expression; expressions `this`, `pN` (a
//               parameter: a reference one is its address), `#N` (a number, or an exe global's address), `(ld SIZE
//               A)`, `(+ A BYTES)`, `(set SIZE A X)`, `(pre+|pre-|post+|post- SIZE A)`, `(eq|ne|and|or X Y)`, `(not
//               X)`, `(atomic fetch_add|add_fetch|fetch_sub|sub_fetch A X)`, `(call "KEY" [THIS] ARGS...)` (an
//               out-of-line callee by function key), `(vcall BYTES THIS ARGS...)` (the slot at that offset of the
//               primary vtable), `(icall F ARGS...)` (through the function pointer F). No row for a body outside the
//               language.
//   ref         an exe address src names, by (file, line, col, kind, address): kind body | constructor |
//               destructor | address | forwarder | global | static_member | original_function | vftable | fold;
//               form: the macro after FNVD_REPLACES_ ("PLAIN" for FNVD_REPLACES), the forwarder's fnvd::original
//               function, the FNVD_ macro of a global, SAME_FUNCTION for a fold; target: the qualified name;
//               function: the mangled name of the function it names (a forwarder: of the function it sits in);
//               entry (VIRTUAL_VIA: the class the exe passes as this), type (a global's; a forwarder's call as an fn
//               key: its convention, result and the arguments it pushes; none for usercall or a call inside a
//               template), local, context (the function around it; a fold: the registered function's name as the
//               comment spells it). A fold is a function under RULES.md's `// Same function as X::f (0xADDR),
//               registered there.`: the same code as the function src registers at the address.
//               ref_param: a forwarder's arguments: the name of the function's own parameter it passes on (NULL
//               where none) and literal (a constant passed without a cast: its type says nothing of the exe's).
//               ref_location: where a usercall function takes each of its parameters (idx from 0, `this` -1):
//               location eax, ecx, edx, ebx, esi, edi or stack. A `fnvd::original::usercall` forwarder's, read from
//               its UserRegs designators, where it passes each parameter of its function once (the registers' and
//               the stack arguments' in the parameters' order); a FNVD_REPLACES_USERCALL/_USERPURGE registration's,
//               from its location list.
//   tu          per TU: ok (no error), errors (the first error lines), seconds, peak_kb.
//   conflict    a key two TUs wrote with different contents (the first one is kept).
//   pdb_function  a function of the Xbox debug PDB (Pdb.cpp; types as model keys, Xbox calling conventions
//               mapped to cdecl/thiscall): name (qualified), class (a member's), type, static, code_size;
//               pdb_param (its parameter names, `this` aside). One row per distinct name and type. pdb_local: each
//               variable its symbols name (S_REGREL32, S_BPREL32, S_REGISTER, S_LOCAL; nested blocks, parameters and
//               `this` included), in order: name, type.
//   pdb_field   each data member of the PDB's complete records: record, name, type (one row per distinct triple).
//   pdb_global  each global data symbol (S_GDATA32, S_LDATA32) and static data member (`Class::member`): name, type.
//   pdb_typed   a src declaration whose type is the one the PDB declares for it (Index.cpp's PdbTyped has the
//               match): kind param (key: the function's mangled name; item: a parameter's index from 0, `this`
//               aside, `ret`, or `this` for a member whose PDB functions are members of its class: type the class's
//               record key), field (key: the record; item: the field's name), global (key: the qualified name a
//               global or static_member ref targets; item empty), local (key: a src function's mangled name; item: a
//               local its PDB function declares, which src does not); type: the PDB's type key. Kind pair (key: a
//               src function's mangled name; item empty, or `hidden` where src's first parameter is the hidden result
//               pointer of the record the PDB's function returns, src's parameter k + 1 being the PDB's k; type: the
//               PDB function's fn key): the PDB function its param items compare with. Kind recorded (key: a src
//               function's mangled name; item: a slot as param's; type: the comment line): a slot whose type differs
//               from the PDB's for a reason of the exe's, which a `// PDB: <form>; the exe <fact>.` line above the
//               declaration or definition states (function_marker `pdb <slot> <line>` while indexing; the slot as
//               Model.cpp's markers reads it from the form). A slot differs from the PDB's where it has neither a
//               param nor a recorded item. Kind field_recorded (key: the record; item: the field's name; type: the
//               comment line): a field whose type differs from the PDB's for a reason a `// PDB:` line in the
//               comment lines right above it states (RULES' `// PDB: <Enum>`, or `// PDB: <form>; the exe <fact>.`;
//               field_marker rows while indexing). A field differs from the PDB's where it has neither a field nor a
//               field_recorded item.
#pragma once

#include "Store.hpp"

#include <clang/Basic/Diagnostic.h>
#include <clang/Frontend/FrontendAction.h>

#include <memory>
#include <string>
#include <vector>

namespace srcdb
{

struct IndexScope
{
	std::string root;                  // the checkout's real path, ending in '/'
	std::vector<std::string> collect;  // repo-relative prefixes of src's files ("src/", "gen/")
	bool headers = false;              // the all-headers TU: every collected file but the main one; else the main file
};

struct TuOutcome
{
	unsigned errors = 0;
	std::string errorText;  // the first error lines
};

// The action that parses one TU and writes its share of the model into store.
std::unique_ptr<clang::FrontendAction> makeIndexAction(Store& store, const IndexScope& scope);

// The diagnostics consumer that counts the TU's errors into outcome.
std::unique_ptr<clang::DiagnosticConsumer> makeIndexDiagnostics(TuOutcome& outcome);

}  // namespace srcdb
