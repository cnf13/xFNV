#include "Havok/Source/Physics/Internal/Collide/ConvexPieceMesh/hkpConvexPieceStreamData.h"

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
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/ThinBoxMotion/hkpThinBoxMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/Rigid/hkpSphereMotion.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Utilities/Actions/AngularDashpot/hkpAngularDashpotAction.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"
#include "Havok/Source/Physics/Utilities/Deprecated/DisableEntity/hkpDisableEntityCollisionFilter.h"
#include "Havok/Source/Physics/Utilities/Serialize/hkpDisplayBindingData.h"
#include "SDK/MSVC/rtti.h"

static void cleanupLoadedObjecthkpConvexPieceStreamData(void* p);

// The reflected members of hkpConvexPieceStreamData.
using hkpConvexPieceStreamDataClass_MembersArray = const hkClassMember[3];
static hkpConvexPieceStreamDataClass_MembersArray hkpConvexPieceStreamDataClass_Members{
    {"convexPieceStream", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT32, 0, 0, 0x8, nullptr},
    {"convexPieceOffsets", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT32, 0, 0, 0x14,
     nullptr},
    {"convexPieceSingleTriangles", nullptr, nullptr, hkClassMember::TYPE_ARRAY, hkClassMember::TYPE_UINT32, 0, 0, 0x20,
     nullptr}
};
hkClass hkpConvexPieceStreamDataClass(
    "hkpConvexPieceStreamData",
    &hkReferencedObjectClass,
    44,
    nullptr,
    0,
    nullptr,
    0,
    hkpConvexPieceStreamDataClass_Members,
    3,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpConvexPieceStreamDataTypeInfo = {
    "hkpConvexPieceStreamData",
    finishLoadedObjecthkpConvexPieceStreamData,
    cleanupLoadedObjecthkpConvexPieceStreamData,
    getVtablehkpConvexPieceStreamData(),
    &typeid(hkpConvexPieceStreamData),
    sizeof(hkpConvexPieceStreamData)
};

static void cleanupLoadedObjecthkpConvexPieceStreamData(void* p) {
    static_cast<hkpConvexPieceStreamData*>(p)->~hkpConvexPieceStreamData();
}

// The type info's finish-loaded-object callback for hkpConvexPieceStreamData: runs its finish constructor.
void finishLoadedObjecthkpConvexPieceStreamData(void* p, int32_t finishing) {
    if (p) {
        hkFinishLoadedObjectFlag flag;
        flag.m_finishing = finishing;
        ::new (p) hkpConvexPieceStreamData(flag);
    }
}

const void* getVtablehkpConvexPieceStreamData() {
    return hkGetVtableOf<hkpConvexPieceStreamData>();
}

// The three arrays free themselves.
hkpConvexPieceStreamData::~hkpConvexPieceStreamData() {}

const hkClass& hkpConvexPieceStreamData::staticClass() {
    return hkpConvexPieceStreamDataClass;
}
