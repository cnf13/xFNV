#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpUnaryAction.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkpRigidBody;

// Havok SDK signature (hkpUtilities.lib, tied by reloc).
const void* getVtablehkpMotorAction();
void finishLoadedObjecthkpMotorAction(void* p, int32_t finishing);
void cleanupLoadedObjecthkpMotorAction(void* p);

// The Havok motor action a bhkMotorAction wraps: a unary action that spins its entity about an axis.
class hkpMotorAction : public hkpUnaryAction {
public:
	hkpMotorAction(hkpRigidBody* body, const hkVector4& axis, float spinRate, float gain);
	hkpMotorAction(hkFinishLoadedObjectFlag aFlag) : hkpUnaryAction(aFlag.m_finishing) {}

	// hkpAction slots 3 and 7. The PDB's parameter names for these are its locals'; these are hkpAction's.
	void applyAction(const hkStepInfo& stepInfo) override;
	hkpAction* clone(const hkArray<hkpEntity*>& newEntities, const hkArray<hkpPhantom*>& newPhantoms) const override;

	static const hkClass& staticClass();

	hkVector4 m_axis;
	float m_spinRate;
	float m_gain;
	hkBool m_active;
};

extern hkClass hkpMotorActionClass;
extern hkTypeInfo hkpMotorActionTypeInfo;
inline constexpr fnvd::Vtable hkpMotorAction_vftable{};
