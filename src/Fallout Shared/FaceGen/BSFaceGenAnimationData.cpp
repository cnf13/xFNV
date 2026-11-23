#include "Fallout Shared/FaceGen/BSFaceGenAnimationData.h"

#include "BSCore/BSSpinLock.h"
#include "BSCore/MemoryManager.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/BGSDestructibleObjectForm.h"
#include "Fallout Shared/BSAnimGroupSequence.h"
#include "Fallout Shared/FaceGen/BSFaceGenKeyframe.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/ScreenCustomSplatter.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiExtraData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiQuaternion.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTListBase.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerList.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTPointerListBase.h"

#include <cstdint>

NiRTTI BSFaceGenAnimationData::ms_RTTI("BSFaceGenAnimationData", &NiExtraData::ms_RTTI);
SettingT<GameSettingCollection> BSFaceGenAnimationData::fExpressionChangePerSec("fExpressionChangePerSec", 2.0f);
SettingT<GameSettingCollection> BSFaceGenAnimationData::fExpressionStrengthAdd("fExpressionStrengthAdd", 1.0f);
SettingT<GameSettingCollection> gs_fSpeechDelay("fSpeechDelay", 0.0f);
SettingT<GameSettingCollection> gs_fBlinkDownTime("fBlinkDownTime", 0.2f);
SettingT<GameSettingCollection> gs_fBlinkUpTime("fBlinkUpTime", 0.17f);
SettingT<GameSettingCollection> gs_fBlinkDelayMin("fBlinkDelayMin", 1.5f);
SettingT<GameSettingCollection> gs_fBlinkDelayMax("fBlinkDelayMax", 4.0f);
SettingT<GameSettingCollection> BSFaceGenAnimationData::fTrackSpeed("fTrackSpeed", 2.0f);
SettingT<GameSettingCollection> BSFaceGenAnimationData::fTrackXY("fTrackXY", 60.0f);
SettingT<GameSettingCollection> BSFaceGenAnimationData::fTrackMinZ("fTrackMinZ", -60.0f);
SettingT<GameSettingCollection> BSFaceGenAnimationData::fTrackMaxZ("fTrackMaxZ", 60.0f);
SettingT<GameSettingCollection> BSFaceGenAnimationData::fTrackEyeXY("fTrackEyeXY", 28.0f);
SettingT<GameSettingCollection> BSFaceGenAnimationData::fTrackEyeZ("fTrackEyeZ", 20.0f);
SettingT<GameSettingCollection> BSFaceGenAnimationData::fTrackDeadZoneXY("fTrackDeadZoneXY", 20.0f);
SettingT<GameSettingCollection> BSFaceGenAnimationData::fTrackDeadZoneZ("fTrackDeadZoneZ", 10.0f);
SettingT<GameSettingCollection> BSFaceGenAnimationData::fTrackFudgeXY("fTrackFudgeXY", 0.0f);
SettingT<GameSettingCollection> BSFaceGenAnimationData::fTrackFudgeZ("fTrackFudgeZ", 0.0f);
SettingT<GameSettingCollection> BSFaceGenAnimationData::fTrackJustAcquiredDuration("fTrackJustAcquiredDuration", 2.0f);
SettingT<GameSettingCollection> gs_fLookDownDisableBlinkingAmt("fLookDownDisableBlinkingAmt", 0.25f);
SettingT<GameSettingCollection> gs_fEyePitchMaxOffsetEmotionHappy("fEyePitchMaxOffsetEmotionHappy", 0.15f);
SettingT<GameSettingCollection> gs_fEyePitchMinOffsetEmotionHappy("fEyePitchMinOffsetEmotionHappy", -0.05f);
SettingT<GameSettingCollection> gs_fEyeHeadingMaxOffsetEmotionHappy("fEyeHeadingMaxOffsetEmotionHappy", 0.2f);
SettingT<GameSettingCollection> gs_fEyeHeadingMinOffsetEmotionHappy("fEyeHeadingMinOffsetEmotionHappy", -0.2f);
SettingT<GameSettingCollection> gs_fEyePitchMaxOffsetEmotionSad("fEyePitchMaxOffsetEmotionSad", -0.05f);
SettingT<GameSettingCollection> gs_fEyePitchMinOffsetEmotionSad("fEyePitchMinOffsetEmotionSad", -0.15f);
SettingT<GameSettingCollection> gs_fEyeHeadingMaxOffsetEmotionSad("fEyeHeadingMaxOffsetEmotionSad", 0.4f);
SettingT<GameSettingCollection> gs_fEyeHeadingMinOffsetEmotionSad("fEyeHeadingMinOffsetEmotionSad", -0.4f);
SettingT<GameSettingCollection> gs_fEyePitchMaxOffsetEmotionAngry("fEyePitchMaxOffsetEmotionAngry", 0.15f);
SettingT<GameSettingCollection> gs_fEyePitchMinOffsetEmotionAngry("fEyePitchMinOffsetEmotionAngry", -0.05f);
SettingT<GameSettingCollection> gs_fEyeHeadingMaxOffsetEmotionAngry("fEyeHeadingMaxOffsetEmotionAngry", 0.1f);
SettingT<GameSettingCollection> gs_fEyeHeadingMinOffsetEmotionAngry("fEyeHeadingMinOffsetEmotionAngry", -0.1f);
SettingT<GameSettingCollection>
    BSFaceGenAnimationData::fEyePitchMaxOffsetEmotionFear("fEyePitchMaxOffsetEmotionFear", 0.0f);
SettingT<GameSettingCollection>
    BSFaceGenAnimationData::fEyePitchMinOffsetEmotionFear("fEyePitchMinOffsetEmotionFear", 0.0f);
SettingT<GameSettingCollection> gs_fEyeHeadingMaxOffsetEmotionFear("fEyeHeadingMaxOffsetEmotionFear", 0.5f);
SettingT<GameSettingCollection> gs_fEyeHeadingMinOffsetEmotionFear("fEyeHeadingMinOffsetEmotionFear", -0.5f);
SettingT<GameSettingCollection> gs_fEyePitchMaxOffsetEmotionNeutral("fEyePitchMaxOffsetEmotionNeutral", 0.15f);
SettingT<GameSettingCollection> gs_fEyePitchMinOffsetEmotionNeutral("fEyePitchMinOffsetEmotionNeutral", -0.05f);
SettingT<GameSettingCollection> gs_fEyeHeadingMaxOffsetEmotionNeutral("fEyeHeadingMaxOffsetEmotionNeutral", 0.2f);
SettingT<GameSettingCollection> gs_fEyeHeadingMinOffsetEmotionNeutral("fEyeHeadingMinOffsetEmotionNeutral", -0.2f);

