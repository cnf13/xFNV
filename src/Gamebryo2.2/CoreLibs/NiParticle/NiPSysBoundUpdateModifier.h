#pragma once

#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysModifier.h"

class BSStripPSysData;
class NiBound;
class NiCloningProcess;
class NiMeshPSysData;
class NiParticleSystem;
class NiPSysData;
class NiStream;

// recomputes the bound of its particle system.
class NiPSysBoundUpdateModifier : public NiPSysModifier {
public:
	NiPSysBoundUpdateModifier();
	~NiPSysBoundUpdateModifier() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void Update(float fTime, NiPSysData* pkData) override;
	void SetSystemPointer(NiParticleSystem* pkTarget) override;

	// Sets how many updates one full pass over the particles is spread across, and resets the per-pass bounds.
	void SetUpdateSkip(int32_t aiUpdateSkip);

protected:
	// Update for mesh particles and for strips: the bound comes from the meshes' world bounds or the strip points.
	void UpdateMeshBounds(float fTime, NiMeshPSysData* pkMeshData);
	void UpdateStripBounds(float fTime, BSStripPSysData* apData);

public:
	static NiObject* CreateObject();

	int16_t m_sUpdateSkip;
	uint16_t m_usUpdateCount;
	NiBound* m_pkSkipBounds;

	static NiRTTI ms_RTTI;
};
