#include "Fallout/AI/SpectatorPackage.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleArray.inl"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESLeveledList.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESTopic.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/Dialogue/MenuTopic.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "SDK/MSVC/rtti.h"

#include <cmath>
#include <new>

// The game settings this file defines, with their IDs and values, in initialization order.
SettingT<INISettingCollection> bDebugSpectatorThreats("bDebugSpectatorThreats:General", false);

using SpectatorThreatInfoArray = BSSimpleArray<SpectatorThreatInfo, 1024>;

using BSSimpleArray_SpectatorThreatInfo_1024 = BSSimpleArray<SpectatorThreatInfo, 1024>;

SpectatorThreatInfo::SpectatorThreatInfo()
	: pSource(nullptr), pTarget(nullptr), eThreatLevel(THREAT_LEVEL_NONE), iLastPerceivedTime(0), fRadius(0.0f),
	  LastKnownPosSource(NiPoint3_ZERO), LastKnownPosTarget(NiPoint3_ZERO), bHasSource(false), bHasTarget(false),
	  spDebugGeometry(nullptr) {}

SpectatorThreatInfo::~SpectatorThreatInfo() {
	if (spDebugGeometry.m_pObject) {
		if (spDebugGeometry.m_pObject->GetParent()) {
			spDebugGeometry.m_pObject->GetParent()->DetachChild(spDebugGeometry.m_pObject);
		}
		spDebugGeometry = nullptr;
	}
}

void SpectatorThreatInfo::Update() {
	TESObjectWEAP* pweapon;
	CombatController* v3;
	const NiPoint3* p_LastKnownPosTarget;
	const NiPoint3* p_LastKnownPosSource;
	uint32_t iLastPerceivedTime;
	if (pSource) {
		iLastPerceivedTime = BSGetTickCount();
	} else {
		iLastPerceivedTime = this->iLastPerceivedTime;
	}
	this->iLastPerceivedTime = iLastPerceivedTime;
	if (pSource) {
		p_LastKnownPosSource = &pSource->GetLocationOnReference();
	} else {
		p_LastKnownPosSource = &LastKnownPosSource;
	}
	LastKnownPosSource.x = p_LastKnownPosSource->x;
	LastKnownPosSource.y = p_LastKnownPosSource->y;
	LastKnownPosSource.z = p_LastKnownPosSource->z;
	if (pTarget) {
		p_LastKnownPosTarget = &pTarget->GetLocationOnReference();
	} else {
		p_LastKnownPosTarget = &LastKnownPosTarget;
	}
	LastKnownPosTarget.x = p_LastKnownPosTarget->x;
	LastKnownPosTarget.y = p_LastKnownPosTarget->y;
	LastKnownPosTarget.z = p_LastKnownPosTarget->z;
	if (pSource) {
		v3 = pSource->GetCombatController();
	} else {
		v3 = 0;
	}
	if (v3) {
		pweapon = v3->GetCurrentWeapon();
	} else {
		pweapon = 0;
	}
	THREAT_LEVEL enewThreatLevel = THREAT_LEVEL_NONE;
	if (pweapon) {
		switch (pweapon->GetWeaponType()) {
		case WEAPON_HAND_TO_HAND:
			enewThreatLevel = THREAT_LEVEL_EXPLOSION;
			fRadius = 512.0f;
			break;
		case WEAPON_ONE_HAND_MELEE:
			enewThreatLevel = THREAT_LEVEL_PROJECTILE_WEAPONS;
			fRadius = 256.0f;
			break;
		case WEAPON_TWO_HAND_MELEE:
			enewThreatLevel = THREAT_LEVEL_MELEE_WEAPONS;
			fRadius = 128.0f;
			break;
		case WEAPON_ONE_HAND_PISTOL:
			enewThreatLevel = THREAT_LEVEL_EXPLOSION;
			fRadius = 512.0f;
			break;
		case WEAPON_ONE_HAND_PISTOL_ENERGY:
			enewThreatLevel = THREAT_LEVEL_EXPLOSION;
			fRadius = 256.0f;
			break;
		default:
			break;
		}
	}
	eThreatLevel = static_cast<THREAT_LEVEL>(BSMAX(eThreatLevel, enewThreatLevel));
}

