#include "BSSystem/Archive.h"

#include "BSAudio/BSAudioManager.h"
#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSRandom.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSSystem/BSArchive.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/BSFileCache.h"
#include "BSSystem/BSHash.h"
#include "BSSystem/FileFinder.h"
#include "BSSystem/Win32/BSWin32FileCache.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/ehvec.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/stdlib.h"
#include "SDK/MSVC/string.h"
#include "SDK/MSVC/sys/stat.h"

#include <zlib.h>

BSCriticalSection ArchiveManager::s_ArchiveListLock;

NiPointer<Archive> ArchiveManager::spLastArchive;

ArchiveManager::MasterArchiveArray ArchiveManager::spMasterArchives;

// BSArchive() clears the directory table after BSArchiveHeader(); BSFile(apArchiveName, READ_ONLY, aiBufferSize,
// false), NiRefObject() and BSCriticalSection() are assumed native.
Archive::Archive(
    const char* apArchiveName,
    uint32_t aiBufferSize,
    bool abSecondaryArchive,
    bool abInvalidateOtherArchives
)
  : BSArchive(), BSFile(apArchiveName, NiFile::READ_ONLY, aiBufferSize, false), NiRefObject(), iFileNameArrayOffset(0),
    iLastDirectoryIndex(0xFFFFFFFF), iLastFileIndex(0xFFFFFFFF), cArchiveFlags(0), pDirectoryStringArray(nullptr),
    pDirectoryStringOffsets(nullptr), pFileNameStringArray(nullptr), pFileNameStringOffsets(nullptr),
    m_iRuntimeID(uiCount++) {
    BSFile::Open(false, false);
    if (!m_bGood) {
        return;
    }
    if (abSecondaryArchive) {
        cArchiveFlags |= ARCHIVE_FLAG_SECONDARY;
    }
    BSFile::SetEndianSwap(false);
    // The 36-byte header, read as one one-byte component.
    uint32_t uiComponentSize = 1;
    m_uiAbsoluteCurrentPos += m_pfnRead(this, static_cast<BSArchiveHeader*>(this), 36, &uiComponentSize, 1);
    if (iTag != 0x415342 || iVersion > 104) {
        cArchiveFlags |= ARCHIVE_FLAG_DISABLED;
        return;
    }
    if (iVersion < 104) {
        oFlags.iFlag &= ~static_cast<uint32_t>(BSArchive::ARCHIVE_EMBEDDED_FILE_NAMES);
    }
    if ((cArchiveFlags & ARCHIVE_FLAG_SECONDARY) == 0) {
        // The directory records, cleared before the read fills them.
        MemoryManager_Singleton.Deallocate(pDirectories);
        pDirectories =
            static_cast<BSDirectoryEntry*>(MemoryManager_Singleton.Allocate(iDirectories * sizeof(BSDirectoryEntry)));
        for (uint32_t i = 0; i < iDirectories; ++i) {
            pDirectories[i].iFiles = 0;
            pDirectories[i].pFiles = nullptr;
        }
        uiComponentSize = 1;
#if FNVD_64BIT
        // The file's records are 16 bytes (the hash, the file count and a 32-bit pointer slot); the 64-bit record's
        // pointer takes 8. They are read at the start of the array in the file's layout and spread over the records,
        // last first; the file's pointer slot is dropped, as the read below fills pFiles.
        constexpr uint32_t kFileRecordSize = 16;
        m_uiAbsoluteCurrentPos += m_pfnRead(this, pDirectories, iDirectories * kFileRecordSize, &uiComponentSize, 1);
        for (uint32_t i = iDirectories; i-- > 0;) {
            BSDirectoryEntry kEntry;
            const char* pRecord = reinterpret_cast<const char*>(pDirectories) + i * kFileRecordSize;
            __builtin_memcpy(static_cast<BSHash*>(&kEntry), pRecord, sizeof(BSHash));
            __builtin_memcpy(&kEntry.iFiles, pRecord + offsetof(BSDirectoryEntry, iFiles), sizeof(kEntry.iFiles));
            kEntry.pFiles = nullptr;
            pDirectories[i] = kEntry;
        }
#else
        m_uiAbsoluteCurrentPos +=
            m_pfnRead(this, pDirectories, iDirectories * sizeof(BSDirectoryEntry), &uiComponentSize, 1);
#endif
        if ((oFlags.iFlag & BSArchive::ARCHIVE_DIRSTRINGS) != 0 && QLoadStrings()) {
            cArchiveFlags |= ARCHIVE_FLAG_HAS_DIRECTORY_STRINGS;
            MemoryManager_Singleton.Deallocate(pDirectoryStringArray);
            MemoryManager_Singleton.Deallocate(pDirectoryStringOffsets);
            pDirectoryStringArray = static_cast<char*>(MemoryManager_Singleton.Allocate(iDirectoryNamesLength));
            pDirectoryStringOffsets =
                static_cast<uint32_t*>(MemoryManager_Singleton.Allocate(iDirectories * sizeof(uint32_t)));
        }
        if ((oFlags.iFlag & BSArchive::ARCHIVE_FILESTRINGS) != 0 && QLoadStrings()) {
            cArchiveFlags |= ARCHIVE_FLAG_HAS_FILE_STRINGS;
            MemoryManager_Singleton.Deallocate(pFileNameStringArray);
            if (pFileNameStringOffsets) {
                for (uint32_t j = 0; j < iDirectories; ++j) {
                    MemoryManager_Singleton.Deallocate(pFileNameStringOffsets[j]);
                }
                MemoryManager_Singleton.Deallocate(pFileNameStringOffsets);
            }
            pFileNameStringArray = static_cast<char*>(MemoryManager_Singleton.Allocate(iFileNamesLength));
            pFileNameStringOffsets =
                static_cast<uint32_t**>(MemoryManager_Singleton.Allocate(iDirectories * sizeof(uint32_t*)));
        }
        // Each directory: its counted name when the archive has directory names, then its file records.
        uint32_t uiNamePos = 0;
        for (uint32_t k = 0; k < iDirectories; ++k) {
            if ((oFlags.iFlag & BSArchive::ARCHIVE_DIRSTRINGS) != 0) {
                uint8_t cLength;
                uiComponentSize = 1;
                m_uiAbsoluteCurrentPos += m_pfnRead(this, &cLength, 1, &uiComponentSize, 1);
                if (QLoadStrings()) {
                    uiComponentSize = 1;
                    m_uiAbsoluteCurrentPos +=
                        m_pfnRead(this, &pDirectoryStringArray[uiNamePos], cLength, &uiComponentSize, 1);
                    pDirectoryStringOffsets[k] = uiNamePos;
                    uiNamePos += cLength;
                }
                else {
                    // The name is read and dropped.
                    char acName[260];
                    uiComponentSize = 1;
                    m_uiAbsoluteCurrentPos += m_pfnRead(this, acName, cLength, &uiComponentSize, 1);
                }
            }
            uint32_t uiFiles = pDirectories[k].iFiles;
            BSFileEntry* pFiles =
                static_cast<BSFileEntry*>(MemoryManager_Singleton.Allocate(uiFiles * sizeof(BSFileEntry)));
            for (uint32_t n = 0; n < uiFiles; ++n) {
                pFiles[n].iSize = 0;
                pFiles[n].iOffset = 0;
            }
            uiComponentSize = 1;
            m_uiAbsoluteCurrentPos += m_pfnRead(this, pFiles, uiFiles * sizeof(BSFileEntry), &uiComponentSize, 1);
            pDirectories[k].pFiles = pFiles;
        }
        // The file names: one block, with each file's offset into it.
        if ((oFlags.iFlag & BSArchive::ARCHIVE_FILESTRINGS) != 0) {
            iFileNameArrayOffset = BSFile::Tell();
            if (QLoadStrings()) {
                uiComponentSize = 1;
                m_uiAbsoluteCurrentPos += m_pfnRead(this, pFileNameStringArray, iFileNamesLength, &uiComponentSize, 1);
                uint32_t uiArrayPos = 0;
                for (uint32_t m = 0; m < iDirectories; ++m) {
                    pFileNameStringOffsets[m] = static_cast<uint32_t*>(
                        MemoryManager_Singleton.Allocate(pDirectories[m].iFiles * sizeof(uint32_t))
                    );
                    for (uint32_t n = 0; n < pDirectories[m].iFiles; ++n) {
                        pFileNameStringOffsets[m][n] = uiArrayPos;
                        uiArrayPos += std::strlen(&pFileNameStringArray[uiArrayPos]) + 1;
                    }
                }
            }
        }
    }
    struct rt::_stat64i32 Stat;
    rt::_stat64i32(pFileName, &Stat);
    ArchiveFileTime = Stat.st_mtime;
    if ((cArchiveFlags & ARCHIVE_FLAG_SECONDARY) == 0) {
        if (ArchiveManager::bInvalidateOlderFiles) {
            FindNewerAndInvalidate();
        }
        if (abInvalidateOtherArchives) {
            for (uint32_t ii = 0; ii < iDirectories; ++ii) {
                for (uint32_t jj = 0; jj < pDirectories[ii].iFiles; ++jj) {
                    ArchiveManager::InvalidateFileInArchives(
                        pDirectories[ii], pDirectories[ii].pFiles[jj], iArchiveType
                    );
                }
            }
        }
    }
    ChangeBufferSize(0x4000);
    GetSize();
}

// The offset of the texture name inside a texture path: past a drive letter and its colon, one leading backslash, the
// data\ prefix, and the textures\ prefix when abStripTexture is set. Each byte is compared signed.
uint32_t FindTexturePathOffset(const char* apPath, bool abStripTexture) {
    const char* pPath = apPath;
    if (pPath[1] == ':') {
        pPath += 3;
    }
    if (*pPath == '\\') {
        ++pPath;
    }
    if (!_strnicmp(pPath, "data\\", 5)) {
        pPath += 5;
    }
    if (abStripTexture && !_strnicmp(pPath, "textures\\", 9)) {
        pPath += 9;
    }
    return static_cast<uint32_t>(pPath - apPath);
}

// Copies the part of apInPath that follows the texture path prefix and lower-cases it in place. Always returns true.
bool StandardizeTexturePath(const char* apInPath, char* apOutPath, uint32_t aiOutSize) {
    *apOutPath = '\0';
    const char* pTexturePath = &apInPath[FindTexturePathOffset(apInPath, false)];
    strcat_s(apOutPath, aiOutSize, pTexturePath);
    _strlwr_s(apOutPath, aiOutSize);
    return true;
}

// Partitions apHashArray[aiLeft..aiRight] around its first hash and sorts both halves.
void QuickSortHashArray(BSHash** apHashArray, int32_t aiLeft, int32_t aiRight) {
    // Orders by CRC, then by the packed character word, unsigned: -1, 0 or 1.
    // EXE-INLINED 0xAFCE90
    auto Compare = [](const BSHash* apHash, const BSHash* apOther) -> int32_t {
        if (apHash->iCRC < apOther->iCRC || (apHash->iCRC == apOther->iCRC && apHash->GetAll() < apOther->GetAll())) {
            return -1;
        }
        if (apHash->iCRC > apOther->iCRC || (apHash->iCRC == apOther->iCRC && apHash->GetAll() > apOther->GetAll())) {
            return 1;
        }
        return 0;
    };
    int32_t iLeft = aiLeft;
    int32_t iRight = aiRight;
    BSHash* pPivot = apHashArray[aiLeft];
    while (aiLeft < aiRight) {
        while (Compare(apHashArray[aiRight], pPivot) >= 0 && aiLeft < aiRight) {
            --aiRight;
        }
        if (aiLeft != aiRight) {
            apHashArray[aiLeft++] = apHashArray[aiRight];
        }
        while (Compare(apHashArray[aiLeft], pPivot) <= 0 && aiLeft < aiRight) {
            ++aiLeft;
        }
        if (aiLeft != aiRight) {
            apHashArray[aiRight--] = apHashArray[aiLeft];
        }
    }
    apHashArray[aiLeft] = pPivot;
    if (iLeft < aiLeft) {
        QuickSortHashArray(apHashArray, iLeft, aiLeft - 1);
    }
    if (iRight > aiLeft) {
        QuickSortHashArray(apHashArray, aiLeft + 1, iRight);
    }
}

void StandardizeModelPath(const char* apFileName, char* apStandardizedFileName, uint32_t aiOutSize) {
    size_t uiLength = std::strlen(apFileName);
    if (uiLength > 3 && !_stricmp(&apFileName[uiLength - 3], "spt")) {
        strcpy_s(apStandardizedFileName, aiOutSize, "Trees");
        strcat_s(apStandardizedFileName, aiOutSize, apFileName);
    }
    else {
        if (!_strnicmp(apFileName, "meshes\\", 7u)) {
            strcpy_s(apStandardizedFileName, aiOutSize, apFileName);
        }
        else {
            strcpy_s(apStandardizedFileName, aiOutSize, "meshes\\");
            strcat_s(apStandardizedFileName, aiOutSize, apFileName);
        }
    }
}

void StandardizeDataPath(const char* apFileName, char* apStandardizedFileName, uint32_t aiOutSize) {
    if (std::strlen(apFileName) > 3 && apFileName[1] == 58) {
        apFileName += 3;
    }
    if (!_strnicmp(apFileName, "data\\", 5u)) {
        apFileName += 5;
    }
    strcpy_s(apStandardizedFileName, aiOutSize, apFileName);
}

// The critical section, the object count and the file are left to ~BSCriticalSection, ~NiRefObject and ~BSFile.
Archive::~Archive() {
    DeleteDirectoryStringArray();
    DeleteFilenameStringArray(false);
    DeleteFileData();
}

