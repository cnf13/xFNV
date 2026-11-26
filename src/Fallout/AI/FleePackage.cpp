#include "Fallout/AI/FleePackage.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/AiFormulas.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/PackageTarget.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/Pathfinding/PathingAvoidNode.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESKey.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TeleportData.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatTimer.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// The door EvaluateReferenceforTeleportDoor found.
static TESObjectREFR* pstaticTeleportDoor;

FleePackage::FleePackage(TESObjectREFR* apActor, TESObjectCELL* pcell, TESObjectREFR* apRef)
  : bCombatMode(true), kFleePoint(NiPoint3::ZERO), fFleeTimer(-1.0f), bFleeSucceeded(0), pTeleportDoor(nullptr),
    pRefFleeTo(nullptr), bknowstarget(false), busehorse(false) {
    SetPackType(PACKAGE_TYPE_FLEE);
    SetMustReachLocation(true);
    SetMustComplete(true);
    bEvaluatePoint = true;
    if (apActor) {
        PackageLocation* pLocation = new PackageLocation();
        if (pcell || apRef) {
            if (apRef) {
                pLocation->SetLocReference(apRef);
            }
            else if (pcell) {
                pLocation->SetLocCell(pcell);
            }
            bEvaluatePoint = false;
        }
        else {
            pLocation->SetLocReference(apActor);
            bEvaluatePoint = true;
        }
        SetPackageLocation(pLocation);
        delete pLocation;
        PackageTarget* pTarget = new PackageTarget();
        SetPackageTarget(pTarget);
        delete pTarget;
        SetProcedureType(PACKAGE_PROCEDURE_FLEE);
        GetPackageTarget()->SetTargType(PTT_REFERENCE);
        GetPackageTarget()->SetTargReference(apActor);
    }
    SetIsCreated(true);
}

FleePackage::~FleePackage() {
    kAvoidRefs.RemoveAll();
}

void FleePackage::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESPackage::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(&bEvaluatePoint, sizeof(bEvaluatePoint), 0);
    apSaveGameBuffer->SaveDataEndian(&bCombatMode, sizeof(bCombatMode), 0);
    apSaveGameBuffer->SaveDataEndian(&kFleePoint, sizeof(kFleePoint), 0);
    apSaveGameBuffer->SaveDataEndian(&fFleeTimer, sizeof(fFleeTimer), 0);
    apSaveGameBuffer->SaveDataEndian(&bFleeSucceeded, sizeof(bFleeSucceeded), 0);
    apSaveGameBuffer->SaveDataEndian(&bknowstarget, sizeof(bknowstarget), 0);
    apSaveGameBuffer->SaveDataEndian(&busehorse, sizeof(busehorse), 0);
    apSaveGameBuffer->SaveFormID(pTeleportDoor, 0);
    apSaveGameBuffer->SaveFormID(pRefFleeTo, 0);
    uint32_t isaveGameCount = 0;
    uint32_t isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
    for (TESObjectREFR* pForm : kAvoidRefs) {
        if (pForm) {
            apSaveGameBuffer->SaveFormID(pForm, 0);
            ++isaveGameCount;
        }
    }
    apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
}

void FleePackage::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESObjectREFR* pForm;
    TESPackage::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(&bEvaluatePoint, sizeof(bEvaluatePoint));
    apLoadGameBuffer->LoadData(&bCombatMode, sizeof(bCombatMode));
    apLoadGameBuffer->LoadData(&kFleePoint, sizeof(kFleePoint));
    apLoadGameBuffer->LoadData(&fFleeTimer, sizeof(fFleeTimer));
    apLoadGameBuffer->LoadData(&bFleeSucceeded, sizeof(bFleeSucceeded));
    apLoadGameBuffer->LoadData(&bknowstarget, sizeof(bknowstarget));
    apLoadGameBuffer->LoadData(&busehorse, sizeof(busehorse));
    apLoadGameBuffer->LoadFormID(pTeleportDoor);
    apLoadGameBuffer->LoadFormID(pRefFleeTo);
    uint32_t isaveGameCount = apLoadGameBuffer->LoadVariableSizedValue();
    for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
        pForm = nullptr;
        apLoadGameBuffer->LoadFormID(pForm);
        kAvoidRefs.AddHead(pForm);
    }
}

