#include "Fallout Shared/TESImageSpace.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESForm.h"

TESImageSpace::TESImageSpace() : TESForm() {
	SetFormType(IMGS_ID);
	TESImageSpace::InitializeData();
}

TESImageSpace::~TESImageSpace() {}

// Clears the image space values.
void TESImageSpace::InitializeData() {
	ourMemset(&Data, 0, sizeof(Data));
}

// Reads the image space's chunks: the editor ID, the bounds and the values. A DNAM chunk shorter than the values fills
// the HDR data only, forces the last HDR value to 1 and copies the rest of the values from the same offset of the
// chunk; the buffer deliberately keeps whatever the short read left, as the game does.
bool TESImageSpace::Load(TESFile* apFile) {
	uint8_t ucFormType = static_cast<uint8_t>(apFile->GetTESForm());
	if (ucFormType != IMGS_ID) {
		return false;
	}
	LoadForm(apFile);
	SetInitialized(false);
	do {
		uint32_t uiChunk = apFile->GetTESChunk();
		if (uiChunk == NO_CHUNK) {
			break;
		}
		switch (uiChunk) {
		case EDID_ID: {
			// A 16-byte aligned buffer of the chunk's size, read up to 0x200.
			char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
			apFile->GetChunkData(pBuffer, 0x200);
			SetFormEditorID(pBuffer);
			break;
		}
		case OBND_ID:
			LoadObjectBound(apFile);
			break;
		case DNAM_ID: {
			ImageSpaceParameterData kData;
			apFile->GetChunkData(&kData, 0x98u);
			if (apFile->GetChunkSize() >= 0x98u) {
				BSmemcpy(&Data, sizeof(Data), &kData, sizeof(Data));
			} else {
				BSmemcpy(&Data, 0x3C, &kData, 0x38);
				Data.pfHDRData[14] = 1.0f;
				// Copies from the chunk's last HDR value, four bytes before the rest of the values, as the game does.
				BSmemcpy(&Data.pfBloomData,
					sizeof(ImageSpaceParameterData) - offsetof(ImageSpaceParameterData, pfBloomData),
					&kData.pfHDRData[14],
					sizeof(ImageSpaceParameterData) - offsetof(ImageSpaceParameterData, pfBloomData));
			}
			if (apFile->QEndian()) {
				Data.Endian();
			}
			break;
		}
		}
	} while (apFile->NextChunk());
	return true;
}

// Writes the values as the DNAM chunk, byte swapped around the write when the save swaps.
void TESImageSpace::Save() {
	TESForm::StartForm();
	if (TESForm::QEndianSwapOnSave()) {
		Data.Endian();
	}
	TESForm::__AddChunkData(DNAM_ID, &Data, 0x98u);
	if (TESForm::QEndianSwapOnSave()) {
		Data.Endian();
	}
	TESForm::CloseForm();
}

void TESImageSpace::Copy(TESForm* apCopy) {
	TESImageSpace* pCopy = dynamic_cast<TESImageSpace*>(apCopy);
	if (pCopy) {
		TESForm::CopyAllComponents(pCopy);
		ourMemcpy(&Data, &pCopy->Data, sizeof(Data));
	}
}

// True when the forms differ: apCopy is not an image space, its components differ, or its values do.
bool TESImageSpace::Compare(TESForm* apCompare) {
	TESImageSpace* pCopy = dynamic_cast<TESImageSpace*>(apCompare);
	if (!pCopy) {
		return true;
	}
	if (TESForm::CompareAllComponents(pCopy)) {
		return true;
	}
	return std::memcmp(&Data, &pCopy->Data, sizeof(Data)) != 0;
}

void TESImageSpace::MakeDefault() {
	Data.pfHDRData[0] = 0.5f;
	Data.pfHDRData[1] = 7.0f;
	Data.pfHDRData[3] = 1.0;
	Data.pfHDRData[4] = 1.0;
	Data.pfHDRData[5] = 1.0;
	Data.pfHDRData[6] = 2.25f;
	Data.pfHDRData[7] = 0.225f;
	Data.pfHDRData[8] = 1.0;
	Data.pfHDRData[9] = 0.0;
	Data.pfHDRData[10] = 0.0;
	Data.pfHDRData[11] = 1.5f;
	Data.pfHDRData[12] = 1.5f;
	Data.pfHDRData[13] = 1.0;
	Data.pfHDRData[2] = 1.0;
	Data.pfBloomData[0] = 0.03f;
	Data.pfBloomData[2] = 0.2f;
	Data.pfBloomData[1] = 0.7f;
	Data.pfGetHitData[0] = 0.5f;
	Data.pfGetHitData[1] = 0.5f;
	Data.pfGetHitData[2] = 0.02f;
	Data.pfNightEyeData[3] = 2.5f;
	Data.pfNightEyeData[0] = 0.21f;
	Data.pfNightEyeData[1] = 0.5f;
	Data.pfNightEyeData[2] = 0.78f;
	Data.pfCinematicData[3] = 1.0;
	Data.pfCinematicData[1] = 0.5f;
	Data.pfCinematicData[2] = 1.0;
	Data.pfCinematicData[0] = 1.0;
	Data.pfCinematicData[7] = 0.0;
	Data.pfCinematicData[4] = 1.0;
	Data.pfCinematicData[5] = 1.0;
	Data.pfCinematicData[6] = 1.0;
}
