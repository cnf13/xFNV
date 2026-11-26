#pragma once

#include "BSMain/Setting.h"
#include "BSMain/XINISetting.h"

#include <stdint.h>

// The INI setting collection of the animation blend settings. It adds no members; only its destructor is its own.
class BlendSettingCollection : public INISettingCollection {
public:
    BlendSettingCollection();
    ~BlendSettingCollection() override;
};

// The hit reaction's blend settings (blend weight pairs per bone, and its timings).
class BlendHit {
public:
    static SettingT<BlendSettingCollection> Head;
    static SettingT<BlendSettingCollection> Body;
    static SettingT<BlendSettingCollection> Spine1;
    static SettingT<BlendSettingCollection> Spine2;
    static SettingT<BlendSettingCollection> LUpperArm;
    static SettingT<BlendSettingCollection> LForeArm;
    static SettingT<BlendSettingCollection> LHand;
    static SettingT<BlendSettingCollection> RUpperArm;
    static SettingT<BlendSettingCollection> RForeArm;
    static SettingT<BlendSettingCollection> RHand;
    static SettingT<BlendSettingCollection> fMinVelGain;
    static SettingT<BlendSettingCollection> fMinHeirGain;
    static SettingT<BlendSettingCollection> fGetUpTime;
    static SettingT<BlendSettingCollection> fKnockDownTime;
    static SettingT<BlendSettingCollection> fHitForceMin;
    static SettingT<BlendSettingCollection> fHitForceMax;
    static SettingT<BlendSettingCollection> fBlockForceMin;
    static SettingT<BlendSettingCollection> fBlockForceMax;
    static SettingT<BlendSettingCollection> sHitSpot;
    static SettingT<BlendSettingCollection> sHitStart;
};

// The quadruped hit reaction's blend settings.
class BlendHitQuad {
public:
    static SettingT<BlendSettingCollection> Head;
    static SettingT<BlendSettingCollection> Body;
    static SettingT<BlendSettingCollection> Spine1;
    static SettingT<BlendSettingCollection> Spine2;
    static SettingT<BlendSettingCollection> LUpperArm;
    static SettingT<BlendSettingCollection> LForeArm;
    static SettingT<BlendSettingCollection> LThigh;
    static SettingT<BlendSettingCollection> LCalf;
    static SettingT<BlendSettingCollection> RUpperArm;
    static SettingT<BlendSettingCollection> RForeArm;
    static SettingT<BlendSettingCollection> RThigh;
    static SettingT<BlendSettingCollection> RCalf;
    static SettingT<BlendSettingCollection> sHitSpot;
    static SettingT<BlendSettingCollection> sHitStart;
};

// The default blend settings (blend weight pairs per bone, and the pass-out thresholds).
class BlendDefault {
public:
    static SettingT<BlendSettingCollection> Head;
    static SettingT<BlendSettingCollection> Body;
    static SettingT<BlendSettingCollection> Spine1;
    static SettingT<BlendSettingCollection> Spine2;
    static SettingT<BlendSettingCollection> LUpperArm;
    static SettingT<BlendSettingCollection> LForeArm;
    static SettingT<BlendSettingCollection> LHand;
    static SettingT<BlendSettingCollection> LThigh;
    static SettingT<BlendSettingCollection> LCalf;
    static SettingT<BlendSettingCollection> LFoot;
    static SettingT<BlendSettingCollection> RUpperArm;
    static SettingT<BlendSettingCollection> RForeArm;
    static SettingT<BlendSettingCollection> RHand;
    static SettingT<BlendSettingCollection> RThigh;
    static SettingT<BlendSettingCollection> RCalf;
    static SettingT<BlendSettingCollection> RFoot;
    static SettingT<BlendSettingCollection> Tail;
    static SettingT<BlendSettingCollection> Shield;
    static SettingT<BlendSettingCollection> Quiver;
    static SettingT<BlendSettingCollection> PonyTail;
    static SettingT<BlendSettingCollection> fHighTrans;
    static SettingT<BlendSettingCollection> fHighRot;
    static SettingT<BlendSettingCollection> fLowTrans;
    static SettingT<BlendSettingCollection> fLowRot;
    static SettingT<BlendSettingCollection> fPassOutTime;
    static SettingT<BlendSettingCollection> fPassOutForce;
};
