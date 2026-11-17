#pragma once

#include "BSCore/BSSpinLock.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Fallout Shared/FaceGen/BSFaceGenKeyframe.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Reconstruction.hpp"

class GameSettingCollection;
class NiAVObject;
class NiNode;
class NiQuaternion;

// The facial expressions; values from the exe's loops (15 expressions).
enum BS_FACEGEN_EXPRESSION : int32_t {
	BS_FG_EXPRESSION_NONE = -1,
	BS_FG_EXPRESSION_ANGER = 0,
	BS_FG_EXPRESSION_FEAR = 1,
	BS_FG_EXPRESSION_HAPPY = 2,
	BS_FG_EXPRESSION_SAD = 3,
	BS_FG_EXPRESSION_SURPRISE = 4,
	BS_FG_EXPRESSION_MOOD_NEUTRAL = 5,
	BS_FG_EXPRESSION_MOOD_AFRAID = 6,
	BS_FG_EXPRESSION_MOOD_ANNOYED = 7,
	BS_FG_EXPRESSION_MOOD_COCKY = 8,
	BS_FG_EXPRESSION_MOOD_DRUGGED = 9,
	BS_FG_EXPRESSION_MOOD_PLEASANT = 10,
	BS_FG_EXPRESSION_MOOD_ANGRY = 11,
	BS_FG_EXPRESSION_MOOD_SAD = 12,
	BS_FG_EXPRESSION_MOOD_PAINED = 13,
	BS_FG_EXPRESSION_COMBAT_ANGRY = 14,
	BS_FG_EXPRESSION_COUNT = 15,
};

// The facial modifiers (blinks, brows, eye look and head rotation).
enum BS_FACEGEN_MODIFIER : int32_t {
	BS_FG_MODIFIER_NONE = -1,
	BS_FG_MODIFIER_BLINK_LEFT = 0,
	BS_FG_MODIFIER_BLINK_RIGHT = 1,
	BS_FG_MODIFIER_BROW_DOWN_LEFT = 2,
	BS_FG_MODIFIER_BROW_DOWN_RIGHT = 3,
	BS_FG_MODIFIER_BROW_IN_LEFT = 4,
	BS_FG_MODIFIER_BROW_IN_RIGHT = 5,
	BS_FG_MODIFIER_BROW_UP_LEFT = 6,
	BS_FG_MODIFIER_BROW_UP_RIGHT = 7,
	BS_FG_MODIFIER_LOOK_DOWN = 8,
	BS_FG_MODIFIER_LOOK_LEFT = 9,
	BS_FG_MODIFIER_LOOK_RIGHT = 10,
	BS_FG_MODIFIER_LOOK_UP = 11,
	BS_FG_MODIFIER_SQUINT_LEFT = 12,
	BS_FG_MODIFIER_SQUINT_RIGHT = 13,
	BS_FG_MODIFIER_HEAD_PITCH = 14,
	BS_FG_MODIFIER_HEAD_ROLL = 15,
	BS_FG_MODIFIER_HEAD_YAW = 16,
	BS_FG_MODIFIER_COUNT = 17,
};

// The FaceGen levels of detail. Values from the Xbox PDB.
enum BS_FACEGEN_LOD : int32_t {
	BS_FACEGEN_LOD_NONE = -1,
	BS_FACEGEN_LOD_LOW = 0,
	BS_FACEGEN_LOD_HIGH = 1,
	BS_FACEGEN_LOD_COUNT = 2,
};

// The speech phonemes (16).
enum BS_FACEGEN_PHONEME : int32_t {
	BS_FG_PHONEME_NONE = -1,
	BS_FG_PHONEME_AAH = 0,
	BS_FG_PHONEME_BIG_AAH = 1,
	BS_FG_PHONEME_B_M_P = 2,
	BS_FG_PHONEME_CH_J_SH = 3,
	BS_FG_PHONEME_D_S_T = 4,
	BS_FG_PHONEME_EEE = 5,
	BS_FG_PHONEME_EH = 6,
	BS_FG_PHONEME_F_V = 7,
	BS_FG_PHONEME_I = 8,
	BS_FG_PHONEME_K = 9,
	BS_FG_PHONEME_N = 10,
	BS_FG_PHONEME_OH = 11,
	BS_FG_PHONEME_OOH_Q = 12,
	BS_FG_PHONEME_R = 13,
	BS_FG_PHONEME_TH = 14,
	BS_FG_PHONEME_W = 15,
	BS_FG_PHONEME_COUNT = 16,
};

