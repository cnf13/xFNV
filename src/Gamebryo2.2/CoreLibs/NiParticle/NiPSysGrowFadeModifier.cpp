#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysGrowFadeModifier.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleInfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

NiRTTI NiPSysGrowFadeModifier::ms_RTTI("NiPSysGrowFadeModifier", &NiPSysModifier::ms_RTTI);

NiPSysGrowFadeModifier::NiPSysGrowFadeModifier()
	: NiPSysModifier(), m_fGrowTime(0.0f), m_usGrowGeneration(0), m_fFadeTime(0.0f), m_usFadeGeneration(0),
	  m_fBaseScale(0.0f) {}

NiPSysGrowFadeModifier::~NiPSysGrowFadeModifier() {}

NiObject* NiPSysGrowFadeModifier::CreateClone(NiCloningProcess& kCloning) {
	NiPSysGrowFadeModifier* pkObject =
		static_cast<NiPSysGrowFadeModifier*>(NiMemObject::operator new(sizeof(NiPSysGrowFadeModifier)));
	NiPSysGrowFadeModifier* pkObject_2 = pkObject;
	if (pkObject) {
		::new (pkObject) NiPSysGrowFadeModifier;
	} else {
		pkObject_2 = 0;
	}
	CopyMembers(pkObject_2, kCloning);
	pkObject_2->m_fGrowTime = m_fGrowTime;
	pkObject_2->m_usGrowGeneration = m_usGrowGeneration;
	pkObject_2->m_fFadeTime = m_fFadeTime;
	pkObject_2->m_usFadeGeneration = m_usFadeGeneration;
	pkObject_2->m_fBaseScale = m_fBaseScale;
	return pkObject_2;
}

// The base scale is streamed from Bethesda stream version 17 on.
void NiPSysGrowFadeModifier::LoadBinary(NiStream& kStream) {
	NiPSysModifier::LoadBinary(kStream);

	NiStreamLoadBinary(kStream, m_fGrowTime);
	NiStreamLoadBinary(kStream, m_usGrowGeneration);
	NiStreamLoadBinary(kStream, m_fFadeTime);
	NiStreamLoadBinary(kStream, m_usFadeGeneration);
	if (kStream.m_BSStreamHeader.uiVersion > 0x10) {
		NiStreamLoadBinary(kStream, m_fBaseScale);
	}
}

void NiPSysGrowFadeModifier::LinkObject(NiStream& kStream) {
	NiPSysModifier::LinkObject(kStream);
}

bool NiPSysGrowFadeModifier::RegisterStreamables(NiStream& kStream) {
	return NiPSysModifier::RegisterStreamables(kStream);
}

void NiPSysGrowFadeModifier::SaveBinary(NiStream& kStream) {
	NiPSysModifier::SaveBinary(kStream);

	NiStreamSaveBinary(kStream, m_fGrowTime);
	NiStreamSaveBinary(kStream, m_usGrowGeneration);
	NiStreamSaveBinary(kStream, m_fFadeTime);
	NiStreamSaveBinary(kStream, m_usFadeGeneration);
	NiStreamSaveBinary(kStream, m_fBaseScale);
}

// The base scale is compared before the fade generation.
bool NiPSysGrowFadeModifier::IsEqual(NiObject* pkObject) {
	if (!NiPSysModifier::IsEqual(pkObject)) {
		return false;
	}

	NiPSysGrowFadeModifier* pkOther = static_cast<NiPSysGrowFadeModifier*>(pkObject);
	if (m_fGrowTime != pkOther->m_fGrowTime || m_usGrowGeneration != pkOther->m_usGrowGeneration ||
		m_fFadeTime != pkOther->m_fFadeTime || m_fBaseScale != pkOther->m_fBaseScale ||
		m_usFadeGeneration != pkOther->m_usFadeGeneration) {
		return false;
	}
	return true;
}

void NiPSysGrowFadeModifier::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiPSysModifier::GetViewerStrings(pkStrings);

	pkStrings->Add(NiGetViewerString(NiPSysGrowFadeModifier::ms_RTTI.GetName()));
	pkStrings->Add(NiGetViewerString("Grow Time", m_fGrowTime));
	pkStrings->Add(NiGetViewerString("Grow Generation", m_usGrowGeneration));
	pkStrings->Add(NiGetViewerString("Fade Time", m_fFadeTime));
	pkStrings->Add(NiGetViewerString("Fade Generation", m_usFadeGeneration));
	pkStrings->Add(NiGetViewerString("Base Scale", m_fBaseScale));
}

// Particles of the grow generation scale up over the grow time after birth, particles of the fade generation
// scale down over the fade time before death; the smaller factor maps 0..1 onto base scale..1. Sizes never
// drop below 0.0001.
void NiPSysGrowFadeModifier::Update(float fTime, NiPSysData* pkData) {
	float fScaleRange = 1.0f - m_fBaseScale;
	for (uint16_t i = 0; i < pkData->m_usActive; i++) {
		NiParticleInfo* pkInfo = &pkData->m_pkParticleInfo[i];

		float fGrow = 1.0f;
		if (pkInfo->m_usGeneration == m_usGrowGeneration && m_fGrowTime > pkInfo->m_fAge && m_fGrowTime != 0.0f) {
			fGrow = pkInfo->m_fAge / m_fGrowTime;
		}

		float fFade = 1.0f;
		float fTimeLeft = pkInfo->m_fLifeSpan - pkInfo->m_fAge;
		if (pkInfo->m_usGeneration == m_usFadeGeneration && m_fFadeTime > fTimeLeft && m_fFadeTime != 0.0f) {
			fFade = fTimeLeft / m_fFadeTime;
		}

		float fScale = fGrow < fFade ? fGrow : fFade;
		fScale = fScale * fScaleRange + m_fBaseScale;
		if (fScale < 0.0001f) {
			fScale = 0.0001f;
		}
		pkData->m_pfSizes[i] = fScale;
	}
}

NiObject* NiPSysGrowFadeModifier::CreateObject() {
	NiPSysGrowFadeModifier* pkObject =
		static_cast<NiPSysGrowFadeModifier*>(NiMemObject::operator new(sizeof(NiPSysGrowFadeModifier)));
	if (!pkObject) {
		return 0;
	}
	return ::new (pkObject) NiPSysGrowFadeModifier;
}
