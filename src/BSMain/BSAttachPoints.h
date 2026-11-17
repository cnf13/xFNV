#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"

class NiCloningProcess;

// A parent/child pair of attach point names.
class BSAttachPointPair {
public:
	BSAttachPointPair(const NiFixedString& arParent, const NiFixedString& arChild);
	~BSAttachPointPair();

	// Assigns the parent name.
	void SetParent(const NiFixedString& arParent);
	// Assigns the child name.
	void SetChild(const NiFixedString& arChild);

	NiFixedString Parent;
	NiFixedString Child;
};

// Extra data on a model that lists its named attach points (the nodes other models are attached to).
class BSAttachPoints : public NiExtraData {
public:
	BSAttachPoints();
	~BSAttachPoints() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) override;
	virtual void CopyMembers(BSAttachPoints* pDest, NiCloningProcess& arCloneData);

	using NiExtraData::CopyMembers;

	// Adds the pair of strings.
	void AddPair(const NiFixedString& arParent, const NiFixedString& arChild);

	// Creates the "ATP" tag.
	static void InitSDM();
	static NiObject* CreateObject();
	static void KillSDM();

	NiTPrimitiveArray<BSAttachPointPair*> pAttachPointsA;

	static inline NiFixedString* pTag;
	static NiRTTI ms_RTTI;
};
