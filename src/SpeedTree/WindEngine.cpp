#include "SpeedTree/WindEngine.h"

#include "BSCore/MemoryManager.h"
#include "SDK/MSVC/stdlib.h"
#include "SpeedTree/SpeedTreeRT.h"
#include "SpeedTree/WindInfo.h"

CWindMatrices CWindEngine::m_cWindMatrices;

CWindEngine::CWindEngine()
  : m_fTimeFrequencyShift(0.0f), m_fWindStrength(0.25f), m_eBranchMethod(CSpeedTreeRT::WIND_NONE),
    m_eFrondMethod(CSpeedTreeRT::WIND_NONE), m_eLeafMethod(CSpeedTreeRT::WIND_NONE), m_bRockingLeaves(true),
    m_fLeafFactors{1.0f, 1.0f}, m_fLeafFrequency(0.0f), m_fLeafThrow(0.0f), m_nStartingMatrix(0), m_nMatrixSpan(4),
    m_uiNumLeafAngles(0), m_pRockingAngles(nullptr), m_pRustleAngles(nullptr), m_fSpeedWindRockScalar(1.0f),
    m_fSpeedWindRustleScalar(1.0f) {}

void CWindEngine::SetLocalMatrices(uint32_t nStartingMatrix, uint32_t nMatrixSpan) {
    m_nStartingMatrix = nStartingMatrix;
    m_nMatrixSpan = nMatrixSpan;
}

// The products run on the x87 stack (kept as double here) and are stored as floats; the factors are read back from
// this engine.
void CWindEngine::Init(const SIdvWindInfo& cWindInfo) {
    m_fLeafFactors[0] = cWindInfo.m_cLeafFactors.m_afData[0];
    m_fLeafFactors[1] = cWindInfo.m_cLeafFactors.m_afData[1];
    m_fLeafFrequency = static_cast<float>(8.0 * cWindInfo.m_fStrength * m_fLeafFactors[1]);
    m_fLeafThrow = static_cast<float>(180.0 * cWindInfo.m_fStrength * m_fLeafFactors[0]);
    m_fWindStrength = cWindInfo.m_fStrength;
}

// The products and the shift run on the x87 stack (kept as double here) and are stored as floats; the old frequency
// goes through a float local.
float CWindEngine::SetWindStrength(float fNewStrength, float fOldStrength, float fOldTimeShift) {
    float fOldFrequency = static_cast<float>(8.0 * fOldStrength * m_fLeafFactors[1]);
    m_fWindStrength = fNewStrength;
    m_fLeafFrequency = static_cast<float>(8.0 * m_fWindStrength * m_fLeafFactors[1]);
    m_fLeafThrow = static_cast<float>(180.0 * m_fWindStrength * m_fLeafFactors[0]);
    m_fTimeFrequencyShift = static_cast<float>(
        static_cast<double>(m_fTime) * fOldFrequency + fOldTimeShift - static_cast<double>(m_fTime) * m_fLeafFrequency
    );
    m_pRockingAngles = nullptr;
    m_uiNumLeafAngles = 0;
    return m_fTimeFrequencyShift;
}
