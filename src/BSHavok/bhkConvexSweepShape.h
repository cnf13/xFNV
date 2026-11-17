#pragma once

#include "BSHavok/bhkConvexShape.h"

class hkpConvexShape;

// The game wrapper of a convex sweep shape. Its members are not declared yet.
class bhkConvexSweepShape : public bhkConvexShape {
public:
    bhkConvexSweepShape();
    ~bhkConvexSweepShape() override;

    const NiRTTI* GetRTTI() const override;
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream& arStream) override;
    bool RegisterStreamables(NiStream& arStream) override;
    // Writes nothing, as the game does.
    void SaveBinary(NiStream& arStream) override {}
    bool IsEqual(NiObject* apObject) override;
    uint32_t QCinfoSize() const override;
    void Init(const hkSerializableCinfo& arInfo) override;
    hkSerializableCinfo& CreateCinfo(bool& arCreated) override;

    // Builds an empty convex sweep shape.
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
};

// The creation info of a convex sweep shape: the shape it sweeps and the linear-velocity limits of the sweep.
struct hkConvexSweepShapeCinfo : public hkConvexShapeCinfo {
    const hkpConvexShape* m_childShape;
    float m_maxLinearVelocity;
    float m_linearVelocityClipDiff;
};
