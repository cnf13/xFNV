#include "Havok/Source/Common/SceneData/Material/hkxMaterialShaderSet.h"

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
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/GeometryUtilities/Mesh/Memory/hkMemoryMeshShape.h"
#include "Havok/Source/Common/SceneData/Environment/hkxEnvironment.h"
#include "Havok/Source/Common/SceneData/Material/hkxMaterialShader.h"

// The reflected members of hkxMaterialShaderSet.
using hkxMaterialShaderSetClass_MembersArray = const hkClassMember[1];
static hkxMaterialShaderSetClass_MembersArray hkxMaterialShaderSetClass_Members{
    {"shaders", &hkxMaterialShaderClass, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_POINTER, 0, 0, 0x8,
     nullptr}
};

hkClass hkxMaterialShaderSetClass(
    "hkxMaterialShaderSet",
    &hkReferencedObjectClass,
    0x14,
    nullptr,
    0,
    nullptr,
    0,
    hkxMaterialShaderSetClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    1
);
hkTypeInfo hkxMaterialShaderSetTypeInfo = {
    "hkxMaterialShaderSet",          finishLoadedObjecthkxMaterialShaderSet, cleanupLoadedObjecthkxMaterialShaderSet,
    getVtablehkxMaterialShaderSet(), &typeid(hkxMaterialShaderSet),          sizeof(hkxMaterialShaderSet)
};

// The member's constructor leaves the shader array empty.
hkxMaterialShaderSet::hkxMaterialShaderSet() {}

// The member's destructor releases the shaders and frees the array.
hkxMaterialShaderSet::~hkxMaterialShaderSet() {}
