#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiParticlesData.h"
#include "Reconstruction.hpp"

class NiCamera;
class NiCloningProcess;
class NiParticleInfo;
class NiStream;

// particle data of a NiParticleSystem: the per-particle simulation state next to NiParticlesData's render data.
class NiPSysData : public NiParticlesData {
public:
	NiPSysData();
	~NiPSysData() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void RemoveParticle(int32_t aiParticle) override;
	virtual uint16_t AddParticle();
	virtual void ResolveAddedParticles();

	void AlphaSortVertices(NiCamera* apCamera);
	// Copies the geometry and particle arrays and the particle state into pkDest when cloning.
	void CopyMembers(NiPSysData* pkDest, NiCloningProcess& kCloning);
	void SortVertices(int32_t a2, int32_t a3);

	// The class' stream loader entry.
	static NiObject* CreateObject();

	NiParticleInfo* m_pkParticleInfo;
	float* m_pfRotationSpeeds;
	uint16_t m_usNumAddedParticles;
	uint16_t m_usAddedParticlesBase;

	static NiRTTI ms_RTTI;
	// What AddParticle returns when the data is full.
	static inline uint16_t INVALID_PARTICLE = 0xFFFF;
};
inline constexpr fnvd::Vtable NiPSysData_vftable{};
