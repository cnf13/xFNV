#pragma once

#include "BSCore/BSSpinLock.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/FaceGen/BSFaceGenKeyframe.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRTTI.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"

class GameSettingCollection;
class NiAVObject;
class NiNode;
class NiQuaternion;

// The facial expressions (15 expressions).
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

// The FaceGen levels of detail.
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

// The kinds of FaceGen morph, as passed to ApplyMorph.
enum BS_FACEGEN_MORPH_TYPE : int32_t {
	BS_FG_MORPH_NONE = -1,
	BS_FG_MORPH_PHONEME = 0,
	BS_FG_MORPH_EXPRESSION = 1,
	BS_FG_MORPH_MODIFIER = 2,
	BS_FG_MORPH_CUSTOM = 3,
	BS_FG_MORPH_COUNT = 4,
};

// The animated state of a FaceGen face: expression, modifier, phoneme and custom morph values, their keyframes, and the
// head and eye tracking. Only the virtuals and some members are declared; the fields are not modelled.
class BSFaceGenAnimationData : public NiExtraData {
public:
	// The head tracking state. SetHeadTrackVector stores 1 on a new target and 0 on none.
	enum ETrackingState : int32_t {
		ETrackingState_None = 0,
		ETrackingState_JustAcquired = 1,
		ETrackingState_Tracking = 2,
	};

	BSFaceGenAnimationData();
	~BSFaceGenAnimationData() override;

	const NiRTTI* GetRTTI() const override { return &ms_RTTI; }

	virtual float GetExpressionValue(BS_FACEGEN_EXPRESSION aeExpression);
	// Slot 38.
	virtual void SetExpressionValue(BS_FACEGEN_EXPRESSION aeExpression, float afValue);
	// Slot 39.
	virtual bool GetModifierValuesAsRotation(NiQuaternion& arQuat);
	// Slot 40.
	virtual void SetModifierValue(BS_FACEGEN_MODIFIER aeModifier, float afValue);
	virtual float GetPhonemeValue(BS_FACEGEN_PHONEME aePhoneme);
	// Slot 42.
	virtual void SetPhonemeValue(BS_FACEGEN_PHONEME aePhoneme, float afValue);
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
	// The body is empty.
	virtual void AddAnimEmptyKeyframe(BS_FACEGEN_MORPH_TYPE aeType, float afTime, bool abForceZero);
	// Slot 63.
	virtual void ClearKeyframeLists(bool abClearExpression, bool abClearPhonemes, bool abClearModifiers,
		bool abClearCustomMorphs);
	// Slot 64.
	virtual void ClearSpeechKeyframeLists(bool abClearExpression, bool abClearPhonemes, bool abClearModifiers,
		bool abClearCustomMorphs);
	// The body is empty.
	virtual void ClearAnimKeyframeLists(bool abClearExpression, bool abClearPhonemes, bool abClearModifiers,
		bool abClearCustomMorphs);
	// Slot 66.
	virtual bool IsSpeechKeyframeListEmpty(BS_FACEGEN_MORPH_TYPE aeType);
	// Always returns true.
	virtual bool IsAnimKeyframeListEmpty(BS_FACEGEN_MORPH_TYPE aeType);
	// Slot 68.
	virtual void AddCustomMorphKeyframe(float* apQuantities, float afTime);
	// The two SetExpressionTarget overloads: the MS ABI groups overloads at the first one and orders them in reverse
	// declaration order, so the array one is declared first to land after the single value one.
	virtual void SetExpressionTarget(float* apQuantities);
	// Slot 69.
	virtual void SetExpressionTarget(BS_FACEGEN_EXPRESSION aeExpression, float afValue);
	// Slot 71.
	virtual void SetExpressionTargetFromMood(int32_t aiMoodindex, bool abTalking);
	// Slot 72.
	virtual void ClearExpressionTarget();
	// The body is empty.
	virtual void AddAnimationDataFromNote(const char* apNote);
	// Slot 74: advances the animation by afTime; true when a value changed.
	virtual bool Update(float afTime);

	// The current value of modifier aeModifier, 0 out of range.
	float GetModifierValue(BS_FACEGEN_MODIFIER aeModifier);
	// Moves the eyes towards their tracking target; true when they moved.
	bool UpdateEyeTracking(float afTime, NiNode* apHeadNode);
	// bShouldHeadTrack.
	bool ShouldHeadTrack();
	void SetShouldHeadTrack(bool abShouldHeadTrack);
	void SetDesiredEyeHeadingPitch(float afHeading, float afPitch);
	// Culling resets the animation morphs first.
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
	// Deletes the "FAD" tag and clears it.
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

extern SettingT<GameSettingCollection> gs_fEyeHeadingMaxOffsetEmotionAngry;

extern SettingT<GameSettingCollection> gs_fEyeHeadingMinOffsetEmotionHappy;

extern SettingT<GameSettingCollection> gs_fEyePitchMaxOffsetEmotionSad;

extern SettingT<GameSettingCollection> gs_fEyePitchMaxOffsetEmotionAngry;

extern SettingT<GameSettingCollection> gs_fLookDownDisableBlinkingAmt;

extern SettingT<GameSettingCollection> gs_fBlinkDelayMin;

extern SettingT<GameSettingCollection> gs_fEyePitchMinOffsetEmotionSad;

extern SettingT<GameSettingCollection> gs_fEyePitchMaxOffsetEmotionNeutral;

extern SettingT<GameSettingCollection> gs_fEyeHeadingMinOffsetEmotionNeutral;

extern SettingT<GameSettingCollection> gs_fEyeHeadingMinOffsetEmotionFear;

extern SettingT<GameSettingCollection> gs_fEyePitchMaxOffsetEmotionHappy;

extern SettingT<GameSettingCollection> gs_fEyeHeadingMaxOffsetEmotionSad;

extern SettingT<GameSettingCollection> gs_fBlinkUpTime;

extern SettingT<GameSettingCollection> gs_fEyePitchMinOffsetEmotionNeutral;

extern SettingT<GameSettingCollection> gs_fEyeHeadingMaxOffsetEmotionFear;

extern SettingT<GameSettingCollection> gs_fEyePitchMinOffsetEmotionHappy;

extern SettingT<GameSettingCollection> gs_fEyeHeadingMaxOffsetEmotionNeutral;

extern SettingT<GameSettingCollection> gs_fBlinkDelayMax;

extern SettingT<GameSettingCollection> gs_fEyeHeadingMinOffsetEmotionSad;

extern SettingT<GameSettingCollection> gs_fEyePitchMinOffsetEmotionAngry;

extern SettingT<GameSettingCollection> gs_fBlinkDownTime;

extern SettingT<GameSettingCollection> gs_fEyeHeadingMinOffsetEmotionAngry;

extern SettingT<GameSettingCollection> gs_fEyeHeadingMaxOffsetEmotionHappy;
