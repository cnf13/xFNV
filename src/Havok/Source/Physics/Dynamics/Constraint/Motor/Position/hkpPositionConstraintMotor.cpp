#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Position/hkpPositionConstraintMotor.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnTrack.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpPointToPathConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Malleable/hkpMalleableConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpLimitedForceConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/SpringDamper/hkpSpringDamperConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Velocity/hkpVelocityConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpSphereMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/ThinBoxMotion/hkpThinBoxMotion.h"
#include "Havok/Source/Physics/Internal/Collide/ConvexPieceMesh/hkpConvexPieceStreamData.h"
#include "Havok/Source/Physics/Utilities/Actions/AngularDashpot/hkpAngularDashpotAction.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"
#include "Havok/Source/Physics/Utilities/Deprecated/DisableEntity/hkpDisableEntityCollisionFilter.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpDisplayBindingData.h"
#include "SDK/MSVC/rtti.h"

// The reflected members of hkpPositionConstraintMotor.
using hkpPositionConstraintMotorClass_MembersArray = const hkClassMember[4];
static hkpPositionConstraintMotorClass_MembersArray hkpPositionConstraintMotorClass_Members{
	{"tau", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x14, nullptr},
	{"damping", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x18, nullptr},
	{"proportionalRecoveryVelocity", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0},
		0x1C, nullptr},
	{"constantRecoveryVelocity", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x20,
		nullptr}};

hkClass hkpPositionConstraintMotorClass("hkpPositionConstraintMotor", &hkpLimitedForceConstraintMotorClass, 36, nullptr,
	0, nullptr, 0, hkpPositionConstraintMotorClass_Members, 4, nullptr, nullptr, 0, 0);
hkTypeInfo hkpPositionConstraintMotorTypeInfo = {"hkpPositionConstraintMotor",
	finishLoadedObjecthkpPositionConstraintMotor, cleanupLoadedObjecthkpPositionConstraintMotor,
	getVtablehkpPositionConstraintMotor(), &typeid(hkpPositionConstraintMotor), sizeof(hkpPositionConstraintMotor)};

void cleanupLoadedObjecthkpPositionConstraintMotor(void* p) {
	static_cast<hkpPositionConstraintMotor*>(p)->~hkpPositionConstraintMotor();
}

// The type info's finish-loaded-object callback for hkpPositionConstraintMotor: its finish constructor.
void finishLoadedObjecthkpPositionConstraintMotor(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag flag;
		flag.m_finishing = finishing;
		::new (p) hkpPositionConstraintMotor(flag);
	}
}

const void* getVtablehkpPositionConstraintMotor() {
	return hkGetVtableOf<hkpPositionConstraintMotor>();
}

hkpPositionConstraintMotor::hkpPositionConstraintMotor(float currentPosition)
	: m_tau(0.800000011920929f), m_damping(1.0f), m_proportionalRecoveryVelocity(2.0f),
	  m_constantRecoveryVelocity(1.0f) {
	m_type.m_storage = 1;
}

hkpPositionConstraintMotor::~hkpPositionConstraintMotor() {}

// Equivalent to new hkpPositionConstraintMotor(*this): hkReferencedObject's operator new (the block and its size), then
// the copy constructor.
hkpConstraintMotor* hkpPositionConstraintMotor::clone() const {
	hkMemoryRouter* Instance = &hkMemoryRouter::getInstance();
	hkpPositionConstraintMotor* result =
		static_cast<hkpPositionConstraintMotor*>(Instance->m_heap->blockAlloc(sizeof(hkpPositionConstraintMotor)));
	result->m_memSizeAndFlags = sizeof(hkpPositionConstraintMotor);
	return ::new (result) hkpPositionConstraintMotor(*this);
}

const hkClass& hkpPositionConstraintMotor::staticClass() {
	return hkpPositionConstraintMotorClass;
}

hkReferencedObject::hkReferencedObject() : m_referenceCount(1) {}

hkReferencedObject::~hkReferencedObject() {}

hkpConstraintMotor::~hkpConstraintMotor() {}
