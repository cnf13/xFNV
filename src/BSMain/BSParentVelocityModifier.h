#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

class NiCloningProcess;
class NiPSysData;
class NiStream;

// gives particles the velocity of the object they come from.
class BSParentVelocityModifier : public NiPSysModifier {
public:
    BSParentVelocityModifier();
    BSParentVelocityModifier(const char* apcName);
    ~BSParentVelocityModifier() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream& arStream) override;
    bool RegisterStreamables(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    bool IsEqual(NiObject* apObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    void Update(float afTime, NiPSysData* apData) override;
    void Initialize(NiPSysData* apData, uint16_t aiNewParticle) override;
    void HandleReset() override;

protected:
    void CopyMembers(BSParentVelocityModifier* apDest, NiCloningProcess& arCloneData);

public:
    static NiObject* CreateObject();

    float fLastTimeDelta;
    float fTimeLast;
    float fDamping;
    NiPoint3 pntLast;
    NiPoint3 velLast;

    static NiRTTI ms_RTTI;
};
