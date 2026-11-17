#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/Sky/SkyObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class NiAVObject;
class Sky;

// The star field object; spRoot comes from SkyObject.
class Stars : public SkyObject {
public:
	// The constructor.
	Stars();
	~Stars() override;

	void Initialize(NiNode* apSkyRoot) override;
	void Update(Sky* apSky, float afSecondsPassed) override;

	void UnloadGeometry();
	void LoadGeometry(const char* apFilename);

	// The star geometry attached to spRoot.
	NiPointer<NiAVObject> spStars;   // 0x08

	float fAlpha;          // 0x0C
};
inline constexpr fnvd::Vtable Stars_vftable{};

// The game settings Stars.cpp defines (PDB), at their exe addresses.
extern SettingT<GameSettingCollection> fStarsRotateXAxis;
extern SettingT<GameSettingCollection> fStarsRotateZAxis;
extern SettingT<GameSettingCollection> fStarsRotateDays;
extern SettingT<GameSettingCollection> fStarsRotateYAxis;
