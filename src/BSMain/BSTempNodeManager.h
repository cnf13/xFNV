#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

struct NiUpdateData;

// The node the temporary effects hang under: each update first removes the children whose time is up.
class BSTempNodeManager : public NiNode {
public:
	BSTempNodeManager();

	~BSTempNodeManager() override;

	const NiRTTI* GetRTTI() const override;
	void UpdateDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
	void UpdateSelectedDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
	void UpdateRigidDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;

protected:
	// Removes the children that expired by afTime.
	void RemoveExpired(float afTime);

public:
	float fPrevTime;

	static NiRTTI ms_RTTI;
};
