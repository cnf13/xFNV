#include "Fallout Shared/BGSDestructibleObjectForm.h"

#include "BSCore/BSRandom.h"
#include "BSCore/BSTimer.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSHavok/bhkCollisionFilter.h"
#include "BSHavok/bhkCollisionObject.h"
#include "BSHavok/bhkRigidBody.h"
#include "BSHavok/bhkWorld.h"
#include "BSHavok/bhkWorldObject.h"
#include "BSMain/BSBlastNode.h"
#include "BSMain/BSDamageStage.h"
#include "BSMain/BSDebrisNode.h"
#include "BSMain/BSRangeNode.h"
#include "BSMain/IOManager.h"
#include "BSMain/QueuedFiles.h"
#include "BSMain/Setting.h"
#include "BSShader/BSFadeNode.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/BGSDebris.h"
#include "Fallout Shared/BGSExplosion.h"
#include "Fallout Shared/BGSMovableStatic.h"
#include "Fallout Shared/BGSProjectile.h"
#include "Fallout Shared/BGSTalkingActivator.h"
#include "Fallout Shared/BGSTerminal.h"
#include "Fallout Shared/BaseFormComponent.h"
#include "Fallout Shared/CombatFormulas.h"
#include "Fallout Shared/ExtraDataList.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/IngredientItem.h"
#include "Fallout Shared/ModelLoader.h"
#include "Fallout Shared/Script.h"
#include "Fallout Shared/TES.h"
#include "Fallout Shared/TESActorBase.h"
#include "Fallout Shared/TESAmmo.h"
#include "Fallout Shared/TESCreature.h"
#include "Fallout Shared/TESFile.h"
#include "Fallout Shared/TESFlora.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESFurniture.h"
#include "Fallout Shared/TESLight.h"
#include "Fallout Shared/TESModel.h"
#include "Fallout Shared/TESModelTextureSwap.h"
#include "Fallout Shared/TESNPC.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectACTI.h"
#include "Fallout Shared/TESObjectARMO.h"
#include "Fallout Shared/TESObjectBOOK.h"
#include "Fallout Shared/TESObjectCELL.h"
#include "Fallout Shared/TESObjectCLOT.h"
#include "Fallout Shared/TESObjectCONT.h"
#include "Fallout Shared/TESObjectDOOR.h"
#include "Fallout Shared/TESObjectMISC.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESObjectTREE.h"
#include "Fallout Shared/TESObjectWEAP.h"
#include "Fallout Shared/TESScript.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/Combat/CombatManager.h"
#include "Fallout/AI/Combat/CombatThreatMap.h"
#include "Fallout/AI/Combat/Explosion.h"
#include "Fallout/AI/Combat/Projectile.h"
#include "Fallout/AI/FixedStrings.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadGame.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAVObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiBound.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiFixedString.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPoint3.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiPointer.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMap.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiTMapBase.h"
#include "Gamebryo2.2/CoreLibs/NiSystem/NiSystem.h"
#include "SDK/MSVC/rtti.h"
#include "xfnv/Compiler.h"

DestructibleRefsTakingDamageMap BGSDestructibleObjectForm::DestructibleObjects(37);

float RandomFloat(float afMin, float afMax) {
    return BSRandom::GetSingleton()->Float(afMin, afMax);
}

NiFixedString* get_g_ModelSwapNode() {
    return FixedStrings::ModelSwapNode;
}

// Swaps the four 4-byte fields read from a big-endian file; the four leading bytes are left alone.
void SAVE_DESTRUCTIBLE_OBJECT_STAGE::Endian() {
    ENDIAN(iSelfDamagePerSecond, false);
    ENDIAN(iExplosionID, false);
    ENDIAN(iDebrisID, false);
    ENDIAN(iDebrisCount, false);
}

// Sets the health, the stage count and the flags; the two padding bytes stay unwritten.
SAVE_DESTRUCTIBLE_OBJECT_DATA::SAVE_DESTRUCTIBLE_OBJECT_DATA() : iHealth(0), cNumStages(0), cFlags(0) {}

// Swaps the health read from a big-endian file; the stage count and flags are bytes.
void SAVE_DESTRUCTIBLE_OBJECT_DATA::Endian() {
    ENDIAN(iHealth, false);
}

DestructibleObjectStage::DestructibleObjectStage()
  : cModelDamageStage(0), cHealthPercentage(0), cFlags(0), iSelfDamagePerSecond(0), pExplosion(nullptr),
    pDebris(nullptr), iDebrisCount(0), pReplacementModel(nullptr) {}

DestructibleObjectStage::~DestructibleObjectStage() {
    if (pReplacementModel) {
        delete pReplacementModel;
    }
}

bool DestructibleObjectStage::GetDisableObject() {
    return (cFlags & FLAG_DISABLE_OBJECT) != 0;
}

bool DestructibleObjectStage::GetCapDamage() {
    return (cFlags & FLAG_CAP_DAMAGE) != 0;
}

bool DestructibleObjectStage::GetDestroyObject() {
    return (cFlags & FLAG_DESTROY_OBJECT) != 0;
}

void DestructibleObjectStage::Copy(DestructibleObjectStage* apCopy) {
    if (apCopy) {
        cHealthPercentage = apCopy->cHealthPercentage;
        iSelfDamagePerSecond = apCopy->iSelfDamagePerSecond;
        cModelDamageStage = apCopy->cModelDamageStage;
        cFlags = apCopy->cFlags;
        pExplosion = apCopy->pExplosion;
        pDebris = apCopy->pDebris;
        iDebrisCount = apCopy->iDebrisCount;
        if (pReplacementModel) {
            delete pReplacementModel;
            pReplacementModel = nullptr;
        }
        if (apCopy->pReplacementModel) {
            pReplacementModel = new TESModelTextureSwap;
            pReplacementModel->CopyComponent(apCopy->pReplacementModel);
        }
    }
}

void DestructibleObjectStage::CreateReplacementModel() {
    if (!pReplacementModel) {
        pReplacementModel = new TESModelTextureSwap;
    }
}

DestructibleObjectData::DestructibleObjectData()
  : iHealth(0), cNumStages(0), cFlags(0), pStagesArray(nullptr), iReplacementModelRefCount(0),
    spPreloadedReplacementModels(nullptr) {}

DestructibleObjectData::~DestructibleObjectData() {
    for (uint32_t i = 0; i < cNumStages; ++i) {
        if (pStagesArray[i]) {
            delete pStagesArray[i];
        }
    }
    ourFree(pStagesArray);
}

BGSDestructibleObjectForm::BGSDestructibleObjectForm() : BaseFormComponent() {
    BGSDestructibleObjectForm::InitializeDataComponent();
}

BGSDestructibleObjectForm::~BGSDestructibleObjectForm() {
    BGSDestructibleObjectForm::ClearDataComponent();
}

void BGSDestructibleObjectForm::InitializeDataComponent() {
    pData = nullptr;
}

void BGSDestructibleObjectForm::ClearDataComponent() {
    if (pData) {
        delete pData;
    }
    pData = nullptr;
}

