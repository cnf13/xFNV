#include "Havok/Source/Common/SceneData/Attributes/hkxAnimatedMatrix.h"

#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkMatrix4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/Geometry/hkGeometry.h"

static void finishLoadedObjecthkxAnimatedMatrix(void* p, int32_t finishing);
// Havok SDK signature (hkSceneData.lib, tied by reloc,target).
static const void* getVtablehkxAnimatedMatrix();
static void cleanupLoadedObjecthkxAnimatedMatrix(void* p);

// The class record's member table. Its enum member reads the enum record through hkxAttributeHintEnum.
using hkxAnimatedMatrixClassMemberTable = hkClassMember[2];
static hkxAnimatedMatrixClassMemberTable hkxAnimatedMatrixClass_Members{
    {"matrices", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_MATRIX4}, 0, 0, 0x8, nullptr},
    {"hint",
     nullptr,
     hkxAttributeHintEnum,
     {hkClassMember::TYPE_ENUM},
     {hkClassMember::TYPE_UINT8},
     0,
     0,
     0x14,
     nullptr}
};
hkClass hkxAnimatedMatrixClass(
    "hkxAnimatedMatrix",
    &hkReferencedObjectClass,
    24,
    nullptr,
    0,
    nullptr,
    0,
    hkxAnimatedMatrixClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    1
);
hkTypeInfo hkxAnimatedMatrixTypeInfo{
    "hkxAnimatedMatrix",
    finishLoadedObjecthkxAnimatedMatrix,
    cleanupLoadedObjecthkxAnimatedMatrix,
    getVtablehkxAnimatedMatrix(),
    &typeid(hkxAnimatedMatrix),
    sizeof(hkxAnimatedMatrix)
};

// The type info's finish-loaded-object callback for hkxAnimatedMatrix: its finish constructor, when the object is
// not null.
static void finishLoadedObjecthkxAnimatedMatrix(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkxAnimatedMatrix(flag);
    }
}

static const void* getVtablehkxAnimatedMatrix() {
    return hkGetVtableOf<hkxAnimatedMatrix>();
}

static void cleanupLoadedObjecthkxAnimatedMatrix(void* p) {
    static_cast<hkxAnimatedMatrix*>(p)->~hkxAnimatedMatrix();
}

// The member frees the matrices.
hkxAnimatedMatrix::~hkxAnimatedMatrix() {}

const hkClass& hkxAnimatedMatrix::staticClass() {
    return hkxAnimatedMatrixClass;
}
