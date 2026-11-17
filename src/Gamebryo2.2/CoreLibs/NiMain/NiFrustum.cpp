#include "Gamebryo2.2/CoreLibs/NiMain/NiFrustum.h"

#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cstring>

NiFrustum::NiFrustum(bool bOrtho) : m_bOrtho(bOrtho) {}

NiFrustum::NiFrustum(float bOrtho, float a2, float a3, float a4, float a5, float a6, bool a7)
  : m_fLeft(bOrtho), m_fRight(a2), m_fTop(a3), m_fBottom(a4), m_fNear(a5), m_fFar(a6), m_bOrtho(a7) {}

// "%s = (N=%g,F=%g,L=%g,R=%g,T=%g,B=%g,O=%u)": the six floats are widened to double for the varargs and the ortho
// flag is zero-extended from its byte.
char* NiFrustum::GetViewerString(const char* pcPrefix) const {
    size_t stLength = strlen(pcPrefix) + 80;
    char* pcString = static_cast<char*>(_NiMalloc(stLength));
    NiSprintf(
        pcString, stLength, "%s = (N=%g,F=%g,L=%g,R=%g,T=%g,B=%g,O=%u)", pcPrefix, m_fNear, m_fFar, m_fLeft, m_fRight,
        m_fTop, m_fBottom, m_bOrtho
    );
    return pcString;
}
