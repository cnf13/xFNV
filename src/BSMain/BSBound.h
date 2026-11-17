#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

class NiObjectNET;

// The "BBX" extra data of a model: its bounding box as centre and half extents.
class BSBound : public NiExtraData {
public:
	BSBound();
	~BSBound() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;

	void GetBoundMin(NiPoint3& arPoint);
	void GetBoundMax(NiPoint3& arPoint);
	// NiExtraData's part, then the bound, into apDest; not virtual.
	void CopyMembers(BSBound* pDest, NiCloningProcess& arCloneData);

	static const NiFixedString& Tag();
	// The object's "BBX" extra data, or null when it has none or one of another class.
	static BSBound* Find(NiObjectNET* apObject);
	// Creates the "BBX" tag.
	static void InitSDM();
	// Releases the "BBX" tag and frees it, leaving pTag null.
	static void KillSDM();
	// The class loader's factory: a new bound with an empty name, zero centre and extents.
	static NiObject* CreateObject();

	NiPoint3 Center;
	NiPoint3 Extents;

	static inline NiFixedString* pTag;
	static NiRTTI ms_RTTI;
};
