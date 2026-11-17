#include "Fallout/Magic/ActiveEffect.h"

#include "BSAudio/BSAudio.h"
#include "BSAudio/BSSoundHandle.h"
#include "BSCore/BSSimpleList.h"
#include "BSCore/MemoryManager.h"
#include "BSCore/RTTI.h"
#include "BSMain/Setting.h"
#include "Fallout Shared/ActorValue.h"
#include "Fallout Shared/ActorValueOwner.h"
#include "Fallout Shared/GlobalFunc.h"
#include "Fallout Shared/Magic/AlchemyItem.h"
#include "Fallout Shared/Magic/EffectArchetypes.h"
#include "Fallout Shared/Magic/EffectItem.h"
#include "Fallout Shared/Magic/EffectItemList.h"
#include "Fallout Shared/Magic/EffectSetting.h"
#include "Fallout Shared/Magic/IngredientItem.h"
#include "Fallout Shared/Magic/MagicFormulas.h"
#include "Fallout Shared/Magic/MagicItem.h"
#include "Fallout Shared/Magic/MagicSystem.h"
#include "Fallout Shared/Magic/SpellItem.h"
#include "Fallout Shared/TempEffects/BSTempEffect.h"
#include "Fallout Shared/TESCondition.h"
#include "Fallout Shared/TESDataHandler.h"
#include "Fallout Shared/TESForm.h"
#include "Fallout Shared/TESObject.h"
#include "Fallout Shared/TESObjectREFR.h"
#include "Fallout Shared/TESSaveLoadGame.h"
#include "Fallout Shared/TESSound.h"
#include "Fallout/AI/Actor.h"
#include "Fallout/AI/AILinearTaskThreadManager.h"
#include "Fallout/AI/BaseProcess.h"
#include "Fallout/AI/PlayerCharacter.h"
#include "Fallout/AI/ProcessLists.h"
#include "Fallout/Interface/Interface.h"
#include "Fallout/Magic/MagicCaster.h"
#include "Fallout/Magic/MagicHitEffect.h"
#include "Fallout/Magic/MagicModelHitEffect.h"
#include "Fallout/Magic/MagicShaderHitEffect.h"
#include "Fallout/Magic/MagicTarget.h"
#include "Fallout/Magic/VampirismEffect.h"
#include "Fallout/Misc/SaveLoad/BGSLoadFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSLoadGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveFormBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveGameBuffer.h"
#include "Fallout/Misc/SaveLoad/BGSSaveLoadDefines.h"
#include "Fallout/Misc/TaskQueueInterface.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiAlphaProperty.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiMemObject.h"
#include "Gamebryo2.2/CoreLibs/NiMain/NiNode.h"
#include "SDK/MSVC/rtti.h"

#include <cmath>

ActiveEffect::ActiveEffect(MagicCaster* a1, MagicItem* a2, EffectItem* a3)
	: fElapsedSeconds(0.0f), pSpell(a2), pEffect(a3), bActive(true), bStarted(false), bFinished(false), bDone(false),
	  iFlags(0), fMagnitude(static_cast<float>(a3->GetMagnitude())), fDuration(static_cast<float>(a3->GetDuration())),
	  pTarget(nullptr), pCaster(a1), eSpellType(a2->GetSpellType()), pSource(nullptr), pHitEffects(nullptr),
	  pDisplacementSpell(nullptr) {
	if (a3 && !a3->Conditions.IsEmpty()) {
		SetFlag(ActiveEffect::HAS_CONDITIONS);
	}
}

ActiveEffect::~ActiveEffect() {
	for (BSSimpleList<MagicHitEffect*>* plist = pHitEffects; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
		MagicHitEffect* peffect = plist->m_item;
		peffect->bFinished = 1;
		peffect->pEffect = 0;
	}
	if (pHitEffects) {
		pHitEffects->RemoveAll();
	}
	delete pHitEffects;
	if (PersistentSound.IsValid()) {
		PersistentSound.Stop();
		PersistentSound.Release();
	}
}

ActiveEffect* ActiveEffect::Copy() {
	ActiveEffect* v2;
	void* v4 = ourMalloc(sizeof(ActiveEffect));
	if (v4) {
		v2 = ::new (v4) ActiveEffect(pCaster, pSpell, pEffect);
	} else {
		v2 = nullptr;
	}
	CopyData(v2);
	return v2;
}

void ActiveEffect::Finalize() {
	bool vCondition1;
	vCondition1 = HasConditions() || ActiveEffect::GetDisplacementSpell();
	if (vCondition1) {
		EvaluateConditions(0.0, 1);
	}
}

uint16_t ActiveEffect::GetSaveSize(Actor* apActor) {
	uint16_t isize = 28;
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 42u) {
		isize = 29;
		for (BSSimpleList<MagicHitEffect*>* plist = pHitEffects; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
			MagicHitEffect** v2 = &plist->m_item;
			isize += (*v2)->GetSaveSize(this, apActor) + 1;
		}
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() < 72u) {
		if (pSaveLoadGameOLD->GetCurrentVersion() >= 65u) {
			++isize;
		}
	} else {
		isize += 4;
	}
	return isize;
}

void ActiveEffect::SaveGame(Actor* apActor) {
	bool btempNoHitEffect;
	uint8_t chitEffectType;
	uint8_t ccount;
	uint32_t isourceID;
	uint32_t icasterID;
	uint32_t itargetID;
	pSaveLoadGameOLD->SaveGameDataOLD(&fElapsedSeconds, 4u);
	pSaveLoadGameOLD->SaveGameDataOLD(&bActive, 1u);
	pSaveLoadGameOLD->SaveGameDataOLD(&bStarted, 1u);
	pSaveLoadGameOLD->SaveGameDataOLD(&bFinished, 1u);
	pSaveLoadGameOLD->SaveGameDataOLD(&bDone, 1u);
	pSaveLoadGameOLD->SaveGameDataOLD(&bWornEnchantment, 1u);
	pSaveLoadGameOLD->SaveGameDataOLD(&fMagnitude, 4u);
	pSaveLoadGameOLD->SaveGameDataOLD(&fDuration, 4u);
	icasterID = 0;
	if (pCaster) {
		icasterID = pCaster->GetMagicCasterFormID();
	}
	pSaveLoadGameOLD->SaveNumericID(&icasterID, 4u);
	itargetID = 0;
	if (pTarget) {
		itargetID = pTarget->GetMagicTargetFormID();
	}
	pSaveLoadGameOLD->SaveNumericID(&itargetID, 4u);
	isourceID = 0;
	if (pSource) {
		isourceID = pSource->iFormID;
	}
	pSaveLoadGameOLD->SaveNumericID(&isourceID, 4u);
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 42u) {
		ccount = 0;
		uint8_t* pcount = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
		pSaveLoadGameOLD->SaveGameDataOLD(&ccount, 1u);
		for (BSSimpleList<MagicHitEffect*>* plist = pHitEffects; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
			MagicHitEffect* phitEffect = plist->m_item;
			chitEffectType = phitEffect->GetType();
			pSaveLoadGameOLD->SaveGameDataOLD(&chitEffectType, 1u);
			phitEffect->SaveGame(this, apActor);
			++ccount;
		}
		*pcount = ccount;
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() < 72u) {
		if (pSaveLoadGameOLD->GetCurrentVersion() >= 65u) {
			btempNoHitEffect = IsFlagSet(ActiveEffect::NO_HITEFFECT_FORCED);
			pSaveLoadGameOLD->SaveGameDataOLD(&btempNoHitEffect, 1u);
		}
	} else {
		pSaveLoadGameOLD->SaveGameDataOLD(&iFlags, 4u);
	}
}

