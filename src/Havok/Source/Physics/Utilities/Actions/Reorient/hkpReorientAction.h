#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpUnaryAction.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class hkpRigidBody;

// Havok SDK signature (hkpUtilities.lib, tied by bytes,reloc).
const void* getVtablehkpReorientAction();
void finishLoadedObjecthkpReorientAction(void* p, int32_t finishing);
void cleanupLoadedObjecthkpReorientAction(void* p);

// Applies restoring torque to keep an entity's axes aligned. The release PDB has no member between the 0x1C-byte
// hkpUnaryAction base and m_rotationAxis, nor after m_damping: the 4 and 8 bytes are the hkVector4 alignment
// (hkputilities.pdb, hkpReorientAction, m_rotationAxis at 0x20, m_damping at 0x44, sizeof 80).
class hkpReorientAction : public hkpUnaryAction {
public:
	hkpReorientAction();
	hkpReorientAction(hkpRigidBody* body, const hkVector4& rotationAxis, const hkVector4& upAxis, float strength,
		float damping);
	hkpReorientAction(hkFinishLoadedObjectFlag aFlag) : hkpUnaryAction(aFlag.m_finishing) {}

	void applyAction(const hkStepInfo& stepInfo) override;
	hkpAction* clone(const hkArray<hkpEntity*>& newEntities, const hkArray<hkpPhantom*>& newPhantoms) const override;

	static const hkClass& staticClass();

	hkVector4 m_rotationAxis;
	hkVector4 m_upAxis;
	float m_strength;
	float m_damping;
};

extern hkClass hkpReorientActionClass;
extern hkTypeInfo hkpReorientActionTypeInfo;
inline constexpr fnvd::Vtable hkpReorientAction_vftable{};
