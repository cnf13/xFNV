#include "Fallout/Misc/SaveLoad/BGSSaveLoadFile.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSSystem/BSFile.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadDefines.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFileEntry.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGlobalData.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadHeader.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSourceTexture.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "SDK/MSVC/string.h"
#include "platform/Paths.h"

#include <vector>

BGSSaveLoadFile::BGSSaveLoadFile(const char* apName, bool abWrite, uint32_t aiBufferSize)
  : pFile(nullptr), iSimulatedFileSize(0), iSimulatedFilePosition(0) {
    pSaveFileName[0] = 0;
    if (pSaveLoadManager->GetSimulatedMode()) {
        return;
    }
    pFile = new BSFile(apName, static_cast<NiFile::OpenMode>(abWrite), aiBufferSize, false);
}

BGSSaveLoadFile::~BGSSaveLoadFile() {
    delete pFile;
}

uint32_t BGSSaveLoadFile::Read(void* apBuffer, uint32_t aiBytes) {
    return pFile->NiBinaryStream::Read(apBuffer, aiBytes);
}

uint32_t BGSSaveLoadFile::Write(const void* apBuffer, uint32_t aiBytes) {
    if (!pSaveLoadManager->GetSimulatedMode()) {
        return static_cast<uint32_t>(pFile->WriteHere(apBuffer, static_cast<int32_t>(aiBytes)));
    }
    uint32_t v5 = iSimulatedFileSize - iSimulatedFilePosition;
    if (aiBytes > v5) {
        iSimulatedFileSize += (aiBytes - v5);
    }
    iSimulatedFilePosition += aiBytes;
    return aiBytes;
}

void BGSSaveLoadFile::SeekCurrent(int32_t aiPosition) {
    if (pSaveLoadManager->GetSimulatedMode()) {
        iSimulatedFilePosition += aiPosition;
    }
    else {
        pFile->Seek(aiPosition, NiFile::ms_iSeekCur);
    }
}

void BGSSaveLoadFile::SeekSet(int32_t aiPosition) {
    if (pSaveLoadManager->GetSimulatedMode()) {
        iSimulatedFilePosition = (uint32_t)aiPosition;
    }
    else {
        pFile->Seek(aiPosition, NiFile::ms_iSeekSet);
    }
}

uint32_t BGSSaveLoadFile::GetFilePosition() {
    if (pSaveLoadManager->GetSimulatedMode()) {
        return iSimulatedFilePosition;
    }

    return pFile->Tell();
}

void BGSSaveLoadFile::SetSaveFileName(const char* apFileName) {
    BSstrcpy(pSaveFileName, sizeof(pSaveFileName), apFileName);
}

bool BGSSaveLoadFile::Open() {
    return pSaveLoadManager->GetSimulatedMode() || pFile->Open(false, false);
}

int32_t BGSSaveLoadFile::SaveGlobalDataRange(BGSSaveLoadFile* apFile, uint32_t aiStart, uint32_t aiEnd) {
    uint32_t aiGlobalDataType;
    int32_t v6 = 0;
    BGSSaveGameBuffer apMemory;
    for (aiGlobalDataType = aiStart; aiGlobalDataType < aiEnd; ++aiGlobalDataType) {
        if (BGSSaveLoadGlobalData::SaveGlobalData(aiGlobalDataType, &apMemory)) {
            SaveGameDataToFileFunction(apFile, &aiGlobalDataType, 4u);
            apMemory.Save(apFile);
            apMemory.ClearPosition();
            ++v6;
        }
    }
    return v6;
}

int32_t BGSSaveLoadFile::SaveGlobalDataRange0_C(BGSSaveLoadFile* a1) {
    return BGSSaveLoadFile::SaveGlobalDataRange(a1, 0, 0xCu);
}

int32_t BGSSaveLoadFile::SaveGlobalDataRange3E8_3E9(BGSSaveLoadFile* apFile) {
    return BGSSaveLoadFile::SaveGlobalDataRange(apFile, 0x3E8, 0x3E9);
}

BGSSaveLoadFile* BGSSaveLoadFile::CreateFileA(const char* apFileName, bool abWrite, uint32_t aiBufferSize) {
    void* v5 = ourMalloc(sizeof(BGSSaveLoadFile));
    if (v5) {
        return ::new (v5) BGSSaveLoadFile(apFileName, abWrite, aiBufferSize);
    }

    return nullptr;
}

