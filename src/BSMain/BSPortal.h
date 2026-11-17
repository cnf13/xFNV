#pragma once

#include "BSMain/BSOcclusionPlane.h"
#include "BSMain/BSPortalSharedNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class BSMultiBoundRoom;

// An occlusion plane that joins two rooms; the geometry seen through it hangs from its shared node.
// It is never constructed in src.
class BSPortal : public BSOcclusionPlane {
public:
	BSPortal();
	BSPortal(NiPoint3* apPosition, NiPoint2* apBounds, NiMatrix3* apMatrix);

	const NiRTTI* GetRTTI() const override;
	// NiObject slot 18.
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	~BSPortal() override;

	// Set the rooms on either side: pMultiBoundRoom[0] and [1] (thiscall, ret 4, the exe's
	// shared +0xFC and +0x100 setters 0x9707F0 and 0x420BA0).
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
inline constexpr fnvd::Vtable BSPortal_vftable{};
