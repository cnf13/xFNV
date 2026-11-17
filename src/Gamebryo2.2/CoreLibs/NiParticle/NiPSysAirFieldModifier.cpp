#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysAirFieldModifier.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleInfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysFieldModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

#include <cfloat>
#include <cmath>

NiRTTI NiPSysAirFieldModifier::ms_RTTI("NiPSysAirFieldModifier", &NiPSysFieldModifier::ms_RTTI);

// This file's non-const copies of pi / 2 and 2 * pi (no code writes them); only Update reads them.
static float s_fHalfPi = 1.5707964f;
static float s_fTwoPi = 6.2831855f;

NiPSysAirFieldModifier::NiPSysAirFieldModifier()
	: m_kLastFieldPosition(m_ckUninitializePosition), m_fLastUpdateTime(3.4028235e+38f) {
	SetDirection(-NiPoint3_UNIT_X);
}

NiPSysAirFieldModifier::~NiPSysAirFieldModifier() {}

NiObject* NiPSysAirFieldModifier::CreateClone(NiCloningProcess& kCloning) {
	NiPSysAirFieldModifier* pkObject;
	NiPSysAirFieldModifier* v3 =
		static_cast<NiPSysAirFieldModifier*>(NiMemObject::operator new(sizeof(NiPSysAirFieldModifier)));
	if (v3) {
		pkObject = ::new (v3) NiPSysAirFieldModifier;
	} else {
		pkObject = 0;
	}
	CopyMembers(pkObject, kCloning);
	pkObject->SetDirection(m_kDirection);
	return pkObject;
}

// Each of the three flags is read into the same temporary and then stored; a short read leaves the temporary unwritten,
// so an uninitialised value is stored, as the game does.
void NiPSysAirFieldModifier::LoadBinary(NiStream& kStream) {
	NiPSysFieldModifier::LoadBinary(kStream);

	m_kDirection.LoadBinary(kStream);
	SetDirection(m_kDirection);
	NiStreamLoadBinary(kStream, m_fAirFriction);
	NiStreamLoadBinary(kStream, m_fInheritVelocity);
	uint8_t ucValue;
	NiStreamLoadBinary(kStream, ucValue);
	m_bInheritRotation = ucValue != 0;
	NiStreamLoadBinary(kStream, ucValue);
	m_bComponentOnly = ucValue != 0;
	NiStreamLoadBinary(kStream, ucValue);
	m_bEnableSpread = ucValue != 0;
	NiStreamLoadBinary(kStream, m_fSpread);
}

void NiPSysAirFieldModifier::LinkObject(NiStream& kStream) {
	NiPSysFieldModifier::LinkObject(kStream);
}

bool NiPSysAirFieldModifier::RegisterStreamables(NiStream& kStream) {
	return NiPSysFieldModifier::RegisterStreamables(kStream);
}

// The flags are written from one-byte copies.
void NiPSysAirFieldModifier::SaveBinary(NiStream& kStream) {
	NiPSysFieldModifier::SaveBinary(kStream);

	m_kDirection.SaveBinary(kStream);
	NiStreamSaveBinary(kStream, m_fAirFriction);
	NiStreamSaveBinary(kStream, m_fInheritVelocity);
	uint8_t ucInheritRotation = m_bInheritRotation;
	NiStreamSaveBinary(kStream, ucInheritRotation);
	uint8_t ucComponentOnly = m_bComponentOnly;
	NiStreamSaveBinary(kStream, ucComponentOnly);
	uint8_t ucEnableSpread = m_bEnableSpread;
	NiStreamSaveBinary(kStream, ucEnableSpread);
	NiStreamSaveBinary(kStream, m_fSpread);
}

// Only the direction is compared besides the base members.
bool NiPSysAirFieldModifier::IsEqual(NiObject* pkObject) {
	if (!NiPSysFieldModifier::IsEqual(pkObject))
		return false;

	NiPSysAirFieldModifier* pkOther = static_cast<NiPSysAirFieldModifier*>(pkObject);
	if (pkOther->m_kDirection != m_kDirection)
		return false;
	return true;
}

void NiPSysAirFieldModifier::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiPSysFieldModifier::GetViewerStrings(pkStrings);

	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
	pkStrings->Add(m_kDirection.GetViewerString("Direction"));
	pkStrings->Add(m_kUnitDirection.GetViewerString("Unit Direction"));
	pkStrings->Add(NiGetViewerString("AirFriction", m_fAirFriction));
	pkStrings->Add(NiGetViewerString("Inherit Velocity", m_fInheritVelocity));
	pkStrings->Add(NiGetViewerString("Inherit Rotation", m_bInheritRotation));
	pkStrings->Add(NiGetViewerString("Component Only", m_bComponentOnly));
	pkStrings->Add(NiGetViewerString("Enable Spread", m_bEnableSpread));
	pkStrings->Add(NiGetViewerString("Spread", m_fSpread));
}

