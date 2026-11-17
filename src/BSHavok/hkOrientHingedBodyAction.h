#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpUnaryAction.h"
#include "Reconstruction.hpp"

class hkpRigidBody;

// Bethesda's action that turns a body hinged on one axis (a door, a lid) towards a target direction with a spring
// and a damper. The exe folds its destructor into hkpUnaryAction's.
class hkOrientHingedBodyAction : public hkpUnaryAction {
public:
	hkOrientHingedBodyAction(hkpRigidBody* entityA, const hkVector4& arHingeAxisLS, const hkVector4& arForwardLS,
		float afStrength, float afDamping, uint32_t userData);

	void applyAction(const hkStepInfo& stepInfo) override;
	hkpAction* clone(const hkArray<hkpEntity*>& newEntities, const hkArray<hkpPhantom*>& newPhantoms) const override;

	void SetEnabled(bool abEnable);

	// Copies arDir into TargetDirWS (thiscall, ret 4, 0x887B20).
	// Inferred type: arDir: hkVector4* from 0x887b0b (parameter 0 of bhkOrientHingedBodyAction::SetTargetDir)
	void setTargetDir(const hkVector4& arDir);

	hkVector4 HingeAxisLS;
	hkVector4 ForwardLS;
	hkVector4 TargetDirWS;
	float fStrength;
	float fDamping;
	float fAngleDiff;
	bool bEnabled;
};
inline constexpr fnvd::Vtable hkOrientHingedBodyAction_vftable{};
