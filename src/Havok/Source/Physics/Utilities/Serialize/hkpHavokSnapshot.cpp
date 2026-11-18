#include "Havok/Source/Physics/Utilities/Serialize/hkpHavokSnapshot.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/Array/hkArrayUtil.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/Registry/hkClassNameRegistry.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/System/Io/Reader/hkStreamReader.h"
#include "Havok/Source/Common/Base/System/Io/Writer/hkStreamWriter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkRefVariant.h"
#include "Havok/Source/Common/Serialize/Resource/hkResource.h"
#include "Havok/Source/Common/Serialize/Util/hkRootLevelContainer.h"
#include "Havok/Source/Common/Serialize/Util/hkSerializeUtil.h"
#include "Havok/Source/Common/Serialize/Util/hkStructureLayout.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShapeClass.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/StorageExtendedMesh/hkpStorageExtendedMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/StorageExtendedMesh/hkpStorageExtendedMeshShapeClass.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/Mesh/hkpMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/StorageMesh/hkpStorageMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/SampledHeightField/hkpSampledHeightFieldShape.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/StorageSampledHeightField/hkpStorageSampledHeightFieldShape.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorld.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpPhysicsData.h"

// Deletes the objects addObjectCallback created; the array goes with its member.
hkpHavokSnapshot::ConvertListener::~ConvertListener() {
    for (int32_t i = 0; i < m_objects.m_size; ++i) {
        if (m_objects.m_data[i]) {
            delete m_objects.m_data[i];
        }
    }
}

// A mesh becomes an hkpStorageMeshShape (and goes on to the next test), an extended mesh an
// hkpStorageExtendedMeshShape, a user-type sampled heightfield an hkpStorageSampledHeightFieldShape, each new object
// kept in m_objects; the world's fixed rigid body is dropped.
void hkpHavokSnapshot::ConvertListener::addObjectCallback(const void*& p, const hkClass*& k) {
    hkBool bIsMesh;
    if (hkpMeshShapeClass.isSuperClass(&bIsMesh, *k)->m_bool && k != &hkpStorageMeshShapeClass) {
        const hkpMeshShape* mesh = static_cast<const hkpMeshShape*>(p);
        hkpStorageMeshShape* pMemory = static_cast<hkpStorageMeshShape*>(
            hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpStorageMeshShape))
        );
        pMemory->m_memSizeAndFlags = sizeof(hkpStorageMeshShape);
        hkpStorageMeshShape* pStorage = ::new (pMemory) hkpStorageMeshShape(mesh);
        // EXE-INLINED none: hkArray<hkReferencedObject*>::pushBack
        hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
        if (m_objects.m_size == (m_objects.m_capacityAndFlags & hkArrayBase<hkReferencedObject*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*pRouter->m_heap, &m_objects, sizeof(hkReferencedObject*));
        }
        m_objects.m_data[m_objects.m_size++] = pStorage;
        p = pStorage;
        k = &hkpStorageMeshShapeClass;
    }
    hkBool bIsExtendedMesh;
    if (hkpExtendedMeshShapeClass.isSuperClass(&bIsExtendedMesh, *k)->m_bool &&
        k != &hkpStorageExtendedMeshShapeClass) {
        const hkpExtendedMeshShape* mesh = static_cast<const hkpExtendedMeshShape*>(p);
        hkpStorageExtendedMeshShape* pMemory = static_cast<hkpStorageExtendedMeshShape*>(
            hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpStorageExtendedMeshShape))
        );
        pMemory->m_memSizeAndFlags = sizeof(hkpStorageExtendedMeshShape);
        hkpStorageExtendedMeshShape* pStorage = ::new (pMemory) hkpStorageExtendedMeshShape(mesh);
        // EXE-INLINED none: hkArray<hkReferencedObject*>::pushBack
        hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
        if (m_objects.m_size == (m_objects.m_capacityAndFlags & hkArrayBase<hkReferencedObject*>::CAPACITY_MASK)) {
            hkArrayUtil::_reserveMore(*pRouter->m_heap, &m_objects, sizeof(hkReferencedObject*));
        }
        m_objects.m_data[m_objects.m_size++] = pStorage;
        p = pStorage;
        k = &hkpStorageExtendedMeshShapeClass;
        return;
    }
    hkBool bIsHeightField;
    if (hkpSampledHeightFieldShapeClass.isSuperClass(&bIsHeightField, *k)->m_bool) {
        const hkpSampledHeightFieldShape* hf = static_cast<const hkpSampledHeightFieldShape*>(p);
        if (hf->m_heightfieldType.m_storage == hkpSampledHeightFieldShape::HEIGHTFIELD_USER) {
            hkpStorageSampledHeightFieldShape* pMemory = static_cast<hkpStorageSampledHeightFieldShape*>(
                hkMemoryRouter::getInstance().m_heap->blockAlloc(sizeof(hkpStorageSampledHeightFieldShape))
            );
            pMemory->m_memSizeAndFlags = sizeof(hkpStorageSampledHeightFieldShape);
            hkpStorageSampledHeightFieldShape* pStorage = ::new (pMemory) hkpStorageSampledHeightFieldShape(hf);
            // EXE-INLINED none: hkArray<hkReferencedObject*>::pushBack
            hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
            if (m_objects.m_size == (m_objects.m_capacityAndFlags & hkArrayBase<hkReferencedObject*>::CAPACITY_MASK)) {
                hkArrayUtil::_reserveMore(*pRouter->m_heap, &m_objects, sizeof(hkReferencedObject*));
            }
            m_objects.m_data[m_objects.m_size++] = pStorage;
            p = pStorage;
            k = &hkpStorageSampledHeightFieldShapeClass;
        }
        return;
    }
    hkBool bIsRigidBody;
    if (hkpRigidBodyClass.isSuperClass(&bIsRigidBody, *k)->m_bool) {
        const hkpRigidBody* rb = static_cast<const hkpRigidBody*>(p);
        if (rb->m_world && rb->m_world->m_fixedRigidBody == rb) {
            p = nullptr;
            k = nullptr;
        }
    }
}

