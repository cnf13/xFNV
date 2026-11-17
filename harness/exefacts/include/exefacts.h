/* exefacts: what FalloutNV.exe's machine code says about a function's interface, as idakit's sync and emit read
 * it (no IDA). The binding is harness/tools/exefacts.py.
 *
 * Every fact function returns 0 with *out set, or -1 where today's Python raises (no answer; exefacts_error() says
 * why). Pointers into a context stay valid until exefacts_close. Not thread-safe. */
#ifndef EXEFACTS_H
#define EXEFACTS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ExefactsContext ExefactsContext;

/* Register families, as bits of a uint8_t */
enum {
    EXEFACTS_EAX = 1, EXEFACTS_ECX = 2, EXEFACTS_EDX = 4, EXEFACTS_EBX = 8,
    EXEFACTS_ESP = 16, EXEFACTS_EBP = 32, EXEFACTS_ESI = 64, EXEFACTS_EDI = 128,
};

/* Instruction sets beyond the base integer one, as bits of a uint8_t (capstone's groups; SSE2: SSE2 or later) */
enum {
    EXEFACTS_X87 = 1, EXEFACTS_SSE1 = 2, EXEFACTS_SSE2 = 4, EXEFACTS_MMX = 8,
};

typedef struct {
    uint8_t ecx;          /* ecx read before written: `this` */
    uint8_t edx;          /* edx read before written: a fastcall second argument */
    const int32_t* args;  /* the stack argument slots read (0: [entry esp + 4]), ascending */
    uint32_t arg_count;
} ExefactsAbi;

enum {
    EXEFACTS_ACCESSOR_NONE = 0,     /* not a trivial accessor */
    EXEFACTS_ACCESSOR_ADDRESS = 1,  /* eax = object + offset: `this`, `this + N`, `&field` */
    EXEFACTS_ACCESSOR_LOAD = 2,     /* eax = the field at object + offset */
    EXEFACTS_ACCESSOR_FLOAT = 3,    /* ST0 = the float field at object + offset (`fld`; eax left as the body left it) */
    EXEFACTS_ACCESSOR_STORE = 4,    /* the field at object + offset = a stack argument's low width bytes (a setter) */
};

/* A trivial accessor: the function's whole body, after an optional `push ebp; mov ebp, esp` frame and `this` spill,
   sets eax to an object pointer plus a constant, loads one field there into eax or one float field into ST0, or
   stores one stack argument into one field there, then returns */
typedef struct {
    uint8_t kind;      /* EXEFACTS_ACCESSOR_* */
    uint8_t width;     /* a load's or store's width in bytes: 1, 2 or 4 (FLOAT: 4, 8 or 10) */
    int8_t extension;  /* a load's extension: 1 movzx, -1 movsx, 0 none (4 bytes, or `mov al/ax`: eax's upper bytes
                          kept) */
    int32_t base;      /* the object: -1 ecx (`this`), else the stack argument slot holding it (0: [entry esp + 4]) */
    int32_t offset;
    uint32_t pops;     /* its `ret N`'s N */
} ExefactsAccessor;

/* The facts over the repository at root (its env/exe/FalloutNV.exe, env/cache/ida/functions.tsv and
   harness/tools/import_pops.tsv); NULL on failure */
ExefactsContext* exefacts_open(const char* root);
void exefacts_close(ExefactsContext* context);
/* The calling thread's last failure, "" for none */
const char* exefacts_error(void);

/* draft.abi's ecx, edx and args */
int exefacts_abi(ExefactsContext* context, uint32_t address, ExefactsAbi* out);
/* The bytes the function pops: its `ret N`, else what its proven tail-jump targets agree on (Facts::pops; a jump
   through a vtable slot proves nothing here, the C API has no slot resolver). known 0: unknown */
typedef struct ExefactsPops {
    uint8_t known;
    uint32_t pops;
} ExefactsPops;
int exefacts_pops(ExefactsContext* context, uint32_t address, ExefactsPops* out);
/* eax, ebx, esi, edi read before written: usercall register inputs */
int exefacts_reg_args(ExefactsContext* context, uint32_t address, uint8_t* out);
/* ebx, esi, edi written outside a push or pop */
int exefacts_clobbers(ExefactsContext* context, uint32_t address, uint8_t* out);
/* The fixed stack arguments of a C-variadic cdecl function, -1 for none */
int exefacts_variadic_fixed_args(ExefactsContext* context, uint32_t address, int32_t* out);
/* The alignment the prologue's `and esp` gives the frame, 0 for none */
int exefacts_frame_align(ExefactsContext* context, uint32_t address, uint32_t* out);
/* The instruction sets (EXEFACTS_X87, _SSE1, _SSE2, _MMX) the function's code uses, 0 where its decode fails */
int exefacts_instruction_sets(ExefactsContext* context, uint32_t address, uint8_t* out);
/* 1 when some return path sets eax */
int exefacts_leaves_eax(ExefactsContext* context, uint32_t address, uint8_t* out);
/* 1 when some return path sets eax to a value it does not read again before the ret (checkbody's VOID-ASM) */
int exefacts_returns_eax(ExefactsContext* context, uint32_t address, uint8_t* out);
/* 1 when every path returns the entry ecx in eax (a constructor's `return this`) */
int exefacts_returns_this(ExefactsContext* context, uint32_t address, uint8_t* out);
/* The bytes of eax every return path writes (width 0 when a path returns a call's eax, leaves eax unset or the decode
   fails) and their extension: 1 when each definition is a movzx, -1 when each is a movsx, 0 otherwise */
typedef struct ExefactsReturnWidth {
    uint8_t width;
    int8_t extension;
} ExefactsReturnWidth;
int exefacts_return_width(ExefactsContext* context, uint32_t address, ExefactsReturnWidth* out);
/* 1 when every path of the function ends in a throw (checkbody World.throws; memoised in query order) */
int exefacts_throws(ExefactsContext* context, uint32_t address, uint8_t* out);
/* The function as a trivial accessor, kind EXEFACTS_ACCESSOR_NONE when it is not one */
int exefacts_trivial_accessor(ExefactsContext* context, uint32_t address, ExefactsAccessor* out);
/* An empty function: the function's whole body, after an optional `push ebp; mov ebp, esp` frame and `this` spill,
   is the frame's teardown and the `ret`; nothing loaded, nothing written outside its own frame, every register as it
   was at entry */
typedef struct ExefactsEmpty {
    uint8_t empty;  /* 1 when the function is empty */
    uint32_t pops;  /* its `ret N`'s N */
} ExefactsEmpty;
/* The function as an empty function, empty 0 when it is not one */
int exefacts_empty_function(ExefactsContext* context, uint32_t address, ExefactsEmpty* out);

#ifdef __cplusplus
}
#endif

#endif
