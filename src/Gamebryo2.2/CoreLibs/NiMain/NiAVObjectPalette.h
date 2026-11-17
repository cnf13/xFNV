#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Reconstruction.hpp"

class NiAVObject;
class NiFixedString;

// abstract name -> NiAVObject lookup that animation sequences bind their targets through
class NiAVObjectPalette : public NiObject {
public:
	~NiAVObjectPalette() override;

	const NiRTTI* GetRTTI() const override;
	// LoadBinary, LinkObject and SaveBinary fold into jumps to the NiObject bodies
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	virtual NiAVObject* GetAVObject(const NiFixedString& akName) = 0;
	virtual void SetAVObject(NiFixedString& akName, NiAVObject* apObject) = 0;

	// Same function as NiAccumulator::CopyMembers (0xAA1000), registered there: a jmp to NiObject::CopyMembers.
	void CopyMembers(NiAVObjectPalette* pkDest, NiCloningProcess& kCloning);

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiAVObjectPalette_vftable{};
