#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkClassMember.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpBinaryAction.h"

#include <stdint.h>

class hkpRigidBody;

// Havok SDK signature (hkpUtilities.lib, tied by reloc).
const void* getVtablehkpDashpotAction();
void finishLoadedObjecthkpDashpotAction(void* p, int32_t finishing);
void cleanupLoadedObjecthkpDashpotAction(void* p);

// A spring-damper impulse between two body attachment points.
class hkpDashpotAction : public hkpBinaryAction {
public:
	// The reflected members of hkpDashpotAction.
	using MembersArray = const hkClassMember[4];

	// The constructor, on apMemory: the binary action over the two bodies, zero points, strength 0.1 and damping
	// 0.01; m_impulse is left unwritten.
	hkpDashpotAction(hkpRigidBody* entityA, hkpRigidBody* entityB, uint32_t userData);
	hkpDashpotAction(hkFinishLoadedObjectFlag aFlag) : hkpBinaryAction(aFlag) {}
	~hkpDashpotAction() override;

	void applyAction(const hkStepInfo& stepInfo) override;
	hkpAction* clone(const hkArray<hkpEntity*>& newEntities, const hkArray<hkpPhantom*>& newPhantoms) const override;

	static const hkClass& staticClass();

	hkVector4 m_point[2];
	float m_strength;
	float m_damping;
	hkVector4 m_impulse;

	static MembersArray Members;
};

extern hkClass hkpDashpotActionClass;
extern hkTypeInfo hkpDashpotActionTypeInfo;
