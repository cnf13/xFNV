#include "Fallout/AI/Combat/CombatProcedureActivateObject.h"

#include "BSCore/NewOverloads.h"
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
        pTargetRef->SetTargeted(true);
    }
}

CombatProcedureActivateObject::~CombatProcedureActivateObject() {}

void CombatProcedureActivateObject::Update() {
    if (!pTargetRef) {
        SetFailed("Targeted reference is NULL for some reason");
        return;
    }
    Actor* pAttacker = GetAttacker();
    Animation* pAnim = pAttacker->GetAnimation();
    if (!pAttacker->pCurrentProcess->eLevel && pAnim) {
        if (!cAnimating) {
            HighProcess* pHighProcess;
            if (!pAnim->SpecialIdleDonePlaying() || pAnim->IsIdleNotLoaded(nullptr)) {
                return;
            }
            pHighProcess = static_cast<HighProcess*>(pAttacker->pCurrentProcess);
            if (pHighProcess->FindSpecialIdletoPlay(pAttacker, pTargetRef->GetObjectReference(), pTargetRef)) {
                cAnimating = 1;
                return;
            }
            cAnimating = -1;
        }
        else if (cAnimating == 1 || cAnimating == 3) {
            if (!pAnim->SpecialIdleDonePlaying() || pAnim->SpecialIdleWorking()) {
                if (cAnimating == 1) {
                    TESIdleForm* pIdleForm = pAnim->GetSpecialIdleForm();
                    if (pIdleForm) {
                        const char* pFileName = pIdleForm->GetModel();
                        if (pFileName) {
                            float fSequenceTime = 0.0f;
                            const char* pUnderscore = std::strrchr(pFileName, '_');
                            if (pUnderscore) {
                                fSequenceTime = std::atof(pUnderscore + 1);
                            }
                            BSAnimGroupSequence* pSeq = pAnim->GetSpecialIdleSequence();
                            if (!pSeq || static_cast<double>(fSequenceTime) < pSeq->GetScaledTime(pAnim->GetTime())) {
                                cAnimating = 2;
                            }
                        }
                    }
                }
                if (cAnimating != 2) {
                    return;
                }
            }
        }
    }
    if (cAnimating != 3) {
        CombatState* pCombatState = pCombatController->pCombatState;
        if (pTargetRef->GetObjectReference() == pBoundObject) {
            pCombatState->PreActivateReference(pTargetRef);
            pTargetRef->Activate(pAttacker, true, nullptr, 1);
        }
        else {
            TESBoundObject* pBoundObjects[5];
            ourMemset(pBoundObjects, 0, sizeof(pBoundObjects));
            uint32_t uiCount = 0;
            for (uint32_t i = 0; i < 5; ++i) {
                CombatState::TargetedObject* pCombatItem = pCombatState->GetTargetedObject(i);
                if (pCombatItem && pCombatItem->pRef == pTargetRef) {
                    pBoundObjects[uiCount++] = pCombatItem->pObject;
                }
            }
            pCombatState->PreActivateReference(pTargetRef);
            for (uint32_t i = 0; i < uiCount; ++i) {
                int32_t iObjectCount = pTargetRef->GetInventoryObjectCount(pBoundObjects[i]);
                if (iObjectCount) {
                    if (iObjectCount < 0) {
                        iObjectCount = -iObjectCount;
                    }
                    pTargetRef->Activate(pAttacker, true, pBoundObjects[i], iObjectCount);
                }
            }
        }
        if (!pAttacker->IsInCombat()) {
            return;
        }
        pCombatState->UpdateInventoryData();
    }
    if (cAnimating == 2) {
        cAnimating = 3;
    }
    else {
        SetFinished();
    }
}

void CombatProcedureActivateObject::ClearReferencePointers(TESObjectREFR* apRef) {
    if (pTargetRef == apRef) {
        pTargetRef = nullptr;
        SetFailed("Targeted reference pointer was cleared");
    }
}

void CombatProcedureActivateObject::DisplayDebugText(
    IDebugText* apDebugText,
    DEBUGTEXT_LOCATION aeLocation,
    DEBUGTEXT_JUSTIFY aeJustify
) {
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
        NiPoint3 kLocation = pTargetRef->GetLocationOnReference();
        const char* pName;
        if (pTargetRef->GetFormEditorIDLength()) {
            pName = pTargetRef->GetFormEditorID();
        }
        else {
            pName = pTargetRef->GetFullName();
        }
        apDebugText->PrintLine(
            aeLocation,
            aeJustify,
            "Target: '%s' (%08X) - Position: (%.02f,%.02f,%.02f)",
            pName,
            pTargetRef->iFormID,
            kLocation.x,
            kLocation.y,
            kLocation.z
        );
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
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&cAnimating), sizeof(cAnimating), 0);
}

// The target reference's form ID goes into pTargetRef, resolved later.
void CombatProcedureActivateObject::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    CombatProcedure::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadFormID(pTargetRef);
    pBoundObject = dynamic_cast<TESBoundObject*>(TESForm::GetFormByNumericID(apLoadGameBuffer->LoadFormID()));
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&cAnimating), sizeof(cAnimating));
}

void CombatProcedureActivateObject::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESObjectREFR* pRef = nullptr;
    if (pTargetRef) {
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTargetRef)));
        pRef = dynamic_cast<TESObjectREFR*>(pForm);
    }
    pTargetRef = pRef;
    if (pTargetRef) {
        pTargetRef->SetTargeted(true);
    }
}

CombatProcedure::~CombatProcedure() {}
