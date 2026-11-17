#pragma once

#include "Fallout Shared/ExtraDataObjects.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class TESObjectLIGH;
class TESObjectREFR;

// The light a magic projectile carries: its light form, the light the engine builds for it and a timed fade.
class MagicLight {
public:
	MagicLight(uint32_t aiLightID, NiAVObject* apObj);
	~MagicLight();

	bool IsFading();
	// Starts a fade in (abIn) or out lasting afTime seconds.
	void Fade(bool abIn, float afTime);
	// Stores afTime as the fade timer.
	void SetFadeTimer(float afTimer);
	// Moves the light onto apRef's 3D (its vtable +0x1D0 getter, called twice), when it has
	// one (thiscall, ret 4, 0x81C4A0). IDA names it SetRootNodeFromProjectile; the PDB name is taken from the
	// overload pair Transfer (the other, 0x81C4E0, takes the node).
	void Transfer(TESObjectREFR* apRef);
	void Update(float afDelta);
	void Init(uint32_t aiLightID, NiAVObject* apObj);
	// Moves the light onto apNode: the parent pointer set to it, the light attached to it, when there is a light
	// object (thiscall, ret 4, 0x81C4E0). The PDB's Transfer(NiAVObject*), void: its one caller (0x81C4A0) never
	// reads eax.
	void Transfer(NiAVObject* apNode);

	static void operator delete(void* apMemory) { ourFree(apMemory); }

	NiPointer<NiNode> pParent;
	TESObjectLIGH* pLightObject;
	LightRefData LightData;
	float fFadeTimer;
	float fFadeTargetTime;
	bool bFadeIn;
	uint8_t padding19[3];
};
