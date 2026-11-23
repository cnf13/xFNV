#pragma once

#include "Fallout Shared/ImageSpaceModifierInstanceTemp.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint2.h"

// A radial blur: ramps up, holds and ramps down around a screen position.
class ImageSpaceModifierInstanceRB : public ImageSpaceModifierInstanceTemp {
public:
    ImageSpaceModifierInstanceRB(
        float afStrength,
        float afAge,
        NiAVObject* apTarget,
        float afDuration,
        float afRampup,
        float afStart,
        float afRampdown,
        float afDownStart,
        const NiPoint2& aCenter
    );
    ~ImageSpaceModifierInstanceRB() override;

    void Apply() override;
    void PrintInfo(char* apBuffer) override;

    // Makes the effect and starts it.
    static ImageSpaceModifierInstanceRB* Trigger(
        float afStrength,
        float afRampup,
        float afStart,
        float afRampdown,
        float afDownStart,
        const NiPoint2& aCenter,
        float afDuration
    );

    float fRampup;
    float fStart;
    float fRampdown;
    float fDownStart;
    NiPoint2 Center;
};
