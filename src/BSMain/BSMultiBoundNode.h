#pragma once

#include "BSMain/BSMultiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTDefaultAllocator.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "BSMain/BSCullingProcess.h"
#include "BSMain/BSNiNode.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class BSMultiBoundRoom;
class BSOcclusionPlane;

// A scene graph node bound to a multibound volume, which decides how its children are culled.
class BSMultiBoundNode : public BSNiNode {
public:
	BSMultiBoundNode();
	// a1 is the node whose children the new node takes.
	BSMultiBoundNode(NiNode* a1);
	~BSMultiBoundNode() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	BSMultiBoundNode* IsMultiBoundNode() override { return this; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
	void UpdateWorldBound() override;
	void OnVisible(NiCullingProcess& kCuller) override;
	void UpdateUpwardPass() override;
	virtual BSMultiBoundRoom* GetMultiBoundRoom() { return nullptr; }
	virtual bool QPointWithin(NiPoint3& aPoint);
	// the overloads sit in the vtable in reverse declaration order: the BSMultiBound one is slot 67 (0xC475C0 calls
	// shape slot 37, whose bodies load the argument's shape pointer), the NiBound one slot 68 (calls shape slot 38)
	virtual BSMultiBoundShape::BSMBIntersectResult CheckBound(const NiBound& aTargetBound);
	virtual BSMultiBoundShape::BSMBIntersectResult CheckBound(const BSMultiBound& aTargetBound);

	void SetMultiBound(BSMultiBound* apMultiBound);
	// spMultiBound; the exe's body is the shared one-field getter 0x6629F0 (thiscall).
	BSMultiBound* QMultiBound();
	void SetCullingMode(BSCullingProcess::BSCPCullingType aCullingMode);
	// BSNiNode's part, then the multibound (cloned) and the culling mode, into apDest; not
	// virtual.
	void CopyMembers(BSMultiBoundNode* pDest, NiCloningProcess& arCloneData);
	bool VisibleAgainstActiveOccluders(NiPoint3 akViewPoint, NiCullingProcess& akCuller, BSOcclusionPlane* apOccluder);

	static void RegisterRTTI();
	static NiObject* CreateObject();
	static BSMultiBoundNode* FindMultiboundNode(NiAVObject* apObject);

	NiPointer<BSMultiBound> spMultiBound;
	BSCullingProcess::BSCPCullingType CullingMode;

	static NiRTTI ms_RTTI;
};

NiSmartPointer(BSMultiBoundNode);

inline constexpr fnvd::Vtable BSMultiBoundNode_vftable{};
