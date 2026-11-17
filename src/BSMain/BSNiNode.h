#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

// base of the engine's own scene graph node types; adds no fields to NiNode
class BSNiNode : public NiNode {
public:
	BSNiNode();
	BSNiNode(NiNode* apNode, bool abKeepChildren);
	~BSNiNode() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	virtual void ReparentSkinInstances(NiNode* apRoot, NiNode* apNewParent);

	void CopyMembers(BSNiNode* pDest, NiCloningProcess& arCloneData);

	static void SetNewName(NiNode* apNode, const char* apAppend);

	static NiRTTI ms_RTTI;
};
