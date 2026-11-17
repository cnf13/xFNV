#include "Fallout Shared/TESFile.h"

#include "BSCore/BSSimpleArray.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/MessageHandler.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/BSSystemFile.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/MenuGameSettings.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystemDesc.h"
#include "SDK/MSVC/stdlib.h"
#include "platform/Paths.h"

#include <cerrno>
#include <cstring>
#include <zlib.h>

void* simpleMalloc(void* opaque, uint32_t aiNumItems, uint32_t aSize) {
    MemoryManager* v3 = &MemoryManager::Instance();
    return v3->Allocate(aSize * aiNumItems);
}

void simpleFree(void* opaque, void* apPtr) {
    MemoryManager* v2 = &MemoryManager::Instance();
    v2->Deallocate(apPtr);
}

// Swaps every field of a record header read from a big-endian file, in field order.
void FORM::Endian() {
    ENDIAN(form, false);
    ENDIAN(length, false);
    ENDIAN(flags, false);
    ENDIAN(iFormID, false);
    ENDIAN(iVersionControl, false);
    ENDIAN(sFormVersion, false);
    ENDIAN(sVCVersion, false);
}

// Byte swaps both fields of a big-endian chunk header.
void CHUNK::Endian() {
    ENDIAN(chunk, false);
    ENDIAN(length, false);
}

void TESFileHeaderInfo::Endian() {
    ENDIAN(fVersion, false);
    ENDIAN(uiFormCount, false);
    ENDIAN(uiNextFormID, false);
}

TESFile* TESFile::GetFileForTempID(uint32_t aiID) {
    TESFile* pownerfile = 0;
    if (TESFile::pOwnedTempIDMap && TESFile::pOwnedTempIDMap->GetAt(aiID, pownerfile)) {
        return pownerfile;
    }
    else {
        return 0;
    }
}

uint32_t TESFile::GetFORMSize() const {
    return sizeof(FORM);
}

const char* TESFile::GetFilename() const {
    return m_Filename;
}

const char* TESFile::GetPath() const {
    return m_Path;
}

TESFileFindData* TESFile::GetFileData() {
    return &m_FileInfo;
}

FORM* TESFile::GetFormData() {
    return &m_currentform;
}

BSSimpleList<char*>* TESFile::GetMasterList() {
    return &listMasters;
}

BSSimpleList<_ULARGE_INTEGER*>* TESFile::GetMasterDataList() {
    return &listMastersData;
}

FORM_GROUP* TESFile::GetCurrentGroup() const {
    return m_grouplist.m_item;
}

uint32_t TESFile::GetChunkSize() const {
    return m_actualChunkSize;
}

bool TESFile::QEndian() const {
    return bMustEndianConvert;
}

int32_t TESFile::GetFormCount() const {
    return fileHeaderInfo.uiFormCount;
}

uint32_t TESFile::GetOffset() const {
    return m_fileoffset;
}

// A failed seek is reported with the stream's error text.
bool TESFile::SetOffset(uint32_t newoffset) {
    FreeDecompressedBuffer();
    m_fileoffset = newoffset;
    if (m_fileoffset < m_filesize) {
        uint32_t uiCurrentOffset = m_fileoffset;
        m_pFile->Seek(static_cast<int32_t>(uiCurrentOffset), NiFile::ms_iSeekSet);
        m_fileoffset = m_pFile->Tell();
        if (m_fileoffset == 0xFFFFFFFF) {
            Warning("MASTERFILE: SetFilePointer() in SetOffset failed with error:\n%s", std::strerror(errno));
            return false;
        }
        else {
            m_formoffset = 0;
            m_chunkoffset = 0;
            ourMemset(&m_currentform, 0, sizeof(FORM));
            GetTESForm();
            return true;
        }
    }
    else {
        ourMemset(&m_currentform, 0, sizeof(FORM));
        return false;
    }
}

// Finds apForm's record in this file, entering the groups it may belong in, and leaves the file on it.
bool TESFile::FindForm(TESForm* apForm) {
    bool bretval = false;
    if (apForm && (GetHasGroups() && (m_pFile && m_pFile->m_bGood))) {
        if (apForm->FindInFileFast(this)) {
            return true;
        }
        TESRewind(1);
        while (m_currentform.form == formEnumString[TES4_ID].uiFormString) {
            NextForm(1);
        }
        uint32_t ifileformtype = formEnumString[apForm->GetSavedFormType()].uiFormString;
        uint32_t ifileformid = apForm->iFormID;
        bool bkeeplooking = true;
        while (m_currentform.form && bkeeplooking) {
            if (m_currentform.form == formEnumString[GRUP_ID].uiFormString) {
                // A group the form may be in is entered (its first record is next), any other is skipped whole.
                if (apForm->BelongsInGroup(&m_currentform, true, false)) {
                    NextForm(1);
                }
                else {
                    NextGroup();
                }
            }
            else if (m_currentform.form == ifileformtype && m_currentform.iFormID == ifileformid) {
                bkeeplooking = false;
                bretval = true;
            }
            else {
                NextForm(1);
            }
        }
    }
    return bretval;
}

TES_RETURN_CODE TESFile::AddTESForm(TESForm* apForm) {
    StartAndEndGroupsForForm(apForm);
    m_pFile->Seek(0, NiFile::ms_iSeekEnd);
    uint32_t iBufferSize = apForm->GetFormBufferSize();
    void* pFormBuffer = apForm->GetFormBuffer();
    TES_RETURN_CODE v6 = EasyWrite(pFormBuffer, iBufferSize);
    if (apForm->GetFormBuffer()) {
        if (apForm->GetFormBufferSize()) {
            ++fileHeaderInfo.uiFormCount;
        }
    }
    apForm->FreeFormBuffer();
    return v6;
}

void TESFile::DeleteFileList(BSSimpleList<TESFile*>* apFileList) {
    while (!apFileList->IsEmpty()) {
        TESFile* pfile = apFileList->m_item;
        if (pfile) {
            delete pfile;
        }
        apFileList->RemoveHead();
    }
}

float TESFile::GetFileVersion() const {
    return fileHeaderInfo.fVersion;
}

uint32_t TESFile::GetMasterCount() const {
    return iMasterCount;
}

uint32_t TESFile::GetIndexCount() const {
    return GetMasterCount();
}

uint32_t TESFile::GetDecompressedBufferSize() const {
    return iDecompressedFormBufferSize;
}

uint8_t TESFile::GetCompileIndex() const {
    return cCompileIndex;
}

void TESFile::SetCompileIndex(uint8_t acIndex) {
    // the compile index is the top byte of every form ID the file creates
    fileHeaderInfo.uiNextFormID = (static_cast<uint32_t>(acIndex) << 24) | (fileHeaderInfo.uiNextFormID & 0xFFFFFF);
    cCompileIndex = acIndex;
}

bool TESFile::GetHasGroups() const {
    return bHasGroups;
}

void TESFile::SetHasGroups(bool abVal) {
    bHasGroups = abVal;
}

uint16_t TESFile::GetFormVersion() const {
    return m_currentform.sFormVersion;
}

void TESFile::ChangeBufferSize(int32_t aiNewSize) {
    m_uiBufferAllocSize = uiDefaultBufferSize;
}

TESFile* TESFile::GetThreadSafeParent() const {
    TESFile* parent = pThreadSafeParent;

    while (parent && parent->pThreadSafeParent) {
        parent = parent->pThreadSafeParent;
    }

    return parent;
}

