#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTConvertingStrideIterator.h"
#include "Reconstruction.hpp"

// The stride iterator over a geometry's NiColorA stream (vertex colours); the PDB adds no members to the converting
// iterator. The PDB's source module is TESLand.cpp.
class NiGeometryDataIteratorNiColorA : public NiTConvertingStrideIterator<NiColorA> {};
