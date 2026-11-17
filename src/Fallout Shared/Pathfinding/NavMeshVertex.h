#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

#include <stdint.h>

struct NavMeshVertex {
    NavMeshVertex();

    // Byte swaps the three coordinates.
    void Endian();

    NiPoint3 Location;
};
