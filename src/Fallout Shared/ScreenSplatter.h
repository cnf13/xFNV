#pragma once

#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

class NiCamera;
class NiNode;
class NiTexture;

// The kind of blood splatter (PDB values); Activate (0x4E0330) takes it and does not read it.
enum ScreenSplatterType : int32_t {
	SCREENSPLATTER_BLOOD = 0,
	SCREENSPLATTER_BLOOD2 = 1,
	SCREENSPLATTER_BLOOD3 = 2,
	SCREENSPLATTER_COUNT = 3,
};

// Only the members used so far; the rest of the class is not reconstructed yet.
class ScreenSplatter {
public:
	// IDA: BloodSplatter::Textures.
	using spTextures_t = NiPointer<NiTexture>[3];

	// Starts auiCount splatters of type aeType for afDuration seconds.
	static void Activate(uint32_t auiCount, float afDuration, ScreenSplatterType aeType);
	// Removes every splatter from the screen while splatters are enabled.
	static void Clear();
	static bool IsActiveSplatter();
	static NiCamera* GetCamera();
	// Inferred type: return: NiNode* from 0x5bac5d (NiPointer<NiNode>::m_pObject (&ScreenSplatter::spRoot))
	static NiNode* GetRoot();
	// Advances the splatters.
	static void Update();
	static void Init();
	// Releases the splatter scene and its textures.
	static void Shutdown();
	static NiNode* CreateGeometry(uint32_t auiCount);

	// Set while the screen splatter scene is drawn: NiDX9Renderer::Do_SetCameraData then keeps the camera location in
	// the view matrix instead of moving it into NiRenderer::PosAdjust. A PC-only static: no name in the PDB.
	static inline bool bResetCameraPos = true;
	static inline bool bEnabled;
	static NiPointer<NiNode> spRoot;
	static NiPointer<NiCamera> spCamera;
	static inline uint32_t uiCount;
	static inline float fWRadius = 0.5f;
	static inline float fHRadius = 0.5f;
	static spTextures_t spTextures;
};

// The game settings ScreenSplatter.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> bBloodSplatterEnabled;
