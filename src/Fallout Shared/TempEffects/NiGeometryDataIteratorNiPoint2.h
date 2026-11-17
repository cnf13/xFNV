#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTConvertingStrideIterator.h"
#include "Reconstruction.hpp"

class NiPoint2;

// The stride iterator over a geometry's NiPoint2 stream (texture coordinates); the PDB adds no members to the
// converting iterator. The PDB's source module is BSTempEffectGeometryDecal.cpp.
class NiGeometryDataIteratorNiPoint2 : public NiTConvertingStrideIterator<NiPoint2> {};
