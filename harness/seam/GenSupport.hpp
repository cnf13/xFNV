#pragma once

// Support for emitted bodies with unresolved spots (idakit emit's gen form): gen::Unwind, the IDA types src does not
// model (gen::Blob), FNVD_AT and Hex-Rays' helpers. Such a body is hybrid-only in src, under
// `#if FNVD_HYBRID  // emitted` (docs/IDAKIT.md), and so is its include of this header.
//
// It is compiled for the hybrid build only (clang-cl, i686, /O2, -fno-delete-null-pointer-checks). clang-cl compiles
// with relaxed aliasing (MSVC compatibility, no -fstrict-aliasing), so a typed load through a reinterpreted pointer is
// a plain load: FNVD_AT needs no memcpy and no volatile.

#include <cstdint>
#include <cstring>

namespace gen {

// One exe unwind-map state's cleanup. The exe's C++ EH frame runs, while state k is live in [ebp-4], the action
// funclets of k, toState(k), ... down to -1. A generated body declares one Unwind per state s (ascending, so they
// are destroyed in descending state order) whose action runs only when the body's gen_state is a state whose chain
// contains s; it sets gen_state where the exe's state is known (before each call) and to -1 before every normal
// return, so the cleanups run only while an exception unwinds through the frame (clang emits plain cleanup
// entries: nothing is caught, and an exception nobody catches is not unwound, as with the exe's
// __CxxFrameHandler3).
template <class F>
struct Unwind {
	F action;
	explicit Unwind(F f) : action(f) {}
	~Unwind() { action(); }
	Unwind(const Unwind&) = delete;
	Unwind& operator=(const Unwind&) = delete;
};

} // namespace gen

#ifndef GEN_SUPPORT_UNWIND_ONLY // the host unit test (ehtest/) takes only gen::Unwind
#include "Reconstruction.hpp"

// The object of type T at byte offset off from base (any pointer or integer address); an lvalue.
#define FNVD_AT(T, base, off) (*(T*)((uint8_t*)(base) + (off)))

namespace gen {

// A value of a type src does not model (an IDA struct by value): exactly N bytes, trivially copyable, passed and
// copied as MSVC passes a POD of that size.
template <unsigned N>
struct alignas(N % 4 == 0 ? 4 : (N % 2 == 0 ? 2 : 1)) Blob {
	uint8_t bytes[N];
};

// An integer wider than 8 bytes (IDA's `__int128`, `unsigned __int128`, `_OWORD`: Hex-Rays reads the exe's 16-byte
// moves as numbers), which i686 C++ has none of: N little-endian bytes, made from and read as an integer as C++
// converts integers of two widths (extended by the source's signedness, truncated), and shifted. The conversions take
// any integer as it is, so that the shifts (members, an exact match) are never ambiguous with C++'s own.
template <unsigned N, bool Signed>
struct Wide {
	static_assert(N % 4 == 0 && N > 8);
	uint32_t words[N / 4];

	template <class T>
	static constexpr bool integer = std::is_integral_v<T> || std::is_enum_v<T>;

