#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiCullingProcess.h"

// The culling process that handles BSFadeNode fade amounts while it collects the visible set.
class BSFadeNodeCuller : public NiCullingProcess {
public:
    explicit BSFadeNodeCuller(NiVisibleArray* apVisSet);

    const NiRTTI* GetRTTI() const override;
    ~BSFadeNodeCuller() override;
    void Process(NiAVObject* apObject) override;
    void Process(const NiCamera* apCamera, NiAVObject* apScene, NiVisibleArray* apVisibleSet) override;

    static void operator delete(void* apMemory);

    static NiRTTI ms_RTTI;
};
