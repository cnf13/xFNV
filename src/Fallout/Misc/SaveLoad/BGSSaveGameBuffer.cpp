#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"

#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadDefines.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFile.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFormHeader.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadManager.h"
#include "SDK/MSVC/string.h"

BGSSaveGameBuffer::BGSSaveGameBuffer() : pBuffer(nullptr), iBufferSize(0), iBufferPosition(0), iDebugOverhead(0) {}

// The buffer is the object's own allocation.
BGSSaveGameBuffer::~BGSSaveGameBuffer() {
	ourFree(pBuffer);
}

// All three functions return null.
TESForm* BGSSaveGameBuffer::GetForm() {
	return nullptr;
}

TESObjectREFR* BGSSaveGameBuffer::GetReference() {
	return nullptr;
}

Actor* BGSSaveGameBuffer::GetActor() {
	return nullptr;
}

void BGSSaveGameBuffer::SaveDataToBuffer(void* apData, size_t aiSize, bool abDirectSave) {
	SaveDataToBuffer(apData, aiSize, iBufferPosition, abDirectSave);
}

void BGSSaveGameBuffer::SaveDataEndian(void* apData, uint32_t aiSize, uint32_t aiUnusuedMetaDataArg) {
	SaveDataToBuffer(apData, aiSize, (bool)0);
}

void BGSSaveGameBuffer::SaveFormID(TESForm* apForm, uint32_t aiUnusuedMetaDataArg) {
	uint32_t v3;
	BGSNumericIDIndex apMemory;
	if (apForm) {
		v3 = apForm->TESForm::GetFormID();
	} else {
		v3 = 0;
	}
	apMemory.~BGSNumericIDIndex();
	::new (&apMemory) BGSNumericIDIndex;
	(&apMemory)->SetNumericID(v3);
	SaveDataToBuffer(static_cast<char*>(static_cast<void*>(&apMemory)), 3u, (bool)0);
}

// Writes the length's low 16 bits, then the string's bytes when the length is not 0.
void BGSSaveGameBuffer::SaveString(const char* apString, uint32_t aiUnusuedMetaDataArg) {
	uint16_t stempLength;
	if (apString) {
		stempLength = static_cast<uint16_t>(std::strlen(apString));
	} else {
		stempLength = 0;
	}
	uint16_t slength = stempLength;
	SaveDataToBuffer(&stempLength, 2u, false);
	if (slength) {
		SaveDataToBuffer(const_cast<char*>(apString), slength, false);
	}
}

uint32_t BGSSaveGameBuffer::StartVariableSizedValue() {
	uint32_t iBufferPosition;
	iBufferPosition = this->iBufferPosition;
	int8_t a2 = 35;
	SaveDataToBuffer(static_cast<char*>(static_cast<void*>(&a2)), 1u, (int32_t)this->iBufferPosition, 0);
	return iBufferPosition;
}

void BGSSaveGameBuffer::SaveVariableSizedValue(uint32_t aiVariableValue, uint32_t aiPosition) {
	uint32_t v4;
	uint8_t cvalue;
	if (aiVariableValue >= 64) {
		if (aiVariableValue >= 0x4000) {
			if (aiVariableValue >= 0x40000000) {
				SaveGameWarning("SAVELOAD: Variable-sized value %08X is too large to be stored.", aiVariableValue);
			} else {
				ShiftBuffer(aiPosition, 3);
				v4 = 4 * aiVariableValue | 2;
				SaveDataToBuffer(&v4, 4u, aiPosition, 0);
			}
		} else {
			uint16_t svalue;
			ShiftBuffer(aiPosition, 1);
			svalue = 4 * aiVariableValue | 1;
			SaveDataToBuffer(&svalue, 2u, aiPosition, 0);
		}
	} else {
		cvalue = 4 * aiVariableValue;
		SaveDataToBuffer(&cvalue, 1u, aiPosition, 0);
	}
}

