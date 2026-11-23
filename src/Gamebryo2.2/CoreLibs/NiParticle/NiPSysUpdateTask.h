#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPool.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTask.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiCriticalSection.h"
#include "SDK/MSVC/ehvec.h"

#include <stdint.h>

class NiParticleSystem;
class NiPSysUpdateTask;

// A task that updates one particle system to a given time.
class NiPSysUpdateTask : public NiTask {
public:
    NiPSysUpdateTask();
    ~NiPSysUpdateTask() override;

    const NiRTTI* GetRTTI() const override;
    void DoTask() override;
    bool Clear() override;

    // Points the task at pkSystem and the time to update it to.
    void Init(NiParticleSystem* pkSystem, float fTime);

    static NiPSysUpdateTask* GetFreeObject();
    // Empties the task pool: clears the free set, deletes the containers and their tasks and resets the pool's size.
    // NiParticleSDM::Shutdown calls it.
    static void _SDMShutdown();

    NiPointer<NiParticleSystem> m_spSystem;
    float m_fTime;

    static NiRTTI ms_RTTI;
    static NiTObjectPool<NiPSysUpdateTask> ms_kPool;
    static NiCriticalSection ms_kPoolCriticalSection;
};
