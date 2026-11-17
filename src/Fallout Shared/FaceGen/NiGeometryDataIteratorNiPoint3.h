#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTConvertingStrideIterator.h"
#include "Reconstruction.hpp"

class NiPoint3;

// The stride iterator over a geometry's NiPoint3 stream (vertices, normals); the PDB adds no members to the
// converting iterator. The PDB's source module is BSFaceGenManager.cpp.
class NiGeometryDataIteratorNiPoint3 : public NiTConvertingStrideIterator<NiPoint3> {
public:
	// Writes akValue's 12 bytes to element aiIndex after the release build's empty bounds
	// check 0x4534F0 (thiscall, ret 8, 0x65B390). IDA calls it NiTConvertingStrideIterator<NiBlendIndices>::Set,
	// but it copies three dwords; the PDB's Set(iIndex, kValue) (BSFaceGenModel.cpp), whose callers
	// (BSFaceGenModel::ApplyCoordinateToExistingMesh, BeamProjectile) pass a NiPoint3.
	void Set(int32_t iIndex, const NiPoint3& kValue);
};
