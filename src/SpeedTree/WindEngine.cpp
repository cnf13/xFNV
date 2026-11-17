#include "SpeedTree/WindEngine.h"

#include "BSCore/MemoryManager.h"
#include "SDK/MSVC/stdlib.h"
#include "SpeedTree/SpeedTreeRT.h"
#include "SpeedTree/WindInfo.h"

CWindMatrices CWindEngine::m_cWindMatrices;

// This file's copy of the statics from SpeedTree's LeafOffsetTable.h.
static const int32_t c_nLeafOffsetCount = 30;
static const float c_fLeafOffsetScalar = 1.0f;
[[maybe_unused]] static const float c_afLeafOffsets[c_nLeafOffsetCount] = {
    c_fLeafOffsetScalar * 6.0f,   c_fLeafOffsetScalar * -10.0f, c_fLeafOffsetScalar * 16.0f,
    c_fLeafOffsetScalar * -8.0f,  c_fLeafOffsetScalar * 2.0f,   c_fLeafOffsetScalar * -18.0f,
    c_fLeafOffsetScalar * -14.0f, c_fLeafOffsetScalar * 0.0f,   c_fLeafOffsetScalar * -4.0f,
    c_fLeafOffsetScalar * 10.0f,  c_fLeafOffsetScalar * -6.0f,  c_fLeafOffsetScalar * 8.0f,
    c_fLeafOffsetScalar * 4.0f,   c_fLeafOffsetScalar * -2.0f,  c_fLeafOffsetScalar * 14.0f,
    c_fLeafOffsetScalar * -12.0f, c_fLeafOffsetScalar * 14.0f,  c_fLeafOffsetScalar * -16.0f,
    c_fLeafOffsetScalar * 4.0f,   c_fLeafOffsetScalar * -18.0f, c_fLeafOffsetScalar * 13.0f,
    c_fLeafOffsetScalar * 9.0f,   c_fLeafOffsetScalar * 17.0f,  c_fLeafOffsetScalar * -9.0f,
    c_fLeafOffsetScalar * 3.0f,   c_fLeafOffsetScalar * -7.0f,  c_fLeafOffsetScalar * -15.0f,
    c_fLeafOffsetScalar * 7.0f,   c_fLeafOffsetScalar * -5.0f,  c_fLeafOffsetScalar * 15.0f
};

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
    m_fLeafFrequency = (float)(8.0 * cWindInfo.m_fStrength * m_fLeafFactors[1]);
    m_fLeafThrow = (float)(180.0 * cWindInfo.m_fStrength * m_fLeafFactors[0]);
    m_fWindStrength = cWindInfo.m_fStrength;
}

// The products and the shift run on the x87 stack (kept as double here) and are stored as floats; the old frequency
// goes through a float local.
float CWindEngine::SetWindStrength(float fNewStrength, float fOldStrength, float fOldTimeShift) {
    float fOldFrequency = (float)(8.0 * fOldStrength * m_fLeafFactors[1]);
    m_fWindStrength = fNewStrength;
    m_fLeafFrequency = (float)(8.0 * m_fWindStrength * m_fLeafFactors[1]);
    m_fLeafThrow = (float)(180.0 * m_fWindStrength * m_fLeafFactors[0]);
    m_fTimeFrequencyShift =
        (float)((double)m_fTime * fOldFrequency + fOldTimeShift - (double)m_fTime * m_fLeafFrequency);
    m_pRockingAngles = nullptr;
    m_uiNumLeafAngles = 0;
    return m_fTimeFrequencyShift;
}
