#include "Havok/Source/Common/SceneData/Mesh/hkxVertexBuffer.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"
#include "Havok/Source/Common/SceneData/Mesh/hkxVertexDescription.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkxVertexBufferVertexDataTypeInfo = {
    "hkxVertexBufferVertexData",
    finishLoadedObjecthkxVertexBufferVertexData,
    cleanupLoadedObjecthkxVertexBufferVertexData,
    nullptr,
    &typeid(hkxVertexBuffer::VertexData),
    sizeof(hkxVertexBuffer::VertexData)
};

// The reflected members of hkxVertexBuffer::VertexData.
using hkxVertexBuffer_VertexDataClass_MembersArray = const hkClassMember[11];
static hkxVertexBuffer_VertexDataClass_MembersArray hkxVertexBuffer_VertexDataClass_Members{
    {"vectorData", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_VECTOR4, 0, 0, 0x0, nullptr},
    {"floatData", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_REAL, 0, 0, 0xC, nullptr},
    {"uint32Data", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT32, 0, 0, 0x18, nullptr},
    {"uint16Data", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT16, 0, 0, 0x24, nullptr},
    {"uint8Data", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT8, 0, 0, 0x30, nullptr},
    {"numVerts", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x3C, nullptr},
    {"vectorStride", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x40, nullptr},
    {"floatStride", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x44, nullptr},
    {"uint32Stride", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x48, nullptr},
    {"uint16Stride", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x4C, nullptr},
    {"uint8Stride", nullptr, nullptr, hkClassMember::TYPE_UINT32, hkClassMember::TYPE_VOID, 0, 0, 0x50, nullptr}
};
hkClass hkxVertexBufferVertexDataClass(
    "hkxVertexBufferVertexData",
    nullptr,
    0x54,
    nullptr,
    0,
    nullptr,
    0,
    hkxVertexBuffer_VertexDataClass_Members,
    11,
    nullptr,
    nullptr,
    0,
    0
);
hkxVertexBuffer::MembersArray hkxVertexBuffer::Members{
    {"data", &hkxVertexBufferVertexDataClass, nullptr, hkClassMember::TYPE_STRUCT, hkClassMember::TYPE_VOID, 0, 0, 0x8,
     nullptr},
    {"desc", &hkxVertexDescriptionClass, nullptr, hkClassMember::TYPE_STRUCT, hkClassMember::TYPE_VOID, 0, 0, 0x5C,
     nullptr}
};
hkClass hkxVertexBufferClass(
    "hkxVertexBuffer",
    &hkReferencedObjectClass,
    0x68,
    nullptr,
    0,
    nullptr,
    0,
    hkxVertexBuffer::Members,
    2,
    nullptr,
    nullptr,
    0,
    1
);
hkTypeInfo hkxVertexBufferTypeInfo = {
    "hkxVertexBuffer",          finishLoadedObjecthkxVertexBuffer, cleanupLoadedObjecthkxVertexBuffer,
    getVtablehkxVertexBuffer(), &typeid(hkxVertexBuffer),          sizeof(hkxVertexBuffer)
};

// The members' destructors free the five data arrays.
hkxVertexBuffer::VertexData::~VertexData() {}

void hkxVertexBuffer::VertexData::clear() {
    m_vectorData.m_size = 0;
    m_floatData.m_size = 0;
    m_uint32Data.m_size = 0;
    m_uint16Data.m_size = 0;
    m_uint8Data.m_size = 0;
    m_numVerts = 0;
    m_vectorStride = 0;
    m_floatStride = 0;
    m_uint32Stride = 0;
    m_uint16Stride = 0;
    m_uint8Stride = 0;
}

