#pragma once

#include "Reconstruction.hpp"

#include <stdint.h>

class hkClass;
struct hkpRootCdPoint;

// The pointer apBase moved by aiOffset bytes. Out of line per pointer type in the exe: each instantiation src calls
// gets an explicit specialization forwarding to it, next to its caller's owner.
template <typename T> T* hkAddByteOffset(T* base, int32_t offset);

// Havok's one-byte boolean. The PDB has its two constructors (hkBool(), hkBool(bool)): the user-provided one makes it
// a class the exe returns through a hidden result pointer, also from free functions (worldOperationUtil's comparator
// 0xCB3F90 writes the byte through its first stack argument).
class hkBool {
public:
	// The two return types whose sizes tell hkIsPodType's test apart (hkTypeTraits.h).
	typedef char CompileTimeTrueType;
	typedef int32_t CompileTimeFalseType;

	hkBool() = default;
	// The byte store the exe inlines where it builds an hkBool from a bool (hkpCharacterProxy::integrateImplementation
	// 0xCADE20 passes one to updateManifold, hkCpuWorldRayCastJob 0xDF8DF0 assigns one); out of line it is Construct.
	constexpr hkBool(bool abValue) : m_bool(abValue) {}

	// Whether m_bool is nonzero. The exe's out-of-line copy (thiscall, ret 0, 0x5609F0) is registered in
	// TESObjectREFR.cpp.
	operator bool() const { return m_bool != 0; }

	// Havok's inline assignment from a bool: the exe stores the byte (hkMonitorStream::setStaticBuffer 0xCB0ED0
	// writes m_isBufferAllocatedOnTheHeap with `mov byte [esi+0x10], 0`); the PDB has no out-of-line copy.
	hkBool& operator=(bool e) {
		m_bool = static_cast<char>(e);
		return *this;
	}

	char m_bool;
};

// The compile-time boolean Havok's array utilities dispatch on (hkIsPodType<T> derives from it): hkTraitBool<1> for
// plain-data elements, hkTraitBool<0> for classes. An empty tag passed by value; the exe pushes a zero byte for it.
template <bool N> struct hkTraitBool {
	typedef hkTraitBool<N> type;
	enum {
		result = N
	};
};

// An enum stored in the integer type STORAGE. The exe calls the assignment out of line per instantiation: each
// instantiation src uses gets an explicit specialization forwarding to it, next to the enum's owner.
template <typename ENUM, typename STORAGE> class hkEnum {
public:
	// The SDK's inline conversion to the enum.
	operator ENUM() const { return static_cast<ENUM>(m_storage); }

	void operator=(ENUM e);

	STORAGE m_storage;
};

// A set of named bit flags stored in the integer type STORAGE. The SDK's accessors are header-inline and the exe
// inlines every one of them (the PDB has no hkFlags function); IDA types the instantiation as the one STORAGE byte.
template <typename BITS, typename STORAGE> class hkFlags {
public:
	// constexpr so that constant tables holding one (hkClassMember::m_flags) are constant-initialised.
	constexpr hkFlags() {}
	constexpr hkFlags(STORAGE s) { m_storage = s; }

	void clear() { m_storage = 0; }
	void setAll(STORAGE s) { m_storage = s; }
	void orWith(STORAGE s) { m_storage |= s; }
	void xorWith(STORAGE s) { m_storage ^= s; }
	void andWith(STORAGE s) { m_storage &= s; }
	void setWithMask(STORAGE s, STORAGE mask) { m_storage = static_cast<STORAGE>((m_storage & ~mask) | (s & mask)); }
	STORAGE get() const { return m_storage; }
	STORAGE get(STORAGE mask) const { return static_cast<STORAGE>(m_storage & mask); }
	bool anyIsSet(STORAGE mask) const { return (m_storage & mask) != 0; }
	bool allAreSet(STORAGE mask) const { return (m_storage & mask) == mask; }
	bool operator==(const hkFlags& f) const { return f.m_storage == m_storage; }
	bool operator!=(const hkFlags& f) const { return f.m_storage != m_storage; }

	STORAGE m_storage;
};

