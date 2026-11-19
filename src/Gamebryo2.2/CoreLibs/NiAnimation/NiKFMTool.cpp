#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiGlobalStringTable.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiPath.h"
#include "SDK/MSVC/string.h"

// The KFM file version this library writes; no code reads it.
extern const char* g_pcCurrentKFMVersion;
const char* g_pcCurrentKFMVersion = "2.2.0.0";

// Standardizes a 260-byte copy of the path and interns it in kString's place.
void NiStandardizeFilePath(NiFixedString& kString) {
    char acPath[0x104];
    strcpy_s(acPath, sizeof(acPath), kString);
    NiPath::Standardize(acPath);
    kString = acPath;
}
