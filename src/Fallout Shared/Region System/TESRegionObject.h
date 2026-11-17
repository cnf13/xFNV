#pragma once

#include "Fallout Shared/Region System/TESRegionObjectBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class TESForm;
class TESRegionObjectList;

// A region's object entry (trees, rocks and the like).
class TESRegionObject : public TESRegionObjectBase {
public:
    // The object's placement settings.
    struct REGION_OBJECT_DATA {
        float fDensity;
        uint8_t cCluster;
        uint8_t cMinSlopeDegrees;
        uint8_t cMaxSlopeDegrees;
        uint8_t cObjectFlags;
        uint16_t sParentRadius;
        uint16_t sGenerateNearRadius;
        float fMinimumZ;
        float fMaximumZ;
        float fSinkBase;
        float fSinkVariance;
        float fScaleVariance;
        int16_t sXAngleVariance;
        int16_t sYAngleVariance;
        int16_t sZAngleVariance;
        uint32_t dVertexColor;
    };

    // Whether the object's form (pForm) is a TESLandTexture.
    bool IsFormLandTexture();

    // The slope angle of aNormal: acosf of z over the length.
    static float ConvertNormalToAngle(NiPoint3 aNormal);

    TESForm* pForm;
    REGION_OBJECT_DATA Data;
    TESRegionObjectList* pChildren;
    TESRegionObject* pParent;
};