// A value padded for the SPU; on PC just the value. The exe calls the conversion out of line (the pointer-sized
// instantiations share 0x458B50): each instantiation src calls gets an explicit specialization forwarding to it, next
// to the member's owner.
template <typename T> class hkPadSpu {
public:
	operator T() const;
	// Stores t. Out of line per instantiation in the exe, like the conversion.
	void operator=(T e);

	T m_storage;
};
// hkLocalBuffer<char[512]>'s block pointer (hkFixedArray::m_data, read by its operator[] 0x5E05B0). The alias names
// the conversion's type, which a conversion-function-id cannot spell.
using hkCharBlock512Ptr = char (*)[512];
// The SDK's inline: stores the enum at STORAGE's width (0x537E90 stores a dword, 0x5407B0 a byte).
template <typename ENUM, typename STORAGE> void hkEnum<ENUM, STORAGE>::operator=(ENUM e) {
	m_storage = static_cast<STORAGE>(e);
}

// The SDK's inline: the value (0x458B50, shared by the pointer-sized instantiations, loads the dword).
template <typename T> hkPadSpu<T>::operator T() const {
	return m_storage;
}

// The SDK's inline: stores t (0x537E90 for the pointer-sized instantiations).
template <typename T> void hkPadSpu<T>::operator=(T e) {
	m_storage = e;
}

// The SDK's inline: apBase moved by aiOffset bytes (0x5E0B50 for char: add). The offset is in bytes, so the pointer
// is taken as untyped memory for the move, as the exe's add does.
template <typename T> T* hkAddByteOffset(T* base, int32_t offset) {
	return reinterpret_cast<T*>(reinterpret_cast<char*>(base) + offset);
}
// An unsigned float packed into one byte (the PDB lists it under TESObjectREFR.cpp, where its inlines were compiled).
class hkUFloat8 {
public:
	enum {
		MAX_VALUE = 0x100,
	};

	// The default constructor stays trivial: hkMotionState and hkContactPointMaterial hold the byte as a member.
	hkUFloat8() = default;
	hkUFloat8(const float f);

	// Packs the float into the nearest table value; returns this.
	hkUFloat8& operator=(const float& fv);
	// The real value, from hkUFloat8_intToReal (defined below the table). The exe's out-of-line copy (thiscall,
	// ret 0, ST0, 0x577BF0) is registered in TESObjectREFR.cpp.
	operator float() const;

	uint8_t m_value;
};

