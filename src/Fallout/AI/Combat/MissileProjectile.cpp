#include "Fallout/AI/Combat/MissileProjectile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMath.h"

#include "BSCore/BSSimpleList.h"
#include "BSCore/BSString.h"
#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkConstraintManager.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkHingeConstraint.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkSerializable.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSCloningProcess.h"
#include "BSMain/Setting.h"
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
    BGSProjectile* apProjectileBase,
    TESObjectREFR* apShooter,
    TESObjectWEAP* apFromWeapon,
    NiPoint3 aOrigin,
    float afZAngle,
    float afXAngle
)
  : Projectile(apProjectileBase, apShooter, apFromWeapon, aOrigin, afZAngle, afXAngle), eImpactResult(IR_NONE),
    bWaitingToInitialize3D(false), fStickTime(0.0f), fLocalRot(0.0f) {
    SetFormType(PMIS_ID);
}

bool MissileProjectile::IsPermanent() {
    return eImpactResult == IR_STICK || Projectile::IsPermanent();
}

void MissileProjectile::SaveGame(BGSSaveFormBuffer* apSaveGameBuffer) {
    Projectile::SaveGame(apSaveGameBuffer);
    apSaveGameBuffer->SaveDataEndian(reinterpret_cast<char*>(&eImpactResult), sizeof(eImpactResult), 0);
}

void MissileProjectile::LoadGame(BGSLoadFormBuffer* apLoadGameBuffer) {
    Projectile::LoadGame(apLoadGameBuffer);
    apLoadGameBuffer->LoadData(reinterpret_cast<char*>(&eImpactResult), sizeof(eImpactResult));
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
    NiPoint3 moveSlot;
    NiBound tracerBound;
    NiNode* pnode = Get3D();
    if (pnode && NiObject::IsKindOf_0(&NiNode::ms_RTTI, pnode)) {
        pnode->SetAppCulled(false);
        if (GetProjectileBase()->Data.fTracerChance > 0.0) {
            if (pnode->GetAtChecked(0)) {
                bool bappCulled = IsFlagSet(Projectile::TRACER);
                pnode->GetAt(0)->SetAppCulled(bappCulled);
            }
            if (pnode->GetAtChecked(1u)) {
                bool bappCulled = !IsFlagSet(Projectile::TRACER);
                pnode->GetAt(1u)->SetAppCulled(bappCulled);
            }
        }
        if (IsFlagSet(Projectile::TRACER)) {
            tracerBound = pnode->GetWorldBound();
            NiPoint3 fromCenter = pnode->GetTranslate() - tracerBound.m_kCenter;
            double dcenterDistance = fromCenter.Length();
            float ffakeDistance = tracerBound.m_fRadius + dcenterDistance;
            double dspeed = GetSpeed();
            float ffakeTime = ffakeDistance / dspeed;
            NiPoint3 fakeMove = *::new (&moveSlot) NiPoint3(0.0f, ffakeDistance, 0.0f);
            Move(ffakeTime, fakeMove);
            Sync3DWithReference(false, true);
            NiUpdateData updateData(0.0f, false, false);
            pnode->UpdateWorldData(updateData);
            double ddistanceMoved = fDistanceMoved;
            if (GetRange() < ddistanceMoved) {
                pnode->SetAppCulled(true);
            }
        }
    }
}