void BGSDestructibleObjectForm::CopyComponent(BaseFormComponent* apCopyFC) {
    BGSDestructibleObjectForm* pCopy = dynamic_cast<BGSDestructibleObjectForm*>(apCopyFC);
    if (!pCopy) {
        return;
    }
    DestructibleObjectData* pCopyData = pCopy->GetData();
    if (!pCopyData) {
        if (pData) {
            delete pData;
        }
        pData = nullptr;
        return;
    }
    if (!pData) {
        pData = new DestructibleObjectData;
    }
    else {
        for (uint32_t i = 0; i < pData->cNumStages; ++i) {
            DestructibleObjectStage* pStage = pData->pStagesArray[i];
            if (pStage) {
                delete pStage;
            }
        }
        ourFree(pData->pStagesArray);
        pData->pStagesArray = nullptr;
    }
    pData->cFlags = pCopyData->cFlags;
    pData->iHealth = pCopyData->iHealth;
    pData->cNumStages = pCopyData->cNumStages;
    if (pData->cNumStages) {
        pData->pStagesArray = AllocateStages(pData->cNumStages);
        for (uint32_t j = 0; j < pData->cNumStages; ++j) {
            pData->pStagesArray[j]->Copy(pCopyData->pStagesArray[j]);
        }
    }
}

bool BGSDestructibleObjectForm::CompareComponent(BaseFormComponent* apCompareFC) {
    BGSDestructibleObjectForm* pcompare = dynamic_cast<BGSDestructibleObjectForm*>(apCompareFC);
    if (!pcompare) {
        return true;
    }
    DestructibleObjectData* pdata = pcompare->pData;
    if (!pdata || !pData) {
        return pdata || pData;
    }
    if (pData->iHealth != pdata->iHealth) {
        return true;
    }
    if (pData->cFlags != pdata->cFlags) {
        return true;
    }
    if (pData->cNumStages != pdata->cNumStages) {
        return true;
    }
    for (uint32_t i = 0; i < pData->cNumStages; ++i) {
        if (pData->pStagesArray[i]->cHealthPercentage != pdata->pStagesArray[i]->cHealthPercentage) {
            return true;
        }
        if (pData->pStagesArray[i]->iSelfDamagePerSecond != pdata->pStagesArray[i]->iSelfDamagePerSecond) {
            return true;
        }
        if (pData->pStagesArray[i]->cModelDamageStage != pdata->pStagesArray[i]->cModelDamageStage) {
            return true;
        }
        if (pData->pStagesArray[i]->cFlags != pdata->pStagesArray[i]->cFlags) {
            return true;
        }
        if (pData->pStagesArray[i]->pExplosion != pdata->pStagesArray[i]->pExplosion) {
            return true;
        }
        if (pData->pStagesArray[i]->pDebris != pdata->pStagesArray[i]->pDebris) {
            return true;
        }
        // The game's check compares one form's iDebrisCount with itself (a bug in the original), which is always equal.
        if ((pData->pStagesArray[i]->pReplacementModel && !pdata->pStagesArray[i]->pReplacementModel) ||
            (!pData->pStagesArray[i]->pReplacementModel && pdata->pStagesArray[i]->pReplacementModel)) {
            return true;
        }
        if (pData->pStagesArray[i]->pReplacementModel &&
            (pdata->pStagesArray[i]->pReplacementModel &&
             pData->pStagesArray[i]->pReplacementModel->CompareComponent(pdata->pStagesArray[i]->pReplacementModel))) {
            return true;
        }
    }
    return false;
}

void BGSDestructibleObjectForm::FindModelSwapNodes(NiAVObject* apRoot, BSSimpleArray<NiNode*, 1024>& arNodeArray) {
    if (apRoot) {
        NiNode* pNode = apRoot->IsNode();
        if (pNode) {
            if (pNode->m_kName.GetHandleString()) {
                NiFixedString* pModelSwapNode = get_g_ModelSwapNode();
                if (NiFixedString::HandlesEqual(pNode->m_kName, *pModelSwapNode)) {
                    arNodeArray.Add(pNode);
                }
            }
            for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
                NiAVObject* pChild = pNode->GetAt(i);
                FindModelSwapNodes(pChild, arNodeArray);
            }
        }
    }
}

bool BGSDestructibleObjectForm::UpdateCurrentDamageStage(TESObjectREFR* apRef, bool abForceUpdate) {
    if (!apRef->Get3D()) {
        return false;
    }
    ExtraDataList* pExtra = apRef->GetExtra();
    float fObjectHealth = pExtra->GetObjectHealth();
    uint32_t uiDamageStage = 0;
    if (fObjectHealth == -1.0) {
        if (!abForceUpdate) {
            return false;
        }
    }
    else {
        float fHealthPercent = fObjectHealth * 100.0 / static_cast<double>(pData->iHealth);
        uint8_t ucHealthPercent = static_cast<int32_t>(FloatCeil(fHealthPercent));
        for (uint32_t i = 0; i < pData->cNumStages; ++i) {
            DestructibleObjectStage* pStage = GetStage(i);
            if (pStage->cHealthPercentage < static_cast<int32_t>(ucHealthPercent)) {
                break;
            }
            if (pStage->cModelDamageStage) {
                uiDamageStage = pStage->cModelDamageStage;
            }
        }
    }
    if (!uiDamageStage && !abForceUpdate) {
        return false;
    }
    NiNode* p3D = apRef->Get3D();
    BGSDestructibleObjectForm::UpdateDamageStageNodes(p3D, uiDamageStage);
    return true;
}

