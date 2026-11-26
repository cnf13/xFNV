#include "Fallout/Magic/MagicProjectile.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkWorld.h"
#include "BSMain/BSCloningProcess.h"
#include "BSMain/BSXFlags.h"
#include "BSMain/Setting.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/BSShaderUtil.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/HighProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessArray.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Interface/FaderManager.h"
#include "Fallout/Magic/MagicBallProjectile.h"
#include "Fallout/Magic/MagicBoltProjectile.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicFogProjectile.h"
#include "Fallout/Magic/MagicLight.h"
#include "Fallout/Magic/MagicSprayProjectile.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Havok/Source/Common/Base/Math/Vector/hkSimdReal.h"
#include "Havok/Source/Common/Base/Math/Vector/hkVector4.h"
#include "Havok/Source/Physics/Collide/Query/Collector/RayCollector/hkpAllRayHitCollector.h"
#include "Havok/Source/Physics/Dynamics/Entity/hkpRigidBody.h"

MagicProjectile::MagicProjectile()
  : MobileObject(), fSpeed(0.0f), fDistanceMoved(0.0f), fAge(0.0f), pCaster(nullptr), pSpell(nullptr),
    pDominantEffect(nullptr), pAVEffect(nullptr) {
    SetStillLoading(true);
    pCurrentProcess = new HighProcess();
    SetObjectReference(GetObjectReference());
}

MagicProjectile::MagicProjectile(
    MagicCaster* apCaster,
    MagicItem* apSpell,
    EffectItem* apEffectItem,
    EffectSetting* apAVEffect,
    NiPoint3 aLocation,
    NiPoint3 aRotation
)
  : MobileObject(), fSpeed(0.0f), fDistanceMoved(0.0f), fAge(0.0f), pCaster(apCaster), pSpell(apSpell),
    pDominantEffect(apEffectItem), pAVEffect(apAVEffect), fLastUpdate(ReferenceProcessLists.GetSystemTimeClock()) {
    pCurrentProcess = new HighProcess();
    SetLocationOnReference(aLocation);
    SetAngleOnReference(aRotation);
    TESObjectREFR* pStatsObject = apCaster->GetCasterStatsObject();
    if (pStatsObject) {
        MobileObject::SetParentCell(pStatsObject->GetParentCell());
    }
    SetObjectReference(GetDummyObjectReference());
}

MagicProjectile::~MagicProjectile() {
    if (pCurrentProcess) {
        ReferenceProcessLists.RemoveReference(this, pCurrentProcess->eLevel);
    }
    if (pAVEffect && pAVEffect->GetModelLength()) {
        pModelLoader->ReleaseModel(pAVEffect->GetModel(), false, 1u);
    }

    if (!pDataHandler->GetClearingData()) {
        CleanUpOtherMobileObjects(false);
    }
}

uint16_t MagicProjectile::GetSaveSize(uint32_t aiFlags) {
    uint16_t usSize = 0;
    usSize += MobileObject::GetSaveSize(aiFlags);
    usSize += 4;
    usSize += 4;
    usSize += 4;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x48) {
        usSize += 4;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x64) {
        usSize += 4;
    }
    return usSize;
}

void MagicProjectile::SaveGame(uint32_t aiFlags) {
    MobileObject::SaveGame(aiFlags);
    uint32_t uiCasterID = pCaster->GetMagicCasterFormID();
    SaveNumericID(&uiCasterID, 4);
    uint32_t uiSpellID = pSpell->GetMagicItemFormID();
    SaveNumericID(&uiSpellID, 4);
    SaveGameDataOLD(&fLastUpdate, sizeof(fLastUpdate));
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x48) {
        SaveGameDataOLD(&fAge, sizeof(fAge));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x64) {
        SaveGameDataOLD(&fDistanceMoved, sizeof(fDistanceMoved));
    }
}

// The pointer fields hold the raw numeric IDs (integer-to-pointer casts) until InitLoadGame resolves them; the
// float fields are read as the untyped byte buffer LoadGameDataOLD takes.
void MagicProjectile::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    MobileObject::LoadGame(aiFlags, aiCurrentFlags);
    uint32_t uiCasterID;
    LoadNumericID(&uiCasterID, 4);
    pCaster = reinterpret_cast<MagicCaster*>(static_cast<uintptr_t>(uiCasterID));
    uint32_t uiSpellID;
    LoadNumericID(&uiSpellID, 4);
    pSpell = reinterpret_cast<MagicItem*>(static_cast<uintptr_t>(uiSpellID));
    LoadGameDataOLD(reinterpret_cast<double*>(&fLastUpdate), sizeof(fLastUpdate));
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x48) {
        LoadGameDataOLD(reinterpret_cast<double*>(&fAge), sizeof(fAge));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x64) {
        LoadGameDataOLD(reinterpret_cast<double*>(&fDistanceMoved), sizeof(fDistanceMoved));
    }
}

