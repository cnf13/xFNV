#pragma once

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"

// Scene triangle indices in either 16-bit or 32-bit storage.
class hkxIndexBuffer : public hkReferencedObject {
public:
    enum IndexType : int32_t {
        INDEX_TYPE_INVALID = 0,
        INDEX_TYPE_TRI_LIST = 1,
        INDEX_TYPE_TRI_STRIP = 2,
        INDEX_TYPE_TRI_FAN = 3,
        INDEX_TYPE_MAX_ID = 4,
    };

    hkxIndexBuffer() = default;

    hkxIndexBuffer(hkFinishLoadedObjectFlag aFlag)
      : hkReferencedObject(aFlag), m_indices16(aFlag), m_indices32(aFlag) {}

    ~hkxIndexBuffer() override;

    int32_t getNumTriangles() const;
    bool getTriangleIndices(uint32_t triIndex, uint32_t& indexAOut, uint32_t& indexBOut, uint32_t& indexCOut);
    int32_t getNumTriangles();
    // The SDK's getTriangleIndices (hkxIndexBuffer.h).
    char getTriangleIndicesPtr(uint32_t triIndex, uint32_t* indexAOut, uint32_t* indexBOut, uint32_t* indexCOut);

    // The reflection class hkxIndexBufferClass.
    static const hkClass& staticClass();

    hkEnum<IndexType, int8_t> m_indexType;
    hkArray<uint16_t> m_indices16;
    hkArray<uint32_t> m_indices32;
    uint32_t m_vertexBaseOffset;
    uint32_t m_length;
};

extern hkClass hkxIndexBufferClass;

extern hkTypeInfo hkxIndexBufferTypeInfo;