// Watches aSource and its combat target: marks both targeted, then updates the threat.
void SpectatorThreatInfo::SetupThreatFromActorToCombatTarget(Actor& aSource) {
	pSource = &aSource;
	pTarget = aSource.GetCombatTarget();
	bHasSource = true;
	bHasTarget = pTarget != nullptr;
	pSource->TESObjectREFR::SetTargeted(true);
	if (pTarget) {
		pTarget->TESObjectREFR::SetTargeted(true);
	}
	Update();
}

void SpectatorThreatInfo::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	TESForm* formByNumericID;
	TESForm* v3;
	Actor* pActor;
	Actor* v5;
	if (pSource) {
		formByNumericID = TESForm::GetFormByNumericID(reinterpret_cast<uintptr_t>(pSource));
		v5 = dynamic_cast<Actor*>(formByNumericID);
	} else {
		v5 = nullptr;
	}
	pSource = v5;
	if (pTarget) {
		v3 = TESForm::GetFormByNumericID(reinterpret_cast<uintptr_t>(pTarget));
		pActor = dynamic_cast<Actor*>(v3);
	} else {
		pActor = nullptr;
	}
	pTarget = pActor;
	if (pSource) {
		pSource->TESObjectREFR::SetTargeted(1);
	}
	if (pTarget) {
		pTarget->TESObjectREFR::SetTargeted(1);
	}
}

NiPoint3 SpectatorThreatInfo::GetLastKnownPosSource() const {
	NiPoint3 result;
	result = LastKnownPosSource;
	return result;
}

NiPoint3 SpectatorThreatInfo::GetLastKnownPosTarget() const {
	NiPoint3 result;
	result = LastKnownPosTarget;
	return result;
}

// Builds the node in a temporary and copies it into nodeOut.
void SpectatorThreatInfo::GetAvoidNode(PathingAvoidNode& nodeOut) const {
	PathingAvoidNode kNode;
	if (bHasSource) {
		if (bHasTarget) {
			nodeOut = *FNVD_RECONSTRUCT(PathingAvoidNode)(&kNode, GetLastKnownPosSource(), GetLastKnownPosTarget(),
				fRadius, 100.0f);
		} else {
			nodeOut = *FNVD_RECONSTRUCT(PathingAvoidNode)(&kNode, GetLastKnownPosSource(), fRadius, 100.0f);
		}
	} else if (bHasTarget) {
		nodeOut = *FNVD_RECONSTRUCT(PathingAvoidNode)(&kNode, GetLastKnownPosTarget(), fRadius, 100.0f);
	}
}

void SpectatorThreatInfo::GetAvoidNode(PathingAvoidNode* apPathingAvoidNode) const {
	NiPoint3 v9;
	PathingAvoidNode v10;
	NiPoint3 v11;
	PathingAvoidNode v12;
	NiPoint3 v13;
	NiPoint3 result;
	PathingAvoidNode apMemory;
	if (bHasSource) {
		float afRadius = fRadius;
		if (bHasTarget) {
			NiPoint3* LastKnownPosTarget = &(result = GetLastKnownPosTarget());
			NiPoint3* LastKnownPosSource = &(v13 = GetLastKnownPosSource());
			*apPathingAvoidNode = *FNVD_RECONSTRUCT(PathingAvoidNode)(&apMemory, *LastKnownPosSource,
				*LastKnownPosTarget, afRadius, 100.0f);
		} else {
			NiPoint3* v3 = &(v11 = GetLastKnownPosSource());
			*apPathingAvoidNode = *FNVD_RECONSTRUCT(PathingAvoidNode)(&v12, *v3, afRadius, 100.0f);
		}
	} else {
		if (bHasTarget) {
			float afRadiusa = fRadius;
			NiPoint3* v4 = &(v9 = GetLastKnownPosTarget());
			*apPathingAvoidNode = *FNVD_RECONSTRUCT(PathingAvoidNode)(&v10, *v4, afRadiusa, 100.0f);
		}
	}
}

