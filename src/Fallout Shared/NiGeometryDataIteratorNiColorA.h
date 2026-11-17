#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTConvertingStrideIterator.h"

// The stride iterator over a geometry's NiColorA stream (vertex colours); it adds no members to the converting
// iterator.
class NiGeometryDataIteratorNiColorA : public NiTConvertingStrideIterator<NiColorA> {};