void FleePackage::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESPackage::InitLoadGame(apLoadGameBuffer);
    TESObjectREFR* pdoor = nullptr;
    if (pTeleportDoor) {
        TESForm* pform = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTeleportDoor)));
        pdoor = dynamic_cast<TESObjectREFR*>(pform);
    }
    pTeleportDoor = pdoor;
    TESObjectREFR* pfleeTo = nullptr;
    if (pRefFleeTo) {
        TESForm* pform = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pRefFleeTo)));
        pfleeTo = dynamic_cast<TESObjectREFR*>(pform);
    }
    pRefFleeTo = pfleeTo;
    BSSimpleList<TESObjectREFR*>* plist = &kAvoidRefs;
    BSSimpleList<TESObjectREFR*>* plastList = nullptr;
    while (plist && !plist->IsEmpty()) {
        TESObjectREFR* pForm = plist->m_item;
        bool bremove = false;
        if (pForm) {
            TESForm* pform = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pForm)));
            pForm = dynamic_cast<TESObjectREFR*>(pform);
            if (pForm) {
                plist->SetItem(pForm);
            }
            else {
                bremove = true;
            }
        }
        else {
            bremove = true;
        }
        if (bremove) {
            if (plastList) {
                plastList->Remove(plist->m_item);
                plist = plastList->m_pkNext;
            }
            else {
                plist->RemoveHead();
            }
        }
        else {
            plastList = plist;
            plist = plist->m_pkNext;
        }
    }
}

uint16_t FleePackage::GetSaveSize() {
    uint16_t ipackageSize = TESPackage::GetSaveSize();
    uint16_t istartSize = ipackageSize;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        ipackageSize += 6;
    }
    uint16_t isize = ipackageSize + 4 * kAvoidRefs.ItemsInList() + 31;
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                isize - istartSize,
                pheader->iFormID,
                pform->GetFormEditorID(),
                pheader->iFlags,
                567,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\FleePackage.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                isize - istartSize,
                567,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\FleePackage.cpp"
            );
        }
    }
    return isize;
}

void FleePackage::SaveGame() {
    uint32_t irefID;
    uint32_t iblockHeader;
    uint16_t icount;
    uint32_t iteleportDoorID;
    uint32_t iactorRunToID;
    uint16_t iblockSize;
    TESPackage::SaveGame();
    iblockSize = 0;
    uint16_t* pblockSize = nullptr;
    uint8_t* itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        iblockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, sizeof(iblockHeader));
        pblockSize = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, sizeof(iblockSize));
    }
    icount = 0;
    uint16_t* pcount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
    pSaveLoadGameOLD->SaveGameDataOLD(&icount, sizeof(icount));
    for (BSSimpleList<TESObjectREFR*>* plist = &kAvoidRefs; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
        irefID = plist->m_item->iFormID;
        SaveNumericID(&irefID, 4);
        ++icount;
    }
    *pcount = icount;
    iactorRunToID = 0;
    if (pRefFleeTo) {
        iactorRunToID = pRefFleeTo->iFormID;
    }
    SaveNumericID(&iactorRunToID, 4);
    iteleportDoorID = 0;
    if (pTeleportDoor) {
        iteleportDoorID = pTeleportDoor->iFormID;
    }
    SaveNumericID(&iteleportDoorID, 4);
    SaveGameDataOLD(&bCombatMode, sizeof(bCombatMode));
    SaveGameDataOLD(&bknowstarget, sizeof(bknowstarget));
    SaveGameDataOLD(&bFleeSucceeded, sizeof(bFleeSucceeded));
    SaveGameDataOLD(&fFleeTimer, sizeof(fFleeTimer));
    SaveGameDataOLD(&kFleePoint, sizeof(kFleePoint));
    SaveGameDataOLD(&bEvaluatePoint, sizeof(bEvaluatePoint));
    SaveGameDataOLD(&busehorse, sizeof(busehorse));
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* peditorID = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(icurrentBufferPosition - itempStartPosition),
                pheader->iFormID,
                peditorID,
                iFlags,
                618,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\FleePackage.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(icurrentBufferPosition - itempStartPosition),
                618,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\FleePackage.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        char* pend = static_cast<char*>(pSaveLoadGameOLD->m_pBuffer);
        // The block size slot is the buffer position SaveGameDataOLD wrote it at: the size counts the bytes after it.
        char* pblockStart = reinterpret_cast<char*>(pblockSize);
        if (pend > pblockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\FleePackage.cpp",
                618
            );
        }
        *pblockSize = static_cast<uint16_t>(pend - pblockStart);
    }
}

