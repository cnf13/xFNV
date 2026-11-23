#include "Fallout/AI/ActorPackage.h"

#include "BSCore/RTTI.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/ActorPackageData.h"
#include "Fallout/AI/DialoguePackage.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/Procedure.h"
#include "Fallout/Misc/Calendar.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadDefines.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "SDK/MSVC/rtti.h"

ActorPackage::ActorPackage() {
    pPackage = nullptr;
    pData = nullptr;
    pTarget = nullptr;
    iCurrentProcedureIndex = 0xFFFFFFFF;
    fPackageStartTime = -1.0f;
    iFlags = 0;
}

ActorPackage::~ActorPackage() {
    if (pData) {
        delete pData;
    }
}

void ActorPackage::InitLoadGameTES(int32_t aiFlags, int32_t aiOldFlags) {
    PACKAGE_PROCEDURE_TYPE eProcedureType;
    int32_t iProcedureCount;
    if (pPackage) {
        eProcedureType = pPackage->TESPackage::GetProcedureType();
        iProcedureCount = Procedure::GetProcedureCount(eProcedureType);
        if (iCurrentProcedureIndex >= iProcedureCount) {
            iCurrentProcedureIndex = iProcedureCount - 1;
        }
    }
}

int32_t ActorPackage::RevertTES(int32_t aiFlags) {
    return 0;
}

void ActorPackage::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    Actor* pActor = apSaveGameBuffer->GetActor();
    apSaveGameBuffer->SaveFormID(pPackage, 0);
    if (pPackage) {
        if (pDataHandler->IsFormIDCreated(pPackage->TESForm::GetFormID())) {
            int8_t cPackType = static_cast<int8_t>(pPackage->GetPackType());
            if (!pPackage->IsPackageOwner(pActor)) {
                cPackType = -1;
            }
            apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&cPackType)), sizeof(cPackType), 0);
            if (cPackType != -1) {
                pPackage->SaveGame(apSaveGameBuffer);
            }
        }
        int8_t cPackDataType;
        if (pData) {
            cPackDataType = static_cast<int8_t>(pData->GetPackageType());
        }
        else {
            cPackDataType = -1;
        }
        apSaveGameBuffer
            ->SaveDataEndian(static_cast<char*>(static_cast<void*>(&cPackDataType)), sizeof(cPackDataType), 0);
        if (cPackDataType != -1) {
            pData->SaveGame(apSaveGameBuffer);
        }
        apSaveGameBuffer->SaveDataEndian(
            static_cast<char*>(static_cast<void*>(&iCurrentProcedureIndex)),
            sizeof(iCurrentProcedureIndex),
            0
        );
        apSaveGameBuffer
            ->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fPackageStartTime)), sizeof(fPackageStartTime), 0);
        apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&iFlags)), sizeof(iFlags), 0);
        apSaveGameBuffer->SaveFormID(pTarget, 0);
    }
}

void ActorPackage::SaveGame(uint32_t aiFlags) {
    uint32_t uiPackageID = 0;
    if (pPackage) {
        uiPackageID = pPackage->iFormID;
    }
    pSaveLoadGameOLD->SaveNumericID(&uiPackageID, 4u);
    if (uiPackageID) {
        pSaveLoadGameOLD->SaveGameDataOLD(&iCurrentProcedureIndex, sizeof(iCurrentProcedureIndex));
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&fPackageStartTime, sizeof(fPackageStartTime));
}

void ActorPackage::HandleDeletedReference(TESObjectREFR* apRef) {
    if (apRef == pTarget) {
        pTarget = nullptr;
    }
}

