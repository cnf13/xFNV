#pragma once

#include "BSAudio/BSAudioListener.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Reconstruction.hpp"
#include "SDK/Windows/mmsystem.h"

class IDirectSound3DListener;

// The audio listener on DirectSound: it wraps a DirectSound 3D listener. Not in the PDB under this name (IDA's name).
class BSWin32AudioListener : public BSAudioListener {
public:
	// The exe stores the argument into pListener (+0x48): the interface pointer itself.
	BSWin32AudioListener(IDirectSound3DListener* apListener);
	~BSWin32AudioListener() override;

	void Init() override;
	void SetPosition(const NiPoint3&) override;
	NiPoint3& GetPosition() override;
	void Update() override;
	void SetOrientation(const NiPoint3&, const NiPoint3&) override;
	void SetVelocity(const NiPoint3&) override;
	void SetRolloffFactor(float afRolloffFactor) override;

	// IDA name. The DirectSound 3D listener (IDA: LPDIRECTSOUND3DLISTENER8; IDirectSound3DListener8 is
	// IDirectSound3DListener); the destructor releases it (0xAEA770).
	IDirectSound3DListener* pListener;
	NiPoint3 kTop;
	NiPoint3 kFront;
};
inline constexpr fnvd::Vtable BSWin32AudioListener_vftable{};
