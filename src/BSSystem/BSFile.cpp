#include "BSSystem/BSFile.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/string.h"
#include "platform/Paths.h"

#include <vector>

// The read function SetEndianSwap installs on a BSFile when it does not swap: reads through the file's ReadF.
uint32_t BSFile::RdNoSwap(
    NiBinaryStream* pThis,
    void* pvBuffer,
    uint32_t uiBytes,
    uint32_t* puiComponentSizes,
    uint32_t uiNumComponents
) {
    return static_cast<BSFile*>(pThis)->ReadF(pvBuffer, uiBytes);
}

// The read function SetEndianSwap installs on a BSFile: reads through the file's ReadF, then swaps.
uint32_t BSFile::RdSwap(
    NiBinaryStream* pThis,
    void* pvBuffer,
    uint32_t uiBytes,
    uint32_t* puiComponentSizes,
    uint32_t uiNumComponents
) {
    if (!uiBytes) {
        return 0;
    }
    uint32_t uiBytesRead = static_cast<BSFile*>(pThis)->ReadF(pvBuffer, uiBytes);
    if (puiComponentSizes) {
        NiBinaryStream::DoByteSwap(pvBuffer, uiBytes, puiComponentSizes, uiNumComponents);
    }
    return uiBytesRead;
}

// The write function SetEndianSwap installs on a BSFile when it swaps: swaps a copy, then writes it through the file's
// WriteF.
uint32_t BSFile::WrSwap(
    NiBinaryStream* apThis,
    const void* apvBuffer,
    uint32_t auiBytes,
    uint32_t* apuiComponentSizes,
    uint32_t auiNumComponents
) {
    if (!auiBytes) {
        return 0;
    }
    void* pvDest = MemoryManager::Instance().Allocate(auiBytes);
    rt::memcpy(pvDest, apvBuffer, auiBytes);
    if (apuiComponentSizes) {
        NiBinaryStream::DoByteSwap(pvDest, auiBytes, apuiComponentSizes, auiNumComponents);
    }
    uint32_t uiWritten = static_cast<BSFile*>(apThis)->WriteF(pvDest, auiBytes);
    MemoryManager::Instance().Deallocate(pvDest);
    return uiWritten;
}

// Every member is cleared except iFileSize, which is deliberately left unset. NiFile() clears the
// buffer and file pointers.
BSFile::BSFile()
  : NiFile(), bUseAuxBuffer(false), m_pAuxBuffer(nullptr), iAuxTrueFilePos(-1), iAuxBufferMinIndex(0),
    iAuxBufferMaxIndex(0), iResult(0), iIOSize(0), iTrueFilePos(0) {
    m_eMode = NiFile::READ_ONLY;
    m_uiBufferAllocSize = 0;
    m_uiBufferReadSize = 0;
    m_uiPos = 0;
    pFileName[0] = '\0';
    m_bGood = false;
}

// NiFile() (assumed native) clears the buffer and file pointers; the mode and buffer sizes are BSFile's.
BSFile::BSFile(const char* apName, NiFile::OpenMode aeMode, uint32_t aiBufferSize, bool abTextMode)
  : NiFile(), bUseAuxBuffer(false), m_pAuxBuffer(nullptr), iAuxTrueFilePos(-1), iAuxBufferMinIndex(0),
    iAuxBufferMaxIndex(0), iResult(0), iIOSize(0), iTrueFilePos(0), iFileSize(0) {
    BSFile::SetEndianSwap(false);
    m_eMode = aeMode;
    m_uiBufferAllocSize = aiBufferSize;
    m_uiBufferReadSize = 0;
    m_uiPos = 0;
    if (std::strlen(apName) < 260) {
        strcpy_s(pFileName, 260, apName);
        if (aeMode == NiFile::WRITE_ONLY) {
            BSFile::Open(false, abTextMode);
        }
    }
    else {
        pFileName[0] = 0;
    }
    if (aeMode == NiFile::READ_ONLY) {
        m_bGood = BSFile::Exist();
    }
}

BSFile::~BSFile() {
    Close();
}

