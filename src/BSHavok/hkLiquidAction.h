#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpUnaryAction.h"

class hkpEntity;
class hkpRigidBody;

// The type-info finish function: rebuilds the vtable of a loaded hkLiquidAction at p (does nothing for null).
void finishLoadedObjecthkLiquidAction(void* p, int32_t finishing);
// The type-info vtable getter: returns hkLiquidAction's vtable address.
const void* getVtablehkLiquidAction();
void cleanupLoadedObjecthkLiquidAction(void* p);
// Destroys the object at p in place (flag 0).
void cleanupLoadedObject(void* p);

// Bethesda's Havok action behind bhkLiquidAction: every step it wakes its entity and pushes it with Force at the
// entity's centre of mass. It overrides applyAction and clone; the destructor and the rest are hkpUnaryAction's. clone
// copies Force.
class hkLiquidAction : public hkpUnaryAction {
public:
    // Constructor from the entity it acts on and its user data.
    hkLiquidAction(hkpRigidBody* entityA, uint32_t userData);
    hkLiquidAction(int32_t aiFinishing);

    // Slot 3.
    void applyAction(const hkStepInfo& stepInfo) override;
    // Slot 7: a copy on newEntities[0] with this action's user data and Force; none unless
    // newEntities holds 2 and newPhantoms none.
    hkpAction* clone(const hkArray<hkpEntity*>& newEntities, const hkArray<hkpPhantom*>& newPhantoms) const override;

    hkVector4 Force;
};

// The class's reflection record.
extern hkClass hkLiquidActionClass;
extern hkTypeInfo hkLiquidActionTypeInfo;
