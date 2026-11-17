#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkResult.h"

#include <stdint.h>

void cleanupLoadedObjecthkxEnum(void* p);

// A scene attribute's mapping between integer values and names.
class hkxEnum : public hkReferencedObject {
public:
	struct Item {
		static const hkClass& staticClass();

		int32_t m_value;
		hkStringPtr m_name;
	};

	hkxEnum() = default;
	hkxEnum(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_items(aFlag) {}
	~hkxEnum() override;

	// The name of the item whose value is val, without its hkStringPtr flag bit; HK_FAILURE
	// when no item has it.
	hkResult getNameOfValue(int32_t val, const char** name) const;
	hkResult getValueOfName(const char* name, int32_t* val) const;

	static const hkClass& staticClass();

	hkArray<Item> m_items;
};

extern hkClass hkxEnumItemClass;
extern hkClass hkxEnumClass;
extern hkTypeInfo hkxEnumTypeInfo;

// The hkTypeInfo records of the reflected types declared here (constant data, the SDK's <Class>Class.cpp).
extern hkTypeInfo hkxEnumItemTypeInfo;