// The base body of the buffer SaveGame slot.
void ActiveEffect::SaveGame(BGSSaveFormBuffer* apBuffer) {
	uint32_t MagicItemFormID;
	uint32_t MagicTargetFormID;
	uint32_t aiFormID;
	uint8_t chitEffectType;
	apBuffer->SaveDataEndian(reinterpret_cast<char*>(&fElapsedSeconds), 4u, 0);
	apBuffer->SaveDataEndian(reinterpret_cast<char*>(&bActive), 1u, 0);
	apBuffer->SaveDataEndian(reinterpret_cast<char*>(&bStarted), 1u, 0);
	apBuffer->SaveDataEndian(reinterpret_cast<char*>(&bFinished), 1u, 0);
	apBuffer->SaveDataEndian(reinterpret_cast<char*>(&bDone), 1u, 0);
	apBuffer->SaveDataEndian(reinterpret_cast<char*>(&bWornEnchantment), 1u, 0);
	apBuffer->SaveDataEndian(reinterpret_cast<char*>(&fMagnitude), 4u, 0);
	apBuffer->SaveDataEndian(reinterpret_cast<char*>(&fDuration), 4u, 0);
	apBuffer->SaveDataEndian(reinterpret_cast<char*>(&iFlags), 4u, 0);
	if (pCaster) {
		aiFormID = pCaster->GetMagicCasterFormID();
	} else {
		aiFormID = 0;
	}
	apBuffer->SaveFormID(aiFormID, 0);
	if (pTarget) {
		MagicTargetFormID = pTarget->GetMagicTargetFormID();
	} else {
		MagicTargetFormID = 0;
	}
	apBuffer->SaveFormID(MagicTargetFormID, 0);
	if (pDisplacementSpell) {
		MagicItemFormID = pDisplacementSpell->GetMagicItemFormID();
	} else {
		MagicItemFormID = 0;
	}
	apBuffer->SaveFormID(MagicItemFormID, 0);
	apBuffer->SaveFormID(pSource, 0);
	uint32_t isaveGameCount = 0;
	uint32_t isaveGameBufferPos = apBuffer->StartVariableSizedValue();
	for (MagicHitEffect* phitEffect : *pHitEffects) {
		if (phitEffect) {
			chitEffectType = phitEffect->GetType();
			apBuffer->SaveDataEndian(reinterpret_cast<char*>(&chitEffectType), 1u, 0);
			phitEffect->SaveGame(apBuffer);
			++isaveGameCount;
		}
	}
	apBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
}

void ActiveEffect::LoadGame(Actor* apActor) {
	bool btempNoHitEffect;
	uint8_t chitEffectType;
	MagicHitEffect* phitEffect;
	uint8_t ccount;
	uint32_t isourceID;
	uint32_t icasterID;
	uint32_t itargetID;
	pSaveLoadGameOLD->LoadGameDataOLD(&fElapsedSeconds, 4u);
	pSaveLoadGameOLD->LoadGameDataOLD(&bActive, 1u);
	pSaveLoadGameOLD->LoadGameDataOLD(&bStarted, 1u);
	pSaveLoadGameOLD->LoadGameDataOLD(&bFinished, 1u);
	pSaveLoadGameOLD->LoadGameDataOLD(&bDone, 1u);
	pSaveLoadGameOLD->LoadGameDataOLD(&bWornEnchantment, 1u);
	pSaveLoadGameOLD->LoadGameDataOLD(&fMagnitude, 4u);
	pSaveLoadGameOLD->LoadGameDataOLD(&fDuration, 4u);
	pSaveLoadGameOLD->LoadNumericID(&icasterID, 4u);
	pCaster = reinterpret_cast<MagicCaster*>(static_cast<uintptr_t>(icasterID));
	pSaveLoadGameOLD->LoadNumericID(&itargetID, 4u);
	pTarget = reinterpret_cast<MagicTarget*>(static_cast<uintptr_t>(itargetID));
	pSaveLoadGameOLD->LoadNumericID(&isourceID, 4u);
	pSource = reinterpret_cast<TESBoundObject*>(static_cast<uintptr_t>(isourceID));
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 42u) {
		pSaveLoadGameOLD->LoadGameDataOLD(&ccount, 1u);
		for (uint32_t i = 0; i < ccount; ++i) {
			pSaveLoadGameOLD->LoadGameDataOLD(&chitEffectType, 1u);
			phitEffect = 0;
			if (chitEffectType == 5) {
				MagicHitEffect* phitEffect_2;
				MagicModelHitEffect* apMemory =
					static_cast<MagicModelHitEffect*>(NiMemObject::operator new(sizeof(MagicModelHitEffect)));
				if (apMemory) {
					phitEffect_2 = ::new (apMemory) MagicModelHitEffect;
				} else {
					phitEffect_2 = 0;
				}
				phitEffect = phitEffect_2;
			} else {
				if (chitEffectType == 6) {
					MagicHitEffect* phitEffect_3;
					MagicShaderHitEffect* v8 =
						static_cast<MagicShaderHitEffect*>(NiMemObject::operator new(sizeof(MagicShaderHitEffect)));
					if (v8) {
						phitEffect_3 = ::new (v8) MagicShaderHitEffect;
					} else {
						phitEffect_3 = 0;
					}
					phitEffect = phitEffect_3;
				} else {
					Warning("SAVELOAD: Unknown hit effect type: %i", chitEffectType);
				}
			}
			phitEffect->LoadGame(this, apActor);
			if (pHitEffects) {
				pHitEffects->AddHead(phitEffect);
			} else {
				BSSimpleList<MagicHitEffect*>* v3;
				void* v7 = ourMalloc(sizeof(BSSimpleList<MagicHitEffect*>));
				if (v7) {
					v3 = ::new (v7) BSSimpleList<MagicHitEffect*>(phitEffect);
				} else {
					v3 = 0;
				}
				pHitEffects = v3;
			}
		}
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() < 72u) {
		if (pSaveLoadGameOLD->GetCurrentVersion() >= 65u) {
			pSaveLoadGameOLD->LoadGameDataOLD(&btempNoHitEffect, 1u);
			if (btempNoHitEffect) {
				ForceNoHitEffect();
			}
		}
	} else {
		pSaveLoadGameOLD->LoadGameDataOLD(&iFlags, 4u);
	}
}

