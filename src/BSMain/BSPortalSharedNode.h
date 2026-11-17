#pragma once

#include "BSCore/BSSimpleArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class BSCompoundFrustum;
class BSPortal;
class ShadowSceneLight;

// The node a portal's geometry hangs from: the compound frustum seen through the portal and the lights that shine
// through it. It is never constructed in src.
class BSPortalSharedNode : public NiNode {
public:
	BSPortalSharedNode(BSPortal* apPortal);

	const NiRTTI* GetRTTI() const override { return &BSPortalSharedNode::ms_RTTI; }
	// The exe's own override, at vtable slot 18 (thiscall, 0xC4F620).
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	// The exe's own override, at vtable slot 53 (thiscall, 0xC4F6D0).
	void OnVisible(NiCullingProcess& kCuller) override;
	~BSPortalSharedNode() override;

	BSCompoundFrustum* QCompoundFrustum();

protected:
	void CopyMembers(BSPortalSharedNode* pDest, NiCloningProcess& arCloneData);

public:
	BSCompoundFrustum* pCompoundFrustum;
	BSSimpleArray<ShadowSceneLight*, 1024> kLights;
	BSPortal* pPortal;

	static NiRTTI ms_RTTI;
};

NiSmartPointer(BSPortalSharedNode);

inline constexpr fnvd::Vtable BSPortalSharedNode_vftable{};
