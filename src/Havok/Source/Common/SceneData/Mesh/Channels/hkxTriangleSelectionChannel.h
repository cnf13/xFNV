#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

// A user channel of a scene mesh section: the indices of the selected triangles. It overrides only the destructor.
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

// The member's destructor frees m_selectedTriangles.
inline hkxTriangleSelectionChannel::~hkxTriangleSelectionChannel() {}
