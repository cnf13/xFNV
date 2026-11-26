#include "BSCore/BSString.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/Settings/XGameSetting.h"
#include "Fallout Shared/TESFile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "xfnv/Compiler.h"

// Reads rSetting's value from the DATA chunk of the next chunk of the open file: an int or float directly, a string
// through a buffer of the chunk's size.
bool GameSettingCollection::ReadSetting(Setting& rSetting) {
    bool bResult = false;
    if (pHandle) {
        TESFile* pFile = static_cast<TESFile*>(pHandle);
        pFile->NextChunk();
        if (pFile->GetTESChunk() == DATA_ID) {
            switch (rSetting.GetType()) {
                case Setting::ST_INT: bResult = pFile->GetChunkData<uint32_t>(rSetting.uValue.uint); break;
                case Setting::ST_FLOAT: bResult = pFile->GetChunkData<uint32_t>(rSetting.uValue.uint); break;
                case Setting::ST_STRING: {
                    // A 16-aligned stack buffer of the chunk's size.
                    char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(pFile->GetChunkSize()));
                    bResult = pFile->GetChunkData(pBuffer, 0);
                    rSetting = pBuffer;
                    break;
                }
                default: break;
            }
        }
    }
    return bResult;
}

uint32_t GameSettingCollection::GetViewerStrings(NiTPrimitiveArray<char*>* apStringsA) {
    char* element = NiGetViewerString("GameSettings");
    apStringsA->Add(element);
    uint32_t I = SettingCollectionMap::GetViewerStrings(apStringsA);
    char* ViewerString_6 = NiGetViewerString("Total GameSettings", I);
    apStringsA->Add(ViewerString_6);
    return I;
}

bool GameSettingCollection::WriteSetting(Setting& arSetting) {
    return false;
}

bool GameSettingCollection::Open(bool abWrite) {
    return (pHandle != nullptr);
}

bool GameSettingCollection::Close() {
    pHandle = nullptr;
    return true;
}

bool GameSettingCollection::ReadSetting(TESFile* apFile, const char* apKey) {
    bool bres = false;
    SetFileHandle(apFile);
    Setting* psetting = GetPtr(apKey);
    if (psetting) {
        bres = ReadSetting(*psetting);
    }
    Close();
    return bres;
}

bool GameSettingCollection::GetSetting(const char* apKey, Setting** appSetting) {
    return Settings.GetAt(apKey, *appSetting);
}

Setting* GameSettingCollection::GetPtr(const char* apID) {
    Setting* appSetting;
    appSetting = nullptr;
    GetSetting(apID, &appSetting);
    return appSetting;
}