// Applies afDamage to apRef (an actor takes its health actor value instead) and runs every destruction stage the health
// percentage crosses: disabling, destroying, capping the damage, swapping in the replacement model (moved so that its
// model swap node stays where the old one was), showing the damage stage nodes, the self damage, and then per crossed
// stage the OnDestructionStageChange event, the debris and the explosion. Last, the combat threat map learns whether
// the object is now a dangerous explosive. abSelfDamage is never read. The percentages are the ceiled float percent
// truncated to an int, of which the low byte is kept.
void BGSDestructibleObjectForm::DamageObject(TESObjectREFR* apRef, float afDamage, [[maybe_unused]] bool abSelfDamage) {
    if (!pData || !apRef || afDamage <= 0.0 || apRef->GetDisabled() || apRef->GetDelete()) {
        return;
    }
    if (!apRef->Get3D()) {
        return;
    }
    uint32_t uiMaxHealth = GetMaxHealth(apRef);
    float fHealth = apRef->GetExtra()->GetObjectHealth();
    if (fHealth == -1.0) {
        fHealth = static_cast<float>(uiMaxHealth);
    }
    if (fHealth == 0.0) {
        return;
    }
    float fOldPercent = fHealth * 100.0 / uiMaxHealth;
    uint8_t cOldPercent = static_cast<uint8_t>(static_cast<int32_t>(FloatCeil(fOldPercent)));
    float fNewHealth = fHealth - afDamage;
    if (apRef->IsActor()) {
        ActorValueOwner* pOwner = static_cast<Actor*>(apRef);
        fNewHealth = static_cast<float>(pOwner->GetActorValue(ActorValue::HEALTH));
    }
    if (fNewHealth <= 0.0) {
        fNewHealth = 0.0f;
    }
    float fNewPercent = fNewHealth * 100.0 / uiMaxHealth;
    uint8_t cNewPercent = static_cast<uint8_t>(static_cast<int32_t>(FloatCeil(fNewPercent)));
    if (!cNewPercent && fNewHealth > 0.0) {
        cNewPercent = 1;
    }
    NiPointer<NiNode> spRoot(apRef->Get3D());
    TESModelTextureSwap* pModelSwap = nullptr;
    int32_t iSelfDamage = 0;
    uint32_t uiFirstStage = 0;
    uint32_t uiLastStage = 0;
    uint32_t uiOldModelStage = 0;
    uint32_t uiNewModelStage = 0;
    int32_t iOldSelfDamage = 0;
    // The stages the damage crosses (below the old percentage, at or above the new one) run from uiFirstStage to
    // uiLastStage; the stages above both percentages give the model stage and self damage the object had.
    for (uint32_t i = 0; i < pData->cNumStages; ++i) {
        DestructibleObjectStage* pStage = GetStage(i);
        if (pStage->cHealthPercentage < cOldPercent && pStage->cHealthPercentage >= cNewPercent) {
            if (pStage->cModelDamageStage) {
                uiNewModelStage = pStage->cModelDamageStage;
            }
            if (pStage->pReplacementModel) {
                pModelSwap = pStage->pReplacementModel;
            }
            iSelfDamage = pStage->iSelfDamagePerSecond;
            uiLastStage = i + 1;
            if (pStage->GetDisableObject()) {
                if (apRef->IsProjectile()) {
                    static_cast<Projectile*>(static_cast<MobileObject*>(apRef))->Kill();
                }
                else {
                    Script::AddPendingDisabledReference(apRef, false);
                    apRef->TESForm::SetDelete(true);
                }
            }
            if (pStage->GetDestroyObject()) {
                apRef->SetDestroyed(true);
            }
            if (pStage->GetCapDamage()) {
                fNewHealth = pStage->cHealthPercentage * uiMaxHealth / 100.0;
                cNewPercent = pStage->cHealthPercentage;
                break;
            }
        }
        else if (cOldPercent <= pStage->cHealthPercentage && cNewPercent < pStage->cHealthPercentage) {
            uiFirstStage = i + 1;
            if (pStage->cModelDamageStage) {
                uiOldModelStage = pStage->cModelDamageStage;
            }
            iOldSelfDamage = pStage->iSelfDamagePerSecond;
        }
    }
    if (fNewHealth <= 0.0) {
        apRef->SetDestroyed(true);
    }
    apRef->GetExtra()->SetObjectHealth(fNewHealth);
    apRef->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
    if (uiLastStage > uiFirstStage) {
        if (!apRef->GetDisabled()) {
            if (pModelSwap) {
                alignas(BSSimpleArray<NiNode*>) uint8_t aOldNodesStorage[sizeof(BSSimpleArray<NiNode*>)];
                BSSimpleArray<NiNode*>* pOldNodes = ::new (aOldNodesStorage) BSSimpleArray<NiNode*>;
                NiTransform kOldTransform;
                FindModelSwapNodes(apRef->Get3D(), *pOldNodes);
                if (pOldNodes->GetSize()) {
                    kOldTransform = (*pOldNodes)[0]->GetWorldTransform();
                }
                bool bUnloadModels = false;
                if (pData->spPreloadedReplacementModels) {
                    NiAtomicIncrement(pData->iReplacementModelRefCount);
                    bUnloadModels = true;
                }
                bool bUnderwater = apRef->QUnderwater();
                bool bAllowChanges = pSaveLoadGame->SetThreadAllowChanges(false);
                apRef->Set3D(nullptr, true);
                if (bUnderwater) {
                    apRef->SetUnderwater(true);
                }
                pSaveLoadGame->SetThreadAllowChanges(bAllowChanges);
                TESBoundObject* pObject = apRef->GetObjectReference();
                apRef->GetExtra()->SetModelSwap(pModelSwap, pObject);
                apRef->AddChange(CHANGE_REFR_EXTRA_GAME_ONLY);
                TES::pInstance->AttachModel(apRef, apRef->GetParentCell(), nullptr, false);
                if (apRef->Get3D()) {
                    NiNode* pNewRoot = apRef->Get3D();
                    NiNode* pNode = pNewRoot ? pNewRoot->IsNode() : nullptr;
                    BSFadeNode* pFadeNode = pNode ? pNode->IsFadeNode() : nullptr;
                    if (pFadeNode) {
                        pFadeNode->TurnFadeNodeOn();
                    }
                    // Keeps the first model swap node where the old model had it: the reference turns by the old
                    // node's rotation times the inverse of the new one's, and moves by the difference of their
                    // translations.
                    if (pOldNodes->GetSize()) {
                        alignas(BSSimpleArray<NiNode*>) uint8_t aNewNodesStorage[sizeof(BSSimpleArray<NiNode*>)];
                        BSSimpleArray<NiNode*>* pNewNodes = ::new (aNewNodesStorage) BSSimpleArray<NiNode*>;
                        FindModelSwapNodes(pNewRoot, *pNewNodes);
                        if (pNewNodes->GetSize()) {
                            NiTransform kNewTransform = (*pNewNodes)[0]->GetWorldTransform();
                            NiMatrix3 kInverse = kNewTransform.m_Rotate.Transpose();
                            NiMatrix3 kOldRotate = kOldTransform.m_Rotate;
                            NiMatrix3 kOrientation = apRef->GetOrientation();
                            apRef->SetOrientation(kOldRotate * kInverse * kOrientation);
                            apRef->Update3DPosition();
                            kNewTransform = (*pNewNodes)[0]->GetWorldTransform();
                            NiPoint3 kLocation = kOldTransform.m_Translate - kNewTransform.m_Translate;
                            kLocation += apRef->GetLocationOnReference();
                            apRef->SetLocationOnReference(kLocation);
                            apRef->Update3DPosition();
                        }
                        pNewNodes->~BSSimpleArray<NiNode*>();
                    }
                    bhkWorld::Activate(pNewRoot, true, true, false);
                }
                if (bUnloadModels) {
                    UnloadReplacementModels();
                }
                pOldNodes->~BSSimpleArray<NiNode*>();
            }
            else if (uiNewModelStage > uiOldModelStage) {
                UpdateDamageStageNodes(apRef->TESObjectREFR::Get3D(), uiNewModelStage);
                apRef->UpdateAddonNodeSounds(false);
            }
        }
        if (iSelfDamage && fNewHealth > 0.0) {
            BGSDestructibleObjectForm::DestructibleObjects.SetAt(apRef, iSelfDamage);
            iOldSelfDamage = iSelfDamage;
        }
        else {
            BGSDestructibleObjectForm::DestructibleObjects.RemoveAt(apRef);
        }
    }
    for (uint32_t i = uiFirstStage; i < uiLastStage; ++i) {
        DestructibleObjectStage* pStage = GetStage(i);
        // The OnDestructionStageChange event flag.
        Script::SetActionFlag(apRef, apRef->GetExtra(), SCRIPT_ACTION_ONDESTRUCTIONSTAGECHANGE);
        uint32_t uiModelStage = 0;
        if (uiNewModelStage) {
            uiModelStage = uiNewModelStage;
        }
        else if (uiOldModelStage) {
            uiModelStage = uiOldModelStage;
        }
        if (pStage->pDebris &&
            pStage->iDebrisCount &&
            !SpawnDebrisAtDebrisNodes(spRoot, pStage->pDebris, uiModelStage, apRef, pStage->iDebrisCount)) {
            NiPoint3 kLocation = apRef->GetLocationOnReference();
            float fScale = apRef->GetScale();
            TESObjectCELL* pCell = apRef->GetParentCell();
            pStage->pDebris->SpawnDebris(pCell, kLocation, pStage->iDebrisCount, fScale);
        }

        if (pStage->pExplosion && !SpawnExplosionsAtBlastNodes(spRoot, pStage->pExplosion, uiModelStage, apRef)) {
            // No blast nodes: the explosion goes off at a random point of the lower half of the bound.
            NiMatrix3 kRotation = apRef->GetOrientation();
            NiBound kBound = spRoot->GetWorldBound();
            NiPoint3 kOffset;
            kOffset.x = RandomFloat(-1.0f, 1.0f);
            kOffset.y = RandomFloat(-1.0f, 1.0f);
            kOffset.z = RandomFloat(-1.0f, 0.0f);
            kOffset *= kBound.m_fRadius;
            NiPoint3 kOrigin = kBound.GetCenter() + kOffset;
            if (apRef->IsProjectile()) {
                Projectile* pProjectile = static_cast<Projectile*>(static_cast<MobileObject*>(apRef));
                TESObjectCELL* pCell = apRef->GetParentCell();
                TESObjectREFR* pShooter = pProjectile->GetShooter();
                Explosion* pExplosion = Explosion::SpawnExplosion(
                    pStage->pExplosion,
                    pShooter,
                    nullptr,
                    pCell,
                    kOrigin,
                    kRotation
                );
                if (pExplosion && pProjectile->GetWeaponSource()) {
                    pExplosion->SetWeaponSource(pProjectile->GetWeaponSource());
                }
            }
            else {
                TESObjectCELL* pCell = apRef->GetParentCell();
                Explosion::SpawnExplosion(pStage->pExplosion, apRef, apRef, pCell, kOrigin, kRotation);
            }
        }
    }
    if (fNewHealth == 0.0) {
        UnloadReplacementModels();
    }
    BGSExplosion* pExplosion = nullptr;
    float fThreatTime = -1.0f;
    if (iOldSelfDamage) {
        float fRequiredDamage = fDangerousObjectExplosionDamage.Float();
        float fRequiredRadius = fDangerousObjectExplosionRadius.Float();
        fThreatTime = CalculateDangerousExplosionTerminalTime(
            cNewPercent,
            fRequiredRadius,
            fRequiredDamage,
            &pExplosion
        );
        if (fCombatThreatTimedExplosionLength.Float() < fThreatTime) {
            fThreatTime = -1.0f;
        }
    }
    if (fThreatTime == -1.0) {
        float fRequiredDamage = fDangerousObjectExplosionDamage.Float();
        float fRequiredRadius = fDangerousObjectExplosionRadius.Float();
        float fExplosionHealth = CalculateDangerousExplosionHealth(
            cNewPercent,
            fRequiredRadius,
            fRequiredDamage,
            &pExplosion
        );
        if (fExplosionHealth != -1.0) {
            // The remaining health is kept as a double.
            double fRemainingHealth = static_cast<double>(fNewHealth) - static_cast<double>(fExplosionHealth);
            if (fCombatThreatDangerousObjectHealth.Float() > fRemainingHealth) {
                fThreatTime = fCombatThreatExplosiveObjectThreatTime.Float();
            }
        }
    }
    if (fThreatTime != -1.0) {
        pCombatManager->GetThreatMap()->AddThreat(apRef, pExplosion, fThreatTime);
    }
    else {
        pCombatManager->GetThreatMap()->RemoveThreat(apRef);
    }
}

