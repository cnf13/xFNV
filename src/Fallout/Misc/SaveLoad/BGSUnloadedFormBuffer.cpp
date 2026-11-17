#include "Fallout/Misc/SaveLoad/BGSUnloadedFormBuffer.h"

#include "BSCore/MemoryManager.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadDefines.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFile.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadFormHeader.h"
#include "SDK/FaceGen/StrContainers.h"
#include "SDK/FaceGen/algs.h"

#include <map>
#include <memory>
#include <stdint.h>
#include <string>
#include <vector>

BGSUnloadedFormBuffer::BGSUnloadedFormBuffer() : pBuffer(nullptr) {}

uint8_t BGSUnloadedFormBuffer::GetVersion() {
    return (uint8_t)pBuffer[1];
}

SIZE_TYPE BGSUnloadedFormBuffer::GetSizeType() {
    return reinterpret_cast<BGSSaveLoadFormInfo*>(pBuffer)->GetSizeType();
}

ENUM_FORM_ID BGSUnloadedFormBuffer::GetFormType() {
    return reinterpret_cast<BGSSaveLoadFormInfo*>(pBuffer)->GetFormType();
}

char* BGSUnloadedFormBuffer::GetAdvancedBuffer() {
    if (!pBuffer) {
        return 0;
    }
    // The buffer's first byte is read as a BGSSaveLoadFormInfo.
    SIZE_TYPE esizeType = reinterpret_cast<BGSSaveLoadFormInfo*>(pBuffer)->GetSizeType();
    return &pBuffer[GetSizeTypeSize(esizeType) + 2];
}

// The header is built on the stack: the form ID, the change flags, then the buffer's info byte and version.
void BGSUnloadedFormBuffer::Save(BGSSaveLoadFile* apFile, uint32_t aiFormID, BGSChangeFlags aiChangeFlags) {
    char* pheader = pBuffer;
    char* pbuffer = pBuffer + 2;
    BGSSaveLoadFormHeader header;
    header.FormIDIndex.SetNumericID(aiFormID);
    header.iChangeFlags = aiChangeFlags;
    header.FormInfo.cData = static_cast<uint8_t>(pheader[0]);
    header.cVersion = static_cast<uint8_t>(pheader[1]);
    apFile->Write(&header, 9u);
    SIZE_TYPE esizeType = header.FormInfo.GetSizeType();
    uint32_t isize = 0;
    pbuffer += LoadGameSizeFromBuffer(pbuffer, isize, esizeType);
    SaveGameSizeToFile(apFile, isize, esizeType);
    if (isize) {
        apFile->Write(pbuffer, isize);
    }
}

void BGSUnloadedFormBuffer::Load(BGSSaveLoadFile* apFile) {
    BGSSaveLoadFormHeader header;
    apFile->Read(&header, 9u);
    SIZE_TYPE esizeType = header.FormInfo.GetSizeType();
    uint32_t idataSize = 0;
    uint32_t isizeTypeSize = LoadGameSizeFromFile(apFile, idataSize, esizeType);
    uint32_t isize = idataSize + isizeTypeSize + 2;
    pBuffer = static_cast<char*>(ourMalloc(isize));
    char* punloadedHeader_2 = pBuffer;
    char* punloadedHeader = punloadedHeader_2;
    punloadedHeader_2[1] = header.cVersion;
    *punloadedHeader = header.FormInfo.cData;
    punloadedHeader_2 += 2;
    SaveGameSizeToBuffer(punloadedHeader_2, idataSize, esizeType);
    punloadedHeader_2 += isizeTypeSize;
    if (idataSize) {
        apFile->Read(punloadedHeader_2, idataSize);
    }
}

uint32_t BGSUnloadedFormBuffer::GetSize() {
    uint32_t ariSize;
    if (!pBuffer) {
        return 0;
    }
    SIZE_TYPE aeSizeType = GetSizeType();
    char* apBuffer = pBuffer + 2;
    ariSize = 0;
    LoadGameSizeFromBuffer(apBuffer, ariSize, aeSizeType);
    return ariSize;
}

uint32_t BGSUnloadedFormBuffer::GetOverhead() {
    SIZE_TYPE SizeType = GetSizeType();
    return GetSizeTypeSize(SizeType) + 9;
}

BGSLoadFormBuffer* BGSUnloadedFormBuffer::FillLoadFormBuffer() {
    BGSLoadFormBuffer* v4;
    BGSLoadFormBuffer* apMemory = static_cast<BGSLoadFormBuffer*>(ourMalloc(sizeof(BGSLoadFormBuffer)));
    if (apMemory) {
        v4 = ::new (apMemory) BGSLoadFormBuffer;
    }
    else {
        v4 = 0;
    }
    uint8_t Version = GetVersion();
    v4->Header.cVersion = Version;
    SIZE_TYPE aeSizeType = GetSizeType();
    int32_t a4 = 2;
    char* apBuffer = pBuffer + 2;
    uint32_t ariSize = 0;
    uint32_t v2 = LoadGameSizeFromBuffer(apBuffer, ariSize, aeSizeType);
    a4 += v2;
    v4->SetBuffer(pBuffer, a4 + ariSize, a4);
    return v4;
}

void BGSUnloadedFormBuffer::Initialize(BGSSaveFormBuffer* apSaveGameBuffer) {
    uint8_t cversion = apSaveGameBuffer->Header.cVersion;
    ENUM_FORM_ID eformType = apSaveGameBuffer->GetFormType();
    uint32_t isize = apSaveGameBuffer->iBufferPosition;
    char* pbuffer = apSaveGameBuffer->pBuffer;
    Initialize(pbuffer, isize, eformType, cversion);
}

// The buffer is the info byte (form type and size type), the version, the data size and the data.
void BGSUnloadedFormBuffer::Initialize(
    char* apBuffer,
    uint32_t aiBufferSize,
    ENUM_FORM_ID aeFormType,
    uint8_t acVersion
) {
    SIZE_TYPE esizeType = GetSizeTypeForSize(aiBufferSize);
    uint32_t isizeTypeSize = GetSizeTypeSize(esizeType);
    pBuffer = static_cast<char*>(ourMalloc(aiBufferSize + isizeTypeSize + 2));
    char* punloadedHeader = pBuffer;
    punloadedHeader[1] = static_cast<char>(acVersion);
    // The buffer's first byte is read as a BGSSaveLoadFormInfo.
    BGSSaveLoadFormInfo* pformInfo = reinterpret_cast<BGSSaveLoadFormInfo*>(punloadedHeader);
    pformInfo->SetSizeType(esizeType);
    pformInfo->SetFormType(aeFormType);
    punloadedHeader += 2;
    SaveGameSizeToBuffer(punloadedHeader, aiBufferSize, esizeType);
    punloadedHeader += isizeTypeSize;
    ourMemcpy(punloadedHeader, apBuffer, aiBufferSize);
}

void BGSUnloadedFormBuffer::DestructKeepBuffer(BGSUnloadedFormBuffer* apThis) {
    apThis->pBuffer = nullptr;
}
