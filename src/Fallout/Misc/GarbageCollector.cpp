#include "Fallout/Misc/GarbageCollector.h"

#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleArray.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/TLSData.h"
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
#include "Reconstruction.hpp"

#include <stdint.h>

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

// Per-address forwarders for the exe's duplicated NiTArray instantiations the collector calls.
// Removes slot uiIndex of the reference array.
// Inferred type: uiIndex: uint32_t: unsigned uses from 0x8691e4
void RemoveAtSwapLast(NiTObjectArray<TESObjectREFR*>* apArray, uint32_t uiIndex) {
	if (uiIndex < apArray->m_usSize) {
		--apArray->m_usSize;
		TESObjectREFR* v2 = apArray->m_pBase[uiIndex];
		apArray->m_pBase[uiIndex] = apArray->m_pBase[apArray->m_usSize];
		apArray->m_pBase[apArray->m_usSize] = nullptr;
		if (v2) {
			--apArray->m_usESize;
		}
	}
}

int32_t au_re_InterlockedExchange(volatile int32_t* apTarget, int32_t aiValue) {
	return InterlockedExchange(apTarget, aiValue);
}

// The exe's cleanup-only C++ EH frame is left out. Each element is copied into the local by NiPointer's out-of-line
// copy constructor (0x559A40, src's SetPointer) and released at the end of each pass (0x45CEC0).
void MoveQueuedNiAVObjects(NiTObjectArray<NiPointer<NiAVObject>>* a1, NiTObjectArray<NiPointer<NiAVObject>>* a2) {
	FNVD_INLINED_LOCAL(NiPointer<NiAVObject>, spObject, std::nullopt, std::nullopt);
	for (uint32_t uiIndex = 0; uiIndex < a1->m_usSize; ++uiIndex) {
		spObject.SetPointer(&a1->operator[](uiIndex));
		if (spObject.m_pObject) {
			NiAVObjectArray_Add(a2, spObject);
		}
		spObject.~NiPointer<NiAVObject>();
	}
}

