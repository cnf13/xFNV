#include "BSShader/VolumetricFogShaderProperty.h"

#include "BSShader/BSShaderProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"

NiRTTI VolumetricFogShaderProperty::ms_RTTI("VolumetricFogShaderProperty", &BSShaderProperty::ms_RTTI);

// The fog is white with a density of 100.
VolumetricFogShaderProperty::VolumetricFogShaderProperty()
	: FogProps(NiColorA_WHITE.r, NiColorA_WHITE.g, NiColorA_WHITE.b, 100.0f) {
	iShaderPropertyType = 22;
}

VolumetricFogShaderProperty::~VolumetricFogShaderProperty() {}

// The exe's cleanup-only EH frame around the construction is left out. CopyMembers still runs when the allocation
// failed, with a null destination, as in the exe; the fog colour is copied afterwards, so it dereferences null there
// too.
NiObject* VolumetricFogShaderProperty::CreateClone(NiCloningProcess& arCloneData) {
	VolumetricFogShaderProperty* pClone =
		static_cast<VolumetricFogShaderProperty*>(NiMemObject::operator new(sizeof(VolumetricFogShaderProperty)));
	if (pClone) {
		pClone = ::new (pClone) VolumetricFogShaderProperty;
	}
	CopyMembers(pClone, arCloneData);
	pClone->FogProps.r = FogProps.r;
	pClone->FogProps.g = FogProps.g;
	pClone->FogProps.b = FogProps.b;
	pClone->FogProps.a = FogProps.a;
	return pClone;
}

void VolumetricFogShaderProperty::LoadBinary(NiStream& kStream) {
	BSShaderProperty::LoadBinary(kStream);
	FogProps.LoadBinary(kStream);
}

void VolumetricFogShaderProperty::LinkObject(NiStream& kStream) {
	BSShaderProperty::LinkObject(kStream);
}

bool VolumetricFogShaderProperty::RegisterStreamables(NiStream& kStream) {
	return BSShaderProperty::RegisterStreamables(kStream);
}

void VolumetricFogShaderProperty::SaveBinary(NiStream& kStream) {
	BSShaderProperty::SaveBinary(kStream);
	FogProps.SaveBinary(kStream);
}

// The exe does not check the type of apObject.
bool VolumetricFogShaderProperty::IsEqual(NiObject* pkObject) {
	if (static_cast<VolumetricFogShaderProperty*>(pkObject)->FogProps != FogProps)
		return false;
	return BSShaderProperty::IsEqual(pkObject);
}

void VolumetricFogShaderProperty::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
	BSShaderProperty::GetViewerStrings(apStrings);
	apStrings->Add(NiGetViewerString(VolumetricFogShaderProperty::ms_RTTI.GetName()));
	apStrings->Add(FogProps.GetViewerString("FogProps"));
}

void VolumetricFogShaderProperty::CopyMembers(VolumetricFogShaderProperty* pDest, NiCloningProcess& arCloneData) {
	NiShadeProperty::CopyMembers(pDest, arCloneData);
	pDest->ulFlags[0] = ulFlags[0];
	pDest->ulFlags[1] = ulFlags[1];
	pDest->fAlpha = fAlpha;
	double fFadeAlpha = this->fFadeAlpha;
	pDest->iLastRenderPassState = 0;
	pDest->fFadeAlpha = fFadeAlpha;
	pDest->iShader = iShader;
	pDest->fDepthBias = fDepthBias;
}

// The exe's cleanup-only EH frame around the construction is left out.
NiObject* VolumetricFogShaderProperty::CreateObject() {
	VolumetricFogShaderProperty* pObject =
		static_cast<VolumetricFogShaderProperty*>(NiMemObject::operator new(sizeof(VolumetricFogShaderProperty)));
	if (pObject) {
		return ::new (pObject) VolumetricFogShaderProperty;
	}
	return nullptr;
}
