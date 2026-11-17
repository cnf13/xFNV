#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Velocity/hkpVelocityConstraintMotor.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnTrack.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Position/hkpPositionConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/SpringDamper/hkpSpringDamperConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/hkpConstraintMotor.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Internal/Collide/ConvexPieceMesh/hkpConvexPieceStreamData.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/ThinBoxMotion/hkpThinBoxMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpSphereMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Utilities/Actions/AngularDashpot/hkpAngularDashpotAction.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpPointToPathConstraintData.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpDisplayBindingData.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Malleable/hkpMalleableConstraintData.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/Utilities/Deprecated/DisableEntity/hkpDisableEntityCollisionFilter.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "SDK/MSVC/rtti.h"

static void cleanupLoadedObjecthkpVelocityConstraintMotor(void* p);

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of
// hkpVelocityConstraintMotor, as the exe's .rdata holds them.
using hkpVelocityConstraintMotorClass_MembersArray = const hkClassMember[3];
static hkpVelocityConstraintMotorClass_MembersArray hkpVelocityConstraintMotorClass_Members{
	{"tau", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x14, nullptr},
	{"velocityTarget", nullptr, nullptr, {hkClassMember::TYPE_REAL}, {hkClassMember::TYPE_VOID}, 0, {0}, 0x18, nullptr},
	{"useVelocityTargetFromConstraintTargets", nullptr, nullptr, {hkClassMember::TYPE_BOOL}, {hkClassMember::TYPE_VOID},
		0, {0}, 0x1C, nullptr}};
hkClass hkpVelocityConstraintMotorClass("hkpVelocityConstraintMotor", &hkpLimitedForceConstraintMotorClass, 32, nullptr,
	0, nullptr, 0, hkpVelocityConstraintMotorClass_Members, 3, nullptr, nullptr, 0, 0);
hkTypeInfo hkpVelocityConstraintMotorTypeInfo = {"hkpVelocityConstraintMotor",
	finishLoadedObjecthkpVelocityConstraintMotor, cleanupLoadedObjecthkpVelocityConstraintMotor,
	getVtablehkpVelocityConstraintMotor(), &typeid(hkpVelocityConstraintMotor), sizeof(hkpVelocityConstraintMotor)};

static void cleanupLoadedObjecthkpVelocityConstraintMotor(void* p) {
	static_cast<hkpVelocityConstraintMotor*>(p)->~hkpVelocityConstraintMotor();
}

// The type info's finish-loaded-object callback for hkpVelocityConstraintMotor: its finish constructor, which the exe
// inlines.
void finishLoadedObjecthkpVelocityConstraintMotor(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag flag;
		flag.m_finishing = finishing;
		::new (p) hkpVelocityConstraintMotor(flag);
	}
}

const void* getVtablehkpVelocityConstraintMotor() {
	return hkGetVtableOf<hkpVelocityConstraintMotor>();
}

// The SDK's `new hkpVelocityConstraintMotor(*this)`: hkReferencedObject's operator new (the block and its size), then
// the copy constructor, which the exe inlines.
hkpConstraintMotor* hkpVelocityConstraintMotor::clone() const {
	hkMemoryRouter* pRouter = &hkMemoryRouter::getInstance();
	hkpVelocityConstraintMotor* pResult =
		static_cast<hkpVelocityConstraintMotor*>(pRouter->m_heap->blockAlloc(sizeof(hkpVelocityConstraintMotor)));
	pResult->m_memSizeAndFlags = static_cast<uint16_t>(sizeof(hkpVelocityConstraintMotor));
	return ::new (pResult) hkpVelocityConstraintMotor(*this);
}

const hkClass& hkpVelocityConstraintMotor::staticClass() {
	return hkpVelocityConstraintMotorClass;
}
