#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"

#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cstring>

NiFrustum::NiFrustum(bool bOrtho) : m_bOrtho(bOrtho) {}

NiFrustum::NiFrustum(float fLeft, float fRight, float fTop, float fBottom, float fNear, float fFar, bool bOrtho)
  : m_fLeft(fLeft), m_fRight(fRight), m_fTop(fTop), m_fBottom(fBottom), m_fNear(fNear), m_fFar(fFar), m_bOrtho(bOrtho) {
}

// "%s = (N=%g,F=%g,L=%g,R=%g,T=%g,B=%g,O=%u)": the six floats are widened to double for the varargs and the ortho
// flag is zero-extended from its byte.
char* NiFrustum::GetViewerString(const char* pcPrefix) const {
    size_t stLength = strlen(pcPrefix) + 80;
    char* pcString = static_cast<char*>(_NiMalloc(stLength));
    NiSprintf(
        pcString,
        stLength,
        "%s = (N=%g,F=%g,L=%g,R=%g,T=%g,B=%g,O=%u)",
        pcPrefix,
        m_fNear,
        m_fFar,
        m_fLeft,
        m_fRight,
        m_fTop,
        m_fBottom,
        m_bOrtho
    );
    return pcString;
}
