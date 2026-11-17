#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

class NiCloningProcess;

// Named data attached to a scene graph object through NiObjectNET.
class NiExtraData : public NiObject {
public:
	NiExtraData();
	// Named after kName (a reference taken).
	NiExtraData(const NiFixedString& kName);
	explicit NiExtraData(const NiFixedString* apName) : NiExtraData(*apName) {}
	~NiExtraData() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	virtual bool IsStreamable() const { return true; }
	virtual bool IsCloneable() const { return true; }

	// NiObject's part, then the name (used by the subclasses' CopyMembers).
	void CopyMembers(NiExtraData* pDest, NiCloningProcess& kCloning);

	const NiFixedString& GetName() const;
	void SetName(const NiFixedString& kName);
	// Zeroes the name's handle without releasing it (the pre-5.0 legacy chain reuses it as a next link).
	void ClearName();

	static NiObject* CreateObject();

	NiFixedString m_kName;

	static NiRTTI ms_RTTI;
};
