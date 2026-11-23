#include "Fallout/Magic/TelekinesisEffect.h"

#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSDevices/Controls.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkMouseSpringAction.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/NiObjectGarbage.h"
#include "BSMain/Setting.h"
#include "BSShader/BSFadeNode.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESEffectShader.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Interface/InterfaceManager.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicShaderHitEffect.h"
#include "Fallout/Magic/ValueModifierEffect.h"
#include "Fallout/Magic/bhkTelekinesisListener.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Utilities/Actions/MouseSpring/hkpMouseSpringAction.h"

// Whether the telekinesis archetype took Instantiate: stores RegisterActiveEffect's whole return byte.
static char bRegistered = EffectArchetypes::RegisterActiveEffect(
    EffectArchetypes::TELEKINESIS,
    &TelekinesisEffect::Instantiate
);

TelekinesisEffect::TelekinesisEffect(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect)
  : ValueModifierEffect(apCaster, apSpell, apEffect), spSpring(nullptr), fHoldDistance(0.0f), fMoveVelocity(0.0f),
    pRef(nullptr), bThrow(false), bFirstUpdate(false) {}

TelekinesisEffect::~TelekinesisEffect() {}

// Allocates a copy, constructs it from this effect's caster, spell and effect item, then copies the effect data.
ActiveEffect* TelekinesisEffect::Copy() {
    TelekinesisEffect* pCopy = new TelekinesisEffect(pCaster, pSpell, pEffect);
    CopyData(pCopy);
    return pCopy;
}

void TelekinesisEffect::Update(float afTimeDelta) {
    bhkWorld* pObjectWorld;
    bhkWorld* pCasterWorld;
    ValueModifierEffect::Update(afTimeDelta);
    bhkMouseSpringAction* pSpringAction = static_cast<bhkMouseSpringAction*>(pPlayer->GetBhkMouseSpringAction());
    if (pSpringAction->GetWorldObject() &&
        ((pCasterWorld = !pCaster || !pPlayer->GetGrabObject()->GetParentCell()
              ? nullptr
              : pCaster->GetCasterStatsObject()->GetParentCell()->GetbhkWorld()),
         (pObjectWorld = !pPlayer->GetGrabObject() || !pPlayer->GetGrabObject()->GetParentCell()
              ? nullptr
              : pPlayer->GetGrabObject()->GetParentCell()->GetbhkWorld()),
         pSpringAction &&
             pRef == pPlayer->GetGrabObject() &&
             pPlayer->GetGrabMode() == PlayerCharacter::GRAB_TELEKINESIS &&
             pCasterWorld == pObjectWorld)) {
        if (appMain->GetControls()->GetUserAction(Controls::UACTN_BLOCK, 0) &&
            appMain->GetControls()->GetUserAction(Controls::UACTN_USE, 0)) {
            if (fMoveVelocity >
                fMagicTelekinesisMoveAccelerate.Float() * 0.10000000149011612 + fMagicTelekinesisMoveBase.Float()) {
                bThrow = true;
            }
            Dispel(false);
        }
        else if (appMain->GetControls()->GetUserAction(Controls::UACTN_USE, 0)) {
            if (fMagicTelekinesisMoveBase.Float() > static_cast<double>(fMoveVelocity)) {
                fMoveVelocity = fMagicTelekinesisMoveBase.Float();
            }
            fMoveVelocity = fMagicTelekinesisMoveAccelerate.Float() * afTimeDelta + fMoveVelocity;
            if (fMagicTelekinesisMoveMax.Float() < static_cast<double>(fMoveVelocity)) {
                fMoveVelocity = fMagicTelekinesisMoveMax.Float();
            }
        }
        else if (appMain->GetControls()->GetUserAction(Controls::UACTN_BLOCK, 0)) {
            if (fMoveVelocity > -fMagicTelekinesisMoveBase.Float()) {
                fMoveVelocity = -fMagicTelekinesisMoveBase.Float();
            }
            fMoveVelocity = fMoveVelocity - fMagicTelekinesisMoveAccelerate.Float() * afTimeDelta;
            if (fMoveVelocity < -fMagicTelekinesisMoveMax.Float()) {
                fMoveVelocity = -fMagicTelekinesisMoveMax.Float();
            }
        }
        else {
            fMoveVelocity = 0.0f;
        }
        fHoldDistance = pPlayer->GetGrabDistance();
        fHoldDistance = fMoveVelocity * afTimeDelta + fHoldDistance;
        if (fMagicTelekinesisDistanceMin.Float() > static_cast<double>(fHoldDistance)) {
            fHoldDistance = fMagicTelekinesisDistanceMin.Float();
        }
        double dMaxDistance = fMagnitude * fMagicUnitsPerFoot.Float();
        if (fHoldDistance > fMagicTelekinesiDistanceMult.Float() * dMaxDistance) {
            dMaxDistance = fMagnitude * fMagicUnitsPerFoot.Float();
            fHoldDistance = fMagicTelekinesiDistanceMult.Float() * dMaxDistance;
        }
        pPlayer->SetTelekinesisDistance(fHoldDistance);
        if (!bFirstUpdate) {
            pPlayer->UpdateMouseSpring(afTimeDelta);
        }
        bFirstUpdate = false;
    }
    else {
        Dispel(false);
    }
}

