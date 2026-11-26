#include "Fallout/Misc/GarbageCollector.h"

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/NewOverloads.h"
#include "BSHavok/WeaponObject.h"
#include "BSHavok/bhkNiCollisionObject.h"
#include "BSHavok/bhkWorld.h"
#include "BSShader/BSFadeNode.h"
#include "BSShader/ParticleShaderProperty.h"
#include "Fallout Shared/Animation.h"
#include "Fallout Shared/BGSBodyPart.h"
#include "Fallout Shared/BipedAnim.h"
#include "Fallout Shared/Distant Terrain System/BGSTerrainManager.h"
#include "Fallout Shared/FaceGen/BSFaceGenManager.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Pathfinding/NavMesh.h"
#include "Fallout Shared/Pathfinding/NavMeshPtr.h"
#include "Fallout Shared/Pathfinding/PathingLocation.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TESConditionFunctions.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout Shared/TESWorldSpace.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/ArrowProjectile.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/Combat/CombatGroup.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/Interface/Menus/SurgeryMenu.h"
#include "Fallout/Magic/ValueModifierEffect.h"
#include "Fallout/Misc/MemoryLevelManager.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/BSMultiTargetTreadTransfController.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiControllerManager.h"
#include "Gamebryo2.2/CoreLibs/NiAnimation/NiMultiTargetTransformController.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSDismemberSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/BSTexturePalette.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiRefObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiSkinInstance.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiStream.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTArray.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTCollection.h"

#include <cstdint>

NiTObjectArray<TESObjectREFR*> GarbageCollector::ObjectGarbageA(0, 5000);
NiTObjectArray<TESObjectREFR*> GarbageCollector::kQueuedTESObjectREFRs(0, 5000);
NiTObjectArray<NiPointer<NiAVObject>> GarbageCollector::Actor3DGarbageA(0, 150);
NiTObjectArray<NiPointer<NiAVObject>> GarbageCollector::kQueuedNiAVObjects(0, 150);
NiTObjectArray<BipedAnim*> GarbageCollector::kBipedAnims(0, 150);
NiTObjectArray<BipedAnim*> GarbageCollector::kQueuedBipedAnims(0, 150);
NiTObjectArray<Animation*> GarbageCollector::AnimGarbageA(0, 150);
NiTObjectArray<NiPointer<BSTempEffect>> GarbageCollector::EffectGarbageA(0, 150);
NiTObjectArray<NiPointer<BSTempEffect>> GarbageCollector::kBSTempEffectArray(0, 150);
BSSimpleArray<NavMesh*, 1024> GarbageCollector::kNavMeshes(70, 0);
BSSimpleArray<NavMesh*, 1024> GarbageCollector::kQueuedNavMeshes(70, 0);
BSSpinLock GarbageCollector::CritSec;

// Removes slot uiIndex of the reference array.
void RemoveAtSwapLast(NiTObjectArray<TESObjectREFR*>* apArray, uint32_t uiIndex) {
    if (uiIndex < apArray->m_usSize) {
        --apArray->m_usSize;
        TESObjectREFR* pRemoved = apArray->m_pBase[uiIndex];
        apArray->m_pBase[uiIndex] = apArray->m_pBase[apArray->m_usSize];
        apArray->m_pBase[apArray->m_usSize] = nullptr;
        if (pRemoved) {
            --apArray->m_usESize;
        }
    }
}

int32_t au_re_InterlockedExchange(volatile int32_t* apTarget, int32_t aiValue) {
    return InterlockedExchange(apTarget, aiValue);
}

// Each element is copied into the local spObject and released at the end of each pass.
void MoveQueuedNiAVObjects(
    NiTObjectArray<NiPointer<NiAVObject>>* apSource,
    NiTObjectArray<NiPointer<NiAVObject>>* apTarget
) {
    for (NiPointer<NiAVObject> spObject : *apSource) {
        if (spObject.m_pObject) {
            apTarget->Add(spObject);
        }
    }
}

