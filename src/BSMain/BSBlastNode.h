#pragma once

#include "BSMain/BSRangeNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

// a range node for blast effects
class BSBlastNode : public BSRangeNode {
public:
	BSBlastNode();

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;

protected:
	void CopyMembers(BSBlastNode* pDest, NiCloningProcess& arCloneData);

public:
	// The class loader's factory: a new blast node, or null when the allocation fails.
	static NiObject* CreateObject();

	static NiRTTI ms_RTTI;
};
