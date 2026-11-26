#pragma once

#include "BSCore/BSString.h"
#include "BSSystem/BSMemoryFile.h"
#include "BSSystemUtilities/BSSaveDataSystemUtility.h"

// A save file in memory with the details the system save dialog shows (title, icon, background).
class BSSaveDataSystemUtilityFile : public BSMemoryFile {
public:
    ~BSSaveDataSystemUtilityFile() override;

    // Sets the icon image's size and pixel buffer.
    void SetIconImage(uint32_t auiWidth, uint32_t auiHeight, void* apvBuffer);
    void SetSubTitle(const char* apcSubTitle);
    void SetDirectoryName(const char* apcName);
    void SetDetail(const char* apcDetail);
    void SetBackgroundImagePath(const char* apcPath);
    void SetSaveCompleteCallback(
        void (*apSaveCompleteCallback)(BSSaveDataSystemUtility::RESULT, BSSaveDataSystemUtilityFile*&)
    );

    char cDirectoryName[256];
    bool bSingleSave;
    char cSubTitle[128];
    char cDetail[1024];
    char cCreateIconImagePath[256];
    uint32_t uiIconImageWidth;
    uint32_t uiIconImageHeight;
    void* pvIconImageBuffer;
    char cIconImagePath[256];
    char cBackgroundImagePath[256];
    // BGSSaveLoadManager::SysUtilSaveCompleteCallback is the callback SetSaveCompleteCallback stores.
    void (*pSaveCompleteCallback)(BSSaveDataSystemUtility::RESULT aeResult, BSSaveDataSystemUtilityFile*& arpFile);
    void (*pLoadCompleteCallback)(BSSaveDataSystemUtility::RESULT aeResult, BSSaveDataSystemUtilityFile*& arpFile);
};

inline void BSSaveDataSystemUtilityFile::SetIconImage(uint32_t auiWidth, uint32_t auiHeight, void* apvBuffer) {
    uiIconImageWidth = auiWidth;
    uiIconImageHeight = auiHeight;
    pvIconImageBuffer = apvBuffer;
}

inline void BSSaveDataSystemUtilityFile::SetSubTitle(const char* apcSubTitle) {
    BSstrcpy(cSubTitle, 0x80u, apcSubTitle);
}

inline void BSSaveDataSystemUtilityFile::SetDirectoryName(const char* apcName) {
    BSstrcpy(cDirectoryName, 256, apcName);
}

inline void BSSaveDataSystemUtilityFile::SetDetail(const char* apcDetail) {
    BSstrcpy(cDetail, 1024, apcDetail);
}

inline void BSSaveDataSystemUtilityFile::SetBackgroundImagePath(const char* apcPath) {
    BSstrcpy(cBackgroundImagePath, 256, apcPath);
}

inline void BSSaveDataSystemUtilityFile::SetSaveCompleteCallback(
    void (*apSaveCompleteCallback)(BSSaveDataSystemUtility::RESULT, BSSaveDataSystemUtilityFile*&)
) {
    pSaveCompleteCallback = apSaveCompleteCallback;
}
