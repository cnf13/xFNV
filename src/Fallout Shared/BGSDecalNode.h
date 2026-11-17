#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "BSCore/BSSimpleList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

// The node holding a reference's decals.
class BGSDecalNode : public NiNode {
public:
	// The decals a node holds: smart pointers, so the list's own instances release them.
	using DecalList = BSSimpleList<NiPointer<NiRefObject>>;

	// The default constructor, CreateClone's.
	BGSDecalNode();
	// A decal node under apNode, skinned when abSkinnedNode.
	BGSDecalNode(NiNode* apNode, bool abSkinnedNode);

	const NiRTTI* GetRTTI() const override { return &BGSDecalNode::ms_RTTI; }
	// The exe's entry is 0x8D0360, the return-true stub many classes share; not registered.
	virtual bool IsDecalNode() { return true; }
	void AttachChild(NiAVObject* apChild, int32_t abFirstAvail) override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	// The exe's entry is 0x4534F0, the empty stub many classes share; not registered.
	void ProcessClone(NiCloningProcess& kCloning) override {}
	void OnVisible(NiCullingProcess& arCuller) override;
	virtual void AttachChild(BSTempEffect* apDecal, bool bFirstAvail);
	~BGSDecalNode() override;

	// Removes apDecal from the node's decal list.
	void RemoveFromDecalList(BSTempEffect* apDecal);
	// Removes apDecal from the decal list and detaches its 3D (BSTempEffect::Get3D) from the
	// node, releasing the detached child (thiscall, ret 4, 0x4EE560). Not virtual: no exe vtable holds it.
	void DetachChild(BSTempEffect* apDecal);
	// The PDB's DetachChild(apDecal, aspAVObject): removes apDecal from the decal list and
	// detaches its 3D (BSTempEffect::Get3D, slot 38) through NiNode::DetachChild 0xA5E780, which hands the detached
	// child back in aspAVObject (thiscall, ret 8, 0x4EE5D0; IDA DetachChild_0). DetachChild 0x4EE560 calls it. Not
	// virtual: no exe vtable holds it.
	void DetachChild(BSTempEffect* apDecal, NiPointer<NiAVObject>& aspAVObject);
	// Gives the node an empty decal list and names it a (skinned) decal node.
	void Init();
	void RemoveDecals();

	// Frees a decal list through its deleting destructor, the NiPointer instantiation
	// (thiscall, ret 4, 0x4EE360); the exe's callers all pass the flags 1.
	static DecalList* FreeDecalList(DecalList* apList, uint32_t auiFlags);
	static void RemoveAllDecals();

	// The decals attached to the node.
	DecalList* pDecalList;
	bool bSkinnedNode;
	uint8_t padding_B1[3];

	static NiRTTI ms_RTTI;
	// The nodes that hold decals (the PDB's kDecalNodes); the items are the nodes themselves.
	static BSSimpleList<BGSDecalNode*> kDecalNodes;
};

// IDA name; not in the PDB. A copy of the NiPointer list constructor (thiscall, ret 0, 0x4EE810) that the exe also
// calls for other NiPointer lists (bhkEntityListener, bhkRigidBody, TES): the item, then the link, are cleared.
BGSDecalNode::DecalList* DecalList_Construct(void* apMemory);

inline constexpr fnvd::Vtable BGSDecalNode_vftable{};
