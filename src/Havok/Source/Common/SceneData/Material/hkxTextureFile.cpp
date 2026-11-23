#include "Havok/Source/Common/SceneData/Material/hkxTextureFile.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/String/hkStringPtr.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/Memory/hkMemoryMeshShape.h"

static void cleanupLoadedObjecthkxTextureFile(void* p);

// The reflected members of hkxTextureFile.
using hkxTextureFileClass_MembersArray = const hkClassMember[3];
static hkxTextureFileClass_MembersArray hkxTextureFileClass_Members{
    {"filename", nullptr, nullptr, hkClassMember::TYPE_STRINGPTR, hkClassMember::TYPE_VOID, 0, 0, 0x8, nullptr},
    {"name", nullptr, nullptr, hkClassMember::TYPE_STRINGPTR, hkClassMember::TYPE_VOID, 0, 0, 0xC, nullptr},
    {"originalFilename", nullptr, nullptr, hkClassMember::TYPE_STRINGPTR, hkClassMember::TYPE_VOID, 0, 0, 0x10, nullptr}
};
hkClass hkxTextureFileClass(
    "hkxTextureFile",
    &hkReferencedObjectClass,
    0x14,
    nullptr,
    0,
    nullptr,
    0,
    hkxTextureFileClass_Members,
    3,
    nullptr,
    nullptr,
    0,
    1
);
hkTypeInfo hkxTextureFileTypeInfo = {
    "hkxTextureFile",
    finishLoadedObjecthkxTextureFile,
    cleanupLoadedObjecthkxTextureFile,
    getVtablehkxTextureFile(),
    &typeid(hkxTextureFile),
    sizeof(hkxTextureFile)
};

static void cleanupLoadedObjecthkxTextureFile(void* p) {
    static_cast<hkxTextureFile*>(p)->~hkxTextureFile();
}

// The members' destructors release the three strings.
hkxTextureFile::~hkxTextureFile() {}

const hkClass& hkxTextureFile::staticClass() {
    return hkxTextureFileClass;
}