// Picks the swapping or plain read and write callbacks.
void BSFile::SetEndianSwap(bool bDoSwap) {
    if (bDoSwap) {
        m_pfnRead = &BSFile::RdSwap;
        m_pfnWrite = &BSFile::WrSwap;
    }
    else {
        m_pfnRead = &BSFile::RdNoSwap;
        m_pfnWrite = &BSFile::WrNoSwap;
    }
}

uint32_t BSFile::GetFileSize() const {
    // GetFileSize is const in NiFile's interface; GetSize caches the length in iFileSize.
    if (!iFileSize) {
        const_cast<BSFile*>(this)->GetSize();
    }
    return iFileSize;
}

bool BSFile::Open(bool abWaitForOpen, bool abTextMode) {
    const char* pMode;
    if (m_pFile) {
        return true;
    }
    m_uiPos = 0;
    m_uiBufferReadSize = 0;
    m_pBuffer = nullptr;
    NiFile::OpenMode eMode = m_eMode;
    if (eMode) {
        if (eMode == NiFile::WRITE_ONLY) {
            const char* pWriteMode;
            if (abTextMode) {
                pWriteMode = "r+t";
            }
            else {
                pWriteMode = "r+b";
            }
            pMode = pWriteMode;
        }
        else {
            const char* pAppendMode;
            if (abTextMode) {
                pAppendMode = "a+t";
            }
            else {
                pAppendMode = "a+b";
            }
            pMode = pAppendMode;
        }
    }
    else {
        const char* pReadMode;
        if (abTextMode) {
            pReadMode = "rt";
        }
        else {
            pReadMode = "rb";
        }
        pMode = pReadMode;
    }
    m_pFile = nullptr;
    rt::fopen_s(&m_pFile, pFileName, pMode);
    if (!m_pFile && m_eMode == NiFile::WRITE_ONLY) {
        m_pFile = nullptr;
        if (abTextMode) {
            rt::fopen_s(&m_pFile, pFileName, "wt");
        }
        else {
            rt::fopen_s(&m_pFile, pFileName, "wb");
        }
        if (m_pFile) {
            std::fclose(m_pFile);
        }
        m_pFile = nullptr;
        rt::fopen_s(&m_pFile, pFileName, pMode);
    }
    CheckIsGood();
    return m_bGood;
}

bool BSFile::OpenByFilePointer(std::FILE* apFile) {
    if (m_pFile) {
        return true;
    }
    m_uiPos = 0;
    m_uiBufferReadSize = 0;
    m_pBuffer = nullptr;
    m_pFile = apFile;
    CheckIsGood();
    return m_bGood;
}

uint32_t BSFile::Read(BSStringT<char16_t>& arString, uint32_t aiMaxLength) {
    char16_t wChar;
    if (!m_pFile) {
        Open(false, false);
    }
    uint32_t ibuffer = 0;
    uint32_t itotal = 0;
    uint32_t ibytesRead = 0;
    while (true) {
        char16_t String[260];
        do {
            uint32_t uiComponentSizes[2];
            uiComponentSizes[0] = 1;
            uint32_t uiRead = m_pfnRead(this, &wChar, 2, uiComponentSizes, 1);
            m_uiAbsoluteCurrentPos += uiRead;
            uiComponentSizes[1] = uiRead;
            ibytesRead += uiRead;
            if (uiRead != 2) {
                wChar = 0;
            }
            if (ibytesRead > aiMaxLength) {
                wChar = 0;
            }
            String[ibuffer++] = wChar;
        }
        while (ibuffer != 260 && wChar);
        if (itotal) {
            arString += static_cast<const char16_t*>(String);
        }
        else {
            arString.Set(String, 0);
        }
        itotal += ibuffer;
        if (!wChar) {
            break;
        }
        ibuffer = 0;
    }
    return ibytesRead >> 1;
}

