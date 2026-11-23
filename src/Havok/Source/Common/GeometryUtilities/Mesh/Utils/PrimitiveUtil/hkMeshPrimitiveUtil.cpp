#include "Havok/Source/Common/GeometryUtilities/Mesh/Utils/PrimitiveUtil/hkMeshPrimitiveUtil.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/LocalArray/hkLocalArray.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Memory/Util/Stack/hkStackMemory.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/hkMeshShape.h"

void hkMeshPrimitiveUtil::appendTriangleIndices(
    hkMeshSection::PrimitiveType primType,
    int32_t numVertices,
    int32_t indexBase,
    hkArrayBase<uint16_t>& indicesOut,
    hkMemoryAllocator& a
) {
    if (primType == hkMeshSection::PRIMITIVE_TYPE_TRIANGLE_LIST) {
        uint16_t* v19 = indicesOut._expandBy(a, numVertices);
        for (int32_t i = 0; i < numVertices; ++i) {
            v19[i] = i + indexBase;
        }
    }
    else {
        if (primType == hkMeshSection::PRIMITIVE_TYPE_TRIANGLE_STRIP) {
            uint16_t* v14;
            int32_t v6 = 3 * numVertices - 6;
            int32_t v10 = 2;
            for (uint16_t* j = indicesOut._expandBy(a, v6); v10 < numVertices; j = v14 + 1) {
                int16_t v12 = indexBase + v10;
                *j = indexBase + v10 - 2;
                uint16_t* v13 = j + 1;
                if ((v10 & 1) != 0) {
                    *v13 = v12--;
                }
                else {
                    *v13 = v12 - 1;
                }
                v14 = v13 + 1;
                *v14 = v12;
                ++v10;
            }
        }
    }
}

void hkMeshPrimitiveUtil::appendTriangleIndices(
    hkMeshSection::PrimitiveType primType,
    int32_t numVertices,
    int32_t indexBase,
    hkArray<uint32_t, hkContainerHeapAllocator>& indicesOut
) {
    if (primType == hkMeshSection::PRIMITIVE_TYPE_TRIANGLE_LIST) {
        uint32_t* v21 = indicesOut.expandBy(numVertices);
        for (int32_t i = 0; i < numVertices; ++i) {
            v21[i] = static_cast<uint16_t>(indexBase + i);
        }
    }
    else {
        if (primType == hkMeshSection::PRIMITIVE_TYPE_TRIANGLE_STRIP) {
            int32_t v4 = 3 * numVertices - 6;
            uint32_t* v10 = indicesOut.expandBy(v4);
            if (numVertices > 2) {
                int32_t v14;
                int32_t v11 = indexBase + 1;
                int32_t v12 = 1 - indexBase;
                do {
                    int32_t* v15;
                    *v10 = v11 - 1;
                    int32_t* v13 = reinterpret_cast<int32_t*>(v10 + 1);
                    v14 = v11 + 1;
                    if ((static_cast<uint8_t>(v12) + static_cast<uint8_t>(v11) & 1) != 0) {
                        *v13 = v14;
                        v15 = v13 + 1;
                        *v15 = v11;
                    }
                    else {
                        *v13 = v11;
                        v15 = v13 + 1;
                        *v15 = v14;
                    }
                    ++v11;
                    v10 = reinterpret_cast<uint32_t*>(v15 + 1);
                }
                while (v12 + v14 < numVertices);
            }
        }
    }
}

// Appends the triangles of a list or strip of 32-bit indices, each offset by indexBase: a list is copied (with
// hkString::memCpy when there is no offset), a strip unwound into one triangle per index after the first two, every
// odd one with its second and third corners swapped to keep the winding.
void hkMeshPrimitiveUtil::appendTriangleIndices32(
    hkMeshSection::PrimitiveType primType,
    const uint32_t* srcIndices,
    int32_t numIndices,
    int32_t indexBase,
    hkArray<uint32_t, hkContainerHeapAllocator>& indicesOut
) {
    if (primType == hkMeshSection::PRIMITIVE_TYPE_TRIANGLE_LIST) {
        uint32_t* pOut = indicesOut.expandBy(numIndices);
        if (indexBase) {
            for (int32_t i = 0; i < numIndices; ++i) {
                pOut[i] = srcIndices[i] + indexBase;
            }
        }
        else {
            hkString::memCpy(pOut, srcIndices, numIndices * static_cast<int32_t>(sizeof(uint32_t)));
        }
    }
    else if (primType == hkMeshSection::PRIMITIVE_TYPE_TRIANGLE_STRIP) {
        const int32_t numNew = 3 * numIndices - 6;
        uint32_t* pOut = indicesOut.expandBy(numNew);
        for (int32_t i = 2; i < numIndices; ++i) {
            const uint32_t* pIn = &srcIndices[i - 2];
            pOut[0] = pIn[0] + indexBase;
            if (i & 1) {
                pOut[1] = pIn[2] + indexBase;
                pOut[2] = pIn[1] + indexBase;
            }
            else {
                pOut[1] = pIn[1] + indexBase;
                pOut[2] = pIn[2] + indexBase;
            }
            pOut += 3;
        }
    }
}