// Loads the flee state from an old-format save, checking the block size like TESPackage::LoadGame. The avoided refs,
// the actor to flee to and the teleport door are stored as raw numeric form IDs here (cast into the pointer fields)
// and resolved by InitLoadGame. TESForm::LoadGameDataOLD takes a raw byte buffer, so the fields are passed as double*.
void FleePackage::LoadGame() {
    TESPackage::LoadGame();
    uint16_t sBlockSize = 0;
    char* pBlockStart = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint32_t iMarker;
        pSaveLoadGameOLD->LoadGameDataOLD(&iMarker, sizeof(iMarker));
        if (iMarker != 0x424C4F4B) {
            LoadFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlyLoadingFormHeader();
            if (pHeader) {
                TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
                uint32_t iFlags = pHeader->iFlags;
                uint8_t cVersion = pHeader->cVersion;
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                    "Currently loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\FleePackage.cpp",
                    626,
                    pHeader->iFormID,
                    pEditorID,
                    cVersion,
                    iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                    "Current version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\FleePackage.cpp",
                    626,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        pBlockStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        pSaveLoadGameOLD->LoadGameDataOLD(&sBlockSize, sizeof(sBlockSize));
    }
    uint16_t sAvoidCount;
    TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&sAvoidCount), sizeof(sAvoidCount));
    for (int32_t i = 0; i < sAvoidCount; ++i) {
        uint32_t iRefID;
        LoadNumericID(&iRefID, 4);
        uint32_t iCopy = iRefID;
        TESObjectREFR* pRef = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(iCopy));
        kAvoidRefs.AddHead(pRef);
    }
    uint32_t iFleeToID;
    LoadNumericID(&iFleeToID, 4);
    pRefFleeTo = reinterpret_cast<Actor*>(static_cast<uintptr_t>(iFleeToID));
    uint32_t iDoorID;
    LoadNumericID(&iDoorID, 4);
    pTeleportDoor = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(iDoorID));
    TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&bCombatMode), sizeof(bCombatMode));
    TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&bknowstarget), sizeof(bknowstarget));
    TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&bFleeSucceeded), sizeof(bFleeSucceeded));
    TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&fFleeTimer), sizeof(fFleeTimer));
    TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&kFleePoint), sizeof(kFleePoint));
    TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&bEvaluatePoint), sizeof(bEvaluatePoint));
    // The zero-extended version is compared signed.
    if (static_cast<int32_t>(pSaveLoadGameOLD->GetCurrentVersion()) >= 0x71) {
        TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&busehorse), sizeof(busehorse));
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        char* pBlockEnd = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        LoadFormHeader* pHeader = pSaveLoadGameOLD->GetCurrentlyLoadingFormHeader();
        if (pHeader) {
            TESForm* pForm = TESForm::GetFormByNumericID(pHeader->iFormID);
            if (pBlockEnd > pBlockStart + sBlockSize) {
                uint32_t iFlags = pHeader->iFlags;
                uint8_t cVersion = pHeader->cVersion;
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading "
                    "form is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(pBlockEnd - (pBlockStart + sBlockSize)),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\FleePackage.cpp",
                    666,
                    pHeader->iFormID,
                    pEditorID,
                    cVersion,
                    iFlags
                );
            }
            else if (pBlockEnd < pBlockStart + sBlockSize) {
                uint32_t iFlags = pHeader->iFlags;
                uint8_t cVersion = pHeader->cVersion;
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                    "form is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>((pBlockStart + sBlockSize) - pBlockEnd),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\FleePackage.cpp",
                    666,
                    pHeader->iFormID,
                    pEditorID,
                    cVersion,
                    iFlags
                );
            }
        }
        else if (pBlockEnd > pBlockStart + sBlockSize) {
            uint8_t cVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(pBlockEnd - (pBlockStart + sBlockSize)),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\FleePackage.cpp",
                666,
                cVersion
            );
        }
        else if (pBlockEnd < pBlockStart + sBlockSize) {
            uint8_t cVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>((pBlockStart + sBlockSize) - pBlockEnd),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\FleePackage.cpp",
                666,
                cVersion
            );
        }
    }
}

void FleePackage::InitLoadGame() {
    TESPackage::InitLoadGame();
    BSSimpleList<TESObjectREFR*>* plist = &kAvoidRefs;
    BSSimpleList<TESObjectREFR*>* plastList = nullptr;
    while (plist && !plist->IsEmpty()) {
        // The TESObjectREFR* entry holds a form ID (uint32_t) stored by LoadGame, not a pointer.
        TESObjectREFR* psavedID = plist->m_item;
        TESObjectREFR* pactor = psavedID;
        if (psavedID) {
            TESForm* pform = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(psavedID)));
            pactor = dynamic_cast<TESObjectREFR*>(pform);
        }
        if (pactor) {
            plist->SetItem(pactor);
            plastList = plist;
            plist = plist->m_pkNext;
        }
        else if (plastList) {
            plastList->Remove(psavedID);
            plist = plastList->m_pkNext;
        }
        else {
            plist->RemoveHead();
        }
    }
    uint32_t iactorRunToID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pRefFleeTo));
    if (iactorRunToID) {
        TESForm* pform = TESForm::GetFormByNumericID(iactorRunToID);
        pRefFleeTo = dynamic_cast<Actor*>(pform);
    }
    uint32_t iteleportDoorID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTeleportDoor));
    if (iteleportDoorID) {
        TESForm* pform = TESForm::GetFormByNumericID(iteleportDoorID);
        pTeleportDoor = dynamic_cast<TESObjectREFR*>(pform);
    }
}

