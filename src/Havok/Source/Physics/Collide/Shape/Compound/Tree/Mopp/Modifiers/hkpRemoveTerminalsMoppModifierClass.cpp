#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/Modifiers/hkpRemoveTerminalsMoppModifierClass.h"

#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Filter/Group/hkpGroupFilter.h"
#include "Havok/Source/Physics/Collide/Filter/Null/hkpNullCollisionFilter.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/Modifiers/hkpRemoveTerminalsMoppModifier.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Sphere/hkpSphereShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Triangle/hkpTriangleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/FastMesh/hkpFastMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Deprecated/MultiSphere/hkpMultiSphereShape.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/Plane/hkpPlaneShape.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/TriSampledHeightField/hkpTriSampledHeightFieldBvTreeShape.h"
#include "Havok/Source/Physics/Collide/Shape/HeightField/TriSampledHeightField/hkpTriSampledHeightFieldCollection.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Transform/hkpTransformShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Collide/Util/ShapeInfo/hkpShapeInfo.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpAction.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpLinearParametricCurve.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainInstance.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainInstanceAction.h"
#include "Havok/Source/Physics/Dynamics/Constraint/ConstraintKit/hkpGenericConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Malleable/hkpMalleableConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintInstance.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpFixedRigidMotion.h"
#include "Havok/Source/Physics/Dynamics/Phantom/hkpAabbPhantom.h"
#include "Havok/Source/Physics/Dynamics/World/Memory/Default/hkpDefaultWorldMemoryWatchDog.h"
#include "Havok/Source/Physics/Internal/Collide/Mopp/Machine/hkpMoppModifier.h"

// The class record's member table.
using hkpRemoveTerminalsMoppModifierClassMemberArray = const hkClassMember[2];
static hkpRemoveTerminalsMoppModifierClassMemberArray hkpRemoveTerminalsMoppModifierClass_Members{
    {"removeInfo", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_UINT32}, 0, 0, 0xC, nullptr},
    {"tempShapesToRemove",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x18,
     nullptr}
};
// Passes one implemented interface with a null table.
hkClass hkpRemoveTerminalsMoppModifierClass(
    "hkpRemoveTerminalsMoppModifier",
    &hkReferencedObjectClass,
    0x1C,
    nullptr,
    1,
    nullptr,
    0,
    hkpRemoveTerminalsMoppModifierClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpRemoveTerminalsMoppModifierTypeInfo{
    "hkpRemoveTerminalsMoppModifier",
    finishLoadedObjecthkpRemoveTerminalsMoppModifier,
    cleanupLoadedObjecthkpRemoveTerminalsMoppModifier,
    getVtablehkpRemoveTerminalsMoppModifier(),
    &typeid(hkpRemoveTerminalsMoppModifier),
    sizeof(hkpRemoveTerminalsMoppModifier)
};

void finishLoadedObjecthkpGenericConstraintData(void* p, int32_t finishing) {
    hkpGenericConstraintData* obj = static_cast<hkpGenericConstraintData*>(p);
    int32_t flag = finishing;
    if (obj) {
        ::new (obj) hkpGenericConstraintData(flag);
    }
}

// Runs the finish constructor hkpTriangleShape(flag) in place.
void finishLoadedObjecthkpTriangleShape(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpTriangleShape(kFlag);
    }
}

// The type info's finish-loaded-object callback for hkpLinearParametricCurve: its finish constructor.
void finishLoadedObjecthkpLinearParametricCurve(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpLinearParametricCurve(kFlag);
    }
}

const void* getVtablehkpLinearParametricCurve() {
    return hkGetVtableOf<hkpLinearParametricCurve>();
}

// The type info's finish-loaded-object callback for hkpFixedRigidMotion: runs its finish constructor.
void finishLoadedObjecthkpFixedRigidMotion(void* p, int32_t finishing) {
    if (p) {
        ::new (p) hkpFixedRigidMotion(finishing);
    }
}

const void* getVtablehkpFixedRigidMotion() {
    return hkGetVtableOf<hkpFixedRigidMotion>();
}

// The type info's finish-loaded-object callback for hkpDefaultWorldMemoryWatchDog: its finish constructor.
void finishLoadedObjecthkpDefaultWorldMemoryWatchDog(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpDefaultWorldMemoryWatchDog(kFlag);
    }
}

const void* getVtablehkpDefaultWorldMemoryWatchDog() {
    return hkGetVtableOf<hkpDefaultWorldMemoryWatchDog>();
}

const void* getVtablehkpConstraintChainInstanceAction() {
    return hkGetVtableOf<hkpConstraintChainInstanceAction>();
}

// As above, for a constraint chain instance action: the action's finish constructor.
void finishLoadedObjecthkpConstraintChainInstanceAction(void* p, int32_t finishing) {
    if (!p) {
        return;
    }
    hkFinishLoadedObjectFlag kFlag;
    kFlag.m_finishing = finishing;
    ::new (p) hkpConstraintChainInstanceAction(kFlag);
}

