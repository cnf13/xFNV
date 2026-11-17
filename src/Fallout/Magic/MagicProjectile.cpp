#include "Fallout/Magic/MagicProjectile.h"

#include "BSCore/MemoryManager.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkEntity.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkPickData.h"
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
    pDominantEffect(apEffectItem), pAVEffect(apAVEffect), fLastUpdate((&ReferenceProcessLists)->GetSystemTimeClock()) {
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
    if (pAVEffect) {
        if (pAVEffect->GetModelLength()) {
            pModelLoader->ReleaseModel(pAVEffect->GetModel(), false, 1u);
        }
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
    SaveGameDataOLD(&fLastUpdate, 4);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x48) {
        SaveGameDataOLD(&fAge, 4);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x64) {
        SaveGameDataOLD(&fDistanceMoved, 4);
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
    LoadGameDataOLD(reinterpret_cast<double*>(&fLastUpdate), 4);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x48) {
        LoadGameDataOLD(reinterpret_cast<double*>(&fAge), 4);
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x64) {
        LoadGameDataOLD(reinterpret_cast<double*>(&fDistanceMoved), 4);
    }
}

void MagicProjectile::InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
    pCaster = MagicCaster::GetMagicCasterByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCaster)));
    pSpell = MagicItem::GetMagicItemByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSpell)));
    // Called through the object, not a pointer upcast: pSpell is deliberately not null-checked, as the game does.
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
    NiNode* apObject = nullptr;
    if (GetDelete() || GetDisabled()) {
        return nullptr;
    }
    if (!Get3D()) {
        EffectSetting* CriticalQueuedRefCount = GetAVEffect();
        apObject = static_cast<NiNode*>(MagicProjectile::Clone3D_0(CriticalQueuedRefCount));
        if (apObject) {
            TESForm* ObjectReference = GetObjectReference();
            if (ObjectReference->GetNoCollision()) {
                SetNoCollision(true);
            }
            Set3DSimple(apObject);
            const NiPoint3* v5 = &GetLocationOnReference();
            apObject->SetTranslate(*v5);
            apObject->SetRotate(GetOrientation());
            bhkNiCollisionObject::ResetSim(apObject, true);
            const NiFixedString* v7 = &BSXFlags::Tag();
            BSXFlags* ExtraData = static_cast<BSXFlags*>(apObject->GetExtraData(*v7));
            if (ExtraData) {
                if (ExtraData->QAddon() && !GetFireOff()) {
                    TESObjectREFR::AddAddonNodes(apObject);
                }
            }
            static_cast<BSFadeNode*>(apObject)->BSFadeNode::SetUserData(this);
            InitAnimation();
            float X = GetScale();
            apObject->SetScale(X);
        }
    }
    return apObject;
}

bool MagicProjectile::IsProjectile() {
    return true;
}

void MagicProjectile::Process(float afDelta) {
    float systemTimeClock = (&ReferenceProcessLists)->GetSystemTimeClock();
    float v4 = systemTimeClock - fLastUpdate;
    if ((systemTimeClock < ((double)fLastUpdate)) || (fLastUpdate < 0.0)) {
        v4 = (float)0.0;
        if ((systemTimeClock > 0.0) && (systemTimeClock < 0.30000001192092896)) {
            v4 = systemTimeClock;
        }
    }
    fLastUpdate = systemTimeClock;
    if (!(pFaderManager->GetFaderAlpha(FADER_TO_BLACK) > 0.0)) {
        fAge = (fAge + v4);
        Update(v4);
    }
}

bool MagicProjectile::ChangeProcessLevel() {
    bool v4 = true;
    TESObjectCELL* ParentCell = GetParentCell();
    if (!TES::pInstance->IsCellLoaded(ParentCell, true)) {
        if (this) {
            delete this;
        }
        return false;
    }
    return v4;
}

bool MagicProjectile::IsTransient() {
    return true;
}

