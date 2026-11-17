#pragma once

#include "BSMain/Setting.h"
#include "Fallout/Interface/FORenderedMenu.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Reconstruction.hpp"

#include <stdint.h>

class NiCamera;

// What the race/sex menu was opened for (the PDB's enum; IDA's PC name for it is FORenderedMenuRaceSex::
// RaceMenuType, whose values agree).
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
	// PC inherits PostDraw but overrides these three base slots, unlike the Xbox method list.
	// Inferred type: a2: float* from 0x7fe8ec (passed to FORenderedMenu::Pick 1); a3: float* from 0x7fe8e8 (passed to
	// FORenderedMenu::Pick 2)
	bool Pick(bool, float*, float*) override;
	// Fetches the menu root through the shared getter 0x63D040 and drops it (thiscall, ret 0,
	// 0x7FE930, slot 7).
	void Unknown07() override;
	void Unknown08() override;
	void Initialize() override;
	void OnMenuOpen() override;
	void OnMenuClose() override;
	// Inferred type: a1: float: used as one at 0x7feb89
	void UpdateDistort(float) override;
	void DoEffect(BSRenderedTexture*, BSRenderedTexture*, NiRenderer::ClearFlags) override;
	// Inferred type: a1: float: used as one at 0x7fec3a
	virtual void UpdateFakeDistort(float);

	// Sets up the lights, scaled by afScale.
	void SetupLights(float afScale);
	// Shows the glow of button auiButton and hides the others.
	void LightButton(uint32_t aiButton);
	// Starts the fake distortion now. Protected in the PDB.
	void TriggerFakeDistort();

	NiPointer<NiAVObject> spButtons[4];
	NiPointer<NiAVObject> spBtnGlow[4];
	NiPointer<NiNode> spGeometry;
	NiPointer<NiNode> spPlayer3D;
	NiPointer<NiNode> spPlayerParent;
	NiPointer<NiCamera> spClonedPlayerCamera;
	NiPointer<NiAVObject> spPlayerLights;
	NiPointer<NiNode> spPlayer3DParent;
	// PC vector/matrix alignment is four bytes, unlike the Xbox PDB's sixteen-byte alignment.
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
	// What the menu was opened for; IDA's PC type for it is the RaceMenuType enum at the same offset.
	RaceSexMenuContext m_Context;
};
inline constexpr fnvd::Vtable FORenderedMenuRaceSex_vftable{};

// The game settings FORenderedMenuRaceSex.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> fRaceSexMenuScale;
extern SettingT<INISettingCollection> fRaceSexMenuZoom;
extern SettingT<INISettingCollection> fRaceSexMenuVPos;
extern SettingT<INISettingCollection> fRaceSexMenuDistortDuration;
extern SettingT<INISettingCollection> fRaceSexMenuHPos;
// INISettingCollection: 0xf94e9e calls 0x44f440.
extern SettingT<INISettingCollection> gs_fDistortHorizontalScale_InterfaceFX;

// INISettingCollection: 0xf94ed2 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fDistortDuration_InterfaceFX;

// INISettingCollection: 0xf94efd calls 0x45cfe0.
extern SettingT<INISettingCollection> gs_iDistortMaxInterval_InterfaceFX;

// INISettingCollection: 0xf94e72 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fDistortVerticalScale_InterfaceFX;
