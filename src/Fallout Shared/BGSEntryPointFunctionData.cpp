#include "Fallout Shared/BGSEntryPointFunctionData.h"

#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"

// The base class's body of the pure virtual Save, which the derived
// kinds call first; it adds one one-byte chunk.
void BGSEntryPointFunctionData::Save() {
    uint8_t cfunctionDataType = GetType();
    TESForm::AddChunk(EPFT_ID, cfunctionDataType);
}
