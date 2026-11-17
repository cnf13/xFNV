#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "SDK/MSVC/ehvec.h"

class NiStream;

// The state of one particle: where it is heading and how long it has lived.
class NiParticleInfo {
public:
    NiParticleInfo();

    void LoadBinary(NiStream& kStream);
    // Componentwise; a NaN component never compares equal.
    bool IsEqual(const NiParticleInfo& kData) const;

    NiPoint3 m_kVelocity;    // 0x00
    float m_fAge;            // 0x0C
    float m_fLifeSpan;       // 0x10
    float m_fLastUpdate;     // 0x14
    uint16_t m_usGeneration; // 0x18
    uint16_t m_usCode;       // 0x1A
};
