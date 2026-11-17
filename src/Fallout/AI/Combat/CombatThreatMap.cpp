#include "Fallout/AI/Combat/CombatThreatMap.h"

#include "BSCore/BSReadWriteLock.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkRefObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/BGSExplosion.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/TESCombatStyle.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/CombatTimeStamp.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "SDK/MSVC/rtti.h"

// The game settings this file defines (PDB), with the IDs and values the exe's initializers pass, in its initialization order.
SettingT<INISettingCollection> bDebugCombatThreats("bDebugCombatThreats:Combat", false);

// The exe's cleanup-only C++ EH frame is left out.
CombatThreat::CombatThreat()
	: pThreat(nullptr), fRadius(0.0f), fStartTime(0.0f), fEndTime(0.0f), fDamage(0.0f), spDebugGeometry(nullptr) {}

// The exe's cleanup-only C++ EH frame is left out.
CombatThreat::CombatThreat(TESObjectREFR* apThreat, float afRadius, float afStartTime, float afEndTime, float afDamage)
	: spDebugGeometry(nullptr) {
	Initialize(apThreat, afRadius, afStartTime, afEndTime, afDamage, false);
	pCachedThreat3D = nullptr;
	pCachedRigidBody = nullptr;
	CachedLinearVelocity = NiPoint3_ZERO;
	fCachedLinearVelocityTimeStamp.fTimeStamp = -3.4028235e+38f;
}

// The exe's cleanup-only C++ EH frame is left out.
CombatThreat::~CombatThreat() {
	if (spDebugGeometry.m_pObject && spDebugGeometry.m_pObject->GetParent()) {
		spDebugGeometry.m_pObject->GetParent()->DetachChild(spDebugGeometry.m_pObject);
	}
}

float CombatThreat::ComputeThreatScore(const NiPoint3& arLocation, Actor* apActor, float afCompareTime,
	float afAdditionalRadius, float afActorHealth) {
	NiPoint3 v19;
	NiPoint3 linearVelocity;
	float fthreatScore = 0.0;
	if (afCompareTime < 0.0 || IsActive(afCompareTime)) {
		float fmodifiedRadius = fRadius + afAdditionalRadius;
		float fsquaredDistanceFromThreat = 0.0f;
		float fextrudeRadius = fmodifiedRadius * gs_fCombatThreatExtrudeVelocityThreshold.Float();
		GetThreatLinearVelocity(&linearVelocity);
		float fvelocityMagnitude = linearVelocity.UnitizeGetLength();
		NiPoint3* a2 = &(v19 = linearVelocity * fvelocityMagnitude * 0.5f);
		const NiPoint3* v7 = &pThreat->GetLocationOnReference();
		NiPoint3 threatLocation = *v7 + *a2;
		if (!(fextrudeRadius > static_cast<double>(fvelocityMagnitude))) {
			float fnegativeExtrudeTime = 0.0;
			fnegativeExtrudeTime = BSMAX(static_cast<float>(0.0), 0.0);
			float v16 = fvelocityMagnitude * fnegativeExtrudeTime - fmodifiedRadius;
			NiPoint3 basePoint = threatLocation + linearVelocity * v16;
			double v15 = fmodifiedRadius * 2.0;
			float* v9 = &fCombatThreatExtrudeTime.Float();
			float fheight = (*v9 - fnegativeExtrudeTime) * fvelocityMagnitude + v15;
			float fheightSquared = fheight * fheight;
			NiPoint3 deltaVector = fheight * linearVelocity;
			NiPoint3 testVector = arLocation - basePoint;
			float fdot = testVector.Dot(deltaVector);
			if (!(fdot < 0.0) && !(fheightSquared < static_cast<double>(fdot))) {
				double v10 = testVector.SqrLength();
				fsquaredDistanceFromThreat = v10 - fdot * fdot / fheightSquared;
			} else {
				fsquaredDistanceFromThreat = 3.4028235e+38f;
			}
		} else {
			NiPoint3 threatVector = threatLocation - arLocation;
			fsquaredDistanceFromThreat = threatVector.SqrLength();
		}
		if (fsquaredDistanceFromThreat < fmodifiedRadius * fmodifiedRadius) {
			double BaseThreatScore = GetBaseThreatScore(apActor, afActorHealth);
			return BaseThreatScore * (1.0 - fsquaredDistanceFromThreat / (fmodifiedRadius * fmodifiedRadius));
		}
	}
	return fthreatScore;
}