void TESFile::SetThreadSafeParent(TESFile* apFile) {
    while (apFile->pThreadSafeParent) {
        apFile = apFile->pThreadSafeParent;
    }
    pThreadSafeParent = apFile;
}

TESFile* TESFile::GetIndexFile(uint32_t index) {
    if (index > GetMasterCount()) {
        return nullptr;
    }

    // index 0 is this file, the others are its masters
    if (index == 0) {
        return this;
    }

    if (!m_pMasterPtrs) {
        return nullptr;
    }

    return m_pMasterPtrs[index - 1];
}

bool TESFile::GetMaster() const {
    return (m_Flags & FILE_MASTER) != 0;
}

bool TESFile::GetOptimizedFile() const {
    return (m_Flags & FILE_OPTIMIZED) != 0;
}

bool TESFile::GetChecked() const {
    return (m_Flags & FILE_CHECKED) != 0;
}

bool TESFile::GetActive() const {
    return (m_Flags & FILE_ACTIVE) != 0;
}

bool TESFile::GetTempIDOwner() const {
    return (m_Flags & FILE_TEMP_ID_OWNER) != 0;
}

bool TESFile::GetLittleEndian() const {
    return (m_Flags & FILE_LITTLE_ENDIAN) != 0;
}

void TESFile::SetChecked(bool checked) {
    if (checked) {
        m_Flags |= FILE_CHECKED;
    }
    else {
        // an unchecked file can not be active either
        m_Flags &= ~FILE_CHECKED;
        m_Flags &= ~FILE_ACTIVE;
    }
}

void TESFile::SetLittleEndian(bool abLittle) {
    if (abLittle) {
        m_Flags |= FILE_LITTLE_ENDIAN;
    }
    else {
        m_Flags &= ~FILE_LITTLE_ENDIAN;
    }
}

void TESFile::SetMaster(bool master) {
    if (master) {
        m_Flags |= FILE_MASTER;
    }
    else {
        m_Flags &= ~FILE_MASTER;
    }
}

void TESFile::SetFileLanguage(LANGUAGE language) {
    // The language goes in the top byte of the flags.
    m_Flags &= 0xFFFFFF;
    m_Flags |= language << 24;
}

void TESFile::ClearCurrentChunk() {
    m_currentchunkID = NO_CHUNK;
    m_actualChunkSize = 0;
}

bool TESFile::IsCurrentFormCompressed() const {
    return m_currentform.form != formEnumString[GRUP_ID].uiFormString &&
        (m_currentform.flags & TESForm::kFormFlags_Compressed) != 0;
}

char* TESFile::GetDecompressedBuffer() {
    if (!pDecompressedFormBuffer) {
        DecompressCurrentForm();
    }

    return pDecompressedFormBuffer;
}

void TESFile::FreeDecompressedBuffer() {
    if (pDecompressedFormBuffer) {
        ourFree(pDecompressedFormBuffer);
        pDecompressedFormBuffer = nullptr;
        iDecompressedFormBufferSize = 0;
    }
}

uint32_t TESFile::GetOffsetChunk() const {
    return m_fileoffset + GetFORMSize() + m_formoffset;
}

CHUNK_ID TESFile::GetTESChunk() {
    if (m_currentchunkID == NO_CHUNK && !ReadChunkHeader()) {
        return NO_CHUNK;
    }

    return m_currentchunkID;
}

ENUM_FORM_ID TESFile::GetTESForm() {
    if (m_currentform.form == 0) {
        if (!ReadFormHeader()) {
            return NO_FORM;
        }

        ClearCurrentChunk();
        GetTESChunk();
    }

    return static_cast<ENUM_FORM_ID>(TESForm::GetFormTypeFromFormString(m_currentform.form));
}

void TESFile::GetTESFormNoRet() {
    if (m_currentform.form == 0) {
        if (!ReadFormHeader()) {
            return;
        }

        ClearCurrentChunk();
        ReadChunkHeader();
    }
}

bool TESFile::NextForm(bool abSkipIgnored) {
    FreeDecompressedBuffer();

    bool found = GetNextForm();

    // optionally step over records flagged as ignored (but never over groups)
    while (found && abSkipIgnored && m_currentform.form != formEnumString[GRUP_ID].uiFormString &&
           m_currentform.form != formEnumString[TOFT_ID].uiFormString &&
           (m_currentform.flags & TESForm::kFormFlag_Ignored) != 0) {
        found = GetNextForm();
    }

    return found;
}

bool TESFile::NextGroup() {
    if (m_currentform.form != formEnumString[GRUP_ID].uiFormString) {
        return false;
    }

    // step into the group: its header is consumed
    m_currentform.form = 0;
    m_currentform.length -= GetFORMSize();

    return GetNextForm();
}

// A group's header is stepped into, any other record is skipped whole. The stream only seeks when its position is
// not already the next record's.
bool TESFile::GetNextForm() {
    uint32_t uiFileOffset = m_fileoffset;
    if (m_currentform.form == formEnumString[GRUP_ID].uiFormString ||
        m_currentform.form == formEnumString[TOFT_ID].uiFormString) {
        m_fileoffset += GetFORMSize();
    }
    else {
        m_fileoffset += m_currentform.length + GetFORMSize();
    }
    if (m_fileoffset >= m_filesize) {
        ourMemset(&m_currentform, 0, sizeof(FORM));
        return false;
    }

    uiFileOffset = m_fileoffset;
    if (m_pFile->QTrueFilePos() != uiFileOffset) {
        m_pFile->Seek(uiFileOffset, NiFile::ms_iSeekSet);
        m_fileoffset = m_pFile->QTrueFilePos();
    }
    if (m_fileoffset == 0xFFFFFFFF) {
        Warning("FORMS: SetFilePointer() in NextForm failed with error:\n%s", std::strerror(errno));
        return false;
    }

    m_formoffset = 0;
    m_chunkoffset = 0;
    ourMemset(&m_currentform, 0, sizeof(FORM));
    GetTESFormNoRet();
    if (!m_currentform.form) {
        Warning(
            "MASTERFILE: Trying to load a bad form in TESFile::NextForm.\r\nFile = %s\r\nOffset = %d", m_Filename,
            m_fileoffset
        );
        return false;
    }
    return true;
}

// A form's file index (its top byte) is rewritten to the compile index of the master it names, or of this file.
bool TESFile::ReadFormHeader() {
    if (!m_pFile) {
        return false;
    }

    bool bRead = true;
    if (m_pFile->BSFile::ReadF(&m_currentform, sizeof(FORM)) != sizeof(FORM)) {
        bRead = false;
    }
    if (!bRead) {
        ourMemset(&m_currentform, 0, sizeof(FORM));
        return false;
    }

    if (QEndian()) {
        m_currentform.Endian();
    }
    if (m_currentform.form == formEnumString[GRUP_ID].uiFormString ||
        m_currentform.form == formEnumString[TOFT_ID].uiFormString) {
        return true;
    }

    TESFile* pMaster = nullptr;
    if (m_pMasterPtrs) {
        pMaster = GetIndexFile((m_currentform.iFormID >> 24) + 1);
    }
    if (pMaster) {
        m_currentform.iFormID =
            (static_cast<uint32_t>(pMaster->GetCompileIndex()) << 24) | (m_currentform.iFormID & 0xFFFFFF);
    }
    else {
        m_currentform.iFormID = (static_cast<uint32_t>(cCompileIndex) << 24) | (m_currentform.iFormID & 0xFFFFFF);
    }
    if (TESForm::IsDefaultForm(m_currentform.iFormID & 0xFFFFFF)) {
        m_currentform.iFormID &= 0xFFFFFF;
    }
    return true;
}

