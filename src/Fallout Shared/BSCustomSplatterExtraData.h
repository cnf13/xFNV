#pragma once

#include "Fallout Shared/BSSplatterData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"

// The splatter an object carries for the custom screen splatter.
class BSCustomSplatterExtraData : public NiExtraData {
public:
    BSCustomSplatterExtraData(BSSplatterData* aSplatterData);

    ~BSCustomSplatterExtraData() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }

    // Creates the "CSD" tag string.
    static void InitTag();
    // Returns the "CSD" tag string.
    static const NiFixedString& Tag();

    BSSplatterData SplatterData;

    static NiRTTI ms_RTTI;
    static inline NiFixedString* pTag;
};