BSFaceGenAnimationData::BSFaceGenAnimationData()
  : NiExtraData(Tag()), pTargetExpressionData(nullptr), CurrentTargetExpressionData(BS_FG_MORPH_EXPRESSION, 15u),
    CurrentAnimExpressionData(BS_FG_MORPH_EXPRESSION, 15u), CurrentAnimModifierData(BS_FG_MORPH_MODIFIER, 17u),
    CurrentAnimPhonemeData(BS_FG_MORPH_PHONEME, 16u), CurrentAnimCustomMorphData(BS_FG_MORPH_CUSTOM, 1u),
    CurrentExpressionData(BS_FG_MORPH_EXPRESSION, 15u), CurrentSpeechModifierData(BS_FG_MORPH_MODIFIER, 17u),
    CurrentModifierData(BS_FG_MORPH_MODIFIER, 17u), CurrentSpeechPhonemeData(BS_FG_MORPH_PHONEME, 16u),
    CurrentPhonemeData(BS_FG_MORPH_PHONEME, 16u), CurrentSpeechCustomMorphData(BS_FG_MORPH_CUSTOM, 1u),
    CurrentCustomMorphData(BS_FG_MORPH_CUSTOM, 1u), HeadTrackVector(NiPoint3::ZERO), fCurrentTorsoHeading(0.0f),
    fCurrentTorsoPitch(0.0f), fCurrentHeadHeading(0.0f), fCurrentHeadPitch(0.0f), fCurrentEyeHeading(0.0f),
    fCurrentEyePitch(0.0f), pBip01Node(nullptr), bAllowRotateTorso(true), fDesiredEyeHeading(0.0f),
    fDesiredEyePitch(0.0f), fCurrentHeadTrackingSpeedH(0.0f), fCurrentHeadTrackingSpeedP(0.0f),
    fCurrentTorsoTrackingSpeed(0.0f), fDesiredTorsoHeading(0.0f), fDesiredTorsoPitch(0.0f), fDesiredHeadHeading(0.0f),
    fDesiredHeadPitch(0.0f), TrackingState(ETrackingState_None), fJustAcquiredTimer(0.0f),
    fExpressionEyeOffsetTimer(0.0f), bFullTrackUpdate(false), bShouldHeadTrack(true), bChangedLastUpdate(false),
    bMustUpdate(false), bDoNotUpdateNextTime(false), bDead(false), bAnimHeadCulled(false),
    bAnimHeadCulledLastFrame(false), bDisregardDisposition(false), fVampirismEffectAmount(0.0f) {}

// The keyframes the lists own are freed here; the members are the compiler's.
BSFaceGenAnimationData::~BSFaceGenAnimationData() {
    BSFaceGenAnimationData::ClearKeyframeLists(true, true, true, true);
}

float BSFaceGenAnimationData::GetExpressionValue(BS_FACEGEN_EXPRESSION aeExpression) {
    if (aeExpression >= BS_FG_EXPRESSION_COUNT) {
        return 0.0f;
    }

    return CurrentExpressionData.GetQuantity(aeExpression);
}

void BSFaceGenAnimationData::SetExpressionValue(BS_FACEGEN_EXPRESSION aeExpression, float afValue) {
    if (aeExpression < BS_FG_EXPRESSION_COUNT) {
        CurrentTargetExpressionData.SetQuantityNoTest(aeExpression, afValue);
    }
}

bool BSFaceGenAnimationData::GetModifierValuesAsRotation(NiQuaternion& arQuat) {
    float fYawQuantity = CurrentModifierData.GetQuantity(BS_FG_MODIFIER_HEAD_YAW);
    float fRollQuantity = CurrentModifierData.GetQuantity(BS_FG_MODIFIER_HEAD_ROLL);
    float fPitchQuantity = CurrentModifierData.GetQuantity(BS_FG_MODIFIER_HEAD_PITCH);
    if (fYawQuantity == 0x1.fffffe0000000p+127) {
        fYawQuantity = 0.0f;
    }
    double dClampedYaw;
    if (!(fYawQuantity > -1.5707963267948966)) {
        dClampedYaw = -1.5707963267948966;
    }
    else if (!(fYawQuantity < 1.5707963267948966)) {
        dClampedYaw = 1.5707963267948966;
    }
    else {
        dClampedYaw = fYawQuantity;
    }
    if (fRollQuantity == 0x1.fffffe0000000p+127) {
        fRollQuantity = 0.0f;
    }
    double dClampedRoll;
    if (!(fRollQuantity > -1.5707963267948966)) {
        dClampedRoll = -1.5707963267948966;
    }
    else if (!(fRollQuantity < 1.5707963267948966)) {
        dClampedRoll = 1.5707963267948966;
    }
    else {
        dClampedRoll = fRollQuantity;
    }
    if (fPitchQuantity == 0x1.fffffe0000000p+127) {
        fPitchQuantity = 0.0f;
    }
    double dClampedPitch;
    if (!(fPitchQuantity > -1.5707963267948966)) {
        dClampedPitch = -1.5707963267948966;
    }
    else if (!(fPitchQuantity < 1.5707963267948966)) {
        dClampedPitch = 1.5707963267948966;
    }
    else {
        dClampedPitch = fPitchQuantity;
    }
    float fPitch = static_cast<float>(dClampedPitch);
    float fRoll = static_cast<float>(dClampedRoll);
    float fYaw = static_cast<float>(dClampedYaw);
    arQuat.FromEulerAnglesXYZ(fYaw, fRoll, fPitch);
    return arQuat != NiQuaternion::IDENTITY;
}

void BSFaceGenAnimationData::SetModifierValue(BS_FACEGEN_MODIFIER aeModifier, float afValue) {
    if (aeModifier < BS_FG_MODIFIER_COUNT) {
        CurrentSpeechModifierData.SetQuantityNoTest(aeModifier, afValue);
    }
}

float BSFaceGenAnimationData::GetPhonemeValue(BS_FACEGEN_PHONEME aePhoneme) {
    if (aePhoneme >= BS_FG_PHONEME_COUNT) {
        return 0.0f;
    }

    return CurrentPhonemeData.GetQuantity(aePhoneme);
}

void BSFaceGenAnimationData::SetPhonemeValue(BS_FACEGEN_PHONEME aePhoneme, float afValue) {
    if (aePhoneme < BS_FG_PHONEME_COUNT) {
        CurrentSpeechPhonemeData.SetQuantityNoTest(aePhoneme, afValue);
    }
}

float BSFaceGenAnimationData::GetCustomMorphValue(BS_FACEGEN_CUSTOM_MORPH aeCustomMorph) {
    if (aeCustomMorph >= BS_FG_CUSTOM_MORPH_COUNT) {
        return 0.0f;
    }

    return CurrentCustomMorphData.GetQuantity(aeCustomMorph);
}

void BSFaceGenAnimationData::SetCustomMorphValue(BS_FACEGEN_CUSTOM_MORPH aeCustomMorph, float afValue) {
    if (aeCustomMorph < BS_FG_CUSTOM_MORPH_COUNT) {
        CurrentSpeechCustomMorphData.SetQuantityNoTest(aeCustomMorph, afValue);
    }
}

void BSFaceGenAnimationData::ResetCurrentMorphs(
    float afTime,
    bool abResetExpression,
    bool abResetPhoneme,
    bool abResetModifier,
    bool abResetCustomMorph,
    bool abForDeath
) {
    AnimLock.Lock(nullptr);
    ResetSpeechCurrentMorphs(
        afTime,
        abResetExpression,
        abResetPhoneme,
        abResetModifier,
        abResetCustomMorph,
        abForDeath
    );
    ResetAnimCurrentMorphs(afTime, abResetExpression, abResetPhoneme, abResetModifier, abResetCustomMorph);
    if (afTime == 0.0) {
        if (abResetExpression) {
            CurrentExpressionData.SetTime(0.0f);
            CurrentExpressionData.ClearData(true);
        }
        if (abResetModifier) {
            CurrentModifierData.SetTime(0.0f);
            CurrentModifierData.ClearData(true);
            if (abForDeath) {
                CurrentModifierData.SetQuantity(0, 1.0);
                CurrentModifierData.SetQuantity(1u, 1.0);
            }
        }
        if (abResetPhoneme) {
            CurrentPhonemeData.SetTime(0.0f);
            CurrentPhonemeData.ClearData(true);
        }
        if (abResetCustomMorph) {
            CurrentCustomMorphData.SetTime(0.0f);
            CurrentCustomMorphData.ClearData(true);
        }
    }
    AnimLock.Unlock();
}

