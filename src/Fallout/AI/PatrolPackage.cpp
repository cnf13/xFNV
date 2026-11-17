#include "Fallout/AI/PatrolPackage.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageData.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/Pathfinding/NavMeshObstacleManager.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/ActorPackageData.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "SDK/MSVC/rtti.h"

// The exe's C++ EH (unwind) frame is left out: it only matters when a call in here throws.
// The other fields stay unset, as in the exe's constructor.
PatrolActorPackageData::PatrolActorPackageData()
	: ActorPackageData(), eState(PATROL_STATE_INIT), pActor(nullptr), pPackage(nullptr), pPackageData(nullptr),
	  References() {}

void PatrolActorPackageData::HandleDeletedReference(TESObjectREFR* apDeletedObj) {
	if (apDeletedObj == pActor) {
		ClearReservedPatrolRef();
		eState = PATROL_STATE_DEBUG_ACTOR_DELETED;
		pActor = 0;
		pPackage = 0;
		pPackageData = 0;
	} else {
		for (uint32_t a3 = 0; a3 < References.iSize; ++a3) {
			if (*static_cast<TESObjectREFR**>(&References.GetAt(a3)) == apDeletedObj) {
				References.Remove(a3, 1);
				if (iCurrentReference >= a3) {
					--iCurrentReference;
				}
				--a3;
			}
		}
		if (pActor) {
			if (!pSaveLoadGame->GetSaveGameLoading() && !SetCurrentReference(iCurrentReference)) {
				const NiPoint3* v2 = &pActor->GetLocationOnReference();
				ResumeRepeatablePatrol(v2);
			}
		}
	}
}

void PatrolActorPackageData::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	uint32_t isaveGameCount;
	apSaveGameBuffer->SaveDataEndian(&eState, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&fCurrentTimeAtReference, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&iCurrentReference, 4u, 0);
	apSaveGameBuffer->SaveDataEndian(&bPatrolDirection, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&bCircularArray, 1u, 0);
	apSaveGameBuffer->SaveDataEndian(&iLoopAroundIndex, 4u, 0);
	apSaveGameBuffer->SaveFormID(pPackage, 0);
	// The exe tests the address &kReferences, which is never null.
	isaveGameCount = References.iSize;
	apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount);
	for (uint32_t i = 0; i < isaveGameCount; ++i) {
		apSaveGameBuffer->SaveFormID(References.operator[](i), 0);
	}
}

void PatrolActorPackageData::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	PatrolActorPackageData::PATROL_STATE eState;
	apLoadGameBuffer->LoadData(&this->eState, 4u);
	// The exe tests both bounds (jl 0, jg 2): a loaded state outside INIT..AT_REF becomes INIT.
	if (this->eState >= PatrolActorPackageData::PATROL_STATE_INIT &&
		this->eState <= PatrolActorPackageData::PATROL_STATE_AT_REF) {
		eState = this->eState;
	} else {
		eState = PatrolActorPackageData::PATROL_STATE_INIT;
	}
	this->eState = eState;
	apLoadGameBuffer->LoadData(&fCurrentTimeAtReference, 4u);
	apLoadGameBuffer->LoadData(&iCurrentReference, 4u);
	apLoadGameBuffer->LoadData(&bPatrolDirection, 1u);
	apLoadGameBuffer->LoadData(&bCircularArray, 1u);
	apLoadGameBuffer->LoadData(&iLoopAroundIndex, 4u);
	uint32_t FormID_0 = apLoadGameBuffer->LoadFormID();
	TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
	pPackage = dynamic_cast<TESPackage*>(FormByNumericID);
	uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
	References.SetSize(isaveGameCount, 1);
	for (uint32_t i = 0; i < isaveGameCount; ++i) {
		// Each slot holds the reference's form ID until InitLoadGame resolves it.
		apLoadGameBuffer->LoadFormID(References.operator[](i));
	}
}

void PatrolActorPackageData::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	uint32_t isaveGameCount = References.iSize;
	for (uint32_t i = 0; i < isaveGameCount; ++i) {
		TESObjectREFR* pResolved;
		bool bremove = 0;
		TESObjectREFR** ppatrolRef = &References.operator[](i);
		// The slot holds the form ID LoadGame read into it.
		if (*ppatrolRef) {
			TESForm* FormByNumericID = TESForm::GetFormByNumericID(*reinterpret_cast<uint32_t*>(ppatrolRef));
			pResolved = dynamic_cast<TESObjectREFR*>(FormByNumericID);
		} else {
			pResolved = 0;
		}
		*ppatrolRef = pResolved;
		if (*ppatrolRef) {
			(*ppatrolRef)->SetTargeted(1);
		} else {
			uint32_t inewReference;
			bremove = 1;
			if (iCurrentReference) {
				inewReference = iCurrentReference - 1;
			} else {
				inewReference = 0;
			}
			iCurrentReference = inewReference;
		}
		if (bremove) {
			References.RemoveFast(i--, 1u);
			--isaveGameCount;
		}
	}
	pActor = apLoadGameBuffer->GetActor();
	if (pPackage) {
		if (!pPackageData) {
			pPackageData = pPackage->GetPatrolPackageData();
		}
		if (!SetCurrentReference(iCurrentReference)) {
			const NiPoint3* v3 = &pActor->GetLocationOnReference();
			ResumeRepeatablePatrol(v3);
		}
		if (!References.IsEmpty() && bCircularArray) {
			ExtraDataList* ppatrolExtra;
			TESObjectREFR* ppatrolRef = References.GetAt(iCurrentReference);
			if (ppatrolRef) {
				ppatrolExtra = &ppatrolRef->m_Extra;
			} else {
				ppatrolExtra = 0;
			}
			if (ppatrolExtra) {
				ppatrolExtra->SetPatrolRefInUse(pActor);
			}
		}
	}
}