BGSSaveLoadFileEntry::BGSSaveLoadFileEntry(char* apFileName, int32_t aiDeviceID)
  : bLoaded(false), bCorrupt(false), iSaveGameNumber(0), pPlayerName(nullptr), pPlayerTitle(nullptr),
    pLocation(nullptr), pPlayTime(nullptr), iPlayerLevel(0), iScreenshotWidth(0), iScreenshotHeight(0),
    iScreenshotOffset(0), FileTime(), iDeviceID(aiDeviceID) {
    uint32_t ilength = std::strlen(apFileName);
    pFileName = static_cast<char*>(ourMalloc(ilength + 1));
    BSstrcpy(pFileName, ilength + 1, apFileName);
}

BGSSaveLoadFileEntry::~BGSSaveLoadFileEntry() {
    ourFree(pFileName);
    ourFree(pPlayTime);
    ourFree(pLocation);
    ourFree(pPlayerName);
    ourFree(pPlayerTitle);
}

// A full save or autosave is written as a new numbered save and the old file deleted; any other save is written
// over its file name.
bool BGSSaveLoadFileEntry::SaveGame() {
    if (pSaveLoadManager->IsSaveFileNameGenerated(pFileName) || pSaveLoadManager->IsSaveFileNameAutosave(pFileName)) {
        if (pSaveLoadManager->SaveGame(nullptr, iDeviceID, false)) {
            DeleteGame();
            return true;
        }
    }
    else if (pSaveLoadManager->SaveGame(pFileName, iDeviceID, false)) {
        pSaveLoadManager->RemoveSaveGame(this);
        return true;
    }
    return false;
}

void BGSSaveLoadFileEntry::DeleteGame() {
    pSaveLoadManager->DeleteSaveFile(pFileName, iDeviceID);
    pSaveLoadManager->RemoveSaveGame(this);
}

bool BGSSaveLoadFileEntry::LoadGame() {
    return (bool)((uint8_t)pSaveLoadManager->LoadGame(pFileName, iDeviceID, false, false));
}

bool BGSSaveLoadFileEntry::IsCorrupt() {
    if (bCorrupt) {
        return true;
    }
    LoadData();
    return (bool)((uint8_t)bCorrupt);
}

// A header with no screenshot size or no location marks the save corrupt.
NiTexture* BGSSaveLoadFileEntry::GetScreenshot() {
    NiSourceTexture* pSaveGameScreenshotTexture = nullptr;
    BGSSaveLoadFile* pFile =
        pSaveLoadManager->OpenSaveFile(pFileName, false, BGSSaveLoadManager::SAVEFILE_SCREENSHOT, iDeviceID);
    if (!pFile) {
        return nullptr;
    }
    if (!bLoaded) {
        BGSSaveLoadHeader::LoadHeaderData(pFile, this);
        if (!iScreenshotWidth && !iScreenshotHeight) {
            bCorrupt = true;
        }
        if (!pLocation) {
            bCorrupt = true;
        }
        bLoaded = true;
    }
    if (iScreenshotWidth && iScreenshotHeight) {
        pSaveGameScreenshotTexture = static_cast<NiSourceTexture*>(
            BGSSaveLoadHeader::LoadHeaderScreenshot(pFile, iScreenshotOffset, iScreenshotWidth, iScreenshotHeight)
        );
    }
    pSaveLoadManager->CloseSaveFile(pFile, false);
    return pSaveGameScreenshotTexture;
}

bool BGSSaveLoadFileEntry::IsQuicksave() {
    return (bool)((uint8_t)pSaveLoadManager->IsSaveFileNameQuicksave(pFileName));
}

bool BGSSaveLoadFileEntry::IsAutosave() {
    return (bool)((uint8_t)pSaveLoadManager->IsSaveFileNameAutosave(pFileName));
}

void BGSSaveLoadFileEntry::CalculateFileTime() {
    if (!FileTime.dwHighDateTime && !FileTime.dwLowDateTime) {
        char FileName[264];
        pSaveLoadManager->GetFullPath(pFileName, FileName, false);
        std::vector<platform::FileEntry> kFound = platform::FindFiles(FileName);
        if (!kFound.empty()) {
            FileTime.dwLowDateTime = static_cast<uint32_t>(kFound.front().writeTime);
            FileTime.dwHighDateTime = static_cast<uint32_t>(kFound.front().writeTime >> 32);
        }
    }
}

bool BGSSaveLoadManager::GetSimulatedMode() {
    return bSimulatedMode;
}
