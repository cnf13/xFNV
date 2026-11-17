#pragma once

#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkClass;
class hkClassMember;
class hkStringPtr;

// An object and the reflection record of its class, through which its members are reached by name.
class hkClassAccessor {
public:
	hkClassAccessor(void* object, const hkClass& klass);

	hkVariant m_variant;
};

// One member of a live object, found through the object's class record: its address and its member record. The
// exe calls every accessor out of line.
class hkClassMemberAccessor {
public:
	struct Vector4 {
		float r[4];
	};

	struct Matrix3 {
		Vector4 v[3];
	};

	struct Transform {
		Vector4 r[3];
		Vector4 t;
	};

	struct SimpleArray {
		void* data;
		int32_t size;
	};

	struct HomogeneousArray {
		const hkClass* klass;
		void* data;
		int32_t size;
	};

	// The hybrid's implicit default constructor: the sites declare the local, then construct it in place.
	hkClassMemberAccessor() = default;
	hkClassMemberAccessor(const hkVariant& v, const char* memberName);
	hkClassMemberAccessor(void* object, const hkClassMember* member);
	hkClassMemberAccessor(void* object, const hkClass& klass, const char* memberName);

	// Points the accessor at the member called memberName of object, an instance of klass; the address stays null
	// when klass has no such member.
	void connect(void* object, const hkClass& klass, const char* memberName);

	// KEEP: the forwarder stays (0xD87370). An exe caller reads ecx after the call, relying on the body leaving it
	// intact (the exe body never writes it), which clang does not guarantee.
	// m_member.
	const hkClassMember& getClassMember() const;

	// The member's address, m_address.
	// Havok SDK signature (hkBase.lib, tied by reloc).
	void* getAddress() const;

	// The pointer element index of the member (PDB asPointer; thiscall, ret 4, 0xD87210).
	void*& asPointer(int32_t index) const;

	// The variant element index of the member (PDB asVariant; thiscall, ret 4, 0xD87360).
	hkVariant& asVariant(int32_t index) const;

	// Real element index of the member.
	float& asReal(int32_t index) const;

	// Half element index of the member.
	hkHalf& asHalf(int32_t index) const;

	// The hkUlong element index of the member. KEEP, not registered (0xD872B0): the exe's caller (0xDBC370) reads edx
	// after the call, relying on the body leaving it intact (the exe body never writes it), which clang does not
	// guarantee.
	hkUlong& asUlong(int32_t index) const { return static_cast<hkUlong*>(m_address)[index]; }
	// C string element index of the member.
	char*& asCstring(int32_t index) const;

	// hkStringPtr element index of the member.
	hkStringPtr& asStringPtr(int32_t index) const;

	// Simple array element index of the member.
	SimpleArray& asSimpleArray(int32_t index) const;

	// Homogeneous array element index of the member.
	HomogeneousArray& asHomogeneousArray(int32_t index) const;

	// The member's object, with the member's class record (thiscall, ret 4, 0xD873F0). The exe returns the
	// hkClassAccessor through a hidden pointer, which object takes as its argument.
	hkClassAccessor* object(hkClassAccessor* arResult) const;
	// Points the accessor at member of object: m_address is object plus the member's offset
	// (PDB connect(void*, const hkClassMember*); thiscall, ret 8, 0xD87130).
	void connect(void* object, const hkClassMember* member);
	// Whether the accessor found its member: m_address is not null (PDB isOk; thiscall, ret 4, 0xD871E0). The exe
	// returns the hkBool through the hidden result pointer apResult.
	hkBool* isOk(hkBool* apResult) const;
	// Havok SDK signature (hkBase.lib, tied by reloc).
	void* asRaw() const;
	hkBool& asBool(int32_t index);
	// Havok SDK signature (hkBase.lib, tied by reloc).
	int32_t& asInt32(int32_t index) const;
	// Havok SDK signature (hkBase.lib, tied by reloc).
	uint32_t& asUint32(int32_t index) const;
	// Havok SDK signature (hkBase.lib, tied by reloc).
	int64_t& asInt64(int32_t index) const;
	// Havok SDK signature (hkBase.lib, tied by reloc).
	uint64_t& asUint64(int32_t index) const;
	// Havok SDK signature (hkBase.lib, tied by reloc).
	int16_t& asInt16(int32_t index) const;
	// Havok SDK signature (hkBase.lib, tied by reloc).
	uint16_t& asUint16(int32_t index) const;
	// Havok SDK signature (hkBase.lib, tied by reloc).
	int8_t& asInt8(int32_t index) const;
	// Havok SDK signature (hkBase.lib, tied by reloc).
	uint8_t& asUint8(int32_t index) const;
	hkClassMemberAccessor::Vector4& asVector4(int32_t index);
	hkClassMemberAccessor::Matrix3& asMatrix3(int32_t index);
	hkClassMemberAccessor::Transform& asTransform(int32_t index);
	hkClassMemberAccessor::Matrix3& asRotation(int32_t index);
	hkClassMemberAccessor* member(hkClassMemberAccessor* apResult, const char* name);

	void* m_address;
	const hkClassMember* m_member;
};

// asUlong is a KEEP body (above).
