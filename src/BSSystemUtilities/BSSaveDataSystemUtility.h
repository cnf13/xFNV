#pragma once

#include "Reconstruction.hpp"

#include <stdint.h>

class BSSaveDataSystemUtilityFile;

// The platform's save-data service: it lists, saves, loads and deletes save files asynchronously and keeps the
// result of the last operation of each kind. The PC build's is a thin class over the game's own save files.
class BSSaveDataSystemUtility {
public:
	// Values from the Xbox PDB.
	enum OPERATION : int32_t {
		OPERATION_NONE = 0x0,
		OPERATION_SAVE = 0x1,
		OPERATION_LOAD = 0x2,
		OPERATION_DELETE = 0x3,
		OPERATION_AUTOSAVE = 0x4,
		OPERATION_AUTOLOAD = 0x5,
	};

	// Values from the Xbox PDB.
	enum RESULT : int32_t {
		RESULT_SUCCESS = 0x0,
		RESULT_CANCEL = 0x1,
		RESULT_FAILED = 0x2,
	};

	// Values from the Xbox PDB.
	enum RESULT_FAILED_REASON : int32_t {
		RESULT_FAILED_REASON_NO_DATA = 0x0,
		RESULT_FAILED_REASON_INSUFFICIENT_SPACE = 0x1,
		RESULT_FAILED_REASON_DATA_CORRUPT = 0x2,
		RESULT_FAILED_REASON_UNKNOWN = 0x3,
	};

	// A save file's directory name.
	struct SaveDataEntry {
		char cName[32];
	};

	BSSaveDataSystemUtility();
	virtual ~BSSaveDataSystemUtility();

	virtual void Initialize();
	virtual void Destroy();
	virtual uint32_t GetSaveDataEntries(const char* apcDirectoryName, BSSaveDataSystemUtility::SaveDataEntry* apEntries,
		uint32_t auiMaxEntryCount);
	virtual bool GetMostRecentSaveDataEntry(const char* apcDirectoryName,
		BSSaveDataSystemUtility::SaveDataEntry& aEntry);
	virtual void SaveData(BSSaveDataSystemUtilityFile* apSaveDataFile);
	virtual void LoadData(BSSaveDataSystemUtilityFile* apSaveDataFile) { pSaveDataFile = apSaveDataFile; }
	virtual void DeleteData(const char* apcDirectoryName) {}
	virtual void AutoSaveData(BSSaveDataSystemUtilityFile* apSaveDataFile) { pSaveDataFile = apSaveDataFile; }
	virtual void AutoLoadData(BSSaveDataSystemUtilityFile* apSaveDataFile) { pSaveDataFile = apSaveDataFile; }

	bool GetMostRecentlyAccessedSaveDataEntry(BSSaveDataSystemUtility::SaveDataEntry& aEntry);

	// The singleton, constructed on first use.
	static BSSaveDataSystemUtility* GetSingleton();
	static void operator delete(void* apMemory);

	bool bIsActive;
	BSSaveDataSystemUtilityFile* pSaveDataFile;
	SaveDataEntry MostRecentEntry;
	OPERATION eCurrentOperation;
	RESULT eSaveResult;
	RESULT eLoadResult;
	RESULT eDeleteResult;
	RESULT_FAILED_REASON eResultFailedReason;
	uint32_t uiInsufficientSpaceFileSize;
	// The PDB's type; the PC build only clears it (constructor 0xAF2ED0).
	void (*pDeleteCompleteCallback)(RESULT);
};
inline constexpr fnvd::Vtable BSSaveDataSystemUtility_vftable{};