// The finish constructor for a constraint chain instance.
void finishLoadedObjecthkpConstraintChainInstance(void* p, int32_t finishing) {
    if (!p) {
        return;
    }
    hkFinishLoadedObjectFlag kFlag;
    kFlag.m_finishing = finishing;
    ::new (p) hkpConstraintChainInstance(kFlag);
}

const void* getVtablehkpTriangleShape() {
    return hkGetVtableOf<hkpTriangleShape>();
}

// Havok's getVtable: a finish-constructed object on the stack (flag 0), whose vptr is returned.
const void* getVtablehkpTriSampledHeightFieldCollection() {
    // The class has no native default constructor.
    return hkGetVtableOf<hkpTriSampledHeightFieldCollection>();
}

void finishLoadedObjecthkpTriSampledHeightFieldCollection(void* p, int32_t finishing) {
    void* apMemory = p;
    hkFinishLoadedObjectFlag aFlag;
    aFlag.m_finishing = finishing;
    if (!apMemory) {
        return;
    }
    ::new (apMemory) hkpTriSampledHeightFieldCollection(aFlag);
}

// As above.
const void* getVtablehkpTriSampledHeightFieldBvTreeShape() {
    // The class has no native default constructor.
    return hkGetVtableOf<hkpTriSampledHeightFieldBvTreeShape>();
}

// The type info's finish-loaded-object callback for hkpTransformShape: runs its finish constructor.
void finishLoadedObjecthkpTransformShape(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpTransformShape(kFlag);
    }
}

const void* getVtablehkpTransformShape() {
    return hkGetVtableOf<hkpTransformShape>();
}

void finishLoadedObjecthkpSphereShape(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpSphereShape(kFlag);
    }
}

const void* getVtablehkpSphereShape() {
    return hkGetVtableOf<hkpSphereShape>();
}

// The type info's finish-loaded-object callback for hkpShapeInfo: its finish constructor.
void finishLoadedObjecthkpShapeInfo(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpShapeInfo(kFlag);
    }
}

// The type info's finish-loaded-object callback for hkpRemoveTerminalsMoppModifier: its finish constructor.
void finishLoadedObjecthkpRemoveTerminalsMoppModifier(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpRemoveTerminalsMoppModifier(kFlag);
    }
}

// The class registry's finish-loaded-object callback for hkpFastMeshShape: its finish constructor.
void finishLoadedObjecthkpFastMeshShape(void* p, int32_t finishing) {
    if (!p) {
        return;
    }
    hkFinishLoadedObjectFlag kFlag;
    kFlag.m_finishing = finishing;
    ::new (p) hkpFastMeshShape(kFlag);
}

// Runs the finish constructor hkpPlaneShape(flag) in place.
void finishLoadedObjecthkpPlaneShape(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpPlaneShape(kFlag);
    }
}

const void* getVtablehkpPlaneShape() {
    return hkGetVtableOf<hkpPlaneShape>();
}

const void* getVtablehkpNullCollisionFilter() {
    return hkGetVtableOf<hkpNullCollisionFilter>();
}

void finishLoadedObjecthkpNullCollisionFilter(void* p, int32_t finishing) {
    void* apMemory = p;
    hkFinishLoadedObjectFlag aFlag;
    aFlag.m_finishing = finishing;
    if (!apMemory) {
        return;
    }
    ::new (apMemory) hkpNullCollisionFilter(aFlag);
}

const void* getVtablehkpMultiSphereShape() {
    return hkGetVtableOf<hkpMultiSphereShape>();
}

// The reflection's getVtable for hkpGroupFilter: runs its finish constructor (flag 0) on a 16-aligned stack object and
// returns the vtable that constructor stores.
const void* getVtablehkpGroupFilter() {
    return hkGetVtableOf<hkpGroupFilter>();
}

void finishLoadedObjecthkpGroupFilter(void* p, int32_t finishing) {
    void* apMemory = p;
    hkFinishLoadedObjectFlag aFlag;
    aFlag.m_finishing = finishing;
    if (!apMemory) {
        return;
    }
    ::new (apMemory) hkpGroupFilter(aFlag);
}

void cleanupLoadedObjecthkpLinearParametricCurve(void* p) {
    static_cast<hkpLinearParametricCurve*>(p)->~hkpLinearParametricCurve();
}

void cleanupLoadedObjecthkpFixedRigidMotion(void* p) {
    static_cast<hkpFixedRigidMotion*>(p)->~hkpFixedRigidMotion();
}

void cleanupLoadedObjecthkpDefaultWorldMemoryWatchDog(void* p) {
    static_cast<hkpDefaultWorldMemoryWatchDog*>(p)->~hkpDefaultWorldMemoryWatchDog();
}

void cleanupLoadedObjecthkpTriSampledHeightFieldCollection(void* p) {
    static_cast<hkpTriSampledHeightFieldCollection*>(p)->~hkpTriSampledHeightFieldCollection();
}

void cleanupLoadedObjecthkpTriSampledHeightFieldBvTreeShape(void* p) {
    static_cast<hkpTriSampledHeightFieldBvTreeShape*>(p)->~hkpTriSampledHeightFieldBvTreeShape();
}