// The field's velocity is magnitude along the unit direction (rotated by the field object when it inherits rotation)
// plus the field object's own velocity since the last update times the inherit factor. With spread, a particle is
// pushed away from the field's center at the field's speed instead, and only inside the cone of half angle spread * pi
// / 2 around the field's velocity. A particle slower than the push along it gains push * air friction * time step,
// attenuated by (1 - distance / maximum distance) ^ attenuation when the maximum distance is used. Each NiPoint3
// component is rounded to float where it is stored.
void NiPSysAirFieldModifier::Update(float fTime, NiPSysData* pkData) {
	if (!pkData->m_usActive || !m_pkFieldObj)
		return;

	if (m_fAttenuation == 0.0f && m_fInheritVelocity == 0.0f && !m_bEnableSpread) {
		if (m_bUseMaxDistance)
			UpdateNoAttenuationNoInheritNoSpread(fTime, pkData);
		else
			UpdateNoAttenuationNoInheritNoSpreadNoMaxDistance(fTime, pkData);
		return;
	}

	// The field object's transform in the particle system's space.
	NiTransform kFieldXform = m_pkFieldObj->m_kWorld;
	NiTransform kSystemXform = m_pkTarget->m_kWorld;
	NiTransform kInvSystemXform;
	kSystemXform.Invert(kInvSystemXform);
	NiTransform kTrans = kInvSystemXform * kFieldXform;

	NiPoint3 kUnitDir;
	if (m_bInheritRotation)
		kUnitDir = kTrans.m_Rotate * m_kUnitDirection;
	else
		kUnitDir = m_kUnitDirection;

	NiPoint3 kFieldVelocity = NiPoint3_ZERO;
	if (m_kLastFieldPosition != m_ckUninitializePosition && m_fLastUpdateTime != FLT_MAX &&
		fTime != m_fLastUpdateTime) {
		NiPoint3 kMoved;
		kMoved.x = kTrans.m_Translate.x - m_kLastFieldPosition.x;
		kMoved.y = kTrans.m_Translate.y - m_kLastFieldPosition.y;
		kMoved.z = kTrans.m_Translate.z - m_kLastFieldPosition.z;
		float fInheritVelocity = m_fInheritVelocity;
		NiPoint3 kInherited;
		kInherited.x = kMoved.x * fInheritVelocity;
		kInherited.y = kMoved.y * fInheritVelocity;
		kInherited.z = kMoved.z * fInheritVelocity;
		float fDeltaT = fTime - m_fLastUpdateTime;
		kFieldVelocity = kInherited / fDeltaT;
	}
	m_fLastUpdateTime = fTime;
	m_kLastFieldPosition = kTrans.m_Translate;
	float fMagnitude = m_fMagnitude;
	NiPoint3 kPush;
	kPush.x = kUnitDir.x * fMagnitude;
	kPush.y = kUnitDir.y * fMagnitude;
	kPush.z = kUnitDir.z * fMagnitude;
	kFieldVelocity.x = kPush.x + kFieldVelocity.x;
	kFieldVelocity.y = kFieldVelocity.y + kPush.y;
	kFieldVelocity.z = kFieldVelocity.z + kPush.z;

	float fSqrSpeed =
		kFieldVelocity.x * kFieldVelocity.x + kFieldVelocity.y * kFieldVelocity.y + kFieldVelocity.z * kFieldVelocity.z;
	if (fSqrSpeed == 0.0f)
		return;

	NiPoint3 kUnitFieldVelocity = kFieldVelocity;
	float fFieldSpeed = kUnitFieldVelocity.UnitizeGetLength();
	float fCosSpread = 0.0f;
	if (m_bEnableSpread) {
		float fSpreadAngle = m_fSpread * s_fHalfPi;
		fCosSpread = s_fCosTableA[static_cast<int32_t>(fSpreadAngle * (512.0 / s_fTwoPi)) & 0x1FF];
	}

	for (uint16_t i = 0; i < pkData->m_usActive; i++) {
		NiParticleInfo* pkInfo = &pkData->m_pkParticleInfo[i];
		float fDeltaT = fTime - pkInfo->m_fLastUpdate;
		if (fDeltaT == 0.0f)
			continue;

		NiPoint3 kVertex = pkData->m_pkVertex[i];
		NiPoint3 kDiff;
		kDiff.x = kVertex.x - kTrans.m_Translate.x;
		kDiff.y = kVertex.y - kTrans.m_Translate.y;
		kDiff.z = kVertex.z - kTrans.m_Translate.z;
		float fSqrDist = kDiff.x * kDiff.x + kDiff.y * kDiff.y + kDiff.z * kDiff.z;
		// a NaN distance counts as beyond the maximum
		if (m_bUseMaxDistance && !(fSqrDist <= m_fMaxDistanceSqr))
			continue;

		NiPoint3 kForce = kFieldVelocity;
		if (m_bEnableSpread) {
			kForce = kDiff;
			kForce.Unitize();
			float fCos =
				kUnitFieldVelocity.x * kForce.x + kUnitFieldVelocity.y * kForce.y + kUnitFieldVelocity.z * kForce.z;
			if (fCosSpread > fCos)
				continue;
			kForce.x = kForce.x * fFieldSpeed;
			kForce.y = kForce.y * fFieldSpeed;
			kForce.z = kForce.z * fFieldSpeed;
		}

		NiPoint3 kVelocity = pkInfo->m_kVelocity;
		NiPoint3 kRelative;
		kRelative.x = kVelocity.x - kForce.x;
		kRelative.y = kVelocity.y - kForce.y;
		kRelative.z = kVelocity.z - kForce.z;
		float fDot = kRelative.x * kForce.x + kRelative.y * kForce.y + kRelative.z * kForce.z;
		if (fDot < 0.0f) {
			float fScale = fDeltaT;
			if (m_bUseMaxDistance && m_fAttenuation != 0.0f) {
				float fDist = std::sqrt(fSqrDist);
				float fBase = 1.0f - fDist / m_fMaxDistance;
				float fFalloff = static_cast<float>(std::pow(fBase, m_fAttenuation));
				fScale = fFalloff * fDeltaT;
			}
			float fAirFriction = m_fAirFriction;
			NiPoint3 kScaled;
			kScaled.x = kForce.x * fAirFriction;
			kScaled.y = kForce.y * fAirFriction;
			kScaled.z = kForce.z * fAirFriction;
			NiPoint3 kAdd;
			kAdd.x = kScaled.x * fScale;
			kAdd.y = kScaled.y * fScale;
			kAdd.z = kScaled.z * fScale;
			kVelocity.x = kAdd.x + kVelocity.x;
			pkInfo->m_kVelocity.x = kVelocity.x;
			kVelocity.y = kAdd.y + kVelocity.y;
			pkInfo->m_kVelocity.y = kVelocity.y;
			kVelocity.z = kAdd.z + kVelocity.z;
			pkInfo->m_kVelocity.z = kVelocity.z;
		}
	}
}