void SpectatorThreatInfo::SaveGame(BGSSaveGameBuffer* a2) {
	uint32_t apData;
	a2->SaveDataEndian(&eThreatLevel, 4u, 0);
	apData = BSGetTickCount() - iLastPerceivedTime;
	a2->SaveDataEndian(&apData, 4u, 0);
	a2->SaveDataEndian(&fRadius, 4u, 0);
	a2->SaveDataEndian(&LastKnownPosSource, 12u, 0);
	a2->SaveDataEndian(&LastKnownPosTarget, 12u, 0);
	a2->SaveDataEndian(&bHasSource, 1u, 0);
	a2->SaveDataEndian(&bHasTarget, 1u, 0);
	a2->SaveFormID(pSource, 0);
	a2->SaveFormID(pTarget, 0);
}

void SpectatorThreatInfo::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	uint32_t apData;
	apLoadGameBuffer->LoadData(&eThreatLevel, 4u);
	apData = 0;
	apLoadGameBuffer->LoadData(&apData, 4u);
	iLastPerceivedTime = BSGetTickCount() - apData;
	apLoadGameBuffer->LoadData(&fRadius, 4u);
	apLoadGameBuffer->LoadData(&LastKnownPosSource, 12u);
	apLoadGameBuffer->LoadData(&LastKnownPosTarget, 12u);
	apLoadGameBuffer->LoadData(&bHasSource, 1u);
	apLoadGameBuffer->LoadData(&bHasTarget, 1u);
	apLoadGameBuffer->LoadFormID(pSource);
	apLoadGameBuffer->LoadFormID(pTarget);
}

void SpectatorThreatInfo::operator delete(void* apMemory) {
	ourFree(apMemory);
}

SpectatorThreatInfo* SpectatorThreatInfo::Assign(SpectatorThreatInfo* apThis, const SpectatorThreatInfo& arOther) {
	apThis->pSource = arOther.pSource;
	apThis->pTarget = arOther.pTarget;
	apThis->eThreatLevel = arOther.eThreatLevel;
	apThis->iLastPerceivedTime = arOther.iLastPerceivedTime;
	apThis->fRadius = arOther.fRadius;
	apThis->LastKnownPosSource = arOther.LastKnownPosSource;
	apThis->LastKnownPosTarget = arOther.LastKnownPosTarget;
	apThis->bHasSource = arOther.bHasSource;
	apThis->bHasTarget = arOther.bHasTarget;
	apThis->spDebugGeometry.operator=(arOther.spDebugGeometry);
	return apThis;
}

SpectatorPackage::SpectatorPackage()
	: iLastPathRequestTime(0), fConversationTimer(0.0f), fEventRadius(0.0f), bFlagCompleted(0),
	  kEventCenter(NiPoint3_ZERO), kThreatInfos(32u, 0), spDebugGeometry(nullptr) {
	SetPackType(PACKAGE_TYPE_SPECTATOR);
	ePROCEDURE_TYPE = PACKAGE_PROCEDURE_OBSERVE_COMBAT;
}

SpectatorPackage::~SpectatorPackage() {
	if (spDebugGeometry.m_pObject) {
		if (spDebugGeometry.m_pObject->GetParent()) {
			spDebugGeometry.m_pObject->GetParent()->DetachChild(spDebugGeometry.m_pObject);
		}
		spDebugGeometry = nullptr;
	}
}

void SpectatorPackage::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	uint32_t isaveGameCount;
	uint32_t apData;
	TESPackage::SaveGame(apSaveGameBuffer);
	apData = appTimer.iLastTime - iLastPathRequestTime;
	apSaveGameBuffer->SaveDataEndian(&apData, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fConversationTimer, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fEventRadius, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&bFlagCompleted, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&kEventCenter, 12u, 0);
	// The address &kThreatInfos is never null.
	isaveGameCount = kThreatInfos.iSize;
	apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount);
	for (uint32_t i = 0; i < isaveGameCount; ++i) {
		SpectatorThreatInfo* threatInfo = &kThreatInfos.operator[](i);
		threatInfo->SaveGame(apSaveGameBuffer);
	}
}

