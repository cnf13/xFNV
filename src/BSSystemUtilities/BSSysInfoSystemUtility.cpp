#include "BSSystemUtilities/BSSysInfoSystemUtility.h"

#include "BSCore/MemoryManager.h"
#include "SDK/MSVC/stdlib.h"

BSSysInfoSystemUtility::BSSysInfoSystemUtility() : eLanguage(LANGUAGE_UNKNOWN) {}

BSSysInfoSystemUtility::~BSSysInfoSystemUtility() {}

void BSSysInfoSystemUtility::Initialize() {
    ;
}

void BSSysInfoSystemUtility::Destroy() {
    eLanguage = BSSysInfoSystemUtility::LANGUAGE_UNKNOWN;
}

void BSSysInfoSystemUtility::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}

BSSysInfoSystemUtility* BSSysInfoSystemUtility::GetSingleton() {
    static BSSysInfoSystemUtility stsysInfoSystemUtility;
    return &stsysInfoSystemUtility;
}