void NiPSysAirFieldModifier::SetDirection(const NiPoint3& kDirection) {
	m_kDirection = kDirection;
	m_kUnitDirection = kDirection;
	m_kUnitDirection.Unitize();
	// EXE-INLINED 0x4390C0
	if (m_kUnitDirection.x == NiPoint3_ZERO.x && m_kUnitDirection.y == NiPoint3_ZERO.y &&
		m_kUnitDirection.z == NiPoint3_ZERO.z)
		m_kUnitDirection = NiPoint3_UNIT_X;
}

// Update's path without attenuation, inherited velocity or spread, with the maximum distance: a particle within the
// maximum distance and slower than the field along it gains field velocity * air friction * time step. Each component
// of the NiPoint3 operations (scaling, difference, dot, add) is rounded to float where it is stored.
void NiPSysAirFieldModifier::UpdateNoAttenuationNoInheritNoSpread(float fTime, NiPSysData* pkData) {
	if (m_fMagnitude == 0.0f)
		return;

	NiTransform kFieldXform = m_pkFieldObj->m_kWorld;
	NiTransform kSystemXform = m_pkTarget->m_kWorld;
	NiTransform kInvSystemXform;
	kSystemXform.Invert(kInvSystemXform);
	NiTransform kTrans = kInvSystemXform * kFieldXform;
	m_fLastUpdateTime = fTime;
	m_kLastFieldPosition = kTrans.m_Translate;

	NiPoint3 kUnitDir;
	if (m_bInheritRotation)
		kUnitDir = kTrans.m_Rotate * m_kUnitDirection;
	else
		kUnitDir = m_kUnitDirection;
	float fMagnitude = m_fMagnitude;
	NiPoint3 kForce;
	kForce.x = kUnitDir.x * fMagnitude;
	kForce.y = kUnitDir.y * fMagnitude;
	kForce.z = kUnitDir.z * fMagnitude;

	for (uint16_t i = 0; i < pkData->m_usActive; i++) {
		NiParticleInfo* pkInfo = &pkData->m_pkParticleInfo[i];
		float fDeltaT = fTime - pkInfo->m_fLastUpdate;
		if (fDeltaT == 0.0f)
			continue;

		NiPoint3 kVertex = pkData->m_pkVertex[i];
		NiPoint3 kDiff;
		kDiff.x = kVertex.x - kTrans.m_Translate.x;
		kDiff.y = kVertex.y - kTrans.m_Translate.y;
		kDiff.z = kVertex.z - kTrans.m_Translate.z;
		float fSqrDist = kDiff.x * kDiff.x + kDiff.y * kDiff.y + kDiff.z * kDiff.z;
		// a NaN distance counts as beyond the maximum
		if (!(fSqrDist <= m_fMaxDistanceSqr))
			continue;

		NiPoint3 kRelative;
		kRelative.x = pkInfo->m_kVelocity.x - kForce.x;
		kRelative.y = pkInfo->m_kVelocity.y - kForce.y;
		kRelative.z = pkInfo->m_kVelocity.z - kForce.z;
		float fDot = kRelative.x * kForce.x + kRelative.y * kForce.y + kRelative.z * kForce.z;
		if (fDot < 0.0f) {
			float fAirFriction = m_fAirFriction;
			NiPoint3 kScaled;
			kScaled.x = kForce.x * fAirFriction;
			kScaled.y = kForce.y * fAirFriction;
			kScaled.z = kForce.z * fAirFriction;
			NiPoint3 kAdd;
			kAdd.x = kScaled.x * fDeltaT;
			kAdd.y = kScaled.y * fDeltaT;
			kAdd.z = kScaled.z * fDeltaT;
			pkInfo->m_kVelocity.x = pkInfo->m_kVelocity.x + kAdd.x;
			pkInfo->m_kVelocity.y = pkInfo->m_kVelocity.y + kAdd.y;
			pkInfo->m_kVelocity.z = pkInfo->m_kVelocity.z + kAdd.z;
		}
	}
}

