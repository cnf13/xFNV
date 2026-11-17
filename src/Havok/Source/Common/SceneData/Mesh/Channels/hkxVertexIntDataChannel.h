#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

// A per-vertex data channel of a scene mesh: one int per vertex. It overrides only the destructor.
class hkxVertexIntDataChannel : public hkReferencedObject {
public:
    // The SDK's finish constructor: a loaded object keeps the fields the load wrote.
    hkxVertexIntDataChannel(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_perVertexInts(aFlag) {}
    ~hkxVertexIntDataChannel() override;

    // The reflection class hkxVertexIntDataChannelClass.
    static const hkClass& staticClass();

    hkArray<int32_t> m_perVertexInts;
};

extern hkClass hkxVertexIntDataChannelClass;
extern hkTypeInfo hkxVertexIntDataChannelTypeInfo;

// The member's destructor frees m_perVertexInts.
inline hkxVertexIntDataChannel::~hkxVertexIntDataChannel() {}
