#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"

// Bethesda's bounding sphere, with type, flag and level queries over NiBound's data; it adds no fields.
class BSNiBound : public NiBound {
public:
    // Copies a plain sphere in (NiBound's implicit copy): the world bound NiAVObject holds is a BSNiBound, and the
    // update passes store NiBound values into it.
    using NiBound::operator=;

    // The constructor: builds the NiBound base and ignores both arguments.
    BSNiBound(uint16_t asFlags, uint16_t asLevel);
};

// The constructor deliberately leaves the sphere unset and ignores both arguments.
inline BSNiBound::BSNiBound(uint16_t asFlags, uint16_t asLevel) {}
