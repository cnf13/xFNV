#pragma once

#include "BSMain/Setting.h"
#include "Fallout/Interface/FORenderedMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTriStrips.h"

class BSFadeNode;

void PlayTerminalShutdownSound();

// The rendered menu of a terminal screen (computers and hacking).
class FORenderedTerminal : public FORenderedMenu {
public:
    FORenderedTerminal();

    // The menu object 3D shadow scene node, where the terminal's screen is drawn.
    NiAVObject* GetParentNodeToRender() override;
    float GetFadeAmount() override;
    void
    Draw(BSRenderedTexture* apTarget, NiRenderer::ClearFlags aeClearMode, BSRenderedTexture* apSwapTarget) override;
    bool OnIdle() override;
    bool Pick(bool abPressed, float* apX, float* apY) override;
    void Unknown07() override;
    // Moves the menu root to x 30490 and updates it at time 0.
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

// The game settings FORenderedTerminal.cpp defines.
extern SettingT<INISettingCollection> fRenderedTerminalFOV;
extern SettingT<INISettingCollection> fRenderedTerminalVPos;
extern SettingT<INISettingCollection> bUseRenderedTerminals;
extern SettingT<INISettingCollection> fRenderedTerminalHPos;
extern SettingT<INISettingCollection> fRenderedTerminalZoom;
extern SettingT<INISettingCollection> fRenderedTerminalScanlineScale;

extern SettingT<INISettingCollection> gs_fScreenLightColorB_RenderedTerminal;

extern SettingT<INISettingCollection> gs_fScreenLightColorR_RenderedTerminal;

extern SettingT<INISettingCollection> gs_fScreenLightRadius_RenderedTerminal;

extern SettingT<INISettingCollection> g_fScreenLightBaseIntensity_RenderedTerminal;

extern SettingT<INISettingCollection> gs_fScreenLightColorG_RenderedTerminal;
