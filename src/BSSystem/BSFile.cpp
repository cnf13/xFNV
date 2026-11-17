#include "BSSystem/BSFile.h"

#include "BSCore/BSStringT.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiBinaryStream.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/stdio.h"
#include "SDK/MSVC/string.h"
#include "platform/Paths.h"

#include <vector>

// The read function SetEndianSwap installs on a BSFile when it does not swap: reads through the file's ReadF.
uint32_t BSFile_RdNoSwap(
    NiBinaryStream* pThis,
    void* pvBuffer,
    uint32_t uiBytes,
    uint32_t* puiComponentSizes,
    uint32_t uiNumComponents
) {
    return static_cast<BSFile*>(pThis)->ReadF(pvBuffer, uiBytes);
}

// The read function SetEndianSwap installs on a BSFile: reads through the file's ReadF, then swaps.
uint32_t BSFile_ReadAndSwap(
    NiBinaryStream* pThis,
    void* pvBuffer,
    uint32_t uiBytes,
    uint32_t* puiComponentSizes,
    uint32_t uiNumComponents
) {
    if (!uiBytes) {
        return 0;
    }
    uint32_t v6 = static_cast<BSFile*>(pThis)->ReadF(pvBuffer, uiBytes);
    if (puiComponentSizes) {
        NiBinaryStream::DoByteSwap(pvBuffer, uiBytes, puiComponentSizes, uiNumComponents);
    }
    return v6;
}

// The write function SetEndianSwap installs on a BSFile when it swaps: swaps a copy, then writes it through the file's
// WriteF.
uint32_t BSFile_WriteAndSwap(
    NiBinaryStream* apThis,
    const void* apvBuffer,
    uint32_t auiBytes,
    uint32_t* apuiComponentSizes,
    uint32_t auiNumComponents
) {
    if (!auiBytes) {
        return 0;
    }
    void* pvDest = MemoryManager_Singleton.Allocate(auiBytes);
    rt::memcpy(pvDest, apvBuffer, auiBytes);
    if (apuiComponentSizes) {
        NiBinaryStream::DoByteSwap(pvDest, auiBytes, apuiComponentSizes, auiNumComponents);
    }
    uint32_t uiWritten = static_cast<BSFile*>(apThis)->WriteF(pvDest, auiBytes);
    MemoryManager_Singleton.Deallocate(pvDest);
    return uiWritten;
}

// Every member is cleared except iFileSize, which is deliberately left unset, as the game does. NiFile() clears the
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
        m_pfnRead = &BSFile_ReadAndSwap;
        m_pfnWrite = &BSFile_WriteAndSwap;
    }
    else {
        m_pfnRead = &BSFile_RdNoSwap;
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
        return 1;
    }
    m_uiPos = 0;
    m_uiBufferReadSize = 0;
    m_pBuffer = 0;
    NiFile::OpenMode eMode = m_eMode;
    if (eMode) {
        if (eMode == NiFile::WRITE_ONLY) {
            const char* v5;
            if (abTextMode) {
                v5 = "r+t";
            }
            else {
                v5 = "r+b";
            }
            pMode = v5;
        }
        else {
            const char* v4;
            if (abTextMode) {
                v4 = "a+t";
            }
            else {
                v4 = "a+b";
            }
            pMode = v4;
        }
    }
    else {
        const char* v6;
        if (abTextMode) {
            v6 = "rt";
        }
        else {
            v6 = "rb";
        }
        pMode = v6;
    }
    m_pFile = 0;
    rt::fopen_s(&m_pFile, pFileName, pMode);
    if (!m_pFile && m_eMode == NiFile::WRITE_ONLY) {
        m_pFile = 0;
        if (abTextMode) {
            rt::fopen_s(&m_pFile, pFileName, "wt");
        }
        else {
            rt::fopen_s(&m_pFile, pFileName, "wb");
        }
        if (m_pFile) {
            std::fclose(m_pFile);
        }
        m_pFile = 0;
        rt::fopen_s(&m_pFile, pFileName, pMode);
    }
    CheckIsGood();
    return m_bGood;
}

