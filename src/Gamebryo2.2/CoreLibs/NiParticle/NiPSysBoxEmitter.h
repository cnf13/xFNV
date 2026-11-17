#pragma once

#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysVolumeEmitter.h"
#include "Reconstruction.hpp"

class NiCloningProcess;
class NiPoint3;
class NiStream;
class NiTransform;

// emits particles inside a box.
class NiPSysBoxEmitter : public NiPSysVolumeEmitter {
public:
	NiPSysBoxEmitter();
	~NiPSysBoxEmitter() override;

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
	static NiObject* CreateObject();

	float m_fEmitterWidth;
	float m_fEmitterHeight;
	float m_fEmitterDepth;

	static NiRTTI ms_RTTI;
};
inline constexpr fnvd::Vtable NiPSysBoxEmitter_vftable{};