void FleePackage::ModFleeTimer(float aftime) {
    fFleeTimer = fFleeTimer + aftime;
}

void FleePackage::SetKnowsTarget(bool abflag) {
    bknowstarget = abflag;
}

void FleePackage::UpdateAvoidedRefList(BaseProcess* apProcess) {
    BSSimpleList<TESObjectREFR*>* plist = &kAvoidRefs;
    float fforgetTime = fFleeIsSafeTimer.Float();
    while (plist && plist->m_item) {
        TESObjectREFR* pref = plist->m_item;
        Actor* pact = pref->IsActor() ? static_cast<Actor*>(pref) : nullptr;
        bool bdelete = pref->IsDead(false) || pref->GetDelete();
        if (!bdelete && pact && bCombatMode) {
            bool bsafe = true;
            if (pact->IsInCombat() || pact == pPlayer) {
                double dTime;
                if (!apProcess ||
                    (dTime = CombatTimer::GetTimer(),
                     !(fforgetTime < dTime - apProcess->GetLastTimeDetected(pact, 0)))) {
                    bsafe = false;
                }
            }
            bdelete = bsafe;
        }

        if (bdelete) {
            plist->RemoveHead();
            plist = &kAvoidRefs;
        }
        else {
            plist = plist->m_pkNext;
        }
    }
}

TESObjectREFR* FleePackage::GetClosestAvoidedRef(TESObjectREFR* apFromRef) {
    if (!bEvaluatePoint || !apFromRef->GetRefPersists()) {
        return nullptr;
    }
    BSSimpleList<TESObjectREFR*>* plist = &kAvoidRefs;
    float fclosestDistance = 0x1.fffffe0000000p+127f;
    TESObjectREFR* pclosestRef = nullptr;
    while (plist && plist->m_item) {
        Actor* pactor = static_cast<Actor*>(plist->m_item);
        plist = plist->m_pkNext;
        float fdistance = pactor->GetDistanceFromReference(apFromRef, false, false);
        if (pactor->GetDisabled() || pactor->IsDead(false)) {
            RemoveAvoidedRef(pactor);
            plist = &kAvoidRefs;
        }
        else if (fdistance < static_cast<double>(fclosestDistance)) {
            fclosestDistance = fdistance;
            pclosestRef = pactor;
        }
    }
    return pclosestRef;
}

bool FleePackage::FindTeleportDoor(Actor* apActor, TESObjectREFR* apThreat) {
    if (!bEvaluatePoint) {
        return false;
    }
    if (!apThreat) {
        return false;
    }
    pcombatActor = apThreat;
    if (apActor->GetRefPersists()) {
        const NiPoint3& kPoint2 = apActor->GetLocationOnReference();
        const NiPoint3& kPoint1 = apActor->GetLocationOnReference();
        TESObjectCELL* pCell = apActor->GetParentCell();
        pDataHandler->EnumReferencesCloseToPoint(
            pCell,
            kPoint1,
            4000.0f,
            kPoint2,
            4000.0f,
            &FleePackage::EvaluateReferenceforTeleportDoor,
            apActor
        );
    }
    pTeleportDoor = pstaticTeleportDoor;
    pcombatActor = nullptr;
    pstaticTeleportDoor = nullptr;
    return pTeleportDoor != nullptr;
}

PathingAvoidNodeArray* FleePackage::CreateAvoidNodeArray() {
    float favoidNodeRadius;
    PathingAvoidNodeArray* pavoidNodeArray;
    pavoidNodeArray = new PathingAvoidNodeArray;
    if (GetPackageTarget()) {
        favoidNodeRadius = static_cast<float>(GetPackageTarget()->iValue);
    }
    else {
        favoidNodeRadius = 512.0f;
    }
    favoidNodeRadius = BSMAX(favoidNodeRadius, 256.0f);
    for (TESObjectREFR* pref : kAvoidRefs) {
        if (pref && !pref->GetDisabled() && !pref->IsDead(false)) {
            int32_t nodeArrayIndex = pavoidNodeArray->iSize;
            pavoidNodeArray->SetSize(nodeArrayIndex + 1, true);
            float fradiusSquared = favoidNodeRadius * favoidNodeRadius;
            float favoidNodeCost = BSMAX(fradiusSquared, 100.0f);
            pavoidNodeArray->GetAt(
                nodeArrayIndex
            ) = PathingAvoidNode(pref->GetLocationOnReference(), favoidNodeRadius, favoidNodeCost);
        }
    }
    return pavoidNodeArray;
}

