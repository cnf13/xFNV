#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/SceneData/Attributes/hkxAttributeGroupClass.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpCdBodyTypeInfo = {"hkpCdBody",        nullptr,          cleanupLoadedObjecthkpCdBody, nullptr,
                                &typeid(hkpCdBody), sizeof(hkpCdBody)};

hkpCdBody::MembersArray hkpCdBody::Members{
    {"shape",
     &hkpShapeClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {0},
     offsetof(hkpCdBody, m_shape),
     nullptr},
    {"shapeKey",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_UINT32},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     offsetof(hkpCdBody, m_shapeKey),
     nullptr},
    {"motion",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     offsetof(hkpCdBody, m_motion),
     nullptr},
    {"parent",
     &hkpCdBodyClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     offsetof(hkpCdBody, m_parent),
     nullptr}
};
hkClass hkpCdBodyClass(
    "hkpCdBody",
    nullptr,
    sizeof(hkpCdBody),
    nullptr,
    0,
    nullptr,
    0,
    hkpCdBody::Members,
    4,
    nullptr,
    nullptr,
    0,
    1
);

const hkClass& hkpCdBody::staticClass() {
    return hkpCdBodyClass;
}