uint32_t BSFile::Read(BSStringT<char>& arString, uint32_t aiMaxLength) {
    char cChar;
    if (!m_pFile) {
        Open(false, false);
    }
    int32_t iIndex = 0;
    int32_t iTotal = 0;
    int32_t result = 0;
    while (true) {
        char apString[264];
        do {
            int32_t iComponentSizes[2];
            iComponentSizes[0] = 1;
            uint32_t uiRead = m_pfnRead(this, &cChar, 1, reinterpret_cast<uint32_t*>(iComponentSizes), 1);
            m_uiAbsoluteCurrentPos += uiRead;
            iComponentSizes[1] = uiRead;
            result += uiRead;
            if (uiRead != 1) {
                cChar = 0;
            }
            if (result > aiMaxLength) {
                cChar = 0;
            }
            apString[iIndex++] = cChar;
        }
        while (iIndex != 259 && cChar);
        apString[iIndex] = 0;
        if (iTotal) {
            arString += apString;
        }
        else {
            arString.Set(apString, 0u);
        }
        iTotal += iIndex;
        if (!cChar) {
            break;
        }
        iIndex = 0;
    }
    return result;
}

uint32_t BSFile::GetLine(char* pBuffer, uint32_t uiMaxBytes, wchar_t cMark) {
    char cChar;
    int32_t i;
    uint32_t uiLine = 0;
    for (i = 0; i + 1 < uiMaxBytes; ++i) {
        uint32_t uiOffset = ReadF(&cChar, 1u);
        uiLine += uiOffset;
        if (uiOffset != 1 || cChar == cMark) {
            break;
        }
        pBuffer[i] = cChar;
    }
    pBuffer[i] = 0;
    return uiLine;
}

uint32_t BSFile::Write(const BSStringT<char16_t>& arString, bool abBinary) {
    size_t sLen;
    int32_t iComponentSize;
    if (arString.sLen == 0xFFFF) {
        char16_t* String = arString.pString;
        sLen = rt::wcslen(String);
    }
    else {
        sLen = arString.sLen;
    }
    char16_t* pString = arString.pString;
    iComponentSize = 1;
    uint32_t uiBytesWritten = m_pfnWrite(
        this,
        pString,
        2 * (sLen + abBinary),
        reinterpret_cast<uint32_t*>(&iComponentSize),
        1
    );
    m_uiAbsoluteCurrentPos += uiBytesWritten;
    iTrueFilePos += uiBytesWritten;
    return uiBytesWritten >> 1;
}

uint32_t BSFile::Write(const BSStringT<char>& arString, bool abBinary) {
    size_t sLen;
    int32_t iComponentSize;
    if (arString.sLen == 0xFFFF) {
        char* Str = arString.pString;
        sLen = std::strlen(Str);
    }
    else {
        sLen = arString.sLen;
    }
    char* pString = arString.pString;
    iComponentSize = 1;
    uint32_t
        ibytesWritten = m_pfnWrite(this, pString, sLen + abBinary, reinterpret_cast<uint32_t*>(&iComponentSize), 1);
    m_uiAbsoluteCurrentPos += ibytesWritten;
    iTrueFilePos += ibytesWritten;
    return ibytesWritten;
}

bool BSFile::Exist() {
    return rt::_access(pFileName, 0) != -1;
}

uint32_t BSFile::WriteF(const void* apBuffer, uint32_t aiBytes) {
    uint32_t result = FileWrite(apBuffer, aiBytes);
    iTrueFilePos += result;
    return result;
}

int32_t BSFile::WriteHere(const void* apBuffer, uint32_t auiBytes) {
    uint32_t uiSize = 1;
    return BinaryWrite(apBuffer, auiBytes, &uiSize, 1u);
}

void BSFile::CheckIsGood() {
    uint8_t bEmpty;
    if (m_pFile) {
        m_bGood = true;
        if (m_uiBufferAllocSize && (!m_pBuffer)) {
            AutoMemContext kMemContext(MEM_CONTEXT_BSFILE);
            bEmpty = m_uiBufferAllocSize == (-1);
            if (m_uiBufferAllocSize == 0xFFFFFFFFu) {
                m_uiBufferAllocSize = GetFileSize();
            }
            m_pBuffer = static_cast<char*>(MemoryManager::Instance().Allocate(m_uiBufferAllocSize));
            if (bEmpty) {
                m_uiBufferReadSize = m_uiBufferAllocSize;
                m_uiPos = 0;
                if (NiFile::DiskRead(m_pBuffer, m_uiBufferAllocSize) != m_uiBufferAllocSize) {
                    m_bGood = false;
                }
            }
        }
    }
    else {
        m_bGood = false;
    }
}