uint32_t BGSDestructibleObjectForm::GetMaxHealth(TESObjectREFR* apRef) {
    if (apRef->IsActor()) {
        // Downcasts to Actor and reads the health through its ActorValueOwner base's float getter, with no null test.
        ActorValueOwner& rOwner = *static_cast<Actor*>(apRef);
        return static_cast<int64_t>(rOwner.GetBaseActorFloatValue(ActorValue::HEALTH));
    }

    return pData->iHealth;
}

uint32_t BGSDestructibleObjectForm::SpawnExplosionsAtBlastNodes(
    NiAVObject* apRoot,
    BGSExplosion* apExplosion,
    uint32_t aiStageIndex,
    TESObjectREFR* apRef
) {
    if (!apRoot) {
        return 0;
    }
    uint32_t uiNumExplosions = 0;
    if (NiObject::IsKindOf_0(&BSBlastNode::ms_RTTI, apRoot)) {
        BSBlastNode* pBlastNode = static_cast<BSBlastNode*>(apRoot);
        if (static_cast<BSRangeNode*>(apRoot)->BSRangeNode::SetCurrent(aiStageIndex)) {
            NiMatrix3 kRotate = pBlastNode->GetWorldTransform().m_Rotate;
            NiPoint3 kTranslate = pBlastNode->GetWorldTranslate();
            TESObjectCELL* pParentCell = apRef->GetParentCell();
            Explosion::SpawnExplosion(apExplosion, apRef, apRef, pParentCell, kTranslate, kRotate);
            ++uiNumExplosions;
        }
    }
    NiNode* pNode = apRoot->IsNode();
    if (pNode) {
        for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
            NiAVObject* pChild = pNode->GetAt(i);
            uiNumExplosions += this->SpawnExplosionsAtBlastNodes(pChild, apExplosion, aiStageIndex, apRef);
        }
    }
    return uiNumExplosions;
}

uint32_t BGSDestructibleObjectForm::SpawnDebrisAtDebrisNodes(
    NiAVObject* apRoot,
    BGSDebris* apDebris,
    uint32_t aiStageIndex,
    TESObjectREFR* apRef,
    uint32_t aiNumParticles
) {
    if (!apRoot) {
        return 0;
    }
    uint32_t uiCount = 0;
    if (NiObject::IsKindOf_0(&BSDebrisNode::ms_RTTI, apRoot) &&
        static_cast<BSRangeNode*>(apRoot)->SetCurrent(aiStageIndex)) {
        float fScale = apRef->GetScale();
        const NiPoint3& kWorldTranslate = apRoot->GetWorldTranslate();
        TESObjectCELL* pParentCell = apRef->TESObjectREFR::GetParentCell();
        apDebris->SpawnDebris(pParentCell, kWorldTranslate, aiNumParticles, fScale);
        uiCount = 1;
    }
    NiNode* pNode = apRoot->IsNode();
    if (pNode) {
        for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
            NiAVObject* pChild = pNode->GetAt(i);
            uiCount += SpawnDebrisAtDebrisNodes(
                static_cast<NiNode*>(pChild),
                apDebris,
                aiStageIndex,
                apRef,
                aiNumParticles
            );
        }
    }
    return static_cast<int32_t>(uiCount);
}

float BGSDestructibleObjectForm::CalculateDangerousExplosionHealth(
    uint8_t acCurrentHealthPercentage,
    float afRequiredRadius,
    float afRequiredDamage,
    BGSExplosion** apOutExplosion
) {
    DestructibleObjectStage* pStage;
    for (uint32_t i = 0;; ++i) {
        if (i >= pData->cNumStages) {
            return -1.0f;
        }
        pStage = pData->pStagesArray[i];
        if (pStage->cHealthPercentage < static_cast<int32_t>(acCurrentHealthPercentage)) {
            if (pStage->pExplosion &&
                static_cast<double>(afRequiredRadius) <= pStage->pExplosion->GetRadiusBSUnits() &&
                static_cast<double>(afRequiredDamage) <= pStage->pExplosion->GetDamage()) {
                break;
            }
        }
    }
    float fHealth = static_cast<double>(pStage->cHealthPercentage) / 100.0 * static_cast<double>(pData->iHealth);
    if (apOutExplosion) {
        *apOutExplosion = pStage->pExplosion;
    }
    return fHealth;
}

