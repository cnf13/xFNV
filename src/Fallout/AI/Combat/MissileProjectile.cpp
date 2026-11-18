#include "Fallout/AI/Combat/MissileProjectile.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkConstraintManager.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkHingeConstraint.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkPickData.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSCloningProcess.h"
#include "BSMain/Setting.h"
#include "BSMain/SettingCollection.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Havok/TESHavokUtilities.h"
#include "Fallout Shared/Havok/TESWaterListener.h"
#include "Fallout Shared/InventoryChanges.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESChildCell.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/Combat/ProjectileTarget.h"
#include "Fallout/AI/Combat/VATS.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/AI/MiddleHighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiViewerStrings.h"
#include "Havok/Source/Common/Base/Math/Matrix/hkTransform.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Agent/Collidable/hkpCollidable.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpAllRayHitCollector.h"
#include "Havok/Source/Physics/Dynamics/Constraint/Bilateral/Hinge/hkpHingeConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Constraint/hkpConstraintData.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpEntity.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"
#include "Havok/Source/Physics/Dynamics/Motion/hkpMotion.h"
#include "Havok/Source/Physics/Dynamics/World/hkpWorldObject.h"

MissileProjectile::MissileProjectile(bool abAdd)
  : Projectile(abAdd), eImpactResult(IR_NONE), bWaitingToInitialize3D(false), fStickTime(0.0f), fLocalRot(0.0f) {
    SetFormType(PMIS_ID);
}

MissileProjectile::MissileProjectile(
    BGSProjectile* a1,
    TESObjectREFR* a2,
    TESObjectWEAP* a3,
    NiPoint3 a4,
    float a5,
    float a6
)
  : Projectile(a1, a2, a3, a4, a5, a6), eImpactResult(IR_NONE), bWaitingToInitialize3D(false), fStickTime(0.0f),
    fLocalRot(0.0f) {
    SetFormType(PMIS_ID);
}

bool MissileProjectile::IsPermanent() {
    return eImpactResult == IR_STICK || Projectile::IsPermanent();
}

void MissileProjectile::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    Projectile::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eImpactResult), 4, 0);
}

void MissileProjectile::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Projectile::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eImpactResult), 4);
}

void MissileProjectile::InitLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Projectile::InitLoadGame(apLoadGameBuffer);
}

void MissileProjectile::Revert(BGSLoadFormBuffer* apLoadGameBuffer) {
    Projectile::Revert(apLoadGameBuffer);
    eImpactResult = IR_NONE;
}

void MissileProjectile::FinishLoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Projectile::FinishLoadGame(apLoadGameBuffer);
    if (Get3D()) {
        Initialize3D();
    }
    else {
        bWaitingToInitialize3D = true;
    }
}

Projectile::PROJECTILE_TYPE MissileProjectile::GetProjectileType() {
    return PROJECTILE_TYPE_MISSILE;
}

// Sets the flags from the base form: a tracer roll, hitscan (never during VATS playback) and gravity.
void MissileProjectile::Initialize() {
    bool bInPlayback = pVATS.GetMode() == VATS::VATS_PLAYBACK;
    bool bIsTracer = GetProjectileBase()->DecideIsTracer();
    bool bHitScan;
    if (bInPlayback) {
        bHitScan = false;
    }
    else {
        bHitScan = GetProjectileBase()->GetHitScan();
    }
    bool bHasGravity = GetProjectileBase()->GetGravity() > 0.0;
    Projectile::SetFlag(Projectile::TRACER, bIsTracer);
    Projectile::SetFlag(Projectile::HITSCAN_MODE, bHitScan);
    Projectile::SetFlag(Projectile::NO_COLLISION, bHitScan);
    Projectile::SetFlag(Projectile::FADING, bIsTracer && bHitScan);
    Projectile::SetFlag(Projectile::NO_3D, !bIsTracer && bHitScan);
    Projectile::SetFlag(Projectile::REORIENT, bHasGravity && !bHitScan);
    Projectile::SetFlag(Projectile::CHECK_BEHIND_WALL, true);
    Projectile::SetFlag(Projectile::KILL_ON_EXPLOSION, !bHitScan);
    Projectile::SetFlag(Projectile::SPAWN_EXPLOSION_AT_IMPACT, bHitScan);
    Projectile::Initialize();
}

void MissileProjectile::Process3D() {
    NiPoint3 dst;
    NiBound tracerBound;
    NiNode* pnode = Get3D();
    if ((pnode) && (NiObject::IsKindOf_0(&NiNode::ms_RTTI, pnode))) {
        pnode->SetAppCulled(false);
        if (GetProjectileBase()->Data.fTracerChance > 0.0) {
            if (pnode->GetAtChecked(0)) {
                bool z = IsFlagSet(Projectile::TRACER);
                NiAVObject* v1 = pnode->GetAt(0);
                v1->SetAppCulled(z);
            }
            if (pnode->GetAtChecked(1u)) {
                bool za = !IsFlagSet(Projectile::TRACER);
                NiAVObject* v2 = pnode->GetAt(1u);
                v2->SetAppCulled(za);
            }
        }
        if (IsFlagSet(Projectile::TRACER)) {
            tracerBound = pnode->GetWorldBound();
            NiPoint3* zb = &(&tracerBound)->m_kCenter;
            NiPoint3 fromCenter = pnode->GetTranslate() - *zb;
            double v8 = fromCenter.Length();
            float ffakeDistance = tracerBound.m_fRadius + v8;
            double Speed = GetSpeed();
            float ffakeTime = ffakeDistance / Speed;
            NiPoint3 x = *::new (&dst) NiPoint3(0.0f, ffakeDistance, 0.0f);
            Move(ffakeTime, x);
            Sync3DWithReference(false, true);
            NiUpdateData UpdateData(0.0f, false, false);
            pnode->UpdateWorldData(UpdateData);
            double IronSightFOV = fDistanceMoved;
            if (GetRange() < IronSightFOV) {
                pnode->SetAppCulled(true);
            }
        }
    }
}

