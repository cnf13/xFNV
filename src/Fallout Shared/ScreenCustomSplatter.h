#pragma once

#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class NiCamera;
class NiNode;
class NiTexture;

float RandomFloatNeg1To1();
// BSRandom's Bool with afPercentTrue.
bool RandomBool(float afPercentTrue);
// Its one caller, Cmd_SetScreenSplatterFade, passes a bool
// (setne) that it stores in ScreenCustomSplatter::bNoFade.
char SetCustomSplatterFade(float a1, bool a2);
void CustomBloodSplatter();

// The kind of custom splatter (PDB values); Activate (0x4DE8E0) takes it and does not read it.
enum ScreenCustomSplatterType : int32_t {
	SCREENSCUSTOMPLATTER_BLOOD = 0,
	SCREENSCUSTOMPLATTER_BLOOD2 = 1,
	SCREENSCUSTOMPLATTER_BLOOD3 = 2,
	SCREENSCUSTOMPLATTER_COUNT = 3,
};

class ScreenCustomSplatter {
public:
	// IDA: CustomBloodSplatter::spTextures.
	using spTextures_t = NiPointer<NiTexture>[3];

	static void Update();
	// Starts a splatter of auiCount drops of type aeType.
	static void Activate(uint32_t auiCount, float afDuration, ScreenCustomSplatterType aeType, float fSplatScale,
		float fOpacityScale);
	static void Init();
	static NiCamera* GetCamera();
	static bool IsActiveSplatter();
	static NiNode* GetCurrentRoot();
	static NiNode* CreateGeometry(uint32_t auiCount, float fSplatScale, float fOpacityScale);
	// Inferred type: a1: char* from 0x4deee5 (passed to SkyTextureManager::LoadTextureInBackground 1); arg4: uint8_t:
	// unsigned uses from 0x4deed3
	static char SetCustomSplatterTexture(char* a1, uint8_t arg4);
	static void Shutdown();

	static NiPointer<NiNode> spRoot;
	static NiPointer<NiCamera> spCamera;
	static inline int32_t uiCount;
	static inline bool bEnabled;
	static inline bool bNoFade;
	// IDA: CustomBloodSplatter::Right (the exe stores the camera frustum's m_fRight here, as ScreenSplatter's).
	static inline float fWRadius = 0.5f;
	// IDA: CustomBloodSplatter::Top (the exe stores the camera frustum's m_fTop here).
	static inline float fHRadius = 0.5f;
	static spTextures_t spTextures;
};

// The game settings ScreenCustomSplatter.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> bCustomSplatterEnabled;
inline float fBloodSplatterFadeStart_0;