void BGSDestructibleObjectForm::InitItem(TESForm* apOwner) {
    if (pData) {
        for (uint32_t i = 0; i < pData->cNumStages; ++i) {
            if (pData->pStagesArray[i]->pExplosion) {
                uint32_t itempint = static_cast<uint32_t>(
                    reinterpret_cast<uintptr_t>(pData->pStagesArray[i]->pExplosion)
                );
                TESFile* pFile = apOwner->GetFile(-1);
                TESForm::AddCompileIndex(itempint, pFile);
                TESForm* pForm = TESForm::GetFormByNumericID(itempint);
                BGSExplosion* pExplosion = dynamic_cast<BGSExplosion*>(pForm);
                pData->pStagesArray[i]->pExplosion = pExplosion;
                if (!pData->pStagesArray[i]->pExplosion) {
                    if (apOwner) {
                        if (apOwner->GetFormEditorIDLength()) {
                            const char* pEditorID = apOwner->GetFormEditorID();
                            Warning(
                                "MASTERFILE: Unable to find stage %i explosion (%08X) on owner object \"%s\".",
                                i,
                                itempint,
                                pEditorID
                            );
                        }
                        else {
                            int32_t iFormID = apOwner->iFormID;
                            Warning(
                                "MASTERFILE: Unable to find stage %i explosion (%08X) on owner object (%08X).",
                                i,
                                itempint,
                                iFormID
                            );
                        }
                    }
                    else {
                        Warning("MASTERFILE: Unable to find stage %i explosion (%08X) on unknown owner.", i, itempint);
                    }
                }
            }
            if (pData->pStagesArray[i]->pDebris) {
                uint32_t itempint = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pData->pStagesArray[i]->pDebris));
                TESFile* pFile = apOwner->GetFile(-1);
                TESForm::AddCompileIndex(itempint, pFile);
                TESForm* pForm = TESForm::GetFormByNumericID(itempint);
                BGSDebris* pDebris = dynamic_cast<BGSDebris*>(pForm);
                pData->pStagesArray[i]->pDebris = pDebris;
                if (!pData->pStagesArray[i]->pDebris) {
                    if (apOwner) {
                        if (apOwner->GetFormEditorIDLength()) {
                            const char* pEditorID = apOwner->GetFormEditorID();
                            Warning(
                                "MASTERFILE: Unable to find stage %i debris (%08X) on owner object \"%s\".",
                                i,
                                itempint,
                                pEditorID
                            );
                        }
                        else {
                            int32_t iFormID = apOwner->iFormID;
                            Warning(
                                "MASTERFILE: Unable to find stage %i debris (%08X) on owner object (%08X).",
                                i,
                                itempint,
                                iFormID
                            );
                        }
                    }
                    else {
                        Warning("MASTERFILE: Unable to find stage %i debris (%08X) on unknown owner.", i, itempint);
                    }
                }
            }
            if (pData->pStagesArray[i]->pReplacementModel) {
                pData->pStagesArray[i]->pReplacementModel->InitItem(apOwner);
            }
        }
        int32_t iFormType = apOwner->GetFormType();
        if (iFormType >= 42 && iFormType <= 43) {
            pData->iHealth = static_cast<TESActorBase*>(apOwner)->TESActorBase::GetHealth();
        }
    }
}

void BGSDestructibleObjectForm::Save() {
    SAVE_DESTRUCTIBLE_OBJECT_STAGE stageSaveData;
    if (pData) {
        SAVE_DESTRUCTIBLE_OBJECT_DATA saveData;
        // Cleared first (a departure), as each stage's record is: the DEST chunk copies all 8 bytes, and the exe
        // leaves the two padding bytes after cFlags unset.
        ourMemset(&saveData, 0, sizeof(saveData));
        saveData.iHealth = pData->iHealth;
        saveData.cNumStages = pData->cNumStages;
        saveData.cFlags = pData->cFlags;
        if (TESForm::QEndianSwapOnSave()) {
            saveData.Endian();
        }
        TESForm::__AddChunkData(DEST_ID, &saveData, 8u);
        if (TESForm::QEndianSwapOnSave()) {
            saveData.Endian();
        }
        for (uint32_t i = 0; i < pData->cNumStages; ++i) {
            ourMemset(&stageSaveData, 0, sizeof(stageSaveData));
            stageSaveData.cStageNum = i;
            stageSaveData.cHealthPercentage = pData->pStagesArray[i]->cHealthPercentage;
            stageSaveData.iSelfDamagePerSecond = pData->pStagesArray[i]->iSelfDamagePerSecond;
            stageSaveData.cModelDamageStage = pData->pStagesArray[i]->cModelDamageStage;
            stageSaveData.cFlags = pData->pStagesArray[i]->cFlags;
            stageSaveData.iDebrisCount = pData->pStagesArray[i]->iDebrisCount;
            if (pData->pStagesArray[i]->pExplosion) {
                stageSaveData.iExplosionID = pData->pStagesArray[i]->pExplosion->iFormID;
            }
            if (pData->pStagesArray[i]->pDebris) {
                stageSaveData.iDebrisID = pData->pStagesArray[i]->pDebris->iFormID;
            }
            if (TESForm::QEndianSwapOnSave()) {
                stageSaveData.Endian();
            }
            TESForm::__AddChunkData(DSTD_ID, &stageSaveData, 20u);
            if (TESForm::QEndianSwapOnSave()) {
                stageSaveData.Endian();
            }
            if (pData->pStagesArray[i]->pReplacementModel) {
                pData->pStagesArray[i]->pReplacementModel->Save(DMDL_ID, DMDT_ID, DMDS_ID);
            }
            TESForm::AddChunk(DSTF_ID);
        }
    }
}

bool BGSDestructibleObjectForm::HasValidStages() {
    if (!pData) {
        return false;
    }
    for (uint32_t i = 0; i < pData->cNumStages; ++i) {
        DestructibleObjectStage* pStage = pData->pStagesArray[i];
        if (pStage &&
            (pStage->pReplacementModel || pStage->pExplosion || pStage->pDebris || pStage->GetDisableObject())) {
            return true;
        }
    }
    return false;
}

void BGSDestructibleObjectForm::QueueFiles(
    TESBoundObject* apObject,
    IO_TASK_PRIORITY aePriority,
    QueuedFile* apParent
) {
    if (!pData) {
        return;
    }
    for (uint32_t i = 0; i < pData->cNumStages; ++i) {
        if (pData->pStagesArray[i]->pReplacementModel) {
            ENUM_LOD_MULT eLODMult = TES::GetLODMult(apObject);
            pModelLoader->QueueModel(
                pData->pStagesArray[i]->pReplacementModel,
                aePriority,
                apParent,
                eLODMult,
                true,
                false,
                false
            );
        }
        if (pData->pStagesArray[i]->pExplosion) {
            BGSPreloadable* pPreloadable = pData->pStagesArray[i]->pExplosion;
            pPreloadable->QueueFiles(aePriority, apParent);
        }
        if (pData->pStagesArray[i]->pDebris) {
            BGSPreloadable* pPreloadable = pData->pStagesArray[i]->pDebris;
            pPreloadable->QueueFiles(aePriority, apParent);
        }
    }
}

void BGSDestructibleObjectForm::CreateData() {
    if (pData) {
        return;
    }
    pData = new DestructibleObjectData;
}

DestructibleObjectData* BGSDestructibleObjectForm::GetData() {
    return pData;
}

DestructibleObjectStage** BGSDestructibleObjectForm::AllocateStages(uint8_t acStagesCount) {
    if (!acStagesCount) {
        return nullptr;
    }
    // new DestructibleObjectStage*[n]: -1 (SIZE_MAX) when the size overflows.
    DestructibleObjectStage** pstages = static_cast<DestructibleObjectStage**>(ourMalloc(
        acStagesCount <= SIZE_MAX / sizeof(DestructibleObjectStage*) ? sizeof(DestructibleObjectStage*) * acStagesCount
                                                                     : SIZE_MAX
    ));
    for (uint32_t i = 0; i < acStagesCount; ++i) {
        pstages[i] = new DestructibleObjectStage;
    }
    return pstages;
}

