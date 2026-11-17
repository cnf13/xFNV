#pragma once

#include "SpeedTree/SpeedTreeRT.h"

class SIdvWindInfo;

// The wind matrices shared by every tree.
class CWindMatrices {
public:
    // Resizes the matrix array to usCount matrices.
    void Resize(uint16_t usCount);

    uint16_t m_usNumMatrices;
    CTransform* m_pMatrices;
};

// A tree's wind state. Members are public here so that their offsets can be pinned.
class CWindEngine {
public:
    CWindEngine();

    void SetLocalMatrices(uint32_t nStartingMatrix, uint32_t nMatrixSpan);
    // Takes the leaf factors and the strength from cWindInfo and derives the leaf frequency and throw.
    void Init(const SIdvWindInfo& cWindInfo);
    // Sets the strength fNewStrength, shifting the time so that the leaves keep their phase, and drops the leaf
    // angle tables. Returns the new time shift.
    float SetWindStrength(float fNewStrength, float fOldStrength, float fOldTimeShift);

    float m_fTimeFrequencyShift;
    float m_fWindStrength;
    CSpeedTreeRT::EWindMethod m_eBranchMethod;
    CSpeedTreeRT::EWindMethod m_eFrondMethod;
    CSpeedTreeRT::EWindMethod m_eLeafMethod;
    bool m_bRockingLeaves;
    uint8_t padding15[3];
    float m_fLeafFactors[2];
    float m_fLeafFrequency;
    float m_fLeafThrow;
    // The first wind matrix this engine uses and how many it spans.
    int32_t m_nStartingMatrix;
    int32_t m_nMatrixSpan;
    uint32_t m_uiNumLeafAngles;
    float* m_pRockingAngles;
    // The angle table of the rustling leaves.
    float* m_pRustleAngles;
    float m_fSpeedWindRockScalar;
    // LoadTree writes it.
    float m_fSpeedWindRustleScalar;

    static CWindMatrices m_cWindMatrices;
    static inline float m_fTime;
};
