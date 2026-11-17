#include "Havok/Source/Physics/Utilities/Serialize/hkpDisplayBindingData.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Animation/Animation/Ik/LookAt/hkaLookAtIkSolver.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Memory/Allocator/hkMemoryAllocator.h"
#include "Havok/Source/Common/Base/Memory/Router/hkMemoryRouter.h"
#include "Havok/Source/Common/Base/Object/hkBaseObject.h"
#include "Havok/Source/Common/Base/Object/hkReferencedObject.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Base/Types/hkRefPtr.h"
#include "Havok/Source/Common/SceneData/Mesh/hkxMesh.h"
#include "Havok/Source/Common/Serialize/Util/hkFinishLoadedObjectFlag.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCdBody.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Agent3/Machine/Nn/hkpAgentNnTrack.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Collection/hkpShapeCollection.h"
#include "Havok/Source/Physics/ConstraintSolver/Constraint/Bridge/hkpBridgeAtoms.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/PointToPath/hkpPointToPathConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Malleable/hkpMalleableConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Position/hkpPositionConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/SpringDamper/hkpSpringDamperConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Motor/Velocity/hkpVelocityConstraintMotor.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpSphereMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/ThinBoxMotion/hkpThinBoxMotion.h"
#include "Havok/Source/Physics/Dynamics/World/hkpPhysicsSystem.h"
#include "Havok/Source/Physics/Internal/Collide/ConvexPieceMesh/hkpConvexPieceStreamData.h"
#include "Havok/Source/Physics/Utilities/Actions/AngularDashpot/hkpAngularDashpotAction.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"
#include "Havok/Source/Physics/Utilities/Deprecated/DisableEntity/hkpDisableEntityCollisionFilter.h"
#include "SDK/MSVC/rtti.h"

// The reflected members of hkpDisplayBindingData::RigidBody, hkpDisplayBindingData::PhysicsSystem and
// hkpDisplayBindingData.
using hkpDisplayBindingDataRigidBodyClass_MembersArray = const hkClassMember[3];
static hkpDisplayBindingDataRigidBodyClass_MembersArray hkpDisplayBindingDataRigidBodyClass_Members{
	{"rigidBody", &hkpRigidBodyClass, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_STRUCT}, 0, 0, 0x8,
		nullptr},
	{"displayObject", &hkxMeshClass, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_STRUCT}, 0, 0, 0xC,
		nullptr},
	{"rigidBodyFromDisplayObjectTransform", nullptr, nullptr, {hkClassMember::TYPE_MATRIX4}, {hkClassMember::TYPE_VOID},
		0, 0, 0x10, nullptr}};
using hkpDisplayBindingDataPhysicsSystemClass_MembersArray = const hkClassMember[2];
static hkpDisplayBindingDataPhysicsSystemClass_MembersArray hkpDisplayBindingDataPhysicsSystemClass_Members{
	{"bindings", &hkpDisplayBindingDataRigidBodyClass, nullptr, {hkClassMember::TYPE_ARRAY},
		{hkClassMember::TYPE_POINTER}, 0, 0, 0x8, nullptr},
	{"system", &hkpPhysicsSystemClass, nullptr, {hkClassMember::TYPE_POINTER}, {hkClassMember::TYPE_STRUCT}, 0, 0, 0x14,
		nullptr}};
using hkpDisplayBindingDataClass_MembersArray = const hkClassMember[2];
static hkpDisplayBindingDataClass_MembersArray hkpDisplayBindingDataClass_Members{
	{"rigidBodyBindings", &hkpDisplayBindingDataRigidBodyClass, nullptr, {hkClassMember::TYPE_ARRAY},
		{hkClassMember::TYPE_POINTER}, 0, 0, 0x8, nullptr},
	{"physicsSystemBindings", &hkpDisplayBindingDataPhysicsSystemClass, nullptr, {hkClassMember::TYPE_ARRAY},
		{hkClassMember::TYPE_POINTER}, 0, 0, 0x14, nullptr}};

