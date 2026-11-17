#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

class NiAVObject;
class NiCamera;
class NiGeometry;
class NiVisibleArray;

// Base of the classes that collect geometry during a scene traversal and draw it afterwards.
class NiAccumulator : public NiObject {
public:
    NiAccumulator();
    ~NiAccumulator() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    virtual void StartAccumulating(const NiCamera* pkCamera);
    // The derived FinishAccumulating bodies call this base version.
    virtual void FinishAccumulating();
    virtual void RegisterObjectArray(NiVisibleArray& arArray) = 0;
    // shared "return false", "return true" and empty stubs
    virtual bool RegisterObject(NiGeometry*) {
        return false;
    }
    virtual bool AddShared(NiAVObject*) {
        return true;
    }
    virtual void ClearSharedMap() {}

    bool IsAccumulating() const;
    // Only calls NiObject::CopyMembers.
    void CopyMembers(NiAccumulator* pkDest, NiCloningProcess& kCloning);

    NiCamera* m_pkCamera;

    static NiRTTI ms_RTTI;
};
