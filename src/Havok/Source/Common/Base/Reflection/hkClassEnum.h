#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

#include <stdint.h>

class hkClassMember;
struct hkCustomAttributes;
class hkStreamWriter;

hkClass* getClassEnumItemClass();
hkClass* getClassEnumClass();

// Havok's reflection record of an enum: its name, its items (value and name pairs) and its attributes.
class hkClassEnum {
public:
	enum FlagValues : uint32_t {
		FLAGS_NONE = 0x0,
	};

	// One value of the enum and its name. The lookup walks m_items comparing m_value and returns the matching item's
	// m_name.
	class Item {
	public:
		// The member table hkClassEnumItemClass describes Item with.
		using MemberArray = const hkClassMember[2];

		int32_t m_value;
		const char* m_name;

		static MemberArray Members;
	};

	// hkClassEnum::Members: the member table hkClassEnumClass describes hkClassEnum with.
	using MemberArray = const hkClassMember[4];

	// The value of the attribute called name, or null when the enum has no attributes.
	const hkVariant* getAttribute(const char* id) const;
	uint32_t getSignature();
	// Havok SDK signature (hkBase.lib, tied by bytes,reloc).
	hkResult getNameOfValue(int32_t val, const char** name) const;
	// Havok SDK signature (hkBase.lib, tied by bytes,reloc).
	hkResult getValueOfName(const char* name, int32_t* val) const;
	// Havok SDK signature (hkBase.lib, tied by bytes,reloc).
	void writeSignature(hkStreamWriter* w) const;
	// Havok SDK signature (hkBase.lib, tied by bytes,reloc).
	hkResult decomposeFlags(int32_t flagValue, hkArray<const char*, hkContainerHeapAllocator>& bitsOut,
		int32_t& bitsOver) const;

	const char* m_name;
	const Item* m_items;
	int32_t m_numItems;
	hkCustomAttributes* m_attributes;
	hkFlags<FlagValues, uint32_t> m_flags;

	static MemberArray Members;
};

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkClassEnumItemTypeInfo;
extern hkTypeInfo hkClassEnumTypeInfo;
