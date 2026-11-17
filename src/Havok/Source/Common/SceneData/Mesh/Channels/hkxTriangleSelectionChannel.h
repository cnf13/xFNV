#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Reconstruction.hpp"

// A user channel of a scene mesh section: the indices of the selected triangles (the SDK's member; the exe's hkClass
// hkxTriangleSelectionChannelClass has it at 0x8 and an object size of 0x14). It overrides only the destructor.
class hkxTriangleSelectionChannel : public hkReferencedObject {
public:
	// The SDK's finish constructor: a loaded object keeps the fields the load wrote.
	hkxTriangleSelectionChannel(hkFinishLoadedObjectFlag aFlag)
		: hkReferencedObject(aFlag), m_selectedTriangles(aFlag) {}

	~hkxTriangleSelectionChannel() override;

	// The reflection class hkxTriangleSelectionChannelClass.
	static const hkClass& staticClass();

	hkArray<int32_t> m_selectedTriangles;
};

extern hkClass hkxTriangleSelectionChannelClass;
extern hkTypeInfo hkxTriangleSelectionChannelTypeInfo;
inline constexpr fnvd::Vtable hkxTriangleSelectionChannel_vftable{};

// The member's destructor frees m_selectedTriangles.
inline hkxTriangleSelectionChannel::~hkxTriangleSelectionChannel() {}