void hkxVertexBuffer::VertexData::setSize(int32_t n) {
    uint32_t m_uint8Stride = this->m_uint8Stride;
    m_numVerts = n;
    int32_t v4 = n * m_uint8Stride;
    hkArray<uint8_t, hkContainerHeapAllocator>* p_m_uint8Data = &m_uint8Data;
    hkMemoryAllocator* m_heap = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v7 = p_m_uint8Data->m_capacityAndFlags & hkArrayBase<uint8_t>::CAPACITY_MASK;
    if (v7 < v4) {
        int32_t v8 = 2 * v7;
        if (v4 >= v8) {
            v8 = v4;
        }
        hkArrayUtil::_reserve(m_heap, p_m_uint8Data, v8, sizeof(uint8_t));
    }
    p_m_uint8Data->m_size = v4;
    int32_t v9 = n * m_uint16Stride >> 1;
    hkMemoryAllocator* v10 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v11 = m_uint16Data.m_capacityAndFlags & hkArrayBase<uint16_t>::CAPACITY_MASK;
    if (v11 < v9) {
        int32_t v12 = 2 * v11;
        if (v9 >= v12) {
            v12 = v9;
        }
        hkArrayUtil::_reserve(v10, &m_uint16Data, v12, sizeof(uint16_t));
    }
    m_uint16Data.m_size = v9;
    int32_t v13 = n * m_uint32Stride >> 2;
    hkMemoryAllocator* v14 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v15 = m_uint32Data.m_capacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK;
    if (v15 < v13) {
        int32_t v16 = 2 * v15;
        if (v13 >= v16) {
            v16 = v13;
        }
        hkArrayUtil::_reserve(v14, &m_uint32Data, v16, sizeof(uint32_t));
    }
    m_uint32Data.m_size = v13;
    int32_t v17 = n * m_floatStride >> 2;
    hkMemoryAllocator* v18 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v19 = m_floatData.m_capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK;
    if (v19 < v17) {
        int32_t v20 = 2 * v19;
        if (v17 >= v20) {
            v20 = v17;
        }
        hkArrayUtil::_reserve(v18, &m_floatData, v20, sizeof(float));
    }
    m_floatData.m_size = v17;
    int32_t v21 = n * m_vectorStride >> 4;
    hkMemoryAllocator* v22 = (&hkMemoryRouter::getInstance())->m_heap;
    int32_t v23 = m_vectorData.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
    if (v23 < v21) {
        int32_t v24 = 2 * v23;
        if (v21 >= v24) {
            v24 = v21;
        }
        hkArrayUtil::_reserve(v22, &m_vectorData, v24, sizeof(hkVector4));
    }
    m_vectorData.m_size = v21;
}

const hkClass& hkxVertexBuffer::VertexData::staticClass() {
    return hkxVertexBufferVertexDataClass;
}

// The members' destructors free the description's and the vertex data's arrays.
hkxVertexBuffer::~hkxVertexBuffer() {}