// An XXXX chunk carries the real size of the next chunk, whose 16-bit size field cannot hold it.
bool TESFile::ReadChunkHeader() {
    CHUNK kChunk;
    if (!IsCurrentFormCompressed()) {
        if (m_pFile->BSFile::ReadF(&kChunk, sizeof(CHUNK)) != sizeof(CHUNK)) {
            ClearCurrentChunk();
            return false;
        }
    }
    else {
        uint32_t uiFormOffset = m_formoffset;
        char* pBuffer = static_cast<char*>(GetDecompressedBuffer());
        if (!pBuffer || uiFormOffset >= GetDecompressedBufferSize()) {
            ClearCurrentChunk();
            return false;
        }
        pBuffer += uiFormOffset;
        ourMemcpy(&kChunk, pBuffer, sizeof(CHUNK));
    }

    if (QEndian()) {
        kChunk.Endian();
    }
    m_currentchunkID = static_cast<CHUNK_ID>(kChunk.chunk);
    m_actualChunkSize = kChunk.length;
    if (kChunk.chunk == XXXX_ID) {
        uint32_t uiSize = 0;
        GetChunkData(&uiSize, 0);
        if (QEndian()) {
            ENDIAN(uiSize, false);
        }
        NextChunk();
        m_actualChunkSize = uiSize;
    }
    return true;
}

// A compressed record holds its decompressed size, then a zlib stream. The inflate state lives on the stack (this zlib
// build takes a caller-provided state) and allocates through the TESFile.cpp callbacks simpleMalloc and simpleFree. An
// AutoMemContext is constructed and destroyed on a stack context, and the constructor's result is ignored.
void TESFile::DecompressCurrentForm() {
    alignas(AutoMemContext) uint8_t aucContext[sizeof(AutoMemContext)];
    AutoMemContext* pContext = reinterpret_cast<AutoMemContext*>(aucContext);
    ::new (aucContext) AutoMemContext(
        MEM_CONTEXT_TES_FORMS, true, "D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout Shared\\TESFile.cpp", 3982
    );
    if (!m_currentform.length || m_currentform.form == formEnumString[GRUP_ID].uiFormString ||
        !IsCurrentFormCompressed()) {
        pContext->~AutoMemContext();
        return;
    }

    uint8_t eform = TESForm::GetFormTypeFromFormString(m_currentform.form);
    if (!eform) {
        pContext->~AutoMemContext();
        return;
    }
    char* pbuffer = static_cast<char*>(ourMalloc(m_currentform.length + 1));
    if (!pbuffer) {
        pContext->~AutoMemContext();
        return;
    }

    if (m_pFile->BSFile::ReadF(pbuffer, m_currentform.length) != m_currentform.length) {
        Warning("TESFile: Failed to read in buffer data for compressed form.");
    }
    else {
        uint32_t ioriginalsize = *reinterpret_cast<uint32_t*>(pbuffer);
        char* pcompressedbuffer = pbuffer + 4;
        if (QEndian()) {
            ENDIAN(ioriginalsize, false);
        }
        pbuffer[m_currentform.length] = 0;
        pDecompressedFormBuffer = static_cast<char*>(ourMalloc(ioriginalsize));
        iDecompressedFormBufferSize = ioriginalsize;

        z_stream_s zstream;
        zstream.zalloc = &simpleMalloc;
        zstream.zfree = &simpleFree;
        zstream.opaque = nullptr;
        zstream.avail_in = 0;
        zstream.next_in = nullptr;
        zstream.state = nullptr;
        int32_t ret = inflateInit(&zstream);
        if (ret != Z_OK) {
            inflateEnd(&zstream);
            Warning("TESFile: Error initializing ZLib inflate stream.");
            FreeDecompressedBuffer();
            ourFree(pbuffer);
            pContext->~AutoMemContext();
            return;
        }

        zstream.avail_in = m_currentform.length - 4;
        zstream.next_in = reinterpret_cast<uint8_t*>(pcompressedbuffer);
        zstream.avail_out = ioriginalsize;
        zstream.next_out = reinterpret_cast<uint8_t*>(pDecompressedFormBuffer);
        ret = inflate(&zstream, Z_NO_FLUSH);
        if (ret == Z_STREAM_ERROR || ret == Z_NEED_DICT || ret == Z_DATA_ERROR || ret == Z_MEM_ERROR) {
            inflateEnd(&zstream);
            Warning("TESFile: Error inflating ZLib stream.");
            FreeDecompressedBuffer();
            ourFree(pbuffer);
            pContext->~AutoMemContext();
            return;
        }
        if (ret != Z_STREAM_END) {
            inflateEnd(&zstream);
            Warning("TESFile: Error: ZLib stream did not terminate.");
            FreeDecompressedBuffer();
            ourFree(pbuffer);
            pContext->~AutoMemContext();
            return;
        }
        inflateEnd(&zstream);
    }
    ourFree(pbuffer);
    pContext->~AutoMemContext();
}

void TESFile::ClearMasterLists() {
    while (GetMasterList()->GetItem()) {
        ourFree(GetMasterList()->GetItem());
        GetMasterList()->RemoveHead();
    }
    while (GetMasterDataList()->GetItem()) {
        ourFree(GetMasterDataList()->GetItem());
        GetMasterDataList()->RemoveHead();
    }
    iMasterCount = 0;
}

// A file whose header record reads as TES4 has this machine's byte order; one that only reads as TES4 after a swap
// has the other. Conversion is needed when the file's order differs from the machine's.
void TESFile::InitEndian() {
    if (!m_pFile) {
        return;
    }

    m_pFile->Seek(0, NiFile::ms_iSeekSet);
    ourMemset(&m_currentform, 0, sizeof(FORM));
    if (!m_pFile->BSFile::ReadF(&m_currentform, sizeof(FORM))) {
        return;
    }

    if (m_currentform.form == formEnumString[TES4_ID].uiFormString) {
        SetLittleEndian(NiSystemDesc::GetSystemDesc().IsLittleEndian());
    }
    else {
        m_currentform.Endian();
        if (m_currentform.form == formEnumString[TES4_ID].uiFormString) {
            SetLittleEndian(!NiSystemDesc::GetSystemDesc().IsLittleEndian());
        }
    }

    bMustEndianConvert = false;
    // the file's flag is read before the machine's
    bool bFileLittleEndian = GetLittleEndian();
    if (bFileLittleEndian != NiSystemDesc::GetSystemDesc().IsLittleEndian()) {
        bMustEndianConvert = true;
    }
}

// A failed seek is reported with the stream's error text.
bool TESFile::SetOffsetChunk(uint32_t newoffset) {
    if (IsCurrentFormCompressed()) {
        Warning("FORMS: SetOffsetChunk failed for compressed form.");
        return false;
    }

    m_formoffset = newoffset - m_fileoffset - GetFORMSize();
    if (m_formoffset >= m_currentform.length) {
        ClearCurrentChunk();
        ourMemset(&m_currentform, 0, sizeof(FORM));
        return false;
    }

    uint32_t uiCurrentOffset = newoffset;
    m_pFile->Seek(uiCurrentOffset, NiFile::ms_iSeekSet);
    uiCurrentOffset = m_pFile->Tell();
    if (uiCurrentOffset == 0xFFFFFFFF) {
        Warning("FORMS: SetFilePointer() in SetOffsetChunk failed with error:\n%s", std::strerror(errno));
        return false;
    }

    m_chunkoffset = 0;
    ClearCurrentChunk();
    GetTESChunk();
    return true;
}

