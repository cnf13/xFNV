#include "BSMain/XRegSetting.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "platform/System.h"

#include <span>

int32_t GetRegValueType(Setting* apSetting) {
    Setting::SETTING_TYPE v2 = Setting::DataType(apSetting->Key());
    if (v2 == Setting::ST_BINARY) {
        return 3;
    }
    if (v2 == Setting::ST_STRING) {
        return 1;
    }
    return 4;
}

uint32_t RegSettingCollection::GetViewerStrings(NiTPrimitiveArray<char*>* apStringsA) {
    char* v5 = NiGetViewerString("RegSettings");
    apStringsA->Add(v5);
    uint32_t viewerStrings = SettingCollectionList::GetViewerStrings(apStringsA);
    char* v4 = NiGetViewerString("Total RegSettings", viewerStrings);
    apStringsA->Add(v4);
    return viewerStrings;
}

// Writes the setting's value under its key, opening the key for the write when it is closed. A char setting ('c'
// prefix) is written under its key with an 's' prefix; the SETTING_TYPE is passed on as the registry type.
bool RegSettingCollection::WriteSetting(Setting& arSetting) {
    bool bResult = false;
    char pcDest[64];
    const char* pKey = arSetting.Key();
    if (pKey) {
        bool bOpened = pHandle == nullptr;
        if (bOpened) {
            Open(true);
        }
        if (pHandle) {
            uint32_t cbData = arSetting.DataSize();
            Setting::SETTING_TYPE eType = Setting::DataType(pKey);
            if (eType == Setting::ST_CHAR) {
                strcpy_s(pcDest, 64, pKey);
                pcDest[0] = 's';
                pKey = pcDest;
            }
            // The store takes the value as untyped bytes (the registry's LPBYTE).
            bResult = platform::RegistryWrite(
                pHandle,
                pKey,
                eType,
                std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(&arSetting.uValue), cbData)
            );
        }
        if (bOpened) {
            Close();
        }
    }
    return bResult;
}

// Reads the setting's value from its key, opening the key for the read when it is closed. A string value (registry
// type 1, REG_SZ) is read under the key with an 's' prefix into a 260-byte buffer and assigned to the setting; any
// other value is read straight into the setting's value.
bool RegSettingCollection::ReadSetting(Setting& arSetting) {
    bool bResult = false;
    char pcKey[64];
    const char* lpValueName = arSetting.Key();
    if (lpValueName) {
        bool bOpened = pHandle == nullptr;
        if (bOpened) {
            Open(false);
        }
        if (pHandle) {
            char Source[260];
            uint8_t* pData;
            uint32_t cbData = arSetting.DataSize();
            uint32_t Type = GetRegValueType(&arSetting);
            if (Type == 1) {
                strcpy_s(pcKey, 64, lpValueName);
                pcKey[0] = 's';
                lpValueName = pcKey;
                Source[0] = 0;
                cbData = 260;
                // The store fills untyped bytes (the registry's LPBYTE): the text buffer.
                pData = reinterpret_cast<uint8_t*>(Source);
            }
            else {
                // The store fills untyped bytes (the registry's LPBYTE): the setting's value in place.
                pData = reinterpret_cast<uint8_t*>(&arSetting.uValue);
            }
            bResult = platform::RegistryRead(pHandle, lpValueName, Type, std::span<uint8_t>(pData, cbData), cbData);
            if (bResult && Type == 1) {
                arSetting = static_cast<const char*>(Source);
            }
        }
        if (bOpened) {
            Close();
        }
    }
    return bResult;
}

// Opens the settings store's key: created when writing, opened otherwise; true when it succeeds.
bool RegSettingCollection::Open(bool bWrite) {
    pHandle = platform::RegistryOpen(pSettingFile, bWrite);
    return pHandle != nullptr;
}

bool RegSettingCollection::Close() {
    if (pHandle) {
        platform::RegistryClose(pHandle);
        pHandle = nullptr;
    }
    return true;
}

void RegSettingCollection::Init() {
    if (!SettingT<RegSettingCollection>::pCollection) {
        SettingT<RegSettingCollection>::pCollection = new RegSettingCollection;
    }
}

RegSettingCollection::RegSettingCollection() {}
