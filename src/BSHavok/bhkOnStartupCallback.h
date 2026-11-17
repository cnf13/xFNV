#pragma once

#include "BSHavok/bhkTaskletManagerCallback.h"

// Initializes a tasklet thread's Havok router and its stack buffer.
class bhkOnStartupCallback : public bhkTaskletManagerCallback {
public:
    bhkOnStartupCallback() : bhkTaskletManagerCallback(TMCB_STARTUP) {}
    ~bhkOnStartupCallback() override;

    void operator()() override;
};

// The tasklet threads' exit callback.
class bhkOnExitCallback : public bhkTaskletManagerCallback {
public:
    bhkOnExitCallback() : bhkTaskletManagerCallback(TMCB_EXIT) {}
    ~bhkOnExitCallback() override;

    // Does nothing on PC.
    void operator()() override {}
};
