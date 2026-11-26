#include "Fallout/Magic/MagicBoltProjectile.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSGameSound.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/NewOverloads.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkCharacterController.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkSphereShape.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSXFlags.h"
#include "BSMain/Setting.h"
#include "BSShader/BSShaderUtil.h"
#include "BSShader/BoltShader.h"
#include "BSShader/BoltShaderProperty.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout Shared/TempEffects/BSTempEffectParticle.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/MobileObject.h"
#include "Fallout/AI/Movement/ActorMover.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Magic/ActiveEffect.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicProjectile.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/bhkCharacterListenerSpell.h"
#include "Fallout/Misc/Main.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMatrix3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTransform.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiUpdateData.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiMemObject.h"
#include "SDK/MSVC/rtti.h"

#include <limits>

BoltSavedData::BoltSavedData() : pAreaBoltData(nullptr) {}

MagicBoltProjectile::AreaBolt::AreaBolt()
  : spBoltProperty(nullptr), spBoltNode(nullptr), spSource(nullptr), spTarget(nullptr) {}

void MagicBoltProjectile::AreaBolt::operator delete(void* apMemory) {
    ourFree(apMemory);
}

MagicBoltProjectile::MagicBoltProjectile()
  : spBoltProperty(nullptr), pAreaBolts(nullptr), spBoltNode(nullptr), spSource(nullptr), spTarget(nullptr),
    spSoundNode(nullptr), pTarget(nullptr), fCollideTime(0.0f) {
    SetState(MagicBoltProjectile::TRUNK);
    fSpeed = 4096.0f;
}

// A bolt the visible player casts starts fShockCastVOffset higher. The 3D cloned from the effect is dropped at once.
// The bolt starts with the lowest possible x velocity; the effect's bolt sound follows the sound node, looping.
MagicBoltProjectile::MagicBoltProjectile(
    MagicCaster* apCaster,
    MagicItem* apSpell,
    EffectItem* apDominantEffect,
    EffectSetting* apAVEffect,
    NiPoint3 aOrigin,
    NiPoint3 aRotation
)
  : MagicProjectile(apCaster, apSpell, apDominantEffect, apAVEffect, aOrigin, aRotation), spBoltProperty(nullptr),
    pAreaBolts(nullptr), spBoltNode(nullptr), spSource(nullptr), spTarget(nullptr), spSoundNode(nullptr),
    pTarget(nullptr), fCollideTime(0.0f) {
    BoltShader::SetSubSegmentCount(iShockSubSegments.UInt());
    SetState(TRUNK);
    double dProjectileSpeed = GetAVEffect()->GetProjectileSpeed();
    fSpeed = static_cast<float>(fMagicProjectileBaseSpeed.Float() * dProjectileSpeed);
    if (apCaster->GetCasterStatsObject() == pPlayer && pPlayer->Get3D(true) && !pPlayer->Get3D(true)->GetAppCulled()) {
        NiPoint3 kLocation = *QLocationOnReference();
        kLocation.z += fShockCastVOffset.Float();
        SetLocationOnReference(kLocation);
    }
    { NiPointer<NiAVObject> spClone(MagicProjectile::Clone3D_0(GetAVEffect())); }
    Init3D();
    Set3DSimple(spBoltNode);
    MagicBoltProjectile::InitHavok();
    alignas(16) hkVector4 kVelocity(-std::numeric_limits<float>::max(), 0.0f, 0.0f, 0.0f);
    static_cast<bhkCharacterController*>(GetCharController())->SetLinearVelocity(kVelocity);
    GetParentCell()->AddReference(this, false);
    SetState(TRUNK);
    if (GetAVEffect()->GetBoltSound()) {
        uint32_t uiSoundID = GetAVEffect()->GetBoltSound()->GetFormID();
        if (Sound.IsValid()) {
            Sound.Release();
        }
        if (spSoundNode) {
            Sound = BSAudio::QInstance().GetSoundHandleByNumericID(
                uiSoundID,
                BSGameSound::SOUND_FLAG_IS_3D | BSGameSound::SOUND_FLAG_ONE_SHOT
            );
            NiPoint3 kPosition = *QLocationOnReference();
            Sound.SetPosition(kPosition.x, kPosition.y, kPosition.z);
            Sound.SetObjectToFollow(spSoundNode);
            Sound.Play(true);
        }
    }
    CheckShotBehindWall();
    TESObjectREFR* pCasterRef = GetCaster() ? GetCaster()->GetCasterStatsObject() : nullptr;
    if (pCasterRef == pPlayer) {
        ReferenceProcessLists.ProjectilePostProcessList.AddHead(this);
    }
    else {
        MagicProjectile::AddTrackedProjectile(fMagicTrackingMultBolt.Float());
    }
}

