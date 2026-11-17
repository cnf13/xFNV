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

DestructibleRefsTakingDamageMap BGSDestructibleObjectForm::DestructibleObjects(37);

float RandomFloat(float afMin, float afMax) {
	BSRandom* singleton = BSRandom::GetSingleton();
	return singleton->Float(afMin, afMax);
}

NiFixedString* get_g_ModelSwapNode() {
	return FixedStrings::ModelSwapNode;
}

// Swaps the four dword fields read from a big-endian file; the four leading bytes are left alone.
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
	return ((cFlags & 2) != 0);
}

bool DestructibleObjectStage::GetCapDamage() {
	return ((cFlags & FLAG_CAP_DAMAGE) != 0);
}

bool DestructibleObjectStage::GetDestroyObject() {
	return ((cFlags & FLAG_DESTROY_OBJECT) != 0);
}

// The exe's cleanup-only C++ EH frame is left out.
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
			pReplacementModel = 0;
		}
		if (apCopy->pReplacementModel) {
			TESModelTextureSwap* v2;
			TESModelTextureSwap* apMemory = static_cast<TESModelTextureSwap*>(ourMalloc(sizeof(TESModelTextureSwap)));
			if (apMemory) {
				v2 = ::new (apMemory) TESModelTextureSwap;
			} else {
				v2 = 0;
			}
			pReplacementModel = v2;
			pReplacementModel->CopyComponent(apCopy->pReplacementModel);
		}
	}
}

