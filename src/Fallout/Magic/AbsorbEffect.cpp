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
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"

// Whether the absorb archetype took Instantiate: stores RegisterActiveEffect's whole return byte.
static char bAbsorbEffectInitialized =
    EffectArchetypes::RegisterActiveEffect(EffectArchetypes::ABSORB, &AbsorbEffect::Instantiate);

AbsorbEffect::AbsorbEffect(MagicCaster* apCaster, MagicItem* apItem, EffectItem* apEffect)
  : ValueModifierEffect(apCaster, apItem, apEffect), spBoltNode(nullptr), spSource(nullptr), spTarget(nullptr),
    spBoltProperty(nullptr) {}

// spBoltProperty and spBoltNode are released by their own destructors.
AbsorbEffect::~AbsorbEffect() {
    BoltShader::Stop(spBoltNode.m_pObject, static_cast<BoltShaderProperty*>(spBoltProperty.m_pObject));
    if (spBoltNode.m_pObject) {
        NiNode* Parent = spBoltNode.m_pObject->m_pkParent;
        if (Parent) {
            Parent->DetachChild(spBoltNode.m_pObject);
        }
    }
}

ActiveEffect* AbsorbEffect::Copy() {
    AbsorbEffect* v2;
    void* v4 = ourMalloc(sizeof(AbsorbEffect));
    if (v4) {
        v2 = ::new (v4) AbsorbEffect(pCaster, pSpell, pEffect);
    }
    else {
        v2 = nullptr;
    }
    CopyData(v2);
    return v2;
}

void AbsorbEffect::Update(float a1) {
    float* v3;
    Actor* ptarget;
    Actor* pcaster;
    float fdistance;
    ValueModifierEffect::Update(a1);
    if (pCaster) {
        pcaster = pCaster->GetCasterAsActor();
    }
    else {
        pcaster = nullptr;
    }
    if (pTarget) {
        ptarget = pTarget->GetTargetAsActor();
    }
    else {
        ptarget = nullptr;
    }
    if (!pEffect->pEffectSetting->IsFlagSet(EffectSetting::RECOVER) && fDuration > 0.0) {
        GetActorValue();
        if (ptarget) {
            ActorValue::Index evalue = GetActorValue();
            if (static_cast<double>(ptarget->GetActorValue(evalue)) <= 0.0) {
                Dispel(false);
            }
        }
        if (!bDone) {
            float v5 = -fMagnitude * a1;
            ModifyActorValue(pcaster, v5, ActorValue::VALUE_NONE);
        }
    }
    if (!bDone &&
        (ptarget &&
         (ptarget->Get3D() &&
          (!ptarget->IsDead(false) &&
           (pcaster &&
            (pcaster->Get3D() &&
             (!pcaster->IsDead(false) &&
              (fdistance = pcaster->GetDistanceFromReference(ptarget, false, false),
               (v3 = &fMagicAbsorbDistanceReachMult.Float(),
                !(static_cast<double>(fdistance) > CombatFormulas::CalcWeaponReach(*v3))))))))))) {
        if (pcaster->pCurrentProcess) {
            BaseProcess* CurrentAIProcess = pcaster->pCurrentProcess;
            DetectionState* v9 = CurrentAIProcess->GetDetectionState(ptarget, 0);
            if (v9) {
                if (!v9->bLineofSight) {
                    Dispel(false);
                }
            }
        }
    }
    else {
        Dispel(false);
    }
    if (!bDone) {
        Update3D(a1);
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
        Actor* pActor;
        if (pCaster) {
            pActor = pCaster->GetCasterAsActor();
        }
        else {
            pActor = nullptr;
        }
        float v1 = -fMagnitude;
        ModifyActorValue(pActor, v1, ActorValue::VALUE_NONE);
    }
    ShowAttributeReductionMessage();
    Start3D();
}

void AbsorbEffect::Finish() {
    ValueModifierEffect::Finish();
    if (pEffect->pEffectSetting->IsFlagSet(EffectSetting::RECOVER)) {
        Actor* pActor;
        if (pCaster) {
            pActor = pCaster->GetCasterAsActor();
        }
        else {
            pActor = nullptr;
        }
        float afMagnitude = fMagnitude;
        float v5 = CheckAdjustNoNegativeMagnitude(afMagnitude);
        ModifyActorValue(pActor, v5, ActorValue::VALUE_NONE);
    }
    BoltShader::Stop(spBoltNode.m_pObject, static_cast<BoltShaderProperty*>(spBoltProperty.m_pObject));
    if (spBoltNode.m_pObject) {
        NiNode* pparent = spBoltNode.m_pObject->m_pkParent;
        if (pparent) {
            pparent->DetachChild(spBoltNode.m_pObject);
        }
        spBoltNode.operator=(static_cast<NiNode*>(nullptr));
    }
}