char* Archive::GetDirectoryString(uint32_t aiDirectoryIndex) {
    uint32_t v8;
    uint32_t v12;
    uint8_t v17;
    if ((cArchiveFlags & 16) != 0) {
        return &this->pDirectoryStringArray[pDirectoryStringOffsets[aiDirectoryIndex]];
    }
    if ((oFlags.iFlag & 1) == 0) {
        return 0;
    }
    uint32_t iDirectories = this->iDirectories;
    Seek(16 * iDirectories + 36, NiFile::ms_iSeekSet);
    for (uint32_t i = 0; i < aiDirectoryIndex; ++i) {
        v12 = 1;
        uint32_t v10 = m_pfnRead(this, &v17, 1, &v12, 1);
        m_uiAbsoluteCurrentPos += v10;
        uint32_t iFiles = pDirectories[i].iFiles;
        Seek(16 * iFiles + v17, NiFile::ms_iSeekCur);
    }
    v8 = 1;
    uint32_t v6 = m_pfnRead(this, &v17, 1, &v8, 1);
    m_uiAbsoluteCurrentPos += v6;
    if (this->pDirectoryStringArray) {
        void* Block = (void*)this->pDirectoryStringArray;
        MemoryManager_Singleton.Deallocate(Block);
        this->pDirectoryStringArray = 0;
    }
    char* v14 = static_cast<char*>(MemoryManager_Singleton.Allocate(v17));
    this->pDirectoryStringArray = v14;
    char* pDirectoryStringArray = this->pDirectoryStringArray;
    uint32_t uiComponentSize = 1;
    m_uiAbsoluteCurrentPos += m_pfnRead(this, pDirectoryStringArray, v17, &uiComponentSize, 1);
    return this->pDirectoryStringArray;
}

// Loaded strings are indexed directly. Otherwise pFileNameStringArray is replaced by a 0x100-byte buffer: with the
// offset table retained the name is read a byte at a time at its offset; else the whole name block is read and
// walked name by name. A walk past the block leaves an empty name.
char* Archive::GetFileString(uint32_t aiDirectoryIndex, uint32_t aiFileIndex) {
    if (cArchiveFlags & ARCHIVE_FLAG_HAS_FILE_STRINGS) {
        return &pFileNameStringArray[pFileNameStringOffsets[aiDirectoryIndex][aiFileIndex]];
    }
    if (!(oFlags.iFlag & ARCHIVE_FILESTRINGS)) {
        return nullptr;
    }
    if (pFileNameStringArray) {
        MemoryManager_Singleton.Deallocate(pFileNameStringArray);
        pFileNameStringArray = nullptr;
    }
    uint32_t ipos = 0;
    pFileNameStringArray = static_cast<char*>(MemoryManager_Singleton.Allocate(0x100));
    if (CheckRetain(
            (oFlags.iFlag & ARCHIVE_RETAIN_FILE_NAME_OFFSETS) != 0, ArchiveManager::iRetainFilenameOffsetTable
        )) {
        uint32_t iseekPos = pFileNameStringOffsets[aiDirectoryIndex][aiFileIndex] + iFileNameArrayOffset;
        Seek(iseekPos, NiFile::ms_iSeekSet);
        uint32_t uiSize = 1;
        BinaryRead(&pFileNameStringArray[ipos], 1, &uiSize, 1);
        while (pFileNameStringArray[ipos++]) {
            uiSize = 1;
            BinaryRead(&pFileNameStringArray[ipos], 1, &uiSize, 1);
        }
        return pFileNameStringArray;
    }
    char* files = static_cast<char*>(MemoryManager_Singleton.Allocate(iFileNamesLength + 1));
    Seek(iFileNameArrayOffset, NiFile::ms_iSeekSet);
    uint32_t uiSize = 1;
    BinaryRead(files, iFileNamesLength, &uiSize, 1);
    files[iFileNamesLength] = '\0';
    char* p = files;
    for (uint32_t i = 0; i < aiDirectoryIndex; ++i) {
        for (uint32_t j = 0; j < pDirectories[i].iFiles; ++j) {
            p += std::strlen(p) + 1;
            if (p >= &files[iFileNamesLength]) {
                *pFileNameStringArray = '\0';
                MemoryManager_Singleton.Deallocate(files);
                return pFileNameStringArray;
            }
        }
    }
    for (uint32_t j = 0; j < aiFileIndex; ++j) {
        p += std::strlen(p) + 1;
        if (p >= &files[iFileNamesLength]) {
            *pFileNameStringArray = '\0';
            MemoryManager_Singleton.Deallocate(files);
            return pFileNameStringArray;
        }
    }
    strcpy_s(pFileNameStringArray, 0x100, p);
    MemoryManager_Singleton.Deallocate(files);
    return pFileNameStringArray;
}

bool Archive::FindFile(
    BSHash& arDirectoryHash,
    BSHash& arFileNameHash,
    uint32_t& ariDirectoryIndex,
    uint32_t& ariFileIndex,
    const char* apFileName
) {
    return FindDirectory(arDirectoryHash, ariDirectoryIndex, apFileName) &&
        FindFileInDirectory(ariDirectoryIndex, arFileNameHash, ariFileIndex, apFileName, false);
}

// Tries the last directory found first, then binary-searches the sorted directory table; the hash compare checks the
// CRC, then the packed characters, both unsigned. With runtime collision checks on, the found directory's name must
// also match the directory part of apFileName (its trailing separator cut).
bool Archive::FindDirectory(BSHash& arDirectoryHash, uint32_t& ariDirectoryIndex, const char* apFileName) {
    if (cArchiveFlags & ARCHIVE_FLAG_DISABLED) {
        return false;
    }
    bool bfound = false;
    uint32_t itempLastDirectoryIndex = iLastDirectoryIndex;
    if (itempLastDirectoryIndex < iDirectories) {
        BSDirectoryEntry* pDirectory = &pDirectories[itempLastDirectoryIndex];
        int32_t iHashCompare;
        if (arDirectoryHash.iCRC < pDirectory->iCRC ||
            (arDirectoryHash.iCRC == pDirectory->iCRC && arDirectoryHash.GetAll() < pDirectory->GetAll())) {
            iHashCompare = -1;
        }
        else if (
            arDirectoryHash.iCRC > pDirectory->iCRC ||
            (arDirectoryHash.iCRC == pDirectory->iCRC && arDirectoryHash.GetAll() > pDirectory->GetAll())
        ) {
            iHashCompare = 1;
        }
        else {
            iHashCompare = 0;
        }
        if (iHashCompare == 0) {
            ariDirectoryIndex = itempLastDirectoryIndex;
            bfound = true;
        }
    }
    if (bfound) {
        return bfound;
    }
    iLastFileIndex = 0xFFFFFFFF;
    if (!iDirectories) {
        return bfound;
    }
    uint32_t ilowerBound = 0;
    uint32_t iupperBound = iDirectories;
    uint32_t icurrent = 0;
    uint32_t ioffset = 1;
    int32_t icompare = 0;
    while (ioffset) {
        ioffset = (iupperBound - ilowerBound) >> 1;
        icurrent = ilowerBound + ioffset;
        BSDirectoryEntry* pDirectory = &pDirectories[icurrent];
        if (arDirectoryHash.iCRC < pDirectory->iCRC ||
            (arDirectoryHash.iCRC == pDirectory->iCRC && arDirectoryHash.GetAll() < pDirectory->GetAll())) {
            icompare = -1;
        }
        else if (
            arDirectoryHash.iCRC > pDirectory->iCRC ||
            (arDirectoryHash.iCRC == pDirectory->iCRC && arDirectoryHash.GetAll() > pDirectory->GetAll())
        ) {
            icompare = 1;
        }
        else {
            icompare = 0;
        }
        if (icompare > 0) {
            ilowerBound = icurrent;
        }
        else if (icompare < 0) {
            iupperBound = icurrent;
        }
        else {
            break;
        }
    }
    if (icompare) {
        return bfound;
    }
    ariDirectoryIndex = icurrent;
    iLastDirectoryIndex = icurrent;
    bfound = true;
    if (apFileName && ArchiveManager::bCheckRuntimeCollisions && (oFlags.iFlag & ARCHIVE_DIRSTRINGS)) {
        char pdirectoryName[0x104];
        _splitpath_s(apFileName, nullptr, 0, pdirectoryName, 0x104, nullptr, 0, nullptr, 0);
        char* pdirectoryString = GetDirectoryString(icurrent);
        char* comp1 = static_cast<char*>(MemoryManager_Singleton.Allocate(std::strlen(pdirectoryString) + 1));
        char* comp2 = static_cast<char*>(MemoryManager_Singleton.Allocate(std::strlen(pdirectoryName) + 1));
        strcpy_s(comp1, std::strlen(pdirectoryString) + 1, pdirectoryString);
        strcpy_s(comp2, std::strlen(pdirectoryName) + 1, pdirectoryName);
        comp2[std::strlen(comp2) - 1] = '\0';
        _strupr_s(comp1, std::strlen(comp1) + 1);
        _strupr_s(comp2, std::strlen(comp2) + 1);
        if (_stricmp(comp1, comp2)) {
            Warning("DEFAULT: HashMap Collision between %s and %s", pdirectoryString, pdirectoryName);
            bfound = false;
        }
        MemoryManager_Singleton.Deallocate(comp1);
        MemoryManager_Singleton.Deallocate(comp2);
    }
    return bfound;
}

// Tries the last file found first, then binary-searches the directory's sorted file entries; a hit must be valid and
// not invalidated by a loose file. With runtime collision checks on, the found file's name must also match the name
// part of apFileName, both cut at their first '.'.
bool Archive::FindFileInDirectory(
    uint32_t aiDirectoryIndex,
    BSHash& arFileNameHash,
    uint32_t& ariFileIndex,
    const char* apFileName,
    bool abSkipSecondaryArchiveCheck
) {
    if (cArchiveFlags & ARCHIVE_FLAG_DISABLED) {
        return false;
    }
    bool bfound = false;
    BSDirectoryEntry* pdirectory = &pDirectories[aiDirectoryIndex];
    uint32_t itempLastFileIndex = iLastFileIndex;
    if (itempLastFileIndex < pdirectory->iFiles) {
        BSFileEntry* pfile = &pdirectory->pFiles[itempLastFileIndex];
        if (CheckValidFile(pfile, abSkipSecondaryArchiveCheck) && !arFileNameHash.Compare(*pfile)) {
            if (CheckInvalidateFile(pfile, apFileName)) {
                return false;
            }
            ariFileIndex = itempLastFileIndex;
            bfound = true;
        }
    }
    if (bfound || !pdirectory->iFiles) {
        return bfound;
    }
    BSFileEntry* pfile = nullptr;
    uint32_t ilowerBound = 0;
    uint32_t iupperBound = pdirectory->iFiles;
    uint32_t icurrent = 0;
    uint32_t ioffset = 1;
    int32_t icompare = 0;
    while (ioffset) {
        ioffset = (iupperBound - ilowerBound) >> 1;
        icurrent = ilowerBound + ioffset;
        pfile = &pdirectory->pFiles[icurrent];
        icompare = arFileNameHash.Compare(*pfile);
        if (icompare > 0) {
            ilowerBound = icurrent;
        }
        else if (icompare < 0) {
            iupperBound = icurrent;
        }
        else {
            break;
        }
    }
    if (icompare || !CheckValidFile(pfile, abSkipSecondaryArchiveCheck) || CheckInvalidateFile(pfile, apFileName)) {
        return bfound;
    }
    ariFileIndex = icurrent;
    iLastFileIndex = icurrent;
    bfound = true;
    if (apFileName && ArchiveManager::bCheckRuntimeCollisions && (oFlags.iFlag & ARCHIVE_FILESTRINGS)) {
        char pfileName[0x104];
        _splitpath_s(apFileName, nullptr, 0, nullptr, 0, pfileName, 0x104, nullptr, 0);
        char* pfileString = GetFileString(aiDirectoryIndex, icurrent);
        char* comp1 = static_cast<char*>(MemoryManager_Singleton.Allocate(std::strlen(pfileString) + 1));
        char* comp2 = static_cast<char*>(MemoryManager_Singleton.Allocate(std::strlen(pfileName) + 1));
        strcpy_s(comp1, std::strlen(pfileString) + 1, pfileString);
        strcpy_s(comp2, std::strlen(pfileName) + 1, pfileName);
        char* extidx;
        strtok_s(comp1, ".", &extidx);
        strtok_s(comp2, ".", &extidx);
        _strupr_s(comp1, std::strlen(comp1) + 1);
        _strupr_s(comp2, std::strlen(comp2) + 1);
        if (_stricmp(comp1, comp2)) {
            Warning("DEFAULT: HashMap Collision between %s and %s", pfileString, pfileName);
            bfound = false;
        }
        MemoryManager_Singleton.Deallocate(comp1);
        MemoryManager_Singleton.Deallocate(comp2);
    }
    return bfound;
}

bool Archive::InvalidateFile(BSHash& arDirectoryHash, BSHash& arFileNameHash, const char* apFileName) {
    uint32_t ifileIndex;
    uint32_t idirectoryIndex;
    if (!FindFile(arDirectoryHash, arFileNameHash, idirectoryIndex, ifileIndex, 0)) {
        return 0;
    }
    BSFileEntry* pFileEntry = &pDirectories[idirectoryIndex].pFiles[ifileIndex];
    pFileEntry->iOffset &= 0x80000000u;
    return 1;
}

bool Archive::InvalidateFile(uint32_t aiDirectoryIndex, BSHash& arFileNameHash, const char* apFileName) {
    uint32_t ifileIndex;
    if (!FindFileInDirectory(aiDirectoryIndex, arFileNameHash, ifileIndex, 0, 0)) {
        return 0;
    }
    BSFileEntry* v6 = &pDirectories[aiDirectoryIndex].pFiles[ifileIndex];
    v6->iOffset &= 0x80000000u;
    return 1;
}

