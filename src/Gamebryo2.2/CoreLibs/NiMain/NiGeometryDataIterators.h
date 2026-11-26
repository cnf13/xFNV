#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTConvertingStrideIterator.h"

#include <stdint.h>

class NiPoint3;

// The stride iterator over a geometry's NiPoint3 stream (vertices, normals); it adds no members to the converting
// iterator.
class NiGeometryDataIteratorNiPoint3 : public NiTConvertingStrideIterator<NiPoint3> {
public:
    // Writes kValue's 12 bytes to element iIndex after an empty bounds check. Callers
    // (BSFaceGenModel::ApplyCoordinateToExistingMesh, BeamProjectile) pass a NiPoint3.
    void Set(int32_t iIndex, const NiPoint3& kValue);
};

class NiPoint2;

// The stride iterator over a geometry's NiPoint2 stream (texture coordinates); it adds no members to the converting
// iterator.
class NiGeometryDataIteratorNiPoint2 : public NiTConvertingStrideIterator<NiPoint2> {};

// The stride iterator over a geometry's NiColorA stream (vertex colours); it adds no members to the converting
// iterator.
class NiGeometryDataIteratorNiColorA : public NiTConvertingStrideIterator<NiColorA> {};

// A vertex's four bone indices.
struct NiBlendIndices {
    uint8_t Indices[4];
};

// The stride iterator over a geometry's blend indices; it adds no members to the converting iterator.
class NiGeometryDataIteratorNiBlendIndices : public NiTConvertingStrideIterator<NiBlendIndices> {
public:
    // Copies element aiIndex's four indices to arData.
    void Get(int32_t aiIndex, NiBlendIndices& arData);
};

inline void NiGeometryDataIteratorNiBlendIndices::Get(int32_t aiIndex, NiBlendIndices& arData) {
    // The stream is raw vertex memory walked as untyped bytes.
    arData = *reinterpret_cast<NiBlendIndices*>(static_cast<char*>(this->GetCurrentPosition()) + m_iStride * aiIndex);
}

// A vertex's four bone weights (16 bytes).
struct NiBlendWeights {
    NiBlendWeights();
    constexpr NiBlendWeights(float f0, float f1, float f2, float f3) : fWeights{f0, f1, f2, f3} {}

    // Returns weight aiIndex.
    float& operator[](int32_t aiIndex);

    float fWeights[4];

    static NiBlendWeights ZERO;
};

// The stride iterator over a geometry's blend weights; it adds no members to the converting iterator.
class NiGeometryDataIteratorNiBlendWeights : public NiTConvertingStrideIterator<NiBlendWeights> {
public:
    // Reads element aiIndex's three stored weights into arData and sets the fourth to one minus their sum.
    void Get(int32_t aiIndex, NiBlendWeights& arData);
};

// The stream is untyped vertex memory (m_pvCurrentPosition) holding three weights per element, read as floats; the
// fourth weight is one minus their sum.
inline void NiGeometryDataIteratorNiBlendWeights::Get(int32_t aiIndex, NiBlendWeights& arData) {
    const float* pfData = reinterpret_cast<const float*>(static_cast<char*>(m_pvCurrentPosition) + m_iStride * aiIndex);
    float fWeight = pfData[0];
    float fRemaining = 1.0 - fWeight;
    arData[0] = fWeight;
    fWeight = pfData[1];
    fRemaining = fRemaining - fWeight;
    arData[1] = fWeight;
    fWeight = pfData[2];
    fRemaining = fRemaining - fWeight;
    arData[2] = fWeight;
    arData[3] = fRemaining;
}

inline NiBlendWeights NiBlendWeights::ZERO{0.0f, 0.0f, 0.0f, 1.0f};

inline float& NiBlendWeights::operator[](int32_t aiIndex) {
    return fWeights[aiIndex];
}

// The element sits m_iStride bytes apart in the untyped vertex stream. The release build's bounds check is an empty
// function.
inline void NiGeometryDataIteratorNiPoint3::Set(int32_t iIndex, const NiPoint3& kValue) {
    void* pvElement = static_cast<uint8_t*>(m_pvCurrentPosition) + m_iStride * static_cast<uint32_t>(iIndex);
    *static_cast<NiPoint3*>(pvElement) = kValue;
}

inline NiBlendWeights::NiBlendWeights() : fWeights{0.0f, 0.0f, 0.0f, 0.0f} {}
