#include "Havok/Source/Physics/Dynamics/Motion/Rigid/ThinBoxMotion/hkpThinBoxMotion.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnTrack.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/ExtendedMeshShape/hkpExtendedMeshShape.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpPointToPathConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Malleable/hkpMalleableConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Position/hkpPositionConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/SpringDamper/hkpSpringDamperConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Velocity/hkpVelocityConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpSphereMotion.h"
#include "Havok/Source/Physics/Internal/Collide/ConvexPieceMesh/hkpConvexPieceStreamData.h"
#include "Havok/Source/Physics/Utilities/Actions/AngularDashpot/hkpAngularDashpotAction.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"
#include "Havok/Source/Physics/Utilities/Deprecated/DisableEntity/hkpDisableEntityCollisionFilter.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpDisplayBindingData.h"
#include "SDK/MSVC/rtti.h"

#include <bit>

hkClass hkpThinBoxMotionClass("hkpThinBoxMotion", &hkpBoxMotionClass, 288, nullptr, 0, nullptr, 0, nullptr, 0, nullptr,
	nullptr, 0, 0);
hkTypeInfo hkpThinBoxMotionTypeInfo = {"hkpThinBoxMotion", finishLoadedObjecthkpThinBoxMotion,
	cleanupLoadedObjecthkpThinBoxMotion, getVtablehkpThinBoxMotion(), &typeid(hkpThinBoxMotion),
	sizeof(hkpThinBoxMotion)};

void cleanupLoadedObjecthkpThinBoxMotion(void* p) {
	static_cast<hkpThinBoxMotion*>(p)->~hkpThinBoxMotion();
}

// The type info's finish-loaded-object callback for hkpThinBoxMotion: runs its finish constructor.
void finishLoadedObjecthkpThinBoxMotion(void* p, int32_t finishing) {
	if (p) {
		hkFinishLoadedObjectFlag kFlag;
		kFlag.m_finishing = finishing;
		::new (p) hkpThinBoxMotion(kFlag);
	}
}

const void* getVtablehkpThinBoxMotion() {
	return hkGetVtableOf<hkpThinBoxMotion>();
}

hkpThinBoxMotion::hkpThinBoxMotion(hkFinishLoadedObjectFlag flag) : hkpBoxMotion(flag.m_finishing) {}

hkpThinBoxMotion::hkpThinBoxMotion(const hkVector4& position, const hkQuaternion& rotation)
	: hkpBoxMotion(position, rotation) {
	m_type.m_storage = MOTION_THIN_BOX_INERTIA;
}

const hkClass& hkpThinBoxMotion::staticClass() {
	return hkpThinBoxMotionClass;
}
