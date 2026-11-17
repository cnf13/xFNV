#pragma once

#include "BSHavok/bhkNiCollisionObject.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"

// The collision object of a phantom; adds no data to bhkNiCollisionObject.
class bhkPCollisionObject : public bhkNiCollisionObject {
public:
    bhkPCollisionObject();
    bhkPCollisionObject(NiAVObject* apObject);
    ~bhkPCollisionObject() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream& arStream) override;
    bool RegisterStreamables(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    bool IsEqual(NiObject* apObject) override;
    void AddViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    void hktoNiTransform() override;
    void NitohkTransform() override;
    void ClearVelocities() override {}

    void
    SetMotionType(hkpMotion::MotionType /*aeType*/, bhkWorldObject* /*apObject*/, bool /*abAllowActivate*/) override {}

    // Always true.
    bool QKeyframe() override;

protected:
    // ; this class's clone calls it at that address.
    void CopyMembers(bhkPCollisionObject* apDest, NiCloningProcess& arCloneData);

public:
    // The class loader's factory: a new object, or null when the allocation fails.
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
};