// The world save: a physics data populated from the world, saved under the root level.
hkBool* hkpHavokSnapshot::save(
    hkBool* apResult,
    const hkpWorld* apWorld,
    hkStreamWriter* apWriter,
    hkBool abBinaryFormat,
    const hkStructureLayout::LayoutRules* apTargetLayout,
    int32_t abSaveContactPoints
) {
    if (!apWorld || !apWriter) {
        apResult->m_bool = 0;
        return apResult;
    }
    char cResult;
    {
        hkpPhysicsData kData;
        kData.populateFromWorld(apWorld, abSaveContactPoints);
        hkBool bResult;
        cResult = hkpHavokSnapshot::save(&bResult, &kData, apWriter, abBinaryFormat, apTargetLayout)->m_bool;
    }
    apResult->m_bool = cResult;
    return apResult;
}

hkBool* hkpHavokSnapshot::save(
    hkBool* apResult,
    const void* apData,
    const hkClass& arDataClass,
    hkStreamWriter* apWriter,
    hkBool abBinaryFormat,
    [[maybe_unused]] const hkStructureLayout::LayoutRules* apTargetLayout,
    hkPackfileWriter::AddObjectListener* apUserListener
) {
    // EXE-INLINED none: hkpHavokSnapshot::ConvertListener's constructor
    XFNV_INLINED_LOCAL(
        hkpHavokSnapshot::ConvertListener, kDefaultConvertListener, hkpHavokSnapshot_ConvertListener_vftable,
        std::nullopt
    );
    kDefaultConvertListener.m_referenceCount = 1;
    kDefaultConvertListener.m_objects.m_data = nullptr;
    kDefaultConvertListener.m_objects.m_size = 0;
    kDefaultConvertListener.m_objects.m_capacityAndFlags = hkArrayBase<hkReferencedObject*>::DONT_DEALLOCATE_FLAG;
    hkPackfileWriter::AddObjectListener* pListener = apUserListener;
    if (!apUserListener) {
        pListener = &kDefaultConvertListener;
    }
    // Tests the hkBool's byte and passes it on zero-extended.
    apResult->m_bool =
        hkSerializeUtil::saveTagfile(apData, arDataClass, apWriter, abBinaryFormat.m_bool != 0, pListener) ==
        HK_SUCCESS;
    kDefaultConvertListener.~ConvertListener();
    return apResult;
}

