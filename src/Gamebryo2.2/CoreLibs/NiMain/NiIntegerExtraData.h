#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Reconstruction.hpp"

// Extra data holding one integer.
class NiIntegerExtraData : public NiExtraData {
public:
	NiIntegerExtraData() : m_iValue(0) {}
	// The value iValue: the base and value stores the exe inlines into the subclasses' constructors.
	explicit NiIntegerExtraData(int32_t iValue) : m_iValue(iValue) {}
	~NiIntegerExtraData() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	// Also NiFloatExtraData::LoadBinary (the two bodies are one function).
	void LoadBinary(NiStream& kStream) override;
	// Shared jmp thunk to NiExtraData::LinkObject, used by most NiExtraData subclasses.
	void LinkObject(NiStream& kStream) override;
	// Shared jmp thunk to NiExtraData::RegisterStreamables, used by most NiExtraData subclasses.
	bool RegisterStreamables(NiStream& kStream) override;
	// Also NiFloatExtraData::SaveBinary (the two bodies are one function).
	void SaveBinary(NiStream& kStream) override;
	// Also NiStringExtraData::IsEqual (the two bodies are one function).
	bool IsEqual(NiObject* pObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;

	// Copies the NiExtraData members and the value into apDest (used by CreateClone).
	void CopyMembers(NiIntegerExtraData* pDest, NiCloningProcess& kCloning);

	// Creates an unnamed extra data of value 0 on the engine's heap (the stream's factory for the class).
	static NiObject* CreateObject();

	int32_t m_iValue;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiIntegerExtraData_vftable{};
