#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"

#include <stdint.h>

// Base of lights and projected-texture effects: which nodes the effect affects.
class NiDynamicEffect : public NiAVObject {
public:
    enum EffectType : uint8_t {
        AMBIENT_LIGHT = 0,
        POINT_LIGHT = 2,
        DIR_LIGHT = 3,
        SPOT_LIGHT = 4,
        TEXTURE_EFFECT = 5,
    };

    NiDynamicEffect();
    ~NiDynamicEffect() override;

    const NiRTTI* GetRTTI() const override {
        return &ms_RTTI;
    }
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;

    void IncRevisionID();
    void DetachAllUnaffectedNodes();
    void CopyMembers(NiDynamicEffect* pkDest, NiCloningProcess& kCloning);
    // Empty: this function does nothing.
    void AttachAffectedNode(NiNode* pkAffectedNode);

    bool m_bOn;
    // an EffectType
    uint8_t m_ucEffectType;
    uint8_t padding9E[2];
    int32_t m_iIndex;
    uint32_t m_uiPushCount;
    uint32_t m_uiRevID;
    NiTPointerList<NiNode*> m_kAffectedNodeList;
    NiTPointerList<NiNode*> m_kUnaffectedNodeList;

    static NiRTTI ms_RTTI;
    static inline int32_t ms_iNextIndex = 1;
};