void MoveQueuedNavMeshes(BSSimpleArray<NavMesh*, 1024>* apSource, BSSimpleArray<NavMesh*, 1024>* apTarget) {
    for (NavMesh* pNavMesh : *apSource) {
        if (pNavMesh) {
            apTarget->Add(pNavMesh);
        }
    }
}

// Each element is copied into a local pointer, appended when it holds an effect and released.
void MoveQueuedTempEffects(
    NiTObjectArray<NiPointer<BSTempEffect>>* apSource,
    NiTObjectArray<NiPointer<BSTempEffect>>* apTarget
) {
    for (NiPointer<BSTempEffect> spEffect : *apSource) {
        if (spEffect.m_pObject) {
            apTarget->Add(spEffect);
        }
    }
}

void Deallocate(void* apMemory) {
    ourFree(apMemory);
}

void* Allocate(uint32_t auiCount) {
    return ourMalloc(auiCount <= SIZE_MAX / sizeof(void*) ? sizeof(void*) * auiCount : SIZE_MAX);
}

GarbageCollector* GarbageCollector::EnterCS(uint32_t auiFlag, const char* apName) {
    uiFlag = auiFlag;
    GarbageCollector::EnterCrit(apName, false);
    bWasSet = (GarbageCollectorCS::uiFlags & auiFlag) != 0;
    GarbageCollectorCS::uiFlags |= auiFlag;
    return this;
}

void GarbageCollector::LeaveCS() {
    if (!bWasSet) {
        GarbageCollectorCS::uiFlags &= ~uiFlag;
    }
    GarbageCollector::LeaveCrit(false);
}