void ActiveEffect::LoadGame(BGSLoadFormBuffer* apBuffer) {
	uint8_t chitEffectType;
	MagicHitEffect* phitEffect;
	apBuffer->LoadData(&fElapsedSeconds, 4u);
	apBuffer->LoadData(&bActive, 1u);
	apBuffer->LoadData(&bStarted, 1u);
	apBuffer->LoadData(&bFinished, 1u);
	apBuffer->LoadData(&bDone, 1u);
	apBuffer->LoadData(&bWornEnchantment, 1u);
	apBuffer->LoadData(&fMagnitude, 4u);
	apBuffer->LoadData(&fDuration, 4u);
	apBuffer->LoadData(&iFlags, 4u);
	apBuffer->LoadFormID(pCaster);
	apBuffer->LoadFormID(pTarget);
	apBuffer->LoadFormID(pDisplacementSpell);
	uint32_t FormID_0 = apBuffer->LoadFormID();
	TESForm* FormByNumericID = TESForm::GetFormByNumericID(FormID_0);
	pSource = dynamic_cast<TESBoundObject*>(FormByNumericID);
	uint32_t isaveGameCount = apBuffer->LoadVariableSizedValue();
	for (uint32_t iloop = 0; iloop < isaveGameCount; ++iloop) {
		chitEffectType = 0;
		apBuffer->LoadData(&chitEffectType, 1u);
		phitEffect = 0;
		if (chitEffectType == 5) {
			MagicHitEffect* phitEffect_2;
			MagicModelHitEffect* apMemory =
				static_cast<MagicModelHitEffect*>(NiMemObject::operator new(sizeof(MagicModelHitEffect)));
			if (apMemory) {
				phitEffect_2 = (MagicHitEffect*)::new (apMemory) MagicModelHitEffect;
			} else {
				phitEffect_2 = 0;
			}
			phitEffect = phitEffect_2;
		} else {
			if (chitEffectType == 6) {
				MagicHitEffect* phitEffect_3;
				MagicShaderHitEffect* v10 =
					static_cast<MagicShaderHitEffect*>(NiMemObject::operator new(sizeof(MagicShaderHitEffect)));
				if (v10) {
					phitEffect_3 = (MagicHitEffect*)::new (v10) MagicShaderHitEffect;
				} else {
					phitEffect_3 = 0;
				}
				phitEffect = phitEffect_3;
			} else {
				Warning("SAVELOAD: Unknown magic hit effect type: %i", chitEffectType);
			}
		}
		phitEffect->pEffect = this;
		phitEffect->LoadGame(apBuffer);
		if (pHitEffects) {
			pHitEffects->AddHead(phitEffect);
		} else {
			BSSimpleList<MagicHitEffect*>* plist;
			void* pMemory = ourMalloc(sizeof(BSSimpleList<MagicHitEffect*>));
			if (pMemory) {
				plist = ::new (pMemory) BSSimpleList<MagicHitEffect*>(phitEffect);
			} else {
				plist = 0;
			}
			pHitEffects = plist;
		}
	}
}

void ActiveEffect::InitLoadGame(Actor* apActor) {
	uint32_t icasterID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCaster));
	if (icasterID) {
		pCaster = MagicCaster::GetMagicCasterByNumericID(icasterID);
	}
	uint32_t itargetID = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTarget));
	if (itargetID) {
		pTarget = MagicTarget::GetMagicTargetByNumericID(itargetID);
		TESForm* FormByNumericID =
			TESForm::GetFormByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pSource)));
		pSource = dynamic_cast<TESBoundObject*>(FormByNumericID);
	}
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 42u) {
		for (BSSimpleList<MagicHitEffect*>* plist = pHitEffects; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
			MagicHitEffect* m_item = plist->m_item;
			m_item->InitLoadGame(this, apActor);
		}
	}
}

void ActiveEffect::InitLoadGame(BGSLoadGameBuffer* a1) {
	MagicItem* MagicItemByNumericID;
	MagicTarget* MagicTargetByNumericID;
	MagicCaster* MagicCasterByNumericID;
	if (pCaster) {
		MagicCasterByNumericID =
			MagicCaster::GetMagicCasterByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pCaster)));
	} else {
		MagicCasterByNumericID = 0;
	}
	pCaster = MagicCasterByNumericID;
	if (pTarget) {
		MagicTargetByNumericID =
			MagicTarget::GetMagicTargetByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pTarget)));
	} else {
		MagicTargetByNumericID = 0;
	}
	pTarget = MagicTargetByNumericID;
	if (pDisplacementSpell) {
		MagicItemByNumericID =
			MagicItem::GetMagicItemByNumericID(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(pDisplacementSpell)));
	} else {
		MagicItemByNumericID = 0;
	}
	pDisplacementSpell = MagicItemByNumericID;
	for (MagicHitEffect* phitEffect : *pHitEffects) {
		if (phitEffect) {
			phitEffect->InitLoadGame(a1);
		}
	}
}

void ActiveEffect::FinishInitLoadGame(Actor* apActor) {
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 42u) {
		for (BSSimpleList<MagicHitEffect*>* plist = pHitEffects; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
			MagicHitEffect** v2 = &plist->m_item;
			MagicHitEffect* phitEffect = *v2;
			(*v2)->FinishInitLoadGame(this, apActor, 0);
			ReferenceProcessLists.AddTempEffect(phitEffect);
		}
	}
	EffectSetting* EffectSetting_ = GetEffectSetting();
	if (EffectSetting_->IsFlagSet(EffectSetting::PERSIST)) {
		StartSound();
	}
}

// The PC body is the shared empty stub.
void ActiveEffect::Revert(Actor*) {}

bool ActiveEffect::CheckDeletedCaster(MagicCaster* apCaster) {
	uint8_t bResult = 0;
	if (apCaster != pCaster) {
		return (bool)bResult;
	}
	pCaster = nullptr;
	return (bool)1;
}

bool ActiveEffect::IsCausingDamage() {
	return false;
}

void ActiveEffect::FinishLoadGame(BGSLoadFormBuffer* apBuffer) {
	EffectSetting* EffectSetting_ = GetEffectSetting();
	if (EffectSetting_->IsFlagSet(EffectSetting::PERSIST)) {
		StartSound();
	}
	for (BSSimpleList<MagicHitEffect*>* ecx0 = pHitEffects; ecx0; ecx0 = ecx0->m_pkNext) {
		MagicHitEffect* pHitEffect = ecx0->m_item;
		if (pHitEffect) {
			pHitEffect->FinishLoadGame(apBuffer);
			ReferenceProcessLists.AddTempEffect(pHitEffect);
		}
	}
}