void BSFaceGenAnimationData::ResetSpeechCurrentMorphs(
    float afTime,
    bool abResetExpression,
    bool abResetPhoneme,
    bool abResetModifier,
    bool abResetCustomMorph,
    bool abForDeath
) {
    ClearKeyframeLists(abResetExpression, abResetPhoneme, abResetModifier, abResetCustomMorph);
    if (abResetExpression) {
        CurrentTargetExpressionData.SetTime(0.0);
        if (!(afTime > 0.0)) {
            CurrentTargetExpressionData.ClearData(true);
        }
        else {
            SetExpressionTarget(BS_FG_EXPRESSION_NONE, 1.0f);
        }
    }
    if (abResetModifier) {
        CurrentSpeechModifierData.SetTime(0.0);
        if (!(afTime > 0.0)) {
            CurrentSpeechModifierData.ClearData(true);
            if (abForDeath) {
                SetModifierValue(BS_FG_MODIFIER_BLINK_LEFT, 1.0f);
                SetModifierValue(BS_FG_MODIFIER_BLINK_RIGHT, 1.0f);
            }
        }
        else if (abForDeath) {
            float apQuantities[18];
            BSFaceGenManager::ResetQuantityArray(apQuantities, 17u, true);
            apQuantities[0] = 1.0f;
            apQuantities[1] = 1.0f;
            AddModifiersKeyframe(apQuantities, afTime);
        }
        else {
            AddSpeechEmptyKeyframe(BS_FG_MORPH_MODIFIER, afTime, true);
        }
    }
    if (abResetPhoneme) {
        CurrentSpeechPhonemeData.SetTime(0.0);
        if (!(afTime > 0.0)) {
            CurrentSpeechPhonemeData.ClearData(true);
        }
        else {
            AddSpeechEmptyKeyframe(BS_FG_MORPH_PHONEME, afTime, true);
        }
    }
    if (abResetCustomMorph) {
        CurrentSpeechCustomMorphData.SetTime(0.0);
        if (!(afTime > 0.0)) {
            CurrentSpeechCustomMorphData.ClearData(true);
        }
        else {
            AddSpeechEmptyKeyframe(BS_FG_MORPH_CUSTOM, afTime, true);
        }
    }
}

void BSFaceGenAnimationData::ResetAnimCurrentMorphs(
    float afTime,
    bool abResetExpression,
    bool abResetPhoneme,
    bool abResetModifier,
    bool abResetCustomMorph
) {
    ClearAnimKeyframeLists(abResetExpression, abResetPhoneme, abResetModifier, abResetCustomMorph);
    if (abResetExpression) {
        CurrentAnimExpressionData.SetTime(0.0);
        if (!(afTime > 0.0)) {
            CurrentAnimExpressionData.ClearData(true);
        }
        else {
            AddAnimEmptyKeyframe(BS_FG_MORPH_EXPRESSION, afTime, true);
        }
    }
    if (abResetModifier) {
        CurrentAnimModifierData.SetTime(0.0);
        if (!(afTime > 0.0)) {
            CurrentAnimModifierData.ClearData(true);
        }
        else {
            AddAnimEmptyKeyframe(BS_FG_MORPH_MODIFIER, afTime, true);
        }
    }
    if (abResetPhoneme) {
        CurrentAnimPhonemeData.SetTime(0.0);
        if (!(afTime > 0.0)) {
            CurrentAnimPhonemeData.ClearData(true);
        }
        else {
            AddAnimEmptyKeyframe(BS_FG_MORPH_PHONEME, afTime, true);
        }
    }
    if (abResetCustomMorph) {
        CurrentAnimCustomMorphData.SetTime(0.0);
        if (!(afTime > 0.0)) {
            CurrentAnimCustomMorphData.ClearData(true);
        }
        else {
            AddAnimEmptyKeyframe(BS_FG_MORPH_CUSTOM, afTime, true);
        }
    }
}

void BSFaceGenAnimationData::SetHeadTrackVector(NiPoint3 aPoint, bool abInstant) {
    if (HeadTrackVector == NiPoint3::ZERO && aPoint != NiPoint3::ZERO) {
        TrackingState = ETrackingState_JustAcquired;
        fJustAcquiredTimer = 0.0f;
    }
    if (aPoint == NiPoint3::ZERO) {
        TrackingState = ETrackingState_None;
    }
    HeadTrackVector = aPoint;
    bShouldHeadTrack = true;
}

void BSFaceGenAnimationData::ClearHeadTracking(bool abInstant) {
    HeadTrackVector = NiPoint3::ZERO;
    bFullTrackUpdate = abInstant;
}

bool BSFaceGenAnimationData::GetChangedLastUpdate() {
    return bChangedLastUpdate;
}

bool BSFaceGenAnimationData::GetMustUpdate() {
    return bMustUpdate;
}

void BSFaceGenAnimationData::SetMustUpdate(bool abMustUpdate) {
    bMustUpdate = abMustUpdate;
}

bool BSFaceGenAnimationData::GetDead() {
    return bDead;
}

void BSFaceGenAnimationData::SetDead(bool abDead, bool abInstant) {
    if (!abDead || bDead) {
        if (!abDead && bDead) {
            bDead = false;
            if (abInstant) {
                ResetCurrentMorphs(0.0, true, true, true, true, false);
            }
            else {
                ResetCurrentMorphs(1.0, true, true, true, true, false);
            }
            bChangedLastUpdate = true;
            bShouldHeadTrack = true;
        }
    }
    else {
        if (abInstant) {
            ResetCurrentMorphs(0.0, true, true, true, true, true);
        }
        else {
            ResetCurrentMorphs(1.0, true, true, true, true, true);
        }
        bDead = abDead;
        bChangedLastUpdate = true;
    }
}

void BSFaceGenAnimationData::SetAnimExpressionValue(BS_FACEGEN_EXPRESSION aeExpression, float afValue) {
    if (aeExpression < BS_FG_EXPRESSION_COUNT) {
        CurrentAnimExpressionData.SetQuantityNoTest(aeExpression, afValue);
    }
}

void BSFaceGenAnimationData::SetAnimModifierValue(BS_FACEGEN_MODIFIER aeModifier, float afValue) {
    if (aeModifier < BS_FG_MODIFIER_COUNT) {
        CurrentAnimModifierData.SetQuantityNoTest(aeModifier, afValue);
    }
}

void BSFaceGenAnimationData::SetAnimPhonemeValue(BS_FACEGEN_PHONEME aePhoneme, float afValue) {
    if (aePhoneme < BS_FG_PHONEME_COUNT) {
        CurrentAnimPhonemeData.SetQuantityNoTest(aePhoneme, afValue);
    }
}

void BSFaceGenAnimationData::SetAnimCustomMorphValue(BS_FACEGEN_CUSTOM_MORPH aeCustomMorph, float afValue) {
    if (aeCustomMorph < BS_FG_CUSTOM_MORPH_COUNT) {
        CurrentAnimCustomMorphData.SetQuantityNoTest(aeCustomMorph, afValue);
    }
}

void BSFaceGenAnimationData::AddPhonemeKeyframe(float* apQuantities, float afTime) {
    BSFaceGenKeyframeMultiple morph(BS_FG_MORPH_PHONEME, BS_FG_PHONEME_COUNT);
    if (!bDead && apQuantities && !(afTime < 0.0)) {
        uint32_t i;
        for (i = 0; i < BS_FG_PHONEME_COUNT; ++i) {
            if (apQuantities[i] < 0.0 || apQuantities[i] > 1.0) {
                return;
            }
        }
        morph.BSFaceGenKeyframe::SetTime(afTime);
        for (i = 0; i < BS_FG_PHONEME_COUNT; ++i) {
            morph.SetQuantity(i, apQuantities[i]);
        }
        morph.BSFaceGenKeyframeMultiple::AddToList(&SpeechPhonemeKeyframeList);
    }
}

void BSFaceGenAnimationData::AddModifiersKeyframe(float* apQuantities, float afTime) {
    BSFaceGenKeyframeMultiple morph(BS_FG_MORPH_MODIFIER, BS_FG_MODIFIER_COUNT);
    if (!bDead && apQuantities && !(afTime < 0.0)) {
        morph.BSFaceGenKeyframe::SetTime(afTime);
        for (uint32_t i = 0; i < BS_FG_MODIFIER_COUNT; ++i) {
            morph.SetQuantity(i, apQuantities[i]);
        }
        morph.BSFaceGenKeyframeMultiple::AddToList(&SpeechModifierKeyframeList);
    }
}