// Frees the per-thread copies. Each copy goes through the empty ClearAuxBuffer before its deleting destructor.
void TESFile::ClearThreadSafeFiles() {
    if (!pThreadSafeFileMap) {
        return;
    }

    NiTMapItem<uint32_t, TESFile*>* pPos = pThreadSafeFileMap->GetFirstPos();
    while (pPos) {
        uint32_t uiThreadID = 0;
        TESFile* pFile = nullptr;
        pThreadSafeFileMap->GetNext(pPos, uiThreadID, pFile);
        pFile->ClearAuxBuffer();
        if (pFile) {
            delete pFile;
        }
    }
    pThreadSafeFileMap->RemoveAll();
    delete pThreadSafeFileMap;
    pThreadSafeFileMap = nullptr;
}

void TESFile::ClearAuxBuffer() {}

uint32_t TESFile::GetCurrentFormFlags() const {
    return m_currentform.flags;
}

uint32_t TESFile::GetID() const {
    return m_currentform.iFormID;
}

bool TESFile::LoadWord(int16_t* apValue) {
    bool bResult = GetChunkData(apValue, 2);
    if (QEndian()) {
        ENDIAN<int16_t>(*apValue, false);
    }
    return bResult;
}

void TESFile::CloseAllOpenGroups() {
    while (GetCurrentGroup() != nullptr) {
        if (pDataHandler->GetSaveLoad()) {
            EndGroup();
        }
        else {
            CloseGroup();
        }
    }
}

void TESFile::TESRewind(bool getform) {
    if (m_pFile != nullptr) {
        m_pFile->Seek(0, NiFile::ms_iSeekSet);
    }
    m_fileoffset = 0;
    m_formoffset = 0;
    m_chunkoffset = 0;
    ourMemset(&m_currentform, 0, sizeof(FORM));
    if (getform) {
        GetTESForm();
    }
}

void TESFile::LoadForm(TESForm* apForm) {
    apForm->SetFormType(static_cast<uint8_t>(GetTESForm()));
    if (apForm->GetTemporary()) {
        apForm->SetFormFlags(GetCurrentFormFlags() | TESForm::kFormFlag_Temporary);
    }
    else {
        apForm->SetFormFlags(GetCurrentFormFlags());
    }
    apForm->SetFormID(GetID(), true);
    apForm->SetFile(this);
}

bool TESFile::GetTempIDCanLoadFromFile(uint32_t aiID, TESFile* apFile) {
    bool bCanLoad = true;
    if (pOwnedTempIDMap != nullptr && aiID != 0) {
        TESFile* pOwner = nullptr;
        if (pOwnedTempIDMap->GetAt(aiID, pOwner) && pOwner != nullptr) {
            TESFile* pLoadFile = apFile->GetThreadSafeParent();
            if (pLoadFile == nullptr) {
                pLoadFile = apFile;
            }
            if (pOwner != pLoadFile) {
                bCanLoad = false;
            }
        }
    }
    return bCanLoad;
}

// A chunk larger than a nonzero auiMaxSize is truncated to auiMaxSize - 1 bytes plus a terminating zero, with a
// warning that names the chunk and record codes as strings. A partly read chunk is first re-sought to its start.
bool TESFile::GetChunkData(void* apData, uint32_t auiMaxSize) {
    if (!m_actualChunkSize) {
        return true;
    }

    bool bCompressed = IsCurrentFormCompressed();
    if (m_chunkoffset) {
        if (!bCompressed) {
            uint32_t uiOffset = m_fileoffset + GetFORMSize() + m_formoffset + sizeof(CHUNK);
            m_pFile->Seek(uiOffset, NiFile::ms_iSeekSet);
        }
        m_chunkoffset = 0;
    }

    if (auiMaxSize && m_actualChunkSize > auiMaxSize) {
        static_cast<char*>(apData)[auiMaxSize - 1] = 0;
        if (!bCompressed) {
            m_chunkoffset = m_pFile->BSFile::ReadF(apData, auiMaxSize - 1);
            if (m_chunkoffset != auiMaxSize - 1) {
                Warning("MASTERFILE: First ReadFile() in GetChunkData failed with error:\n%s", std::strerror(errno));
                return false;
            }
        }
        else {
            uint32_t uiOffset = m_formoffset + sizeof(CHUNK);
            char* pSource = static_cast<char*>(GetDecompressedBuffer()) + uiOffset;
            ourMemcpy(apData, pSource, auiMaxSize - 1);
            m_chunkoffset = auiMaxSize - 1;
        }

        char cChunkID[5];
        cChunkID[0] = static_cast<char>(m_currentchunkID & 0xFF);
        cChunkID[1] = static_cast<char>((m_currentchunkID & 0xFF00) >> 8);
        cChunkID[2] = static_cast<char>((m_currentchunkID & 0xFF0000) >> 16);
        cChunkID[3] = static_cast<char>((m_currentchunkID & 0xFF000000) >> 24);
        cChunkID[4] = 0;
        char cFormID[5];
        cFormID[0] = static_cast<char>(m_currentform.form & 0xFF);
        cFormID[1] = static_cast<char>((m_currentform.form & 0xFF00) >> 8);
        cFormID[2] = static_cast<char>((m_currentform.form & 0xFF0000) >> 16);
        cFormID[3] = static_cast<char>((m_currentform.form & 0xFF000000) >> 24);
        cFormID[4] = 0;
        Warning(
            "FORMS: Chunk size %d too big in chunk %s_ID in form %s_ID (%08X).\r\n"
            "Max size is %d, data truncated to \"%s\".\r\n",
            m_actualChunkSize, cChunkID, cFormID, m_currentform.iFormID, auiMaxSize, apData
        );
    }
    else if (!bCompressed) {
        m_chunkoffset = m_pFile->BSFile::ReadF(apData, m_actualChunkSize);
        if (m_chunkoffset != m_actualChunkSize) {
            Warning("MASTERFILE: Second ReadFile() in GetChunkData failed with error:\n%s", std::strerror(errno));
            return false;
        }
    }
    else {
        uint32_t uiOffset = m_formoffset + sizeof(CHUNK);
        char* pSource = static_cast<char*>(GetDecompressedBuffer()) + uiOffset;
        ourMemcpy(apData, pSource, m_actualChunkSize);
        m_chunkoffset = m_actualChunkSize;
    }
    return true;
}

void TESFile::EndGroup() {
    FORM_GROUP* pgroup = GetCurrentGroup();
    if (pgroup) {
        if (m_pFile) {
            m_pFile->Seek(0, NiFile::ms_iSeekEnd);
            pgroup->GroupData.length = m_pFile->Tell() - pgroup->iGroupOffset;
            m_pFile->Seek(pgroup->iGroupOffset, NiFile::ms_iSeekSet);
            EasyWrite(pgroup, 24u);
        }
        CloseGroup();
    }
}

void TESFile::CloseGroup() {
    void* groupListHead = GetCurrentGroup();
    if (groupListHead) {
        (&m_grouplist)->RemoveHead();
        ourFree(groupListHead);
    }
}

