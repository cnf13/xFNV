"""ctypes binding of harness/exefacts (libexefacts.so): the exe facts idakit's sync and emit read, without
IDA, answered as draft.py and checkbody.World.throws do.

A missing or stale library (older than any file under harness/exefacts) is built on first use with CMake into
build/exefacts (clang, Release). A fact the Python would raise for raises ExefactsError.
"""

from __future__ import annotations

import ctypes
import fcntl
import functools
import os
import subprocess

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SOURCE = os.path.join(ROOT, "harness", "exefacts")
OUTPUT = os.path.join(ROOT, "build", "exefacts")
LIBRARY = os.path.join(OUTPUT, "libexefacts.so")

EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI = (1 << i for i in range(8))
FAMILIES = {"eax": EAX, "ebx": EBX, "ecx": ECX, "edi": EDI, "edx": EDX, "esi": ESI}


class ExefactsError(Exception):
    pass


class Abi(ctypes.Structure):
    _fields_ = [("ecx", ctypes.c_uint8), ("edx", ctypes.c_uint8), ("args", ctypes.POINTER(ctypes.c_int32)),
                ("arg_count", ctypes.c_uint32)]


class Accessor(ctypes.Structure):
    _fields_ = [("kind", ctypes.c_uint8), ("width", ctypes.c_uint8), ("extension", ctypes.c_int8),
                ("base", ctypes.c_int32), ("offset", ctypes.c_int32), ("pops", ctypes.c_uint32)]


class ReturnWidth(ctypes.Structure):
    _fields_ = [("width", ctypes.c_uint8), ("extension", ctypes.c_int8)]


class Empty(ctypes.Structure):
    _fields_ = [("empty", ctypes.c_uint8), ("pops", ctypes.c_uint32)]


class Pops(ctypes.Structure):
    _fields_ = [("known", ctypes.c_uint8), ("pops", ctypes.c_uint32)]


def _newest_source() -> float:
    return max(os.path.getmtime(os.path.join(d, f)) for d, _, files in os.walk(SOURCE) for f in files)


def build(force: bool = False) -> str:
    """Builds the library when missing, stale or forced; returns its path. Concurrent builders wait for each other."""
    os.makedirs(OUTPUT, exist_ok=True)
    with open(os.path.join(OUTPUT, ".lock"), "w") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX)
        if not force and os.path.exists(LIBRARY) and os.path.getmtime(LIBRARY) >= _newest_source():
            return LIBRARY
        if not os.path.exists(os.path.join(OUTPUT, "CMakeCache.txt")):
            subprocess.run(["cmake", "-S", SOURCE, "-B", OUTPUT, "-G", "Ninja", "-DCMAKE_CXX_COMPILER=clang++",
                            "-DCMAKE_BUILD_TYPE=Release"], check=True, stdout=subprocess.DEVNULL)
        subprocess.run(["cmake", "--build", OUTPUT], check=True, stdout=subprocess.DEVNULL)
        os.utime(LIBRARY)
        return LIBRARY


_lib: ctypes.CDLL | None = None
_context: int | None = None


def _open() -> tuple[ctypes.CDLL, int]:
    global _lib, _context
    if _context is None:
        import fn
        # the library reads env/cache/ida/functions.tsv: current with the canonical IDA database first
        fn.refresh_functions()
        lib = ctypes.CDLL(build())
        lib.exefacts_open.argtypes = [ctypes.c_char_p]
        lib.exefacts_open.restype = ctypes.c_void_p
        lib.exefacts_error.restype = ctypes.c_char_p
        out_types = {"abi": ctypes.POINTER(Abi), "pops": ctypes.POINTER(Pops),
                     "reg_args": ctypes.POINTER(ctypes.c_uint8), "clobbers": ctypes.POINTER(ctypes.c_uint8),
                     "variadic_fixed_args": ctypes.POINTER(ctypes.c_int32),
                     "frame_align": ctypes.POINTER(ctypes.c_uint32),
                     "instruction_sets": ctypes.POINTER(ctypes.c_uint8), "leaves_eax": ctypes.POINTER(ctypes.c_uint8),
                     "returns_eax": ctypes.POINTER(ctypes.c_uint8), "returns_this": ctypes.POINTER(ctypes.c_uint8),
                     "return_width": ctypes.POINTER(ReturnWidth),
                     "throws": ctypes.POINTER(ctypes.c_uint8), "trivial_accessor": ctypes.POINTER(Accessor),
                     "empty_function": ctypes.POINTER(Empty)}
        for name, out in out_types.items():
            fn = getattr(lib, "exefacts_" + name)
            fn.argtypes = [ctypes.c_void_p, ctypes.c_uint32, out]
            fn.restype = ctypes.c_int
        context = lib.exefacts_open(ROOT.encode())
        if not context:
            raise ExefactsError(lib.exefacts_error().decode())
        _lib, _context = lib, context
    return _lib, _context


def _query(name: str, address: int, out):
    lib, context = _open()
    if getattr(lib, "exefacts_" + name)(context, address, ctypes.byref(out)) != 0:
        raise ExefactsError(f"{name} {address:#x}: {lib.exefacts_error().decode()}")
    return out


