#pragma once

#include "Gamebryo2.2/CoreLibs/NiCollision/NiSphereBV.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// Conversions from bounds to bounding volumes; the results live in static volumes that the next call overwrites.
class NiCollisionConvert {
public:
    // Fills ms_kSphereABV with the sphere and returns it.
    static NiBoundingVolume* ConvertToSphereBV_Fast(float fRadius, const NiPoint3& kCenter);
    static NiBoundingVolume* ConvertToSphereBV_Safe(float fRadius, const NiPoint3& kCenter);
    // usQuantity is declared int32_t rather than unsigned short so that the caller's whole value is passed on.
    static NiBoundingVolume* ConvertToBoxBV(int32_t usQuantity, const NiPoint3* pkVertex);

    // The sphere ConvertToSphereBV_Fast fills.
    static NiSphereBV ms_kSphereABV;
};
