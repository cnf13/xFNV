#include "Havok/Source/Common/SceneData/Attributes/hkxSparselyAnimatedEnum.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/Memory/hkMemoryMeshShape.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroup.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxEnum.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxSparselyAnimatedInt.h"

static void cleanupLoadedObjecthkxSparselyAnimatedEnum(void* p);

// The class record's member table.
using hkxSparselyAnimatedEnumClassMemberArray = const hkClassMember[1];
static hkxSparselyAnimatedEnumClassMemberArray hkxSparselyAnimatedEnumClass_Members{
    {"enum", &hkxEnumClass, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_STRUCT}, 0, 0, 0x20, nullptr}
};
hkClass hkxSparselyAnimatedEnumClass(
    "hkxSparselyAnimatedEnum",
    &hkxSparselyAnimatedIntClass,
    36,
    nullptr,
    0,
    nullptr,
    0,
    hkxSparselyAnimatedEnumClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    1
);
hkTypeInfo hkxSparselyAnimatedEnumTypeInfo = {
    "hkxSparselyAnimatedEnum",
    finishLoadedObjecthkxSparselyAnimatedEnum,
    cleanupLoadedObjecthkxSparselyAnimatedEnum,
    getVtablehkxSparselyAnimatedEnum(),
    &typeid(hkxSparselyAnimatedEnum),
    sizeof(hkxSparselyAnimatedEnum)
};

static void cleanupLoadedObjecthkxSparselyAnimatedEnum(void* p) {
    static_cast<hkxSparselyAnimatedEnum*>(p)->~hkxSparselyAnimatedEnum();
}

// The member releases the enum; the base frees the samples.
hkxSparselyAnimatedEnum::~hkxSparselyAnimatedEnum() {}

const hkClass& hkxSparselyAnimatedEnum::staticClass() {
    return hkxSparselyAnimatedEnumClass;
}