void hkxVertexBuffer::setNumVertices(int32_t n, const hkxVertexDescription& format) {
    if (n == static_cast<int32_t>(m_data.m_numVerts) && format == m_desc) {
        return;
    }
    m_desc.m_decls.m_size = 0;
    m_data.clear();
    for (const hkxVertexDescription::ElementDecl& srcDecl : format.m_decls) {
        hkxVertexDescription::ElementDecl& decl = m_desc.m_decls.expandOne();
        decl.m_type.m_storage = srcDecl.m_type.m_storage;
        decl.m_usage.m_storage = srcDecl.m_usage.m_storage;
        switch (srcDecl.m_type.m_storage) {
            case hkxVertexDescription::HKX_DT_UINT8:
                decl.m_byteOffset = m_data.m_uint8Stride;
                m_data.m_uint8Stride += 4;
                break;
            case hkxVertexDescription::HKX_DT_INT16:
                decl.m_byteOffset = m_data.m_uint16Stride;
                m_data.m_uint16Stride += 4;
                break;
            case hkxVertexDescription::HKX_DT_UINT32:
                decl.m_byteOffset = m_data.m_uint32Stride;
                m_data.m_uint32Stride += 4;
                break;
            case hkxVertexDescription::HKX_DT_FLOAT:
                decl.m_byteOffset = m_data.m_floatStride;
                m_data.m_floatStride += 4;
                break;
            case hkxVertexDescription::HKX_DT_FLOAT2:
                decl.m_byteOffset = m_data.m_floatStride;
                m_data.m_floatStride += 8;
                break;
            case hkxVertexDescription::HKX_DT_FLOAT3:
            case hkxVertexDescription::HKX_DT_FLOAT4:
                decl.m_byteOffset = m_data.m_vectorStride;
                m_data.m_vectorStride += 16;
                break;
            default: break;
        }
    }
    for (hkxVertexDescription::ElementDecl& decl : m_desc.m_decls) {
        switch (decl.m_type.m_storage) {
            case hkxVertexDescription::HKX_DT_UINT8: decl.m_byteStride = m_data.m_uint8Stride; break;
            case hkxVertexDescription::HKX_DT_INT16: decl.m_byteStride = m_data.m_uint16Stride; break;
            case hkxVertexDescription::HKX_DT_UINT32: decl.m_byteStride = m_data.m_uint32Stride; break;
            case hkxVertexDescription::HKX_DT_FLOAT:
            case hkxVertexDescription::HKX_DT_FLOAT2: decl.m_byteStride = m_data.m_floatStride; break;
            case hkxVertexDescription::HKX_DT_FLOAT3:
            case hkxVertexDescription::HKX_DT_FLOAT4: decl.m_byteStride = m_data.m_vectorStride; break;
            default: break;
        }
    }
    // EXE-INLINED 0xE3E250 (hkxVertexBuffer::VertexData::setSize)
    m_data.m_numVerts = n;
    int32_t iNumUint8 = n * m_data.m_uint8Stride;
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    int32_t iCapacity = m_data.m_uint8Data.m_capacityAndFlags & hkArrayBase<uint8_t>::CAPACITY_MASK;
    if (iCapacity < iNumUint8) {
        int32_t iNewCapacity = 2 * iCapacity;
        if (iNumUint8 >= iNewCapacity) {
            iNewCapacity = iNumUint8;
        }
        hkArrayUtil::_reserve(*pHeap, &m_data.m_uint8Data, iNewCapacity, sizeof(uint8_t));
    }
    m_data.m_uint8Data.m_size = iNumUint8;
    int32_t iNumUint16 = n * m_data.m_uint16Stride >> 1;
    pHeap = hkMemoryRouter::getInstance().m_heap;
    iCapacity = m_data.m_uint16Data.m_capacityAndFlags & hkArrayBase<uint16_t>::CAPACITY_MASK;
    if (iCapacity < iNumUint16) {
        int32_t iNewCapacity = 2 * iCapacity;
        if (iNumUint16 >= iNewCapacity) {
            iNewCapacity = iNumUint16;
        }
        hkArrayUtil::_reserve(*pHeap, &m_data.m_uint16Data, iNewCapacity, sizeof(uint16_t));
    }
    m_data.m_uint16Data.m_size = iNumUint16;
    int32_t iNumUint32 = n * m_data.m_uint32Stride >> 2;
    pHeap = hkMemoryRouter::getInstance().m_heap;
    iCapacity = m_data.m_uint32Data.m_capacityAndFlags & hkArrayBase<uint32_t>::CAPACITY_MASK;
    if (iCapacity < iNumUint32) {
        int32_t iNewCapacity = 2 * iCapacity;
        if (iNumUint32 >= iNewCapacity) {
            iNewCapacity = iNumUint32;
        }
        hkArrayUtil::_reserve(*pHeap, &m_data.m_uint32Data, iNewCapacity, sizeof(uint32_t));
    }
    m_data.m_uint32Data.m_size = iNumUint32;
    int32_t iNumFloats = n * m_data.m_floatStride >> 2;
    pHeap = hkMemoryRouter::getInstance().m_heap;
    iCapacity = m_data.m_floatData.m_capacityAndFlags & hkArrayBase<float>::CAPACITY_MASK;
    if (iCapacity < iNumFloats) {
        int32_t iNewCapacity = 2 * iCapacity;
        if (iNumFloats >= iNewCapacity) {
            iNewCapacity = iNumFloats;
        }
        hkArrayUtil::_reserve(*pHeap, &m_data.m_floatData, iNewCapacity, sizeof(float));
    }
    m_data.m_floatData.m_size = iNumFloats;
    int32_t iNumVectors = n * m_data.m_vectorStride >> 4;
    pHeap = hkMemoryRouter::getInstance().m_heap;
    iCapacity = m_data.m_vectorData.m_capacityAndFlags & hkArrayBase<hkVector4>::CAPACITY_MASK;
    if (iCapacity < iNumVectors) {
        int32_t iNewCapacity = 2 * iCapacity;
        if (iNumVectors >= iNewCapacity) {
            iNewCapacity = iNumVectors;
        }
        hkArrayUtil::_reserve(*pHeap, &m_data.m_vectorData, iNewCapacity, sizeof(hkVector4));
    }
    m_data.m_vectorData.m_size = iNumVectors;
}

