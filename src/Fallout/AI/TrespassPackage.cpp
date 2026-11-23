#include "Fallout/AI/TrespassPackage.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/PackageTarget.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"

TrespassPackage::TrespassPackage()
  : fwarningtimer(0.0f), iwarning(0), pOwner(nullptr), pObjectOwned(nullptr), iCrimeNumber(static_cast<uint32_t>(-1)),
    iwarninglevel(3) {}

TrespassPackage::TrespassPackage(Actor* apCriminal, TESForm* apOwner, TESObjectCELL* OpenSound, int32_t auiWarningLevel)
  : fwarningtimer(0.0f), iwarning(0), pOwner(apOwner), pObjectOwned(OpenSound), iCrimeNumber(static_cast<uint32_t>(-1)),
    iwarninglevel(auiWarningLevel), iNumberActors(0) {
    SetPackType(PACKAGE_TYPE_TRESPASS);
    SetMustReachLocation(true);
    SetMustComplete(true);
    // SetPackageLocation and SetPackageTarget copy the temporaries.
    PackageLocation* pLoc = new PackageLocation();
    pLoc->SetLocReference(apCriminal);
    SetPackageLocation(pLoc);
    delete pLoc;
    PackageTarget* pNew = new PackageTarget();
    SetPackageTarget(pNew);
    delete pNew;
    ePROCEDURE_TYPE = PACKAGE_PROCEDURE_TRESPASS;
    GetPackageTarget()->SetTargType(PTT_REFERENCE);
    GetPackageTarget()->SetTargReference(apCriminal);
    GetPackageTarget()->iValue = 128u;
}

TrespassPackage::~TrespassPackage() {
    if (pPlayer) {
        pPlayer->GetExtra()->ClearTrespassPackage(this);
    }
}

void TrespassPackage::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESPackage::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fwarningtimer), sizeof(fwarningtimer), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iwarning), sizeof(iwarning), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iCrimeNumber), sizeof(iCrimeNumber), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iwarninglevel), sizeof(iwarninglevel), 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iNumberActors), sizeof(iNumberActors), 0);
    apSaveGameBuffer->SaveFormID(pOwner, static_cast<uint32_t>(0));
    apSaveGameBuffer->SaveFormID(pObjectOwned, static_cast<uint32_t>(0));
}

void TrespassPackage::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESPackage::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&fwarningtimer, sizeof(fwarningtimer));
    apLoadGameBuffer->LoadData(&iwarning, sizeof(iwarning));
    apLoadGameBuffer->LoadData(&iCrimeNumber, sizeof(iCrimeNumber));
    apLoadGameBuffer->LoadData(&iwarninglevel, sizeof(iwarninglevel));
    apLoadGameBuffer->LoadData(&iNumberActors, sizeof(iNumberActors));
    apLoadGameBuffer->LoadFormID(pOwner);
    apLoadGameBuffer->LoadFormID(pObjectOwned);
}

void TrespassPackage::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm* pForm;
    TESPackage::InitLoadGame(apLoadGameBuffer);
    if (pOwner) {
        pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pOwner)));
    }
    else {
        pForm = nullptr;
    }
    pOwner = pForm;
    if (pObjectOwned) {
        pForm = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pObjectOwned)));
    }
    else {
        pForm = nullptr;
    }
    pObjectOwned = pForm;
}

// Three dwords of its own, two more from save version 0x6C (added one at a time), then two more.
uint16_t TrespassPackage::GetSaveSize() {
    uint16_t usSize = TESPackage::GetSaveSize();
    usSize += 12;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x6C) {
        usSize += 4;
        usSize += 4;
    }
    usSize += 8;
    return usSize;
}

void TrespassPackage::SaveGame() {
    TESPackage::SaveGame();
    SaveGameDataOLD(&fwarningtimer, sizeof(fwarningtimer));
    SaveGameDataOLD(&iwarning, sizeof(iwarning));
    SaveGameDataOLD(&iCrimeNumber, sizeof(iCrimeNumber));
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 108u) {
        SaveGameDataOLD(&iwarninglevel, sizeof(iwarninglevel));
        SaveGameDataOLD(&iNumberActors, sizeof(iNumberActors));
    }
    int32_t iOwnerID = 0;
    if (pOwner) {
        iOwnerID = pOwner->GetFormID();
    }
    SaveNumericID(reinterpret_cast<uint32_t*>(&iOwnerID), 4);
    uint32_t uiObjectOwnedID = 0;
    if (pObjectOwned) {
        uiObjectOwnedID = pObjectOwned->GetFormID();
    }
    SaveNumericID(&uiObjectOwnedID, 4);
}

void TrespassPackage::LoadGame() {
    uint32_t uiObjectOwnedID;
    uint32_t uiOwnerID;
    TESPackage::LoadGame();
    LoadGameDataOLD(&fwarningtimer, sizeof(fwarningtimer));
    LoadGameDataOLD(&iwarning, sizeof(iwarning));
    LoadGameDataOLD(&iCrimeNumber, sizeof(iCrimeNumber));
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 108u) {
        LoadGameDataOLD(&iwarninglevel, sizeof(iwarninglevel));
        LoadGameDataOLD(&iNumberActors, sizeof(iNumberActors));
    }
    LoadNumericID(&uiOwnerID, 4);
    pOwner = reinterpret_cast<TESForm*>(static_cast<uintptr_t>(uiOwnerID));
    LoadNumericID(&uiObjectOwnedID, 4);
    pObjectOwned = reinterpret_cast<TESForm*>(static_cast<uintptr_t>(uiObjectOwnedID));
}

void TrespassPackage::InitLoadGame() {
    TESPackage::InitLoadGame();
    TESForm* pFormA = pOwner;
    if (pFormA) {
        pOwner = TESForm::GetFormByNumericID(reinterpret_cast<uintptr_t>(pFormA));
    }
    TESForm* pFormB = pObjectOwned;
    if (pFormB) {
        pObjectOwned = TESForm::GetFormByNumericID(reinterpret_cast<uintptr_t>(pFormB));
    }
}

void TrespassPackage::ModNumberActors(int32_t aiNumber) {
    iNumberActors += aiNumber;
}

void TrespassPackage::ModWarningTimer(float aftime) {
    fwarningtimer = fwarningtimer + aftime;
}

void TrespassPackage::ModWarning(int32_t ainumb) {
    iwarning += ainumb;
}

void TrespassPackage::operator delete(void* apMemory) {
    ourFree(apMemory);
}
