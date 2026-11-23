#include "BSMovie/CBSAStream.h"

#include "BSCore/BSCriticalSection.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSMovie/CAsyncStream.h"
#include "BSSystem/BSFile.h"
#include "BSSystem/FileFinder.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"

CBSAStream::CBSAStream(char* apPath) : CAsyncStream(), ullBytesRead{} {
    BSstrcpy(cPath, 260, apPath);
    pFile = static_cast<BSFile*>(FileFinder::GetFile(cPath, NiFile::READ_ONLY, 0x4000u, 8u));
    if (!pFile || !pFile->m_bGood) {
        if (pFile) {
            delete pFile;
        }
        pFile = nullptr;
    }
}

CBSAStream::~CBSAStream() {
    if (pFile) {
        delete pFile;
    }
}

int32_t CBSAStream::Seek(int64_t aiPosition) {
    if (aiPosition < 0 || (aiPosition > ullFileSize.QuadPart || (!pFile || !pFile->m_bGood))) {
        return 1;
    }
    ullBytesRead.QuadPart = aiPosition;
    pFile->Seek(aiPosition, NiFile::ms_iSeekSet);
    return 0;
}

int32_t CBSAStream::Read(uint8_t* apBuffer, uint32_t auiLength, int32_t abAligned, uint32_t* apBytesRead) {
    BSCriticalSectionGuard kGuard(&kLock);
    if (pFile && pFile->m_bGood) {
        uint32_t uiClampedLength;
        if (ullBytesRead.QuadPart + auiLength <= ullFileSize.QuadPart) {
            uiClampedLength = auiLength;
        }
        else {
            uiClampedLength = ullFileSize.u.LowPart - ullBytesRead.u.LowPart;
        }
        *apBytesRead = pFile->NiBinaryStream::Read(apBuffer, auiLength);
        ullBytesRead.QuadPart += uiClampedLength;
        return 0;
    }

    return 1;
}

int64_t CBSAStream::GetFileSize(int64_t* apSizeAvailable) {
    if (pFile && pFile->m_bGood) {
        ullFileSize.u.LowPart = pFile->GetFileSize();
        ullFileSize.u.HighPart = 0;
        *apSizeAvailable = ullFileSize.QuadPart;
        return ullFileSize.QuadPart;
    }

    *apSizeAvailable = 0;
    return 0;
}

// Reads need no alignment: one byte.
uint32_t CBSAStream::Alignment() {
    return 1;
}

void CBSAStream::Lock() {
    kLock.Lock();
}

void CBSAStream::Unlock() {
    kLock.Unlock();
}

bool CBSAStream::IsOpen() {
    return pFile != nullptr;
}

BSCriticalSectionGuard::BSCriticalSectionGuard(BSCriticalSection* apLock) : pLock(apLock) {
    pLock->Lock();
}

BSCriticalSectionGuard::~BSCriticalSectionGuard() {
    pLock->Unlock();
}
