#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTConvertingStrideIterator.h"

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
