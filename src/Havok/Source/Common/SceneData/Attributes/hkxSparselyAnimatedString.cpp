#include "Havok/Source/Common/SceneData/Attributes/hkxSparselyAnimatedString.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/Memory/hkMemoryMeshShape.h"

static void cleanupLoadedObjecthkxSparselyAnimatedString(void* p);

// The reflected members of hkxSparselyAnimatedString.
using hkxSparselyAnimatedStringClass_MembersArray = const hkClassMember[2];
static hkxSparselyAnimatedStringClass_MembersArray hkxSparselyAnimatedStringClass_Members{
    {"strings", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_STRINGPTR}, 0, {0}, 0x8, nullptr},
    {"times", nullptr, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_REAL}, 0, {0}, 0x14, nullptr}
};
hkClass hkxSparselyAnimatedStringClass(
    "hkxSparselyAnimatedString",
    &hkReferencedObjectClass,
    0x20,
    nullptr,
    0,
    nullptr,
    0,
    hkxSparselyAnimatedStringClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    2
);
hkTypeInfo hkxSparselyAnimatedStringTypeInfo = {
    "hkxSparselyAnimatedString",
    finishLoadedObjecthkxSparselyAnimatedString,
    cleanupLoadedObjecthkxSparselyAnimatedString,
    getVtablehkxSparselyAnimatedString(),
    &typeid(hkxSparselyAnimatedString),
    sizeof(hkxSparselyAnimatedString)
};

static void cleanupLoadedObjecthkxSparselyAnimatedString(void* p) {
    static_cast<hkxSparselyAnimatedString*>(p)->~hkxSparselyAnimatedString();
}

// The members' destructors destroy the strings and free both arrays.
hkxSparselyAnimatedString::~hkxSparselyAnimatedString() {}

const hkClass& hkxSparselyAnimatedString::staticClass() {
    return hkxSparselyAnimatedStringClass;
}
