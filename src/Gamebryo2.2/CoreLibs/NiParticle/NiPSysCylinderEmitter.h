#pragma once

#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysVolumeEmitter.h"

class NiCloningProcess;
class NiPoint3;
class NiStream;
class NiTransform;

// emits particles inside a cylinder.
class NiPSysCylinderEmitter : public NiPSysVolumeEmitter {
public:
	NiPSysCylinderEmitter();
	~NiPSysCylinderEmitter() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	NiObject* CreateClone(NiCloningProcess& kCloning) override;
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void ProcessClone(NiCloningProcess& kCloning) override;

protected:
	void ComputeVolumeInitialPositionAndVelocity(NiTransform& kEmitterToPSys, NiPoint3& kPosition,
		NiPoint3& kVelocity) override;

public:
	float GetEmitterHeight() const { return m_fEmitterHeight; }
	void SetEmitterHeight(float fEmitterHeight);

	static NiObject* CreateObject();

	float m_fEmitterRadius;
	float m_fEmitterHeight;

	static NiRTTI ms_RTTI;
};