CombatTimeStamp* CombatThreat::GetStartTime(CombatTimeStamp* apOut) {
	// The exe copies the time stamp's dword with integer moves.
	*apOut = fStartTime;
	return apOut;
}

void CombatThreat::Initialize(TESObjectREFR* apThreat, float afRadius, float afStartTime, float afEndTime,
	float afDamage, bool abUpdatingThreat) {
	float a2;
	CombatTimeStamp kEndStamp;
	CombatTimeStamp kStartStamp;
	pThreat = apThreat;
	fRadius = afRadius;
	if (((uint8_t)abUpdatingThreat) == 0) {
		a2 = CombatTimer::GetTimer() + afStartTime;
		kStartStamp.~CombatTimeStamp();
		::new (&kStartStamp) CombatTimeStamp(a2);
		fStartTime = kStartStamp;
	}
	float v7 = CombatTimer::GetTimer() + afEndTime;
	kEndStamp.~CombatTimeStamp();
	::new (&kEndStamp) CombatTimeStamp(v7);
	fEndTime = kEndStamp;
	fDamage = afDamage;
}

// test ah, 1 / jne on the start and test ah, 0x41 / jp on the end: unordered compares are inactive.
bool CombatThreat::IsActive(float afCompareTime) {
	return afCompareTime >= fStartTime && afCompareTime <= fEndTime;
}

// test ah, 0x41 / jne: unordered is not expired.
bool CombatThreat::IsExpired(float afCompareTime) {
	return afCompareTime > fEndTime;
}

void CombatThreat::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fRadius), 4, 0);
	fStartTime.SaveGame(apSaveGameBuffer);
	fEndTime.SaveGame(apSaveGameBuffer);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fDamage), 4, 0);
}

float CombatThreat::GetBaseThreatScore(Actor* apActor, float afActorHealth) {
	float fcurrentHealth;
	if (apActor) {
		fcurrentHealth = static_cast<float>(apActor->GetActorValue(ActorValue::HEALTH));
	} else {
		fcurrentHealth = afActorHealth;
	}
	return fDamage * 2.0 / fcurrentHealth;
}

NiPoint3* CombatThreat::GetThreatLinearVelocity(NiPoint3* apResult) {
	CombatTimer v8;
	if (fCachedLinearVelocityTimeStamp.ElapsedTime() < 0.33000001311302185) {
		*apResult = CachedLinearVelocity;
		return apResult;
	}
	NiAVObject* pthreat3D = pThreat->Get3D();
	if (pthreat3D) {
		Projectile* pprojectile;
		bhkCharacterController* pcharacterController;
		if (!pThreat->IsProjectile()) {
			goto LABEL_9;
		}
		pprojectile = static_cast<Projectile*>(pThreat);
		pcharacterController = pprojectile->GetCharController();
		if (pcharacterController) {
			pcharacterController->GetLinearVelocity(CachedLinearVelocity);
			float a2 = CombatTimer::GetTimer();
			FNVD_CONSTRUCTED_LOCAL(CombatTimeStamp, apMemory, nullptr, a2);
			fCachedLinearVelocityTimeStamp = apMemory;
			*apResult = CachedLinearVelocity;
			return apResult;
		}
		if (pprojectile->IsFlagSet(Projectile::NO_COLLISION)) {
			CachedLinearVelocity = NiPoint3_ZERO;
			float a2a = CombatTimer::GetTimer();
			FNVD_CONSTRUCTED_LOCAL(CombatTimeStamp, v9, nullptr, a2a);
			fCachedLinearVelocityTimeStamp = v9;
			*apResult = CachedLinearVelocity;
			return apResult;
		} else {
		LABEL_9:;
			if (pCachedThreat3D != pthreat3D) {
				pCachedThreat3D = pthreat3D;
				bhkCollisionObject* pcollisionObject = FindFirstCollisionObject(pCachedThreat3D);
				if (pcollisionObject) {
					NiObject* pworldObject = pcollisionObject->GetRigidBody();
					if (pworldObject) {
						pCachedRigidBody =
							static_cast<bhkRigidBody*>(NiObject::DynamicCastStatic(&bhkRigidBody::ms_RTTI,
								pworldObject));
					}
				}
			}
			if (pCachedRigidBody) {
				pCachedRigidBody->GetLinearVelocityNi(&CachedLinearVelocity);
			} else {
				CachedLinearVelocity = NiPoint3_ZERO;
			}
			float a2b = CombatTimer::GetTimer();
			::new (&v8.fTargetTime) CombatTimeStamp(a2b);
			fCachedLinearVelocityTimeStamp.fTimeStamp = v8.fTargetTime;
			*apResult = CachedLinearVelocity;
			return apResult;
		}
	} else {
		float a2c = CombatTimer::GetTimer();
		v8.SetStartTime(a2c);
		fCachedLinearVelocityTimeStamp.fTimeStamp = v8.fStartTime;
		*apResult = NiPoint3_ZERO;
		return apResult;
	}
}