const void* hkxVertexBuffer::getVertexDataPtr(const hkxVertexDescription::ElementDecl& elem) const {
    return const_cast<hkxVertexBuffer*>(this)->getVertexDataPtr(elem);
}

// The element's data: the array its type is stored in, plus its byte offset (the arrays are indexed as untyped bytes).
void* hkxVertexBuffer::getVertexDataPtr(const hkxVertexDescription::ElementDecl& elem) {
    switch (elem.m_type.m_storage) {
        case hkxVertexDescription::HKX_DT_UINT8:
            return reinterpret_cast<char*>(m_data.m_uint8Data.m_data) + elem.m_byteOffset;
        case hkxVertexDescription::HKX_DT_INT16:
            return reinterpret_cast<char*>(m_data.m_uint16Data.m_data) + elem.m_byteOffset;
        case hkxVertexDescription::HKX_DT_UINT32:
            return reinterpret_cast<char*>(m_data.m_uint32Data.m_data) + elem.m_byteOffset;
        case hkxVertexDescription::HKX_DT_FLOAT:
        case hkxVertexDescription::HKX_DT_FLOAT2:
            return reinterpret_cast<char*>(m_data.m_floatData.m_data) + elem.m_byteOffset;
        case hkxVertexDescription::HKX_DT_FLOAT3:
        case hkxVertexDescription::HKX_DT_FLOAT4:
            return reinterpret_cast<char*>(m_data.m_vectorData.m_data) + elem.m_byteOffset;
        default: return nullptr;
    }
}

void hkxVertexBuffer::copy(const hkxVertexBuffer& other, bool resize) {
    hkxVertexDescription* p_m_desc = &m_desc;
    char v5 = m_desc == other.m_desc;
    if (resize) {
        setNumVertices(static_cast<int32_t>(other.m_data.m_numVerts), *p_m_desc);
    }
    if (v5) {
        uint32_t m_numVerts = other.m_data.m_numVerts;
        if (m_numVerts >= m_data.m_numVerts) {
            m_numVerts = m_data.m_numVerts;
        }
        uint32_t m_uint8Stride = m_data.m_uint8Stride;
        if (m_uint8Stride) {
            hkString::memCpy(m_data.m_uint8Data.begin(), other.m_data.m_uint8Data.m_data, m_numVerts * m_uint8Stride);
        }
        uint32_t m_uint16Stride = m_data.m_uint16Stride;
        if (m_uint16Stride) {
            hkString::memCpy(
                m_data.m_uint16Data.begin(), other.m_data.m_uint16Data.m_data, m_numVerts * m_uint16Stride
            );
        }
        uint32_t m_uint32Stride = m_data.m_uint32Stride;
        if (m_uint32Stride) {
            hkString::memCpy(
                m_data.m_uint32Data.begin(), other.m_data.m_uint32Data.m_data, m_numVerts * m_uint32Stride
            );
        }
        uint32_t m_floatStride = m_data.m_floatStride;
        if (m_floatStride) {
            hkString::memCpy(m_data.m_floatData.begin(), other.m_data.m_floatData.m_data, m_numVerts * m_floatStride);
        }
        uint32_t m_vectorStride = m_data.m_vectorStride;
        if (m_vectorStride) {
            hkString::memCpy(
                m_data.m_vectorData.begin(), other.m_data.m_vectorData.m_data, m_numVerts * m_vectorStride
            );
        }
    }
    else {
        for (uint32_t i = 0; i < other.m_data.m_numVerts; ++i) {
            copyVertex(other, i, i);
        }
    }
}