void GarbageCollector::Update() {
    GarbageCollector::EnterCrit("GarbageCollector::Update()", true);
    uint32_t uiNumObjects = 0;
    uint32_t uiRemoveCount = 0;
    uint32_t uiFreeMult = 1;
    if (MemoryLevelManager::QInstance()->WasLastCheckedMemLevelCloseToLow()) {
        uiFreeMult = 2;
    }
    if (GarbageCollector::AnimGarbageA.m_usSize) {
        if (pModelLoader->TryLockModelMap()) {
            uiNumObjects = GarbageCollector::AnimGarbageA.m_usSize;
            uiRemoveCount = 10 * uiFreeMult;
            while (uiNumObjects && uiRemoveCount) {
                --uiNumObjects;
                --uiRemoveCount;
                Animation* pAnim = GarbageCollector::AnimGarbageA[uiNumObjects];
                GarbageCollector::AnimGarbageA.RemoveAt(uiNumObjects);
                if (pAnim) {
                    delete pAnim;
                }
            }
            pModelLoader->UnlockModelMap();
        }
    }
    else if (GarbageCollector::kBipedAnims.m_usSize) {
        if (pModelLoader->TryLockModelMap()) {
            uiNumObjects = GarbageCollector::kBipedAnims.m_usSize;
            uiRemoveCount = 10 * uiFreeMult;
            while (uiNumObjects && uiRemoveCount) {
                --uiNumObjects;
                --uiRemoveCount;
                BipedAnim* pBiped = GarbageCollector::kBipedAnims[uiNumObjects];
                GarbageCollector::kBipedAnims.RemoveAt(uiNumObjects);
                if (pBiped) {
                    delete pBiped;
                }
            }
            pModelLoader->UnlockModelMap();
        }
    }
    else if (GarbageCollector::kQueuedBipedAnims.m_usSize) {
        GarbageCollector::CopyArrays(&GarbageCollector::kQueuedBipedAnims, &GarbageCollector::kBipedAnims);
        GarbageCollector::kQueuedBipedAnims.RemoveAll();
    }
    else if (GarbageCollector::Actor3DGarbageA.m_usSize) {
        uiNumObjects = GarbageCollector::Actor3DGarbageA.m_usSize;
        uiRemoveCount = 10 * uiFreeMult;
        while (uiNumObjects && uiRemoveCount) {
            --uiNumObjects;
            --uiRemoveCount;
            // RemoveAt returns its result by value in spObject, which is released at the end of the
            // iteration.
            NiPointer<NiAVObject> spObject = GarbageCollector::Actor3DGarbageA.RemoveAt(uiNumObjects);
            if (spObject != static_cast<NiAVObject*>(nullptr)) {
                spObject->RemoveGarbageMark();
            }
        }
    }
    else if (GarbageCollector::kQueuedNiAVObjects.m_usSize) {
        MoveQueuedNiAVObjects(&GarbageCollector::kQueuedNiAVObjects, &GarbageCollector::Actor3DGarbageA);
        GarbageCollector::kQueuedNiAVObjects.RemoveAll();
    }
    else if (GarbageCollector::ObjectGarbageA.m_usSize) {
        if (pModelLoader->TryLockModelMap()) {
            uiNumObjects = GarbageCollector::ObjectGarbageA.m_usSize;
            uiRemoveCount = 20 * uiFreeMult;
            int32_t iActors = 1;
            while (uiNumObjects && uiRemoveCount && iActors) {
                --uiNumObjects;
                --uiRemoveCount;
                TESObjectREFR* pRef = GarbageCollector::ObjectGarbageA[0];
                if (pRef->IsActor() || pRef->GetTargeted()) {
                    --iActors;
                }
                if (pRef) {
                    delete pRef;
                }
            }
            pModelLoader->UnlockModelMap();
        }
    }
    else if (GarbageCollector::kQueuedTESObjectREFRs.m_usSize) {
        GarbageCollector::CopyArrays(&GarbageCollector::kQueuedTESObjectREFRs, &GarbageCollector::ObjectGarbageA);
        GarbageCollector::kQueuedTESObjectREFRs.RemoveAll();
    }
    else if (GarbageCollector::kNavMeshes.iSize) {
        uiNumObjects = GarbageCollector::kNavMeshes.iSize;
        uiRemoveCount = 5 * uiFreeMult;
        while (uiNumObjects && uiRemoveCount) {
            --uiNumObjects;
            --uiRemoveCount;
            GarbageCollector::kNavMeshes.GetAt(0)->DecRefCount();
            GarbageCollector::kNavMeshes.Remove(0, true);
        }
    }
    else if (GarbageCollector::kQueuedNavMeshes.iSize) {
        MoveQueuedNavMeshes(&GarbageCollector::kQueuedNavMeshes, &GarbageCollector::kNavMeshes);
        GarbageCollector::kQueuedNavMeshes.Clear(true);
    }
    GarbageCollector::LeaveCrit(true);
}

void GarbageCollector::Add(NavMeshPtr& arNavMesh) {
    GarbageCollector kGuard;
    kGuard.EnterCS(GarbageCollectorCS::GARBAGE_NAVMESHES, "GarbageCollector::Add(NavMeshPtr)");
    // The navmesh's NiRefObject part.
    static_cast<NiRefObject*>(arNavMesh.m_pObject)->IncRefCount();
    if (bCollectGarbage) {
        NavMesh* pNavMesh = arNavMesh;
        kNavMeshes.Add(pNavMesh);
    }
    else {
        NavMesh* pNavMesh = arNavMesh;
        kQueuedNavMeshes.Add(pNavMesh);
    }
    kGuard.LeaveCS();
}

