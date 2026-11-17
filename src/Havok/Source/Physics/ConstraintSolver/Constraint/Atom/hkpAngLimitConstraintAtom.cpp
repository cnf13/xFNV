#include "Havok/Source/Physics/ConstraintSolver/Constraint/Atom/hkpAngLimitConstraintAtom.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Internal/GeometryProcessing/hkGeometryProcessing.h"

namespace {
    // Per member its offset into this struct (-1: no default), then the defaults (angularLimitsTauFactor 1.0).
    struct hkpAngLimitConstraintAtom_DefaultStruct {
        int32_t s_defaultOffsets[5];
        float m_angularLimitsTauFactor;
    };
} // namespace

// The reflected types' hkTypeInfo records: constant .data (no initialiser): the type name, the finish and cleanup
// functions, no vtable, the RTTI type descriptor and the size.
hkTypeInfo hkpAngLimitConstraintAtomTypeInfo = {
    "hkpAngLimitConstraintAtom",
    finishLoadedObjecthkpAngLimitConstraintAtom,
    cleanupLoadedObjecthkpAngLimitConstraintAtom,
    nullptr,
    &typeid(hkpAngLimitConstraintAtom),
    sizeof(hkpAngLimitConstraintAtom)
};

// The reflected members of hkpAngLimitConstraintAtom.
using hkpAngLimitConstraintAtomClass_MembersArray = const hkClassMember[5];
static hkpAngLimitConstraintAtomClass_MembersArray hkpAngLimitConstraintAtomClass_Members{
    {"isEnabled", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x2, nullptr},
    {"limitAxis", nullptr, nullptr, hkClassMember::TYPE_UINT8, hkClassMember::TYPE_VOID, 0, 0, 0x3, nullptr},
    {"minAngle", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x4, nullptr},
    {"maxAngle", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x8, nullptr},
    {"angularLimitsTauFactor", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0xC, nullptr}
};

// The defaults the class record names.
static const hkpAngLimitConstraintAtom_DefaultStruct hkpAngLimitConstraintAtom_Default{{-1, -1, -1, -1, 0x14}, 1.0f};
hkClass hkpAngLimitConstraintAtomClass(
    "hkpAngLimitConstraintAtom",
    &hkpConstraintAtomClass,
    16,
    nullptr,
    0,
    nullptr,
    0,
    hkpAngLimitConstraintAtomClass_Members,
    5,
    &hkpAngLimitConstraintAtom_Default,
    nullptr,
    0,
    0
);

const hkClass& hkpAngLimitConstraintAtom::staticClass() {
    return hkpAngLimitConstraintAtomClass;
}