void DestructibleObjectStage::CreateReplacementModel() {
	TESModelTextureSwap* v1;
	void* v3;
	if (!pReplacementModel) {
		v3 = ourMalloc(sizeof(TESModelTextureSwap));
		if (v3) {
			v1 = ::new (v3) TESModelTextureSwap;
		} else {
			v1 = nullptr;
		}
		pReplacementModel = v1;
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
	if (pData)
		delete pData;
	pData = nullptr;
}

void BGSDestructibleObjectForm::CopyComponent(BaseFormComponent* apCopyFC) {
	BGSDestructibleObjectForm* pCopy = dynamic_cast<BGSDestructibleObjectForm*>(apCopyFC);
	if (!pCopy)
		return;
	DestructibleObjectData* pCopyData = pCopy->GetData();
	if (!pCopyData) {
		if (pData)
			delete pData;
		pData = nullptr;
		return;
	}
	if (!pData) {
		void* pMemory = ourMalloc(sizeof(DestructibleObjectData));
		pData = pMemory ? ::new (pMemory) DestructibleObjectData : nullptr;
	} else {
		for (uint32_t i = 0; i < pData->cNumStages; ++i) {
			DestructibleObjectStage* pStage = pData->pStagesArray[i];
			if (pStage)
				delete pStage;
		}
		ourFree(pData->pStagesArray);
		pData->pStagesArray = nullptr;
	}
	pData->cFlags = pCopyData->cFlags;
	pData->iHealth = pCopyData->iHealth;
	pData->cNumStages = pCopyData->cNumStages;
	if (pData->cNumStages) {
		pData->pStagesArray = AllocateStages(pData->cNumStages);
		for (uint32_t j = 0; j < pData->cNumStages; ++j)
			pData->pStagesArray[j]->Copy(pCopyData->pStagesArray[j]);
	}
}

bool BGSDestructibleObjectForm::CompareComponent(BaseFormComponent* apCompareFC) {
	BGSDestructibleObjectForm* pcompare = dynamic_cast<BGSDestructibleObjectForm*>(apCompareFC);
	if (!pcompare) {
		return 1;
	}
	DestructibleObjectData* pdata = pcompare->pData;
	if (!pdata || !pData) {
		return pdata || pData;
	}
	if (pData->iHealth != pdata->iHealth) {
		return 1;
	}
	if (pData->cFlags != pdata->cFlags) {
		return 1;
	}
	if (pData->cNumStages != pdata->cNumStages) {
		return 1;
	}
	for (uint32_t i = 0; i < pData->cNumStages; ++i) {
		if (pData->pStagesArray[i]->cHealthPercentage != pdata->pStagesArray[i]->cHealthPercentage) {
			return 1;
		}
		if (pData->pStagesArray[i]->iSelfDamagePerSecond != pdata->pStagesArray[i]->iSelfDamagePerSecond) {
			return 1;
		}
		if (pData->pStagesArray[i]->cModelDamageStage != pdata->pStagesArray[i]->cModelDamageStage) {
			return 1;
		}
		if (pData->pStagesArray[i]->cFlags != pdata->pStagesArray[i]->cFlags) {
			return 1;
		}
		if (pData->pStagesArray[i]->pExplosion != pdata->pStagesArray[i]->pExplosion) {
			return 1;
		}
		if (pData->pStagesArray[i]->pDebris != pdata->pStagesArray[i]->pDebris) {
			return 1;
		}
		// The exe compares one form's iDebrisCount with itself (0x478AE5, a bug in the original), which is always
		// equal.
		if ((pData->pStagesArray[i]->pReplacementModel && !pdata->pStagesArray[i]->pReplacementModel) ||
			(!pData->pStagesArray[i]->pReplacementModel && pdata->pStagesArray[i]->pReplacementModel)) {
			return 1;
		}
		if (pData->pStagesArray[i]->pReplacementModel &&
			(pdata->pStagesArray[i]->pReplacementModel &&
				pData->pStagesArray[i]
					->pReplacementModel->CompareComponent(pdata->pStagesArray[i]->pReplacementModel))) {
			return 1;
		}
	}
	return 0;
}

void BGSDestructibleObjectForm::FindModelSwapNodes(NiAVObject* apRoot, BSSimpleArray<NiNode*, 1024>& arNodeArray) {
	if (apRoot) {
		NiNode* pnode = apRoot->IsNode();
		if (pnode) {
			if ((&pnode->m_kName)->GetHandleString()) {
				NiFixedString* g_ModelSwapNode = get_g_ModelSwapNode();
				if (NiFixedString::HandlesEqual(pnode->m_kName, *g_ModelSwapNode)) {
					arNodeArray.Add(pnode);
				}
			}
			for (uint32_t i = 0; i < pnode->GetArrayCount(); ++i) {
				NiAVObject* v4 = pnode->GetAt(i);
				FindModelSwapNodes(v4, arNodeArray);
			}
		}
	}
}

bool BGSDestructibleObjectForm::UpdateCurrentDamageStage(TESObjectREFR* apRef, bool abForceUpdate) {
	if (!apRef->Get3D()) {
		return 0;
	}
	ExtraDataList* Extra = apRef->GetExtra();
	float ObjectHealth = Extra->GetObjectHealth();
	uint32_t uiDamageStage = 0;
	if (ObjectHealth == -1.0) {
		if (!abForceUpdate) {
			return 0;
		}
	} else {
		float aIn = ObjectHealth * 100.0 / static_cast<double>(pData->iHealth);
		uint8_t v10 = static_cast<int32_t>(FloatCeil(aIn));
		for (uint32_t aiIndex = 0; aiIndex < pData->cNumStages; ++aiIndex) {
			DestructibleObjectStage* Stage = GetStage(aiIndex);
			if (Stage->cHealthPercentage < static_cast<int32_t>(v10)) {
				break;
			}
			if (Stage->cModelDamageStage) {
				uiDamageStage = Stage->cModelDamageStage;
			}
		}
	}
	if (!uiDamageStage && !abForceUpdate) {
		return 0;
	}
	NiNode* v5 = apRef->Get3D();
	BGSDestructibleObjectForm::UpdateDamageStageNodes(v5, uiDamageStage);
	return 1;
}

// Applies afDamage to apRef (an actor takes its health actor value instead) and runs every destruction stage the
// health percentage crosses: disabling, destroying, capping the damage, swapping in the replacement model (moved so
// that its model swap node stays where the old one was), showing the damage stage nodes, the self damage, and then
// per crossed stage the OnDestructionStageChange event, the debris and the explosion. Last, the combat threat map
// learns whether the object is now a dangerous explosive. abSelfDamage is never read.
// The percentages are the ceiled float percent truncated to an int (the exe's fistp under a truncating control word),
// of which the low byte is kept. The exe's cleanup-only EH frame (the root pointer and the two node arrays) and its
// stack cookie check are left out; the arrays are constructed and destroyed where the exe does it.
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
				} else {
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
		} else if (cOldPercent <= pStage->cHealthPercentage && cNewPercent < pStage->cHealthPercentage) {
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
				kOldTransform.~NiTransform();
				::new (&kOldTransform) NiTransform;
				FindModelSwapNodes(apRef->Get3D(), *pOldNodes);
				if (pOldNodes->GetSize()) {
					// SWEEP-INLINE 0x6A7AD0
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
							// SWEEP-INLINE 0x6A7AD0
							NiTransform kNewTransform = (*pNewNodes)[0]->GetWorldTransform();
							NiMatrix3 kInverse = kNewTransform.m_Rotate.Transpose();
							NiMatrix3 kOldRotate = kOldTransform.m_Rotate;
							NiMatrix3 kOrientation = apRef->GetOrientation();
							apRef->SetOrientation(kOldRotate * kInverse * kOrientation);
							apRef->Update3DPosition();
							// SWEEP-INLINE 0x6A7AD0
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
			} else if (uiNewModelStage > uiOldModelStage) {
				UpdateDamageStageNodes(apRef->TESObjectREFR::Get3D(), uiNewModelStage);
				apRef->UpdateAddonNodeSounds(false);
			}
		}
		if (iSelfDamage && fNewHealth > 0.0) {
			BGSDestructibleObjectForm::DestructibleObjects.SetAt(apRef, iSelfDamage);
			iOldSelfDamage = iSelfDamage;
		} else {
			BGSDestructibleObjectForm::DestructibleObjects.RemoveAt(apRef);
		}
	}
	for (uint32_t i = uiFirstStage; i < uiLastStage; ++i) {
		DestructibleObjectStage* pStage = GetStage(i);
		// The OnDestructionStageChange event flag (IDA's kEvent_OnDestructionStageChange; not in the PDB).
		Script::SetActionFlag(apRef, apRef->GetExtra(), 0x200000);
		uint32_t uiModelStage = 0;
		if (uiNewModelStage) {
			uiModelStage = uiNewModelStage;
		} else if (uiOldModelStage) {
			uiModelStage = uiOldModelStage;
		}
		if (pStage->pDebris && pStage->iDebrisCount) {
			if (!SpawnDebrisAtDebrisNodes(spRoot.operator->(), pStage->pDebris, uiModelStage, apRef,
					pStage->iDebrisCount)) {
				NiPoint3 kLocation = apRef->GetLocationOnReference();
				float fScale = apRef->GetScale();
				TESObjectCELL* pCell = apRef->GetParentCell();
				pStage->pDebris->SpawnDebris(pCell, kLocation, pStage->iDebrisCount, fScale);
			}
		}
		if (pStage->pExplosion) {
			if (!SpawnExplosionsAtBlastNodes(spRoot.operator->(), pStage->pExplosion, uiModelStage, apRef)) {
				// No blast nodes: the explosion goes off at a random point of the lower half of the bound.
				NiMatrix3 kRotation = apRef->GetOrientation();
				NiBound kBound = spRoot->GetWorldBound();
				NiPoint3 kOffset;
				kOffset.~NiPoint3();
				::new (&kOffset) NiPoint3;
				kOffset.x = RandomFloat(-1.0f, 1.0f);
				kOffset.y = RandomFloat(-1.0f, 1.0f);
				kOffset.z = RandomFloat(-1.0f, 0.0f);
				kOffset *= kBound.m_fRadius;
				NiPoint3 kOrigin = kBound.GetCenter() + kOffset;
				if (apRef->IsProjectile()) {
					Projectile* pProjectile = static_cast<Projectile*>(static_cast<MobileObject*>(apRef));
					TESObjectCELL* pCell = apRef->GetParentCell();
					TESObjectREFR* pShooter = pProjectile->GetShooter();
					Explosion* pExplosion =
						Explosion::SpawnExplosion(pStage->pExplosion, pShooter, nullptr, pCell, kOrigin, kRotation);
					if (pExplosion && pProjectile->GetWeaponSource()) {
						pExplosion->SetWeaponSource(pProjectile->GetWeaponSource());
					}
				} else {
					TESObjectCELL* pCell = apRef->GetParentCell();
					Explosion::SpawnExplosion(pStage->pExplosion, apRef, apRef, pCell, kOrigin, kRotation);
				}
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
		fThreatTime =
			CalculateDangerousExplosionTerminalTime(cNewPercent, fRequiredRadius, fRequiredDamage, &pExplosion);
		if (fCombatThreatTimedExplosionLength.Float() < fThreatTime) {
			fThreatTime = -1.0f;
		}
	}
	if (fThreatTime == -1.0) {
		float fRequiredDamage = fDangerousObjectExplosionDamage.Float();
		float fRequiredRadius = fDangerousObjectExplosionRadius.Float();
		float fExplosionHealth =
			CalculateDangerousExplosionHealth(cNewPercent, fRequiredRadius, fRequiredDamage, &pExplosion);
		if (fExplosionHealth != -1.0) {
			// The exe keeps the remaining health as a double (fstp qword).
			double fRemainingHealth = static_cast<double>(fNewHealth) - static_cast<double>(fExplosionHealth);
			if (fCombatThreatDangerousObjectHealth.Float() > fRemainingHealth) {
				fThreatTime = fCombatThreatExplosiveObjectThreatTime.Float();
			}
		}
	}
	if (fThreatTime != -1.0) {
		pCombatManager->GetThreatMap()->AddThreat(apRef, pExplosion, fThreatTime);
	} else {
		pCombatManager->GetThreatMap()->RemoveThreat(apRef);
	}
}

uint32_t BGSDestructibleObjectForm::GetMaxHealth(TESObjectREFR* apRef) {
	if (apRef->IsActor()) {
		// The source's downcast to Actor, then its ActorValueOwner base's float getter (slot 1), with no null test.
		ActorValueOwner& rOwner = *static_cast<Actor*>(apRef);
		return static_cast<int64_t>(rOwner.GetBaseActorFloatValue(ActorValue::HEALTH));
	} else {
		return pData->iHealth;
	}
}

uint32_t BGSDestructibleObjectForm::SpawnExplosionsAtBlastNodes(NiAVObject* apRoot, BGSExplosion* apExplosion,
	uint32_t aiStageIndex, TESObjectREFR* apRef) {
	if (!apRoot) {
		return 0;
	}
	uint32_t inumExplosions = 0;
	if (NiObject::IsKindOf_0(&BSBlastNode::ms_RTTI, apRoot)) {
		BSBlastNode* pblastNode = static_cast<BSBlastNode*>(apRoot);
		if (static_cast<BSRangeNode*>(apRoot)->BSRangeNode::SetCurrent(aiStageIndex)) {
			NiMatrix3 kRotate = pblastNode->GetWorldTransform().m_Rotate;
			NiPoint3 kTranslate = pblastNode->GetWorldTranslate();
			TESObjectCELL* ParentCell = apRef->GetParentCell();
			Explosion::SpawnExplosion(apExplosion, apRef, apRef, ParentCell, kTranslate, kRotate);
			++inumExplosions;
		}
	}
	NiNode* pnode = apRoot->IsNode();
	if (pnode) {
		for (uint32_t i = 0; i < pnode->GetArrayCount(); ++i) {
			NiAVObject* pchild = pnode->GetAt(i);
			inumExplosions += this->SpawnExplosionsAtBlastNodes(pchild, apExplosion, aiStageIndex, apRef);
		}
	}
	return inumExplosions;
}

uint32_t BGSDestructibleObjectForm::SpawnDebrisAtDebrisNodes(NiAVObject* apRoot, BGSDebris* apDebris,
	uint32_t aiStageIndex, TESObjectREFR* apRef, uint32_t aiNumParticles) {
	TESObjectCELL* parentCell;
	NiAVObject* v8;
	const NiPoint3* worldTranslate;
	float afScale;
	uint32_t a1;
	if (!apRoot) {
		return 0;
	}
	uint32_t uiCount = 0;
	if (NiObject::IsKindOf_0(&BSDebrisNode::ms_RTTI, apRoot) &&
		static_cast<BSRangeNode*>(apRoot)->SetCurrent(aiStageIndex)) {
		afScale = apRef->GetScale();
		worldTranslate = &apRoot->GetWorldTranslate();
		parentCell = apRef->TESObjectREFR::GetParentCell();
		apDebris->SpawnDebris(parentCell, *worldTranslate, aiNumParticles, afScale);
		uiCount = 1;
	}
	NiNode* pNode = apRoot->IsNode();
	if (pNode) {
		for (a1 = 0; a1 < ((uint32_t)pNode->GetArrayCount()); ++a1) {
			v8 = pNode->GetAt(a1);
			uiCount += ((uint32_t)SpawnDebrisAtDebrisNodes(static_cast<NiNode*>(v8), apDebris, aiStageIndex, apRef,
				aiNumParticles));
		}
	}
	return (int32_t)uiCount;
}

float BGSDestructibleObjectForm::CalculateDangerousExplosionHealth(uint8_t acCurrentHealthPercentage,
	float afRequiredRadius, float afRequiredDamage, BGSExplosion** apOutExplosion) {
	DestructibleObjectStage* v10;
	for (uint32_t i = 0;; ++i) {
		if (i >= pData->cNumStages) {
			return -1.0f;
		}
		v10 = pData->pStagesArray[i];
		if (v10->cHealthPercentage < static_cast<int32_t>(acCurrentHealthPercentage)) {
			DestructibleObjectStage* v5 = pData->pStagesArray[i];
			if (v10->pExplosion) {
				if (static_cast<double>(afRequiredRadius) <= v10->pExplosion->GetRadiusBSUnits() &&
					static_cast<double>(afRequiredDamage) <= v10->pExplosion->GetDamage()) {
					break;
				}
			}
		}
	}
	float v9 = static_cast<double>(v10->cHealthPercentage) / 100.0 * static_cast<double>(pData->iHealth);
	if (apOutExplosion) {
		*apOutExplosion = v10->pExplosion;
	}
	return v9;
}

void BGSDestructibleObjectForm::InitItem(TESForm* apOwner) {
	if (pData) {
		for (uint32_t i = 0; i < pData->cNumStages; ++i) {
			if (pData->pStagesArray[i]->pExplosion) {
				uint32_t itempint =
					static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pData->pStagesArray[i]->pExplosion));
				TESFile* File = apOwner->GetFile(-1);
				TESForm::AddCompileIndex(itempint, File);
				TESForm* FormByNumericID = TESForm::GetFormByNumericID(itempint);
				BGSExplosion* v4 = dynamic_cast<BGSExplosion*>(FormByNumericID);
				pData->pStagesArray[i]->pExplosion = v4;
				if (!pData->pStagesArray[i]->pExplosion) {
					if (apOwner) {
						if (apOwner->GetFormEditorIDLength()) {
							const char* v8 = apOwner->GetFormEditorID();
							Warning("MASTERFILE: Unable to find stage %i explosion (%08X) on owner object \"%s\".", i,
								itempint, v8);
						} else {
							int32_t CastingType = apOwner->iFormID;
							Warning("MASTERFILE: Unable to find stage %i explosion (%08X) on owner object (%08X).", i,
								itempint, CastingType);
						}
					} else {
						Warning("MASTERFILE: Unable to find stage %i explosion (%08X) on unknown owner.", i, itempint);
					}
				}
			}
			if (pData->pStagesArray[i]->pDebris) {
				uint32_t itempint_2 =
					static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pData->pStagesArray[i]->pDebris));
				TESFile* v5 = apOwner->GetFile(-1);
				TESForm::AddCompileIndex(itempint_2, v5);
				TESForm* v6 = TESForm::GetFormByNumericID(itempint_2);
				BGSDebris* v7 = dynamic_cast<BGSDebris*>(v6);
				pData->pStagesArray[i]->pDebris = v7;
				if (!pData->pStagesArray[i]->pDebris) {
					if (apOwner) {
						if (apOwner->GetFormEditorIDLength()) {
							const char* v10 = apOwner->GetFormEditorID();
							Warning("MASTERFILE: Unable to find stage %i debris (%08X) on owner object \"%s\".", i,
								itempint_2, v10);
						} else {
							int32_t v11 = apOwner->iFormID;
							Warning("MASTERFILE: Unable to find stage %i debris (%08X) on owner object (%08X).", i,
								itempint_2, v11);
						}
					} else {
						Warning("MASTERFILE: Unable to find stage %i debris (%08X) on unknown owner.", i, itempint_2);
					}
				}
			}
			if (pData->pStagesArray[i]->pReplacementModel) {
				pData->pStagesArray[i]->pReplacementModel->InitItem(apOwner);
			}
		}
		int32_t FormType = apOwner->GetFormType();
		if (FormType >= 42 && FormType <= 43) {
			pData->iHealth = static_cast<TESActorBase*>(apOwner)->TESActorBase::GetHealth();
		}
	}
}

