#include "Fallout/Misc/SaveLoad/BGSSaveLoadHeader.h"

#include "BSCore/BSString.h"
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
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPixelFormat.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTexture.h"
#include "SDK/MSVC/string.h"

void BGSSaveLoadHeader::LoadHeaderData(BGSSaveLoadFile* apFile, BGSSaveLoadFileEntry* apFileEntry) {
    uint32_t iplayerLevel;
    uint32_t isaveGameNumber;
    int32_t iversion;
    uint32_t iscreenshotWidth;
    char apBuffer[264];
    uint32_t iscreenshotHeight;
    apFile->SeekSet(0);
    std::memset(apBuffer, 0, 260);
    uint32_t v2 = std::strlen(pSaveGameHeaderString);
    apFile->Read(apBuffer, v2);
    if (!BSstrcmp(apBuffer, pSaveGameHeaderString)) {
        BGSLoadGameBuffer loadBuffer;
        loadBuffer.Load(apFile);
        BGSLoadGameBuffer* apLoadGameBuffer = &loadBuffer;
        iversion = 0;
        loadBuffer.LoadData(&iversion, 4u);
        apFileEntry->iVersion = iversion;
        if (iversion == 48) {
            char* NextPos = apLoadGameBuffer->GetNextPos();
            if (NextPos[4] == 124) {
                apFileEntry->SetVersionInfo("ENGLISH");
            }
            else {
                char Src[68];
                apLoadGameBuffer->LoadData(Src, 64u);
                apFileEntry->SetVersionInfo(Src);
            }
        }
        else {
            char v6[64];
            apLoadGameBuffer->LoadData(v6, 64u);
            apFileEntry->SetVersionInfo(v6);
        }
        iscreenshotWidth = 0;
        iscreenshotHeight = 0;
        apLoadGameBuffer->LoadData(&iscreenshotWidth, 4u);
        apLoadGameBuffer->LoadData(&iscreenshotHeight, 4u);
        isaveGameNumber = 0;
        apLoadGameBuffer->LoadData(&isaveGameNumber, 4u);
        apFileEntry->iSaveGameNumber = isaveGameNumber;
        apFileEntry->pPlayerName = apLoadGameBuffer->LoadString(nullptr);
        apFileEntry->pPlayerTitle = apLoadGameBuffer->LoadString(nullptr);
        if (iversion > 47) {
            iplayerLevel = 0;
            apLoadGameBuffer->LoadData(&iplayerLevel, 4u);
            apFileEntry->iPlayerLevel = iplayerLevel;
        }
        char* String = apLoadGameBuffer->LoadString(nullptr);
        apFileEntry->pLocation = String;
        apFileEntry->pPlayTime = apLoadGameBuffer->LoadString(nullptr);
        apFileEntry->iScreenshotWidth = iscreenshotWidth;
        apFileEntry->iScreenshotHeight = iscreenshotHeight;
        uint32_t FilePosition = apFile->GetFilePosition();
        apFileEntry->iScreenshotOffset = FilePosition;
    }
}

NiTexture* BGSSaveLoadHeader::LoadHeaderScreenshot(
    BGSSaveLoadFile* apFile,
    uint32_t aiOffset,
    uint32_t aiWidth,
    uint32_t aiHeight
) {
    NiPixelData* ppixelData;
    NiPixelFormat pixelFormat;
    apFile->SeekSet(static_cast<int32_t>(aiOffset));
    pixelFormat = NiPixelFormat_RGB24;
    ppixelData = new NiPixelData(aiWidth, aiHeight, pixelFormat, 1u, 1u);
    apFile->Read(ppixelData->GetPixels(0u, 0u), 3 * (aiHeight * aiWidth));
    NiTexture::FormatPrefs prefs;
    prefs.m_ePixelLayout = NiTexture::FormatPrefs::TRUE_COLOR_32;
    prefs.m_eMipMapped = NiTexture::FormatPrefs::NO;
    prefs.m_eAlphaFmt = NiTexture::FormatPrefs::NONE;
    NiFixedString apString("BGSSaveLoadHeader_Screenshot");
    NiFixedString* v6 = &apString;
    const char* HandleString = v6->GetHandleString();
    NiSourceTexture* v9 = NiSourceTexture::Create(ppixelData, HandleString, prefs);
    return v9;
}

