#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

// A user channel of a scene mesh section: the indices of the selected vertices. It overrides only the destructor.
class hkxVertexSelectionChannel : public hkReferencedObject {
public:
    // The SDK's finish constructor: a loaded object keeps the fields the load wrote.
    hkxVertexSelectionChannel(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_selectedVertices(aFlag) {}
    ~hkxVertexSelectionChannel() override;

    // The reflection class hkxVertexSelectionChannelClass.
    static const hkClass& staticClass();

    hkArray<int32_t> m_selectedVertices;
};

extern hkClass hkxVertexSelectionChannelClass;
extern hkTypeInfo hkxVertexSelectionChannelTypeInfo;

// The member's destructor frees m_selectedVertices.
inline hkxVertexSelectionChannel::~hkxVertexSelectionChannel() {}
