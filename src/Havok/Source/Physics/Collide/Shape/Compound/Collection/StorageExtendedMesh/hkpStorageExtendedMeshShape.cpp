#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/StorageExtendedMesh/hkpStorageExtendedMeshShape.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkString.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/Mesh/hkpNamedMeshMaterial.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeContainer.h"

#include <bit>
#include <new>

int32_t calcNumIndices(int32_t numTriangles, int32_t indicesPerTriangle) {
    if (indicesPerTriangle == 1) {
        return numTriangles + 2;
    }
    if (indicesPerTriangle == 2) {
        return 2 * numTriangles + 1;
    }
    return 4 * numTriangles;
}

// Eight empty arrays.
hkpStorageExtendedMeshShape::MeshSubpartStorage::MeshSubpartStorage() {}

// The loaded arrays stay as loaded.
hkpStorageExtendedMeshShape::MeshSubpartStorage::MeshSubpartStorage(hkFinishLoadedObjectFlag flag)
  : hkReferencedObject(flag), m_vertices(flag), m_indices8(flag), m_indices16(flag), m_indices32(flag),
    m_materialIndices(flag), m_materials(flag), m_namedMaterials(flag), m_materialIndices16(flag) {}

// The arrays free their own storage.
hkpStorageExtendedMeshShape::MeshSubpartStorage::~MeshSubpartStorage() {}

// Three empty arrays.
hkpStorageExtendedMeshShape::ShapeSubpartStorage::ShapeSubpartStorage() {}

// The loaded arrays stay as loaded.
hkpStorageExtendedMeshShape::ShapeSubpartStorage::ShapeSubpartStorage(hkFinishLoadedObjectFlag flag)
  : hkReferencedObject(flag), m_materialIndices(flag), m_materials(flag), m_materialIndices16(flag) {}

// The arrays free their own storage.
hkpStorageExtendedMeshShape::ShapeSubpartStorage::~ShapeSubpartStorage() {}

// When finishing, each subpart's storage is finished and the subpart's data pointers point back into it.
hkpStorageExtendedMeshShape::hkpStorageExtendedMeshShape(hkFinishLoadedObjectFlag flag)
  : hkpExtendedMeshShape(flag), m_meshstorage(flag), m_shapestorage(flag) {
    if (!flag.m_finishing) {
        return;
    }
    for (int32_t i = 0; i < m_trianglesSubparts.m_size; ++i) {
        if (m_meshstorage.m_data[i]) {
            ::new (m_meshstorage.m_data[i]) MeshSubpartStorage(flag);
        }
        MeshSubpartStorage* pStorage = m_meshstorage.m_data[i];
        TrianglesSubpart& rPart = m_trianglesSubparts.m_data[i];
        rPart.m_vertexBase = &(*pStorage->m_vertices.begin())(0);
        if (rPart.m_stridingType.m_storage == INDICES_INT8) {
            rPart.m_indexBase = pStorage->m_indices8.begin();
        }
        else if (rPart.m_stridingType.m_storage == INDICES_INT16) {
            rPart.m_indexBase = pStorage->m_indices16.begin();
        }
        else {
            rPart.m_indexBase = pStorage->m_indices32.begin();
        }
        if (rPart.m_materialIndexStridingType.m_storage == MATERIAL_INDICES_INT8) {
            rPart.m_materialIndexBase = pStorage->m_materialIndices.begin();
        }
        else {
            rPart.m_materialIndexBase = pStorage->m_materialIndices16.begin();
        }
        if (pStorage->m_namedMaterials.m_size) {
            rPart.m_materialBase = pStorage->m_namedMaterials.begin();
        }
        else {
            rPart.m_materialBase = pStorage->m_materials.begin();
        }
    }
    for (int32_t i = 0; i < m_shapesSubparts.m_size; ++i) {
        if (m_shapestorage.m_data[i]) {
            ::new (m_shapestorage.m_data[i]) ShapeSubpartStorage(flag);
        }
        ShapeSubpartStorage* pStorage = m_shapestorage.m_data[i];
        ShapesSubpart& rPart = m_shapesSubparts.m_data[i];
        if (rPart.m_materialIndexStridingType.m_storage == MATERIAL_INDICES_INT8) {
            rPart.m_materialIndexBase = pStorage->m_materialIndices.begin();
        }
        else {
            rPart.m_materialIndexBase = pStorage->m_materialIndices16.begin();
        }
        rPart.m_materialBase = pStorage->m_materials.begin();
    }
}