void finishLoadedObjecthkpTriSampledHeightFieldBvTreeShape(void* p, int32_t finishing) {
    void* apMemory = p;
    hkFinishLoadedObjectFlag aFlag;
    aFlag.m_finishing = finishing;
    if (!apMemory) {
        return;
    }
    ::new (apMemory) hkpTriSampledHeightFieldBvTreeShape(aFlag);
}

void cleanupLoadedObjecthkpRemoveTerminalsMoppModifier(void* p) {
    static_cast<hkpRemoveTerminalsMoppModifier*>(p)->~hkpRemoveTerminalsMoppModifier();
}

void cleanupLoadedObjecthkpPlaneShape(void* p) {
    static_cast<hkpPlaneShape*>(p)->~hkpPlaneShape();
}

void cleanupLoadedObjecthkpNullCollisionFilter(void* p) {
    static_cast<hkpNullCollisionFilter*>(p)->~hkpNullCollisionFilter();
}

void cleanupLoadedObjecthkpMultiSphereShape(void* p) {
    static_cast<hkpMultiSphereShape*>(p)->~hkpMultiSphereShape();
}

// As getVtablehkpTriSampledHeightFieldCollection.
const void* getVtablehkpMalleableConstraintData() {
    return hkGetVtableOf<hkpMalleableConstraintData>();
}

// As above.
const void* getVtablehkpGenericConstraintData() {
    // The finish constructor's flag is an int (0: nothing to finish).
    alignas(16) uint8_t storage[sizeof(hkpGenericConstraintData)];
    return *reinterpret_cast<const void* const*>(::new (storage) hkpGenericConstraintData(0));
}

// As above.
const void* getVtablehkpAabbPhantom() {
    // The class has no native default constructor.
    return hkGetVtableOf<hkpAabbPhantom>();
}

// The class registry's finish-loaded-object callback: the null check, then the class's finish constructor.
void finishLoadedObjecthkpAabbPhantom(void* p, int32_t finishing) {
    void* apMemory = p;
    hkFinishLoadedObjectFlag aFlag;
    aFlag.m_finishing = finishing;
    if (!apMemory) {
        return;
    }
    ::new (apMemory) hkpAabbPhantom(aFlag);
}

const void* getVtablehkpRemoveTerminalsMoppModifier() {
    return hkGetVtableOf<hkpRemoveTerminalsMoppModifier>();
}

hkpGroupFilter::hkpGroupFilter(hkFinishLoadedObjectFlag aFlag) : hkpCollisionFilter(aFlag) {
    if (aFlag.m_finishing) {
        m_type.m_storage = hkpCollisionFilter::HK_FILTER_GROUP;
    }
}

hkpNullCollisionFilter::hkpNullCollisionFilter(hkFinishLoadedObjectFlag aFlag) : hkpCollisionFilter(aFlag) {
    if (aFlag.m_finishing) {
        m_type.m_storage = hkpCollisionFilter::HK_FILTER_NULL;
    }
}

const hkClass& hkpRemoveTerminalsMoppModifier::staticClass() {
    return hkpRemoveTerminalsMoppModifierClass;
}

// The class registry's finish-loaded-object callback for a malleable constraint data: the null check, then the data's
// finish constructor.
void finishLoadedObjecthkpMalleableConstraintData(void* p, int32_t finishing) {
    if (!p) {
        return;
    }
    hkFinishLoadedObjectFlag kFlag;
    kFlag.m_finishing = finishing;
    ::new (static_cast<hkpMalleableConstraintData*>(p)) hkpMalleableConstraintData(kFlag);
}

const hkClass& hkpFixedRigidMotion::staticClass() {
    return hkpFixedRigidMotionClass;
}

hkpTriSampledHeightFieldBvTreeShape::hkpTriSampledHeightFieldBvTreeShape(hkFinishLoadedObjectFlag aFlag)
  : hkpBvTreeShape(aFlag), m_childContainer(aFlag) {
    if (aFlag.m_finishing) {
        m_type.m_storage = HK_SHAPE_BV_TREE;
        m_bvTreeType.m_storage = hkpBvTreeShape::BVTREE_TRISAMPLED_HEIGHTFIELD;
    }
}

hkpTriSampledHeightFieldCollection::hkpTriSampledHeightFieldCollection(hkFinishLoadedObjectFlag aFlag)
  : hkpShapeCollection(aFlag), m_weldingInfo(aFlag) {
    if (aFlag.m_finishing) {
        m_type.m_storage = HK_SHAPE_COLLECTION;
        m_collectionType.m_storage = hkpShapeCollection::COLLECTION_TRISAMPLED_HEIGHTFIELD;
    }
}

// The SDK's finish constructor: hkpPhantom's, and the overlap array left empty.
hkpAabbPhantom::hkpAabbPhantom(hkFinishLoadedObjectFlag aFlag) : hkpPhantom(aFlag) {}

hkpMoppModifier::~hkpMoppModifier() {}

void hkpMoppModifier::operator delete(void* apMemory) {
    ourFree(apMemory);
}