// The real value of each of the 256 packed values.
using hkUFloat8Table = float[hkUFloat8::MAX_VALUE];
inline hkUFloat8Table hkUFloat8_intToReal{0.0f, 0.01f, 0.02f, 0.03f, 0.04f, 0.05f, 0.06f, 0.07f, 0.075f, 0.08f, 0.085f,
	0.091f, 0.098f, 0.104f, 0.111f, 0.119f, 0.127f, 0.136f, 0.145f, 0.155f, 0.166f, 0.177f, 0.19f, 0.203f, 0.217f,
	0.231f, 0.247f, 0.264f, 0.282f, 0.302f, 0.323f, 0.345f, 0.368f, 0.394f, 0.421f, 0.45f, 0.481f, 0.514f, 0.549f,
	0.587f, 0.627f, 0.67f, 0.716f, 0.765f, 0.818f, 0.874f, 0.934f, 0.998f, 1.067f, 1.14f, 1.218f, 1.302f, 1.391f,
	1.487f, 1.589f, 1.698f, 1.815f, 1.939f, 2.072f, 2.215f, 2.367f, 2.529f, 2.703f, 2.889f, 3.087f, 3.299f, 3.526f,
	3.768f, 4.027f, 4.304f, 4.599f, 4.915f, 5.253f, 5.613f, 5.999f, 6.411f, 6.851f, 7.322f, 7.825f, 8.362f, 8.937f,
	9.551f, 10.21f, 10.91f, 11.66f, 12.46f, 13.31f, 14.23f, 15.2f, 16.25f, 17.37f, 18.56f, 19.83f, 21.2f, 22.65f,
	24.21f, 25.87f, 27.65f, 29.55f, 31.57f, 33.74f, 36.06f, 38.54f, 41.18f, 44.01f, 47.04f, 50.27f, 53.72f, 57.41f,
	61.35f, 65.57f, 70.07f, 74.88f, 80.03f, 85.52f, 91.4f, 97.68f, 104.4f, 111.6f, 119.2f, 127.4f, 136.2f, 145.5f,
	155.5f, 166.2f, 177.6f, 189.8f, 202.8f, 216.8f, 231.7f, 247.6f, 264.6f, 282.7f, 302.2f, 322.9f, 345.1f, 368.8f,
	394.1f, 421.2f, 450.1f, 481.1f, 514.1f, 549.4f, 587.2f, 627.5f, 670.6f, 716.6f, 765.9f, 818.5f, 874.7f, 934.8f,
	999.0f, 1068.0f, 1141.0f, 1219.0f, 1303.0f, 1393.0f, 1488.0f, 1590.0f, 1700.0f, 1816.0f, 1941.0f, 2074.0f, 2217.0f,
	2369.0f, 2532.0f, 2706.0f, 2892.0f, 3090.0f, 3303.0f, 3530.0f, 3772.0f, 4031.0f, 4308.0f, 4604.0f, 4920.0f, 5258.0f,
	5619.0f, 6005.0f, 6418.0f, 6858.0f, 7329.0f, 7833.0f, 8371.0f, 8946.0f, 9560.0f, 10217.0f, 10919.0f, 11669.0f,
	12470.0f, 13327.0f, 14242.0f, 15220.0f, 16266.0f, 17383.0f, 18577.0f, 19853.0f, 21217.0f, 22674.0f, 24231.0f,
	25896.0f, 27674.0f, 29575.0f, 31607.0f, 33778.0f, 36098.0f, 38577.0f, 41227.0f, 44059.0f, 47085.0f, 50319.0f,
	53775.0f, 57469.0f, 61416.0f, 65635.0f, 70143.0f, 74961.0f, 80110.0f, 85612.0f, 91493.0f, 97777.0f, 104493.0f,
	111670.0f, 119340.0f, 127538.0f, 136298.0f, 145660.0f, 155664.0f, 166356.0f, 177783.0f, 189994.0f, 203044.0f,
	216991.0f, 231895.0f, 247823.0f, 264846.0f, 283037.0f, 302478.0f, 323254.0f, 345457.0f, 369186.0f, 394544.0f,
	421644.0f, 450605.0f, 481556.0f, 514632.0f, 549980.0f, 587757.0f, 628128.0f, 671272.0f, 717379.0f, 766654.0f,
	819313.0f, 875589.0f, 935730.0f, 1000002.0f};

inline hkUFloat8::operator float() const {
	return hkUFloat8_intToReal[m_value];
}

// A 16-bit real: the top half of an IEEE single. Its user-provided constructor makes it a class return (hidden
// result pointer), as in the exe.
class hkHalf {
public:
	hkHalf() {}

	// The real value: the bits as the top half of a single, inline in the exe (movsx, shl 16).
	operator float() const {
		union {
			uint32_t ui;
			float f;
		} kBits;
		kBits.ui = static_cast<uint32_t>(static_cast<int32_t>(m_value)) << 16;
		return kBits.f;
	}

	int16_t m_value;
};

// An object and the reflection record of its class.
struct hkVariant {
	void* m_object;
	const hkClass* m_class;
};

// An unsigned integer the size of a pointer.
using hkUlong = uintptr_t;
// An unsigned 64-bit integer.
using hkUint64 = uint64_t;

// Not named in the PDB or IDA; names chosen here. The exe's shared float constants +HK_REAL_MAX (3.40282e+38f,
// 0x7F7FFFEE) and -HK_REAL_MAX, which its code loads with movss and splats to four lanes (an empty AABB's min and max).
inline float rdata_104DA48 = 3.40282e+38f;
inline float rdata_104E148 = -3.40282e+38f;

// The SDK's inline: of the pair (pairA, pairB), the one that is not a, by xor of the three addresses.
template <typename TYPE> inline TYPE hkSelectOther(TYPE a, TYPE pairA, TYPE pairB) {
	return reinterpret_cast<TYPE>(reinterpret_cast<hkUlong>(a) ^ reinterpret_cast<hkUlong>(pairA) ^
		reinterpret_cast<hkUlong>(pairB));
}

// The smallest power of two not below in (0 for 0): the SDK's inline, which hkArray's capacity shrinking uses.
inline uint32_t hkNextPowerOf2(uint32_t in) {
	in -= 1;

	in |= in >> 16;
	in |= in >> 8;
	in |= in >> 4;
	in |= in >> 2;
	in |= in >> 1;

	return in + 1;
}
