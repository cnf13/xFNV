#pragma once

class hkpWorld;
struct hkStepInfo;

// The interface the world reports the end of a collide step to. It holds no state.
// hkpWorldCallbackUtil::firePostCollideCallback calls postCollideCallback with the world and the step.
class hkpWorldPostCollideListener {
public:
    virtual ~hkpWorldPostCollideListener();

    virtual void postCollideCallback(hkpWorld* world, const hkStepInfo& stepInfo) = 0;
};
