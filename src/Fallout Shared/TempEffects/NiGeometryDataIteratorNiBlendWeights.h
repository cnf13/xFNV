#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTConvertingStrideIterator.h"

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

inline NiBlendWeights NiBlendWeights::ZERO{0.0f, 0.0f, 0.0f, 1.0f};
