#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"

// The 3D audio listener: position, orientation, velocity and the environment the listener hears.
class BSAudioListener {
public:
	virtual ~BSAudioListener();

	virtual void Init() = 0;
	virtual void SetPosition(const NiPoint3&) = 0;
	virtual NiPoint3& GetPosition();
	virtual void Update() {}
	virtual void SetOrientation(const NiPoint3&, const NiPoint3&) = 0;
	virtual NiPoint3& GetFrontOrientation();
	virtual NiPoint3& GetTopOrientation();
	virtual void SetVelocity(const NiPoint3&) = 0;
	virtual BSAudioListener* SetGain(float afGain);
	virtual void SetEnvironmentType(uint32_t aiType);
	virtual uint32_t GetEnvironmentType();
	virtual void SetRolloffFactor(float afRolloffFactor);

	static void operator delete(void* apMemory);

	NiPoint3 ListenerPosition;
	NiPoint3 FrontOrientation;
	NiPoint3 TopOrientation;
	NiPoint3 vVelocity;
	uint32_t dwTimeLastUpdated;
	float fVelocity;
	float fRolloffFactor;
	float fGain;
	uint32_t iEvironmentType;
};
