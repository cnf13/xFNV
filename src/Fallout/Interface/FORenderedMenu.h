#pragma once

#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiScreenElements.h"
#include "Reconstruction.hpp"

class NiCamera;
class NiPointLight;
class ShadowSceneNode;
class Tile;

// A menu drawn into a 3D scene (the pipboy screen, terminals, the VATS and lockpick scenes).
// The smart pointers whose engine class is not reconstructed yet are declared as NiPointer<NiRefObject>.
class FORenderedMenu {
public:
	FORenderedMenu();
	virtual ~FORenderedMenu();

	virtual bool QRequiresDepthBuffer() { return false; }
	virtual NiAVObject* GetParentNodeToRender() { return this->spScreenParent; }
	// Renders the menu into apTarget.
	virtual void Draw(BSRenderedTexture* apTarget, NiRenderer::ClearFlags aeClearMode, BSRenderedTexture* apSwapTarget);
	virtual void PostDraw() {}
	// Updates the screen effects; the exe returns true (al = 1) once it has run.
	virtual bool OnIdle();
	// PC only (no Xbox name): picks the screen geometry under the cursor; true on a hit.
	virtual bool Pick(bool abPressed, float* apX, float* apY);
	// PC only (no Xbox name): empty in FORenderedMenu.
	virtual void Unknown07() {}
	virtual void Unknown08() {}
	virtual void Initialize();
	// Releases the light and every scene object, and marks the menu uninitialised.
	virtual void Shutdown();
	virtual float GetFadeAmount() { return 1.0f; }
	virtual void OnMenuOpen();
	virtual void OnMenuClose();
	// The screen effects, advanced every frame.
	virtual void UpdateBurst(float afTime);
	virtual void UpdateVertHold(float afTime);
	virtual void UpdateShudder(float afTime);
	virtual void UpdatePulse(float afTime);
	virtual void UpdateLight(float afTime);
	virtual void UpdateDistort(float afTime);
	// Post-processes apSource into apDest.
	virtual void DoEffect(BSRenderedTexture* apSource, BSRenderedTexture* apDest, NiRenderer::ClearFlags aeClearMode);

	// Whether the scene graph of the menu has been created.
	bool GetInitialized();
	// bMenuRendering (thiscall, ret 0, 0x7FAEF0). The exe returns the raw byte (mov al), so the result is a
	// uint8_t: callers see any non-zero value as stored.
	uint8_t IsMenuRendering();
	void SetScreenLightBaseIntensity(float afIntensity);
	void SetScreenLightFlickerIntensity(float afIntensity);
	// Turns the scanline effect on or off; afScanlineScale is the engine's scanline scale.
	void SetDoScanlines(const bool abEnable, const float afScale);
	void SetScreenLightColor(NiColor& aColor);
	void SetTintColor(const NiColorA& aNewColor);
	NiColorA* GetTintColor(NiColorA* apResult);
	// Replaces the node the screen geometry hangs under.
	void SetScreenParent(NiAVObject* apScreenParent);
	// Replaces the screen's triangle shape.
	void SetScreenGeometry(NiGeometry* apScreenTriShape);
	// spScreenTriShape's object.
	NiGeometry* GetScreenGeometry();
	// Applies the default screen texture and render properties before updating the geometry.
	void PrepareGeometry(NiGeometry* apGeometry);
	// Detaches the screen light from the scene and releases it.
	void ClearLight();
	// Shows apTexture on the screen geometry (null: the default texture).
	void SetScreenTexture(NiTexture* apTexture);
	// Starts a screen burst of the given length and strength, with a chance of its sound.
	void TriggerBurst(float afBurstDuration, float afBurstIntensity, float afSoundChance);
	NiCamera* GetCamera();
	// Stores the colour the menu's target is cleared to.
	void SetBackgroundColor(const NiColorA& aColor);
	// Stores the blur intensity of the screen effect.
	void SetBlurIntensity(const float afIntensity);
	// Stores fBlurRadius.
	void SetBlurRadius(const float afRadius) { fBlurRadius = afRadius; }
	// Replaces spMenuRoot.
	void SetMenuRoot(NiAVObject* apMenuRoot);
	// Creates the menu light from the named node of the menu's scene; false when the node
	// is missing.
	bool CreateMenuLight(const char* apLightNodeName, ShadowSceneNode* apShadowSceneNode);
	// Returns spLight's object.
	NiPointLight* GetMenuLight();
	// spMenuRoot's object (through NiPointer::operator->; thiscall, ret 0, 0x63D040, the body
	// BGSBodyPart::GetIKStartNodeName shares).
	NiAVObject* GetMenuRoot();
	void SetScanlineVisible();
	void UnhideMessageQuantityTutorialMenus();
	void SetCamera(NiCamera* apCamera);
	void HideMessageQuantityTutorialMenus();
	void CreateLight();
	void TriggerShudder(float afDuration, float afIntensity, float afFrequency);
	// Replaces spDefaultTex.
	void SetDefaultTex(NiTexture* apTex);
	// Stores fNextDistortTime.
	void SetNextDistortTime(float afTime);

