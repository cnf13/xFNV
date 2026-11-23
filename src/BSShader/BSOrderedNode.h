#pragma once

#include "BSMain/BSNiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

// A scene graph node whose children are drawn in order.
class BSOrderedNode : public BSNiNode {
public:
    BSOrderedNode();
    ~BSOrderedNode() override;

    const NiRTTI* GetRTTI() const override;
    NiObject* CreateClone(NiCloningProcess& kCloning) override;
    void LoadBinary(NiStream& kStream) override;
    void LinkObject(NiStream& kStream) override;
    bool RegisterStreamables(NiStream& kStream) override;
    void SaveBinary(NiStream& kStream) override;
    bool IsEqual(NiObject* pkObject) override;
    void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
    void OnVisible(NiCullingProcess& arCuller) override;

    void CopyMembers(BSOrderedNode* apDest, NiCloningProcess& arCloneData);

    // Stream factory for this class: a new node built by BSOrderedNode::BSOrderedNode.
    static NiObject* CreateObject();

    NiBound StaticBound;
    bool bHasStaticBound;

    static NiRTTI ms_RTTI;
};