// The entry's offset keeps its secondary-archive bit out; its size keeps the compression-toggle and checked bits
// out. A compression toggle that differs from the archive's default opens a CompressedArchiveFile.
ArchiveFile*
Archive::GetFile(uint32_t aiDirectoryIndex, uint32_t aiFileIndex, uint32_t aiBufferSize, const char* apFileName) {
    ArchiveFile* pfile = nullptr;
    BSFileEntry* pfileEntry = &pDirectories[aiDirectoryIndex].pFiles[aiFileIndex];
    if (pfileEntry && CheckValidFile(pfileEntry, false)) {
        uint32_t ioffset = pfileEntry->iOffset & 0x7FFFFFFF;
        if (((oFlags.iFlag & ARCHIVE_COMPRESSED) != 0) == ((pfileEntry->iSize & 0x40000000) != 0)) {
            void* pMemory = MemoryManager_Singleton.Allocate(sizeof(ArchiveFile));
            pfile = pMemory ? ::new (pMemory)
                                  ArchiveFile(apFileName, this, ioffset, pfileEntry->iSize & 0x3FFFFFFF, aiBufferSize)
                            : nullptr;
        }
        else {
            void* pMemory = MemoryManager_Singleton.Allocate(sizeof(CompressedArchiveFile));
            pfile = pMemory
                ? ::new (pMemory)
                      CompressedArchiveFile(apFileName, this, ioffset, pfileEntry->iSize & 0x3FFFFFFF, aiBufferSize)
                : nullptr;
        }
    }
    return pfile;
}

BSFileEntry* Archive::GetFileEntryForFile(BSHash& arDirectoryHash, BSHash& arFileNameHash, const char* apFileName) {
    uint32_t idirectoryIndex;
    uint32_t ifileIndex;
    if (FindDirectory(arDirectoryHash, idirectoryIndex, apFileName) &&
        FindFileInDirectory(idirectoryIndex, arFileNameHash, ifileIndex, apFileName, true)) {
        return &pDirectories[idirectoryIndex].pFiles[ifileIndex];
    }
    return nullptr;
}

// As GetFile, for an entry already found; the entry's size is read before its offset.
ArchiveFile* Archive::GetFileByFileEntry(BSFileEntry* apFileEntry, uint32_t aiBufferSize, const char* apFileName) {
    ArchiveFile* pfile = nullptr;
    if (apFileEntry && CheckValidFile(apFileEntry, false)) {
        if (((oFlags.iFlag & ARCHIVE_COMPRESSED) != 0) == ((apFileEntry->iSize & 0x40000000) != 0)) {
            void* pMemory = MemoryManager_Singleton.Allocate(sizeof(ArchiveFile));
            if (pMemory) {
                uint32_t iSize = apFileEntry->iSize & 0x3FFFFFFF;
                uint32_t iOffset = apFileEntry->iOffset & 0x7FFFFFFF;
                pfile = ::new (pMemory) ArchiveFile(apFileName, this, iOffset, iSize, aiBufferSize);
            }
        }
        else {
            void* pMemory = MemoryManager_Singleton.Allocate(sizeof(CompressedArchiveFile));
            if (pMemory) {
                uint32_t iSize = apFileEntry->iSize & 0x3FFFFFFF;
                uint32_t iOffset = apFileEntry->iOffset & 0x7FFFFFFF;
                pfile = ::new (pMemory) CompressedArchiveFile(apFileName, this, iOffset, iSize, aiBufferSize);
            }
        }
    }
    return pfile;
}

bool Archive::CheckRetain(bool abArchive, int32_t aiINI) {
    if (aiINI == 1) {
        return abArchive;
    }
    return aiINI != 0;
}

// An entry not yet checked is invalidated by a loose copy under Data\: its offset is cleared (the secondary-archive
// bit kept). Otherwise the entry is marked checked so later lookups skip the disk.
bool Archive::CheckInvalidateFile(BSFileEntry* apFile, const char* apFileName) {
    if (!ArchiveManager::bInvalidateOlderFiles || (apFile->iSize & 0x80000000)) {
        return false;
    }
    if (apFileName) {
        char pfilename[0x104];
        strcpy_s(pfilename, 0x104, "Data\\");
        strcat_s(pfilename, 0x104, apFileName);
        if (rt::_access(pfilename, 0) != -1) {
            apFile->iOffset &= 0x80000000;
            return true;
        }
    }
    apFile->iSize |= 0x80000000;
    return false;
}

// The entry's secondary-archive bit must match the archive's, unless skipped, and its offset must be nonzero.
bool Archive::CheckValidFile(BSFileEntry* apFileEntry, bool abSkipSecondaryArchiveCheck) {
    return (abSkipSecondaryArchiveCheck ||
            ((apFileEntry->iOffset & 0x80000000) != 0) == ((cArchiveFlags & ARCHIVE_FLAG_SECONDARY) != 0)) &&
        (apFileEntry->iOffset & 0x7FFFFFFF) != 0;
}

// Name tables stay loaded while files are invalidated or collisions checked, or when the archive or the INI asks
// to retain them; a voice archive also needs its file names or their offsets retained.
bool Archive::QLoadStrings() {
    return (ArchiveManager::bInvalidateOlderFiles || ArchiveManager::bCheckRuntimeCollisions ||
            (oFlags.iFlag & ARCHIVE_RETAIN_STRINGS_DURING_STARTUP) ||
            CheckRetain(
                (oFlags.iFlag & ARCHIVE_RETAIN_DIRECTORY_NAMES) != 0, ArchiveManager::iRetainDirectoryStringTable
            ) ||
            CheckRetain((oFlags.iFlag & ARCHIVE_RETAIN_FILE_NAMES) != 0, ArchiveManager::iRetainFilenameStringTable) ||
            CheckRetain(
                (oFlags.iFlag & ARCHIVE_RETAIN_FILE_NAME_OFFSETS) != 0, ArchiveManager::iRetainFilenameOffsetTable
            )) &&
        (!(iArchiveType & ARCHIVE_TYPE_VOICES) ||
         CheckRetain((oFlags.iFlag & ARCHIVE_RETAIN_FILE_NAMES) != 0, ArchiveManager::iRetainFilenameStringTable) ||
         CheckRetain(
             (oFlags.iFlag & ARCHIVE_RETAIN_FILE_NAME_OFFSETS) != 0, ArchiveManager::iRetainFilenameOffsetTable
         ));
}

void Archive::ReloadFilenamesAndIndices() {
    uint32_t v4;
    if (this->iFiles) {
        if (!this->pFileNameStringArray || !pFileNameStringOffsets) {
            cArchiveFlags &= ~32;
            if (pFileNameStringOffsets) {
                for (uint32_t i = 0;; ++i) {
                    uint32_t iDirectories = this->iDirectories;
                    if (i >= iDirectories) {
                        break;
                    }
                    void* Block = (void*)pFileNameStringOffsets[i];
                    MemoryManager_Singleton.Deallocate(Block);
                }
                void* apMem = (void*)pFileNameStringOffsets;
                MemoryManager_Singleton.Deallocate(apMem);
            }
            void* pFileNameStringArray = (void*)this->pFileNameStringArray;
            MemoryManager_Singleton.Deallocate(pFileNameStringArray);
            size_t Size = this->iFileNamesLength;
            char* v13 = static_cast<char*>(MemoryManager_Singleton.Allocate(Size));
            this->pFileNameStringArray = v13;
            uint32_t v7 = this->iDirectories;
            size_t aSize = v7 <= SIZE_MAX / sizeof(uint32_t*) ? sizeof(uint32_t*) * v7 : SIZE_MAX;
            uint32_t** v12 = static_cast<uint32_t**>(MemoryManager_Singleton.Allocate(aSize));
            pFileNameStringOffsets = v12;
            if ((oFlags.iFlag & 2) != 0) {
                Seek(iFileNameArrayOffset, NiFile::ms_iSeekSet);
                uint32_t iFileNamesLength = this->iFileNamesLength;
                char* v3 = this->pFileNameStringArray;
                v4 = 1;
                m_uiAbsoluteCurrentPos += m_pfnRead(this, v3, iFileNamesLength, &v4, 1);
                uint32_t v19 = 0;
                for (uint32_t j = 0; j < this->iDirectories; ++j) {
                    uint32_t iFiles = pDirectories[j].iFiles;
                    uint32_t* v11 = static_cast<uint32_t*>(MemoryManager_Singleton.Allocate(
                        iFiles <= SIZE_MAX / sizeof(uint32_t) ? sizeof(uint32_t) * iFiles : SIZE_MAX
                    ));
                    pFileNameStringOffsets[j] = v11;
                    for (uint32_t k = 0; k < pDirectories[j].iFiles; ++k) {
                        pFileNameStringOffsets[j][k] = v19;
                        v19 += std::strlen(&this->pFileNameStringArray[v19]) + 1;
                    }
                }
            }
            cArchiveFlags |= 32;
        }
    }
}

uint32_t Archive::GetDirectoryIndexForFileEntry(BSFileEntry* apFileEntry) {
    uintptr_t pFiles;
    BSDirectoryEntry* v4;
    uint32_t i;
    for (i = 0; i < ((uint32_t)((int32_t)iDirectories)); ++i) {
        v4 = &pDirectories[i];
        pFiles = reinterpret_cast<uintptr_t>(v4->pFiles);
        if ((reinterpret_cast<uintptr_t>(apFileEntry) >= pFiles) &&
            (reinterpret_cast<uintptr_t>(apFileEntry) < ((16 * v4->iFiles) + pFiles))) {
            return i;
        }
    }
    return (uint32_t)((-1));
}

// The address distance is divided unsigned, not as a signed pointer difference.
uint32_t Archive::GetFileIndexForFileEntryFromDirectory(uint32_t aiDirectoryIndex, BSFileEntry* apFileEntry) {
    return static_cast<uint32_t>(
               reinterpret_cast<uintptr_t>(apFileEntry) -
               reinterpret_cast<uintptr_t>(pDirectories[aiDirectoryIndex].pFiles)
           ) /
        sizeof(BSFileEntry);
}

char* Archive::GetFileNameForFileEntry(BSFileEntry* apFileEntry) {
    uint32_t aiDirectoryIndex = GetDirectoryIndexForFileEntry(apFileEntry);
    if (aiDirectoryIndex == ((-1))) {
        return nullptr;
    }
    uint32_t aiFileIndex = GetFileIndexForFileEntryFromDirectory(aiDirectoryIndex, apFileEntry);
    return GetFileString(aiDirectoryIndex, aiFileIndex);
}

void Archive::DeleteFileData() {
    uint32_t i;
    if ((cArchiveFlags & 4) == 0) {
        if (pDirectories) {
            for (i = 0; i < ((uint32_t)((int32_t)iDirectories)); ++i) {
                (&MemoryManager_Singleton)->Deallocate(pDirectories[i].pFiles);
            }
            (&MemoryManager_Singleton)->Deallocate(pDirectories);
        }
        pDirectories = nullptr;
    }
}

void Archive::DeleteDirectoryStringArray() {
    if ((cArchiveFlags & 4) == 0) {
        if (pDirectoryStringArray) {
            (&MemoryManager_Singleton)->Deallocate(pDirectoryStringArray);
        }
        if (pDirectoryStringOffsets) {
            (&MemoryManager_Singleton)->Deallocate(pDirectoryStringOffsets);
        }
        pDirectoryStringArray = nullptr;
        pDirectoryStringOffsets = (uint32_t*)0;
    }
    cArchiveFlags &= (~0x10u);
}

const char* Archive::DoesFileCollide(BSHash& arDirectoryHash, BSHash& arFileNameHash, char* apFileName) {
    char String2[260];
    char Str[260];
    uint32_t ifileIndex;
    uint32_t idirectoryIndex;
    if (!ArchiveManager::bCheckRuntimeCollisions) {
        return 0;
    }
    if ((oFlags.iFlag & 1) == 0) {
        return 0;
    }
    if ((oFlags.iFlag & 2) == 0) {
        return 0;
    }
    _splitpath_s(apFileName, 0, 0, String2, 260u, Str, 260u, 0, 0);
    if (!FindFile(arDirectoryHash, arFileNameHash, idirectoryIndex, ifileIndex, 0)) {
        return 0;
    }
    const char* pdirectoryString = GetDirectoryString(idirectoryIndex);
    const char* pfileString = GetFileString(idirectoryIndex, ifileIndex);
    size_t v13 = std::strlen(pdirectoryString);
    char* comp1 = static_cast<char*>(MemoryManager_Singleton.Allocate(v13 + 1));
    size_t v12 = std::strlen(pfileString);
    char* comp2 = static_cast<char*>(MemoryManager_Singleton.Allocate(v12 + 1));
    size_t stDestSize = std::strlen(pdirectoryString);
    strcpy_s(comp1, stDestSize, pdirectoryString);
    uint32_t SizeInBytes = std::strlen(pfileString);
    strcpy_s(comp2, SizeInBytes, pfileString);
    size_t v9 = std::strlen(comp1);
    _strupr_s(comp1, v9);
    size_t v8 = std::strlen(comp2);
    _strupr_s(comp2, v8);
    size_t v7 = std::strlen(String2);
    _strupr_s(String2, v7);
    size_t v6 = std::strlen(Str);
    _strupr_s(Str, v6);
    if (_stricmp(comp1, String2)) {
        MemoryManager_Singleton.Deallocate(comp1);
        MemoryManager_Singleton.Deallocate(comp2);
        return pdirectoryString;
    }
    if (!_stricmp(comp2, Str)) {
        return 0;
    }
    MemoryManager_Singleton.Deallocate(comp1);
    MemoryManager_Singleton.Deallocate(comp2);
    return pfileString;
}