void MagicProjectile::SetDisplayGeometry(bool abTF) {
    bhkCharacterController* CharController = GetCharController();
    if (CharController) {
        if (abTF) {
            CharController->BuildDisplayGeometry(nullptr);
            NiAVObject* NiObject_ = CharController->GetNiObject();
            BSShaderUtil::RecursiveSetPropertyWireframe(NiObject_, 1);
            BuildAreaEffectGeometry();
        }
        else {
            CharController->RemoveCollisionDebug();
            KillAreaEffectGeometry();
        }
    }
}

bool MagicProjectile::IsActive() {
    return (bool)((uint8_t)(!TESForm::GetDelete()));
}

bool MagicProjectile::CheckDeletedCaster(MagicCaster* apCaster) {
    int8_t v3 = 0;
    if (apCaster == pCaster) {
        pCaster = nullptr;
        v3 = 1;
        SetDelete(true);
    }
    return (bool)v3;
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
    alignas(16) hkSimdReal kScale;
    kScale.~hkSimdReal();
    ::new (&kScale) hkSimdReal(fImpulse);
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
    // Constructed in place and never destroyed; there is no destructor call, as the game does.
    alignas(bhkPickData) uint8_t kPickMemory[sizeof(bhkPickData)];
    bhkPickData* pPick = ::new (kPickMemory) bhkPickData;
    NiPoint3 kFrom;
    NiPoint3 kProjectile;
    NiPoint3 kDirection;
    kProjectile = GetLocationOnReference();
    kFrom = pPlayer->GetLocationOnReference();
    // The eye height is kept as a double and the float z is added to the double product, as the game does.
    double dEyeHeight = pPlayer->GetEyeHeight();
    kFrom.z = (float)(pPlayer->GetScale() * dEyeHeight + kFrom.z);
    kDirection = kProjectile - kFrom;
    bhkCharacterController* pController = static_cast<bhkCharacterController*>(GetCharController());
    CFilter kFilter;
    kFilter.~CFilter();
    ::new (&kFilter) CFilter(0);
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
                // Built with the copy constructor; it is deliberately never destroyed, as the game does.
                alignas(hkpWorldRayCastOutput) uint8_t kHitMemory[sizeof(hkpWorldRayCastOutput)];
                hkpWorldRayCastOutput& kHit =
                    *::new (kHitMemory)
                        hkpWorldRayCastOutput(pCollector->getHits().operator[]<hkpWorldRayCastOutput>(i));
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
    NiNode* parent;
    NiNode* v2;
    NiFixedString* v3;
    NiNode* v4;
    NiFixedString apString;
    int32_t v9;
    int8_t v6 = 0;
    if (Get3D()) {
        v4 = Get3D();
        v3 = ::new (static_cast<void*>(&apString)) NiFixedString(const_cast<char*>("MagicAreaDisplay"));
        v9 = 0;
        v6 = 1;
        v2 = static_cast<NiNode*>(v4->GetObjectByName(*v3));
    }
    else {
        v2 = nullptr;
    }
    NiNode* v8 = v2;
    v9 = (-1);
    if ((v6 & 1) != 0) {}
    if (v8) {
        if (v8->NiAVObject::GetParent()) {
            parent = v8->NiAVObject::GetParent();
            parent->DetachChild(v8);
        }
    }
}

NiAVObject* MagicProjectile::Clone3D(const char* apModelPath) {
    BSCloningProcess a2;
    NiNode* v5 = nullptr;
    NiNode* file = nullptr;
    if (apModelPath) {
        if (*apModelPath) {
            file = pModelLoader->LoadFile(apModelPath, LOD_MULT_NONE, true, false, false, false);
        }
    }
    if (!file) {
        return v5;
    }
    float v3 = (float)1.0;
    a2.~BSCloningProcess();
    ::new (&a2) BSCloningProcess(1.0);
    int32_t v6 = 0;
    v5 = static_cast<NiNode*>(file->Clone(a2));
    if (v5) {
        v5->SetScale(1.0);
        v5->SetAppCulled(false);
        v5->SetRotate(NiMatrix3_IDENTITY);
        v5->SetTranslate(NiPoint3_ZERO);
        if (v5->NiAVObject::GetParent()) {
            v5->DetachParent();
        }
        v5->UpdateProperties();
    }
    v6 = (-1);
    return v5;
}