// The custom morphs (only the vampirism one).
enum BS_FACEGEN_CUSTOM_MORPH : int32_t {
	BS_FG_CUSTOM_MORPH_VAMPIRISM = 0,
	BS_FG_CUSTOM_MORPH_COUNT = 1,
};

// The kinds of FaceGen morph; values from the exe's ApplyMorph calls.
enum BS_FACEGEN_MORPH_TYPE : int32_t {
	BS_FG_MORPH_NONE = -1,
	BS_FG_MORPH_PHONEME = 0,
	BS_FG_MORPH_EXPRESSION = 1,
	BS_FG_MORPH_MODIFIER = 2,
	BS_FG_MORPH_CUSTOM = 3,
	BS_FG_MORPH_COUNT = 4,
};

// The animated state of a FaceGen face: expression, modifier, phoneme and custom morph values, their keyframes,
// and the head and eye tracking. Only the virtuals and the members called so far are declared; the fields are not
// modelled. Every virtual forwards to the exe's slot (addresses from the exe's vtable at 0x1064E1C).
class BSFaceGenAnimationData : public NiExtraData {
public:
	// The head tracking state (the PDB's values; SetHeadTrackVector stores 1 on a new target and 0 on none).
	enum ETrackingState : int32_t {
		ETrackingState_None = 0,
		ETrackingState_JustAcquired = 1,
		ETrackingState_Tracking = 2,
	};

