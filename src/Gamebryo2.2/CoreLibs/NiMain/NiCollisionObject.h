#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"

class NiAVObject;
struct NiUpdateData;

// Links a scene graph object to its collision representation.
class NiCollisionObject : public NiObject {
public:
    // What NiAVObject::LoadBinary hands Initialize for a pre-5.0.0.19 file's collision data.
    struct OldNifInit {
        NiAVObject* m_pkAVObject;
        NiStream* m_pkStream;
        bool m_bABV;
        uint16_t m_usProgagationBits;
    };

    NiCollisionObject();
    NiCollisionObject(NiAVObject* pkSceneObject);
    ~NiCollisionObject() override = 0;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void AddViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    virtual void SetSceneGraphObject(NiAVObject* pkSceneObject);
    virtual void UpdateWorldData(NiUpdateData& arUpdateData) = 0;
    virtual void RecreateWorldData() = 0;
    virtual void Initialize(void*) {}
    virtual void Convert(uint32_t, void*) {}

    NiAVObject* GetSceneGraphObject();

    NiAVObject* m_pkSceneObject;

    static NiRTTI ms_RTTI;
};

inline NiAVObject* NiCollisionObject::GetSceneGraphObject() {
    return m_pkSceneObject;
}
