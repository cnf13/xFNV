#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTConvertingStrideIterator.h"
#include "Reconstruction.hpp"

// A vertex's four bone indices (the PDB's source module is NiSkinInstance_Xenon.cpp; 4 bytes).
struct NiBlendIndices {
	uint8_t Indices[4];
};

// The stride iterator over a geometry's blend indices; the PDB adds no members to the converting iterator. The PDB's
// source module is BSTempEffectGeometryDecal.cpp.
class NiGeometryDataIteratorNiBlendIndices : public NiTConvertingStrideIterator<NiBlendIndices> {
public:
	// Copies element aiIndex's four indices to arData (thiscall, ret 8, 0x685E20). IDA calls it
	// NiGeometryDataIteratorNiBlendWeights::Get, but it copies the 4-byte elements the blend-indices iterator walks.
	void Get(int32_t aiIndex, NiBlendIndices& arData);
};