void AbsorbEffect::Start3D() {
    TESObjectREFR* pcaster;
    TESObjectREFR* ptarget;
    NiNode* v29;
    FNVD_INLINED_LOCAL(NiFixedString, v33, std::nullopt, std::nullopt);
    FNVD_INLINED_LOCAL(NiFixedString, apString, std::nullopt, std::nullopt);
    void* apMemory = NiMemObject::operator new(sizeof(NiNode));
    if (apMemory) {
        v29 = ::new (apMemory) NiNode(0);
    }
    else {
        v29 = nullptr;
    }
    spBoltNode.operator=(v29);
    const NiFixedString* v1 = &BSXFlags::Tag();
    BSXFlags* pextra = static_cast<BSXFlags*>(spBoltNode.m_pObject->GetExtraData(*v1));
    if (!pextra) {
        BSXFlags* pextra_2;
        BSXFlags* v35 = static_cast<BSXFlags*>(NiMemObject::operator new(sizeof(BSXFlags)));
        if (v35) {
            pextra_2 = ::new (v35) BSXFlags;
        }
        else {
            pextra_2 = nullptr;
        }
        pextra = pextra_2;
        const NiFixedString* v2 = &BSXFlags::Tag();
        spBoltNode.m_pObject->AddExtraData(*v2, pextra_2);
    }
    pextra->SetFlags(1u, true);
    if (this->pTarget) {
        MagicTarget* pTarget = this->pTarget;
        ptarget = pTarget->GetTargetStatsObject();
    }
    else {
        ptarget = nullptr;
    }
    if (this->pCaster) {
        MagicCaster* pCaster = this->pCaster;
        pcaster = pCaster->GetCasterStatsObject();
    }
    else {
        pcaster = nullptr;
    }
    if (ptarget) {
        if (pcaster) {
            NiNode* pparentNode = static_cast<NiNode*>(ptarget->GetParentCell()->GetDynamicNode());
            pparentNode->AttachChild(spBoltNode.m_pObject, 1);
            spBoltProperty.operator=(static_cast<BSShaderProperty*>(nullptr));
            spBoltProperty.operator=(static_cast<BSShaderProperty*>(BoltShader::Start(spBoltNode.m_pObject, true)));
            Animation* panim = pcaster->GetAnimation();
            if (panim) {
                NiControllerManager* ControllerManager = panim->GetControllerManager();
                NiAVObjectPalette* ObjectPalette = ControllerManager->GetObjectPalette();
                NiFixedString* v22 = ::new (static_cast<void*>(&apString)) NiFixedString("Bip01 Spine1");
                spSource.operator=(static_cast<NiNode*>(ObjectPalette->GetAVObject(*v22)));
                apString.~NiFixedString();
                if (spSource.operator==(static_cast<NiNode*>(nullptr))) {
                    spSource.operator=(ptarget->Get3D());
                }
            }
            Animation* v40 = ptarget->GetAnimation();
            if (v40) {
                NiControllerManager* v4 = v40->GetControllerManager();
                NiAVObjectPalette* v21 = v4->GetObjectPalette();
                NiFixedString* v20 = ::new (static_cast<void*>(&v33)) NiFixedString("Bip01 Spine1");
                spTarget.operator=(static_cast<NiNode*>(v21->GetAVObject(*v20)));
                v33.~NiFixedString();
                if (spTarget.operator==(static_cast<NiNode*>(nullptr))) {
                    spTarget.operator=(pcaster->Get3D());
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
                static_cast<BoltShaderProperty*>(spBoltProperty.m_pObject)
                    ->SetBoltGrowWidth(fAbsorbBoltGrowWidth.Float());
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
}

void AbsorbEffect::Update3D(float afDelta) {
    static char bskip;
    if (spSource.m_pObject) {
        if (spTarget.m_pObject) {
            if (spBoltNode.m_pObject) {
                if (spBoltProperty.m_pObject) {
                    if (!bskip) {
                        NiPoint3 toTarget;
                        NiMatrix3 rot;
                        NiPoint3 sourceLoc = NiPoint3_ZERO;
                        sourceLoc = spSource.m_pObject->GetWorldTranslate();
                        spBoltNode.m_pObject->SetTranslate(sourceLoc);
                        toTarget = spTarget.m_pObject->GetWorldTranslate() - sourceLoc;
                        float fz = toTarget.z;
                        toTarget.z = 0.0f;
                        float fxy = toTarget.Length();
                        float Y = -NiATan2(fz, fxy);
                        float fZAngle = NiATan2(toTarget.x, toTarget.y);
                        rot.FromEulerAnglesZYX(fZAngle, 0.0f, Y);
                        spBoltNode.m_pObject->SetRotate(rot);
                        float X = fAbsorbMoveSpeed.Float();
                        float fZAnglea = toTarget.Length();
                        static_cast<BoltShaderProperty*>(spBoltProperty.m_pObject)
                            ->BoltShaderProperty::UpdateAndMoveBolt(afDelta, fZAnglea, X, false);
                    }
                }
            }
        }
    }
}

ActiveEffect* AbsorbEffect::Instantiate(MagicCaster* a1, MagicItem* a2, EffectItem* a3) {
    void* v5 = ourMalloc(sizeof(AbsorbEffect));
    if (v5) {
        return ::new (v5) AbsorbEffect(a1, a2, a3);
    }
    else {
        return nullptr;
    }
}
