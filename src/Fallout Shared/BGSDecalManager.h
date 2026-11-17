#pragma once

#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"

#include <stdint.h>

class BGSDecalEmitter;
class BGSTextureSet;
class BSShaderAccumulator;
class BSTempEffectSimpleDecal;
class DECAL_CREATION_DATA;
class DecalCaster;
class NiCamera;
class NiNode;
class NiShadeProperty;
class TESObjectCELL;
class TESObjectREFR;

int32_t BSSM_ShaderPropertyIsLighting(NiShadeProperty* apShaderProp);

// The decal manager: the pending simple decals and the decal emitters, with the occlusion query's render target.
class BGSDecalManager {
public:
	enum DECAL_TYPE : int32_t {
		DECAL_TYPE_NONE = 0,
		DECAL_TYPE_SIMPLE = 1,
		DECAL_TYPE_PROJECTED = 2,
		DECAL_TYPE_SKINNED = 3,
		DECAL_TYPE_PERMENANT_SIMPLE = 4,
		DECAL_TYPE_PERMENANT_PROJECTED = 5,
		DECAL_TYPE_PERMENANT_SKINNED = 6,
		DECAL_TYPE_COUNT = 7,
	};

	BGSDecalManager();
	~BGSDecalManager();

	// Updates the simple decals.
	void UpdateSimpleDecals();
	void UpdateDecalEmitters();
	void UpdateDecals();
	// Places the decal arDecalData as aeDecalType; abForceAdd skips the per-frame limits.
	void AddDecal(DECAL_CREATION_DATA& arDecalData, DECAL_TYPE aeDecalType, bool abForceAdd);
	bool AddGeometryDecalRecurse(DECAL_CREATION_DATA& arDecalData);

protected:
	// Places the projected (and skinned) decal of arDecalData on arDecalData.pAVObj and its children; false once
	// the decal is placed.
	bool AddProjectedDecalRecurse(DECAL_CREATION_DATA& arDecalData);

public:
	void IssueDecalOcclusionQuery(BSTempEffectSimpleDecal* apDecal);
	void ClearPendingQueries();

	static void operator delete(void* apMemory) { ourFree(apMemory); }
	// The decal manager singleton.
	static BGSDecalManager* GetSingleton();
	static void ClearDecalsThisFrame();
	// Whether apCollidee takes decals of aeDecalType.
	static bool IsValidDecalImpact(TESObjectREFR* apCollidee, DECAL_TYPE aeDecalType);
	static void AddEmitter(BGSDecalEmitter* apEmitter);
	static void RemoveAllDecals();
	static BGSDecalManager* Destroy_();

	// The occlusion query's render target, borrowed on first use by IssueDecalOcclusionQuery.
	NiPointer<BSRenderedTexture> spQueryTexture;
	// Set by AddDecal so the next decal clears the query target.
	bool bClearQueryTexture;
	uint8_t padding05[3];
	NiTPointerList<NiPointer<BSTempEffectSimpleDecal>> PendingSimpleDecalList;
	NiTPointerList<BGSDecalEmitter*> DecalEmitterList;
	NiPointer<BSShaderAccumulator> spQueryAccum;
	NiPointer<NiCamera> spQueryCamera;

	static inline BGSDecalManager* pSingleton;
	static inline int32_t iDecalsThisFrame;
	static inline int32_t iSkinnedDecalsThisFrame;
	// The skinned decal nodes placed so far (AddProjectedDecalRecurse counts them).
	static inline int32_t iSkinnedDecalCount;
	// The INI settings: iMaxSkinDecalsPerFrame:Display, fDecalLifetime:Display, uMaxSkinDecalPerActor:Decals,
	// bDebugDecals:Decals.
	static SettingT<RendererSettingCollection> iMaxSkinDecalsPerFrame;
	static SettingT<INISettingCollection> fDecalLifetime;
	static SettingT<INISettingCollection> iMaxSkinDecalPerActor;
	static SettingT<INISettingCollection> bDebugDecals;
	static inline DecalCaster* pDecalCaster;
	static inline int32_t iDecalCount;
	static inline int32_t iDecalDebugFlags;
	static NiPointer<NiAVObject> spDebugDecalBox;
	static SettingT<INISettingCollection> bDecalMultithreaded;
};

// The settings of one decal to place: where, on what, its texture and size.
class DECAL_CREATION_DATA {
public:
	DECAL_CREATION_DATA();

	NiPoint3 pOrigin;
	NiPoint3 pDirection;
	NiPoint3 pSurfaceNormal;
	TESObjectREFR* pObjRefr;
	NiAVObject* pAVObj;
	NiNode* pClone;
	BGSTextureSet* pTexSet;
	int32_t iIndex;
	float fDecalWidth;
	float fDecalHeight;
	float fDecalDepth;
	float fRotation;
	TESObjectCELL* pParentCell;
	float fParallaxOcclusionScale;
	NiNode* pSkinnedDecalNode;
	float fSpecular;
	float fDecalEpsilon;
	float fPlacementRadius;
	NiColor fVertexColor;
	uint32_t iBodyParts;
	char cSubTex;
	bool bPermanent;
	bool bUseOcclusionQuery;
	bool bUseParallaxOcclusion;
	bool bAlphaTesting;
	bool bAlphaBlending;
	uint8_t ucParallaxOcclusionMaxPasses;
	bool bModelSpace;
	bool bForceFade;
	bool bTwoSided;
};

// The game settings BGSDecalManager.cpp defines.
extern SettingT<GameSettingCollection> fSprayDecalsDistance;
extern SettingT<GameSettingCollection> fSprayDecalsGravity;
extern SettingT<GameSettingCollection> iSprayDecalsDebug;

extern SettingT<INISettingCollection> gs_bDecals_Decals;

extern SettingT<INISettingCollection> gs_bProfileDecals_Decals;

extern SettingT<RendererSettingCollection> gs_iMaxDecalsPerFrame_Display;

extern SettingT<INISettingCollection> gs_bDecalOcclusionQuery_Decals;

extern SettingT<INISettingCollection> gs_bSkinnedDecals_Decals;

extern SettingT<INISettingCollection> gs_uMaxSkinDecals_Decals;
