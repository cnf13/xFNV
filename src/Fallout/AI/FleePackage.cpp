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
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// The door EvaluateReferenceforTeleportDoor found.
static TESObjectREFR* pstaticTeleportDoor;

// Not read or written by any code.
static uint32_t data_11F2F14;

// Nothing reads or writes pstaticActor.
static Actor* pstaticActor;

FleePackage::FleePackage(TESObjectREFR* apActor, TESObjectCELL* pcell, TESObjectREFR* apRef)
  : bCombatMode(true), kFleePoint(NiPoint3_ZERO), fFleeTimer(-1.0f), bFleeSucceeded(0), pTeleportDoor(nullptr),
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
    apSaveGameBuffer->SaveDataEndian(&bEvaluatePoint, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bCombatMode, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&kFleePoint, 12u, 0);
    apSaveGameBuffer->SaveDataEndian(&fFleeTimer, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&bFleeSucceeded, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&bknowstarget, 1u, 0);
    apSaveGameBuffer->SaveDataEndian(&busehorse, 1u, 0);
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
    apLoadGameBuffer->LoadData(&bEvaluatePoint, 1u);
    apLoadGameBuffer->LoadData(&bCombatMode, 1u);
    apLoadGameBuffer->LoadData(&kFleePoint, 12u);
    apLoadGameBuffer->LoadData(&fFleeTimer, 4u);
    apLoadGameBuffer->LoadData(&bFleeSucceeded, 1u);
    apLoadGameBuffer->LoadData(&bknowstarget, 1u);
    apLoadGameBuffer->LoadData(&busehorse, 1u);
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
    TESObjectREFR* v6;
    TESObjectREFR* v7;
    TESPackage::InitLoadGame(apLoadGameBuffer);
    if (pTeleportDoor) {
        TESForm* FormByNumericID =
            TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTeleportDoor)));
        v7 = dynamic_cast<TESObjectREFR*>(FormByNumericID);
    }
    else {
        v7 = nullptr;
    }
    pTeleportDoor = v7;
    if (pRefFleeTo) {
        TESForm* v3 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pRefFleeTo)));
        v6 = dynamic_cast<TESObjectREFR*>(v3);
    }
    else {
        v6 = nullptr;
    }
    pRefFleeTo = v6;
    BSSimpleList<TESObjectREFR*>* plist = &kAvoidRefs;
    BSSimpleList<TESObjectREFR*>* plastList = nullptr;
    while (plist && !plist->IsEmpty()) {
        TESObjectREFR* pForm = plist->m_item;
        bool bremove = false;
        if (pForm) {
            TESForm* v4 = TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pForm)));
            pForm = dynamic_cast<TESObjectREFR*>(v4);
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
                BSSimpleList<TESObjectREFR*>* v5 = plist;
                plastList->Remove(v5->m_item);
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
    uint16_t SaveSize = TESPackage::GetSaveSize();
    uint16_t v5 = SaveSize;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        SaveSize += 6;
    }
    uint16_t isize = SaveSize + 4 * kAvoidRefs.ItemsInList() + 31;
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(pheader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s", isize - v5,
                pheader->iFormID, FormByNumericID->GetFormEditorID(), pheader->iFlags, 567,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\FleePackage.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s", isize - v5, 567,
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
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, 4u);
        pblockSize = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, 2u);
    }
    icount = 0;
    uint16_t* pcount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
    pSaveLoadGameOLD->SaveGameDataOLD(&icount, 2u);
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
    SaveGameDataOLD(&bCombatMode, 1);
    SaveGameDataOLD(&bknowstarget, 1);
    SaveGameDataOLD(&bFleeSucceeded, 1);
    SaveGameDataOLD(&fFleeTimer, 4);
    SaveGameDataOLD(&kFleePoint, 12);
    SaveGameDataOLD(&bEvaluatePoint, 1);
    SaveGameDataOLD(&busehorse, 1);
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* v1 = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(BufferPointer - itempStartPosition), pheader->iFormID, v1, iFlags, 618,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\FleePackage.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(BufferPointer - itempStartPosition), 618,
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
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\FleePackage.cpp", 618
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
        pSaveLoadGameOLD->LoadGameDataOLD(&iMarker, 4);
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
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\FleePackage.cpp", 626, pHeader->iFormID,
                    pEditorID, cVersion, iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  "
                    "Current version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\FleePackage.cpp", 626,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        pBlockStart = static_cast<char*>(pSaveLoadGameOLD->GetBufferPointer());
        pSaveLoadGameOLD->LoadGameDataOLD(&sBlockSize, 2);
    }
    uint16_t sAvoidCount;
    TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&sAvoidCount), 2);
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
    TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&bCombatMode), 1);
    TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&bknowstarget), 1);
    TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&bFleeSucceeded), 1);
    TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&fFleeTimer), 4);
    TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&kFleePoint), 12);
    TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&bEvaluatePoint), 1);
    // The zero-extended version is compared signed.
    if (static_cast<int32_t>(pSaveLoadGameOLD->GetCurrentVersion()) >= 0x71) {
        TESForm::LoadGameDataOLD(reinterpret_cast<double*>(&busehorse), 1);
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
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\FleePackage.cpp", 666, pHeader->iFormID,
                    pEditorID, cVersion, iFlags
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
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\FleePackage.cpp", 666, pHeader->iFormID,
                    pEditorID, cVersion, iFlags
                );
            }
        }
        else if (pBlockEnd > pBlockStart + sBlockSize) {
            uint8_t cVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(pBlockEnd - (pBlockStart + sBlockSize)),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\FleePackage.cpp", 666, cVersion
            );
        }
        else if (pBlockEnd < pBlockStart + sBlockSize) {
            uint8_t cVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>((pBlockStart + sBlockSize) - pBlockEnd),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Ai\\FleePackage.cpp", 666, cVersion
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
            TESForm* FormByNumericID =
                TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(psavedID)));
            pactor = dynamic_cast<TESObjectREFR*>(FormByNumericID);
        }
        if (pactor) {
            plist->SetItem(pactor);
            plastList = plist;
            plist = plist->m_pkNext;
        }
        else {
            if (plastList) {
                plastList->Remove(psavedID);
                plist = plastList->m_pkNext;
            }
            else {
                plist->RemoveHead();
            }
        }
    }
    uint32_t iactorRunToID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pRefFleeTo));
    if (iactorRunToID) {
        TESForm* v2 = TESForm::GetFormByNumericID(iactorRunToID);
        pRefFleeTo = dynamic_cast<Actor*>(v2);
    }
    uint32_t iteleportDoorID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTeleportDoor));
    if (iteleportDoorID) {
        TESForm* v3 = TESForm::GetFormByNumericID(iteleportDoorID);
        pTeleportDoor = dynamic_cast<TESObjectREFR*>(v3);
    }
}

