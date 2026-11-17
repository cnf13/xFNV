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
    PACKAGE_PROCEDURE_TYPE parent;
    int32_t procedureCount;
    if (pPackage) {
        parent = pPackage->TESPackage::GetProcedureType();
        procedureCount = Procedure::GetProcedureCount(static_cast<PACKAGE_PROCEDURE_TYPE>((int32_t)parent));
        if (iCurrentProcedureIndex >= procedureCount) {
            iCurrentProcedureIndex = (uint32_t)(procedureCount - 1);
        }
    }
}

int32_t ActorPackage::RevertTES(int32_t a1) {
    return 0;
}

void ActorPackage::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    uint32_t v2;
    int8_t v3;
    int8_t apData;
    int8_t v6;
    Actor* v7 = apSaveGameBuffer->GetActor();
    apSaveGameBuffer->SaveFormID(pPackage, (uint32_t)0);
    if (pPackage) {
        v2 = pPackage->TESForm::GetFormID();
        if (pDataHandler->IsFormIDCreated(v2)) {
            apData = (int8_t)pPackage->GetPackType();
            if (!pPackage->IsPackageOwner(v7)) {
                apData = (-1);
            }
            apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&apData)), 1u, 0);
            if (apData != (-1)) {
                pPackage->SaveGame(apSaveGameBuffer);
            }
        }
        if (pData) {
            v3 = (int8_t)pData->GetPackageType();
        }
        else {
            v3 = (-1);
        }
        v6 = v3;
        apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&v6)), 1u, 0);
        if (v6 != (-1)) {
            pData->SaveGame(apSaveGameBuffer);
        }
        apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&iCurrentProcedureIndex)), 4u, 0);
        apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&fPackageStartTime)), 4u, 0);
        apSaveGameBuffer->SaveDataEndian(static_cast<char*>(static_cast<void*>(&iFlags)), 4u, 0);
        apSaveGameBuffer->SaveFormID(pTarget, (uint32_t)0);
    }
}

void ActorPackage::SaveGame(uint32_t aiFlags) {
    uint32_t ipackageID;
    ipackageID = 0;
    if (pPackage) {
        ipackageID = pPackage->iFormID;
    }
    pSaveLoadGameOLD->SaveNumericID(&ipackageID, 4u);
    if (ipackageID) {
        pSaveLoadGameOLD->SaveGameDataOLD(&iCurrentProcedureIndex, 4u);
    }
    pSaveLoadGameOLD->SaveGameDataOLD(&fPackageStartTime, 4u);
}

void ActorPackage::HandleDeletedReference(TESObjectREFR* apRef) {
    if (apRef == pTarget) {
        pTarget = nullptr;
    }
}

void ActorPackage::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    char cpackType;
    char cpackDataType;
    uint32_t ipackageID = 0;
    bool bforcedToZero = apLoadGameBuffer->LoadFormID(ipackageID);
    if (ipackageID || bforcedToZero) {
        bool bshared = false;
        if (pDataHandler->IsFormIDCreated(ipackageID)) {
            cpackType = 0;
            apLoadGameBuffer->LoadData(&cpackType, 1u);
            if (cpackType == -1) {
                pPackage = reinterpret_cast<TESPackage*>(static_cast<uintptr_t>(ipackageID));
                bshared = true;
            }
            else {
                pPackage = TESPackage::CreatePackage(static_cast<PACKAGE_TYPE>(cpackType));
                uint32_t CastingType = pPackage->iFormID;
                pSaveLoadGame->AddChangedFormID(ipackageID, CastingType);
                pPackage->LoadGame(apLoadGameBuffer);
            }
        }
        else {
            TESPackage* v4;
            if (ipackageID) {
                TESForm* FormByNumericID = TESForm::GetFormByNumericID(ipackageID);
                v4 = dynamic_cast<TESPackage*>(FormByNumericID);
            }
            else {
                v4 = nullptr;
            }
            pPackage = v4;
        }
        cpackDataType = -1;
        apLoadGameBuffer->LoadData(&cpackDataType, 1u);
        if (cpackDataType == -1) {
            ActorPackageData* pData = this->pData;
            if (pData) {
                delete pData;
            }
            this->pData = nullptr;
        }
        else {
            this->pData = ActorPackageData::CreateActorPackageData(static_cast<PACKAGE_TYPE>(cpackDataType));
            this->pData->LoadGame(apLoadGameBuffer);
        }
        apLoadGameBuffer->LoadData(&iCurrentProcedureIndex, 4u);
        apLoadGameBuffer->LoadData(&fPackageStartTime, 4u);
        apLoadGameBuffer->LoadData(&iFlags, 4u);
        apLoadGameBuffer->LoadFormID(pTarget);
        if (bshared) {
            SetFlag(ActorPackage::SAVELOAD_SHARED_PACKAGE, true);
        }
    }
    if (bforcedToZero) {
        Clear();
    }
}

