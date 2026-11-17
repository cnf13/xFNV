#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexTransformShapeBase.h"

#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/Capsule/hkpCapsuleShape.h"
#include "Havok/Source/Physics/Collide/Shape/Convex/hkpConvexShape.h"
#include "Havok/Source/Physics/Collide/Shape/Misc/Bv/hkpBvShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShape.h"
#include "Havok/Source/Physics/Collide/Shape/hkpShapeType.h"
#include "Havok/Source/Physics/Utilities/Dynamics/Inertia/hkpInertiaTensorComputer.h"

// hkpConvexTransformShapeBase's reflected members.
using hkpConvexTransformShapeBaseClass_MembersArray = const hkClassMember[2];
static hkpConvexTransformShapeBaseClass_MembersArray hkpConvexTransformShapeBaseClass_Members{
    {"childShape",
     &hkpSingleShapeContainerClass,
     nullptr,
     {hkClassMember::TYPE_STRUCT},
     {hkClassMember::TYPE_VOID},
     0,
     {0},
     0x14,
     nullptr},
    {"childShapeSize",
     nullptr,
     nullptr,
     {hkClassMember::TYPE_INT32},
     {hkClassMember::TYPE_VOID},
     0,
     {hkClassMember::SERIALIZE_IGNORED},
     0x1C,
     nullptr}
};

hkClass hkpConvexTransformShapeBaseClass(
    "hkpConvexTransformShapeBase",
    &hkpConvexShapeClass,
    0x20,
    nullptr,
    0,
    nullptr,
    0,
    hkpConvexTransformShapeBaseClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);

hkpConvexTransformShapeBase::hkpConvexTransformShapeBase(int32_t a1, float a2, hkpConvexShape* a3, int32_t a4)
  : hkpConvexShape(static_cast<hkpShapeType>(a1), a2),
    m_childShape(a3, static_cast<hkpShapeContainer::ReferencePolicy>(a4)) {}

hkpConvexTransformShapeBase::~hkpConvexTransformShapeBase() {}
