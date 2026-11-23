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
    uint32_t uiIndex = 0;
    if (!FormIDToIndexMap.GetAt(aiFormID, uiIndex)) {
        uiIndex = iCurrentIndex++;
        FormIDToIndexMap.SetAt(aiFormID, uiIndex);
        IndexToFormIDMap.SetAt(uiIndex, aiFormID);
    }
    return uiIndex;
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

    return 0;
}

void BGSSaveLoadFormIDMap::Save(BGSSaveLoadFile* apFile) {
    int32_t iCount = static_cast<int32_t>(iCurrentIndex) - 1;
    SaveGameDataToFileFunction(apFile, &iCount, 4u);
    for (int32_t iIndex = 1; static_cast<uint32_t>(iIndex) < iCurrentIndex; ++iIndex) {
        uint32_t uiFormID = GetFormID(iIndex);
        SaveGameDataToFileFunction(apFile, &uiFormID, 4u);
    }
}

void BGSSaveLoadFormIDMap::Clear() {
    FormIDToIndexMap.RemoveAll();
    IndexToFormIDMap.RemoveAll();
    iCurrentIndex = 0;
    AddFormID(0);
}

void BGSSaveLoadFormIDMap::Load(BGSSaveLoadFile* apFile) {
    Clear();
    uint32_t uiCount = 0;
    LoadGameDataFromFileFunction(apFile, &uiCount, 4u);
    for (uint32_t i = 0; i < uiCount; ++i) {
        uint32_t uiFormID = 0;
        LoadGameDataFromFileFunction(apFile, &uiFormID, 4u);
        uint32_t uiConvertedFormID = ConvertFormID(uiFormID);
        if (uiConvertedFormID) {
            FormIDToIndexMap.SetAt(uiConvertedFormID, iCurrentIndex);
        }
        // The index is incremented after the call.
        IndexToFormIDMap.SetAt(iCurrentIndex, uiConvertedFormID);
        ++iCurrentIndex;
    }
}

uint32_t BGSSaveLoadFormIDMap::ConvertFormID(uint32_t aiFormID) {
    if (static_cast<uint8_t>(aiFormID >> 24) == 255) {
        return aiFormID;
    }
    uint8_t ucSaveMod = pSaveLoadGame->GetSaveMod(static_cast<uint8_t>(aiFormID >> 24));
    if (ucSaveMod == 255) {
        return 0;
    }

    return (ucSaveMod << 24) | (aiFormID & 16777215);
}
