#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpBallSocketConstraintAtom.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"

namespace {
// Per member its offset into this struct (-1: no default), then the defaults.
struct hkpBallSocketConstraintAtom_DefaultStruct {
	int32_t s_defaultOffsets[3];
	hkUFloat8 m_stabilizationFactor;
	float m_maxImpulse;
};
} // namespace

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpBallSocketConstraintAtomTypeInfo = {"hkpBallSocketConstraintAtom",
	finishLoadedObjecthkpBallSocketConstraintAtom, cleanupLoadedObjecthkpBallSocketConstraintAtom, nullptr,
	&typeid(hkpBallSocketConstraintAtom), sizeof(hkpBallSocketConstraintAtom)};

// The reflected members of hkpBallSocketConstraintAtom.
using hkpBallSocketConstraintAtomClass_MembersArray = const hkClassMember[3];
static hkpBallSocketConstraintAtomClass_MembersArray hkpBallSocketConstraintAtomClass_Members{
	{"bodiesToNotify", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x2, nullptr},
	{"stabilizationFactor", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x3, nullptr},
	{"maxImpulse", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x4, nullptr}};

static hkpBallSocketConstraintAtom_DefaultStruct hkpBallSocketConstraintAtom_Default{{-1, 0xC, 0x10}};
hkClass hkpBallSocketConstraintAtomClass("hkpBallSocketConstraintAtom", &hkpConstraintAtomClass, 8, nullptr, 0, nullptr,
	0, hkpBallSocketConstraintAtomClass_Members, 3, &hkpBallSocketConstraintAtom_Default, nullptr, 0, 1);

const hkClass& hkpBallSocketConstraintAtom::staticClass() {
	return hkpBallSocketConstraintAtomClass;
}