PathingAvoidNode* CombatThreat::GetAvoidNode(PathingAvoidNode* apResult, Actor* apActor) {
	NiPoint3 v19;
	NiPoint3 linearVelocity;
	GetThreatLinearVelocity(&linearVelocity);
	float fextrudeRadius = fRadius * gs_fCombatThreatExtrudeVelocityThreshold.Float();
	float fthreatScore = GetBaseThreatScore(apActor, 0.0f);
	float fvelocityMagnitude = linearVelocity.UnitizeGetLength();
	NiPoint3* a2 = &(v19 = linearVelocity * fvelocityMagnitude * 0.5f);
	const NiPoint3* v4 = &pThreat->GetLocationOnReference();
	NiPoint3 threatLocation = *v4 + *a2;
	if (!(fextrudeRadius > static_cast<double>(fvelocityMagnitude))) {
		NiPoint3 v18;
		float fvelocityMagnitude_2 = linearVelocity.UnitizeGetLength();
		float v22 = 0.0f;
		float fnegativeExtrudeTime = BSMAX(0.0f, 0.0f);
		float v14 = fvelocityMagnitude_2 * fnegativeExtrudeTime - fRadius;
		threatLocation.operator+=((v18 = linearVelocity * v14));
		double v13 = fRadius * 2.0;
		float* v7 = &fCombatThreatExtrudeTime.Float();
		float v12 = (*v7 - fnegativeExtrudeTime) * fvelocityMagnitude_2 + v13;
		NiPoint3 threatLoc2 = threatLocation + linearVelocity * v12;
		float* v9 = &fCombatThreatAvoidCost.Float();
		float v11 = fthreatScore * *v9;
		::new (apResult) PathingAvoidNode(threatLocation, threatLoc2, fRadius, v11);
	} else {
		float* v5 = &fCombatThreatAvoidCost.Float();
		float v15 = fthreatScore * *v5;
		::new (apResult) PathingAvoidNode(threatLocation, fRadius, v15);
	}
	return apResult;
}

void CombatThreat::Load(BGSLoadGameBuffer* a1) {
	a1->LoadData(reinterpret_cast<char*>(&fRadius), 4u);
	fStartTime.LoadGame(a1);
	fEndTime.LoadGame(a1);
	a1->LoadData(reinterpret_cast<char*>(&fDamage), 4u);
}

void CombatThreat::InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
	TESObjectREFR* pRef;
	if (pThreat) {
		// The threat holds the reference's form ID until it is resolved here (CombatThreatMap::LoadGame).
		TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pThreat)));
		pRef = dynamic_cast<TESObjectREFR*>(pForm);
	} else {
		pRef = nullptr;
	}
	pThreat = pRef;
	if (pThreat) {
		pThreat->SetTargeted(1);
	}
}

// The exe's cleanup-only C++ EH frame is left out.
CombatThreatMap::CombatThreatMap() : NiTMap<uint32_t, CombatThreat*>(37u) {}

CombatThreatMap::~CombatThreatMap() {
	RemoveAll();
}