void MissileProjectile::UpdateProjectile(float afDelta) {
    NiMatrix3 mMatrixLocal_2;
    NiMatrix3 mMatrix;
    NiPoint3 oldpos;
    NiPoint3 newpos;
    if ((bWaitingToInitialize3D) && (Get3D())) {
        Initialize3D();
    }

    double IronSightFOV = fDistanceMoved;
    if (GetRange() < IronSightFOV && (ImpactList.IsEmpty() || bImpactProcessed)) {
        Kill();
    }
    if (!GetDelete() && !GetDisabled()) {
        if (eImpactResult == MissileProjectile::IR_STICK) {
            fStickTime = fStickTime - afDelta;
            if (fStickTime <= 0.0) {
                Kill();
            }
        }
        if (IsFlagSet(Projectile::POST_PROCESS) && eImpactResult == MissileProjectile::IR_STICK) {
            Projectile::ImpactData* pimpact = (&ImpactList)->m_item;
            if (pimpact) {
                NiAVObject* ptargetAV;
                if (pimpact->pTargetWorldObject) {
                    ptargetAV = GetAVObjectForCollidable(&pimpact->pTargetWorldObject->m_collidable);
                }
                else {
                    ptargetAV = nullptr;
                }
                NiAVObject* p3d = (NiAVObject*)Get3D();
                if ((ptargetAV) && (p3d)) {
                    NiTransform newTrans = ptargetAV->GetWorldTransform() * kFollowOffset;
                    p3d->SetRotate(newTrans.m_Rotate);
                    SetRotation(&newTrans.m_Rotate);
                    p3d->SetTranslate(newTrans.m_Translate);
                    SetLocationOnReference(newTrans.m_Translate);
                }

                Sync3DWithReference(true, false);
            }
        }
        else {
            if (!bImpactProcessed ||
                (IsFlagSet(Projectile::HITSCAN_MODE) || eImpactResult == MissileProjectile::IR_IMPALE)) {
                bool bfirstUpdate = GetAge() <= 0.0;
                bool breorient = false;
                if (pTarget) {
                    ProjectileTarget* CurrentFurniture = pTarget;
                    NiPoint3 fG = CurrentFurniture->GetWorldLocation();
                    AimAtPoint(fG);
                }
                if (!bfirstUpdate) {
                    if (pTarget) {
                        const NiPoint3* angle = &GetLocationOnReference();
                        ProjectileTarget* v3 = pTarget;
                        NiPoint3 targetDelta = v3->GetWorldLocation() - *angle;
                        double v14 = targetDelta.Length();
                        if (fProjectileReorientTracerMin.Float() < v14) {
                            breorient = true;
                        }
                    }
                    if (bProjectileDebug.Bool()) {
                        oldpos = GetLocationOnReference();
                        Move(afDelta);
                        newpos = GetLocationOnReference();
                        NiColorA color(1.0f, 1.0f, 0.0f, 1.0f);
                        NiAVObject* pdebug = (NiAVObject*)MakeLine(oldpos, color, newpos, color, true);
                        float* v5 = &fProjectileDebugDuration.Float();
                        TES::pInstance->AddTempDebugObject(pdebug, *v5);
                    }
                    else {
                        Move(afDelta);
                    }
                }
                bool v13 = ImpactList.IsEmpty() || (&ImpactList)->m_item->bProcessed;
                if (!v13) {
                    double v12;
                    if (!IsFlagSet(Projectile::HITSCAN_MODE) || (v12 = fDistanceMoved, GetRange() < v12)) {
                        SetLocationOnReference((&ImpactList)->m_item->kLocation);
                    }
                }
                Sync3DWithReference(breorient, !bfirstUpdate);
                if (Get3D()) {
                    NiNode* v6 = Get3D();
                    v6->UpdateWorldLoc();
                }
                if (GetProjectileBase()->GetRotates()) {
                    NiMatrix3 mMatrixLocal;
                    if (!(GetProjectileBase()->Data.fRotationX > 0.0)) {
                        if (!(GetProjectileBase()->Data.fRotationY > 0.0)) {
                            if (GetProjectileBase()->Data.fRotationZ > 0.0) {
                                fLocalRot = GetProjectileBase()->Data.fRotationZ * afDelta + fLocalRot;
                                mMatrixLocal.MakeZRotation(fLocalRot);
                            }
                        }
                        else {
                            fLocalRot = GetProjectileBase()->Data.fRotationY * afDelta + fLocalRot;
                            mMatrixLocal.MakeYRotation(fLocalRot);
                        }
                    }
                    else {
                        fLocalRot = GetProjectileBase()->Data.fRotationX * afDelta + fLocalRot;
                        mMatrixLocal.MakeXRotation(fLocalRot);
                    }
                    if (Get3D()) {
                        mMatrix = (*&Get3D()->m_kLocal).m_Rotate;
                        mMatrix = *::new (&mMatrixLocal_2) NiMatrix3(mMatrix * mMatrixLocal);
                        NiNode* v7 = Get3D();
                        v7->SetRotate(mMatrix);
                    }
                    else {
                        Warning(
                            WARN_TEXTURES,
                            "---> Get3D() has failed in MissileProjectile::UpdateProjectile for projectile: %s!",
                            GetFullName()
                        );
                    }
                }
            }
        }
        CheckExplosion(afDelta);
        double fAge = GetAge();
        if (fArrowAgeMax.Float() < fAge && eImpactResult != MissileProjectile::IR_STICK) {
            Kill();
        }
        double v10 = fDistanceMoved;
        if (GetRange() < v10 && (ImpactList.IsEmpty() || bImpactProcessed)) {
            if (Get3D()) {
                NiNode* v8 = Get3D();
                v8->SetAppCulled(true);
            }
            Kill();
        }
    }
}

