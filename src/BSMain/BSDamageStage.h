#pragma once

#include "BSMain/BSRangeNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Reconstruction.hpp"

// a range node for one damage stage; its update passes skip app-culled subtrees
class BSDamageStage : public BSRangeNode {
public:
	BSDamageStage();

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	void UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
	void UpdateSelectedDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
	void UpdateRigidDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;

protected:
	void CopyMembers(BSDamageStage* pDest, NiCloningProcess& arCloneData);

public:
	// The class loader's factory: a new damage stage, or null when the allocation fails.
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable BSDamageStage_vftable{};
