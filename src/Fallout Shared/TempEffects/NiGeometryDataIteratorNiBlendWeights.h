#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTConvertingStrideIterator.h"
#include "Reconstruction.hpp"

// A vertex's four bone weights (16 bytes).
struct NiBlendWeights {
	NiBlendWeights();
	// The PDB's NiBlendWeights(f0, f1, f2, f3) (NiGeometryData.cpp). The hybrid build keeps the class an aggregate.
	constexpr NiBlendWeights(float f0, float f1, float f2, float f3) : fWeights{f0, f1, f2, f3} {}

	// Weight aiIndex (the PDB's operator[]): the exe's shared out-of-line element accessor
	// (thiscall, ret 4).
	// Same function as hkVector4::operator() (0x560D30), registered there.
	float& operator[](int32_t aiIndex);

	float fWeights[4];

	// The PDB's NiGeometryData module holds it: the 16 bytes after NiGeometryData::ms_usNextID in the exe's .data,
	// which no code in the exe reads.
	static NiBlendWeights ZERO;
};

// The stride iterator over a geometry's blend weights; the PDB adds no members to the converting iterator. The PDB's
// source module is BSTempEffectGeometryDecal.cpp.
class NiGeometryDataIteratorNiBlendWeights : public NiTConvertingStrideIterator<NiBlendWeights> {
public:
	// Reads element aiIndex's three stored weights into arData and sets the fourth to one minus their sum.
	void Get(int32_t aiIndex, NiBlendWeights& arData);
};

inline NiBlendWeights NiBlendWeights::ZERO{0.0f, 0.0f, 0.0f, 1.0f};
