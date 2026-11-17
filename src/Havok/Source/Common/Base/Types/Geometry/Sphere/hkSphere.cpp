#include "Havok/Source/Common/Base/Types/Geometry/Sphere/hkSphere.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkSphereTypeInfo = {"hkSphere", nullptr,           cleanupLoadedObjecthkSphere,
                               nullptr,    &typeid(hkSphere), sizeof(hkSphere)};

using hkSphereClass_MembersArray = const hkClassMember[1];
static hkSphereClass_MembersArray hkSphereClass_Members{
    {"pos", nullptr, nullptr, hkClassMember::TYPE_VECTOR4, hkClassMember::TYPE_VOID, 0, 0, 0x0, nullptr}
};

hkClass
    hkSphereClass("hkSphere", nullptr, 0x10, nullptr, 0, nullptr, 0, hkSphereClass_Members, 1, nullptr, nullptr, 0, 0);
