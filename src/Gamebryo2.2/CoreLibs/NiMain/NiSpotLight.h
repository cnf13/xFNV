#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointLight.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Reconstruction.hpp"

// a point light limited to a cone around its world direction
class NiSpotLight : public NiPointLight {
public:
	NiSpotLight();
	~NiSpotLight() override;

	const NiRTTI* GetRTTI() const override;
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& stream) override;
	// LinkObject and RegisterStreamables fold into jumps to the NiPointLight bodies
	void LinkObject(NiStream& stream) override;
	bool RegisterStreamables(NiStream& stream) override;
	void SaveBinary(NiStream& stream) override;
	bool IsEqual(NiObject* pObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pStrings) override;
	void UpdateWorldData(NiUpdateData& UpdateData) override;

protected:
	void CopyMembers(NiSpotLight* pDest, NiCloningProcess& kCloning);

public:
	static NiObject* CreateObject();

	NiPoint3 m_kWorldDir;
	float m_fOuterSpotAngle;
	float m_fInnerSpotAngle;
	float m_fSpotExponent;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiSpotLight_vftable{};