void BSFaceGenAnimationData::AddSpeechEmptyKeyframe(BS_FACEGEN_MORPH_TYPE aeType, float afTime, bool abForceZero) {
    if (!bDead) {
        if (aeType) {
            if (aeType == BS_FG_MORPH_MODIFIER) {
                BSFaceGenKeyframeMultiple kKeyframe(BS_FG_MORPH_MODIFIER, 17u);
                kKeyframe.BSFaceGenKeyframe::SetTime(afTime);
                kKeyframe.BSFaceGenKeyframeMultiple::AddToList(&SpeechModifierKeyframeList);
            }
            else if (aeType == BS_FG_MORPH_CUSTOM) {
                BSFaceGenKeyframeMultiple kKeyframe(BS_FG_MORPH_CUSTOM, 1u);
                kKeyframe.BSFaceGenKeyframe::SetTime(afTime);
                kKeyframe.BSFaceGenKeyframeMultiple::AddToList(&SpeechCustomMorphKeyframeList);
            }
        }
        else {
            BSFaceGenKeyframeMultiple kKeyframe(BS_FG_MORPH_PHONEME, 16u);
            kKeyframe.BSFaceGenKeyframe::SetTime(afTime);
            kKeyframe.BSFaceGenKeyframeMultiple::AddToList(&SpeechPhonemeKeyframeList);
        }
    }
}

void BSFaceGenAnimationData::AddAnimEmptyKeyframe(BS_FACEGEN_MORPH_TYPE aeType, float afTime, bool abForceZero) {}

void BSFaceGenAnimationData::ClearKeyframeLists(
    bool abClearExpression,
    bool abClearPhonemes,
    bool abClearModifiers,
    bool abClearCustomMorphs
) {
    ClearSpeechKeyframeLists(abClearExpression, abClearPhonemes, abClearModifiers, abClearCustomMorphs);
    ClearAnimKeyframeLists(abClearExpression, abClearPhonemes, abClearModifiers, abClearCustomMorphs);
}

void BSFaceGenAnimationData::ClearSpeechKeyframeLists(
    bool abClearExpression,
    bool abClearPhonemes,
    bool abClearModifiers,
    bool abClearCustomMorphs
) {
    if (abClearExpression) {
        ClearExpressionTarget();
    }
    if (abClearModifiers && SpeechModifierKeyframeList.m_uiCount) {
        BSFaceGenKeyframe* pNext;
        for (BSFaceGenKeyframe* pDelete = SpeechModifierKeyframeList.GetHead(); pDelete; pDelete = pNext) {
            delete pDelete;
            SpeechModifierKeyframeList.RemoveHead();
            if (SpeechModifierKeyframeList.m_uiCount) {
                pNext = SpeechModifierKeyframeList.GetHead();
            }
            else {
                pNext = nullptr;
            }
        }
    }

    if (abClearPhonemes && SpeechPhonemeKeyframeList.m_uiCount) {
        BSFaceGenKeyframe* pNext;
        for (BSFaceGenKeyframe* pDelete = SpeechPhonemeKeyframeList.GetHead(); pDelete; pDelete = pNext) {
            delete pDelete;
            SpeechPhonemeKeyframeList.RemoveHead();
            if (SpeechPhonemeKeyframeList.m_uiCount) {
                pNext = SpeechPhonemeKeyframeList.GetHead();
            }
            else {
                pNext = nullptr;
            }
        }
    }

    if (abClearCustomMorphs && SpeechCustomMorphKeyframeList.m_uiCount) {
        BSFaceGenKeyframe* pNext;
        for (BSFaceGenKeyframe* pDelete = SpeechCustomMorphKeyframeList.GetHead(); pDelete; pDelete = pNext) {
            delete pDelete;
            SpeechCustomMorphKeyframeList.RemoveHead();
            if (SpeechCustomMorphKeyframeList.m_uiCount) {
                pNext = SpeechCustomMorphKeyframeList.GetHead();
            }
            else {
                pNext = nullptr;
            }
        }
    }
}

void BSFaceGenAnimationData::ClearAnimKeyframeLists(
    bool abClearExpression,
    bool abClearPhonemes,
    bool abClearModifiers,
    bool abClearCustomMorphs
) {}

bool BSFaceGenAnimationData::IsSpeechKeyframeListEmpty(BS_FACEGEN_MORPH_TYPE aeType) {
    switch (aeType) {
        case 0: {
            return SpeechPhonemeKeyframeList.m_uiCount == 0;
        }
        case 2: {
            return SpeechModifierKeyframeList.m_uiCount == 0;
        }
        case 3: {
            return SpeechCustomMorphKeyframeList.m_uiCount == 0;
        }
    }
    return true;
}

bool BSFaceGenAnimationData::IsAnimKeyframeListEmpty(BS_FACEGEN_MORPH_TYPE aeType) {
    return true;
}

void BSFaceGenAnimationData::AddCustomMorphKeyframe(float* apQuantities, float afTime) {
    alignas(BSFaceGenKeyframeMultiple) uint8_t apThis[sizeof(BSFaceGenKeyframeMultiple)];
    ::new (&reinterpret_cast<BSFaceGenKeyframeMultiple&>(apThis)) BSFaceGenKeyframeMultiple(BS_FG_MORPH_CUSTOM, 1u);
    if (!bDead && apQuantities && !(afTime < 0.0)) {
        reinterpret_cast<BSFaceGenKeyframeMultiple&>(apThis).BSFaceGenKeyframe::SetTime(afTime);
        for (uint32_t aiWhichMorph = 0; !aiWhichMorph; aiWhichMorph = 1) {
            reinterpret_cast<BSFaceGenKeyframeMultiple&>(apThis).SetQuantity(0, *apQuantities);
        }
        reinterpret_cast<BSFaceGenKeyframeMultiple&>(apThis).BSFaceGenKeyframeMultiple::AddToList(
            &SpeechCustomMorphKeyframeList
        );
        reinterpret_cast<BSFaceGenKeyframeMultiple&>(apThis).~BSFaceGenKeyframeMultiple();
    }
    else {
        reinterpret_cast<BSFaceGenKeyframeMultiple&>(apThis).~BSFaceGenKeyframeMultiple();
    }
}

void BSFaceGenAnimationData::SetExpressionTarget(float* apQuantities) {
    if (!bDead && apQuantities) {
        if (pTargetExpressionData) {
            delete pTargetExpressionData;
        }
        pTargetExpressionData = new BSFaceGenKeyframeMultiple(BS_FG_MORPH_EXPRESSION, 15u);
        for (uint32_t iExp = 0; iExp < 15; ++iExp) {
            pTargetExpressionData->SetQuantity(iExp, apQuantities[iExp]);
        }
        if (CurrentTargetExpressionData.Compare(pTargetExpressionData)) {
            return;
        }
    }
    ClearExpressionTarget();
}

void BSFaceGenAnimationData::SetExpressionTarget(BS_FACEGEN_EXPRESSION aeExpression, float afValue) {
    if (aeExpression >= BS_FG_EXPRESSION_NONE &&
        aeExpression <= BS_FG_EXPRESSION_COUNT &&
        !(afValue < 0.0) &&
        !(afValue > 1.0)) {
        if (pTargetExpressionData) {
            pTargetExpressionData->ClearData(true);
        }
        else {
            pTargetExpressionData = new BSFaceGenKeyframeMultiple(BS_FG_MORPH_EXPRESSION, 15u);
            pTargetExpressionData->SetToZero();
        }
        if (aeExpression != BS_FG_EXPRESSION_NONE && afValue != 0.0) {
            pTargetExpressionData->SetQuantity(aeExpression, afValue);
            if (!CurrentTargetExpressionData.Compare(pTargetExpressionData)) {
                ClearExpressionTarget();
            }
        }
    }
}

