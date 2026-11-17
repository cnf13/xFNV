#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysSpawnModifier.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGeometryData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiParticleInfo.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysData.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiParticleSystem.h"

// This file's own copy of 2π (stored data, never written); SpawnParticle reads it, not NI_TWO_PI.
static float s_fTwoPi = 6.2831855f;

NiRTTI NiPSysSpawnModifier::ms_RTTI("NiPSysSpawnModifier", &NiPSysModifier::ms_RTTI);

// The matrix-point product the exe inlines here: each row is (e0 * x + e1 * y) + e2 * z, rounded to float.
static NiPoint3 RotatePoint(const NiMatrix3& arMatrix, const NiPoint3& arPoint) {
	NiPoint3 kResult;
	kResult.x =
		arMatrix.m_pEntry[0][0] * arPoint.x + arMatrix.m_pEntry[0][1] * arPoint.y + arMatrix.m_pEntry[0][2] * arPoint.z;
	kResult.y =
		arMatrix.m_pEntry[1][0] * arPoint.x + arMatrix.m_pEntry[1][1] * arPoint.y + arMatrix.m_pEntry[1][2] * arPoint.z;
	kResult.z =
		arMatrix.m_pEntry[2][0] * arPoint.x + arMatrix.m_pEntry[2][1] * arPoint.y + arMatrix.m_pEntry[2][2] * arPoint.z;
	return kResult;
}

NiPSysSpawnModifier::NiPSysSpawnModifier()
	: NiPSysModifier(), m_usNumSpawnGenerations(1), m_fPercentageSpawned(1.0f), m_usMinNumToSpawn(1),
	  m_usMaxNumToSpawn(1), m_fSpawnSpeedChaos(0.0f), m_fSpawnDirChaos(0.0f), m_fLifeSpan(0.0f), m_fLifeSpanVar(0.0f) {}

NiPSysSpawnModifier::~NiPSysSpawnModifier() {}

NiObject* NiPSysSpawnModifier::CreateClone(NiCloningProcess& kCloning) {
	NiPSysSpawnModifier* pkObject =
		static_cast<NiPSysSpawnModifier*>(NiMemObject::operator new(sizeof(NiPSysSpawnModifier)));
	NiPSysSpawnModifier* pkObject_2 = pkObject;
	if (pkObject) {
		::new (pkObject) NiPSysSpawnModifier;
	} else {
		pkObject_2 = 0;
	}
	CopyMembers(pkObject_2, kCloning);
	pkObject_2->m_usNumSpawnGenerations = m_usNumSpawnGenerations;
	pkObject_2->m_fPercentageSpawned = m_fPercentageSpawned;
	pkObject_2->m_usMinNumToSpawn = m_usMinNumToSpawn;
	pkObject_2->m_usMaxNumToSpawn = m_usMaxNumToSpawn;
	pkObject_2->m_fSpawnSpeedChaos = m_fSpawnSpeedChaos;
	pkObject_2->m_fSpawnDirChaos = m_fSpawnDirChaos;
	pkObject_2->m_fLifeSpan = m_fLifeSpan;
	pkObject_2->m_fLifeSpanVar = m_fLifeSpanVar;
	return pkObject_2;
}

void NiPSysSpawnModifier::LoadBinary(NiStream& kStream) {
	NiPSysModifier::LoadBinary(kStream);

	NiStreamLoadBinary(kStream, m_usNumSpawnGenerations);
	NiStreamLoadBinary(kStream, m_fPercentageSpawned);
	NiStreamLoadBinary(kStream, m_usMinNumToSpawn);
	NiStreamLoadBinary(kStream, m_usMaxNumToSpawn);
	NiStreamLoadBinary(kStream, m_fSpawnSpeedChaos);
	NiStreamLoadBinary(kStream, m_fSpawnDirChaos);
	NiStreamLoadBinary(kStream, m_fLifeSpan);
	NiStreamLoadBinary(kStream, m_fLifeSpanVar);
}

void NiPSysSpawnModifier::LinkObject(NiStream& kStream) {
	NiPSysModifier::LinkObject(kStream);
}

bool NiPSysSpawnModifier::RegisterStreamables(NiStream& kStream) {
	return NiPSysModifier::RegisterStreamables(kStream);
}

