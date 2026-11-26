#pragma once

#include "BSCore/NewOverloads.h"

#include <cstdint>

// The work a tasklet runs on a tasklet thread.
class BSTaskletData {
public:
    BSTaskletData();
    virtual ~BSTaskletData();

    // Runs before Process; false skips the task.
    virtual bool OnStartup();
    // The task's work.
    virtual void Process() {}
    // Runs after Process.
    virtual void OnComplete() {}

    void TaskYield();

    // The deleting destructor frees through ourFree.
    static void operator delete(void* apMemory);

    // Cleared by the constructor.
    bool bYielding;
};

inline void BSTaskletData::operator delete(void* apMemory) {
    ourFree(apMemory);
}

inline BSTaskletData::BSTaskletData() : bYielding(false) {}

inline BSTaskletData::~BSTaskletData() {}

// The default task always runs.
inline bool BSTaskletData::OnStartup() {
    return true;
}

inline void BSTaskletData::TaskYield() {
    bYielding = true;
}
