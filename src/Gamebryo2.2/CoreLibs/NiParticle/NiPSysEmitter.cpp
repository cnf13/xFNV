#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitter.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleInfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

NiRTTI NiPSysEmitter::ms_RTTI("NiPSysEmitter", &NiPSysModifier::ms_RTTI);

NiPSysEmitter::NiPSysEmitter()
	: m_fSpeed(0.0f), m_fSpeedVar(0.0f), m_fDeclination(0.0f), m_fDeclinationVar(0.0f), m_fPlanarAngle(0.0f),
	  m_fPlanarAngleVar(0.0f), m_kInitialColor(NiColorA_WHITE), m_fInitialRadius(1.0f), m_fRadiusVar(0.0f),
	  m_fLifeSpan(0.0f), m_fLifeSpanVar(0.0f), m_fScale(1.0f) {}

NiPSysEmitter::NiPSysEmitter(const char* apcName, uint32_t auiOrder, float afSpeed, float afSpeedVar,
	float afDeclination, float afDeclinationVar, float afPlanarAngle, float afPlanarAngleVar, NiColorA akInitialColor,
	float afInitialRadius, float afLifeSpan, float afLifeSpanVar, float afRadiusVar)
	: NiPSysModifier(NiFixedString(apcName), auiOrder), m_fSpeed(afSpeed), m_fSpeedVar(afSpeedVar),
	  m_fDeclination(afDeclination), m_fDeclinationVar(afDeclinationVar), m_fPlanarAngle(afPlanarAngle),
	  m_fPlanarAngleVar(afPlanarAngleVar), m_kInitialColor(akInitialColor), m_fInitialRadius(afInitialRadius),
	  m_fRadiusVar(afRadiusVar), m_fLifeSpan(afLifeSpan), m_fLifeSpanVar(afLifeSpanVar), m_fScale(1.0f) {}

// Older files have no radius variation. The scale is not streamed.
void NiPSysEmitter::LoadBinary(NiStream& kStream) {
	NiPSysModifier::LoadBinary(kStream);

	NiStreamLoadBinary(kStream, m_fSpeed);
	NiStreamLoadBinary(kStream, m_fSpeedVar);
	NiStreamLoadBinary(kStream, m_fDeclination);
	NiStreamLoadBinary(kStream, m_fDeclinationVar);
	NiStreamLoadBinary(kStream, m_fPlanarAngle);
	NiStreamLoadBinary(kStream, m_fPlanarAngleVar);
	m_kInitialColor.LoadBinary(kStream);
	NiStreamLoadBinary(kStream, m_fInitialRadius);
	if (kStream.GetFileVersion() >= NiStream::GetVersion(10, 3, 0, 2)) {
		NiStreamLoadBinary(kStream, m_fRadiusVar);
	}
	NiStreamLoadBinary(kStream, m_fLifeSpan);
	NiStreamLoadBinary(kStream, m_fLifeSpanVar);
}

// The exe's slot is a bare jmp to NiPSysModifier::LinkObject.
void NiPSysEmitter::LinkObject(NiStream& kStream) {
	NiPSysModifier::LinkObject(kStream);
}

// The exe's slot is a bare jmp to NiPSysModifier::RegisterStreamables.
bool NiPSysEmitter::RegisterStreamables(NiStream& kStream) {
	return NiPSysModifier::RegisterStreamables(kStream);
}

void NiPSysEmitter::SaveBinary(NiStream& kStream) {
	NiPSysModifier::SaveBinary(kStream);

	NiStreamSaveBinary(kStream, m_fSpeed);
	NiStreamSaveBinary(kStream, m_fSpeedVar);
	NiStreamSaveBinary(kStream, m_fDeclination);
	NiStreamSaveBinary(kStream, m_fDeclinationVar);
	NiStreamSaveBinary(kStream, m_fPlanarAngle);
	NiStreamSaveBinary(kStream, m_fPlanarAngleVar);
	m_kInitialColor.SaveBinary(kStream);
	NiStreamSaveBinary(kStream, m_fInitialRadius);
	NiStreamSaveBinary(kStream, m_fRadiusVar);
	NiStreamSaveBinary(kStream, m_fLifeSpan);
	NiStreamSaveBinary(kStream, m_fLifeSpanVar);
}

