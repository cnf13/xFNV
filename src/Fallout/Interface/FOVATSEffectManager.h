#pragma once

#include "BSMain/Setting.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/TESBipedModelForm.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRenderer.h"
#include "Reconstruction.hpp"
#include "SDK/MSVC/ehvec.h"

class Actor;
class BSDismemberSkinInstance;
class BSOcclusionQuery;
class BSShaderAccumulator;
class NiStencilProperty;

// The VATS target highlight: draws the selected targets into a texture, queries their occlusion and composites the
// scan, pulse and burst effects. Only the members the reconstructed code reaches are declared.
class FOVATSEffectManager {
public:
	// Which occlusion query a draw into the target texture makes. Values from the Xbox PDB; the exe compares the
	// dword with 1 and 2 (0x800F30).
	enum VATS_OCCLUSION_QUERY : int32_t {
		VATS_OCCLUSION_QUERY_NONE = 0,
		VATS_OCCLUSION_QUERY_FULL = 1,
		VATS_OCCLUSION_QUERY_PARTIAL = 2,
	};

	// Whether a draw covers the selected body parts or the whole target. Values from the Xbox PDB; the exe compares
	// the dword with 1 (0x801450).
	enum VATS_DRAW_MODE : int32_t {
		VATS_DRAW_SELECTED_PARTS = 0,
		VATS_DRAW_ENTIRE_TARGET = 1,
	};

	// Whether the highlight covers one target or several. Values from the Xbox PDB.
	enum VATS_EFFECT_MODE : int32_t {
		SINGLE_TARGET = 0,
		MULTIPLE_TARGETS = 1,
	};

	// One target of the VATS highlight: the reference and its 3D.
	struct VATS_TARGET {
		VATS_TARGET();

		TESObjectREFR* pRef;
		NiPointer<NiAVObject> spObj;
	};

	FOVATSEffectManager();
	~FOVATSEffectManager();

	// The per-frame update of the VATS effects.
	void OnIdle();
	// Sets the single target, its limb and whether it is occluded.
	void SetTarget(TESObjectREFR* apTarget, BGSBodyPart::LIMB_ENUM aeLimb, bool abDoOcclusion);
	// Empties the multi target list.
	void ClearMultiTargets();
	// Draws the single target.
	void DrawSingleTarget(bool abQueryMode, VATS_DRAW_MODE aeDrawMode);
	// Draws the multiple targets.
	void DrawMultipleTargets();
	// Draws the single target, or the multiple targets in the multi target mode.
	void DrawTarget(bool abQueryMode, VATS_DRAW_MODE aeDrawMode);

protected:
	// The pulse intensity and radius from the noise at afTime times the pulse rate.
	void UpdatePulse(float afTime);

public:
	// Highlights apTarget in the multi target mode.
	void SetMultiTarget(TESObjectREFR* apTarget);
	// Starts the burst: active, the duration, the intensity and the current time (thiscall, ret
	// 0xC, 0x800650; IDA's name is 800650).
	void TriggerBurst(float afBurstDuration, float afBurstIntensity, bool abPlaySound);
	// Sets eMode from *apMode.
	void SetEffectMode(const VATS_EFFECT_MODE& aMode);
	// The single target's 3D, SingleTarget.spObj (+8; the exe calls the shared getter
	// 0x5585E0).
	NiAVObject* GetTarget();
	void PrepareHighlightEffect(BSRenderedTexture* apSourceColors, BSRenderedTexture* apSourceMask,
		BSRenderedTexture* apTarget, bool abClearTarget);
	void PrepareScanEffect(BSRenderedTexture* apSourceColors, BSRenderedTexture* apSourceMask,
		BSRenderedTexture* apTarget, bool abClearTarget);
	bool OcclusionEnabled();
	// Draws the targets into apTarget.
	void DrawTargetToTexture(BSRenderedTexture* apTarget, VATS_OCCLUSION_QUERY aQueryType, bool abQueryMode,
		VATS_DRAW_MODE aeDrawMode, NiRenderer::ClearFlags aeClearMode, bool abStopRenderTarget);
	// Sets up the image-space effect from the scene and target textures.
	void PrepareEffect(BSRenderedTexture* apSourceColors, BSRenderedTexture* apSourceMask, BSRenderedTexture* apTarget,
		bool abClearTarget);
	// Turns the interface image-space effects (10 and 11) off and returns their textures; clears the interface mode.
	void ShutdownEffect();
	// The visible share of body part aeBodyPart's area in afResult_out, or -1 and false when its occlusion result is
	// not valid. The PC build adds aeBodyPart (not in the PDB) before the PDB's afResult_out.
	bool GetVATSGeometryOcclusionPercent(uint32_t aeBodyPart, float& afResult_out);
	// EffectMode (the exe calls the shared getter 0x559450).
	VATS_EFFECT_MODE GetEffectMode();
	bool IsScanning();
	void DisableScan();
	void TriggerDistort();
	void TriggerScan();
	void UpdateTargetPulse(float afTime);
	void SetSelectedMultiTarget(TESObjectREFR* apTarget);
	void UpdateBurst(float afTime);
	void UpdateDistort(float afTime);
	void DisableOcclusion();
	void UpdateScan(float afTime);
	void ShowBipedObjectOnActor(Actor* apActor, BIPED_OBJECT aeObject, bool abShow);
	void* ClearsData();
	void* ClearsData_();
	FOVATSEffectManager* Clears();
	void TargetAdditionalLimb(BGSBodyPart::LIMB_ENUM aeLimb);
	void IsolateBodyPart();
	void EnableBodyPartOnSkinInstance(BSDismemberSkinInstance* apInstance, BGSBodyPart::LIMB_ENUM aeLimb,
		bool abEnable);
	void RestoreBodyParts();
	void FreeOcclusionQueries();

