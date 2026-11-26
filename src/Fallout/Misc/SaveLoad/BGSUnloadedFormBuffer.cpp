#include "Fallout/Misc/SaveLoad/BGSUnloadedFormBuffer.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
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

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>

BGSUnloadedFormBuffer::BGSUnloadedFormBuffer() : pBuffer(nullptr) {}

uint8_t BGSUnloadedFormBuffer::GetVersion() {
    return static_cast<uint8_t>(pBuffer[1]);
}

SIZE_TYPE BGSUnloadedFormBuffer::GetSizeType() {
    return reinterpret_cast<BGSSaveLoadFormInfo*>(pBuffer)->GetSizeType();
}

ENUM_FORM_ID BGSUnloadedFormBuffer::GetFormType() {
    return reinterpret_cast<BGSSaveLoadFormInfo*>(pBuffer)->GetFormType();
}

char* BGSUnloadedFormBuffer::GetAdvancedBuffer() {
    if (!pBuffer) {
        return nullptr;
    }
    // The buffer's first byte is read as a BGSSaveLoadFormInfo.
    SIZE_TYPE eSizeType = reinterpret_cast<BGSSaveLoadFormInfo*>(pBuffer)->GetSizeType();
    return &pBuffer[GetSizeTypeSize(eSizeType) + 2];
}

// The header is built on the stack: the form ID, the change flags, then the buffer's info byte and version.
void BGSUnloadedFormBuffer::Save(BGSSaveLoadFile* apFile, uint32_t aiFormID, BGSChangeFlags aiChangeFlags) {
    char* pHeader = pBuffer;
    char* pbuffer = pBuffer + 2;
    BGSSaveLoadFormHeader header;
    header.FormIDIndex.SetNumericID(aiFormID);
    header.iChangeFlags = aiChangeFlags;
    header.FormInfo.cData = static_cast<uint8_t>(pHeader[0]);
    header.cVersion = static_cast<uint8_t>(pHeader[1]);
    apFile->Write(&header, 9u);
    SIZE_TYPE eSizeType = header.FormInfo.GetSizeType();
    uint32_t uiSize = 0;
    pbuffer += LoadGameSizeFromBuffer(pbuffer, uiSize, eSizeType);
    SaveGameSizeToFile(apFile, uiSize, eSizeType);
    if (uiSize) {
        apFile->Write(pbuffer, uiSize);
    }
}

void BGSUnloadedFormBuffer::Load(BGSSaveLoadFile* apFile) {
    BGSSaveLoadFormHeader header;
    apFile->Read(&header, 9u);
    SIZE_TYPE eSizeType = header.FormInfo.GetSizeType();
    uint32_t uiDataSize = 0;
    uint32_t uiSizeTypeSize = LoadGameSizeFromFile(apFile, uiDataSize, eSizeType);
    uint32_t uiSize = uiDataSize + uiSizeTypeSize + 2;
    pBuffer = static_cast<char*>(ourMalloc(uiSize));
    char* pUnloadedHeader = pBuffer;
    pUnloadedHeader[1] = header.cVersion;
    pUnloadedHeader[0] = header.FormInfo.cData;
    pUnloadedHeader += 2;
    SaveGameSizeToBuffer(pUnloadedHeader, uiDataSize, eSizeType);
    pUnloadedHeader += uiSizeTypeSize;
    if (uiDataSize) {
        apFile->Read(pUnloadedHeader, uiDataSize);
    }
}

uint32_t BGSUnloadedFormBuffer::GetSize() {
    if (!pBuffer) {
        return 0;
    }
    SIZE_TYPE eSizeType = GetSizeType();
    uint32_t uiSize = 0;
    LoadGameSizeFromBuffer(pBuffer + 2, uiSize, eSizeType);
    return uiSize;
}

uint32_t BGSUnloadedFormBuffer::GetOverhead() {
    SIZE_TYPE eSizeType = GetSizeType();
    return GetSizeTypeSize(eSizeType) + 9;
}

BGSLoadFormBuffer* BGSUnloadedFormBuffer::FillLoadFormBuffer() {
    BGSLoadFormBuffer* pLoadBuffer;
    pLoadBuffer = new BGSLoadFormBuffer;
    uint8_t cVersion = GetVersion();
    pLoadBuffer->Header.cVersion = cVersion;
    SIZE_TYPE eSizeType = GetSizeType();
    int32_t iOffset = 2;
    uint32_t uiSize = 0;
    uint32_t uiSizeTypeSize = LoadGameSizeFromBuffer(pBuffer + 2, uiSize, eSizeType);
    iOffset += uiSizeTypeSize;
    pLoadBuffer->SetBuffer(pBuffer, iOffset + uiSize, iOffset);
    return pLoadBuffer;
}

void BGSUnloadedFormBuffer::Initialize(BGSSaveFormBuffer* apSaveGameBuffer) {
    uint8_t cVersion = apSaveGameBuffer->Header.cVersion;
    ENUM_FORM_ID eFormType = apSaveGameBuffer->GetFormType();
    uint32_t uiSize = apSaveGameBuffer->iBufferPosition;
    char* pbuffer = apSaveGameBuffer->pBuffer;
    Initialize(pbuffer, uiSize, eFormType, cVersion);
}

// The buffer is the info byte (form type and size type), the version, the data size and the data.
void BGSUnloadedFormBuffer::Initialize(
    char* apBuffer,
    uint32_t aiBufferSize,
    ENUM_FORM_ID aeFormType,
    uint8_t acVersion
) {
    SIZE_TYPE eSizeType = GetSizeTypeForSize(aiBufferSize);
    uint32_t uiSizeTypeSize = GetSizeTypeSize(eSizeType);
    pBuffer = static_cast<char*>(ourMalloc(aiBufferSize + uiSizeTypeSize + 2));
    char* pUnloadedHeader = pBuffer;
    pUnloadedHeader[1] = static_cast<char>(acVersion);
    // The buffer's first byte is read as a BGSSaveLoadFormInfo.
    BGSSaveLoadFormInfo* pFormInfo = reinterpret_cast<BGSSaveLoadFormInfo*>(pUnloadedHeader);
    pFormInfo->SetSizeType(eSizeType);
    pFormInfo->SetFormType(aeFormType);
    pUnloadedHeader += 2;
    SaveGameSizeToBuffer(pUnloadedHeader, aiBufferSize, eSizeType);
    pUnloadedHeader += uiSizeTypeSize;
    ourMemcpy(pUnloadedHeader, apBuffer, aiBufferSize);
}

void BGSUnloadedFormBuffer::DestructKeepBuffer(BGSUnloadedFormBuffer* apThis) {
    apThis->pBuffer = nullptr;
}
