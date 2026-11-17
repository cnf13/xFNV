#pragma once

#include "BSMain/BSNiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

// a node whose children show only while its current level lies inside [cMin, cMax]
class BSRangeNode : public BSNiNode {
public:
	// Called with the node and the new visibility whenever a range node changes it.
	using VisibilityChangedCallback = void (*)(NiNode*, bool);

	BSRangeNode();
	~BSRangeNode() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	virtual void ParseNameForRanges();

	// moves to level acLevel and shows or hides the children for it
	bool SetCurrent(uint8_t acVal);
	void CopyMembers(BSRangeNode* pDest, NiCloningProcess& arCloneData);

	// The class loader's factory: a new range node.
	static NiObject* CreateObject();
	static void SetVisibilityChangedCB(void (*apVisibilityChangedCB)(NiNode*, bool));

	bool bEnabled;
	// The levels compare unsigned.
	uint8_t cCurrent;
	uint8_t cMin;
	uint8_t cMax;

	static NiRTTI ms_RTTI;
	static inline VisibilityChangedCallback pVisibilityChangedCB{};
};
