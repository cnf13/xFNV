#include "Havok/Source/Physics/Collide/Shape/Deprecated/StorageMesh/hkpStorageMeshShape.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/Mesh/hkpMeshMaterial.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/Mesh/hkpMeshShape.h"

// Havok SDK signature (hkpCollide.lib, tied by reloc,target).
static const void* getVtablehkpStorageMeshShape();
static void cleanupLoadedObjecthkpStorageMeshShape(void* p);

// The reflected members of hkpStorageMeshShape::SubpartStorage.
using hkpStorageMeshShapeSubpartStorageClass_MembersArray = const hkClassMember[6];
static hkpStorageMeshShapeSubpartStorageClass_MembersArray hkpStorageMeshShapeSubpartStorageClass_Members{
    {"vertices", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_REAL}, 0, {0}, 0x8, nullptr},
    {"indices16", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT16}, 0, {0}, 0x14, nullptr},
    {"indices32", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT32}, 0, {0}, 0x20, nullptr},
    {"materialIndices",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_UINT8},
     0,
     {0},
     0x2C,
     nullptr},
    {"materials", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT32}, 0, {0}, 0x38, nullptr},
    {"materialIndices16",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_UINT16},
     0,
     {0},
     0x44,
     nullptr}
};
hkClass hkpStorageMeshShapeSubpartStorageClass(
    "hkpStorageMeshShapeSubpartStorage",
    &hkReferencedObjectClass,
    0x50,
    nullptr,
    0,
    nullptr,
    0,
    hkpStorageMeshShapeSubpartStorageClass_Members,
    6,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpStorageMeshShapeSubpartStorageTypeInfo = {
    "hkpStorageMeshShapeSubpartStorage",
    finishLoadedObjecthkpStorageMeshShapeSubpartStorage,
    cleanupLoadedObjecthkpStorageMeshShapeSubpartStorage,
    getVtablehkpStorageMeshShapeSubpartStorage(),
    &typeid(hkpStorageMeshShape::SubpartStorage),
    sizeof(hkpStorageMeshShape::SubpartStorage)
};

// The reflected members of hkpStorageMeshShape.
using hkpStorageMeshShapeClass_MembersArray = const hkClassMember[1];
static hkpStorageMeshShapeClass_MembersArray hkpStorageMeshShapeClass_Members{
    {"storage",
     &hkpStorageMeshShapeSubpartStorageClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_POINTER},
     0,
     {0},
     0x60,
     nullptr}
};
hkClass hkpStorageMeshShapeClass(
    "hkpStorageMeshShape",
    &hkpMeshShapeClass,
    0x70,
    nullptr,
    0,
    nullptr,
    0,
    hkpStorageMeshShapeClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpStorageMeshShapeTypeInfo = {
    "hkpStorageMeshShape",          finishLoadedObjecthkpStorageMeshShape, cleanupLoadedObjecthkpStorageMeshShape,
    getVtablehkpStorageMeshShape(), &typeid(hkpStorageMeshShape),          sizeof(hkpStorageMeshShape)
};

// Constructs a finished-loading shape on the stack and returns the vtable its constructor stored.
static const void* getVtablehkpStorageMeshShape() {
    return hkGetVtableOf<hkpStorageMeshShape>();
}

static void cleanupLoadedObjecthkpStorageMeshShape(void* p) {
    static_cast<hkpStorageMeshShape*>(p)->~hkpStorageMeshShape();
}

const void* getVtablehkpStorageMeshShapeSubpartStorage() {
    return hkGetVtableOf<hkpStorageMeshShape::SubpartStorage>();
}

void cleanupLoadedObjecthkpStorageMeshShapeSubpartStorage(void* p) {
    static_cast<hkpStorageMeshShape::SubpartStorage*>(p)->~SubpartStorage();
}

// Factory for a storage mesh's subpart storage: constructs one on p when p is set.
void finishLoadedObjecthkpStorageMeshShapeSubpartStorage(void* p, int32_t a1) {
    if (p != nullptr) {
        ::new (p) hkpStorageMeshShape::SubpartStorage(a1);
    }
}

// Factory for a storage mesh: constructs one on apMemory when apMemory is set.
void finishLoadedObjecthkpStorageMeshShape(void* p, int32_t finishing) {
    void* apMemory = p;
    hkpShapeType aType = static_cast<hkpShapeType>(finishing);
    if (apMemory != nullptr) {
        ::new (apMemory) hkpStorageMeshShape(aType);
    }
}

int32_t sub_D4B390(int32_t numTriangles, int32_t indicesPerTriangle) {
    if (indicesPerTriangle == 1) {
        return numTriangles + 2;
    }
    if (indicesPerTriangle == 2) {
        return 2 * numTriangles + 1;
    }
    return 3 * numTriangles;
}

hkpStorageMeshShape::SubpartStorage::SubpartStorage() {}

// The finish constructor: a1 is the finish flag, which leaves the loaded members as they are.
hkpStorageMeshShape::SubpartStorage::SubpartStorage(int32_t a1)
  : hkReferencedObject(hkFinishLoadedObjectFlag{a1}), m_vertices(hkFinishLoadedObjectFlag{a1}),
    m_indices16(hkFinishLoadedObjectFlag{a1}), m_indices32(hkFinishLoadedObjectFlag{a1}),
    m_materialIndices(hkFinishLoadedObjectFlag{a1}), m_materials(hkFinishLoadedObjectFlag{a1}),
    m_materialIndices16(hkFinishLoadedObjectFlag{a1}) {}

hkpStorageMeshShape::SubpartStorage::~SubpartStorage() {}

const hkClass& hkpStorageMeshShape::SubpartStorage::staticClass() {
    return hkpStorageMeshShapeSubpartStorageClass;
}

// The finish constructor: a1 is the finish flag; when finishing, each subpart points again into its loaded storage.
hkpStorageMeshShape::hkpStorageMeshShape(hkpShapeType a1)
  : hkpMeshShape(hkFinishLoadedObjectFlag{static_cast<int32_t>(a1)}),
    m_storage(hkFinishLoadedObjectFlag{static_cast<int32_t>(a1)}) {
    if (a1) {
        for (int32_t i = 0; i < m_subparts.m_size; ++i) {
            SubpartStorage* pStorage = m_storage.m_data[i];
            Subpart& kPart = m_subparts.m_data[i];
            kPart.m_vertexBase = pStorage->m_vertices.begin();
            if (kPart.m_stridingType.m_storage == 1) {
                kPart.m_indexBase = pStorage->m_indices16.begin();
            }
            else {
                kPart.m_indexBase = pStorage->m_indices32.begin();
            }
            if (kPart.m_materialIndexStridingType.m_storage == 1) {
                kPart.m_materialIndexBase = pStorage->m_materialIndices.begin();
            }
            else {
                kPart.m_materialIndexBase = pStorage->m_materialIndices16.begin();
            }
            // Havok's storage keeps the materials as raw words that the subpart reads as strided hkpMeshMaterial
            // records.
            kPart.m_materialBase = reinterpret_cast<const hkpMeshMaterial*>(pStorage->m_materials.begin());
        }
    }
}

hkpStorageMeshShape::hkpStorageMeshShape(float radius, int32_t numbits) : hkpMeshShape(radius, numbits) {}

hkpStorageMeshShape::hkpStorageMeshShape(const hkpMeshShape* mesh)
  : hkpMeshShape(mesh->m_radius, mesh->m_numBitsForSubpartIndex) {
    m_scaling = mesh->m_scaling;
    m_userData = mesh->m_userData;
    m_disableWelding.m_bool = mesh->m_disableWelding.m_bool;
    for (int32_t i = 0; i < mesh->m_subparts.m_size; ++i) {
        hkpStorageMeshShape::addSubpart(mesh->m_subparts.m_data[i]);
    }
    m_weldingInfo.copyFromArray(hkContainerHeapAllocator().get(&m_weldingInfo), mesh->m_weldingInfo, hkTypeIsPod());
    m_weldingType.m_storage = mesh->m_weldingType.m_storage;
}

hkpStorageMeshShape::~hkpStorageMeshShape() {
    for (int32_t i = 0; i < m_storage.m_size; ++i) {
        m_storage.m_data[i]->removeReference();
    }
}

// Appends a subpart whose vertices, indices, material indices and materials are copies held in a new SubpartStorage.
// The input's stridings are byte counts over memory the caller lays out freely (the SDK's hkAddByteOffsetConst), so
// its pointers step through char.
void hkpStorageMeshShape::addSubpart(const hkpMeshShape::Subpart& partIn) {
    // EXE-INLINED none: hkArray<hkpMeshShape::Subpart>::expandOne
    hkMemoryAllocator& subpartAlloc = hkMemoryRouter::getInstance().heap();
    if (m_subparts.m_size == m_subparts.getCapacity()) {
        hkArrayUtil::_reserveMore(subpartAlloc, &m_subparts, sizeof(hkpMeshShape::Subpart));
    }
    hkpMeshShape::Subpart* pNew = m_subparts.m_data + m_subparts.m_size;
    if (pNew != nullptr) {
        // EXE-INLINED none: hkpMeshShape::Subpart::Subpart
        pNew->m_materialIndexStridingType = hkpMeshShape::MATERIAL_INDICES_INT8;
        pNew->m_materialIndexStriding = 0;
        pNew->m_materialStriding = 0;
        pNew->m_numMaterials = 1;
        pNew->m_materialBase = nullptr;
        pNew->m_materialIndexBase = nullptr;
        pNew->m_triangleOffset = -1;
        pNew->m_flipAlternateTriangles = 0;
    }
    hkpMeshShape::Subpart& part = m_subparts.m_data[m_subparts.m_size++];

    hkpStorageMeshShape::SubpartStorage* pNewStorage = static_cast<hkpStorageMeshShape::SubpartStorage*>(
        hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpStorageMeshShape::SubpartStorage))
    );
    pNewStorage->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkpStorageMeshShape::SubpartStorage));
    pNewStorage = ::new (pNewStorage) hkpStorageMeshShape::SubpartStorage;
    // EXE-INLINED none: hkArray<hkpStorageMeshShape::SubpartStorage*>::pushBack
    hkMemoryAllocator& storageAlloc = hkMemoryRouter::getInstance().heap();
    if (m_storage.m_size == m_storage.getCapacity()) {
        hkArrayUtil::_reserveMore(storageAlloc, &m_storage, sizeof(hkpStorageMeshShape::SubpartStorage*));
    }
    m_storage.m_data[m_storage.m_size++] = pNewStorage;
    hkpStorageMeshShape::SubpartStorage* pStorage = m_storage.m_data[m_storage.m_size - 1];

    // The vertices, packed as three floats each.
    float* pVertices = pStorage->m_vertices.expandBy(3 * partIn.m_numVertices);
    const float* pVertexIn = partIn.m_vertexBase;
    for (int32_t i = 0; i < partIn.m_numVertices; ++i) {
        pVertices[0] = pVertexIn[0];
        pVertices[1] = pVertexIn[1];
        pVertices[2] = pVertexIn[2];
        pVertices += 3;
        pVertexIn = reinterpret_cast<const float*>(reinterpret_cast<const char*>(pVertexIn) + partIn.m_vertexStriding);
    }
    part.m_vertexBase = pStorage->m_vertices.m_data;
    part.m_vertexStriding = 3 * sizeof(float);
    part.m_numVertices = partIn.m_numVertices;

    // The indices, three per triangle; a striding of more than three indices is packed to three.
    const uint32_t uiIndexStriding = static_cast<uint32_t>(partIn.m_indexStriding);
    void* pIndexBase;
    if (partIn.m_stridingType.m_storage == hkpMeshShape::INDICES_INT16) {
        const int32_t iIndicesPerTriangle = static_cast<int32_t>(uiIndexStriding >> 1);
        part.m_indexStriding =
            static_cast<int32_t>(sizeof(uint16_t)) * (iIndicesPerTriangle > 2 ? 3 : iIndicesPerTriangle);
        uint16_t* pIndices = pStorage->m_indices16.expandBy(sub_D4B390(partIn.m_numTriangles, iIndicesPerTriangle));
        const uint16_t* pIndexIn = static_cast<const uint16_t*>(partIn.m_indexBase);
        pIndexBase = pIndices;
        for (int32_t i = 0; i < partIn.m_numTriangles; ++i) {
            pIndices[0] = pIndexIn[0];
            pIndices[1] = pIndexIn[1];
            pIndices[2] = pIndexIn[2];
            pIndices = reinterpret_cast<uint16_t*>(reinterpret_cast<char*>(pIndices) + part.m_indexStriding);
            pIndexIn =
                reinterpret_cast<const uint16_t*>(reinterpret_cast<const char*>(pIndexIn) + partIn.m_indexStriding);
        }
    }
    else {
        const int32_t iIndicesPerTriangle = static_cast<int32_t>(uiIndexStriding >> 2);
        part.m_indexStriding =
            static_cast<int32_t>(sizeof(uint32_t)) * (iIndicesPerTriangle > 2 ? 3 : iIndicesPerTriangle);
        uint32_t* pIndices = pStorage->m_indices32.expandBy(sub_D4B390(partIn.m_numTriangles, iIndicesPerTriangle));
        const uint32_t* pIndexIn = static_cast<const uint32_t*>(partIn.m_indexBase);
        pIndexBase = pIndices;
        for (int32_t i = 0; i < partIn.m_numTriangles; ++i) {
            pIndices[0] = pIndexIn[0];
            pIndices[1] = pIndexIn[1];
            pIndices[2] = pIndexIn[2];
            pIndices = reinterpret_cast<uint32_t*>(reinterpret_cast<char*>(pIndices) + part.m_indexStriding);
            pIndexIn =
                reinterpret_cast<const uint32_t*>(reinterpret_cast<const char*>(pIndexIn) + partIn.m_indexStriding);
        }
    }
    part.m_stridingType.m_storage = partIn.m_stridingType.m_storage;
    part.m_numTriangles = partIn.m_numTriangles;
    part.m_flipAlternateTriangles = partIn.m_flipAlternateTriangles;
    part.m_indexBase = pIndexBase;
    part.m_materialIndexStridingType.m_storage = partIn.m_materialIndexStridingType.m_storage;

    // The material indices, one per triangle, or the one shared index when the striding is 0.
    const void* pMaterialIndexIn = partIn.m_materialIndexBase;
    if (pMaterialIndexIn != nullptr) {
        if (partIn.m_materialIndexStridingType.m_storage == hkpMeshShape::MATERIAL_INDICES_INT8) {
            hkArray<uint8_t>& materialIndices = pStorage->m_materialIndices;
            if (partIn.m_materialIndexStriding == 0) {
                // EXE-INLINED none: hkArray<uint8_t>::pushBack
                hkMemoryAllocator& alloc = hkMemoryRouter::getInstance().heap();
                if (materialIndices.m_size == materialIndices.getCapacity()) {
                    hkArrayUtil::_reserveMore(alloc, &materialIndices, sizeof(uint8_t));
                }
                materialIndices.m_data[materialIndices.m_size] = *static_cast<const uint8_t*>(pMaterialIndexIn);
                ++materialIndices.m_size;
                part.m_materialIndexBase = &materialIndices.m_data[materialIndices.m_size - 1];
            }
            else {
                uint8_t* pMaterialIndices = materialIndices.expandBy(partIn.m_numTriangles);
                part.m_materialIndexBase = pMaterialIndices;
                const uint8_t* pIn = static_cast<const uint8_t*>(partIn.m_materialIndexBase);
                for (int32_t i = 0; i < partIn.m_numTriangles; ++i) {
                    pMaterialIndices[i] = *pIn;
                    pIn += partIn.m_materialIndexStriding;
                }
            }
        }
        else {
            hkArray<uint16_t>& materialIndices = pStorage->m_materialIndices16;
            if (partIn.m_materialIndexStriding == 0) {
                // EXE-INLINED none: hkArray<uint16_t>::pushBack
                hkMemoryAllocator& alloc = hkMemoryRouter::getInstance().heap();
                if (materialIndices.m_size == materialIndices.getCapacity()) {
                    hkArrayUtil::_reserveMore(alloc, &materialIndices, sizeof(uint16_t));
                }
                materialIndices.m_data[materialIndices.m_size] = *static_cast<const uint16_t*>(pMaterialIndexIn);
                ++materialIndices.m_size;
                part.m_materialIndexBase = &materialIndices.m_data[materialIndices.m_size - 1];
            }
            else {
                uint16_t* pMaterialIndices = materialIndices.expandBy(partIn.m_numTriangles);
                part.m_materialIndexBase = pMaterialIndices;
                const uint16_t* pIn = static_cast<const uint16_t*>(partIn.m_materialIndexBase);
                for (int32_t i = 0; i < partIn.m_numTriangles; ++i) {
                    pMaterialIndices[i] = *pIn;
                    pIn = reinterpret_cast<const uint16_t*>(
                        reinterpret_cast<const char*>(pIn) + partIn.m_materialIndexStriding
                    );
                }
            }
        }
        // The copies are packed: a non-zero striding becomes the index's size.
        part.m_materialIndexStriding = partIn.m_materialIndexStriding;
        if (part.m_materialIndexStriding != 0) {
            switch (part.m_materialIndexStridingType.m_storage) {
                case hkpMeshShape::MATERIAL_INDICES_INT8: part.m_materialIndexStriding = sizeof(uint8_t); break;
                case hkpMeshShape::MATERIAL_INDICES_INT16: part.m_materialIndexStriding = sizeof(uint16_t); break;
                default: break;
            }
        }
    }
    else {
        part.m_materialIndexBase = nullptr;
        part.m_materialIndexStriding = 0;
    }

    // The materials: their filter infos, packed, or the one shared material when the striding is 0. A subpart without
    // material indices points both at zeros.
    if (part.m_materialIndexBase != nullptr) {
        hkArray<uint32_t>& materials = pStorage->m_materials;
        if (partIn.m_materialStriding == 0) {
            const hkpMeshMaterial* pMaterialIn = partIn.m_materialBase;
            // EXE-INLINED none: hkArray<uint32_t>::pushBack
            hkMemoryAllocator& alloc = hkMemoryRouter::getInstance().heap();
            if (materials.m_size == materials.getCapacity()) {
                hkArrayUtil::_reserveMore(alloc, &materials, sizeof(uint32_t));
            }
            materials.m_data[materials.m_size] = pMaterialIn->m_filterInfo;
            materials.m_size += 1;
            part.m_materialStriding = 0;
            part.m_numMaterials = 1;
        }
        else {
            uint32_t* pMaterials = materials.expandBy(partIn.m_numMaterials);
            const hkpMeshMaterial* pMaterialIn = partIn.m_materialBase;
            for (int32_t i = 0; i < partIn.m_numMaterials; ++i) {
                pMaterials[i] = pMaterialIn->m_filterInfo;
                pMaterialIn = reinterpret_cast<const hkpMeshMaterial*>(
                    reinterpret_cast<const char*>(pMaterialIn) + partIn.m_materialStriding
                );
            }
            part.m_materialStriding = sizeof(uint32_t);
            part.m_numMaterials = partIn.m_numMaterials;
        }
        // Havok's storage keeps the materials as raw words (hkArray<hkUint32>, as in its header) that the subpart
        // reads as strided hkpMeshMaterial records.
        part.m_materialBase = reinterpret_cast<const hkpMeshMaterial*>(materials.m_data);
        part.m_triangleOffset = partIn.m_triangleOffset;
    }
    else {
        part.m_numMaterials = 1;
        // The SDK points a subpart without materials at its zero vector, read as one material with filter info 0.
        part.m_materialBase = reinterpret_cast<const hkpMeshMaterial*>(&hkVector4Zero);
        part.m_materialStriding = 0;
        part.m_materialIndexBase = &hkVector4Zero;
        part.m_triangleOffset = partIn.m_triangleOffset;
    }
}

const hkClass& hkpStorageMeshShape::staticClass() {
    return hkpStorageMeshShapeClass;
}