void BGSDestructibleObjectForm::ApplyTerminalDamage(TESObjectREFR* apRef) {
    float fcurrentHealth = apRef->m_Extra.GetObjectHealth();
    if (fcurrentHealth != -1.0) {
        float fHealthPercent = fcurrentHealth * 100.0 / static_cast<double>(pData->iHealth);
        uint8_t ccurrentHealthPercentage = static_cast<int32_t>(FloatCeil(fHealthPercent));
        uint32_t ilastStageSelfDPS = 0;
        uint8_t clastStageHealthPercent = 100;
        TESModelTextureSwap* plastReplacementModel = nullptr;
        for (uint32_t i = 0; i < pData->cNumStages; ++i) {
            DestructibleObjectStage* pstage = pData->pStagesArray[i];
            if (pstage->cHealthPercentage < static_cast<int32_t>(ccurrentHealthPercentage)) {
                if (!ilastStageSelfDPS) {
                    break;
                }
                if (pstage->pReplacementModel) {
                    plastReplacementModel = pstage->pReplacementModel;
                }
            }
            ilastStageSelfDPS = pstage->iSelfDamagePerSecond;
            clastStageHealthPercent = pstage->cHealthPercentage;
        }
        if (clastStageHealthPercent < static_cast<int32_t>(ccurrentHealthPercentage)) {
            float fnewHealth = static_cast<double>(clastStageHealthPercent) / 100.0 *
                static_cast<double>(pData->iHealth);
            apRef->m_Extra.SetObjectHealth(fnewHealth);
            if (plastReplacementModel) {
                apRef->Set3D(nullptr, true);
                apRef->m_Extra.SetModelSwap(plastReplacementModel, apRef->GetObjectReference());
            }
            UpdateCurrentDamageStage(apRef, false);
        }
    }
}

void BGSDestructibleObjectForm::UnloadReplacementModels() {
    if (pData && pData->spPreloadedReplacementModels.m_pObject) {
        NiAtomicDecrement(pData->iReplacementModelRefCount);
        if (pData->iReplacementModelRefCount <= 0) {
            pData->spPreloadedReplacementModels = static_cast<QueuedFile*>(nullptr);
        }
    }
}

void BGSDestructibleObjectForm::PreloadReplacementModels(TESBoundObject* apObject) {
    if (pData) {
        if (!pData->spPreloadedReplacementModels.m_pObject) {
            pData->spPreloadedReplacementModels = new QueuedFile(IO_TASK_PRIORITY_VERY_LOW);
            QueueFiles(apObject, IO_TASK_PRIORITY_VERY_LOW, pData->spPreloadedReplacementModels.m_pObject);
            if (!pData->spPreloadedReplacementModels->GetChildCount()) {
                pData->spPreloadedReplacementModels = static_cast<QueuedFile*>(nullptr);
            }
        }
        if (pData->spPreloadedReplacementModels.m_pObject) {
            NiAtomicIncrement(pData->iReplacementModelRefCount);
        }
    }
}

DestructibleObjectStage* BGSDestructibleObjectForm::GetStage(uint32_t aiIndex) {
    if (pData && aiIndex < pData->cNumStages) {
        return pData->pStagesArray[aiIndex];
    }

    return nullptr;
}

float BGSDestructibleObjectForm::CalculateDangerousExplosionTerminalTime(
    uint8_t acCurrentHealthPercentage,
    float afRequiredRadius,
    float afRequiredDamage,
    BGSExplosion** apOutExplosion
) {
    DestructibleObjectStage* pStage;
    float fTerminalTime = 0.0f;
    float fCurrentHealth = static_cast<float>(static_cast<double>(pData->iHealth) * acCurrentHealthPercentage / 100.0);
    int32_t iSelfDamagePerSecond = 0;
    for (uint32_t i = 0;; ++i) {
        if (i >= pData->cNumStages) {
            return -0x1p+0f;
        }
        pStage = pData->pStagesArray[i];
        if (pStage->cHealthPercentage < acCurrentHealthPercentage) {
            if (!iSelfDamagePerSecond) {
                return -0x1p+0f;
            }
            float fStageHealth = static_cast<float>(
                static_cast<double>(pData->iHealth) * pStage->cHealthPercentage / 100.0
            );
            fTerminalTime = static_cast<float>(
                (fCurrentHealth - fStageHealth) / static_cast<double>(static_cast<uint32_t>(iSelfDamagePerSecond)) +
                fTerminalTime
            );
            fCurrentHealth = fStageHealth;
            if (pStage->pExplosion &&
                static_cast<double>(afRequiredRadius) <= pStage->pExplosion->GetRadiusBSUnits() &&
                static_cast<double>(afRequiredDamage) <= pStage->pExplosion->BGSExplosion::GetDamage()) {
                break;
            }
        }
        iSelfDamagePerSecond = pStage->iSelfDamagePerSecond;
    }
    if (apOutExplosion) {
        *apOutExplosion = pStage->pExplosion;
    }
    return fTerminalTime;
}

bool BGSDestructibleObjectForm::GetTargetableInVATS() {
    return pData && (pData->cFlags & DestructibleObjectData::FLAG_VATS_TARGETABLE) != 0;
}

bool BGSDestructibleObjectForm::HideSmallDebris(NiAVObject* ap3D) {
    CFilter kFilter;
    bool bhidSomething = false;
    if (ap3D) {
        bool bcull = false;
        bhkCollisionObject* pcol = bhkCollisionObject::GetbhkCollisionObject(ap3D);
        if (pcol) {
            bhkRigidBody* prb = pcol->GetRigidBody();
            if (prb) {
                CFilter* pFilter = prb->GetCollisionFilter(&kFilter);
                if (pFilter->GetLayer() == L_DEBRIS_SMALL) {
                    bcull = true;
                }
            }
        }
        if (bcull) {
            bhidSomething = true;
            ap3D->SetAppCulled(true);
        }
        else {
            NiNode* pNode = ap3D->IsNode();
            if (pNode) {
                uint32_t uiCount = pNode->GetArrayCount();
                for (int32_t i = 0; i < uiCount; ++i) {
                    NiAVObject* pChild = pNode->GetAt(i);
                    if (BGSDestructibleObjectForm::HideSmallDebris(pChild)) {
                        bhidSomething = true;
                    }
                }
            }
        }
    }
    return bhidSomething;
}

void BGSDestructibleObjectForm::UpdateDestructibleObjects() {
    float fsecondsPassed = appTimer.fDelta;
    NiTMapItem<TESObjectREFR*, uint32_t>* it = BGSDestructibleObjectForm::DestructibleObjects.GetFirstPos();
    while (it) {
        TESObjectREFR* pref = nullptr;
        uint32_t idamagePerSecond = 0;
        BGSDestructibleObjectForm::DestructibleObjects.GetNext(it, pref, idamagePerSecond);
        if (pref && !pref->GetDisabled() && !pref->GetDelete()) {
            float fdamage = static_cast<double>(idamagePerSecond) * fsecondsPassed;
            if (iNumHWThreads.Int() == 1) {
                if (pref->IsActor()) {
                    // Actor::DoDamage (vtable +0x338): the damage as health damage, no fatigue damage, no source.
                    static_cast<Actor*>(pref)->DoDamage(fdamage, 0.0f, nullptr);
                }
                else {
                    pref->DamageObject(fdamage, true);
                }
            }
            else {
                TaskQueueInterface::GetInstance()->QueueUpdateDestructibleObject(pref, fdamage, true);
            }
        }
    }
}