// The pointer members release their objects in their own destructors; each area bolt is stopped and detached here,
// then deleted.
MagicBoltProjectile::~MagicBoltProjectile() {
    MagicCaster* pPlayerCaster;
    if (pPlayer) {
        pPlayerCaster = &static_cast<MagicCaster&>(static_cast<Actor&>(static_cast<Character&>(*pPlayer)));
    }
    else {
        pPlayerCaster = nullptr;
    }
    if (GetCaster() != pPlayerCaster) {
        MagicProjectile::RemoveTrackedProjectile(fMagicTrackingMultBolt.Float());
    }
    ReferenceProcessLists.ProjectilePostProcessList.Remove(static_cast<MobileObject*>(this));
    BoltShader::Stop(spBoltNode.m_pObject, spBoltProperty.m_pObject);
    if (spBoltNode.m_pObject) {
        NiNode* pParent = spBoltNode->m_pkParent;
        if (pParent) {
            pParent->DetachChild(spBoltNode.m_pObject);
        }
    }
    while (pAreaBolts) {
        MagicBoltProjectile::AreaBolt* pNextBolt = pAreaBolts->pNext;
        BoltShader::Stop(pAreaBolts->spBoltNode.m_pObject, pAreaBolts->spBoltProperty.m_pObject);
        if (pAreaBolts->spBoltNode.m_pObject) {
            NiNode* pParent = pAreaBolts->spBoltNode->m_pkParent;
            if (pParent) {
                pParent->DetachChild(pAreaBolts->spBoltNode.m_pObject);
            }
        }
        delete pAreaBolts;
        pAreaBolts = pNextBolt;
    }
    if (Sound.IsValid()) {
        Sound.Stop();
        Sound.Release();
    }
}

uint16_t MagicBoltProjectile::GetSaveSize(uint32_t aiFlags) {
    int16_t sBaseSize = static_cast<int16_t>(MagicProjectile::GetSaveSize(aiFlags) + 8);
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x30u) {
        sBaseSize += 32;
    }
    uint16_t sSize = static_cast<uint16_t>(sBaseSize + 6);
    for (MagicBoltProjectile::AreaBolt* pBolt = pAreaBolts; pBolt; pBolt = pBolt->pNext) {
        sSize += 4;
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 0x71u) {
        sSize += 4;
    }
    return sSize;
}

// The area bolts' count is written as a placeholder first and patched through the save buffer's position taken before
// that write.
void MagicBoltProjectile::SaveGame(uint32_t aiFlags) {
    MagicProjectile::SaveGame(aiFlags);
    pSaveLoadGameOLD->SaveGameDataOLD(&eState, sizeof(eState));
    SaveGameDataOLD(&fSpeed, sizeof(fSpeed));
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 48u) {
        NiPoint3 startingLocation = Get3D()->GetTranslate();
        SaveGameDataOLD(&startingLocation, sizeof(startingLocation));
        NiQuaternion rotation;
        Get3D()->GetRotate(rotation);
        pSaveLoadGameOLD->SaveGameDataOLD(&rotation, sizeof(rotation));
        float fBoltTimer = 0.0f;
        if (spBoltProperty.m_pObject) {
            fBoltTimer = spBoltProperty->fTimer;
        }
        SaveGameDataOLD(&fBoltTimer, sizeof(fBoltTimer));
    }
    uint32_t iTargetID = 0;
    if (pTarget) {
        iTargetID = pTarget->iFormID;
    }
    SaveNumericID(&iTargetID, 4);
    uint16_t iCount = 0;
    uint16_t* pCount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
    SaveGameDataOLD(&iCount, sizeof(iCount));
    for (MagicBoltProjectile::AreaBolt* pBolt = pAreaBolts; pBolt; pBolt = pBolt->pNext) {
        uint32_t iMagicTargetID = 0;
        if (pBolt->spTarget.m_pObject) {
            TESObjectREFR* pRef = TESObjectREFR::FindReferenceFor3D(pBolt->spTarget.m_pObject);
            if (pRef) {
                iMagicTargetID = pRef->iFormID;
            }
        }
        SaveNumericID(&iMagicTargetID, 4);
        ++iCount;
    }
    *pCount = iCount;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
        SaveGameDataOLD(&fCollideTime, sizeof(fCollideTime));
    }
}

// The saved bolt state is read into a BoltSavedData kept in pAreaBolts until FinishInitLoadGame applies and frees it.
// The area bolts' targets are read as a count, then stored as a count followed by that many form IDs.
void MagicBoltProjectile::LoadGame(uint32_t aiFlags, uint32_t aiCurrentFlags) {
    InitHavok();
    MagicProjectile::LoadGame(aiFlags, aiCurrentFlags);
    pSaveLoadGameOLD->LoadGameDataOLD(&eState, sizeof(eState));
    LoadGameDataOLD(&fSpeed, sizeof(fSpeed));
    BoltSavedData* pData = new BoltSavedData;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 48u) {
        LoadGameDataOLD(&pData->StartingLocation, sizeof(pData->StartingLocation));
        pSaveLoadGameOLD->LoadGameDataOLD(&pData->Rotation, sizeof(pData->Rotation));
        LoadGameDataOLD(&pData->fBoltTimer, sizeof(pData->fBoltTimer));
    }
    if (pSaveLoadGameOLD->GetCurrentVersion() < 48u) {
        pData->StartingLocation = pPlayer->GetLocationOnReference();
        pData->Rotation = NiQuaternion::IDENTITY;
    }
    uint32_t iTargetID;
    LoadNumericID(&iTargetID, 4);
    pTarget = reinterpret_cast<TESObjectREFR*>(static_cast<uintptr_t>(iTargetID));
    uint16_t iCount;
    LoadGameDataOLD(&iCount, sizeof(iCount));
    if (iCount) {
        uint32_t uiEntries = iCount + 1u;
        pData->pAreaBoltData = static_cast<uint32_t*>(
            ourMalloc(uiEntries > SIZE_MAX / sizeof(uint32_t) ? SIZE_MAX : sizeof(uint32_t) * uiEntries)
        );
        pData->pAreaBoltData[0] = iCount;
        LoadNumericID(&pData->pAreaBoltData[1], iCount * sizeof(uint32_t));
    }
    pSavedData = pData;
    if (pSaveLoadGameOLD->GetCurrentVersion() >= 113u) {
        LoadGameDataOLD(&fCollideTime, sizeof(fCollideTime));
    }
}