// A compressed record's chunks are read from the decompressed buffer, so the stream only seeks for plain ones.
bool TESFile::NextChunk() {
    uint32_t uiDataSize = m_currentform.length;
    if (IsCurrentFormCompressed()) {
        uiDataSize = GetDecompressedBufferSize();
    }

    m_formoffset += m_actualChunkSize + sizeof(CHUNK);
    if (m_formoffset >= uiDataSize) {
        ClearCurrentChunk();
        return false;
    }

    uint32_t uiOffset = m_fileoffset + GetFORMSize() + m_formoffset;
    if (!IsCurrentFormCompressed()) {
        m_pFile->Seek(uiOffset, NiFile::ms_iSeekSet);
    }
    m_chunkoffset = 0;
    ClearCurrentChunk();
    GetTESChunk();
    return true;
}

void TESFile::SetTempIDOwnedByFile(uint32_t aiID, TESFile* apFile) {
    if (!TESFile::pOwnedTempIDMap) {
        NiTMap<uint32_t, TESFile*>* v2;
        NiTMap<uint32_t, TESFile*>* apMemory =
            static_cast<NiTMap<uint32_t, TESFile*>*>(ourMalloc(sizeof(NiTMap<uint32_t, TESFile*>)));
        if (apMemory) {
            v2 = ::new (apMemory) NiTMap<uint32_t, TESFile*>(1001u);
        }
        else {
            v2 = 0;
        }
        TESFile::pOwnedTempIDMap = v2;
    }
    TESFile::pOwnedTempIDMap->SetAt(aiID, apFile);
    if (!apFile->GetTempIDOwner()) {
        apFile->SetTempIDOwner(1);
        ++TESFile::iOwnerFileCount;
    }
}

// Opens apFilename in apPath and reads its header record, warning when it is not a TES file or cannot be closed.
TESFile::TESFile(const char* apPath, const char* apFilename, NiFile::OpenMode auiAccessMode)
  : m_lastError(TES_ERROR_NONE), pThreadSafeParent(nullptr), pThreadSafeFileMap(nullptr), m_pLockedFile(nullptr),
    m_pFile(nullptr), m_pBuffer(nullptr), m_uiBufferAllocSize(uiDefaultBufferSize), m_firstCellOffset(0),
    m_currCellOffset(0), m_pCurrCell(nullptr), m_currRefOffset(0), m_currentchunkID(NO_CHUNK), m_actualChunkSize(0),
    m_filesize(0), m_fileoffset(0), m_formoffset(0), m_chunkoffset(0), m_saveformoffset(0), m_savechunkoffset(0),
    m_grouplist(), bMustEndianConvert(false), bCloseFileOverride(false), m_Flags(0), listMasters(), listMastersData(),
    iMasterCount(0), m_pMasterPtrs(nullptr), cCompileIndex(0xFF), cCreatedBy(), cSummary(),
    pDecompressedFormBuffer(nullptr), iDecompressedFormBufferSize(0), bCached(false) {
    ourMemset(&m_currentform, 0, sizeof(m_currentform));
    ourMemset(&m_FileInfo, 0, sizeof(m_FileInfo));
    ourMemset(&DeletedFormTime, 0, sizeof(DeletedFormTime));
    ourMemset(&fileHeaderInfo, 0, sizeof(fileHeaderInfo));
    fileHeaderInfo.uiNextFormID = 2048;
    fileHeaderInfo.fVersion = 1.34f;
    if (OpenTES(apPath, apFilename, auiAccessMode, false)) {
        if (m_FileInfo.nFileSizeHigh || m_FileInfo.nFileSizeLow) {
            if (LoadTESInfo()) {
                Warning("MASTERFILE: File '%s' is not a valid TES file.", apFilename);
            }
        }
        if (!CloseTES()) {
            Warning("MASTERFILE: Could not close file '%s'.", apFilename);
        }
    }
}

// Closes the file and frees the master names, the master data, the buffer and the master table.
TESFile::~TESFile() {
    bCloseFileOverride = true;
    CloseTES();
    for (BSSimpleList<char*>* pNode = GetMasterList(); pNode; pNode = pNode->m_pkNext) {
        if (pNode->m_item) {
            ourFree(pNode->m_item);
        }
    }
    GetMasterList()->RemoveAll();
    for (BSSimpleList<_ULARGE_INTEGER*>* pNode = GetMasterDataList(); pNode; pNode = pNode->m_pkNext) {
        if (pNode->m_item) {
            ourFree(pNode->m_item);
        }
    }
    GetMasterDataList()->RemoveAll();
    ourFree(m_pBuffer);
    ourFree(m_pMasterPtrs);
    m_pMasterPtrs = nullptr;
    RemoveFromOwnedIDMap();
    ClearThreadSafeFiles();
}

void TESFile::operator delete(void* apMemory) {
    ourFree(apMemory);
}

// Reads the file's TES4 header record (HEDR, CNAM, SNAM, MAST, DATA chunks), its flags and language, skips any further
// TES4 records to learn whether the file has groups, and rewinds. Returns TES_ERROR-style codes: 2 when the file cannot
// be opened, 11 when the first record is not TES4, else 0. The CNAM and SNAM buffers are each initialised to "" (one
// zeroed byte, allocated even for an empty chunk) so an empty chunk (malformed data) is not read uninitialised. The
// chunk buffers are allocated on the stack with __builtin_alloca at the chunk size.
TES_RETURN_CODE TESFile::LoadTESInfo() {
    ClearMasterLists();
    if (m_pFile) {
        TESRewind(true);
    }
    else if (!OpenTES(m_Path, m_Filename, NiFile::READ_ONLY, false)) {
        return static_cast<TES_RETURN_CODE>(2);
    }
    m_filesize = m_pFile->GetSize();
    if (m_currentform.form != formEnumString[TES4_ID].uiFormString) {
        return static_cast<TES_RETURN_CODE>(11);
    }
    do {
        switch (m_currentchunkID) {
            case HEDR_ID:
                GetChunkData(&fileHeaderInfo, 12);
                if (QEndian()) {
                    fileHeaderInfo.Endian();
                }
                if (fileHeaderInfo.uiNextFormID <= 0x7FF) {
                    fileHeaderInfo.uiNextFormID = 0x800;
                }
                fileHeaderInfo.uiNextFormID =
                    (static_cast<uint32_t>(cCompileIndex) << 24) | (fileHeaderInfo.uiNextFormID & 0xFFFFFF);
                break;
            case CNAM_ID: {
                uint32_t uiChunkSize = GetChunkSize();
                char* pBuffer = static_cast<char*>(__builtin_alloca(uiChunkSize ? uiChunkSize : 1));
                // Cleared first because an empty chunk leaves the buffer unwritten (see above); the game does not do
                // this.
                pBuffer[0] = '\0';
                GetChunkData(pBuffer, 0x200);
                cCreatedBy.Set(pBuffer, 0);
                break;
            }
            case SNAM_ID: {
                uint32_t uiChunkSize = GetChunkSize();
                char* pBuffer = static_cast<char*>(__builtin_alloca(uiChunkSize ? uiChunkSize : 1));
                // Cleared first because an empty chunk leaves the buffer unwritten (see above); the game does not do
                // this.
                pBuffer[0] = '\0';
                GetChunkData(pBuffer, 0x200);
                cSummary.Set(pBuffer, 0);
                break;
            }
            case MAST_ID: {
                void* pMaster = ourMalloc(GetChunkSize());
                GetChunkData(pMaster, 0);
                GetMasterList()->AddTail(static_cast<char*>(pMaster));
                ++iMasterCount;
                break;
            }
            case DATA_ID: {
                void* pMasterData = ourMalloc(sizeof(double));
                GetChunkData(pMasterData, 8);
                if (QEndian()) {
                    ENDIAN<double>(*static_cast<double*>(pMasterData), false);
                }
                GetMasterDataList()->AddTail(static_cast<_ULARGE_INTEGER*>(pMasterData));
                break;
            }
            default: break;
        }
    }
    while (NextChunk());
    if (m_currentform.flags & 1) {
        m_Flags |= 1;
    }
    else {
        m_Flags &= ~1u;
    }
    if (m_currentform.flags & 0x10) {
        m_Flags |= 0x10;
    }
    else {
        m_Flags &= ~0x10u;
    }
    if (m_currentform.flags & 0x80) {
        m_Flags |= 0x80;
    }
    else {
        m_Flags &= ~0x80u;
    }
    SetFileLanguage(static_cast<LANGUAGE>(m_Flags >> 24));
    while (GetTESForm() == TES4_ID) {
        NextForm(true);
    }
    // Keeps only the low byte of the form type.
    uint8_t cFormType = static_cast<uint8_t>(GetTESForm());
    SetHasGroups(cFormType == GRUP_ID || cFormType == NO_FORM);
    TESRewind(true);
    return static_cast<TES_RETURN_CODE>(0);
}

