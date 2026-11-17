#pragma once

#include "Havok/Source/Common/Serialize/Data/hkDataObject.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkDataArrayImpl;

// Reference to a dynamically represented serialized array.
class hkDataArray {
public:
	enum Type : int32_t {
		TYPE_VOID = 0x0,
		TYPE_BYTE = 0x1,
		TYPE_INT = 0x2,
		TYPE_REAL = 0x3,
		TYPE_VEC_4 = 0x4,
		TYPE_VEC_8 = 0x5,
		TYPE_VEC_12 = 0x6,
		TYPE_VEC_16 = 0x7,
		TYPE_OBJECT = 0x8,
		TYPE_STRUCT = 0x9,
		TYPE_CSTRING = 0xA,
		TYPE_NUM_BASIC_TYPES = 0xB,
		TYPE_MASK_BASIC_TYPES = 0xF,
		TYPE_ARRAY = 0x10,
		TYPE_ARRAY_BYTE = 0x11,
		TYPE_ARRAY_INT = 0x12,
		TYPE_ARRAY_REAL = 0x13,
		TYPE_ARRAY_VEC_4 = 0x14,
		TYPE_ARRAY_VEC_8 = 0x15,
		TYPE_ARRAY_VEC_12 = 0x16,
		TYPE_ARRAY_VEC_16 = 0x17,
		TYPE_ARRAY_OBJECT = 0x18,
		TYPE_ARRAY_STRUCT = 0x19,
		TYPE_ARRAY_CSTRING = 0x1A,
		TYPE_TUPLE = 0x20,
		TYPE_TUPLE_BYTE = 0x21,
		TYPE_TUPLE_INT = 0x22,
		TYPE_TUPLE_REAL = 0x23,
		TYPE_TUPLE_VEC_4 = 0x24,
		TYPE_TUPLE_VEC_8 = 0x25,
		TYPE_TUPLE_VEC_12 = 0x26,
		TYPE_TUPLE_VEC_16 = 0x27,
		TYPE_TUPLE_OBJECT = 0x28,
		TYPE_TUPLE_STRUCT = 0x29,
		TYPE_TUPLE_CSTRING = 0x2A,
	};

	// One element of the array: the implementation and the element's index, which the assignments and accessors
	// pass to the implementation's virtuals (operator=(hkHalf), 0xDBAD40, calls m_impl's slot 15 with m_index).
	// Layout as the PC PDB's.
	struct Value {
		// The element's type.
		hkDataObject::Type getType() const;
		// The element as an int.
		int32_t asInt() const;
		// The element as a real.
		float asReal() const;
		// The element's nreal reals.
		const float* asVec(int32_t nreal) const;
		// Stores nreal reals.
		void setVec(const float* ra, int32_t nreal);
		// The element as a data object, into apResult; returns apResult.
		hkDataObject* asObject(hkDataObject* apResult) const;
		// The element as an array, into apResult: m_impl's asArray (slot 22) with m_index; returns apResult.
		hkDataArray* asArray(hkDataArray* apResult) const;
		// The element as a C string.
		const char* asString() const;

		// Stores an int, a real, a data object or a C string in the element.
		void operator=(int32_t aiValue);
		void operator=(float afValue);
		void operator=(const hkDataObject& arValue);
		void operator=(const char* apValue);
		// Stores a half: m_impl's setHalf
		// (slot 15) with m_index and h's dword as pushed.
		void setHalf(hkHalf h);
		void setInt64(int64_t i);

		hkDataArrayImpl* m_impl;
		int32_t m_index;
	};

	// A null array, safe to destroy: some sites declare the local, then a call fills it in (the by-value result).
	hkDataArray() : m_impl(nullptr) {}
	// Takes an external reference on a1.
	hkDataArray(hkDataArrayImpl* a1);
	// Releases m_impl's external reference, deleting it with the last reference.
	~hkDataArray();

	void clear();
	void reserve(int32_t n);
	// Stores the n values at a: each tail-jumps to m_impl's setAll overload of the same element type
	// (hkDataArrayImpl's slots 35 down to 24 in the PDB's order; ret 8 there): hkBool 0xDBADE0, char 0xDBADF0,
	// signed char 0xDBAE00, unsigned char 0xDBAE10, short 0xDBAE20, unsigned short 0xDBAE30, int 0xDBAE40, unsigned
	// int 0xDBAE50, __int64 0xDBAE60, unsigned __int64 0xDBAE70, hkReal 0xDBAE80, hkHalf 0xDBAE90.
	void setAll(const hkBool* a, int32_t n);
	void setAll(const char* a, int32_t n);
	void setAll(const int8_t* a, int32_t n);
	void setAll(const uint8_t* a, int32_t n);
	void setAll(const int16_t* a, int32_t n);
	void setAll(const uint16_t* a, int32_t n);
	void setAll(const int32_t* a, int32_t n);
	void setAll(const uint32_t* a, int32_t n);
	void setAll(const int64_t* a, int32_t n);
	void setAll(const uint64_t* a, int32_t n);
	void setAll(const float* a, int32_t n);
	void setAll(const hkHalf* a, int32_t n);
	// Havok SDK signature (hkSerialize.lib, tied by reloc).
	void setSize(int32_t n);
	// Havok SDK signature (hkSerialize.lib, tied by reloc).
	hkDataObject::Type getType() const;
	// The PDB's Value operator[](int i): m_impl and i (thiscall, ret 8, 0xDBAB90). src names it access because the
	// exe returns the Value through the hidden result pointer apResult.
	Value* access(Value* apResult, int32_t i);
	// The PDB's const Value operator[](int i) const: the same code as the non-const one, at its own address (thiscall,
	// ret 8, 0xDBABB0). Its callers read the element (Copier::copyIntoNativeArray, Writer::_writeArrayItems).
	Value* access(Value* apResult, int32_t i) const;
	// The PDB's getSize: tail-calls m_impl's getSize, slot 5 (thiscall, ret 0, 0xDBAC00).
	int32_t getSize() const;
	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	// Havok SDK: result through the hidden result pointer apResult.
	hkDataClass* getClass(hkDataClass* apResult) const;
	// Havok SDK signature (hkSerialize.lib, tied by bytes,reloc).
	// Havok SDK: result through the hidden result pointer apResult.
	hkDataArray* swizzleObjectMember(hkDataArray* apResult, const char* name) const;

	hkDataArrayImpl* m_impl;
};
