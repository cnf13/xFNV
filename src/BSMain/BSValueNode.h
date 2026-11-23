#pragma once

#include "BSMain/BSNiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

class NiBillboardNode;
class NiCloningProcess;

// A scene graph node that carries an integer value.
class BSValueNode : public BSNiNode {
public:
    enum VALUE_NODE_FLAGS : uint8_t {
        FLAG_BILLBOARD = 0x1,
    };

    BSValueNode();
    BSValueNode(NiBillboardNode* apNode);
    ~BSValueNode() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void LoadBinary(NiStream& arStream) override;
    void LinkObject(NiStream& arStream) override;
    bool RegisterStreamables(NiStream& arStream) override;
    void SaveBinary(NiStream& arStream) override;
    bool IsEqual(NiObject* apObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
    void UpdateWorldData(NiUpdateData& UpdateData) override;
    virtual bool ParseNameForValue();

    void CopyMembers(BSValueNode* pDest, NiCloningProcess& arCloneData);
    // Returns the node's value (iValue).
    int32_t QValue();

    static NiObject* CreateObject();

    uint8_t m_uFlags;
    int32_t iValue;
    NiObject* spReservedObject;

    static NiRTTI ms_RTTI;
};