char FleePackage::FleeSucceeded() {
    return bFleeSucceeded;
}

bool FleePackage::IsFleeListEmpty() {
    return kAvoidRefs.IsEmpty();
}

void FleePackage::RemoveAvoidedRef(TESObjectREFR* apRef) {
    kAvoidRefs.Remove(apRef);
    Actor* pTargReference = nullptr;
    if (TESPackage::GetPackageTarget()) {
        pTargReference = static_cast<Actor*>(TESPackage::GetPackageTarget()->GetTargReference());
    }
    if (pTargReference && static_cast<const void*>(pTargReference) == static_cast<const void*>(apRef)) {
        if (kAvoidRefs.IsEmpty()) {
            TESPackage::GetPackageTarget()->SetTargReference(nullptr);
            TESPackage::GetLocation()->SetLocReference(nullptr);
        }
        else {
            TESObjectREFR* pFirstRef = kAvoidRefs.m_item;
            TESPackage::GetPackageTarget()->SetTargReference(pFirstRef);
            pFirstRef = kAvoidRefs.m_item;
            TESPackage::GetLocation()->SetLocReference(pFirstRef);
        }
    }

    if (static_cast<const void*>(apRef) == static_cast<const void*>(pRefFleeTo)) {
        pRefFleeTo = nullptr;
    }
}

bool FleePackage::ShouldShutDown() {
    uint8_t abAreAllCombatTargetsSearching;
    if (kAvoidRefs.ItemsInList() == 1) {
        abAreAllCombatTargetsSearching = 0;
        if (static_cast<const void*>(kAvoidRefs.m_item) == static_cast<const void*>(pPlayer) &&
            !pPlayer->IsPlayerCharacterInCombat(reinterpret_cast<bool*>(&abAreAllCombatTargetsSearching))) {
            return true;
        }
    }
    return false;
}

void FleePackage::SetEvaluatePoint(bool abEvaluatePoint) {
    bEvaluatePoint = abEvaluatePoint;
}

void FleePackage::AddAvoidedRef(TESObjectREFR* apRef) {
    if (!kAvoidRefs.IsInList(apRef)) {
        kAvoidRefs.AddHead(apRef);
    }
}

bool FleePackage::EvaluateReferenceforTeleportDoor(TESObjectREFR* apRef, void* aiFilterData) {
    if (!apRef || apRef->GetDelete() || apRef->GetTemporary() || apRef->GetDisabled()) {
        return false;
    }
    PACKAGE_OBJECT_TYPE eUseItem = POTYPE_NONE;
    Actor* pactor = static_cast<Actor*>(aiFilterData);
    if (!aiFilterData) {
        return false;
    }
    TESObjectDOOR* pdoor = static_cast<TESObjectDOOR*>(apRef->GetObjectReference());
    if (pdoor == pJailMarker) {
        return false;
    }
    if (!apRef->GetTeleport()) {
        return false;
    }
    float fdistance = pcombatActor && pcombatActor->GetInterior() ? 128.0f : 800.0f;
    float fourDistFromDoor = apRef->GetDistanceFromReference(pactor, false, false);
    float fcombatActorDistFromDoor = apRef->GetDistanceFromReference(pcombatActor, false, false);
    if (!(fdistance < static_cast<double>(fcombatActorDistFromDoor)) &&
        !(fourDistFromDoor < static_cast<double>(fcombatActorDistFromDoor))) {
        return false;
    }
    LockData* pLock = apRef->GetLock();
    if (pLock &&
        pLock->IsLocked() &&
        !apRef->IsAnOwner(pactor, true) &&
        (!pLock->pKey || !pactor->HasObjects(pLock->pKey, POTYPE_NONE, 1, 0, eUseItem))) {
        return false;
    }
    PathingLocation actorLoc(pactor);
    PathingLocation doorLoc(apRef->GetLinkedDoorTeleportPosition(), apRef);
    if (Pathing::CanCreatePath(actorLoc, doorLoc, nullptr)) {
        pstaticTeleportDoor = apRef;
        return true;
    }
    return false;
}