void BGSSaveLoadHeader::SaveHeader(BGSSaveLoadFile* apFile) {
    uint32_t isaveGameNumber;
    uint32_t iplayerLevel;
    char plocation[260];
    int32_t iversion;
    char pfileName[260];
    int32_t iscreenshotWidth;
    char lname[64];
    char pplayTime[260];
    int32_t iscreenshotHeight;
    uint32_t v1 = std::strlen(pSaveGameHeaderString);
    apFile->Write(pSaveGameHeaderString, v1);
    BGSSaveGameBuffer saveBuffer;
    BGSSaveGameBuffer* apSaveGameBuffer = &saveBuffer;
    iversion = pSaveLoadManager->GetMajorVersion();
    apSaveGameBuffer->SaveDataEndian(&iversion, 4u, 0);
    ourMemset(lname, 0, 64u);
    const char* v2 = sLanguage.String();
    BSstrcat(lname, 64, v2);
    apSaveGameBuffer->SaveDataEndian(lname, 64u, 0);
    double ScreenHeight = static_cast<double>(Renderer::GetScreenHeight());
    bool bwideScreen = ScreenHeight / static_cast<double>(Renderer::GetScreenWidth()) < 0.75;
    iscreenshotWidth = bwideScreen ? 512 : 432;
    iscreenshotHeight = iscreenshotWidth;
    std::memset(pfileName, 0, 260);
    if (bOutputSaveGameScreenshot.Bool()) {
        const char* v3 = sLocalSavePath.String();
        BSstrcpy(pfileName, 260, v3);
        BSstrcat(pfileName, 260, apFile->pSaveFileName);
        BSstrcat(pfileName, 260, ".bmp");
    }
    NiPixelData* pscreenshot = nullptr;
    if (pSaveLoadManager->bSimulatedMode) {
        double v6 = static_cast<double>(Renderer::GetScreenHeight());
        float finverseAspectRatio = v6 / static_cast<double>(Renderer::GetScreenWidth());
        iscreenshotHeight = static_cast<int32_t>(static_cast<double>(iscreenshotHeight) * finverseAspectRatio);
    }
    else {
        pscreenshot = ScreenShot::TakeSaveGameScreenShot(iscreenshotWidth, iscreenshotHeight, pfileName);
    }
    apSaveGameBuffer->SaveDataEndian(&iscreenshotWidth, 4u, 0);
    apSaveGameBuffer->SaveDataEndian(&iscreenshotHeight, 4u, 0);
    isaveGameNumber = pSaveLoadManager->iCurrentSaveGameNumber;
    apSaveGameBuffer->SaveDataEndian(&isaveGameNumber, 4u, 0);
    const char* FullName = pPlayer->GetFullName();
    apSaveGameBuffer->SaveString(FullName, 0);
    const char* KarmicTitle = TESActorBaseData::GetKarmicTitle(pPlayer);
    apSaveGameBuffer->SaveString(KarmicTitle, 0);
    // The 16-bit level is zero-extended to 32 bits before being saved as 4 bytes.
    iplayerLevel = pPlayer->GetLevel();
    apSaveGameBuffer->SaveDataEndian(&iplayerLevel, 4u, 0);
    pSaveLoadManager->GetPlayerLocationName(plocation);
    apSaveGameBuffer->SaveString(plocation, 0);
    pSaveLoadManager->GetPlayTimeString(pplayTime, true);
    apSaveGameBuffer->SaveString(pplayTime, 0);
    saveBuffer.Save(apFile);
    if (pSaveLoadManager->bSimulatedMode) {
        apFile->Write(nullptr, 3 * (iscreenshotHeight * iscreenshotWidth));
    }
    else {
        apFile->Write(pscreenshot->GetPixels(0u, 0u), 3 * (iscreenshotHeight * iscreenshotWidth));
    }
    if (pscreenshot) {
        delete pscreenshot;
    }
}

int32_t BGSSaveLoadHeader::LoadHeader(BGSSaveLoadFile* apFile) {
    uint32_t apData;
    uint32_t v10;
    char apBuffer[264];
    uint32_t v13;
    std::memset(apBuffer, 0, 260);
    uint32_t v1 = std::strlen(pSaveGameHeaderString);
    if (!apFile->Read(apBuffer, v1)) {
        return 0;
    }
    if (BSstrcmp(apBuffer, pSaveGameHeaderString)) {
        return -1;
    }
    BGSLoadGameBuffer loadBuffer;
    loadBuffer.Load(apFile);
    BGSLoadGameBuffer* apLoadGameBuffer = &loadBuffer;
    apData = 0;
    loadBuffer.LoadData(&apData, 4u);
    if (apData == 48) {
        char* NextPos = apLoadGameBuffer->GetNextPos();
        if (NextPos[4] != 124) {
            char v5[68];
            apLoadGameBuffer->LoadData(v5, 64u);
        }
    }
    else {
        char v4[64];
        apLoadGameBuffer->LoadData(v4, 64u);
    }
    v10 = 0;
    v13 = 0;
    apLoadGameBuffer->LoadData(&v10, 4u);
    apLoadGameBuffer->LoadData(&v13, 4u);
    apFile->SeekCurrent(3 * (v13 * v10));
    int32_t iversion = apData;
    return iversion;
}

void BGSSaveLoadFileEntry::LoadData() {
    if (!bLoaded && !bCorrupt) {
        BGSSaveLoadFile* pfile =
            pSaveLoadManager->OpenSaveFile(pFileName, false, BGSSaveLoadManager::SAVEFILE_LOAD, iDeviceID);
        if (pfile) {
            BGSSaveLoadHeader::LoadHeaderData(pfile, this);
            if (!iScreenshotWidth && !iScreenshotHeight) {
                bCorrupt = true;
            }
            if (!pLocation) {
                bCorrupt = true;
            }
            if (pSaveLoadManager->IsSaveFileNameCustom(pFileName)) {
                ourFree(pLocation);
                uint32_t ilength = BSstrlen(pFileName) + 1;
                pLocation = static_cast<char*>(ourMalloc(ilength));
                BSstrcpy(pLocation, static_cast<int32_t>(ilength), pFileName);
            }
            pSaveLoadManager->CloseSaveFile(pfile, false);
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
    return (bool)((uint8_t)pSaveLoadManager->IsSaveFileNameSystemSave(pFileName));
}

int32_t BGSSaveLoadFileEntry::SetVersionInfo(const char* Src) {
    return BSstrcpy(pcLanguage, 64, Src);
}

int32_t BGSSaveLoadFileEntry::Compare(BGSSaveLoadFileEntry* a1, BGSSaveLoadFileEntry* a2) {
    _FILETIME result;
    _FILETIME arFileTime;
    a1->GetFileTime(arFileTime);
    a2->GetFileTime(result);
    if (arFileTime.dwHighDateTime > result.dwHighDateTime) {
        return (-1);
    }
    if (arFileTime.dwHighDateTime < result.dwHighDateTime) {
        return 1;
    }
    if (arFileTime.dwLowDateTime <= result.dwLowDateTime) {
        return (int32_t)((uint8_t)(arFileTime.dwLowDateTime < result.dwLowDateTime));
    }

    return (-1);
}
