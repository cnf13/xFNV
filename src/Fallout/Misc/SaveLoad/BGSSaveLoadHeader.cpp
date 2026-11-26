#include "Fallout/Misc/SaveLoad/BGSSaveLoadHeader.h"

#include "BSCore/BSString.h"
#include "BSCore/NewOverloads.h"
#include "Fallout Shared/Renderer.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFile.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFileEntry.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Fallout/Misc/ScreenShot.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "SDK/MSVC/string.h"

void BGSSaveLoadHeader::LoadHeaderData(BGSSaveLoadFile* apFile, BGSSaveLoadFileEntry* apFileEntry) {
    uint32_t uiPlayerLevel;
    uint32_t uiSaveGameNumber;
    int32_t iVersion;
    uint32_t uiScreenshotWidth;
    char cBuffer[264];
    uint32_t uiScreenshotHeight;
    apFile->SeekSet(0);
    std::memset(cBuffer, 0, 260);
    apFile->Read(cBuffer, std::strlen(pSaveGameHeaderString));
    if (!BSstrcmp(cBuffer, pSaveGameHeaderString)) {
        BGSLoadGameBuffer loadBuffer;
        loadBuffer.Load(apFile);
        iVersion = 0;
        loadBuffer.LoadData(&iVersion, sizeof(iVersion));
        apFileEntry->iVersion = iVersion;
        if (iVersion == 48) {
            char* pcNextPos = loadBuffer.GetNextPos();
            if (pcNextPos[4] == 124) {
                apFileEntry->SetVersionInfo("ENGLISH");
            }
            else {
                char cVersion[68];
                loadBuffer.LoadData(cVersion, 64u);
                apFileEntry->SetVersionInfo(cVersion);
            }
        }
        else {
            char cVersion[64];
            loadBuffer.LoadData(cVersion, sizeof(cVersion));
            apFileEntry->SetVersionInfo(cVersion);
        }
        uiScreenshotWidth = 0;
        uiScreenshotHeight = 0;
        loadBuffer.LoadData(&uiScreenshotWidth, sizeof(uiScreenshotWidth));
        loadBuffer.LoadData(&uiScreenshotHeight, sizeof(uiScreenshotHeight));
        uiSaveGameNumber = 0;
        loadBuffer.LoadData(&uiSaveGameNumber, sizeof(uiSaveGameNumber));
        apFileEntry->iSaveGameNumber = uiSaveGameNumber;
        apFileEntry->pPlayerName = loadBuffer.LoadString(nullptr);
        apFileEntry->pPlayerTitle = loadBuffer.LoadString(nullptr);
        if (iVersion > 47) {
            uiPlayerLevel = 0;
            loadBuffer.LoadData(&uiPlayerLevel, sizeof(uiPlayerLevel));
            apFileEntry->iPlayerLevel = uiPlayerLevel;
        }
        apFileEntry->pLocation = loadBuffer.LoadString(nullptr);
        apFileEntry->pPlayTime = loadBuffer.LoadString(nullptr);
        apFileEntry->iScreenshotWidth = uiScreenshotWidth;
        apFileEntry->iScreenshotHeight = uiScreenshotHeight;
        apFileEntry->iScreenshotOffset = apFile->GetFilePosition();
    }
}

NiTexture* BGSSaveLoadHeader::LoadHeaderScreenshot(
    BGSSaveLoadFile* apFile,
    uint32_t aiOffset,
    uint32_t aiWidth,
    uint32_t aiHeight
) {
    NiPixelData* pPixelData;
    NiPixelFormat pixelFormat;
    apFile->SeekSet(static_cast<int32_t>(aiOffset));
    pixelFormat = NiPixelFormat::RGB24;
    pPixelData = new NiPixelData(aiWidth, aiHeight, pixelFormat, 1u, 1u);
    apFile->Read(pPixelData->GetPixels(0u, 0u), 3 * (aiHeight * aiWidth));
    NiTexture::FormatPrefs prefs;
    prefs.m_ePixelLayout = NiTexture::FormatPrefs::TRUE_COLOR_32;
    prefs.m_eMipMapped = NiTexture::FormatPrefs::NO;
    prefs.m_eAlphaFmt = NiTexture::FormatPrefs::NONE;
    NiFixedString kName("BGSSaveLoadHeader_Screenshot");
    const char* pcName = kName.GetHandleString();
    return NiSourceTexture::Create(pPixelData, pcName, prefs);
}