// KEEP: the DEST chunk is an 8-byte SAVE_DESTRUCTIBLE_OBJECT_DATA on the stack whose constructor (0x4781B0) sets
// only iHealth, cNumStages and cFlags; __AddChunkData copies all 8 bytes (push 8 at 0x477F75), so the two padding
// bytes are uninitialised stack the exe writes into the form buffer on every path. Unregistered.
void BGSDestructibleObjectForm::Save() {
	SAVE_DESTRUCTIBLE_OBJECT_STAGE stageSaveData;
	if (pData) {
		FNVD_INLINED_LOCAL(SAVE_DESTRUCTIBLE_OBJECT_DATA, saveData, std::nullopt, std::nullopt);
		::new (&saveData) SAVE_DESTRUCTIBLE_OBJECT_DATA;
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
	if (!pData)
		return false;
	for (uint32_t i = 0; i < pData->cNumStages; ++i) {
		DestructibleObjectStage* pStage = pData->pStagesArray[i];
		if (pStage &&
			(pStage->pReplacementModel || pStage->pExplosion || pStage->pDebris || pStage->GetDisableObject()))
			return true;
	}
	return false;
}

void BGSDestructibleObjectForm::QueueFiles(TESBoundObject* apObject, IO_TASK_PRIORITY aePriority,
	QueuedFile* apParent) {
	if (!pData)
		return;
	for (uint32_t i = 0; i < pData->cNumStages; ++i) {
		if (pData->pStagesArray[i]->pReplacementModel) {
			ENUM_LOD_MULT eLODMult = TES::GetLODMult(apObject);
			pModelLoader->QueueModel(pData->pStagesArray[i]->pReplacementModel, aePriority, apParent, eLODMult, true,
				false, false);
		}
		// BGSPreloadable::QueueFiles, slot 4, through the BGSPreloadable subobjects (+0x64, +0x18).
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
	if (pData)
		return;
	void* pMemory = ourMalloc(sizeof(DestructibleObjectData));
	pData = pMemory ? ::new (pMemory) DestructibleObjectData : nullptr;
}

DestructibleObjectData* BGSDestructibleObjectForm::GetData() {
	return pData;
}

// The exe's cleanup-only C++ EH frame is left out.
DestructibleObjectStage** BGSDestructibleObjectForm::AllocateStages(uint8_t acStagesCount) {
	if (!acStagesCount) {
		return 0;
	}
	// new DestructibleObjectStage*[n]: -1 when the size overflows 32 bits (the exe tests n >> 30, its pointers being
	// 4 bytes).
	DestructibleObjectStage** pstages =
		static_cast<DestructibleObjectStage**>(ourMalloc(acStagesCount <= SIZE_MAX / sizeof(DestructibleObjectStage*)
				? sizeof(DestructibleObjectStage*) * acStagesCount
				: SIZE_MAX));
	for (uint32_t i = 0; i < acStagesCount; ++i) {
		DestructibleObjectStage* v3;
		DestructibleObjectStage* apMemory =
			static_cast<DestructibleObjectStage*>(ourMalloc(sizeof(DestructibleObjectStage)));
		if (apMemory) {
			v3 = ::new (apMemory) DestructibleObjectStage;
		} else {
			v3 = 0;
		}
		pstages[i] = v3;
	}
	return pstages;
}

void BGSDestructibleObjectForm::ApplyTerminalDamage(TESObjectREFR* apRef) {
	float fcurrentHealth = (&apRef->m_Extra)->GetObjectHealth();
	if (fcurrentHealth != -1.0) {
		float aIn = fcurrentHealth * 100.0 / static_cast<double>(pData->iHealth);
		uint8_t ccurrentHealthPercentage = static_cast<int32_t>(FloatCeil(aIn));
		uint32_t ilastStageSelfDPS = 0;
		uint8_t clastStageHealthPercent = 100;
		TESModelTextureSwap* plastReplacementModel = 0;
		for (uint32_t i = 0; i < pData->cNumStages; ++i) {
			DestructibleObjectStage* pstage = pData->pStagesArray[i];
			if (pstage->cHealthPercentage < static_cast<int32_t>(ccurrentHealthPercentage)) {
				if (!ilastStageSelfDPS) {
					break;
				}
				DestructibleObjectStage* v2 = pData->pStagesArray[i];
				if (pstage->pReplacementModel) {
					DestructibleObjectStage* v3 = pData->pStagesArray[i];
					plastReplacementModel = pstage->pReplacementModel;
				}
			}
			DestructibleObjectStage* v4 = pData->pStagesArray[i];
			ilastStageSelfDPS = pstage->iSelfDamagePerSecond;
			clastStageHealthPercent = pstage->cHealthPercentage;
		}
		if (clastStageHealthPercent < static_cast<int32_t>(ccurrentHealthPercentage)) {
			float fnewHealth =
				static_cast<double>(clastStageHealthPercent) / 100.0 * static_cast<double>(pData->iHealth);
			(&apRef->m_Extra)->SetObjectHealth(fnewHealth);
			if (plastReplacementModel) {
				apRef->Set3D(0, 1);
				(&apRef->m_Extra)->SetModelSwap(plastReplacementModel, apRef->GetObjectReference());
			}
			UpdateCurrentDamageStage(apRef, 0);
		}
	}
}

void BGSDestructibleObjectForm::UnloadReplacementModels() {
	if (pData) {
		if (pData->spPreloadedReplacementModels.m_pObject) {
			NiAtomicDecrement(pData->iReplacementModelRefCount);
			if (pData->iReplacementModelRefCount <= 0) {
				pData->spPreloadedReplacementModels.operator=(static_cast<QueuedFile*>(0));
			}
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void BGSDestructibleObjectForm::PreloadReplacementModels(TESBoundObject* apObject) {
	if (pData) {
		if (!pData->spPreloadedReplacementModels.m_pObject) {
			QueuedFile* v2;
			QueuedFile* apMemory = static_cast<QueuedFile*>(ourMalloc(sizeof(QueuedFile)));
			if (apMemory) {
				v2 = ::new (apMemory) QueuedFile(IO_TASK_PRIORITY_VERY_LOW);
			} else {
				v2 = 0;
			}
			pData->spPreloadedReplacementModels.operator=(v2);
			QueueFiles(apObject, IO_TASK_PRIORITY_VERY_LOW, pData->spPreloadedReplacementModels.m_pObject);
			if (!pData->spPreloadedReplacementModels.m_pObject->GetChildCount()) {
				pData->spPreloadedReplacementModels.operator=(static_cast<QueuedFile*>(0));
			}
		}
		if (pData->spPreloadedReplacementModels.m_pObject) {
			NiAtomicIncrement(pData->iReplacementModelRefCount);
		}
	}
}

DestructibleObjectStage* BGSDestructibleObjectForm::GetStage(uint32_t aiIndex) {
	if (pData && (aiIndex < ((uint32_t)((int32_t)pData->cNumStages)))) {
		return pData->pStagesArray[aiIndex];
	} else {
		return nullptr;
	}
}

float BGSDestructibleObjectForm::CalculateDangerousExplosionTerminalTime(uint8_t acCurrentHealthPercentage,
	float afRequiredRadius, float afRequiredDamage, BGSExplosion** apOutExplosion) {
	DestructibleObjectStage* v5;
	float v9;
	DestructibleObjectStage* v10;
	uint32_t i;
	float v12 = (float)0.0;
	float v13 = (float)((((double)pData->iHealth) * ((double)acCurrentHealthPercentage)) / 100.0);
	int32_t iSelfDamagePerSecond = 0;
	for (i = 0;; ++i) {
		if (i >= ((uint32_t)((int32_t)pData->cNumStages))) {
			return (-0x1p+0f);
		}
		v10 = pData->pStagesArray[i];
		if (((int32_t)v10->cHealthPercentage) < ((int32_t)acCurrentHealthPercentage)) {
			if (!iSelfDamagePerSecond) {
				return (-0x1p+0f);
			}
			v9 = (float)((((double)pData->iHealth) * ((double)v10->cHealthPercentage)) / 100.0);
			v12 = (float)(((v13 - v9) / ((double)((uint32_t)iSelfDamagePerSecond))) + v12);
			v13 = v9;
			v5 = pData->pStagesArray[i];
			if (v10->pExplosion) {
				if ((((double)afRequiredRadius) <= v10->pExplosion->GetRadiusBSUnits()) &&
					(((double)afRequiredDamage) <= v10->pExplosion->BGSExplosion::GetDamage())) {
					break;
				}
			}
		}
		iSelfDamagePerSecond = v10->iSelfDamagePerSecond;
	}
	if (apOutExplosion) {
		*apOutExplosion = v10->pExplosion;
	}
	return v12;
}

bool BGSDestructibleObjectForm::GetTargetableInVATS() {
	return (pData && ((pData->cFlags & 1) != 0));
}

bool BGSDestructibleObjectForm::HideSmallDebris(NiAVObject* ap3D) {
	CFilter a2;
	bool bhidSomething = 0;
	if (ap3D) {
		bool bcull = 0;
		bhkCollisionObject* pcol = bhkCollisionObject::GetbhkCollisionObject(ap3D);
		if (pcol) {
			bhkRigidBody* prb = pcol->GetRigidBody();
			if (prb) {
				CFilter* CollisionFilter = prb->GetCollisionFilter(&a2);
				if (CollisionFilter->GetLayer() == L_DEBRIS_SMALL) {
					bcull = 1;
				}
			}
		}
		if (bcull) {
			bhidSomething = 1;
			ap3D->SetAppCulled(1);
		} else {
			NiNode* pnode = ap3D->IsNode();
			if (pnode) {
				uint32_t icount = pnode->GetArrayCount();
				for (int32_t i = 0; i < icount; ++i) {
					NiAVObject* v2 = pnode->GetAt(i);
					if (BGSDestructibleObjectForm::HideSmallDebris(v2)) {
						bhidSomething = 1;
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
		TESObjectREFR* pref = 0;
		uint32_t idamagePerSecond = 0;
		BGSDestructibleObjectForm::DestructibleObjects.GetNext(it, pref, idamagePerSecond);
		if (pref) {
			if (!pref->GetDisabled() && !pref->GetDelete()) {
				float fdamage = static_cast<double>(idamagePerSecond) * fsecondsPassed;
				if (iNumHWThreads.Int() == 1) {
					if (pref->IsActor()) {
						// Actor::DoDamage (vtable +0x338): the damage as health damage, no fatigue damage, no source.
						static_cast<Actor*>(pref)->DoDamage(fdamage, 0.0f, nullptr);
					} else {
						pref->DamageObject(fdamage, 1);
					}
				} else {
					TESObjectREFR* v1 = pref;
					TaskQueueInterface* Instance = TaskQueueInterface::GetInstance();
					Instance->QueueUpdateDestructibleObject(v1, fdamage, 1);
				}
			}
		}
	}
}

void BGSDestructibleObjectForm::UpdateDamageStageNodes(NiAVObject* apRoot, uint32_t aiStageIndex) {
	NiAVObject* v2;
	uint32_t a1;
	NiNode* v4;
	if (apRoot) {
		if (NiObject::IsKindOf_0(&BSDamageStage::ms_RTTI, apRoot)) {
			static_cast<BSRangeNode*>(apRoot)->SetCurrent(aiStageIndex);
		}
		v4 = apRoot->IsNode();
		if (v4) {
			for (a1 = 0; a1 < ((uint32_t)v4->GetArrayCount()); ++a1) {
				v2 = v4->GetAt(a1);
				BGSDestructibleObjectForm::UpdateDamageStageNodes(v2, aiStageIndex);
			}
		}
	}
}

int32_t BGSDestructibleObjectForm::CalcDestructionStage(TESObjectREFR* apRef) {
	int32_t cNumStages = -1;
	if (apRef) {
		BGSDestructibleObjectForm* DestructionForm =
			BGSDestructibleObjectForm::GetDestructionForm(apRef->GetObjectReference());
		DestructibleObjectData* v3 = DestructionForm ? DestructionForm->pData : 0;
		if (v3) {
			float ObjectHealth = (&apRef->m_Extra)->GetObjectHealth();
			if (ObjectHealth != -1.0) {
				float aIn = ObjectHealth * 100.0 / static_cast<double>(DestructionForm->GetMaxHealth(apRef));
				uint8_t v5 = static_cast<int32_t>(FloatCeil(aIn));
				cNumStages = v3->cNumStages;
				for (int32_t i = 0; i < v3->cNumStages; ++i) {
					if (v3->pStagesArray[i]->cHealthPercentage < static_cast<int32_t>(v5)) {
						return i;
					}
				}
			}
		}
	}
	return cNumStages;
}

void BGSDestructibleObjectForm::ClearDestruction(TESObjectREFR* apRef) {
	if (apRef->GetDestructible()) {
		TESForm* ObjectReference = apRef->GetObjectReference();
		BGSDestructibleObjectForm::GetDestructionForm(ObjectReference);
		ExtraDataList* Extra = &apRef->m_Extra;
		float ObjectHealth = Extra->GetObjectHealth();
		if (ObjectHealth != -1.0) {
			ExtraDataList* v3 = &apRef->m_Extra;
			v3->RemoveObjectHealth();
			if (apRef->GetDestroyed()) {
				apRef->SetDestroyed(0);
			}
			ExtraDataList* v4 = &apRef->m_Extra;
			if (v4->GetModelSwap()) {
				ExtraDataList* v5 = &apRef->m_Extra;
				v5->RemoveModelSwap();
				if (apRef->Get3D()) {
					bool v12 = pSaveLoadGame->SetThreadAllowChanges(0);
					apRef->Set3D(0, 1);
					pSaveLoadGame->SetThreadAllowChanges(v12);
					TESObjectCELL* ParentCell = apRef->GetParentCell();
					TES::pInstance->AttachModel(apRef, ParentCell, 0, 0);
					if (apRef->Get3D()) {
						BSFadeNode* v9;
						NiNode* v10;
						NiNode* apObject = apRef->Get3D();
						if (apObject) {
							v10 = apObject->IsNode();
						} else {
							v10 = 0;
						}
						if (v10) {
							v9 = v10->IsFadeNode();
						} else {
							v9 = 0;
						}
						if (v9) {
							v9->TurnFadeNodeOn();
						}
						bhkWorld::Activate(apObject, 1, 1, 0);
					}
				}
			} else {
				if (apRef->Get3D()) {
					NiNode* v7 = apRef->TESObjectREFR::Get3D();
					BGSDestructibleObjectForm::UpdateDamageStageNodes(v7, 0);
				}
			}
			if (apRef->GetDisabled()) {
				apRef->Enable();
			}
			BGSDestructibleObjectForm::DestructibleObjects.RemoveAt(apRef);
			CombatThreatMap* Time = pCombatManager->pThreatMap;
			Time->RemoveThreat(apRef);
		}
	}
}

void BGSDestructibleObjectForm::LoadChunk(BGSDestructibleObjectForm* apDestruct, TESFile* apFile) {
	if (apFile && apDestruct) {
		CHUNK_ID TESChunk = apFile->GetTESChunk();
		apDestruct->CreateData();
		DestructibleObjectData* Data = apDestruct->pData;
		if (TESChunk == DSTD_ID) {
			SAVE_DESTRUCTIBLE_OBJECT_STAGE kStage;
			ourMemset(&kStage, 0, sizeof(kStage));
			apFile->GetChunkData(&kStage, 20u);
			if (apFile->QEndian()) {
				kStage.Endian();
			}
			uint8_t cStageNum = kStage.cStageNum;
			if (kStage.cStageNum < static_cast<int32_t>(Data->cNumStages)) {
				Data->pStagesArray[cStageNum]->cModelDamageStage = kStage.cModelDamageStage;
				Data->pStagesArray[cStageNum]->cHealthPercentage = kStage.cHealthPercentage;
				Data->pStagesArray[cStageNum]->cFlags = kStage.cFlags;
				Data->pStagesArray[cStageNum]->iSelfDamagePerSecond = kStage.iSelfDamagePerSecond;
				// The file's form IDs go into the pointer fields as raw values; InitItem resolves them.
				Data->pStagesArray[cStageNum]->pExplosion =
					reinterpret_cast<BGSExplosion*>(static_cast<uintptr_t>(static_cast<uint32_t>(kStage.iExplosionID)));
				Data->pStagesArray[cStageNum]->pDebris =
					reinterpret_cast<BGSDebris*>(static_cast<uintptr_t>(static_cast<uint32_t>(kStage.iDebrisID)));
				Data->pStagesArray[cStageNum]->iDebrisCount = kStage.iDebrisCount;
				do {
					TESChunk = apFile->GetTESChunk();
					if (TESChunk == NO_CHUNK) {
						break;
					}
					if (TESChunk > DMDS_ID) {
						if (TESChunk == DMDT_ID) {
							Data->pStagesArray[cStageNum]->CreateReplacementModel();
							TESModel::LoadModelTextureChunk(Data->pStagesArray[cStageNum]->pReplacementModel, apFile);
						}
					} else {
						switch (TESChunk) {
						case DMDS_ID: {
							Data->pStagesArray[cStageNum]->CreateReplacementModel();
							uint32_t uiDataSize = apFile->GetChunkSize();
							// the exe's _alloca (__alloca_probe_16): a 16-byte aligned buffer of the chunk's size
							char* pBuffer = static_cast<char*>(__builtin_alloca_with_align(uiDataSize, 128));
							apFile->GetChunkData(pBuffer, uiDataSize);
							uint16_t FormVersion = apFile->GetFormVersion();
							Data->pStagesArray[cStageNum]->pReplacementModel->UnpackTextureSwapChunkData(pBuffer,
								uiDataSize, FormVersion);
							break;
						}
						case DSTF_ID: {
							return;
						}
						case DMDL_ID: {
							Data->pStagesArray[cStageNum]->CreateReplacementModel();
							// the exe's _alloca (__alloca_probe_16): a 16-byte aligned buffer of the chunk's size, read
							// with no size limit
							char* pBuffer =
								static_cast<char*>(__builtin_alloca_with_align(apFile->GetChunkSize(), 128));
							apFile->GetChunkData(pBuffer, 0);
							Data->pStagesArray[cStageNum]->pReplacementModel->SetModel(pBuffer);
							break;
						}
						}
					}
				} while (apFile->NextChunk());
			}
		} else if (TESChunk == DEST_ID) {
			FNVD_INLINED_LOCAL(SAVE_DESTRUCTIBLE_OBJECT_DATA, kData, std::nullopt, std::nullopt);
			::new (&kData) SAVE_DESTRUCTIBLE_OBJECT_DATA;
			if (apFile->GetChunkSize() == 8) {
				apFile->GetChunkData(&kData, 0);
				if (apFile->QEndian()) {
					kData.Endian();
				}
				Data->iHealth = kData.iHealth;
				Data->cNumStages = kData.cNumStages;
				Data->cFlags = kData.cFlags;
				if (Data->cNumStages) {
					DestructibleObjectStage** Stages = apDestruct->AllocateStages(Data->cNumStages);
					Data->pStagesArray = Stages;
				}
			} else {
				TESForm* pForm = dynamic_cast<TESForm*>(apDestruct);
				const char* pEditorID = pForm->GetFormEditorID();
				Warning("MASTERFILE: Old Destruction data found on form %08X %s.  It needs to be resaved.",
					pForm->GetFormID(), pEditorID);
				apDestruct->BGSDestructibleObjectForm::ClearDataComponent();
			}
		}
	}
}

BGSDestructibleObjectForm* BGSDestructibleObjectForm::GetDestructionForm(TESForm* apForm) {
	BGSDestructibleObjectForm* pDestructible = nullptr;
	if (!apForm)
		return pDestructible;
	switch (apForm->GetFormType()) {
	case INGR_ID:
		pDestructible = static_cast<IngredientItem*>(apForm);
		break;
	case ALCH_ID:
		pDestructible = static_cast<AlchemyItem*>(apForm);
		break;
	case WEAP_ID:
		pDestructible = static_cast<TESObjectWEAP*>(apForm);
		break;
	case MISC_ID:
	case KEYM_ID:
	case COBJ_ID:
	case IMOD_ID:
		pDestructible = static_cast<TESObjectMISC*>(apForm);
		break;
	case CONT_ID:
		pDestructible = static_cast<TESObjectCONT*>(apForm);
		break;
	case CLOT_ID:
		pDestructible = static_cast<TESObjectCLOT*>(apForm);
		break;
	case BOOK_ID:
		pDestructible = static_cast<TESObjectBOOK*>(apForm);
		break;
	case ARMO_ID:
		pDestructible = static_cast<TESObjectARMO*>(apForm);
		break;
	case LIGH_ID:
		pDestructible = static_cast<TESObjectLIGH*>(apForm);
		break;
	case FURN_ID:
		pDestructible = static_cast<TESFurniture*>(apForm);
		break;
	case AMMO_ID:
		pDestructible = static_cast<TESAmmo*>(apForm);
		break;
	case ACTI_ID:
		pDestructible = static_cast<TESObjectACTI*>(apForm);
		break;
	case MSTT_ID:
		pDestructible = static_cast<BGSMovableStatic*>(apForm);
		break;
	case TREE_ID:
		pDestructible = static_cast<TESObjectTREE*>(apForm);
		break;
	case FLOR_ID:
		pDestructible = static_cast<TESFlora*>(apForm);
		break;
	case DOOR_ID:
		pDestructible = static_cast<TESObjectDOOR*>(apForm);
		break;
	case TACT_ID:
		pDestructible = static_cast<BGSTalkingActivator*>(apForm);
		break;
	case TERM_ID:
		pDestructible = static_cast<BGSTerminal*>(apForm);
		break;
	case PROJ_ID:
		pDestructible = static_cast<BGSProjectile*>(apForm);
		break;
	case NPC__ID:
		pDestructible = static_cast<TESNPC*>(apForm);
		break;
	case CREA_ID:
		pDestructible = static_cast<TESCreature*>(apForm);
		break;
	default:
		break;
	}
	return pDestructible;
}

bool BGSDestructibleObjectForm::IsDestructible(TESForm* apForm) {
	BGSDestructibleObjectForm* pDestructible = GetDestructionForm(apForm);
	if (!pDestructible)
		return false;
	return pDestructible->HasValidStages();
}

int32_t BGSDestructibleObjectForm::GetModelSwapIndex(TESForm* apForm, TESModel* apModelSwap) {
	uint32_t i;
	BGSDestructibleObjectForm* destructionForm = BGSDestructibleObjectForm::GetDestructionForm(apForm);
	if (!destructionForm) {
		return (-1);
	}
	DestructibleObjectData* v4 = destructionForm->BGSDestructibleObjectForm::GetData();
	if (!v4) {
		return (-1);
	}
	for (i = 0; i < ((uint32_t)((int32_t)v4->cNumStages)); ++i) {
		if (static_cast<const void*>(v4->pStagesArray[i]->pReplacementModel) == static_cast<const void*>(apModelSwap)) {
			return (int32_t)i;
		}
	}
	return (-1);
}

TESModel* BGSDestructibleObjectForm::GetModelSwap(TESForm* apForm, int32_t aiModelSwapIndex) {
	BGSDestructibleObjectForm* destructionForm = BGSDestructibleObjectForm::GetDestructionForm(apForm);
	if ((!destructionForm) || (aiModelSwapIndex == ((-1)))) {
		return nullptr;
	}
	DestructibleObjectData* v3 = destructionForm->BGSDestructibleObjectForm::GetData();
	if (!v3) {
		return nullptr;
	}
	if (aiModelSwapIndex >= ((int32_t)v3->cNumStages)) {
		return nullptr;
	}
	return v3->pStagesArray[aiModelSwapIndex]->pReplacementModel;
}

void BGSDestructibleObjectForm::RemoveDestructibleObject(TESObjectREFR* apRef) {
	BGSDestructibleObjectForm::DestructibleObjects.RemoveAt(apRef);
}

uint32_t BGSDestructibleObjectForm::GetSelfDamage(TESObjectREFR* apRef, float afHealth) {
	if (!apRef || afHealth == 0.0) {
		return 0;
	}
	TESForm* ObjectReference = apRef->GetObjectReference();
	BGSDestructibleObjectForm* DestructionForm = BGSDestructibleObjectForm::GetDestructionForm(ObjectReference);
	if (!DestructionForm) {
		return 0;
	}
	DestructibleObjectData* FormHealth_0 = DestructionForm->GetData();
	if (!FormHealth_0) {
		return 0;
	}
	float aIn = afHealth * 100.0 / static_cast<double>(DestructionForm->GetMaxHealth(apRef));
	uint8_t v8 = static_cast<int32_t>(FloatCeil(aIn));
	uint32_t iSelfDamagePerSecond = 0;
	for (uint32_t i = 0; i < FormHealth_0->cNumStages; ++i) {
		DestructibleObjectStage* v6 = FormHealth_0->pStagesArray[i];
		if (v6->cHealthPercentage < static_cast<int32_t>(v8)) {
			break;
		}
		DestructibleObjectStage* v4 = FormHealth_0->pStagesArray[i];
		iSelfDamagePerSecond = v6->iSelfDamagePerSecond;
	}
	return iSelfDamagePerSecond;
}

void BGSDestructibleObjectForm::SetSelfDamage(TESObjectREFR* apRef, uint32_t aiSelfDamage) {
	if (aiSelfDamage) {
		BGSDestructibleObjectForm::DestructibleObjects.SetAt(apRef, aiSelfDamage);
	} else {
		BGSDestructibleObjectForm::DestructibleObjects.RemoveAt(apRef);
	}
}

DestructibleRefsTakingDamageMap* BGSDestructibleObjectForm::GetDestructibleObjects() {
	return &BGSDestructibleObjectForm::DestructibleObjects;
}

// The value goes through x87 as a 64-bit integer; the range is the double difference of the bounds, and the sum is
// rounded to a float local before the ST0 return.
float BSRandom::Float(float afMin, float afMax) {
	uint32_t uiValue = UnsignedInt();
	float fResult = static_cast<float>(uiValue * (static_cast<double>(afMax) - afMin) / 4294967296.0 + afMin);
	return fResult;
}

float BGSExplosion::GetDamage() {
	return Data.fDamage;
}

float FloatCeil(float aIn) {
	float v5 = fFloor(aIn);
	if (!(v5 - aIn >= 0.0)) {
		return v5 + 1.0;
	} else {
		return v5;
	}
}

const NiPoint3& NiBound::GetCenter() const {
	return m_kCenter;
}