void BSFaceGenAnimationData::SetExpressionTargetFromMood(int32_t aiMoodindex, bool abTalking) {
    if (pTargetExpressionData) {
        pTargetExpressionData->ClearData(true);
    }
    else {
        pTargetExpressionData = new BSFaceGenKeyframeMultiple(BS_FG_MORPH_EXPRESSION, 15u);
        pTargetExpressionData->SetToZero();
    }
    if (!bDead) {
        switch (aiMoodindex) {
            case 0: {
                pTargetExpressionData->SetQuantity(5u, 1.0f);
                break;
            }
            case 1: {
                pTargetExpressionData->SetQuantity(6u, 1.0f);
                break;
            }
            case 2: {
                pTargetExpressionData->SetQuantity(7u, 1.0f);
                break;
            }
            case 3: {
                pTargetExpressionData->SetQuantity(8u, 1.0f);
                break;
            }
            case 4: {
                pTargetExpressionData->SetQuantity(9u, 1.0f);
                break;
            }
            case 5: {
                pTargetExpressionData->SetQuantity(10u, 1.0f);
                break;
            }
            case 6: {
                pTargetExpressionData->SetQuantity(11u, 1.0f);
                break;
            }
            case 7: {
                pTargetExpressionData->SetQuantity(12u, 1.0f);
                break;
            }
            default: {
                break;
            }
        }
        if (!CurrentTargetExpressionData.Compare(pTargetExpressionData)) {
            ClearExpressionTarget();
        }
    }
}

void BSFaceGenAnimationData::ClearExpressionTarget() {
    if (pTargetExpressionData) {
        delete pTargetExpressionData;
    }
    pTargetExpressionData = nullptr;
}

void BSFaceGenAnimationData::AddAnimationDataFromNote(const char* apNote) {}

bool BSFaceGenAnimationData::Update(float afTime) {
    AnimLock.Lock(nullptr);
    float fLookDown = CurrentModifierData.GetQuantity(BS_FG_MODIFIER_LOOK_DOWN);
    if (BSFaceGenManager::GetUseProceduralEyeBlinking() &&
        !bDead &&
        SpeechModifierKeyframeList.IsEmpty() &&
        gs_fBlinkDownTime.Float() > 0.0 &&
        gs_fBlinkUpTime.Float() > 0.0 &&
        gs_fBlinkDelayMin.Float() > 0.0) {
        if (gs_fBlinkDelayMin.Float() <= static_cast<double>(gs_fBlinkDelayMax.Float())) {
            if (gs_fLookDownDisableBlinkingAmt.Float() > static_cast<double>(fLookDown)) {
                // ResetQuantityArray fills 17 modifier quantities and AddModifiersKeyframe reads them.
                float apQuantities[BS_FG_MODIFIER_COUNT];
                float fBlinkDelayRange = gs_fBlinkDelayMax.Float() - gs_fBlinkDelayMin.Float();
                BSFaceGenManager::ResetQuantityArray(apQuantities, 17u, false);
                apQuantities[0] = 0.0f;
                apQuantities[1] = 0.0f;
                float fBlinkDelay = RandomFloat0To1() * fBlinkDelayRange + gs_fBlinkDelayMin.Float();
                AddModifiersKeyframe(apQuantities, fBlinkDelay);
                BSFaceGenManager::ResetQuantityArray(apQuantities, 17u, false);
                apQuantities[0] = 1.0f;
                apQuantities[1] = 1.0f;
                AddModifiersKeyframe(apQuantities, gs_fBlinkDownTime.Float());
                BSFaceGenManager::ResetQuantityArray(apQuantities, 17u, false);
                apQuantities[0] = 0.0f;
                apQuantities[1] = 0.0f;
                AddModifiersKeyframe(apQuantities, gs_fBlinkUpTime.Float());
            }
        }
    }
    if (afTime > 0.0) {
        // UpdateToTarget, IsTargetAvailable, CopyFrom and MergeMorphDataFrom are called on the keyframes through base
        // pointers so that they dispatch through the vtable; calls on the members themselves would be
        // devirtualised.
        BSFaceGenKeyframe* pTargetExpression = &CurrentTargetExpressionData;
        BSFaceGenKeyframe* pAnimExpression = &CurrentAnimExpressionData;
        BSFaceGenKeyframe* pExpression = &CurrentExpressionData;
        BSFaceGenKeyframe* pAnimModifier = &CurrentAnimModifierData;
        BSFaceGenKeyframe* pModifier = &CurrentModifierData;
        BSFaceGenKeyframe* pAnimPhoneme = &CurrentAnimPhonemeData;
        BSFaceGenKeyframe* pPhoneme = &CurrentPhonemeData;
        BSFaceGenKeyframe* pAnimCustomMorph = &CurrentAnimCustomMorphData;
        BSFaceGenKeyframe* pCustomMorph = &CurrentCustomMorphData;
        bChangedLastUpdate = false;
        bool bChangedSpeech = pTargetExpression->UpdateToTarget(afTime, pTargetExpressionData);
        bool bChangedAnim = (!bAnimHeadCulledLastFrame && bAnimHeadCulled) || pAnimExpression->IsTargetAvailable();
        if (!bChangedAnim && !bChangedSpeech) {
            ClearExpressionTarget();
        }
        if (bChangedSpeech) {
            pExpression->CopyFrom(&CurrentTargetExpressionData);
            if (bChangedAnim) {
                pExpression->MergeMorphDataFrom(&CurrentAnimExpressionData, 1.0f, false, false);
            }
        }
        else if (bChangedAnim) {
            pExpression->CopyFrom(&CurrentAnimExpressionData);
        }
        bool bChangedExpression = bChangedSpeech || bChangedAnim;
        bChangedSpeech = BSFaceGenAnimationData::UpdateKeyframeList(
            afTime,
            &SpeechModifierKeyframeList,
            &CurrentSpeechModifierData,
            false
        );
        bChangedAnim = (!bAnimHeadCulledLastFrame && bAnimHeadCulled) || pAnimModifier->IsTargetAvailable();
        if (bChangedSpeech) {
            pModifier->CopyFrom(&CurrentSpeechModifierData);
            if (bChangedAnim) {
                pModifier->MergeMorphDataFrom(&CurrentAnimModifierData, 1.0f, false, false);
            }
        }
        else if (bChangedAnim) {
            pModifier->CopyFrom(&CurrentAnimModifierData);
        }
        bool bChangedModifier = bChangedSpeech || bChangedAnim;
        bChangedSpeech = BSFaceGenAnimationData::UpdateKeyframeList(
            afTime,
            &SpeechPhonemeKeyframeList,
            &CurrentSpeechPhonemeData,
            false
        );
        bChangedAnim = (!bAnimHeadCulledLastFrame && bAnimHeadCulled) || pAnimPhoneme->IsTargetAvailable();
        if (bChangedSpeech) {
            pPhoneme->CopyFrom(&CurrentSpeechPhonemeData);
            if (bChangedAnim) {
                pPhoneme->MergeMorphDataFrom(&CurrentAnimPhonemeData, 1.0f, false, false);
            }
        }
        else if (bChangedAnim) {
            pPhoneme->CopyFrom(&CurrentAnimPhonemeData);
        }
        bool bChangedPhoneme = bChangedSpeech || bChangedAnim;
        bChangedSpeech = BSFaceGenAnimationData::UpdateKeyframeList(
            afTime,
            &SpeechCustomMorphKeyframeList,
            &CurrentSpeechCustomMorphData,
            false
        );
        bChangedAnim = (!bAnimHeadCulledLastFrame && bAnimHeadCulled) || pAnimCustomMorph->IsTargetAvailable();
        if (bChangedSpeech) {
            pCustomMorph->CopyFrom(&CurrentSpeechCustomMorphData);
            if (bChangedAnim) {
                pCustomMorph->MergeMorphDataFrom(&CurrentAnimCustomMorphData, 1.0f, false, false);
            }
        }
        else if (bChangedAnim) {
            pCustomMorph->CopyFrom(&CurrentAnimCustomMorphData);
        }
        bool bChangedCustomMorph = bChangedSpeech || bChangedAnim;
        bChangedLastUpdate = bChangedExpression || bChangedModifier || bChangedPhoneme || bChangedCustomMorph;
    }
    bAnimHeadCulledLastFrame = bAnimHeadCulled;
    AnimLock.Unlock();
    return bChangedLastUpdate;
}