void GarbageCollector::ClearAll(bool abTryEnter) {
    if (!GarbageCollector::bRunning) {
        bool bLocked = true;
        if (abTryEnter) {
            bLocked = GarbageCollector::TryEnterCrit();
        }
        else {
            GarbageCollector::EnterCrit("GarbageCollector::ClearAll()", true);
        }
        if (bLocked) {
            GarbageCollector::bRunning = true;
            bool bCollectGarbage = GarbageCollector::bCollectGarbage;
            GarbageCollector::bCollectGarbage = false;
            if (!GarbageCollectorCS::GetFlag(GarbageCollectorCS::GARBAGE_TEMP_EFFECTS) && BSTexturePalette::TryLock()) {
                GarbageCollector::EffectGarbageA.RemoveAll();
                BSTexturePalette::Unlock();
            }
            if (!GarbageCollectorCS::GetFlag(GarbageCollectorCS::GARBAGE_ANIMATIONS) &&
                pModelLoader->TryLockModelMap()) {
                uint32_t uiCount = GarbageCollector::AnimGarbageA.m_usSize;
                for (uint32_t uiIndex = 0; uiIndex < uiCount; ++uiIndex) {
                    Animation* pAnim = GarbageCollector::AnimGarbageA[uiIndex];
                    if (pAnim) {
                        delete pAnim;
                    }
                }
                GarbageCollector::AnimGarbageA.RemoveAll();
                pModelLoader->UnlockModelMap();
            }
            if (!GarbageCollectorCS::GetFlag(GarbageCollectorCS::GARBAGE_BIPED_ANIMS) &&
                pModelLoader->TryLockModelMap()) {
                uint32_t uiCount = GarbageCollector::kBipedAnims.m_usSize;
                for (uint32_t i = 0; i < uiCount; ++i) {
                    BipedAnim* pBiped = GarbageCollector::kBipedAnims[i];
                    if (pBiped) {
                        delete pBiped;
                    }
                }
                GarbageCollector::kBipedAnims.RemoveAll();
                pModelLoader->UnlockModelMap();
            }
            if (!GarbageCollectorCS::GetFlag(GarbageCollectorCS::GARBAGE_NIAVOBJECTS)) {
                for (const NiPointer<NiAVObject>& spObject : GarbageCollector::Actor3DGarbageA) {
                    if (spObject.m_pObject) {
                        spObject->RemoveGarbageMark();
                    }
                }
                GarbageCollector::Actor3DGarbageA.RemoveAll();
            }
            if (!GarbageCollectorCS::GetFlag(GarbageCollectorCS::GARBAGE_REFERENCES) &&
                pModelLoader->TryLockModelMap()) {
                while (GarbageCollector::ObjectGarbageA.m_usSize) {
                    TESObjectREFR* pRef = GarbageCollector::ObjectGarbageA[0];
                    if (pRef) {
                        delete pRef;
                    }
                }
                GarbageCollector::ObjectGarbageA.RemoveAll();
                pModelLoader->UnlockModelMap();
            }
            if (!GarbageCollectorCS::GetFlag(GarbageCollectorCS::GARBAGE_NAVMESHES)) {
                for (uint32_t k = 0; k < GarbageCollector::kNavMeshes.iSize; ++k) {
                    GarbageCollector::kNavMeshes.GetAt(static_cast<int32_t>(k))->DecRefCount();
                }
                GarbageCollector::kNavMeshes.Clear(true);
            }
            GarbageCollector::bCollectGarbage = bCollectGarbage;
            if (GarbageCollector::bCollectGarbage) {
                GarbageCollector::CopyArrays(
                    &GarbageCollector::kQueuedTESObjectREFRs,
                    &GarbageCollector::ObjectGarbageA
                );
                GarbageCollector::kQueuedTESObjectREFRs.RemoveAll();
                MoveQueuedNiAVObjects(&GarbageCollector::kQueuedNiAVObjects, &GarbageCollector::Actor3DGarbageA);
                GarbageCollector::kQueuedNiAVObjects.RemoveAll();
                GarbageCollector::CopyArrays(&GarbageCollector::kQueuedBipedAnims, &GarbageCollector::kBipedAnims);
                GarbageCollector::kQueuedBipedAnims.RemoveAll();
                MoveQueuedTempEffects(&GarbageCollector::kBSTempEffectArray, &GarbageCollector::EffectGarbageA);
                GarbageCollector::kBSTempEffectArray.RemoveAll();
                MoveQueuedNavMeshes(&GarbageCollector::kQueuedNavMeshes, &GarbageCollector::kNavMeshes);
                GarbageCollector::kQueuedNavMeshes.Clear(true);
            }
            GarbageCollector::bRunning = false;
            GarbageCollector::LeaveCrit(true);
        }
    }
}

