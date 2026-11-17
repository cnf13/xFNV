#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

// A per-vertex data channel of a scene mesh: one vector per vertex. It overrides only the destructor.
class hkxVertexVectorDataChannel : public hkReferencedObject {
public:
    // The SDK's finish constructor: a loaded object keeps the fields the load wrote.
    hkxVertexVectorDataChannel(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_perVertexVectors(aFlag) {}

    ~hkxVertexVectorDataChannel() override;

    // The reflection class hkxVertexVectorDataChannelClass.
    static const hkClass& staticClass();

    hkArray<hkVector4> m_perVertexVectors;
};

extern hkClass hkxVertexVectorDataChannelClass;
extern hkTypeInfo hkxVertexVectorDataChannelTypeInfo;

// The member's destructor frees m_perVertexVectors.
inline hkxVertexVectorDataChannel::~hkxVertexVectorDataChannel() {}