// The dummy base object a projectile is given.
TESBoundObject* MagicProjectile::GetDummyObjectReference() {
    if (!MagicProjectile::pDummyObjectRef) {
        TESAmmo* pammoObject;
        TESAmmo* apMemory = static_cast<TESAmmo*>(ourMalloc(sizeof(TESAmmo)));
        if (apMemory) {
            pammoObject = ::new (apMemory) TESAmmo;
        }
        else {
            pammoObject = nullptr;
        }
        pammoObject->SetModel("marker_error.nif");
        MagicProjectile::pDummyObjectRef = pammoObject;
        pDataHandler->AddFormToDataHandler(pammoObject);
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
    EffectSetting* pavEffect;
    MagicProjectile* pprojectile_2 = nullptr;
    if (apAVEffect) {
        pavEffect = apAVEffect;
    }
    else {
        pavEffect = apDominantEffect->pEffectSetting;
    }
    switch (apAVEffect->GetProjectileType()) {
        case MagicSystem::BALL: {
            MagicProjectile* pprojectile;
            MagicBallProjectile* a1 = static_cast<MagicBallProjectile*>(ourMalloc(sizeof(MagicBallProjectile)));
            if (a1) {
                pprojectile = ::new (a1) MagicBallProjectile(
                    apCaster, apSpell, apDominantEffect, pavEffect, aOrigin, aRotation, apData3D, apLight
                );
            }
            else {
                pprojectile = nullptr;
            }
            pprojectile_2 = pprojectile;
            break;
        }
        case MagicSystem::BOLT: {
            MagicProjectile* pprojectile_3;
            MagicBoltProjectile* v15 = static_cast<MagicBoltProjectile*>(ourMalloc(sizeof(MagicBoltProjectile)));
            if (v15) {
                pprojectile_3 = (MagicProjectile*)::new (v15)
                    MagicBoltProjectile(apCaster, apSpell, apDominantEffect, pavEffect, aOrigin, aRotation);
            }
            else {
                pprojectile_3 = nullptr;
            }
            pprojectile_2 = pprojectile_3;
            break;
        }
        case MagicSystem::SPRAY: {
            MagicProjectile* pprojectile_5;
            MagicSprayProjectile* v13 = static_cast<MagicSprayProjectile*>(ourMalloc(sizeof(MagicSprayProjectile)));
            if (v13) {
                pprojectile_5 = (MagicProjectile*)::new (v13)
                    MagicSprayProjectile(apCaster, apSpell, apDominantEffect, pavEffect, aOrigin, aRotation);
            }
            else {
                pprojectile_5 = nullptr;
            }
            pprojectile_2 = pprojectile_5;
            break;
        }
        case MagicSystem::FOG: {
            MagicProjectile* pprojectile_4;
            MagicFogProjectile* apMemory = static_cast<MagicFogProjectile*>(ourMalloc(sizeof(MagicFogProjectile)));
            if (apMemory) {
                pprojectile_4 = (MagicProjectile*)::new (apMemory) MagicFogProjectile(
                    apCaster, apSpell, apDominantEffect, pavEffect, aOrigin, aRotation, apData3D, apLight
                );
            }
            else {
                pprojectile_4 = nullptr;
            }
            pprojectile_2 = pprojectile_4;
            break;
        }
        default: {
            break;
        }
    }
    if (pprojectile_2) {
        ReferenceProcessLists.AddReference(pprojectile_2, PROCESS_TYPE_HIGH, false, false, nullptr);
    }
}

NiAVObject* MagicProjectile::Clone3D_0(EffectSetting* apAVEffect) {
    const char* v1 = apAVEffect->GetModel();
    return MagicProjectile::Clone3D(v1);
}