	Wide() = default;
	template <class T, std::enable_if_t<integer<T>, int> = 0>
	Wide(T x)
	{
		int64_t value = (int64_t)x;
		bool extend = (std::is_enum_v<T> || std::is_signed_v<T>) && value < 0;
		for (unsigned k = 0; k < N / 4; ++k)
			words[k] = k * 4 < sizeof(T) ? (uint32_t)((uint64_t)value >> (32 * k)) : extend ? 0xFFFFFFFFu : 0;
	}
	template <class T, std::enable_if_t<integer<T>, int> = 0>
	operator T() const
	{
		if constexpr (std::is_same_v<T, bool>)
		{
			for (uint32_t word : words)
				if (word)
					return true;
			return false;
		}
		else
			return (T)(words[0] | (uint64_t)words[1] << 32);
	}
	Wide operator<<(int n) const
	{
		Wide out{};
		for (int bit = 0; bit + n < (int)(8 * N); ++bit)
			if (words[bit / 32] >> (bit % 32) & 1)
				out.words[(bit + n) / 32] |= 1u << ((bit + n) % 32);
		return out;
	}
	Wide operator>>(int n) const
	{
		bool negative = Signed && (int32_t)words[N / 4 - 1] < 0;
		Wide out{};
		for (int bit = 0; bit < (int)(8 * N); ++bit)
			if (bit + n < (int)(8 * N) ? words[(bit + n) / 32] >> ((bit + n) % 32) & 1 : negative)
				out.words[bit / 32] |= 1u << (bit % 32);
		return out;
	}
};

// A temporary named as an lvalue for the rest of its full-expression: a raw member read of a call's result takes its
// address (`FNVD_AT(T, &gen::lvalue(_mm_shuffle_ps(a, b, 170)), 0x0)`).
template <class T>
inline T& lvalue(T&& x)
{
	return x;
}

// x's bytes stored into the object dst, as many as x has (Hex-Rays' assignment of a value to an object of an abstract
// class, which C++ has no value of: the exe's store of those bytes).
template <class To, class From>
inline To& store(To& dst, const From& x)
{
	static_assert(sizeof(From) <= sizeof(To));
	__builtin_memcpy(static_cast<void*>(&dst), static_cast<const void*>(&x), sizeof(From));
	return dst;
}

// A global read as another type than the one declared at its address (a src global Hex-Rays reads as another type,
// or two emitted bodies of one file reading one exe global as different types): the object of type T there.
template <class T, class U>
inline T& as(U& object)
{
	return *(T*)&object;
}

// Pointer arithmetic in bytes (Hex-Rays scales by the IDA pointee size, which src types may not share).
template <class P>
inline P padd(P p, int32_t bytes)
{
	return (P)((uint8_t*)p + bytes);
}
template <class P>
inline P& padd_assign(P& p, int32_t bytes)
{
	p = (P)((uint8_t*)p + bytes);
	return p;
}
template <class P>
inline P ppostadd(P& p, int32_t bytes)
{
	P old = p;
	p = (P)((uint8_t*)p + bytes);
	return old;
}

// Compound assignments whose operation's signedness differs from the left side's declared type.
template <class T>
inline T& sshr_assign(T& x, int n)
{
	using S = std::make_signed_t<T>;
	x = (T)((S)x >> n);
	return x;
}
template <class T>
inline T& ushr_assign(T& x, int n)
{
	using U = std::make_unsigned_t<T>;
	x = (T)((U)x >> n);
	return x;
}
template <class T, class V>
inline T& sdiv_assign(T& x, V v)
{
	using S = std::make_signed_t<T>;
	x = (T)((S)x / (S)v);
	return x;
}
template <class T, class V>
inline T& udiv_assign(T& x, V v)
{
	using U = std::make_unsigned_t<T>;
	x = (T)((U)x / (U)v);
	return x;
}
template <class T, class V>
inline T& smod_assign(T& x, V v)
{
	using S = std::make_signed_t<T>;
	x = (T)((S)x % (S)v);
	return x;
}
template <class T, class V>
inline T& umod_assign(T& x, V v)
{
	using U = std::make_unsigned_t<T>;
	x = (T)((U)x % (U)v);
	return x;
}

// Hex-Rays partial-register helpers (defs.h): rvalue forms read the bytes of x's object representation (little
// endian, as the exe's memory and registers hold it, so a float or a struct gives its bits, not a converted value);
// _ref forms name the bytes in place. The signed forms (SLOBYTE, ...) are these, cast to the signed type.
template <class R, unsigned Off, class T>
inline R part(const T& x)
{
	static_assert(Off + sizeof(R) <= sizeof(T));
	R out;
	__builtin_memcpy(&out, (const uint8_t*)&x + Off, sizeof(R));
	return out;
}
template <class T>
inline uint8_t lobyte(T x)
{
	return part<uint8_t, 0>(x);
}
template <class T>
inline uint8_t hibyte(T x)
{
	return part<uint8_t, sizeof(T) - 1>(x);
}
template <int K, class T>
inline uint8_t byten(T x)
{
	return part<uint8_t, K>(x);
}
template <class T>
inline uint16_t loword(T x)
{
	return part<uint16_t, 0>(x);
}
template <class T>
inline uint16_t hiword(T x)
{
	return part<uint16_t, sizeof(T) - 2>(x);
}
template <int K, class T>
inline uint16_t wordn(T x)
{
	return part<uint16_t, 2 * K>(x);
}
template <class T>
inline uint32_t lodword(T x)
{
	return part<uint32_t, 0>(x);
}
template <class T>
inline uint32_t hidword(T x)
{
	return part<uint32_t, sizeof(T) - 4>(x);
}
template <int K, class T>
inline uint32_t dwordn(T x)
{
	return part<uint32_t, 4 * K>(x);
}
template <class T>
inline uint8_t& lobyte_ref(T& x)
{
	return ((uint8_t*)&x)[0];
}
template <class T>
inline uint8_t& hibyte_ref(T& x)
{
	return ((uint8_t*)&x)[sizeof(T) - 1];
}
template <int K, class T>
inline uint8_t& byten_ref(T& x)
{
	return ((uint8_t*)&x)[K];
}
template <class T>
inline uint16_t& loword_ref(T& x)
{
	return ((uint16_t*)&x)[0];
}
template <class T>
inline uint16_t& hiword_ref(T& x)
{
	return *(uint16_t*)((uint8_t*)&x + sizeof(T) - 2);
}
template <int K, class T>
inline uint16_t& wordn_ref(T& x)
{
	return ((uint16_t*)&x)[K];
}
template <class T>
inline uint32_t& lodword_ref(T& x)
{
	return ((uint32_t*)&x)[0];
}
template <class T>
inline uint32_t& hidword_ref(T& x)
{
	return ((uint32_t*)&x)[1];
}
template <int K, class T>
inline uint32_t& dwordn_ref(T& x)
{
	return ((uint32_t*)&x)[K];
}

inline uint64_t pair64(uint32_t high, uint32_t low)
{
	return ((uint64_t)high << 32) | low;
}
inline uint32_t pair32(uint16_t high, uint16_t low)
{
	return ((uint32_t)high << 16) | low;
}
inline uint16_t pair16(uint8_t high, uint8_t low)
{
	return (uint16_t)(((uint32_t)high << 8) | low);
}

template <class T>
inline T rol(T x, int n)
{
	using U = std::make_unsigned_t<T>;
	constexpr int bits = 8 * sizeof(T);
	n &= bits - 1;
	return (T)(((U)x << n) | ((U)x >> ((bits - n) & (bits - 1))));
}
template <class T>
inline T ror(T x, int n)
{
	using U = std::make_unsigned_t<T>;
	constexpr int bits = 8 * sizeof(T);
	n &= bits - 1;
	return (T)(((U)x >> n) | ((U)x << ((bits - n) & (bits - 1))));
}

// The sign bit (__SETS__), carry of an add (__CFADD__), overflow of a subtract/add (__OFSUB__, __OFADD__).
template <class T>
inline uint8_t sets(T x)
{
	return (uint8_t)((std::make_signed_t<T>)x < 0);
}
template <class T, class U>
inline uint8_t cfadd(T x, U y)
{
	using V = std::make_unsigned_t<T>;
	return (uint8_t)((V)((V)x + (V)y) < (V)x);
}
template <class T, class U>
inline uint8_t ofsub(T x, U y)
{
	using S = std::make_signed_t<T>;
	S r = (S)((std::make_unsigned_t<T>)x - (std::make_unsigned_t<T>)y);
	return (uint8_t)((((S)x ^ (S)y) & ((S)x ^ r)) < 0);
}
template <class T, class U>
inline uint8_t ofadd(T x, U y)
{
	using S = std::make_signed_t<T>;
	S r = (S)((std::make_unsigned_t<T>)x + (std::make_unsigned_t<T>)y);
	return (uint8_t)(((~((S)x ^ (S)y)) & ((S)x ^ r)) < 0);
}

// The carry of a left shift (__CFSHL__): the last bit shifted out of x's width.
template <class T>
inline uint8_t cfshl(T x, int n)
{
	using U = std::make_unsigned_t<T>;
	return (uint8_t)(((U)x >> (8 * sizeof(T) - n)) & 1);
}

// Bit reinterpretation (COERCE_FLOAT and friends). An x87 value (a double) coerced to a 4-byte type is the bits the
// exe's `fstp dword` stores: the value rounded to a float.
template <class To, class From>
inline To coerce(From x)
{
	if constexpr (sizeof(To) != sizeof(From) && std::is_floating_point_v<From> && sizeof(To) == sizeof(float))
		return coerce<To>(static_cast<float>(x));
	else
	{
		static_assert(sizeof(To) == sizeof(From));
		To out;
		__builtin_memcpy(&out, &x, sizeof(To));
		return out;
	}
}

// Hex-Rays' qmemcpy and memset32: the exe's inline `rep movs` and `rep stosd`. Inline asm, because clang
// turns a copy or fill loop into a call to the DLL's CRT (memcpy, memset), which the exe does not make. qmemcpy saves
// and loads esi itself (its inputs in eax and edx): clang refuses an asm operand in esi where esi is its base pointer
// (a realigned frame with a dynamic stack), "Interference usage of base pointer/frame pointer".
inline void* qmemcpy(void* dst, const void* src, uint32_t count)
{
	__asm__ volatile("pushl %%esi\n\t"
					 "pushl %%edi\n\t"
					 "movl %%edx, %%esi\n\t"
					 "movl %%eax, %%edi\n\t"
					 "rep movsb\n\t"
					 "popl %%edi\n\t"
					 "popl %%esi"
					 : "+c"(count)
					 : "a"(dst), "d"(src)
					 : "memory");
	return dst;
}
inline void* memset32(void* dst, uint32_t value, uint32_t count)
{
	void* d = dst;
	__asm__ volatile("rep stosl" : "+D"(d), "+c"(count) : "a"(value) : "memory");
	return dst;
}

// abs32: the exe's branchless `cdq; xor; sub` (INT32_MIN stays INT32_MIN, as there)
inline int32_t abs32(int32_t x)
{
	return (int32_t)(x < 0 ? 0u - (uint32_t)x : (uint32_t)x);
}

// __SETP__: the parity flag of x - y (set when the low byte of the result has an even number of 1 bits)
template <class T, class U>
inline uint8_t setp(T x, U y)
{
	return (uint8_t)!__builtin_parity((uint8_t)(x - y));
}

// JUMPOUT: Hex-Rays' jump to an exe address outside the function (a tail jump it did not resolve), as a call of that
// address. Not what the exe does (the target runs on this function's frame); always a marked spot.
inline void jumpout(uint32_t address)
{
	((void (*)())(uintptr_t)address)();
}

// A value's bytes as a type C++ does not convert it to (a record, an SSE vector, gen::Blob; idakit emit's bits
// conversion), the bytes past a narrower value zero (Hex-Rays' `(__m128)x` of a dword: movd zero-extends).
template <class To, class From>
inline To bitcast(const From& x)
{
	static_assert(sizeof(From) <= sizeof(To));
	unsigned char bytes[sizeof(To)] = {};
	__builtin_memcpy(bytes, static_cast<const void*>(&x), sizeof(From));
	To out;
	__builtin_memcpy(static_cast<void*>(&out), bytes, sizeof(To));
	return out;
}
// A volatile object's bytes (a slot the exe reuses, which the decompile reads as volatile), read byte by byte.
template <class To, class From>
inline To bitcast(const volatile From& x)
{
	static_assert(sizeof(From) <= sizeof(To));
	unsigned char bytes[sizeof(To)] = {};
	const volatile unsigned char* from = reinterpret_cast<const volatile unsigned char*>(&x);
	for (size_t k = 0; k < sizeof(From); ++k)
		bytes[k] = from[k];
	To out;
	__builtin_memcpy(static_cast<void*>(&out), bytes, sizeof(To));
	return out;
}

// A dword zero-extended into a wider value: an xmm slot written by `movss/movd xmm, dword; movaps [slot], xmm`.
template <class To>
inline To zext32(uint32_t x)
{
	static_assert(sizeof(To) >= sizeof(x));
	To out{};
	__builtin_memcpy(&out, &x, sizeof(x));
	return out;
}

// fabs/fsqrt (Hex-Rays: fabs(x), sqrt(x)).
inline double fabs_(double x)
{
	return __builtin_fabs(x);
}
inline double sqrt_(double x)
{
	return __builtin_sqrt(x);
}

// Interlocked operations (lock-prefixed instructions in the exe).
inline int32_t interlocked_increment(volatile int32_t* p)
{
	return __atomic_add_fetch(p, 1, __ATOMIC_SEQ_CST);
}
inline int32_t interlocked_decrement(volatile int32_t* p)
{
	return __atomic_sub_fetch(p, 1, __ATOMIC_SEQ_CST);
}
inline int32_t interlocked_exchange(volatile int32_t* p, int32_t v)
{
	return __atomic_exchange_n(p, v, __ATOMIC_SEQ_CST);
}
inline int32_t interlocked_exchange_add(volatile int32_t* p, int32_t v)
{
	return __atomic_fetch_add(p, v, __ATOMIC_SEQ_CST);
}
inline int32_t interlocked_compare_exchange(volatile int32_t* p, int32_t exchange, int32_t comparand)
{
	__atomic_compare_exchange_n(p, &comparand, exchange, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
	return comparand;
}

// float -> int the way an inline `fld; fistp` does it: in the current x87 rounding mode (nearest by default), not
// the truncation of a C++ cast (_ftol2)
inline int32_t fist32(double v)
{
	int32_t r;
	__asm__ volatile("fistpl %0" : "=m"(r) : "t"(v) : "st");
	return r;
}
inline int64_t fist64(double v)
{
	int64_t r;
	__asm__ volatile("fistpll %0" : "=m"(r) : "t"(v) : "st");
	return r;
}

} // namespace gen

#endif // GEN_SUPPORT_UNWIND_ONLY
