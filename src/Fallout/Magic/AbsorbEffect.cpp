#include "Fallout/Magic/AbsorbEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSMain/BSXFlags.h"
#include "BSMain/Setting.h"
#include "BSShader/BSShaderProperty.h"
#include "BSShader/BoltShader.h"
#include "BSShader/BoltShaderProperty.h"
#include "BSShader/ParticleShaderProperty.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/ValueModifierEffect.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObjectPalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiColor.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

// Whether the absorb archetype took Instantiate: stores RegisterActiveEffect's whole return byte.
static char bAbsorbEffectInitialized = EffectArchetypes::RegisterActiveEffect(
    EffectArchetypes::ABSORB,
    &AbsorbEffect::Instantiate
);

AbsorbEffect::AbsorbEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect)
  : ValueModifierEffect(apCaster, apItem, apEffect), spBoltNode(nullptr), spSource(nullptr), spTarget(nullptr),
    spBoltProperty(nullptr) {}

// spBoltProperty and spBoltNode are released by their own destructors.
AbsorbEffect::~AbsorbEffect() {
    BoltShader::Stop(spBoltNode.m_pObject, static_cast<BoltShaderProperty*>(spBoltProperty.m_pObject));
    if (spBoltNode.m_pObject) {
        NiNode* pparent = spBoltNode->m_pkParent;
        if (pparent) {
            pparent->DetachChild(spBoltNode.m_pObject);
        }
    }
}

ActiveEffect* AbsorbEffect::Copy() {
    AbsorbEffect* pcopy = new AbsorbEffect(pCaster, pSpell, pEffect);
    CopyData(pcopy);
    return pcopy;
}

void AbsorbEffect::Update(float afDelta) {
    float fdistance;
    ValueModifierEffect::Update(afDelta);
    Actor* pcaster = pCaster ? pCaster->GetCasterAsActor() : nullptr;
    Actor* ptarget = pTarget ? pTarget->GetTargetAsActor() : nullptr;
    if (!pEffect->pEffectSetting->IsFlagSet(EffectSetting::RECOVER) && fDuration > 0.0) {
        GetActorValue();
        if (ptarget) {
            ActorValue::Index evalue = GetActorValue();
            if (static_cast<double>(ptarget->GetActorValue(evalue)) <= 0.0) {
                Dispel(false);
            }
        }
        if (!bDone) {
            float fchange = -fMagnitude * afDelta;
            ModifyActorValue(pcaster, fchange, ActorValue::VALUE_NONE);
        }
    }
    if (!bDone &&
        ptarget &&
        ptarget->Get3D() &&
        !ptarget->IsDead(false) &&
        pcaster &&
        pcaster->Get3D() &&
        !pcaster->IsDead(false) &&
        (fdistance = pcaster->GetDistanceFromReference(ptarget, false, false),
         !(static_cast<double>(fdistance) > CombatFormulas::CalcWeaponReach(fMagicAbsorbDistanceReachMult.Float())))) {
        if (pcaster->pCurrentProcess) {
            DetectionState* pdetection = pcaster->pCurrentProcess->GetDetectionState(ptarget, 0);
            if (pdetection && !pdetection->bLineofSight) {
                Dispel(false);
            }
        }
    }
    else {
        Dispel(false);
    }
    if (!bDone) {
        Update3D(afDelta);
    }
}

void AbsorbEffect::FinishInitLoadGame(Actor* apActor) {
    ValueModifierEffect::FinishInitLoadGame(apActor);
    Start3D();
}

bool AbsorbEffect::CheckDeletedCaster(MagicCaster* apCaster) {
    bool bDeleted = ActiveEffect::CheckDeletedCaster(apCaster);
    if (bDeleted) {
        Dispel(true);
    }
    return bDeleted;
}

void AbsorbEffect::CopyData(ActiveEffect* apDestination) {
    ValueModifierEffect::CopyData(apDestination);
}

void AbsorbEffect::Start() {
    ValueModifierEffect::Start();
    if (pEffect->pEffectSetting->IsFlagSet(EffectSetting::RECOVER) || fDuration == 0.0) {
        Actor* pActor = pCaster ? pCaster->GetCasterAsActor() : nullptr;
        ModifyActorValue(pActor, -fMagnitude, ActorValue::VALUE_NONE);
    }
    ShowAttributeReductionMessage();
    Start3D();
}

void AbsorbEffect::Finish() {
    ValueModifierEffect::Finish();
    if (pEffect->pEffectSetting->IsFlagSet(EffectSetting::RECOVER)) {
        Actor* pActor = pCaster ? pCaster->GetCasterAsActor() : nullptr;
        float fmagnitude = CheckAdjustNoNegativeMagnitude(fMagnitude);
        ModifyActorValue(pActor, fmagnitude, ActorValue::VALUE_NONE);
    }
    BoltShader::Stop(spBoltNode.m_pObject, static_cast<BoltShaderProperty*>(spBoltProperty.m_pObject));
    if (spBoltNode.m_pObject) {
        NiNode* pparent = spBoltNode->m_pkParent;
        if (pparent) {
            pparent->DetachChild(spBoltNode.m_pObject);
        }
        spBoltNode = static_cast<NiNode*>(nullptr);
    }
}

