#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysDragFieldModifier.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
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

#include <cmath>

NiRTTI NiPSysDragFieldModifier::ms_RTTI("NiPSysDragFieldModifier", &NiPSysFieldModifier::ms_RTTI);

NiPSysDragFieldModifier::NiPSysDragFieldModifier() : m_bUseDirection(false), m_kDirection(NiPoint3_ZERO) {}

NiPSysDragFieldModifier::~NiPSysDragFieldModifier() {}

NiObject* NiPSysDragFieldModifier::CreateClone(NiCloningProcess& kCloning) {
	NiPSysDragFieldModifier* pkObject =
		static_cast<NiPSysDragFieldModifier*>(NiMemObject::operator new(sizeof(NiPSysDragFieldModifier)));
	NiPSysDragFieldModifier* pkObject_2 = pkObject;
	if (pkObject) {
		::new (pkObject) NiPSysDragFieldModifier;
	} else {
		pkObject_2 = 0;
	}
	CopyMembers(pkObject_2, kCloning);
	pkObject_2->m_bUseDirection = m_bUseDirection;
	pkObject_2->m_kDirection = m_kDirection;
	return pkObject_2;
}

// KEEP (niread:): the exe reads the use-direction flag into the stream argument's own slot, so a short read
// (malformed data) leaves the low byte of the stream's address there and the exe tests it; src cannot express it.
// porting.md's unwritten-slot exception covers GetChunkData only.
void NiPSysDragFieldModifier::LoadBinary(NiStream& kStream) {
	NiPSysFieldModifier::LoadBinary(kStream);

	uint8_t ucUseDirection;
	NiStreamLoadBinary(kStream, ucUseDirection);
	m_bUseDirection = ucUseDirection != 0;
	m_kDirection.LoadBinary(kStream);
}

void NiPSysDragFieldModifier::LinkObject(NiStream& kStream) {
	NiPSysFieldModifier::LinkObject(kStream);
}

bool NiPSysDragFieldModifier::RegisterStreamables(NiStream& kStream) {
	return NiPSysFieldModifier::RegisterStreamables(kStream);
}

// The flag is written from a one-byte copy.
void NiPSysDragFieldModifier::SaveBinary(NiStream& kStream) {
	NiPSysFieldModifier::SaveBinary(kStream);

	uint8_t ucUseDirection = m_bUseDirection;
	NiStreamSaveBinary(kStream, ucUseDirection);
	m_kDirection.SaveBinary(kStream);
}

bool NiPSysDragFieldModifier::IsEqual(NiObject* pkObject) {
	if (!NiPSysFieldModifier::IsEqual(pkObject))
		return false;

	NiPSysDragFieldModifier* pkOther = static_cast<NiPSysDragFieldModifier*>(pkObject);
	if (pkOther->m_bUseDirection != m_bUseDirection)
		return false;
	if (pkOther->m_kDirection != m_kDirection)
		return false;
	return true;
}

void NiPSysDragFieldModifier::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiPSysFieldModifier::GetViewerStrings(pkStrings);

	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
	pkStrings->Add(NiGetViewerString("Use Direction", m_bUseDirection));
	pkStrings->Add(m_kDirection.GetViewerString("Direction"));
}

