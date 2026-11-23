#include "SpeedTree/LibVector_Source/IdvVector.h"
#include "SpeedTree/SpeedTreeRT.h"

#include <limits>

// Both corners are built by CVec's default constructor, then set to the unset state.
CRegion::CRegion() {
    Initialize();
}

// The union of two boxes: a whole copy of this, then per axis (i < 3, signed) the smaller minimum and the larger
// maximum, both compared against this box's own values (NaN keeps the copy).
CRegion CRegion::operator+(const CRegion& cRegion) const {
    CRegion cUnion = *this;
    for (int32_t i = 0; i < 3; ++i) {
        if (m_cMin.m_afData[i] > cRegion.m_cMin.m_afData[i]) {
            cUnion.m_cMin.m_afData[i] = cRegion.m_cMin.m_afData[i];
        }
        if (m_cMax.m_afData[i] < cRegion.m_cMax.m_afData[i]) {
            cUnion.m_cMax.m_afData[i] = cRegion.m_cMax.m_afData[i];
        }
    }
    CRegion cResult = cUnion;
    return cResult;
}

// The box grown to contain a point, unrolled over three axes, comparing the copy's values (NaN keeps the copy).
CRegion CRegion::operator^(const CVec3& cVec) const {
    CRegion cGrown = *this;
    if (cGrown.m_cMin.m_afData[0] > cVec.m_afData[0]) {
        cGrown.m_cMin.m_afData[0] = cVec.m_afData[0];
    }
    if (cGrown.m_cMin.m_afData[1] > cVec.m_afData[1]) {
        cGrown.m_cMin.m_afData[1] = cVec.m_afData[1];
    }
    if (cGrown.m_cMin.m_afData[2] > cVec.m_afData[2]) {
        cGrown.m_cMin.m_afData[2] = cVec.m_afData[2];
    }
    if (cGrown.m_cMax.m_afData[0] < cVec.m_afData[0]) {
        cGrown.m_cMax.m_afData[0] = cVec.m_afData[0];
    }
    if (cGrown.m_cMax.m_afData[1] < cVec.m_afData[1]) {
        cGrown.m_cMax.m_afData[1] = cVec.m_afData[1];
    }
    if (cGrown.m_cMax.m_afData[2] < cVec.m_afData[2]) {
        cGrown.m_cMax.m_afData[2] = cVec.m_afData[2];
    }
    CRegion cResult = cGrown;
    return cResult;
}

float* CRegion::Initialize() {
    m_cMin.m_afData[2] = std::numeric_limits<float>::max();
    m_cMin.m_afData[1] = std::numeric_limits<float>::max();
    m_cMin.m_afData[0] = std::numeric_limits<float>::max();
    m_cMax.m_afData[2] = -std::numeric_limits<float>::max();
    m_cMax.m_afData[1] = -std::numeric_limits<float>::max();
    m_cMax.m_afData[0u] = -std::numeric_limits<float>::max();
    return m_cMin.m_afData;
}