// A master missing from pFileList leaves a null entry.
bool TESFile::GenIndexTable(BSSimpleList<TESFile*>* pFileList, bool showerror) {
    BSSimpleList<TESFile*>* pIter = pFileList;
    BSSimpleList<char*>* pListMasters = GetMasterList();
    uint32_t uiIndex = GetMasterCount();
    if (m_pMasterPtrs) {
        ourFree(m_pMasterPtrs);
    }
    m_pMasterPtrs = nullptr;
    if (!uiIndex) {
        return true;
    }

    // new TESFile*[count]: a size that overflows asks for all ones
    m_pMasterPtrs = static_cast<TESFile**>(
        ourMalloc(uiIndex > SIZE_MAX / sizeof(TESFile*) ? SIZE_MAX : sizeof(TESFile*) * uiIndex)
    );
    TESFile** ppMasters = m_pMasterPtrs;
    uiIndex = 0;
    bool bResult = true;
    while (pListMasters && pListMasters->GetItem()) {
        pIter = pFileList;
        while (pIter && pIter->GetItem()) {
            TESFile* pFile = pIter->GetItem();
            if (!BSstricmp(static_cast<const char*>(pListMasters->GetItem()), pFile->GetFilename())) {
                ppMasters[uiIndex] = pFile;
                break;
            }
            pIter = pIter->GetNext();
            if (!pIter || !pIter->GetItem()) {
                ppMasters[uiIndex] = nullptr;
                if (showerror) {
                    Warning("MASTERFILE: Missing Masterfile: %s", pListMasters->GetItem());
                }
                bResult = false;
            }
        }
        ++uiIndex;
        pListMasters = pListMasters->GetNext();
    }
    return bResult;
}

// The float version is widened to double for the compare with the constant 1.340000033378601: the version is not too
// high when it is lower, equal or unordered.
bool TESFile::IsFileVersionTooHigh() {
    bool bTooHigh = false;
    if (GetFileVersion() > 1.340000033378601) {
        bTooHigh = true;
        MessageHandler::IncDisableWarningCount(false);
        char acMessage[0x108];
        std::sprintf(acMessage, "File %s is a higher version than this EXE can load.", GetFilename());
        Warning(acMessage);
        MessageHandler::IncDisableWarningCount(true);
    }
    return bTooHigh;
}

// Tests only the first node's item; index counts from 1.
const char* TESFile::GetIndexName(uint32_t index) {
    BSSimpleList<char*>* pIter = GetMasterList();
    if (!pIter->GetItem()) {
        return nullptr;
    }
    for (uint32_t i = 1; i < index; ++i) {
        pIter = pIter->GetNext();
        if (!pIter) {
            return nullptr;
        }
    }
    return pIter->GetItem();
}

// A master is unchanged while the file the directory search finds has the size kept for it; a master that is not
// found has changed when a size was kept.
bool TESFile::MastersChanged() {
    BSSimpleList<char*>* pMNList = GetMasterList();
    BSSimpleList<_ULARGE_INTEGER*>* pMDList = GetMasterDataList();
    bool filechanged = 0;
    while (pMNList && (pMNList->m_item && (pMDList && pMDList->m_item))) {
        char FileName[264];
        BSsprintf(FileName, 260, "%s", pMNList->m_item);
        _ULARGE_INTEGER* plargeint = pMDList->m_item;
        const std::vector<platform::FileEntry> kFound = platform::FindFiles(FileName);
        if (kFound.empty()) {
            if (plargeint->u.LowPart || plargeint->u.HighPart) {
                filechanged = 1;
            }
        }
        else {
            const uint64_t uiSize = kFound.front().size;
            if (static_cast<uint32_t>(uiSize) != plargeint->u.LowPart ||
                static_cast<uint32_t>(uiSize >> 32) != plargeint->u.HighPart) {
                filechanged = 1;
            }
        }
        pMNList = pMNList->m_pkNext;
        pMDList = pMDList->m_pkNext;
    }
    return filechanged;
}

bool TESFile::OpenTES(NiFile::OpenMode auiAccessMode, bool abLock) {
    return OpenTES(m_Path, m_Filename, auiAccessMode, abLock);
}