void GarbageCollector::Add(Animation* apAnimation) {
    GarbageCollector kCollector;
    kCollector.EnterCS(GarbageCollectorCS::GARBAGE_ANIMATIONS, "GarbageCollector::Add(Animation)");
    apAnimation->ShutdownAllAnimIdles();
    if (apAnimation->GetControllerManager()) {
        apAnimation->GetControllerManager()->DeactivateAll(0.0f);
    }
    NiNode* pSceneRoot = apAnimation->GetSceneRoot();
    if (pSceneRoot) {
        NiTimeController* pkControl = pSceneRoot->GetController(&NiMultiTargetTransformController::ms_RTTI);
        if (pkControl) {
            pSceneRoot->RemoveController(pkControl);
        }
        pkControl = pSceneRoot->GetController(&BSMultiTargetTreadTransfController::ms_RTTI);
        if (pkControl) {
            pSceneRoot->RemoveController(pkControl);
        }
    }
    pModelLoader->CancelReplacementKFList(apAnimation);
    if (!bCollectGarbage || (pSaveLoadGame && pSaveLoadGame->GetSaveGameLoading())) {
        if (apAnimation) {
            delete apAnimation;
        }
    }
    else {
        AnimGarbageA.Add(apAnimation);
    }
    kCollector.LeaveCS();
}

void GarbageCollector::Add(BSTempEffect* apEffect) {
    GarbageCollector kGuard;
    kGuard.EnterCS(GarbageCollectorCS::GARBAGE_TEMP_EFFECTS, "GarbageCollector::Add(BSTempEffect)");
    if (bCollectGarbage) {
        NiPointer<BSTempEffect> spEffect(apEffect);
        EffectGarbageA.Add(spEffect);
    }
    else {
        NiPointer<BSTempEffect> spEffect(apEffect);
        kBSTempEffectArray.Add(spEffect);
    }
    kGuard.LeaveCS();
}

void GarbageCollector::Add(NiAVObject* apObject) {
    GarbageCollector kGuard;
    kGuard.EnterCS(GarbageCollectorCS::GARBAGE_NIAVOBJECTS, "GarbageCollector::Add(NiAVObject)");
    if (apObject) {
        apObject->AddGarbageMark();
    }
    if (bCollectGarbage) {
        NiPointer<NiAVObject> spObject(apObject);
        Actor3DGarbageA.Add(spObject);
    }
    else {
        NiPointer<NiAVObject> spObject(apObject);
        kQueuedNiAVObjects.Add(spObject);
    }
    kGuard.LeaveCS();
}

void GarbageCollector::Add(TESObjectREFR* apRef) {
    GarbageCollector kGuard;
    kGuard.EnterCS(GarbageCollectorCS::GARBAGE_REFERENCES, "GarbageCollector::Add(TESObjectREFR)");
    if (TESForm::GetFormByNumericID(apRef->iFormID) == apRef) {
        apRef->SetDelete(true);
        apRef->Release3DRelatedData();
        if (apRef->IsActor()) {
            static_cast<Actor*>(apRef)->StopCombat(nullptr);
        }
        if (apRef->IsProjectile()) {
            Projectile* pProjectile = static_cast<Projectile*>(apRef);
            pProjectile->pDesiredTarget = nullptr;
            pProjectile->SetFlag(Projectile::ACTIVE_IN_INVENTORY, false);
        }
        pModelLoader->CancelReference(apRef);
        apRef->RemoveFromDataStructures();
        if (apRef->GetObjectReference()->TESForm::GetHasSpokenFlag()) {
            TESWorldSpace* pWorld = apRef->GetWorldSpace();
            if (pWorld && pWorld->GetTerrainManager()) {
                pWorld->GetTerrainManager()->ReleaseTree(apRef);
            }
        }
        apRef->SetParentCell(nullptr);
        if (!bCollectGarbage || (pSaveLoadGame && pSaveLoadGame->GetSaveGameLoading())) {
            if (pModelLoader->TryLockModelMap()) {
                if (apRef) {
                    delete apRef;
                }
                pModelLoader->UnlockModelMap();
            }
            else {
                GarbageCollector::kQueuedTESObjectREFRs.Add(apRef);
            }
        }
        else {
            GarbageCollector::ObjectGarbageA.Add(apRef);
        }
    }
    else {
        if (apRef) {
            delete apRef;
        }
    }
    kGuard.LeaveCS();
}