void MagicBoltProjectile::InitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
    MagicProjectile::InitLoadGame(aiFlags, aiOldFlags);
    uint32_t iTargetID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTarget));
    if (iTargetID) {
        TESForm* pForm = TESForm::GetFormByNumericID(iTargetID);
        pTarget = dynamic_cast<TESObjectREFR*>(pForm);
        if (pTarget) {
            NiAVObject* pAV = pTarget->Get3D();
            if (pAV) {
                NiFixedString kName("Bip01 Spine2");
                spTarget = pAV->GetObjectByName(kName);
            }
        }
    }
    Init3D();
    Set3DSimple(spBoltNode.m_pObject);
    GetParentCell()->AddReference(this, false);
    if (pAVEffect->GetBoltSound()) {
        uint32_t iSoundFileID = pAVEffect->GetBoltSound()->iFormID;
        if (Sound.IsValid()) {
            Sound.Release();
        }
        if (Get3D()) {
            Sound = BSAudio::QInstance().GetSoundHandleByNumericID(iSoundFileID, 258u);
            NiPoint3 kPosition = GetLocationOnReference();
            Sound.SetPosition(kPosition.x, kPosition.y, kPosition.z);
            Sound.SetObjectToFollow(spSoundNode.m_pObject);
            Sound.Play(true);
        }
    }
}

// Applies the state LoadGame left in pAreaBolts as a BoltSavedData: the bolt node's place and rotation, the bolt's
// shape towards the target (or the projectile), the area targets; then frees it (its target array stays allocated,
// deliberately). The actor's filter result is the filter word itself
// (MobileObject::GetCollisionFilter returns its argument), so GetGroup runs on that filter.
void MagicBoltProjectile::FinishInitLoadGame(uint32_t aiFlags, uint32_t aiOldFlags) {
    MobileObject::FinishInitLoadGame(aiFlags, aiOldFlags);
    bhkCharacterController* pCon = GetCharController();
    if (pCon) {
        uint32_t uiGroup;
        if (GetCaster() && GetCaster()->GetCasterStatsObject()) {
            TESObjectREFR* pRef = GetCaster()->GetCasterStatsObject();
            Actor* pActor = pRef->IsActor() ? static_cast<Actor*>(pRef) : nullptr;
            if (pActor) {
                CFilter kFilter;
                pActor->GetCollisionFilter(&kFilter.iFilter);
                uiGroup = kFilter.GetGroup();
            }
            else {
                bhkCollisionObject* pCol = FindFirstCollisionObject(pRef->Get3D());
                bhkRigidBody* pRB = pCol ? pCol->GetRigidBody() : nullptr;
                if (pRB) {
                    CFilter kFilter;
                    uiGroup = pRB->GetCollisionFilter(&kFilter)->GetGroup();
                }
                else {
                    uiGroup = bhkCollisionFilter::GetNewSystemGroup();
                }
            }
        }
        else {
            bhkCollisionObject* pCol = FindFirstCollisionObject(Get3D());
            bhkRigidBody* pRB = pCol ? pCol->GetRigidBody() : nullptr;
            if (pRB) {
                CFilter kFilter;
                uiGroup = pRB->GetCollisionFilter(&kFilter)->GetGroup();
            }
            else {
                uiGroup = bhkCollisionFilter::GetNewSystemGroup();
            }
        }
        CFilter filter;
        pCon->GetCollisionFilter(&filter);
        filter.SetLayer(L_SPELL);
        filter.SetGroup(uiGroup);
        pCon->SetCollisionFilter(filter);
    }
    TESObjectREFR* pCaster = GetCaster() ? GetCaster()->GetCasterStatsObject() : nullptr;
    if (pCaster != pPlayer) {
        MagicProjectile::AddTrackedProjectile(fMagicTrackingMultBolt.Float());
    }
    BoltSavedData* pTempData = pSavedData;
    if (pTempData) {
        if (spBoltNode.m_pObject) {
            spBoltNode->SetTranslate(pTempData->StartingLocation);
            spBoltNode->SetRotate_0(pTempData->Rotation);
            bool bPostHit = eState != MagicBoltProjectile::TRUNK;
            if (spTarget.m_pObject) {
                NiMatrix3 rot;
                NiPoint3 toTarget = spTarget->GetWorldTranslate() - pTempData->StartingLocation;
                spBoltProperty->UpdateBolt(pTempData->fBoltTimer, toTarget.Length(), fSpeed, bPostHit);
                float fZ = toTarget.z;
                toTarget.z = 0.0f;
                float fXY = toTarget.Length();
                float fYAngle = -NiATan2(fZ, fXY);
                float fZAngle = NiATan2(toTarget.x, toTarget.y);
                rot.FromEulerAnglesZYX(fZAngle, 0.0f, fYAngle);
                spBoltNode->SetRotate(rot);
            }
            else {
                NiPoint3 endLocation = GetLocationOnReference();
                float fBoltLength = (pTempData->StartingLocation - endLocation).Length();
                spBoltProperty->UpdateBolt(pTempData->fBoltTimer, fBoltLength, fSpeed, bPostHit);
                if (spBoltProperty->IsAtMaxLength() || eState == MagicBoltProjectile::PERSIST) {
                    SetState(MagicBoltProjectile::PERSIST);
                    CFilter filter;
                    GetCharController()->GetCollisionFilter(&filter);
                    filter.SetDisabled(1);
                    GetCharController()->SetCollisionFilter(filter);
                }
            }
        }
        pAreaBolts = nullptr;
        if (pTempData->pAreaBoltData) {
            // Only the count's low 16 bits are read.
            uint16_t iCount = static_cast<uint16_t>(pTempData->pAreaBoltData[0]);
            for (uint32_t i = 0; i < iCount; ++i) {
                uint32_t iMagicTargetID = pTempData->pAreaBoltData[i + 1];
                MagicTarget* pMagicTarget = MagicTarget::GetMagicTargetByNumericID(iMagicTargetID);
                if (pMagicTarget) {
                    AddAreaTarget(pMagicTarget, nullptr);
                }
            }
        }
        ourFree(pTempData);
    }
}

