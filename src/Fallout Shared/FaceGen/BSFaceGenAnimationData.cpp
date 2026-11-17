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
#include "Reconstruction.hpp"

#include <stdint.h>

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
    CurrentCustomMorphData(BS_FG_MORPH_CUSTOM, 1u), HeadTrackVector(NiPoint3_ZERO), fCurrentTorsoHeading(0.0f),
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
    if (((int32_t)aeExpression) >= 15) {
        return (float)(0.0);
    }
    else {
        return (float)((float)(&CurrentExpressionData)->GetQuantity(aeExpression));
    }
}

void BSFaceGenAnimationData::SetExpressionValue(BS_FACEGEN_EXPRESSION aeExpression, float afValue) {
    if (((int32_t)aeExpression) < 15) {
        (&CurrentTargetExpressionData)->SetQuantityNoTest(aeExpression, afValue);
    }
}

bool BSFaceGenAnimationData::GetModifierValuesAsRotation(NiQuaternion& arQuat) {
    double v2;
    double v3;
    double v4;
    double v5;
    double v6;
    double v7;
    float quantity = (float)(&CurrentModifierData)->GetQuantity(BS_FG_MODIFIER_HEAD_YAW);
    float v9 = (float)(&CurrentModifierData)->GetQuantity(BS_FG_MODIFIER_HEAD_ROLL);
    float v11 = (float)(&CurrentModifierData)->GetQuantity(BS_FG_MODIFIER_HEAD_PITCH);
    if (quantity == 0x1.fffffe0000000p+127) {
        quantity = (float)0.0;
    }
    if (!(quantity > (-1.5707963267948966))) {
        v6 = (-1.5707963267948966);
    }
    else {
        if (!(quantity < 1.5707963267948966)) {
            v7 = 1.5707963267948966;
        }
        else {
            v7 = (double)quantity;
        }
        v6 = v7;
    }
    if (v9 == 0x1.fffffe0000000p+127) {
        v9 = (float)0.0;
    }
    if (!(v9 > (-1.5707963267948966))) {
        v4 = (-1.5707963267948966);
    }
    else {
        if (!(v9 < 1.5707963267948966)) {
            v5 = 1.5707963267948966;
        }
        else {
            v5 = (double)v9;
        }
        v4 = v5;
    }
    if (v11 == 0x1.fffffe0000000p+127) {
        v11 = (float)0.0;
    }
    if (!(v11 > (-1.5707963267948966))) {
        v2 = (-1.5707963267948966);
    }
    else {
        if (!(v11 < 1.5707963267948966)) {
            v3 = 1.5707963267948966;
        }
        else {
            v3 = (double)v11;
        }
        v2 = v3;
    }
    float fPitch = (float)v2;
    float fRoll = (float)v4;
    float fYaw = (float)v6;
    (&arQuat)->FromEulerAnglesXYZ(fYaw, fRoll, fPitch);
    return (&arQuat)->operator!=(NiQuaternion_IDENTITY);
}

void BSFaceGenAnimationData::SetModifierValue(BS_FACEGEN_MODIFIER aeModifier, float afValue) {
    if (((int32_t)aeModifier) < 17) {
        (&CurrentSpeechModifierData)->SetQuantityNoTest(aeModifier, afValue);
    }
}

float BSFaceGenAnimationData::GetPhonemeValue(BS_FACEGEN_PHONEME aePhoneme) {
    if (((int32_t)aePhoneme) >= 16) {
        return (float)(0.0);
    }
    else {
        return (float)((float)(&CurrentPhonemeData)->GetQuantity(aePhoneme));
    }
}

void BSFaceGenAnimationData::SetPhonemeValue(BS_FACEGEN_PHONEME aePhoneme, float afValue) {
    if (((int32_t)aePhoneme) < 16) {
        (&CurrentSpeechPhonemeData)->SetQuantityNoTest(aePhoneme, afValue);
    }
}

float BSFaceGenAnimationData::GetCustomMorphValue(BS_FACEGEN_CUSTOM_MORPH aeCustomMorph) {
    if (((int32_t)aeCustomMorph) >= 1) {
        return (float)(0.0);
    }
    else {
        return (float)((float)(&CurrentCustomMorphData)->GetQuantity(aeCustomMorph));
    }
}