// A locked file is opened for writing. When the plugin is missing the error tells whether only its temp file
// (<name>.tes) exists. A file whose first chunk is not HEDR and whose version is not 0.85 (NaN included) is reopened
// after the version is set to 0.85.
bool TESFile::OpenTES(const char* ppath, const char* pfilename, NiFile::OpenMode accessmode, bool lock) {
    if (!ppath || !pfilename) {
        return false;
    }
    if (lock) {
        accessmode = NiFile::WRITE_ONLY;
    }

    char FileName[260];
    BSstrcpy(FileName, 260, ppath);
    BSstrcpy(m_Path, 260, ppath);
    BSstrcat(FileName, 260, pfilename);
    BSstrcpy(m_Filename, 260, pfilename);
    if (!m_pFile) {
        // The directory search's entry fills the kept find data the way FindFirstFileA did: every field it gives
        // (attributes, the three times, size, name, short name); the reserved words and the bytes after each name's
        // NUL are zero.
        const std::vector<platform::FileEntry> kFound = platform::FindFiles(FileName);
        if (kFound.empty()) {
            std::strcpy(FileName, m_Path);
            char* pExtension = std::strchr(m_Filename, '.');
            if (pExtension) {
                *pExtension = '\0';
                std::strcat(FileName, m_Filename);
                std::strcat(FileName, ".tes");
                *pExtension = '.';
            }
            else {
                std::strcat(FileName, m_Filename);
                std::strcat(FileName, ".tes");
            }
            if (platform::FindFiles(FileName).empty()) {
                m_lastError = TES_ERROR_NO_FILE;
            }
            else {
                m_lastError = TES_ERROR_FILE_IN_USE;
            }
            return false;
        }
        const platform::FileEntry& rEntry = kFound.front();
        TESFileFindData FindFileData = {};
        FindFileData.dwFileAttributes = rEntry.attributes;
        FindFileData.ftCreationTime.dwLowDateTime = static_cast<uint32_t>(rEntry.creationTime);
        FindFileData.ftCreationTime.dwHighDateTime = static_cast<uint32_t>(rEntry.creationTime >> 32);
        FindFileData.ftLastAccessTime.dwLowDateTime = static_cast<uint32_t>(rEntry.accessTime);
        FindFileData.ftLastAccessTime.dwHighDateTime = static_cast<uint32_t>(rEntry.accessTime >> 32);
        FindFileData.ftLastWriteTime.dwLowDateTime = static_cast<uint32_t>(rEntry.writeTime);
        FindFileData.ftLastWriteTime.dwHighDateTime = static_cast<uint32_t>(rEntry.writeTime >> 32);
        FindFileData.nFileSizeHigh = static_cast<uint32_t>(rEntry.size >> 32);
        FindFileData.nFileSizeLow = static_cast<uint32_t>(rEntry.size);
        BSstrcpy(FindFileData.cFileName, sizeof(FindFileData.cFileName), rEntry.name.c_str());
        BSstrcpy(FindFileData.cAlternateFileName, sizeof(FindFileData.cAlternateFileName), rEntry.shortName.c_str());
        if (std::memcmp(&m_FileInfo, &FindFileData, sizeof(TESFileFindData))) {
            // bit 1 of m_Flags: the file changed since its find data was kept
            m_Flags |= 0x2;
            ourMemcpy(&m_FileInfo, &FindFileData, sizeof(TESFileFindData));
        }
        void* pMemory = ourMalloc(sizeof(BSFile));
        m_pFile = pMemory ? ::new (pMemory) BSFile(FileName, accessmode, m_uiBufferAllocSize, false) : nullptr;
        if (!m_pFile) {
            return false;
        }
        m_pFile->Open(1, false);
        if (!m_pFile->FileIsGood()) {
            // ENOENT and EACCES
            int32_t iError = errno;
            if (iError == 2) {
                m_lastError = TES_ERROR_NO_FILE;
            }
            else if (iError == 13) {
                m_lastError = TES_ERROR_FILE_OPEN;
            }
            return false;
        }
    }

    if (m_pFile->FileIsGood() && lock) {
        TakeOwnership();
    }
    m_filesize = m_pFile->GetSize();
    if (accessmode == NiFile::READ_ONLY) {
        InitEndian();
    }
    TESRewind(accessmode == NiFile::READ_ONLY);
    if (m_currentchunkID != HEDR_ID && GetFileVersion() != 0.8500000238418579) {
        SetFileVersion(0.85f);
        return OpenTES(ppath, pfilename, accessmode, false);
    }
    return true;
}

void TESFile::SetFileVersion(float afVal) {
    fileHeaderInfo.fVersion = afVal;
}

bool TESFile::TakeOwnership() {
    return false;
}

TESFile* TESFile::GetThreadSafeFile() {
    TESFile* pFile = this;
    while (pFile->GetThreadSafeParent()) {
        pFile = pFile->GetThreadSafeParent();
    }
    if (pFile) {
        uint32_t uiThreadID = BSGetCurrentThreadID();
        if (uiThreadID == appMain->GetThread()) {
            return pFile;
        }
    }
    return pFile->GetThreadSafeFileForThread(BSGetCurrentThreadID());
}

TESFile* TESFile::GetThreadSafeFileForThread(uint32_t aiThreadID) {
    TESFile* pretfile = nullptr;
    if (pThreadSafeFileMap) {
        pThreadSafeFileMap->GetAt(aiThreadID, pretfile);
    }
    if (!pretfile) {
        TESFile* pMemory = static_cast<TESFile*>(ourMalloc(sizeof(TESFile)));
        if (pMemory) {
            pretfile = ::new (pMemory) TESFile(GetPath(), m_Filename, NiFile::READ_ONLY);
        }
        else {
            pretfile = nullptr;
        }
        bool bMaster = GetMaster();
        pretfile->SetMaster(bMaster);
        uint8_t ucCompileIndex = GetCompileIndex();
        pretfile->SetCompileIndex(ucCompileIndex);
        BSSimpleList<TESFile*>* pFileList = pDataHandler->GetFileList();
        pretfile->GenIndexTable(pFileList, 0);
        pretfile->SetThreadSafeParent(this);
        pretfile->OpenTES(NiFile::READ_ONLY, 0);
        if (!pThreadSafeFileMap) {
            NiTPointerMap<uint32_t, TESFile*>* pMap;
            NiTPointerMap<uint32_t, TESFile*>* pMapMemory =
                static_cast<NiTPointerMap<uint32_t, TESFile*>*>(ourMalloc(sizeof(NiTPointerMap<uint32_t, TESFile*>)));
            if (pMapMemory) {
                pMap = ::new (pMapMemory) NiTPointerMap<uint32_t, TESFile*>(37u);
            }
            else {
                pMap = nullptr;
            }
            pThreadSafeFileMap = pMap;
        }
        pThreadSafeFileMap->SetAt(aiThreadID, pretfile);
        pretfile->ClearAuxBuffer();
    }
    return pretfile;
}

void TESFile::SetDescription(const char* apDescription) {
    (&cSummary)->operator=(apDescription);
}

void TESFile::SetMaybeShouldBeReloaded(bool abReload) {
    bCached = (bool)((uint8_t)abReload);
}

bool TESFile::GetMaybeShouldBeReloaded() {
    return (bool)((uint8_t)bCached);
}

bool TESFile::AddAuxBuffer(uint32_t aiMinIndex, uint32_t aiMaxIndex) {
    return true;
}

// Outside of clearing data a file stays open unless bCloseFileOverride is set. A file saved through a locked temp
// file then replaces the plugin with the temp file (<name>.tes).
bool TESFile::CloseTES() {
    if (!bCloseFileOverride && pDataHandler && !pDataHandler->GetClearingData()) {
        return true;
    }

    CloseAllOpenGroups();
    if (m_pFile) {
        delete m_pFile;
        m_pFile = nullptr;
    }
    FreeDecompressedBuffer();
    if (!m_pLockedFile) {
        return true;
    }

    char NewFilename[260];
    char OldFilename[260];
    BSstrcpy(NewFilename, 260, m_Path);
    BSstrcat(NewFilename, 260, m_Filename);
    BSstrcpy(OldFilename, 260, m_Path);
    char* pExtension = std::strchr(m_Filename, '.');
    if (pExtension) {
        *pExtension = '\0';
        BSstrcat(OldFilename, 260, m_Filename);
        BSstrcat(OldFilename, 260, ".tes");
        *pExtension = '.';
    }
    else {
        BSstrcat(OldFilename, 260, m_Filename);
        BSstrcat(OldFilename, 260, ".tes");
    }
    delete m_pLockedFile;
    m_pLockedFile = nullptr;

    if (BSSystemFile::DeleteFileA(NewFilename)) {
        TESDataHandler::RegisterError();
        m_lastError = TES_ERROR_FILE_OPEN;
        Warning(
            "MASTERFILE: Unable to complete operation due to failure removing previous file.\r\n"
            "Plugin '%s' may be locked or read only.\r\n\r\nTemp file '%s' contains saved data.",
            NewFilename, OldFilename
        );
        return false;
    }
    if (rt::rename(OldFilename, NewFilename)) {
        TESDataHandler::RegisterError();
        m_lastError = TES_ERROR_FILE_OPEN;
        Warning(
            "MASTERFILE: Unable to complete operation due to failure renaming temp file.\r\n"
            "Temp file '%s' contains saved data.",
            OldFilename
        );
        return false;
    }
    return true;
}

