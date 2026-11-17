#pragma once

#include "BSShader/TallGrassShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class BSMultiBoundNode;

// A batch of instances of one grass model in one cell, drawn as a single geometry under a multibound node.
class TallGrassGroup {
public:
    // One grass instance: position and colour.
    class GrassInstanceData {
    public:
        float fXPos;
        float fYPos;
        float fZPos;
        float fColor;
    };

    TallGrassGroup(
        TallGrassShaderProperty::CachedGeometry* apCachedGeometry,
        int32_t aiCellX,
        int32_t aiCellY,
        bool abLowDetail
    );
    ~TallGrassGroup();

    // Recomputes the bound of the group.
    void UpdateGroupBound();
    // Removes the grass instance aiInstanceIndex.
    int32_t Remove(uint16_t aiInstanceIndex);
    // Releases the group's instances.
    void Clear();
    void SetGrassParam(TallGrassShaderProperty::GrassParam* apGrassParam);
    int32_t RemoveTerrainChunk(int32_t alTerrainChunkSeed);
    bool Add(TallGrassGroup::GrassInstanceData* apInstanceData, int32_t alTerrainChunkSeed);

    NiPointer<NiGeometry> spGrassGeometry;
    NiPointer<BSMultiBoundNode> spMBGrassNode;
    NiPointer<TallGrassShaderProperty::CachedGeometry> spCachedGeometry;
    NiPointer<TallGrassShaderProperty> spGrassProperty;
    uint16_t iMaxInstanceCount;
    uint16_t iInstanceCount;
    // iMaxInstanceCount instances
    GrassInstanceData* pInstanceData;
    // the terrain chunk seed of each instance
    int32_t* plTerrainChunkSeed;
    TallGrassShaderProperty::GrassParam kGrassParam;
    // the cell's x and y
    int32_t iCellCoord[2];
    bool bLowDetail;
    uint8_t padding45[3];
};