	BSFaceGenAnimationData();
	~BSFaceGenAnimationData() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }

	// Slot 37 (thiscall, ret 4, float in ST0, 0x6499E0).
	virtual float GetExpressionValue(BS_FACEGEN_EXPRESSION aeExpression);
	// Slot 38.
	virtual void SetExpressionValue(BS_FACEGEN_EXPRESSION aeExpression, float afValue);
	// Slot 39.
	virtual bool GetModifierValuesAsRotation(NiQuaternion& arQuat);
	// Slot 40.
	virtual void SetModifierValue(BS_FACEGEN_MODIFIER aeModifier, float afValue);
	// Slot 41 (thiscall, ret 4, float in ST0, 0x649AA0).
	virtual float GetPhonemeValue(BS_FACEGEN_PHONEME aePhoneme);
	// Slot 42.
	virtual void SetPhonemeValue(BS_FACEGEN_PHONEME aePhoneme, float afValue);
	// Slot 43 (thiscall, ret 4, float in ST0, 0x649B10).
	virtual float GetCustomMorphValue(BS_FACEGEN_CUSTOM_MORPH aeCustomMorph);
	// Slot 44.
	virtual void SetCustomMorphValue(BS_FACEGEN_CUSTOM_MORPH aeCustomMorph, float afValue);
	// Slot 45.
	virtual void ResetCurrentMorphs(float afTime, bool abResetExpression, bool abResetPhoneme, bool abResetModifier,
		bool abResetCustomMorph, bool abForDeath);
	// Slot 46.
	virtual void ResetSpeechCurrentMorphs(float afTime, bool abResetExpression, bool abResetPhoneme,
		bool abResetModifier, bool abResetCustomMorph, bool abForDeath);
	// Slot 47.
	virtual void ResetAnimCurrentMorphs(float afTime, bool abResetExpression, bool abResetPhoneme, bool abResetModifier,
		bool abResetCustomMorph);
	// Slot 48; the point is passed by value.
	virtual void SetHeadTrackVector(NiPoint3 aPoint, bool abInstant);
	// Slot 49.
	virtual void ClearHeadTracking(bool abInstant);
	// Slot 50.
	virtual bool GetChangedLastUpdate();
	// Slot 51.
	virtual bool GetMustUpdate();
	// Slot 52.
	virtual void SetMustUpdate(bool abMustUpdate);
	// Slot 53.
	virtual bool GetDead();
	// Slot 54.
	virtual void SetDead(bool abDead, bool abInstant);
	// Slot 55.
	virtual void SetAnimExpressionValue(BS_FACEGEN_EXPRESSION aeExpression, float afValue);
	// Slot 56.
	virtual void SetAnimModifierValue(BS_FACEGEN_MODIFIER aeModifier, float afValue);
	// Slot 57.
	virtual void SetAnimPhonemeValue(BS_FACEGEN_PHONEME aePhoneme, float afValue);
	// Slot 58.
	virtual void SetAnimCustomMorphValue(BS_FACEGEN_CUSTOM_MORPH aeCustomMorph, float afValue);
	// Slot 59.
	virtual void AddPhonemeKeyframe(float* apQuantities, float afTime);
	// Slot 60.
	virtual void AddModifiersKeyframe(float* apQuantities, float afTime);
	// Slot 61.
	virtual void AddSpeechEmptyKeyframe(BS_FACEGEN_MORPH_TYPE aeType, float afTime, bool abForceZero);
	// Slot 62; the exe's body is a shared empty function (thiscall, ret 0xC, 0x8D05E0).
	virtual void AddAnimEmptyKeyframe(BS_FACEGEN_MORPH_TYPE aeType, float afTime, bool abForceZero);
	// Slot 63.
	virtual void ClearKeyframeLists(bool abClearExpression, bool abClearPhonemes, bool abClearModifiers,
		bool abClearCustomMorphs);
	// Slot 64.
	virtual void ClearSpeechKeyframeLists(bool abClearExpression, bool abClearPhonemes, bool abClearModifiers,
		bool abClearCustomMorphs);
	// Slot 65; the exe's body is a shared empty function (thiscall, ret 0x10, 0x8CC730).
	virtual void ClearAnimKeyframeLists(bool abClearExpression, bool abClearPhonemes, bool abClearModifiers,
		bool abClearCustomMorphs);
	// Slot 66.
	virtual bool IsSpeechKeyframeListEmpty(BS_FACEGEN_MORPH_TYPE aeType);
	// Slot 67; the exe's body is a shared "return true" (thiscall, ret 4, 0x401290).
	virtual bool IsAnimKeyframeListEmpty(BS_FACEGEN_MORPH_TYPE aeType);
	// Slot 68.
	virtual void AddCustomMorphKeyframe(float* apQuantities, float afTime);
	// The two SetExpressionTarget overloads: the MS ABI groups overloads at the first one and orders them in reverse
	// declaration order, so the array one is declared first to land in slot 70 after the single value in slot 69.
	// Slot 70.
	virtual void SetExpressionTarget(float* apQuantities);
	// Slot 69.
	virtual void SetExpressionTarget(BS_FACEGEN_EXPRESSION aeExpression, float afValue);
	// Slot 71.
	virtual void SetExpressionTargetFromMood(int32_t aiMoodindex, bool abTalking);
	// Slot 72.
	virtual void ClearExpressionTarget();
	// Slot 73; the exe's body is a shared empty function (thiscall, ret 4, 0x4534F0).
	virtual void AddAnimationDataFromNote(const char* apNote);
	// Slot 74: advances the animation by afTime; true when a value changed.
	virtual bool Update(float afTime);

	// The current value of modifier aeModifier, 0 out of range (thiscall, ret 4, float in
	// ST0, 0x5DD4A0).
	float GetModifierValue(BS_FACEGEN_MODIFIER aeModifier);
	// Moves the eyes towards their tracking target; true when they moved.
	bool UpdateEyeTracking(float afTime, NiNode* apHeadNode);
	// bShouldHeadTrack.
	bool ShouldHeadTrack();
	// Sets bShouldHeadTrack.
	void SetShouldHeadTrack(bool abShouldHeadTrack);
	void SetDesiredEyeHeadingPitch(float afHeading, float afPitch);
	// Sets bAnimHeadCulled; culling resets the animation morphs (slot 47) first.
	void SetAnimHeadCulled(bool abCulled);
	void SetEyePosition(float afHeading, float afPitch);
	BSFaceGenAnimationData* CreateCopyBS();
	BS_FACEGEN_EXPRESSION GetCurrentExpression();
	void ModifyEyeHeadingAndPitchBasedOnExpression(BS_FACEGEN_EXPRESSION aexp, float& afHeading, float& afPitch,
		float afTime);

	// The eye tracking X and Y limits.
	static void GetTrackEyeXYMinMax(float& afMinOut, float& afMaxOut);
	// The eye tracking Z limits.
	static void GetTrackEyeZMinMax(float& afMinOut, float& afMaxOut);
	// Advances apDest by afTime through apList's keyframes, merging and freeing each one it passes and blending in
	// the next; true when a merge changed apDest.
	static bool UpdateKeyframeList(float afTime, NiTPointerList<BSFaceGenKeyframe*>* apList, BSFaceGenKeyframe* apDest,
		bool abClearDestValueIfNoUpdate);
	// Creates the "FAD" tag fixed string.
	static void InitTag();
	// IDA name KillSDM; not in the PDB. Deletes the "FAD" tag and clears it (cdecl, ret 0, 0x649EC0).
	static void KillSDM();
	static float GetExpressionTargetChangePerSec();
	static float GetExpressionStrengthAddPerSec();
	static float GetSpeechDelay();
	static void GetTrackDeadZoneXYMinMax(float& afMinOut, float& afMaxOut);
	static void GetTrackDeadZoneZMinMax(float& afMinOut, float& afMaxOut);
	static const NiFixedString& Tag();

	BSFaceGenKeyframeMultiple* pTargetExpressionData;
	BSFaceGenKeyframeMultiple CurrentTargetExpressionData;
	BSFaceGenKeyframeMultiple CurrentAnimExpressionData;
	BSFaceGenKeyframeMultiple CurrentAnimModifierData;
	BSFaceGenKeyframeMultiple CurrentAnimPhonemeData;
	BSFaceGenKeyframeMultiple CurrentAnimCustomMorphData;
	BSFaceGenKeyframeMultiple CurrentExpressionData;
	NiTPointerList<BSFaceGenKeyframe*> SpeechModifierKeyframeList;
	BSFaceGenKeyframeMultiple CurrentSpeechModifierData;
	BSFaceGenKeyframeMultiple CurrentModifierData;
	NiTPointerList<BSFaceGenKeyframe*> SpeechPhonemeKeyframeList;
	BSFaceGenKeyframeMultiple CurrentSpeechPhonemeData;
	BSFaceGenKeyframeMultiple CurrentPhonemeData;
	NiTPointerList<BSFaceGenKeyframe*> SpeechCustomMorphKeyframeList;
	BSFaceGenKeyframeMultiple CurrentSpeechCustomMorphData;
	BSFaceGenKeyframeMultiple CurrentCustomMorphData;
	NiPoint3 HeadTrackVector;
	float fCurrentTorsoHeading;
	float fCurrentTorsoPitch;
	float fCurrentHeadHeading;
	float fCurrentHeadPitch;
	float fCurrentEyeHeading;
	float fCurrentEyePitch;
	NiAVObject* pBip01Node;
	bool bAllowRotateTorso;
	uint8_t padding14D[3];
	float fDesiredEyeHeading;
	float fDesiredEyePitch;
	float fCurrentHeadTrackingSpeedH;
	float fCurrentHeadTrackingSpeedP;
	float fCurrentTorsoTrackingSpeed;
	float fDesiredTorsoHeading;
	float fDesiredTorsoPitch;
	float fDesiredHeadHeading;
	float fDesiredHeadPitch;
	ETrackingState TrackingState;
	float fJustAcquiredTimer;
	float fExpressionEyeOffsetTimer;
	float fExpressionEyeOffsetHeading;
	float fExpressionEyeOffsetPitch;
	bool bFullTrackUpdate;
	bool bShouldHeadTrack;
	bool bHeadTrackingLocked;
	bool bChangedLastUpdate;
	bool bMustUpdate;
	bool bDoNotUpdateNextTime;
	bool bDead;
	bool bAnimHeadCulled;
	bool bAnimHeadCulledLastFrame;
	bool bDisregardDisposition;
	uint8_t padding192[2];
	float fVampirismEffectAmount;
	// IDA's unk198 and unk19C; no PDB member sits here (the PDB puts AnimLock right after fVampirismEffectAmount).
	uint8_t padding198[8];
	// The constructor builds the lock at this + 0x1A0.
	BSSpinLock AnimLock;

	static NiRTTI ms_RTTI;
	// How fast an expression changes per second, and the strength it is given.
	static SettingT<GameSettingCollection> fExpressionChangePerSec;
	static SettingT<GameSettingCollection> fExpressionStrengthAdd;
	static inline NiFixedString* pTag;
	static SettingT<GameSettingCollection> fTrackEyeXY;
	static SettingT<GameSettingCollection> fTrackEyeZ;
	static SettingT<GameSettingCollection> fTrackDeadZoneXY;
	static SettingT<GameSettingCollection> fTrackDeadZoneZ;
	static SettingT<GameSettingCollection> fTrackSpeed;
	static SettingT<GameSettingCollection> fTrackXY;
	static SettingT<GameSettingCollection> fTrackMinZ;
	static SettingT<GameSettingCollection> fTrackMaxZ;
	static SettingT<GameSettingCollection> fTrackFudgeXY;
	static SettingT<GameSettingCollection> fTrackFudgeZ;
	static SettingT<GameSettingCollection> fTrackJustAcquiredDuration;
	static SettingT<GameSettingCollection> fEyePitchMaxOffsetEmotionFear;
	static SettingT<GameSettingCollection> fEyePitchMinOffsetEmotionFear;
};

