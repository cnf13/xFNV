#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// Current and previous water displacement offsets, with the reference's last position.
class WadingWaterData {
public:
    WadingWaterData();

    NiPoint2 fDisplaceOffset;
    NiPoint2 fLastDisplaceOffset;
    NiPoint3 fLastPosition;
};
