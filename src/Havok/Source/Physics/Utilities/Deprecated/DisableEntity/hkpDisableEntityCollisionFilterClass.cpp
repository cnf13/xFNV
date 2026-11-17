#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnTrack.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Position/hkpPositionConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/SpringDamper/hkpSpringDamperConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Velocity/hkpVelocityConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntityClass.h"
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
#include "Havok/Source/Physics/Collide/Filter/hkpCollisionFilter.h"
#include "SDK/MSVC/rtti.h"

static void cleanupLoadedObjecthkpDisableEntityCollisionFilter(void* p);

// The member table of hkpDisableEntityCollisionFilter's class record, as the Havok SDK's class file has it. Not
// named in the PDB or IDA; name chosen here.
using hkpDisableEntityCollisionFilterClass_MembersArray = const hkClassMember[1];
static hkpDisableEntityCollisionFilterClass_MembersArray hkpDisableEntityCollisionFilterClass_Members{
	{"disabledEntities", &hkpEntityClass, nullptr, {hkClassMember::TYPE_ARRAY}, {hkClassMember::TYPE_POINTER}, 0, {0},
		0x34, nullptr}};
hkClass hkpDisableEntityCollisionFilterClass("hkpDisableEntityCollisionFilter", &hkpCollisionFilterClass, 64, nullptr,
	1, nullptr, 0, hkpDisableEntityCollisionFilterClass_Members, 1, nullptr, nullptr, 0, 0);
hkTypeInfo hkpDisableEntityCollisionFilterTypeInfo = {"hkpDisableEntityCollisionFilter",
	finishLoadedObjecthkpDisableEntityCollisionFilter, cleanupLoadedObjecthkpDisableEntityCollisionFilter,
	getVtablehkpDisableEntityCollisionFilter(), &typeid(hkpDisableEntityCollisionFilter),
	sizeof(hkpDisableEntityCollisionFilter)};

static void cleanupLoadedObjecthkpDisableEntityCollisionFilter(void* p) {
	static_cast<hkpDisableEntityCollisionFilter*>(p)->~hkpDisableEntityCollisionFilter();
}

// The type info's vtable getter for hkpDisableEntityCollisionFilter: the exe runs the out-of-line finish constructor,
// flag clear, on a 16-aligned local it never destroys, and returns the vptr the constructor stored: the class's vtable.
const void* getVtablehkpDisableEntityCollisionFilter() {
	return hkGetVtableOf<hkpDisableEntityCollisionFilter>();
}