void BGSDestructibleObjectForm::UpdateDamageStageNodes(NiAVObject* apRoot, uint32_t aiStageIndex) {
    if (apRoot) {
        if (NiObject::IsKindOf_0(&BSDamageStage::ms_RTTI, apRoot)) {
            static_cast<BSRangeNode*>(apRoot)->SetCurrent(aiStageIndex);
        }
        NiNode* pNode = apRoot->IsNode();
        if (pNode) {
            for (uint32_t i = 0; i < pNode->GetArrayCount(); ++i) {
                NiAVObject* pChild = pNode->GetAt(i);
                BGSDestructibleObjectForm::UpdateDamageStageNodes(pChild, aiStageIndex);
            }
        }
    }
}

int32_t BGSDestructibleObjectForm::CalcDestructionStage(TESObjectREFR* apRef) {
    int32_t iNumStages = -1;
    if (apRef) {
        BGSDestructibleObjectForm* pDestructionForm = BGSDestructibleObjectForm::GetDestructionForm(
            apRef->GetObjectReference()
        );
        DestructibleObjectData* pDestructData = pDestructionForm ? pDestructionForm->pData : nullptr;
        if (pDestructData) {
            float fObjectHealth = apRef->m_Extra.GetObjectHealth();
            if (fObjectHealth != -1.0) {
                float fHealthPercent = fObjectHealth * 100.0 /
                    static_cast<double>(pDestructionForm->GetMaxHealth(apRef));
                uint8_t ucHealthPercent = static_cast<int32_t>(FloatCeil(fHealthPercent));
                iNumStages = pDestructData->cNumStages;
                for (int32_t i = 0; i < pDestructData->cNumStages; ++i) {
                    if (pDestructData->pStagesArray[i]->cHealthPercentage < static_cast<int32_t>(ucHealthPercent)) {
                        return i;
                    }
                }
            }
        }
    }
    return iNumStages;
}

void BGSDestructibleObjectForm::ClearDestruction(TESObjectREFR* apRef) {
    if (apRef->GetDestructible()) {
        TESForm* pBaseObject = apRef->GetObjectReference();
        BGSDestructibleObjectForm::GetDestructionForm(pBaseObject);
        float fObjectHealth = apRef->m_Extra.GetObjectHealth();
        if (fObjectHealth != -1.0) {
            apRef->m_Extra.RemoveObjectHealth();
            if (apRef->GetDestroyed()) {
                apRef->SetDestroyed(false);
            }
            if (apRef->m_Extra.GetModelSwap()) {
                apRef->m_Extra.RemoveModelSwap();
                if (apRef->Get3D()) {
                    bool bAllowChanges = pSaveLoadGame->SetThreadAllowChanges(false);
                    apRef->Set3D(nullptr, true);
                    pSaveLoadGame->SetThreadAllowChanges(bAllowChanges);
                    TESObjectCELL* pParentCell = apRef->GetParentCell();
                    TES::pInstance->AttachModel(apRef, pParentCell, nullptr, false);
                    if (apRef->Get3D()) {
                        NiNode* p3D = apRef->Get3D();
                        NiNode* pNode = p3D ? p3D->IsNode() : nullptr;
                        BSFadeNode* pFadeNode = pNode ? pNode->IsFadeNode() : nullptr;
                        if (pFadeNode) {
                            pFadeNode->TurnFadeNodeOn();
                        }
                        bhkWorld::Activate(p3D, true, true, false);
                    }
                }
            }
            else if (apRef->Get3D()) {
                NiNode* p3D = apRef->TESObjectREFR::Get3D();
                BGSDestructibleObjectForm::UpdateDamageStageNodes(p3D, 0);
            }
            if (apRef->GetDisabled()) {
                apRef->Enable();
            }
            BGSDestructibleObjectForm::DestructibleObjects.RemoveAt(apRef);
            CombatThreatMap* pThreatMap = pCombatManager->pThreatMap;
            pThreatMap->RemoveThreat(apRef);
        }
    }
}

void BGSDestructibleObjectForm::LoadChunk(BGSDestructibleObjectForm* apDestruct, TESFile* apFile) {
    if (apFile && apDestruct) {
        CHUNK_ID eChunkID = apFile->GetTESChunk();
        apDestruct->CreateData();
        DestructibleObjectData* pDestructData = apDestruct->pData;
        if (eChunkID == DSTD_ID) {
            SAVE_DESTRUCTIBLE_OBJECT_STAGE kStage;
            ourMemset(&kStage, 0, sizeof(kStage));
            apFile->GetChunkData(&kStage, 20u);
            if (apFile->QEndian()) {
                kStage.Endian();
            }
            uint8_t cStageNum = kStage.cStageNum;
            if (kStage.cStageNum < static_cast<int32_t>(pDestructData->cNumStages)) {
                pDestructData->pStagesArray[cStageNum]->cModelDamageStage = kStage.cModelDamageStage;
                pDestructData->pStagesArray[cStageNum]->cHealthPercentage = kStage.cHealthPercentage;
                pDestructData->pStagesArray[cStageNum]->cFlags = kStage.cFlags;
                pDestructData->pStagesArray[cStageNum]->iSelfDamagePerSecond = kStage.iSelfDamagePerSecond;
                // The file's form IDs go into the pointer fields as raw values; InitItem resolves them.
                pDestructData->pStagesArray[cStageNum]->pExplosion = reinterpret_cast<BGSExplosion*>(
                    static_cast<uintptr_t>(static_cast<uint32_t>(kStage.iExplosionID))
                );
                pDestructData->pStagesArray[cStageNum]->pDebris = reinterpret_cast<BGSDebris*>(
                    static_cast<uintptr_t>(static_cast<uint32_t>(kStage.iDebrisID))
                );
                pDestructData->pStagesArray[cStageNum]->iDebrisCount = kStage.iDebrisCount;
                do {
                    eChunkID = apFile->GetTESChunk();
                    if (eChunkID == NO_CHUNK) {
                        break;
                    }
                    if (eChunkID > DMDS_ID) {
                        if (eChunkID == DMDT_ID) {
                            pDestructData->pStagesArray[cStageNum]->CreateReplacementModel();
                            TESModel::LoadModelTextureChunk(
                                pDestructData->pStagesArray[cStageNum]->pReplacementModel,
                                apFile
                            );
                        }
                    }
                    else {
                        switch (eChunkID) {
                            case DMDS_ID: {
                                pDestructData->pStagesArray[cStageNum]->CreateReplacementModel();
                                uint32_t uiDataSize = apFile->GetChunkSize();
                                // A 16-byte aligned stack buffer of the chunk's size.
                                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(uiDataSize));
                                apFile->GetChunkData(pBuffer, uiDataSize);
                                uint16_t usFormVersion = apFile->GetFormVersion();
                                pDestructData->pStagesArray[cStageNum]
                                    ->pReplacementModel->UnpackTextureSwapChunkData(pBuffer, uiDataSize, usFormVersion);
                                break;
                            }
                            case DSTF_ID: {
                                return;
                            }
                            case DMDL_ID: {
                                pDestructData->pStagesArray[cStageNum]->CreateReplacementModel();
                                // A 16-byte aligned buffer of the chunk's size, read with no size limit.
                                char* pBuffer = static_cast<char*>(XFNV_ALLOCA16(apFile->GetChunkSize()));
                                apFile->GetChunkData(pBuffer, 0);
                                pDestructData->pStagesArray[cStageNum]->pReplacementModel->SetModel(pBuffer);
                                break;
                            }
                        }
                    }
                }
                while (apFile->NextChunk());
            }
        }
        else if (eChunkID == DEST_ID) {
            SAVE_DESTRUCTIBLE_OBJECT_DATA kData;
            if (apFile->GetChunkSize() == 8) {
                apFile->GetChunkData(&kData, 0);
                if (apFile->QEndian()) {
                    kData.Endian();
                }
                pDestructData->iHealth = kData.iHealth;
                pDestructData->cNumStages = kData.cNumStages;
                pDestructData->cFlags = kData.cFlags;
                if (pDestructData->cNumStages) {
                    pDestructData->pStagesArray = apDestruct->AllocateStages(pDestructData->cNumStages);
                }
            }
            else {
                TESForm* pForm = dynamic_cast<TESForm*>(apDestruct);
                const char* pEditorID = pForm->GetFormEditorID();
                Warning(
                    "MASTERFILE: Old Destruction data found on form %08X %s.  It needs to be resaved.",
                    pForm->GetFormID(),
                    pEditorID
                );
                apDestruct->BGSDestructibleObjectForm::ClearDataComponent();
            }
        }
    }
}