uint32_t Archive::FindNewerAndInvalidate() {
    _FILETIME LastWriteTime;
    uint32_t inumInvalidated = 0;
    if (!ArchiveManager::pInvalidateDirectoryHashes && !ArchiveManager::pInvalidateFileNameHashes) {
        return 0;
    }
    if ((oFlags.iFlag & 1) != 0 && (oFlags.iFlag & 2) != 0) {
        for (uint32_t i_2 = 0; i_2 < iDirectories; ++i_2) {
            bool v14;
            BSDirectoryEntry* pdirectory = &pDirectories[i_2];
            if (!ArchiveManager::pInvalidateDirectoryHashes) {
                goto LABEL_20;
            }
            v14 = 0;
            for (uint32_t i = 0; i < ArchiveManager::pInvalidateDirectoryHashes->iSize; ++i) {
                BSHash* phash = ArchiveManager::pInvalidateDirectoryHashes->pBuffer[i];
                if (!phash->Compare(*pdirectory)) {
                    v14 = 1;
                    break;
                }
            }
            if (v14) {
                for (uint32_t j = 0; j < pdirectory->iFiles; ++j) {
                    pdirectory->pFiles[j].iOffset &= 0x80000000u;
                }
                inumInvalidated += pdirectory->iFiles;
            }
            else {
                char FileName[260];
            LABEL_20:;
                strcpy_s(FileName, 260, "Data\\");
                char* pdirectoryname = GetDirectoryString(i_2);
                strcat_s(FileName, 260, pdirectoryname);
                int32_t iresult = rt::_access(FileName, 0);
                if (iresult != -1) {
                    for (uint32_t k = 0; k < pdirectory->iFiles; ++k) {
                        BSFileEntry* v9 = &pdirectory->pFiles[k];
                        if (ArchiveManager::pInvalidateFileNameHashes) {
                            char v8 = 0;
                            for (uint32_t m = 0; m < ArchiveManager::pInvalidateFileNameHashes->iSize; ++m) {
                                int32_t v3;
                                BSHash* v6 = ArchiveManager::pInvalidateFileNameHashes->pBuffer[m];
                                // EXE-INLINED 0xAFCE90
                                // BSHash::Compare(*v9): the CRC, then the four characters packed into one word.
                                uint32_t uiNameAll = static_cast<uint32_t>(static_cast<uint8_t>(v6->cLast)) |
                                    static_cast<uint32_t>(static_cast<uint8_t>(v6->cLast2)) << 8 |
                                    static_cast<uint32_t>(static_cast<uint8_t>(v6->cLength)) << 16 |
                                    static_cast<uint32_t>(static_cast<uint8_t>(v6->cFirst)) << 24;
                                uint32_t uiEntryAll = static_cast<uint32_t>(static_cast<uint8_t>(v9->cLast)) |
                                    static_cast<uint32_t>(static_cast<uint8_t>(v9->cLast2)) << 8 |
                                    static_cast<uint32_t>(static_cast<uint8_t>(v9->cLength)) << 16 |
                                    static_cast<uint32_t>(static_cast<uint8_t>(v9->cFirst)) << 24;
                                if (v6->iCRC < v9->iCRC || (v6->iCRC == v9->iCRC && uiNameAll < uiEntryAll)) {
                                    v3 = -1;
                                }
                                else if (v6->iCRC > v9->iCRC || (v6->iCRC == v9->iCRC && uiNameAll > uiEntryAll)) {
                                    v3 = 1;
                                }
                                else {
                                    v3 = 0;
                                }
                                if (!v3) {
                                    v8 = 1;
                                    break;
                                }
                            }
                            if (v8) {
                                v9->iOffset &= 0x80000000u;
                                ++inumInvalidated;
                            }
                        }
                    }
                }
            }
        }
    }
    else {
        PlatformGetLastWriteTime(&LastWriteTime);
        return InvalidateOlderFilesByPath(ArchiveManager::sMasterFilePath, "", &LastWriteTime);
    }
    return inumInvalidated;
}

void Archive::DeleteFilenameStringArray(bool abPreserveOffsets) {
    if ((cArchiveFlags & 4) == 0) {
        if (pFileNameStringArray) {
            MemoryManager_Singleton.Deallocate(pFileNameStringArray);
        }
        pFileNameStringArray = 0;
        if (pFileNameStringOffsets) {
            if (!abPreserveOffsets) {
                for (uint32_t i = 0; i < iDirectories; ++i) {
                    MemoryManager_Singleton.Deallocate(pFileNameStringOffsets[i]);
                }
                MemoryManager_Singleton.Deallocate(pFileNameStringOffsets);
                pFileNameStringOffsets = 0;
            }
        }
    }
    cArchiveFlags &= ~32;
}

void Archive::operator delete(void* apMemory, size_t aSize) {
    NiMemObject::operator delete(apMemory, aSize);
}

// BSFile() is assumed native; spArchive takes its reference through NiPointer's constructor.
ArchiveFile::ArchiveFile(
    const char* apFileName,
    Archive* apArchive,
    uint32_t aiOffset,
    uint32_t aiSize,
    uint32_t aiBufferSize
)
  : BSFile(), spArchive(apArchive), iArchiveOffset(aiOffset) {
    if (apFileName) {
        strcpy_s(pFileName, 260, apFileName);
    }
    iFileSize = aiSize;
    ArchiveFile::SetEndianSwap(false);
    // The whole entry when it fits the archive buffer size or the caller asks for it (aiBufferSize -1).
    if (aiSize <= ArchiveManager::iArchiveFileBufferSize || aiBufferSize == 0xFFFFFFFFu) {
        m_uiBufferAllocSize = aiSize;
    }
    else {
        m_uiBufferAllocSize = ArchiveManager::iArchiveFileBufferSize;
    }
    ArchiveFile::Open(false, false);
    bool bEmbeddedNames = (spArchive->oFlags.iFlag & BSArchive::ARCHIVE_EMBEDDED_FILE_NAMES) != 0;
    if (aiBufferSize == 0xFFFFFFFFu || bEmbeddedNames) {
        m_uiBufferReadSize = ReadFromArchive(m_pBuffer, m_uiBufferAllocSize, 0);
        if (bEmbeddedNames) {
            // The entry starts with its counted name: take it as the file name and skip it.
            uint8_t cLength = m_pBuffer[m_uiPos];
            rt::memcpy(pFileName, &m_pBuffer[m_uiPos + 1], cLength);
            pFileName[cLength] = 0;
            m_uiPos += cLength + 1;
            iTrueFilePos = cLength + 1;
        }
    }
}

uint32_t ArchiveFile::GetSize() {
    return iFileSize;
}

void ArchiveFile::SetEndianSwap(bool bDoSwap) {
    if (bDoSwap) {
        BSFile::SetEndianSwap(bDoSwap);
    }
    else {
        m_pfnRead = &ArchiveFile::RdNoSwap;
        m_pfnWrite = &BSFile::WrNoSwap;
    }
}

bool ArchiveFile::Open(bool abWaitForOpen, bool abTextMode) {
    if (m_pBuffer) {
        return 1;
    }
    m_uiPos = 0;
    m_uiBufferReadSize = 0;
    m_pBuffer = 0;
    if (m_uiBufferAllocSize) {
        if (!m_pBuffer) {
            m_pBuffer = static_cast<char*>(MemoryManager_Singleton.Allocate(m_uiBufferAllocSize));
        }
    }
    m_bGood = 1;
    return m_bGood;
}

uint32_t ArchiveFile::Read(BSStringT<char>& arString, uint32_t aiMaxLength) {
    uint32_t _iAuxTrueFilePos;
    uint32_t _uiTrueFilePos;
    spArchive.m_pObject->GetDevice();
    if (spArchive.m_pObject) {
        EnterCriticalSection(&spArchive.m_pObject->ArchiveCriticalSection.CriticalSection);
    }
    if (iAuxTrueFilePos == -1) {
        _uiTrueFilePos = iTrueFilePos;
    }
    else {
        _uiTrueFilePos = iAuxTrueFilePos;
    }
    int32_t v11 = _uiTrueFilePos + iArchiveOffset;
    Archive* m_pObject = spArchive.m_pObject;
    if (m_pObject->iAuxTrueFilePos == -1) {
        Archive* v4 = spArchive.m_pObject;
        _iAuxTrueFilePos = m_pObject->iTrueFilePos;
    }
    else {
        Archive* v3 = spArchive.m_pObject;
        _iAuxTrueFilePos = m_pObject->iAuxTrueFilePos;
    }
    if (_iAuxTrueFilePos != v11) {
        spArchive.m_pObject->Seek(v11, 0);
    }
    uint32_t ibytesRead = spArchive.m_pObject->Read(arString, iFileSize);
    iTrueFilePos += ibytesRead;
    if (spArchive.m_pObject) {
        LeaveCriticalSection(&spArchive.m_pObject->ArchiveCriticalSection.CriticalSection);
    }
    return ibytesRead;
}

uint32_t ArchiveFile::Read(BSStringT<char16_t>& arString, uint32_t aiMaxLength) {
    uint32_t iAuxTrueFilePos;
    uint32_t iTrueFilePos;
    spArchive.m_pObject->GetDevice();
    if (spArchive.m_pObject) {
        EnterCriticalSection(&spArchive.m_pObject->ArchiveCriticalSection.CriticalSection);
    }
    if (this->iAuxTrueFilePos == -1) {
        iTrueFilePos = this->iTrueFilePos;
    }
    else {
        iTrueFilePos = this->iAuxTrueFilePos;
    }
    int32_t v11 = iTrueFilePos + iArchiveOffset;
    Archive* m_pObject = spArchive.m_pObject;
    if (m_pObject->iAuxTrueFilePos == -1) {
        Archive* v4 = spArchive.m_pObject;
        iAuxTrueFilePos = m_pObject->iTrueFilePos;
    }
    else {
        Archive* v3 = spArchive.m_pObject;
        iAuxTrueFilePos = m_pObject->iAuxTrueFilePos;
    }
    if (iAuxTrueFilePos != v11) {
        spArchive.m_pObject->Seek(v11, 0);
    }
    uint32_t ibytesRead = spArchive.m_pObject->Read(arString, iFileSize);
    this->iTrueFilePos += ibytesRead;
    if (spArchive.m_pObject) {
        LeaveCriticalSection(&spArchive.m_pObject->ArchiveCriticalSection.CriticalSection);
    }
    return ibytesRead;
}

// Tests the pointer field itself.
bool ArchiveFile::Exist() {
    return spArchive.m_pObject != nullptr;
}

// The size compare is unsigned.
uint32_t ArchiveFile::ReadF(void* apReadBuffer, uint32_t aiByteCount) {
    if (iTrueFilePos + aiByteCount > iFileSize) {
        aiByteCount = iFileSize - iTrueFilePos;
    }
    uint32_t uiRead = ReadFromBuffer(apReadBuffer, aiByteCount);
    iTrueFilePos += uiRead;
    return uiRead;
}

uint32_t ArchiveFile::WriteF(const void*, uint32_t /*aiBytes*/) {
    return 0;
}

// The archive's reference is released by spArchive's NiPointer destructor, the file closed by ~BSFile.
ArchiveFile::~ArchiveFile() {
    if (m_pBuffer) {
        MemoryManager_Singleton.Deallocate(m_pBuffer);
    }
    m_pBuffer = nullptr;
    m_bGood = false;
}

void ArchiveFile::Seek(int32_t aiByteCount, int32_t aiSeekFlags) {
    uint32_t uiOrgPos = iTrueFilePos;
    if (aiSeekFlags == 1) {
        iTrueFilePos += aiByteCount;
        if (iTrueFilePos > iFileSize) {
            iTrueFilePos = iFileSize;
        }
    }
    else {
        if (aiSeekFlags == 2) {
            if (aiByteCount < 0) {
                aiByteCount = -aiByteCount;
            }
            if (((uint32_t)aiByteCount) > iFileSize) {
                aiByteCount = (int32_t)iFileSize;
            }
            iTrueFilePos = iFileSize - aiByteCount;
        }
        else {
            if (((uint32_t)aiByteCount) > iFileSize) {
                aiByteCount = (int32_t)iFileSize;
            }
            iTrueFilePos = (uint32_t)aiByteCount;
        }
    }
    m_uiPos += (iTrueFilePos - uiOrgPos);
    if (m_uiPos >= m_uiBufferReadSize) {
        NiFile::Flush();
    }
}

uint32_t ArchiveFile::ReadFromArchive(void* apReadBuffer, uint32_t aiByteCount, int32_t aiOffset) {
    uint32_t itrueBytesRead;
    uint32_t iadditionalBytesRead = 0;
    int32_t v15 = aiOffset + (iTrueFilePos + iArchiveOffset);
    spArchive.m_pObject->GetDevice();
    if (spArchive.m_pObject) {
        EnterCriticalSection(&spArchive.m_pObject->ArchiveCriticalSection.CriticalSection);
    }
    int32_t uiAbsolutePos = spArchive.m_pObject->m_uiCurrentFilePos;
    int32_t v17 = uiAbsolutePos - spArchive.m_pObject->m_uiBufferReadSize;
    if (v17 <= v15 && uiAbsolutePos > v15) {
        int32_t ilength = spArchive.m_pObject->m_uiBufferReadSize - (v15 - v17);
        // The byte count is compared signed here, unsigned everywhere else.
        if (ilength > static_cast<int32_t>(aiByteCount)) {
            ilength = aiByteCount;
        }
        char* v6 = spArchive.m_pObject->m_pBuffer + (v15 - v17);
        rt::memcpy(apReadBuffer, v6, ilength);
        apReadBuffer = (void*)(static_cast<char*>(apReadBuffer) + ilength);
        if (aiByteCount == ilength) {
            if (spArchive.m_pObject) {
                LeaveCriticalSection(&spArchive.m_pObject->ArchiveCriticalSection.CriticalSection);
            }
            return aiByteCount;
        }
        aiByteCount -= ilength;
        v15 += ilength;
        iadditionalBytesRead = ilength;
    }
    spArchive.m_pObject->m_uiPos = 0;
    spArchive.m_pObject->m_uiBufferReadSize = 0;
    if (v15 != uiAbsolutePos) {
        spArchive.m_pObject->NiFile::Seek(v15 - uiAbsolutePos, NiFile::ms_iSeekCur);
        spArchive.m_pObject->iTrueFilePos = v15;
    }
    if (aiOffset + (aiByteCount + iTrueFilePos) > iFileSize) {
        aiByteCount = iFileSize - (aiOffset + iTrueFilePos);
    }
    uint32_t ibytesRead = 0;
    if (v15 >= spArchive.m_pObject->iFileSize) {
        itrueBytesRead = 0;
    }
    else {
        if (aiByteCount >= spArchive.m_pObject->m_uiBufferAllocSize) {
            itrueBytesRead = spArchive.m_pObject->DiskRead(apReadBuffer, aiByteCount);
            ibytesRead = itrueBytesRead;
        }
        else {
            uint32_t v7;
            Archive* m_pObject = spArchive.m_pObject;
            void* pBuffer = (void*)m_pObject->m_pBuffer;
            itrueBytesRead = m_pObject->DiskRead(pBuffer, aiByteCount);
            spArchive.m_pObject->m_uiPos = itrueBytesRead;
            spArchive.m_pObject->m_uiBufferReadSize = itrueBytesRead;
            if (itrueBytesRead >= aiByteCount) {
                v7 = aiByteCount;
            }
            else {
                v7 = itrueBytesRead;
            }
            ibytesRead = v7;
            const void* v9 = (const void*)spArchive.m_pObject->m_pBuffer;
            rt::memcpy(apReadBuffer, v9, v7);
        }
    }
    spArchive.m_pObject->iTrueFilePos = itrueBytesRead + v15;
    if (spArchive.m_pObject) {
        LeaveCriticalSection(&spArchive.m_pObject->ArchiveCriticalSection.CriticalSection);
    }
    return iadditionalBytesRead + ibytesRead;
}

