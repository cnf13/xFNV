#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Reconstruction.hpp"

// a node that overrides how the accumulator sorts its subtree
class NiSortAdjustNode : public NiNode {
public:
	enum SortingMode : int32_t {
		SORTING_INHERIT = 0,
		SORTING_OFF = 1,
		SORTING_MAX = 2,
	};

	NiSortAdjustNode();
	~NiSortAdjustNode() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& stream) override;
	void LinkObject(NiStream& stream) override;
	// folded with NiSwitchNode::RegisterStreamables: both jump to the NiNode body
	bool RegisterStreamables(NiStream& stream) override;
	void SaveBinary(NiStream& stream) override;
	bool IsEqual(NiObject* pObject) override;
	// folds into a jump to the NiNode body
	void ProcessClone(NiCloningProcess& kCloning) override;
	void OnVisible(NiCullingProcess& kCuller) override;

	static NiObject* CreateObject();

	SortingMode m_eSortingMode;

	static NiRTTI ms_RTTI;
};

inline constexpr fnvd::Vtable NiSortAdjustNode_vftable{};
