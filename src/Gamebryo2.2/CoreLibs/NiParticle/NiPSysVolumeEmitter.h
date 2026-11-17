#pragma once

#include "Gamebryo2.2/CoreLibs/NiParticle/NiPSysEmitter.h"

class NiAVObject;
class NiCloningProcess;
class NiPoint3;
class NiStream;
class NiTransform;

// base of the emitters that spawn inside a volume around an object.
class NiPSysVolumeEmitter : public NiPSysEmitter {
public:
	NiPSysVolumeEmitter();
	// The (name, object, shape) constructor: the emitter part with order 1000, the class's vtable, then the emitter
	// object.
	NiPSysVolumeEmitter(const char* apcName, NiAVObject* apEmitterObj, float afSpeed, float afSpeedVar,
		float afDeclination, float afDeclinationVar, float afPlanarAngle, float afPlanarAngleVar,
		NiColorA akInitialColor, float afInitialRadius, float afLifeSpan, float afLifeSpanVar, float afRadiusVar);
	~NiPSysVolumeEmitter() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	void LoadBinary(NiStream& kStream) override;
	void LinkObject(NiStream& kStream) override;
	bool RegisterStreamables(NiStream& kStream) override;
	void SaveBinary(NiStream& kStream) override;
	bool IsEqual(NiObject* pkObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void ProcessClone(NiCloningProcess& kCloning) override;
	void ComputeInitialPositionAndVelocity(NiPoint3& kPosition, NiPoint3& kVelocity) override;
	virtual void SetEmitterObj(NiAVObject* pkEmitterObj) { m_pkEmitterObj = pkEmitterObj; }

protected:
	virtual void ComputeVolumeInitialPositionAndVelocity(NiTransform& arEmitterToPSys, NiPoint3& arPosition,
		NiPoint3& arVelocity) = 0;

public:
	void CopyMembers(NiPSysVolumeEmitter* pkDest, NiCloningProcess& kCloning);

	NiAVObject* m_pkEmitterObj;

	static NiRTTI ms_RTTI;
};