	static void operator delete(void* apMemory);

	NiPointer<NiTriShape> spScreenTriShape;
	NiPointer<NiNode> spScreenParent;
	NiPointer<NiScreenElements> spPartialScreenGeometry;
	// NiTexture smart pointer.
	NiPointer<NiRefObject> spDefaultTex;
	// SceneGraph smart pointer.
	NiPointer<NiRefObject> spMenuRoot;
	Tile* pMenuTile;
	// NiPointLight smart pointer.
	NiPointer<NiRefObject> spLight;
	ShadowSceneNode* pLightShadowSceneNode;
	// NiCamera smart pointer.
	NiPointer<NiRefObject> spCamera;
	bool bInitialized;
	bool bScaledTexCoords;
	bool bUsePartialScreenGeometry;
	uint8_t padding2B;
	NiColor ScreenLightColor;
	float fScreenLightBaseIntensity;
	float fScreenLightFlickerIntensity;
	float fBlurRadius;
	float fBlurIntensity;
	float fScanlineScale;
	// Stored as the raw byte the caller passed.
	uint8_t bDoScanlines;
	bool bBurstActive;
	uint8_t padding4E[0x2];
	float fBurstStartTime;
	float fBurstDuration;
	float fBurstMaximumIntensity;
	float fBurstCurrentIntensity;
	bool bVertHoldActive;
	uint8_t padding61[0x3];
	float fVertHoldStartTime;
	float fVertHoldDuration;
	float fVertHoldSpeed;
	float fVertHoldCurrentPosition;
	bool bShudderActive;
	uint8_t padding75[0x3];
	float fShudderStartTime;
	float fShudderDuration;
	float fShudderIntensity;
	float fShudderFrequency;
	float fShudderCurrentPosition;
	float fPulseCurrentBrighten;
	float fPulseCurrentRadius;
	uint8_t bPulseAffectsLight;
	bool bDistortActive;
	uint8_t padding96[0x2];
	float fDistortStartTime;
	float fDistortPosition;
	float fDistortDuration;
	float fNextDistortTime;
	NiColorA BackgroundColor;
	bool bMenuOpen;
	uint8_t bMenuRendering;
	uint8_t paddingBA[0x2];
	NiPointer<NiTexture> spScanlinesTexture;
	NiColorA TintColor;
	bool bHidQuantity;
	bool bHidTutorial;
	bool bHidMessage;
	uint8_t paddingD3;
	// BSShaderAccumulator smart pointer.
	NiPointer<NiRefObject> spAccumulator;

	static inline uint8_t bSoundsActive;
};
inline constexpr fnvd::Vtable FORenderedMenu_vftable{};

// The game settings FORenderedMenu.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> fMiniBurstIntensity;
extern SettingT<INISettingCollection> fPulseBrightenIntensity;
extern SettingT<INISettingCollection> fDefaultShudderFrequency;
extern SettingT<INISettingCollection> fDefaultShudderIntensity;
extern SettingT<INISettingCollection> fScreenLightVertHoldRate;
extern SettingT<INISettingCollection> fDefaultVertHoldSpeed;
extern SettingT<INISettingCollection> fDefaultShudderDuration;
extern SettingT<INISettingCollection> fPulseRadiusIntensity;
extern SettingT<INISettingCollection> fScreenLightBurstModifier;
extern SettingT<INISettingCollection> fDefaultBurstIntensity;
extern SettingT<INISettingCollection> fDefaultBlurRadius;
extern SettingT<INISettingCollection> fMiniBurstDuration;
extern SettingT<INISettingCollection> fDefaultBlurIntensity;
extern SettingT<INISettingCollection> fDefaultBurstDuration;
extern SettingT<INISettingCollection> fScreenLightPulseModifier;
extern SettingT<INISettingCollection> fVertHoldChance;
extern SettingT<INISettingCollection> fShudderChance;
extern SettingT<INISettingCollection> fDefaultVertHoldDuration;
extern SettingT<INISettingCollection> fScreenLightRadius;

// INISettingCollection: 0xf94f2d calls 0x45cfe0.
extern SettingT<INISettingCollection> gs_sScanlineTexture_InterfaceFX;