// Gives a high-process bolt a character controller (a 1-unit sphere in the spell layer, in the caster's collision
// group, without gravity) when it has none, then puts the controller in the cell's world at the bolt's position, 8
// units higher when it changes worlds. MagicBallProjectile::InitHavok's steps, except that the projectile flag is set
// before the quick-simulate one.
void MagicBoltProjectile::InitHavok() {
    if (!pCurrentProcess || pCurrentProcess->GetProcessLevel() != PROCESS_TYPE_HIGH) {
        return;
    }
    bhkCharacterController* pController = static_cast<bhkCharacterController*>(GetCharController());
    TESObjectCELL* pCell = GetParentCell();
    bhkWorld* pWorld = pCell ? pCell->GetbhkWorld() : nullptr;
    if (!pController) {
        alignas(16) uint8_t aCinfoMemory[sizeof(bhkCharacterControllerCinfo)];
        bhkCharacterControllerCinfo* pCinfo = ::new (aCinfoMemory) bhkCharacterControllerCinfo(true);
        pCinfo->SetAcrobatics(1.0f);
        pCinfo->SetObject(Get3D());
        pCinfo->SetPosition(GetLocationOnReference());
        pCinfo->SetWorld(pWorld);
        pCinfo->SetGravity(0.0f);
        // NiNew bhkSphereShape: 0x14 bytes.
        bhkSphereShape* pShape = new bhkSphereShape(1.0f, true);
        pCinfo->SetShape(pShape);
        pCinfo->SetAvoidBox(false);
        if (pSaveLoadGameOLD->GetLoading()) {
            pCinfo->SetColFilter(L_SPELL, 0);
        }
        else {
            TESObjectREFR* pCasterRef = nullptr;
            if (GetCaster()) {
                pCasterRef = GetCaster()->GetCasterStatsObject();
            }
            if (pCasterRef) {
                if (pCasterRef->IsActor()) {
                    CFilter kFilter;
                    static_cast<MobileObject*>(pCasterRef)->GetCollisionFilter(&kFilter.iFilter);
                    pCinfo->SetColFilter(L_SPELL, kFilter.GetGroup());
                }
                else {
                    bhkCollisionObject* pCollisionObject = FindFirstCollisionObject(pCasterRef->Get3D());
                    bhkRigidBody* pBody;
                    if (pCollisionObject) {
                        pBody = pCollisionObject->GetRigidBody();
                    }
                    else {
                        pBody = nullptr;
                    }
                    uint32_t uiGroup;
                    if (pBody) {
                        CFilter kFilter;
                        pBody->GetCollisionFilter(&kFilter);
                        uiGroup = kFilter.GetGroup();
                    }
                    else {
                        uiGroup = bhkCollisionFilter::GetNewSystemGroup();
                    }
                    pCinfo->SetColFilter(L_SPELL, uiGroup);
                }
            }
            else {
                bhkCollisionObject* pCollisionObject = FindFirstCollisionObject(Get3D());
                bhkRigidBody* pBody;
                if (pCollisionObject) {
                    pBody = pCollisionObject->GetRigidBody();
                }
                else {
                    pBody = nullptr;
                }
                uint32_t uiGroup;
                if (pBody) {
                    CFilter kFilter;
                    pBody->GetCollisionFilter(&kFilter);
                    uiGroup = kFilter.GetGroup();
                }
                else {
                    uiGroup = bhkCollisionFilter::GetNewSystemGroup();
                }
                pCinfo->SetColFilter(L_SPELL, uiGroup);
            }
        }
        pCinfo->SetStartState(HK_CHARACTER_USER_STATE_1);
        pController = new bhkCharacterListenerSpell(*pCinfo, this);
        pController->SetFallTime(1.0f);
        pController->SetIsProjectile(true);
        pController->SetQuickSimulate(Main::GetSceneIsComplex());
        pCurrentProcess->SetCharController(NiPointer<bhkCharacterController>(pController));
        pController->SetMOBObject(this);
        pCinfo->~bhkCharacterControllerCinfo();
    }
    if (!pController) {
        return;
    }
    if (pCell && pWorld) {
        pController->SetWaterHeight(pCell->GetWaterHeight());
    }
    else {
        pController->SetWaterHeight(-std::numeric_limits<float>::max());
    }
    bhkWorld* pOldWorld = pController->GetWorld();
    pController->SetWorld(pWorld);
    NiNode* p3D = Get3D();
    NiAVObject* pOldObject = pController->GetNiObject();
    bool bAddToWorld = true;
    if (pOldObject != p3D) {
        pController->SetNiObject(p3D);
        if (pWorld && pWorld->QDebugDisplay()) {
            pController->BuildDisplayGeometry(nullptr);
            BSShaderUtil::RecursiveSetPropertyWireframe(pController->GetNiObject(), 1);
        }
        bhkWorld::SetMotion(p3D, hkpMotion::MOTION_KEYFRAMED, true, false, true);
        bAddToWorld = true;
    }
    NiPoint3 kPosition = GetLocationOnReference();
    if (pOldWorld != pWorld) {
        if (pOldWorld) {
            bhkWorld::RemoveObjects(p3D, true, false);
        }
        kPosition.z = static_cast<float>(kPosition.z + 8.0);
        bAddToWorld = true;
    }
    if (!pWorld) {
        return;
    }
    pController->SetPosition(kPosition);
    if (bAddToWorld) {
        CFilter kFilter;
        pController->GetCollisionFilter(&kFilter);
        uint32_t uiGroup = kFilter.GetGroup();
        pWorld->AddObjects(p3D, true, false, uiGroup, false);
    }
    else {
        bhkWorld::UpdatePosition(p3D, true, false);
    }
}