void BGSSaveGameBuffer::SaveVariableSizedValue(uint32_t auiSize) {
	uint32_t v2;
	uint8_t cvalue;
	if (auiSize >= 64) {
		if (auiSize >= 0x4000) {
			if (auiSize >= 0x40000000) {
				SaveGameWarning("SAVELOAD: Variable-sized value %08X is too large to be stored.", auiSize);
			} else {
				v2 = 4 * auiSize | 2;
				SaveDataToBuffer(&v2, 4u, 0);
			}
		} else {
			uint16_t svalue = 4 * auiSize | 1;
			SaveDataToBuffer(&svalue, 2u, 0);
		}
	} else {
		cvalue = 4 * auiSize;
		SaveDataToBuffer(&cvalue, 1u, 0);
	}
}

void BGSSaveGameBuffer::SaveFormID(uint32_t aiFormID, uint32_t aiUnusuedMetaDataArg) {
	BGSNumericIDIndex apMemory;
	apMemory.~BGSNumericIDIndex();
	::new (&apMemory) BGSNumericIDIndex;
	(&apMemory)->SetNumericID(aiFormID);
	SaveDataToBuffer(static_cast<char*>(static_cast<void*>(&apMemory)), 3u, (bool)0);
}

void BGSSaveGameBuffer::SaveDataToBuffer(void* apData, size_t aiSize, uint32_t aiPosition, bool abDirectSave) {
	int32_t v6 = !abDirectSave;
	while (v6 + (aiSize + aiPosition) > iBufferSize) {
		ExpandBuffer();
	}
	if (!pSaveLoadManager->GetSimulatedMode()) {
		ourMemcpy(&pBuffer[aiPosition], apData, aiSize);
	}
	if (!abDirectSave) {
		if (!pSaveLoadManager->GetSimulatedMode()) {
			pBuffer[aiSize + aiPosition] = 124;
		}
		if (aiPosition == iBufferPosition) {
			++iDebugOverhead;
		}
	}
	if (v6 + (aiSize + aiPosition) > iBufferPosition) {
		iBufferPosition = v6 + (aiSize + aiPosition);
	}
}

void BGSSaveGameBuffer::Save(BGSSaveLoadFile* apFile, uint32_t aiSize) {
	if (pSaveLoadManager->GetSimulatedMode()) {
		if (aiSize) {
			apFile->Write(nullptr, aiSize);
		}
	} else {
		if (pBuffer) {
			if (aiSize) {
				apFile->Write(pBuffer, aiSize);
			}
		}
	}
}

void BGSSaveGameBuffer::Save(BGSSaveLoadFile* apFile) {
	SaveGameDataToFileFunction(apFile, &iBufferPosition, 4);
	Save(apFile, iBufferPosition);
}

void BGSSaveGameBuffer::ExpandBuffer() {
	char* poldBuffer = pBuffer;
	uint32_t ioldBufferSize = iBufferSize;
	iBufferSize = ioldBufferSize + 0x10000;
	if (!pSaveLoadManager->bSimulatedMode) {
		pBuffer = static_cast<char*>(ourMalloc(iBufferSize));
		if (poldBuffer) {
			ourMemcpy(pBuffer, poldBuffer, ioldBufferSize);
		}
		ourFree(poldBuffer);
	}
}

// The shifted position is compared with the buffer size signed, and the two unsigned byte counts are passed to
// BSmemmove's int32_t parameters as they are.
void BGSSaveGameBuffer::ShiftBuffer(uint32_t aiPosition, int32_t aiAmount) {
	int32_t ishiftedPosition = aiAmount + iBufferPosition;
	if (ishiftedPosition > 0) {
		while (ishiftedPosition > static_cast<int32_t>(iBufferSize)) {
			ExpandBuffer();
		}
		if (!pSaveLoadManager->bSimulatedMode) {
			char* psource = pBuffer + aiPosition;
			char* pdest = pBuffer + aiPosition + aiAmount;
			uint32_t idestSize = iBufferSize - (aiPosition + aiAmount);
			BSmemmove(pdest, static_cast<int32_t>(idestSize), psource,
				static_cast<int32_t>(iBufferPosition - aiPosition));
		}
		iBufferPosition = ishiftedPosition;
	}
}

// The int32_t sizes are passed on to memmove_s as 32-bit unsigned values, zero-extended, as the game does.
int32_t BSmemmove(void* apDest, int32_t aiDestSize, void* apSrc, int32_t aiCount) {
	return memmove_s(apDest, static_cast<uint32_t>(aiDestSize), apSrc, static_cast<uint32_t>(aiCount));
}
