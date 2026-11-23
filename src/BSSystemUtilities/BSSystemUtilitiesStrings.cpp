#include "BSSystemUtilities/BSSystemUtilitiesStrings.h"

#include "BSCore/MemoryManager.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"

BSSystemUtilitiesStrings::BSSystemUtilitiesStrings() {
    for (char*& pcString : pcStrings) {
        pcString = nullptr;
    }
}

BSSystemUtilitiesStrings::~BSSystemUtilitiesStrings() {}

void BSSystemUtilitiesStrings::Initialize() {
    Destroy();
}

void BSSystemUtilitiesStrings::Destroy() {
    for (char*& pcString : pcStrings) {
        if (pcString) {
            MemoryManager::Instance().Deallocate(pcString);
            pcString = nullptr;
        }
    }
}

void BSSystemUtilitiesStrings::SetString(BSSystemUtilitiesStrings::STRING aeType, const char* apcString) {
    if (pcStrings[aeType]) {
        MemoryManager::Instance().Deallocate(pcStrings[aeType]);
        pcStrings[aeType] = nullptr;
    }
    if (apcString) {
        size_t Size = std::strlen(apcString) + 1;
        pcStrings[aeType] = static_cast<char*>(MemoryManager::Instance().Allocate(Size));
        std::memset(pcStrings[aeType], 0, Size);
        strcpy_s(pcStrings[aeType], Size, apcString);
    }
}

void BSSystemUtilitiesStrings::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}

BSSystemUtilitiesStrings* BSSystemUtilitiesStrings::QInstance() {
    static BSSystemUtilitiesStrings ststringsUtility;
    return &ststringsUtility;
}