void ActiveEffect::CopyData(ActiveEffect* apDestination) {
	apDestination->fElapsedSeconds = fElapsedSeconds;
	apDestination->pSpell = pSpell;
	apDestination->pEffect = pEffect;
	apDestination->bActive = (bool)((uint8_t)bActive);
	apDestination->bStarted = (bool)((uint8_t)bStarted);
	apDestination->bFinished = (bool)((uint8_t)bFinished);
	apDestination->bDone = (bool)((uint8_t)bDone);
	apDestination->bWornEnchantment = (bool)((uint8_t)bWornEnchantment);
	apDestination->fMagnitude = fMagnitude;
	apDestination->fDuration = fDuration;
	apDestination->pTarget = pTarget;
	apDestination->pCaster = pCaster;
	apDestination->eSpellType = static_cast<MagicSystem::SpellType>((int32_t)eSpellType);
	apDestination->pSource = pSource;
	apDestination->iFlags = iFlags;
	apDestination->pDisplacementSpell = pDisplacementSpell;
}

bool ActiveEffect::CheckCast(MagicCaster* apCaster) {
	return true;
}

bool ActiveEffect::CheckTarget(MagicTarget* apTarget) {
	return true;
}

MagicItem* ActiveEffect::GetSpell() {
	return pSpell;
}

MagicSystem::SpellType ActiveEffect::GetSpellType() {
	return eSpellType;
}

EffectItem* ActiveEffect::GetEffectItem() {
	return pEffect;
}

TESBoundObject* ActiveEffect::GetEnchantmentSource() {
	return pSource;
}

MagicTarget* ActiveEffect::GetTarget() {
	return pTarget;
}

void ActiveEffect::ForceNoHitEffect() {
	SetFlag(NO_HITEFFECT_SHADER);
	SetFlag(NO_HITEFFECT_NIF);
}

// KEEP: the exe returns bDone in AL while retaining the upper EAX bytes from `this`.
int8_t ActiveEffect::IsDone() {
	return bDone;
}

void ActiveEffect::SetFlag(ActiveEffect::ActiveEffectFlags aiFlag) {
	iFlags |= aiFlag;
}

// Returns whether the FROM_WORN_ENCHANTMENT bit is set.
bool ActiveEffect::FromWornEnchantment() {
	return (iFlags & FROM_WORN_ENCHANTMENT) != 0;
}

MagicItem* ActiveEffect::GetDisplacementSpell() {
	return pDisplacementSpell;
}

bool ActiveEffect::IsQueuedForRemoval() const {
	return (iFlags & 0x80000000) != 0;
}

// Sets or clears the queued-for-removal bit (0x80000000) that IsQueuedForRemoval tests. Tests the whole byte; bool
// would assume 0/1.
void ActiveEffect::SetQueuedForRemoval(uint8_t abQueued) {
	if (abQueued) {
		iFlags |= 0x80000000;
	} else {
		iFlags &= 0x7FFFFFFF;
	}
}

void ActiveEffect::UpdateEffect(float afSecondsElapsed) {
	if (HasConditions() || GetDisplacementSpell()) {
		EvaluateConditions(afSecondsElapsed, 0);
	}
	if (afSecondsElapsed > 0.0 && !bDone) {
		MagicItem* pspell = pSpell;
		MagicTarget* ptarget = pTarget;
		TESObjectREFR* ptargetStatsObject = ptarget->GetTargetStatsObject();
		bool bpermanent = MagicSystem::IsSpellTypePermanent(eSpellType);
		if (!bStarted && !bFinished && bActive) {
			if (ptargetStatsObject == pPlayer && Interface::IsInMenuMode()) {
				SetFlag(ActiveEffect::DEFERRED_SOUND);
			} else {
				StartSound();
			}
			Start();
			if (!bDone && !bFinished) {
				if (!IsFlagSet(ActiveEffect::NO_HITEFFECT_FORCED)) {
					EffectSetting* EffectSetting_ = GetEffectSetting();
					if (!EffectSetting_->IsFlagSet(EffectSetting::NO_HIT_EFFECT)) {
						if (eSpellType != MagicSystem::ABILITY ||
							((GetEffectSetting(), NiAlphaProperty::GetType()) && fMagnitude < 100.0)) {
							if (eSpellType == MagicSystem::ABILITY) {
								SetFlag(ActiveEffect::NO_HITEFFECT_NIF);
							}
							// Food shows no hit effect: the spell as an ingredient or a potion, each asked through its
							// EffectItemList's IsFood.
							IngredientItem* pingredient = nullptr;
							if (pspell->GetSpellType() == MagicSystem::WORTCRAFT) {
								pingredient = static_cast<IngredientItem*>(pspell);
							}
							AlchemyItem* ppotion = nullptr;
							if (pspell->GetSpellType() == MagicSystem::POTION) {
								ppotion = static_cast<AlchemyItem*>(pspell);
							}
							bool bfood;
							if (pingredient) {
								bfood = static_cast<EffectItemList*>(pingredient)->IsFood();
							} else if (ppotion) {
								bfood = static_cast<EffectItemList*>(ppotion)->IsFood();
							} else {
								bfood = false;
							}
							if (!bfood) {
								if (ptargetStatsObject == pPlayer && Interface::IsInMenuMode()) {
									SetFlag(ActiveEffect::DEFERRED_HITEFFECT);
								} else {
									if (this->pHitEffects) {
										this->pHitEffects->RemoveAll();
										BSSimpleList<MagicHitEffect*>* pHitEffects = this->pHitEffects;
										if (pHitEffects) {
											delete pHitEffects;
										}
										this->pHitEffects = 0;
									}
									if (AILinearTaskThreadManager::ShouldQueue3DTask()) {
										TaskQueueInterface* Instance = TaskQueueInterface::GetInstance();
										Instance->QueueMHEInstantiateHitEffects(this);
									} else {
										this->pHitEffects = MagicHitEffect::InstantiateHitEffects(this);
									}
								}
							}
						}
					}
				}
				bStarted = 1;
			}
		}
		if (!bDone) {
			double v8;
			float fupdateTime = afSecondsElapsed;
			if (bpermanent || (FromWornEnchantment() || (v8 = fElapsedSeconds + afSecondsElapsed, !(fDuration < v8)))) {
				fElapsedSeconds = fElapsedSeconds + afSecondsElapsed;
			} else {
				double LastTimeProcessed = fDuration;
				fupdateTime = LastTimeProcessed - fElapsedSeconds;
				fElapsedSeconds = fDuration;
			}
			if (bActive) {
				bool bdeferredHitEffect = IsFlagSet(ActiveEffect::DEFERRED_HITEFFECT);
				bool bdeferredSound = IsFlagSet(ActiveEffect::DEFERRED_SOUND);
				if ((bdeferredHitEffect || bdeferredSound) && ptargetStatsObject == pPlayer &&
					!Interface::IsInMenuMode()) {
					if (bdeferredHitEffect) {
						if (this->pHitEffects) {
							this->pHitEffects->RemoveAll();
							BSSimpleList<MagicHitEffect*>* v16 = this->pHitEffects;
							if (v16) {
								delete v16;
							}
							this->pHitEffects = 0;
						}
						this->pHitEffects = MagicHitEffect::InstantiateHitEffects(this);
						ClearFlag(ActiveEffect::DEFERRED_HITEFFECT);
					}
					if (bdeferredSound) {
						StartSound();
						ClearFlag(ActiveEffect::DEFERRED_SOUND);
					}
				}
				Update(fupdateTime);
			}
			if (!bpermanent && !FromWornEnchantment()) {
				if (pEffect->pEffectSetting->IsFlagSet(EffectSetting::NO_DURATION)) {
					bDone = 1;
				} else {
					if (static_cast<double>(fElapsedSeconds) >= fDuration) {
						bDone = 1;
					}
				}
			}
			if (pTarget->MagicTargetIsActor()) {
				Actor* pActor = static_cast<Actor*>(pTarget);
				if (pActor) {
					if (pActor->IsDead(1)) {
						EffectSetting* v4 = GetEffectSetting();
						if (!v4->IsFlagSet(EffectSetting::NO_DEATH_DISPEL)) {
							bDone = 1;
						}
					}
				}
			}
		}
	}
	if (bStarted && ((bDone || !bActive) && !bFinished)) {
		Finish();
		bFinished = 1;
		if (PersistentSound.IsValid()) {
			PersistentSound.Stop();
		}
	}
}

