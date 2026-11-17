#include "Fallout Shared/TESScriptVariable.h"

#include "BSCore/BSStringT.h"
#include "Fallout Shared/TESFile.h"

void ScriptVariable::Load(TESFile* apFile) {
	if (apFile != nullptr && apFile->GetTESChunk() == SLSD_ID) {
		apFile->GetChunkData(&data, sizeof(data));
#if FNVD_64BIT
		// The file's record is the 32-bit SCRIPT_LOCAL (0x18 bytes), its list link a 4-byte slot at 4.
		TESFile::WidenFormIDRecords(&data, 1, 0x18, sizeof(data), {{4, offsetof(SCRIPT_LOCAL, pNext)}});
#endif
		if (apFile->QEndian())
			data.Endian();
	}
}

SCRIPT_LOCAL::SCRIPT_LOCAL() : uiID(0), fValue(0.0), bIsInteger(false) {}

SCRIPT_LOCAL::SCRIPT_LOCAL(const SCRIPT_LOCAL* apSource)
	: uiID(apSource->uiID), fValue(apSource->fValue), bIsInteger(apSource->bIsInteger) {}

// Swaps the variable's id and value into the machine's byte order.
void SCRIPT_LOCAL::Endian() {
	ENDIAN(uiID, false);
	ENDIAN(fValue, false);
}

ScriptVariable::ScriptVariable() : data(), cName() {}

// The exe's cleanup-only C++ EH frame is left out.
ScriptVariable::ScriptVariable(const ScriptVariable* apSource) : data(), cName() {
	data.uiID = apSource->data.uiID;
	data.fValue = apSource->data.fValue;
	data.bIsInteger = apSource->data.bIsInteger;
	cName.Copy(apSource->cName);
}

ScriptVariable::~ScriptVariable() {}

bool ScriptVariable::Compare(ScriptVariable* apCompare) {
	if (!apCompare) {
		return 1;
	}
	if (data.bIsInteger != apCompare->data.bIsInteger) {
		return 1;
	}
	if (data.uiID != apCompare->data.uiID) {
		return 1;
	}
	if (apCompare->data.fValue == data.fValue) {
		return static_cast<uint8_t>(cName.IsNotEqual(apCompare->cName)) != 0;
	}
	return 1;
}
