#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiFloatController.h"

// Rolls its target about its local x axis by the float data.
class NiRollController : public NiFloatController {
public:
    NiRollController();
    ~NiRollController() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& stream) override;
    void LinkObject(NiStream& stream) override;
    bool RegisterStreamables(NiStream& stream) override;
    void SaveBinary(NiStream& stream) override;
    bool IsEqual(NiObject* pObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;
    void Update(NiUpdateData& UpdateData) override;
    bool IsTransformController() const override {
        return true;
    }

protected:
    bool TargetIsRequiredType() const override;

public:
    void CopyMembers(NiRollController* pDest, NiCloningProcess& kCloning);

    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
};
