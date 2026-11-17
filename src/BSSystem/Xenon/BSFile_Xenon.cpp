#include "BSSystem/Xenon/BSFile_Xenon.h"

#include "BSSystem/BSFile.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiFile.h"

#include <cstdint>

void BSFile::Seek(int32_t aiOffset, int32_t aiWhence) {
    uint32_t uiPos;
    if (aiWhence == NiFile::ms_iSeekSet) {
        uiPos = aiOffset;
        if (m_eMode == NiFile::READ_ONLY) {
            aiWhence = NiFile::ms_iSeekCur;
            aiOffset -= iTrueFilePos;
        }
    }
    else {
        if (aiWhence == NiFile::ms_iSeekCur) {
            uiPos = aiOffset + iTrueFilePos;
        }
        else {
            if (aiWhence == NiFile::ms_iSeekEnd) {
                uiPos = GetSize() - aiOffset;
            }
            else {
                uiPos = iTrueFilePos;
            }
        }
    }
    if (uiPos != iTrueFilePos) {
        NiFile::Seek(aiOffset, aiWhence);
        iTrueFilePos = uiPos;
    }
}

uint32_t BSFile::GetSize() {
    int32_t uiOffset;
    iFileSize = (uint32_t)0;
    if (((int32_t)m_eMode) == 1) {
        NiFile::Flush();
    }
    Open(false, false);
    if (m_pFile) {
        uiOffset = std::ftell(m_pFile);
        std::fseek(m_pFile, 0, 2);
        iFileSize = (uint32_t)std::ftell(m_pFile);
        std::fseek(m_pFile, uiOffset, 0);
    }
    return iFileSize;
}

// Opens the file through the vtable when it is not open yet, then reads through NiFile and advances the position.
uint32_t BSFile::ReadF(void* apBuffer, uint32_t aiBytes) {
    if (!m_pFile) {
        Open(false, false);
    }
    size_t uiRead = FileRead(apBuffer, aiBytes);
    iTrueFilePos += uiRead;
    return uiRead;
}

bool BSFile::AddAuxBuffer(uint32_t aiMinIndex, uint32_t aiMaxIndex) {
    return true;
}

void BSFile::RemoveAuxBuffer() {}