// Four bytes of its own; save version 0x40 added two more four-byte values.
uint16_t TelekinesisEffect::GetSaveSize(Actor* apActor) {
    uint16_t usSize = ValueModifierEffect::GetSaveSize(apActor);
    usSize += 4;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x40) {
        usSize += 4;
        usSize += 4;
    }
    return usSize;
}

// The target is saved as its form ID; save version 0x40 added the hold distance and move velocity.
void TelekinesisEffect::SaveGame(Actor* apActor) {
    ValueModifierEffect::SaveGame(apActor);
    uint32_t uiRefID = 0;
    if (pRef) {
        uiRefID = pRef->GetFormID();
    }
    pSaveLoadGameOLD->SaveNumericID(&uiRefID, 4);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x40) {
        pSaveLoadGameOLD->SaveGameDataOLD(&fHoldDistance, sizeof(fHoldDistance));
        pSaveLoadGameOLD->SaveGameDataOLD(&fMoveVelocity, sizeof(fMoveVelocity));
    }
}

// The target field holds the loaded form ID until the load game resolves it.
void TelekinesisEffect::LoadGame(Actor* apActor) {
    ValueModifierEffect::LoadGame(apActor);
    uint32_t uiRefID;
    pSaveLoadGameOLD->LoadNumericID(&uiRefID, 4);
    pRef = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(uiRefID));
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x40) {
        pSaveLoadGameOLD->LoadGameDataOLD(&fHoldDistance, sizeof(fHoldDistance));
        pSaveLoadGameOLD->LoadGameDataOLD(&fMoveVelocity, sizeof(fMoveVelocity));
    }
}

void TelekinesisEffect::FinishInitLoadGame(Actor* apActor) {
    ValueModifierEffect::FinishInitLoadGame(apActor);
    TESObjectREFR* pRef = this->pRef;
    if (pRef) {
        TESForm* pForm = TESForm::GetFormByNumericID(reinterpret_cast<uintptr_t>(pRef));
        this->pRef = dynamic_cast<TESObjectREFR*>(pForm);
    }
    if (this->pRef) {
        CreateSpring();
    }
    bFirstUpdate = true;
}

void TelekinesisEffect::Revert(Actor* apActor) {
    ValueModifierEffect::Revert(apActor);
}

// A deleted caster dispels the effect at once.
bool TelekinesisEffect::CheckDeletedCaster(MagicCaster* apCaster) {
    bool bDeleted = ValueModifierEffect::CheckDeletedCaster(apCaster);
    if (bDeleted) {
        Dispel(true);
    }
    return bDeleted;
}

void TelekinesisEffect::CopyData(ActiveEffect* apSource) {
    ValueModifierEffect::CopyData(apSource);
}

void TelekinesisEffect::Start() {
    ValueModifierEffect::Start();
    pRef = InterfaceManager::GetTelekinesisTarget();
    CreateSpring();
    bFirstUpdate = false;
}

void TelekinesisEffect::Finish() {
    DoThrow();
    TESEffectShader* pShader = GetEffectSetting()->data.pEffectShader;
    ReferenceProcessLists.FinishMagicShaderHitEffect(pRef, pShader);
    ValueModifierEffect::Finish();
}