void BGSSaveLoadHeader::SaveHeader(BGSSaveLoadFile* apFile) {
    uint32_t uiSaveGameNumber;
    uint32_t uiPlayerLevel;
    char pLocation[260];
    int32_t iVersion;
    char pFileName[260];
    int32_t iScreenshotWidth;
    char lname[64];
    char pPlayTime[260];
    int32_t iScreenshotHeight;
    apFile->Write(pSaveGameHeaderString, std::strlen(pSaveGameHeaderString));
    BGSSaveGameBuffer saveBuffer;
    iVersion = pSaveLoadManager->GetMajorVersion();
    saveBuffer.SaveDataEndian(&iVersion, sizeof(iVersion), 0);
    ourMemset(lname, 0, 64u);
    BSstrcat(lname, 64, sLanguage.String());
    saveBuffer.SaveDataEndian(lname, sizeof(lname), 0);
    double dScreenHeight = static_cast<double>(Renderer::GetScreenHeight());
    bool bWideScreen = dScreenHeight / static_cast<double>(Renderer::GetScreenWidth()) < 0.75;
    iScreenshotWidth = bWideScreen ? 512 : 432;
    iScreenshotHeight = iScreenshotWidth;
    std::memset(pFileName, 0, 260);
    if (bOutputSaveGameScreenshot.Bool()) {
        BSstrcpy(pFileName, 260, sLocalSavePath.String());
        BSstrcat(pFileName, 260, apFile->pSaveFileName);
        BSstrcat(pFileName, 260, ".bmp");
    }
    NiPixelData* pScreenshot = nullptr;
    if (pSaveLoadManager->bSimulatedMode) {
        dScreenHeight = static_cast<double>(Renderer::GetScreenHeight());
        float fInverseAspectRatio = dScreenHeight / static_cast<double>(Renderer::GetScreenWidth());
        iScreenshotHeight = static_cast<int32_t>(static_cast<double>(iScreenshotHeight) * fInverseAspectRatio);
    }
    else {
        pScreenshot = ScreenShot::TakeSaveGameScreenShot(iScreenshotWidth, iScreenshotHeight, pFileName);
    }
    saveBuffer.SaveDataEndian(&iScreenshotWidth, sizeof(iScreenshotWidth), 0);
    saveBuffer.SaveDataEndian(&iScreenshotHeight, sizeof(iScreenshotHeight), 0);
    uiSaveGameNumber = pSaveLoadManager->iCurrentSaveGameNumber;
    saveBuffer.SaveDataEndian(&uiSaveGameNumber, sizeof(uiSaveGameNumber), 0);
    saveBuffer.SaveString(pPlayer->GetFullName(), 0);
    saveBuffer.SaveString(TESActorBaseData::GetKarmicTitle(pPlayer), 0);
    // The 16-bit level is zero-extended to 32 bits before being saved as 4 bytes.
    uiPlayerLevel = pPlayer->GetLevel();
    saveBuffer.SaveDataEndian(&uiPlayerLevel, sizeof(uiPlayerLevel), 0);
    pSaveLoadManager->GetPlayerLocationName(pLocation);
    saveBuffer.SaveString(pLocation, 0);
    pSaveLoadManager->GetPlayTimeString(pPlayTime, true);
    saveBuffer.SaveString(pPlayTime, 0);
    saveBuffer.Save(apFile);
    if (pSaveLoadManager->bSimulatedMode) {
        apFile->Write(nullptr, 3 * (iScreenshotHeight * iScreenshotWidth));
    }
    else {
        apFile->Write(pScreenshot->GetPixels(0u, 0u), 3 * (iScreenshotHeight * iScreenshotWidth));
    }
    if (pScreenshot) {
        delete pScreenshot;
    }
}