PatrolActorPackageData::~PatrolActorPackageData() {
	if (!pDataHandler->GetClearingData()) {
		ClearReservedPatrolRef();
	}
	References.SetSize(0, 1);
	eState = PATROL_STATE_DEBUG_DESTRUCTED;
	pActor = nullptr;
	pPackage = nullptr;
	pPackageData = nullptr;
}

bool PatrolActorPackageData::Init(Actor* apActor, TESPackage* apPackage) {
	pActor = apActor;
	pPackage = apPackage;
	pPackageData = apPackage->GetPatrolPackageData();
	iCurrentReference = 0;
	bPatrolDirection = 1;
	iLoopAroundIndex = 0;
	bool bvalid = BuildReferenceArray();
	if (bvalid) {
		if (pPackageData->bRepeatable) {
			const NiPoint3* v3 = &apActor->GetLocationOnReference();
			ResumeRepeatablePatrol(v3);
		}
	}
	return bvalid;
}

bool PatrolActorPackageData::SetNextReferenceForPatrol(bool abWaitForOccupiedPoints) {
	bool brepeatable;
	uint32_t inextReference_2;
	uint32_t inextReference = iCurrentReference;
	bool bkeepPatrolling = 1;
	if (pPackageData) {
		brepeatable = pPackageData->bRepeatable;
	} else {
		brepeatable = 0;
	}
	if (bPatrolDirection) {
		inextReference_2 = inextReference + 1;
		if (inextReference_2 == References.iSize) {
			if (brepeatable) {
				if (bCircularArray) {
					inextReference_2 = iLoopAroundIndex;
				} else {
					if (References.iSize == 1) {
						inextReference_2 = 0;
					} else {
						inextReference_2 -= 2;
					}
					bPatrolDirection = !bPatrolDirection;
				}
			} else {
				inextReference_2 = 0;
				bkeepPatrolling = 0;
			}
		}
	} else {
		inextReference_2 = inextReference - 1;
		if (inextReference_2 == -1) {
			if (brepeatable) {
				if (bCircularArray) {
					inextReference_2 = 0;
					bkeepPatrolling = 0;
				} else {
					inextReference_2 = References.iSize != 1;
					bPatrolDirection = !bPatrolDirection;
				}
			} else {
				inextReference_2 = 0;
				bkeepPatrolling = 0;
			}
		}
	}
	if (abWaitForOccupiedPoints) {
		if (inextReference_2 != iCurrentReference && !SetCurrentReference(inextReference_2)) {
			uint32_t isize = References.iSize;
			uint32_t iavailable = isize - 1;
			for (uint32_t i = 0; i < isize; ++i) {
				ExtraDataList* pnewExtra;
				TESObjectREFR* pref = References.GetAt(i);
				if (pref) {
					pnewExtra = &pref->m_Extra;
				} else {
					pnewExtra = 0;
				}
				if (i != iCurrentReference && (!pnewExtra || !pnewExtra->GetPatrolRefCanBeUsedBy(pActor))) {
					--iavailable;
				}
			}
			if (!iavailable) {
				return SetNextReferenceForPatrol(0);
			}
		}
	} else {
		iCurrentReference = inextReference_2;
	}
	return bkeepPatrolling;
}

void PatrolActorPackageData::ClearReservedPatrolRef() {
	ExtraDataList* pExtraDataList;
	TESObjectREFR* pReference;
	if (((uint32_t)((int32_t)iCurrentReference)) < (&References)->BSSimpleArray<TESObjectREFR*>::GetSize()) {
		pReference = (&References)->GetAt(iCurrentReference);
		if (pReference) {
			pExtraDataList = pReference->TESObjectREFR::GetExtra();
		} else {
			pExtraDataList = nullptr;
		}
		if (pExtraDataList) {
			pExtraDataList->SetPatrolRefInUse(nullptr);
		}
	}
}

TESObjectREFR* PatrolActorPackageData::GetCurrentReference() {
	bool vCondition1;
	vCondition1 = (&References)->IsEmpty();
	if (vCondition1) {
		return nullptr;
	} else {
		return (&References)->GetAt(iCurrentReference);
	}
}

