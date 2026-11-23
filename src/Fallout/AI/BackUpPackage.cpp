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
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&pBackUpPoint), sizeof(pBackUpPoint), 0);
}

void BackUpPackage::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESPackage::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&pBackUpPoint), sizeof(pBackUpPoint));
}

void BackUpPackage::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    TESPackage::InitLoadGame(apLoadGameBuffer);
}

uint16_t BackUpPackage::GetSaveSize() {
    uint16_t isaveSize = TESPackage::GetSaveSize();
    uint16_t ibaseSize = isaveSize;
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        isaveSize += 6;
    }
    uint16_t isize = isaveSize + 12;
    if (bDebugSaveBuffer.Bool()) {
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            InitializationFlow(
                "GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                isize - ibaseSize,
                pheader->iFormID,
                pform->GetFormEditorID(),
                pheader->iFlags,
                211,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BackUpPackage.cpp"
            );
        }
        else {
            InitializationFlow(
                "GetSaveSize(): %-5i ending at line %i in file %s",
                isize - ibaseSize,
                211,
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
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, sizeof(iblockHeader));
        pblockSize = pSaveLoadGameOLD->m_pBuffer;
        pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, sizeof(iblockSize));
    }
    icount = 0;
    pSaveLoadGameOLD->SaveGameDataOLD(&icount, sizeof(icount));
    SaveGameDataOLD(&pBackUpPoint, sizeof(pBackUpPoint));
    if (bDebugSaveBuffer.Bool()) {
        uint8_t* pbufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            uint32_t iFlags = pheader->iFlags;
            const char* pcEditorID = pform->GetFormEditorID();
            InitializationFlow(
                "SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
                static_cast<int32_t>(pbufferPosition - itempStartPosition),
                pheader->iFormID,
                pcEditorID,
                iFlags,
                262,
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BackUpPackage.cpp"
            );
        }
        else {
            InitializationFlow(
                "SaveGame(): %-5i ending at line %i in file %s",
                static_cast<int32_t>(pbufferPosition - itempStartPosition),
                262,
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
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BackUpPackage.cpp",
                262
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
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, sizeof(iblockHeader));
        if (iblockHeader != 0x424C4F4B) {
            LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
            if (pheader) {
                TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
                    "loading form is %08X %s with version %i and flags %08X",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BackUpPackage.cpp",
                    270,
                    pheader->iFormID,
                    pform->GetFormEditorID(),
                    pheader->cVersion,
                    pheader->iFlags
                );
            }
            else {
                Warning(
                    "SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
                    "version is %i",
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BackUpPackage.cpp",
                    270,
                    pSaveLoadGameOLD->GetCurrentVersion()
                );
            }
        }
        istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        pSaveLoadGameOLD->LoadGameDataOLD(&iblockSize, sizeof(iblockSize));
    }
    LoadGameDataOLD(&pBackUpPoint, sizeof(pBackUpPoint));
    if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
        uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
        LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
        if (pheader) {
            TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
            if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
                if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
                    uint32_t iFlags = pheader->iFlags;
                    int32_t cVersion = pheader->cVersion;
                    const char* pcEditorID = pform->GetFormEditorID();
                    Warning(
                        "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
                        "form is %08X %s with version %i and flags %08X",
                        static_cast<int32_t>(&istartBufferPosition[iblockSize] - icurrentBufferPosition),
                        "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BackUpPackage.cpp",
                        310,
                        pheader->iFormID,
                        pcEditorID,
                        cVersion,
                        iFlags
                    );
                }
            }
            else {
                uint32_t iFlags = pheader->iFlags;
                int32_t cVersion = pheader->cVersion;
                const char* pcEditorID = pform->GetFormEditorID();
                Warning(
                    "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
                    "is %08X %s with version %i and flags %08X",
                    static_cast<int32_t>(icurrentBufferPosition - &istartBufferPosition[iblockSize]),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BackUpPackage.cpp",
                    310,
                    pheader->iFormID,
                    pcEditorID,
                    cVersion,
                    iFlags
                );
            }
        }
        else if (icurrentBufferPosition <= &istartBufferPosition[iblockSize]) {
            if (icurrentBufferPosition < &istartBufferPosition[iblockSize]) {
                int32_t iCurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
                Warning(
                    "SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
                    "%i",
                    static_cast<int32_t>(&istartBufferPosition[iblockSize] - icurrentBufferPosition),
                    "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BackUpPackage.cpp",
                    310,
                    iCurrentVersion
                );
            }
        }
        else {
            int32_t iCurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
            Warning(
                "SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
                static_cast<int32_t>(icurrentBufferPosition - &istartBufferPosition[iblockSize]),
                "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\AI\\BackUpPackage.cpp",
                310,
                iCurrentVersion
            );
        }
    }
}

void BackUpPackage::InitLoadGame() {
    TESPackage::InitLoadGame();
}

void BackUpPackage::InitializeBackUpPackage(Actor* apActor, TESObjectCELL* apcell, TESObjectREFR* apRef) {
    NiPoint3 kExteriorPoint;
    NiPoint3 kInteriorPoint;
    SetPackType(PACKAGE_TYPE_BACKUP);
    SetMustReachLocation(true);
    SetMustComplete(true);
    if (apActor) {
        PackageTarget* pTarget;
        PackageLocation* pLocation;
        pLocation = new PackageLocation;
        if (apcell || apRef) {
            if (apRef) {
                pLocation->SetLocReference(apRef);
            }
            else if (apcell) {
                pLocation->SetLocCell(apcell);
            }
        }
        else {
            pLocation->SetLocReference(apActor);
        }
        pLocation->SetRadius(50u);
        pBackUpPoint = NiPoint3::ZERO;
        if (apcell) {
            NiPoint3* pPoint;
            if (apcell->GetInterior()) {
                pPoint = &(kInteriorPoint = BackUpPackage::CalculateBackUpPointInterior(apActor, apRef));
            }
            else {
                pPoint = &(kExteriorPoint = BackUpPackage::CalculateBackUpPointExterior(apActor, apRef));
            }
            pBackUpPoint.x = pPoint->x;
            pBackUpPoint.y = pPoint->y;
            pBackUpPoint.z = pPoint->z;
        }
        SetPackageLocation(pLocation);
        if (pLocation) {
            delete pLocation;
        }
        pTarget = new PackageTarget;
        SetPackageTarget(pTarget);
        if (pTarget) {
            delete pTarget;
        }
        SetProcedureType(PACKAGE_PROCEDURE_BACK_UP);
        GetPackageTarget()->SetTargType(PTT_REFERENCE);
        GetPackageTarget()->SetTargReference(apActor);
    }
    SetIsCreated(true);
}

NiPoint3 BackUpPackage::GetBackUpPoint() {
    return pBackUpPoint;
}

// aptarget is tested and read before apActor.
NiPoint3 BackUpPackage::CalculateBackUpPointExterior(Actor* apActor, TESObjectREFR* aptarget) {
    NiPoint3 kBackUpPoint = NiPoint3::ZERO;
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
    NiPoint3 kBackUpPoint = NiPoint3::ZERO;
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