// With equal descriptions it copies one stride of each array from the arrays' starts (not the vertices' own data),
// deliberately, as the game does; otherwise it copies each element other has with the same usage index and type.
// KEEP: the usage count array is indexed by the element's usage value (0xE3E13B, [esp+eax*4+0x1c]); a usage above
// HKX_DU_USERDATA reads past the 257 counts into the exe's frame, an uninitialised read.
void hkxVertexBuffer::copyVertex(const hkxVertexBuffer& other, int32_t vertFrom, int32_t vertTo) {
    bool bSameDesc = m_desc == other.m_desc;
    int32_t usageCount[hkxVertexDescription::HKX_DU_USERDATA + 1];
    hkString::memSet(usageCount, 0, sizeof(usageCount));
    if (vertTo >= static_cast<int32_t>(m_data.m_numVerts) ||
        vertFrom >= static_cast<int32_t>(other.m_data.m_numVerts)) {
        return;
    }
    if (bSameDesc) {
        if (m_data.m_uint8Stride) {
            hkString::memCpy(
                m_data.m_uint8Data.m_data, other.m_data.m_uint8Data.m_data, static_cast<int32_t>(m_data.m_uint8Stride)
            );
        }
        if (m_data.m_uint16Stride) {
            hkString::memCpy(
                m_data.m_uint16Data.m_data, other.m_data.m_uint16Data.m_data,
                static_cast<int32_t>(m_data.m_uint16Stride)
            );
        }
        if (m_data.m_uint32Stride) {
            hkString::memCpy(
                m_data.m_uint32Data.m_data, other.m_data.m_uint32Data.m_data,
                static_cast<int32_t>(m_data.m_uint32Stride)
            );
        }
        if (m_data.m_floatStride) {
            hkString::memCpy(
                m_data.m_floatData.m_data, other.m_data.m_floatData.m_data, static_cast<int32_t>(m_data.m_floatStride)
            );
        }
        if (m_data.m_vectorStride) {
            hkString::memCpy(
                m_data.m_vectorData.m_data, other.m_data.m_vectorData.m_data,
                static_cast<int32_t>(m_data.m_vectorStride)
            );
        }
        return;
    }
    for (const hkxVertexDescription::ElementDecl& decl : m_desc.m_decls) {
        const hkxVertexDescription::ElementDecl* pOtherDecl = other.m_desc.getElementDecl(
            static_cast<hkxVertexDescription::DataUsage>(decl.m_usage.m_storage), usageCount[decl.m_usage.m_storage]
        );
        if (pOtherDecl && decl.m_type.m_storage == pOtherDecl->m_type.m_storage) {
            ++usageCount[decl.m_usage.m_storage];
            char* pDst = static_cast<char*>(getVertexDataPtr(decl));
            const char* pSrc = static_cast<const char*>(other.getVertexDataPtr(*pOtherDecl));
            int32_t iSize = 0;
            switch (decl.m_type.m_storage) {
                case hkxVertexDescription::HKX_DT_UINT8:
                case hkxVertexDescription::HKX_DT_INT16:
                case hkxVertexDescription::HKX_DT_UINT32:
                case hkxVertexDescription::HKX_DT_FLOAT: iSize = 4; break;
                case hkxVertexDescription::HKX_DT_FLOAT2: iSize = 8; break;
                case hkxVertexDescription::HKX_DT_FLOAT3:
                case hkxVertexDescription::HKX_DT_FLOAT4: iSize = 16; break;
                default: break;
            }
            hkString::memCpy(pDst + vertTo * decl.m_byteStride, pSrc + vertFrom * pOtherDecl->m_byteStride, iSize);
        }
    }
}

const hkClass& hkxVertexBuffer::staticClass() {
    return hkxVertexBufferClass;
}