hkBool* hkpHavokSnapshot::saveUnderRootLevel(
    hkBool* apResult,
    const void* apData,
    const hkClass& arDataClass,
    hkStreamWriter* apWriter,
    hkBool abBinaryFormat,
    const hkStructureLayout::LayoutRules* apTargetLayout
) {
    using NamedVariant = hkRootLevelContainer::NamedVariant;
    hkRootLevelContainer kContainer;
    hkArray<NamedVariant>& rVariants = kContainer.m_namedVariants;
    rVariants.m_data = nullptr;
    rVariants.m_size = 0;
    rVariants.m_capacityAndFlags = hkArrayBase<NamedVariant>::DONT_DEALLOCATE_FLAG;
    // EXE-INLINED none: hkArray<NamedVariant>::expandOne
    hkMemoryRouter& rRouter = hkMemoryRouter::getInstance();
    if (rVariants.m_size == rVariants.getCapacity()) {
        hkArrayUtil::_reserveMore(*rRouter.m_heap, &rVariants, sizeof(NamedVariant));
    }
    NamedVariant* pNew = rVariants.m_data + rVariants.m_size;
    if (pNew) {
        // EXE-INLINED none: NamedVariant's default constructor
        ::new (&pNew->m_name) hkStringPtr;
        ::new (&pNew->m_className) hkStringPtr;
        pNew->m_variant.m_pntr = nullptr;
    }
    NamedVariant& rVariant = rVariants.m_data[rVariants.m_size++];
    // EXE-INLINED none: NamedVariant::set
    rVariant.m_name = arDataClass.getName();
    rVariant.m_variant.set(const_cast<void*>(apData), &arDataClass);
    rVariant.m_className = rVariant.m_variant.getClass() ? rVariant.m_variant.getClass()->getName() : nullptr;
    hkpHavokSnapshot::save(
        apResult, &kContainer, hkRootLevelContainerClass, apWriter, abBinaryFormat, apTargetLayout, nullptr
    );
    // EXE-INLINED none: hkArray<NamedVariant>'s destructor
    hkMemoryAllocator* pHeap = hkMemoryRouter::getInstance().m_heap;
    if (rVariants.m_size > 0) {
        NamedVariant* pVariant = rVariants.m_data;
        int32_t iCount = rVariants.m_size;
        do {
            if (pVariant->m_variant.m_pntr) {
                pVariant->m_variant.m_pntr->removeReference();
            }
            pVariant->m_variant.m_pntr = nullptr;
            pVariant->m_className.~hkStringPtr();
            pVariant->m_name.~hkStringPtr();
            ++pVariant;
        }
        while (--iCount);
    }
    rVariants.m_size = 0;
    if (rVariants.m_capacityAndFlags >= 0) {
        pHeap->bufFree(
            rVariants.m_data,
            sizeof(NamedVariant) * (rVariants.m_capacityAndFlags & hkArrayBase<NamedVariant>::CAPACITY_MASK)
        );
    }
    return apResult;
}

hkBool* hkpHavokSnapshot::save(
    hkBool* apResult,
    const hkpPhysicsData* apData,
    hkStreamWriter* apWriter,
    hkBool abBinaryFormat,
    const hkStructureLayout::LayoutRules* apTargetLayout
) {
    hkpHavokSnapshot::saveUnderRootLevel(
        apResult, apData, hkpPhysicsDataClass, apWriter, abBinaryFormat, apTargetLayout
    );
    return apResult;
}

// The physics data of a snapshot: the root container's first hkpPhysicsData, else the object named "SnapshotSave"
// or "hkpPhysicsData". The resource goes to allocatedData only when one is found.
hkpPhysicsData* hkpHavokSnapshot::load(hkStreamReader* reader, hkResource** allocatedData) {
    if (!reader || !allocatedData) {
        return nullptr;
    }
    hkResource* pResource = hkSerializeUtil::load(reader, nullptr, nullptr);
    if (!pResource) {
        return nullptr;
    }
    hkRootLevelContainer* pContainer = pResource->getContents<hkRootLevelContainer>();
    if (!pContainer) {
        return nullptr;
    }
    hkpPhysicsData* pData =
        static_cast<hkpPhysicsData*>(pContainer->findObjectByType(hkpPhysicsDataClass.getName(), nullptr));
    if (!pData) {
        const char* apcNames[] = {"SnapshotSave", "hkpPhysicsData", nullptr};
        int32_t i = 0;
        const char* pcName = apcNames[0];
        do {
            pData = static_cast<hkpPhysicsData*>(pContainer->findObjectByName(pcName, nullptr));
            if (pData) {
                break;
            }
            pcName = apcNames[++i];
        }
        while (pcName);
        if (!pData) {
            return nullptr;
        }
    }
    *allocatedData = pResource;
    return pData;
}

hkRootLevelContainer::NamedVariant::NamedVariant(const char* name, void* object, const hkClass* klass)
  : m_name(), m_className(), m_variant() {
    m_name = name;
    m_variant.set(object, klass);
    m_className = m_variant.getClass() ? m_variant.getClass()->getName() : nullptr;
}

// Natively: NamedVariant(const char*, const hkVariant&) below.
hkRootLevelContainer::NamedVariant::NamedVariant(const char* name, const hkVariant& v)
  : m_name(), m_className(), m_variant(v.m_object, v.m_class) {
    m_name = name;
    m_className = m_variant.getClass() ? m_variant.getClass()->getName() : nullptr;
}

// m_variant stays as loaded.
hkRootLevelContainer::NamedVariant::NamedVariant(hkFinishLoadedObjectFlag flag) : m_name(flag), m_className(flag) {}

const hkClass& hkRootLevelContainer::NamedVariant::staticClass() {
    return hkRootLevelContainerNamedVariantClass;
}