void ActiveEffect::EvaluateConditions(float afTimeDelta, bool abForceUpdate) {
	if (HasConditions() || GetDisplacementSpell()) {
		MagicTarget* pTarget = GetTarget();
		if (pTarget) {
			if (pTarget->GetTargetStatsObject()) {
				float v8;
				double v9;
				float v10;
				float fActiveEffectConditionUpdateInterval = ::fActiveEffectConditionUpdateInterval.Float();
				if (abForceUpdate ||
					(v10 = fElapsedSeconds / fActiveEffectConditionUpdateInterval,
						(v9 = std::floor(v10),
							(v8 = (fElapsedSeconds + afTimeDelta) / fActiveEffectConditionUpdateInterval,
								v9 != std::floor(v8))))) {
					TESObjectREFR* pTargetActor = pTarget->GetTargetStatsObject();
					EffectItem* CastingType = GetEffectItem();
					TESCondition* Data = &CastingType->Conditions;
					bool bIsTrue = Data->IsTrue(pTargetActor, 0);
					bool v12 = !CheckDisplacementSpellOnTarget() && bIsTrue;
					if (v12 && !bActive && bFinished && !bDone) {
						bStarted = 0;
						bFinished = 0;
					}
					bActive = v12;
				}
			}
		}
	}
}

// Effects cast from worn armour or clothing are marked as coming from a worn enchantment.
void ActiveEffect::SetEnchantmentSource(TESBoundObject* apSource) {
	pSource = apSource;
	if (pSource) {
		if (pSource->GetFormType() == ARMO_ID || pSource->GetFormType() == CLOT_ID) {
			SetFlag(FROM_WORN_ENCHANTMENT);
		}
	}
}

void ActiveEffect::Dispel(bool abNow) {
	bDone = (bool)1;
	SetFlag(WAS_DISPELED);
	if (abNow) {
		UpdateEffect(0.0);
	}
}

void ActiveEffect::ClearFlag(ActiveEffect::ActiveEffectFlags aiFlag) {
	iFlags &= (~((int32_t)aiFlag));
}

void ActiveEffect::SetMagnitude(float afMagnitude) {
	fMagnitude = afMagnitude;
}

// Scales the duration of a no-magnitude effect, else the magnitude (floored, at least 1), by afMultiplier; effects with
// neither are left alone. The products are rounded to float before the floors, as the game does.
void ActiveEffect::AdjustEffectiveness(float afMultiplier) {
	if (GetEffectItem()->GetEffectSetting()->CheckEffectFlag(EffectSetting::NO_DURATION | EffectSetting::NO_MAGNITUDE))
		return;
	if (afMultiplier == 1.0)
		return;
	if (GetEffectItem()->GetEffectSetting()->IsFlagSet(EffectSetting::NO_MAGNITUDE)) {
		float fDurationScaled = GetDuration() * afMultiplier;
		SetDuration(fDurationScaled);
	} else {
		float fMagnitudeScaled = GetMagnitude() * afMultiplier;
		float fNewMagnitude;
		if (FloatFloor(fMagnitudeScaled) < 1.0) {
			fNewMagnitude = 1.0f;
		} else {
			float fMagnitudeScaledAgain = GetMagnitude() * afMultiplier;
			fNewMagnitude = FloatFloor(fMagnitudeScaledAgain);
		}
		SetMagnitude(fNewMagnitude);
	}
}

void ActiveEffect::SetDuration(float afDuration) {
	fDuration = afDuration;
}

float ActiveEffect::GetDuration() {
	return fDuration;
}

float ActiveEffect::GetMagnitude() {
	return fMagnitude;
}

bool ActiveEffect::CheckDisplacementSpellOnTarget() {
	bool bresult = 0;
	bool bignoreAddiction = 0;
	if (this->pTarget->MagicTargetIsActor()) {
		Actor* pactor = static_cast<Actor*>(this->pTarget);
		if (pactor) {
			if (pactor->GetActorValue(ActorValue::IGNORE_CRIPPLED_LIMBS) > 0) {
				bignoreAddiction = 1;
			}
		}
	}
	SpellItem* pspell = dynamic_cast<SpellItem*>(pSpell);
	if (pDisplacementSpell) {
		if (this->pTarget) {
			MagicTarget* Target = this->pTarget;
			for (BSSimpleList<ActiveEffect*>* plist = Target->GetActiveEffectList(); plist && !bresult;
				plist = plist->m_pkNext) {
				ActiveEffect* pitem = plist->m_item;
				if (pitem) {
					if (pitem->bActive) {
						if (pitem->pSpell == pDisplacementSpell) {
							bresult = 1;
						} else {
							if (pitem->pSpell->GetSpellType() == MagicSystem::POTION) {
								AlchemyItem* ppotion = dynamic_cast<AlchemyItem*>(pitem->pSpell);
								if (ppotion) {
									bresult = ppotion->GetAddictionItem() == pspell;
								}
							}
						}
					}
				}
			}
		}
	}
	if (bresult && bignoreAddiction) {
		return 0;
	}
	return bresult;
}