void BSFaceGenAnimationData::SetCustomMorphValue(BS_FACEGEN_CUSTOM_MORPH aeCustomMorph, float afValue) {
    if (((int32_t)aeCustomMorph) < 1) {
        (&CurrentSpeechCustomMorphData)->SetQuantityNoTest(aeCustomMorph, afValue);
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
    (&AnimLock)->Lock(nullptr);
    ResetSpeechCurrentMorphs(
        afTime, abResetExpression, abResetPhoneme, abResetModifier, abResetCustomMorph, abForDeath
    );
    ResetAnimCurrentMorphs(afTime, abResetExpression, abResetPhoneme, abResetModifier, abResetCustomMorph);
    if (afTime == 0.0) {
        if (abResetExpression) {
            (&CurrentExpressionData)->SetTime((float)0.0);
            (&CurrentExpressionData)->ClearData((bool)1);
        }
        if (abResetModifier) {
            (&CurrentModifierData)->SetTime((float)0.0);
            (&CurrentModifierData)->ClearData((bool)1);
            if (abForDeath) {
                (&CurrentModifierData)->SetQuantity(0, 1.0);
                (&CurrentModifierData)->SetQuantity(1u, 1.0);
            }
        }
        if (abResetPhoneme) {
            (&CurrentPhonemeData)->SetTime((float)0.0);
            (&CurrentPhonemeData)->ClearData((bool)1);
        }
        if (abResetCustomMorph) {
            (&CurrentCustomMorphData)->SetTime((float)0.0);
            (&CurrentCustomMorphData)->ClearData((bool)1);
        }
    }
    (&AnimLock)->Unlock();
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
            CurrentTargetExpressionData.ClearData(1);
        }
        else {
            SetExpressionTarget(BS_FG_EXPRESSION_NONE, 1.0f);
        }
    }
    if (abResetModifier) {
        CurrentSpeechModifierData.SetTime(0.0);
        if (!(afTime > 0.0)) {
            CurrentSpeechModifierData.ClearData(1);
            if (abForDeath) {
                SetModifierValue(BS_FG_MODIFIER_BLINK_LEFT, 1.0f);
                SetModifierValue(BS_FG_MODIFIER_BLINK_RIGHT, 1.0f);
            }
        }
        else {
            if (abForDeath) {
                float apQuantities[18];
                BSFaceGenManager::ResetQuantityArray(apQuantities, 17u, 1);
                apQuantities[0] = 1.0f;
                apQuantities[1] = 1.0f;
                AddModifiersKeyframe(apQuantities, afTime);
            }
            else {
                AddSpeechEmptyKeyframe(BS_FG_MORPH_MODIFIER, afTime, 1);
            }
        }
    }
    if (abResetPhoneme) {
        CurrentSpeechPhonemeData.SetTime(0.0);
        if (!(afTime > 0.0)) {
            CurrentSpeechPhonemeData.ClearData(1);
        }
        else {
            AddSpeechEmptyKeyframe(BS_FG_MORPH_PHONEME, afTime, 1);
        }
    }
    if (abResetCustomMorph) {
        CurrentSpeechCustomMorphData.SetTime(0.0);
        if (!(afTime > 0.0)) {
            CurrentSpeechCustomMorphData.ClearData(1);
        }
        else {
            AddSpeechEmptyKeyframe(BS_FG_MORPH_CUSTOM, afTime, 1);
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
            CurrentAnimExpressionData.ClearData(1);
        }
        else {
            AddAnimEmptyKeyframe(BS_FG_MORPH_EXPRESSION, afTime, 1);
        }
    }
    if (abResetModifier) {
        CurrentAnimModifierData.SetTime(0.0);
        if (!(afTime > 0.0)) {
            CurrentAnimModifierData.ClearData(1);
        }
        else {
            AddAnimEmptyKeyframe(BS_FG_MORPH_MODIFIER, afTime, 1);
        }
    }
    if (abResetPhoneme) {
        CurrentAnimPhonemeData.SetTime(0.0);
        if (!(afTime > 0.0)) {
            CurrentAnimPhonemeData.ClearData(1);
        }
        else {
            AddAnimEmptyKeyframe(BS_FG_MORPH_PHONEME, afTime, 1);
        }
    }
    if (abResetCustomMorph) {
        CurrentAnimCustomMorphData.SetTime(0.0);
        if (!(afTime > 0.0)) {
            CurrentAnimCustomMorphData.ClearData(1);
        }
        else {
            AddAnimEmptyKeyframe(BS_FG_MORPH_CUSTOM, afTime, 1);
        }
    }
}

void BSFaceGenAnimationData::SetHeadTrackVector(NiPoint3 aPoint, bool abInstant) {
    if ((&HeadTrackVector)->operator==(NiPoint3_ZERO) && (&aPoint)->operator!=(NiPoint3_ZERO)) {
        TrackingState = ETrackingState_JustAcquired;
        fJustAcquiredTimer = (float)0.0;
    }
    if ((&aPoint)->operator==(NiPoint3_ZERO)) {
        TrackingState = ETrackingState_None;
    }
    HeadTrackVector = aPoint;
    bShouldHeadTrack = (bool)1;
}

void BSFaceGenAnimationData::ClearHeadTracking(bool abInstant) {
    HeadTrackVector = NiPoint3_ZERO;
    bFullTrackUpdate = (bool)((uint8_t)abInstant);
}

bool BSFaceGenAnimationData::GetChangedLastUpdate() {
    return bChangedLastUpdate;
}

bool BSFaceGenAnimationData::GetMustUpdate() {
    return bMustUpdate;
}

void BSFaceGenAnimationData::SetMustUpdate(bool abMustUpdate) {
    bMustUpdate = (bool)((uint8_t)abMustUpdate);
}

bool BSFaceGenAnimationData::GetDead() {
    return (bool)((uint8_t)bDead);
}

void BSFaceGenAnimationData::SetDead(bool abDead, bool abInstant) {
    if (!abDead || bDead) {
        if (!abDead && bDead) {
            bDead = 0;
            if (abInstant) {
                ResetCurrentMorphs(0.0, 1, 1, 1, 1, 0);
            }
            else {
                ResetCurrentMorphs(1.0, 1, 1, 1, 1, 0);
            }
            bChangedLastUpdate = 1;
            bShouldHeadTrack = 1;
        }
    }
    else {
        if (abInstant) {
            ResetCurrentMorphs(0.0, 1, 1, 1, 1, 1);
        }
        else {
            ResetCurrentMorphs(1.0, 1, 1, 1, 1, 1);
        }
        bDead = abDead;
        bChangedLastUpdate = 1;
    }
}

void BSFaceGenAnimationData::SetAnimExpressionValue(BS_FACEGEN_EXPRESSION aeExpression, float afValue) {
    if (((int32_t)aeExpression) < 15) {
        (&CurrentAnimExpressionData)->SetQuantityNoTest(aeExpression, afValue);
    }
}