void GarbageCollector::Add(BipedAnim* apBiped) {
    GarbageCollector kGuard;
    kGuard.EnterCS(GarbageCollectorCS::GARBAGE_BIPED_ANIMS, "GarbageCollector::Add(BipedAnim)");
    if (!bCollectGarbage || (pSaveLoadGame && pSaveLoadGame->GetSaveGameLoading())) {
        if (pModelLoader->TryLockModelMap()) {
            if (apBiped) {
                delete apBiped;
            }
            pModelLoader->UnlockModelMap();
        }
        else {
            kQueuedBipedAnims.Add(apBiped);
        }
    }
    else {
        kBipedAnims.Add(apBiped);
    }
    kGuard.LeaveCS();
}

void GarbageCollector::EnterCrit(const char* apName, bool) {
    CritSec.Lock(apName);
}

bool GarbageCollector::TryEnterCrit() {
    return CritSec.TryLock();
}

void GarbageCollector::LeaveCrit(bool) {
    CritSec.Unlock();
}

void GarbageCollector::Remove(TESObjectREFR* apRef) {
    EnterCrit("GarbageCollector::Remove(TESObjectREFR)", true);
    uint32_t uiSize = ObjectGarbageA.GetSize();
    for (uint32_t i = 0; i < uiSize; ++i) {
        if (ObjectGarbageA[i] == apRef) {
            RemoveAtSwapLast(&ObjectGarbageA, i);
            break;
        }
    }
    uiSize = kQueuedTESObjectREFRs.GetSize();
    for (uint32_t i = 0; i < uiSize; ++i) {
        if (kQueuedTESObjectREFRs[i] == apRef) {
            RemoveAtSwapLast(&kQueuedTESObjectREFRs, i);
            break;
        }
    }
    LeaveCrit(true);
}

void GarbageCollector::ClearTempEffects() {
    if (EffectGarbageA.GetSize() && pModelLoader->TryLockModelMap()) {
        EnterCrit("GarbageCollector::ClearTempEffects()", true);
        EffectGarbageA.RemoveAll();
        pModelLoader->UnlockModelMap();
        LeaveCrit(true);
    }
}

void GarbageCollector::CopyArrays(NiTObjectArray<TESObjectREFR*>* apSource, NiTObjectArray<TESObjectREFR*>* apTarget) {
    for (TESObjectREFR* pElement : *apSource) {
        if (pElement) {
            apTarget->Add(pElement);
        }
    }
}

void GarbageCollector::CopyArrays(NiTObjectArray<BipedAnim*>* apSource, NiTObjectArray<BipedAnim*>* apTarget) {
    for (BipedAnim* pElement : *apSource) {
        if (pElement) {
            apTarget->Add(pElement);
        }
    }
}

void GarbageCollector::CanRunGarbageCollector(bool abFlagSet) {
    bCollectGarbage = abFlagSet;
}

bool GarbageCollectorCS::GetFlag(uint32_t auiFlag) {
    return (auiFlag & uiFlags) != 0;
}

uint32_t NiAVObject::AddGarbageMark() {
    return au_re_InterlockedExchange(reinterpret_cast<volatile int32_t*>(&m_uiRefCount), m_uiRefCount | 0x40000000);
}

uint32_t NiAVObject::RemoveGarbageMark() {
    return au_re_InterlockedExchange(reinterpret_cast<volatile int32_t*>(&m_uiRefCount), m_uiRefCount & ~0x40000000u);
}
