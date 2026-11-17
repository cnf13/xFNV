#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysFieldModifier.h"
#include "Reconstruction.hpp"

class NiCloningProcess;
class NiPSysData;
class NiStream;

// spins particles around an axis.
class NiPSysVortexFieldModifier : public NiPSysFieldModifier {
public:
	NiPSysVortexFieldModifier();
	~NiPSysVortexFieldModifier() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void Update(float fTime, NiPSysData* pkData) override;

protected:
	// The part of kPos - kOrigin perpendicular to kAxis, crossed with kAxis and unitized; zero on the axis.
	NiPoint3 VortexDirection(NiPoint3& kVortexPos, NiPoint3& kDirection, NiPoint3& kParticle);
	void UpdateNoAttenuation(float fTime, NiPSysData* pkData);
	void UpdateNoAttenuationNoDist(float fTime, NiPSysData* pkData);

public:
	static NiObject* CreateObject();

	NiPoint3 m_kDirection;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiPSysVortexFieldModifier_vftable{};
