#include "Havok/Source/Physics/Collide/Shape/Convex/ConvexTransform/hkpConvexTransformShape.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"

static void finishLoadedObjecthkpConvexTransformShape(void* p, int32_t finishing);
// Havok SDK signature (hkpCollide.lib, tied by reloc,target).
static const void* getVtablehkpConvexTransformShape();
static void cleanupLoadedObjecthkpConvexTransformShape(void* p);

// The members.
using hkpConvexTransformShapeClass_MembersArray = const hkClassMember[1];
static hkpConvexTransformShapeClass_MembersArray hkpConvexTransformShapeClass_Members{
    {"transform", nullptr, nullptr, {hkClassMember::TYPE_TRANSFORM}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x20, nullptr}
};
hkClass hkpConvexTransformShapeClass(
    "hkpConvexTransformShape",
    &hkpConvexTransformShapeBaseClass,
    0x60,
    nullptr,
    0,
    nullptr,
    0,
    hkpConvexTransformShapeClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpConvexTransformShapeTypeInfo = {
    "hkpConvexTransformShape",
    finishLoadedObjecthkpConvexTransformShape,
    cleanupLoadedObjecthkpConvexTransformShape,
    getVtablehkpConvexTransformShape(),
    &typeid(hkpConvexTransformShape),
    sizeof(hkpConvexTransformShape)
};

static const void* getVtablehkpConvexTransformShape() {
    return hkGetVtableOf<hkpConvexTransformShape>();
}

static void cleanupLoadedObjecthkpConvexTransformShape(void* p) {
    static_cast<hkpConvexTransformShape*>(p)->~hkpConvexTransformShape();
}

static void finishLoadedObjecthkpConvexTransformShape(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkpConvexTransformShape(flag);
    }
}

// The class record and type info of hkpConvexTransformShape, as the SDK's generated file has them (the SDK has no
// .cpp of the class here: this file holds its reflection data).
