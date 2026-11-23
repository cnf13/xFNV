#pragma once

#include "BSHavok/bhkShape.h"

// The game base of the height field shapes (bhkPlaneShape derives from it). Its members are not declared yet.
class bhkHeightFieldShape : public bhkShape {
public:
    bhkHeightFieldShape();
    ~bhkHeightFieldShape() override;

    const NiRTTI* GetRTTI() const override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;

    static NiRTTI ms_RTTI;
    // Live wrappers of this class, counted by the constructors.
    static inline uint32_t iCount;
};
