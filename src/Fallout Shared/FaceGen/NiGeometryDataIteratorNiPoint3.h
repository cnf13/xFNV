#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTConvertingStrideIterator.h"

class NiPoint3;

// The stride iterator over a geometry's NiPoint3 stream (vertices, normals); it adds no members to the converting
// iterator.
class NiGeometryDataIteratorNiPoint3 : public NiTConvertingStrideIterator<NiPoint3> {
public:
	// Writes kValue's 12 bytes to element iIndex after an empty bounds check. Callers
	// (BSFaceGenModel::ApplyCoordinateToExistingMesh, BeamProjectile) pass a NiPoint3.
	void Set(int32_t iIndex, const NiPoint3& kValue);
};