void MagicProjectile::InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
    pCaster = MagicCaster::GetMagicCasterByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCaster)));
    pSpell = MagicItem::GetMagicItemByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSpell)));
    // Called through the object, not a pointer upcast: pSpell is deliberately not null-checked.
    pDominantEffect = (*pSpell).GetCostliestEffect(MagicSystem::TARGET, false);
    pAVEffect = pSpell->GetAVEffect(MagicSystem::SELF);
    MobileObject* pThis = this;
    pThis->Load3D(false);
    MobileObject::InitLoadGame(aiFlags, aiOldFlags);
    ReferenceProcessLists.RemoveReference(this, GetCurrentProcessType());
    ReferenceProcessLists.AddReference(this, GetCurrentProcessType(), false, false, nullptr);
    if (!pCaster || !pSpell) {
        pSaveLoadGameOLD->AddFormToDeferredDeletionsList(this);
    }
}

NiAVObject* MagicProjectile::Load3D(bool abBackgroundLoading) {
    NiNode* pObject = nullptr;
    if (GetDelete() || GetDisabled()) {
        return nullptr;
    }
    if (!Get3D()) {
        pObject = static_cast<NiNode*>(MagicProjectile::Clone3D_0(GetAVEffect()));
        if (pObject) {
            if (GetObjectReference()->GetNoCollision()) {
                SetNoCollision(true);
            }
            Set3DSimple(pObject);
            pObject->SetTranslate(GetLocationOnReference());
            pObject->SetRotate(GetOrientation());
            bhkNiCollisionObject::ResetSim(pObject, true);
            BSXFlags* pBSXFlags = static_cast<BSXFlags*>(pObject->GetExtraData(BSXFlags::Tag()));
            if (pBSXFlags && pBSXFlags->QAddon() && !GetFireOff()) {
                TESObjectREFR::AddAddonNodes(pObject);
            }

            static_cast<BSFadeNode*>(pObject)->BSFadeNode::SetUserData(this);
            InitAnimation();
            float fScale = GetScale();
            pObject->SetScale(fScale);
        }
    }
    return pObject;
}

bool MagicProjectile::IsProjectile() {
    return true;
}

void MagicProjectile::Process(float afDelta) {
    float fSystemTimeClock = ReferenceProcessLists.GetSystemTimeClock();
    float fElapsed = fSystemTimeClock - fLastUpdate;
    if (fSystemTimeClock < static_cast<double>(fLastUpdate) || fLastUpdate < 0.0) {
        fElapsed = 0.0f;
        if (fSystemTimeClock > 0.0 && fSystemTimeClock < 0.30000001192092896) {
            fElapsed = fSystemTimeClock;
        }
    }
    fLastUpdate = fSystemTimeClock;
    if (!(pFaderManager->GetFaderAlpha(FADER_TO_BLACK) > 0.0)) {
        fAge = fAge + fElapsed;
        Update(fElapsed);
    }
}

bool MagicProjectile::ChangeProcessLevel() {
    TESObjectCELL* pParentCell = GetParentCell();
    if (!TES::pInstance->IsCellLoaded(pParentCell, true)) {
        if (this) {
            delete this;
        }
        return false;
    }
    return true;
}

bool MagicProjectile::IsTransient() {
    return true;
}

void MagicProjectile::SetDisplayGeometry(bool abTF) {
    bhkCharacterController* pCharController = GetCharController();
    if (pCharController) {
        if (abTF) {
            pCharController->BuildDisplayGeometry(nullptr);
            BSShaderUtil::RecursiveSetPropertyWireframe(pCharController->GetNiObject(), 1);
            BuildAreaEffectGeometry();
        }
        else {
            pCharController->RemoveCollisionDebug();
            KillAreaEffectGeometry();
        }
    }
}

bool MagicProjectile::IsActive() {
    return !TESForm::GetDelete();
}

bool MagicProjectile::CheckDeletedCaster(MagicCaster* apCaster) {
    int8_t cCleared = 0;
    if (apCaster == pCaster) {
        pCaster = nullptr;
        cCleared = 1;
        SetDelete(true);
    }
    return static_cast<bool>(cCleared);
}

bool MagicProjectile::CheckAddTarget(MagicTarget* apSpellTarget, ActiveEffect* apEffect) {
    return true;
}

