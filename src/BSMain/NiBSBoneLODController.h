#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTSet.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTimeController.h"
#include "Reconstruction.hpp"

class NiNode;

// Switches the bones of a skinned object between levels of detail.
class NiBSBoneLODController : public NiTimeController {
public:
	NiBSBoneLODController();
	~NiBSBoneLODController() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void Update(NiUpdateData&) override {}
	virtual bool CanBeExtracted() { return false; }

protected:
	void ProcessClone(NiCloningProcess& arCloneData) override;
	void PostLinkObject(NiStream& kStream) override;
	bool TargetIsRequiredType() const override;

public:
	// The level of detail the bones are at.
	int32_t GetBoneLOD() const;
	// Switches the bones to LOD iLOD (-1: all off); false when there are no LODs, iLOD is current or out of range.
	bool SetBoneLOD(int32_t iLOD);
	void CleanUp();
	// Copies the base's members and the LOD state into the clone.
	void CopyMembers(NiBSBoneLODController* pkDest, NiCloningProcess& arCloneData);
	bool InitializeData(NiNode* pkBoneRoot);
	void CollectMarkedBones(NiNode* pkBone, NiTPointerMap<NiNode*, uint32_t>& kBoneToLOD,
		NiTPointerMap<uint32_t, bool>& kUsedLODs);
	void BuildBoneMap(NiNode* pkBone, NiTPointerMap<NiNode*, uint32_t>& kBoneToLOD, uint32_t& uiCurrentLOD);
	bool GetHasBones();

	static NiObject* CreateObject();
	static bool ProcessScene(NiNode* pkSceneRoot);
	static void FindBoneRootNodes(NiNode* pkNode, NiTPrimitiveSet<NiNode*>& kBoneRootNodes);
	static void InitLoaders();

	int32_t m_iLOD;
	uint32_t m_uiNumLODs;
	NiTPrimitiveArray<NiTPrimitiveSet<NiNode*>*> m_kBoneArray;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiBSBoneLODController_vftable{};