void TelekinesisEffect::DoThrow() {
    if (!pPlayer->GetBhkMouseSpringAction()) {
        return;
    }
    hkpEntity* pEntity = static_cast<bhkMouseSpringAction*>(pPlayer->GetBhkMouseSpringAction())->GetEntity();
    pPlayer->DestroyMouseSpring();
    if (!pEntity) {
        return;
    }
    bhkRigidBody* pBody = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(pEntity));
    if (bThrow) {
        NiPoint3 kDirection;
        alignas(16) NiMatrix3 kRotation;
        NiPoint3 kAngle = pPlayer->GetAngleOnReference();
        kRotation.FromEulerAnglesZXY(kAngle.z, kAngle.x, kAngle.y);
        kRotation.GetCol(1, kDirection);
        float fSpeed = fMagicTelekinesisThrow.Float() + fMoveVelocity;
        NiPoint3 kVelocity = kDirection * fSpeed;
        alignas(16) hkVector4 kImpulse;
        NI2HK(kImpulse, kVelocity);
        hkSimdReal kMass(pBody->GetMass());
        kImpulse.mul4(kMass);
        (**pBody)->applyLinearImpulse(kImpulse);
    }
    else {
        pBody->SetLinearVelocity(NiPoint3::ZERO);
    }
}

void TelekinesisEffect::CreateSpring() {
    CFilter kListenerFilter;
    CFilter kWeaponFilter;
    CFilter kClutterFilter;
    NiPoint3 location;
    if (pRef && pCaster && pRef->GetObjectReference()->GetFormType() != CONT_ID) {
        NiAVObject* pAV = pRef->Get3D();
        bhkCollisionObject* pCol = pAV ? FindFirstCollisionObject(pAV) : nullptr;
        bhkRigidBody* pBody = pCol ? pCol->GetRigidBody() : nullptr;
        if (pBody) {
            bhkTelekinesisListener::AllowToFinish(pBody);
        }
        if (pBody &&
            (pBody->GetCollisionFilter(&kClutterFilter)->GetLayer() == L_CLUTTER ||
             pBody->GetCollisionFilter(&kWeaponFilter)->GetLayer() == L_WEAPON)) {
            NiPoint3 dir;
            NiPoint3 origin;
            new bhkTelekinesisListener(pRef, pBody, pBody->GetCollisionFilter(&kListenerFilter)->iFilter);
            BSFadeNode* pAVLOD = static_cast<BSFadeNode*>(NiObject::DynamicCastStatic(&BSFadeNode::ms_RTTI, pAV));
            if (pAVLOD) {
                pAVLOD->SetLODMultType(LOD_MULT_DISTANTLOD);
            }
            if (pPlayer->GetBhkMouseSpringAction()) {
                pPlayer->DestroyMouseSpring();
            }
            if (!pSaveLoadGameOLD->GetLoading()) {
                fHoldDistance = 10000.0f;
            }
            pPlayer->CreateMouseSpring(pRef, PlayerCharacter::GRAB_TELEKINESIS, fHoldDistance);
            pPlayer->GetEyeVector(origin, dir, true);
            if (!pSaveLoadGameOLD->GetLoading()) {
                bhkMouseSpringAction* pSpringAction = static_cast<bhkMouseSpringAction*>(
                    pPlayer->GetBhkMouseSpringAction()
                );
                pSpringAction->GetMousePositionInWorld(location);
                NiPoint3 originToRef = location - origin;
                fHoldDistance = dir.Dot(originToRef);
                pPlayer->SetTelekinesisDistance(fHoldDistance);
                dir *= fHoldDistance;
            }
            if (GetEffectSetting()->data.pEffectShader) {
                MagicShaderHitEffect* pEffect = new MagicShaderHitEffect(
                    pRef,
                    GetEffectSetting()->data.pEffectShader,
                    -1.0f
                );
                if (pEffect->Init()) {
                    ReferenceProcessLists.AddTempEffect(pEffect);
                }
                else if (pEffect) {
                    delete pEffect;
                }
            }
        }
        else {
            Dispel(false);
        }
    }
    else {
        Dispel(false);
    }
}

ActiveEffect* TelekinesisEffect::Instantiate(MagicCaster* apCaster, MagicItem* apSpell, EffectItem* apEffect) {
    return new TelekinesisEffect(apCaster, apSpell, apEffect);
}

hkpMouseSpringAction* bhkMouseSpringAction::operator*() const {
    if (this) {
        return static_cast<hkpMouseSpringAction*>(GethkObject());
    }

    return nullptr;
}

// The game-unit overload.
void bhkMouseSpringAction::GetMousePositionInWorld(NiPoint3& arPos) {
    hkpMouseSpringAction* pm = **this;
    if (pm) {
        HK2NI(arPos, pm->m_mousePositionInWorld);
    }
}
