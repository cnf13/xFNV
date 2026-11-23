#pragma once

#include "Fallout Shared/ImageSpaceModifierInstanceTemp.h"

// A timed depth-of-field effect: the focus distance and range, a motion blur amount and the mode.
class ImageSpaceModifierInstanceDOF : public ImageSpaceModifierInstanceTemp {
public:
    enum DepthOfFieldMode : int32_t {
        DOF_FRONT_BACK = 0x0,
        DOF_FRONT = 0x1,
        DOF_BACK = 0x2,
        DOF_FRONT_BACK_MASK = 0x3,
        DOF_FRONT_MASK = 0x4,
        DOF_BACK_MASK = 0x5,
        DOF_NONE = 0x6,
    };

    ImageSpaceModifierInstanceDOF(
        float afStrength,
        float afAge,
        DepthOfFieldMode aMode,
        float afDistance,
        float afRange,
        float afMotionBlur,
        NiAVObject* apTarget,
        float afDuration
    );

    ~ImageSpaceModifierInstanceDOF() override;

    void Apply() override;
    void PrintInfo(char* apBuffer) override;

    void SetDistance(float afDistance);
    void SetMaskConst(float afMaskConst);

    // Starts a depth of field instance; null when none can be made.
    static ImageSpaceModifierInstanceDOF* Trigger(
        float afDistance,
        float afRange,
        DepthOfFieldMode aMode,
        float afStrength,
        float afMaskConst,
        float afDuration
    );

    float fDistance;
    float fRange;
    float fMaskConst;
    DepthOfFieldMode Mode;
};
