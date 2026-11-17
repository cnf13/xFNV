#pragma once

#include "BSCore/BSSimpleList.h"
#include "BSMain/Setting.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"

class BSMultiBoundRoom;
class BSNiBoundArray;
class BSPortal;
class NiCamera;
class TESObjectREFR;

// How the level-of-detail distance multiplier of a fade node is picked.
enum ENUM_LOD_MULT : uint32_t {
	LOD_MULT_NONE = 0,
	LOD_MULT_OBJECTS = 1,
	LOD_MULT_ITEMS = 2,
	LOD_MULT_ACTORS = 3,
	LOD_MULT_TREES = 4,
	LOD_MULT_LANDSCAPE = 5,
	LOD_MULT_DISTANTLOD = 6,
	LOD_MULT_ACTOR_BODY_PART = 7,
	LOD_MULT_RENDEREDMENU = 8,
	LOD_MULT_INVISIBLE = 9,
	LOD_MULT_DIST_OVERRIDE = 10,
	LOD_MULT_TOTAL = 11,
};

// A node whose children fade in and out with distance.
class BSFadeNode : public NiNode {
public:
	using FadeOutMultiplierTable = float[LOD_MULT_TOTAL];
	using MaxAlphaAccessFunc = float (*)(BSFadeNode*);
	// The INI setting iFadeNodeMinNearDistance:LOD. SetRange reads its value in place.
	using MinNearDistanceSetting = SettingT<INISettingCollection>;

	BSFadeNode();
	~BSFadeNode() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }
	BSFadeNode* IsFadeNode() override { return this; }
	NiObject* CreateClone(NiCloningProcess& arCloneData) override;
	void LoadBinary(NiStream& arStream) override;
	// Only calls NiNode::LinkObject.
	void LinkObject(NiStream& arStream) override;
	bool RegisterStreamables(NiStream& arStream) override;
	void SaveBinary(NiStream& arStream) override;
	bool IsEqual(NiObject* apObject) override;
	void GetViewerStrings(NiTPrimitiveArray<char*>* pkStrings) override;
	void UpdateDownwardPass(NiUpdateData& arUpdateData, uint32_t aiFlags) override;
	void UpdateSelectedDownwardPass(NiUpdateData& UpdateData, uint32_t uFlags) override;
	void OnVisible(NiCullingProcess& arCuller) override;

	// Whether the node is faded in.
	bool IsVisible();
	void SetFadingFlag(bool abFadedIn);
	void TurnFadeNodeOn();
	void SetRange(float afNearDist, float afFarDist);
	// Sets or clears the ignore-fade flag.
	void SetIgnoreFade(bool abIgnoreFade);
	// The faded-in flag of the node.
	uint32_t QFadingFlag();
	// Fades the node in or out by its distance from apCamera, at once when abNoFade.
	bool CheckFadeRadius(NiCamera* apCamera, bool abNoFade);
	void SetLODMultType(ENUM_LOD_MULT aeMultType);
	void ForceLODMultType(ENUM_LOD_MULT aeMultType);
	ENUM_LOD_MULT GetMultType() const;
	float GetLastFadePercent() const;
	TESObjectREFR* GetLinkedObj() const;
	float GetMaxAlpha();
	// Sets the linked reference (pLinkedObj).
	void SetUserData(void* apData);
	uint32_t QHasMovingSound();
	void TurnFadeNodeOff();
	float GetBoundRadius();
	bool ShouldUpdate(NiCamera* apCamera, float afDistMult);

	static void RegisterRTTI();
	static void SetFadeInTime(float afFadeTime);
	static void SetFadeOutTime(float afFadeTime);
	static void SetFadeInThreshold(float afFadeIn);
	static void SetFadeOutThreshold(float afFadeOut);
	static void SetDistanceMultiplier(float afDistanceMultiplier);
	static void SetLODFadeOutObjectMult(float afMultiplier);
	// A scene graph walk callback: flags apObj as having a moving sound when it is a fade node.
	static void AddMovingSoundCallback(NiAVObject* apObj);
	static void SetLODFadeOutItemMult(float afMultiplier);
	static void SetLODFadeOutActorMult(float afMultiplier);
	static float QFadeOutMultiplier(ENUM_LOD_MULT aeType);
	static void SetFadeOutMultiplier(ENUM_LOD_MULT aeType, float afValue);
	// The default access function installed in pMaxAlphaAccessFunc: the alpha of the linked actor, or 1.
	static float GetLinkedActorAlpha(BSFadeNode* apNode);
	// Installs the function GetMaxAlpha asks for the node's maximum alpha.
	static void SetMaxAlphaAccessFunc(MaxAlphaAccessFunc apFunc);
	static bool IsFadeEnabled();
	static void SetFadeEnabled(bool abVal);
	static void IncFadeCounter();
	// Builds the shared fade alpha property and interns the name of the fade controller sequence.
	static void InitSDM();
	// Allocates and constructs a fade node for the stream's create-object table.
	static NiObject* CreateObject();
	static void KillSDM();

	float fNearDistSqr;
	float fFarDistSqr;
	float fLastFade;
	float fCurrentFade;
	float fBoundRadius;
	float fTimeSinceUpdate;
	ENUM_LOD_MULT eMultType;
	// Signed: it is compared signed and printed through the int viewer string.
	int32_t iFrameCounter;
	TESObjectREFR* pUserData;
	BSNiBoundArray* pBoundArray;
	BSSimpleList<BSMultiBoundRoom*> ParentRoomList;
	BSSimpleList<BSPortal*> ParentPortalList;

	static NiRTTI ms_RTTI;
	// The frame counter IncFadeCounter advances; the update bodies compare iFrameCounter with it signed.
	static inline int32_t iFadeCounter;
	// The alpha property every fade node shares, and the interned name of the fade controller sequence.
	static NiPointer<NiAlphaProperty> spFadeAlpha;
	static inline NiFixedString* pFadeNodeAnimName;
	static inline bool bFadeEnabled = true;
	static inline float fFadeInTime = 2.0f;
	static inline float fFadeOutTime = 2.0f;
	static inline float fFadeInAlphaThreshold = 0.7f;
	static inline float fFadeOutAlphaThreshold = 0.3f;
	static inline float fDistanceMultiplier = 1.0f;
	static inline float fLODFadeOutActorMult = 1.0f;
	static inline float fLODFadeOutItemMult = 1.0f;
	static inline float fLODFadeOutObjectMult = 1.0f;
	static inline FadeOutMultiplierTable pfFadeOutMultiplierA{0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f, -1.0f, 0.0f,
		1e-06f, 1.0f};
	static MinNearDistanceSetting iMinNearDistance;
	static inline MaxAlphaAccessFunc pMaxAlphaAccessFunc{};
	static inline float fMaxFadeIncrement = 0.1f;
};
