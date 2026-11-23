#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"

#include "BSCore/MemoryManager.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadDefines.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFile.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFormHeader.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"

uint32_t BGSLoadGameBuffer::Load(BGSSaveLoadFile* apFile) {
    LoadGameDataFromFileFunction(apFile, &iBufferSize, 4u);
    uint32_t uiRead = Load(apFile, iBufferSize);
    if (uiRead == iBufferSize) {
        return uiRead;
    }

    return static_cast<uint32_t>(-1);
}

void BGSLoadGameBuffer::LoadDataFromBuffer(void* apData, size_t aiSize) {
    if (!iBufferSize) {
        return;
    }
    ourMemcpy(apData, &pBuffer[iBufferPosition], aiSize);
    iBufferPosition += aiSize;
    if (pBuffer[iBufferPosition] != '|') {
        SaveGameWarning("SAVELOAD: Invalid spacer hit while loading data from the buffer");
    }
    ++iBufferPosition;
}

// Reads a length-prefixed string, allocating it when the caller passes no buffer.
char* BGSLoadGameBuffer::LoadString(char* apString) {
    uint16_t usLength = 0;
    LoadDataFromBuffer(reinterpret_cast<char*>(&usLength), sizeof(usLength));
    if (usLength) {
        if (!apString) {
            apString = static_cast<char*>(ourMalloc(usLength + 1));
        }
        LoadDataFromBuffer(apString, usLength);
        apString[usLength] = '\0';
    }
    else if (apString) {
        *apString = '\0';
    }
    return apString;
}

// The plain buffer's data has the save game's current minor version.
uint8_t BGSLoadGameBuffer::GetVersion() {
    return pSaveLoadGame->GetCurrentMinorVersion();
}

// The plain buffer belongs to no form.
TESForm* BGSLoadGameBuffer::GetForm() {
    return nullptr;
}

TESObjectREFR* BGSLoadGameBuffer::GetReference() {
    return nullptr;
}

Actor* BGSLoadGameBuffer::GetActor() {
    return nullptr;
}

void BGSLoadGameBuffer::LoadData(void* apData, uint32_t aiSize) {
    LoadDataFromBuffer(apData, aiSize);
}

uint32_t BGSLoadGameBuffer::Load(BGSSaveLoadFile* apFile, uint32_t aiSize) {
    iBufferSize = aiSize;
    pBuffer = static_cast<char*>(ourMalloc(iBufferSize));
    uint32_t uiRead = apFile->Read(pBuffer, iBufferSize);
    if (uiRead == iBufferSize) {
        return uiRead;
    }

    return -1;
}

void BGSLoadGameBuffer::AdvanceBuffer(uint32_t aiNumBytes) {
    iBufferPosition += aiNumBytes;
}

void BGSLoadGameBuffer::SetBuffer(char* apBuffer, uint32_t aiSize, uint32_t aiPosition) {
    pBuffer = apBuffer;
    iBufferSize = aiSize;
    iBufferPosition = aiPosition;
}

BGSLoadGameBuffer::BGSLoadGameBuffer() : pBuffer(nullptr), iBufferSize(0), iBufferPosition(0) {}

// The buffer is the object's own allocation.
BGSLoadGameBuffer::~BGSLoadGameBuffer() {
    ourFree(pBuffer);
}

bool BGSLoadGameBuffer::LoadFormID(uint32_t& apFormID) {
    BGSNumericIDIndex kIndex;
    LoadDataFromBuffer(&kIndex, sizeof(kIndex));
    apFormID = kIndex.GetNumericID();
    return !kIndex.IsZero() && !apFormID;
}

uint32_t BGSLoadGameBuffer::LoadFormID() {
    BGSNumericIDIndex kIndex;
    LoadDataFromBuffer(&kIndex, sizeof(kIndex));
    return kIndex.GetNumericID();
}

uint32_t BGSLoadGameBuffer::LoadVariableSizedValue() {
    uint8_t cvalue = pBuffer[iBufferPosition];
    SIZE_TYPE esizeType = static_cast<SIZE_TYPE>(cvalue & 3);
    switch (esizeType) {
        case SIZE_CHAR: {
            uint8_t cvalue2 = 0;
            LoadDataFromBuffer(&cvalue2, sizeof(cvalue2));
            return cvalue2 >> 2;
        }
        case SIZE_SHORT: {
            uint16_t svalue = 0;
            LoadDataFromBuffer(&svalue, sizeof(svalue));
            return svalue >> 2;
        }
        case SIZE_INT: {
            uint32_t ivalue = 0;
            LoadDataFromBuffer(&ivalue, sizeof(ivalue));
            return ivalue >> 2;
        }
        default: return 0;
    }
}

char* BGSLoadGameBuffer::GetNextPos() {
    return &pBuffer[iBufferPosition];
}

void BGSLoadGameBuffer::LoadMatrixAsQuaternion(NiMatrix3* rot, int32_t aiSize) {
    NiQuaternion kQuaternion;
    LoadData(&kQuaternion, sizeof(kQuaternion));
    kQuaternion.ToRotation(*rot);
}
