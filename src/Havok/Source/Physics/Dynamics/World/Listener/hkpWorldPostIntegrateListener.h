#pragma once

class hkpWorld;
struct hkStepInfo;

// The interface the world reports the end of an integrate step to. It holds no state.
// hkpWorldCallbackUtil::firePostIntegrateCallback calls postIntegrateCallback with the world and the step.
class hkpWorldPostIntegrateListener {
public:
    virtual ~hkpWorldPostIntegrateListener();

    virtual void postIntegrateCallback(hkpWorld* world, const hkStepInfo& stepInfo) = 0;
};