bool MagicProjectile::PostProcess() {
    return true;
}

void MagicProjectile::ApplyCollisionForce(NiPoint3 aContactPoint, NiPoint3 aDirection, bhkRigidBody* apTargetBody) {
    if (!apTargetBody) {
        return;
    }
    ahkpWorld* pWorld = static_cast<ahkpWorld*>(apTargetBody->GethkWorldRw());
    if (!pWorld) {
        return;
    }
    float fImpulse = 1750.0f;
    CFilter kFilter;
    apTargetBody->GetCollisionFilter(&kFilter);
    switch (kFilter.GetLayer()) {
        case L_TRAP: fImpulse = fImpulse * fProjectileKnockMultTrap.Float(); break;
        case L_BIPED:
        case L_DEADBIP: fImpulse = fImpulse * fProjectileKnockMultBiped.Float(); break;
        case L_ANIMSTATIC:
        case L_PROPS: fImpulse = fImpulse * fProjectileKnockMultProp.Float(); break;
        default: fImpulse = fImpulse * fProjectileKnockMultClutter.Float(); break;
    }
    float fMass = apTargetBody->GetMass();
    if (fProjectileKnockMinMass.Float() > fMass) {
        fImpulse = fMass / fProjectileKnockMinMass.Float() * fImpulse;
    }
    fImpulse = fImpulse * fProjectileCollisionImpulseScale.Float();
    hkVector4 kImpulse;
    NI2HK(kImpulse, aDirection);
    hkVector4 kPoint;
    NI2HK(kPoint, aContactPoint);
    kImpulse.normalize3();
    alignas(16) hkSimdReal kScale(fImpulse);
    kImpulse.mul4(kScale);
    pWorld->markForWrite();
    static_cast<hkpRigidBody*>(**apTargetBody)->applyPointImpulse(kImpulse, kPoint);
    pWorld->unmarkForWrite();
}

void MagicProjectile::CheckShotBehindWall() {
    NiAVObject* p3D = Get3D();
    if ((GetCaster() && GetCaster()->GetCasterStatsObject() != pPlayer) || !p3D) {
        return;
    }
    // Constructed in place and never destroyed; there is no destructor call.
    alignas(bhkPickData) uint8_t kPickMemory[sizeof(bhkPickData)];
    bhkPickData* pPick = ::new (kPickMemory) bhkPickData;
    NiPoint3 kFrom;
    NiPoint3 kProjectile;
    NiPoint3 kDirection;
    kProjectile = GetLocationOnReference();
    kFrom = pPlayer->GetLocationOnReference();
    // The eye height is kept as a double and the float z is added to the double product.
    double dEyeHeight = pPlayer->GetEyeHeight();
    kFrom.z = static_cast<float>(pPlayer->GetScale() * dEyeHeight + kFrom.z);
    kDirection = kProjectile - kFrom;
    bhkCharacterController* pController = static_cast<bhkCharacterController*>(GetCharController());
    CFilter kFilter(0);
    uint32_t uiFilter;
    CFilter kControllerFilter;
    if (pController) {
        kFilter.iFilter = pController->GetCollisionFilter(&kControllerFilter)->iFilter;
    }
    else {
        kFilter.iFilter = *pPlayer->GetCollisionFilter(&uiFilter);
    }
    pPick->SetFilter(kFilter);
    pPick->NiSetFrom(kFrom);
    pPick->NiSetTo(kFrom + kDirection);
    alignas(hkpAllRayHitCollector) uint8_t kCollectorMemory[sizeof(hkpAllRayHitCollector)];
    hkpAllRayHitCollector* pCollector = ::new (kCollectorMemory) hkpAllRayHitCollector;
    pPick->SetAllCollector(pCollector);
    bool bDone = false;
    if (pPlayer->GetParentCell()) {
        bhkWorld* pWorld = pPlayer->GetParentCell()->GetbhkWorld();
        if (pWorld->PickObject(*pPick)) {
            bool bSearching = true;
            for (int32_t i = 0; bSearching; ++i) {
                if (i >= pCollector->getHits().getSize() || bDone) {
                    break;
                }
                // Built with the copy constructor; it is deliberately never destroyed.
                alignas(hkpWorldRayCastOutput) uint8_t kHitMemory[sizeof(hkpWorldRayCastOutput)];
                hkpWorldRayCastOutput& kHit = *::new (kHitMemory) hkpWorldRayCastOutput(pCollector->getHits()[i]);
                NiAVObject* pObject = GetAVObjectForCollidable(kHit.m_rootCollidable);
                if (!pObject) {
                    continue;
                }
                bhkEntity* pEntity = bhkEntity::Getbhk(hkpGetRigidBody(kHit.m_rootCollidable));
                TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(pObject);
                if (pRef != pPlayer && pRef != this && !pRef->IsActor() && pEntity) {
                    Collide(kFrom + kDirection * kHit.m_hitFraction, static_cast<bhkRigidBody*>(pEntity), pRef, false);
                    bSearching = false;
                }
            }
        }
    }
    pCollector->~hkpAllRayHitCollector();
}