uint32_t BSFile::QTrueFilePos() {
    return iTrueFilePos;
}

// Closes the open file, releases its buffer, and clears both pointers.
void BSFile::Close() {
    if (m_bGood && m_pFile) {
        NiFile::Flush();
        std::fclose(m_pFile);
    }

    MemoryManager::Instance().Deallocate(m_pBuffer);
    m_pBuffer = nullptr;
    m_pFile = nullptr;
}

char BSFile::Flush() {
    return NiFile::Flush();
}

bool BSFile::ChangeBufferSize(uint32_t aiNewSize) {
    uint32_t uiMaxSize;
    bool bResult = true;
    uint32_t uiCurrentSize = GetSize();
    if (uiCurrentSize >= 0x4000) {
        uiMaxSize = uiCurrentSize;
    }
    else {
        uiMaxSize = 0x4000;
    }
    if (aiNewSize > uiMaxSize) {
        aiNewSize = uiMaxSize;
    }
    if (aiNewSize == m_uiBufferAllocSize) {
        return true;
    }
    AutoMemContext kMemContext(MEM_CONTEXT_BSFILE);
    NiFile::Seek(0, NiFile::ms_iSeekSet);
    NiFile::Flush();
    MemoryManager::Instance().Deallocate(m_pBuffer);
    m_uiBufferAllocSize = aiNewSize;
    m_pBuffer = static_cast<char*>(MemoryManager::Instance().Allocate(m_uiBufferAllocSize));
    if (!m_pBuffer) {
        m_uiBufferAllocSize = uiTESStandardBufferSize;
        m_pBuffer = static_cast<char*>(MemoryManager::Instance().Allocate(m_uiBufferAllocSize));
        bResult = false;
    }
    iTrueFilePos = 0;
    return bResult;
}

_SYSTEMTIME* BSFile::GetLastSaveTime(_SYSTEMTIME* result) {
    _SYSTEMTIME LocalTime;
    _SYSTEMTIME SystemTime;
    _FILETIME FileTime;
    if (m_pFile) {
        // Searches for the file's name and takes the first match's last write time.
        const std::vector<platform::FileEntry> kFound = platform::FindFiles(GetDevice());
        if (!kFound.empty()) {
            FileTime.dwLowDateTime = static_cast<uint32_t>(kFound[0].writeTime);
            FileTime.dwHighDateTime = static_cast<uint32_t>(kFound[0].writeTime >> 32);
        }
        else {
            // Zeroed like a file that is not open (a departure): with no match nothing writes the file time; the exe
            // converts it unset.
            std::memset(&LocalTime, 0, sizeof(LocalTime));
            *result = LocalTime;
            return result;
        }
        FileTimeToSystemTime(&FileTime, &SystemTime);
        SystemTimeToTzSpecificLocalTime(nullptr, &SystemTime, &LocalTime);
        *result = LocalTime;
        return result;
    }

    std::memset(&LocalTime, 0, sizeof(LocalTime));
    *result = LocalTime;
    return result;
}

void BSFile::operator delete(void* apMemory) {
    MemoryManager::Instance().Deallocate(apMemory);
}

uint32_t BSFile::WrNoSwap(
    NiBinaryStream* apThis,
    const void* apBuffer,
    uint32_t aiBytes,
    uint32_t* /*apNothing1*/,
    uint32_t /*aiNothing2*/
) {
    return static_cast<BSFile*>(apThis)->WriteF(apBuffer, aiBytes);
}

// Reads a 16-bit value into arVal and records the byte count.
BSFile& BSFile::operator>>(int16_t& arVal) {
    iResult = NiBinaryStream::Read(&arVal, 2u);
    return *this;
}

// Reads a 32-bit value into arVal and records the byte count.
BSFile& BSFile::operator>>(uint32_t& arVal) {
    iResult = NiBinaryStream::Read(&arVal, 4u);
    return *this;
}