	VATS_EFFECT_MODE EffectMode;
	VATS_TARGET SingleTarget;
	int32_t iNumMultiTargets;
	int32_t iSelectedMultiTarget;
	VATS_TARGET pMultiTargets[32];
	// The PDB's NiPointer<NiStencilProperty>; the exe's constructor initialises it as a NiPointer at +0x114.
	NiPointer<NiStencilProperty> spStencilProp;
	bool bOcclusionEnabled;
	bool bWaitingForOcclusionQuery[16];
	bool bEffectEnabled;
	// Alignment padding before the pointer arrays.
	uint8_t padding12A[2];
	BSOcclusionQuery* pPartialLimbQuery[16];
	BSOcclusionQuery* pEntireLimbQuery[16];
	// IDA name; not in the PDB. The exe tests the whole byte; bool would assume 0/1.
	uint8_t bValidOcclusionResult[16];
	uint32_t uiTotalArea[16];
	uint32_t uiVisibleArea[16];
	// The targeted limb (PDB eLimb, before iNumAdditionalLimbs; the PC build has no iEntireLimbResult or
	// iPartialLimbResult here). SetTarget (0x800AC0) stores its LIMB_ENUM argument; 0x801450, 0x801DA0 and 0x8022C0
	// pass it to BGSBodyPartData::GetBodyPart, DrawTargetToTexture (0x800F30) indexes the limb queries with it.
	BGSBodyPart::LIMB_ENUM eLimb;
	int32_t iNumAdditionalLimbs;
	BGSBodyPart::LIMB_ENUM pAdditionalLimbs[16];
	bool bDistortActive;
	uint8_t padding285[3];
	float fDistortStartTime;
	float fDistortPosition;
	float fDistortDuration;
	float fNextDistortTime;
	bool bBurstActive;
	uint8_t padding299[3];
	float fBurstStartTime;
	float fBurstDuration;
	float fBurstMaximumIntensity;
	float fBurstCurrentIntensity;
	float fPulseCurrentIntensity;
	float fPulseCurrentRadius;
	bool bScanning;
	bool bExitScanMode;
	uint8_t padding2B6[2];
	float fScanStartTime;
	float fScanPosition;
	float fPulseIntensity;
	NiPointer<NiTexture> spScanlinesTexture;
	uint32_t uiPartialResultMult;
	NiPointer<BSShaderAccumulator> spAccumulator;
};

// The game settings FOVATSEffectManager.cpp defines (PDB), at their exe addresses.
extern SettingT<INISettingCollection> fVATSColorR;
extern SettingT<INISettingCollection> fVATSColorB;
extern SettingT<INISettingCollection> fVATSColorG;
extern SettingT<INISettingCollection> fVATSTargetPulseRate;

// INISettingCollection: 0xf95992 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fScanModeScanlineFrequency_VATS;

// INISettingCollection: 0xf958d2 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fDistortDuration_VATS;

// INISettingCollection: 0xf95842 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fBlurIntensity_VATS;

// INISettingCollection: 0xf95812 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fBlurRadius_VATS;

// INISettingCollection: 0xf95b6d calls 0x45cfe0.
extern SettingT<INISettingCollection> gs_sScanlineTexture_VATS;

// INISettingCollection: 0xf957e2 calls 0x44f440.
extern SettingT<INISettingCollection> gs_fScanlineFrequency_VATS;

// INISettingCollection: 0xf9589e calls 0x44f440.
extern SettingT<INISettingCollection> gs_fDistortHorizontalScale_VATS;