// The scale is not compared.
bool NiPSysEmitter::IsEqual(NiObject* pkObject) {
	if (!NiPSysModifier::IsEqual(pkObject)) {
		return false;
	}

	NiPSysEmitter* pkOther = static_cast<NiPSysEmitter*>(pkObject);
	if (m_fSpeed != pkOther->m_fSpeed || m_fSpeedVar != pkOther->m_fSpeedVar ||
		m_fDeclination != pkOther->m_fDeclination || m_fDeclinationVar != pkOther->m_fDeclinationVar ||
		m_fPlanarAngle != pkOther->m_fPlanarAngle || m_fPlanarAngleVar != pkOther->m_fPlanarAngleVar ||
		pkOther->m_kInitialColor != m_kInitialColor || m_fInitialRadius != pkOther->m_fInitialRadius ||
		m_fRadiusVar != pkOther->m_fRadiusVar || m_fLifeSpan != pkOther->m_fLifeSpan ||
		m_fLifeSpanVar != pkOther->m_fLifeSpanVar) {
		return false;
	}
	return true;
}

void NiPSysEmitter::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiPSysModifier::GetViewerStrings(pkStrings);

	pkStrings->Add(NiGetViewerString(NiPSysEmitter::ms_RTTI.GetName()));
	pkStrings->Add(NiGetViewerString("Speed", m_fSpeed));
	pkStrings->Add(NiGetViewerString("Speed Variation", m_fSpeedVar));
	pkStrings->Add(NiGetViewerString("Declination", m_fDeclination));
	pkStrings->Add(NiGetViewerString("Declination Variation", m_fDeclinationVar));
	pkStrings->Add(NiGetViewerString("Planar Angle", m_fPlanarAngle));
	pkStrings->Add(NiGetViewerString("Planar Angle Variation", m_fPlanarAngleVar));
	pkStrings->Add(m_kInitialColor.GetViewerString("Initial Color"));
	pkStrings->Add(NiGetViewerString("Initial Radius", m_fInitialRadius));
	pkStrings->Add(NiGetViewerString("Radius Variation", m_fRadiusVar));
	pkStrings->Add(NiGetViewerString("Life Span", m_fLifeSpan));
	pkStrings->Add(NiGetViewerString("Life Span Variation", m_fLifeSpanVar));
}