bool BSFile::OpenByFilePointer(std::FILE* apFile) {
    if (m_pFile) {
        return (bool)1;
    }
    m_uiPos = (uint32_t)0;
    m_uiBufferReadSize = (uint32_t)0;
    m_pBuffer = nullptr;
    m_pFile = static_cast<std::FILE*>(apFile);
    CheckIsGood();
    return (bool)((uint8_t)m_bGood);
}

uint32_t BSFile::Read(BSStringT<char16_t>& arString, uint32_t aiMaxLength) {
    char16_t v7;
    if (!m_pFile) {
        Open(0, 0);
    }
    uint32_t ibuffer = 0;
    uint32_t itotal = 0;
    uint32_t ibytesRead = 0;
    while (1) {
        char16_t String[260];
        do {
            uint32_t v6[2];
            v6[0] = 1;
            uint32_t v5 = m_pfnRead(this, &v7, 2, v6, 1);
            m_uiAbsoluteCurrentPos += v5;
            v6[1] = v5;
            ibytesRead += v5;
            if (v5 != 2) {
                v7 = 0;
            }
            if (ibytesRead > aiMaxLength) {
                v7 = 0;
            }
            String[ibuffer++] = v7;
        }
        while (ibuffer != 260 && v7);
        if (itotal) {
            arString.operator+=(static_cast<const char16_t*>(String));
        }
        else {
            arString.Set(String, 0);
        }
        itotal += ibuffer;
        if (!v7) {
            break;
        }
        ibuffer = 0;
    }
    return ibytesRead >> 1;
}

uint32_t BSFile::Read(BSStringT<char>& arString, uint32_t aiMaxLength) {
    char v7;
    if (!m_pFile) {
        Open(0, 0);
    }
    int32_t v11 = 0;
    int32_t v9 = 0;
    int32_t result = 0;
    while (1) {
        char apString[264];
        do {
            int32_t v6[2];
            v6[0] = 1;
            uint32_t v5 = m_pfnRead(this, &v7, 1, reinterpret_cast<uint32_t*>(v6), 1);
            m_uiAbsoluteCurrentPos += v5;
            v6[1] = v5;
            result += v5;
            if (v5 != 1) {
                v7 = 0;
            }
            if (result > aiMaxLength) {
                v7 = 0;
            }
            apString[v11++] = v7;
        }
        while (v11 != 259 && v7);
        apString[v11] = 0;
        if (v9) {
            arString.operator+=(apString);
        }
        else {
            arString.Set(apString, 0u);
        }
        v9 += v11;
        if (!v7) {
            break;
        }
        v11 = 0;
    }
    return result;
}

uint32_t BSFile::GetLine(char* pBuffer, uint32_t uiMaxBytes, wchar_t cMark) {
    char v6;
    int32_t i;
    uint32_t uiLine = 0;
    for (i = 0; i + 1 < uiMaxBytes; ++i) {
        uint32_t uiOffset = ReadF(&v6, 1u);
        uiLine += uiOffset;
        if (uiOffset != 1 || v6 == cMark) {
            break;
        }
        pBuffer[i] = v6;
    }
    pBuffer[i] = 0;
    return uiLine;
}

uint32_t BSFile::Write(const BSStringT<char16_t>& arString, bool abBinary) {
    size_t sLen;
    int32_t v7;
    if (arString.sLen == 0xFFFF) {
        char16_t* String = arString.pString;
        sLen = rt::wcslen(String);
    }
    else {
        sLen = arString.sLen;
    }
    char16_t* pString = arString.pString;
    v7 = 1;
    uint32_t v3 = m_pfnWrite(this, pString, 2 * (sLen + abBinary), reinterpret_cast<uint32_t*>(&v7), 1);
    m_uiAbsoluteCurrentPos += v3;
    iTrueFilePos += v3;
    return v3 >> 1;
}

