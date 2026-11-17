#pragma once

#include "Fallout Shared/FaceGen/BSFaceGenModel.h"
#include "Reconstruction.hpp"
#include "SDK/FaceGen/TriFileIO.h"

#include <memory>
#include <vector>

template <class T> class FR2Vect3TC;

// The FR2Vect3TC<float> vector's const operator[], which the exe calls out of line on vectors other than
// FffTriFileDataS::vtxList too (BSFaceGenMorphDataHair's constructor on a diff morph's vtxDelta): checked against
// size().
const FR2Vect3TC<float>&
VectorSubscript(const std::vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>* apThis, uint32_t _Pos);

// The data of a FaceGen .tri file: the vertex, triangle, quad, texture and label lists and the difference and static
// morphs. The PDB declares them as std::vector members (vtxList, triList, quadList, txtData, txtTriList, txtQuadList,
// vtxLabelList, surfLabelList, diffMorphList, statMorphList); the exe lays out ten 24-byte vectors for them, whose
// element sizes fffReadTriFile (0xAB5EC0) gives: 12-byte triangles, 16-byte quads, 8-byte texture points, 32-byte
// vertex labels and 44-byte surface labels. The PDB places the class in BSFaceGenModel.cpp.
class FffTriFileDataS {
public:
	FffTriFileDataS();
	~FffTriFileDataS();

	// The vertex count of the model itself: vtxList's size, lowered to the first vertex of
	// any static morph.
	uint32_t numModelVtx() const;
	// Not named in the PDB or IDA; name chosen here. vtxList.size(), which the exe calls out of line (the
	// FR2Vect3TC<float> vector's size, thiscall, ret 0, 0x65C700).
	// Inferred type: return: uint32_t: unsigned uses from 0x65a290
	uint32_t vtxListSize() const;
	// Not named in the PDB or IDA; name chosen here. vtxList[i], which the exe calls out of line (the
	// FR2Vect3TC<float> vector's const operator[], thiscall, ret 4, 0x65C720).
	// Inferred type: i: uint32_t: unsigned uses from 0x65c732
	const FR2Vect3TC<float>& vtxListAt(uint32_t i) const;
	// Copies d's lists into this one: each list resized to d's count, the plain lists copied whole and the labels'
	// and morphs' elements one by one (thiscall, ret 4, 0xAB46F0). Defined in TriFileIO.cpp, the PDB's file.
	void copyFrom(const FffTriFileDataS& d);
	void extractSurfLblPts(std::vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>& ptList);
	void extractVtxLblPts(std::vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>& ptList);

	static void operator delete(void* apMemory);

	std::vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>> vtxList;
	std::vector<FR2Vect3TC<int32_t>, std::allocator<FR2Vect3TC<int32_t>>> triList;
	std::vector<FutVect4TC<int32_t>, std::allocator<FutVect4TC<int32_t>>> quadList;
	std::vector<FR2Vect2TC<float>, std::allocator<FR2Vect2TC<float>>> txtData;
	std::vector<FR2Vect3TC<int32_t>, std::allocator<FR2Vect3TC<int32_t>>> txtTriList;
	std::vector<FutVect4TC<int32_t>, std::allocator<FutVect4TC<int32_t>>> txtQuadList;
	std::vector<FffTriVtxLabelS, std::allocator<FffTriVtxLabelS>> vtxLabelList;
	std::vector<FffTriSurfLabelS, std::allocator<FffTriSurfLabelS>> surfLabelList;
	// PDB name (IDA charC0): the difference morphs; BSFaceGenMorphDataHair's constructor 0x6621D0 reads element 0's
	// label and vertex deltas (+0x1C, FffTriDiffMorphS::vtxDelta).
	std::vector<FffTriDiffMorphS, std::allocator<FffTriDiffMorphS>> diffMorphList;
	// PDB name (IDA charD8): the static morphs; 0x65A5A0 reads each element's firstVtxOffset (+0x1C).
	std::vector<FffTriStatMorphS, std::allocator<FffTriStatMorphS>> statMorphList;
};