// A hitscan missile processes its impact once it has travelled to the impact's range (less one 1/30 s step), or
// once it has faded out. After processing, it stops being VATS's first or last projectile, then acts on
// eImpactResult.
bool MissileProjectile::ProcessImpacts() {
    bool bProcessed = false;
    if (!bImpactProcessed && !ImpactList.IsEmpty()) {
        bool bProcess = true;
        if (Projectile::IsFlagSet(Projectile::HITSCAN_MODE)) {
            double dRange = GetRange();
            float fStep = GetSpeed() * 0.0333000011742115;
            float fRemaining = dRange - std::fabs(fStep);
            bProcess = GetAlpha() <= 0.0 || fRemaining < GetDistanceMoved();
        }
        if (bProcess) {
            bProcessed = Projectile::ProcessImpacts();
        }
    }
    if (bProcessed) {
        if (pVATS.GetFirstProjectile() == this) {
            pVATS.SetFirstProjectile(nullptr);
        }
        if (pVATS.GetLastProjectile() == this) {
            pVATS.SetLastProjectile(nullptr);
        }
        switch (eImpactResult) {
            case IR_DESTROY: Kill(); break;
            case IR_BOUNCE: ProcessBounce(); break;
            case IR_IMPALE: ProcessImpale(); break;
            case IR_STICK: ProcessStick(); break;
            default: break;
        }
    }
    return bProcessed;
}

MissileProjectile::~MissileProjectile() {
    if (GetProjectileBase()->GetPinsLimbs()) {
        NiNode* p3D = TESObjectREFR::Get3D();
        if (p3D) {
            NiNode* pLimb = static_cast<NiNode*>(p3D->GetObjectByName(*FixedStrings::QPinnedLimb()));
            if (pLimb) {
                pLimb->GetParent()->DetachChild(pLimb);
            }
        }
    }
}

// A missile takes a new impact only while it has none or the last one is processed. An impaling missile only
// sticks into static world geometry.
bool MissileProjectile::AddImpact(
    TESObjectREFR* apCollidee,
    const NiPoint3& aContactPoint,
    const NiPoint3& aContactNormal,
    const hkpCollidable* apTargetCollidable,
    uint32_t aiKey,
    HavokMaterialType aeForceMaterial
) {
    bool bCanAdd;
    bCanAdd = ImpactList.IsEmpty() || ImpactList.GetItem()->bProcessed;
    COL_LAYER eLayer;
    if (apTargetCollidable) {
        CFilter kFilter;
        eLayer = XFNV_RECONSTRUCT(CFilter)(&kFilter, apTargetCollidable->getCollisionFilterInfo())->GetLayer();
    }
    else {
        eLayer = (COL_LAYER)0;
    }
    if (!bCanAdd) {
        return false;
    }
    if (!Projectile::IsValidImpact(apCollidee, apTargetCollidable, this, GetProjectileBase()) ||
        (eImpactResult == IR_IMPALE && eLayer != L_STATIC && eLayer != L_ANIMSTATIC && eLayer != L_TREES &&
         eLayer != L_GROUND && eLayer != L_TERRAIN && eLayer != L_DEBRIS_LARGE) ||
        !Projectile::AddImpact(apCollidee, aContactPoint, aContactNormal, apTargetCollidable, aiKey, aeForceMaterial)) {
        return false;
    }
    DetermineImpactResult();
    return true;
}

void MissileProjectile::Initialize3D() {
    if (eImpactResult == MissileProjectile::IR_STICK || eImpactResult == MissileProjectile::IR_IMPALE) {
        Projectile::ImpactData* pimpact = ImpactList.m_item;
        if (eImpactResult == MissileProjectile::IR_STICK) {
            Projectile::ImpactData* pimpact_2;
            if (ImpactList.m_pkNext) {
                pimpact_2 = ImpactList.m_pkNext->m_item;
            }
            else {
                pimpact_2 = nullptr;
            }
            pimpact = pimpact_2;
        }
        if ((pimpact) && (pimpact->pCollidee) && (pimpact->pCollidee->IsActor())) {
            ProcessImpale();
        }
    }
    if (eImpactResult == MissileProjectile::IR_STICK) {
        ProcessStick();
    }
}

void MissileProjectile::DetermineImpactResult() {
    BGSProjectile* actorBase;
    BGSProjectile* v2;
    if (((int32_t)eImpactResult) == 3) {
        eImpactResult = MissileProjectile::IR_STICK;
    }
    else {
        bool vCondition1;
        vCondition1 = !(&ImpactList)->IsEmpty();
        if (vCondition1) {
            eImpactResult = MissileProjectile::IR_DESTROY;
            actorBase = Projectile::GetProjectileBase();
            bool vCondition2;
            vCondition2 = actorBase->GetPinsLimbs();
            if (vCondition2) {
                eImpactResult = MissileProjectile::IR_IMPALE;
            }
            if (Projectile::IsFlagSet(Projectile::HITSCAN_MODE) && (!Projectile::IsFlagSet(Projectile::NO_3D))) {
                eImpactResult = MissileProjectile::IR_NONE;
            }
            if (!Projectile::IsFlagSet(Projectile::HITSCAN_MODE)) {
                v2 = Projectile::GetProjectileBase();
                if (v2->IsFlagSet(BGSProjectile::BOUNCE_ON_IMPACT) || Projectile::GetSecondaryItemChange()) {
                    eImpactResult = MissileProjectile::IR_BOUNCE;
                }
            }
        }
    }
}

