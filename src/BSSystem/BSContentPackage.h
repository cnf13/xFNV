#pragma once

#include <stdint.h>

int32_t return_0_2();

// One downloadable content package found by BSDLCSystemUtility::LoadDLC: the package's display name and a root name.
class BSContentPackage {
public:
    // Stores apDisplayName, the DLC table entry's wide name LoadDLC passes.
    BSContentPackage(const char16_t* apDisplayName);
    // The PC body does nothing.
    ~BSContentPackage();

    // The display name.
    const char16_t* DisplayName() const;
    // The package's file name. On PC it returns the empty string.
    const char* FileName() const;

    const char16_t* m_contentData;
    char* m_rootName;
};