void SpectatorPackage::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	int32_t ipathTimeDelta;
	TESPackage::LoadGame(apLoadGameBuffer);
	ipathTimeDelta = 0;
	apLoadGameBuffer->LoadData(&ipathTimeDelta, 4u);
	uint32_t Time = appTimer.iLastTime;
	iLastPathRequestTime = Time - ipathTimeDelta;
	apLoadGameBuffer->LoadData(&fTimer, 4u);
	apLoadGameBuffer->LoadData(&fConversationTimer, 4u);
	apLoadGameBuffer->LoadData(&fEventRadius, 4u);
	apLoadGameBuffer->LoadData(&bFlagCompleted, 1u);
	apLoadGameBuffer->LoadData(&kEventCenter, 12u);
	uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
	kThreatInfos.SetSize(isaveGameCount, 1);
	for (uint32_t i = 0; i < isaveGameCount; ++i) {
		SpectatorThreatInfo* threatInfo = &kThreatInfos.operator[](i);
		threatInfo->LoadGame(apLoadGameBuffer);
	}
}

void SpectatorPackage::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	SpectatorThreatInfo* v3;
	uint32_t i;
	TESPackage::InitLoadGame(apLoadGameBuffer);
	uint32_t formFlags = (&kThreatInfos)->BSSimpleArray<SpectatorThreatInfo>::GetSize();
	for (i = 0; i < formFlags; ++i) {
		v3 = &kThreatInfos[i];
		v3->InitLoadGame(apLoadGameBuffer);
	}
}

void SpectatorPackage::Copy(TESForm* apCopy) {
	TESPackage::Copy(apCopy);
	SpectatorPackage* pcopy = dynamic_cast<SpectatorPackage*>(apCopy);
	if (pcopy) {
		iLastPathRequestTime = pcopy->iLastPathRequestTime;
		fTimer = pcopy->fTimer;
		fConversationTimer = pcopy->fConversationTimer;
		fEventRadius = pcopy->fEventRadius;
		kEventCenter.x = pcopy->kEventCenter.x;
		kEventCenter.y = pcopy->kEventCenter.y;
		kEventCenter.z = pcopy->kEventCenter.z;
		bFlagCompleted = pcopy->bFlagCompleted;
		int32_t isize = pcopy->kThreatInfos.iSize;
		kThreatInfos.SetSize(static_cast<uint32_t>(isize), 1);
		for (int32_t i = 0; i < isize; ++i) {
			SpectatorThreatInfo* v3 = &pcopy->kThreatInfos.operator[](i);
			SpectatorThreatInfo* v2 = &kThreatInfos.operator[](i);
			SpectatorThreatInfo::Assign(v2, *v3);
		}
	}
}

uint16_t SpectatorPackage::GetSaveSize() {
	return (uint16_t)(((int16_t)TESPackage::GetSaveSize()) + 8);
}

void SpectatorPackage::SaveGame() {
	TESPackage::SaveGame();
	SaveGameDataOLD(&fTimer, 4);
	SaveGameDataOLD(&fConversationTimer, 4);
}

void SpectatorPackage::LoadGame() {
	TESPackage::LoadGame();
	LoadGameDataOLD(&fTimer, 4);
	LoadGameDataOLD(&fConversationTimer, 4);
}

void SpectatorPackage::InitLoadGame() {
	TESPackage::InitLoadGame();
}

void SpectatorPackage::AddThreatFromActorToCombatTarget(Actor& aSourceActor) {
	Actor* ptargetActor = aSourceActor.GetCombatTarget();
	int32_t isize = kThreatInfos.iSize;
	SpectatorThreatInfo* pstoredInfo = 0;
	for (int32_t i = 0; i < isize; ++i) {
		SpectatorThreatInfo* scanInfo = &kThreatInfos.operator[](i);
		if (scanInfo->pSource == &aSourceActor && scanInfo->pTarget == ptargetActor) {
			pstoredInfo = scanInfo;
			break;
		}
	}
	if (!pstoredInfo) {
		int32_t index = kThreatInfos.iSize;
		kThreatInfos.SetSize(index + 1, 1);
		pstoredInfo = &kThreatInfos.operator[](index);
	}
	pstoredInfo->SetupThreatFromActorToCombatTarget(aSourceActor);
	ComputeCenterPoint();
}

