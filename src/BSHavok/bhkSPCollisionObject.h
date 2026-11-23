#pragma once

#include "BSHavok/bhkPCollisionObject.h"

// The collision object of a simple-shape phantom; adds no data to bhkPCollisionObject.
class bhkSPCollisionObject : public bhkPCollisionObject {
public:
    bhkSPCollisionObject();
    bhkSPCollisionObject(NiNode* apObject);
    ~bhkSPCollisionObject() override;

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

protected:
    // (both jump to the base's). The body below exists for this class's own clone.
    void CopyMembers(bhkSPCollisionObject* apDest, NiCloningProcess& arCloneData);

public:
    // The class loader's factory: a new object, or null when the allocation fails.
    static NiObject* CreateObject();

    static NiRTTI ms_RTTI;
};