void MagicBoltProjectile::Update(float afDelta) {
    static char bStop;
    CFilter filter;
    NiPoint3 kMove;
    if (afDelta != 0.0 && !GetDelete()) {
        bhkCharacterController* pController;
        float fHavokWaterHeight;
        float fDistance;
        double dWaterHeight;
        NiPoint3 kNewPos;
        NiPoint3 kOldPos;
        NiPoint3 startingLocation = Get3D()->GetTranslate();
        if (!spBoltProperty.m_pObject || eState) {
            if (eState == MagicBoltProjectile::BRANCH) {
                MagicBoltProjectile::AreaBolt* pBranch = pAreaBolts;
                MagicBoltProjectile::AreaBolt* pPrevious = nullptr;
                while (pBranch) {
                    UpdateBranch(afDelta, pBranch);
                    MagicBoltProjectile::AreaBolt* pNextBranch = pBranch->pNext;
                    if (pBranch->spBoltProperty->IsFinished()) {
                        NiNode* pParent = pBranch->spBoltNode->m_pkParent;
                        if (pParent) {
                            pParent->DetachChild(pBranch->spBoltNode.m_pObject);
                        }
                        if (pPrevious) {
                            pPrevious->pNext = pBranch->pNext;
                        }
                        else {
                            pAreaBolts = pBranch->pNext;
                        }
                        pBranch->spBoltNode = static_cast<NiNode*>(nullptr);
                        pBranch->spBoltProperty = static_cast<BoltShaderProperty*>(nullptr);
                        pBranch->spSource = static_cast<NiAVObject*>(nullptr);
                        pBranch->spTarget = static_cast<NiAVObject*>(nullptr);
                        if (pBranch) {
                            delete pBranch;
                        }
                    }
                    else {
                        pPrevious = pBranch;
                    }
                    pBranch = pNextBranch;
                }
            }
        }
        else {
            pController = GetCharController();
            fHavokWaterHeight = pController->QWaterHeight();
            dWaterHeight = HK2NI(fHavokWaterHeight);
            if (GetLocationOnReference().z <= dWaterHeight) {
                Collide(GetLocationOnReference(), nullptr, nullptr, true);
            }
            fDistance = fSpeed * afDelta;
            kMove = NiPoint3(0.0f, fDistance, 0.0f);
            kOldPos = NiPoint3::ZERO;
            if (GetCharController()) {
                GetCharController()->GetPosition(kOldPos);
            }
            Move(afDelta, kMove, 15u);
            kNewPos = NiPoint3::ZERO;
            if (GetCharController()) {
                GetCharController()->GetPosition(kNewPos);
            }
            fDistanceMoved = (kNewPos - kOldPos).Length() + fDistanceMoved;
            if (fMagicProjectileMaxDistance.Float() < static_cast<double>(fDistanceMoved)) {
                SetDelete(true);
            }
            if (!IsActive()) {
                return;
            }
        }
        if (iShockDebug.Int() <= 0 && !bStop) {
            bool bPostHit = eState != MagicBoltProjectile::TRUNK;
            if (spTarget.m_pObject) {
                NiMatrix3 rot;
                NiPoint3 toTarget = spTarget->GetWorldTranslate() - startingLocation;
                if (!bPostHit ||
                    spBoltProperty->fSegmentLength * static_cast<double>(spBoltProperty->iSegmentsPerBolt) <
                        fAge * fSpeed) {
                    float fBoltSpeed = fSpeed;
                    float fBoltLength = toTarget.Length();
                    spBoltProperty->UpdateBolt(afDelta, fBoltLength, fBoltSpeed, bPostHit);
                }
                float fZ = toTarget.z;
                toTarget.z = 0.0f;
                float fXY = toTarget.Length();
                float fYAngle = -NiATan2(fZ, fXY);
                float fZAngle = NiATan2(toTarget.x, toTarget.y);
                rot.FromEulerAnglesZYX(fZAngle, 0.0f, fYAngle);
                spBoltNode->SetRotate(rot);
            }
            else if (GetCharController()) {
                NiPoint3 kPosition;
                GetCharController()->GetPosition(kPosition);
                float fLength = (startingLocation - kPosition).Length();
                if (!bPostHit ||
                    spBoltProperty->fSegmentLength * static_cast<double>(spBoltProperty->iSegmentsPerBolt) <
                        fAge * fSpeed) {
                    spBoltProperty->UpdateBolt(afDelta, fLength, fSpeed, bPostHit);
                }
                if (spBoltProperty->IsAtMaxLength()) {
                    CFilter kNewFilter;
                    SetState(MagicBoltProjectile::PERSIST);
                    GetCharController()->GetCollisionFilter(&filter);
                    filter.SetDisabled(1);
                    kNewFilter.iFilter = filter.iFilter;
                    GetCharController()->SetCollisionFilter(kNewFilter);
                }
            }
        }
        if (spSoundNode.m_pObject) {
            const NiPoint3& kLocation = GetLocationOnReference();
            spSoundNode->SetWorldTranslate(kLocation);
        }
        if (spBoltProperty->IsFinished() ||
            (eState == MagicBoltProjectile::BRANCH && fLastUpdate - fCollideTime > 3.0)) {
            if (Sound.IsValid()) {
                Sound.Stop();
                Sound.Release();
            }
            if (!pAreaBolts) {
                SetDelete(true);
            }
        }
    }
}

