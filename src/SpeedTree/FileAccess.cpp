#include "SpeedTree/FileAccess.h"

#include "BSCore/MemoryManager.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/crtdefs.h"

#include <memory>
#include <vector>

// The reader keeps its own copy of the bytes.
CTreeFileAccess::CTreeFileAccess(const uint8_t* pMemory, int32_t nNumBytes) : m_nIndex(0) {
    m_vMemory.resize(static_cast<uint32_t>(nNumBytes), 0);
    for (int32_t i = 0; i < nNumBytes; ++i) {
        m_vMemory[static_cast<uint32_t>(i)] = pMemory[i];
    }
}

EFileToken CTreeFileAccess::PeekToken() {
    int32_t nIndex = m_nIndex;
    return static_cast<EFileToken>(*reinterpret_cast<const int32_t*>(&m_vMemory[nIndex]));
}

// The float's address is taken before the position moves on, and read after.
float CTreeFileAccess::ParseFloat() {
    int32_t nIndex = m_nIndex;
    const float* pValue = reinterpret_cast<const float*>(&m_vMemory[nIndex]);
    m_nIndex += 4;
    return *pValue;
}

uint8_t CTreeFileAccess::ParseByte() {
    uint32_t v2 = m_nIndex++;
    return m_vMemory[v2];
}

// The int's address is taken before the position moves on, and read after. m_vMemory is the file's raw bytes, read as
// an int in place (hence the reinterpret_cast).
int32_t CTreeFileAccess::ParseInt() {
    int32_t nIndex = m_nIndex;
    const int32_t* pValue = reinterpret_cast<const int32_t*>(&m_vMemory[nIndex]);
    m_nIndex += 4;
    return *pValue;
}

// Reads the length token, then that many bytes appended one at a time.
std::string CTreeFileAccess::ParseString() {
    std::string strTemp;
    int32_t nLength = ParseToken();
    for (int32_t i = 0; i < nLength; ++i) {
        strTemp.append(1, static_cast<char>(m_vMemory[m_nIndex++]));
    }
    std::string strReturn;
    strReturn.assign(strTemp, 0, std::string::npos);
    return strReturn;
}

// The three floats go to an array local, zeroed z, y, x first as the CVec3 constructor does.
float* CTreeFileAccess::ParseVector3(float* a1) {
    float afValue[3];
    afValue[2] = 0.0f;
    afValue[1] = 0.0f;
    afValue[0] = 0.0f;
    for (int32_t i = 0; i < 3; ++i) {
        afValue[i] = ParseFloat();
    }
    a1[0] = afValue[0];
    a1[1] = afValue[1];
    a1[2] = afValue[2];
    return a1;
}

// m_vMemory is the file's raw bytes, read as an int in place (hence the reinterpret_cast). The address is taken before
// the position moves on (by 4, twice), and read after.
int32_t CTreeFileAccess::ParseLongSkipPadding() {
    int32_t nIndex = m_nIndex;
    const int32_t* pValue = reinterpret_cast<const int32_t*>(&m_vMemory[static_cast<uint32_t>(nIndex)]);
    m_nIndex += 4;
    m_nIndex += 4;
    return *pValue;
}
