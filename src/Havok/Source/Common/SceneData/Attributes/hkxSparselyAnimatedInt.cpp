#include "Havok/Source/Common/SceneData/Attributes/hkxSparselyAnimatedInt.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/Memory/hkMemoryMeshShape.h"

static void cleanupLoadedObjecthkxSparselyAnimatedInt(void* p);

// The class record's member table.
using hkxSparselyAnimatedIntClassMemberArray = const hkClassMember[2];
static hkxSparselyAnimatedIntClassMemberArray hkxSparselyAnimatedIntClass_Members{
    {"ints", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_INT32}, 0, 0, 0x8, nullptr},
    {"times", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_REAL}, 0, 0, 0x14, nullptr}
};
hkClass hkxSparselyAnimatedIntClass(
    "hkxSparselyAnimatedInt",
    &hkReferencedObjectClass,
    32,
    nullptr,
    0,
    nullptr,
    0,
    hkxSparselyAnimatedIntClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    1
);
hkTypeInfo hkxSparselyAnimatedIntTypeInfo = {"hkxSparselyAnimatedInt",
                                             finishLoadedObjecthkxSparselyAnimatedInt,
                                             cleanupLoadedObjecthkxSparselyAnimatedInt,
                                             getVtablehkxSparselyAnimatedInt(),
                                             &typeid(hkxSparselyAnimatedInt),
                                             sizeof(hkxSparselyAnimatedInt)};

static void cleanupLoadedObjecthkxSparselyAnimatedInt(void* p) {
    static_cast<hkxSparselyAnimatedInt*>(p)->~hkxSparselyAnimatedInt();
}

// The members free the times and the ints.
hkxSparselyAnimatedInt::~hkxSparselyAnimatedInt() {}

const hkClass& hkxSparselyAnimatedInt::staticClass() {
    return hkxSparselyAnimatedIntClass;
}
