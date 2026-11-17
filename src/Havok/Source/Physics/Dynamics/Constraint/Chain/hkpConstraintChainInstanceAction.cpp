#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainInstanceAction.h"

#include "BSCore/RTTI.h"
#include "Havok/Source/Common/Base/Container/hkContainerAllocators.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/Physics/hkStepInfo.h"
#include "Havok/Source/Physics/Collide/Shape/Compound/Tree/Mopp/Modifiers/hkpRemoveTerminalsMoppModifierClass.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Chain/hkpConstraintChainInstance.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Utilities/VisualDebugger/hkpPhysicsContext.h"

static void cleanupLoadedObjecthkpConstraintChainInstanceAction(void* p);

using hkpConstraintChainInstanceActionClass_MembersArray = const hkClassMember[1];
static hkpConstraintChainInstanceActionClass_MembersArray hkpConstraintChainInstanceActionClass_Members{
    {"constraintInstance",
     &hkpConstraintChainInstanceClass,
     nullptr,
     {hkClassMember::TYPE_POINTER},
     {hkClassMember::TYPE_STRUCT},
     0,
     {hkClassMember::NOT_OWNED},
     0x18,
     nullptr}
};
hkClass hkpConstraintChainInstanceActionClass(
    "hkpConstraintChainInstanceAction",
    &hkpActionClass,
    28,
    nullptr,
    0,
    nullptr,
    0,
    hkpConstraintChainInstanceActionClass_Members,
    1,
    nullptr,
    nullptr,
    0,
    0
);
hkTypeInfo hkpConstraintChainInstanceActionTypeInfo = {
    "hkpConstraintChainInstanceAction",
    finishLoadedObjecthkpConstraintChainInstanceAction,
    cleanupLoadedObjecthkpConstraintChainInstanceAction,
    getVtablehkpConstraintChainInstanceAction(),
    &typeid(hkpConstraintChainInstanceAction),
    sizeof(hkpConstraintChainInstanceAction)
};

static void cleanupLoadedObjecthkpConstraintChainInstanceAction(void* p) {
    static_cast<hkpConstraintChainInstanceAction*>(p)->~hkpConstraintChainInstanceAction();
}

void hkpConstraintChainInstanceAction::applyAction(const hkStepInfo& stepInfo) {
    ;
}

// The SDK's entitiesOut = m_constraintInstance->m_chainedEntities: hkArray's inline operator=, the heap allocator's
// copyFromArray.
void hkpConstraintChainInstanceAction::getEntities(hkArray<hkpEntity*, hkContainerHeapAllocator>& entitiesOut) {
    hkArray<hkpEntity*, hkContainerHeapAllocator>& chainedEntities = m_constraintInstance->m_chainedEntities;
    entitiesOut.copyFromArray(hkContainerHeapAllocator().get(&entitiesOut), chainedEntities, hkTypeIsPod());
}

void hkpConstraintChainInstanceAction::entityRemovedCallback(hkpEntity* entity) {
    m_constraintInstance->entityRemovedCallback(entity);
}

hkpAction* hkpConstraintChainInstanceAction::clone(
    const hkArray<hkpEntity*>& newEntities,
    const hkArray<hkpPhantom*>& newPhantoms
) const {
    return nullptr;
}

const hkClass& hkpConstraintChainInstanceAction::staticClass() {
    return hkpConstraintChainInstanceActionClass;
}
