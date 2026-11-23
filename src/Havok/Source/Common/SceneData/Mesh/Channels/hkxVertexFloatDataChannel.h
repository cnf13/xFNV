#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"

// A per-vertex data channel of a scene mesh: one float per vertex and what the floats measure. It overrides only the
// destructor.
class hkxVertexFloatDataChannel : public hkReferencedObject {
public:
    enum VertexFloatDimensions : int32_t {
        FLOAT = 0,
        DISTANCE = 1,
        ANGLE = 2,
    };

    // The SDK's finish constructor: a loaded object keeps the fields the load wrote.
    hkxVertexFloatDataChannel(hkFinishLoadedObjectFlag aFlag) : hkReferencedObject(aFlag), m_perVertexFloats(aFlag) {}
    ~hkxVertexFloatDataChannel() override;

    // The reflection class hkxVertexFloatDataChannelClass.
    static const hkClass& staticClass();

    hkArray<float> m_perVertexFloats;
    hkEnum<VertexFloatDimensions, uint8_t> m_dimensions;
};

extern hkClass hkxVertexFloatDataChannelClass;
extern hkTypeInfo hkxVertexFloatDataChannelTypeInfo;

// The member's destructor frees m_perVertexFloats.
inline hkxVertexFloatDataChannel::~hkxVertexFloatDataChannel() {}
