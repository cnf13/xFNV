#include "Havok/Source/Physics/Utilities/Serialize/Display/hkpSerializedDisplayRbTransforms.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"

static void cleanupLoadedObjecthkpSerializedDisplayRbTransformsDisplayTransformPair(void* p);
static void finishLoadedObjecthkpSerializedDisplayRbTransforms(void* p, int32_t finishing);
static void cleanupLoadedObjecthkpSerializedDisplayRbTransforms(void* p);

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpSerializedDisplayRbTransformsDisplayTransformPairTypeInfo = {
    "hkpSerializedDisplayRbTransformsDisplayTransformPair",
    nullptr,
    cleanupLoadedObjecthkpSerializedDisplayRbTransformsDisplayTransformPair,
    nullptr,
    &typeid(hkpSerializedDisplayRbTransforms::DisplayTransformPair),
    sizeof(hkpSerializedDisplayRbTransforms::DisplayTransformPair)
};

// The reflected members of hkpSerializedDisplayRbTransforms::DisplayTransformPair and hkpSerializedDisplayRbTransforms.
using hkpSerializedDisplayRbTransformsDisplayTransformPairClass_MembersArray = const hkClassMember[2];
static hkpSerializedDisplayRbTransformsDisplayTransformPairClass_MembersArray
    hkpSerializedDisplayRbTransformsDisplayTransformPairClass_Members{
        {"rb",
         &hkpRigidBodyClass,
         nullptr,
         {hkClassMember::TYPE_POINTER},
         {hkClassMember::TYPE_STRUCT},
         0,
         0,
         0x0,
         nullptr},
        {"localToDisplay",
         nullptr,
         nullptr,
         {hkClassMember::TYPE_TRANSFORM},
         {hkClassMember::TYPE_VOID},
         0,
         0,
         0x10,
         nullptr}
};
using hkpSerializedDisplayRbTransformsClass_MembersArray = const hkClassMember[1];
static hkpSerializedDisplayRbTransformsClass_MembersArray hkpSerializedDisplayRbTransformsClass_Members{
    {"transforms",
     &hkpSerializedDisplayRbTransformsDisplayTransformPairClass,
     nullptr,
     {hkClassMember::TYPE_ARRAY},
     {hkClassMember::TYPE_STRUCT},
     0,
     0,
     0x8,
     nullptr}
};
hkClass hkpSerializedDisplayRbTransformsDisplayTransformPairClass(
    "hkpSerializedDisplayRbTransformsDisplayTransformPair",
    nullptr,
    80,
    nullptr,
    0,
    nullptr,
    0,
    hkpSerializedDisplayRbTransformsDisplayTransformPairClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkClass hkpSerializedDisplayRbTransformsClass(
    "hkpSerializedDisplayRbTransforms",
    &hkReferencedObjectClass,
    20,
    nullptr,
    0,
    nullptr,
    0,
    hkpSerializedDisplayRbTransformsClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpSerializedDisplayRbTransformsTypeInfo = {
    "hkpSerializedDisplayRbTransforms",
    finishLoadedObjecthkpSerializedDisplayRbTransforms,
    cleanupLoadedObjecthkpSerializedDisplayRbTransforms,
    getVtablehkpSerializedDisplayRbTransforms(),
    &typeid(hkpSerializedDisplayRbTransforms),
    sizeof(hkpSerializedDisplayRbTransforms)
};

static void cleanupLoadedObjecthkpSerializedDisplayRbTransformsDisplayTransformPair(void* p) {
    ;
}

// The type info's finish-loaded-object callback for hkpSerializedDisplayRbTransforms: the null check, then its finish
// constructor.
static void finishLoadedObjecthkpSerializedDisplayRbTransforms(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag kFlag;
        kFlag.m_finishing = finishing;
        ::new (p) hkpSerializedDisplayRbTransforms(kFlag);
    }
}

static void cleanupLoadedObjecthkpSerializedDisplayRbTransforms(void* p) {
    static_cast<hkpSerializedDisplayRbTransforms*>(p)->~hkpSerializedDisplayRbTransforms();
}

const void* getVtablehkpSerializedDisplayRbTransforms() {
    return hkGetVtableOf<hkpSerializedDisplayRbTransforms>();
}

const hkClass& hkpSerializedDisplayRbTransforms::DisplayTransformPair::staticClass() {
    return hkpSerializedDisplayRbTransformsDisplayTransformPairClass;
}

// Each pair's rigid body loses its reference; the array's storage goes with m_transforms.
hkpSerializedDisplayRbTransforms::~hkpSerializedDisplayRbTransforms() {
    for (const DisplayTransformPair& rPair : m_transforms) {
        rPair.m_rb->removeReference();
    }
}

const hkClass& hkpSerializedDisplayRbTransforms::staticClass() {
    return hkpSerializedDisplayRbTransformsClass;
}
