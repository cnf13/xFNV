#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysDragModifier.h"

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
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

#include <cmath>

NiRTTI NiPSysDragModifier::ms_RTTI("NiPSysDragModifier", &NiPSysModifier::ms_RTTI);

NiPSysDragModifier::NiPSysDragModifier()
	: m_pkDragObj(nullptr), m_kDragAxis(NiPoint3_UNIT_X), m_fPercentage(0.05f), m_fRange(3.4028235e+38f),
	  m_fFalloff(3.4028235e+38f) {}

NiPSysDragModifier::~NiPSysDragModifier() {}

NiObject* NiPSysDragModifier::CreateClone(NiCloningProcess& kCloning) {
	NiPSysDragModifier* v4;
	void* v3 = NiMemObject::operator new(sizeof(NiPSysDragModifier));
	if (v3) {
		v4 = ::new (v3) NiPSysDragModifier;
	} else {
		v4 = nullptr;
	}
	NiPSysModifier::CopyMembers(v4, kCloning);
	v4->m_kDragAxis.x = m_kDragAxis.x;
	v4->m_kDragAxis.y = m_kDragAxis.y;
	v4->m_kDragAxis.z = m_kDragAxis.z;
	v4->m_fPercentage = m_fPercentage;
	v4->m_fRange = m_fRange;
	v4->m_fFalloff = m_fFalloff;
	return v4;
}

void NiPSysDragModifier::LoadBinary(NiStream& kStream) {
	NiPSysModifier::LoadBinary(kStream);

	kStream.ReadLinkID();
	m_kDragAxis.LoadBinary(kStream);
	NiStreamLoadBinary(kStream, m_fPercentage);
	NiStreamLoadBinary(kStream, m_fRange);
	NiStreamLoadBinary(kStream, m_fFalloff);
}

void NiPSysDragModifier::LinkObject(NiStream& kStream) {
	NiPSysModifier::LinkObject(kStream);

	m_pkDragObj = static_cast<NiAVObject*>(kStream.GetObjectFromLinkID());
}

bool NiPSysDragModifier::RegisterStreamables(NiStream& kStream) {
	return NiPSysModifier::RegisterStreamables(kStream);
}

void NiPSysDragModifier::SaveBinary(NiStream& kStream) {
	NiPSysModifier::SaveBinary(kStream);

	kStream.SaveLinkID(m_pkDragObj);
	m_kDragAxis.SaveBinary(kStream);
	NiStreamSaveBinary(kStream, m_fPercentage);
	NiStreamSaveBinary(kStream, m_fRange);
	NiStreamSaveBinary(kStream, m_fFalloff);
}

bool NiPSysDragModifier::IsEqual(NiObject* pkObject) {
	if (!NiPSysModifier::IsEqual(pkObject))
		return false;

	NiPSysDragModifier* pkOther = static_cast<NiPSysDragModifier*>(pkObject);
	if ((m_pkDragObj && !pkOther->m_pkDragObj) || (!m_pkDragObj && pkOther->m_pkDragObj))
		return false;
	if (m_pkDragObj && pkOther->m_pkDragObj && !m_pkDragObj->IsEqual(pkOther->m_pkDragObj))
		return false;
	if (pkOther->m_kDragAxis != m_kDragAxis)
		return false;
	if (pkOther->m_fPercentage != m_fPercentage)
		return false;
	if (pkOther->m_fRange != m_fRange)
		return false;
	if (pkOther->m_fFalloff != m_fFalloff)
		return false;
	return true;
}

void NiPSysDragModifier::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiPSysModifier::GetViewerStrings(pkStrings);

	pkStrings->Add(NiGetViewerString(NiPSysDragModifier::ms_RTTI.GetName()));
	pkStrings->Add(NiGetViewerString("Drag Object", m_pkDragObj ? (const char*)m_pkDragObj->m_kName : "None"));
	pkStrings->Add(m_kDragAxis.GetViewerString("Drag Axis"));
	pkStrings->Add(NiGetViewerString("Percentage", m_fPercentage));
	pkStrings->Add(NiGetViewerString("Range", m_fRange));
	pkStrings->Add(NiGetViewerString("RangeFalloff", m_fFalloff));
}