float BSFaceGenAnimationData::GetModifierValue(BS_FACEGEN_MODIFIER aeModifier) {
    if (aeModifier >= BS_FG_MODIFIER_COUNT) {
        return 0.0f;
    }

    return CurrentModifierData.GetQuantity(aeModifier);
}

bool BSFaceGenAnimationData::UpdateEyeTracking(float afTime, NiNode* apHeadNode) {
    AnimLock.Lock(nullptr);
    bool bRes = !bDead;
    if (bRes) {
        bRes = true;
        BS_FACEGEN_EXPRESSION mostSignificantExpression = GetCurrentExpression();
        float fExpModDesiredHeading = fDesiredEyeHeading;
        float fExpModDesiredPitch = fDesiredEyePitch;
        if (mostSignificantExpression < BS_FG_EXPRESSION_COUNT) {
            ModifyEyeHeadingAndPitchBasedOnExpression(
                mostSignificantExpression,
                fExpModDesiredHeading,
                fExpModDesiredPitch,
                afTime
            );
        }
        float fEyeHeadingDelta = fExpModDesiredHeading - fCurrentEyeHeading;
        float fEyePitchDelta = fExpModDesiredPitch - fCurrentEyePitch;
        float fMaxAbsEyeDeltaThisTime = BSFaceGenAnimationData::fTrackSpeed.Float() * afTime;
        BSClamp(fEyeHeadingDelta, -fMaxAbsEyeDeltaThisTime, fMaxAbsEyeDeltaThisTime);
        BSClamp(fEyePitchDelta, -fMaxAbsEyeDeltaThisTime, fMaxAbsEyeDeltaThisTime);
        fCurrentEyeHeading = fCurrentEyeHeading + fEyeHeadingDelta;
        fCurrentEyePitch = fCurrentEyePitch + fEyePitchDelta;
        SetEyePosition(fCurrentEyeHeading, fCurrentEyePitch);
    }
    AnimLock.Unlock();
    return bRes;
}

bool BSFaceGenAnimationData::ShouldHeadTrack() {
    return bShouldHeadTrack;
}

void BSFaceGenAnimationData::SetShouldHeadTrack(bool abShouldHeadTrack) {
    bShouldHeadTrack = abShouldHeadTrack;
}

void BSFaceGenAnimationData::SetDesiredEyeHeadingPitch(float afHeading, float afPitch) {
    fDesiredEyeHeading = afHeading;
    fDesiredEyePitch = afPitch;
}

void BSFaceGenAnimationData::SetAnimHeadCulled(bool abCulled) {
    if (abCulled != bAnimHeadCulled) {
        if (abCulled) {
            ResetAnimCurrentMorphs(0.0f, true, true, true, true);
        }
        bAnimHeadCulled = abCulled;
    }
}

void BSFaceGenAnimationData::SetEyePosition(float afHeading, float afPitch) {
    float fEyeZMax;
    float fEyeXYMin;
    float fEyeXYMax;
    float fEyeZMin;
    if (BSFaceGenManager::GetUseProceduralEyeBlinking()) {
        BSFaceGenAnimationData::GetTrackEyeXYMinMax(fEyeXYMin, fEyeXYMax);
        BSFaceGenAnimationData::GetTrackEyeZMinMax(fEyeZMin, fEyeZMax);
        BSClamp(afHeading, fEyeXYMin, fEyeXYMax);
        BSClamp(afPitch, fEyeZMin, fEyeZMax);
        if (afHeading >= 0.0) {
            CurrentModifierData.SetQuantity(BS_FG_MODIFIER_LOOK_RIGHT, afHeading / fEyeXYMax);
            CurrentModifierData.SetQuantity(BS_FG_MODIFIER_LOOK_LEFT, 0.0);
        }
        else {
            CurrentModifierData.SetQuantity(BS_FG_MODIFIER_LOOK_RIGHT, 0.0);
            CurrentModifierData.SetQuantity(BS_FG_MODIFIER_LOOK_LEFT, afHeading / fEyeXYMin);
        }
        if (afPitch >= 0.0) {
            CurrentModifierData.SetQuantity(BS_FG_MODIFIER_LOOK_UP, afPitch / fEyeZMax);
            CurrentModifierData.SetQuantity(BS_FG_MODIFIER_LOOK_DOWN, 0.0);
        }
        else {
            CurrentModifierData.SetQuantity(BS_FG_MODIFIER_LOOK_UP, 0.0);
            CurrentModifierData.SetQuantity(BS_FG_MODIFIER_LOOK_DOWN, afPitch / fEyeZMin);
        }
        fCurrentEyeHeading = afHeading;
        fCurrentEyePitch = afPitch;
    }
}

