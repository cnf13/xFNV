#pragma once

#include "BSSystem/BSMemoryFile.h"
#include "BSSystemUtilities/BSSaveDataSystemUtility.h"
#include "Reconstruction.hpp"

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
	void SetSaveCompleteCallback(void (*apSaveCompleteCallback)(BSSaveDataSystemUtility::RESULT,
		BSSaveDataSystemUtilityFile*&));

	char cDirectoryName[256];
	bool bSingleSave;
	char cSubTitle[128];
	char cDetail[1024];
	char cCreateIconImagePath[256];
	uint8_t padding7E5[3];
	uint32_t uiIconImageWidth;
	uint32_t uiIconImageHeight;
	void* pvIconImageBuffer;
	char cIconImagePath[256];
	char cBackgroundImagePath[256];
	// The PDB's BSSaveDataSystemUtility::SaveCompleteCallback and LoadCompleteCallback types (cdecl: the exe's
	// BGSSaveLoadManager::SysUtilSaveCompleteCallback, 0x852740, is the one SetSaveCompleteCallback stores).
	void (*pSaveCompleteCallback)(BSSaveDataSystemUtility::RESULT aeResult, BSSaveDataSystemUtilityFile*& arpFile);
	void (*pLoadCompleteCallback)(BSSaveDataSystemUtility::RESULT aeResult, BSSaveDataSystemUtilityFile*& arpFile);
};