void CombatThreatMap::UpdateThreats() {
	NiTMapItem<uint32_t, CombatThreat*>* it = GetFirstPos();
	if (it) {
		Lock.LockForWrite();
		float ftime = CombatTimer::GetTimer();
		while (it) {
			uint32_t ithreatID = 0;
			CombatThreat* pthreat = 0;
			GetNext(it, ithreatID, pthreat);
			if (pthreat) {
				if (pthreat->IsExpired(ftime)) {
					if (pthreat) {
						delete pthreat;
					}
					RemoveAt(ithreatID);
				}
			}
		}
		Lock.UnlockWrite();
	}
}

void CombatThreatMap::RemoveThreat(TESForm* apForm) {
	Lock.LockForWrite();
	CombatThreat* pthreat = 0;
	if (GetAt(apForm->iFormID, pthreat)) {
		if (pthreat) {
			delete pthreat;
		}
		RemoveAt(apForm->iFormID);
	}
	Lock.UnlockWrite();
}

// The exe's cleanup-only C++ EH frame is left out.
void CombatThreatMap::AddThreat(TESObjectREFR* apRef, float afRadius, float afStartTime, float afEndTime,
	float afDamage) {
	Lock.LockForWrite();
	apRef->SetTargeted(1);
	CombatThreat* pthreat = 0;
	if (GetAt(apRef->iFormID, pthreat)) {
		pthreat->Initialize(apRef, afRadius, afStartTime, afEndTime, afDamage, 1);
	} else {
		CombatThreat* pthreat_2;
		CombatThreat* pThreat = static_cast<CombatThreat*>(ourMalloc(sizeof(CombatThreat)));
		if (pThreat) {
			pthreat_2 = ::new (pThreat) CombatThreat(apRef, afRadius, afStartTime, afEndTime, afDamage);
		} else {
			pthreat_2 = 0;
		}
		pthreat = pthreat_2;
		SetAt(apRef->iFormID, pthreat_2);
	}
	if (bDebugCombatThreats.Bool()) {
	}
	Lock.UnlockWrite();
}

void CombatThreatMap::AddThreat(TESObjectREFR* apRef, BGSExplosion* apExplosion, float afTime) {
	float* v4;
	float afExplosionRadius;
	float fRadius;
	float fDamage = (float)apExplosion->BGSExplosion::GetDamage();
	if (((&fDangerousProjectileExplosionDamage)->Float()) <= ((double)fDamage)) {
		afExplosionRadius = (float)apExplosion->GetRadiusBSUnits();
		v4 = &fDangerousProjectileExplosionDamage.Float();
		fRadius = (float)CombatFormulas::CalcExplosionDamageDistance(afExplosionRadius, fDamage, *v4);
		AddThreat(apRef, fRadius, (float)0.0, afTime, fDamage);
	}
}

float CombatThreatMap::ComputeThreatScore(NiPoint3 aLocation, Actor* apActor, float afTime, float afAdditionalRadius,
	CombatTimeStamp afSkipOlderThreats, float afActorHealth) {
	CombatTimeStamp apOut;
	Lock.LockForRead();
	float fthreatScore = 0.0;
	float fcompareTime = 0.0;
	if (!(afTime < 0.0)) {
		fcompareTime = CombatTimer::GetTimer() + afTime;
	} else {
		fcompareTime = afTime;
	}
	NiTMapItem<uint32_t, CombatThreat*>* it = GetFirstPos();
	while (it) {
		uint32_t ithreatID = 0;
		CombatThreat* pthreat = 0;
		GetNext(it, ithreatID, pthreat);
		CombatTimeStamp* StartTime = pthreat->GetStartTime(&apOut);
		double DecalMinWidth = StartTime->fTimeStamp;
		if (!(afSkipOlderThreats.fTimeStamp >= DecalMinWidth)) {
			float fscore =
				pthreat->ComputeThreatScore(aLocation, apActor, fcompareTime, afAdditionalRadius, afActorHealth);
			fthreatScore = fthreatScore + fscore;
			if (fthreatScore > 1.0) {
				fthreatScore = 1.0f;
				break;
			}
		}
	}
	Lock.UnlockRead();
	return fthreatScore;
}