void FleePackage::ModFleeTimer(float aftime) {
    fFleeTimer = (fFleeTimer + aftime);
}

void FleePackage::SetKnowsTarget(bool abflag) {
    bknowstarget = (bool)((uint8_t)abflag);
}

void FleePackage::UpdateAvoidedRefList(BaseProcess* apProcess) {
    BSSimpleList<TESObjectREFR*>* plist = &kAvoidRefs;
    float fforgetTime = fFleeIsSafeTimer.Float();
    while (plist && plist->m_item) {
        TESObjectREFR* pref = plist->m_item;
        Actor* pact = pref->IsActor() ? static_cast<Actor*>(pref) : nullptr;
        bool v4 = pref->IsDead(false) || pref->GetDelete();
        bool bdelete = v4;
        if ((!v4) && (pact) && (bCombatMode)) {
            bool v2 = true;
            if (pact->IsInCombat() || pact == pPlayer) {
                double Timer;
                if (!apProcess ||
                    (Timer = CombatTimer::GetTimer(),
                     !(fforgetTime < Timer - apProcess->GetLastTimeDetected(pact, 0)))) {
                    v2 = false;
                }
            }
            bdelete = v2;
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
    Actor* a2;
    float distanceFromReference;
    if ((!bEvaluatePoint) || (!apFromRef->GetRefPersists())) {
        return nullptr;
    }
    BSSimpleList<TESObjectREFR*>* ecx0 = &kAvoidRefs;
    float v7 = 0x1.fffffe0000000p+127f;
    TESObjectREFR* v8 = nullptr;
    while (ecx0 && ecx0->m_item) {
        a2 = static_cast<Actor*>(ecx0->m_item);
        ecx0 = ecx0->m_pkNext;
        distanceFromReference = a2->GetDistanceFromReference(apFromRef, false, false);
        if (a2->GetDisabled() || a2->IsDead(false)) {
            RemoveAvoidedRef(a2);
            ecx0 = &kAvoidRefs;
        }
        else {
            if (distanceFromReference < ((double)v7)) {
                v7 = distanceFromReference;
                v8 = a2;
            }
        }
    }
    return v8;
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
        const NiPoint3* v6 = &apActor->GetLocationOnReference();
        const NiPoint3* v5 = &apActor->GetLocationOnReference();
        TESObjectCELL* ParentCell = apActor->GetParentCell();
        pDataHandler->EnumReferencesCloseToPoint(
            ParentCell, *v5, 4000.0f, *v6, 4000.0f, &FleePackage::EvaluateReferenceforTeleportDoor, apActor
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
    PathingAvoidNode v10;
    void* apMemory = NiMemObject::operator new(sizeof(PathingAvoidNodeArray));
    if (apMemory) {
        pavoidNodeArray = ::new (apMemory) PathingAvoidNodeArray;
    }
    else {
        pavoidNodeArray = nullptr;
    }
    if (GetPackageTarget()) {
        favoidNodeRadius = static_cast<float>(GetPackageTarget()->iValue);
    }
    else {
        favoidNodeRadius = 512.0f;
    }
    float favoidNodeRadius_2 = BSMAX(favoidNodeRadius, 256.0f);
    for (TESObjectREFR* pref : kAvoidRefs) {
        if ((pref) && (!pref->GetDisabled() && !pref->IsDead(false))) {
            int32_t nodeArrayIndex = pavoidNodeArray->iSize;
            pavoidNodeArray->SetSize(nodeArrayIndex + 1, true);
            float v6 = favoidNodeRadius_2 * favoidNodeRadius_2;
            float favoidNodeCost = BSMAX(v6, 100.0f);
            float a2 = favoidNodeCost;
            const NiPoint3* v1 = &pref->GetLocationOnReference();
            PathingAvoidNode* v2 = XFNV_RECONSTRUCT(PathingAvoidNode)(&v10, *v1, favoidNodeRadius_2, a2);
            pavoidNodeArray->GetAt(nodeArrayIndex) = *v2;
        }
    }
    return pavoidNodeArray;
}

char FleePackage::FleeSucceeded() {
    return (int8_t)bFleeSucceeded;
}

bool FleePackage::IsFleeListEmpty() {
    return kAvoidRefs.IsEmpty();
}

void FleePackage::RemoveAvoidedRef(TESObjectREFR* apRef) {
    PackageTarget* packageTarget;
    PackageTarget* v3;
    PackageLocation* v4;
    PackageTarget* v5;
    PackageLocation* location;
    TESObjectREFR* v7;
    TESObjectREFR* v8;
    (&kAvoidRefs)->Remove(apRef);
    Actor* targReference = nullptr;
    if (TESPackage::GetPackageTarget()) {
        packageTarget = TESPackage::GetPackageTarget();
        targReference = static_cast<Actor*>(packageTarget->GetTargReference());
    }
    if ((targReference) && (static_cast<const void*>(targReference) == static_cast<const void*>(apRef))) {
        bool vCondition1;
        vCondition1 = (&kAvoidRefs)->IsEmpty();
        if (vCondition1) {
            v5 = TESPackage::GetPackageTarget();
            v5->SetTargReference(nullptr);
            location = TESPackage::GetLocation();
            location->SetLocReference(nullptr);
        }
        else {
            v7 = (&kAvoidRefs)->m_item;
            v3 = TESPackage::GetPackageTarget();
            v3->SetTargReference(v7);
            v8 = (&kAvoidRefs)->m_item;
            v4 = TESPackage::GetLocation();
            v4->SetLocReference(v8);
        }
    }

    if (static_cast<const void*>(apRef) == static_cast<const void*>(pRefFleeTo)) {
        pRefFleeTo = nullptr;
    }
}

bool FleePackage::ShouldShutDown() {
    uint8_t abAreAllCombatTargetsSearching;
    uint8_t result = 0;
    bool vCondition1;
    vCondition1 = (&kAvoidRefs)->ItemsInList() == 1;
    if (vCondition1) {
        abAreAllCombatTargetsSearching = 0;
        if ((static_cast<const void*>((&kAvoidRefs)->m_item) == static_cast<const void*>(pPlayer)) &&
            (!pPlayer->IsPlayerCharacterInCombat((bool*)(&abAreAllCombatTargetsSearching)))) {
            return true;
        }
    }
    return (bool)result;
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
    float fdistance;
    if (!apRef || (apRef->GetDelete() || (apRef->GetTemporary() || apRef->GetDisabled()))) {
        return false;
    }
    PACKAGE_OBJECT_TYPE eUseItem = POTYPE_NONE;
    Actor* pactor = static_cast<Actor*>(aiFilterData);
    if (!aiFilterData) {
        return false;
    }
    TESObjectDOOR* ObjectReference = static_cast<TESObjectDOOR*>(apRef->GetObjectReference());
    if (ObjectReference == pJailMarker) {
        return false;
    }
    if (!apRef->GetTeleport()) {
        return false;
    }
    if (pcombatActor && pcombatActor->GetInterior()) {
        fdistance = 128.0f;
    }
    else {
        fdistance = 800.0f;
    }
    float fourDistFromDoor = apRef->GetDistanceFromReference(pactor, false, false);
    float fcombatActorDistFromDoor = apRef->GetDistanceFromReference(pcombatActor, false, false);
    if (!(fdistance < static_cast<double>(fcombatActorDistFromDoor)) &&
        !(fourDistFromDoor < static_cast<double>(fcombatActorDistFromDoor))) {
        return false;
    }
    LockData* pLock = apRef->GetLock();
    if (pLock &&
        (pLock->IsLocked() &&
         (!apRef->IsAnOwner(pactor, true) &&
          (!pLock->pKey || !pactor->HasObjects(pLock->pKey, POTYPE_NONE, 1, 0, eUseItem))))) {
        return false;
    }
    PathingLocation actorLoc(pactor);
    const NiPoint3* LinkedDoorTeleportPosition = &apRef->GetLinkedDoorTeleportPosition();
    PathingLocation doorLoc(*LinkedDoorTeleportPosition, apRef);
    if (Pathing::CanCreatePath(actorLoc, doorLoc, nullptr)) {
        pstaticTeleportDoor = apRef;
        return true;
    }
    return false;
}