BSFaceGenAnimationData* BSFaceGenAnimationData::CreateCopyBS() {
    BSFaceGenAnimationData* pRetVal = new BSFaceGenAnimationData;
    pRetVal->CurrentTargetExpressionData = &CurrentTargetExpressionData;
    pRetVal->CurrentAnimExpressionData = &CurrentAnimExpressionData;
    pRetVal->CurrentExpressionData = &CurrentExpressionData;
    pRetVal->CurrentSpeechModifierData = &CurrentSpeechModifierData;
    pRetVal->CurrentAnimModifierData = &CurrentAnimModifierData;
    pRetVal->CurrentModifierData = &CurrentModifierData;
    pRetVal->CurrentSpeechPhonemeData = &CurrentSpeechPhonemeData;
    pRetVal->CurrentAnimPhonemeData = &CurrentAnimPhonemeData;
    pRetVal->CurrentPhonemeData = &CurrentPhonemeData;
    pRetVal->CurrentSpeechCustomMorphData = &CurrentSpeechCustomMorphData;
    pRetVal->CurrentAnimCustomMorphData = &CurrentAnimCustomMorphData;
    pRetVal->CurrentCustomMorphData = &CurrentCustomMorphData;
    pRetVal->HeadTrackVector = HeadTrackVector;
    pRetVal->bChangedLastUpdate = bChangedLastUpdate;
    pRetVal->bFullTrackUpdate = bFullTrackUpdate;
    pRetVal->bMustUpdate = bMustUpdate;
    pRetVal->bAnimHeadCulled = bAnimHeadCulled;
    pRetVal->bAnimHeadCulledLastFrame = bAnimHeadCulledLastFrame;
    if (pTargetExpressionData) {
        pRetVal->pTargetExpressionData = new BSFaceGenKeyframeMultiple(pTargetExpressionData);
    }
    else {
        pRetVal->pTargetExpressionData = nullptr;
    }
    for (void* iter = SpeechModifierKeyframeList.m_pkHead; iter;
         iter = SpeechModifierKeyframeList.GetNextPos(static_cast<NiTListItem<BSFaceGenKeyframe*>*>(iter))) {
        BSFaceGenKeyframe* pTemp = SpeechModifierKeyframeList.Get(static_cast<NiTListItem<BSFaceGenKeyframe*>*>(iter));
        BSFaceGenKeyframeMultiple* pCurrent = nullptr;
        if (pTemp->IsMultiple()) {
            pCurrent = static_cast<BSFaceGenKeyframeMultiple*>(pTemp);
        }
        BSFaceGenKeyframe* pCopy;
        if (pCurrent) {
            pCopy = new BSFaceGenKeyframeMultiple(pCurrent);
        }
        else {
            pCopy = nullptr;
        }
        pRetVal->SpeechModifierKeyframeList.AddTail(pCopy);
    }
    for (void* iter = SpeechPhonemeKeyframeList.m_pkHead; iter;
         iter = SpeechPhonemeKeyframeList.GetNextPos(static_cast<NiTListItem<BSFaceGenKeyframe*>*>(iter))) {
        BSFaceGenKeyframe* pTemp = SpeechPhonemeKeyframeList.Get(static_cast<NiTListItem<BSFaceGenKeyframe*>*>(iter));
        BSFaceGenKeyframeMultiple* pCurrent = nullptr;
        if (pTemp->IsMultiple()) {
            pCurrent = static_cast<BSFaceGenKeyframeMultiple*>(pTemp);
        }
        BSFaceGenKeyframe* pCopy;
        if (pCurrent) {
            pCopy = new BSFaceGenKeyframeMultiple(pCurrent);
        }
        else {
            pCopy = nullptr;
        }
        pRetVal->SpeechPhonemeKeyframeList.AddTail(pCopy);
    }
    for (void* iter = SpeechCustomMorphKeyframeList.m_pkHead; iter;
         iter = SpeechCustomMorphKeyframeList.GetNextPos(static_cast<NiTListItem<BSFaceGenKeyframe*>*>(iter))) {
        BSFaceGenKeyframe* pTemp = SpeechCustomMorphKeyframeList.Get(
            static_cast<NiTListItem<BSFaceGenKeyframe*>*>(iter)
        );
        BSFaceGenKeyframeMultiple* pCurrent = nullptr;
        if (pTemp->IsMultiple()) {
            pCurrent = static_cast<BSFaceGenKeyframeMultiple*>(pTemp);
        }
        BSFaceGenKeyframe* pCopy;
        if (pCurrent) {
            pCopy = new BSFaceGenKeyframeMultiple(pCurrent);
        }
        else {
            pCopy = nullptr;
        }
        pRetVal->SpeechCustomMorphKeyframeList.AddTail(pCopy);
    }
    return pRetVal;
}

BS_FACEGEN_EXPRESSION BSFaceGenAnimationData::GetCurrentExpression() {
    BS_FACEGEN_EXPRESSION mostSignificantExpression = BS_FG_EXPRESSION_NONE;
    float fgreatestFVal = 0.0f;
    for (BS_FACEGEN_EXPRESSION eexp = BS_FG_EXPRESSION_NONE; eexp < BS_FG_EXPRESSION_COUNT;
         eexp = static_cast<BS_FACEGEN_EXPRESSION>(eexp + 1)) {
        float fval = GetExpressionValue(eexp);
        if (!(fval <= 0.0) && fval <= 1.0 && fgreatestFVal < static_cast<double>(fval)) {
            fgreatestFVal = fval;
            mostSignificantExpression = eexp;
        }
    }
    return mostSignificantExpression;
}

void BSFaceGenAnimationData::ModifyEyeHeadingAndPitchBasedOnExpression(
    BS_FACEGEN_EXPRESSION aexp,
    float& afHeading,
    float& afPitch,
    float afTime
) {
    fExpressionEyeOffsetTimer = fExpressionEyeOffsetTimer - afTime;
    if (fExpressionEyeOffsetTimer <= 0.0) {
        switch (aexp) {
            case BS_FG_EXPRESSION_NONE:
            case BS_FG_EXPRESSION_ANGER:
            case BS_FG_EXPRESSION_MOOD_COCKY:
            case BS_FG_EXPRESSION_MOOD_ANGRY: {
                if (RandomBool(0.3f)) {
                    fExpressionEyeOffsetTimer = RandomFloat(2.0f, 3.0f);
                    fExpressionEyeOffsetHeading = 0.0f;
                    fExpressionEyeOffsetPitch = 0.0f;
                }
                else {
                    fExpressionEyeOffsetTimer = RandomFloat(0.5f, 1.5f);
                    float fMaxOffset = gs_fEyePitchMaxOffsetEmotionAngry;
                    float fMinOffset = gs_fEyePitchMinOffsetEmotionAngry;
                    fExpressionEyeOffsetPitch = RandomFloat(fMinOffset, fMaxOffset);
                    fMaxOffset = gs_fEyeHeadingMaxOffsetEmotionAngry;
                    fMinOffset = gs_fEyeHeadingMinOffsetEmotionAngry;
                    fExpressionEyeOffsetHeading = RandomFloat(fMinOffset, fMaxOffset);
                }
                break;
            }
            case BS_FG_EXPRESSION_FEAR:
            case BS_FG_EXPRESSION_MOOD_AFRAID: {
                fExpressionEyeOffsetTimer = RandomFloat(0.5f, 1.5f);
                fExpressionEyeOffsetPitch = 0.0f;
                if (RandomBool(0.5f)) {
                    fExpressionEyeOffsetHeading = 0.0f;
                }
                else {
                    float fMaxOffset = gs_fEyeHeadingMaxOffsetEmotionFear;
                    float fMinOffset = gs_fEyeHeadingMinOffsetEmotionFear;
                    fExpressionEyeOffsetHeading = RandomFloat(fMinOffset, fMaxOffset);
                }
                break;
            }
            case BS_FG_EXPRESSION_HAPPY:
            case BS_FG_EXPRESSION_SURPRISE: {
                if (RandomBool(0.3f)) {
                    fExpressionEyeOffsetTimer = RandomFloat(3.0f, 4.0f);
                    fExpressionEyeOffsetHeading = 0.0f;
                    fExpressionEyeOffsetPitch = 0.0f;
                }
                else {
                    fExpressionEyeOffsetTimer = RandomFloat(0.5f, 1.5f);
                    float fMaxOffset = gs_fEyePitchMaxOffsetEmotionHappy;
                    float fMinOffset = gs_fEyePitchMinOffsetEmotionHappy;
                    fExpressionEyeOffsetPitch = RandomFloat(fMinOffset, fMaxOffset);
                    fMaxOffset = gs_fEyeHeadingMaxOffsetEmotionHappy;
                    fMinOffset = gs_fEyeHeadingMinOffsetEmotionHappy;
                    fExpressionEyeOffsetHeading = RandomFloat(fMinOffset, fMaxOffset);
                }
                break;
            }
            case BS_FG_EXPRESSION_SAD:
            case BS_FG_EXPRESSION_MOOD_DRUGGED:
            case BS_FG_EXPRESSION_MOOD_SAD: {
                fExpressionEyeOffsetTimer = RandomFloat(2.0f, 3.0f);
                if (RandomBool(0.3f)) {
                    fExpressionEyeOffsetHeading = 0.0f;
                    fExpressionEyeOffsetPitch = 0.0f;
                }
                else {
                    float fMaxOffset = gs_fEyePitchMaxOffsetEmotionSad;
                    float fMinOffset = gs_fEyePitchMinOffsetEmotionSad;
                    fExpressionEyeOffsetPitch = RandomFloat(fMinOffset, fMaxOffset);
                    fMaxOffset = gs_fEyeHeadingMaxOffsetEmotionSad;
                    fMinOffset = gs_fEyeHeadingMinOffsetEmotionSad;
                    fExpressionEyeOffsetHeading = RandomFloat(fMinOffset, fMaxOffset);
                }
                break;
            }
            case BS_FG_EXPRESSION_MOOD_NEUTRAL: {
                if (RandomBool(0.3f)) {
                    fExpressionEyeOffsetTimer = RandomFloat(3.0f, 4.0f);
                    fExpressionEyeOffsetHeading = 0.0f;
                    fExpressionEyeOffsetPitch = 0.0f;
                }
                else {
                    fExpressionEyeOffsetTimer = RandomFloat(0.5f, 1.5f);
                    float fMaxOffset = gs_fEyePitchMaxOffsetEmotionNeutral;
                    float fMinOffset = gs_fEyePitchMinOffsetEmotionNeutral;
                    fExpressionEyeOffsetPitch = RandomFloat(fMinOffset, fMaxOffset);
                    fMaxOffset = gs_fEyeHeadingMaxOffsetEmotionNeutral;
                    fMinOffset = gs_fEyeHeadingMinOffsetEmotionNeutral;
                    fExpressionEyeOffsetHeading = RandomFloat(fMinOffset, fMaxOffset);
                }
                break;
            }
            default: {
                break;
            }
        }
    }
    afHeading = afHeading + fExpressionEyeOffsetHeading;
    afPitch = afPitch + fExpressionEyeOffsetPitch;
}

