#include "Fallout/AI/BackUpPackage.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/Package.h"
#include "Fallout Shared/PackageLocation.h"
#include "Fallout Shared/PackageTarget.h"
#include "Fallout Shared/Pathfinding/Pathfind.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

BackUpPackage::BackUpPackage() {}

BackUpPackage::~BackUpPackage() {}

void BackUpPackage::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    TESPackage::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&pBackUpPoint), 0xC, 0);
}

void BackUpPackage::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESPackage::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pBackUpPoint), 0xC);
}

void BackUpPackage::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESPackage::InitLoadGame(apLoadGameBuffer);
}

uint16_t BackUpPackage::GetSaveSize() {
    uint16_t SaveSize = TESPackage::GetSaveSize();
    uint16_t v4 = SaveSize;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        SaveSize += 6;
    }
    uint16_t isize = SaveSize + 12;
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* FormByNumericID = TESForm::GetFormByNumericID(pheader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s", isize - v4,
                pheader->iFormID, FormByNumericID->GetFormEditorID(), pheader->iFlags, 211,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BackUpPackage.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s", isize - v4, 211,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BackUpPackage.cpp"
            );
        }
    }
    return isize;
}

// The old-format save: with save blocks, a 'BLOK' marker and a 16-bit size slot, written back at the end; then a
// zero count and the back-up point.
void BackUpPackage::SaveGame() {
    uint32_t iblockHeader;
    uint16_t icount;
    uint16_t iblockSize;
    TESPackage::SaveGame();
    iblockSize = 0;
    // The reserved size slot in the save buffer (untyped bytes there).
    void* pblockSize = nullptr;
    uint8_t* itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    if (bDebugSaveBuffer.Bool()) {
        itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        iblockHeader = 0x424C4F4B;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, 4u);
        pblockSize = pSaveLoadGameOLD->m_pBuffer;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, 2u);
    }
    icount = 0;
    pSaveLoadGameOLD->SaveGameDataOLD(&icount, 2u);
    SaveGameDataOLD(&pBackUpPoint, 12);
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* v1 = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(BufferPointer - itempStartPosition), pheader->iFormID, v1, iFlags, 262,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BackUpPackage.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(BufferPointer - itempStartPosition), 262,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BackUpPackage.cpp"
            );
        }
    }
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* pcurrent = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        uint8_t* pblockStart = static_cast<uint8_t*>(pblockSize);
        if (pcurrent > pblockStart + 0xFFFF) {
            Warning(
                "SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BackUpPackage.cpp", 262
            );
        }
        *static_cast<uint16_t*>(pblockSize) = static_cast<uint16_t>(pcurrent - pblockStart);
    }
}

// The old-format load: with save blocks, the 'BLOK' marker is checked and the block's 16-bit size read, and the
// buffer position after the back-up point is checked against the block's end.
void BackUpPackage::LoadGame() {
    uint32_t iblockHeader;
    uint16_t iblockSize;
    TESPackage::LoadGame();
    iblockSize = 0;
    uint8_t* istartBufferPosition = nullptr;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, 4u);
        if (iblockHeader != 0x424C4F4B) {
            LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pheader) {
                TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BackUpPackage.cpp", 270, pheader->iFormID,
                    pform->GetFormEditorID(), pheader->cVersion, pheader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BackUpPackage.cpp", 270,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockSize, 2u);
    }
    LoadGameDataOLD(&pBackUpPoint, 12);
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* pheader_2 = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pheader_2) {
            TESForm* pform_2 = TESForm::GetFormByNumericID(pheader_2->iFormID);
            if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
                if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
                    uint32_t iFlags = pheader_2->iFlags;
                    int32_t cVersion = pheader_2->cVersion;
                    const char* v2 = pform_2->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        static_cast<int32_t>(&istartBufferPosition[iblockSize] - icurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BackUpPackage.cpp", 310,
                        pheader_2->iFormID, v2, cVersion, iFlags
                    );
                }
            }
            else {
                uint32_t v5 = pheader_2->iFlags;
                int32_t v3 = pheader_2->cVersion;
                const char* v1 = pform_2->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(icurrentBufferPosition - &istartBufferPosition[iblockSize]),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BackUpPackage.cpp", 310, pheader_2->iFormID,
                    v1, v3, v5
                );
            }
        }
        else {
            if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
                if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
                    int32_t CurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                        "%i",
                        static_cast<int32_t>(&istartBufferPosition[iblockSize] - icurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BackUpPackage.cpp", 310, CurrentVersion
                    );
                }
            }
            else {
                int32_t v7 = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                    static_cast<int32_t>(icurrentBufferPosition - &istartBufferPosition[iblockSize]),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BackUpPackage.cpp", 310, v7
                );
            }
        }
    }
}

