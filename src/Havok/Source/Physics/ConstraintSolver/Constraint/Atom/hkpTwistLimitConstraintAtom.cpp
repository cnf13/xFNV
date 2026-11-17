#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpTwistLimitConstraintAtom.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"

namespace {
// The SDK's default-value block of the class record: per member its offset into this struct (-1: no default), then
// the defaults (angularLimitsTauFactor 1.0).
struct hkpTwistLimitConstraintAtom_DefaultStruct {
	int32_t s_defaultOffsets[6];
	float m_angularLimitsTauFactor;
};
} // namespace

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpTwistLimitConstraintAtomTypeInfo = {"hkpTwistLimitConstraintAtom",
	finishLoadedObjecthkpTwistLimitConstraintAtom, cleanupLoadedObjecthkpTwistLimitConstraintAtom, nullptr,
	&typeid(hkpTwistLimitConstraintAtom), sizeof(hkpTwistLimitConstraintAtom)};

// Not named in the PDB or IDA; name chosen here (the SDK's class-file name). The reflected members of
// hkpTwistLimitConstraintAtom, as the exe's .rdata holds them.
using hkpTwistLimitConstraintAtomClass_MembersArray = const hkClassMember[6];
static hkpTwistLimitConstraintAtomClass_MembersArray hkpTwistLimitConstraintAtomClass_Members{
	{"isEnabled", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x2, nullptr},
	{"twistAxis", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x3, nullptr},
	{"refAxis", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x4, nullptr},
	{"minAngle", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x8, nullptr},
	{"maxAngle", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0xC, nullptr},
	{"angularLimitsTauFactor", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x10,
		nullptr}};

// The defaults the class record names (.rdata). Not named in the PDB or IDA; name chosen here (the Havok SDK's).
static const hkpTwistLimitConstraintAtom_DefaultStruct hkpTwistLimitConstraintAtom_Default{{-1, -1, -1, -1, -1, 0x18},
	1.0f};
hkClass hkpTwistLimitConstraintAtomClass("hkpTwistLimitConstraintAtom", &hkpConstraintAtomClass, 20, nullptr, 0,
	nullptr, 0, hkpTwistLimitConstraintAtomClass_Members, 6, &hkpTwistLimitConstraintAtom_Default, nullptr, 0, 0);

const hkClass& hkpTwistLimitConstraintAtom::staticClass() {
	return hkpTwistLimitConstraintAtomClass;
}
