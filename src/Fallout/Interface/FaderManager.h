#pragma once

#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

class NiNode;
class TileShaderProperty;

// The faders FaderManager::CreateFader makes.
enum FADER_ENUM : int32_t {
	FADER_BLACK_UNDER_MENUS = 0,
	FADER_TO_BLACK = 1,
	FADER_TO_WHITE = 2,
	FADER_COUNT = 3,
};

// One fader's state; FaderData holds one per FADER_ENUM.
struct FADER_DATA {
	bool bBelowMenus;
	char* pFaderTextureFilename;
	bool bFadingIn;
	// Padding, cleared together with bFadingIn by the FaderData initialiser.
	uint8_t unk09[3];
	float fFadeSpeed;
	NiNode* pRoot;
	float fAlpha;
	NiPointer<TileShaderProperty> spTileShaderProp;
};

// Fades the interface's scene graph roots in and out together.
class FaderManager {
public:
	FaderManager();
	~FaderManager();

	// Adds the root node apNode to fade, below or above the menus, scaled to the screen and placed afOneToOneDistance
	// ahead (20 nearer above the menus).
	void AddRoot(NiNode* apNode, float afOneToOneDistance, bool abBelowMenus);
	// Starts the fader aeFader at afFadeSpeed, from black when abBeginBlack.
	void CreateFader(FADER_ENUM aeFader, float afFadeSpeed, bool abBeginBlack);
	// Steps every active fader's alpha by the real frame time; detaches a fader that faded out. The argument is
	// unused.
	void UpdateFaders(bool abUpdateLoading);
	// Whether the fader aeFader is fading in and fully opaque.
	bool IsFaderVisible(FADER_ENUM aeFader);
	float GetFaderAlpha(FADER_ENUM aeFader);
	bool IsFaderActive(FADER_ENUM aeFader);
	// Removes the fader aeFader, at once when abForce.
	void RemoveFader(FADER_ENUM aeFader, bool abForce);
	bool IsFadingIn(FADER_ENUM aeFader);

	static void operator delete(void* apMemory);

	NiNode* pRootBelowMenus;
	NiNode* pRootAboveMenus;
};

using FaderDataArray = FADER_DATA[FADER_COUNT];
extern FaderDataArray FaderData;

// Frames a fading-out fader holds its alpha before it starts stepping.
inline int8_t cFaderManagerSkipFadeOutFrames;
inline FaderManager* pFaderManager;
