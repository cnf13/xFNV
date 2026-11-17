#include "Havok/Source/Physics/Dynamics/Constraint/Breakable/hkpBreakableConstraintDataClass.h"

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Breakable/hkpBreakableConstraintData.h"

// The reflected members of hkpBreakableConstraintData.
using hkpBreakableConstraintDataClass_MembersArray = const hkClassMember[7];
static hkpBreakableConstraintDataClass_MembersArray hkpBreakableConstraintDataClass_Members{
    {"atoms", &hkpBridgeAtomsClass, nullptr, hkClassMember::TYPE_STRUCT, hkClassMember::TYPE_VOID, 0, 0, 0xC, nullptr},
    {"constraintData", &hkpConstraintDataClass, nullptr, hkClassMember::TYPE_POINTER, hkClassMember::TYPE_STRUCT, 0, 0,
     0x18, nullptr},
    {"childRuntimeSize", nullptr, nullptr, hkClassMember::TYPE_UINT16, hkClassMember::TYPE_VOID, 0, 0, 0x1C, nullptr},
    {"childNumSolverResults", nullptr, nullptr, hkClassMember::TYPE_UINT16, hkClassMember::TYPE_VOID, 0, 0, 0x1E,
     nullptr},
    {"solverResultLimit", nullptr, nullptr, hkClassMember::TYPE_REAL, hkClassMember::TYPE_VOID, 0, 0, 0x20, nullptr},
    {"removeWhenBroken", nullptr, nullptr, hkClassMember::TYPE_BOOL, hkClassMember::TYPE_VOID, 0, 0, 0x24, nullptr},
    {"revertBackVelocityOnBreak", nullptr, nullptr, hkClassMember::TYPE_BOOL, hkClassMember::TYPE_VOID, 0, 0, 0x25,
     nullptr}
};

hkClass hkpBreakableConstraintDataClass(
    "hkpBreakableConstraintData",
    &hkpConstraintDataClass,
    40,
    nullptr,
    0,
    nullptr,
    0,
    hkpBreakableConstraintDataClass_Members,
    7,
    nullptr,
    nullptr,
    0,
    0
);

hkTypeInfo hkpBreakableConstraintDataTypeInfo = {
    "hkpBreakableConstraintData",
    finishLoadedObjecthkpBreakableConstraintData,
    cleanupLoadedObjecthkpBreakableConstraintData,
    getVtablehkpBreakableConstraintData(),
    &typeid(hkpBreakableConstraintData),
    sizeof(hkpBreakableConstraintData)
};

// The class registry's vtable getter: returns the class's vtable.
const void* getVtablehkpBreakableConstraintData() {
    return hkGetVtableOf<hkpBreakableConstraintData>();
}

// The class registry's finish-loaded-object callback for a breakable constraint data: the null check, then the class's
// finish constructor.
void finishLoadedObjecthkpBreakableConstraintData(void* p, int32_t finishing) {
    if (!p) {
        return;
    }
    hkFinishLoadedObjectFlag flag;
    flag.m_finishing = finishing;
    ::new (p) hkpBreakableConstraintData(flag);
}

void cleanupLoadedObjecthkpBreakableConstraintData(void* p) {
    static_cast<hkpBreakableConstraintData*>(p)->~hkpBreakableConstraintData();
}

const hkClass& hkpBreakableConstraintData::staticClass() {
    return hkpBreakableConstraintDataClass;
}