// Calls ArchiveFile's ReadF directly.
uint32_t ArchiveFile::RdNoSwap(
    NiBinaryStream* apThis,
    void* apBuffer,
    uint32_t aiBytes,
    uint32_t* /*apNothing1*/,
    uint32_t /*aiNothing2*/
) {
    return static_cast<ArchiveFile*>(apThis)->ArchiveFile::ReadF(apBuffer, aiBytes);
}

CompressedArchiveFile::CompressedArchiveFile(
    const char* apFileName,
    Archive* apArchive,
    uint32_t aiOffset,
    uint32_t aiSize,
    uint32_t aiBufferSize
)
  : ArchiveFile(apFileName, apArchive, aiOffset, aiSize, aiBufferSize) {
    CompressedArchiveFile::SetEndianSwap(false);
    ArchiveFile::ReadF(&iUncompressedFileSize, 4);
    z_stream_s* pStream = static_cast<z_stream_s*>(MemoryManager_Singleton.Allocate(sizeof(z_stream_s)));
    pZStream = pStream;
    pStream->zalloc = nullptr;
    pStream->zfree = nullptr;
    pStream->opaque = nullptr;
    pStream->avail_in = 0;
    pStream->next_in = nullptr;
    pStream->state = nullptr;
    if (inflateInit(pStream)) {
        inflateEnd(pStream);
        MemoryManager_Singleton.Deallocate(pStream);
        pZStream = nullptr;
    }
    else {
        iUncompressedBufferSize = iUncompressedFileSize >= ArchiveManager::iArchiveFileBufferSize
            ? ArchiveManager::iArchiveFileBufferSize
            : iUncompressedFileSize;
        pUncompressedBuffer = static_cast<char*>(MemoryManager_Singleton.Allocate(iUncompressedBufferSize));
        iUncompressedBufferPos = 0;
        iUncompressedBufferReadSize = 0;
    }
}

uint32_t CompressedArchiveFile::GetSize() {
    return iUncompressedFileSize;
}

uint32_t CompressedArchiveFile::GetFileSize() const {
    return iUncompressedFileSize;
}

// The swapping branch is BSFile's; the plain one installs this class's own read function and BSFile's write function.
void CompressedArchiveFile::SetEndianSwap(bool bDoSwap) {
    if (bDoSwap) {
        BSFile::SetEndianSwap(bDoSwap);
    }
    else {
        m_pfnRead = &CompressedArchiveFile::RdNoSwap;
        m_pfnWrite = &BSFile::WrNoSwap;
    }
}

bool CompressedArchiveFile::Open(bool abWaitForOpen, bool abTextMode) {
    return ArchiveFile::Open(abWaitForOpen, abTextMode);
}

uint32_t CompressedArchiveFile::Read(BSStringT<char>& /*arString*/, uint32_t /*aiMaxLength*/) {
    return 0;
}

uint32_t CompressedArchiveFile::Read(BSStringT<char16_t>& /*arString*/, uint32_t /*aiMaxLength*/) {
    return 0;
}

// Tests the pointer field itself.
bool CompressedArchiveFile::Exist() {
    return spArchive.m_pObject != nullptr;
}

void CompressedArchiveFile::Seek(int32_t aiByteCount, int32_t aiSeekFlags) {
    ReadF(0, aiByteCount);
}

uint32_t CompressedArchiveFile::ReadF(void* apReadBuffer, uint32_t aiByteCount) {
    size_t uiBytesLeft = aiByteCount;
    uint32_t uiReadBytes = 0;
    z_stream_s* pStream = static_cast<z_stream_s*>(pZStream);
    size_t uiAvail = iUncompressedBufferReadSize - iUncompressedBufferPos;
    if (aiByteCount <= uiAvail) {
        if (apReadBuffer) {
            rt::memcpy(apReadBuffer, &pUncompressedBuffer[iUncompressedBufferPos], aiByteCount);
        }
        iUncompressedBufferPos += aiByteCount;
        return aiByteCount;
    }
    else {
        if (uiAvail) {
            if (apReadBuffer) {
                rt::memcpy(apReadBuffer, &pUncompressedBuffer[iUncompressedBufferPos], uiAvail);
                apReadBuffer = static_cast<char*>(apReadBuffer) + uiAvail;
            }
            uiBytesLeft = aiByteCount - uiAvail;
            uiReadBytes = uiAvail;
        }
        iUncompressedBufferPos = 0;
        iUncompressedBufferReadSize = 0;
        while (uiBytesLeft) {
            pStream->avail_out = iUncompressedBufferSize;
            pStream->next_out = reinterpret_cast<uint8_t*>(pUncompressedBuffer);
            while (pStream->avail_out) {
                uint32_t uiBytesLeftInFile = m_uiBufferReadSize - m_uiPos;
                pStream->avail_in = uiBytesLeftInFile;
                pStream->next_in = reinterpret_cast<uint8_t*>(&m_pBuffer[m_uiPos]);
                if (uiBytesLeftInFile) {
                    int32_t eInflateError = inflate(pStream, 2);
                    if (eInflateError == -2 || (eInflateError == 2 || (eInflateError == -3 || eInflateError == -4))) {
                        inflateEnd(pStream);
                        uiBytesLeft = 0;
                        break;
                    }
                    if (eInflateError == 1) {
                        break;
                    }
                }
                m_uiPos += uiBytesLeftInFile - pStream->avail_in;
                iTrueFilePos += uiBytesLeftInFile - pStream->avail_in;
                if (!pStream->avail_in) {
                    NiFile::Flush();
                    m_uiBufferReadSize = ReadFromArchive(m_pBuffer, m_uiBufferAllocSize, 0);
                }
            }
            iUncompressedBufferReadSize = iUncompressedBufferSize - pStream->avail_out;
            if (!iUncompressedBufferReadSize) {
                break;
            }
            if (uiBytesLeft <= iUncompressedBufferReadSize) {
                if (apReadBuffer) {
                    rt::memcpy(apReadBuffer, pUncompressedBuffer, uiBytesLeft);
                }
                iUncompressedBufferPos += uiBytesLeft;
                uiReadBytes += uiBytesLeft;
                uiBytesLeft = 0;
            }
            else {
                uiBytesLeft -= iUncompressedBufferReadSize;
                uiReadBytes += iUncompressedBufferReadSize;
                if (apReadBuffer) {
                    rt::memcpy(apReadBuffer, pUncompressedBuffer, iUncompressedBufferReadSize);
                    apReadBuffer = static_cast<char*>(apReadBuffer) + iUncompressedBufferReadSize;
                }
            }
        }
    }
    return uiReadBytes;
}

uint32_t CompressedArchiveFile::WriteF(const void*, uint32_t /*aiBytes*/) {
    return 0;
}

CompressedArchiveFile::~CompressedArchiveFile() {
    if (pZStream) {
        z_stream_s* pStream = static_cast<z_stream_s*>(pZStream);
        inflateEnd(pStream);
        MemoryManager_Singleton.Deallocate(pStream);
    }
    if (pUncompressedBuffer) {
        MemoryManager_Singleton.Deallocate(pUncompressedBuffer);
    }
}

uint32_t CompressedArchiveFile::RdNoSwap(
    NiBinaryStream* apThis,
    void* apBuffer,
    uint32_t aiBytes,
    uint32_t* /*apNothing1*/,
    uint32_t /*aiNothing2*/
) {
    return static_cast<CompressedArchiveFile*>(apThis)->CompressedArchiveFile::ReadF(apBuffer, aiBytes);
}

// Copies the archive list and the master file path into the manager's buffers; a missing list becomes an empty string
// in a 0-byte allocation, written anyway, deliberately, as the game does.
void ArchiveManager::Init(
    bool abUseArchives,
    const char* apMasterFilePath,
    const char* apArchiveList,
    uint32_t aiFileBufferSize
) {
    bUseArchives = abUseArchives;
    if (apArchiveList) {
        int32_t iListSize = std::strlen(apArchiveList) + 1;
        sMasterArchiveList = static_cast<char*>(MemoryManager_Singleton.Allocate(iListSize));
        strcpy_s(sMasterArchiveList, iListSize, apArchiveList);
    }
    else {
        sMasterArchiveList = static_cast<char*>(MemoryManager_Singleton.Allocate(0));
        *sMasterArchiveList = '\0';
    }
    if (apMasterFilePath) {
        strcpy_s(sMasterFilePath, 0x104, apMasterFilePath);
    }
    else {
        strcpy_s(sMasterFilePath, 0x104, "");
    }
    iArchiveFileBufferSize = aiFileBufferSize;
}

void ArchiveManager::SetCheckRuntimeCollisions(bool abCheck) {
    bCheckRuntimeCollisions = abCheck;
}

// Copies the invalidation file name into a new buffer; without a name the old buffer stays.
void ArchiveManager::SetInvalidation(bool abInvalidate, const char* apInvalidationFile) {
    bInvalidateOlderFiles = abInvalidate;
    if (apInvalidationFile) {
        int32_t iListSize = std::strlen(apInvalidationFile) + 1;
        sArchiveInvalidationFile = static_cast<char*>(MemoryManager_Singleton.Allocate(iListSize));
        strcpy_s(sArchiveInvalidationFile, iListSize, apInvalidationFile);
    }
}

// After startup archive files read through a 128 KB buffer.
void ArchiveManager::SetStartupFinished() {
    iArchiveFileBufferSize = 0x20000;
}

// Opens apArchiveName (or the file cache's copy of it) and registers it: the base game's archive goes last, a DLC
// archive goes before the first listed archive it outranks, anything else first. A bad or disabled archive is deleted.
Archive*
ArchiveManager::OpenArchive(const char* apArchiveName, uint16_t aiForceArchiveType, bool abInvalidateOtherArchives) {
    char pcDest[260];
    char cCachedName[260];
    if (!bUseArchives) {
        return nullptr;
    }
    strcpy_s(pcDest, 260, apArchiveName);
    FileFinder::FILE_LOCATION eFound;
    if (FileFinder::pFinder) {
        // -1: any archive type.
        eFound = FileFinder::pFinder->Exist(apArchiveName, pcDest, FileFinder::SKIP_ARCHIVE, -1);
    }
    else {
        eFound = FileFinder::NOTFOUND;
    }
    if (!eFound) {
        return nullptr;
    }
    if (BSFileCache::QInstance().IsFileCached(pcDest)) {
        BSFileCache::QInstance().GetCachedFileName(pcDest, cCachedName);
    }
    else {
        strcpy_s(cCachedName, 260, pcDest);
    }
    void* pMemory = NiMemObject::operator new(sizeof(Archive));
    Archive* pArchive;
    if (pMemory) {
        pArchive = ::new (pMemory) Archive(cCachedName, 0x40000, false, abInvalidateOtherArchives);
    }
    else {
        pArchive = nullptr;
    }
    if (!pArchive || !pArchive->m_bGood || (pArchive->cArchiveFlags & Archive::ARCHIVE_FLAG_DISABLED)) {
        if (pArchive) {
            delete pArchive;
        }
        return nullptr;
    }
    if (!pArchiveList) {
        void* pNode = MemoryManager_Singleton.Allocate(sizeof(BSSimpleList<NiPointer<Archive>>));
        pArchiveList = pNode ? new (pNode) BSSimpleList<NiPointer<Archive>>() : nullptr;
    }
    int32_t uiDLCArchive = 0;
    if (apArchiveName && std::strlen(apArchiveName)) {
        if (std::strstr(apArchiveName, "Fallo")) {
            uiDLCArchive = 0;
        }
        else if (std::strstr(apArchiveName, "DeadM")) {
            uiDLCArchive = 1;
        }
        else if (std::strstr(apArchiveName, "Hones")) {
            uiDLCArchive = 2;
        }
        else if (std::strstr(apArchiveName, "Lones")) {
            uiDLCArchive = 3;
        }
        else if (std::strstr(apArchiveName, "OldWo")) {
            uiDLCArchive = 4;
        }
        else if (std::strstr(apArchiveName, "Updat")) {
            uiDLCArchive = 5;
        }
        else {
            Warning("Loading a user created or misnamed BSA");
            uiDLCArchive = 6;
        }
    }
    else {
        Warning("Loading a user created or misnamed BSA");
        uiDLCArchive = 6;
    }
    if (uiDLCArchive <= 0) {
        pArchiveList->AddTail(NiPointer<Archive>(pArchive));
    }
    else {
        BSSimpleList<NiPointer<Archive>>* pIter = pArchiveList;
        bool bAdded = false;
        while (pIter && pIter->m_item.m_pObject) {
            const char* pName = pIter->m_item.m_pObject->pFileName;
            if (pName && std::strlen(pName)) {
                if (std::strstr(pName, "Fallo")) {
                    bAdded = true;
                }
                else if (std::strstr(pName, "DeadM")) {
                    bAdded = true;
                }
                else if (std::strstr(pName, "Hones") && uiDLCArchive > 1) {
                    bAdded = true;
                }
                else if (std::strstr(pName, "Lones") && uiDLCArchive > 2) {
                    bAdded = true;
                }
                else if (std::strstr(pName, "OldWo") && uiDLCArchive > 3) {
                    bAdded = true;
                }
                else if (std::strstr(pName, "Updat")) {
                    if (uiDLCArchive > 3) {
                        bAdded = true;
                    }
                }
            }
            if (bAdded) {
                pIter->AddHead(NiPointer<Archive>(pArchive));
                break;
            }
            pIter = pIter->m_pkNext;
        }
        if (!bAdded) {
            pArchiveList->AddHead(NiPointer<Archive>(pArchive));
        }
    }
    uint16_t usForceArchiveType = aiForceArchiveType;
    if (usForceArchiveType) {
        pArchive->iArchiveType = usForceArchiveType;
    }
    return pArchive;
}