void ActorPackage::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    char cPackType;
    char cPackDataType;
    uint32_t uiPackageID = 0;
    bool bForcedToZero = apLoadGameBuffer->LoadFormID(uiPackageID);
    if (uiPackageID || bForcedToZero) {
        bool bShared = false;
        if (pDataHandler->IsFormIDCreated(uiPackageID)) {
            cPackType = 0;
            apLoadGameBuffer->LoadData(&cPackType, sizeof(cPackType));
            if (cPackType == -1) {
                pPackage = reinterpret_cast<TESPackage*>(static_cast<uintptr_t>(uiPackageID));
                bShared = true;
            }
            else {
                pPackage = TESPackage::CreatePackage(static_cast<PACKAGE_TYPE>(cPackType));
                pSaveLoadGame->AddChangedFormID(uiPackageID, pPackage->iFormID);
                pPackage->LoadGame(apLoadGameBuffer);
            }
        }
        else {
            TESPackage* pNewPackage = nullptr;
            if (uiPackageID) {
                TESForm* pForm = TESForm::GetFormByNumericID(uiPackageID);
                pNewPackage = dynamic_cast<TESPackage*>(pForm);
            }
            pPackage = pNewPackage;
        }
        cPackDataType = -1;
        apLoadGameBuffer->LoadData(&cPackDataType, sizeof(cPackDataType));
        if (cPackDataType == -1) {
            ActorPackageData* pData = this->pData;
            if (pData) {
                delete pData;
            }
            this->pData = nullptr;
        }
        else {
            this->pData = ActorPackageData::CreateActorPackageData(static_cast<PACKAGE_TYPE>(cPackDataType));
            this->pData->LoadGame(apLoadGameBuffer);
        }
        apLoadGameBuffer->LoadData(&iCurrentProcedureIndex, sizeof(iCurrentProcedureIndex));
        apLoadGameBuffer->LoadData(&fPackageStartTime, sizeof(fPackageStartTime));
        apLoadGameBuffer->LoadData(&iFlags, sizeof(iFlags));
        apLoadGameBuffer->LoadFormID(pTarget);
        if (bShared) {
            SetFlag(ActorPackage::SAVELOAD_SHARED_PACKAGE, true);
        }
    }
    if (bForcedToZero) {
        Clear();
    }
}

void ActorPackage::InitLoadGameBGS(BGSLoadFormBuffer* apBuffer) {
    if (GetFlag(ActorPackage::SAVELOAD_SHARED_PACKAGE)) {
        uint32_t uiPackageID = pSaveLoadGame->GetChangedFormID(
            static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pPackage))
        );
        TESPackage* pNewPackage = nullptr;
        if (uiPackageID) {
            TESForm* pForm = TESForm::GetFormByNumericID(uiPackageID);
            pNewPackage = dynamic_cast<TESPackage*>(pForm);
        }
        pPackage = pNewPackage;
    }
    else if (pPackage) {
        if (pDataHandler->IsFormIDCreated(pPackage->iFormID)) {
            pPackage->InitLoadGame(apBuffer);
        }
    }
    if (pPackage) {
        if (pPackage->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_NONE) {
            pPackage->CalculateProcedureType(nullptr);
        }
        pPackage->IncRefCount();
        int32_t iRange = Procedure::GetProcedureCount(pPackage->ePROCEDURE_TYPE);
        if (iCurrentProcedureIndex >= iRange) {
            iCurrentProcedureIndex = iRange - 1;
        }
    }
    if (pData) {
        pData->InitLoadGame(apBuffer);
    }
    TESObjectREFR* pNewTarget = nullptr;
    if (pTarget) {
        TESForm* pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTarget)));
        pNewTarget = dynamic_cast<TESObjectREFR*>(pForm);
    }
    pTarget = pNewTarget;
    if (pTarget) {
        pTarget->SetTargeted(true);
    }
}

void ActorPackage::Copy(ActorPackage* apDestination) {
    apDestination->pPackage = pPackage;
    apDestination->pData = pData;
    apDestination->iCurrentProcedureIndex = iCurrentProcedureIndex;
    apDestination->fPackageStartTime = fPackageStartTime;
    pPackage = nullptr;
    pData = nullptr;
    iCurrentProcedureIndex = 0xFFFFFFFF;
    fPackageStartTime = -1.0f;
}

void ActorPackage::InitFromPackage(TESPackage* apPackage, TESObjectREFR* apTarget) {
    if (apPackage == pPackage) {
        return;
    }

    Clear();

    if (apPackage == nullptr) {
        return;
    }

    apPackage->IncRefCount();
    fPackageStartTime = appCalendar.GetHour();
    pPackage = apPackage;
    pData = ActorPackageData::CreateActorPackageData(apPackage->GetPackType());
    iCurrentProcedureIndex = 0;
    pTarget = apTarget;
}

