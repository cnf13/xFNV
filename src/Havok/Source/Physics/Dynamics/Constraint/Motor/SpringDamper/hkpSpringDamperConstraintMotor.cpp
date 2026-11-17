#include "Havok/Source/Physics/Dynamics/Constraint/Motor/SpringDamper/hkpSpringDamperConstraintMotor.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnTrack.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpPointToPathConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Malleable/hkpMalleableConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Position/hkpPositionConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Velocity/hkpVelocityConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpLimitedForceConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/ThinBoxMotion/hkpThinBoxMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpSphereMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Internal/Collide/ConvexPieceMesh/hkpConvexPieceStreamData.h"
#include "Havok/Source/Physics/Utilities/Actions/AngularDashpot/hkpAngularDashpotAction.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"
#include "Havok/Source/Physics/Utilities/Deprecated/DisableEntity/hkpDisableEntityCollisionFilter.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpDisplayBindingData.h"
#include "SDK/MSVC/rtti.h"

// The reflected members of hkpSpringDamperConstraintMotor.
using hkpSpringDamperConstraintMotorClass_MembersArray = const hkClassMember[2];
static hkpSpringDamperConstraintMotorClass_MembersArray hkpSpringDamperConstraintMotorClass_Members{
    {"springConstant", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x14, nullptr},
    {"springDamping", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x18, nullptr}
};

hkClass hkpSpringDamperConstraintMotorClass(
    "hkpSpringDamperConstraintMotor",
    &hkpLimitedForceConstraintMotorClass,
    28,
    nullptr,
    0,
    nullptr,
    0,
    hkpSpringDamperConstraintMotorClass_Members,
    2,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpSpringDamperConstraintMotorTypeInfo = {
    "hkpSpringDamperConstraintMotor",
    finishLoadedObjecthkpSpringDamperConstraintMotor,
    cleanupLoadedObjecthkpSpringDamperConstraintMotor,
    getVtablehkpSpringDamperConstraintMotor(),
    &typeid(hkpSpringDamperConstraintMotor),
    sizeof(hkpSpringDamperConstraintMotor)
};

void cleanupLoadedObjecthkpSpringDamperConstraintMotor(void* p) {
    static_cast<hkpSpringDamperConstraintMotor*>(p)->~hkpSpringDamperConstraintMotor();
}

// The type info's finish-loaded-object callback for hkpSpringDamperConstraintMotor: its finish constructor.
void finishLoadedObjecthkpSpringDamperConstraintMotor(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkpSpringDamperConstraintMotor(flag);
    }
}

const void* getVtablehkpSpringDamperConstraintMotor() {
    return hkGetVtableOf<hkpSpringDamperConstraintMotor>();
}

hkpSpringDamperConstraintMotor::hkpSpringDamperConstraintMotor() : m_springConstant(0.0f), m_springDamping(0.0f) {
    m_type.m_storage = 3;
}

hkpSpringDamperConstraintMotor::hkpSpringDamperConstraintMotor(
    float springConstant,
    float springDamping,
    float maxForce
)
  : m_springConstant(springConstant), m_springDamping(springDamping) {
    m_minForce = -maxForce;
    m_maxForce = maxForce;
    m_type.m_storage = 3;
}

hkpSpringDamperConstraintMotor::~hkpSpringDamperConstraintMotor() {}

// new hkpSpringDamperConstraintMotor(*this): hkReferencedObject's operator new (the block and its size), then the copy
// constructor.
hkpConstraintMotor* hkpSpringDamperConstraintMotor::clone() const {
    hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
    hkpSpringDamperConstraintMotor* result = static_cast<hkpSpringDamperConstraintMotor*>(
        Instance->m_heap->blockAlloc(sizeof(hkpSpringDamperConstraintMotor))
    );
    result->m_memSizeAndFlags = sizeof(hkpSpringDamperConstraintMotor);
    return ::new (result) hkpSpringDamperConstraintMotor(*this);
}

const hkClass& hkpSpringDamperConstraintMotor::staticClass() {
    return hkpSpringDamperConstraintMotorClass;
}