void AbsorbEffect::Start3D() {
    spBoltNode = new NiNode(0);
    const NiFixedString* pTag = &BSXFlags::Tag();
    BSXFlags* pextra = static_cast<BSXFlags*>(spBoltNode->GetExtraData(*pTag));
    if (!pextra) {
        pextra = new BSXFlags;
        pTag = &BSXFlags::Tag();
        spBoltNode->AddExtraData(*pTag, pextra);
    }
    pextra->SetFlags(BSXFlags::ANIMATED, true);
    TESObjectREFR* ptarget = pTarget ? pTarget->GetTargetStatsObject() : nullptr;
    TESObjectREFR* pcaster = pCaster ? pCaster->GetCasterStatsObject() : nullptr;
    if (ptarget && pcaster) {
        NiNode* pparentNode = static_cast<NiNode*>(ptarget->GetParentCell()->GetDynamicNode());
        pparentNode->AttachChild(spBoltNode.m_pObject, 1);
        spBoltProperty = static_cast<BSShaderProperty*>(nullptr);
        spBoltProperty = static_cast<BSShaderProperty*>(BoltShader::Start(spBoltNode.m_pObject, true));
        Animation* panim = pcaster->GetAnimation();
        if (panim) {
            NiAVObjectPalette* pPalette = panim->GetControllerManager()->GetObjectPalette();
            spSource = static_cast<NiNode*>(pPalette->GetAVObject("Bip01 Spine1"));
            if (spSource == static_cast<NiNode*>(nullptr)) {
                spSource = ptarget->Get3D();
            }
        }
        panim = ptarget->GetAnimation();
        if (panim) {
            NiAVObjectPalette* pPalette = panim->GetControllerManager()->GetObjectPalette();
            spTarget = static_cast<NiNode*>(pPalette->GetAVObject("Bip01 Spine1"));
            if (spTarget == static_cast<NiNode*>(nullptr)) {
                spTarget = pcaster->Get3D();
            }
        }
        if (spBoltProperty.m_pObject) {
            // BoltShader::Start returns a BoltShaderProperty; spBoltProperty holds it as its base.
            BoltShaderProperty* pbolt = static_cast<BoltShaderProperty*>(spBoltProperty.m_pObject);
            pbolt->SetBoltsRadius(fAbsorbBoltsRadius.Float());
            static_cast<BoltShaderProperty*>(spBoltProperty.m_pObject)
                ->SetBoltsRadiusStrength(fAbsorbBoltsRadiusStrength.Float());
            static_cast<BoltShaderProperty*>(spBoltProperty.m_pObject)
                ->SetAbsorbSegmentLength(fAbsorbSegmentLength.Float());
            static_cast<BoltShaderProperty*>(spBoltProperty.m_pObject)
                ->SetSegmentVariance(fAbsorbSegmentVariance.Float());
            static_cast<BoltShaderProperty*>(spBoltProperty.m_pObject)
                ->SetAbsorbTortuosityVariance(fAbsorbTortuosityVariance.Float());
            static_cast<BoltShaderProperty*>(spBoltProperty.m_pObject)
                ->SetBoltSmallWidth(fAbsorbBoltSmallWidth.Float());
            static_cast<BoltShaderProperty*>(spBoltProperty.m_pObject)->SetBoltGrowWidth(fAbsorbBoltGrowWidth.Float());
            static_cast<BoltShaderProperty*>(spBoltProperty.m_pObject)->SetAbsorbNumBolts(iAbsorbNumBolts.Int());
            // Each colour is a NiColorA temporary built from the blue, green and red settings read in that order,
            // then passed by value.
            float fCoreB = fAbsorbCoreColorB.Float();
            float fCoreG = fAbsorbCoreColorG.Float();
            NiColorA kCoreColor(fAbsorbCoreColorR.Float(), fCoreG, fCoreB, 1.0f);
            static_cast<BoltShaderProperty*>(spBoltProperty.m_pObject)
                ->SetAbsorbCoreColor(kCoreColor.r, kCoreColor.g, kCoreColor.b, kCoreColor.a);
            float fGlowB = fAbsorbGlowColorB.Float();
            float fGlowG = fAbsorbGlowColorG.Float();
            NiColorA kGlowColor(fAbsorbGlowColorR.Float(), fGlowG, fGlowB, 1.0f);
            static_cast<BoltShaderProperty*>(spBoltProperty.m_pObject)
                ->SetAbsorbGlowColor(kGlowColor.r, kGlowColor.g, kGlowColor.b, kGlowColor.a);
            static_cast<BoltShaderProperty*>(spBoltProperty.m_pObject)->SetMoving(false);
            static_cast<BoltShaderProperty*>(spBoltProperty.m_pObject)->SetStartFromPoint(true);
            static_cast<BoltShaderProperty*>(spBoltProperty.m_pObject)->InitSegmentData();
        }
    }
}

void AbsorbEffect::Update3D(float afDelta) {
    static char bskip;
    if (spSource.m_pObject && spTarget.m_pObject && spBoltNode.m_pObject && spBoltProperty.m_pObject && !bskip) {
        NiPoint3 toTarget;
        NiMatrix3 rot;
        NiPoint3 sourceLoc = NiPoint3::ZERO;
        sourceLoc = spSource->GetWorldTranslate();
        spBoltNode->SetTranslate(sourceLoc);
        toTarget = spTarget->GetWorldTranslate() - sourceLoc;
        float fz = toTarget.z;
        toTarget.z = 0.0f;
        float fxy = toTarget.Length();
        float fYAngle = -NiATan2(fz, fxy);
        float fZAngle = NiATan2(toTarget.x, toTarget.y);
        rot.FromEulerAnglesZYX(fZAngle, 0.0f, fYAngle);
        spBoltNode->SetRotate(rot);
        float fmoveSpeed = fAbsorbMoveSpeed.Float();
        float fdistance = toTarget.Length();
        static_cast<BoltShaderProperty*>(spBoltProperty.m_pObject)
            ->BoltShaderProperty::UpdateAndMoveBolt(afDelta, fdistance, fmoveSpeed, false);
    }
}

ActiveEffect* AbsorbEffect::Instantiate(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect) {
    return new AbsorbEffect(apCaster, apItem, apEffect);
}
