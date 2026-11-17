#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Reconstruction.hpp"

// a node that draws only one of its children, chosen by index
class NiSwitchNode : public NiNode {
public:
	enum : int32_t {
		MAX_POS = 9,
	};

	enum : int32_t {
		UPDATE_ONLY_ACTIVE_CHILD_MASK = 1,
		UPDATECONTROLLERS_MASK = 2,
	};

	// An empty switch node showing child -1.
	NiSwitchNode();
	~NiSwitchNode() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	// LinkObject and RegisterStreamables fold into jumps to the NiNode bodies
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void UpdateControllers(NiUpdateData& UpdateData) override;
	void UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
	void UpdateSelectedDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
	void UpdateRigidDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
	void UpdateWorldBound() override;
	void OnVisible(NiCullingProcess& kCuller) override;
	void AttachChild(NiAVObject* pkChild, int32_t bFirstAvail) override;
	void DetachChild(NiAVObject* pkChild, NiPointer<NiAVObject>& aspAVObject) override;
	void DetachChildAt(uint32_t i, NiPointer<NiAVObject>& aspAVObject) override;
	void SetAt(uint32_t i, NiAVObject* pkChild, NiPointer<NiAVObject>& aspAVObject) override;
	using NiNode::DetachChild;
	using NiNode::DetachChildAt;
	using NiNode::SetAt;

	// thiscall, ret 0, 0xB5AA40: the child at m_iIndex, or null when the index is out of range.
	NiAVObject* GetActiveChild() const;

protected:
	void CopyMembers(NiSwitchNode* pkDest, NiCloningProcess& kCloning);

public:
	static NiObject* CreateObject();

	uint16_t m_uFlags;
	int32_t m_iIndex;
	float m_fSavedTime;
	uint32_t m_uiRevID;
	NiTPrimitiveArray<uint32_t> m_kChildRevID;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiSwitchNode_vftable{};
