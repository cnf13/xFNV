#include "BSHavok/bhkWindListener.h"

#include "BSHavok/bhkEntity.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkRotation.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"

// This file's copy of 2 pi (6.2831855f).
static float f6_2831855 = 6.2831855f;

bhkWindListener::bhkWindListener() : bhkEntityListener(1) {}

bhkWindListener::~bhkWindListener() {}

// Pushes every listed body along the wind: a randomised speed, floored at 0 and capped at fMaxWind, scaled by the
// frames afTime spans (afTime over the double -0.0167f, truncated), along a randomised direction wrapped into [0, 2
// pi]. Every intermediate is a float.
void bhkWindListener::Update(float afTime) {
	if (!bEnabled || !(fWindSpeed != 0.0f))
		return;
	int32_t iTicks = 1 - static_cast<int32_t>(static_cast<double>(afTime) / -0.016699999570846558);
	float fScale = static_cast<float>(iTicks);
	for (BSSimpleList<NiPointer<bhkEntity>>* pNode = &Entities; pNode && pNode->m_item.m_pObject;
		pNode = pNode->m_pkNext) {
		hkpEntity* pEntity = static_cast<hkpEntity*>(pNode->m_item.m_pObject->phkObject);
		float fSpeed =
			static_cast<float>(static_cast<double>(GetRandom(fWindSpeed)) + static_cast<double>(fWindSpeed) * 0.25);
		if (0.0f > fSpeed)
			fSpeed = 0.0f;
		if (fMaxWind < fSpeed)
			fSpeed = fMaxWind;
		fSpeed = fSpeed * fScale;
		float fAngle = fWindDir + GetRandom(0.7853982f);
		if (f6_2831855 < fAngle)
			fAngle = fAngle - f6_2831855;
		else if (0.0f > fAngle)
			fAngle = fAngle + f6_2831855;
		alignas(16) hkVector4 kWind;
		kWind.m_quad[0] = 0.0f;
		kWind.m_quad[1] = fSpeed;
		kWind.m_quad[2] = 0.0f;
		kWind.m_quad[3] = 0.0f;
		alignas(16) hkVector4 kAxis;
		kAxis.m_quad[0] = 0.0f;
		kAxis.m_quad[1] = 0.0f;
		kAxis.m_quad[2] = 1.0f;
		kAxis.m_quad[3] = 0.0f;
		alignas(16) hkRotation kRotation;
		kRotation.setAxisAngle(kAxis, fAngle);
		alignas(16) hkVector4 kForce;
		kForce.setRotatedDir(kRotation, kWind);
		if (pEntity) {
			pEntity->activate();
			// Called through a base pointer, so the call is indirect.
			hkpMotion* pMotion = &pEntity->m_motion;
			pMotion->applyForce(afTime, kForce);
		}
	}
}

// The wind is not randomised: the value is always zero.
float bhkWindListener::GetRandom(float) {
	return 0.0f;
}

void bhkWindListener::SetWind(float afSpeed, float afAngle) {
	bhkWindListener::fWindSpeed = (float)(bhkWindListener::fMaxWind * afSpeed);
	bhkWindListener::fWindDir = afAngle;
}