void TESFile::SetActive(bool active) {
    uint32_t v2;
    if (active) {
        v2 = m_Flags | 8;
    }
    else {
        v2 = m_Flags & 0xFFFFFFF7u;
    }
    m_Flags = v2;
}

void TESFile::SetTempIDOwner(bool abVal) {
    int32_t v2;
    if (abVal) {
        v2 = ((int32_t)m_Flags) | 32;
    }
    else {
        v2 = (int32_t)(((int32_t)m_Flags) & (~0x20u));
    }
    m_Flags = (uint32_t)v2;
}

uint16_t TESFile::GetFileFormVersion() {
    return (uint16_t)15;
}

BSSimpleList<FORM_GROUP*>* TESFile::GetGroupList() {
    return &m_grouplist;
}

TES_RETURN_CODE TESFile::EasyWrite(void* ptr, uint32_t count) {
    if (ptr && (uint32_t)m_pFile->WriteHere(ptr, count) < count) {
        // Reads errno into a local that is never used, as the game does.
        int32_t iError = errno;
        (void)iError;
        m_lastError = TES_ERROR_WRITE_FAILURE;
        Warning("MASTERFILE: Write Error in TES file");
        return m_lastError;
    }
    m_lastError = TES_ERROR_NONE;
    return m_lastError;
}

void TESFile::TESRewindChunk() {
    m_formoffset = 0;
    m_chunkoffset = 0;
    int32_t v2 = m_formoffset + (GetFORMSize() + m_fileoffset);
    if (!IsCurrentFormCompressed()) {
        m_pFile->Seek(v2, NiFile::ms_iSeekSet);
        m_pFile->Tell();
    }
    ClearCurrentChunk();
    GetTESChunk();
}

TES_RETURN_CODE TESFile::StartForm(TESForm* apForm) {
    StartAndEndGroupsForForm(apForm);
    m_saveform.flags = apForm->iFormFlags;
    // The flags a record header keeps when it is saved.
    m_saveform.flags &= 0x30032FE0u;
    m_saveform.form = formEnumString[apForm->GetFormType()].uiFormString;
    m_saveform.iFormID = apForm->iFormID;
    m_saveform.sFormVersion = TESFile::GetFileFormVersion();
    m_saveform.sVCVersion = 0;
    m_saveform.iVersionControl = 0;
    m_saveform.length = 0;
    m_pFile->Seek(0, NiFile::ms_iSeekEnd);
    m_saveformoffset = m_pFile->Tell();
    m_savechunkoffset = 0;
    return EasyWrite(&m_saveform, 24u);
}

uint32_t TESFile::CloseForm() {
    ++fileHeaderInfo.uiFormCount;
    m_saveform.length = m_savechunkoffset;
    m_pFile->Seek(m_saveformoffset, NiFile::ms_iSeekSet);
    return EasyWrite(&m_saveform, 24u);
}

void TESFile::StartGroup(FORM* apNewGroupData) {
    if (apNewGroupData) {
        OpenGroup(apNewGroupData);
        if (m_pFile) {
            FORM_GROUP* CurrentGroup = GetCurrentGroup();
            m_pFile->Seek(0, NiFile::ms_iSeekEnd);
            CurrentGroup->iGroupOffset = m_pFile->Tell();
            EasyWrite(CurrentGroup, 24u);
            ++fileHeaderInfo.uiFormCount;
        }
    }
}

void TESFile::OpenGroup(FORM* apGroupData) {
    if (apGroupData) {
        FORM_GROUP* a2 = static_cast<FORM_GROUP*>(ourMalloc(sizeof(FORM_GROUP)));
        m_grouplist.AddHead(a2);
        ourMemcpy(a2, apGroupData, 24u);
    }
}

void TESFile::StartAndEndGroupsForForm(TESForm* apForm) {
    FORM groupform;
    if (apForm && apForm->GetFormType() != TES4_ID) {
        FORM_GROUP* pgroup = 0;
        FORM_GROUP* pstopgroup = 0;
        for (BSSimpleList<FORM_GROUP*>* pgrouplist = GetGroupList(); pgrouplist; pgrouplist = pgrouplist->m_pkNext) {
            pgroup = pgrouplist->m_item;
            if (pgroup) {
                if (!apForm->BelongsInGroup(&pgroup->GroupData, 1, 1)) {
                    pstopgroup = pgroup;
                }
            }
        }
        if (pstopgroup) {
        LABEL_10:;
            for (pgroup = GetCurrentGroup(); pgroup; pgroup = 0) {
                EndGroup();
                if (pgroup != pstopgroup) {
                    goto LABEL_10;
                }
            }
        }
        for (pgroup = GetCurrentGroup(); !pgroup || !apForm->BelongsInGroup(&pgroup->GroupData, 0, 1);
             pgroup = GetCurrentGroup()) {
            apForm->CreateGroupData(&groupform, pgroup);
            if (groupform.form != formEnumString[2].uiFormString) {
                int32_t CastingType = apForm->iFormID;
                const char* v2 = apForm->GetFormEditorID();
                Warning(
                    "FORMS: Failed to CreateGroupData for %s form '%s' (%08X)", apForm->GetFormTypeString(), v2,
                    CastingType
                );
                return;
            }
            StartGroup(&groupform);
        }
    }
}

void TESFile::RemoveFromOwnedIDMap() {
    if (GetTempIDOwner()) {
        SetTempIDOwner(0);
        if (--TESFile::iOwnerFileCount) {
            NiTPrimitiveArray<uint32_t> ownedIDs(100, 100);
            uint32_t iindex = 0;
            NiTMapItem<uint32_t, TESFile*>* iter = TESFile::pOwnedTempIDMap->GetFirstPos();
            while (iter) {
                TESFile* pfile = nullptr;
                uint32_t iid = 0;
                TESFile::pOwnedTempIDMap->GetNext(iter, iid, pfile);
                if (pfile == this) {
                    ownedIDs.SetAtGrow(iindex++, iid);
                }
            }
            for (uint32_t i = 0; i < iindex; ++i) {
                uint32_t uiID = ownedIDs.operator[](i);
                TESFile::pOwnedTempIDMap->RemoveAt(uiID);
            }
        }
        else {
            TESFile::pOwnedTempIDMap->RemoveAll();
            if (TESFile::pOwnedTempIDMap) {
                delete TESFile::pOwnedTempIDMap;
            }
            TESFile::pOwnedTempIDMap = nullptr;
        }
    }
}

BSStringT<char>* TESFile::GetSummary() {
    return &cSummary;
}

// Uses the auxiliary file position unless it is unset, then the true file position.
uint32_t BSFile::Tell() {
    if (iAuxTrueFilePos == -1) {
        return iTrueFilePos;
    }
    return iAuxTrueFilePos;
}

// Returns the file's open state from BSFile::m_bGood.
bool BSFile::FileIsGood() const {
    return m_bGood;
}

bool NiSystemDesc::IsLittleEndian() const {
    return true;
}