void BSFaceGenAnimationData::SetAnimModifierValue(BS_FACEGEN_MODIFIER aeModifier, float afValue) {
    if (((int32_t)aeModifier) < 17) {
        (&CurrentAnimModifierData)->SetQuantityNoTest(aeModifier, afValue);
    }
}

void BSFaceGenAnimationData::SetAnimPhonemeValue(BS_FACEGEN_PHONEME aePhoneme, float afValue) {
    if (((int32_t)aePhoneme) < 16) {
        (&CurrentAnimPhonemeData)->SetQuantityNoTest(aePhoneme, afValue);
    }
}

void BSFaceGenAnimationData::SetAnimCustomMorphValue(BS_FACEGEN_CUSTOM_MORPH aeCustomMorph, float afValue) {
    if (((int32_t)aeCustomMorph) < 1) {
        (&CurrentAnimCustomMorphData)->SetQuantityNoTest(aeCustomMorph, afValue);
    }
}

void BSFaceGenAnimationData::AddPhonemeKeyframe(float* apQuantities, float afTime) {
    BSFaceGenKeyframeMultiple morph(BS_FG_MORPH_PHONEME, BS_FG_PHONEME_COUNT);
    if (!bDead && (apQuantities && !(afTime < 0.0))) {
        uint32_t i;
        for (i = 0; i < BS_FG_PHONEME_COUNT; ++i) {
            if (apQuantities[i] < 0.0 || apQuantities[i] > 1.0) {
                goto LABEL_9;
            }
        }
        morph.BSFaceGenKeyframe::SetTime(afTime);
        for (i = 0; i < BS_FG_PHONEME_COUNT; ++i) {
            morph.SetQuantity(i, apQuantities[i]);
        }
        morph.BSFaceGenKeyframeMultiple::AddToList(&SpeechPhonemeKeyframeList);
    }
    else {
    LABEL_9:;
    }
}

void BSFaceGenAnimationData::AddModifiersKeyframe(float* apQuantities, float afTime) {
    BSFaceGenKeyframeMultiple morph(BS_FG_MORPH_MODIFIER, BS_FG_MODIFIER_COUNT);
    if (!bDead && (apQuantities && !(afTime < 0.0))) {
        morph.BSFaceGenKeyframe::SetTime(afTime);
        for (uint32_t i = 0; i < BS_FG_MODIFIER_COUNT; ++i) {
            morph.SetQuantity(i, apQuantities[i]);
        }
        morph.BSFaceGenKeyframeMultiple::AddToList(&SpeechModifierKeyframeList);
    }
    else {}
}