NiSmartPointer(BSFaceGenAnimationData);

inline constexpr fnvd::Vtable BSFaceGenAnimationData_vftable{};

// GameSettingCollection: 0xf81332 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fEyeHeadingMaxOffsetEmotionAngry;

// GameSettingCollection: 0xf811e2 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fEyeHeadingMinOffsetEmotionHappy;

// GameSettingCollection: 0xf81212 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fEyePitchMaxOffsetEmotionSad;

// GameSettingCollection: 0xf812d2 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fEyePitchMaxOffsetEmotionAngry;

// GameSettingCollection: 0xf81122 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fLookDownDisableBlinkingAmt;

// GameSettingCollection: 0xf80eb2 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fBlinkDelayMin;

// GameSettingCollection: 0xf81242 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fEyePitchMinOffsetEmotionSad;

// GameSettingCollection: 0xf81452 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fEyePitchMaxOffsetEmotionNeutral;

// GameSettingCollection: 0xf814e2 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fEyeHeadingMinOffsetEmotionNeutral;

// GameSettingCollection: 0xf81422 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fEyeHeadingMinOffsetEmotionFear;

// GameSettingCollection: 0xf81152 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fEyePitchMaxOffsetEmotionHappy;

// GameSettingCollection: 0xf81272 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fEyeHeadingMaxOffsetEmotionSad;

// GameSettingCollection: 0xf80e82 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fBlinkUpTime;

// GameSettingCollection: 0xf81482 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fEyePitchMinOffsetEmotionNeutral;

// GameSettingCollection: 0xf813f2 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fEyeHeadingMaxOffsetEmotionFear;

// GameSettingCollection: 0xf81182 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fEyePitchMinOffsetEmotionHappy;

// GameSettingCollection: 0xf814b2 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fEyeHeadingMaxOffsetEmotionNeutral;

// GameSettingCollection: 0xf80ee2 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fBlinkDelayMax;

// GameSettingCollection: 0xf812a2 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fEyeHeadingMinOffsetEmotionSad;

// GameSettingCollection: 0xf81302 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fEyePitchMinOffsetEmotionAngry;

// GameSettingCollection: 0xf80e52 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fBlinkDownTime;

// GameSettingCollection: 0xf81362 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fEyeHeadingMinOffsetEmotionAngry;

// GameSettingCollection: 0xf811b2 calls 0x40e0b0.
extern SettingT<GameSettingCollection> gs_fEyeHeadingMaxOffsetEmotionHappy;