void MissileProjectile::UpdateProjectile(float afDelta) {
    NiMatrix3 rotProduct;
    NiMatrix3 mMatrix;
    NiPoint3 oldpos;
    NiPoint3 newpos;
    if (bWaitingToInitialize3D && Get3D()) {
        Initialize3D();
    }

    double ddistanceMoved = fDistanceMoved;
    if (GetRange() < ddistanceMoved && (ImpactList.IsEmpty() || bImpactProcessed)) {
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
            Projectile::ImpactData* pimpact = ImpactList.m_item;
            if (pimpact) {
                NiAVObject* ptargetAV = nullptr;
                if (pimpact->pTargetWorldObject) {
                    ptargetAV = GetAVObjectForCollidable(&pimpact->pTargetWorldObject->m_collidable);
                }
                NiAVObject* p3d = Get3D();
                if (ptargetAV && p3d) {
                    NiTransform newTrans = ptargetAV->GetWorldTransform() * kFollowOffset;
                    p3d->SetRotate(newTrans.m_Rotate);
                    SetRotation(&newTrans.m_Rotate);
                    p3d->SetTranslate(newTrans.m_Translate);
                    SetLocationOnReference(newTrans.m_Translate);
                }

                Sync3DWithReference(true, false);
            }
        }
        else if (
            !bImpactProcessed || (IsFlagSet(Projectile::HITSCAN_MODE) || eImpactResult == MissileProjectile::IR_IMPALE)
        ) {
            bool bfirstUpdate = GetAge() <= 0.0;
            bool breorient = false;
            if (pTarget) {
                NiPoint3 targetLocation = pTarget->GetWorldLocation();
                AimAtPoint(targetLocation);
            }
            if (!bfirstUpdate) {
                if (pTarget) {
                    const NiPoint3& kLocation = GetLocationOnReference();
                    NiPoint3 targetDelta = pTarget->GetWorldLocation() - kLocation;
                    double dtargetDistance = targetDelta.Length();
                    if (fProjectileReorientTracerMin.Float() < dtargetDistance) {
                        breorient = true;
                    }
                }
                if (bProjectileDebug.Bool()) {
                    oldpos = GetLocationOnReference();
                    Move(afDelta);
                    newpos = GetLocationOnReference();
                    NiColorA color(1.0f, 1.0f, 0.0f, 1.0f);
                    NiAVObject* pdebug = reinterpret_cast<NiAVObject*>(MakeLine(oldpos, color, newpos, color, true));
                    TES::pInstance->AddTempDebugObject(pdebug, fProjectileDebugDuration.Float());
                }
                else {
                    Move(afDelta);
                }
            }
            bool bimpactsProcessed = ImpactList.IsEmpty() || ImpactList.m_item->bProcessed;
            if (!bimpactsProcessed) {
                if (!IsFlagSet(Projectile::HITSCAN_MODE) ||
                    (ddistanceMoved = fDistanceMoved, GetRange() < ddistanceMoved)) {
                    SetLocationOnReference(ImpactList.m_item->kLocation);
                }
            }
            Sync3DWithReference(breorient, !bfirstUpdate);
            if (Get3D()) {
                Get3D()->UpdateWorldLoc();
            }
            if (GetProjectileBase()->GetRotates()) {
                NiMatrix3 mMatrixLocal;
                if (GetProjectileBase()->Data.fRotationX > 0.0) {
                    fLocalRot = GetProjectileBase()->Data.fRotationX * afDelta + fLocalRot;
                    mMatrixLocal.MakeXRotation(fLocalRot);
                }
                else if (GetProjectileBase()->Data.fRotationY > 0.0) {
                    fLocalRot = GetProjectileBase()->Data.fRotationY * afDelta + fLocalRot;
                    mMatrixLocal.MakeYRotation(fLocalRot);
                }
                else if (GetProjectileBase()->Data.fRotationZ > 0.0) {
                    fLocalRot = GetProjectileBase()->Data.fRotationZ * afDelta + fLocalRot;
                    mMatrixLocal.MakeZRotation(fLocalRot);
                }
                if (Get3D()) {
                    mMatrix = Get3D()->m_kLocal.m_Rotate;
                    mMatrix = *::new (&rotProduct) NiMatrix3(mMatrix * mMatrixLocal);
                    Get3D()->SetRotate(mMatrix);
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
        CheckExplosion(afDelta);
        double fAge = GetAge();
        if (fArrowAgeMax.Float() < fAge && eImpactResult != MissileProjectile::IR_STICK) {
            Kill();
        }
        ddistanceMoved = fDistanceMoved;
        if (GetRange() < ddistanceMoved && (ImpactList.IsEmpty() || bImpactProcessed)) {
            if (Get3D()) {
                Get3D()->SetAppCulled(true);
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
    bool bCanAdd = ImpactList.IsEmpty() || ImpactList.GetItem()->bProcessed;
    COL_LAYER eLayer;
    if (apTargetCollidable) {
        eLayer = CFilter(apTargetCollidable->getCollisionFilterInfo()).GetLayer();
    }
    else {
        eLayer = L_UNIDENTIFIED;
    }
    if (!bCanAdd) {
        return false;
    }
    if (!Projectile::IsValidImpact(apCollidee, apTargetCollidable, this, GetProjectileBase()) ||
        (eImpactResult == IR_IMPALE &&
         eLayer != L_STATIC &&
         eLayer != L_ANIMSTATIC &&
         eLayer != L_TREES &&
         eLayer != L_GROUND &&
         eLayer != L_TERRAIN &&
         eLayer != L_DEBRIS_LARGE) ||
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
            pimpact = ImpactList.m_pkNext ? ImpactList.m_pkNext->m_item : nullptr;
        }
        if (pimpact && pimpact->pCollidee && pimpact->pCollidee->IsActor()) {
            ProcessImpale();
        }
    }
    if (eImpactResult == MissileProjectile::IR_STICK) {
        ProcessStick();
    }
}

void MissileProjectile::DetermineImpactResult() {
    if (eImpactResult == MissileProjectile::IR_IMPALE) {
        eImpactResult = MissileProjectile::IR_STICK;
    }
    else if (!ImpactList.IsEmpty()) {
        eImpactResult = MissileProjectile::IR_DESTROY;
        if (Projectile::GetProjectileBase()->GetPinsLimbs()) {
            eImpactResult = MissileProjectile::IR_IMPALE;
        }
        if (Projectile::IsFlagSet(Projectile::HITSCAN_MODE) && !Projectile::IsFlagSet(Projectile::NO_3D)) {
            eImpactResult = MissileProjectile::IR_NONE;
        }
        if (!Projectile::IsFlagSet(Projectile::HITSCAN_MODE)) {
            BGSProjectile* pprojectileBase = Projectile::GetProjectileBase();
            if (pprojectileBase->IsFlagSet(BGSProjectile::BOUNCE_ON_IMPACT) || Projectile::GetSecondaryItemChange()) {
                eImpactResult = MissileProjectile::IR_BOUNCE;
            }
        }
    }
}

bool MissileProjectile::ProcessBounce() {
    if (Get3D() && !ImpactList.IsEmpty()) {
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
            NiUpdateData updateData(0.0f, false, false);
            p3d->Update(updateData);
        }
        bhkWorld::SetMotion(p3d, hkpMotion::MOTION_DYNAMIC, true, false, true);
        uint32_t uiGroup = bhkCollisionFilter::GetNewSystemGroup();
        bhkWorld::SetGroup(p3d, uiGroup, true, false);
        bhkWorld::EnableCollision(p3d, true, true, false);
        if (!pSaveLoadGameOLD->GetLoading()) {
            NiPoint3 force;
            NiMatrix3 invrot;
            NiPoint3 vton;
            NiPoint3 reflectedVelocity = NiPoint3::ZERO;
            NiPoint3 accum = -1.0f * GetMovementDirection();
            NiPoint3::UnitizeVector(accum);
            NiPoint3::UnitizeVector(psCollisionData->kNormal);
            float fnormalDotDirection = psCollisionData->kNormal.Dot(accum);
            vton = 2.0f * (fnormalDotDirection * psCollisionData->kNormal - accum);
            reflectedVelocity = accum + vton;
            float fbounceSpeed = GetProjectileBase()->GetSpeed() * fArrowBounceLinearSpeed.Float();
            reflectedVelocity *= fbounceSpeed;
            TESHavokUtilities::AddVelocity(p3d, reflectedVelocity, true);
            NiPoint3 angularVelocity = NiPoint3::ZERO;
            Get3D()->GetWorldTransform().m_Rotate.Inverse(invrot);
            force = invrot * psCollisionData->kNormal;
            double dabsY = NiAbs(force.y);
            if (NiAbs(force.z) < dabsY) {
                dabsY = NiAbs(force.y);
                force.z = NiSign(force.z) * dabsY;
            }
            force.y = 0.0f;
            NiPoint3::UnitizeVector(force);
            NiPoint3 angle(-force.z, 0.0f, -force.x);
            angularVelocity = p3d->GetWorldTransform().m_Rotate * angle;
            angularVelocity *= fArrowBounceLinearSpeed.Float();
            TESHavokUtilities::AddAngularVelocity(p3d, angularVelocity, true);
        }
    }

    return true;
}

void MissileProjectile::ProcessImpale() {
    CFilter actorFilter;
    CFilter controllerGroupFilter;
    alignas(16) CFilter bodyFilter;
    CFilter shooterFilter;
    CFilter controllerFilter;
    CFilter filter;
    NiMatrix3 shotRotProduct;
    NiMatrix3 shotRot;
    NiMatrix3 rotProduct;
    NiMatrix3 rotHeading;
    NiTransform rotSlot;
    NiTransform projectileWorld;
    NiTransform rot;
    Projectile::ImpactData* pimpact;
    NiNode* p3d;
    NiNode* p3dObject;
    static float fpinLimDepth = fCombatMissileImpaleDepth.Float();
    bool bloading = pSaveLoadGame->GetSaveGameLoading();
    if (this->ImpactList.IsEmpty()) {
        this->eImpactResult = MissileProjectile::IR_STICK;
        this->ProcessStick();
        return;
    }
    p3dObject = this->Get3D();
    p3d = p3dObject->IsNode();
    pimpact = this->ImpactList.m_item;
    this->fStickTime = 10.0f;
    if (this->eImpactResult == MissileProjectile::IR_STICK) {
        pimpact = this->ImpactList.m_pkNext ? this->ImpactList.m_pkNext->m_item : nullptr;
    }
    if (pimpact && (pimpact->pCollidee && pimpact->pCollidee->IsActor())) {
        if (!pimpact->bProcessed) {
            Actor* pactor = static_cast<Actor*>(pimpact->pCollidee);
            if (!pactor->GetDismembered(pimpact->eDamageLimb)) {
                this->Kill();
                return;
            }
            ExtraDismemberedLimbs* pextraLimbs;
            pextraLimbs = pactor->m_Extra.GetDismembermentExtra();
            char cLimbExploded;
            cLimbExploded = 0;
            for (uint32_t i = 0; i < pextraLimbs->DismemberedLimbs.iSize; ++i) {
                if (pextraLimbs->DismemberedLimbs.GetAt(static_cast<int32_t>(i))->cLimb ==
                        static_cast<int8_t>(static_cast<uint32_t>(pimpact->eDamageLimb)) &&
                    pextraLimbs->DismemberedLimbs.GetAt(static_cast<int32_t>(i))->bLimbExploded) {
                    cLimbExploded = 1;
                }
            }
            NiNode* ptarget3D;
            ptarget3D = pimpact->pCollidee ? pimpact->pCollidee->Get3D() : nullptr;
            NiAVObject* ptargetObject;
            ptargetObject = ptarget3D;
            if (ptarget3D && !cLimbExploded) {
                const char* plimbNodeName = pLimbEnumNames[pimpact->eDamageLimb];
                NiAVObject* pdismemberedLimb = ptargetObject ? ptargetObject->GetObjectByName(plimbNodeName) : nullptr;
                float f3dScale = pdismemberedLimb->m_kWorld.m_fScale;
                if (pdismemberedLimb) {
                    alignas(16) BSCloningProcess cloneProcess(1.0f);
                    cloneProcess.SetScale(1.0f);
                    pdismemberedLimb = static_cast<NiAVObject*>(pdismemberedLimb->Clone(cloneProcess));
                    pactor->RemoveDismemberedLimb(pimpact->eDamageLimb);
                    pdismemberedLimb->SetAppCulled(false);
                    pdismemberedLimb->SetScale(f3dScale);
                    NiNode* pdismemberedNode = pdismemberedLimb->IsNode();
                    if (pdismemberedNode) {
                        for (int32_t n = 0; n < pdismemberedNode->GetArrayCount(); ++n) {
                            NiAVObject* pobj = pdismemberedNode->GetAt(static_cast<uint32_t>(n));
                            if (pobj && Animation::FindSkinnedNode(pobj)) {
                                BipedAnim::AttachToSkeleton(pdismemberedNode, pobj, pdismemberedNode, false);
                            }
                        }
                    }
                    NiUpdateData updateData(0.0f, false, false);
                    pdismemberedLimb->Update(updateData);
                    bhkWorld* phw = this->GetParentCell()->GetbhkWorld();
                    if (phw) {
                        phw->AddObjects(pdismemberedLimb, true, false, 0, true);
                    }
                    pdismemberedLimb->SetTranslate(NiPoint3::ZERO);
                    pdismemberedLimb->SetRotate(NiMatrix3::IDENTITY);
                    if (pdismemberedNode && pdismemberedNode->GetAtChecked(0)) {
                        rot = pdismemberedNode->GetAt(0)->GetWorldTransform();
                        if (!bloading) {
                            this->SetLocationOnReference(pimpact->kLocation);
                        }
                        this->Sync3DWithReference(false, false);
                        alignas(16) NiUpdateData worldUpdateData(0.0f, false, false);
                        p3d->UpdateWorldData(worldUpdateData);
                        projectileWorld = p3d->GetWorldTransform();
                        NiTransform rotMult;
                        projectileWorld.InvertNonUniform(rotMult);
                        rot = *::new (&rotSlot) NiTransform(rotMult * rot);
                        rot.m_Translate.y = rot.m_Translate.y - fpinLimDepth;
                        pdismemberedNode->GetAt(0)->SetTranslate(rot.m_Translate);
                        pdismemberedNode->GetAt(0)->SetRotate(rot.m_Rotate);
                        float fScale = rot.m_fScale;
                        pdismemberedNode->GetAt(0)->SetScale(fScale);
                    }

                    p3d->AttachChild(pdismemberedLimb, 1);
                    pdismemberedLimb->SetName(*FixedStrings::QPinnedLimb());
                    bhkWorld::UpdatePosition(pdismemberedLimb, true, true);
                    bhkWorld::SetMotion(pdismemberedLimb, hkpMotion::MOTION_KEYFRAMED, true, true, true);
                    NiUpdateData attachUpdateData(0.0f, false, false);
                    p3d->Update(attachUpdateData);
                    char cPinned = 0;
                    if (pdismemberedLimb) {
                        alignas(16) NiMatrix3 rot2;
                        NiPoint3 direction;
                        NiAVObject* pconstrainedObject = nullptr;
                        this->GetParentCell()->GetbhkWorld();
                        bhkPickData pd;
                        NiPoint3 origin = this->GetLocationOnReference();
                        float fHeading = this->GetHeading(false);
                        rotHeading.MakeZRotation(fHeading);
                        float fLooking = this->GetLooking();
                        rot2.MakeXRotation(fLooking);
                        rotHeading = *::new (&rotProduct) NiMatrix3(rotHeading * rot2);
                        rotHeading.GetCol(1u, direction);
                        direction.Unitize();
                        pd.NiSetFrom(origin - direction * 512.0f);
                        pd.NiSetLength(direction * 1024.0f);
                        hkpAllRayHitCollector collector;
                        pd.SetAllCollector(&collector);
                        if (Projectile::PickSceneGraph(&pd, pdismemberedLimb, false, true) &&
                            pd.pAllCollector->getHits().m_size > 0) {
                            pconstrainedObject = GetAVObjectForCollidable(
                                pd.pAllCollector->getHits()[0].m_rootCollidable
                            );
                        }
                        if (pconstrainedObject) {
                            bhkCollisionObject* pcol = FindFirstCollisionObject(this->Get3D());
                            bhkRigidBody* pprojectileRB = pcol ? pcol->GetRigidBody() : nullptr;
                            bhkCollisionObject* plimbCol = FindFirstCollisionObject(pconstrainedObject);
                            bhkRigidBody* plimbRB = plimbCol ? plimbCol->GetRigidBody() : nullptr;
                            if (plimbRB && plimbRB != pprojectileRB) {
                                alignas(16) NiMatrix3 shotRot2;
                                NiPoint3 shotDirection;
                                bhkHingeConstraint* ph;
                                hkVector4 fixPointWorld;
                                hkVector4 axis;
                                NiPoint3 yAxisWorld;
                                alignas(16) hkHingeConstraintCinfo ci;
                                ci.hkHingeConstraintCinfo::CreateData();
                                ci.SetEntities(plimbRB, pprojectileRB);
                                hkTransform fixPointa((**plimbRB)->getTransform());
                                hkTransform fixPointb((**pprojectileRB)->getTransform());
                                fixPointa.m_rotation.renormalize();
                                fixPointb.m_rotation.renormalize();
                                this->Get3D()->GetWorldTransform().m_Rotate.GetCol(1u, yAxisWorld);
                                NI2HKNS(axis, yAxisWorld);
                                axis.normalize3();
                                projectileWorld = this->Get3D()->GetWorldTransform();
                                float fDepth = fpinLimDepth;
                                alignas(16) NiPoint3 offset = -NiPoint3::UNIT_Y * fDepth;
                                NiPoint3 hingePoint = projectileWorld * offset;
                                NI2HK(fixPointWorld, hingePoint);
                                ci.SetInWorldSpace(fixPointa, fixPointb, fixPointWorld, axis);
                                ph = new bhkHingeConstraint(&ci);
                                plimbRB->AddAction(ph);
                                bhkWorld* pworld = this->GetParentCell()->GetbhkWorld();
                                plimbRB->AddActConToWorld(pworld);
                                this->GetParentCell()->GetbhkWorld();
                                bhkPickData shotPick;
                                NiPoint3 shotOrigin = this->GetLocationOnReference();
                                fHeading = this->GetHeading(false);
                                shotRot.MakeZRotation(fHeading);
                                fLooking = this->GetLooking();
                                shotRot2.MakeXRotation(fLooking);
                                shotRot = *::new (&shotRotProduct) NiMatrix3(shotRot * shotRot2);
                                shotRot.GetCol(1u, shotDirection);
                                shotDirection.Unitize();
                                // The filter's one-field constructor is a trivial store.
                                filter.iFilter = 0;
                                if (this->GetCharController()) {
                                    bhkCharacterController* pcharController = this->GetCharController();
                                    filter.iFilter = pcharController->GetCollisionFilter(&controllerFilter)->iFilter;
                                }
                                else {
                                    filter.SetLayer(L_PROJECTILE);
                                    if (this->GetShooter() && this->GetShooter()->IsActor()) {
                                        Actor* pshooter = static_cast<Actor*>(this->GetShooter());
                                        // MobileObject::GetCollisionFilter returns its result pointer, &shooterFilter.
                                        pshooter->GetCollisionFilter(&shooterFilter.iFilter);
                                        uint32_t uiGroup = shooterFilter.GetGroup();
                                        filter.SetGroup(uiGroup);
                                    }
                                    else {
                                        uint32_t uiGroup = bhkCollisionFilter::GetNewSystemGroup();
                                        filter.SetGroup(uiGroup);
                                    }
                                }
                                double dRange = this->GetRange();
                                float fRemaining = dRange - this->fDistanceMoved;
                                alignas(16) NiPoint3 length = shotDirection * fRemaining;
                                shotPick.SetFilter(filter);
                                shotPick.NiSetFrom(shotOrigin);
                                shotPick.NiSetLength(length);
                                hkpAllRayHitCollector shotCollector;
                                shotPick.SetAllCollector(&shotCollector);
                                this->GetParentCell()->GetbhkWorld()->PickObject(shotPick);
                                for (int32_t i = 0; !cPinned && i < shotPick.pAllCollector->getHits().m_size; ++i) {
                                    COL_LAYER ebodyLayer;
                                    hkpWorldRayCastOutput point(shotPick.pAllCollector->getHits()[i]);
                                    NiAVObject* pav = GetAVObjectForCollidable(point.m_rootCollidable);
                                    hkpRigidBody* phkBody = hkpGetRigidBody(point.m_rootCollidable);
                                    bhkRigidBody* pbody = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(phkBody));
                                    TESObjectREFR* pref = TESObjectREFR::FindReferenceFor3D(pav);
                                    if (pbody) {
                                        ebodyLayer = pbody->GetCollisionFilter(&bodyFilter)->GetLayer();
                                    }
                                    else {
                                        ebodyLayer = L_UNIDENTIFIED;
                                    }
                                    if (pbody &&
                                        ((ebodyLayer == L_STATIC ||
                                          (ebodyLayer == L_ANIMSTATIC ||
                                           (ebodyLayer == L_TREES ||
                                            (ebodyLayer == L_GROUND ||
                                             (ebodyLayer == L_TERRAIN || ebodyLayer == L_DEBRIS_LARGE))))) &&
                                         (pbody->GetMotionType() == 5 || pbody->GetMotionType() == 4))) {
                                        cPinned = 1;
                                    }
                                    else if (pbody && (pref != this && (pref && pref != pimpact->pCollidee))) {
                                        break;
                                    }
                                }
                            }
                        }
                    }
                    pimpact->bProcessed = true;
                    if (cPinned) {
                        if (this->pTarget) {
                            this->pTarget = nullptr;
                        }
                        bhkWorld::UpdatePosition(pdismemberedLimb, true, true);
                        bhkWorld::SetMotion(pdismemberedLimb, hkpMotion::MOTION_KEYFRAMED, true, true, true);
                        if (this->GetCharController()) {
                            uint32_t uiGroup = this->GetCharController()
                                                   ->GetCollisionFilter(&controllerGroupFilter)
                                                   ->GetGroup();
                            bhkWorld::SetGroup(pdismemberedLimb, uiGroup, true, true);
                            if (!bloading) {
                                hkVector4 hkpos;
                                this->SetLocationOnReference(pimpact->kLocation);
                                NI2HK(hkpos, pimpact->kLocation);
                                this->GetCharController()->SetPosition(hkpos);
                            }
                            this->Sync3DWithReference(false, false);
                        }
                        if (!bloading) {
                            this->bImpactProcessed = false;
                        }
                    }
                    else {
                        if (!bloading) {
                            this->SetLocationOnReference(pimpact->kLocation);
                        }
                        this->Sync3DWithReference(false, false);
                        this->SetFlag(Projectile::NO_CHARCONTROLLER, true);
                        this->RemoveCharController();
                        bhkWorld::UpdatePosition(pdismemberedLimb, true, true);
                        // MobileObject::GetCollisionFilter returns its result pointer, &actorFilter.
                        pactor->GetCollisionFilter(&actorFilter.iFilter);
                        uint32_t uiGroup = actorFilter.GetGroup();
                        bhkWorld::SetGroup(p3d, uiGroup, true, true);
                        bhkWorld::SetMotion(p3d, hkpMotion::MOTION_DYNAMIC, true, true, true);
                        bhkWorld::EnableCollision(p3d, true, true, false);
                        float fSpeed = this->GetSpeed();
                        alignas(16) NiPoint3 velocity = this->GetMovementDirection() * fSpeed / 2.0f;
                        TESHavokUtilities::AddVelocity(p3d, velocity, true);
                        MissileProjectile* plastProjectile = static_cast<MissileProjectile*>(pVATS.pLastProjectile);
                        if (plastProjectile == this) {
                            pVATS.SetLastProjectile(nullptr);
                        }
                    }
                }
                else {
                    this->bImpactProcessed = false;
                }
            }
            else {
                this->Kill();
            }
        }
    }
    else {
        this->eImpactResult = MissileProjectile::IR_STICK;
        this->ProcessStick();
    }
}

void MissileProjectile::ProcessStick() {
    NiTransform kDest;
    CFilter targetFilter;
    CFilter filter;
    static float fstickDepth = fCombatMissileStickDepth.Float();
    bool bloading = pSaveLoadGame->GetSaveGameLoading();
    if (Get3D() && !ImpactList.IsEmpty()) {
        if (pVATS.pLastProjectile == this) {
            pVATS.SetLastProjectile(nullptr);
        }
        fStickTime = 10.0f;
        Projectile::ImpactData* pimpact = ImpactList.m_item;
        if (pimpact && pimpact->pTargetWorldObject) {
            COL_LAYER ebodyLayer;
            NiAVObject* pav = Get3D();
            NiAVObject* ptargetAV = GetAVObjectForCollidable(pimpact->pTargetWorldObject->getCollidable());
            bhkRigidBody* ptargetRB = static_cast<bhkRigidBody*>(bhkEntity::Getbhk(pimpact->pTargetWorldObject));
            if (ptargetRB) {
                ebodyLayer = ptargetRB->GetCollisionFilter(&targetFilter)->GetLayer();
            }
            else {
                ebodyLayer = L_UNIDENTIFIED;
            }
            if (!ptargetRB || (ebodyLayer == L_DEBRIS_SMALL || (pimpact->pCollidee && pimpact->pCollidee->IsActor()))) {
                ProcessBounce();
            }
            else {
                NiAVObject* pdismemberedLimb;
                SetFlag(Projectile::NO_CHARCONTROLLER, true);
                RemoveCharController();
                bhkCollisionObject* pcol = FindFirstCollisionObject(pav);
                bhkRigidBody* pprojectileRB = pcol ? pcol->GetRigidBody() : nullptr;
                if (pprojectileRB) {
                    pprojectileRB->GetCollisionFilter(&filter);
                    filter.SetLayer(L_NONCOLLIDABLE);
                    pprojectileRB->SetCollisionFilter(filter);
                }
                if (!bloading) {
                    SetLocationOnReference(pimpact->kLocation + GetMovementDirection() * fstickDepth);
                }
                Sync3DWithReference(false, false);
                bhkWorld::UpdatePosition(pav, true, true);
                NiUpdateData updateData(0.0f, false, false);
                pav->Update(updateData);
                if (ptargetRB->GetMotionType() == 5) {
                    bhkWorld::SetMotion(pav, hkpMotion::MOTION_FIXED, true, false, true);
                }
                else {
                    bhkWorld::SetMotion(pav, hkpMotion::MOTION_KEYFRAMED, true, false, true);
                    if (!IsFlagSet(Projectile::POST_PROCESS)) {
                        SetFlag(Projectile::POST_PROCESS, true);
                        // The list's address is tested for null, which it never is.
                        BSSimpleList<void*>* plist = &ReferenceProcessLists.ProjectilePostProcessList;
                        if (plist) {
                            plist->AddHead(this);
                        }
                    }
                    ptargetRB->SetFlags(bhkWorldObject::STICKING_TARGET, true);
                    if (!bloading && ptargetAV) {
                        NiTransform targetInvert;
                        ptargetAV->GetWorldTransform().Invert(targetInvert);
                        kFollowOffset = *::new (&kDest) NiTransform(targetInvert * pav->GetWorldTransform());
                    }
                }
                if (pav) {
                    NiFixedString* pPinnedLimb = FixedStrings::QPinnedLimb();
                    pdismemberedLimb = pav->GetObjectByName(*pPinnedLimb);
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