BGSDestructibleObjectForm* BGSDestructibleObjectForm::GetDestructionForm(TESForm* apForm) {
    BGSDestructibleObjectForm* pDestructible = nullptr;
    if (!apForm) {
        return pDestructible;
    }
    switch (apForm->GetFormType()) {
        case INGR_ID: pDestructible = static_cast<IngredientItem*>(apForm); break;
        case ALCH_ID: pDestructible = static_cast<AlchemyItem*>(apForm); break;
        case WEAP_ID: pDestructible = static_cast<TESObjectWEAP*>(apForm); break;
        case MISC_ID:
        case KEYM_ID:
        case COBJ_ID:
        case IMOD_ID: pDestructible = static_cast<TESObjectMISC*>(apForm); break;
        case CONT_ID: pDestructible = static_cast<TESObjectCONT*>(apForm); break;
        case CLOT_ID: pDestructible = static_cast<TESObjectCLOT*>(apForm); break;
        case BOOK_ID: pDestructible = static_cast<TESObjectBOOK*>(apForm); break;
        case ARMO_ID: pDestructible = static_cast<TESObjectARMO*>(apForm); break;
        case LIGH_ID: pDestructible = static_cast<TESObjectLIGH*>(apForm); break;
        case FURN_ID: pDestructible = static_cast<TESFurniture*>(apForm); break;
        case AMMO_ID: pDestructible = static_cast<TESAmmo*>(apForm); break;
        case ACTI_ID: pDestructible = static_cast<TESObjectACTI*>(apForm); break;
        case MSTT_ID: pDestructible = static_cast<BGSMovableStatic*>(apForm); break;
        case TREE_ID: pDestructible = static_cast<TESObjectTREE*>(apForm); break;
        case FLOR_ID: pDestructible = static_cast<TESFlora*>(apForm); break;
        case DOOR_ID: pDestructible = static_cast<TESObjectDOOR*>(apForm); break;
        case TACT_ID: pDestructible = static_cast<BGSTalkingActivator*>(apForm); break;
        case TERM_ID: pDestructible = static_cast<BGSTerminal*>(apForm); break;
        case PROJ_ID: pDestructible = static_cast<BGSProjectile*>(apForm); break;
        case NPC__ID: pDestructible = static_cast<TESNPC*>(apForm); break;
        case CREA_ID: pDestructible = static_cast<TESCreature*>(apForm); break;
        default: break;
    }
    return pDestructible;
}

bool BGSDestructibleObjectForm::IsDestructible(TESForm* apForm) {
    BGSDestructibleObjectForm* pDestructible = GetDestructionForm(apForm);
    if (!pDestructible) {
        return false;
    }
    return pDestructible->HasValidStages();
}

int32_t BGSDestructibleObjectForm::GetModelSwapIndex(TESForm* apForm, TESModel* apModelSwap) {
    BGSDestructibleObjectForm* pDestructionForm = BGSDestructibleObjectForm::GetDestructionForm(apForm);
    if (!pDestructionForm) {
        return -1;
    }
    DestructibleObjectData* pDestructData = pDestructionForm->BGSDestructibleObjectForm::GetData();
    if (!pDestructData) {
        return -1;
    }
    for (uint32_t i = 0; i < pDestructData->cNumStages; ++i) {
        if (static_cast<const void*>(pDestructData->pStagesArray[i]->pReplacementModel) ==
            static_cast<const void*>(apModelSwap)) {
            return static_cast<int32_t>(i);
        }
    }
    return -1;
}

TESModel* BGSDestructibleObjectForm::GetModelSwap(TESForm* apForm, int32_t aiModelSwapIndex) {
    BGSDestructibleObjectForm* pDestructionForm = BGSDestructibleObjectForm::GetDestructionForm(apForm);
    if (!pDestructionForm || aiModelSwapIndex == -1) {
        return nullptr;
    }
    DestructibleObjectData* pDestructData = pDestructionForm->BGSDestructibleObjectForm::GetData();
    if (!pDestructData) {
        return nullptr;
    }
    if (aiModelSwapIndex >= pDestructData->cNumStages) {
        return nullptr;
    }
    return pDestructData->pStagesArray[aiModelSwapIndex]->pReplacementModel;
}

void BGSDestructibleObjectForm::RemoveDestructibleObject(TESObjectREFR* apRef) {
    BGSDestructibleObjectForm::DestructibleObjects.RemoveAt(apRef);
}

uint32_t BGSDestructibleObjectForm::GetSelfDamage(TESObjectREFR* apRef, float afHealth) {
    if (!apRef || afHealth == 0.0) {
        return 0;
    }
    TESForm* pBaseObject = apRef->GetObjectReference();
    BGSDestructibleObjectForm* pDestructionForm = BGSDestructibleObjectForm::GetDestructionForm(pBaseObject);
    if (!pDestructionForm) {
        return 0;
    }
    DestructibleObjectData* pDestructData = pDestructionForm->GetData();
    if (!pDestructData) {
        return 0;
    }
    float fHealthPercent = afHealth * 100.0 / static_cast<double>(pDestructionForm->GetMaxHealth(apRef));
    uint8_t ucHealthPercent = static_cast<int32_t>(FloatCeil(fHealthPercent));
    uint32_t iSelfDamagePerSecond = 0;
    for (uint32_t i = 0; i < pDestructData->cNumStages; ++i) {
        DestructibleObjectStage* pStage = pDestructData->pStagesArray[i];
        if (pStage->cHealthPercentage < static_cast<int32_t>(ucHealthPercent)) {
            break;
        }
        iSelfDamagePerSecond = pStage->iSelfDamagePerSecond;
    }
    return iSelfDamagePerSecond;
}

void BGSDestructibleObjectForm::SetSelfDamage(TESObjectREFR* apRef, uint32_t aiSelfDamage) {
    if (aiSelfDamage) {
        BGSDestructibleObjectForm::DestructibleObjects.SetAt(apRef, aiSelfDamage);
    }
    else {
        BGSDestructibleObjectForm::DestructibleObjects.RemoveAt(apRef);
    }
}

DestructibleRefsTakingDamageMap* BGSDestructibleObjectForm::GetDestructibleObjects() {
    return &BGSDestructibleObjectForm::DestructibleObjects;
}

// The range is the double difference of the bounds, and the sum is rounded to a float local before it is returned.
float BSRandom::Float(float afMin, float afMax) {
    uint32_t uiValue = UnsignedInt();
    float fResult = static_cast<float>(uiValue * (static_cast<double>(afMax) - afMin) / 4294967296.0 + afMin);
    return fResult;
}

float BGSExplosion::GetDamage() {
    return Data.fDamage;
}

float FloatCeil(float aIn) {
    float fFloored = fFloor(aIn);
    if (!(fFloored - aIn >= 0.0)) {
        return fFloored + 1.0;
    }

    return fFloored;
}

const NiPoint3& NiBound::GetCenter() const {
    return m_kCenter;
}
