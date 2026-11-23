#pragma once

#include <cstdint>

class NiBox;

// The box clamp of the segment-box NiSqrDistance::Compute: takes the point, the box (whose m_afExtent it reads) and
// the squared distance.
void Case000(float* apPnt, const NiBox* apBox, float* apfSqrDistance);
// The segment-box case where the direction has no zero component: the products of the direction and the point less
// the box's m_afExtent choose the face handed to Face.
float* CaseNoZeros(const NiBox* apBox, float* apfSqrDistance, float* apPnt, float* apDir, float* apfLParam);
// The line-box distance case where the line direction is zero along one axis (ai2) only.
void Case0(
    int32_t ai1,
    int32_t ai0,
    const NiBox* apBox,
    int32_t ai2,
    float* apfLParam,
    float* apfSqrDistance,
    float* apPnt,
    float* apDir
);
// The face case: apBox is the box whose m_afExtent is read, apPnt and apDir the point and direction, and ai2 the
// remaining axis.
float* Face(
    int32_t ai1,
    const NiBox* apBox,
    int32_t ai0,
    float* apPmE,
    float* apfLParam,
    float* apfSqrDistance,
    float* apPnt,
    float* apDir,
    int32_t ai2
);
void CaseTwoZeros(
    float* afPnt,
    const NiBox* pkBox,
    int32_t i0,
    const float* afDir,
    float* pfLParam,
    float* pfSqrDistance,
    int32_t i2,
    int32_t i1
);