// Takes the fraction magnitude * time step * direction scale / (attenuation * distance + 1) of each particle's velocity
// away, all of it once the fraction reaches 1. The direction scale is the cosine between the velocity and the field
// direction (in the system's space) when the direction is used, else 1. Particles beyond the maximum distance (when
// used) and particles updated at afTime keep their velocity. With the direction used but zero, the game never sets its
// unit direction and takes the direction scale from an uninitialized NiPoint3; here that unit direction starts at zero.
void NiPSysDragFieldModifier::Update(float fTime, NiPSysData* pkData) {
	if (m_fMagnitude == 0.0f)
		return;
	if (!pkData->m_usActive || !m_pkFieldObj)
		return;

	if (!m_bUseDirection && m_fAttenuation == 0.0f) {
		if (m_bUseMaxDistance)
			UpdateNoAttenuationNoDir(fTime, pkData);
		else
			UpdateNoAttenuationNoDirNoDist(fTime, pkData);
		return;
	}

	// The field object's transform in the particle system's space.
	NiTransform kFieldXform = m_pkFieldObj->m_kWorld;
	NiTransform kSystemXform = m_pkTarget->m_kWorld;
	NiTransform kInvSystemXform;
	kSystemXform.Invert(kInvSystemXform);
	NiTransform kTrans = kInvSystemXform * kFieldXform;

	NiPoint3 kUnitDir = NiPoint3_ZERO;
	if (m_bUseDirection && m_kDirection != NiPoint3_ZERO) {
		kUnitDir = kTrans.m_Rotate * m_kDirection;
		kUnitDir.Unitize();
	}

	for (uint16_t i = 0; i < pkData->m_usActive; i++) {
		NiParticleInfo* pkInfo = &pkData->m_pkParticleInfo[i];
		float fDeltaT = fTime - pkInfo->m_fLastUpdate;
		if (fDeltaT == 0.0f)
			continue;

		NiPoint3 kDiff = pkData->m_pkVertex[i] - kTrans.m_Translate;
		float fSqrDist = kDiff.x * kDiff.x + kDiff.y * kDiff.y + kDiff.z * kDiff.z;
		float fDist = std::sqrt(fSqrDist);
		// a NaN distance counts as beyond the maximum
		if (m_bUseMaxDistance && !(fDist <= m_fMaxDistance))
			continue;

		float fDirScale = 1.0f;
		if (m_bUseDirection) {
			NiPoint3 kUnitVel = pkInfo->m_kVelocity;
			kUnitVel.Unitize();
			fDirScale = kUnitVel.Dot(kUnitDir);
		}

		float fFactor = fDeltaT * m_fMagnitude * fDirScale / (m_fAttenuation * fDist + 1.0);
		if (fFactor < 1.0f)
			pkInfo->m_kVelocity = pkInfo->m_kVelocity - pkInfo->m_kVelocity * fFactor;
		else
			pkInfo->m_kVelocity = NiPoint3_ZERO;
	}
}

// Update's path without attenuation or direction, with the maximum distance: a particle within the maximum distance
// keeps 1 - magnitude * time step of its velocity, none from a step of 1 up.
void NiPSysDragFieldModifier::UpdateNoAttenuationNoDir(float fTime, NiPSysData* pkData) {
	NiTransform kFieldXform = m_pkFieldObj->m_kWorld;
	NiTransform kSystemXform = m_pkTarget->m_kWorld;
	NiTransform kInvSystemXform;
	kSystemXform.Invert(kInvSystemXform);
	NiTransform kTrans = kInvSystemXform * kFieldXform;

	for (uint16_t i = 0; i < pkData->m_usActive; i++) {
		NiParticleInfo* pkInfo = &pkData->m_pkParticleInfo[i];
		float fFactor = (fTime - pkInfo->m_fLastUpdate) * m_fMagnitude;
		if (fFactor == 0.0f)
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

		if (fFactor < 1.0f) {
			float fScale = 1.0f - fFactor;
			pkInfo->m_kVelocity.x = pkInfo->m_kVelocity.x * fScale;
			pkInfo->m_kVelocity.y = pkInfo->m_kVelocity.y * fScale;
			pkInfo->m_kVelocity.z = pkInfo->m_kVelocity.z * fScale;
		} else {
			pkInfo->m_kVelocity = NiPoint3_ZERO;
		}
	}
}

// Update's path without attenuation, direction or maximum distance: every particle keeps 1 - magnitude * time step
// of its velocity, none from a step of 1 up.
void NiPSysDragFieldModifier::UpdateNoAttenuationNoDirNoDist(float fTime, NiPSysData* pkData) {
	for (uint16_t i = 0; i < pkData->m_usActive; i++) {
		NiParticleInfo* pkInfo = &pkData->m_pkParticleInfo[i];
		float fFactor = (fTime - pkInfo->m_fLastUpdate) * m_fMagnitude;
		if (fFactor < 1.0f) {
			float fScale = 1.0f - fFactor;
			// EXE-INLINED 0x439180
			pkInfo->m_kVelocity.x = pkInfo->m_kVelocity.x * fScale;
			pkInfo->m_kVelocity.y = pkInfo->m_kVelocity.y * fScale;
			pkInfo->m_kVelocity.z = pkInfo->m_kVelocity.z * fScale;
		} else {
			pkInfo->m_kVelocity = NiPoint3_ZERO;
		}
	}
}

NiObject* NiPSysDragFieldModifier::CreateObject() {
	NiPSysDragFieldModifier* pkObject =
		static_cast<NiPSysDragFieldModifier*>(NiMemObject::operator new(sizeof(NiPSysDragFieldModifier)));
	if (!pkObject) {
		return 0;
	}
	return ::new (pkObject) NiPSysDragFieldModifier;
}