NiPoint3 SpectatorPackage::GetEventCenter() {
	return kEventCenter;
}

// Empties the output array, reserves two entries per threat, then appends each threat's source actor when there is one.
void SpectatorPackage::CreateThreatArray(BSSimpleArray<Actor*>* apRetThreats) {
	apRetThreats->Clear(true);
	int32_t iThreatCount = (int32_t)kThreatInfos.GetSize();
	apRetThreats->SetReservedSize((uint32_t)iThreatCount * 2);
	for (int32_t i = 0; i < iThreatCount; ++i) {
		Actor* pSource = kThreatInfos[(uint32_t)i].GetSource();
		if (pSource) {
			apRetThreats->Add(pSource);
		}
	}
}

bool SpectatorPackage::IsComplete() const {
	return (bool)((uint8_t)((!(&kThreatInfos)->iSize) || bFlagCompleted));
}

void SpectatorPackage::ComputeCenterPoint() {
	if (kThreatInfos.iSize) {
		int32_t i;
		int32_t isize = kThreatInfos.iSize;
		int32_t icount = 0;
		NiPoint3 maxPoint = NiPoint3_ZERO;
		NiPoint3 minPoint = NiPoint3_ZERO;
		bool bInitialized = 0;
		for (i = 0; i < isize; ++i) {
			SpectatorThreatInfo* info = &kThreatInfos.operator[](i);
			if (info->bHasSource && info->bHasTarget) {
				float X = info->GetLastKnownPosTarget().x;
				maxPoint.x = BSMAX(info->GetLastKnownPosSource().x, X);
				float Xa = info->GetLastKnownPosTarget().y;
				maxPoint.y = BSMAX(info->GetLastKnownPosSource().y, Xa);
				float Xb = info->GetLastKnownPosTarget().z;
				maxPoint.z = BSMAX(info->GetLastKnownPosSource().z, Xb);
				float Xc = info->GetLastKnownPosTarget().x;
				minPoint.x = BSMIN(info->GetLastKnownPosSource().x, Xc);
				float Xd = info->GetLastKnownPosTarget().y;
				minPoint.y = BSMIN(info->GetLastKnownPosSource().y, Xd);
				float Xe = info->GetLastKnownPosTarget().z;
				minPoint.z = BSMIN(info->GetLastKnownPosSource().z, Xe);
				icount += 2;
				break;
			}
			if (info->bHasSource) {
				maxPoint = info->GetLastKnownPosSource();
				minPoint = info->GetLastKnownPosSource();
				++icount;
				break;
			}
			if (info->bHasTarget) {
				maxPoint = info->GetLastKnownPosTarget();
				minPoint = info->GetLastKnownPosTarget();
				++icount;
				break;
			}
		}
		++i;
		while (i < isize) {
			SpectatorThreatInfo* info_2 = &kThreatInfos.operator[](i);
			if (info_2->bHasSource) {
				maxPoint.x = BSMAX(maxPoint.x, info_2->GetLastKnownPosSource().x);
				maxPoint.y = BSMAX(maxPoint.y, info_2->GetLastKnownPosSource().y);
				maxPoint.z = BSMAX(maxPoint.z, info_2->GetLastKnownPosSource().z);
				minPoint.x = BSMIN(minPoint.x, info_2->GetLastKnownPosSource().x);
				minPoint.y = BSMIN(minPoint.y, info_2->GetLastKnownPosSource().y);
				minPoint.z = BSMIN(minPoint.z, info_2->GetLastKnownPosSource().z);
				++icount;
			}
			if (info_2->bHasTarget) {
				maxPoint.x = BSMAX(maxPoint.x, info_2->GetLastKnownPosTarget().x);
				maxPoint.y = BSMAX(maxPoint.y, info_2->GetLastKnownPosTarget().y);
				maxPoint.z = BSMAX(maxPoint.z, info_2->GetLastKnownPosTarget().z);
				minPoint.x = BSMIN(minPoint.x, info_2->GetLastKnownPosTarget().x);
				minPoint.y = BSMIN(minPoint.y, info_2->GetLastKnownPosTarget().y);
				minPoint.z = BSMIN(minPoint.z, info_2->GetLastKnownPosTarget().z);
				++icount;
			}
			++i;
		}
		if (icount) {
			NiPoint3 centerPoint = maxPoint;
			centerPoint.operator+=(minPoint);
			centerPoint.operator*=(0.5f);
			float fdistanceSquared = 0.0f;
			float ffarthestDistanceSquared = 0.0f;
			for (i = 0; i < isize; ++i) {
				SpectatorThreatInfo* info_3 = &kThreatInfos.operator[](i);
				if (info_3->bHasSource || info_3->bHasTarget) {
					if (info_3->bHasSource) {
						fdistanceSquared = (centerPoint - info_3->GetLastKnownPosSource()).SqrLength();
						if (ffarthestDistanceSquared <= static_cast<double>(fdistanceSquared)) {
							ffarthestDistanceSquared = fdistanceSquared;
						}
					}
					if (info_3->bHasTarget) {
						fdistanceSquared = (centerPoint - info_3->GetLastKnownPosTarget()).SqrLength();
						if (ffarthestDistanceSquared <= static_cast<double>(fdistanceSquared)) {
							ffarthestDistanceSquared = fdistanceSquared;
						}
					}
				}
			}
			kEventCenter = centerPoint;
			fEventRadius = std::sqrtf(ffarthestDistanceSquared);
		} else {
			bFlagCompleted = 1;
		}
	}
}

