#pragma once

#include "BSMain/BSOcclusionPlane.h"
#include "BSMain/BSPortalSharedNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class BSMultiBoundRoom;

// An occlusion plane that joins two rooms; the geometry seen through it hangs from its shared node.
class BSPortal : public BSOcclusionPlane {
public:
	BSPortal();
	BSPortal(NiPoint3* apPosition, NiPoint2* apBounds, NiMatrix3* apMatrix);

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	~BSPortal() override;

	// Set the rooms on either side: pMultiBoundRoom[0] and [1].
	void SetMultiBoundRoomLeft(BSMultiBoundRoom* apRoom);
	void SetMultiBoundRoomRight(BSMultiBoundRoom* apRoom);
	// spSharedGeometry = apNode.
	void SetSharedGeometryNode(BSPortalSharedNode* apNode);
	BSPortalSharedNode* QSharedGeometryNode();
	uint32_t AnyRoomsVisible();
	// The portal's members into apDest; not virtual.
	void CopyMembers(BSPortal* pDest, NiCloningProcess& arCloneData);

	// The two rooms the portal joins.
	BSMultiBoundRoom* pMultiBoundRoom[2];
	NiPointer<BSPortalSharedNode> spSharedGeometry;

	static NiRTTI ms_RTTI;
};
