#include "Fallout/AI/Combat/CombatProcedureActivateObject.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMenu/IDebugText.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESIdleForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/CombatProcedure.h"
#include "Fallout/AI/Combat/CombatState.h"
#include "Fallout/AI/CombatController.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "SDK/MSVC/rtti.h"

// Marks the target reference as targeted.
CombatProcedureActivateObject::CombatProcedureActivateObject(TESObjectREFR* apTargetRef, TESBoundObject* apBoundObject)
	: pTargetRef(apTargetRef), pBoundObject(apBoundObject), cAnimating(0) {
	if (pTargetRef) {
		pTargetRef->SetTargeted(1);
	}
}

CombatProcedureActivateObject::~CombatProcedureActivateObject() {}

void CombatProcedureActivateObject::Update() {
	if (!pTargetRef) {
		SetFailed("Targeted reference is NULL for some reason");
		return;
	}
	Actor* pattacker = GetAttacker();
	Animation* panim = pattacker->GetAnimation();
	if (pattacker->pCurrentProcess->eLevel || !panim) {
		goto LABEL_25;
	}
	if (!cAnimating) {
		HighProcess* phighProcess_2;
		if (!panim->SpecialIdleDonePlaying() || panim->IsIdleNotLoaded(0)) {
			return;
		}
		phighProcess_2 = static_cast<HighProcess*>(pattacker->pCurrentProcess);
		if (phighProcess_2->FindSpecialIdletoPlay(pattacker, pTargetRef->GetObjectReference(), pTargetRef)) {
			cAnimating = 1;
			return;
		}
		cAnimating = -1;
	LABEL_25:;
		if (cAnimating != 3) {
			CombatState* pcombatState = pCombatController->pCombatState;
			if (pTargetRef->GetObjectReference() == pBoundObject) {
				pcombatState->PreActivateReference(pTargetRef);
				pTargetRef->Activate(pattacker, 1, 0, 1);
			} else {
				TESBoundObject* pboundObjects[5];
				ourMemset(pboundObjects, 0, sizeof(pboundObjects));
				uint32_t icount = 0;
				for (uint32_t i = 0; i < 5; ++i) {
					CombatState::TargetedObject* pcombatItem = pcombatState->GetTargetedObject(i);
					if (pcombatItem) {
						if (pcombatItem->pRef == pTargetRef) {
							pboundObjects[icount++] = pcombatItem->pObject;
						}
					}
				}
				pcombatState->PreActivateReference(pTargetRef);
				for (uint32_t i_2 = 0; i_2 < icount; ++i_2) {
					int32_t icount_2 = pTargetRef->GetInventoryObjectCount(pboundObjects[i_2]);
					if (icount_2) {
						if (icount_2 < 0) {
							icount_2 = -icount_2;
						}
						pTargetRef->Activate(pattacker, 1, pboundObjects[i_2], icount_2);
					}
				}
			}
			if (!pattacker->IsInCombat()) {
				return;
			}
			pcombatState->UpdateInventoryData();
		}
		if (cAnimating == 2) {
			cAnimating = 3;
		} else {
			SetFinished();
		}
		return;
	}
	if (cAnimating != 1 && cAnimating != 3) {
		goto LABEL_25;
	}
	if (panim->SpecialIdleDonePlaying() && !panim->SpecialIdleWorking()) {
		goto LABEL_25;
	}
	if (cAnimating == 1) {
		TESIdleForm* pidleForm = panim->GetSpecialIdleForm();
		if (pidleForm) {
			const char* pfileName = pidleForm->GetModel();
			if (pfileName) {
				float afTime;
				float fsequenceTime = 0.0f;
				const char* punderscore = std::strrchr(pfileName, '_');
				if (punderscore) {
					fsequenceTime = std::atof(punderscore + 1);
				}
				BSAnimGroupSequence* pseq = panim->GetSpecialIdleSequence();
				if (!pseq ||
					(afTime = panim->GetTime(), static_cast<double>(fsequenceTime) < pseq->GetScaledTime(afTime))) {
					cAnimating = 2;
				}
			}
		}
	}
	if (cAnimating == 2) {
		goto LABEL_25;
	}
}

void CombatProcedureActivateObject::ClearReferencePointers(TESObjectREFR* apRef) {
	if (pTargetRef == apRef) {
		pTargetRef = 0;
		SetFailed("Targeted reference pointer was cleared");
	}
}

void CombatProcedureActivateObject::DisplayDebugText(IDebugText* apDebugText, DEBUGTEXT_LOCATION aeLocation,
	DEBUGTEXT_JUSTIFY aeJustify) {
	switch (cAnimating) {
	case 255: {
		apDebugText->PrintLine(aeLocation, aeJustify, "NO ANIM");
		break;
	}
	case 1: {
		apDebugText->PrintLine(aeLocation, aeJustify, "ANIM PLAYING");
		break;
	}
	case 2: {
		apDebugText->PrintLine(aeLocation, aeJustify, "ANIM GRABBING");
		break;
	}
	case 3: {
		apDebugText->PrintLine(aeLocation, aeJustify, "ANIM FINISHING PLAYING");
		break;
	}
	default: {
		break;
	}
	}
	if (pTargetRef) {
		const char* FullName;
		NiPoint3 v7 = pTargetRef->GetLocationOnReference();
		if (pTargetRef->GetFormEditorIDLength()) {
			FullName = pTargetRef->GetFormEditorID();
		} else {
			FullName = pTargetRef->GetFullName();
		}
		int32_t CastingType = pTargetRef->iFormID;
		apDebugText->PrintLine(aeLocation, aeJustify, "Target: '%s' (%08X) - Position: (%.02f,%.02f,%.02f)", FullName,
			CastingType, v7.x, v7.y, v7.z);
	}
}

CombatProcedure::COMBAT_PROCEDURES CombatProcedureActivateObject::GetType() {
	return CombatProcedure::ACTIVATE_OBJECT;
}

// The procedure's data, the target reference's and the object's form IDs, then the state byte.
void CombatProcedureActivateObject::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
	CombatProcedure::SaveGame(apSaveGameBuffer);
	apSaveGameBuffer->SaveFormID(pTargetRef, 0);
	apSaveGameBuffer->SaveFormID(pBoundObject, 0);
	apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&cAnimating), 1, 0);
}

// The target reference's form ID goes into pTargetRef, resolved later.
void CombatProcedureActivateObject::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	CombatProcedure::LoadGame(apLoadGameBuffer);
	apLoadGameBuffer->LoadFormID(pTargetRef);
	pBoundObject = dynamic_cast<TESBoundObject*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
	apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&cAnimating), 1);
}

void CombatProcedureActivateObject::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
	TESObjectREFR* v3;
	if (pTargetRef) {
		TESForm* FormByNumericID =
			TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTargetRef)));
		v3 = dynamic_cast<TESObjectREFR*>(FormByNumericID);
	} else {
		v3 = 0;
	}
	pTargetRef = v3;
	if (pTargetRef) {
		pTargetRef->SetTargeted(1);
	}
}

CombatProcedure::~CombatProcedure() {}