void NiPSysSpawnModifier::SaveBinary(NiStream& kStream) {
	NiPSysModifier::SaveBinary(kStream);

	NiStreamSaveBinary(kStream, m_usNumSpawnGenerations);
	NiStreamSaveBinary(kStream, m_fPercentageSpawned);
	NiStreamSaveBinary(kStream, m_usMinNumToSpawn);
	NiStreamSaveBinary(kStream, m_usMaxNumToSpawn);
	NiStreamSaveBinary(kStream, m_fSpawnSpeedChaos);
	NiStreamSaveBinary(kStream, m_fSpawnDirChaos);
	NiStreamSaveBinary(kStream, m_fLifeSpan);
	NiStreamSaveBinary(kStream, m_fLifeSpanVar);
}

bool NiPSysSpawnModifier::IsEqual(NiObject* pkObject) {
	if (!NiPSysModifier::IsEqual(pkObject)) {
		return false;
	}

	NiPSysSpawnModifier* pkOther = static_cast<NiPSysSpawnModifier*>(pkObject);
	if (pkOther->m_usNumSpawnGenerations != m_usNumSpawnGenerations ||
		pkOther->m_fPercentageSpawned != m_fPercentageSpawned || pkOther->m_usMinNumToSpawn != m_usMinNumToSpawn ||
		pkOther->m_usMaxNumToSpawn != m_usMaxNumToSpawn || pkOther->m_fSpawnSpeedChaos != m_fSpawnSpeedChaos ||
		pkOther->m_fSpawnDirChaos != m_fSpawnDirChaos || pkOther->m_fLifeSpan != m_fLifeSpan ||
		pkOther->m_fLifeSpanVar != m_fLifeSpanVar) {
		return false;
	}
	return true;
}

void NiPSysSpawnModifier::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiPSysModifier::GetViewerStrings(pkStrings);

	pkStrings->Add(NiGetViewerString(NiPSysSpawnModifier::ms_RTTI.GetName()));
	pkStrings->Add(NiGetViewerString("Num Spawn Generations", m_usNumSpawnGenerations));
	pkStrings->Add(NiGetViewerString("Percentage Spawned", m_fPercentageSpawned));
	pkStrings->Add(NiGetViewerString("Min Num to Spawn", m_usMinNumToSpawn));
	pkStrings->Add(NiGetViewerString("Max Num to Spawn", m_usMaxNumToSpawn));
	pkStrings->Add(NiGetViewerString("Spawn Speed Chaos", m_fSpawnSpeedChaos));
	pkStrings->Add(NiGetViewerString("Spawn Dir Chaos", m_fSpawnDirChaos));
	pkStrings->Add(NiGetViewerString("Life Span", m_fLifeSpan));
	pkStrings->Add(NiGetViewerString("Life Span Variation", m_fLifeSpanVar));
}

// Particles past the last generation never spawn. Otherwise, with probability m_fPercentageSpawned, a count between
// the min and max (rounded to nearest, at least one) is spawned. SpawnParticle gets the caller's whole dword
// (0xC23EB3 `push ecx`).
void NiPSysSpawnModifier::SpawnParticles(float fCurrentTime, float fSpawnTime, int32_t usOldIndex,
	NiParticleSystem* pkOldPSystem) {
	NiPSysData* pkOldData = static_cast<NiPSysData*>(pkOldPSystem->m_spModelData.m_pObject);
	if (pkOldData->m_pkParticleInfo[static_cast<uint16_t>(usOldIndex)].m_usGeneration >= m_usNumSpawnGenerations) {
		return;
	}

	float fRandom = NiUnitRandom();
	if (fRandom > m_fPercentageSpawned) {
		return;
	}

	float fNumToSpawn = static_cast<int32_t>(m_usMaxNumToSpawn - m_usMinNumToSpawn) * NiUnitRandom();
	uint16_t usNumToSpawn = static_cast<uint16_t>(static_cast<int32_t>(fNumToSpawn));
	// EXE-INLINED none: NiFmod (the exe's __CIfmod on the float and fld1)
	float fFraction = static_cast<float>(std::fmod(fNumToSpawn, 1.0));
	if (fFraction > 0.5f) {
		usNumToSpawn++;
	}
	usNumToSpawn = m_usMinNumToSpawn + usNumToSpawn;
	if (usNumToSpawn == 0) {
		usNumToSpawn = 1;
	}

	for (uint16_t i = 0; i < usNumToSpawn; i++) {
		SpawnParticle(fCurrentTime, fSpawnTime, usOldIndex, pkOldPSystem);
	}
}