void NiPSysDragModifier::ProcessClone(NiCloningProcess& kCloning) {
	NiPSysModifier::ProcessClone(kCloning);

	NiObject* pkObject = nullptr;
	kCloning.m_pkCloneMap->GetAt(this, pkObject);
	NiPSysDragModifier* pkClone = static_cast<NiPSysDragModifier*>(pkObject);
	if (m_pkDragObj) {
		if (kCloning.m_pkCloneMap->GetAt(m_pkDragObj, pkObject))
			pkClone->m_pkDragObj = static_cast<NiAVObject*>(pkObject);
		else
			pkClone->m_pkDragObj = m_pkDragObj;
	}
}

// Takes away the part of each velocity along the drag axis: the percentage per 1/30 s step, fading linearly to none
// between the range and the falloff distance from the drag object. A step product above 1 removes the whole part. Each
// NiPoint3 component is rounded to float where it is stored.
void NiPSysDragModifier::Update(float fTime, NiPSysData* pkData) {
	// a NaN percentage does nothing
	if (!(m_fPercentage > 0.0f))
		return;
	if (!pkData->m_usActive || !m_pkDragObj)
		return;

	// The drag object's transform in the particle system's space.
	NiTransform kDragXform = m_pkDragObj->m_kWorld;
	NiTransform kSystemXform = m_pkTarget->m_kWorld;
	NiTransform kInvSystemXform;
	kSystemXform.Invert(kInvSystemXform);
	NiTransform kTrans = kInvSystemXform * kDragXform;

	NiPoint3 kDir = kTrans.m_Rotate * m_kDragAxis;
	kDir.Unitize();
	// EXE-INLINED 0x4A7290
	float fDirSqrLength = kDir.x * kDir.x + kDir.y * kDir.y + kDir.z * kDir.z;
	float fFalloffWidth = m_fFalloff - m_fRange;

	for (uint16_t i = 0; i < pkData->m_usActive; i++) {
		// EXE-INLINED 0x439EF0
		const NiPoint3& kPos = pkData->m_pkVertex[i];
		float fDiffX = kPos.x - kTrans.m_Translate.x;
		float fDiffY = kPos.y - kTrans.m_Translate.y;
		float fDiffZ = kPos.z - kTrans.m_Translate.z;
		NiParticleInfo* pkInfo = &pkData->m_pkParticleInfo[i];
		// EXE-INLINED 0x4A7290
		float fSqrDist = fDiffX * fDiffX + fDiffY * fDiffY + fDiffZ * fDiffZ;
		float fDist = static_cast<float>(std::sqrt(fSqrDist));
		float fPercentage = m_fPercentage;
		float fSteps = (fTime - pkInfo->m_fLastUpdate) / 0.03333330154418945;

		// a NaN distance gets the full percentage
		if (m_fRange < fDist) {
			if (m_fFalloff <= fDist)
				continue;
			float fBeyondRange = fDist - m_fRange;
			fPercentage = (1.0f - fBeyondRange / fFalloffWidth) * fPercentage;
		}

		NiPoint3& kVelocity = pkInfo->m_kVelocity;
		// EXE-INLINED 0x4B6190
		float fDot = kVelocity.x * kDir.x + kVelocity.y * kDir.y + kVelocity.z * kDir.z;
		float fScale;
		if (fSteps * fPercentage > 1.0)
			fScale = -fDot / fDirSqrLength;
		else
			fScale = fSteps * -fPercentage * (fDot / fDirSqrLength);
		// EXE-INLINED 0x45BB20
		float fAddX = kDir.x * fScale;
		float fAddY = kDir.y * fScale;
		float fAddZ = kDir.z * fScale;
		// EXE-INLINED 0x63C8A0
		kVelocity.x += fAddX;
		kVelocity.y += fAddY;
		kVelocity.z += fAddZ;
	}
}

NiObject* NiPSysDragModifier::CreateObject() {
	void* v0 = NiMemObject::operator new(sizeof(NiPSysDragModifier));
	if (v0) {
		return ::new (v0) NiPSysDragModifier;
	} else {
		return nullptr;
	}
}