bool PatrolActorPackageData::BuildReferenceArray() {
	bool brepeatable;
	TESObjectREFR* pcurrentReference = PatrolActorPackageData::GetInitialReferenceForSetup(pActor, pPackage);
	bCircularArray = 0;
	References.SetSize(0, 1);
	if (pPackageData) {
		brepeatable = pPackageData->bRepeatable;
	} else {
		brepeatable = 0;
	}
	while (pcurrentReference) {
		pcurrentReference->SetTargeted(1);
		References.Add(pcurrentReference);
		pcurrentReference = pcurrentReference->GetLinkedRef();
		if (!pcurrentReference) {
			break;
		}
		int32_t iloopIndex =
			References.Find(pcurrentReference, 0, &BSSimpleArray<TESObjectREFR*>::DefaultEqualFunctionByAddress);
		if (iloopIndex != -1) {
			if (brepeatable) {
				bCircularArray = 1;
				iLoopAroundIndex = iloopIndex;
			} else {
				References.Add(pcurrentReference);
			}
			return !References.IsEmpty();
		}
	}
	return !References.IsEmpty();
}

void PatrolActorPackageData::ResumeRepeatablePatrol(const NiPoint3* arPoint) {
	if (pActor) {
		int32_t FormFlags = References.iSize;
		SetCurrentReference(0);
		this->bPatrolDirection = 1;
		TESObjectREFR* v20 = 0;
		const NiPoint3* pt = 0;
		uint32_t iCurrentReference = 0;
		bool bPatrolDirection = 1;
		TESObjectREFR* CurrentReference = GetCurrentReference();
		if (CurrentReference) {
			const NiPoint3* v15 = &CurrentReference->GetLocationOnReference();
			float v19 = 3.4028235e+38f;
			uint32_t aiRefIndex = 0;
			bool v12 = 1;
			char v16 = 0;
			for (int32_t i = 0; i < FormFlags; ++i) {
				ExtraDataList* Extra;
				iCurrentReference = this->iCurrentReference;
				bPatrolDirection = this->bPatrolDirection;
				v20 = CurrentReference;
				pt = v15;
				SetNextReferenceForPatrol(0);
				TESObjectREFR* v8 = GetCurrentReference();
				if (v8) {
					Extra = v8->GetExtra();
				} else {
					Extra = 0;
				}
				ExtraDataList* v7 = Extra;
				if (!Extra || v7->GetPatrolRefCanBeUsedBy(pActor)) {
					NiPoint3 v4;
					NiPoint3 result;
					CurrentReference = GetCurrentReference();
					if (!CurrentReference) {
						break;
					}
					v15 = &CurrentReference->GetLocationOnReference();
					result = *pt - *arPoint;
					v4 = *v15 - *pt;
					float v6 = result.SqrXYLength();
					if (v19 > static_cast<double>(v6)) {
						v19 = v6;
						aiRefIndex = iCurrentReference;
						v12 = bPatrolDirection;
						v16 = 1;
					}
				}
			}
			if (v16) {
				SetCurrentReference(aiRefIndex);
				this->bPatrolDirection = v12;
			}
			this->bPatrolDirection = 1;
		}
	}
}

bool PatrolActorPackageData::SetCurrentReference(uint32_t aiRefIndex) {
	int32_t aiclampedRefIndex;
	if (!References.IsEmpty() && bCircularArray) {
		ExtraDataList* pnewExtra;
		aiclampedRefIndex = aiRefIndex;
		int32_t isize = References.iSize - 1;
		BSClamp<int32_t>(aiclampedRefIndex, 0, isize);
		bool bsuccess = 1;
		TESObjectREFR* pnewRef = References.GetAt(aiclampedRefIndex);
		if (pnewRef) {
			pnewExtra = &pnewRef->m_Extra;
		} else {
			pnewExtra = 0;
		}
		if (pnewExtra) {
			if (pnewExtra->GetPatrolRefCanBeUsedBy(pActor)) {
				ClearReservedPatrolRef();
				pnewExtra->SetPatrolRefInUse(pActor);
				iCurrentReference = aiclampedRefIndex;
			} else {
				return 0;
			}
		}
		return bsuccess;
	} else {
		iCurrentReference = aiRefIndex;
		return 1;
	}
}

TESObjectREFR* PatrolActorPackageData::GetInitialReferenceForSetup(Actor* apActor, TESPackage* apPack) {
	TESObjectREFR* linkedRef;
	TESObjectREFR* locReference;
	TESObjectREFR* v5 = nullptr;
	PackageLocation* location = apPack->GetLocation();
	if (location) {
		locReference = location->GetLocReference();
		if (locReference) {
			v5 = locReference;
		}
	}
	if (!v5) {
		linkedRef = apActor->TESObjectREFR::GetLinkedRef();
		if (linkedRef) {
			return linkedRef;
		}
	}
	return v5;
}
