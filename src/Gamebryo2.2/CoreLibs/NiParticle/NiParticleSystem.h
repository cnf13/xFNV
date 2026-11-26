#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiParticles.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTaskManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"

#include <stdint.h>

class NiCloningProcess;
class NiPSysModifier;
class NiStream;
struct NiUpdateData;

// a particle system: NiParticles plus the list of modifiers that move and age the particles each update.
class NiParticleSystem : public NiParticles {
public:
    NiParticleSystem();
    ~NiParticleSystem() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void ProcessClone(NiCloningProcess& kCloning) override;
    void SetSelectiveUpdateFlags(bool& bSelectiveUpdate, int32_t bSelectiveUpdateTransforms, bool& bRigid) override;
    void UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
    void UpdateSelectedDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
    void UpdateRigidDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
    void UpdateWorldData(NiUpdateData& UpdateData) override;
    void UpdateWorldBound() override;
    void SetModelData(NiGeometryData* pkModelData) override;
    virtual void UpdateSystem(float fTime);
    virtual void Do_UpdateSystem(float fTime);

    void InitializeNewParticle(uint16_t usNewParticle);
    NiPSysModifier* GetModifierByName(const NiFixedString& kName);
    // Inserts the modifier before the first one with a higher order (at the tail otherwise) and points it at
    // this system.
    void AddModifier(NiPSysModifier* pkModifier);
    // The number of modifiers.
    uint32_t GetModifierCount();
    // The uiIndex-th modifier, or null past the end.
    NiPSysModifier* GetModifierAt(uint32_t uiIndex);
    uint16_t GetMaxNumParticles();
    // When cloning, copies NiTriShape's members, then the world-space flag, a clone of every modifier, the last time,
    // the dynamic-bounds flag and a clone of the model data into pkDest. It touches only NiParticleSystem's members.
    void CopyMembers(NiParticleSystem* pkDest, NiCloningProcess& kCloning);

protected:
    // Empties the particle data, rewinds the clock and resets every modifier and modifier controller.
    void PerformSystemReset();

public:
    static NiObject* CreateObject();

    bool m_bWorldSpace;
    NiTPointerList<NiPointer<NiPSysModifier>> m_kModifierList;
    float m_fLastTime;
    bool m_bResetSystem;
    bool m_bDynamicBounds;
    NiTransform m_kUnmodifiedWorld;

    static NiRTTI ms_RTTI;
    static inline NiTaskManager::TaskPriority ms_eParallelUpdatePriority{NiTaskManager::IMMEDIATE};
    static inline bool ms_bParallelUpdateEnabled;
    static inline uint32_t iThreadID;
};

inline NiPSysModifier* NiParticleSystem::GetModifierAt(uint32_t uiIndex) {
    NiTListItem<NiPointer<NiPSysModifier>>* kIter;
    if (uiIndex >= m_kModifierList.m_uiCount) {
        return nullptr;
    }
    uint32_t uiCount = 0;
    kIter = m_kModifierList.m_pkHead;
    while (kIter) {
        NiPSysModifier* pkModifier = m_kModifierList.GetNext(kIter).m_pObject;
        if (uiCount++ == uiIndex) {
            return pkModifier;
        }
    }
    return nullptr;
}
