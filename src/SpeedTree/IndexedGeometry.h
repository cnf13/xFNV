#pragma once

#include "SpeedTree/SpeedTreeRT.h"

#include <vector>

class CIndexedGeometry {
public:
    CIndexedGeometry(CWindEngine* pWindEngine, bool bRetainTexCoords);
    ~CIndexedGeometry();

    // Sets map 0's texture coordinate pTexCoord at the current vertex, growing the coordinates when the vertex is new;
    // with retained coordinates and sIndex > -1, also keeps sIndex and the unflipped coordinate.
    void AddVertexTexCoord0(const float* pTexCoord, int16_t sIndex);
    // Replaces the texture coordinates of map chChangedMapIndex.
    void ChangeTexCoord(uint8_t chChangedMapIndex, const float* pNewTexCoords);
    // Appends the vertex normal pNormal.
    void AddVertexNormal(const float* pNormal);
    // Appends the vertex coordinate pCoord, and to the original coordinates when vertex
    // weighting is on with CPU wind.
    void AddVertexCoord(const float* pCoord);
    // Appends the vertex tangent pTangent.
    void AddVertexTangent(const float* pTangent);
    // Appends the vertex binormal pBinormal.
    void AddVertexBinormal(const float* pBinormal);
    // The map 0 texture coordinate of the vertex nVertexIndex.
    const float* GetVertexTexCoord0(uint32_t nVertexIndex) const;
    // The number of strips in the LOD level sLodLevel, 0 for a negative level.
    uint16_t GetNumStrips(int16_t sLodLevel) const;
    // Frees every strip of the LOD level nLodLevel, leaving null pointers in its strip array.
    void DeleteLodStrip(uint16_t usLodLevel);
    // Appends the strip pStrip of nStripLength indices to the LOD level nLodLevel at the current strip counter and
    // adds its triangles to the level's count.
    void AddStrip(uint16_t nLodLevel, uint16_t* pStrip, uint16_t nStripLength);
    // Appends the texture coordinate pTexCoord to map 1, its t negated when the trees' textures are flipped.
    void AddVertexTexCoord1(const float* pTexCoord);
    // Appends the next vertex's wind weight, stored as 1 - fWindWeight, and its wind matrix, the engine's local
    // matrix ucWindMatrixIndex wraps to.
    void AddVertexWind(float fWindWeight, uint8_t ucWindMatrixIndex);
    // The strip lengths of the LOD level sLodLevel, null for a level out of range or without strips. Pushed
    // zero-extended and read as a short, like GetNumStrips.
    const uint16_t* GetStripLengthsPointer(int16_t sLodLevel) const;
    // The first element of each vertex array, null when the array is empty.
    const uint32_t* GetVertexColors() const;
    const float* GetVertexCoords() const;
    const float* GetVertexNormals() const;
    const float* GetVertexBinormals() const;
    const float* GetVertexTangents() const;
    const float* GetVertexTexCoords0() const;
    const float* GetVertexTexCoords1() const;
    const float* GetVertexWindWeights() const;
    const uint8_t* GetVertexWindMatrixIndices() const;
    // Starts the strips of the LOD level nLodLevel over: the level becomes the current one, no strip is counted and
    // the level's triangle count is zeroed.
    void ResetStripCounter(uint16_t nLodLevel);
    // The strips of the LOD level nLodLevel, null when it has none. Returns non-const pointers from a const method.
    uint16_t** GetStripsPointer(uint16_t nLodLevel) const;
    // The coordinates of the vertex nVertexIndex, as computed and as loaded.
    const float* GetVertexCoord(uint32_t nVertexIndex) const;
    const float* GetOrigVertexCoord(uint32_t nVertexIndex) const;
    // Packs pColor and appends it to m_vColors.
    void AddVertexColor(const float* pColor);
    uint32_t ColorFloatsToLong(const float* pColor) const;
    // Applies the wind matrices to the vertices of the LOD level nLodLevel once; true when it did.
    bool ComputeWindEffect(uint16_t nLodLevel);
    // Merges each LOD's strips into one, the face order toggled when bToggleFaceOrdering.
    void CombineStrips(bool bToggleFaceOrdering);
    // Grows cExtents by every vertex.
    void ComputeExtents(CRegion& cExtents) const;
    // Transforms the vertices (and their normals) by cTransform.
    void Transform(const CTransform& cTransform);
    void SetNumLodLevels(uint16_t nLevels);

protected:
    // Frees the strips.
    void DeleteIndexData();

public:
    bool m_bRetainTexCoords;
    CWindEngine* m_pWindEngine;
    bool m_bVertexWeighting;
    bool m_bManualLighting;
    bool* m_pVertexWindComputed;
    uint16_t m_nVertexSize;
    bool m_bValid;
    CSpeedTreeRT::EWindMethod m_eWindMethod;
    float m_fTrunkWidth;
    float m_fTrunkLength;
    uint16_t m_nNumLodLevels;
    uint16_t m_nVertexCounter;
    uint16_t m_nVertexCounterLodLevel;
    uint16_t m_nStripCounter;
    std::vector<int32_t, std::allocator<int32_t>> m_vTriangleCounts;
    std::vector<
        std::vector<uint16_t, std::allocator<uint16_t>>,
        std::allocator<std::vector<uint16_t, std::allocator<uint16_t>>>>
        m_vStripLengths;
    std::vector<
        std::vector<uint16_t*, std::allocator<uint16_t*>>,
        std::allocator<std::vector<uint16_t*, std::allocator<uint16_t*>>>>
        m_vStrips;
    std::vector<uint32_t, std::allocator<uint32_t>> m_vColors;
    std::vector<float, std::allocator<float>> m_vCoords;
    std::vector<float, std::allocator<float>> m_vOrigCoords;
    std::vector<float, std::allocator<float>> m_vNormals;
    std::vector<float, std::allocator<float>> m_vBinormals;
    std::vector<float, std::allocator<float>> m_vTangents;
    std::vector<float, std::allocator<float>> m_vTexCoords0;
    std::vector<float, std::allocator<float>> m_vOrigTexCoords0;
    std::vector<uint8_t, std::allocator<uint8_t>> m_vTexIndices0;
    std::vector<float, std::allocator<float>> m_vTexCoords1;
    std::vector<float, std::allocator<float>> m_vWindWeights;
    std::vector<uint8_t, std::allocator<uint8_t>> m_vWindMatrixIndices;
};