void SpectatorPackage::MakeCombatComment(Actor* apActor, bool bflag) {
	if (bflag) {
		if (kThreatInfos.iSize) {
			Actor* v5;
			int32_t v10 = Random() % 100;
			Actor* FormHealth_0 = (&kThreatInfos.GetFirst())->pTarget;
			if (FormHealth_0 && FormHealth_0->IsActor()) {
				v5 = FormHealth_0;
			} else {
				v5 = 0;
			}
			if (v5) {
				if (apActor->pCurrentProcess) {
					BaseProcess* CurrentAIProcess = apActor->pCurrentProcess;
					if (!CurrentAIProcess->GetGreetingFlag() &&
						fAISocialchanceForConversation.Float() > static_cast<double>(v10) &&
						fConversationTimer <= 0.0) {
						TESTopic* Topic = TESTopic::GetTopic(DT_MISCELLANEOUS, 0);
						if (Topic) {
							apActor->SetDialogueItemTarget(v5);
							BaseProcess* v3 = apActor->pCurrentProcess;
							v3->ProcessGreet(apActor, Topic, 0, 0, 1, 0);
							fConversationTimer = fAISpectatorCommentTimer.Float();
						}
					}
				}
				fConversationTimer = fConversationTimer - appTimer.fDelta;
			}
		}
	} else {
		BaseProcess* v6 = apActor->pCurrentProcess;
		v6->SetGreetingFlag(0);
		apActor->StopMoving();
	}
}

void SpectatorPackage::LookAtCombatants(Actor* apActor, bool bflag) {
	TESObjectREFR* v5;
	Actor* FormHealth_0;
	if (bflag &&
		(kThreatInfos.iSize &&
			((FormHealth_0 = (&kThreatInfos.GetFirst())->pTarget) == 0 || !FormHealth_0->IsActor() ? v5 = 0
																								   : v5 = FormHealth_0,
				v5))) {
		BaseProcess* CurrentAIProcess = apActor->pCurrentProcess;
		CurrentAIProcess->SetActionHeadTrackTarget(v5);
	} else {
		BaseProcess* v3 = apActor->pCurrentProcess;
		v3->ClearActionHeadTrackTarget(1);
	}
}

