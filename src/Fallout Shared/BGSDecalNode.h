#pragma once

#include "BSCore/BSSimpleList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

// The node holding a reference's decals.
class BGSDecalNode : public NiNode {
public:
    // The decals a node holds: smart pointers, so the list's own instances release them.
    using DecalList = BSSimpleList<NiPointer<NiRefObject>>;

    // The default constructor, CreateClone's.
    BGSDecalNode();
    // A decal node under apNode, skinned when abSkinnedNode.
    BGSDecalNode(NiNode* apNode, bool abSkinnedNode);

    const NiRTTI* GetRTTI() const override {
        return &BGSDecalNode::ms_RTTI;
    }
    virtual bool IsDecalNode() {
        return true;
    }
    void AttachChild(NiAVObject* apChild, int32_t abFirstAvail) override;
    NiObject* CreateClone(NiCloningProcess& arCloneData) override;
    void ProcessClone(NiCloningProcess& kCloning) override {}
    void OnVisible(NiCullingProcess& arCuller) override;
    virtual void AttachChild(BSTempEffect* apDecal, bool bFirstAvail);
    ~BGSDecalNode() override;

    // Removes apDecal from the node's decal list.
    void RemoveFromDecalList(BSTempEffect* apDecal);
    // Removes apDecal from the decal list and detaches its 3D (BSTempEffect::Get3D) from the node, releasing the
    // detached child. Not virtual.
    void DetachChild(BSTempEffect* apDecal);
    // Removes apDecal from the decal list and detaches its 3D (BSTempEffect::Get3D) through NiNode::DetachChild, which
    // hands the detached child back in aspAVObject. Not virtual.
    void DetachChild(BSTempEffect* apDecal, NiPointer<NiAVObject>& aspAVObject);
    // Gives the node an empty decal list and names it a (skinned) decal node.
    void Init();
    void RemoveDecals();

    static void RemoveAllDecals();

    // The decals attached to the node.
    DecalList* pDecalList;
    bool bSkinnedNode;

    static NiRTTI ms_RTTI;
    // The nodes that hold decals; the items are the nodes themselves.
    static BSSimpleList<BGSDecalNode*> kDecalNodes;
};