hkClass hkpDisplayBindingDataRigidBodyClass("hkpDisplayBindingDataRigidBody", &hkReferencedObjectClass, 80, nullptr, 0,
	nullptr, 0, hkpDisplayBindingDataRigidBodyClass_Members, 3, nullptr, nullptr, 0, 1);
hkTypeInfo hkpDisplayBindingDataRigidBodyTypeInfo = {"hkpDisplayBindingDataRigidBody",
	finishLoadedObjecthkpDisplayBindingDataRigidBody, cleanupLoadedObjecthkpDisplayBindingDataRigidBody,
	getVtablehkpDisplayBindingDataRigidBody(), &typeid(hkpDisplayBindingData::RigidBody),
	sizeof(hkpDisplayBindingData::RigidBody)};
hkClass hkpDisplayBindingDataPhysicsSystemClass("hkpDisplayBindingDataPhysicsSystem", &hkReferencedObjectClass, 24,
	nullptr, 0, nullptr, 0, hkpDisplayBindingDataPhysicsSystemClass_Members, 2, nullptr, nullptr, 0, 1);
hkTypeInfo hkpDisplayBindingDataPhysicsSystemTypeInfo = {"hkpDisplayBindingDataPhysicsSystem",
	finishLoadedObjecthkpDisplayBindingDataPhysicsSystem, cleanupLoadedObjecthkpDisplayBindingDataPhysicsSystem,
	getVtablehkpDisplayBindingDataPhysicsSystem(), &typeid(hkpDisplayBindingData::PhysicsSystem),
	sizeof(hkpDisplayBindingData::PhysicsSystem)};
hkClass hkpDisplayBindingDataClass("hkpDisplayBindingData", &hkReferencedObjectClass, 32, nullptr, 0, nullptr, 0,
	hkpDisplayBindingDataClass_Members, 2, nullptr, nullptr, 0, 1);
hkTypeInfo hkpDisplayBindingDataTypeInfo = {"hkpDisplayBindingData", finishLoadedObjecthkpDisplayBindingData,
	cleanupLoadedObjecthkpDisplayBindingData, getVtablehkpDisplayBindingData(), &typeid(hkpDisplayBindingData),
	sizeof(hkpDisplayBindingData)};

void cleanupLoadedObjecthkpDisplayBindingDataPhysicsSystem(void* p) {
	static_cast<hkpDisplayBindingData::PhysicsSystem*>(p)->~PhysicsSystem();
}

void cleanupLoadedObjecthkpDisplayBindingData(void* p) {
	static_cast<hkpDisplayBindingData*>(p)->~hkpDisplayBindingData();
}

void cleanupLoadedObjecthkpDisplayBindingDataRigidBody(void* p) {
	static_cast<hkpDisplayBindingData::RigidBody*>(p)->~RigidBody();
}

// Out of line: the members' hkRefPtr destructors need hkpRigidBody, hkxMesh and hkpPhysicsSystem complete.
hkpDisplayBindingData::RigidBody::RigidBody(hkFinishLoadedObjectFlag aFlag)
	: hkReferencedObject(aFlag), m_rigidBody(aFlag), m_displayObject(aFlag) {}

// The display object's and rigid body's references go with their members.
hkpDisplayBindingData::RigidBody::~RigidBody() {}

const hkClass& hkpDisplayBindingData::RigidBody::staticClass() {
	return hkpDisplayBindingDataRigidBodyClass;
}

hkpDisplayBindingData::PhysicsSystem::PhysicsSystem(hkFinishLoadedObjectFlag aFlag)
	: hkReferencedObject(aFlag), m_bindings(aFlag), m_system(aFlag) {}

// The system's reference and the bindings array go with their members.
hkpDisplayBindingData::PhysicsSystem::~PhysicsSystem() {}

const hkClass& hkpDisplayBindingData::PhysicsSystem::staticClass() {
	return hkpDisplayBindingDataPhysicsSystemClass;
}

// Both bindings arrays go with their members.
hkpDisplayBindingData::~hkpDisplayBindingData() {}

const hkClass& hkpDisplayBindingData::staticClass() {
	return hkpDisplayBindingDataClass;
}
