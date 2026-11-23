#pragma once

#include "Gamebryo2.2/CoreLibs/NiAnimation/NiTextKey.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "SDK/MSVC/ehvec.h"

// Extra data holding the text keys of an animation.
class NiTextKeyExtraData : public NiExtraData {
public:
    NiTextKeyExtraData();
    ~NiTextKeyExtraData() override;

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

    NiTextKey* GetKeys(uint32_t& uiNumKeys) const;
    // Copies NiExtraData's part, then a copy of the keys; CreateClone calls it on this with the new object.
    void CopyMembers(NiTextKeyExtraData* pDest, NiCloningProcess& kCloning);

    static NiObject* CreateObject();

    uint32_t m_uiNumKeys;
    NiTextKey* m_pKeys;

    static NiRTTI ms_RTTI;
};
