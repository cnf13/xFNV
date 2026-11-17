#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFogProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPlane.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

// Bethesda's fog: start/end distances (also under water), the fog plane, falloff, height and the water colour.
class BSFogProperty : public NiFogProperty {
public:
    BSFogProperty();

    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    const NiRTTI* GetRTTI() const override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;

    // Sets the fog's start and end distances.
    void SetDistance(float afStart, float afEnd);
    // Sets the start distance from afDepth (0 to 1) and the end distance to the 163840 maximum.
    void SetDepth(float afDepth);
    void SetFogHeight(float afHeight);
    void SetWaterColor(const NiColor& aColor);
    void CopyMembers(BSFogProperty* pDest, NiCloningProcess& arCloneData);
    void SetWaterDistance(float afStart, float afEnd);
    // Sets fPower.
    void SetPower(float afPower);
    int32_t SetColor(float* a1);

    float fStartDistance;
    float fEndDistance;
    float fStartWaterDistance;
    float fEndWaterDistance;
    NiPlane kPlane;
    float fFalloff;
    float fHeight;
    NiColor kWaterColor;
    float fPower;

    static NiRTTI ms_RTTI;
};

NiSmartPointer(BSFogProperty);
