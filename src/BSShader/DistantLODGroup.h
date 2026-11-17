#pragma once

#include "BSShader/DistantLODShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

// A batch of instances of one distant LOD model drawn as a single geometry.
class DistantLODGroup {
public:
    // One instance: normalized position and a scale/colour value.
    class DistantLODInstanceData {
    public:
        float fXPosNorm;
        float fYPosNorm;
        float fZPosNorm;
        float fScaleColor;
    };

    DistantLODGroup(DistantLODShaderProperty::CachedGeometry* apCachedGeometry, uint32_t aiCellChunk, bool abLowDetail);
    ~DistantLODGroup();

    // Updates the group's bound.
    void UpdateGroupBound();
    void SetDistantLODParam(DistantLODShaderProperty::DistantLODParam* apDistantLODParam);
    bool Add(DistantLODGroup::DistantLODInstanceData* apInstanceData, uint32_t aiCellKey);
    void BD6150();

    NiPointer<NiGeometry> spDistantLODGeometry;
    NiPointer<DistantLODShaderProperty::CachedGeometry> spCachedGeometry;
    NiPointer<DistantLODShaderProperty> spDistantLODProperty;
    uint16_t iMaxInstanceCount;
    uint16_t iInstanceCount;
    // iMaxInstanceCount instances
    DistantLODInstanceData* pInstanceData;
    // the cell key of each instance
    uint32_t* piCellKeys;
    DistantLODShaderProperty::DistantLODParam kDistantLODParam;
    uint32_t iCellChunk;
};
