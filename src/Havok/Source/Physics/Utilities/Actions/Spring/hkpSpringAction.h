#pragma once

#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Common/Base/Reflection/hkClass.h"
#include "Havok/Source/Common/Base/Reflection/hkTypeInfo.h"
#include "Havok/Source/Common/Base/Types/hkBaseTypes.h"
#include "Havok/Source/Physics/Dynamics/Action/hkpBinaryAction.h"

#include <stdint.h>

class hkpRigidBody;

// Havok SDK signature (hkpUtilities.lib, tied by reloc).
const void* getVtablehkpSpringAction();
void finishLoadedObjecthkpSpringAction(void* p, int32_t finishing);
void cleanupLoadedObjecthkpSpringAction(void* p);

// A damped spring between two body-space attachment points.
class hkpSpringAction : public hkpBinaryAction {
public:
	hkpSpringAction(hkpRigidBody* entityA, hkpRigidBody* entityB, hkUlong userData);
	hkpSpringAction(hkFinishLoadedObjectFlag aFlag) : hkpBinaryAction(aFlag) {}
	~hkpSpringAction() override;

	void applyAction(const hkStepInfo& stepInfo) override;
	hkpAction* clone(const hkArray<hkpEntity*>& newEntities, const hkArray<hkpPhantom*>& newPhantoms) const override;

	void setPositionsInWorldSpace(const hkVector4& pivotA, const hkVector4& pivotB);
	void setPositionsInBodySpace(const hkVector4& pivotA, const hkVector4& pivotB);

	static const hkClass& staticClass();

	hkVector4 m_lastForce;
	hkVector4 m_positionAinA;
	hkVector4 m_positionBinB;
	float m_restLength;
	float m_strength;
	float m_damping;
	hkBool m_onCompression;
	hkBool m_onExtension;
};

extern hkClass hkpBinaryActionClass;
extern hkClass hkpSpringActionClass;
extern hkTypeInfo hkpSpringActionTypeInfo;
inline constexpr fnvd::Vtable hkpSpringAction_vftable{};
