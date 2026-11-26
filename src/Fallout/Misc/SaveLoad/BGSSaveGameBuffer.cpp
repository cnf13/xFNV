#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"

#include "BSCore/BSString.h"
#include "BSCore/NewOverloads.h"
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
    SaveDataToBuffer(apData, aiSize, false);
}

void BGSSaveGameBuffer::SaveFormID(TESForm* apForm, uint32_t aiUnusuedMetaDataArg) {
    uint32_t uiFormID = apForm ? apForm->TESForm::GetFormID() : 0;
    BGSNumericIDIndex kIndex;
    kIndex.SetNumericID(uiFormID);
    SaveDataToBuffer(&kIndex, sizeof(kIndex), false);
}

// Writes the length's low 16 bits, then the string's bytes when the length is not 0.
void BGSSaveGameBuffer::SaveString(const char* apString, uint32_t aiUnusuedMetaDataArg) {
    uint16_t sTempLength = 0;
    if (apString) {
        sTempLength = static_cast<uint16_t>(std::strlen(apString));
    }
    uint16_t sLength = sTempLength;
    SaveDataToBuffer(&sTempLength, sizeof(sTempLength), false);
    if (sLength) {
        SaveDataToBuffer(const_cast<char*>(apString), sLength, false);
    }
}

uint32_t BGSSaveGameBuffer::StartVariableSizedValue() {
    uint32_t uiPosition = iBufferPosition;
    int8_t cPlaceholder = 35;
    SaveDataToBuffer(&cPlaceholder, sizeof(cPlaceholder), iBufferPosition, false);
    return uiPosition;
}

void BGSSaveGameBuffer::SaveVariableSizedValue(uint32_t aiVariableValue, uint32_t aiPosition) {
    if (aiVariableValue >= 64) {
        if (aiVariableValue >= 0x4000) {
            if (aiVariableValue >= 0x40000000) {
                SaveGameWarning("SAVELOAD: Variable-sized value %08X is too large to be stored.", aiVariableValue);
            }
            else {
                ShiftBuffer(aiPosition, 3);
                uint32_t uiValue = 4 * aiVariableValue | 2;
                SaveDataToBuffer(&uiValue, sizeof(uiValue), aiPosition, false);
            }
        }
        else {
            ShiftBuffer(aiPosition, 1);
            uint16_t sValue = 4 * aiVariableValue | 1;
            SaveDataToBuffer(&sValue, sizeof(sValue), aiPosition, false);
        }
    }
    else {
        uint8_t cValue = 4 * aiVariableValue;
        SaveDataToBuffer(&cValue, sizeof(cValue), aiPosition, false);
    }
}

void BGSSaveGameBuffer::SaveVariableSizedValue(uint32_t auiSize) {
    if (auiSize >= 64) {
        if (auiSize >= 0x4000) {
            if (auiSize >= 0x40000000) {
                SaveGameWarning("SAVELOAD: Variable-sized value %08X is too large to be stored.", auiSize);
            }
            else {
                uint32_t uiValue = 4 * auiSize | 2;
                SaveDataToBuffer(&uiValue, sizeof(uiValue), false);
            }
        }
        else {
            uint16_t sValue = 4 * auiSize | 1;
            SaveDataToBuffer(&sValue, sizeof(sValue), false);
        }
    }
    else {
        uint8_t cValue = 4 * auiSize;
        SaveDataToBuffer(&cValue, sizeof(cValue), false);
    }
}

void BGSSaveGameBuffer::SaveFormID(uint32_t aiFormID, uint32_t aiUnusuedMetaDataArg) {
    BGSNumericIDIndex kIndex;
    kIndex.SetNumericID(aiFormID);
    SaveDataToBuffer(&kIndex, sizeof(kIndex), false);
}

void BGSSaveGameBuffer::SaveDataToBuffer(void* apData, size_t aiSize, uint32_t aiPosition, bool abDirectSave) {
    int32_t iMarkerSize = !abDirectSave;
    while (iMarkerSize + (aiSize + aiPosition) > iBufferSize) {
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
    if (iMarkerSize + (aiSize + aiPosition) > iBufferPosition) {
        iBufferPosition = iMarkerSize + (aiSize + aiPosition);
    }
}

void BGSSaveGameBuffer::Save(BGSSaveLoadFile* apFile, uint32_t aiSize) {
    if (pSaveLoadManager->GetSimulatedMode()) {
        if (aiSize) {
            apFile->Write(nullptr, aiSize);
        }
    }
    else if (pBuffer && aiSize) {
        apFile->Write(pBuffer, aiSize);
    }
}

void BGSSaveGameBuffer::Save(BGSSaveLoadFile* apFile) {
    SaveGameDataToFileFunction(apFile, &iBufferPosition, 4);
    Save(apFile, iBufferPosition);
}

void BGSSaveGameBuffer::ExpandBuffer() {
    char* pOldBuffer = pBuffer;
    uint32_t uiOldBufferSize = iBufferSize;
    iBufferSize = uiOldBufferSize + 0x10000;
    if (!pSaveLoadManager->bSimulatedMode) {
        pBuffer = static_cast<char*>(ourMalloc(iBufferSize));
        if (pOldBuffer) {
            ourMemcpy(pBuffer, pOldBuffer, uiOldBufferSize);
        }
        ourFree(pOldBuffer);
    }
}

// The shifted position is compared with the buffer size signed, and the two unsigned byte counts are passed to
// BSmemmove's int32_t parameters as they are.
void BGSSaveGameBuffer::ShiftBuffer(uint32_t aiPosition, int32_t aiAmount) {
    int32_t iShiftedPosition = aiAmount + iBufferPosition;
    if (iShiftedPosition > 0) {
        while (iShiftedPosition > static_cast<int32_t>(iBufferSize)) {
            ExpandBuffer();
        }
        if (!pSaveLoadManager->bSimulatedMode) {
            char* pSource = pBuffer + aiPosition;
            char* pDest = pBuffer + aiPosition + aiAmount;
            uint32_t uiDestSize = iBufferSize - (aiPosition + aiAmount);
            BSmemmove(
                pDest,
                static_cast<int32_t>(uiDestSize),
                pSource,
                static_cast<int32_t>(iBufferPosition - aiPosition)
            );
        }
        iBufferPosition = iShiftedPosition;
    }
}