// appendTriangleIndices32's form for 16-bit indices.
void hkMeshPrimitiveUtil::appendTriangleIndices16(
    hkMeshSection::PrimitiveType primType,
    const uint16_t* srcIndices,
    int32_t numIndices,
    int32_t indexBase,
    hkArray<uint16_t, hkContainerHeapAllocator>& indicesOut
) {
    if (primType == hkMeshSection::PRIMITIVE_TYPE_TRIANGLE_LIST) {
        uint16_t* pOut = indicesOut.expandBy(numIndices);
        if (indexBase) {
            for (int32_t i = 0; i < numIndices; ++i) {
                pOut[i] = static_cast<uint16_t>(srcIndices[i] + indexBase);
            }
        }
        else {
            hkString::memCpy(pOut, srcIndices, numIndices * static_cast<int32_t>(sizeof(uint16_t)));
        }
    }
    else if (primType == hkMeshSection::PRIMITIVE_TYPE_TRIANGLE_STRIP) {
        const int32_t numNew = 3 * numIndices - 6;
        uint16_t* pOut = indicesOut.expandBy(numNew);
        for (int32_t i = 2; i < numIndices; ++i) {
            const uint16_t* pIn = &srcIndices[i - 2];
            pOut[0] = static_cast<uint16_t>(pIn[0] + indexBase);
            if (i & 1) {
                pOut[1] = static_cast<uint16_t>(pIn[2] + indexBase);
                pOut[2] = static_cast<uint16_t>(pIn[1] + indexBase);
            }
            else {
                pOut[1] = static_cast<uint16_t>(pIn[1] + indexBase);
                pOut[2] = static_cast<uint16_t>(pIn[2] + indexBase);
            }
            pOut += 3;
        }
    }
}

// The section's triangles as 16-bit indices: generated from its vertex range when it has no indices, its 16-bit
// indices appended as they are; 32-bit indices are not appended.
void hkMeshPrimitiveUtil::appendTriangleIndices(
    const hkMeshSection& section,
    hkArray<uint16_t, hkContainerHeapAllocator>& indicesOut
) {
    switch (section.m_indexType.m_storage) {
        case hkMeshSection::INDEX_TYPE_NONE:
            appendTriangleIndices(
                static_cast<hkMeshSection::PrimitiveType>(section.m_primitiveType.m_storage),
                section.m_numIndices,
                section.m_vertexStartIndex,
                indicesOut,
                hkMemoryRouter::getInstance().heap()
            );
            break;
        case hkMeshSection::INDEX_TYPE_UINT16: {
            const hkMeshSection::PrimitiveType primType = static_cast<hkMeshSection::PrimitiveType>(
                section.m_primitiveType.m_storage
            );
            if (primType == hkMeshSection::PRIMITIVE_TYPE_TRIANGLE_LIST ||
                primType == hkMeshSection::PRIMITIVE_TYPE_TRIANGLE_STRIP) {
                appendTriangleIndices16(
                    primType,
                    static_cast<const uint16_t*>(section.m_indices),
                    section.m_numIndices,
                    0,
                    indicesOut
                );
            }
            break;
        }
        default: break;
    }
}

void hkMeshPrimitiveUtil::appendTriangleIndices(
    const hkMeshShape* shape,
    int32_t sectionIndex,
    hkArray<uint16_t, hkContainerHeapAllocator>& indicesOut
) {
    hkMeshSection v3;
    shape->lockSection(sectionIndex, 1, v3);
    hkMeshPrimitiveUtil::appendTriangleIndices(v3, indicesOut);
    shape->unlockSection(v3);
}

// The section's triangles as 32-bit indices: generated from its vertex range when it has no indices, 32-bit indices
// appended as they are, 16-bit ones unwound into a stack-allocated local array first and appended one by one.
void hkMeshPrimitiveUtil::appendTriangleIndices(
    const hkMeshSection& section,
    hkArray<uint32_t, hkContainerHeapAllocator>& indicesOut
) {
    switch (section.m_indexType.m_storage) {
        case hkMeshSection::INDEX_TYPE_NONE:
            appendTriangleIndices(
                static_cast<hkMeshSection::PrimitiveType>(section.m_primitiveType.m_storage),
                section.m_numIndices,
                section.m_vertexStartIndex,
                indicesOut
            );
            break;
        case hkMeshSection::INDEX_TYPE_UINT16: {
            const hkMeshSection::PrimitiveType primType = static_cast<hkMeshSection::PrimitiveType>(
                section.m_primitiveType.m_storage
            );
            if (primType != hkMeshSection::PRIMITIVE_TYPE_TRIANGLE_LIST &&
                primType != hkMeshSection::PRIMITIVE_TYPE_TRIANGLE_STRIP) {
                break;
            }
            const int32_t numIndices = section.m_numIndices;
            hkLocalArray<uint16_t> indices16(numIndices);
            appendTriangleIndices16(
                primType,
                static_cast<const uint16_t*>(section.m_indices),
                section.m_numIndices,
                0,
                indices16
            );
            for (int32_t i = 0; i < section.m_numIndices; ++i) {
                const uint32_t index = indices16.m_data[i];
                indicesOut.pushBack(index);
            }
            break;
        }
        case hkMeshSection::INDEX_TYPE_UINT32: {
            const hkMeshSection::PrimitiveType primType = static_cast<hkMeshSection::PrimitiveType>(
                section.m_primitiveType.m_storage
            );
            if (primType == hkMeshSection::PRIMITIVE_TYPE_TRIANGLE_LIST ||
                primType == hkMeshSection::PRIMITIVE_TYPE_TRIANGLE_STRIP) {
                appendTriangleIndices32(
                    primType,
                    static_cast<const uint32_t*>(section.m_indices),
                    section.m_numIndices,
                    0,
                    indicesOut
                );
            }
            break;
        }
        default: break;
    }
}