void MoveQueuedNavMeshes(BSSimpleArray<NavMesh*, 1024>* apSource, BSSimpleArray<NavMesh*, 1024>* apTarget) {
	for (uint32_t i = 0; i < apSource->iSize; ++i) {
		// The exe reads the element through the out-of-line operator[] (0x6A7AD0) into a local whose address Add takes.
		NavMesh* pNavMesh = (*apSource)[i];
		if (pNavMesh) {
			apTarget->Add(pNavMesh);
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out. Each element is copied into a local pointer through the shared
// out-of-line copy (0x559A40), appended when it holds an effect and released through the shared destructor
// (0x45CEC0).
void MoveQueuedTempEffects(NiTObjectArray<NiPointer<BSTempEffect>>* apSource,
	NiTObjectArray<NiPointer<BSTempEffect>>* apTarget) {
	FNVD_INLINED_LOCAL(NiPointer<BSTempEffect>, spEffect, std::nullopt, std::nullopt);
	for (uint32_t uiIndex = 0; uiIndex < apSource->m_usSize; ++uiIndex) {
		spEffect.SetPointer(&(*apSource)[uiIndex]);
		if (spEffect.m_pObject) {
			apTarget->Add(spEffect);
		}
		spEffect.~NiPointer<BSTempEffect>();
	}
}

void Deallocate(void* a1) {
	ourFree(a1);
}

void* Allocate(uint32_t a1) {
	return ourMalloc(a1 <= SIZE_MAX / sizeof(void*) ? sizeof(void*) * a1 : SIZE_MAX);
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

// The exe's cleanup-only C++ EH frame is left out.
void GarbageCollector::Update() {
	GarbageCollector::EnterCrit("GarbageCollector::Update()", 1);
	uint32_t inumObjects = 0;
	uint32_t iremovecount = 0;
	uint32_t ifreemult = 1;
	if (MemoryLevelManager::QInstance()->WasLastCheckedMemLevelCloseToLow()) {
		ifreemult = 2;
	}
	if (GarbageCollector::AnimGarbageA.m_usSize) {
		if (pModelLoader->TryLockModelMap()) {
			inumObjects = GarbageCollector::AnimGarbageA.m_usSize;
			iremovecount = 10 * ifreemult;
			while (inumObjects && iremovecount) {
				--inumObjects;
				--iremovecount;
				Animation* panim = GarbageCollector::AnimGarbageA.operator[](inumObjects);
				GarbageCollector::AnimGarbageA.RemoveAt(inumObjects);
				if (panim) {
					delete panim;
				}
			}
			pModelLoader->UnlockModelMap();
		}
	} else {
		if (GarbageCollector::kBipedAnims.m_usSize) {
			if (pModelLoader->TryLockModelMap()) {
				inumObjects = GarbageCollector::kBipedAnims.m_usSize;
				iremovecount = 10 * ifreemult;
				while (inumObjects && iremovecount) {
					--inumObjects;
					--iremovecount;
					BipedAnim* pbiped = GarbageCollector::kBipedAnims.operator[](inumObjects);
					GarbageCollector::kBipedAnims.RemoveAt(inumObjects);
					if (pbiped) {
						delete pbiped;
					}
				}
				pModelLoader->UnlockModelMap();
			}
		} else {
			if (GarbageCollector::kQueuedBipedAnims.m_usSize) {
				GarbageCollector::CopyArrays(&GarbageCollector::kQueuedBipedAnims, &GarbageCollector::kBipedAnims);
				GarbageCollector::kQueuedBipedAnims.RemoveAll();
			} else {
				if (GarbageCollector::Actor3DGarbageA.m_usSize) {
					inumObjects = GarbageCollector::Actor3DGarbageA.m_usSize;
					iremovecount = 10 * ifreemult;
					while (inumObjects && iremovecount) {
						--inumObjects;
						--iremovecount;
						// RemoveAt (0x869260) builds its by-value result in spObject; the exe releases it at the end of
						// the iteration (0x45CEC0).
						NiPointer<NiAVObject> spObject = GarbageCollector::Actor3DGarbageA.RemoveAt(inumObjects);
						if (spObject != static_cast<NiAVObject*>(nullptr)) {
							spObject.m_pObject->RemoveGarbageMark();
						}
					}
				} else {
					if (GarbageCollector::kQueuedNiAVObjects.m_usSize) {
						MoveQueuedNiAVObjects(&GarbageCollector::kQueuedNiAVObjects,
							&GarbageCollector::Actor3DGarbageA);
						GarbageCollector::kQueuedNiAVObjects.RemoveAll();
					} else {
						if (GarbageCollector::ObjectGarbageA.m_usSize) {
							if (pModelLoader->TryLockModelMap()) {
								inumObjects = GarbageCollector::ObjectGarbageA.m_usSize;
								iremovecount = 20 * ifreemult;
								int32_t iActors = 1;
								while (inumObjects && (iremovecount && iActors)) {
									--inumObjects;
									--iremovecount;
									TESObjectREFR* pref = GarbageCollector::ObjectGarbageA.operator[](0);
									if (pref->IsActor() || pref->GetTargeted()) {
										--iActors;
									}
									if (pref) {
										delete pref;
									}
								}
								pModelLoader->UnlockModelMap();
							}
						} else {
							if (GarbageCollector::kQueuedTESObjectREFRs.m_usSize) {
								GarbageCollector::CopyArrays(&GarbageCollector::kQueuedTESObjectREFRs,
									&GarbageCollector::ObjectGarbageA);
								GarbageCollector::kQueuedTESObjectREFRs.RemoveAll();
							} else {
								if (GarbageCollector::kNavMeshes.iSize) {
									inumObjects = GarbageCollector::kNavMeshes.iSize;
									iremovecount = 5 * ifreemult;
									while (inumObjects && iremovecount) {
										--inumObjects;
										--iremovecount;
										GarbageCollector::kNavMeshes.GetAt(0)->DecRefCount();
										GarbageCollector::kNavMeshes.Remove(0, 1);
									}
								} else {
									if (GarbageCollector::kQueuedNavMeshes.iSize) {
										MoveQueuedNavMeshes(&GarbageCollector::kQueuedNavMeshes,
											&GarbageCollector::kNavMeshes);
										GarbageCollector::kQueuedNavMeshes.Clear(1);
									}
								}
							}
						}
					}
				}
			}
		}
	}
	GarbageCollector::LeaveCrit(1);
}

// The exe's cleanup-only C++ EH frame is left out.
void GarbageCollector::Add(NavMeshPtr& arNavMesh) {
	GarbageCollector kGuard;
	kGuard.EnterCS(32, "GarbageCollector::Add(NavMeshPtr)");
	// The navmesh's NiRefObject part (+0x1C); the exe reads the pointer through the one-field getter 0x559450.
	static_cast<NiRefObject*>(arNavMesh.m_pObject)->IncRefCount();
	if (GetThreadTLSData()->bCollectGarbage) {
		NavMesh* pNavMesh = arNavMesh.operator NavMesh*();
		kNavMeshes.Add(pNavMesh);
	} else {
		NavMesh* pNavMesh = arNavMesh.operator NavMesh*();
		kQueuedNavMeshes.Add(pNavMesh);
	}
	kGuard.LeaveCS();
}

void GarbageCollector::ClearAll(bool abTryEnter) {
	if (!GarbageCollector::bRunning) {
		bool bLocked = 1;
		if (abTryEnter) {
			bLocked = GarbageCollector::TryEnterCrit();
		} else {
			GarbageCollector::EnterCrit("GarbageCollector::ClearAll()", 1);
		}
		if (bLocked) {
			GarbageCollector::bRunning = 1;
			bool bCollectGarbage = GetThreadTLSData()->bCollectGarbage;
			GetThreadTLSData()->bCollectGarbage = 0;
			if (!GarbageCollectorCS::GetFlag(16u) && BSTexturePalette::TryLock()) {
				GarbageCollector::EffectGarbageA.RemoveAll();
				BSTexturePalette::Unlock();
			}
			if (!GarbageCollectorCS::GetFlag(8u) && pModelLoader->TryLockModelMap()) {
				uint32_t icount = GarbageCollector::AnimGarbageA.m_usSize;
				for (uint32_t uiIndex = 0; uiIndex < icount; ++uiIndex) {
					Animation* panim = GarbageCollector::AnimGarbageA.operator[](uiIndex);
					if (panim) {
						delete panim;
					}
				}
				GarbageCollector::AnimGarbageA.RemoveAll();
				pModelLoader->UnlockModelMap();
			}
			if (!GarbageCollectorCS::GetFlag(4u) && pModelLoader->TryLockModelMap()) {
				uint32_t icount_2 = GarbageCollector::kBipedAnims.m_usSize;
				for (uint32_t i = 0; i < icount_2; ++i) {
					BipedAnim* pbiped = GarbageCollector::kBipedAnims.operator[](i);
					if (pbiped) {
						delete pbiped;
					}
				}
				GarbageCollector::kBipedAnims.RemoveAll();
				pModelLoader->UnlockModelMap();
			}
			if (!GarbageCollectorCS::GetFlag(2u)) {
				for (uint32_t j = 0; j < GarbageCollector::Actor3DGarbageA.m_usSize; ++j) {
					if ((&GarbageCollector::Actor3DGarbageA.operator[](j))->m_pObject) {
						(&GarbageCollector::Actor3DGarbageA.operator[](j))->m_pObject->RemoveGarbageMark();
					}
				}
				GarbageCollector::Actor3DGarbageA.RemoveAll();
			}
			if (!GarbageCollectorCS::GetFlag(1u) && pModelLoader->TryLockModelMap()) {
				while (GarbageCollector::ObjectGarbageA.m_usSize) {
					TESObjectREFR* pref = GarbageCollector::ObjectGarbageA.operator[](0);
					if (pref) {
						delete pref;
					}
				}
				GarbageCollector::ObjectGarbageA.RemoveAll();
				pModelLoader->UnlockModelMap();
			}
			if (!GarbageCollectorCS::GetFlag(32u)) {
				for (uint32_t k = 0; k < GarbageCollector::kNavMeshes.iSize; ++k) {
					GarbageCollector::kNavMeshes.GetAt(static_cast<int32_t>(k))->DecRefCount();
				}
				GarbageCollector::kNavMeshes.Clear(1);
			}
			GetThreadTLSData()->bCollectGarbage = bCollectGarbage;
			if (GetThreadTLSData()->bCollectGarbage) {
				GarbageCollector::CopyArrays(&GarbageCollector::kQueuedTESObjectREFRs,
					&GarbageCollector::ObjectGarbageA);
				GarbageCollector::kQueuedTESObjectREFRs.RemoveAll();
				MoveQueuedNiAVObjects(&GarbageCollector::kQueuedNiAVObjects, &GarbageCollector::Actor3DGarbageA);
				GarbageCollector::kQueuedNiAVObjects.RemoveAll();
				GarbageCollector::CopyArrays(&GarbageCollector::kQueuedBipedAnims, &GarbageCollector::kBipedAnims);
				GarbageCollector::kQueuedBipedAnims.RemoveAll();
				MoveQueuedTempEffects(&GarbageCollector::kBSTempEffectArray, &GarbageCollector::EffectGarbageA);
				GarbageCollector::kBSTempEffectArray.RemoveAll();
				MoveQueuedNavMeshes(&GarbageCollector::kQueuedNavMeshes, &GarbageCollector::kNavMeshes);
				GarbageCollector::kQueuedNavMeshes.Clear(1);
			}
			GarbageCollector::bRunning = 0;
			GarbageCollector::LeaveCrit(1);
		}
	}
}

// The exe's cleanup-only C++ EH frame is left out.
void GarbageCollector::Add(BSTempEffect* apEffect) {
	GarbageCollector kGuard;
	kGuard.EnterCS(16, "GarbageCollector::Add(BSTempEffect)");
	if (GetThreadTLSData()->bCollectGarbage) {
		NiPointer<BSTempEffect> spEffect(apEffect);
		EffectGarbageA.Add(spEffect);
	} else {
		NiPointer<BSTempEffect> spEffect(apEffect);
		kBSTempEffectArray.Add(spEffect);
	}
	kGuard.LeaveCS();
}

// The exe's cleanup-only C++ EH frame is left out.
void GarbageCollector::Add(NiAVObject* apObject) {
	GarbageCollector kGuard;
	kGuard.EnterCS(2, "GarbageCollector::Add(NiAVObject)");
	if (apObject) {
		apObject->AddGarbageMark();
	}
	if (GetThreadTLSData()->bCollectGarbage) {
		NiPointer<NiAVObject> spObject(apObject);
		NiAVObjectArray_Add(&Actor3DGarbageA, spObject);
	} else {
		NiPointer<NiAVObject> spObject(apObject);
		NiAVObjectArray_Add(&kQueuedNiAVObjects, spObject);
	}
	kGuard.LeaveCS();
}

// The exe's cleanup-only C++ EH frame is left out.
void GarbageCollector::Add(TESObjectREFR* apRef) {
	GarbageCollector kGuard;
	kGuard.EnterCS(1, "GarbageCollector::Add(TESObjectREFR)");
	if (TESForm::GetFormByNumericID(apRef->iFormID) == apRef) {
		apRef->SetDelete(1);
		apRef->Release3DRelatedData();
		if (apRef->IsActor()) {
			static_cast<Actor*>(apRef)->StopCombat(nullptr);
		}
		if (apRef->IsProjectile()) {
			Projectile* pProjectile = static_cast<Projectile*>(apRef);
			// The exe clears it through the shared one-field setter 0x420BA0 (a trivial accessor).
			pProjectile->pDesiredTarget = nullptr;
			pProjectile->SetFlag(Projectile::ACTIVE_IN_INVENTORY, false);
		}
		pModelLoader->CancelReference(apRef);
		apRef->RemoveFromDataStructures();
		if (apRef->GetObjectReference()->TESForm::GetHasSpokenFlag()) {
			TESWorldSpace* pworld = apRef->GetWorldSpace();
			if (pworld) {
				if (pworld->GetTerrainManager()) {
					pworld->GetTerrainManager()->ReleaseTree(apRef);
				}
			}
		}
		apRef->SetParentCell(0);
		if (!GetThreadTLSData()->bCollectGarbage || (pSaveLoadGame && pSaveLoadGame->GetSaveGameLoading())) {
			if (pModelLoader->TryLockModelMap()) {
				if (apRef) {
					delete apRef;
				}
				pModelLoader->UnlockModelMap();
			} else {
				GarbageCollector::kQueuedTESObjectREFRs.Add(apRef);
			}
		} else {
			GarbageCollector::ObjectGarbageA.Add(apRef);
		}
	} else {
		if (apRef) {
			delete apRef;
		}
	}
	kGuard.LeaveCS();
}

// The exe's cleanup-only C++ EH frame is left out.
void GarbageCollector::Add(BipedAnim* apBiped) {
	GarbageCollector kGuard;
	kGuard.EnterCS(4, "GarbageCollector::Add(BipedAnim)");
	if (!GetThreadTLSData()->bCollectGarbage || (pSaveLoadGame && pSaveLoadGame->GetSaveGameLoading())) {
		if (pModelLoader->TryLockModelMap()) {
			if (apBiped) {
				delete apBiped;
			}
			pModelLoader->UnlockModelMap();
		} else {
			kQueuedBipedAnims.Add(apBiped);
		}
	} else {
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
	if (EffectGarbageA.GetSize()) {
		if (pModelLoader->TryLockModelMap()) {
			EnterCrit("GarbageCollector::ClearTempEffects()", true);
			EffectGarbageA.RemoveAll();
			pModelLoader->UnlockModelMap();
			LeaveCrit(true);
		}
	}
}

void GarbageCollector::CopyArrays(NiTObjectArray<TESObjectREFR*>* apSource, NiTObjectArray<TESObjectREFR*>* apTarget) {
	for (uint32_t i = 0; i < apSource->GetSize(); ++i) {
		TESObjectREFR* pElement = (*apSource)[i];
		if (pElement)
			apTarget->Add(pElement);
	}
}

void GarbageCollector::CopyArrays(NiTObjectArray<BipedAnim*>* apSource, NiTObjectArray<BipedAnim*>* apTarget) {
	for (uint32_t i = 0; i < apSource->GetSize(); ++i) {
		BipedAnim* pElement = (*apSource)[i];
		if (pElement)
			apTarget->Add(pElement);
	}
}

void GarbageCollector::CanRunGarbageCollector(bool abFlagSet) {
	GetThreadTLSData()->bCollectGarbage = abFlagSet;
}

bool GarbageCollectorCS::GetFlag(uint32_t auiFlag) {
	return (auiFlag & uiFlags) != 0;
}

uint32_t NiAVObject::AddGarbageMark() {
	return au_re_InterlockedExchange(reinterpret_cast<volatile int32_t*>(&m_uiRefCount), m_uiRefCount | 0x40000000);
}

uint32_t NiAVObject::RemoveGarbageMark() {
	return au_re_InterlockedExchange(reinterpret_cast<volatile int32_t*>(&m_uiRefCount), m_uiRefCount & 0xBFFFFFFF);
}
