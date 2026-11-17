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
    PackageLocation* ploc = new PackageLocation();
    ploc->SetLocReference(apCriminal);
    SetPackageLocation(ploc);
    delete ploc;
    PackageTarget* pnew = new PackageTarget();
    SetPackageTarget(pnew);
    delete pnew;
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
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&fwarningtimer), 4u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iwarning), 4u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iCrimeNumber), 4u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iwarninglevel), 4u, 0);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&iNumberActors), 4u, 0);
    apSaveGameBuffer->SaveFormID(pOwner, static_cast<uint32_t>(0));
    apSaveGameBuffer->SaveFormID(pObjectOwned, static_cast<uint32_t>(0));
}

void TrespassPackage::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESPackage::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&fwarningtimer, 4u);
    apLoadGameBuffer->LoadData(&iwarning, 4u);
    apLoadGameBuffer->LoadData(&iCrimeNumber, 4u);
    apLoadGameBuffer->LoadData(&iwarninglevel, 4u);
    apLoadGameBuffer->LoadData(&iNumberActors, 4u);
    apLoadGameBuffer->LoadFormID(pOwner);
    apLoadGameBuffer->LoadFormID(pObjectOwned);
}

void TrespassPackage::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESForm* v2;
    TESForm* FormByNumericID;
    TESPackage::InitLoadGame(apLoadGameBuffer);
    if (pOwner) {
        FormByNumericID = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pOwner)));
    }
    else {
        FormByNumericID = nullptr;
    }
    pOwner = FormByNumericID;
    if (pObjectOwned) {
        v2 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pObjectOwned)));
    }
    else {
        v2 = nullptr;
    }
    pObjectOwned = v2;
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
    uint32_t apNumericID;
    int32_t a2;
    TESPackage::SaveGame();
    SaveGameDataOLD(&fwarningtimer, 4);
    SaveGameDataOLD(&iwarning, 4);
    SaveGameDataOLD(&iCrimeNumber, 4);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 108u) {
        SaveGameDataOLD(&iwarninglevel, 4);
        SaveGameDataOLD(&iNumberActors, 4);
    }
    a2 = 0;
    if (pOwner) {
        a2 = pOwner->GetFormID();
    }
    SaveNumericID(reinterpret_cast<uint32_t*>(&a2), 4);
    apNumericID = 0;
    if (pObjectOwned) {
        apNumericID = pObjectOwned->GetFormID();
    }
    SaveNumericID(&apNumericID, 4);
}

void TrespassPackage::LoadGame() {
    uint32_t iobjectOwnedID;
    uint32_t a1;
    TESPackage::LoadGame();
    LoadGameDataOLD(&fwarningtimer, 4);
    LoadGameDataOLD(&iwarning, 4);
    LoadGameDataOLD(&iCrimeNumber, 4);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 108u) {
        LoadGameDataOLD(&iwarninglevel, 4);
        LoadGameDataOLD(&iNumberActors, 4);
    }
    LoadNumericID(&a1, 4);
    pOwner = reinterpret_cast<TESForm*>(static_cast<uintptr_t>(a1));
    LoadNumericID(&iobjectOwnedID, 4);
    pObjectOwned = reinterpret_cast<TESForm*>(static_cast<uintptr_t>(iobjectOwnedID));
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
    fwarningtimer = (fwarningtimer + aftime);
}

void TrespassPackage::ModWarning(int32_t ainumb) {
    iwarning += ainumb;
}

void TrespassPackage::operator delete(void* apMemory) {
    ourFree(apMemory);
}
