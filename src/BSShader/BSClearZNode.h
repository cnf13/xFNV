#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Reconstruction.hpp"

// A scene graph node that can override the camera's near and far clip distances for its subtree.
class BSClearZNode : public NiNode {
public:
	BSClearZNode();
	~BSClearZNode() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void OnVisible(NiCullingProcess& arCuller) override;

	bool bChangeFarClip;
	bool bChangeNearClip;
	uint8_t paddingAE[2];
	float fFarClip;
	float fNearClip;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable BSClearZNode_vftable{};
