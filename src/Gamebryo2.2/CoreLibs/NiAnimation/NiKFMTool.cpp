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
	NiGlobalStringTable::GlobalStringHandle kOld = kString.GetHandle();
	// EXE-INLINED none: NiFixedString::operator=(const char*)
	if (kOld != acPath) {
		kString.SetHandle(NiGlobalStringTable::AddString(acPath));
		if (kOld != NiGlobalStringTable::NULL_STRING)
			InterlockedDecrementUInt32(reinterpret_cast<volatile uint32_t*>(const_cast<char*>(kOld) - 8));
	}
}