void BSFaceGenAnimationData::GetTrackEyeXYMinMax(float& afMinOut, float& afMaxOut) {
    afMaxOut = static_cast<float>(BSFaceGenAnimationData::fTrackEyeXY.Float() * 0.01745329238474369);
    if (afMaxOut < 0.0) {
        afMaxOut = 0.0f;
    }
    if (afMaxOut > 1.5707963267948966) {
        afMaxOut = 1.5707963705062866f;
    }
    afMinOut = static_cast<float>(afMaxOut * (-1.0));
}

void BSFaceGenAnimationData::GetTrackEyeZMinMax(float& afMinOut, float& afMaxOut) {
    afMaxOut = static_cast<float>(BSFaceGenAnimationData::fTrackEyeZ.Float() * 0.01745329238474369);
    if (afMaxOut < 0.0) {
        afMaxOut = 0.0f;
    }
    if (afMaxOut > 1.5707963267948966) {
        afMaxOut = 1.5707963705062866f;
    }
    afMinOut = static_cast<float>(afMaxOut * (-1.0));
}

// Each sum, difference and quotient is rounded to float before it is passed.
bool BSFaceGenAnimationData::UpdateKeyframeList(
    float afTime,
    NiTPointerList<BSFaceGenKeyframe*>* apList,
    BSFaceGenKeyframe* apDest,
    bool abClearDestValueIfNoUpdate
) {
    bool bChanged = false;
    if (!apDest || !apList) {
        return false;
    }
    if (apList->GetSize() && apList->GetHead()) {
        BSFaceGenKeyframe* pFirst = apList->GetHead();
        BS_FACEGEN_MORPH_TYPE eFirstType = pFirst->GetType();
        if (eFirstType != apDest->GetType()) {
            return false;
        }
    }
    if (!apList->GetSize()) {
        apDest->SetTime(0.0f);
        return false;
    }
    float fTime = apDest->GetTime() + afTime;
    apDest->SetTime(fTime);
    BSFaceGenKeyframe* pKeyframe = apList->GetHead();
    while (pKeyframe) {
        double dDestTime = apDest->GetTime();
        // Stops on a later keyframe or a NaN time.
        if (!(pKeyframe->GetTime() <= dDestTime)) {
            break;
        }
        bChanged = static_cast<uint8_t>(bChanged) |
            static_cast<uint8_t>(apDest->MergeMorphDataFrom(pKeyframe, 1.0f, true, abClearDestValueIfNoUpdate));
        dDestTime = apDest->GetTime();
        float fRemaining = dDestTime - pKeyframe->GetTime();
        apDest->SetTime(fRemaining);
        apList->RemoveHead();
        delete pKeyframe;
        if (apList->GetSize()) {
            pKeyframe = apList->GetHead();
        }
        else {
            pKeyframe = nullptr;
        }
    }
    if (pKeyframe) {
        double dDestTime = apDest->GetTime();
        float fBlend = dDestTime / pKeyframe->GetTime();
        bChanged = static_cast<uint8_t>(bChanged) |
            static_cast<uint8_t>(apDest->MergeMorphDataFrom(pKeyframe, fBlend, true, abClearDestValueIfNoUpdate));
    }
    else {
        apDest->SetTime(0.0f);
    }
    return bChanged;
}

void BSFaceGenAnimationData::InitTag() {
    BSFaceGenAnimationData::pTag = new NiFixedString(const_cast<char*>("FAD"));
}

void BSFaceGenAnimationData::KillSDM() {
    if (BSFaceGenAnimationData::pTag) {
        delete BSFaceGenAnimationData::pTag;
    }
    BSFaceGenAnimationData::pTag = nullptr;
}

float BSFaceGenAnimationData::GetExpressionTargetChangePerSec() {
    if (!(BSFaceGenAnimationData::fExpressionChangePerSec.Float() > 0.0)) {
        return 0.0f;
    }

    return BSFaceGenAnimationData::fExpressionChangePerSec.Float();
}

float BSFaceGenAnimationData::GetExpressionStrengthAddPerSec() {
    if (!(BSFaceGenAnimationData::fExpressionStrengthAdd.Float() > 0.0)) {
        return 0.0f;
    }

    return BSFaceGenAnimationData::fExpressionStrengthAdd.Float();
}

float BSFaceGenAnimationData::GetSpeechDelay() {
    return gs_fSpeechDelay.Float();
}

void BSFaceGenAnimationData::GetTrackDeadZoneXYMinMax(float& afMinOut, float& afMaxOut) {
    float fEyeMin;
    float fEyeMax;
    afMaxOut = static_cast<float>(BSFaceGenAnimationData::fTrackDeadZoneXY.Float() * 0.01745329238474369);
    if (afMaxOut < 0.0) {
        afMaxOut = 0.0f;
    }
    if (afMaxOut > 1.5707963267948966) {
        afMaxOut = 1.5707963705062866f;
    }
    BSFaceGenAnimationData::GetTrackEyeXYMinMax(fEyeMin, fEyeMax);
    if (fEyeMax < static_cast<double>(afMaxOut)) {
        afMaxOut = fEyeMax;
    }
    afMinOut = static_cast<float>(afMaxOut * (-1.0));
}

void BSFaceGenAnimationData::GetTrackDeadZoneZMinMax(float& afMinOut, float& afMaxOut) {
    float fEyeMin;
    float fEyeMax;
    afMaxOut = static_cast<float>(BSFaceGenAnimationData::fTrackDeadZoneZ.Float() * 0.01745329238474369);
    if (afMaxOut < 0.0) {
        afMaxOut = 0.0f;
    }
    if (afMaxOut > 1.5707963267948966) {
        afMaxOut = 1.5707963705062866f;
    }
    BSFaceGenAnimationData::GetTrackEyeZMinMax(fEyeMin, fEyeMax);
    if (fEyeMax < static_cast<double>(afMaxOut)) {
        afMaxOut = fEyeMax;
    }
    afMinOut = static_cast<float>(afMaxOut * (-1.0));
}

const NiFixedString& BSFaceGenAnimationData::Tag() {
    return *BSFaceGenAnimationData::pTag;
}

bool BSFaceGenManager::GetUseProceduralEyeBlinking() {
    return bUseProceduralEyeBlinking;
}

// NaN in any component compares as unequal.
bool NiQuaternion::operator!=(const NiQuaternion& q) const {
    int32_t iNotEqual;
    if (m_fW != q.m_fW) {
        iNotEqual = 1;
    }
    else if (m_fX != q.m_fX) {
        iNotEqual = 1;
    }
    else if (m_fY != q.m_fY) {
        iNotEqual = 1;
    }
    else if (m_fZ != q.m_fZ) {
        iNotEqual = 1;
    }
    else {
        iNotEqual = 0;
    }
    return iNotEqual != 0;
}