void SpectatorPackage::Update(Actor* pOwner) {
	char v18 = 0;
	int32_t isize = kThreatInfos.iSize;
	for (int32_t i = 0; i < isize; ++i) {
		SpectatorThreatInfo* info = &kThreatInfos.operator[](i);
		if (info->pSource) {
			if (info->pTarget) {
				Actor* Source = info->pSource;
				Actor* pcurrentTarget = Source->GetCombatTarget();
				if (pcurrentTarget) {
					if (pcurrentTarget != info->pTarget) {
						AddThreatFromActorToCombatTarget(*pOwner);
						v18 = 1;
					}
				}
			}
		}
	}
	int32_t isize_2 = kThreatInfos.iSize;
	for (int32_t i_2 = 0; i_2 < isize_2; ++i_2) {
		SpectatorThreatInfo* info_2 = &kThreatInfos.operator[](i_2);
		Actor* psource = info_2->pSource;
		Actor* ptarget = info_2->pTarget;
		uint32_t v2 = BSGetTickCount();
		if (v2 > info_2->iLastPerceivedTime + 2000) {
			double v6 = static_cast<double>(BSGetTickCount());
			double CastingType = static_cast<double>(info_2->iLastPerceivedTime);
			if (!(fAISpectatorRememberThreatTimer.Float() * 1000.0 + CastingType < v6)) {
				if (ptarget &&
					(ptarget->IsDead(0) ||
						(ptarget->GetDelete() || (ptarget->IsParalyzed() || ptarget->IsKnockedOut())))) {
					info_2->pTarget = 0;
					v18 = 1;
				} else {
					DetectionState* v3;
					BaseProcess* CurrentAIProcess;
					if (!psource ||
						(!psource->IsNpc() ||
							(!pOwner->pCurrentProcess ||
								(!pOwner->pCurrentProcess ? v3 = 0
														  : (CurrentAIProcess = pOwner->pCurrentProcess,
																v3 = CurrentAIProcess->GetDetectionState(psource, 0)),
									v3 && v3->iLevel > 0)))) {
						if (!psource || (!ptarget || psource->GetCombatTarget() == ptarget)) {
							v18 = 1;
							info_2->Update();
						}
					}
				}
			} else {
				kThreatInfos.RemoveFast(static_cast<uint32_t>(i_2), 1u);
				--isize_2;
				--i_2;
				v18 = 1;
			}
		}
	}
	if (v18) {
		ComputeCenterPoint();
	}
}

void SpectatorPackage::HandleDeletedActor(void* a2) {
	int32_t isize = kThreatInfos.iSize;
	for (int32_t i = 0; i < isize; ++i) {
		SpectatorThreatInfo* scanInfo = &kThreatInfos.operator[](i);
		if (scanInfo->pSource == a2) {
			kThreatInfos.RemoveFast(i, 1u);
			--isize;
			--i;
		} else {
			if (scanInfo->pTarget == a2) {
				scanInfo->pTarget = 0;
				scanInfo->bHasTarget = 0;
			}
		}
	}
}

void SpectatorPackage::RemoveActorAsThreatSourceOrTarget(Actor& aActor) {
	int32_t isize = kThreatInfos.iSize;
	for (int32_t i = 0; i < isize; ++i) {
		SpectatorThreatInfo* scanInfo = &kThreatInfos.operator[](i);
		if (scanInfo->pSource == &aActor || scanInfo->pTarget == &aActor) {
			kThreatInfos.RemoveFast(static_cast<uint32_t>(i), 1u);
			--isize;
			--i;
		}
	}
}

PathingAvoidNodeArray* SpectatorPackage::CreateAvoidNodeArray() {
	PathingAvoidNodeArray* pavoidNodeArray;
	PathingAvoidNodeArray* apMemory =
		static_cast<PathingAvoidNodeArray*>(NiMemObject::operator new(sizeof(PathingAvoidNodeArray)));
	if (apMemory) {
		pavoidNodeArray = ::new (apMemory) PathingAvoidNodeArray;
	} else {
		pavoidNodeArray = 0;
	}
	int32_t isize = kThreatInfos.iSize;
	for (int32_t i = 0; i < isize; ++i) {
		const SpectatorThreatInfo& scanInfo = kThreatInfos.operator[](i);
		int32_t nodeArrayIndex = pavoidNodeArray->iSize;
		pavoidNodeArray->SetSize(nodeArrayIndex + 1, 1);
		PathingAvoidNode* v1 = &pavoidNodeArray->GetAt(nodeArrayIndex);
		scanInfo.GetAvoidNode(v1);
	}
	return pavoidNodeArray;
}

void SpectatorPackage::operator delete(void* apMemory) {
	ourFree(apMemory);
}
