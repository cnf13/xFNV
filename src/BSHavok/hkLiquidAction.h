#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpUnaryAction.h"
#include "Reconstruction.hpp"

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
// entity's centre of mass. The exe's vtable has 8 slots; it overrides applyAction (slot 3) and clone (slot 7), the
// destructor and the rest are hkpUnaryAction's (the RTTI's base). Layout from the exe's hkLiquidActionClass record
// (Force at 0x20, size 0x30) and clone (0xD2C9D0), which allocates 0x30 and copies Force. The PDB's source is
// BSHavok\hkLiquidAction.cpp.
class hkLiquidAction : public hkpUnaryAction {
public:
	// The exe's constructor from the entity it acts on and its user data (thiscall, ret 8,
	// 0xD2C910).
	hkLiquidAction(hkpRigidBody* entityA, uint32_t userData);
	hkLiquidAction(int32_t aiFinishing);

	// Slot 3.
	void applyAction(const hkStepInfo& stepInfo) override;
	// Slot 7: a copy on newEntities[0] with this action's user data and Force; none unless
	// newEntities holds 2 and newPhantoms none.
	hkpAction* clone(const hkArray<hkpEntity*>& newEntities, const hkArray<hkpPhantom*>& newPhantoms) const override;

	hkVector4 Force;
};
inline constexpr fnvd::Vtable hkLiquidAction_vftable{};

// The class's reflection record.
extern hkClass hkLiquidActionClass;
extern hkTypeInfo hkLiquidActionTypeInfo;
