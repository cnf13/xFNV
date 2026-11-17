#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpConeLimitConstraintAtom.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassEnum.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"

namespace {
// Per member its offset into this struct (-1: no default), then the defaults (memOffsetToAngleOffset 1,
// angularLimitsTauFactor 1.0).
struct hkpConeLimitConstraintAtom_DefaultStruct {
	int32_t s_defaultOffsets[8];
	uint8_t m_memOffsetToAngleOffset;
	float m_angularLimitsTauFactor;
};
} // namespace

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpConeLimitConstraintAtomTypeInfo = {"hkpConeLimitConstraintAtom",
	finishLoadedObjecthkpConeLimitConstraintAtom, cleanupLoadedObjecthkpConeLimitConstraintAtom, nullptr,
	&typeid(hkpConeLimitConstraintAtom), sizeof(hkpConeLimitConstraintAtom)};

// The reflected enum of hkpConeLimitConstraintAtom.
using hkpConeLimitConstraintAtomMeasurementModeEnumItemsArray = const hkClassEnum::Item[2];
static hkpConeLimitConstraintAtomMeasurementModeEnumItemsArray hkpConeLimitConstraintAtomMeasurementModeEnumItems{
	{hkpConeLimitConstraintAtom::ZERO_WHEN_VECTORS_ALIGNED, "ZERO_WHEN_VECTORS_ALIGNED"},
	{hkpConeLimitConstraintAtom::ZERO_WHEN_VECTORS_PERPENDICULAR, "ZERO_WHEN_VECTORS_PERPENDICULAR"}};
using hkpConeLimitConstraintAtomEnumsArray = const hkClassEnum[1];
static hkpConeLimitConstraintAtomEnumsArray hkpConeLimitConstraintAtomEnums{
	{"MeasurementMode", hkpConeLimitConstraintAtomMeasurementModeEnumItems, 2, nullptr, {0}}};

// The defaults the class record names.
static const hkpConeLimitConstraintAtom_DefaultStruct hkpConeLimitConstraintAtom_Default{
	{-1, -1, -1, -1, 0x20, -1, -1, 0x24}, 1, 1.0f};

static const hkClassEnum* hkpConeLimitConstraintAtomMeasurementModeEnum = &hkpConeLimitConstraintAtomEnums[0];

// The reflected members of hkpConeLimitConstraintAtom. The angleMeasurementMode member's enum is
// hkpConeLimitConstraintAtomMeasurementModeEnum, stored by the array's dynamic initializer.
using hkpConeLimitConstraintAtomClass_MembersArray = hkClassMember[8];
static hkpConeLimitConstraintAtomClass_MembersArray hkpConeLimitConstraintAtomClass_Members{
	{"isEnabled", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x2, nullptr},
	{"twistAxisInA", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x3, nullptr},
	{"refAxisInB", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x4, nullptr},
	{"angleMeasurementMode", nullptr, hkpConeLimitConstraintAtomMeasurementModeEnum, hkClassMember::TYPE_ENUM,
		hkClassMember::TYPE_UINT8, 0, 0, 0x5, nullptr},
	{"memOffsetToAngleOffset", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x6,
		nullptr},
	{"minAngle", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x8, nullptr},
	{"maxAngle", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0xC, nullptr},
	{"angularLimitsTauFactor", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x10,
		nullptr}};
hkClass hkpConeLimitConstraintAtomClass("hkpConeLimitConstraintAtom", &hkpConstraintAtomClass, 20, nullptr, 0,
	hkpConeLimitConstraintAtomEnums, 1, hkpConeLimitConstraintAtomClass_Members, 8, &hkpConeLimitConstraintAtom_Default,
	nullptr, 0, 0);

const hkClass& hkpConeLimitConstraintAtom::staticClass() {
	return hkpConeLimitConstraintAtomClass;
}
