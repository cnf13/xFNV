#include "Fallout Shared/ExtraDataObjects.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFullName.h"
#include "Fallout Shared/TESReputation.h"

MapMarkerData::MapMarkerData() : cFlags(0), cOriginalFlags(0), sType(0), pReputation(nullptr) {}

const char* MapMarkerData::GetLocationName() {
	return LocationName.GetFullName();
}

bool MapMarkerData::Load(TESFile* apFile) {
	if (!apFile) {
		return 0;
	}
	if (apFile->GetTESChunk() != XMRK_ID) {
		return 0;
	}
	if (!apFile->NextChunk()) {
		return 0;
	}
	if (apFile->GetTESChunk() == FNAM_ID) {
		apFile->GetChunkData(&cFlags, 1u);
		cOriginalFlags = cFlags;
		if (!apFile->NextChunk()) {
			return 0;
		}
	}
	if (apFile->GetTESChunk() != FULL_ID) {
		return 0;
	}
	TESFullName::LoadFullNameChunk(&LocationName, apFile);
	if (!apFile->NextChunk()) {
		return 0;
	}
	if (apFile->GetTESChunk() != TNAM_ID) {
		return 0;
	}
	apFile->LoadWord(reinterpret_cast<int16_t*>(&sType));
	uint32_t uRewind = apFile->GetOffsetChunk();
	if (!apFile->NextChunk()) {
		return 0;
	}
	if (apFile->GetTESChunk() == WMI1_ID) {
		uint32_t itempint = 0;
		apFile->GetChunkData<uint32_t>(itempint);
		if (itempint) {
			pReputation = reinterpret_cast<TESReputation*>(static_cast<uintptr_t>(itempint));
		}
	} else {
		apFile->SetOffsetChunk(uRewind);
	}
	return 1;
}

void MapMarkerData::Save() {
	TESForm::AddChunk(FNAM_ID, cFlags);
	TESForm::AddChunkArray(FULL_ID, LocationName.GetFullName(), LocationName.GetFullNameLength() + 1);
	TESForm::AddChunk(TNAM_ID, sType);
	if (pReputation) {
		TESForm::AddChunk(WMI1_ID, pReputation->iFormID);
	}
}

bool MapMarkerData::Compare(MapMarkerData* apCompare) {
	if (LocationName.CompareComponent(&apCompare->LocationName)) {
		return 1;
	}
	if (cFlags != apCompare->cFlags) {
		return 1;
	}
	if (sType == apCompare->sType) {
		return pReputation != apCompare->pReputation;
	}
	return 1;
}

bool MapMarkerData::GetHidden() {
	return (cFlags & 4) != 0;
}