Archive* ArchiveManager::GetArchive(const char* apArchiveName) {
    Archive* m_pObject;
    if (!ArchiveManager::pArchiveList) {
        return 0;
    }
    EnterCriticalSection(&ArchiveManager::s_ArchiveListLock.CriticalSection);
    BSSimpleList<NiPointer<Archive>>* v4 = ArchiveManager::pArchiveList;
    while (1) {
        if (!v4) {
            LeaveCriticalSection(&ArchiveManager::s_ArchiveListLock.CriticalSection);
            return 0;
        }
        m_pObject = v4->m_item.m_pObject;
        if (v4->m_item.m_pObject) {
            InterlockedIncrementInt32(reinterpret_cast<int32_t*>(&m_pObject->m_uiRefCount));
        }
        if (m_pObject) {
            if (!_stricmp(m_pObject->GetDevice(), apArchiveName)) {
                break;
            }
        }
        v4 = v4->GetNext();
        if (m_pObject) {
            if (!InterlockedDecrementInt32(reinterpret_cast<int32_t*>(&m_pObject->m_uiRefCount))) {
                m_pObject->DeleteThis();
            }
        }
    }
    LeaveCriticalSection(&ArchiveManager::s_ArchiveListLock.CriticalSection);
    if (!InterlockedDecrementInt32(reinterpret_cast<int32_t*>(&m_pObject->m_uiRefCount))) {
        m_pObject->DeleteThis();
    }
    return m_pObject;
}

void ArchiveManager::ReloadArchiveFilenamesAndIndices() {
    uint8_t v0;
    BSWin32FileCache* v1;
    BSWin32FileCache* v2;
    BSWin32FileCache* singleton;
    BSSimpleList<NiPointer<Archive>>* i;
    if (ArchiveManager::pArchiveList) {
        singleton = static_cast<BSWin32FileCache*>(&BSFileCache::QInstance());
        v0 = (uint8_t)((singleton->eFileCacheState == 2) || (singleton->eFileCacheState == 3));
        if (!v0) {
            v2 = static_cast<BSWin32FileCache*>(&BSFileCache::QInstance());
            if (v2->eFileCacheState == 1) {
                v2->eFileCacheState = FILE_CACHING_PAUSED;
            }
        }
        for (i = ArchiveManager::pArchiveList; i; i = i->m_pkNext) {
            i->m_item.m_pObject->ReloadFilenamesAndIndices();
        }
        if (!v0) {
            v1 = static_cast<BSWin32FileCache*>(&BSFileCache::QInstance());
            if (v1->eFileCacheState == 2) {
                v1->eFileCacheState = FILE_CACHING_IN_PROGRESS;
            }
        }
    }
}

// Opens the file from the first open archive of its group that has it; null when none does. The list is walked
// without the list lock.
ArchiveFile* ArchiveManager::GetFile(const char* apFileName, uint32_t aiBufferSize, ARCHIVE_TYPE aeArchiveType) {
    uint32_t iDirectoryIndex = 0;
    uint32_t iFileIndex = 0;
    apFileName = TrimFileName(apFileName);
    if (aeArchiveType == ARCHIVE_TYPE_ALL) {
        aeArchiveType = GetArchiveTypeFromFileName(const_cast<char*>(apFileName));
    }
    BSHash kDirectoryHash;
    BSHash kFilenameHash;
    BSHash::MakeDirAndFile(apFileName, kDirectoryHash, kFilenameHash);
    BSSimpleList<NiPointer<Archive>>* pList = pArchiveList;
    while (pList) {
        NiPointer<Archive> spArchive = pList->m_item;
        if (spArchive && (aeArchiveType & spArchive->iArchiveType) &&
            spArchive->FindFile(kDirectoryHash, kFilenameHash, iDirectoryIndex, iFileIndex, apFileName)) {
            return spArchive->GetFile(iDirectoryIndex, iFileIndex, aiBufferSize, apFileName);
        }
        pList = pList->m_pkNext;
    }
    return nullptr;
}

// The first open archive of the file's group that has the file, or null; the list is walked under the list lock.
Archive* ArchiveManager::GetArchiveForFile(const char* apFileName, ARCHIVE_TYPE aeArchiveType) {
    if (!pArchiveList) {
        return nullptr;
    }
    apFileName = TrimFileName(apFileName);
    if (aeArchiveType == ARCHIVE_TYPE_ALL) {
        aeArchiveType = GetArchiveTypeFromFileName(const_cast<char*>(apFileName));
    }
    // Outputs of Archive::FindFile only.
    uint32_t iFileIndex;
    uint32_t iDirectoryIndex;
    BSHash kDirectoryHash;
    BSHash kFilenameHash;
    BSHash::MakeDirAndFile(apFileName, kDirectoryHash, kFilenameHash);
    EnterCriticalSection(&s_ArchiveListLock.CriticalSection);
    BSSimpleList<NiPointer<Archive>>* pList = pArchiveList;
    while (pList) {
        NiPointer<Archive> spArchive = pList->m_item;
        if (spArchive && (aeArchiveType & spArchive->iArchiveType) &&
            spArchive->FindFile(kDirectoryHash, kFilenameHash, iDirectoryIndex, iFileIndex, apFileName)) {
            LeaveCriticalSection(&s_ArchiveListLock.CriticalSection);
            // The raw pointer is taken before the local reference is released.
            return spArchive;
        }
        pList = pList->m_pkNext;
    }
    LeaveCriticalSection(&s_ArchiveListLock.CriticalSection);
    return nullptr;
}

bool ArchiveManager::FindFile(const char* apFileName, ARCHIVE_TYPE aeArchiveType) {
    return GetArchiveForFile(apFileName, aeArchiveType) != nullptr;
}

char ArchiveManager::GetLastWriteTimeForFile(char* apFileName, _FILETIME* apFiletime, ARCHIVE_TYPE aeArchiveType) {
    std::memset(apFiletime, 0, sizeof(_FILETIME));
    Archive* parchive = ArchiveManager::GetArchiveForFile(apFileName, aeArchiveType);
    if (parchive) {
        apFileName = parchive->GetDevice();
    }
    return FileFinder::PlatformGetFileTime(apFileName, *apFiletime);
}

bool ArchiveManager::InvalidateFile(const char* apFileName) {
    Archive* sparchive;
    BSHash directoryHash;
    BSHash filenameHash;
    if (!ArchiveManager::pArchiveList) {
        return 0;
    }
    const char* Str1 = ArchiveManager::TrimFileName(apFileName);
    BSHash::MakeDirAndFile(Str1, directoryHash, filenameHash);
    BSSimpleList<NiPointer<Archive>>* plist = ArchiveManager::pArchiveList;
    while (1) {
        if (!plist) {
            return 0;
        }
        sparchive = plist->m_item.m_pObject;
        if (plist->m_item.m_pObject) {
            InterlockedIncrementInt32(reinterpret_cast<int32_t*>(&sparchive->m_uiRefCount));
        }
        if (sparchive) {
            if (sparchive->InvalidateFile(directoryHash, filenameHash, Str1)) {
                break;
            }
        }
        plist = plist->GetNext();
        if (sparchive) {
            sparchive->DecRefCount();
        }
    }
    sparchive->DecRefCount();
    return 1;
}

// Opens the entry's file from the archive that holds the entry; aeArchiveTypeIndex is unused.
ArchiveFile* ArchiveManager::GetFileByFileEntry(
    ARCHIVE_TYPE_INDEX aeArchiveTypeIndex,
    BSFileEntry* apFileEntry,
    uint32_t aiBufferSize,
    const char* apFileName
) {
    ArchiveFile* pFile = nullptr;
    // Computed and never used, as the game does.
    [[maybe_unused]] bool bInSecondaryArchive = (apFileEntry->iOffset & 0x80000000) != 0;
    NiPointer<Archive> spArchive = GetArchiveForFileEntry(apFileEntry, ARCHIVE_TYPE_ALL);
    if (spArchive) {
        pFile = spArchive->GetFileByFileEntry(apFileEntry, aiBufferSize, apFileName);
    }
    return pFile;
}

// The entry of the file in the first open archive of the group that has it, or null; no list lock.
BSFileEntry* ArchiveManager::GetFileEntryForFileFromAllArchives(
    ARCHIVE_TYPE_INDEX aeArchiveTypeIndex,
    BSHash& arDirectoryHash,
    BSHash& arFileNameHash,
    const char* apFileName
) {
    BSSimpleList<NiPointer<Archive>>* pList = pArchiveList;
    while (pList) {
        NiPointer<Archive> spArchive = pList->m_item;
        if (spArchive && ((1 << aeArchiveTypeIndex) & spArchive->iArchiveType)) {
            BSFileEntry* pReturn = spArchive->GetFileEntryForFile(arDirectoryHash, arFileNameHash, apFileName);
            if (pReturn) {
                return pReturn;
            }
        }
        pList = pList->m_pkNext;
    }
    return nullptr;
}

// The open archive holding apFileEntry: first among the archives of the group (any archive for ARCHIVE_TYPE_ALL), then
// among the others. Under the list lock; for ARCHIVE_TYPE_ALL the miss returns with the lock still held, deliberately,
// as the game does.
Archive* ArchiveManager::GetArchiveForFileEntry(BSFileEntry* apFileEntry, ARCHIVE_TYPE aeArchiveType) {
    EnterCriticalSection(&s_ArchiveListLock.CriticalSection);
    BSSimpleList<NiPointer<Archive>>* pList = pArchiveList;
    while (pList) {
        NiPointer<Archive> spArchive = pList->m_item;
        if (spArchive && ((aeArchiveType & spArchive->iArchiveType) || aeArchiveType == ARCHIVE_TYPE_ALL) &&
            spArchive->IsFileEntryInArchive(apFileEntry)) {
            LeaveCriticalSection(&s_ArchiveListLock.CriticalSection);
            return spArchive;
        }
        pList = pList->m_pkNext;
    }
    if (aeArchiveType == ARCHIVE_TYPE_ALL) {
        return nullptr;
    }
    pList = pArchiveList;
    while (pList) {
        NiPointer<Archive> spArchive = pList->m_item;
        if (spArchive && !(aeArchiveType & spArchive->iArchiveType) && spArchive->IsFileEntryInArchive(apFileEntry)) {
            LeaveCriticalSection(&s_ArchiveListLock.CriticalSection);
            return spArchive;
        }
        pList = pList->m_pkNext;
    }
    LeaveCriticalSection(&s_ArchiveListLock.CriticalSection);
    return nullptr;
}

// Adds to apFileNameList the archive files matching apSearchName (wildcards allowed) that the list does not hold
// yet, each prefixed with the directory part of apBaseFilename. The hashes of the listed names are built once,
// by the first archive that has the directory, and freed here.
void ArchiveManager::AddToFileList(
    BSSimpleList<const char*>* apFileNameList,
    const char* apSearchName,
    const char* apBaseFilename,
    ARCHIVE_TYPE aeArchiveType
) {
    if (!pArchiveList) {
        return;
    }
    apSearchName = TrimFileName(apSearchName);
    char pBaseFileName[0x104];
    strcpy_s(pBaseFileName, 0x104, apBaseFilename);
    // Calls the C strrchr, not the C++ char* overload.
    char* pSlash = const_cast<char*>(std::strrchr(static_cast<const char*>(pBaseFileName), '\\'));
    if (pSlash) {
        pSlash[1] = '\0';
    }
    if (aeArchiveType == ARCHIVE_TYPE_ALL) {
        aeArchiveType = GetArchiveTypeFromFileName(const_cast<char*>(apSearchName));
    }
    BSHash** pHashArray = nullptr;
    BSHash kDirectoryHash;
    BSHash kSearchHash;
    BSHash::MakeDirAndFile(apSearchName, kDirectoryHash, kSearchHash);
    // Counts the items in place, null check of the first node included, instead of calling BSSimpleList::ItemsInList;
    // calling the member on the pointer would let the compiler drop that check.
    uint32_t iLength = 0;
    for (BSSimpleList<const char*>* pNode = apFileNameList; pNode; pNode = pNode->m_pkNext) {
        if (pNode->m_item) {
            ++iLength;
        }
    }
    uint32_t iNumFiles = iLength;
    BSSimpleList<NiPointer<Archive>>* pList = pArchiveList;
    while (pList) {
        NiPointer<Archive> spArchive = pList->m_item;
        if (spArchive && (aeArchiveType & spArchive->iArchiveType)) {
            pHashArray = AddArchiveToFileList(
                spArchive, iNumFiles, pHashArray, kDirectoryHash, kSearchHash, apFileNameList, pBaseFileName
            );
        }
        pList = pList->m_pkNext;
    }
    if (pHashArray) {
        for (uint32_t i = 0; i < iNumFiles; ++i) {
            MemoryManager_Singleton.Deallocate(pHashArray[i]);
        }
        MemoryManager_Singleton.Deallocate(pHashArray);
    }
}