// Saves every threat as its reference's form ID and the threat, with the count in front. The exe re-tests the
// position after every GetNext, which advances it in place and clears the threat pointer before filling it.
void CombatThreatMap::SaveGame(BGSSaveGameBuffer* apSaveGameBuffer) {
	uint32_t uiCount = 0;
	uint32_t uiPosition = apSaveGameBuffer->StartVariableSizedValue();
	NiTMapItem<uint32_t, CombatThreat*>* pPos = GetFirstPos();
	while (pPos) {
		CombatThreat* pThreat = nullptr;
		uint32_t uiFormID;
		GetNext(pPos, uiFormID, pThreat);
		if (pThreat) {
			apSaveGameBuffer->SaveFormID(uiFormID, 0);
			pThreat->SaveGame(apSaveGameBuffer);
			++uiCount;
		}
	}
	apSaveGameBuffer->SaveVariableSizedValue(uiCount, uiPosition);
}

void CombatThreatMap::FinishLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
	NiTMapItem<uint32_t, CombatThreat*>* it;
	it = GetFirstPos();
	while (it) {
		uint32_t irefID;
		CombatThreat* pthreat = 0;
		GetNext(it, irefID, pthreat);
		if (pthreat) {
		}
	}
}

void CombatThreatMap::RemoveAll() {
	NiTMapItem<uint32_t, CombatThreat*>* a2;
	a2 = GetFirstPos();
	while (a2) {
		uint32_t a3 = 0;
		CombatThreat* a4 = 0;
		GetNext(a2, a3, a4);
		if (a4) {
			delete a4;
		}
	}
	NiTMapBase<DFALL<NiTMapItem<uint32_t, CombatThreat*>>, uint32_t, CombatThreat*>::RemoveAll();
}

// The exe's cleanup-only C++ EH frame is left out.
PathingAvoidNodeArray* CombatThreatMap::CreateAvoidNodeArray(const NiPoint3& arLocation, Actor* apActor, float afTime) {
	PathingAvoidNode v11;
	PathingAvoidNode apResult;
	NiTMapItem<uint32_t, CombatThreat*>* it;
	TESCombatStyle* pcombatStyle = apActor->GetCombatStyle();
	if (pcombatStyle->GetIgnoreThreats()) {
		return 0;
	}
	float fcompareTime = 0.0;
	if (!(afTime < 0.0)) {
		fcompareTime = CombatTimer::GetTimer() + afTime;
	} else {
		fcompareTime = afTime;
	}
	PathingAvoidNodeArray* pavoidNodeArray = 0;
	CombatThreat* pbestThreat = 0;
	float fhighestThreatScore = 0.0f;
	Lock.LockForRead();
	it = GetFirstPos();
	while (it) {
		uint32_t ithreatID = 0;
		CombatThreat* pthreat = 0;
		GetNext(it, ithreatID, pthreat);
		float fthreatScore = pthreat->ComputeThreatScore(arLocation, apActor, fcompareTime, 1024.0f, 0.0f);
		if (fhighestThreatScore < static_cast<double>(fthreatScore)) {
			fhighestThreatScore = fthreatScore;
			pbestThreat = pthreat;
		}
		float* v5 = &fCombatThreatSignificantScore.Float();
		if (*v5 < static_cast<double>(fthreatScore)) {
			if (!pavoidNodeArray) {
				PathingAvoidNodeArray* pavoidNodeArray_2;
				void* apMemory = NiMemObject::operator new(sizeof(PathingAvoidNodeArray));
				if (apMemory) {
					pavoidNodeArray_2 = ::new (apMemory) PathingAvoidNodeArray;
				} else {
					pavoidNodeArray_2 = 0;
				}
				PathingAvoidNodeArray* v16 = pavoidNodeArray_2;
				pavoidNodeArray = pavoidNodeArray_2;
			}
			PathingAvoidNode* AvoidNode = pthreat->GetAvoidNode(&apResult, apActor);
			pavoidNodeArray->Add(*AvoidNode);
		}
	}
	Lock.UnlockRead();
	if (!pavoidNodeArray) {
		if (pbestThreat) {
			PathingAvoidNodeArray* pavoidNodeArray_3;
			PathingAvoidNodeArray* v12 =
				static_cast<PathingAvoidNodeArray*>(NiMemObject::operator new(sizeof(PathingAvoidNodeArray)));
			if (v12) {
				pavoidNodeArray_3 = ::new (v12) PathingAvoidNodeArray;
			} else {
				pavoidNodeArray_3 = 0;
			}
			pavoidNodeArray = pavoidNodeArray_3;
			PathingAvoidNode* v7 = pbestThreat->GetAvoidNode(&v11, apActor);
			pavoidNodeArray->Add(*v7);
		}
	}
	return pavoidNodeArray;
}