void ActiveEffect::StartSound() {
	uint32_t isoundFileID;
	FNVD_INLINED_LOCAL(BSSoundHandle, a2, std::nullopt, std::nullopt);
	FNVD_INLINED_LOCAL(BSSoundHandle, soundHandle_2, std::nullopt, std::nullopt);
	EffectSetting* EffectSetting_ = GetEffectSetting();
	if (EffectSetting_->data.pHitSound) {
		EffectSetting* v2 = GetEffectSetting();
		isoundFileID = v2->data.pHitSound->iFormID;
	} else {
		isoundFileID = 0;
	}
	if (isoundFileID) {
		if (pTarget) {
			MagicItem* Spell = pSpell;
			if (Spell->GetSpellType() != MagicSystem::ABILITY) {
				Actor* ptargetActor;
				BSSoundHandle soundHandle;
				if (pTarget) {
					ptargetActor = pTarget->GetTargetAsActor();
				} else {
					ptargetActor = 0;
				}
				EffectSetting* v4 = GetEffectSetting();
				bool bpersistent = v4->IsFlagSet(EffectSetting::PERSIST);
				EffectSetting* v5 = GetEffectSetting();
				if (v5->data.pHitSound) {
					EffectSetting* v6 = GetEffectSetting();
					TESSound* v7 = v6->data.pHitSound;
					bpersistent = TESSound::ConvertLoopFlag(v7->GetData().iFlags) != 0;
				}
				if (ptargetActor) {
					uint32_t uiflags = bpersistent ? 274 : 258;
					BSSoundHandle* arHandle = ptargetActor->PlaySound(&a2, isoundFileID, bpersistent, uiflags, 0);
					soundHandle.operator=(*arHandle);
				} else {
					MagicTarget* Target = pTarget;
					TESObjectREFR* ptargetObject = Target->GetTargetStatsObject();
					if (ptargetObject) {
						BSAudio* v9 = &BSAudio::QInstance();
						v9->GetSoundHandleByNumericID(&soundHandle_2, isoundFileID, 258u);
						NiPoint3 v20 = ptargetObject->GetLocationOnReference();
						soundHandle_2.SetPosition(v20.x, v20.y, v20.z);
						soundHandle_2.Play(bpersistent);
					}
				}
				if (bpersistent) {
					if (PersistentSound.IsValid()) {
						PersistentSound.Stop();
						PersistentSound.Release();
					}
					PersistentSound.operator=(soundHandle);
				}
			}
		}
	}
}

ActiveEffect* ActiveEffect::Activate(MagicCaster* apCaster, MagicItem* apMagicItem, EffectItem* apEffectItem,
	TESBoundObject* apObject) {
	EffectSetting* psetting;
	ActiveEffect* presult = 0;
	if (apEffectItem) {
		psetting = apEffectItem->pEffectSetting;
	} else {
		psetting = 0;
	}
	EffectArchetypes::InstantiateFunction pInstantiate =
		EffectArchetypes::Archetypes[psetting->GetEffectArchetype()].pInstantiateFunction;
	if (pInstantiate) {
		presult = pInstantiate(apCaster, apMagicItem, apEffectItem);
	}
	if (presult) {
		presult->SetEnchantmentSource(apObject);
	}
	return presult;
}

ActiveEffect* ActiveEffect::LoadActiveEffect(BGSLoadFormBuffer* apLoadGameBuffer) {
	int32_t EffectArchetype;
	MagicItem* pspell;
	EffectItem* peffect;
	uint8_t ceffectIndex;
	uint32_t ispellID = 0;
	apLoadGameBuffer->LoadFormID(ispellID);
	uint32_t isavedSpellID = ispellID;
	if (ispellID) {
		pspell = MagicItem::GetMagicItemByNumericID(ispellID);
	} else {
		pspell = 0;
	}
	ceffectIndex = 0;
	apLoadGameBuffer->LoadData(&ceffectIndex, 1u);
	uint32_t ieffectArchetypeID = apLoadGameBuffer->LoadVariableSizedValue();
	if (pspell &&
		((peffect = pspell->GetEffectItemByIndex(ceffectIndex)) != 0 &&
			(EffectArchetype = peffect->pEffectSetting->GetEffectArchetype(), EffectArchetype == ieffectArchetypeID))) {
		apLoadGameBuffer->LoadVariableSizedValue();
		ActiveEffect* pactiveEffect = ActiveEffect::Activate(0, pspell, peffect, 0);
		pactiveEffect->LoadGame(apLoadGameBuffer);
		return pactiveEffect;
	} else {
		apLoadGameBuffer->AdvanceBuffer(apLoadGameBuffer->LoadVariableSizedValue());
		SaveGameWarning("GAME EFFECTS: Could not load active effect with spell %08X, effect index %i, and archetype ID "
						"%i",
			isavedSpellID, ceffectIndex, ieffectArchetypeID);
		return 0;
	}
}

void ActiveEffect::LoadActiveEffectList(BGSLoadFormBuffer* apLoadGameBuffer,
	BSSimpleList<ActiveEffect*>* apActiveEffectList) {
	RevertActiveEffectList(apLoadGameBuffer, apActiveEffectList);
	uint32_t uiCount = apLoadGameBuffer->LoadVariableSizedValue();
	for (uint32_t i = 0; i < uiCount; ++i) {
		ActiveEffect* pEffect = LoadActiveEffect(apLoadGameBuffer);
		if (pEffect)
			apActiveEffectList->Insert(pEffect, &MagicTarget::ActiveEffectListCompareFn);
	}
}