void MagicBoltProjectile::Collide(
    NiPoint3 aContactPoint,
    bhkRigidBody* apTargetBody,
    TESObjectREFR* apCollidee,
    bool abFizzle
) {
    Actor* pActor;
    CFilter filter;
    if (apCollidee && apCollidee->IsActor()) {
        pActor = static_cast<Actor*>(apCollidee);
    }
    else {
        pActor = nullptr;
    }
    if (!pActor || !pActor->GetGhost()) {
        fCollideTime = fLastUpdate;
        if (GetCharController()) {
            CFilter kNewFilter;
            if (GetCharController()->GetFlag(bhkCharacterController::DEBUGDISP)) {
                BuildAreaEffectGeometry();
            }
            GetCharController()->GetCollisionFilter(&filter);
            filter.SetDisabled(1);
            kNewFilter.iFilter = filter.iFilter;
            GetCharController()->SetCollisionFilter(kNewFilter);
        }
        SetState(MagicBoltProjectile::BRANCH);
        NiAVObject* pAV = Get3D();
        if (pAV) {
            NiPoint3 kForward = pAV->GetWorldTransform().m_Rotate * NiPoint3::UNIT_Y;
            ApplyCollisionForce(aContactPoint, kForward, apTargetBody);
        }
        if (apCollidee && (apCollidee != pPlayer || pPlayer->Is3rdPerson())) {
            pTarget = apCollidee;
            pAV = pTarget->Get3D();
            if (pAV) {
                NiFixedString kName("Bip01 Spine2");
                spTarget = pAV->GetObjectByName(kName);
            }
        }

        NiPoint3 startingLocation = Get3D()->GetTranslate();
        float fBoltLength = (startingLocation - aContactPoint).Length();
        spBoltProperty->UpdateBolt(0.01f, fBoltLength, fSpeed, false);
        if (!abFizzle) {
            NiPoint3 kLocation = GetLocationOnReference();
            GetCaster()->FindTargets_0(pSpell, GetParentCell(), kLocation, this, apCollidee, false, 1.0f, 1.0f);
        }
        pTarget = nullptr;
        if (pAVEffect->GetModel()) {
            double dArea = static_cast<double>(GetDominantEffect()->GetArea());
            float fScale = fMagicAreaScale.Float() * dArea;
            if (fMagicAreaScaleMax.Float() < static_cast<double>(fScale)) {
                fScale = fMagicAreaScaleMax.Float();
            }
            else if (fMagicAreaScaleMin.Float() > static_cast<double>(fScale)) {
                fScale = fMagicAreaScaleMin.Float();
            }
            const char* pFilename = pAVEffect->GetModel();
            NiPoint3 orient;
            if (Get3D()) {
                orient = Get3D()->m_kLocal.m_Rotate * NiPoint3::UNIT_Y;
            }
            else {
                orient = NiPoint3::UNIT_Y;
            }
            BSTempEffectParticle* pAreaEffect = BSTempEffectParticle::Spawn(
                GetParentCell(),
                5.0f,
                pFilename,
                orient,
                aContactPoint,
                1.0f,
                7u,
                nullptr
            );
            pAreaEffect->PlaySequence("SpecialIdle_AreaEffect");
        }
        if (pAVEffect->data.pAreaSound && !abFizzle) {
            uint32_t iSoundFileID = pAVEffect->data.pAreaSound->iFormID;
            BSSoundHandle soundHand = BSAudio::QInstance().GetSoundHandleByNumericID(iSoundFileID, 258u);
            NiPoint3 kPosition = GetLocationOnReference();
            soundHand.SetPosition(kPosition.x, kPosition.y, kPosition.z);
            soundHand.Play(false);
        }
    }
}

