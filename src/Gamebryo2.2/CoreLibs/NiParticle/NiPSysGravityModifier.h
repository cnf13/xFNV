#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Reconstruction.hpp"

class NiAVObject;
class NiCloningProcess;
class NiPSysData;
class NiStream;

// pulls particles toward a plane or a point.
class NiPSysGravityModifier : public NiPSysModifier {
public:
	// the shape of the pull.
	enum ForceType {
		FORCE_PLANAR = 0x0,
		FORCE_SPHERICAL = 0x1,
	};

	NiPSysGravityModifier();
	~NiPSysGravityModifier() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void ProcessClone(NiCloningProcess& kCloning) override;
	void Update(float fTime, NiPSysData* pkData) override;

	static NiObject* CreateObject();

	NiAVObject* m_pkGravityObj;
	NiPoint3 m_kGravityAxis;
	float m_fDecay;
	float m_fStrength;
	ForceType m_eType;
	float m_fTurbulence;
	float m_fScale;
	bool m_bWorldAligned;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiPSysGravityModifier_vftable{};
