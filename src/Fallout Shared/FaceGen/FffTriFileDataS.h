#pragma once

#include "Fallout Shared/FaceGen/BSFaceGenModel.h"
#include "SDK/FaceGen/TriFileIO.h"

#include <memory>
#include <vector>

template <class T>
class FR2Vect3TC;

// The FR2Vect3TC<float> vector's const operator[], used on vectors other than FffTriFileDataS::vtxList too
// (BSFaceGenMorphDataHair's constructor on a diff morph's vtxDelta): checked against size().
const FR2Vect3TC<float>&
VectorSubscript(const std::vector<FR2Vect3TC<float>, std::allocator<FR2Vect3TC<float>>>* apThis, uint32_t _Pos);

// The data of a FaceGen .tri file: the vertex, triangle, quad, texture and label lists and the difference and static
// morphs.
class FffTriFileDataS {
public:
    FffTriFileDataS();
    ~FffTriFileDataS();

    // The vertex count of the model itself: vtxList's size, lowered to the first vertex of
    // any static morph.
    uint32_t numModelVtx() const;
    // Returns vtxList.size().
    uint32_t vtxListSize() const;
    const FR2Vect3TC<float>& vtxListAt(uint32_t i) const;
    // Copies d's lists into this one: each list resized to d's count, the plain lists copied whole and the labels' and
    // morphs' elements one by one. Defined in TriFileIO.cpp.
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
    // The difference morphs; BSFaceGenMorphDataHair's constructor reads element 0's label and vertex deltas
    // (FffTriDiffMorphS::vtxDelta).
    std::vector<FffTriDiffMorphS, std::allocator<FffTriDiffMorphS>> diffMorphList;
    // The static morphs.
    std::vector<FffTriStatMorphS, std::allocator<FffTriStatMorphS>> statMorphList;
};
