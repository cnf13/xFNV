#include "Gamebryo2.2/CoreLibs/NiMain/NiMaterialProperty.h"

#include "Gamebryo2.2/CoreLibs/NiMain/NiCloningProcess.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObjectNET.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStreamMacros.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI NiMaterialProperty::ms_RTTI("NiMaterialProperty", &NiProperty::ms_RTTI);
NiPointer<NiMaterialProperty> NiMaterialProperty::ms_spDefault;

// Each property takes the next index; colours start black, the shine at 4.
NiMaterialProperty::NiMaterialProperty()
	: m_iIndex(InterlockedIncrementInt32(&NiMaterialProperty::ms_iNextIndex)), m_spec(0.0f, 0.0f, 0.0f),
	  m_emit(0.0f, 0.0f, 0.0f), m_external_emit(nullptr), m_fShine(4.0f), m_fAlpha(1.0f), m_fEmitMult(1.0f),
	  m_uiRevID(1), m_pvRendererData(nullptr) {}

// The renderer drops what it holds for the material.
NiMaterialProperty::~NiMaterialProperty() {
	NiRenderer* pkRenderer = NiRenderer::ms_pkRenderer;
	if (pkRenderer) {
		pkRenderer->PurgeMaterial(this);
	}
}

NiObject* NiMaterialProperty::CreateClone(NiCloningProcess& kCloning) {
	void* v3;
	NiMaterialProperty* v4;
	if (m_spControllers) {
		v3 = NiMemObject::operator new(sizeof(NiMaterialProperty));
		if (v3) {
			v4 = ::new (v3) NiMaterialProperty;
			CopyMembers(v4, kCloning);
			return v4;
		} else {
			CopyMembers(nullptr, kCloning);
			return nullptr;
		}
	} else {
		NiObject::CopyMembers(this, kCloning);
		return this;
	}
}

// Old files (Bethesda stream version below 0x1A) also store ambient and diffuse colors; both are read into one
// zeroed scratch color and dropped. The old layout has no emissive multiplier, so m_fEmitMult stays as it was.
void NiMaterialProperty::LoadBinary(NiStream& stream) {
	NiProperty::LoadBinary(stream);
	if (stream.m_BSStreamHeader.uiVersion < 0x1A) {
		NiColor kUnused = {0.0f, 0.0f, 0.0f};
		kUnused.LoadBinary(stream);
		kUnused.LoadBinary(stream);
		m_spec.LoadBinary(stream);
		m_emit.LoadBinary(stream);
		NiStreamLoadBinary(stream, m_fShine);
		NiStreamLoadBinary(stream, m_fAlpha);
	} else {
		m_spec.LoadBinary(stream);
		m_emit.LoadBinary(stream);
		NiStreamLoadBinary(stream, m_fShine);
		NiStreamLoadBinary(stream, m_fAlpha);
		NiStreamLoadBinary(stream, m_fEmitMult);
	}
}

void NiMaterialProperty::LinkObject(NiStream& stream) {
	NiProperty::LinkObject(stream);
}

bool NiMaterialProperty::RegisterStreamables(NiStream& stream) {
	return NiProperty::RegisterStreamables(stream);
}

void NiMaterialProperty::SaveBinary(NiStream& stream) {
	NiProperty::SaveBinary(stream);
	m_spec.SaveBinary(stream);
	m_emit.SaveBinary(stream);
	NiStreamSaveBinary(stream, m_fShine);
	NiStreamSaveBinary(stream, m_fAlpha);
	NiStreamSaveBinary(stream, m_fEmitMult);
}

// The external emittance is compared by pointer, not by color; NaN floats count as unequal.
bool NiMaterialProperty::IsEqual(NiObject* pObject) {
	if (!NiProperty::IsEqual(pObject)) {
		return false;
	}
	NiMaterialProperty* pkMat = static_cast<NiMaterialProperty*>(pObject);
	if (m_spec != pkMat->m_spec) {
		return false;
	}
	if (m_emit != pkMat->m_emit) {
		return false;
	}
	if (m_external_emit != pkMat->m_external_emit) {
		return false;
	}
	if (m_fShine != pkMat->m_fShine) {
		return false;
	}
	if (m_fAlpha != pkMat->m_fAlpha) {
		return false;
	}
	if (m_fEmitMult != pkMat->m_fEmitMult) {
		return false;
	}
	return true;
}

// With an external emittance set, that color is shown (under its own label) instead of m_emit.
void NiMaterialProperty::GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) {
	NiProperty::GetViewerStrings(pStrings);
	pStrings->Add(NiGetViewerString(ms_RTTI.GetName()));
	pStrings->Add(m_spec.GetViewerString("m_spec"));
	pStrings->Add(m_external_emit ? m_external_emit->GetViewerString("m_external_emit")
								  : m_emit.GetViewerString("m_emit"));
	pStrings->Add(NiGetViewerString("m_fShine", m_fShine));
	pStrings->Add(NiGetViewerString("m_fAlpha", m_fAlpha));
	pStrings->Add(NiGetViewerString("m_fEmitMult", m_fEmitMult));
}

void NiMaterialProperty::IncRevID(int32_t) {
	m_uiRevID = m_uiRevID + 1;
}

void NiMaterialProperty::SetAlpha(float c) {
	m_fAlpha = c;
	IncRevID(1);
}

void NiMaterialProperty::SetEmittance(const NiColor& c) {
	m_emit = c;
	IncRevID(1);
}

void NiMaterialProperty::SetShineness(float a2) {
	m_fShine = a2;
	IncRevID(1);
}

void NiMaterialProperty::CopyMembers(NiMaterialProperty* pDest, NiCloningProcess& kCloning) {
	NiProperty::CopyMembers(pDest, kCloning);
	pDest->m_spec = m_spec;
	pDest->m_emit = m_emit;
	pDest->m_external_emit = m_external_emit;
	pDest->m_fShine = m_fShine;
	pDest->m_fAlpha = m_fAlpha;
	pDest->m_fEmitMult = m_fEmitMult;
}

void NiMaterialProperty::_SDMInit() {
	NiMaterialProperty* pkDefault =
		static_cast<NiMaterialProperty*>(NiMemObject::operator new(sizeof(NiMaterialProperty)));
	if (pkDefault) {
		pkDefault = ::new (pkDefault) NiMaterialProperty;
	}
	// SWEEP-INLINE 0xA759FC
	ms_spDefault = pkDefault;
}

// Releases ms_spDefault through the pointer's assignment.
void NiMaterialProperty::_SDMShutdown() {
	// SWEEP-INLINE 0xA75A40
	ms_spDefault = nullptr;
}

int32_t NiMaterialProperty::GetType() {
	return 2;
}

NiObject* NiMaterialProperty::CreateObject() {
	void* v0 = NiMemObject::operator new(sizeof(NiMaterialProperty));
	if (v0) {
		return ::new (v0) NiMaterialProperty;
	} else {
		return nullptr;
	}
}
