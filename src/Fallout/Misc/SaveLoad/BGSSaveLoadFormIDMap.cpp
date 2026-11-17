#include "Fallout/Misc/SaveLoad/BGSSaveLoadFormIDMap.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadDefines.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFile.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"

BGSSaveLoadFormIDMap::BGSSaveLoadFormIDMap(uint32_t iBucketCount)
  : FormIDToIndexMap(iBucketCount), IndexToFormIDMap(iBucketCount), iCurrentIndex(0) {
    AddFormID(0);
}

BGSSaveLoadFormIDMap::~BGSSaveLoadFormIDMap() {
    FormIDToIndexMap.RemoveAll();
    IndexToFormIDMap.RemoveAll();
}

uint32_t BGSSaveLoadFormIDMap::AddFormID(uint32_t aiFormID) {
    if (pDataHandler->IsFormIDCreated(aiFormID)) {
        return aiFormID;
    }
    uint32_t iindex = 0;
    if (!FormIDToIndexMap.GetAt(aiFormID, iindex)) {
        iindex = iCurrentIndex++;
        FormIDToIndexMap.SetAt(aiFormID, iindex);
        IndexToFormIDMap.SetAt(iindex, aiFormID);
    }
    return iindex;
}

uint32_t BGSSaveLoadFormIDMap::GetFormID(uint32_t aiFormIDIndex) {
    if (pDataHandler->IsFormIDCreated(aiFormIDIndex)) {
        return aiFormIDIndex;
    }
    uint32_t uiFormID = 0;
    if ((&IndexToFormIDMap)
            ->NiTMapBase<DFALL<NiTMapItem<uint32_t, uint32_t>>, uint32_t, uint32_t>::GetAt(aiFormIDIndex, uiFormID)) {
        return uiFormID;
    }
    else {
        return (uint32_t)0;
    }
}

void BGSSaveLoadFormIDMap::Save(BGSSaveLoadFile* apFile) {
    uint32_t formID;
    int32_t auiIndex;
    int32_t v5 = ((int32_t)iCurrentIndex) - 1;
    SaveGameDataToFileFunction(apFile, &v5, 4u);
    for (auiIndex = 1; ((uint32_t)auiIndex) < ((uint32_t)((int32_t)iCurrentIndex)); ++auiIndex) {
        formID = GetFormID(auiIndex);
        SaveGameDataToFileFunction(apFile, &formID, 4u);
    }
}

void BGSSaveLoadFormIDMap::Clear() {
    FormIDToIndexMap.RemoveAll();
    IndexToFormIDMap.RemoveAll();
    iCurrentIndex = 0;
    AddFormID(0);
}

void BGSSaveLoadFormIDMap::Load(BGSSaveLoadFile* apFile) {
    uint32_t iformID;
    uint32_t icount;
    Clear();
    icount = 0;
    LoadGameDataFromFileFunction(apFile, &icount, 4u);
    for (uint32_t i = 0; i < icount; ++i) {
        iformID = 0;
        LoadGameDataFromFileFunction(apFile, &iformID, 4u);
        uint32_t iconvertedFormID = ConvertFormID(iformID);
        if (iconvertedFormID) {
            FormIDToIndexMap.SetAt(iconvertedFormID, iCurrentIndex);
        }
        // The index is incremented after the call.
        IndexToFormIDMap.SetAt(iCurrentIndex, iconvertedFormID);
        ++iCurrentIndex;
    }
}

uint32_t BGSSaveLoadFormIDMap::ConvertFormID(uint32_t aiFormID) {
    if (((uint8_t)(aiFormID >> 24)) == 255) {
        return aiFormID;
    }
    uint8_t saveMod = pSaveLoadGame->GetSaveMod((uint8_t)(aiFormID >> 24));
    if (saveMod == 255) {
        return (uint32_t)0;
    }
    else {
        return (saveMod << 24) | (aiFormID & 16777215);
    }
}