void BackUpPackage::InitLoadGame() {
    TESPackage::InitLoadGame();
}

void BackUpPackage::InitializeBackUpPackage(Actor* apActor, TESObjectCELL* apcell, TESObjectREFR* apRef) {
    NiPoint3 result;
    NiPoint3 v12;
    SetPackType(PACKAGE_TYPE_BACKUP);
    SetMustReachLocation(true);
    SetMustComplete(true);
    if (apActor) {
        PackageTarget* v7;
        PackageLocation* v8;
        PackageLocation* v13 = static_cast<PackageLocation*>(ourMalloc(sizeof(PackageLocation)));
        if (v13) {
            v8 = ::new (v13) PackageLocation;
        }
        else {
            v8 = nullptr;
        }
        if (apcell || apRef) {
            if (apRef) {
                v8->SetLocReference(apRef);
            }
            else {
                if (apcell) {
                    v8->SetLocCell(apcell);
                }
            }
        }
        else {
            v8->SetLocReference(apActor);
        }
        v8->SetRadius(50u);
        pBackUpPoint = NiPoint3_ZERO;
        if (apcell) {
            NiPoint3* v4;
            if (apcell->GetInterior()) {
                v4 = &(v12 = BackUpPackage::CalculateBackUpPointInterior(apActor, apRef));
            }
            else {
                v4 = &(result = BackUpPackage::CalculateBackUpPointExterior(apActor, apRef));
            }
            pBackUpPoint.x = v4->x;
            pBackUpPoint.y = v4->y;
            pBackUpPoint.z = v4->z;
        }
        SetPackageLocation(v8);
        if (v8) {
            delete v8;
        }
        PackageTarget* v10 = static_cast<PackageTarget*>(ourMalloc(sizeof(PackageTarget)));
        if (v10) {
            v7 = ::new (v10) PackageTarget;
        }
        else {
            v7 = nullptr;
        }
        SetPackageTarget(v7);
        if (v7) {
            delete v7;
        }
        SetProcedureType(PACKAGE_PROCEDURE_BACK_UP);
        PackageTarget* PackageTarget_ = GetPackageTarget();
        PackageTarget_->SetTargType(PTT_REFERENCE);
        PackageTarget* v6 = GetPackageTarget();
        v6->SetTargReference(apActor);
    }
    SetIsCreated(true);
}

NiPoint3* BackUpPackage::GetBackUpPoint(NiPoint3* apResult) {
    *apResult = pBackUpPoint;
    return apResult;
}

// aptarget is tested and read before apActor.
NiPoint3 BackUpPackage::CalculateBackUpPointExterior(Actor* apActor, TESObjectREFR* aptarget) {
    NiPoint3 kBackUpPoint = NiPoint3_ZERO;
    if (aptarget && apActor) {
        NiPoint3 kTargetPos = aptarget->GetLocationOnReference();
        NiPoint3 kPos = apActor->GetLocationOnReference();
        float fDistance = 300.0f;
        NiPoint3 kDirection = kPos - kTargetPos;
        kDirection.Unitize();
        kDirection = kPos + kDirection * fDistance;
        kBackUpPoint = kDirection;
        TESObjectCELL* pCell = apActor->GetParentCell();
        Pathing::FindPointOnNavMesh(apActor->GetWorldSpace(), pCell, kDirection, 200.0f, &kBackUpPoint);
        static_cast<void>(kBackUpPoint == kDirection);
    }
    return NiPoint3(kBackUpPoint);
}

// aptarget is tested first, then apActor, and the avoid position is read before the actor's. The distance is a float
// local.
NiPoint3 BackUpPackage::CalculateBackUpPointInterior(Actor* apActor, TESObjectREFR* aptarget) {
    NiPoint3 kBackUpPoint = NiPoint3_ZERO;
    if (aptarget && apActor) {
        NiPoint3 kAvoidPos = aptarget->GetLocationOnReference();
        NiPoint3 kActorPos = apActor->GetLocationOnReference();
        float fDistance = 300.0f;
        NiPoint3 kDirection = kActorPos - kAvoidPos;
        kDirection.Unitize();
        kDirection = kActorPos + kDirection * fDistance;
        kBackUpPoint = kDirection;
        TESObjectCELL* pCell = apActor->GetParentCell();
        Pathing::FindPointOnNavMesh(apActor->GetWorldSpace(), pCell, kDirection, 200.0f, &kBackUpPoint);
    }
    return NiPoint3(kBackUpPoint);
}
