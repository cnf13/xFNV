#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Reconstruction.hpp"

class NiCloningProcess;

// Named data attached to a scene graph object through NiObjectNET.
class NiExtraData : public NiObject {
public:
	NiExtraData();
	// Named after kName (a reference taken).
	NiExtraData(const NiFixedString& kName);
	// Named after *apName (the hybrid's Construct from a pointer, 0xA7B440).
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

	// thiscall, ret 0, 0xA62660: the exe calls this getter out of line (one body shared by
	// every getter of a field at +0x8).
	const NiFixedString& GetName() const;
	// The exe's NiExtraData derives from NiObjectNET, so its name field is NiObjectNET's; this
	// is the body the exe calls for it (0xA5B950, thiscall, ret 4; the PDB's NiObjectNET::SetName).
	void SetName(const NiFixedString& kName);
	// Zeroes the name's handle without releasing it (the pre-5.0 legacy chain reuses it as a
	// next link). The exe calls it out of line (thiscall, ret 0).
	void ClearName();

	static NiObject* CreateObject();

	NiFixedString m_kName;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiExtraData_vftable{};
