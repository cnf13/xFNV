#include "Fallout Shared/AudioMarkerData.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"

#include <bit>

AudioBuoyMarkerData::AudioBuoyMarkerData() : AudioMarker() {}

// The exe's destructor (0x483710) is empty.
AudioBuoyMarkerData::~AudioBuoyMarkerData() {}

bool AudioBuoyMarkerData::Compare(AudioBuoyMarkerData* apCompare) {
	return true;
}

bool AudioBuoyMarkerData::Load(TESFile* apFile) {
	return (bool)((uint8_t)(apFile && (apFile->GetTESChunk() == 1263684941)));
}

// The PDB's AudioMarkerData::GetLocationController. The exe reads the controller field as a form ID inline (the
// field holds the ID until the controller is looked up).
TESForm* AudioMarker::GetMediaLocationController() {
	return TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pMediaLocationController)));
}

void AudioMarker::Save() {
	TESForm::AddChunkArray(FULL_ID, kName.GetFullName(), kName.GetFullNameLength() + 1);
	if (pMediaLocationController) {
		// The field holds the controller's form ID until the controller is looked up.
		TESForm::AddChunk(CNAM_ID, static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pMediaLocationController)));
	}
	TESForm::AddChunk(BNAM_ID, static_cast<uint32_t>(bUseControllerValues));
	TESForm::AddChunk(MNAM_ID, std::bit_cast<uint32_t>(fLayer2TriggerPerc));
	TESForm::AddChunk(NNAM_ID, std::bit_cast<uint32_t>(fLayer3TriggerPerc));
}
