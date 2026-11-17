#include "Gamebryo2.2/CoreLibs/NiMain/NiFogProperty.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefCount.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemoryDefines.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"

#include <cstring>

NiRTTI NiFogProperty::ms_RTTI("NiFogProperty", &NiProperty::ms_RTTI);
NiPointer<NiFogProperty> NiFogProperty::ms_spDefault;

NiFogProperty::NiFogProperty() : m_uFlags(0), m_fDepth(1.0f), m_kColor(NiColor_BLACK) {}

NiFogProperty::~NiFogProperty() {}

NiObject* NiFogProperty::CreateClone(NiCloningProcess& kCloning) {
	if (m_spControllers) {
		NiFogProperty* v4;
		NiFogProperty* v3 = static_cast<NiFogProperty*>(NiMemObject::operator new(sizeof(NiFogProperty)));
		if (v3) {
			v4 = ::new (v3) NiFogProperty;
		} else {
			v4 = 0;
		}
		CopyMembers(v4, kCloning);
		v4->m_uFlags = m_uFlags;
		v4->m_fDepth = m_fDepth;
		v4->m_kColor.r = m_kColor.r;
		v4->m_kColor.g = m_kColor.g;
		v4->m_kColor.b = m_kColor.b;
		return v4;
	} else {
		NiObject::CopyMembers(this, kCloning);
		return this;
	}
}

void NiFogProperty::LoadBinary(NiStream& kStream) {
	NiProperty::LoadBinary(kStream);
	if (kStream.m_uiNifFileVersion >= 0x0A000102) {
		NiStreamLoadBinary(kStream, m_uFlags);
	} else if (kStream.m_uiNifFileVersion >= 0x05000016) {
		m_uFlags = kStream.m_usNiPropertyFlags & 7;
	} else {
		// Older files kept "enabled" in bit 1 and the function in bits 3-4.
		uint16_t usOldFlags = kStream.m_usNiPropertyFlags;
		m_uFlags = ((usOldFlags >> 1) & 1) ? 1 : 0;
		m_uFlags = static_cast<uint16_t>((m_uFlags & ~FOG_FUNC_MASK) | (((usOldFlags >> 3) & 3) << FOG_FUNC_POS));
	}
	NiStreamLoadBinary(kStream, m_fDepth);
	m_kColor.LoadBinary(kStream);
}

void NiFogProperty::LinkObject(NiStream& kStream) {
	NiProperty::LinkObject(kStream);
}

bool NiFogProperty::RegisterStreamables(NiStream& kStream) {
	return NiProperty::RegisterStreamables(kStream);
}

void NiFogProperty::SaveBinary(NiStream& kStream) {
	NiProperty::SaveBinary(kStream);
	NiStreamSaveBinary(kStream, m_uFlags);
	NiStreamSaveBinary(kStream, m_fDepth);
	m_kColor.SaveBinary(kStream);
}

bool NiFogProperty::IsEqual(NiObject* pkObject) {
	if (!NiProperty::IsEqual(pkObject)) {
		return false;
	}
	NiFogProperty* pkFog = static_cast<NiFogProperty*>(pkObject);
	if ((m_uFlags & FOG_MASK) != (pkFog->m_uFlags & FOG_MASK)) {
		return false;
	}
	if (m_fDepth != pkFog->m_fDepth) {
		return false;
	}
	if ((m_uFlags & FOG_FUNC_MASK) != (pkFog->m_uFlags & FOG_FUNC_MASK)) {
		return false;
	}
	if (m_kColor != pkFog->m_kColor) {
		return false;
	}
	return true;
}

void NiFogProperty::GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) {
	NiProperty::GetViewerStrings(pkStrings);
	pkStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
	pkStrings->Add(NiGetViewerString("Enable", (m_uFlags & FOG_MASK) != 0));
	pkStrings->Add(NiGetViewerString("Depth", m_fDepth));
	pkStrings->Add(GetViewerString("Function", static_cast<FogFunction>((m_uFlags >> 1) & 3)));
	pkStrings->Add(m_kColor.GetViewerString("Color"));
}

void NiFogProperty::SetFog(bool bFog) {
	SetBit(bFog, FOG_MASK);
}

void NiFogProperty::SetBit(bool bVal, uint16_t uMask) {
	if (bVal) {
		m_uFlags = uint16_t(m_uFlags | uMask);
	} else {
		m_uFlags = uint16_t(m_uFlags & ~uint32_t(uMask));
	}
}

// No default case: any other function value returns the buffer unwritten.
char* NiFogProperty::GetViewerString(const char* pcPrefix, FogFunction eFunc) {
	size_t stLength = strlen(pcPrefix) + 20;
	char* pcString = static_cast<char*>(_NiMalloc(stLength));
	switch (eFunc) {
	case FOG_Z_LINEAR:
		NiSprintf(pcString, stLength, "%s = FOG_Z_LINEAR", pcPrefix);
		break;
	case FOG_RANGE_SQ:
		NiSprintf(pcString, stLength, "%s = FOG_RANGE_SQ", pcPrefix);
		break;
	case FOG_VERTEX_ALPHA:
		NiSprintf(pcString, stLength, "%s = FOG_VERTEX_ALPHA", pcPrefix);
		break;
	}
	return pcString;
}

// The engine's default fog property: the exe inlines the constructor (0xA99ED0). The previous default is released
// and the new one takes a reference.
void NiFogProperty::_SDMInit() {
	NiFogProperty* pkDefault = static_cast<NiFogProperty*>(NiMemObject::operator new(sizeof(NiFogProperty)));
	if (pkDefault) {
		pkDefault = ::new (pkDefault) NiFogProperty;
	} else {
		pkDefault = nullptr;
	}

	ms_spDefault = pkDefault;
}

// Drops the engine's default fog property, releasing the one reference ms_spDefault held; the release deletes the
// object before the static is cleared.
void NiFogProperty::_SDMShutdown() {
	ms_spDefault = nullptr;
}

NiObject* NiFogProperty::CreateObject() {
	NiFogProperty* pkObject = static_cast<NiFogProperty*>(NiMemObject::operator new(sizeof(NiFogProperty)));
	if (!pkObject) {
		return 0;
	}
	return ::new (pkObject) NiFogProperty;
}