void ActorPackage::InitLoadGameBGS(BGSLoadFormBuffer* apBuffer) {
    TESObjectREFR* v6;
    if (GetFlag(ActorPackage::SAVELOAD_SHARED_PACKAGE)) {
        TESPackage* v7;
        uint32_t ipackageID =
            pSaveLoadGame->GetChangedFormID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pPackage)));
        if (ipackageID) {
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(ipackageID);
            v7 = dynamic_cast<TESPackage*>(FormByNumericID);
        }
        else {
            v7 = nullptr;
        }
        pPackage = v7;
    }
    else {
        if (pPackage) {
            uint32_t CastingType = pPackage->iFormID;
            if (pDataHandler->IsFormIDCreated(CastingType)) {
                pPackage->InitLoadGame(apBuffer);
            }
        }
    }
    if (pPackage) {
        if (pPackage->ePROCEDURE_TYPE == PACKAGE_PROCEDURE_NONE) {
            pPackage->CalculateProcedureType(nullptr);
        }
        pPackage->IncRefCount();
        PACKAGE_PROCEDURE_TYPE Count = pPackage->ePROCEDURE_TYPE;
        int32_t irange = Procedure::GetProcedureCount(Count);
        if (iCurrentProcedureIndex >= irange) {
            iCurrentProcedureIndex = irange - 1;
        }
    }
    if (pData) {
        pData->InitLoadGame(apBuffer);
    }
    if (pTarget) {
        TESForm* v5 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTarget)));
        v6 = dynamic_cast<TESObjectREFR*>(v5);
    }
    else {
        v6 = nullptr;
    }
    pTarget = v6;
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
            TESPackage* apForm = pPackage;
            pPackage = nullptr;
            if (apForm) {
                delete apForm;
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
    else {
        if (pPackage) {
            uint32_t CastingType = pPackage->iFormID;
            if (pDataHandler->IsFormIDCreated(CastingType)) {
                pPackage->FinishLoadGame(apLoadGameBuffer);
            }
        }
    }
    if (this->pData) {
        if (pPackage) {
            PACKAGE_TYPE PackType = pPackage->GetPackType();
            if (PackType != this->pData->GetPackageType()) {
                const char* v6 = packageTypeToString[pPackage->GetPackType()];
                const char* v5 = packageTypeToString[this->pData->GetPackageType()];
                int32_t v4 = pPackage->iFormID;
                SaveGameWarning(
                    "Package '%s' (%08X) changed type from %s to %s, so the actor package data was "
                    "replaced",
                    pPackage->GetFormEditorID(), v4, v5, v6
                );
                ActorPackageData* pData = this->pData;
                if (pData) {
                    delete pData;
                }
                this->pData = ActorPackageData::CreateActorPackageData(pPackage->GetPackType());
            }
        }
    }
}

void ActorPackage::RevertBGS(BGSLoadFormBuffer* a2) {
    Clear();
}

uint16_t ActorPackage::GetSaveSize(uint32_t aiFlags) {
    int16_t v3 = 4;
    if (pPackage) {
        v3 = 8;
    }
    return v3 + 4;
}

// a2 is tested against a zero mask, so the branch that rebuilds a created package from the save (IsFormIDCreated,
// TESSaveLoadGame::CreatePackage) never runs and is left out; a3 is unused.
void ActorPackage::LoadGameTES(uint32_t a2, uint32_t a3) {
    uint32_t ipackageID;
    bool bforcedToZero = pSaveLoadGameOLD->LoadNumericID(&ipackageID, 4u);
    Clear();
    bool bnewPackageNeeded = false;
    if (ipackageID) {
        TESForm* pForm = TESForm::GetFormByNumericID(ipackageID);
        TESPackage* pNewPackage = dynamic_cast<TESPackage*>(pForm);
        InitFromPackage(pNewPackage, nullptr);
        if (pPackage) {
            pPackage->CalculateProcedureType(pTarget);
        }
        else {
            bnewPackageNeeded = true;
        }
        pSaveLoadGameOLD->LoadGameDataOLD(&iCurrentProcedureIndex, 4u);
    }
    else {
        if (bforcedToZero) {
            pSaveLoadGameOLD->AdvanceBuffer(4u);
            iCurrentProcedureIndex = 0;
            bnewPackageNeeded = true;
        }
    }
    if (bnewPackageNeeded) {
        if (ipackageID) {
            Warning("AI: Couldn't find package %08X while loading and will try to choose a new one.", ipackageID);
        }
        else {
            Warning("AI: Couldn't find a plugin package while loading and will try to choose a new one.");
        }
        iCurrentProcedureIndex = -1;
    }
    pSaveLoadGameOLD->LoadGameDataOLD(&fPackageStartTime, 4u);
}
