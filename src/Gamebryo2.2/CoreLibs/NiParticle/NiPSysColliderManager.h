#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

class NiCloningProcess;
class NiPSysCollider;
class NiPSysData;
class NiStream;

// runs the colliders of its particle system.
class NiPSysColliderManager : public NiPSysModifier {
public:
    NiPSysColliderManager();
    ~NiPSysColliderManager() override;

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
    void Update(float fTime, NiPSysData* pkData) override;

    // NiPSysModifier's copy, then a clone of the collider list into pkDest when cloning.
    void CopyMembers(NiPSysColliderManager* pkDest, NiCloningProcess& kCloning);

    static NiObject* CreateObject();

    NiPointer<NiPSysCollider> m_spColliders;

    static NiRTTI ms_RTTI;
};
