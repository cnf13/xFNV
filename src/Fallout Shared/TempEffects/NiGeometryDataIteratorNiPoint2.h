#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiTConvertingStrideIterator.h"

class NiPoint2;

// The stride iterator over a geometry's NiPoint2 stream (texture coordinates); it adds no members to the converting
// iterator.
class NiGeometryDataIteratorNiPoint2 : public NiTConvertingStrideIterator<NiPoint2> {};