void ActiveEffect::LoadActiveEffectListTES(BSSimpleList<ActiveEffect*>* apActiveEffectList, Actor* apActor) {
	uint32_t iblockHeader;
	uint16_t icount;
	uint16_t iblockSize = 0;
	uint8_t* istartBufferPosition = 0;
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		pSaveLoadGameOLD->LoadGameDataOLD(&iblockHeader, 4u);
		if (iblockHeader != 0x424C4F4B) {
			LoadFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
			if (pheader) {
				TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
				Warning("SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Currently "
						"loading form is %08X %s with version %i and flags %08X",
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\ActiveEffect.cpp", 1000, pheader->iFormID,
					pform->GetFormEditorID(), pheader->cVersion, pheader->iFlags);
			} else {
				Warning("SAVELOAD: (LoadGame Buffer error) Block Header is incorrect in file %s on line %i.  Current "
						"version is %i",
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\ActiveEffect.cpp", 1000,
					pSaveLoadGameOLD->GetCurrentVersion());
			}
		}
		istartBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
		pSaveLoadGameOLD->LoadGameDataOLD(&iblockSize, 2u);
	}
	float fvampirismMagnitude = 0.0f;
	for (BSSimpleList<ActiveEffect*>* plist = apActiveEffectList; plist && !plist->IsEmpty(); plist = plist->m_pkNext) {
		ActiveEffect* peffect = plist->m_item;
		if (dynamic_cast<VampirismEffect*>(peffect)) {
			fvampirismMagnitude = peffect->fMagnitude;
		}
		if (peffect) {
			delete peffect;
		}
	}
	apActiveEffectList->RemoveAll();
	pSaveLoadGameOLD->LoadGameDataOLD(&icount, 2u);
	for (int32_t i = 0; i < icount; ++i) {
		ActiveEffect* peffect_2 = ActiveEffect::LoadActiveEffectTES(apActor);
		// The game reads apActor's current AI process here (a trivial accessor) and drops the result.
		if (dynamic_cast<VampirismEffect*>(peffect_2)) {
			if (apActor) {
			}
		}
		if (peffect_2) {
			apActiveEffectList->Insert(peffect_2, &MagicTarget::ActiveEffectListCompareFn);
		}
	}
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		uint8_t* icurrentBufferPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
		LoadFormHeader* pheader_2 = pSaveLoadGameOLD->m_pCurrentlyLoadingFormHeader;
		uint8_t* iendBufferPosition = &istartBufferPosition[iblockSize];
		if (pheader_2) {
			TESForm* pform_2 = TESForm::GetFormByNumericID(pheader_2->iFormID);
			if (icurrentBufferPosition <= iendBufferPosition) {
				if (icurrentBufferPosition < iendBufferPosition) {
					uint32_t iFlags = pheader_2->iFlags;
					int32_t cVersion = pheader_2->cVersion;
					const char* v3 = pform_2->GetFormEditorID();
					Warning("SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Currently loading "
							"form is %08X %s with version %i and flags %08X",
						static_cast<int32_t>(iendBufferPosition - icurrentBufferPosition),
						"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\ActiveEffect.cpp", 1039,
						pheader_2->iFormID, v3, cVersion, iFlags);
				}
			} else {
				uint32_t v6 = pheader_2->iFlags;
				int32_t v4 = pheader_2->cVersion;
				const char* v2 = pform_2->GetFormEditorID();
				Warning("SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Currently loading form "
						"is %08X %s with version %i and flags %08X",
					static_cast<int32_t>(icurrentBufferPosition - iendBufferPosition),
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\ActiveEffect.cpp", 1039,
					pheader_2->iFormID, v2, v4, v6);
			}
		} else {
			if (icurrentBufferPosition <= iendBufferPosition) {
				if (icurrentBufferPosition < iendBufferPosition) {
					int32_t CurrentVersion = pSaveLoadGameOLD->GetCurrentVersion();
					Warning("SAVELOAD: LoadGame Buffer underrun of %i bytes in file %s on line %i.  Current version is "
							"%i",
						static_cast<int32_t>(iendBufferPosition - icurrentBufferPosition),
						"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\ActiveEffect.cpp", 1039,
						CurrentVersion);
				}
			} else {
				int32_t v8 = pSaveLoadGameOLD->GetCurrentVersion();
				Warning("SAVELOAD: LoadGame Buffer overrun of %i bytes in file %s on line %i.  Current version is %i",
					static_cast<int32_t>(icurrentBufferPosition - iendBufferPosition),
					"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\ActiveEffect.cpp", 1039, v8);
			}
		}
	}
}

void ActiveEffect::RevertActiveEffectList(BGSLoadFormBuffer* apLoadGameBuffer,
	BSSimpleList<ActiveEffect*>* apActiveEffectList) {
	for (ActiveEffect* pactiveEffect : *apActiveEffectList) {
		if (pactiveEffect) {
			pactiveEffect->Revert(apLoadGameBuffer);
			delete pactiveEffect;
		}
	}
	apActiveEffectList->RemoveAll();
}

void ActiveEffect::InitLoadGameActiveEffectList(BSSimpleList<ActiveEffect*>* apActiveEffectList, Actor* apActor) {
	while (apActiveEffectList && !apActiveEffectList->IsEmpty()) {
		ActiveEffect* m_item = apActiveEffectList->m_item;
		m_item->InitLoadGame(apActor);
		apActiveEffectList = apActiveEffectList->m_pkNext;
	}
}

void ActiveEffect::FinishInitLoadGameActiveEffectList(BSSimpleList<ActiveEffect*>* apActiveEffectList, Actor* apActor) {
	while (apActiveEffectList && !apActiveEffectList->IsEmpty()) {
		ActiveEffect* m_item = apActiveEffectList->m_item;
		m_item->FinishInitLoadGame(apActor);
		apActiveEffectList = apActiveEffectList->m_pkNext;
	}
	if (apActor) {
		if (apActor->pCurrentProcess) {
			BaseProcess* CurrentAIProcess = apActor->pCurrentProcess;
			CurrentAIProcess->SetShouldRefreshMagicShadersEffects(1, 1, 1);
		}
	}
}

void ActiveEffect::FinishLoadActiveEffectList(BGSLoadFormBuffer* apLoadGameBuffer,
	BSSimpleList<ActiveEffect*>* apActiveEffectList) {
	while (apActiveEffectList) {
		ActiveEffect* pactiveEffect = apActiveEffectList->m_item;
		if (pactiveEffect) {
			pactiveEffect->FinishLoadGame(apLoadGameBuffer);
		}
		apActiveEffectList = apActiveEffectList->m_pkNext;
	}
}

void ActiveEffect::RevertActiveEffectList(BSSimpleList<ActiveEffect*>* apActiveEffectList, Actor* apActor) {
	while (apActiveEffectList && !apActiveEffectList->IsEmpty()) {
		ActiveEffect* m_item = apActiveEffectList->m_item;
		m_item->Revert(apActor);
		apActiveEffectList = apActiveEffectList->m_pkNext;
	}
}

void ActiveEffect::SaveActiveEffectList(BGSSaveFormBuffer* apSaveGameBuffer,
	BSSimpleList<ActiveEffect*>* apActiveEffectList) {
	uint32_t isaveGameCount = 0;
	uint32_t isaveGameBufferPos = apSaveGameBuffer->StartVariableSizedValue();
	while (apActiveEffectList) {
		ActiveEffect* a2 = apActiveEffectList->m_item;
		if (a2) {
			ActiveEffect::SaveActiveEffect(apSaveGameBuffer, a2);
			++isaveGameCount;
		}
		apActiveEffectList = apActiveEffectList->m_pkNext;
	}
	apSaveGameBuffer->SaveVariableSizedValue(isaveGameCount, isaveGameBufferPos);
}