hkpStorageExtendedMeshShape::hkpStorageExtendedMeshShape(float a1, int32_t a2) : hkpExtendedMeshShape(a1, a2) {}

// mesh's radius, subpart bits, scaling, user data and welding, and copies of its subparts in this shape's storage.
hkpStorageExtendedMeshShape::hkpStorageExtendedMeshShape(const hkpExtendedMeshShape* mesh)
  : hkpExtendedMeshShape(mesh->m_triangleRadius, mesh->m_numBitsForSubpartIndex) {
    m_scaling = mesh->m_scaling;
    m_userData = mesh->m_userData;
    m_disableWelding.m_bool = mesh->m_disableWelding.m_bool;
    for (const TrianglesSubpart& part : mesh->m_trianglesSubparts) {
        hkpStorageExtendedMeshShape::addTrianglesSubpart(part);
    }
    for (const ShapesSubpart& part : mesh->m_shapesSubparts) {
        hkpStorageExtendedMeshShape::addShapesSubpart(part);
    }
    m_weldingInfo.copyFromArray(hkContainerHeapAllocator().get(&m_weldingInfo), mesh->m_weldingInfo, hkTypeIsPod());
    m_weldingType.m_storage = mesh->m_weldingType.m_storage;
    recalcAabbExtents();
}

// Each storage's reference goes back; the subparts and the arrays are destroyed by the base and the arrays.
hkpStorageExtendedMeshShape::~hkpStorageExtendedMeshShape() {
    for (MeshSubpartStorage* pStorage : m_meshstorage) {
        pStorage->removeReference();
    }
    for (ShapeSubpartStorage* pStorage : m_shapestorage) {
        pStorage->removeReference();
    }
}