// Each requested particle gets a random life span; one whose age already exceeds it is not emitted. The rest get
// a direction from the declination (angle from +z) and planar angle (around z) through the 512-step sine and
// cosine tables, a random speed along it, then the subclass places it and may adjust the velocity. Emission
// stops when the data has no free particle.
void NiPSysEmitter::EmitParticles(float fTime, uint16_t usNumParticles, const float* pfAges) {
	NiPSysData* pkData = static_cast<NiPSysData*>(m_pkTarget->m_spModelData.m_pObject);
	NiPoint3* pkVertex = pkData->m_pkVertex;
	NiColorA* pkColor = pkData->m_pkColor;
	float* pfRadii = pkData->m_pfRadii;
	float* pfSizes = pkData->m_pfSizes;
	uint8_t* pucTextureIndices = pkData->m_pucTextureIndices;
	uint8_t ucSubTextureOffsetCount = pkData->m_ucSubTextureOffsetCount;

	for (uint16_t i = 0; i < usNumParticles; i++) {
		float fAge = pfAges[i];
		float fLifeSpan = (NiUnitRandom() - 0.5) * m_fLifeSpanVar + m_fLifeSpan;
		if (fLifeSpan < fAge) {
			continue;
		}

		uint16_t usNewParticle = pkData->AddParticle();
		if (usNewParticle == NiPSysData::INVALID_PARTICLE) {
			return;
		}

		NiParticleInfo* pkInfo = &pkData->m_pkParticleInfo[usNewParticle];

		float fSpeed = (NiUnitRandom() - 0.5) * m_fSpeedVar + m_fSpeed;
		float fDeclination = m_fDeclinationVar * NiSymmetricRandom() + m_fDeclination;
		float fPlanarAngle = m_fPlanarAngleVar * NiSymmetricRandom() + m_fPlanarAngle;

		double dTableScale = 512.0 / NI_TWO_PI;
		int32_t iDeclination = static_cast<int32_t>(fDeclination * dTableScale) & 0x1FF;
		float fSinDeclination = s_fSinTableA[iDeclination];
		int32_t iPlanarAngle = static_cast<int32_t>(dTableScale * fPlanarAngle) & 0x1FF;
		// The exe builds the direction and scales it by the speed inline (no NiPoint3 constructor or operator* calls).
		NiPoint3 kDirection;
		kDirection.x = fSinDeclination * s_fCosTableA[iPlanarAngle];
		kDirection.y = fSinDeclination * s_fSinTableA[iPlanarAngle];
		kDirection.z = s_fCosTableA[iDeclination];
		NiPoint3 kVelocity;
		kVelocity.x = kDirection.x * fSpeed;
		kVelocity.y = kDirection.y * fSpeed;
		kVelocity.z = kDirection.z * fSpeed;
		pkInfo->m_kVelocity = kVelocity;
		pkInfo->m_fAge = fAge;
		pkInfo->m_usGeneration = 0;
		pkInfo->m_fLifeSpan = fLifeSpan;

		ComputeInitialPositionAndVelocity(pkVertex[usNewParticle], pkInfo->m_kVelocity);

		if (pkColor) {
			pkColor[usNewParticle] = m_kInitialColor;
		}
		if (pfRadii) {
			pfRadii[usNewParticle] = (m_fRadiusVar * NiSymmetricRandom() + m_fInitialRadius) * m_fScale;
		}
		if (pfSizes) {
			pfSizes[usNewParticle] = 1.0f;
		}
		if (pucTextureIndices) {
			pucTextureIndices[usNewParticle] = static_cast<uint8_t>(rt::rand() % ucSubTextureOffsetCount);
		}

		pkInfo->m_fLastUpdate = fTime - pkInfo->m_fAge;
		m_pkTarget->InitializeNewParticle(usNewParticle);
	}
}

void NiPSysEmitter::SetSpeed(float fSpeed) {
	m_fSpeed = fSpeed;
}

void NiPSysEmitter::SetInitialRadius(float fInitialRadius) {
	m_fInitialRadius = fInitialRadius;
}

void NiPSysEmitter::SetLifeSpan(float fLifeSpan) {
	m_fLifeSpan = fLifeSpan;
}

void NiPSysEmitter::CopyMembers(NiPSysEmitter* pkDest, NiCloningProcess& kCloning) {
	NiPSysModifier::CopyMembers(pkDest, kCloning);
	pkDest->m_fSpeed = m_fSpeed;
	pkDest->m_fSpeedVar = m_fSpeedVar;
	pkDest->m_fDeclination = m_fDeclination;
	pkDest->m_fDeclinationVar = m_fDeclinationVar;
	pkDest->m_fPlanarAngle = m_fPlanarAngle;
	pkDest->m_fPlanarAngleVar = m_fPlanarAngleVar;
	pkDest->m_kInitialColor = m_kInitialColor;
	pkDest->m_fInitialRadius = m_fInitialRadius;
	pkDest->m_fLifeSpan = m_fLifeSpan;
	pkDest->m_fLifeSpanVar = m_fLifeSpanVar;
	pkDest->m_fRadiusVar = m_fRadiusVar;
}