void ActiveEffect::SaveActiveEffectList(BSSimpleList<ActiveEffect*>* apActiveEffectList, Actor* apActor) {
	uint32_t iblockHeader;
	uint16_t icount;
	uint16_t iblockSize;
	iblockSize = 0;
	uint16_t* pblockSize = 0;
	uint8_t* itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
	if (bDebugSaveBuffer.Bool()) {
		itempStartPosition = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
	}
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		// The block tag 'BLOK', written as a little-endian 4-byte value ("KOLB").
		iblockHeader = 0x424C4F4B;
		pSaveLoadGameOLD->SaveGameDataOLD(&iblockHeader, 4u);
		pblockSize = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
		pSaveLoadGameOLD->SaveGameDataOLD(&iblockSize, 2u);
	}
	icount = 0;
	uint16_t* pcount = static_cast<uint16_t*>(pSaveLoadGameOLD->m_pBuffer);
	pSaveLoadGameOLD->SaveGameDataOLD(&icount, 2u);
	while (apActiveEffectList && !apActiveEffectList->IsEmpty()) {
		ActiveEffect::SaveActiveEffect(apActiveEffectList->m_item, apActor);
		++icount;
		apActiveEffectList = apActiveEffectList->m_pkNext;
	}
	*pcount = icount;
	if (bDebugSaveBuffer.Bool()) {
		uint8_t* BufferPointer = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
		SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
		if (pheader) {
			TESForm* pform = TESForm::GetFormByNumericID(pheader->iFormID);
			uint32_t iFlags = pheader->iFlags;
			const char* v2 = pform->GetFormEditorID();
			InitializationFlow("SaveGame(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
				BufferPointer - itempStartPosition, pheader->iFormID, v2, iFlags, 992,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\ActiveEffect.cpp");
		} else {
			InitializationFlow("SaveGame(): %-5i ending at line %i in file %s", BufferPointer - itempStartPosition, 992,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\ActiveEffect.cpp");
		}
	}
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		// The block's size is the byte distance from its size field to the buffer's end, unsigned compares; the
		// size field is raw save-buffer memory, read here as bytes.
		uint8_t* pend = static_cast<uint8_t*>(pSaveLoadGameOLD->m_pBuffer);
		uint8_t* pstart = reinterpret_cast<uint8_t*>(pblockSize);
		if (pend > pstart + 0xFFFF) {
			Warning("SAVELOAD: Save Game Block in file %s on line %i is greater than maximum short size",
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\ActiveEffect.cpp", 992);
		}
		*pblockSize = static_cast<uint16_t>(pend - pstart);
	}
}

void ActiveEffect::SaveActiveEffect(ActiveEffect* apEffect, Actor* apActor) {
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 42u) {
		uint16_t usSaveSize = apEffect->GetSaveSize(apActor);
		pSaveLoadGameOLD->SaveGameDataOLD(&usSaveSize, 2u);
	}
	uint32_t uiSpellID = apEffect->pSpell->GetMagicItemFormID();
	pSaveLoadGameOLD->SaveNumericID(&uiSpellID, 4u);
	uint8_t ucEffectIndex = apEffect->pSpell->GetEffectItemIndex(apEffect->pEffect);
	pSaveLoadGameOLD->SaveGameDataOLD(&ucEffectIndex, 1u);
	apEffect->SaveGame(apActor);
}

uint16_t ActiveEffect::GetActiveEffectSaveSize(ActiveEffect* apEffect, Actor* apActor) {
	int16_t v3 = 0;
	if (pSaveLoadGameOLD->GetCurrentVersion() >= 42u) {
		v3 = 2;
	}
	return apEffect->GetSaveSize(apActor) + (v3 + 5);
}

uint16_t ActiveEffect::GetActiveEffectListSaveSize(BSSimpleList<ActiveEffect*>* apActiveEffectList, Actor* apActor) {
	int16_t v6 = 0;
	if (pSaveLoadGameOLD->UseSaveGameBlocks()) {
		v6 = 6;
	}
	uint16_t isize = v6 + 2;
	while (apActiveEffectList && !apActiveEffectList->IsEmpty()) {
		ActiveEffect** v2 = &apActiveEffectList->m_item;
		isize += ActiveEffect::GetActiveEffectSaveSize(*v2, apActor);
		apActiveEffectList = apActiveEffectList->m_pkNext;
	}
	if (bDebugSaveBuffer.Bool()) {
		SaveFormHeader* pheader = pSaveLoadGameOLD->m_pCurrentlySavingFormHeader;
		if (pheader) {
			TESForm* FormByNumericID = TESForm::GetFormByNumericID(pheader->iFormID);
			InitializationFlow("GetSaveSize(): %-5i for form %08X %s with flags %08X ending at line %i in file %s",
				isize, pheader->iFormID, FormByNumericID->GetFormEditorID(), pheader->iFlags, 968,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\ActiveEffect.cpp");
		} else {
			InitializationFlow("GetSaveSize(): %-5i ending at line %i in file %s", isize, 968,
				"D:\\_Fallout3\\Platforms\\Common\\Code\\Fallout\\Magic\\ActiveEffect.cpp");
		}
	}
	return isize;
}

void ActiveEffect::operator delete(void* apMemory) {
	ourFree(apMemory);
}

ActiveEffect* ActiveEffect::LoadActiveEffectTES(Actor* a1) {
	ActiveEffect* v2;
	EffectItem* effectItemByIndex;
	uint8_t v4;
	uint32_t apFormID;
	uint16_t apData = 28;
	if (((uint32_t)((int32_t)pSaveLoadGameOLD->GetCurrentVersion())) >= 0x2Au) {
		pSaveLoadGameOLD->LoadGameDataOLD(&apData, 2u);
	}
	pSaveLoadGameOLD->LoadNumericID(&apFormID, 4u);
	pSaveLoadGameOLD->LoadGameDataOLD(&v4, 1u);
	MagicItem* magicItemByNumericID = MagicItem::GetMagicItemByNumericID(apFormID);
	if (magicItemByNumericID &&
		((effectItemByIndex = static_cast<EffectItemList*>(magicItemByNumericID)->GetEffectItemByIndex(v4)) != 0)) {
		v2 = ActiveEffect::Activate(nullptr, magicItemByNumericID, effectItemByIndex, nullptr);
		v2->LoadGame(a1);
		return v2;
	} else {
		pSaveLoadGameOLD->AdvanceBuffer(apData);
		Warning("GAME EFFECTS: Could not load active effect with actor effect %08X and effect index %i", apFormID, v4);
		return nullptr;
	}
}

void ActiveEffect::InitLoadActiveEffectList(BGSLoadFormBuffer* apLoadGameBuffer,
	BSSimpleList<ActiveEffect*>* apActiveEffectList) {
	while (apActiveEffectList) {
		ActiveEffect* pactiveEffect = apActiveEffectList->m_item;
		if (pactiveEffect) {
			pactiveEffect->InitLoadGame(apLoadGameBuffer);
		}
		apActiveEffectList = apActiveEffectList->m_pkNext;
	}
}

void ActiveEffect::SaveActiveEffect(BGSSaveFormBuffer* apSaveGameBuffer, ActiveEffect* apEffect) {
	uint32_t aiFormID;
	char apData;
	if (apEffect->pSpell) {
		aiFormID = apEffect->pSpell->GetMagicItemFormID();
	} else {
		aiFormID = 0;
	}
	apSaveGameBuffer->SaveFormID(aiFormID, 0);
	apData = apEffect->pSpell->GetEffectItemIndex(apEffect->pEffect);
	apSaveGameBuffer->SaveDataEndian(&apData, 1u, 0);
	uint32_t aiVariableValue = apEffect->pEffect->pEffectSetting->GetEffectArchetype();
	apSaveGameBuffer->SaveVariableSizedValue(aiVariableValue);
	uint32_t aiPosition = apSaveGameBuffer->StartVariableSizedValue();
	int32_t CastingType = apSaveGameBuffer->iBufferPosition;
	apEffect->SaveGame(apSaveGameBuffer);
	int32_t v3 = apSaveGameBuffer->iBufferPosition;
	apSaveGameBuffer->SaveVariableSizedValue(v3 - CastingType, aiPosition);
}