def abi(address: int) -> dict:
    """draft.abi's ecx, edx (read before written) and args (the stack argument slots read, ascending)."""
    out = _query("abi", address, Abi())
    return {"ecx": bool(out.ecx), "edx": bool(out.edx), "args": [out.args[i] for i in range(out.arg_count)]}


def tail_pops(address: int) -> int | None:
    """The bytes the function pops (its `ret N`, else what its proven tail-jump targets agree on; a vtable-slot jump
    proves nothing here), None when unknown."""
    out = _query("pops", address, Pops())
    return out.pops if out.known else None


def asm_reg_args(address: int) -> str:
    """eax, ebx, edi, esi read before written, comma-separated, '' for none."""
    mask = _query("reg_args", address, ctypes.c_uint8()).value
    return ",".join(name for name, bit in FAMILIES.items() if mask & bit)


def usercall_clobbers(address: int, regs: list[str]) -> bool:
    """A callee-saved register among regs (ebx, esi, edi) written outside a push or pop."""
    mask = _query("clobbers", address, ctypes.c_uint8()).value
    return any(mask & FAMILIES[r] for r in regs if r in ("ebx", "esi", "edi"))


# Cached: the query decodes every caller of the function (about 1 s for a callee with thousands of callers), and
# checkbody asks it again each time it analyses a function calling it (several times a row)
@functools.lru_cache(maxsize=None)
def variadic_fixed_args(address: int) -> int | None:
    """The fixed stack arguments of a C-variadic cdecl function (callers clean up two or more different byte counts,
    it pops nothing and takes the address of its argument area: va_start), else None."""
    fixed = _query("variadic_fixed_args", address, ctypes.c_int32()).value
    return None if fixed < 0 else fixed


def asm_frame_align(address: int) -> int:
    """The alignment the prologue's `and esp` gives the frame, else 0."""
    return _query("frame_align", address, ctypes.c_uint32()).value


def instruction_sets(address: int) -> set[str]:
    """The instruction sets beyond the base one the function's code uses: 'x87', 'sse1', 'sse2' (SSE2 or later),
    'mmx'."""
    mask = _query("instruction_sets", address, ctypes.c_uint8()).value
    return {name for name, bit in (("x87", 1), ("sse1", 2), ("sse2", 4), ("mmx", 8)) if mask & bit}


def leaves_eax(address: int) -> bool:
    """Some return path sets eax."""
    return bool(_query("leaves_eax", address, ctypes.c_uint8()).value)


def returns_eax(address: int) -> bool:
    """Some return path sets eax to a value it does not read again before the ret (leaves_eax without the temporaries:
    a debug setter's `mov eax, [ebp-4]; mov [eax+X], ecx` keeps `this` in eax only to store through it)."""
    return bool(_query("returns_eax", address, ctypes.c_uint8()).value)


def returns_this(address: int) -> bool:
    """Every return path returns the entry ecx in eax (a constructor's `return this`)."""
    return bool(_query("returns_this", address, ctypes.c_uint8()).value)


def return_width(address: int) -> tuple[int, int] | None:
    """(width, extension): the bytes of eax every return path writes (4: mov/movzx/movsx eax; 1: mov al) and their
    extension (1 every definition a movzx, -1 every one a movsx, else 0); None where a path returns a call's eax or
    leaves eax unset."""
    out = _query("return_width", address, ReturnWidth())
    return (out.width, out.extension) if out.width else None


def throws(target: int | None) -> bool:
    """Every path of the function ends in a throw (as checkbody World.throws; memoised in query order)."""
    return target is not None and bool(_query("throws", target, ctypes.c_uint8()).value)


def trivial_accessor(address: int) -> dict | None:
    """The function as a trivial accessor (its whole body, after an optional `push ebp; mov ebp, esp` frame and
    `this` spill, sets eax to an object pointer plus a constant, loads one field there into eax, or one float field
    into ST0, or stores one stack argument into one field there, then returns), else None. load: the field at
    base + offset (width bytes; extension 1 movzx, -1 movsx, 0 none: 4 bytes, x87, or `mov al/ax`), in ST0 when x87,
    else in eax; store: that field = a stack argument's low width bytes; neither: eax = base + offset; base -1 is
    ecx (`this`), else the stack argument slot holding the object; pops: its `ret N`'s N."""
    out = _query("trivial_accessor", address, Accessor())
    if out.kind == 0:
        return None
    return {"load": out.kind in (2, 3), "x87": out.kind == 3, "store": out.kind == 4, "base": out.base,
            "offset": out.offset, "width": out.width, "extension": out.extension, "pops": out.pops}


def empty_function(address: int) -> int | None:
    """The function's `ret N`'s N when it is empty (its whole body, after an optional `push ebp; mov ebp, esp` frame
    and `this` spill, is the frame's teardown and the `ret`: nothing loaded, nothing written outside its own frame,
    every register as it was at entry), else None."""
    out = _query("empty_function", address, Empty())
    return out.pops if out.empty else None


if __name__ == "__main__":
    print(build(force=True))
