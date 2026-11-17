#include "Gamebryo2.2/CoreLibs/NiMain/NiIntegerExtraData.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiIntegerExtraData::ms_RTTI("NiIntegerExtraData", &NiExtraData::ms_RTTI);

NiIntegerExtraData::~NiIntegerExtraData() {}

const NiRTTI* NiIntegerExtraData::GetRTTI() const {
	return &ms_RTTI;
}

// A failed allocation still runs the copy on null, as the game does.
NiObject* NiIntegerExtraData::CreateClone(NiCloningProcess& kCloning) {
	NiIntegerExtraData* pkClone =
		static_cast<NiIntegerExtraData*>(NiMemObject::operator new(sizeof(NiIntegerExtraData)));
	if (pkClone) {
		::new (pkClone) NiIntegerExtraData;
	}
	NiExtraData::CopyMembers(pkClone, kCloning);
	pkClone->m_iValue = m_iValue;
	return pkClone;
}

void NiIntegerExtraData::LoadBinary(NiStream& kStream) {
	NiExtraData::LoadBinary(kStream);
	NiStreamLoadBinary(kStream, m_iValue);
}

void NiIntegerExtraData::LinkObject(NiStream& kStream) {
	NiExtraData::LinkObject(kStream);
}

bool NiIntegerExtraData::RegisterStreamables(NiStream& kStream) {
	return NiExtraData::RegisterStreamables(kStream);
}

void NiIntegerExtraData::SaveBinary(NiStream& kStream) {
	NiExtraData::SaveBinary(kStream);
	NiStreamSaveBinary(kStream, m_iValue);
}

// Only the values are compared: neither the class nor the name is checked.
bool NiIntegerExtraData::IsEqual(NiObject* pObject) {
	if (!pObject)
		return false;

	return m_iValue == static_cast<NiIntegerExtraData*>(pObject)->m_iValue;
}

void NiIntegerExtraData::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
	NiExtraData::GetViewerStrings(pStrings);
	pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
	pStrings->Add(NiGetViewerString("m_iValue", m_iValue));
}

void NiIntegerExtraData::CopyMembers(NiIntegerExtraData* pDest, NiCloningProcess& kCloning) {
	NiExtraData::CopyMembers(pDest, kCloning);
	pDest->m_iValue = m_iValue;
}

NiObject* NiIntegerExtraData::CreateObject() {
	NiIntegerExtraData* pkObject =
		static_cast<NiIntegerExtraData*>(NiMemObject::operator new(sizeof(NiIntegerExtraData)));
	if (!pkObject)
		return nullptr;
	return ::new (pkObject) NiIntegerExtraData;
}