void MagicBoltProjectile::AddAreaTarget(MagicTarget* apTarget, ActiveEffect* apActiveEffect) {
    if (!apActiveEffect ||
        (apActiveEffect->pEffect == GetDominantEffect() && pTarget != apTarget->GetTargetStatsObject())) {
        NiPoint3 location;
        NiNode* pNode = new NiNode(0);
        NiNode* pParentNode = static_cast<NiNode*>(GetParentCell()->GetDynamicNode());
        pParentNode->AttachChild(pNode, 1);
        BoltShaderProperty* pProp = BoltShader::Start(pNode, false);
        if (pProp) {
            pProp->SetAbsorbNumBolts(iShockBranchNumBolts.Int());
            pProp->SetBoltsRadius(fShockBranchBoltsRadius.Float());
            pProp->SetBoltsRadiusStrength(fShockBranchBoltsRadiusStrength.Float());
            pProp->SetBoltSmallWidth(fShockBoltSmallWidth.Float());
            pProp->SetBoltGrowWidth(fShockBoltGrowWidth.Float());
            float fCoreB = fShockCoreColorB.Float();
            float fCoreG = fShockCoreColorG.Float();
            NiColorA kCoreColor(fShockCoreColorR.Float(), fCoreG, fCoreB, 1.0f);
            pProp->SetAbsorbCoreColor(kCoreColor.r, kCoreColor.g, kCoreColor.b, kCoreColor.a);
            float fGlowB = fShockGlowColorB.Float();
            float fGlowG = fShockGlowColorG.Float();
            NiColorA kGlowColor(fShockGlowColorR.Float(), fGlowG, fGlowB, 1.0f);
            pProp->SetAbsorbGlowColor(kGlowColor.r, kGlowColor.g, kGlowColor.b, kGlowColor.a);
            pProp->SetAbsorbSegmentLength(fShockBranchSegmentLength.Float());
            pProp->iSegmentsPerBolt = iShockBranchSegmentsPerBolt.Int();
            pProp->SetSegmentVariance(fShockBranchSegmentVariance.Float());
            pProp->SetAbsorbTortuosityVariance(fShockBranchSubSegmentVariance.Float());
            pProp->SetStartFromPoint(false);
            pProp->InitSegmentData();
        }
        GetCharController()->GetPosition(location);
        NiAVObject* pSource = nullptr;
        NiAVObject* pAV = pTarget ? pTarget->Get3D() : nullptr;
        if (pAV) {
            NiFixedString kName("Bip01 Spine2");
            pSource = pAV->GetObjectByName(kName);
        }
        pAV = apTarget->GetTargetStatsObject()->Get3D();
        if (pAV) {
            NiAVObject* pTargetNode = pAV->GetObjectByName("Bip01 Spine2");
            if (!pTargetNode) {
                pTargetNode = pAV;
            }
            MagicBoltProjectile::AreaBolt* pNewBolt = new MagicBoltProjectile::AreaBolt;
            pNewBolt->spBoltProperty = pProp;
            pNewBolt->spBoltNode = pNode;
            pNewBolt->spSource = pSource;
            pNewBolt->spTarget = pTargetNode;
            pNewBolt->SourceLoc = location;
            pNewBolt->pNext = pAreaBolts;
            UpdateBranch(0.0f, pNewBolt);
            pAreaBolts = pNewBolt;
        }
    }
}

bool MagicBoltProjectile::PostProcess() {
    MagicCaster* pCaster = GetCaster();
    TESObjectREFR* pCasterRef = pCaster ? pCaster->GetCasterStatsObject() : nullptr;
    TESObjectREFR* pActorRef = pCasterRef && pCasterRef->IsActor() ? pCasterRef : nullptr;
    PlayerCharacter* pCasterActor = static_cast<PlayerCharacter*>(pActorRef);
    NiNode* pMagicNode = pActorRef ? pCasterActor->GetMagicNode() : nullptr;
    bhkCharacterController* pChar = GetCharController();
    if (!pMagicNode || !pChar || !spBoltNode.m_pObject) {
        return true;
    }
    if (fAge < 0.5 && pCasterActor == pPlayer && !pPlayer->Is3rdPerson()) {
        NiPoint3 startLocation = pMagicNode->GetWorldTranslate();
        spBoltNode->SetTranslate(startLocation);
    }
    if (fAge > 0.0) {
        NiPoint3 endLocation;
        NiMatrix3 rot;
        NiPoint3 startLocation = spBoltNode->GetWorldTranslate();
        pChar->GetPosition(endLocation);
        NiPoint3 toTarget = endLocation - startLocation;
        float fZ = toTarget.z;
        toTarget.z = 0.0f;
        float fXY = toTarget.Length();
        float fYAngle = -NiATan2(fZ, fXY);
        float fZAngle = NiATan2(toTarget.x, toTarget.y);
        rot.FromEulerAnglesZYX(fZAngle, 0.0f, fYAngle);
        spBoltNode->SetRotate(rot);
    }
    NiUpdateData UpdateData(0.0f, false, false);
    spBoltNode->Update(UpdateData);
    return false;
}