uint32_t BSFile::Write(const BSStringT<char>& arString, bool abBinary) {
    size_t sLen;
    int32_t v7;
    if (arString.sLen == 0xFFFF) {
        char* Str = arString.pString;
        sLen = std::strlen(Str);
    }
    else {
        sLen = arString.sLen;
    }
    char* pString = arString.pString;
    v7 = 1;
    uint32_t ibytesWritten = m_pfnWrite(this, pString, sLen + abBinary, reinterpret_cast<uint32_t*>(&v7), 1);
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
    int32_t eMemContext;
    if (m_pFile) {
        m_bGood = (bool)1;
        if (m_uiBufferAllocSize) {
            if (!m_pBuffer) {
                eMemContext = GetThreadMemContext();
                GetThreadMemContext() = 23;
                bEmpty = (uint8_t)(m_uiBufferAllocSize == ((-1)));
                if (m_uiBufferAllocSize == 0xFFFFFFFFu) {
                    m_uiBufferAllocSize = GetFileSize();
                }
                m_pBuffer = static_cast<char*>(
                    static_cast<void*>(static_cast<int8_t*>((&MemoryManager_Singleton)->Allocate(m_uiBufferAllocSize)))
                );
                if (bEmpty) {
                    m_uiBufferReadSize = m_uiBufferAllocSize;
                    m_uiPos = (uint32_t)0;
                    if (((uint32_t)NiFile::DiskRead(m_pBuffer, m_uiBufferAllocSize)) != m_uiBufferAllocSize) {
                        m_bGood = (bool)0;
                    }
                }
                GetThreadMemContext() = eMemContext;
            }
        }
    }
    else {
        m_bGood = (bool)0;
    }
}

uint32_t BSFile::QTrueFilePos() {
    return (uint32_t)((int32_t)iTrueFilePos);
}

// Closes the open file, releases its buffer, and clears both pointers.
void BSFile::Close() {
    if (m_bGood && m_pFile) {
        NiFile::Flush();
        std::fclose(m_pFile);
    }

    MemoryManager_Singleton.Deallocate(m_pBuffer);
    m_pBuffer = nullptr;
    m_pFile = nullptr;
}

char BSFile::Flush() {
    return (int8_t)((uint8_t)NiFile::Flush());
}

bool BSFile::ChangeBufferSize(uint32_t aiNewSize) {
    uint32_t v3;
    bool bResult = 1;
    uint32_t uiCurrentSize = GetSize();
    if (uiCurrentSize >= 0x4000) {
        v3 = uiCurrentSize;
    }
    else {
        v3 = 0x4000;
    }
    if (aiNewSize > v3) {
        aiNewSize = v3;
    }
    if (aiNewSize == m_uiBufferAllocSize) {
        return 1;
    }
    MEM_CONTEXT eMemContext = GetThreadTLSData()->eMemContext;
    GetThreadTLSData()->eMemContext = MEM_CONTEXT_BSFILE;
    NiFile::Seek(0, NiFile::ms_iSeekSet);
    NiFile::Flush();
    MemoryManager_Singleton.Deallocate(m_pBuffer);
    m_uiBufferAllocSize = aiNewSize;
    m_pBuffer = static_cast<char*>(MemoryManager_Singleton.Allocate(m_uiBufferAllocSize));
    if (!m_pBuffer) {
        m_uiBufferAllocSize = uiTESStandardBufferSize;
        m_pBuffer = static_cast<char*>(MemoryManager_Singleton.Allocate(m_uiBufferAllocSize));
        bResult = 0;
    }
    iTrueFilePos = 0;
    GetThreadTLSData()->eMemContext = eMemContext;
    return bResult;
}

// KEEP: when FindFirstFileA fails the exe skips the ftLastWriteTime copy (je 0xB0045D) and still passes the unset
// FILETIME local to FileTimeToSystemTime, an uninitialised read.
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
        FileTimeToSystemTime(&FileTime, &SystemTime);
        SystemTimeToTzSpecificLocalTime(0, &SystemTime, &LocalTime);
        *result = LocalTime;
        return result;
    }
    else {
        std::memset(&LocalTime, 0, sizeof(LocalTime));
        *result = LocalTime;
        return result;
    }
}

void BSFile::operator delete(void* apMemory) {
    MemoryManager_Singleton.Deallocate(apMemory);
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
