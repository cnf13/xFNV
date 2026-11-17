#include "Havok/Source/Animation/Animation/Mapper/hkaSkeletonMapperClass.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Mapper/hkaSkeletonMapper.h"
#include "Havok/Source/Animation/Animation/MotionAnalysis/hkaFootstepAnalysisInfo.h"
#include "Havok/Source/Animation/Ragdoll/Instance/hkaRagdollInstance.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"

// Havok SDK signature (hkaAnimation.lib, tied by bytes,reloc).
static const void* getVtablehkaSkeletonMapper();

// The reflection's enum records: the names of ConstraintSource's values, and the enum; then the member table.
using hkaSkeletonMapperConstraintSourceEnumItemsArray = const hkClassEnum::Item[3];
static hkaSkeletonMapperConstraintSourceEnumItemsArray hkaSkeletonMapperConstraintSourceEnumItems{
    {0, "NO_CONSTRAINTS"},
    {1, "REFERENCE_POSE"},
    {2, "CURRENT_POSE"}
};
using hkaSkeletonMapperEnumsArray = const hkClassEnum[1];
static hkaSkeletonMapperEnumsArray hkaSkeletonMapperEnums{
    {"ConstraintSource", hkaSkeletonMapperConstraintSourceEnumItems, 3, nullptr, {0}}
};
using hkaSkeletonMapperClass_MembersArray = const hkClassMember[1];
static hkaSkeletonMapperClass_MembersArray hkaSkeletonMapperClass_Members{
    {"mapping",
     &hkaSkeletonMapperDataClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x10,
     nullptr}
};
hkClass hkaSkeletonMapperClass(
    "hkaSkeletonMapper",
    &hkReferencedObjectClass,
    0x80,
    nullptr,
    0,
    hkaSkeletonMapperEnums,
    1,
    hkaSkeletonMapperClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);

static const void* getVtablehkaSkeletonMapper() {
    return hkGetVtableOf<hkaSkeletonMapper>();
}

// The finish function the reflection uses for hkaSkeletonMapper: constructs the loaded mapper in place with its finish
// constructor.
static void finishLoadedObjecthkaSkeletonMapper(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkaSkeletonMapper(flag);
    }
}

// After finishLoadedObjecthkaSkeletonMapper, which the record names.
hkTypeInfo hkaSkeletonMapperTypeInfo{
    "hkaSkeletonMapper",          finishLoadedObjecthkaSkeletonMapper, cleanupLoadedObjecthkaSkeletonMapper,
    getVtablehkaSkeletonMapper(), &typeid(hkaSkeletonMapper),          sizeof(hkaSkeletonMapper)
};

// The type info's vtable getter for hkaRagdollInstance: finish-constructs one on a 16-aligned stack object and returns
// its vtable.
const void* getVtablehkaRagdollInstance() {
    return hkGetVtableOf<hkaRagdollInstance>();
}

void cleanupLoadedObjecthkaRagdollInstance(void* p) {
    static_cast<hkaRagdollInstance*>(p)->~hkaRagdollInstance();
}

void cleanupLoadedObjecthkaSkeletonMapper(void* p) {
    static_cast<hkaSkeletonMapper*>(p)->~hkaSkeletonMapper();
}

const hkClass& hkaSkeletonMapper::staticClass() {
    return hkaSkeletonMapperClass;
}

// The reflection's finish function for the class: finishes the loaded object with its finish constructor, when the
// object is not null.
void finishLoadedObjecthkaFootstepAnalysisInfo(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkaFootstepAnalysisInfo(flag);
    }
}

// The loaded object's arrays stay as loaded: each is built with its own finish constructor.
hkaFootstepAnalysisInfo::hkaFootstepAnalysisInfo(hkFinishLoadedObjectFlag flag)
  : hkReferencedObject(flag), m_name(flag), m_nameStrike(flag), m_nameLift(flag), m_nameLock(flag), m_nameUnlock(flag),
    m_minPos(flag), m_maxPos(flag), m_minVel(flag), m_maxVel(flag), m_allBonesDown(flag), m_anyBonesDown(flag) {}

// The finish function the reflection uses for the class: finishes the loaded object with its finish constructor, when
// the object is not null.
void finishLoadedObjecthkaRagdollInstance(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkaRagdollInstance(flag);
    }
}

// The loaded object keeps what was loaded: only the vptr is set.
hkaRagdollInstance::hkaRagdollInstance(hkFinishLoadedObjectFlag flag)
  : hkReferencedObject(flag), m_rigidBodies(flag), m_constraints(flag), m_boneToRigidBodyMap(flag) {}