void MagicProjectile::KillAreaEffectGeometry() {
    NiFixedString kName;
    int8_t cConstructed = 0;
    NiNode* pAreaDisplay = nullptr;
    if (Get3D()) {
        NiNode* p3D = Get3D();
        ::new (static_cast<void*>(&kName)) NiFixedString(const_cast<char*>("MagicAreaDisplay"));
        cConstructed = 1;
        pAreaDisplay = static_cast<NiNode*>(p3D->GetObjectByName(kName));
    }
    if ((cConstructed & 1) != 0) {}
    if (pAreaDisplay && pAreaDisplay->NiAVObject::GetParent()) {
        NiNode* pParent = pAreaDisplay->NiAVObject::GetParent();
        pParent->DetachChild(pAreaDisplay);
    }
}

NiAVObject* MagicProjectile::Clone3D(const char* apModelPath) {
    NiNode* pClone = nullptr;
    NiNode* pFile = nullptr;
    if (apModelPath && *apModelPath) {
        pFile = pModelLoader->LoadFile(apModelPath, LOD_MULT_NONE, true, false, false, false);
    }

    if (!pFile) {
        return pClone;
    }
    BSCloningProcess kCloning(1.0);
    pClone = static_cast<NiNode*>(pFile->Clone(kCloning));
    if (pClone) {
        pClone->SetScale(1.0);
        pClone->SetAppCulled(false);
        pClone->SetRotate(NiMatrix3::IDENTITY);
        pClone->SetTranslate(NiPoint3::ZERO);
        if (pClone->NiAVObject::GetParent()) {
            pClone->DetachParent();
        }
        pClone->UpdateProperties();
    }
    return pClone;
}

// The dummy base object a projectile is given.
TESBoundObject* MagicProjectile::GetDummyObjectReference() {
    if (!MagicProjectile::pDummyObjectRef) {
        TESAmmo* pAmmoObject = new TESAmmo;
        pAmmoObject->SetModel("marker_error.nif");
        MagicProjectile::pDummyObjectRef = pAmmoObject;
        pDataHandler->AddFormToDataHandler(pAmmoObject);
    }
    return MagicProjectile::pDummyObjectRef;
}

void MagicProjectile::Launch(
    MagicCaster* apCaster,
    NiAVObject* apData3D,
    MagicLight* apLight,
    MagicItem* apSpell,
    EffectItem* apDominantEffect,
    EffectSetting* apAVEffect,
    NiPoint3 aOrigin,
    NiPoint3 aRotation
) {
    EffectSetting* pavEffect = apAVEffect ? apAVEffect : apDominantEffect->pEffectSetting;
    MagicProjectile* pProjectile = nullptr;
    switch (apAVEffect->GetProjectileType()) {
        case MagicSystem::BALL: {
            pProjectile = new MagicBallProjectile(
                apCaster,
                apSpell,
                apDominantEffect,
                pavEffect,
                aOrigin,
                aRotation,
                apData3D,
                apLight
            );
            break;
        }
        case MagicSystem::BOLT: {
            pProjectile = (MagicProjectile*)new MagicBoltProjectile(
                apCaster,
                apSpell,
                apDominantEffect,
                pavEffect,
                aOrigin,
                aRotation
            );
            break;
        }
        case MagicSystem::SPRAY: {
            pProjectile = (MagicProjectile*)new MagicSprayProjectile(
                apCaster,
                apSpell,
                apDominantEffect,
                pavEffect,
                aOrigin,
                aRotation
            );
            break;
        }
        case MagicSystem::FOG: {
            pProjectile = (MagicProjectile*)new MagicFogProjectile(
                apCaster,
                apSpell,
                apDominantEffect,
                pavEffect,
                aOrigin,
                aRotation,
                apData3D,
                apLight
            );
            break;
        }
        default: {
            break;
        }
    }
    if (pProjectile) {
        ReferenceProcessLists.AddReference(pProjectile, PROCESS_TYPE_HIGH, false, false, nullptr);
    }
}

NiAVObject* MagicProjectile::Clone3D_0(EffectSetting* apAVEffect) {
    return MagicProjectile::Clone3D(apAVEffect->GetModel());
}