void BSFaceGenAnimationData::AddSpeechEmptyKeyframe(BS_FACEGEN_MORPH_TYPE aeType, float afTime, bool abForceZero) {
    if (!bDead) {
        if (aeType) {
            if (aeType == BS_FG_MORPH_MODIFIER) {
                BSFaceGenKeyframeMultiple v6(BS_FG_MORPH_MODIFIER, 17u);
                v6.BSFaceGenKeyframe::SetTime(afTime);
                v6.BSFaceGenKeyframeMultiple::AddToList(&SpeechModifierKeyframeList);
            }
            else {
                if (aeType == BS_FG_MORPH_CUSTOM) {
                    BSFaceGenKeyframeMultiple v5(BS_FG_MORPH_CUSTOM, 1u);
                    v5.BSFaceGenKeyframe::SetTime(afTime);
                    v5.BSFaceGenKeyframeMultiple::AddToList(&SpeechCustomMorphKeyframeList);
                }
            }
        }
        else {
            BSFaceGenKeyframeMultiple apMemory(BS_FG_MORPH_PHONEME, 16u);
            apMemory.BSFaceGenKeyframe::SetTime(afTime);
            apMemory.BSFaceGenKeyframeMultiple::AddToList(&SpeechPhonemeKeyframeList);
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
    if (abClearModifiers) {
        if (SpeechModifierKeyframeList.m_uiCount) {
            BSFaceGenKeyframe* pdelete_2;
            for (BSFaceGenKeyframe* pdelete = SpeechModifierKeyframeList.GetHead(); pdelete; pdelete = pdelete_2) {
                delete pdelete;
                SpeechModifierKeyframeList.RemoveHead();
                if (SpeechModifierKeyframeList.m_uiCount) {
                    pdelete_2 = SpeechModifierKeyframeList.GetHead();
                }
                else {
                    pdelete_2 = 0;
                }
            }
        }
    }
    if (abClearPhonemes) {
        if (SpeechPhonemeKeyframeList.m_uiCount) {
            BSFaceGenKeyframe* pdelete_4;
            for (BSFaceGenKeyframe* pdelete_3 = SpeechPhonemeKeyframeList.GetHead(); pdelete_3; pdelete_3 = pdelete_4) {
                delete pdelete_3;
                SpeechPhonemeKeyframeList.RemoveHead();
                if (SpeechPhonemeKeyframeList.m_uiCount) {
                    pdelete_4 = SpeechPhonemeKeyframeList.GetHead();
                }
                else {
                    pdelete_4 = 0;
                }
            }
        }
    }
    if (abClearCustomMorphs) {
        if (SpeechCustomMorphKeyframeList.m_uiCount) {
            BSFaceGenKeyframe* pdelete_6;
            for (BSFaceGenKeyframe* pdelete_5 = SpeechCustomMorphKeyframeList.GetHead(); pdelete_5;
                 pdelete_5 = pdelete_6) {
                delete pdelete_5;
                SpeechCustomMorphKeyframeList.RemoveHead();
                if (SpeechCustomMorphKeyframeList.m_uiCount) {
                    pdelete_6 = SpeechCustomMorphKeyframeList.GetHead();
                }
                else {
                    pdelete_6 = 0;
                }
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
    switch ((int32_t)aeType) {
        case 0: {
            return ((&SpeechPhonemeKeyframeList)->m_uiCount == 0);
        }
        case 2: {
            return ((&SpeechModifierKeyframeList)->m_uiCount == 0);
        }
        case 3: {
            return ((&SpeechCustomMorphKeyframeList)->m_uiCount == 0);
        }
    }
    return (bool)1;
}

bool BSFaceGenAnimationData::IsAnimKeyframeListEmpty(BS_FACEGEN_MORPH_TYPE aeType) {
    return true;
}

void BSFaceGenAnimationData::AddCustomMorphKeyframe(float* apQuantities, float afTime) {
    alignas(BSFaceGenKeyframeMultiple) uint8_t apThis[sizeof(BSFaceGenKeyframeMultiple)];
    ::new (&reinterpret_cast<BSFaceGenKeyframeMultiple&>(apThis)) BSFaceGenKeyframeMultiple(BS_FG_MORPH_CUSTOM, 1u);
    if (!bDead && (apQuantities && !(afTime < 0.0))) {
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
    BSFaceGenKeyframeMultiple* v2;
    BSFaceGenKeyframeMultiple* apMemory;
    if (bDead) {
        goto LABEL_13;
    }
    if (!apQuantities) {
        goto LABEL_13;
    }
    if (pTargetExpressionData) {
        delete pTargetExpressionData;
    }
    apMemory = static_cast<BSFaceGenKeyframeMultiple*>(ourMalloc(sizeof(BSFaceGenKeyframeMultiple)));
    v2 = apMemory ? ::new (apMemory) BSFaceGenKeyframeMultiple(BS_FG_MORPH_EXPRESSION, 15u) : 0;
    pTargetExpressionData = v2;
    for (uint32_t iexp = 0; iexp < 15; ++iexp) {
        pTargetExpressionData->SetQuantity(iexp, apQuantities[iexp]);
    }
    if (!CurrentTargetExpressionData.Compare(pTargetExpressionData)) {
    LABEL_13:;
        ClearExpressionTarget();
    }
}

void BSFaceGenAnimationData::SetExpressionTarget(BS_FACEGEN_EXPRESSION aeExpression, float afValue) {
    if (aeExpression >= BS_FG_EXPRESSION_NONE && aeExpression <= BS_FG_EXPRESSION_COUNT && !(afValue < 0.0) &&
        !(afValue > 1.0)) {
        if (pTargetExpressionData) {
            pTargetExpressionData->ClearData(1);
        }
        else {
            BSFaceGenKeyframeMultiple* v3;
            BSFaceGenKeyframeMultiple* apMemory =
                static_cast<BSFaceGenKeyframeMultiple*>(ourMalloc(sizeof(BSFaceGenKeyframeMultiple)));
            if (apMemory) {
                v3 = ::new (apMemory) BSFaceGenKeyframeMultiple(BS_FG_MORPH_EXPRESSION, 15u);
            }
            else {
                v3 = 0;
            }
            pTargetExpressionData = v3;
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
        pTargetExpressionData->ClearData(1);
    }
    else {
        BSFaceGenKeyframeMultiple* v3;
        BSFaceGenKeyframeMultiple* apMemory =
            static_cast<BSFaceGenKeyframeMultiple*>(ourMalloc(sizeof(BSFaceGenKeyframeMultiple)));
        if (apMemory) {
            v3 = ::new (apMemory) BSFaceGenKeyframeMultiple(BS_FG_MORPH_EXPRESSION, 15u);
        }
        else {
            v3 = 0;
        }
        pTargetExpressionData = v3;
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
    pTargetExpressionData = 0;
}

void BSFaceGenAnimationData::AddAnimationDataFromNote(const char* apNote) {}

bool BSFaceGenAnimationData::Update(float afTime) {
    AnimLock.Lock(0);
    float flookdown = CurrentModifierData.GetQuantity(BS_FG_MODIFIER_LOOK_DOWN);
    if (BSFaceGenManager::GetUseProceduralEyeBlinking() && !bDead && SpeechModifierKeyframeList.IsEmpty() &&
        gs_fBlinkDownTime.Float() > 0.0 && gs_fBlinkUpTime.Float() > 0.0 && gs_fBlinkDelayMin.Float() > 0.0) {
        float* v2 = &gs_fBlinkDelayMax.Float();
        if (gs_fBlinkDelayMin.Float() <= static_cast<double>(*v2)) {
            float* v3 = &gs_fLookDownDisableBlinkingAmt.Float();
            if (*v3 > static_cast<double>(flookdown)) {
                // ResetQuantityArray fills 17 modifier quantities and AddModifiersKeyframe reads them.
                float apQuantities[BS_FG_MODIFIER_COUNT];
                float* v4 = &gs_fBlinkDelayMax.Float();
                float v16 = *v4 - gs_fBlinkDelayMin.Float();
                BSFaceGenManager::ResetQuantityArray(apQuantities, 17u, 0);
                apQuantities[0] = 0.0f;
                apQuantities[1] = 0.0f;
                float* v5 = &gs_fBlinkDelayMin.Float();
                float v14 = RandomFloat0To1() * v16 + *v5;
                AddModifiersKeyframe(apQuantities, v14);
                BSFaceGenManager::ResetQuantityArray(apQuantities, 17u, 0);
                apQuantities[0] = 1.0f;
                apQuantities[1] = 1.0f;
                float* v6 = &gs_fBlinkDownTime.Float();
                AddModifiersKeyframe(apQuantities, *v6);
                BSFaceGenManager::ResetQuantityArray(apQuantities, 17u, 0);
                apQuantities[0] = 0.0f;
                apQuantities[1] = 0.0f;
                float* v7 = &gs_fBlinkUpTime.Float();
                AddModifiersKeyframe(apQuantities, *v7);
            }
        }
    }
    if (afTime > 0.0) {
        // UpdateToTarget, IsTargetAvailable, CopyFrom and MergeMorphDataFrom are called on the keyframes through base
        // pointers so that they dispatch through the vtable, as the game does; calls on the members themselves would be
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
        bChangedLastUpdate = 0;
        bool bchangedSpeech = pTargetExpression->UpdateToTarget(afTime, pTargetExpressionData);
        bool bchangedAnim = (!bAnimHeadCulledLastFrame && bAnimHeadCulled) || pAnimExpression->IsTargetAvailable();
        if (!bchangedAnim && !bchangedSpeech) {
            ClearExpressionTarget();
        }
        if (bchangedSpeech) {
            pExpression->CopyFrom(&CurrentTargetExpressionData);
            if (bchangedAnim) {
                pExpression->MergeMorphDataFrom(&CurrentAnimExpressionData, 1.0f, 0, 0);
            }
        }
        else {
            if (bchangedAnim) {
                pExpression->CopyFrom(&CurrentAnimExpressionData);
            }
        }
        bool v13 = bchangedSpeech || bchangedAnim;
        bchangedSpeech = BSFaceGenAnimationData::UpdateKeyframeList(
            afTime, &SpeechModifierKeyframeList, &CurrentSpeechModifierData, 0
        );
        bchangedAnim = (!bAnimHeadCulledLastFrame && bAnimHeadCulled) || pAnimModifier->IsTargetAvailable();
        if (bchangedSpeech) {
            pModifier->CopyFrom(&CurrentSpeechModifierData);
            if (bchangedAnim) {
                pModifier->MergeMorphDataFrom(&CurrentAnimModifierData, 1.0f, 0, 0);
            }
        }
        else {
            if (bchangedAnim) {
                pModifier->CopyFrom(&CurrentAnimModifierData);
            }
        }
        bool v12 = bchangedSpeech || bchangedAnim;
        bchangedSpeech = BSFaceGenAnimationData::UpdateKeyframeList(
            afTime, &SpeechPhonemeKeyframeList, &CurrentSpeechPhonemeData, 0
        );
        bchangedAnim = (!bAnimHeadCulledLastFrame && bAnimHeadCulled) || pAnimPhoneme->IsTargetAvailable();
        if (bchangedSpeech) {
            pPhoneme->CopyFrom(&CurrentSpeechPhonemeData);
            if (bchangedAnim) {
                pPhoneme->MergeMorphDataFrom(&CurrentAnimPhonemeData, 1.0f, 0, 0);
            }
        }
        else {
            if (bchangedAnim) {
                pPhoneme->CopyFrom(&CurrentAnimPhonemeData);
            }
        }
        bool v11 = bchangedSpeech || bchangedAnim;
        bchangedSpeech = BSFaceGenAnimationData::UpdateKeyframeList(
            afTime, &SpeechCustomMorphKeyframeList, &CurrentSpeechCustomMorphData, 0
        );
        bchangedAnim = (!bAnimHeadCulledLastFrame && bAnimHeadCulled) || pAnimCustomMorph->IsTargetAvailable();
        if (bchangedSpeech) {
            pCustomMorph->CopyFrom(&CurrentSpeechCustomMorphData);
            if (bchangedAnim) {
                pCustomMorph->MergeMorphDataFrom(&CurrentAnimCustomMorphData, 1.0f, 0, 0);
            }
        }
        else {
            if (bchangedAnim) {
                pCustomMorph->CopyFrom(&CurrentAnimCustomMorphData);
            }
        }
        bool v10 = bchangedSpeech || bchangedAnim;
        bool v9 = v13 || (v12 || (v11 || v10));
        bChangedLastUpdate = v9;
    }
    bAnimHeadCulledLastFrame = bAnimHeadCulled;
    AnimLock.Unlock();
    return bChangedLastUpdate;
}

float BSFaceGenAnimationData::GetModifierValue(BS_FACEGEN_MODIFIER aeModifier) {
    if (((int32_t)aeModifier) >= 17) {
        return (float)(0.0);
    }
    else {
        return (float)((float)(&CurrentModifierData)->GetQuantity(aeModifier));
    }
}

bool BSFaceGenAnimationData::UpdateEyeTracking(float afTime, NiNode* apHeadNode) {
    float feyeHeadingDelta;
    float feyePitchDelta;
    AnimLock.Lock(0);
    bool bres = !bDead;
    if (bres) {
        bres = 1;
        BS_FACEGEN_EXPRESSION mostSignificantExpression = GetCurrentExpression();
        float fexpModDesiredHeading = fDesiredEyeHeading;
        float fexpModDesiredPitch = fDesiredEyePitch;
        if (mostSignificantExpression < BS_FG_EXPRESSION_COUNT) {
            ModifyEyeHeadingAndPitchBasedOnExpression(
                mostSignificantExpression, fexpModDesiredHeading, fexpModDesiredPitch, afTime
            );
        }
        feyeHeadingDelta = fexpModDesiredHeading - fCurrentEyeHeading;
        feyePitchDelta = fexpModDesiredPitch - fCurrentEyePitch;
        float fmaxAbsEyeDeltaThisTime = BSFaceGenAnimationData::fTrackSpeed.Float() * afTime;
        BSClamp(feyeHeadingDelta, -fmaxAbsEyeDeltaThisTime, fmaxAbsEyeDeltaThisTime);
        BSClamp(feyePitchDelta, -fmaxAbsEyeDeltaThisTime, fmaxAbsEyeDeltaThisTime);
        fCurrentEyeHeading = fCurrentEyeHeading + feyeHeadingDelta;
        fCurrentEyePitch = fCurrentEyePitch + feyePitchDelta;
        SetEyePosition(fCurrentEyeHeading, fCurrentEyePitch);
    }
    AnimLock.Unlock();
    return bres;
}

bool BSFaceGenAnimationData::ShouldHeadTrack() {
    return (bool)((uint8_t)bShouldHeadTrack);
}

void BSFaceGenAnimationData::SetShouldHeadTrack(bool abShouldHeadTrack) {
    bShouldHeadTrack = (bool)((uint8_t)abShouldHeadTrack);
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
    float v3;
    float v4;
    float v5;
    float v6;
    float fEyeZMax;
    float fEyeXYMin;
    float fEyeXYMax;
    float fEyeZMin;
    if (BSFaceGenManager::GetUseProceduralEyeBlinking()) {
        BSFaceGenAnimationData::GetTrackEyeXYMinMax(fEyeXYMin, fEyeXYMax);
        BSFaceGenAnimationData::GetTrackEyeZMinMax(fEyeZMin, fEyeZMax);
        BSClamp(afHeading, fEyeXYMin, fEyeXYMax);
        BSClamp(afPitch, fEyeZMin, fEyeZMax);
        if (!(afHeading >= 0.0)) {
            (&CurrentModifierData)->SetQuantity(BS_FG_MODIFIER_LOOK_RIGHT, 0.0);
            v5 = afHeading / fEyeXYMin;
            (&CurrentModifierData)->SetQuantity(BS_FG_MODIFIER_LOOK_LEFT, v5);
        }
        else {
            v6 = afHeading / fEyeXYMax;
            (&CurrentModifierData)->SetQuantity(BS_FG_MODIFIER_LOOK_RIGHT, v6);
            (&CurrentModifierData)->SetQuantity(BS_FG_MODIFIER_LOOK_LEFT, 0.0);
        }
        if (!(afPitch >= 0.0)) {
            (&CurrentModifierData)->SetQuantity(BS_FG_MODIFIER_LOOK_UP, 0.0);
            v3 = afPitch / fEyeZMin;
            (&CurrentModifierData)->SetQuantity(BS_FG_MODIFIER_LOOK_DOWN, v3);
        }
        else {
            v4 = afPitch / fEyeZMax;
            (&CurrentModifierData)->SetQuantity(BS_FG_MODIFIER_LOOK_UP, v4);
            (&CurrentModifierData)->SetQuantity(BS_FG_MODIFIER_LOOK_DOWN, 0.0);
        }
        fCurrentEyeHeading = afHeading;
        fCurrentEyePitch = afPitch;
    }
}

BSFaceGenAnimationData* BSFaceGenAnimationData::CreateCopyBS() {
    BSFaceGenAnimationData* pretval;
    BSFaceGenKeyframe* v31;
    void* iter;
    void* apMemory = NiMemObject::operator new(sizeof(BSFaceGenAnimationData));
    if (apMemory) {
        pretval = ::new (apMemory) BSFaceGenAnimationData;
    }
    else {
        pretval = 0;
    }
    pretval->CurrentTargetExpressionData.operator=(&CurrentTargetExpressionData);
    pretval->CurrentAnimExpressionData.operator=(&CurrentAnimExpressionData);
    pretval->CurrentExpressionData.operator=(&CurrentExpressionData);
    pretval->CurrentSpeechModifierData.operator=(&CurrentSpeechModifierData);
    pretval->CurrentAnimModifierData.operator=(&CurrentAnimModifierData);
    pretval->CurrentModifierData.operator=(&CurrentModifierData);
    pretval->CurrentSpeechPhonemeData.operator=(&CurrentSpeechPhonemeData);
    pretval->CurrentAnimPhonemeData.operator=(&CurrentAnimPhonemeData);
    pretval->CurrentPhonemeData.operator=(&CurrentPhonemeData);
    pretval->CurrentSpeechCustomMorphData.operator=(&CurrentSpeechCustomMorphData);
    pretval->CurrentAnimCustomMorphData.operator=(&CurrentAnimCustomMorphData);
    pretval->CurrentCustomMorphData.operator=(&CurrentCustomMorphData);
    pretval->HeadTrackVector.x = HeadTrackVector.x;
    pretval->HeadTrackVector.y = HeadTrackVector.y;
    pretval->HeadTrackVector.z = HeadTrackVector.z;
    pretval->bChangedLastUpdate = bChangedLastUpdate;
    pretval->bFullTrackUpdate = bFullTrackUpdate;
    pretval->bMustUpdate = bMustUpdate;
    pretval->bAnimHeadCulled = bAnimHeadCulled;
    pretval->bAnimHeadCulledLastFrame = bAnimHeadCulledLastFrame;
    if (pTargetExpressionData) {
        BSFaceGenKeyframeMultiple* v8;
        BSFaceGenKeyframeMultiple* v20 =
            static_cast<BSFaceGenKeyframeMultiple*>(ourMalloc(sizeof(BSFaceGenKeyframeMultiple)));
        if (v20) {
            v8 = ::new (v20) BSFaceGenKeyframeMultiple(pTargetExpressionData);
        }
        else {
            v8 = 0;
        }
        pretval->pTargetExpressionData = v8;
    }
    else {
        pretval->pTargetExpressionData = 0;
    }
    for (iter = SpeechModifierKeyframeList.m_pkHead; iter;
         iter = SpeechModifierKeyframeList.GetNextPos(static_cast<NiTListItem<BSFaceGenKeyframe*>*>(iter))) {
        BSFaceGenKeyframe* ptemp = SpeechModifierKeyframeList.Get(static_cast<NiTListItem<BSFaceGenKeyframe*>*>(iter));
        bool v1 = ptemp->IsMultiple();
        BSFaceGenKeyframeMultiple* pcurrent = v1 ? static_cast<BSFaceGenKeyframeMultiple*>(ptemp) : 0;
        if (pcurrent) {
            BSFaceGenKeyframeMultiple* pcopy;
            BSFaceGenKeyframeMultiple* v18 =
                static_cast<BSFaceGenKeyframeMultiple*>(ourMalloc(sizeof(BSFaceGenKeyframeMultiple)));
            if (v18) {
                pcopy = ::new (v18) BSFaceGenKeyframeMultiple(pcurrent);
            }
            else {
                pcopy = 0;
            }
            BSFaceGenKeyframeMultiple* v19 = pcopy;
            v31 = pcopy;
        }
        else {
            v31 = 0;
        }
        BSFaceGenKeyframe* arg0 = v31;
        pretval->SpeechModifierKeyframeList.AddTail(arg0);
    }
    for (iter = SpeechPhonemeKeyframeList.m_pkHead; iter;
         iter = SpeechPhonemeKeyframeList.GetNextPos(static_cast<NiTListItem<BSFaceGenKeyframe*>*>(iter))) {
        BSFaceGenKeyframe* ptemp_2 = SpeechPhonemeKeyframeList.Get(static_cast<NiTListItem<BSFaceGenKeyframe*>*>(iter));
        bool v2 = ptemp_2->IsMultiple();
        BSFaceGenKeyframeMultiple* pcurrent_2 = v2 ? static_cast<BSFaceGenKeyframeMultiple*>(ptemp_2) : 0;
        if (pcurrent_2) {
            BSFaceGenKeyframeMultiple* pcopy_2;
            BSFaceGenKeyframeMultiple* v15 =
                static_cast<BSFaceGenKeyframeMultiple*>(ourMalloc(sizeof(BSFaceGenKeyframeMultiple)));
            if (v15) {
                pcopy_2 = ::new (v15) BSFaceGenKeyframeMultiple(pcurrent_2);
            }
            else {
                pcopy_2 = 0;
            }
            BSFaceGenKeyframeMultiple* v16 = pcopy_2;
            v31 = pcopy_2;
        }
        else {
            v31 = 0;
        }
        BSFaceGenKeyframe* element = v31;
        pretval->SpeechPhonemeKeyframeList.AddTail(element);
    }
    for (iter = SpeechCustomMorphKeyframeList.m_pkHead; iter;
         iter = SpeechCustomMorphKeyframeList.GetNextPos(static_cast<NiTListItem<BSFaceGenKeyframe*>*>(iter))) {
        BSFaceGenKeyframe* ptemp_3 =
            SpeechCustomMorphKeyframeList.Get(static_cast<NiTListItem<BSFaceGenKeyframe*>*>(iter));
        bool v3 = ptemp_3->IsMultiple();
        BSFaceGenKeyframeMultiple* pcurrent_3 = v3 ? static_cast<BSFaceGenKeyframeMultiple*>(ptemp_3) : 0;
        if (pcurrent_3) {
            BSFaceGenKeyframeMultiple* pcopy_3;
            BSFaceGenKeyframeMultiple* v12 =
                static_cast<BSFaceGenKeyframeMultiple*>(ourMalloc(sizeof(BSFaceGenKeyframeMultiple)));
            if (v12) {
                pcopy_3 = ::new (v12) BSFaceGenKeyframeMultiple(pcurrent_3);
            }
            else {
                pcopy_3 = 0;
            }
            BSFaceGenKeyframeMultiple* v13 = pcopy_3;
            v31 = pcopy_3;
        }
        else {
            v31 = 0;
        }
        BSFaceGenKeyframe* v11 = v31;
        pretval->SpeechCustomMorphKeyframeList.AddTail(v11);
    }
    return pretval;
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
                    float a2 = gs_fEyePitchMaxOffsetEmotionAngry.operator float();
                    float a1 = gs_fEyePitchMinOffsetEmotionAngry.operator float();
                    fExpressionEyeOffsetPitch = RandomFloat(a1, a2);
                    float a2a = gs_fEyeHeadingMaxOffsetEmotionAngry.operator float();
                    float a1a = gs_fEyeHeadingMinOffsetEmotionAngry.operator float();
                    fExpressionEyeOffsetHeading = RandomFloat(a1a, a2a);
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
                    float a2f = gs_fEyeHeadingMaxOffsetEmotionFear.operator float();
                    float a1f = gs_fEyeHeadingMinOffsetEmotionFear.operator float();
                    fExpressionEyeOffsetHeading = RandomFloat(a1f, a2f);
                }
                break;
            }
            case BS_FG_EXPRESSION_HAPPY:
            case BS_FG_EXPRESSION_SURPRISE: {
                if (RandomBool(0.3f)) {
                    goto LABEL_15;
                }
                fExpressionEyeOffsetTimer = RandomFloat(0.5f, 1.5f);
                float a2b = gs_fEyePitchMaxOffsetEmotionHappy.operator float();
                float a1b = gs_fEyePitchMinOffsetEmotionHappy.operator float();
                fExpressionEyeOffsetPitch = RandomFloat(a1b, a2b);
                float a2c = gs_fEyeHeadingMaxOffsetEmotionHappy.operator float();
                float a1c = gs_fEyeHeadingMinOffsetEmotionHappy.operator float();
                fExpressionEyeOffsetHeading = RandomFloat(a1c, a2c);
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
                    float a2d = gs_fEyePitchMaxOffsetEmotionSad.operator float();
                    float a1d = gs_fEyePitchMinOffsetEmotionSad.operator float();
                    fExpressionEyeOffsetPitch = RandomFloat(a1d, a2d);
                    float a2e = gs_fEyeHeadingMaxOffsetEmotionSad.operator float();
                    float a1e = gs_fEyeHeadingMinOffsetEmotionSad.operator float();
                    fExpressionEyeOffsetHeading = RandomFloat(a1e, a2e);
                }
                break;
            }
            case BS_FG_EXPRESSION_MOOD_NEUTRAL: {
                if (RandomBool(0.3f)) {
                LABEL_15:;
                    fExpressionEyeOffsetTimer = RandomFloat(3.0f, 4.0f);
                    fExpressionEyeOffsetHeading = 0.0f;
                    fExpressionEyeOffsetPitch = 0.0f;
                }
                else {
                    fExpressionEyeOffsetTimer = RandomFloat(0.5f, 1.5f);
                    float a2g = gs_fEyePitchMaxOffsetEmotionNeutral.operator float();
                    float a1g = gs_fEyePitchMinOffsetEmotionNeutral.operator float();
                    fExpressionEyeOffsetPitch = RandomFloat(a1g, a2g);
                    float a2h = gs_fEyeHeadingMaxOffsetEmotionNeutral.operator float();
                    float a1h = gs_fEyeHeadingMinOffsetEmotionNeutral.operator float();
                    fExpressionEyeOffsetHeading = RandomFloat(a1h, a2h);
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
    afMaxOut = (float)(((&BSFaceGenAnimationData::fTrackEyeXY)->Float()) * 0.01745329238474369);
    if ((afMaxOut) < 0.0) {
        afMaxOut = (float)0.0;
    }
    if ((afMaxOut) > 1.5707963267948966) {
        afMaxOut = 1.5707963705062866f;
    }
    afMinOut = (float)((afMaxOut) * (-1.0));
}

void BSFaceGenAnimationData::GetTrackEyeZMinMax(float& afMinOut, float& afMaxOut) {
    afMaxOut = (float)(((&BSFaceGenAnimationData::fTrackEyeZ)->Float()) * 0.01745329238474369);
    if ((afMaxOut) < 0.0) {
        afMaxOut = (float)0.0;
    }
    if ((afMaxOut) > 1.5707963267948966) {
        afMaxOut = 1.5707963705062866f;
    }
    afMinOut = (float)((afMaxOut) * (-1.0));
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
        double dRemaining = apDest->GetTime();
        float fRemaining = dRemaining - pKeyframe->GetTime();
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
    NiFixedString* v0;
    void* v1 = ourMalloc(sizeof(NiFixedString));
    if (v1) {
        v0 = ::new (static_cast<void*>(v1)) NiFixedString(const_cast<char*>("FAD"));
    }
    else {
        v0 = nullptr;
    }
    BSFaceGenAnimationData::pTag = v0;
}

void BSFaceGenAnimationData::KillSDM() {
    if (BSFaceGenAnimationData::pTag) {
        delete BSFaceGenAnimationData::pTag;
    }
    BSFaceGenAnimationData::pTag = 0;
}

float BSFaceGenAnimationData::GetExpressionTargetChangePerSec() {
    if (!(((&BSFaceGenAnimationData::fExpressionChangePerSec)->Float()) > 0.0)) {
        return (float)0.0;
    }
    else {
        return (&BSFaceGenAnimationData::fExpressionChangePerSec)->Float();
    }
}

float BSFaceGenAnimationData::GetExpressionStrengthAddPerSec() {
    if (!(((&BSFaceGenAnimationData::fExpressionStrengthAdd)->Float()) > 0.0)) {
        return (float)0.0;
    }
    else {
        return (&BSFaceGenAnimationData::fExpressionStrengthAdd)->Float();
    }
}

float BSFaceGenAnimationData::GetSpeechDelay() {
    return (&gs_fSpeechDelay)->Float();
}

void BSFaceGenAnimationData::GetTrackDeadZoneXYMinMax(float& afMinOut, float& afMaxOut) {
    float a1;
    float a2;
    afMaxOut = (float)(((&BSFaceGenAnimationData::fTrackDeadZoneXY)->Float()) * 0.01745329238474369);
    if (afMaxOut < 0.0) {
        afMaxOut = (float)0.0;
    }
    if (afMaxOut > 1.5707963267948966) {
        afMaxOut = 1.5707963705062866f;
    }
    BSFaceGenAnimationData::GetTrackEyeXYMinMax(a1, a2);
    if (a2 < ((double)afMaxOut)) {
        afMaxOut = a2;
    }
    afMinOut = (float)(afMaxOut * (-1.0));
}

void BSFaceGenAnimationData::GetTrackDeadZoneZMinMax(float& afMinOut, float& afMaxOut) {
    float a1;
    float a2;
    afMaxOut = (float)(((&BSFaceGenAnimationData::fTrackDeadZoneZ)->Float()) * 0.01745329238474369);
    if (afMaxOut < 0.0) {
        afMaxOut = (float)0.0;
    }
    if (afMaxOut > 1.5707963267948966) {
        afMaxOut = 1.5707963705062866f;
    }
    BSFaceGenAnimationData::GetTrackEyeZMinMax(a1, a2);
    if (a2 < ((double)afMaxOut)) {
        afMaxOut = a2;
    }
    afMinOut = (float)(afMaxOut * (-1.0));
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
