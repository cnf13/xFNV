#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiLODData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSwitchNode.h"

// a switch node that picks its child from the camera distance through its LOD data
class NiLODNode : public NiSwitchNode {
public:
	NiLODNode();
	~NiLODNode() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void UpdateWorldData(NiUpdateData& UpdateData) override;
	void OnVisible(NiCullingProcess& kCuller) override;

	// Updates the children whose revision is behind.
	void UpdateAllChildren();
	void SetLODData(NiLODData* pkLODData);
	void CopyMembers(NiLODNode* pkDest, NiCloningProcess& kCloning);

	static NiObject* CreateObject();

	NiPointer<NiLODData> m_spLODData;
	bool m_bLODActive;

	static NiRTTI ms_RTTI;
	// forced LOD level for every node; negative lets each node choose from the camera distance
	static inline int32_t ms_iGlobalLOD = -1;
};
