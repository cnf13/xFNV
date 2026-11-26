#include "Fallout/Misc/SaveLoad/BGSSaveLoadChangesMap.h"

#include "BSCore/NewOverloads.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout/Misc/SaveLoad/BGSChangeFlags.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameSubBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSUnloadedFormBuffer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"

BGSSaveLoadChangesMap::BGSSaveLoadChangesMap() : NiTPointerMap<uint32_t, BGSFormChanges*>(0x13AFu) {}

BGSSaveLoadChangesMap::~BGSSaveLoadChangesMap() {
    RemoveAllChanges();
}

bool BGSSaveLoadChangesMap::RemoveChanges(uint32_t aiFormID) {
    BGSFormChanges* pChanges = GetChanges(aiFormID);
    if (!pChanges) {
        return false;
    }
    delete pChanges;
    RemoveAt(aiFormID);
    return true;
}

BGSFormChanges* BGSSaveLoadChangesMap::GetChanges(uint32_t aiFormID) {
    BGSFormChanges* pChanges = nullptr;
    if (GetAt(aiFormID, pChanges)) {
        return pChanges;
    }
    return nullptr;
}

// The flags are copied when the form has changes; otherwise empty flags are constructed.
BGSChangeFlags* BGSSaveLoadChangesMap::GetChangeFlags(BGSChangeFlags* apResult, uint32_t aiFormID) {
    BGSFormChanges* pChanges = GetChanges(aiFormID);
    if (pChanges) {
        *apResult = pChanges->iChangeFlags;
    }
    else {
        ::new (apResult) BGSChangeFlags(0);
    }
    return apResult;
}

BGSUnloadedFormBuffer* BGSSaveLoadChangesMap::GetChangeBuffer(BGSUnloadedFormBuffer* apResult, uint32_t aiFormID) {
    BGSFormChanges* pChanges = GetChanges(aiFormID);
    if (pChanges) {
        apResult->pBuffer = pChanges->UnloadedFormBuffer.pBuffer;
    }
    else {
        apResult->pBuffer = nullptr;
    }
    return apResult;
}

bool BGSSaveLoadChangesMap::CheckChangeFlags(uint32_t aiFormID, BGSChangeFlags aiChangeFlags) {
    BGSFormChanges* pChanges = GetChanges(aiFormID);
    if (!pChanges) {
        return false;
    }
    return pChanges->iChangeFlags.CheckFlags(aiChangeFlags);
}

void BGSSaveLoadChangesMap::AddChangeFlags(uint32_t aiFormID, BGSChangeFlags aiChangeFlags) {
    BGSFormChanges* pFormChanges = GetChanges(aiFormID);
    if (pFormChanges) {
        if (pFormChanges->UnloadedFormBuffer.pBuffer) {
            return;
        }
    }
    else {
        pFormChanges = new BGSFormChanges;
        SetAt(aiFormID, pFormChanges);
    }
    pFormChanges->iChangeFlags.AddFlags(aiChangeFlags.iFlags);
}

bool BGSSaveLoadChangesMap::RemoveChangeFlags(uint32_t aiFormID, BGSChangeFlags aiChangeFlags) {
    BGSFormChanges* pFormChanges = GetChanges(aiFormID);
    if (pFormChanges && !pFormChanges->UnloadedFormBuffer.pBuffer) {
        pFormChanges->iChangeFlags.RemoveFlags(aiChangeFlags.iFlags);
        if (pFormChanges->iChangeFlags.iFlags) {
            return false;
        }
        if (pDataHandler->IsFormIDCreated(aiFormID)) {
            return false;
        }
        delete pFormChanges;
        RemoveAt(aiFormID);
    }
    return true;
}

void BGSSaveLoadChangesMap::SetChangeFlags(uint32_t aiFormID, BGSChangeFlags aiChangeFlags) {
    BGSFormChanges* pFormChanges = GetChanges(aiFormID);
    if (!pFormChanges) {
        pFormChanges = new BGSFormChanges;
        SetAt(aiFormID, pFormChanges);
    }
    pFormChanges->iChangeFlags.iFlags = aiChangeFlags.iFlags;
}

void BGSSaveLoadChangesMap::SetChanges(
    uint32_t aiFormID,
    BGSChangeFlags aiChangeFlags,
    BGSUnloadedFormBuffer akBuffer
) {
    BGSFormChanges* pFormChanges = GetChanges(aiFormID);
    if (!pFormChanges) {
        pFormChanges = new BGSFormChanges;
        SetAt(aiFormID, pFormChanges);
    }
    pFormChanges->iChangeFlags.iFlags = aiChangeFlags.iFlags;
    pFormChanges->UnloadedFormBuffer = akBuffer;
}

void BGSSaveLoadChangesMap::RemoveAllChanges() {
    NiTMapItem<uint32_t, BGSFormChanges*>* it = GetFirstPos();
    while (it) {
        BGSFormChanges* pChanges = nullptr;
        uint32_t uiFormID = 0;
        GetNext(it, uiFormID, pChanges);
        if (pChanges) {
            delete pChanges;
        }
    }
    RemoveAll();
}

BGSFormChanges::BGSFormChanges() : iChangeFlags(), UnloadedFormBuffer() {}

BGSFormChanges::~BGSFormChanges() {
    ourFree(UnloadedFormBuffer.pBuffer);
}