void CombatThreatMap::AddProjectile(Projectile* apProjectile) {
	if (!apProjectile->IsFlagSet(Projectile::HITSCAN_MODE)) {
		BGSProjectile* pBaseForm = apProjectile->GetProjectileBase();
		BGSExplosion* CursorRoot = pBaseForm->GetExplosion();
		if (pBaseForm->GetHasExplosion()) {
			if (CursorRoot) {
				float afExplosionRadius = CursorRoot->GetRadiusBSUnits();
				float afBaseExplosionDamage = CursorRoot->GetDamage();
				if (fDangerousProjectileExplosionDamage.Float() <= static_cast<double>(afBaseExplosionDamage)) {
					float* v2 = &fDangerousProjectileExplosionDamage.Float();
					float v8 =
						CombatFormulas::CalcExplosionDamageDistance(afExplosionRadius, afBaseExplosionDamage, *v2);
					if (fDangerousProjectileExplosionRadius.Float() <= static_cast<double>(v8)) {
						float afStartTime = 0.0;
						float afEndTime = 3.4028235e+38f;
						if (pBaseForm->GetHasExplosionAltTrigger()) {
							if (apProjectile->GetExplosionTimer() == 3.4028234663852886e+38) {
								afEndTime = fCombatThreatProximityExplosionAvoidTime.Float();
							} else {
								afEndTime = apProjectile->GetExplosionTimer() + 1.0;
								if (fCombatThreatTimedExplosionLength.Float() < static_cast<double>(afEndTime)) {
									afStartTime = afEndTime - fCombatThreatTimedExplosionLength.Float();
								}
							}
						}
						if (afEndTime > 0.0) {
							AddThreat(apProjectile, v8, afStartTime, afEndTime, afBaseExplosionDamage);
						}
					}
				}
			}
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void CombatThreatMap::LoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
	uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
	for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
		CombatThreat* pthreat;
		uint32_t irefID;
		bool bremove = 0;
		CombatThreat* v4 = static_cast<CombatThreat*>(ourMalloc(sizeof(CombatThreat)));
		if (v4) {
			pthreat = ::new (v4) CombatThreat;
		} else {
			pthreat = 0;
		}
		apLoadGameBuffer->LoadFormID(irefID);
		// The threat keeps the reference's form ID until InitLoadGame resolves it.
		// EXE-CAST: uint32_t (a form ID) as TESObjectREFR*
		pthreat->pThreat = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(irefID));
		pthreat->Load(apLoadGameBuffer);
		if (!irefID) {
			bremove = 1;
		}
		if (bremove) {
			if (pthreat) {
				delete pthreat;
			}
		} else {
			SetAt(irefID, pthreat);
		}
	}
}

void CombatThreatMap::InitLoadGame(BGSLoadGameBuffer* apLoadGameBuffer) {
	NiTMapItem<uint32_t, CombatThreat*>* it = GetFirstPos();
	while (it) {
		uint32_t irefID;
		CombatThreat* pthreat = 0;
		GetNext(it, irefID, pthreat);
		if (pthreat) {
			pthreat->InitLoadGame(apLoadGameBuffer);
			if (!pthreat->pThreat) {
				if (pthreat) {
					delete pthreat;
				}
				RemoveAt(irefID);
			}
		}
	}
}

// The projectile's alternate explosion trigger flag.
bool BGSProjectile::GetHasExplosionAltTrigger() {
	return IsFlagSet(EXPLOSION_ALT_TRIGGER);
}

float Projectile::GetExplosionTimer() {
	return fExplosionTimer;
}
