#pragma once

#include "Fallout Shared/TESTexture.h"

// A texture file name whose texture may be at most 1024 pixels wide.
class TESTexture1024 : public TESTexture {
public:
    TESTexture1024();
    ~TESTexture1024();

    uint32_t GetMaxAllowedSize() override {
        return 1024;
    }
};
