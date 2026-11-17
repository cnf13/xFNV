#pragma once

#include "BSMain/Setting.h"
#include "Fallout/Interface/FORenderedMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"
#include "Reconstruction.hpp"

class BSFadeNode;

void PlayTerminalShutdownSound();

// The rendered menu of a terminal screen (computers and hacking).
class FORenderedTerminal : public FORenderedMenu {
public:
	// The constructor, thiscall at 0x705FE0.
	FORenderedTerminal();

	// The menu object 3D shadow scene node, where the terminal's screen is drawn (slot 2).
	NiAVObject* GetParentNodeToRender() override;
	float GetFadeAmount() override;
	void Draw(BSRenderedTexture* apTarget, NiRenderer::ClearFlags aeClearMode,
		BSRenderedTexture* apSwapTarget) override;
	bool OnIdle() override;
	// The PDB has no FORenderedTerminal::Pick; the slot's name is FORenderedMenu's.
	bool Pick(bool abPressed, float* apX, float* apY) override;
	// The slot's name is FORenderedMenu's (no PDB name).
	void Unknown07() override;
	// Moves the menu root (the shared getter 0x63D040) to x 30490 and updates it at time 0
	// (thiscall, ret 0, 0x7FF900, slots 4 and 8; IDA: PostRender). The slot's name is FORenderedMenu's.
	void PostDraw() override;
	void Unknown08() override;
	void Initialize() override;
	void OnMenuOpen() override;
	void OnMenuClose() override;

	// Plays the shutdown sound, lets go of the screen and leaves the rendered menu (the exit button's click).
	void CloseMenu();
	// Starts the screen's fade out: the geometry's fade range drops to 0 and OnIdle follows the fade.
	void FadeOut();
	void InitLight();

	// Releases the terminal's static geometry.
	static void ReleaseStaticGeometry();

	// The screen geometry's local translation, set on it before each draw.
	NiPoint3 OriginalPos;
	// The exit (power) button's geometry.
	NiPointer<NiTriStrips> spExitButton;
	// The terminal geometry's fade percent when last updated.
	float fLastFadePercent;
	bool bFadingOut;
	uint8_t paddingED[3];

	static NiPointer<BSFadeNode> spGeometry;
};

// The terminal model's file path.
inline char* pTerminalFile = const_cast<char*>("Terminals\\TerminalInterface01.NIF");
inline constexpr fnvd::Vtable FORenderedTerminal_vftable{};

// The game settings FORenderedTerminal.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> fRenderedTerminalFOV;
extern SettingT<INISettingCollection> fRenderedTerminalVPos;
extern SettingT<INISettingCollection> bUseRenderedTerminals;
extern SettingT<INISettingCollection> fRenderedTerminalHPos;
extern SettingT<INISettingCollection> fRenderedTerminalZoom;
extern SettingT<INISettingCollection> fRenderedTerminalScanlineScale;

// INISettingCollection: 0xf9552e calls 0x44f440.
extern SettingT<INISettingCollection> gs_fScreenLightColorB_RenderedTerminal;

// INISettingCollection: 0xf954ce calls 0x44f440.
extern SettingT<INISettingCollection> gs_fScreenLightColorR_RenderedTerminal;

// INISettingCollection: 0xf954a2 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fScreenLightRadius_RenderedTerminal;

// INISettingCollection: 0xf95472 calls 0x44f440.
extern SettingT<INISettingCollection> g_fScreenLightBaseIntensity_RenderedTerminal;

// INISettingCollection: 0xf954fe calls 0x44f440.
extern SettingT<INISettingCollection> gs_fScreenLightColorG_RenderedTerminal;