int32_t BGSSaveLoadHeader::LoadHeader(BGSSaveLoadFile* apFile) {
    uint32_t uiVersion;
    uint32_t uiScreenshotWidth;
    char cBuffer[264];
    uint32_t uiScreenshotHeight;
    std::memset(cBuffer, 0, 260);
    if (!apFile->Read(cBuffer, std::strlen(pSaveGameHeaderString))) {
        return 0;
    }
    if (BSstrcmp(cBuffer, pSaveGameHeaderString)) {
        return -1;
    }
    BGSLoadGameBuffer loadBuffer;
    loadBuffer.Load(apFile);
    uiVersion = 0;
    loadBuffer.LoadData(&uiVersion, sizeof(uiVersion));
    if (uiVersion == 48) {
        char* pcNextPos = loadBuffer.GetNextPos();
        if (pcNextPos[4] != 124) {
            char cVersion[68];
            loadBuffer.LoadData(cVersion, 64u);
        }
    }
    else {
        char cVersion[64];
        loadBuffer.LoadData(cVersion, sizeof(cVersion));
    }
    uiScreenshotWidth = 0;
    uiScreenshotHeight = 0;
    loadBuffer.LoadData(&uiScreenshotWidth, sizeof(uiScreenshotWidth));
    loadBuffer.LoadData(&uiScreenshotHeight, sizeof(uiScreenshotHeight));
    apFile->SeekCurrent(3 * (uiScreenshotHeight * uiScreenshotWidth));
    return uiVersion;
}

void BGSSaveLoadFileEntry::LoadData() {
    if (!bLoaded && !bCorrupt) {
        BGSSaveLoadFile* pFile = pSaveLoadManager
                                     ->OpenSaveFile(pFileName, false, BGSSaveLoadManager::SAVEFILE_LOAD, iDeviceID);
        if (pFile) {
            BGSSaveLoadHeader::LoadHeaderData(pFile, this);
            if (!iScreenshotWidth && !iScreenshotHeight) {
                bCorrupt = true;
            }
            if (!pLocation) {
                bCorrupt = true;
            }
            if (pSaveLoadManager->IsSaveFileNameCustom(pFileName)) {
                ourFree(pLocation);
                uint32_t uiLength = BSstrlen(pFileName) + 1;
                pLocation = static_cast<char*>(ourMalloc(uiLength));
                BSstrcpy(pLocation, static_cast<int32_t>(uiLength), pFileName);
            }
            pSaveLoadManager->CloseSaveFile(pFile, false);
            bLoaded = true;
        }
        else {
            bCorrupt = true;
        }
    }
}

void BGSSaveLoadFileEntry::SetLoaded() {
    bLoaded = true;
}

uint32_t BGSSaveLoadFileEntry::GetPlayerLevel() {
    LoadData();
    return iPlayerLevel;
}

uint32_t BGSSaveLoadFileEntry::GetScreenshotWidth() {
    LoadData();
    return iScreenshotWidth;
}

uint32_t BGSSaveLoadFileEntry::GetScreenshotHeight() {
    LoadData();
    return iScreenshotHeight;
}

uint32_t BGSSaveLoadFileEntry::GetScreenshotOffset() {
    LoadData();
    return iScreenshotOffset;
}

const char* BGSSaveLoadFileEntry::GetPlayerName() {
    LoadData();
    return pPlayerName;
}

const char* BGSSaveLoadFileEntry::GetPlayTime() {
    LoadData();
    return pPlayTime;
}

const char* BGSSaveLoadFileEntry::GetLocation() {
    LoadData();
    return pLocation;
}

const char* BGSSaveLoadFileEntry::GetPlayerTitle() {
    LoadData();
    return pPlayerTitle;
}

uint32_t BGSSaveLoadFileEntry::GetSaveGameNumber() {
    LoadData();
    return iSaveGameNumber;
}

bool BGSSaveLoadFileEntry::IsSystemSave() {
    return pSaveLoadManager->IsSaveFileNameSystemSave(pFileName);
}

int32_t BGSSaveLoadFileEntry::SetVersionInfo(const char* Src) {
    return BSstrcpy(pcLanguage, 64, Src);
}

int32_t BGSSaveLoadFileEntry::Compare(BGSSaveLoadFileEntry* apEntry1, BGSSaveLoadFileEntry* apEntry2) {
    _FILETIME kFileTime2;
    _FILETIME kFileTime1;
    apEntry1->GetFileTime(kFileTime1);
    apEntry2->GetFileTime(kFileTime2);
    if (kFileTime1.dwHighDateTime > kFileTime2.dwHighDateTime) {
        return -1;
    }
    if (kFileTime1.dwHighDateTime < kFileTime2.dwHighDateTime) {
        return 1;
    }
    if (kFileTime1.dwLowDateTime <= kFileTime2.dwLowDateTime) {
        return kFileTime1.dwLowDateTime < kFileTime2.dwLowDateTime;
    }

    return -1;
}