// The new particle starts at the old one's position, both brought into the target's space. Its velocity is a
// random direction within m_fSpawnDirChaos half-turns of the old velocity, scaled by up to 1 + m_fSpawnSpeedChaos.
// Colors, radii, sizes and rotations are copied when both data objects have them.
void NiPSysSpawnModifier::SpawnParticle(float fCurrentTime, float fSpawnTime, int32_t aiOldIndex,
	NiParticleSystem* pkOldPSystem) {
	uint16_t usOldIndex = static_cast<uint16_t>(aiOldIndex);
	NiPSysData* pkOldData = static_cast<NiPSysData*>(pkOldPSystem->m_spModelData.m_pObject);
	NiColorA* pkOldColors = pkOldData->m_pkColor;
	float* pfOldRadii = pkOldData->m_pfRadii;
	float* pfOldSizes = pkOldData->m_pfSizes;
	float* pfOldRotationAngles = pkOldData->m_pfRotationAngles;
	NiPoint3* pkOldRotationAxes = pkOldData->m_pkRotationAxes;
	float* pfOldRotationSpeeds = pkOldData->m_pfRotationSpeeds;
	NiParticleInfo* pkOldInfo = &pkOldData->m_pkParticleInfo[usOldIndex];
	NiPoint3 kPosition = pkOldData->m_pkVertex[usOldIndex];
	NiPoint3 kVelocity = pkOldInfo->m_kVelocity;

	NiParticleSystem* pkTarget = m_pkTarget;
	if (pkOldPSystem != pkTarget) {
		NiTransform kTransform = pkOldPSystem->m_kWorld;
		kPosition = kTransform * kPosition;
		kVelocity = RotatePoint(kTransform.m_Rotate, kVelocity);

		NiTransform kTargetWorld = pkTarget->m_kWorld;
		kTargetWorld.Invert(kTransform);
		kPosition = kTransform * kPosition;
		kVelocity = RotatePoint(kTransform.m_Rotate, kVelocity);
	}

	NiPSysData* pkData = static_cast<NiPSysData*>(m_pkTarget->m_spModelData.m_pObject);
	uint16_t usNewParticle = pkData->AddParticle();
	if (usNewParticle == NiPSysData::INVALID_PARTICLE) {
		return;
	}

	float fAge = fCurrentTime - fSpawnTime;
	NiParticleInfo* pkNewInfo = &pkData->m_pkParticleInfo[usNewParticle];
	NiPoint3* pkVertices = pkData->m_pkVertex;
	NiColorA* pkColors = pkData->m_pkColor;
	float* pfRadii = pkData->m_pfRadii;
	float* pfSizes = pkData->m_pfSizes;
	float* pfRotationAngles = pkData->m_pfRotationAngles;
	NiPoint3* pkRotationAxes = pkData->m_pkRotationAxes;
	float* pfRotationSpeeds = pkData->m_pfRotationSpeeds;

	double dLengthSqrXY = kVelocity.x * kVelocity.x + kVelocity.y * kVelocity.y;
	float fLengthSqr = static_cast<float>(dLengthSqrXY + kVelocity.z * kVelocity.z);
	// The exe calls __CIsqrt (0xEC6040) out of line on the float reloaded from the stack.
	float fSpeed = std::sqrt(fLengthSqr);
	float fSpeedChaos = m_fSpawnSpeedChaos * NiUnitRandom();
	float fNewSpeed = (fSpeedChaos + 1.0) * fSpeed;

	// A random direction around +z, looked up in the 512-step tables.
	float fDeclination = m_fSpawnDirChaos * NiUnitRandom() * 3.1415927f;
	float fPlanarAngle = s_fTwoPi * NiUnitRandom();
	double dTableScale = 512.0 / s_fTwoPi;
	int32_t iDeclination = static_cast<int32_t>(fDeclination * dTableScale) & 0x1FF;
	float fSinDeclination = s_fSinTableA[iDeclination];
	int32_t iPlanarAngle = static_cast<int32_t>(dTableScale * fPlanarAngle) & 0x1FF;
	NiPoint3 kDirection;
	kDirection.x = fSinDeclination * s_fCosTableA[iPlanarAngle];
	kDirection.y = fSinDeclination * s_fSinTableA[iPlanarAngle];
	kDirection.z = s_fCosTableA[iDeclination];

	// The rotation taking +z onto the velocity (Rodrigues' formula on the skew matrix of the velocity's xy part).
	// The branch is chosen by the sign of (direction x velocity) . (vy, -vx, 0); near zero the velocity is
	// along z and the rotation is the identity or, for -z, the negated identity.
	NiMatrix3 kRotation = NiMatrix3_IDENTITY;
	NiMatrix3 kSkew;
	kSkew.m_pEntry[0][0] = 0.0f;
	kSkew.m_pEntry[0][1] = 0.0f;
	kSkew.m_pEntry[0][2] = -kVelocity.x;
	kSkew.m_pEntry[1][0] = 0.0f;
	kSkew.m_pEntry[1][1] = 0.0f;
	kSkew.m_pEntry[1][2] = -kVelocity.y;
	kSkew.m_pEntry[2][0] = kVelocity.x;
	kSkew.m_pEntry[2][1] = kVelocity.y;
	kSkew.m_pEntry[2][2] = 0.0f;

	NiPoint3 kCross;
	kCross.x = kDirection.y * kVelocity.z - kDirection.z * kVelocity.y;
	kCross.y = kDirection.z * kVelocity.x - kDirection.x * kVelocity.z;
	kCross.z = kDirection.x * kVelocity.y - kDirection.y * kVelocity.x;
	float fDot = kCross.x * kVelocity.y + kCross.y * kSkew.m_pEntry[0][2] + kCross.z * 0.0;

	if (fDot > 1e-4f) {
		float fInvSpeed = 1.0f / fSpeed;
		float fScale = (fSpeed - kVelocity.z) / dLengthSqrXY;
		kRotation = kRotation + (kSkew * (kRotation + kSkew * fScale)) * fInvSpeed;
	} else if (-fDot > 1e-4f) {
		float fInvSpeed = 1.0f / fSpeed;
		float fScale = (fSpeed - kVelocity.z) / dLengthSqrXY;
		kRotation = kRotation - (kSkew * (kRotation - kSkew * fScale)) * fInvSpeed;
	} else if (kVelocity.z < 0.0f) {
		// The exe builds the three columns inline (no NiPoint3 constructor call) and passes them to the out-of-line
		// three-column NiMatrix3 constructor (0x476980).
		NiPoint3 kCol0;
		kCol0.x = -1.0f;
		kCol0.y = 0.0f;
		kCol0.z = 0.0f;
		NiPoint3 kCol1;
		kCol1.x = 0.0f;
		kCol1.y = -1.0f;
		kCol1.z = 0.0f;
		NiPoint3 kCol2;
		kCol2.x = 0.0f;
		kCol2.y = 0.0f;
		kCol2.z = -1.0f;
		kRotation = NiMatrix3(kCol0, kCol1, kCol2);
	}

	NiMatrix3 kScaled;
	for (int32_t iRow = 0; iRow < 3; iRow++) {
		for (int32_t iCol = 0; iCol < 3; iCol++) {
			kScaled.m_pEntry[iRow][iCol] = kRotation.m_pEntry[iRow][iCol] * fNewSpeed;
		}
	}
	pkNewInfo->m_kVelocity = RotatePoint(kScaled, kDirection);

	pkNewInfo->m_fAge = fAge;
	pkNewInfo->m_fLifeSpan = (NiUnitRandom() - 0.5) * m_fLifeSpanVar + m_fLifeSpan;
	pkNewInfo->m_usGeneration = pkOldInfo->m_usGeneration + 1;
	pkVertices[usNewParticle] = kPosition;

	if (pkOldColors && pkColors) {
		pkColors[usNewParticle] = pkOldColors[usOldIndex];
	}
	if (pfOldRadii && pfRadii) {
		pfRadii[usNewParticle] = pfOldRadii[usOldIndex];
	}
	if (pfOldSizes && pfSizes) {
		pfSizes[usNewParticle] = pfOldSizes[usOldIndex];
	}
	// The speeds go with the angles, unchecked.
	if (pfOldRotationAngles && pfRotationAngles) {
		pfRotationAngles[usNewParticle] = pfOldRotationAngles[usOldIndex];
		pfRotationSpeeds[usNewParticle] = pfOldRotationSpeeds[usOldIndex];
	}
	if (pkOldRotationAxes && pkRotationAxes) {
		pkRotationAxes[usNewParticle] = pkOldRotationAxes[usOldIndex];
	}

	pkNewInfo->m_fLastUpdate = fCurrentTime - pkNewInfo->m_fAge;
	m_pkTarget->InitializeNewParticle(usNewParticle);
}

void NiPSysSpawnModifier::SetLifeSpan(float fLifeSpan) {
	m_fLifeSpan = fLifeSpan;
}

NiObject* NiPSysSpawnModifier::CreateObject() {
	NiPSysSpawnModifier* pkObject =
		static_cast<NiPSysSpawnModifier*>(NiMemObject::operator new(sizeof(NiPSysSpawnModifier)));
	if (!pkObject) {
		return 0;
	}
	return ::new (pkObject) NiPSysSpawnModifier;
}
