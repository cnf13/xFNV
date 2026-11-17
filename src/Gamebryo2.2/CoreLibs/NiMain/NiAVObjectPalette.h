#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

class NiAVObject;
class NiFixedString;

// abstract name -> NiAVObject lookup that animation sequences bind their targets through
class NiAVObjectPalette : public NiObject {
public:
	~NiAVObjectPalette() override;

	const NiRTTI* GetRTTI() const override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	virtual NiAVObject* GetAVObject(const NiFixedString& akName) = 0;
	virtual void SetAVObject(NiFixedString& akName, NiAVObject* apObject) = 0;

	// Only calls NiObject::CopyMembers.
	void CopyMembers(NiAVObjectPalette* pkDest, NiCloningProcess& kCloning);

	static NiRTTI ms_RTTI;
};
