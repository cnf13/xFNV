#pragma once

#include "BSMain/Setting.h"
#include "Fallout/Interface/FORenderedMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

#include <stdint.h>

class NiCamera;

// What the race/sex menu was opened for.
enum RaceSexMenuContext : int32_t {
	RSM_CONTEXT_NONE = 0,
	RSM_CONTEXT_CHARGEN = 1,
	RSM_CONTEXT_BARBER = 2,
	RSM_CONTEXT_PLASTIC_SURGEON = 3,
};

class FORenderedMenuRaceSex : public FORenderedMenu {
public:
	FORenderedMenuRaceSex();
	~FORenderedMenuRaceSex() override;

	bool QRequiresDepthBuffer() override;
	NiAVObject* GetParentNodeToRender() override;
	void Draw(BSRenderedTexture*, NiRenderer::ClearFlags, BSRenderedTexture*) override;
	bool OnIdle() override;
	// Inherits PostDraw but overrides these three base functions.
	bool Pick(bool, float*, float*) override;
	// Fetches the menu root through the shared getter and drops it.
	void Unknown07() override;
	void Unknown08() override;
	void Initialize() override;
	void OnMenuOpen() override;
	void OnMenuClose() override;
	void UpdateDistort(float) override;
	void DoEffect(BSRenderedTexture*, BSRenderedTexture*, NiRenderer::ClearFlags) override;
	virtual void UpdateFakeDistort(float);

	// Sets up the lights, scaled by afScale.
	void SetupLights(float afScale);
	// Shows the glow of button auiButton and hides the others.
	void LightButton(uint32_t aiButton);
	void TriggerFakeDistort();

	NiPointer<NiAVObject> spButtons[4];
	NiPointer<NiAVObject> spBtnGlow[4];
	NiPointer<NiNode> spGeometry;
	NiPointer<NiNode> spPlayer3D;
	NiPointer<NiNode> spPlayerParent;
	NiPointer<NiCamera> spClonedPlayerCamera;
	NiPointer<NiAVObject> spPlayerLights;
	NiPointer<NiNode> spPlayer3DParent;
	// PC vector/matrix alignment is four bytes, unlike the Xbox's sixteen-byte alignment.
	NiPoint3 ptSavedPlayerTranslation;
	NiMatrix3 mtxSavedPlayerRotation;
	NiPoint3 ptSavedBip01Translation;
	bool bPlayer3DCulled;
	uint8_t padding14D[3];
	float fDistortStart;
	float fDistortPos;
	bool bFakeDistortActive;
	uint8_t padding159[3];
	float fFakeDistortStartTime;
	float fFakeDistortPosition;
	float fFakeDistortDuration;
	float fNextFakeDistortTime;
	// What the menu was opened for.
	RaceSexMenuContext m_Context;
};

// The game settings FORenderedMenuRaceSex.cpp defines.
extern SettingT<INISettingCollection> fRaceSexMenuScale;
extern SettingT<INISettingCollection> fRaceSexMenuZoom;
extern SettingT<INISettingCollection> fRaceSexMenuVPos;
extern SettingT<INISettingCollection> fRaceSexMenuDistortDuration;
extern SettingT<INISettingCollection> fRaceSexMenuHPos;
extern SettingT<INISettingCollection> gs_fDistortHorizontalScale_InterfaceFX;

extern SettingT<INISettingCollection> gs_fDistortDuration_InterfaceFX;

extern SettingT<INISettingCollection> gs_iDistortMaxInterval_InterfaceFX;

extern SettingT<INISettingCollection> gs_fDistortVerticalScale_InterfaceFX;
