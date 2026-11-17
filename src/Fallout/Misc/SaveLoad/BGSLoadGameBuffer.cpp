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
    uint32_t v4 = Load(apFile, iBufferSize);
    if (v4 == ((uint32_t)((int32_t)iBufferSize))) {
        return v4;
    }
    else {
        return (uint32_t)(-1);
    }
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
    LoadDataFromBuffer(reinterpret_cast<char*>(&usLength), 2);
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
    uint32_t v5 = apFile->Read(pBuffer, iBufferSize);
    if (v5 == iBufferSize) {
        return v5;
    }
    else {
        return -1;
    }
}

void BGSLoadGameBuffer::AdvanceBuffer(uint32_t aiNumBytes) {
    iBufferPosition += ((int32_t)aiNumBytes);
}

void BGSLoadGameBuffer::SetBuffer(char* apBuffer, uint32_t aiSize, uint32_t aiPosition) {
    pBuffer = apBuffer;
    iBufferSize = (uint32_t)((int32_t)aiSize);
    iBufferPosition = (uint32_t)((int32_t)aiPosition);
}

BGSLoadGameBuffer::BGSLoadGameBuffer() : pBuffer(nullptr), iBufferSize(0), iBufferPosition(0) {}

// The buffer is the object's own allocation.
BGSLoadGameBuffer::~BGSLoadGameBuffer() {
    ourFree(pBuffer);
}

bool BGSLoadGameBuffer::LoadFormID(uint32_t& apFormID) {
    BGSNumericIDIndex apMemory;
    LoadDataFromBuffer(&apMemory, 3u);
    apFormID = apMemory.GetNumericID();
    return !apMemory.IsZero() && !apFormID;
}

uint32_t BGSLoadGameBuffer::LoadFormID() {
    BGSNumericIDIndex a2;
    a2.~BGSNumericIDIndex();
    ::new (&a2) BGSNumericIDIndex;
    LoadDataFromBuffer(static_cast<char*>(static_cast<void*>(&a2)), 3u);
    return (&a2)->GetNumericID();
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
    return static_cast<char*>(static_cast<void*>(&pBuffer[iBufferPosition]));
}

void BGSLoadGameBuffer::LoadMatrixAsQuaternion(NiMatrix3* rot, int32_t a2) {
    NiQuaternion ecx0;
    LoadData(&ecx0, 16u);
    ecx0.ToRotation(*rot);
}