void MagicBoltProjectile::SetState(State aeNewState) {
    eState = aeNewState;
}

void MagicBoltProjectile::UpdateBranch(float afDelta, MagicBoltProjectile::AreaBolt* apBranch) {
    NiPoint3 toTarget;
    NiMatrix3 rot;
    NiNode* pNode = apBranch->spBoltNode.m_pObject;
    if (apBranch->spSource.m_pObject) {
        apBranch->SourceLoc = apBranch->spSource->GetWorldTranslate();
    }
    pNode->SetTranslate(apBranch->SourceLoc);
    toTarget = apBranch->spTarget->GetWorldTranslate() - apBranch->SourceLoc;
    double dElapsed = fLastUpdate - fCollideTime;
    bool bFinish = fShockBranchLifetime.Float() < dElapsed;
    float fBoltSpeed = fSpeed;
    float fBoltLength = toTarget.Length();
    BoltShaderProperty* pProp = apBranch->spBoltProperty.m_pObject;
    pProp->UpdateAndMoveBolt(afDelta, fBoltLength, fBoltSpeed, bFinish);
    float fZ = toTarget.z;
    toTarget.z = 0.0f;
    float fXY = toTarget.Length();
    float fYAngle = -NiATan2(fZ, fXY);
    float fZAngle = NiATan2(toTarget.x, toTarget.y);
    rot.FromEulerAnglesZYX(fZAngle, 0.0f, fYAngle);
    pNode->SetRotate(rot);
}

void MagicBoltProjectile::Init3D() {
    NiMatrix3 rot;
    spSoundNode = new NiNode(0);
    const NiPoint3& kSoundLocation = GetLocationOnReference();
    spSoundNode->SetWorldTranslate(kSoundLocation);
    spBoltNode = new NiNode(0);
    const NiPoint3& kBoltLocation = GetLocationOnReference();
    spBoltNode->SetTranslate(kBoltLocation);
    const NiFixedString& kTag = BSXFlags::Tag();
    BSXFlags* pExtra = static_cast<BSXFlags*>(spBoltNode->GetExtraData(kTag));
    if (!pExtra) {
        pExtra = new BSXFlags;
        const NiFixedString& kNewTag = BSXFlags::Tag();
        spBoltNode->AddExtraData(kNewTag, pExtra);
    }
    pExtra->SetFlags(BSXFlags::ANIMATED, true);
    spBoltProperty = static_cast<BoltShaderProperty*>(nullptr);
    NiPoint3 kAngle = GetAngleOnReference();
    rot.FromEulerAnglesZXY(kAngle.z, kAngle.x, kAngle.y);
    spBoltNode->SetRotate(rot);
    spBoltProperty = BoltShader::Start(spBoltNode.m_pObject, true);
    if (spBoltProperty.m_pObject) {
        spBoltProperty->SetAbsorbNumBolts(iShockNumBolts.Int());
        spBoltProperty->SetBoltsRadius(fShockBoltsRadius.Float());
        spBoltProperty->SetBoltsRadiusStrength(fShockBoltsRadiusStrength.Float());
        spBoltProperty->SetBoltSmallWidth(fShockBoltSmallWidth.Float());
        spBoltProperty->SetBoltGrowWidth(fShockBoltGrowWidth.Float());
        float fCoreB = fShockCoreColorB.Float();
        float fCoreG = fShockCoreColorG.Float();
        NiColorA kCoreColor(fShockCoreColorR.Float(), fCoreG, fCoreB, 1.0f);
        spBoltProperty->SetAbsorbCoreColor(kCoreColor.r, kCoreColor.g, kCoreColor.b, kCoreColor.a);
        float fGlowB = fShockGlowColorB.Float();
        float fGlowG = fShockGlowColorG.Float();
        NiColorA kGlowColor(fShockGlowColorR.Float(), fGlowG, fGlowB, 1.0f);
        spBoltProperty->SetAbsorbGlowColor(kGlowColor.r, kGlowColor.g, kGlowColor.b, kGlowColor.a);
        spBoltProperty->SetAbsorbSegmentLength(fShockSegmentLength.Float());
        spBoltProperty->iSegmentsPerBolt = iShockSegmentsPerBolt.Int();
        spBoltProperty->SetSegmentVariance(fShockSegmentVariance.Float());
        spBoltProperty->SetAbsorbTortuosityVariance(fShockSubSegmentVariance.Float());
        spBoltProperty->SetStartFromPoint(true);
        spBoltProperty->InitSegmentData();
    }
}