bool MissileProjectile::ProcessBounce() {
    if ((Get3D()) && (!ImpactList.IsEmpty())) {
        if (pVATS.pLastProjectile == this) {
            pVATS.SetLastProjectile(nullptr);
        }
        NiAVObject* p3d = Get3D();
        Projectile::ImpactData* psCollisionData = ImpactList.m_item;
        SetFlag(Projectile::NO_CHARCONTROLLER, true);
        RemoveCharController();
        bhkCollisionObject* pcoll = FindFirstCollisionObject(p3d);
        if (pcoll) {
            pcoll->SetNotify(true);
        }
        if (!pSaveLoadGameOLD->GetLoading() && !GetSecondaryItemChange()) {
            p3d->SetTranslate(psCollisionData->kLocation);
            NiUpdateData UpdateData(0.0f, false, false);
            p3d->Update(UpdateData);
        }
        bhkWorld::SetMotion(p3d, hkpMotion::MOTION_DYNAMIC, true, false, true);
        uint32_t NewSystemGroup = bhkCollisionFilter::GetNewSystemGroup();
        bhkWorld::SetGroup(p3d, NewSystemGroup, true, false);
        bhkWorld::EnableCollision(p3d, true, true, false);
        if (!pSaveLoadGameOLD->GetLoading()) {
            NiPoint3 force;
            NiMatrix3 invrot;
            NiPoint3 vton;
            NiPoint3 reflectedVelocity = NiPoint3_ZERO;
            NiPoint3 accum = -1.0f * GetMovementDirection();
            NiPoint3::UnitizeVector(accum);
            NiPoint3::UnitizeVector(psCollisionData->kNormal);
            float fnormalDotDirection = psCollisionData->kNormal.Dot(accum);
            vton = 2.0f * (fnormalDotDirection * psCollisionData->kNormal - accum);
            reflectedVelocity = accum + vton;
            float* v5 = &fArrowBounceLinearSpeed.Float();
            float v12 = GetProjectileBase()->GetSpeed() * *v5;
            reflectedVelocity.operator*=(v12);
            TESHavokUtilities::AddVelocity(p3d, reflectedVelocity, true);
            NiPoint3 angularVelocity = NiPoint3_ZERO;
            NiNode* v6 = Get3D();
            (&v6->GetWorldTransform())->m_Rotate.Inverse(invrot);
            force = invrot * psCollisionData->kNormal;
            double v11 = NiAbs(force.y);
            if (NiAbs(force.z) < v11) {
                double z = NiAbs(force.y);
                force.z = NiSign(force.z) * z;
            }
            force.y = 0.0f;
            NiPoint3::UnitizeVector(force);
            NiPoint3 angle(-force.z, 0.0f, -force.x);
            angularVelocity = (&p3d->GetWorldTransform())->m_Rotate * angle;
            angularVelocity.operator*=(fArrowBounceLinearSpeed.Float());
            TESHavokUtilities::AddAngularVelocity(p3d, angularVelocity, true);
        }
    }

    return true;
}