// Appends a copy of aSubpart whose vertices, indices and materials are copied into a new MeshSubpartStorage owned by
// the shape, then points the copy's data at that storage. The storage's arrays grow by hkArray::expandBy and pushBack.
// The source arrays are raw memory read at a byte striding, so they are stepped through byte pointers.
void hkpStorageExtendedMeshShape::addTrianglesSubpart(const TrianglesSubpart& partIn) {
    TrianglesSubpart* pSubpart = expandOneTriangleSubparts();
    *pSubpart = partIn;

    // EXE-INLINED none: MeshSubpartStorage::operator new (HK_DECLARE_CLASS_ALLOCATOR)
    MeshSubpartStorage* pNewStorage =
        static_cast<MeshSubpartStorage*>(hkMemoryRouter::getInstance().heap().blockAlloc(sizeof(MeshSubpartStorage)));
    pNewStorage->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(MeshSubpartStorage));
    pNewStorage = ::new (pNewStorage) MeshSubpartStorage;

    m_meshstorage.pushBack(pNewStorage);
    MeshSubpartStorage* pStorage = m_meshstorage.m_data[m_meshstorage.m_size - 1];

    // The vertices, as vectors with w 0.
    hkVector4* pVertex = pStorage->m_vertices.expandBy(partIn.m_numVertices);
    const float* pSrcVertex = partIn.m_vertexBase;
    for (int32_t i = 0; i < partIn.m_numVertices; i++) {
        pVertex[i].set(pSrcVertex[0], pSrcVertex[1], pSrcVertex[2], 0.0f);
        pSrcVertex =
            reinterpret_cast<const float*>(reinterpret_cast<const uint8_t*>(pSrcVertex) + partIn.m_vertexStriding);
    }
    pSubpart->m_vertexBase = &pStorage->m_vertices[0](0);
    pSubpart->m_vertexStriding = sizeof(hkVector4);
    pSubpart->m_numVertices = partIn.m_numVertices;

    // The indices, three per triangle, at a striding of at most four of the index type. The 16- and 32-bit strides are
    // divided by the index size unsigned.
    const void* pIndexBase;
    if (partIn.m_stridingType.m_storage == INDICES_INT8) {
        const int32_t iStride = partIn.m_indexStriding;
        pSubpart->m_indexStriding = (iStride > 2) ? 4 : iStride;
        const int32_t iNumIndices = calcNumIndices(partIn.m_numTriangleShapes, iStride);
        uint8_t* pDst = pStorage->m_indices8.expandBy(iNumIndices);
        hkString::memSet(pDst, 0, iNumIndices);
        pIndexBase = pDst;
        const uint8_t* pSrc = static_cast<const uint8_t*>(partIn.m_indexBase);
        for (int32_t i = 0; i < partIn.m_numTriangleShapes; i++) {
            pDst[0] = pSrc[0];
            pDst[1] = pSrc[1];
            pDst[2] = pSrc[2];
            pDst += pSubpart->m_indexStriding;
            pSrc += partIn.m_indexStriding;
        }
    }
    else if (partIn.m_stridingType.m_storage == INDICES_INT16) {
        const int32_t iStride = static_cast<int32_t>(static_cast<uint32_t>(partIn.m_indexStriding) / sizeof(uint16_t));
        pSubpart->m_indexStriding = static_cast<int32_t>(((iStride > 2) ? 4 : iStride) * sizeof(uint16_t));
        const int32_t iNumIndices = calcNumIndices(partIn.m_numTriangleShapes, iStride);
        uint16_t* pDst = pStorage->m_indices16.expandBy(iNumIndices);
        hkString::memSet(pDst, 0, static_cast<int32_t>(iNumIndices * sizeof(uint16_t)));
        pIndexBase = pDst;
        const uint16_t* pSrc = static_cast<const uint16_t*>(partIn.m_indexBase);
        for (int32_t i = 0; i < partIn.m_numTriangleShapes; i++) {
            pDst[0] = pSrc[0];
            pDst[1] = pSrc[1];
            pDst[2] = pSrc[2];
            pDst = reinterpret_cast<uint16_t*>(reinterpret_cast<uint8_t*>(pDst) + pSubpart->m_indexStriding);
            pSrc = reinterpret_cast<const uint16_t*>(reinterpret_cast<const uint8_t*>(pSrc) + partIn.m_indexStriding);
        }
    }
    else {
        const int32_t iStride = static_cast<int32_t>(static_cast<uint32_t>(partIn.m_indexStriding) / sizeof(uint32_t));
        pSubpart->m_indexStriding = static_cast<int32_t>(((iStride > 2) ? 4 : iStride) * sizeof(uint32_t));
        const int32_t iNumIndices = calcNumIndices(partIn.m_numTriangleShapes, iStride);
        uint32_t* pDst = pStorage->m_indices32.expandBy(iNumIndices);
        hkString::memSet(pDst, 0, static_cast<int32_t>(iNumIndices * sizeof(uint32_t)));
        pIndexBase = pDst;
        const uint32_t* pSrc = static_cast<const uint32_t*>(partIn.m_indexBase);
        for (int32_t i = 0; i < partIn.m_numTriangleShapes; i++) {
            pDst[0] = pSrc[0];
            pDst[1] = pSrc[1];
            pDst[2] = pSrc[2];
            pDst = reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(pDst) + pSubpart->m_indexStriding);
            pSrc = reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(pSrc) + partIn.m_indexStriding);
        }
    }
    pSubpart->m_stridingType = partIn.m_stridingType;
    pSubpart->m_numTriangleShapes = partIn.m_numTriangleShapes;
    pSubpart->m_flipAlternateTriangles = partIn.m_flipAlternateTriangles;
    pSubpart->m_indexBase = pIndexBase;
    pSubpart->m_materialIndexStridingType = partIn.m_materialIndexStridingType;

    // The material indices: one per triangle, or the single shared one when the striding is 0.
    const void* pMaterialIndexBase = partIn.m_materialIndexBase;
    if (pMaterialIndexBase) {
        if (partIn.m_materialIndexStridingType.m_storage == MATERIAL_INDICES_INT8) {
            hkArray<uint8_t>& indices = pStorage->m_materialIndices;
            if (partIn.m_materialIndexStriding == 0) {
                indices.pushBack(*static_cast<const uint8_t*>(pMaterialIndexBase));
                pSubpart->m_materialIndexBase = &indices.m_data[indices.m_size - 1];
            }
            else {
                uint8_t* pDst = indices.expandBy(partIn.m_numTriangleShapes);
                pSubpart->m_materialIndexBase = pDst;
                const uint8_t* pSrc = static_cast<const uint8_t*>(partIn.m_materialIndexBase);
                for (int32_t i = 0; i < partIn.m_numTriangleShapes; i++) {
                    pDst[i] = *pSrc;
                    pSrc += partIn.m_materialIndexStriding;
                }
            }
        }
        else {
            hkArray<uint16_t>& indices = pStorage->m_materialIndices16;
            if (partIn.m_materialIndexStriding == 0) {
                indices.pushBack(*static_cast<const uint16_t*>(pMaterialIndexBase));
                pSubpart->m_materialIndexBase = &indices.m_data[indices.m_size - 1];
            }
            else {
                uint16_t* pDst = indices.expandBy(partIn.m_numTriangleShapes);
                pSubpart->m_materialIndexBase = pDst;
                const uint16_t* pSrc = static_cast<const uint16_t*>(partIn.m_materialIndexBase);
                for (int32_t i = 0; i < partIn.m_numTriangleShapes; i++) {
                    pDst[i] = *pSrc;
                    pSrc = reinterpret_cast<const uint16_t*>(
                        reinterpret_cast<const uint8_t*>(pSrc) + partIn.m_materialIndexStriding
                    );
                }
            }
        }
        pSubpart->m_materialIndexStriding = partIn.m_materialIndexStriding;
        if (partIn.m_materialIndexStriding != 0) {
            if (pSubpart->m_materialIndexStridingType.m_storage == MATERIAL_INDICES_INT8) {
                pSubpart->m_materialIndexStriding = sizeof(uint8_t);
            }
            else if (pSubpart->m_materialIndexStridingType.m_storage == MATERIAL_INDICES_INT16) {
                pSubpart->m_materialIndexStriding = sizeof(uint16_t);
            }
        }
    }
    else {
        pSubpart->m_materialIndexBase = nullptr;
        pSubpart->m_materialIndexStriding = 0;
    }

    // The materials: named ones, or Material records (restitution 0 and friction 1 when the source materials are
    // plain hkpMeshMaterials).
    if (pSubpart->m_materialIndexBase) {
        if (partIn.m_materialClass == &hkpNamedMeshMaterialClass) {
            const int32_t iNumMaterials = partIn.m_numMaterials;
            hkArray<hkpNamedMeshMaterial>& materials = pStorage->m_namedMaterials;
            const int32_t iOldSize = materials.m_size;
            materials.reserve(iOldSize + iNumMaterials);
            // EXE-INLINED none: hkArrayUtil::construct<hkpNamedMeshMaterial> (the inlined default constructor:
            // m_name "Default", m_filterInfo 0, behind placement new's null test).
            hkpNamedMeshMaterial* pNew = materials.m_data + materials.m_size;
            for (int32_t iLeft = iNumMaterials; iLeft > 0; iLeft--) {
                if (pNew) {
                    ::new (&pNew->m_name) hkStringPtr("Default");
                    pNew->m_filterInfo = 0;
                }
                pNew++;
            }
            materials.m_size += iNumMaterials;

            hkpNamedMeshMaterial* pDst = materials.m_data + iOldSize;
            const hkpNamedMeshMaterial* pSrc = static_cast<const hkpNamedMeshMaterial*>(partIn.m_materialBase);
            for (int32_t i = 0; i < partIn.m_numMaterials; i++) {
                // The copy is built in place by the default constructor (placement new, null test included), then
                // assigned.
                hkpNamedMeshMaterial* pCopy = nullptr;
                if (pDst) {
                    pDst->m_name.~hkStringPtr();
                    ::new (&pDst->m_name) hkStringPtr("Default");
                    pDst->m_filterInfo = 0;
                    pCopy = pDst;
                }
                pCopy->m_filterInfo = pSrc->m_filterInfo;
                pCopy->m_name = pSrc->m_name;
                pSrc = reinterpret_cast<const hkpNamedMeshMaterial*>(
                    reinterpret_cast<const uint8_t*>(pSrc) + partIn.m_materialStriding
                );
                pDst++;
            }
            pSubpart->m_materialBase = materials.m_data;
            if (partIn.m_materialStriding != 0) {
                pSubpart->m_materialStriding = static_cast<int16_t>(sizeof(hkpNamedMeshMaterial));
                pSubpart->m_numMaterials = partIn.m_numMaterials;
            }
            else {
                pSubpart->m_materialStriding = 0;
                pSubpart->m_numMaterials = 1;
            }
        }
        else {
            const int32_t iNumMaterials = partIn.m_numMaterials;
            hkArray<Material>& materials = pStorage->m_materials;
            const int32_t iOldSize = materials.m_size;
            materials.reserve(iOldSize + iNumMaterials);
            materials.m_size += iNumMaterials;
            Material* pDst = materials.m_data + iOldSize;
            const hkpMeshMaterial* pSrc = partIn.m_materialBase;
            for (int32_t i = 0; i < partIn.m_numMaterials; i++) {
                pDst->m_filterInfo = pSrc->m_filterInfo;
                if (partIn.m_materialClass == &hkpStorageExtendedMeshShapeMaterialClass) {
                    const Material* pSrcMaterial = static_cast<const Material*>(pSrc);
                    pDst->m_restitution = pSrcMaterial->m_restitution;
                    pDst->m_friction = pSrcMaterial->m_friction;
                    pDst->m_userData = pSrcMaterial->m_userData;
                }
                else {
                    // EXE-INLINED none: hkHalf::hkHalf(const float&) (the float's top 16 bits, sar 16)
                    pDst->m_friction.m_value = static_cast<int16_t>(std::bit_cast<int32_t>(1.0f) >> 16);
                    pDst->m_restitution.m_value = static_cast<int16_t>(std::bit_cast<int32_t>(0.0f) >> 16);
                }
                pSrc = reinterpret_cast<const hkpMeshMaterial*>(
                    reinterpret_cast<const uint8_t*>(pSrc) + partIn.m_materialStriding
                );
                pDst++;
            }
            pSubpart->m_materialBase = materials.m_data;
            if (partIn.m_materialStriding != 0) {
                pSubpart->m_materialStriding = static_cast<int16_t>(sizeof(Material));
                pSubpart->m_numMaterials = partIn.m_numMaterials;
            }
            else {
                pSubpart->m_materialStriding = 0;
                pSubpart->m_numMaterials = 1;
            }
        }
    }

    recalcAabbExtents();
    m_cachedNumChildShapes += _getNumChildShapesInTrianglesSubpart(*pSubpart, m_trianglesSubparts.m_size - 1);
}