// Update's path without attenuation, inherited velocity, spread or maximum distance: a particle slower than the field
// along it gains field velocity * air friction * time step. Each NiPoint3 component is rounded to float.
void NiPSysAirFieldModifier::UpdateNoAttenuationNoInheritNoSpreadNoMaxDistance(float fTime, NiPSysData* pkData) {
	if (m_fMagnitude == 0.0f)
		return;

	NiTransform kFieldXform = m_pkFieldObj->m_kWorld;
	NiTransform kSystemXform = m_pkTarget->m_kWorld;
	NiTransform kInvSystemXform;
	kSystemXform.Invert(kInvSystemXform);
	NiTransform kTrans = kInvSystemXform * kFieldXform;
	m_fLastUpdateTime = fTime;
	m_kLastFieldPosition = kTrans.m_Translate;

	NiPoint3 kUnitDir;
	if (m_bInheritRotation)
		kUnitDir = kTrans.m_Rotate * m_kUnitDirection;
	else
		kUnitDir = m_kUnitDirection;
	float fMagnitude = m_fMagnitude;
	NiPoint3 kForce;
	kForce.x = kUnitDir.x * fMagnitude;
	kForce.y = kUnitDir.y * fMagnitude;
	kForce.z = kUnitDir.z * fMagnitude;

	for (uint16_t i = 0; i < pkData->m_usActive; i++) {
		NiParticleInfo* pkInfo = &pkData->m_pkParticleInfo[i];
		float fDeltaT = fTime - pkInfo->m_fLastUpdate;
		if (fDeltaT == 0.0f)
			continue;

		NiPoint3 kRelative;
		kRelative.x = pkInfo->m_kVelocity.x - kForce.x;
		kRelative.y = pkInfo->m_kVelocity.y - kForce.y;
		kRelative.z = pkInfo->m_kVelocity.z - kForce.z;
		float fDot = kRelative.x * kForce.x + kRelative.y * kForce.y + kRelative.z * kForce.z;
		if (fDot < 0.0f) {
			float fAirFriction = m_fAirFriction;
			NiPoint3 kScaled;
			kScaled.x = kForce.x * fAirFriction;
			kScaled.y = kForce.y * fAirFriction;
			kScaled.z = kForce.z * fAirFriction;
			NiPoint3 kAdd;
			kAdd.x = kScaled.x * fDeltaT;
			kAdd.y = kScaled.y * fDeltaT;
			kAdd.z = kScaled.z * fDeltaT;
			pkInfo->m_kVelocity.x = pkInfo->m_kVelocity.x + kAdd.x;
			pkInfo->m_kVelocity.y = pkInfo->m_kVelocity.y + kAdd.y;
			pkInfo->m_kVelocity.z = pkInfo->m_kVelocity.z + kAdd.z;
		}
	}
}

NiObject* NiPSysAirFieldModifier::CreateObject() {
	void* v0 = NiMemObject::operator new(sizeof(NiPSysAirFieldModifier));
	if (v0) {
		return ::new (v0) NiPSysAirFieldModifier;
	} else {
		return nullptr;
	}
}