void ActorPackage::Clear() {
    if (pPackage) {
        if (pPackage == pPlayer->pClosestConversation) {
            pPlayer->pClosestConversation = nullptr;
        }
        if (pPackage->DecRefCount() || !pPackage->GetIsCreated()) {
            pPackage = nullptr;
        }
        else {
            TESPackage* pOldPackage = pPackage;
            pPackage = nullptr;
            if (pOldPackage) {
                delete pOldPackage;
            }
        }
    }
    if (this->pData) {
        ActorPackageData* pData = this->pData;
        if (pData) {
            delete pData;
        }
        this->pData = nullptr;
    }
    fPackageStartTime = -1.0f;
    iCurrentProcedureIndex = -1;
    iFlags = 0;
    pTarget = nullptr;
}

void ActorPackage::SetFlag(ACTOR_PACKAGE_FLAG aeFlag, bool abValue) {
    if (abValue) {
        iFlags = (1 << aeFlag) | iFlags;
    }
    else {
        iFlags = ~(1 << aeFlag) & iFlags;
    }
}

bool ActorPackage::GetFlag(ACTOR_PACKAGE_FLAG aeFlag) {
    return ((1 << aeFlag) & iFlags) != 0;
}

void ActorPackage::SetTarget(TESObjectREFR* apTarget) {
    pTarget = apTarget;
}

void ActorPackage::NullInstanceData() {
    pData = nullptr;
}

void ActorPackage::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    if (GetFlag(ActorPackage::SAVELOAD_SHARED_PACKAGE)) {
        SetFlag(ActorPackage::SAVELOAD_SHARED_PACKAGE, false);
    }
    else if (pPackage) {
        if (pDataHandler->IsFormIDCreated(pPackage->iFormID)) {
            pPackage->FinishLoadGame(apLoadGameBuffer);
        }
    }
    if (this->pData && pPackage) {
        PACKAGE_TYPE ePackType = pPackage->GetPackType();
        if (ePackType != this->pData->GetPackageType()) {
            const char* pcNewType = packageTypeToString[pPackage->GetPackType()];
            const char* pcOldType = packageTypeToString[this->pData->GetPackageType()];
            int32_t iPackageFormID = pPackage->iFormID;
            SaveGameWarning(
                "Package '%s' (%08X) changed type from %s to %s, so the actor package data was "
                "replaced",
                pPackage->GetFormEditorID(),
                iPackageFormID,
                pcOldType,
                pcNewType
            );
            ActorPackageData* pData = this->pData;
            if (pData) {
                delete pData;
            }
            this->pData = ActorPackageData::CreateActorPackageData(pPackage->GetPackType());
        }
    }
}

void ActorPackage::RevertBGS(BGSLoadFormBuffer* apLoadGameBuffer) {
    Clear();
}

uint16_t ActorPackage::GetSaveSize(uint32_t aiFlags) {
    int16_t sSize = 4;
    if (pPackage) {
        sSize = 8;
    }
    return sSize + 4;
}

// aiFlags is tested against a zero mask, so the branch that rebuilds a created package from the save
// (IsFormIDCreated, TESSaveLoadGame::CreatePackage) never runs and is left out; aiCurrentFlags is unused.
void ActorPackage::LoadGameTES(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    uint32_t uiPackageID;
    bool bForcedToZero = pSaveLoadGameOLD->LoadNumericID(&uiPackageID, 4u);
    Clear();
    bool bNewPackageNeeded = false;
    if (uiPackageID) {
        TESForm* pForm = TESForm::GetFormByNumericID(uiPackageID);
        TESPackage* pNewPackage = dynamic_cast<TESPackage*>(pForm);
        InitFromPackage(pNewPackage, nullptr);
        if (pPackage) {
            pPackage->CalculateProcedureType(pTarget);
        }
        else {
            bNewPackageNeeded = true;
        }
        pSaveLoadGameOLD->LoadGameDataOLD(&iCurrentProcedureIndex, sizeof(iCurrentProcedureIndex));
    }
    else if (bForcedToZero) {
        pSaveLoadGameOLD->AdvanceBuffer(4u);
        iCurrentProcedureIndex = 0;
        bNewPackageNeeded = true;
    }
    if (bNewPackageNeeded) {
        if (uiPackageID) {
            Warning("AI: Couldn't find package %08X while loading and will try to choose a new one.", uiPackageID);
        }
        else {
            Warning("AI: Couldn't find a plugin package while loading and will try to choose a new one.");
        }
        iCurrentProcedureIndex = -1;
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&fPackageStartTime, sizeof(fPackageStartTime));
}
