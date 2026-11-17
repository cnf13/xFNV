#include "BSSystemUtilities/BSSaveDataSystemUtility.h"

#include "BSCore/MemoryManager.h"
#include "BSSystemUtilities/BSSaveDataSystemUtilityFile.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

BSSaveDataSystemUtility::BSSaveDataSystemUtility()
  : bIsActive(false), pSaveDataFile(nullptr), eCurrentOperation(OPERATION_NONE), pDeleteCompleteCallback(nullptr) {
    MostRecentEntry.cName[0] = 0;
}

BSSaveDataSystemUtility::~BSSaveDataSystemUtility() {}

void BSSaveDataSystemUtility::Initialize() {
    Destroy();
}

void BSSaveDataSystemUtility::Destroy() {
    pSaveDataFile = nullptr;
}

// The PC build lists no entries.
uint32_t BSSaveDataSystemUtility::GetSaveDataEntries(
    const char* apcDirectoryName,
    SaveDataEntry* apEntries,
    uint32_t auiMaxEntryCount
) {
    return 0;
}

bool BSSaveDataSystemUtility::GetMostRecentSaveDataEntry(const char* apcDirectoryName, SaveDataEntry& aEntry) {
    return (bool)0;
}

void BSSaveDataSystemUtility::SaveData(BSSaveDataSystemUtilityFile* apSaveDataFile) {
    pSaveDataFile = apSaveDataFile;
}

bool BSSaveDataSystemUtility::GetMostRecentlyAccessedSaveDataEntry(BSSaveDataSystemUtility::SaveDataEntry& aEntry) {
    if (!std::strlen(MostRecentEntry.cName)) {
        return 0;
    }
    strcpy_s(aEntry.cName, 32, MostRecentEntry.cName);
    return 1;
}

BSSaveDataSystemUtility* BSSaveDataSystemUtility::GetSingleton() {
    static BSSaveDataSystemUtility stsaveDataSystemUtility;
    return &stsaveDataSystemUtility;
}

void BSSaveDataSystemUtility::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
}