// Adds the files of apArchive's directory that match arSearchHash and pass CheckValidFile, unless their hash is in
// the sorted array of the listed names. The array is built on the first call (apHashArray null) from the
// aiNumFiles list items and returned for the next archives.
BSHash** ArchiveManager::AddArchiveToFileList(
    Archive* apArchive,
    uint32_t aiNumFiles,
    BSHash** apHashArray,
    BSHash& arDirectoryHash,
    BSHash& arSearchHash,
    BSSimpleList<const char*>* apFileNameList,
    const char* apBaseFilename
) {
    EnterCriticalSection(reinterpret_cast<_RTL_CRITICAL_SECTION*>(&apArchive->ArchiveCriticalSection));
    uint32_t iDirectoryIndex;
    if (apArchive->FindDirectory(arDirectoryHash, iDirectoryIndex, nullptr)) {
        if (!apHashArray && aiNumFiles > 0) {
            // new BSHash*[aiNumFiles]: the byte count saturates to SIZE_MAX when it overflows.
            size_t iArraySize = aiNumFiles > SIZE_MAX / sizeof(BSHash*) ? SIZE_MAX : sizeof(BSHash*) * aiNumFiles;
            apHashArray = static_cast<BSHash**>(MemoryManager_Singleton.Allocate(iArraySize));
            BSSimpleList<const char*>* pSearchList = apFileNameList;
            for (uint32_t i = 0; i < aiNumFiles; ++i) {
                const char* pFilename = pSearchList->m_item;
                void* pMemory = MemoryManager_Singleton.Allocate(sizeof(BSHash));
                apHashArray[i] = pMemory ? ::new (pMemory) BSHash(pFilename, BSHash::HASH_FILENAME) : nullptr;
                pSearchList = pSearchList->m_pkNext;
            }
            QuickSortHashArray(apHashArray, 0, aiNumFiles - 1);
        }
        BSDirectoryEntry* pDirectory = &apArchive->pDirectories[iDirectoryIndex];
        for (uint32_t i = 0; i < pDirectory->iFiles; ++i) {
            BSFileEntry* pFile = &pDirectory->pFiles[i];
            if (!apArchive->CheckValidFile(pFile, false)) {
                continue;
            }
            int32_t iCompare = arSearchHash.WildCardCompare(*pFile);
            if (iCompare == 0) {
                bool bFound = false;
                if (aiNumFiles) {
                    uint32_t iLowerBound = 0;
                    uint32_t iUpperBound = aiNumFiles;
                    uint32_t iCurrent = 0;
                    uint32_t iOffset = 1;
                    int32_t iHashCompare = 0;
                    while (iOffset) {
                        iOffset = (iUpperBound - iLowerBound) >> 1;
                        iCurrent = iLowerBound + iOffset;
                        // Hash order: CRC first, then the packed character word, both unsigned.
                        const BSHash* pHash = apHashArray[iCurrent];
                        if (pFile->iCRC < pHash->iCRC ||
                            (pFile->iCRC == pHash->iCRC && pFile->GetAll() < pHash->GetAll())) {
                            iHashCompare = -1;
                        }
                        else if (
                            pFile->iCRC > pHash->iCRC ||
                            (pFile->iCRC == pHash->iCRC && pFile->GetAll() > pHash->GetAll())
                        ) {
                            iHashCompare = 1;
                        }
                        else {
                            iHashCompare = 0;
                        }
                        if (iHashCompare > 0) {
                            iLowerBound = iCurrent;
                        }
                        else if (iHashCompare < 0) {
                            iUpperBound = iCurrent;
                        }
                        else {
                            bFound = true;
                            break;
                        }
                    }
                }
                if (!bFound) {
                    char* pFileName = apArchive->GetFileString(iDirectoryIndex, i);
                    uint32_t iFoundLen = std::strlen(apBaseFilename) + std::strlen(pFileName) + 1;
                    char* pFound = static_cast<char*>(MemoryManager_Singleton.Allocate(iFoundLen));
                    strcpy_s(pFound, iFoundLen, apBaseFilename);
                    strcat_s(pFound, iFoundLen, pFileName);
                    apFileNameList->AddHead(pFound);
                }
            }
            else if (iCompare < 0) {
                break;
            }
        }
    }
    LeaveCriticalSection(reinterpret_cast<_RTL_CRITICAL_SECTION*>(&apArchive->ArchiveCriticalSection));
    return apHashArray;
}

// The group of the file from its last three characters.
ARCHIVE_TYPE ArchiveManager::GetArchiveTypeFromFileName(char* apFileName) {
    uint32_t iLength = std::strlen(apFileName);
    return GetArchiveTypeFromFileExtension(apFileName + iLength - 3);
}

// Binary search of the extension table (a leading or second-character dot is skipped); ARCHIVE_TYPE_MISC when the
// search misses. The search's upper bound is the table's 26 entries.
ARCHIVE_TYPE ArchiveManager::GetArchiveTypeFromFileExtension(char* apExtension) {
    uint32_t iLowerBound = 0;
    uint32_t iUpperBound = 26;
    uint32_t iCurrent = 0;
    uint32_t iOffset = 1;
    int32_t iCompare = 0;
    if (*apExtension == '.') {
        apExtension += 1;
    }
    else if (apExtension[1] == '.') {
        apExtension += 2;
    }
    while (iOffset) {
        iOffset = (iUpperBound - iLowerBound) >> 1;
        iCurrent = iLowerBound + iOffset;
        iCompare = _stricmp(apExtension, ArchiveTypeExtensionsMap[iCurrent].Extension);
        if (iCompare > 0) {
            iLowerBound = iCurrent;
        }
        else if (iCompare < 0) {
            iUpperBound = iCurrent;
        }
        else {
            break;
        }
    }
    if (iCompare == 0) {
        return ArchiveTypeExtensionsMap[iCurrent].eArchiveType;
    }
    return ARCHIVE_TYPE_MISC;
}

bool ArchiveManager::QInvalidateOlderFiles() {
    return ArchiveManager::bInvalidateOlderFiles;
}

bool ArchiveManager::OutputProfileStats(char* apFileName) {
    return false;
}

void ArchiveManager::SetRetainTables(int32_t aiDirString, int32_t aiFileString, int32_t aiFileOffset) {
    ArchiveManager::iRetainDirectoryStringTable = aiDirString;
    ArchiveManager::iRetainFilenameStringTable = aiFileString;
    ArchiveManager::iRetainFilenameOffsetTable = aiFileOffset;
}

bool ArchiveManager::WildCardMatch(const char* apSearchName, BSHash* apHash) {
    BSHash v3;
    v3.~BSHash();
    ::new (&v3) BSHash(apSearchName, BSHash::HASH_FILENAME);
    return (apHash->WildCardCompare(v3) == 0);
}

bool ArchiveManager::RemoveArchive(Archive* apArchive) {
    if (!ArchiveManager::pArchiveList || !apArchive) {
        return false;
    }
    EnterCriticalSection(&ArchiveManager::s_ArchiveListLock.CriticalSection);
    {
        NiPointer<Archive> spArchive(apArchive);
        ArchiveManager::pArchiveList->Remove(spArchive);
    }
    for (uint32_t i = 0; i < 9; ++i) {
        if (ArchiveManager::spMasterArchives[0][i].m_pObject == apArchive) {
            ArchiveManager::spMasterArchives[0][i] = nullptr;
        }
        if (ArchiveManager::spMasterArchives[1][i].m_pObject == apArchive) {
            ArchiveManager::spMasterArchives[1][i] = nullptr;
        }
    }
    if (ArchiveManager::spLastArchive.m_pObject == apArchive) {
        ArchiveManager::spLastArchive = nullptr;
    }
    LeaveCriticalSection(&ArchiveManager::s_ArchiveListLock.CriticalSection);
    return true;
}

// Frees the archive list, then the two invalidation hash arrays' strings and the arrays themselves, then clears the
// master archive pointers.
void ArchiveManager::CloseAll() {
    if (pArchiveList) {
        pArchiveList->RemoveAll();
        BSSimpleList<NiPointer<Archive>>* pList = pArchiveList;
        if (pList) {
            pList->~BSSimpleList<NiPointer<Archive>>();
            (&MemoryManager_Singleton)->Deallocate(pList);
        }
        pArchiveList = nullptr;
    }
    if (pInvalidateFileNameHashes) {
        for (uint32_t i = 0; i < pInvalidateFileNameHashes->iSize; ++i) {
            (&MemoryManager_Singleton)->Deallocate(pInvalidateFileNameHashes->pBuffer[i]);
        }
        delete pInvalidateFileNameHashes;
    }
    if (pInvalidateDirectoryHashes) {
        for (uint32_t i = 0; i < pInvalidateDirectoryHashes->iSize; ++i) {
            (&MemoryManager_Singleton)->Deallocate(pInvalidateDirectoryHashes->pBuffer[i]);
        }
        delete pInvalidateDirectoryHashes;
    }
    for (uint32_t i = 0; i < 9; ++i) {
        spMasterArchives[0][i] = nullptr;
        spMasterArchives[1][i] = nullptr;
    }
}

void ArchiveManager::AddArchive(Archive* apArchive) {
    EnterCriticalSection(&ArchiveManager::s_ArchiveListLock.CriticalSection);
    if (ArchiveManager::pArchiveList) {
        NiPointer<Archive> spArchive(apArchive);
        ArchiveManager::pArchiveList->AddTail(spArchive);
    }
    LeaveCriticalSection(&ArchiveManager::s_ArchiveListLock.CriticalSection);
}

void ArchiveManager::Shutdown() {
    if (ArchiveManager::sArchiveInvalidationFile) {
        MemoryManager_Singleton.Deallocate(ArchiveManager::sArchiveInvalidationFile);
    }
    if (ArchiveManager::sMasterArchiveList) {
        MemoryManager_Singleton.Deallocate(ArchiveManager::sMasterArchiveList);
    }
}

// The plugins file is never closed, as the game does.
bool ArchiveManager::OpenMasterArchives() {
    bool bAllMasterArchivesFound = true;
    if (!ArchiveManager::bUseArchives) {
        return bAllMasterArchivesFound;
    }
    ArchiveManager::LoadInvalidationFile(ArchiveManager::sArchiveInvalidationFile);
    // Selection sort of the extension table by extension, case-insensitive.
    for (uint32_t i = 0; i < 25; ++i) {
        uint32_t uiMin = i;
        for (uint32_t j = i + 1; j < 26; ++j) {
            if (_stricmp(
                    ArchiveManager::ArchiveTypeExtensionsMap[j].Extension,
                    ArchiveManager::ArchiveTypeExtensionsMap[uiMin].Extension
                ) < 0) {
                uiMin = j;
            }
        }
        if (i != uiMin) {
            ArchiveTypeExtension kTemp = ArchiveManager::ArchiveTypeExtensionsMap[i];
            ArchiveManager::ArchiveTypeExtensionsMap[i] = ArchiveManager::ArchiveTypeExtensionsMap[uiMin];
            ArchiveManager::ArchiveTypeExtensionsMap[uiMin] = kTemp;
        }
    }
    uint32_t uiStartTime = GetTickCount();
    uint32_t iStringSize = 0x8000;
    char* pString = static_cast<char*>(MemoryManager_Singleton.Allocate(iStringSize));
    strcpy_s(pString, iStringSize, ArchiveManager::sMasterArchiveList);
    char pluginsPath[260];
    BSsprintf(pluginsPath, 260, "%sPlugins.txt", "");
    std::FILE* pPlugins = nullptr;
    rt::fopen_s(&pPlugins, pluginsPath, "r");
    if (pPlugins) {
        while (!std::feof(pPlugins)) {
            char PluginName[260];
            rt::fgets(PluginName, 260, pPlugins);
            if (PluginName[0] == '#' || !PluginName[0] || PluginName[0] == '\n') {
                continue;
            }
            const char* pFound = std::strstr(PluginName, ".esp");
            if (!pFound) {
                pFound = std::strstr(PluginName, ".esm");
            }
            if (!pFound) {
                continue;
            }
            // C's strstr returns a pointer into the writable PluginName, which its declaration here types const.
            char* pExt = PluginName + (pFound - PluginName);
            strcpy_s(pExt, 260 - (pExt - PluginName), ".bsa");
            char FullPath[280];
            strcpy_s(FullPath, 260, "Data\\");
            strcat_s(FullPath, 260, PluginName);
            if (rt::_access(FullPath, 0) != -1) {
                if (pString[std::strlen(pString) - 2] == '\n') {
                    pString[std::strlen(pString) - 2] = '\0';
                }
                strcat_s(pString, 260, ", ");
                strcat_s(pString, 260, PluginName);
            }
        }
    }
    char* pStringAlloc = pString;
    char* pFileName = pString;
    strtok_s(pFileName, ",", &pString);
    while (*pFileName) {
        while (*pFileName == ' ' || *pFileName == '\t' || *pFileName == '\n') {
            ++pFileName;
        }
        Archive* pArchive = ArchiveManager::OpenArchive(pFileName, 0, 0);
        if (pArchive) {
            pArchive->IncRefCount();
        }
        if (pArchive) {
            for (uint32_t archiveType = 0; archiveType < 9; ++archiveType) {
                if ((static_cast<uint16_t>(1 << archiveType) & pArchive->iArchiveType) != 0 &&
                    !ArchiveManager::spMasterArchives[0][archiveType].m_pObject) {
                    ArchiveManager::spMasterArchives[0][archiveType].operator=(pArchive);
                    if (MasterArchiveBufferSize[archiveType] > pArchive->m_uiBufferAllocSize) {
                        pArchive->ChangeBufferSize(MasterArchiveBufferSize[archiveType]);
                    }
                }
            }
        }
        else {
            bAllMasterArchivesFound = false;
        }
        pFileName = pString;
        strtok_s(pFileName, ",", &pString);
        if (pArchive) {
            pArchive->DecRefCount();
        }
    }
    MemoryManager_Singleton.Deallocate(pStringAlloc);
    // Computes the elapsed time and never reads it, as the game does.
    uiStartTime = GetTickCount() - uiStartTime;
    (void)uiStartTime;
    return bAllMasterArchivesFound;
}

