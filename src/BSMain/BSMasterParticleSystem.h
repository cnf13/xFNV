#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"

class NiCloningProcess;
class NiParticles;
class NiStream;
struct NiUpdateData;

// a node that drives a set of particle systems under it with one shared particle cap and clock.
class BSMasterParticleSystem : public NiNode {
public:
    // Counts collected for the master and its child particle systems.
    class DebugInfo {
    public:
        uint32_t uiParticleSystemCount;
        uint32_t uiActiveParticleCount;
        uint32_t uiMaxParticleCount;
        uint32_t uiActiveEmitterObjCount;
        uint32_t uiMaxEmitterObjCount;
        uint32_t uiModifierCount;
    };

    BSMasterParticleSystem();
    ~BSMasterParticleSystem() override;

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
    void PostLinkObject(NiStream& kStream) override;
    void UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
    void UpdateSelectedDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
    void UpdateRigidDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;

    // Gathers the particle geometry under the node into ChildParticles, replacing the old list.
    void UpdateParticleSystemList();

    // The emitter object at the round-robin iterator, which moves on and wraps to the head of the list.
    NiAVObject* GetNextEmitterObj() {
        NiAVObject* pkObject = lEmitterObjList.GetNext(FirstEmitterObjIter);
        if (!FirstEmitterObjIter) {
            FirstEmitterObjIter = lEmitterObjList.GetHeadPos();
        }
        return pkObject;
    }

    void SetEditorIndex(uint32_t auiEditorIndex);
    void SetIndex(uint32_t auiIndex);
    void SetMaxParticlesPerFrame(uint16_t ausMaxParticlesPerFrame);
    void ClearAllParticles();
    void ClearAllEmitterObj();
    void SetMaxEmitterObj(uint16_t ausMaxEmitterObj);

protected:
    // Drops the emitter objects nobody else references or that are culled, recounts the active ones (at most
    // usMaxEmitterObj) and parks the iterator on the first one past the cap.
    void ClearUnreferencedEmitters();
    // Whether any of ChildParticles has live particles.
    bool ChildHasActiveParticles();
    // Appends apObject's particle geometry, or that of its subtree when it is a node, to apNiParticles.
    void FindNiParticlesRecurse(NiAVObject* apObject, NiTPrimitiveArray<NiParticles*>* apNiParticles);
    void CopyMembers(BSMasterParticleSystem* apDest, NiCloningProcess& arCloneData);

public:
    void AddEmitterObj(NiAVObject* apEmitterObj);
    void GetDebugInfo(BSMasterParticleSystem::DebugInfo& apInfo);

    static NiObject* CreateObject();
    static bool IsMasterParticleSystem(NiParticles* apSystem);

    NiTPointerList<NiPointer<NiAVObject>> lEmitterObjList;
    uint16_t usActiveEmitterObjCount;
    uint16_t usMaxEmitterObj;
    uint16_t usMaxParticlesPerFrame;
    NiTListItem<NiPointer<NiAVObject>>* FirstEmitterObjIter;
    uint32_t uiIndex;
    uint32_t uiEditorIndex;
    NiTPrimitiveArray<NiParticles*> ChildParticles;
    float fMasterPSysTime;

    static NiRTTI ms_RTTI;
};