void MissileProjectile::ProcessImpale() {
    CFilter v41;
    CFilter v43;
    alignas(16) CFilter v46;
    CFilter v57;
    CFilter v59;
    CFilter filter;
    NiMatrix3 rot2_4;
    NiMatrix3 rot_3;
    NiTransform projectileWorld_2;
    XFNV_INLINED_LOCAL(hkpAllRayHitCollector, collector, std::nullopt, std::nullopt);
    NiMatrix3 rot2_2;
    NiMatrix3 rot_2;
    NiTransform v103;
    NiTransform projectileWorld;
    NiTransform rot;
    Projectile::ImpactData* pimpact_2;
    NiNode* p3d;
    NiNode* v131;
    int32_t v140;
    int32_t* v135 = &v140;
    static float fpinLimDepth = fCombatMissileImpaleDepth.Float();
    bool bloading = pSaveLoadGame->GetSaveGameLoading();
    if (this->ImpactList.IsEmpty()) {
        goto LABEL_107;
    }
    v131 = this->Get3D();
    p3d = v131->IsNode();
    pimpact_2 = (&this->ImpactList)->m_item;
    this->fStickTime = 10.0f;
    if (this->eImpactResult == MissileProjectile::IR_STICK) {
        Projectile::ImpactData* pimpact;
        if (this->ImpactList.m_pkNext) {
            pimpact = this->ImpactList.m_pkNext->m_item;
        }
        else {
            pimpact = nullptr;
        }
        pimpact_2 = pimpact;
    }
    if (pimpact_2 && (pimpact_2->pCollidee && pimpact_2->pCollidee->IsActor())) {
        if (!pimpact_2->bProcessed) {
            NiAVObject* ptarget3D_2;
            NiNode* ptarget3D;
            char v125;
            ExtraDismemberedLimbs* pextraLimbs;
            Actor* pactor = static_cast<Actor*>(pimpact_2->pCollidee);
            if (!pactor->GetDismembered(pimpact_2->eDamageLimb)) {
                goto LABEL_106;
            }
            pextraLimbs = (&pactor->m_Extra)->GetDismembermentExtra();
            v125 = 0;
            for (uint32_t i = 0; i < pextraLimbs->DismemberedLimbs.iSize; ++i) {
                if (pextraLimbs->DismemberedLimbs.GetAt(static_cast<int32_t>(i))->cLimb ==
                        static_cast<int8_t>(static_cast<uint32_t>(pimpact_2->eDamageLimb)) &&
                    pextraLimbs->DismemberedLimbs.GetAt(static_cast<int32_t>(i))->bLimbExploded) {
                    v125 = 1;
                }
            }
            ptarget3D = pimpact_2->pCollidee ? pimpact_2->pCollidee->Get3D() : nullptr;
            ptarget3D_2 = (NiAVObject*)ptarget3D;
            if (ptarget3D && !v125) {
                const char* plimbNodeName = pLimbEnumNames[pimpact_2->eDamageLimb];
                NiAVObject* pdismemberedLimb_2 = ptarget3D_2 ? ptarget3D_2->GetObjectByName(plimbNodeName) : nullptr;
                float f3dScale = pdismemberedLimb_2->m_kWorld.m_fScale;
                if (pdismemberedLimb_2) {
                    alignas(16) BSCloningProcess cloneProcess(1.0f);
                    cloneProcess.SetScale(1.0f);
                    NiAVObject* pdismemberedLimb_3 = static_cast<NiAVObject*>(pdismemberedLimb_2->Clone(cloneProcess));
                    pactor->RemoveDismemberedLimb(pimpact_2->eDamageLimb);
                    pdismemberedLimb_2 = pdismemberedLimb_3;
                    pdismemberedLimb_3->SetAppCulled(false);
                    pdismemberedLimb_2->SetScale(f3dScale);
                    NiNode* pdismemberedNode = pdismemberedLimb_2->IsNode();
                    if (pdismemberedNode) {
                        for (int32_t n = 0; n < pdismemberedNode->GetArrayCount(); ++n) {
                            NiAVObject* pobj = pdismemberedNode->GetAt(static_cast<uint32_t>(n));
                            if ((pobj) && (Animation::FindSkinnedNode(pobj))) {
                                BipedAnim::AttachToSkeleton(pdismemberedNode, pobj, pdismemberedNode, false);
                            }
                        }
                    }
                    NiUpdateData updateData(0.0f, false, false);
                    pdismemberedLimb_2->Update(updateData);
                    bhkWorld* phw = this->GetParentCell()->GetbhkWorld();
                    if (phw) {
                        phw->AddObjects(pdismemberedLimb_2, true, false, 0, true);
                    }
                    pdismemberedLimb_2->SetTranslate(NiPoint3_ZERO);
                    pdismemberedLimb_2->SetRotate(NiMatrix3_IDENTITY);
                    if ((pdismemberedNode) && (pdismemberedNode->GetAtChecked(0))) {
                        rot = pdismemberedNode->GetAt(0)->GetWorldTransform();
                        if (!bloading) {
                            this->SetLocationOnReference(pimpact_2->kLocation);
                        }
                        this->Sync3DWithReference(false, false);
                        alignas(16) NiUpdateData UpdateData(0.0f, false, false);
                        p3d->UpdateWorldData(UpdateData);
                        projectileWorld = p3d->GetWorldTransform();
                        NiTransform rotMult;
                        projectileWorld.InvertNonUniform(rotMult);
                        rot = *::new (&v103) NiTransform(rotMult * rot);
                        rot.m_Translate.y = rot.m_Translate.y - fpinLimDepth;
                        NiAVObject* v1 = pdismemberedNode->GetAt(0);
                        v1->SetTranslate(rot.m_Translate);
                        NiAVObject* v2 = pdismemberedNode->GetAt(0);
                        v2->SetRotate(rot.m_Rotate);
                        float a2 = rot.m_fScale;
                        NiAVObject* pdismemberedLimb_4 = pdismemberedNode->GetAt(0);
                        pdismemberedLimb_4->SetScale(a2);
                    }

                    p3d->AttachChild(pdismemberedLimb_2, 1);
                    pdismemberedLimb_2->SetName(*FixedStrings::QPinnedLimb());
                    bhkWorld::UpdatePosition(pdismemberedLimb_2, true, true);
                    bhkWorld::SetMotion(pdismemberedLimb_2, hkpMotion::MOTION_KEYFRAMED, true, true, true);
                    NiUpdateData UpdateData_2(0.0f, false, false);
                    p3d->Update(UpdateData_2);
                    char v101 = 0;
                    if (pdismemberedLimb_2) {
                        alignas(16) NiMatrix3 rot2;
                        NiPoint3 direction;
                        NiAVObject* pconstrainedObject = nullptr;
                        bhkWorld* pw = this->GetParentCell()->GetbhkWorld();
                        bhkPickData pd;
                        NiPoint3 origin = this->GetLocationOnReference();
                        float a2a = this->GetHeading(false);
                        rot_2.MakeZRotation(a2a);
                        float a2b = this->GetLooking();
                        rot2.MakeXRotation(a2b);
                        rot_2 = *::new (&rot2_2) NiMatrix3(rot_2 * rot2);
                        rot_2.GetCol(1u, direction);
                        direction.Unitize();
                        pd.NiSetFrom(origin - direction * 512.0f);
                        pd.NiSetLength(direction * 1024.0f);
                        ::new (&collector) hkpAllRayHitCollector;
                        pd.SetAllCollector(&collector);
                        if (Projectile::PickSceneGraph(&pd, pdismemberedLimb_2, false, true) &&
                            (&pd.pAllCollector->getHits())->m_size > 0) {
                            const hkpWorldRayCastOutput* v7 =
                                &(&pd.pAllCollector->getHits())->operator[]<hkpWorldRayCastOutput>(0);
                            pconstrainedObject = GetAVObjectForCollidable(v7->m_rootCollidable);
                        }
                        collector.~hkpAllRayHitCollector();
                        if (pconstrainedObject) {
                            NiNode* v8 = this->Get3D();
                            bhkCollisionObject* pcol = FindFirstCollisionObject(v8);
                            bhkRigidBody* pprojectileRB = pcol ? pcol->GetRigidBody() : nullptr;
                            bhkCollisionObject* plimbCol = FindFirstCollisionObject(pconstrainedObject);
                            bhkRigidBody* plimbRB = plimbCol ? plimbCol->GetRigidBody() : nullptr;
                            if ((plimbRB) && (plimbRB != pprojectileRB)) {
                                alignas(16) NiMatrix3 rot2_3;
                                NiPoint3 direction_2;
                                bhkHingeConstraint* ph;
                                hkVector4 fixPointWorld;
                                hkVector4 axis;
                                NiPoint3 yAxisWorld;
                                alignas(16) hkHingeConstraintCinfo ci;
                                ci.hkHingeConstraintCinfo::CreateData();
                                ci.SetEntities(plimbRB, pprojectileRB);
                                const hkTransform* Transform = &plimbRB->operator*()->getTransform();
                                hkTransform fixPointa(*Transform);
                                const hkTransform* v10 = &pprojectileRB->operator*()->getTransform();
                                hkTransform fixPointb(*v10);
                                (&fixPointa)->m_rotation.renormalize();
                                (&fixPointb)->m_rotation.renormalize();
                                NiNode* v11 = this->Get3D();
                                (&v11->GetWorldTransform())->m_Rotate.GetCol(1u, yAxisWorld);
                                NI2HKNS(axis, yAxisWorld);
                                axis.normalize3();
                                projectileWorld_2 = this->Get3D()->GetWorldTransform();
                                float a2c = fpinLimDepth;
                                alignas(16) NiPoint3 offset = -NiPoint3_UNIT_Y * a2c;
                                NiPoint3 hingePoint__offset = projectileWorld_2 * offset;
                                NI2HK(fixPointWorld, hingePoint__offset);
                                ci.SetInWorldSpace(fixPointa, fixPointb, fixPointWorld, axis);
                                bhkHingeConstraint* v71 = static_cast<bhkHingeConstraint*>(
                                    NiMemObject::operator new(sizeof(bhkHingeConstraint))
                                );
                                if (v71) {
                                    ph = ::new (v71) bhkHingeConstraint(&ci);
                                }
                                else {
                                    ph = nullptr;
                                }
                                plimbRB->AddAction(ph);
                                bhkWorld* v13 = this->GetParentCell()->GetbhkWorld();
                                plimbRB->AddActConToWorld(v13);
                                bhkWorld* pw_2 = this->GetParentCell()->GetbhkWorld();
                                bhkPickData pd_2;
                                NiPoint3 origin_2 = this->GetLocationOnReference();
                                float a2d = this->GetHeading(false);
                                rot_3.MakeZRotation(a2d);
                                float a2e = this->GetLooking();
                                rot2_3.MakeXRotation(a2e);
                                rot_3 = *::new (&rot2_4) NiMatrix3(rot_3 * rot2_3);
                                rot_3.GetCol(1u, direction_2);
                                direction_2.Unitize();
                                // The filter's one-field constructor is a trivial store.
                                filter.iFilter = 0;
                                if (this->GetCharController()) {
                                    bhkCharacterController* CharController = this->GetCharController();
                                    filter.iFilter = CharController->GetCollisionFilter(&v59)->iFilter;
                                }
                                else {
                                    TESObjectREFR* Shooter;
                                    filter.SetLayer(L_PROJECTILE);
                                    if (this->GetShooter() && (Shooter = this->GetShooter(), Shooter->IsActor())) {
                                        Actor* pactor_2 = static_cast<Actor*>(this->GetShooter());
                                        // MobileObject::GetCollisionFilter returns its result pointer, &v57.
                                        pactor_2->GetCollisionFilter(&v57.iFilter);
                                        uint32_t Group = v57.GetGroup();
                                        filter.SetGroup(Group);
                                    }
                                    else {
                                        uint32_t NewSystemGroup = bhkCollisionFilter::GetNewSystemGroup();
                                        filter.SetGroup(NewSystemGroup);
                                    }
                                }
                                double Range = this->GetRange();
                                float v55 = Range - this->fDistanceMoved;
                                alignas(16) NiPoint3 length = direction_2 * v55;
                                pd_2.SetFilter(filter);
                                pd_2.NiSetFrom(origin_2);
                                pd_2.NiSetLength(length);
                                hkpAllRayHitCollector collector_2;
                                pd_2.SetAllCollector(&collector_2);
                                bhkWorld* v52 = this->GetParentCell()->GetbhkWorld();
                                v52->PickObject(pd_2);
                                for (int32_t i_2 = 0; !v101; ++i_2) {
                                    COL_LAYER ebodyLayer;
                                    int32_t FormHealth_0 = (&pd_2.pAllCollector->getHits())->m_size;
                                    if (i_2 >= FormHealth_0) {
                                        break;
                                    }
                                    const hkpWorldRayCastOutput* v20 =
                                        &(&pd_2.pAllCollector->getHits())->operator[]<hkpWorldRayCastOutput>(i_2);
                                    hkpWorldRayCastOutput point(*v20);
                                    NiAVObject* pav = GetAVObjectForCollidable(point.m_rootCollidable);
                                    hkpRigidBody* RigidBody = hkpGetRigidBody(point.m_rootCollidable);
                                    bhkRigidBody* pbody = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(RigidBody));
                                    TESObjectREFR* pref = TESObjectREFR::FindReferenceFor3D(pav);
                                    if (pbody) {
                                        CFilter* v22 = pbody->GetCollisionFilter(&v46);
                                        ebodyLayer = v22->GetLayer();
                                    }
                                    else {
                                        ebodyLayer = L_UNIDENTIFIED;
                                    }
                                    COL_LAYER ebodyLayer_2 = ebodyLayer;
                                    if (pbody &&
                                        ((ebodyLayer_2 == L_STATIC ||
                                          (ebodyLayer_2 == L_ANIMSTATIC ||
                                           (ebodyLayer_2 == L_TREES ||
                                            (ebodyLayer_2 == L_GROUND ||
                                             (ebodyLayer_2 == L_TERRAIN || ebodyLayer_2 == L_DEBRIS_LARGE))))) &&
                                         (pbody->GetMotionType() == 5 || pbody->GetMotionType() == 4))) {
                                        v101 = 1;
                                    }
                                    else {
                                        if (pbody && (pref != this && (pref && pref != pimpact_2->pCollidee))) {
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    pimpact_2->bProcessed = true;
                    if (v101) {
                        if (this->pTarget) {
                            this->pTarget = nullptr;
                        }
                        bhkWorld::UpdatePosition(pdismemberedLimb_2, true, true);
                        bhkWorld::SetMotion(pdismemberedLimb_2, hkpMotion::MOTION_KEYFRAMED, true, true, true);
                        if (this->GetCharController()) {
                            bhkCharacterController* v23 = this->GetCharController();
                            uint32_t v25 = v23->GetCollisionFilter(&v43)->GetGroup();
                            bhkWorld::SetGroup(pdismemberedLimb_2, v25, true, true);
                            if (!bloading) {
                                hkVector4 hkpos;
                                this->SetLocationOnReference(pimpact_2->kLocation);
                                NI2HK(hkpos, pimpact_2->kLocation);
                                bhkCharacterController* v26 = this->GetCharController();
                                v26->SetPosition(hkpos);
                            }
                            this->Sync3DWithReference(false, false);
                        }
                        if (!bloading) {
                            this->bImpactProcessed = false;
                        }
                    }
                    else {
                        if (!bloading) {
                            this->SetLocationOnReference(pimpact_2->kLocation);
                        }
                        this->Sync3DWithReference(false, false);
                        this->SetFlag(Projectile::NO_CHARCONTROLLER, true);
                        this->RemoveCharController();
                        bhkWorld::UpdatePosition(pdismemberedLimb_2, true, true);
                        // MobileObject::GetCollisionFilter returns its result pointer, &v41.
                        pactor->GetCollisionFilter(&v41.iFilter);
                        uint32_t v28 = v41.GetGroup();
                        bhkWorld::SetGroup(p3d, v28, true, true);
                        bhkWorld::SetMotion(p3d, hkpMotion::MOTION_DYNAMIC, true, true, true);
                        bhkWorld::EnableCollision(p3d, true, true, false);
                        float afTime = this->GetSpeed();
                        alignas(16) NiPoint3 velocity = this->GetMovementDirection() * afTime / 2.0f;
                        TESHavokUtilities::AddVelocity(p3d, velocity, true);
                        MissileProjectile* LastProjectile = static_cast<MissileProjectile*>(pVATS.pLastProjectile);
                        if (LastProjectile == this) {
                            pVATS.SetLastProjectile(nullptr);
                        }
                    }
                }
                else {
                    this->bImpactProcessed = false;
                }
            }
            else {
            LABEL_106:;
                this->Kill();
            }
        }
    }
    else {
    LABEL_107:;
        this->eImpactResult = MissileProjectile::IR_STICK;
        this->ProcessStick();
    }
}

void MissileProjectile::ProcessStick() {
    NiTransform kDest;
    CFilter apResult;
    CFilter filter;
    static float fCombatMissileStickDepth_11F2020 = fCombatMissileStickDepth.Float();
    bool bloading = pSaveLoadGame->GetSaveGameLoading();
    if ((Get3D()) && (!ImpactList.IsEmpty())) {
        if (pVATS.pLastProjectile == this) {
            pVATS.SetLastProjectile(nullptr);
        }
        fStickTime = 10.0f;
        Projectile::ImpactData* pimpact = (&ImpactList)->m_item;
        if ((pimpact) && (pimpact->pTargetWorldObject)) {
            COL_LAYER ebodyLayer;
            NiAVObject* pav = (NiAVObject*)Get3D();
            NiAVObject* ptargetAV = GetAVObjectForCollidable(pimpact->pTargetWorldObject->getCollidable());
            bhkRigidBody* ptargetRB = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(pimpact->pTargetWorldObject));
            if (ptargetRB) {
                CFilter* CollisionFilter = ptargetRB->GetCollisionFilter(&apResult);
                ebodyLayer = CollisionFilter->GetLayer();
            }
            else {
                ebodyLayer = L_UNIDENTIFIED;
            }
            if (!ptargetRB || (ebodyLayer == L_DEBRIS_SMALL || (pimpact->pCollidee && pimpact->pCollidee->IsActor()))) {
                ProcessBounce();
            }
            else {
                NiAVObject* pdismemberedLimb;
                bhkRigidBody* pprojectileRB;
                SetFlag(Projectile::NO_CHARCONTROLLER, true);
                RemoveCharController();
                bhkCollisionObject* pcol = FindFirstCollisionObject(pav);
                if (pcol) {
                    pprojectileRB = pcol->GetRigidBody();
                }
                else {
                    pprojectileRB = nullptr;
                }
                if (pprojectileRB) {
                    pprojectileRB->GetCollisionFilter(&filter);
                    filter.SetLayer(L_NONCOLLIDABLE);
                    pprojectileRB->SetCollisionFilter(filter);
                }
                if (!bloading) {
                    SetLocationOnReference(
                        pimpact->kLocation + GetMovementDirection() * fCombatMissileStickDepth_11F2020
                    );
                }
                Sync3DWithReference(false, false);
                bhkWorld::UpdatePosition(pav, true, true);
                NiUpdateData UpdateData(0.0f, false, false);
                pav->Update(UpdateData);
                if (ptargetRB->GetMotionType() == 5) {
                    bhkWorld::SetMotion(pav, hkpMotion::MOTION_FIXED, true, false, true);
                }
                else {
                    bhkWorld::SetMotion(pav, hkpMotion::MOTION_KEYFRAMED, true, false, true);
                    if (!IsFlagSet(Projectile::POST_PROCESS)) {
                        SetFlag(Projectile::POST_PROCESS, true);
                        // The list's address is tested for null, which it never is, as the game does.
                        BSSimpleList<void*>* plist = &ReferenceProcessLists.ProjectilePostProcessList;
                        if (plist) {
                            void* item = this;
                            plist->AddHead(item);
                        }
                    }
                    ptargetRB->SetFlags(256u, true);
                    if ((!bloading) && (ptargetAV)) {
                        NiTransform targetInvert;
                        (&ptargetAV->GetWorldTransform())->Invert(targetInvert);
                        kFollowOffset = *::new (&kDest) NiTransform(targetInvert * pav->GetWorldTransform());
                    }
                }
                if (pav) {
                    NiFixedString* PinnedLimb = FixedStrings::QPinnedLimb();
                    pdismemberedLimb = pav->GetObjectByName(*PinnedLimb);
                }
                else {
                    pdismemberedLimb = nullptr;
                }
                if (pdismemberedLimb) {
                    bhkWorld::SetMotion(pdismemberedLimb, hkpMotion::MOTION_DYNAMIC, true, true, true);
                }
            }
        }
    }
}

hkConstraintCinfo::hkConstraintCinfo()
  : pData(nullptr), ePriority(hkpConstraintInstance::PRIORITY_PSI), pEntityA(nullptr), pEntityB(nullptr) {}

hkConstraintCinfo::~hkConstraintCinfo() {
    SetData(nullptr);
}

uint32_t hkConstraintCinfo::CreateData() {
    return 0;
}

void hkConstraintCinfo::GetViewerStrings(NiTPrimitiveArray<char*>* apStrings) {
    char* element;
    char apDest[32];
    TLSData* v3 = GetThreadTLSData();
    MEM_CONTEXT eMemContext = v3->eMemContext;
    v3->eMemContext = MEM_CONTEXT_HAVOK;
    bool v4 = pData == nullptr;
    if (!v4) {
        uint32_t v5 = pData->getType();
        const char* TypeName = bhkConstraintManager::GetTypeName(v5);
        char* ViewerString_9 = NiGetViewerString("Type", TypeName);
        uint32_t m_usSize = apStrings->m_usSize;
        element = ViewerString_9;
        if (m_usSize >= apStrings->m_usMaxSize) {
            apStrings->SetSize(m_usSize + apStrings->m_usGrowBy);
        }
        apStrings->SetAt(m_usSize, element);
    }
    BSsprintf(apDest, 32, "0x%8X", pEntityA);
    char* v9 = NiGetViewerString("hkpEntityA", static_cast<const char*>(apDest));
    uint32_t v10 = apStrings->m_usSize;
    element = v9;
    if (v10 >= apStrings->m_usMaxSize) {
        apStrings->SetSize(v10 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v10, element);
    BSsprintf(apDest, 32, "0x%8X", pEntityB);
    char* v11 = NiGetViewerString("hkpEntityB", static_cast<const char*>(apDest));
    uint32_t v12 = apStrings->m_usSize;
    element = v11;
    if (v12 >= apStrings->m_usMaxSize) {
        apStrings->SetSize(v12 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v12, element);
    char* ViewerString_5 = NiGetViewerString("ePriority", ePriority);
    uint32_t v14 = apStrings->m_usSize;
    uint32_t m_usMaxSize = apStrings->m_usMaxSize;
    element = ViewerString_5;
    if (v14 >= m_usMaxSize) {
        apStrings->SetSize(v14 + apStrings->m_usGrowBy);
    }
    apStrings->SetAt(v14, element);
    v3->eMemContext = eMemContext;
}

void hkConstraintCinfo::SetEntityB(bhkEntity* apEntity) {
    hkpEntity* v2;
    if (apEntity) {
        v2 = static_cast<hkpEntity*>(apEntity->operator*());
    }
    else {
        v2 = nullptr;
    }
    pEntityB = v2;
}

void hkConstraintCinfo::SetEntityA(bhkEntity* apEntity) {
    hkpEntity* v2;
    if (apEntity) {
        v2 = static_cast<hkpEntity*>(apEntity->operator*());
    }
    else {
        v2 = nullptr;
    }
    pEntityA = v2;
}

void hkConstraintCinfo::SetEntities(bhkEntity* a1, bhkEntity* a2) {
    SetEntityA(static_cast<bhkRigidBody*>(a1));
    SetEntityB(static_cast<bhkRigidBody*>(a2));
}

hkHingeConstraintCinfo::hkHingeConstraintCinfo() {}

hkHingeConstraintCinfo::~hkHingeConstraintCinfo() {}

void hkHingeConstraintCinfo::SetInWorldSpace(
    const hkTransform& arBodyATrans,
    const hkTransform& arBodyBTrans,
    const hkVector4& arPivot,
    const hkVector4& arAxis
) {
    hkpHingeConstraintData* pData = static_cast<hkpHingeConstraintData*>(this->pData);
    if (pData) {
        pData->setInWorldSpace(arBodyATrans, arBodyBTrans, arPivot, arAxis);
    }
}
