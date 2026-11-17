#pragma once

#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Container/Array/hkArray.h"
#include "Havok/Source/Common/Base/Math/Quaternion/hkQuaternion.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpBinaryAction.h"
#include "Reconstruction.hpp"

#include <stdint.h>

struct hkStepInfo;
class hkpRigidBody;

void cleanupLoadedObjecthkpAngularDashpotAction(void* p);
// Havok SDK signature (hkpUtilities.lib, tied by reloc).
const void* getVtablehkpAngularDashpotAction();
void finishLoadedObjecthkpAngularDashpotAction(void* p, int32_t finishing);

// An action that turns entity B towards a fixed rotation relative to entity A, as a spring (strength) with damping.
class hkpAngularDashpotAction : public hkpBinaryAction {
public:
	// Runs the SDK's constructor (thiscall, ret 0xC, 0xD4DBE0) on apMemory: the two rigid
	// bodies it acts on and its user data (the PDB's unsigned long, pointer-wide on the native build). Returns
	// apMemory.
	hkpAngularDashpotAction(hkpRigidBody* apEntityA, hkpRigidBody* apEntityB, hkUlong auiUserData);
	hkpAngularDashpotAction(hkFinishLoadedObjectFlag aFlag) : hkpBinaryAction(aFlag) {}

	void applyAction(const hkStepInfo& stepInfo) override;
	hkpAction* clone(const hkArray<hkpEntity*>& newEntities, const hkArray<hkpPhantom*>& newPhantoms) const override;

	static const hkClass& staticClass();

	hkQuaternion m_rotation;
	float m_strength;
	float m_damping;
};

extern hkClass hkpAngularDashpotActionClass;
extern hkTypeInfo hkpAngularDashpotActionTypeInfo;
inline constexpr fnvd::Vtable hkpAngularDashpotAction_vftable{};