bool ArchiveManager::CloseArchive(const char* apArchiveName) {
    FileFinder::FILE_LOCATION v2;
    char acPath[268];
    if (!ArchiveManager::pArchiveList) {
        return 0;
    }
    strcpy_s(acPath, 260, apArchiveName);
    if (FileFinder::pFinder) {
        v2 = FileFinder::pFinder->Exist(apArchiveName, acPath, FileFinder::SKIP_ARCHIVE, FileFinder::ARCHIVE_TYPE_ANY);
    }
    else {
        v2 = FileFinder::NOTFOUND;
    }
    if (v2 == FileFinder::NOTFOUND) {
        return 0;
    }
    for (BSSimpleList<NiPointer<Archive>>* i = ArchiveManager::pArchiveList; i; i = i->GetNext()) {
        Archive* apArchive = i->m_item.m_pObject;
        if (i->m_item.m_pObject) {
            if (!_stricmp(apArchive->GetDevice(), acPath)) {
                ArchiveManager::RemoveArchive(apArchive);
                return 1;
            }
        }
    }
    return 0;
}

void ArchiveManager::LoadInvalidationFile(const char* apFileName) {
    if (!apFileName) {
        return;
    }
    BSFile* pFile = static_cast<BSFile*>(FileFinder::GetFile(apFileName, NiFile::READ_ONLY, 0x4000));
    if (pFile && pFile->Open(false, false)) {
        char cLineEnd;
        char cLine[264];
        while (pFile->GetLine(cLine, 260, 13)) {
            // Reads the line feed after the carriage return GetLine stops at.
            uint32_t uiComponentSize = 1;
            pFile->BinaryRead(&cLineEnd, 1, &uiComponentSize, 1);
            if (std::strstr(cLine, "\\")) {
                if (!ArchiveManager::pInvalidateDirectoryHashes) {
                    void* pMemory = MemoryManager_Singleton.Allocate(sizeof(BSSimpleArray<BSHash*, 1024>));
                    ArchiveManager::pInvalidateDirectoryHashes =
                        pMemory ? ::new (pMemory) BSSimpleArray<BSHash*, 1024> : nullptr;
                }
                char* pDirectory = cLine;
                if (cLine[0] == '\\') {
                    ++pDirectory;
                }
                void* pHashMemory = MemoryManager_Singleton.Allocate(sizeof(BSHash));
                BSHash* pHash = pHashMemory ? ::new (pHashMemory) BSHash(pDirectory, BSHash::HASH_DIRECTORY) : nullptr;
                ArchiveManager::pInvalidateDirectoryHashes->Add(pHash);
            }
            else {
                if (!ArchiveManager::pInvalidateFileNameHashes) {
                    void* pMemory = MemoryManager_Singleton.Allocate(sizeof(BSSimpleArray<BSHash*, 1024>));
                    ArchiveManager::pInvalidateFileNameHashes =
                        pMemory ? ::new (pMemory) BSSimpleArray<BSHash*, 1024> : nullptr;
                }
                void* pHashMemory = MemoryManager_Singleton.Allocate(sizeof(BSHash));
                BSHash* pHash = pHashMemory ? ::new (pHashMemory) BSHash(cLine, BSHash::HASH_FILENAME) : nullptr;
                ArchiveManager::pInvalidateFileNameHashes->Add(pHash);
            }
        }
    }
    if (pFile) {
        delete pFile;
    }
}

BSFileEntry* ArchiveManager::GetFileEntryForFileOnlyFromMasterArchives(
    ARCHIVE_TYPE_INDEX aeArchiveTypeIndex,
    BSHash& arDirectoryHash,
    BSHash& arFileNameHash,
    const char* apFileName
) {
    NiPointer<Archive>* v5;
    BSFileEntry* pentry = 0;
    if (ArchiveManager::spMasterArchives[0][aeArchiveTypeIndex].m_pObject) {
        v5 = &ArchiveManager::spMasterArchives[0][aeArchiveTypeIndex];
    }
    else {
        v5 = &ArchiveManager::spMasterArchives[1][aeArchiveTypeIndex];
    }
    Archive* m_pObject = v5->m_pObject;
    if (v5->m_pObject) {
        InterlockedIncrementInt32(reinterpret_cast<int32_t*>(&m_pObject->m_uiRefCount));
    }
    if (m_pObject) {
        pentry = m_pObject->GetFileEntryForFile(arDirectoryHash, arFileNameHash, apFileName);
        m_pObject->DecRefCount();
    }
    return pentry;
}

BSFileEntry* ArchiveManager::GetFileEntryForFile_1(
    uint16_t aeArchiveTypeIndex,
    BSHash* arDirectoryHash,
    BSHash* arFileNameHash,
    char* apFileName
) {
    BSFileEntry* FileEntryForFile;
    Archive* m_pObject;
    BSSimpleList<NiPointer<Archive>>* v7 = ArchiveManager::pArchiveList;
    while (1) {
        if (!v7) {
            return 0;
        }
        m_pObject = v7->m_item.m_pObject;
        if (v7->m_item.m_pObject) {
            InterlockedIncrementInt32(reinterpret_cast<int32_t*>(&m_pObject->m_uiRefCount));
        }
        if (m_pObject) {
            if ((aeArchiveTypeIndex & m_pObject->iArchiveType) != 0) {
                FileEntryForFile = m_pObject->GetFileEntryForFile(*arDirectoryHash, *arFileNameHash, apFileName);
                if (FileEntryForFile) {
                    break;
                }
            }
        }
        v7 = v7->GetNext();
        if (m_pObject) {
            m_pObject->DecRefCount();
        }
    }
    m_pObject->DecRefCount();
    return FileEntryForFile;
}

bool ArchiveManager::GetFileNameForFileEntry(
    ARCHIVE_TYPE_INDEX aeArchiveTypeIndex,
    BSFileEntry* apFileEntry,
    char* apFileName
) {
    bool bResult = 0;
    Archive* spArchive =
        ArchiveManager::spMasterArchives[(apFileEntry->iOffset & 0x80000000u) != 0][aeArchiveTypeIndex].m_pObject;
    if (spArchive) {
        InterlockedIncrementInt32(reinterpret_cast<int32_t*>(&spArchive->m_uiRefCount));
        EnterCriticalSection(&spArchive->ArchiveCriticalSection.CriticalSection);
        const char* pcSrc = spArchive->GetFileNameForFileEntry(apFileEntry);
        if (pcSrc) {
            strcpy_s(apFileName, 260, pcSrc);
            bResult = 1;
        }
        LeaveCriticalSection(&spArchive->ArchiveCriticalSection.CriticalSection);
    }
    if (spArchive) {
        spArchive->DecRefCount();
    }
    return bResult;
}

bool ArchiveManager::GetRandomFileNameForDirectory(
    const char* apDirectory,
    char* apFileName,
    ARCHIVE_TYPE aeArchiveType
) {
    uint32_t iDirectoryIndex = 0;
    apDirectory = ArchiveManager::TrimFileName(apDirectory);
    uint32_t iLength = std::strlen(apDirectory);
    if (aeArchiveType == ARCHIVE_TYPE_ALL && apDirectory[iLength - 1] != '\\') {
        aeArchiveType = GetArchiveTypeFromFileName(const_cast<char*>(apDirectory));
    }
    BSHash directoryHash(apDirectory, BSHash::HASH_DIRECTORY);
    BSDirectoryEntry* pDirectory = nullptr;
    NiPointer<Archive> spFoundArchive;
    BSSimpleList<NiPointer<Archive>>* pList = ArchiveManager::pArchiveList;
    while (pList) {
        NiPointer<Archive> spArchive = pList->m_item;
        if (spArchive && (aeArchiveType & spArchive->iArchiveType) != 0 &&
            spArchive->FindDirectory(directoryHash, iDirectoryIndex, 0)) {
            spFoundArchive.SetPointer_(&spArchive);
            pDirectory = &spFoundArchive->pDirectories[iDirectoryIndex];
            break;
        }
        pList = pList->GetNext();
    }
    if (spFoundArchive && pDirectory && pDirectory->iFiles) {
        bool bUseCriticalSection = (spFoundArchive->cArchiveFlags & Archive::ARCHIVE_FLAG_HAS_FILE_STRINGS) == 0;
        if (bUseCriticalSection) {
            EnterCriticalSection(&spFoundArchive->ArchiveCriticalSection.CriticalSection);
        }
        uint32_t uiMax = pDirectory->iFiles;
        if ((Singleton_guard & 1) == 0) {
            Singleton_guard |= 1u;
            BSRandom::Singleton.~BSRandom();
            ::new (&BSRandom::Singleton) BSRandom;
        }
        uint32_t uiFileIndex = BSRandom::Singleton.UnsignedInt(uiMax);
        char* pString = spFoundArchive->GetFileString(iDirectoryIndex, uiFileIndex);
        strcpy_s(apFileName, 260, apDirectory);
        // The C form of strrchr returns a non-const pointer into apFileName.
        char* pSlash = const_cast<char*>(std::strrchr(static_cast<const char*>(apFileName), '\\'));
        if (pSlash) {
            strcpy_s(pSlash + 1, static_cast<size_t>(260 - (pSlash - apFileName) - 1), pString);
            if (bUseCriticalSection) {
                LeaveCriticalSection(&spFoundArchive->ArchiveCriticalSection.CriticalSection);
            }
            return true;
        }
        // Returns here without leaving the critical section, deliberately, as the game does.
        return false;
    }
    return false;
}

bool ArchiveManager::GetFileNameForSmallestFileInDirectory(
    const char* apDirectory,
    char* apFileName,
    ARCHIVE_TYPE aeArchiveType
) {
    uint32_t idirectoryIndex = 0;
    apDirectory = TrimFileName(apDirectory);
    uint32_t ilength = std::strlen(apDirectory);
    if (aeArchiveType == ARCHIVE_TYPE_ALL && apDirectory[ilength - 1] != '\\') {
        aeArchiveType = GetArchiveTypeFromFileName(const_cast<char*>(apDirectory));
    }
    BSHash directoryHash(apDirectory, BSHash::HASH_DIRECTORY);
    BSDirectoryEntry* sptempLastArchive = nullptr;
    NiPointer<Archive> spLastArchive = nullptr;
    if (!spLastArchive.m_pObject) {
        BSSimpleList<NiPointer<Archive>>* plist = pArchiveList;
        while (plist) {
            NiPointer<Archive> sparchive = plist->m_item;
            if (sparchive.m_pObject &&
                (static_cast<uint16_t>(aeArchiveType) & sparchive.m_pObject->iArchiveType) != 0 &&
                sparchive.m_pObject->FindDirectory(directoryHash, idirectoryIndex, 0)) {
                spLastArchive.SetPointer_(&sparchive);
                sptempLastArchive = &spLastArchive.m_pObject->pDirectories[idirectoryIndex];
                break;
            }
            plist = plist->GetNext();
        }
    }
    if (spLastArchive.m_pObject && sptempLastArchive && sptempLastArchive->iFiles) {
        bool buseCriticalSection = true;
        if (spLastArchive.m_pObject->cArchiveFlags & Archive::ARCHIVE_FLAG_HAS_FILE_STRINGS) {
            buseCriticalSection = false;
        }
        if (buseCriticalSection) {
            EnterCriticalSection(&spLastArchive.m_pObject->ArchiveCriticalSection.CriticalSection);
        }
        uint32_t ismallestFileIndex = 0;
        uint32_t ismallestFileSize = 0xFFFFFFFF;
        // The top two bits of a file's size are flags.
        for (uint32_t i = 0; i < sptempLastArchive->iFiles; ++i) {
            uint32_t iSize = sptempLastArchive->pFiles[i].iSize & 0x3FFFFFFF;
            if (iSize < ismallestFileSize) {
                ismallestFileSize = iSize;
                ismallestFileIndex = i;
            }
        }
        char* pstring = spLastArchive.m_pObject->GetFileString(idirectoryIndex, ismallestFileIndex);
        strcpy_s(apFileName, 260, apDirectory);
        // Calls the C strrchr, not the C++ char* overload.
        char* pslash = const_cast<char*>(std::strrchr(static_cast<const char*>(apFileName), '\\'));
        if (!pslash) {
            // Returns here with the archive's critical section still entered, as the game does.
            return false;
        }
        strcpy_s(pslash + 1, static_cast<size_t>(260 - (pslash - apFileName) - 1), pstring);
        if (buseCriticalSection) {
            LeaveCriticalSection(&spLastArchive.m_pObject->ArchiveCriticalSection.CriticalSection);
        }
        return true;
    }
    return false;
}

void ArchiveManager::InvalidateFileInArchives(BSHash& arDirectoryHash, BSHash& arFileHash, uint32_t aiArchiveTypes) {
    EnterCriticalSection(&ArchiveManager::s_ArchiveListLock.CriticalSection);
    BSSimpleList<NiPointer<Archive>>* kIter = ArchiveManager::pArchiveList;
    while (kIter) {
        if (!kIter->GetNext() && !kIter->m_item.m_pObject) {
            break;
        }
        Archive* pArchive = kIter->m_item.m_pObject;
        if (kIter->m_item.m_pObject) {
            InterlockedIncrementInt32(reinterpret_cast<int32_t*>(&pArchive->m_uiRefCount));
        }
        if ((static_cast<uint16_t>(aiArchiveTypes) & pArchive->iArchiveType) != 0) {
            pArchive->InvalidateFile(arDirectoryHash, arFileHash, 0);
        }
        kIter = kIter->GetNext();
        if (pArchive) {
            pArchive->DecRefCount();
        }
    }
    LeaveCriticalSection(&ArchiveManager::s_ArchiveListLock.CriticalSection);
}

char ArchiveManager::CloseArchive(Archive* a1) {
    if ((!ArchiveManager::pArchiveList) || (!a1)) {
        return 0;
    }
    ArchiveManager::RemoveArchive(a1);
    return 1;
}

void ArchiveManager::CleanupArchiveStringsAfterStartup() {
    ;
}

uint32_t BSstrlen(const char* apString) {
    return std::strlen(apString);
}
