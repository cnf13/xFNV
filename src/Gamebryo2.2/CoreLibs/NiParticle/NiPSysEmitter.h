#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"
#include "Reconstruction.hpp"

class NiPSysData;
class NiPoint3;
class NiStream;

// base of the particle emitters.
class NiPSysEmitter : public NiPSysModifier {
public:
	NiPSysEmitter();
	NiPSysEmitter(const char* apcName, uint32_t auiOrder, float afSpeed, float afSpeedVar, float afDeclination,
		float afDeclinationVar, float afPlanarAngle, float afPlanarAngleVar, NiColorA akInitialColor,
		float afInitialRadius, float afLifeSpan, float afLifeSpanVar, float afRadiusVar);
	~NiPSysEmitter() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void Update(float, NiPSysData*) override {}
	virtual void EmitParticles(float fTime, uint16_t usNumParticles, const float* pfAges);
	virtual void ComputeInitialPositionAndVelocity(NiPoint3& arPosition, NiPoint3& arVelocity) = 0;

	// Stores m_fSpeed.
	void SetSpeed(float fSpeed);
	// Stores
	// m_fInitialRadius.
	void SetInitialRadius(float fInitialRadius);
	// Stores m_fLifeSpan.
	void SetLifeSpan(float fLifeSpan);

protected:
	void CopyMembers(NiPSysEmitter* pkDest, NiCloningProcess& kCloning);

public:
	float m_fSpeed;
	float m_fSpeedVar;
	float m_fDeclination;
	float m_fDeclinationVar;
	float m_fPlanarAngle;
	float m_fPlanarAngleVar;
	NiColorA m_kInitialColor;
	float m_fInitialRadius;
	float m_fRadiusVar;
	float m_fLifeSpan;
	float m_fLifeSpanVar;
	float m_fScale;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiPSysEmitter_vftable{};