// The SDK's addShapesSubpart: a new subpart copied from aSubpart, whose material indices and materials are copied
// into a new storage object the subpart then points into. Returns the new subpart's index.
int32_t hkpStorageExtendedMeshShape::addShapesSubpart(const hkpExtendedMeshShape::ShapesSubpart& partIn) {
    ShapesSubpart* subpart = expandOneShapesSubparts();

    ShapeSubpartStorage* pNewStorage = static_cast<ShapeSubpartStorage*>(
        hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(ShapeSubpartStorage))
    );
    pNewStorage->m_memSizeAndFlags = sizeof(ShapeSubpartStorage);
    ::new (pNewStorage) ShapeSubpartStorage;

    m_shapestorage.pushBack(pNewStorage);
    ShapeSubpartStorage* storage = m_shapestorage.m_data[m_shapestorage.m_size - 1];

    *subpart = partIn;
    subpart->m_materialIndexStridingType.m_storage = partIn.m_materialIndexStridingType.m_storage;
    const void* pIndexBase = partIn.m_materialIndexBase;
    if (pIndexBase) {
        if (partIn.m_materialIndexStridingType.m_storage == MATERIAL_INDICES_INT8) {
            hkArray<uint8_t>& indices = storage->m_materialIndices;
            if (partIn.m_materialIndexStriding == 0) {
                // One index shared by every child shape.
                indices.pushBack(*static_cast<const uint8_t*>(pIndexBase));
                subpart->m_materialIndexBase = &indices.m_data[indices.m_size - 1];
            }
            else {
                int32_t numIndices = partIn.m_childShapes.m_size;
                uint8_t* pDst = indices.expandBy(numIndices);
                subpart->m_materialIndexBase = pDst;
                // The source indices are strided bytes.
                const uint8_t* pSrc = static_cast<const uint8_t*>(partIn.m_materialIndexBase);
                for (int32_t i = 0; i < numIndices; i++) {
                    pDst[i] = *pSrc;
                    pSrc += partIn.m_materialIndexStriding;
                }
            }
        }
        else {
            hkArray<uint16_t>& indices16 = storage->m_materialIndices16;
            if (partIn.m_materialIndexStriding == 0) {
                // One index shared by every child shape.
                indices16.pushBack(*static_cast<const uint16_t*>(pIndexBase));
                subpart->m_materialIndexBase = &indices16.m_data[indices16.m_size - 1];
            }
            else {
                int32_t numIndices = partIn.m_childShapes.m_size;
                uint16_t* pDst = indices16.expandBy(numIndices);
                subpart->m_materialIndexBase = pDst;
                // The source indices are 16-bit values a byte stride apart: untyped memory stepped through as bytes.
                const uint16_t* pSrc = static_cast<const uint16_t*>(partIn.m_materialIndexBase);
                for (int32_t i = 0; i < numIndices; i++) {
                    pDst[i] = *pSrc;
                    pSrc = reinterpret_cast<const uint16_t*>(
                        reinterpret_cast<const uint8_t*>(pSrc) + partIn.m_materialIndexStriding
                    );
                }
            }
        }
        // The copies are packed: a non-zero striding becomes the index size.
        uint16_t indexStriding = partIn.m_materialIndexStriding;
        subpart->m_materialIndexStriding = indexStriding;
        if (indexStriding) {
            switch (subpart->m_materialIndexStridingType.m_storage) {
                case MATERIAL_INDICES_INT8: subpart->m_materialIndexStriding = sizeof(uint8_t); break;
                case MATERIAL_INDICES_INT16: subpart->m_materialIndexStriding = sizeof(uint16_t); break;
            }
        }
    }
    else {
        subpart->m_materialIndexBase = nullptr;
        subpart->m_materialIndexStriding = 0;
    }

    if (subpart->m_materialIndexBase) {
        int32_t numMaterials = partIn.m_numMaterials;
        hkArray<Material>& materials = storage->m_materials;
        // The SDK's expandBy, written out: hkArrayUtil has no construct for Material, whose construction is empty.
        // EXE-INLINED none: hkArray<hkpStorageExtendedMeshShape::Material>::expandBy
        hkMemoryAllocator& heap = hkMemoryRouter::getInstance().heap();
        int32_t oldSize = materials.m_size;
        materials._reserve(heap, oldSize + numMaterials);
        materials.m_size += numMaterials;
        Material* pDst = materials.m_data + oldSize;
        // The source materials are a byte stride apart: untyped memory stepped through as bytes. Materials of
        // another class get restitution 0 and friction 1 (hkHalf: the top half of the single).
        const hkpMeshMaterial* pSrc = partIn.m_materialBase;
        for (int32_t i = 0; i < partIn.m_numMaterials; i++) {
            pDst->m_filterInfo = pSrc->m_filterInfo;
            if (partIn.m_materialClass == &hkpStorageExtendedMeshShapeMaterialClass) {
                const Material* pSrcMaterial = static_cast<const Material*>(pSrc);
                pDst->m_restitution = pSrcMaterial->m_restitution;
                pDst->m_friction = pSrcMaterial->m_friction;
                pDst->m_userData = pSrcMaterial->m_userData;
            }
            else {
                pDst->m_friction.m_value = std::bit_cast<int32_t>(1.0f) >> 16;
                pDst->m_restitution.m_value = std::bit_cast<int32_t>(0.0f) >> 16;
            }
            pSrc = reinterpret_cast<const hkpMeshMaterial*>(
                reinterpret_cast<const uint8_t*>(pSrc) + partIn.m_materialStriding
            );
            pDst++;
        }
        if (partIn.m_materialStriding) {
            subpart->m_materialStriding = sizeof(Material);
            subpart->m_numMaterials = partIn.m_numMaterials;
        }
        else {
            subpart->m_materialStriding = 0;
            subpart->m_numMaterials = 1;
        }
        subpart->m_materialBase = storage->m_materials.m_data;
    }
    recalcAabbExtents();
    m_cachedNumChildShapes += _getNumChildShapesInShapesSubpart(*subpart);
    return m_shapesSubparts.m_size - 1;
}
